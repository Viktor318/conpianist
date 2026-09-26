/*
 *  This file is part of ConPianist. See <https://github.com/hugbug/conpianist>.
 *
 *  Copyright (C) 2018 Andrey Prygunkov <hugbug@users.sourceforge.net>
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

#include "../JuceLibraryCode/JuceHeader.h"

#include "LocalMidiConnector.h"

LocalMidiConnector::LocalMidiConnector(AudioDeviceManager* audioDeviceManager)
	: m_audioDeviceManager(audioDeviceManager)
{
	m_audioDeviceManager->addMidiInputDeviceCallback("", this);
	if (m_audioDeviceManager->getDefaultMidiOutput())
	{
		m_outputName = m_audioDeviceManager->getDefaultMidiOutput()->getName();
	}
}

LocalMidiConnector::~LocalMidiConnector()
{
	m_audioDeviceManager->removeMidiInputDeviceCallback("", this);
}

void LocalMidiConnector::handleIncomingMidiMessage(MidiInput* source, const MidiMessage& message)
{
	if (m_listener)
	{
		m_listener->IncomingMidiMessage(message);
	}
}

void LocalMidiConnector::SendMessage(const MidiMessage& message)
{
	std::lock_guard<std::mutex> guard(m_sendMutex);
	if (m_audioDeviceManager->getDefaultMidiOutput())
	{
		m_audioDeviceManager->getDefaultMidiOutput()->sendMessageNow(message);
	}
}

//==============================================================================

MidiDeviceConnector::~MidiDeviceConnector()
{
	SetPorts("", "");
}

void MidiDeviceConnector::SetPorts(const String& inputName, const String& outputName)
{
	std::unique_ptr<MidiInput> oldInput;
	std::unique_ptr<MidiOutput> oldOutput;
	{
		std::lock_guard<std::mutex> guard(m_mutex);
		if (inputName != m_inputName) oldInput = std::move(m_input);
		if (outputName != m_outputName) oldOutput = std::move(m_output);
		m_inputName = inputName;
		m_outputName = outputName;
	}
	if (oldInput) oldInput->stop();
	oldInput.reset(); // closed outside the lock: the MIDI thread may be waiting for it
	oldOutput.reset();
	Refresh();
}

void MidiDeviceConnector::Refresh()
{
	String inputName, outputName;
	bool needInput, needOutput;
	{
		std::lock_guard<std::mutex> guard(m_mutex);
		inputName = m_inputName;
		outputName = m_outputName;
		// the same port for input and output (e.g. loopMIDI) would send everything
		// back to itself endlessly, so the input is not used then
		needInput = !m_input && inputName.isNotEmpty() && inputName != outputName;
		needOutput = !m_output && outputName.isNotEmpty();
	}

	std::unique_ptr<MidiOutput> output;
	if (needOutput)
	{
		for (auto& device : MidiOutput::getAvailableDevices())
		{
			if (device.name == outputName)
			{
				output = MidiOutput::openDevice(device.identifier);
				break;
			}
		}
	}

	std::unique_ptr<MidiInput> input;
	if (needInput)
	{
		for (auto& device : MidiInput::getAvailableDevices())
		{
			if (device.name == inputName)
			{
				input = MidiInput::openDevice(device.identifier, this);
				break;
			}
		}
	}

	{
		std::lock_guard<std::mutex> guard(m_mutex);
		if (output && outputName == m_outputName && !m_output) m_output = std::move(output);
		if (input && inputName == m_inputName && !m_input)
		{
			m_input = std::move(input);
			m_input->start();
		}
	}
}

bool MidiDeviceConnector::IsOutputOpen()
{
	std::lock_guard<std::mutex> guard(m_mutex);
	return m_output != nullptr;
}

void MidiDeviceConnector::Send(const MidiMessage& message)
{
	std::lock_guard<std::mutex> guard(m_mutex);
	if (m_output)
	{
		m_output->sendMessageNow(message);
	}
}

void MidiDeviceConnector::handleIncomingMidiMessage(MidiInput*, const MidiMessage& message)
{
	if (onIncoming)
	{
		onIncoming(message);
	}
}

bool LocalMidiConnector::IsConnected()
{
	return m_audioDeviceManager->getDefaultMidiOutput() != nullptr &&
		m_audioDeviceManager->getDefaultMidiOutput()->getName() == m_outputName;
}
