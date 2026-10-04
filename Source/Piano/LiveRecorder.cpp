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

#include "LiveRecorder.h"
#include <map>

static double NowMs()
{
	return Time::getMillisecondCounterHiRes();
}

bool LiveRecorder::IsRecordable(const MidiMessage& message)
{
	return message.isNoteOnOrOff() || message.isController() || message.isPitchWheel() ||
		message.isProgramChange() || message.isChannelPressure() || message.isAftertouch();
}

// alignToDownbeat: if the beats are running (the last beat was a short time ago), the
// recording begins at the last downbeat instead of now.
void LiveRecorder::Begin(double nowMs, bool alignToDownbeat)
{
	m_events.clear();
	m_noteCount = 0;
	m_saved = false;
	m_startMs = nowMs;
	if (alignToDownbeat && m_lastDownbeatMs > 0 && nowMs - m_lastBeatMs < 2500 &&
		nowMs - m_lastDownbeatMs < 15000)
	{
		m_startMs = m_lastDownbeatMs;
	}
	m_lastEventMs = nowMs;
	m_endMs = nowMs;
	for (auto& source : m_held)
	{
		std::fill(std::begin(source), std::end(source), 0);
	}
	std::fill(std::begin(m_pedal), std::end(m_pedal), 0);
	m_state = stRecording;
}

void LiveRecorder::Arm()
{
	const ScopedLock lock(m_lock);
	m_events.clear();
	m_noteCount = 0;
	m_state = stArmed;
}

void LiveRecorder::Start()
{
	const ScopedLock lock(m_lock);
	Begin(NowMs(), true);
}

void LiveRecorder::StartCountIn(int measures)
{
	const ScopedLock lock(m_lock);
	m_events.clear();
	m_noteCount = 0;
	// the next downbeat begins the count-in; the recording starts after its measures
	m_downbeatsLeft = std::max(0, measures) + 1;
	m_state = stCountIn;
}

int LiveRecorder::GetCountInMeasuresLeft() const
{
	const ScopedLock lock(m_lock);
	return m_state == stCountIn ? m_downbeatsLeft : 0;
}

void LiveRecorder::Beat(bool downbeat)
{
	const double nowMs = NowMs();
	const ScopedLock lock(m_lock);
	m_lastBeatMs = nowMs;
	if (!downbeat || nowMs - m_lastDownbeatMs < 100)
	{
		return; // not a downbeat, or the same downbeat reported twice
	}
	m_lastDownbeatMs = nowMs;
	if (m_state == stCountIn)
	{
		if (--m_downbeatsLeft <= 0)
		{
			Begin(nowMs, false);
		}
	}
	else if (m_state == stRecording && m_noteCount == 0)
	{
		// nothing was played yet: the empty measures at the beginning are left out
		MoveStart(nowMs);
	}
}

// Moves the beginning of the recording later; what was recorded before it (e.g. the
// pedal) is kept at the beginning.
void LiveRecorder::MoveStart(double newStartMs)
{
	const double delta = newStartMs - m_startMs;
	for (Event& event : m_events)
	{
		event.time = std::max(0.0, event.time - delta);
	}
	m_startMs = newStartMs;
}

void LiveRecorder::Stop()
{
	const ScopedLock lock(m_lock);
	if (m_state == stRecording)
	{
		// the end is the last played event, not the moment of stopping
		const double endMs = m_lastEventMs;
		for (int source = 1; source <= NumSources; source++)
		{
			for (int note = 0; note < 128; note++)
			{
				if (m_held[source][note] > 0)
				{
					m_events.push_back({endMs - m_startMs, source, MidiMessage::noteOff(1, note)});
					m_held[source][note] = 0;
				}
			}
			if (m_pedal[source] > 0)
			{
				m_events.push_back({endMs - m_startMs, source, MidiMessage::controllerEvent(1, 64, 0)});
				m_pedal[source] = 0;
			}
		}
		m_endMs = endMs;
	}
	m_state = stIdle;
}

void LiveRecorder::Clear()
{
	const ScopedLock lock(m_lock);
	m_events.clear();
	m_noteCount = 0;
	m_state = stIdle;
}

LiveRecorder::State LiveRecorder::GetState() const
{
	const ScopedLock lock(m_lock);
	return m_state;
}

void LiveRecorder::SetSetup(int source, const Setup& setup)
{
	if (source >= 1 && source <= NumSources)
	{
		const ScopedLock lock(m_lock);
		m_setup[source] = setup;
	}
}

void LiveRecorder::Store(int source, const MidiMessage& message, double nowMs)
{
	MidiMessage copy(message);
	copy.setChannel(1); // the channel is assigned when the file is written
	copy.setTimeStamp(0);
	if (message.isNoteOn() && m_noteCount == 0 &&
		!(m_lastDownbeatMs > 0 && nowMs - m_lastBeatMs < 2500))
	{
		// no beats to keep in step with: the recording begins with its first note, the
		// silence before it is left out
		MoveStart(nowMs);
	}
	m_events.push_back({nowMs - m_startMs, source, copy});
	m_lastEventMs = nowMs;

	if (message.isNoteOn())
	{
		m_held[source][message.getNoteNumber()]++;
		m_noteCount++;
	}
	else if (message.isNoteOff())
	{
		int& held = m_held[source][message.getNoteNumber()];
		held = std::max(0, held - 1);
	}
	else if (message.isControllerOfType(64))
	{
		m_pedal[source] = message.getControllerValue();
	}
}

void LiveRecorder::Add(int source, const MidiMessage& message)
{
	if (source < 1 || source > NumSources || !IsRecordable(message))
	{
		return;
	}

	const double nowMs = NowMs();
	const ScopedLock lock(m_lock);

	if ((m_state == stArmed || m_state == stCountIn) && message.isNoteOn())
	{
		// The first note starts the recording. Also during the count-in: a note played
		// before its end is not lost, the recording begins with the measure of the note
		// (e.g. the metronome was already running and the player starts on the next bell).
		Begin(nowMs, true);
	}
	if (m_state != stRecording)
	{
		if (message.isControllerOfType(64))
		{
			m_pedal[source] = 0; // not recorded
		}
		return;
	}
	if (message.isNoteOff() && m_held[source][message.getNoteNumber()] == 0)
	{
		return; // the note was started before the recording
	}
	Store(source, message, nowMs);
}

void LiveRecorder::AddStyle(const MidiMessage& message)
{
	if (message.isSysEx())
	{
		// XG part mode of an accompaniment channel: F0 43 1n 4C 08 pp 07 mm F7
		const uint8* data = message.getSysExData();
		if (message.getSysExDataSize() == 7 && data[0] == 0x43 && (data[1] & 0xf0) == 0x10 &&
			data[2] == 0x4c && data[3] == 0x08 && data[4] >= 8 && data[4] <= 15 && data[5] == 0x07)
		{
			const ScopedLock lock(m_lock);
			m_styleSetup[data[4] - 8].partMode = data[6];
		}
		return;
	}

	const int channel = message.getChannel();
	if (channel < 9 || channel > 16 || !IsRecordable(message))
	{
		return;
	}
	const int index = channel - 9;
	const double nowMs = NowMs();
	const ScopedLock lock(m_lock);

	// the settings of the channel: remembered, not recorded
	if (message.isProgramChange())
	{
		m_styleSetup[index].voice = (m_styleBankMsb[index] << 16) | (m_styleBankLsb[index] << 8) |
			message.getProgramChangeNumber();
		return;
	}
	if (message.isController())
	{
		const int value = message.getControllerValue();
		switch (message.getControllerNumber())
		{
			case 0: m_styleBankMsb[index] = value; return;
			case 32: m_styleBankLsb[index] = value; return;
			case 7: m_styleSetup[index].volume = value; return;
			case 10: m_styleSetup[index].pan = value; return;
			case 91: m_styleSetup[index].reverb = value; return;
		}
	}

	// the accompaniment does not start the recording, it is only recorded with it
	if (m_state != stRecording)
	{
		return;
	}
	const int source = srcStyle9 + index;
	if (message.isNoteOff() && m_held[source][message.getNoteNumber()] == 0)
	{
		return;
	}
	const double lastEventMs = m_lastEventMs;
	Store(source, message, nowMs);
	m_lastEventMs = lastEventMs; // the automatic stop waits for the player, not for the style
}

bool LiveRecorder::HasData() const
{
	const ScopedLock lock(m_lock);
	return m_state == stIdle && m_noteCount > 0;
}

bool LiveRecorder::IsUnsaved() const
{
	const ScopedLock lock(m_lock);
	return m_state == stRecording || (m_state == stIdle && m_noteCount > 0 && !m_saved);
}

void LiveRecorder::MarkSaved()
{
	const ScopedLock lock(m_lock);
	m_saved = true;
}

int LiveRecorder::GetNoteCount() const
{
	const ScopedLock lock(m_lock);
	return m_noteCount;
}

double LiveRecorder::GetLengthSeconds() const
{
	const ScopedLock lock(m_lock);
	if (m_state == stRecording)
	{
		return (NowMs() - m_startMs) / 1000.0;
	}
	return m_noteCount > 0 ? (m_endMs - m_startMs) / 1000.0 : 0.0;
}

double LiveRecorder::GetSecondsSinceLastEvent() const
{
	const ScopedLock lock(m_lock);
	return m_state == stRecording ? (NowMs() - m_lastEventMs) / 1000.0 : 0.0;
}

int LiveRecorder::GetHeldNoteCount() const
{
	const ScopedLock lock(m_lock);
	int count = 0;
	for (int source = 1; source < srcStyle9; source++) // not the accompaniment
	{
		for (int note = 0; note < 128; note++)
		{
			count += m_held[source][note] > 0 ? 1 : 0;
		}
		count += m_pedal[source] > 0 ? 1 : 0;
	}
	return count;
}

// Writes the events of a source into the track with the notes moved to the grid.
// Returns the time of the last event (ticks).
double LiveRecorder::AddQuantized(MidiMessageSequence& track, int source, int channel,
	double ticksPerMs, int grid, bool quantizeEnds, bool triplets, double measureTicks) const
{
	struct Note
	{
		double on;
		double off;
		MidiMessage onMessage;
		MidiMessage offMessage;
		bool dropped;
	};
	struct Out
	{
		double tick;
		int kind; // at the same time: note offs, then the other messages, then note ons
		MidiMessage message;
	};

	std::vector<Note> notes;
	std::vector<Out> out;
	std::vector<size_t> open[128]; // notes not ended yet, by note number

	for (const Event& event : m_events)
	{
		if (event.source != source)
		{
			continue;
		}
		MidiMessage message(event.message);
		message.setChannel(channel);
		message.setTimeStamp(0); // see Save
		const double tick = std::floor(std::max(0.0, event.time) * ticksPerMs + 0.5);
		if (message.isNoteOn())
		{
			const int number = message.getNoteNumber();
			open[number].push_back(notes.size());
			notes.push_back({tick, tick + grid, message, MidiMessage::noteOff(channel, number), false});
		}
		else if (message.isNoteOff())
		{
			std::vector<size_t>& list = open[message.getNoteNumber()];
			if (!list.empty())
			{
				Note& note = notes[list.front()];
				note.off = tick;
				note.offMessage = message;
				list.erase(list.begin());
			}
		}
		else
		{
			out.push_back({tick, 1, message});
		}
	}

	// The measure is divided into windows of two grid steps; in a window the notes are
	// moved either to the grid or to its triplets (three steps in the window). A note
	// played a little before a window belongs to that window.
	const double window = grid * 2.0;
	const double tripletGrid = window / 3.0;
	const double early = tripletGrid / 2.0;
	struct Place
	{
		long long key;  // identifies the window
		double start;   // beginning of the window
		double length;  // the last window of a measure can be shorter
		double offset;  // position in the window (negative: a little before it)
	};
	auto placeOf = [=](double tick)
		{
			const double shifted = tick + early;
			const double measure = std::floor(shifted / measureTicks);
			const double inMeasure = shifted - measure * measureTicks;
			const double index = std::floor(inMeasure / window);
			const double start = measure * measureTicks + index * window;
			return Place{(long long)measure * 4096 + (long long)index, start,
				std::min(window, measureTicks - index * window), tick - start};
		};
	auto snap = [](const Place& place, double step)
		{
			double offset = std::floor(place.offset / step + 0.5) * step;
			// the end of the window (the beginning of the next one) is a grid point too
			if (offset > place.length || std::abs(place.offset - place.length) < std::abs(place.offset - offset))
			{
				offset = place.length;
			}
			return place.start + offset;
		};

	// which windows are played in triplets: where the notes are clearly nearer to the
	// triplet grid (in doubt the plain grid is kept)
	std::map<long long, bool> tripletWindows;
	if (triplets)
	{
		std::map<long long, std::pair<double, double>> errors; // plain, triplet
		for (const Note& note : notes)
		{
			const Place place = placeOf(note.on);
			std::pair<double, double>& error = errors[place.key];
			error.first += std::abs(note.on - snap(place, grid));
			error.second += std::abs(note.on - snap(place, tripletGrid));
		}
		for (const auto& item : errors)
		{
			tripletWindows[item.first] = item.second.second < item.second.first * 0.8;
		}
	}
	auto stepOf = [&](const Place& place)
		{
			const auto found = tripletWindows.find(place.key);
			return found != tripletWindows.end() && found->second ? tripletGrid : (double)grid;
		};

	for (Note& note : notes)
	{
		const double length = std::max(1.0, note.off - note.on);
		const Place onPlace = placeOf(note.on);
		const double onStep = stepOf(onPlace);
		note.on = std::max(0.0, snap(onPlace, onStep));
		if (quantizeEnds)
		{
			// a note is at least one grid step long if its end is moved to the grid too
			const Place offPlace = placeOf(note.off);
			note.off = std::max(note.on + onStep, snap(offPlace, stepOf(offPlace)));
		}
		else
		{
			note.off = note.on + length;
		}
	}

	// notes of the same pitch must not overlap: two notes moved to the same time become
	// one note, and a note ends when the next one of the same pitch begins
	size_t last[128];
	bool hasLast[128] = {};
	for (size_t i = 0; i < notes.size(); i++)
	{
		Note& note = notes[i];
		const int number = note.onMessage.getNoteNumber();
		if (hasLast[number])
		{
			Note& previous = notes[last[number]];
			if (note.on <= previous.on)
			{
				previous.off = std::max(previous.off, note.off);
				note.dropped = true;
				continue;
			}
			if (previous.off > note.on)
			{
				previous.off = note.on;
			}
		}
		last[number] = i;
		hasLast[number] = true;
	}

	for (const Note& note : notes)
	{
		if (!note.dropped)
		{
			out.push_back({note.on, 2, note.onMessage});
			out.push_back({note.off, 0, note.offMessage});
		}
	}
	std::stable_sort(out.begin(), out.end(), [](const Out& a, const Out& b)
		{
			return a.tick < b.tick || (a.tick == b.tick && a.kind < b.kind);
		});

	double lastTick = 0;
	for (const Out& item : out)
	{
		track.addEvent(item.message, item.tick);
		lastTick = std::max(lastTick, item.tick);
	}
	return lastTick;
}

bool LiveRecorder::Save(const File& file, int tempo, int beatNumerator, int beatDenominator,
	bool includeStyle, int reverbType, String& error, int quantizeTicks, bool quantizeEnds,
	bool quantizeTriplets) const
{
	const ScopedLock lock(m_lock);
	if (m_noteCount == 0)
	{
		error = "Nothing was recorded";
		return false;
	}

	const int ticksPerQuarter = TicksPerQuarter;
	const double ticksPerMs = tempo * ticksPerQuarter / 60000.0;

	// the sources that played notes
	bool used[NumSources + 1] = {};
	for (const Event& event : m_events)
	{
		if (event.message.isNoteOn())
		{
			used[event.source] = true;
		}
	}
	if (!includeStyle)
	{
		for (int source = srcStyle9; source <= NumSources; source++)
		{
			used[source] = false;
		}
	}

	// MIDI channel of each source: the accompaniment keeps its channels (9..16); a Mixer
	// channel keeps its number if it is free; the keyboard parts take the first free
	// channels (channel 10 is the drum channel)
	int channelOf[NumSources + 1] = {};
	bool taken[17] = {};
	for (int source = srcStyle9; source <= NumSources; source++)
	{
		if (used[source])
		{
			channelOf[source] = 9 + (source - srcStyle9);
			taken[channelOf[source]] = true;
		}
	}
	auto freeChannel = [&taken]()
		{
			for (int channel = 1; channel <= 16; channel++)
			{
				if (!taken[channel] && channel != 10)
				{
					return channel;
				}
			}
			return 0;
		};
	for (int source = srcMixer1; source < srcMain; source++)
	{
		if (used[source])
		{
			const int own = source - srcMixer1 + 1;
			const int channel = !taken[own] ? own : freeChannel();
			channelOf[source] = channel;
			taken[channel] = true;
		}
	}
	for (int source = srcMain; source <= srcLeft; source++)
	{
		if (used[source])
		{
			const int channel = freeChannel();
			channelOf[source] = channel;
			taken[channel] = true;
		}
	}
	taken[0] = false;

	MidiFile midiFile;
	midiFile.setTicksPerQuarterNote(ticksPerQuarter);

	// conductor track: tempo, beat, reverb type
	MidiMessageSequence conductor;
	{
		// written by hand: 24 MIDI clocks per metronome click and 8 thirty-second notes
		// per quarter note, the usual values (some programs do not accept others)
		int power = 0;
		while ((1 << (power + 1)) <= std::max(1, beatDenominator)) power++;
		const uint8 data[] = {0xff, 0x58, 0x04, (uint8)jlimit(1, 127, beatNumerator), (uint8)power, 24, 8};
		conductor.addEvent(MidiMessage(data, (int)sizeof(data), 0.0), 0);
	}
	conductor.addEvent(MidiMessage::tempoMetaEvent(60000000 / std::max(1, tempo)), 0);
	if (reverbType != NoValue)
	{
		const uint8 data[] = {0x43, 0x10, 0x4c, 0x02, 0x01, 0x00,
			(uint8)((reverbType >> 8) & 0x7f), (uint8)(reverbType & 0x7f)};
		conductor.addEvent(MidiMessage::createSysExMessage(data, sizeof(data)), 0);
	}
	midiFile.addTrack(conductor);

	static const char* names[] = {"", "", "", "Main", "Layer", "Left"};
	int skipped = 0;
	for (int source = 1; source <= NumSources; source++)
	{
		if (!used[source])
		{
			continue;
		}
		const int channel = channelOf[source];
		if (channel == 0)
		{
			skipped++; // no free channel
			continue;
		}

		MidiMessageSequence track;
		const String name = source < srcMain ? "Mixer " + String(source) :
			source < srcStyle9 ? String(names[source - srcMain + 3]) :
			"Style " + String(source - srcStyle9 + 9);
		track.addEvent(MidiMessage::textMetaEvent(3, name), 0);

		// voice and mixer settings
		const Setup& setup = source >= srcStyle9 ? m_styleSetup[source - srcStyle9] : m_setup[source];
		if (setup.partMode != NoValue)
		{
			const uint8 data[] = {0x43, 0x10, 0x4c, 0x08, (uint8)(channel - 1), 0x07, (uint8)setup.partMode};
			track.addEvent(MidiMessage::createSysExMessage(data, sizeof(data)), 0);
		}
		if (setup.voice != NoValue)
		{
			track.addEvent(MidiMessage::controllerEvent(channel, 0, (setup.voice >> 16) & 0x7f), 0);
			track.addEvent(MidiMessage::controllerEvent(channel, 32, (setup.voice >> 8) & 0x7f), 0);
			track.addEvent(MidiMessage::programChange(channel, setup.voice & 0x7f), 0);
		}
		if (setup.volume != NoValue) track.addEvent(MidiMessage::controllerEvent(channel, 7, jlimit(0, 127, setup.volume)), 0);
		if (setup.pan != NoValue) track.addEvent(MidiMessage::controllerEvent(channel, 10, jlimit(0, 127, setup.pan)), 0);
		if (setup.reverb != NoValue) track.addEvent(MidiMessage::controllerEvent(channel, 91, jlimit(0, 127, setup.reverb)), 0);

		double lastTick = 0;
		if (quantizeTicks > 0 && source < srcStyle9)
		{
			// the length of a measure: the quarter note is ticksPerQuarter ticks long
			const double measureTicks = std::max(1, beatNumerator) * 4.0 * ticksPerQuarter / std::max(1, beatDenominator);
			lastTick = AddQuantized(track, source, channel, ticksPerMs, quantizeTicks, quantizeEnds,
				quantizeTriplets, measureTicks);
		}
		else
		{
			for (const Event& event : m_events)
			{
				if (event.source != source)
				{
					continue;
				}
				MidiMessage message(event.message);
				message.setChannel(channel);
				// addEvent adds its time to the time stamp of the message: messages coming from a
				// MIDI input carry the time of their arrival, which must not be added
				message.setTimeStamp(0);
				const double tick = std::floor(std::max(0.0, event.time) * ticksPerMs + 0.5);
				track.addEvent(message, tick);
				lastTick = std::max(lastTick, tick);
			}
		}
		track.addEvent(MidiMessage::endOfTrack(), lastTick + 1);
		track.updateMatchedPairs();
		midiFile.addTrack(track);
	}

	file.getParentDirectory().createDirectory();
	file.deleteFile();
	FileOutputStream stream(file);
	if (!stream.openedOk() || !midiFile.writeTo(stream, 1))
	{
		error = "Cannot write the file";
		return false;
	}
	stream.flush();
	if (skipped > 0)
	{
		error = "Not all parts fit into the 16 MIDI channels";
	}
	return true;
}
