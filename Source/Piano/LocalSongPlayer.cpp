/*
 *  This file is part of ConPianist. See <https://github.com/Viktor318/conpianist>.
 *
 *  Copyright (C) 2026 Viktor Oszkó
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

#include "LocalSongPlayer.h"

#include <algorithm>
#include <limits>

LocalSongPlayer::~LocalSongPlayer()
{
	stopTimer();
}

//==============================================================================
// Loading

bool LocalSongPlayer::Load(const File& file)
{
	FileInputStream stream(file);
	MidiFile midiFile;
	if (!stream.openedOk() || !midiFile.readFrom(stream))
	{
		return false;
	}

	// only the usual "ticks per quarter note" format is supported (not SMPTE time)
	const int ticksPerQuarter = midiFile.getTimeFormat();
	if (ticksPerQuarter <= 0)
	{
		return false;
	}

	Unload();

	const ScopedLock lock(m_lock);

	m_ticksPerQuarter = ticksPerQuarter;
	m_events.clear();
	m_endTick = 0;

	struct Meta { int tick; double secondsPerQuarter; int numerator; int denominator; };
	std::vector<Meta> tempos;
	std::vector<Meta> signatures;

	for (int t = 0; t < midiFile.getNumTracks(); t++)
	{
		const MidiMessageSequence* track = midiFile.getTrack(t);
		for (int i = 0; i < track->getNumEvents(); i++)
		{
			const MidiMessage& msg = track->getEventPointer(i)->message;
			const int tick = roundToInt(msg.getTimeStamp());
			m_endTick = std::max(m_endTick, tick);

			if (msg.isTempoMetaEvent())
			{
				tempos.push_back({tick, msg.getTempoSecondsPerQuarterNote(), 0, 0});
			}
			else if (msg.isTimeSignatureMetaEvent())
			{
				int numerator, denominator;
				msg.getTimeSignatureInfo(numerator, denominator);
				signatures.push_back({tick, 0.0, numerator, denominator});
			}
			else if (!msg.isMetaEvent() && !msg.isSysEx() && msg.getChannel() > 0)
			{
				// System exclusive messages (e.g. GM/XG reset) are skipped on purpose:
				// they could reset the piano's own settings.
				m_events.push_back({tick, 0.0, msg, false});
			}
		}
	}

	// tempo map (default: 120 BPM)
	auto byTick = [](const Meta& a, const Meta& b) { return a.tick < b.tick; };
	std::stable_sort(tempos.begin(), tempos.end(), byTick);
	m_tempoMap.clear();
	m_tempoMap.push_back({0, 0.0, 0.5 / m_ticksPerQuarter});
	for (const Meta& tempo : tempos)
	{
		TempoPoint& last = m_tempoMap.back();
		const double secondsPerTick = tempo.secondsPerQuarter / m_ticksPerQuarter;
		if (tempo.tick == last.tick)
		{
			last.secondsPerTick = secondsPerTick;
		}
		else
		{
			const double seconds = last.seconds + (tempo.tick - last.tick) * last.secondsPerTick;
			m_tempoMap.push_back({tempo.tick, seconds, secondsPerTick});
		}
	}
	m_baseBpm = 60.0 / (m_tempoMap.front().secondsPerTick * m_ticksPerQuarter);

	// time signature map (default: 4/4); changes are assumed to start a new measure
	std::stable_sort(signatures.begin(), signatures.end(), byTick);
	m_timeSignatures.clear();
	m_timeSignatures.push_back({0, 0, 4, m_ticksPerQuarter});
	for (const Meta& signature : signatures)
	{
		TimeSignaturePoint& last = m_timeSignatures.back();
		const int ticksPerBeat = std::max(1, m_ticksPerQuarter * 4 / std::max(1, signature.denominator));
		const int numerator = std::max(1, signature.numerator);
		if (signature.tick == last.tick)
		{
			last.numerator = numerator;
			last.ticksPerBeat = ticksPerBeat;
		}
		else
		{
			const int ticksPerMeasure = last.numerator * last.ticksPerBeat;
			const int measures = (signature.tick - last.tick + ticksPerMeasure - 1) / ticksPerMeasure;
			m_timeSignatures.push_back({signature.tick, last.measure + measures, numerator, ticksPerBeat});
		}
	}

	// order at the same tick: note-offs first, then controllers/programs, then note-ons
	auto rank = [](const MidiMessage& msg) { return msg.isNoteOff() ? 0 : msg.isNoteOn() ? 2 : 1; };
	std::stable_sort(m_events.begin(), m_events.end(), [&](const Event& a, const Event& b)
		{
			return a.tick != b.tick ? a.tick < b.tick : rank(a.message) < rank(b.message);
		});

	// first note of each channel; everything before it is "setup"
	int firstNoteTick[NumChannels];
	std::fill(std::begin(firstNoteTick), std::end(firstNoteTick), std::numeric_limits<int>::max());
	for (bool& used : m_usedChannels)
	{
		used = false;
	}
	for (const Event& event : m_events)
	{
		if (event.message.isNoteOn())
		{
			const int ch = event.message.getChannel() - 1;
			firstNoteTick[ch] = std::min(firstNoteTick[ch], event.tick);
			m_usedChannels[ch] = true;
		}
	}

	for (int ch = 0; ch < NumChannels; ch++)
	{
		for (int cc = 0; cc < 128; cc++)
		{
			m_midSongController[ch][cc] = false;
			m_setupController[ch][cc] = NoValue;
		}
		m_midSongProgram[ch] = false;
		m_midSongPitchWheel[ch] = false;
		m_setupProgram[ch] = NoValue;
		m_setupPitchWheel[ch] = NoValue;
	}

	for (Event& event : m_events)
	{
		const MidiMessage& msg = event.message;
		const int ch = msg.getChannel() - 1;
		event.seconds = TickToSeconds(event.tick);
		event.setup = !msg.isNoteOnOrOff() && event.tick <= firstNoteTick[ch];

		if (msg.isController())
		{
			if (event.setup)
				m_setupController[ch][msg.getControllerNumber()] = msg.getControllerValue();
			else
				m_midSongController[ch][msg.getControllerNumber()] = true;
		}
		else if (msg.isProgramChange())
		{
			if (event.setup)
				m_setupProgram[ch] = msg.getProgramChangeNumber();
			else
				m_midSongProgram[ch] = true;
		}
		else if (msg.isPitchWheel())
		{
			if (event.setup)
				m_setupPitchWheel[ch] = msg.getPitchWheelValue();
			else
				m_midSongPitchWheel[ch] = true;
		}
	}

	m_endSeconds = TickToSeconds(m_endTick);

	for (auto& channel : m_sounding)
	{
		std::fill(std::begin(channel), std::end(channel), NoValue);
	}

	// send the setup part of the song (voices, volumes etc.)
	SilenceAll();
	for (const Event& event : m_events)
	{
		if (event.setup)
		{
			Send(event.message);
		}
	}

	m_playing = false;
	m_songSeconds = 0.0;
	m_nextEvent = 0;
	m_tempoFactor = 1.0;
	m_loopBeginTick = NoValue;
	m_loopEndTick = NoValue;
	m_reportedPosition = {1, 1};
	m_loaded = true;

	startTimer(1);
	return true;
}

void LocalSongPlayer::Unload()
{
	stopTimer();

	const ScopedLock lock(m_lock);
	if (m_loaded)
	{
		SilenceAll();
	}
	m_loaded = false;
	m_playing = false;
	m_events.clear();
}

bool LocalSongPlayer::IsLoaded() const
{
	const ScopedLock lock(m_lock);
	return m_loaded;
}

std::vector<int> LocalSongPlayer::GetUsedChannels() const
{
	const ScopedLock lock(m_lock);
	std::vector<int> channels;
	for (int ch = 0; ch < NumChannels; ch++)
	{
		if (m_loaded && m_usedChannels[ch])
		{
			channels.push_back(ch + 1);
		}
	}
	return channels;
}

//==============================================================================
// Transport

void LocalSongPlayer::Play()
{
	bool changed = false;
	{
		const ScopedLock lock(m_lock);
		if (!m_loaded || m_playing)
		{
			return;
		}

		if (m_nextEvent >= m_events.size())
		{
			// at the end of the song: start again from the beginning
			SeekToTick(0);
		}

		// restore controllers (e.g. sustain pedal) that were reset when pausing
		Chase(roundToInt(SecondsToTick(m_songSeconds)));
		m_playing = true;
		Rebase();
		changed = true;
	}
	Notify(false, changed);
}

void LocalSongPlayer::Pause()
{
	bool changed = false;
	{
		const ScopedLock lock(m_lock);
		if (m_playing)
		{
			m_playing = false;
			SilenceAll();
			changed = true;
		}
	}
	Notify(false, changed);
}

void LocalSongPlayer::Stop()
{
	bool playingChanged = false;
	bool positionChanged = false;
	{
		const ScopedLock lock(m_lock);
		if (!m_loaded)
		{
			return;
		}
		playingChanged = m_playing;
		m_playing = false;
		SilenceAll();
		SeekToTick(0);
		positionChanged = UpdateReportedPosition();
	}
	Notify(positionChanged, playingChanged);
}

bool LocalSongPlayer::IsPlaying() const
{
	const ScopedLock lock(m_lock);
	return m_playing;
}

//==============================================================================
// Position, tempo, transpose, loop

LocalSongPlayer::Position LocalSongPlayer::GetPosition() const
{
	const ScopedLock lock(m_lock);
	return m_reportedPosition;
}

LocalSongPlayer::Position LocalSongPlayer::GetLength() const
{
	const ScopedLock lock(m_lock);
	if (!m_loaded)
	{
		return {0, 0};
	}
	// the last event (usually a note-off) often lies exactly on the next bar line
	return TickToPosition(std::max(0, m_endTick - 1));
}

void LocalSongPlayer::SetPosition(Position position)
{
	bool positionChanged = false;
	{
		const ScopedLock lock(m_lock);
		if (!m_loaded)
		{
			return;
		}
		SilenceAll();
		SeekToTick(PositionToTick(position));
		positionChanged = UpdateReportedPosition();
	}
	Notify(positionChanged, false);
}

int LocalSongPlayer::GetBaseTempo() const
{
	const ScopedLock lock(m_lock);
	return roundToInt(m_baseBpm);
}

void LocalSongPlayer::SetTempo(int bpm)
{
	const ScopedLock lock(m_lock);
	if (m_playing)
	{
		// continue from the current position with the new speed
		m_songSeconds = m_startSongSeconds + (Time::getMillisecondCounterHiRes() - m_startMs) / 1000.0 * m_tempoFactor;
	}
	m_tempoFactor = jlimit(0.05, 10.0, bpm / m_baseBpm);
	Rebase();
}

void LocalSongPlayer::SetTranspose(int semitones)
{
	const ScopedLock lock(m_lock);
	// notes already sounding keep their pitch and are released correctly (see m_sounding)
	m_transpose = jlimit(-24, 24, semitones);
}

void LocalSongPlayer::SetLoop(Position begin, Position end)
{
	const ScopedLock lock(m_lock);
	const int beginTick = PositionToTick(begin);
	const int endTick = PositionToTick(end);
	if (endTick > beginTick)
	{
		m_loopBeginTick = beginTick;
		m_loopEndTick = endTick;
	}
	else
	{
		m_loopBeginTick = NoValue;
		m_loopEndTick = NoValue;
	}
}

void LocalSongPlayer::ResetLoop()
{
	const ScopedLock lock(m_lock);
	m_loopBeginTick = NoValue;
	m_loopEndTick = NoValue;
}

//==============================================================================
// Playback thread

void LocalSongPlayer::hiResTimerCallback()
{
	bool positionChanged = false;
	bool playingChanged = false;
	{
		const ScopedLock lock(m_lock);
		if (!m_loaded || !m_playing)
		{
			return;
		}

		m_songSeconds = m_startSongSeconds + (Time::getMillisecondCounterHiRes() - m_startMs) / 1000.0 * m_tempoFactor;

		// A-B loop: jump back to the beginning of the loop
		if (m_loopEndTick != NoValue && m_songSeconds >= TickToSeconds(m_loopEndTick))
		{
			SilenceAll();
			SeekToTick(m_loopBeginTick);
			Rebase();
		}

		while (m_nextEvent < m_events.size() && m_events[m_nextEvent].seconds <= m_songSeconds)
		{
			const Event& event = m_events[m_nextEvent++];
			if (!event.setup)
			{
				SendEvent(event.message);
			}
		}

		if (m_nextEvent >= m_events.size() && m_songSeconds >= m_endSeconds)
		{
			// end of the song: stop and go back to the beginning, like the piano does
			m_playing = false;
			SilenceAll();
			SeekToTick(0);
			playingChanged = true;
		}

		positionChanged = UpdateReportedPosition();
	}
	Notify(positionChanged, playingChanged);
}

//==============================================================================
// Helpers (called with m_lock held)

void LocalSongPlayer::Send(const MidiMessage& message)
{
	if (sendMidi)
	{
		sendMidi(message);
	}
}

void LocalSongPlayer::SendEvent(const MidiMessage& message)
{
	const int ch = message.getChannel() - 1;
	const bool transposable = message.getChannel() != DrumChannel;

	if (message.isNoteOn())
	{
		const int note = message.getNoteNumber();
		const int sounding = transposable ? jlimit(0, 127, note + m_transpose) : note;
		if (m_sounding[ch][note] != NoValue)
		{
			// the same note is started again: release the previous one first
			Send(MidiMessage::noteOff(ch + 1, m_sounding[ch][note]));
		}
		m_sounding[ch][note] = sounding;
		MidiMessage msg(message);
		msg.setNoteNumber(sounding);
		Send(msg);
	}
	else if (message.isNoteOff())
	{
		const int note = message.getNoteNumber();
		if (m_sounding[ch][note] != NoValue)
		{
			Send(MidiMessage::noteOff(ch + 1, m_sounding[ch][note], message.getVelocity()));
			m_sounding[ch][note] = NoValue;
		}
	}
	else if (message.isAftertouch())
	{
		const int note = message.getNoteNumber();
		if (m_sounding[ch][note] != NoValue)
		{
			MidiMessage msg(message);
			msg.setNoteNumber(m_sounding[ch][note]);
			Send(msg);
		}
	}
	else
	{
		Send(message);
	}
}

void LocalSongPlayer::SilenceAll()
{
	for (int ch = 0; ch < NumChannels; ch++)
	{
		for (int note = 0; note < 128; note++)
		{
			if (m_sounding[ch][note] != NoValue)
			{
				Send(MidiMessage::noteOff(ch + 1, m_sounding[ch][note]));
				m_sounding[ch][note] = NoValue;
			}
		}
		if (m_usedChannels[ch])
		{
			Send(MidiMessage::controllerEvent(ch + 1, 64, 0)); // sustain pedal off
			Send(MidiMessage::allNotesOff(ch + 1));
		}
	}
}

// Sends the state of the controllers, programs and pitch wheel that change during
// the song, as it is at the given position. Setup values (sent at load time) are only
// resent where a later change has to be undone, e.g. after jumping backwards.
void LocalSongPlayer::Chase(int tick)
{
	int controller[NumChannels][128];
	int program[NumChannels];
	int pitchWheel[NumChannels];
	for (int ch = 0; ch < NumChannels; ch++)
	{
		std::fill(std::begin(controller[ch]), std::end(controller[ch]), NoValue);
		program[ch] = NoValue;
		pitchWheel[ch] = NoValue;
	}

	for (const Event& event : m_events)
	{
		if (event.tick >= tick)
		{
			break;
		}
		if (event.setup)
		{
			continue;
		}
		const MidiMessage& msg = event.message;
		const int ch = msg.getChannel() - 1;
		if (msg.isController())
			controller[ch][msg.getControllerNumber()] = msg.getControllerValue();
		else if (msg.isProgramChange())
			program[ch] = msg.getProgramChangeNumber();
		else if (msg.isPitchWheel())
			pitchWheel[ch] = msg.getPitchWheelValue();
	}

	for (int ch = 0; ch < NumChannels; ch++)
	{
		for (int cc = 0; cc < 128; cc++)
		{
			if (!m_midSongController[ch][cc])
			{
				continue;
			}
			int value = controller[ch][cc] != NoValue ? controller[ch][cc] : m_setupController[ch][cc];
			if (value == NoValue && (cc == 1 || cc == 64))
			{
				value = 0; // modulation, sustain pedal: default is off
			}
			if (value != NoValue)
			{
				Send(MidiMessage::controllerEvent(ch + 1, cc, value));
			}
		}

		if (m_midSongProgram[ch])
		{
			const int value = program[ch] != NoValue ? program[ch] : m_setupProgram[ch];
			if (value != NoValue)
			{
				Send(MidiMessage::programChange(ch + 1, value));
			}
		}

		if (m_midSongPitchWheel[ch])
		{
			int value = pitchWheel[ch] != NoValue ? pitchWheel[ch] : m_setupPitchWheel[ch];
			Send(MidiMessage::pitchWheel(ch + 1, value != NoValue ? value : 8192));
		}
	}
}

void LocalSongPlayer::Rebase()
{
	m_startMs = Time::getMillisecondCounterHiRes();
	m_startSongSeconds = m_songSeconds;
}

void LocalSongPlayer::SeekToTick(int tick)
{
	tick = jlimit(0, m_endTick, tick);
	m_songSeconds = TickToSeconds(tick);
	m_nextEvent = (size_t)(std::lower_bound(m_events.begin(), m_events.end(), tick,
		[](const Event& event, int t) { return event.tick < t; }) - m_events.begin());
	Chase(tick);
	Rebase();
}

bool LocalSongPlayer::UpdateReportedPosition()
{
	const Position position = TickToPosition(roundToInt(std::floor(SecondsToTick(m_songSeconds))));
	if (position != m_reportedPosition)
	{
		m_reportedPosition = position;
		return true;
	}
	return false;
}

void LocalSongPlayer::Notify(bool positionChanged, bool playingChanged)
{
	if ((positionChanged || playingChanged) && onChanged)
	{
		onChanged(positionChanged, playingChanged);
	}
}

//==============================================================================
// Conversions

double LocalSongPlayer::TickToSeconds(double tick) const
{
	auto it = std::upper_bound(m_tempoMap.begin(), m_tempoMap.end(), tick,
		[](double t, const TempoPoint& point) { return t < point.tick; });
	const TempoPoint& point = *(it - 1);
	return point.seconds + (tick - point.tick) * point.secondsPerTick;
}

double LocalSongPlayer::SecondsToTick(double seconds) const
{
	auto it = std::upper_bound(m_tempoMap.begin(), m_tempoMap.end(), seconds,
		[](double s, const TempoPoint& point) { return s < point.seconds; });
	const TempoPoint& point = *(it - 1);
	return point.tick + (seconds - point.seconds) / point.secondsPerTick;
}

const LocalSongPlayer::TimeSignaturePoint& LocalSongPlayer::TimeSignatureAt(int tick) const
{
	auto it = std::upper_bound(m_timeSignatures.begin(), m_timeSignatures.end(), tick,
		[](int t, const TimeSignaturePoint& point) { return t < point.tick; });
	return *(it - 1);
}

LocalSongPlayer::Position LocalSongPlayer::TickToPosition(int tick) const
{
	const TimeSignaturePoint& point = TimeSignatureAt(std::max(0, tick));
	const int ticksPerMeasure = point.numerator * point.ticksPerBeat;
	const int delta = std::max(0, tick - point.tick);
	return {point.measure + delta / ticksPerMeasure + 1, (delta % ticksPerMeasure) / point.ticksPerBeat + 1};
}

int LocalSongPlayer::PositionToTick(Position position) const
{
	// find the time signature valid for the requested measure
	const int measure = std::max(0, position.measure - 1);
	const TimeSignaturePoint* point = &m_timeSignatures.front();
	for (const TimeSignaturePoint& p : m_timeSignatures)
	{
		if (p.measure <= measure)
			point = &p;
	}
	// beats outside the measure (e.g. beat 0) simply continue into the neighbouring measure
	const int tick = point->tick + (measure - point->measure) * point->numerator * point->ticksPerBeat +
		(position.beat - 1) * point->ticksPerBeat;
	return jlimit(0, m_endTick, tick);
}
