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
#include "AudioCapture.h"
#include "Settings.h"

// The audio input of the program: the device chosen in the Audio Interface window (the USB
// audio interface of the piano), shared by that window and by the audio recording. An
// ASIO device can be opened once only, so both use this one device manager; the device is
// open only while somebody uses it (Acquire, Release), so it is not taken from other
// programs. The levels of the input for the meters, and the recording itself (see
// AudioCapture). The MIDI connections of the program do not use this device manager.
class AudioRecorder : private AudioIODeviceCallback
{
public:
	explicit AudioRecorder(Settings& settings);
	~AudioRecorder() override;

	static const int NumChannels = AudioCapture::NumChannels;
	// The meters: each one takes the peaks on its own.
	enum Meter { meterInterfaceWindow, meterRecorderWindow, NumMeters };

	AudioDeviceManager& GetDeviceManager() { return deviceManager; }
	// Opens the device chosen the last time, if nobody uses it yet; every Acquire needs a
	// Release. If that device cannot be opened (e.g. the piano is switched off), no other one
	// is opened instead: the sound of another input (a microphone) must not be recorded as
	// the piano. Only without a chosen device is the default input of the system opened.
	void Acquire();
	void Release();
	bool IsOpen() const;          // a device with an input is open
	bool HasChosenDevice() const; // a device was chosen in the Audio Interface window
	void SaveState();             // the chosen device and its setup, into the settings
	// The input latency the driver reports, in samples; the sample rate (0: no device).
	int GetInputLatencySamples() const;
	double GetSampleRate() const;
	// The highest level of a channel since this meter asked the last time.
	float TakePeak(Meter meter, int channel);

	AudioCapture& GetCapture() { return capture; }

	// A level meter bar, the same in every window: from -60 dBFS to 0, green up to -12 dBFS,
	// yellow up to -3 dBFS, red above it (the signal is too high). level: 0..1.
	static void DrawLevelBar(Graphics& g, Rectangle<float> bar, float level);
	// The LAME encoder for the MP3 files (lame.exe): beside the program, or in the Lame
	// folder of the data folder of the program; a file that does not exist if it is not there.
	static File FindLame();
	static File GetLameFolder(); // the folder in the data folder

private:
	Settings& settings;
	AudioDeviceManager deviceManager;
	AudioCapture capture;
	int users = 0;
	std::atomic<float> peaks[NumMeters][NumChannels];

	void audioDeviceIOCallbackWithContext(const float* const* inputChannelData, int numInputChannels,
		float* const* outputChannelData, int numOutputChannels, int numSamples,
		const AudioIODeviceCallbackContext& context) override;
	void audioDeviceAboutToStart(AudioIODevice* device) override;
	void audioDeviceStopped() override;
	void audioDeviceError(const String& errorMessage) override;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioRecorder)
};
