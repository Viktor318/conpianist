/*
 *  This file is part of ConPianist. See <https://github.com/Viktor318/conpianist>.
 *
 *  Copyright (C) 2026 Viktor Oszkó <oszko.viktor@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "AudioInputComponent.h"

namespace
{
	const Colour BackgroundColour(0xff323e44);
	const float FallFactor = 0.85f;   // the level shown falls by this per timer call (30 Hz)
	const int LevelRows = 3;          // "Level", left, right

	// The bottom of the lowest control (combo box, button, list) inside the component, in
	// its coordinates: the device setup leaves empty space under its controls.
	int LowestControlBottom(const Component& parent)
	{
		int bottom = 0;
		for (Component* child : parent.getChildren())
		{
			if (!child->isVisible())
			{
				continue;
			}
			const bool control = dynamic_cast<ComboBox*>(child) != nullptr || dynamic_cast<Button*>(child) != nullptr ||
				dynamic_cast<ListBox*>(child) != nullptr;
			bottom = jmax(bottom, control ? child->getBottom() : child->getY() + LowestControlBottom(*child));
		}
		return bottom;
	}
}

AudioInputComponent::AudioInputComponent(Settings& settings, AudioRecorder& audioRecorder) :
	settings(settings),
	audioRecorder(audioRecorder),
	deviceManager(audioRecorder.GetDeviceManager())
{
	// input only: up to two channels (the piano sends stereo), no output, no MIDI
	selector = std::make_unique<AudioDeviceSelectorComponent>(deviceManager, 1, NumMeters, 0, 0,
		false, false, true, false);
	selector->setSize(Width, 200);
	addAndMakeVisible(selector.get());
	setSize(Width, selector->getHeight() + MeterAreaHeight);
}

AudioInputComponent::~AudioInputComponent()
{
	SetActive(false);
	selector = nullptr; // before the device manager
}

void AudioInputComponent::SetActive(bool shouldBeActive)
{
	if (shouldBeActive == active)
	{
		return;
	}
	active = shouldBeActive;

	if (active)
	{
		LogDeviceTypes();
		audioRecorder.Acquire();
		deviceManager.addChangeListener(this);
		LogCurrentDevice();
		UpdateLatency();
		startTimerHz(30);
	}
	else
	{
		stopTimer();
		deviceManager.removeChangeListener(this);
		audioRecorder.Release(); // (closes the device if the recording does not use it)
		for (int ch = 0; ch < NumMeters; ch++)
		{
			levels[ch] = 0.0f;
		}
		repaint();
	}
}

// The top of the level rows ("Level", left, right): in the middle between the lowest
// control of the device setup (its buttons, if it has any) and the latency row.
int AudioInputComponent::GetMeterTop() const
{
	const int contentBottom = selector->getY() + LowestControlBottom(*selector);
	const int latencyTop = getHeight() - MeterRowHeight * 3 / 2;
	return jmax(contentBottom + 4, contentBottom + (latencyTop - contentBottom - LevelRows * MeterRowHeight) / 2);
}

void AudioInputComponent::paint(Graphics& g)
{
	g.fillAll(BackgroundColour);

	// Every row begins where the controls of the device setup begin (as JUCE places them);
	// the bars are as wide as they were beside the dBFS values.
	const int controlsX = proportionOfWidth(0.35f);
	const int right = proportionOfWidth(0.95f);
	const int channelWidth = 16;
	const int barLeft = controlsX + channelWidth + 3;
	const int barRight = right - 86;
	int y = GetMeterTop();

	g.setFont(Font(FontOptions(15.0f)));
	g.setColour(Colours::white);
	g.drawText(TRANS("Level"), controlsX, y, right - controlsX, MeterRowHeight, Justification::centredLeft);
	if (recording)
	{
		// (in the row of the label, after it: the window keeps its height)
		const int x = controlsX + GlyphArrangement::getStringWidthInt(g.getCurrentFont(), TRANS("Level")) + 12;
		g.setColour(Colours::orange);
		g.drawFittedText(TRANS("The device cannot be changed during a recording"), x, y, right - x, MeterRowHeight,
			Justification::centredLeft, 1, 0.8f);
		g.setColour(Colours::white);
	}
	y += MeterRowHeight;
	const String channelNames[NumMeters] = {TRANS("L"), TRANS("R")};
	for (int ch = 0; ch < NumMeters; ch++)
	{
		g.setColour(Colours::white);
		g.drawText(channelNames[ch], controlsX, y, channelWidth, MeterRowHeight, Justification::centredLeft);
		const Rectangle<float> bar((float)barLeft, (float)(y + 6), (float)(barRight - barLeft), (float)(MeterRowHeight - 12));
		AudioRecorder::DrawLevelBar(g, bar, levels[ch]);
		y += MeterRowHeight;
	}

	g.setColour(Colours::white);
	g.drawText(TRANS("Latency") + " " + latencyText, controlsX, getHeight() - MeterRowHeight * 3 / 2, right - controlsX,
		MeterRowHeight, Justification::centredLeft);
}

void AudioInputComponent::resized()
{
	selector->setBounds(0, 0, getWidth(), selector->getHeight());
}

// The device setup changes its height (e.g. when the device type changes): the window
// follows it.
void AudioInputComponent::childBoundsChanged(Component* child)
{
	if (child == selector.get())
	{
		// (taller if the level rows would not fit between the controls and the latency row)
		const int rowsBottom = selector->getY() + LowestControlBottom(*selector) + 4 + LevelRows * MeterRowHeight + 4;
		setSize(Width, jmax(selector->getBottom() + MeterAreaHeight, rowsBottom + MeterRowHeight * 3 / 2));
	}
}

// (message thread) another device or setup was chosen
void AudioInputComponent::changeListenerCallback(ChangeBroadcaster*)
{
	LogCurrentDevice();
	UpdateLatency();
	audioRecorder.SaveState();
	repaint();
}

void AudioInputComponent::timerCallback()
{
	// While audio is recorded (from waiting for the first note to the end of the audio),
	// the device setup is disabled: changing it would break the recording.
	const AudioCapture::State state = audioRecorder.GetCapture().GetState();
	const bool nowRecording = state == AudioCapture::stArmed || state == AudioCapture::stWriting || state == AudioCapture::stTail;
	if (nowRecording != recording)
	{
		recording = nowRecording;
		selector->setEnabled(!recording);
		repaint();
	}

	bool changed = false;
	for (int ch = 0; ch < NumMeters; ch++)
	{
		const float peak = audioRecorder.TakePeak(AudioRecorder::meterInterfaceWindow, ch);
		float level = jmax(peak, levels[ch] * FallFactor);
		if (level < 0.0001f)
		{
			level = 0.0f;
		}
		changed = changed || std::abs(level - levels[ch]) > 0.0001f;
		levels[ch] = level;
	}
	if (changed)
	{
		repaint(0, GetMeterTop(), getWidth(), LevelRows * MeterRowHeight);
	}
}

// The latency the driver reports for the input (for placing the audio recording beside
// the MIDI recording).
void AudioInputComponent::UpdateLatency()
{
	AudioIODevice* device = deviceManager.getCurrentAudioDevice();
	if (device == nullptr || device->getCurrentSampleRate() <= 0.0)
	{
		latencyText = TRANS("no device is open");
		return;
	}
	const int samples = device->getInputLatencyInSamples();
	const int ms = roundToInt(samples * 1000.0 / device->getCurrentSampleRate());
	latencyText = TRANS("input NUMBER ms").replace("NUMBER", String(ms)) + " (" +
		TRANS("NUMBER samples").replace("NUMBER", String(samples)) + ")";
}

// The device types and their input devices, in the log (which driver sees the piano).
void AudioInputComponent::LogDeviceTypes()
{
	for (AudioIODeviceType* type : deviceManager.getAvailableDeviceTypes())
	{
		type->scanForDevices();
		const StringArray names = type->getDeviceNames(true);
		Logger::writeToLog("Audio input: " + type->getTypeName() + ": " +
			(names.isEmpty() ? String("(no input devices)") : names.joinIntoString(", ")));
	}
}

void AudioInputComponent::LogCurrentDevice()
{
	AudioIODevice* device = deviceManager.getCurrentAudioDevice();
	if (device == nullptr)
	{
		Logger::writeToLog("Audio input: no device is open");
		return;
	}
	const double sampleRate = device->getCurrentSampleRate();
	const int latency = device->getInputLatencyInSamples();
	Logger::writeToLog("Audio input: " + device->getTypeName() + ", " + device->getName() +
		", " + String(device->getActiveInputChannels().countNumberOfSetBits()) + " input channels, " +
		String(sampleRate, 0) + " Hz, buffer " + String(device->getCurrentBufferSizeSamples()) +
		" samples, input latency " + String(latency) + " samples (" +
		String(sampleRate > 0.0 ? latency * 1000.0 / sampleRate : 0.0, 1) + " ms)");
}

//==============================================================================
AudioInputWindow::AudioInputWindow(Settings& settings, AudioRecorder& audioRecorder) :
	DocumentWindow(TRANS("Audio Interface"), BackgroundColour,
		DocumentWindow::minimiseButton | DocumentWindow::closeButton),
	settings(settings)
{
	const bool usingNativeTitleBar = (SystemStats::getOperatingSystemType() & SystemStats::Windows) ||
		(SystemStats::getOperatingSystemType() & SystemStats::MacOSX);
	setUsingNativeTitleBar(usingNativeTitleBar);
	content = new AudioInputComponent(settings, audioRecorder);
	setContentOwned(content, true);
	setResizable(false, false);
	setAlwaysOnTop(true);
}

void AudioInputWindow::closeButtonPressed()
{
	setVisible(false);
}

// the device is open only while the window is shown
void AudioInputWindow::visibilityChanged()
{
	DocumentWindow::visibilityChanged();
	if (content != nullptr)
	{
		content->SetActive(isVisible());
	}
}

// The position is remembered (saved with the settings when the program exits).
void AudioInputWindow::moved()
{
	DocumentWindow::moved();
	if (isShowing() && !isMinimised() && getX() > -10000 && getY() > -10000)
	{
		settings.audioInputWindowPos = getPosition();
	}
}

bool AudioInputWindow::RestorePosition()
{
	if (!Settings::IsWindowPosUsable(settings.audioInputWindowPos, getWidth()))
	{
		return false;
	}
	setTopLeftPosition(settings.audioInputWindowPos);
	return true;
}
