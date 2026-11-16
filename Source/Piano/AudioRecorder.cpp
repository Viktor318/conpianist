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


#include "AudioRecorder.h"

AudioRecorder::AudioRecorder(Settings& settings) :
	settings(settings)
{
	for (auto& meter : peaks)
	{
		for (auto& peak : meter)
		{
			peak = 0.0f;
		}
	}
}

AudioRecorder::~AudioRecorder()
{
	capture.Clear();
	if (users > 0)
	{
		deviceManager.removeAudioCallback(this);
		deviceManager.closeAudioDevice();
	}
}

void AudioRecorder::Acquire()
{
	if (users++ > 0)
	{
		return;
	}
	// the device chosen the last time; without it (or if it is not available) the default
	// input of the system
	std::unique_ptr<XmlElement> state = parseXML(settings.audioState);
	const String error = deviceManager.initialise(NumChannels, 0, state.get(), true);
	if (error.isNotEmpty())
	{
		Logger::writeToLog("Audio input: " + error);
	}
	deviceManager.addAudioCallback(this);
}

void AudioRecorder::Release()
{
	if (users <= 0 || --users > 0)
	{
		return;
	}
	deviceManager.removeAudioCallback(this);
	SaveState();
	deviceManager.closeAudioDevice();
	Logger::writeToLog("Audio input: closed");
	for (auto& meter : peaks)
	{
		for (auto& peak : meter)
		{
			peak = 0.0f;
		}
	}
}

bool AudioRecorder::IsOpen() const
{
	AudioIODevice* device = deviceManager.getCurrentAudioDevice();
	return users > 0 && device != nullptr && device->isOpen() &&
		device->getActiveInputChannels().countNumberOfSetBits() > 0;
}

bool AudioRecorder::HasChosenDevice() const
{
	return settings.audioState.isNotEmpty();
}

// The chosen device and setup, kept in the settings (saved when the program exits).
void AudioRecorder::SaveState()
{
	if (std::unique_ptr<XmlElement> state = deviceManager.createStateXml())
	{
		settings.audioState = state->toString(XmlElement::TextFormat().singleLine().withoutHeader());
	}
}

int AudioRecorder::GetInputLatencySamples() const
{
	AudioIODevice* device = deviceManager.getCurrentAudioDevice();
	return device != nullptr ? device->getInputLatencyInSamples() : 0;
}

double AudioRecorder::GetSampleRate() const
{
	AudioIODevice* device = deviceManager.getCurrentAudioDevice();
	return device != nullptr ? device->getCurrentSampleRate() : 0.0;
}

float AudioRecorder::TakePeak(Meter meter, int channel)
{
	return peaks[meter][channel].exchange(0.0f);
}

// (audio thread) the levels for the meters, and the samples for the recording
void AudioRecorder::audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
	int numInputChannels, float* const* outputChannelData, int numOutputChannels, int numSamples,
	const AudioIODeviceCallbackContext&)
{
	const double nowMs = Time::getMillisecondCounterHiRes();
	for (int ch = 0; ch < jmin(numInputChannels, NumChannels); ch++)
	{
		if (inputChannelData[ch] == nullptr)
		{
			continue;
		}
		const Range<float> range = FloatVectorOperations::findMinAndMax(inputChannelData[ch], numSamples);
		const float peak = jmax(std::abs(range.getStart()), std::abs(range.getEnd()));
		for (auto& meter : peaks)
		{
			float previous = meter[ch].load();
			while (peak > previous && !meter[ch].compare_exchange_weak(previous, peak))
			{
			}
		}
	}
	if (numInputChannels > 0)
	{
		capture.Feed(inputChannelData, numInputChannels, numSamples, nowMs);
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

void AudioRecorder::audioDeviceAboutToStart(AudioIODevice* device)
{
	capture.Prepare(device->getCurrentSampleRate(), device->getInputLatencyInSamples());
}

void AudioRecorder::audioDeviceStopped()
{
	capture.DeviceStopped();
}

void AudioRecorder::audioDeviceError(const String& errorMessage)
{
	Logger::writeToLog("Audio input: error: " + errorMessage);
}
