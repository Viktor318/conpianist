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
	const Colour MeterBackgroundColour(0xff1b2226);
	const Colour MeterColour(0xff42a2c8);
	const Colour MeterClipColour(0xffe0503c);
	const float MinDecibels = -60.0f; // the left end of the meter
	const float ClipLevel = 0.89f;    // -1 dBFS: the meter turns red
	const float FallFactor = 0.85f;   // the level shown falls by this per timer call (30 Hz)

	// "-18" with a real minus sign
	String DecibelText(float decibels)
	{
		const int value = roundToInt(decibels);
		return value < 0 ? String(CharPointer_UTF8("\xe2\x88\x92")) + String(-value) : String(value);
	}
}

AudioInputComponent::AudioInputComponent(Settings& settings) :
	settings(settings)
{
	for (auto& peak : peaks)
	{
		peak = 0.0f;
	}
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
		// the device chosen the last time; without it (or if it is not available) the
		// default input of the system
		std::unique_ptr<XmlElement> state = parseXML(settings.audioState);
		const String error = deviceManager.initialise(NumMeters, 0, state.get(), true);
		if (error.isNotEmpty())
		{
			Logger::writeToLog("Audio input: " + error);
		}
		deviceManager.addAudioCallback(this);
		deviceManager.addChangeListener(this);
		LogCurrentDevice();
		UpdateLatency();
		startTimerHz(30);
	}
	else
	{
		stopTimer();
		deviceManager.removeChangeListener(this);
		deviceManager.removeAudioCallback(this);
		SaveState();
		deviceManager.closeAudioDevice();
		Logger::writeToLog("Audio input: closed");
		for (int ch = 0; ch < NumMeters; ch++)
		{
			peaks[ch] = 0.0f;
			levels[ch] = 0.0f;
		}
		repaint();
	}
}

// the top of the meters: under the device setup
int AudioInputComponent::GetMeterTop() const
{
	return selector->getBottom() + MeterRowHeight / 2;
}

void AudioInputComponent::paint(Graphics& g)
{
	g.fillAll(BackgroundColour);

	// the labels end where the controls of the device setup begin (as JUCE places them)
	const int controlsX = proportionOfWidth(0.35f);
	const int right = proportionOfWidth(0.95f);
	const int decibelsWidth = 80;
	const int channelWidth = 16;
	const int barLeft = controlsX + channelWidth + 3;
	const int barRight = right - decibelsWidth - 6;
	int y = GetMeterTop();

	g.setFont(Font(FontOptions(15.0f)));
	const String channelNames[NumMeters] = {TRANS("L"), TRANS("R")};
	for (int ch = 0; ch < NumMeters; ch++)
	{
		g.setColour(Colours::white);
		if (ch == 0)
		{
			g.drawText(TRANS("Level"), 0, y, controlsX - 4, MeterRowHeight, Justification::centredRight);
		}
		g.drawText(channelNames[ch], controlsX, y, channelWidth, MeterRowHeight, Justification::centredLeft);

		const float level = levels[ch];
		const float decibels = level > 0.0f ? Decibels::gainToDecibels(level) : MinDecibels - 1.0f;
		const Rectangle<float> bar((float)barLeft, (float)(y + 6), (float)(barRight - barLeft), (float)(MeterRowHeight - 12));
		g.setColour(MeterBackgroundColour);
		g.fillRect(bar);
		const float proportion = jlimit(0.0f, 1.0f, (decibels - MinDecibels) / -MinDecibels);
		g.setColour(level >= ClipLevel ? MeterClipColour : MeterColour);
		g.fillRect(bar.withWidth(bar.getWidth() * proportion));

		g.setColour(Colours::white);
		const String text = decibels < MinDecibels ?
			"< " + DecibelText(MinDecibels) + " dBFS" : DecibelText(decibels) + " dBFS";
		g.drawText(text, barRight + 6, y, decibelsWidth, MeterRowHeight, Justification::centredLeft);
		y += MeterRowHeight;
	}

	g.drawText(TRANS("Latency"), 0, y, controlsX - 4, MeterRowHeight, Justification::centredRight);
	g.drawText(latencyText, controlsX, y, right - controlsX, MeterRowHeight, Justification::centredLeft);
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
		setSize(Width, selector->getBottom() + MeterAreaHeight);
	}
}

// (audio thread) the highest level of each channel since the last timer call
void AudioInputComponent::audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
	int numInputChannels, float* const* outputChannelData, int numOutputChannels, int numSamples,
	const AudioIODeviceCallbackContext&)
{
	for (int ch = 0; ch < jmin(numInputChannels, NumMeters); ch++)
	{
		if (inputChannelData[ch] == nullptr)
		{
			continue;
		}
		const Range<float> range = FloatVectorOperations::findMinAndMax(inputChannelData[ch], numSamples);
		const float peak = jmax(std::abs(range.getStart()), std::abs(range.getEnd()));
		float previous = peaks[ch].load();
		while (peak > previous && !peaks[ch].compare_exchange_weak(previous, peak))
		{
		}
	}
	// nothing is played (the device may have outputs, e.g. an ASIO driver)
	for (int ch = 0; ch < numOutputChannels; ch++)
	{
		if (outputChannelData[ch] != nullptr)
		{
			FloatVectorOperations::clear(outputChannelData[ch], numSamples);
		}
	}
}

void AudioInputComponent::audioDeviceAboutToStart(AudioIODevice*)
{
}

void AudioInputComponent::audioDeviceStopped()
{
}

void AudioInputComponent::audioDeviceError(const String& errorMessage)
{
	Logger::writeToLog("Audio input: error: " + errorMessage);
}

// (message thread) another device or setup was chosen
void AudioInputComponent::changeListenerCallback(ChangeBroadcaster*)
{
	LogCurrentDevice();
	UpdateLatency();
	SaveState();
	repaint();
}

void AudioInputComponent::timerCallback()
{
	bool changed = false;
	for (int ch = 0; ch < NumMeters; ch++)
	{
		const float peak = peaks[ch].exchange(0.0f);
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
		repaint(0, GetMeterTop(), getWidth(), NumMeters * MeterRowHeight);
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

// The chosen device and setup, kept in the settings (saved when the program exits).
void AudioInputComponent::SaveState()
{
	if (std::unique_ptr<XmlElement> state = deviceManager.createStateXml())
	{
		settings.audioState = state->toString(XmlElement::TextFormat().singleLine().withoutHeader());
	}
}

//==============================================================================
AudioInputWindow::AudioInputWindow(Settings& settings) :
	DocumentWindow(TRANS("Audio Interface"), BackgroundColour,
		DocumentWindow::minimiseButton | DocumentWindow::closeButton),
	settings(settings)
{
	const bool usingNativeTitleBar = (SystemStats::getOperatingSystemType() & SystemStats::Windows) ||
		(SystemStats::getOperatingSystemType() & SystemStats::MacOSX);
	setUsingNativeTitleBar(usingNativeTitleBar);
	content = new AudioInputComponent(settings);
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
