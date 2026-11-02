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

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Settings.h"

// Audio Interface window (Main menu, Tools): the audio input the sound of the piano comes
// from (the USB audio interface of the piano), for the audio recording. The device setup
// of JUCE (device type, device, input channels, sample rate, buffer size), the level of
// the left and right input and the input latency the driver reports. The device is open
// only while the window is shown (so it does not take the device, e.g. an ASIO driver,
// from other programs); its setup is saved in the settings. The MIDI connections of the
// program do not use this device manager.
class AudioInputComponent : public Component,
	private AudioIODeviceCallback,
	private ChangeListener,
	private Timer
{
public:
	explicit AudioInputComponent(Settings& settings);
	~AudioInputComponent() override;

	// Opens the chosen device (the window is shown) or closes it (the window is hidden).
	void SetActive(bool shouldBeActive);

	void paint(Graphics& g) override;
	void resized() override;
	void childBoundsChanged(Component* child) override;

private:
	static const int NumMeters = 2; // left and right
	static const int Width = 520;
	static const int MeterRowHeight = 22;
	static const int MeterAreaHeight = 4 * MeterRowHeight; // two meters, latency, space

	Settings& settings;
	AudioDeviceManager deviceManager;
	std::unique_ptr<AudioDeviceSelectorComponent> selector;
	bool active = false;
	// the highest level since the last timer call, written by the audio thread
	std::atomic<float> peaks[NumMeters];
	float levels[NumMeters] = {}; // the level shown, falling slowly
	String latencyText;

	void audioDeviceIOCallbackWithContext(const float* const* inputChannelData, int numInputChannels,
		float* const* outputChannelData, int numOutputChannels, int numSamples,
		const AudioIODeviceCallbackContext& context) override;
	void audioDeviceAboutToStart(AudioIODevice* device) override;
	void audioDeviceStopped() override;
	void audioDeviceError(const String& errorMessage) override;
	void changeListenerCallback(ChangeBroadcaster* source) override;
	void timerCallback() override;

	void LogDeviceTypes();
	void LogCurrentDevice();
	void UpdateLatency();
	void SaveState();
	int GetMeterTop() const;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioInputComponent)
};

// The Audio Interface window: not modal and always on top, like the other windows.
// Closing only hides it (and closes the device).
class AudioInputWindow : public DocumentWindow
{
public:
	explicit AudioInputWindow(Settings& settings);
	void closeButtonPressed() override;
	void visibilityChanged() override;
	void moved() override;
	// Moves the window where it was the last time; false if that is not known (or not
	// on a screen any more).
	bool RestorePosition();

private:
	Settings& settings;
	AudioInputComponent* content = nullptr; // owned by the window
};
