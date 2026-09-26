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

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

#include "MidiConnector.h"

#include <mutex>

class LocalMidiConnector : public MidiConnector, public MidiInputCallback
{
public:
	LocalMidiConnector(AudioDeviceManager* audioDeviceManager);
	~LocalMidiConnector();
	void SendMessage(const MidiMessage& message) override;
	bool IsConnected() override;
	void handleIncomingMidiMessage(MidiInput* source, const MidiMessage& message) override;

private:
	AudioDeviceManager* m_audioDeviceManager;
	String m_outputName;
	std::mutex m_sendMutex; // messages are sent from the queue thread and the song player
};

// The additional MIDI ports, independent of the piano's port: MIDI Out (a MIDI device,
// e.g. loopMIDI to a software instrument) and MIDI In 2 (e.g. a MIDI keyboard).
class MidiDeviceConnector : private MidiInputCallback
{
public:
	~MidiDeviceConnector() override;

	// Called from the MIDI thread for every message from MIDI In 2.
	std::function<void(const MidiMessage&)> onIncoming;

	// Opens the ports by name ("" = none). A port that is not available now is
	// opened later by Refresh(), e.g. when loopMIDI is started after ConPianist.
	void SetPorts(const String& inputName, const String& outputName);
	void Refresh();
	bool IsOutputOpen();
	const String& GetOutputName() const { return m_outputName; }
	void Send(const MidiMessage& message);

private:
	std::mutex m_mutex;
	String m_inputName;
	String m_outputName;
	std::unique_ptr<MidiInput> m_input;
	std::unique_ptr<MidiOutput> m_output;

	void handleIncomingMidiMessage(MidiInput* source, const MidiMessage& message) override;
};
