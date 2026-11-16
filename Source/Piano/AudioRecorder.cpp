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
	// the device chosen the last time, and no other one if it is not available; without a
	// chosen device the default input of the system
	std::unique_ptr<XmlElement> state = parseXML(settings.audioState);
	const String error = deviceManager.initialise(NumChannels, 0, state.get(), false);
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
	// (the chosen device could not be opened: the choice is kept)
	if (deviceManager.getCurrentAudioDevice() == nullptr)
	{
		return;
	}
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

void AudioRecorder::DrawLevelBar(Graphics& g, Rectangle<float> bar, float level)
{
	const float minDecibels = -60.0f;
	g.setColour(Colour(0xff1b2226));
	g.fillRect(bar);
	if (level <= 0.0f)
	{
		return;
	}
	const float decibels = Decibels::gainToDecibels(level);
	auto position = [&](float db) { return bar.getX() + bar.getWidth() * jlimit(0.0f, 1.0f, (db - minDecibels) / -minDecibels); };
	const struct { float from, to; uint32 colour; } zones[] = {
		{minDecibels, -12.0f, 0xff50be5a}, {-12.0f, -3.0f, 0xffebc83c}, {-3.0f, 0.0f, 0xffe0503c}};
	for (const auto& zone : zones)
	{
		if (decibels > zone.from)
		{
			const float x0 = position(zone.from);
			const float x1 = position(jmin(decibels, zone.to));
			g.setColour(Colour(zone.colour));
			g.fillRect(Rectangle<float>(x0, bar.getY(), x1 - x0, bar.getHeight()));
		}
	}
}

File AudioRecorder::GetLameFolder()
{
	return File::getSpecialLocation(File::userApplicationDataDirectory).getChildFile("ConPianist").getChildFile("Lame");
}

File AudioRecorder::FindLame()
{
#if JUCE_WINDOWS
	const String name = "lame.exe";
#else
	const String name = "lame";
#endif
	const File beside = File::getSpecialLocation(File::currentExecutableFile).getSiblingFile(name);
	return beside.existsAsFile() ? beside : GetLameFolder().getChildFile(name);
}
