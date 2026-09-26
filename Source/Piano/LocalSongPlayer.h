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

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

#include <functional>
#include <vector>

// Plays a standard MIDI file by sending its events to a MIDI output in real time.
// Used when the piano is connected via a MIDI port (USB) instead of the network:
// in that case the song cannot be uploaded to the piano's own player, so ConPianist
// plays it itself and the piano (or any other MIDI device) just produces the sound.
//
// Positions are 1-based (measure 1, beat 1 is the start of the song), the same way
// the piano reports them, so the rest of the program can use them unchanged.
//
// Setup events (program changes, controllers etc. before the first note of a channel)
// are sent once when the song is loaded and never again. This way voices chosen by the
// user in the Mixer are kept until the MIDI file is loaded again - the same behaviour
// as with the piano's own player.
class LocalSongPlayer : private HighResolutionTimer
{
public:
	struct Position
	{
		int measure;
		int beat;
		bool operator==(const Position& rhs) const { return measure == rhs.measure && beat == rhs.beat; }
		bool operator!=(const Position& rhs) const { return !(*this == rhs); }
	};

	// Sends one MIDI message to the output. Called from the playback thread.
	std::function<void(const MidiMessage&)> sendMidi;

	// Called when the position or the playing state changed (from the playback thread
	// or from the thread that called a method of this class).
	std::function<void(bool positionChanged, bool playingChanged)> onChanged;

	LocalSongPlayer() = default;
	~LocalSongPlayer() override;

	bool Load(const File& file);
	void Unload();
	bool IsLoaded() const;

	void Play();
	void Pause();
	void Stop();
	bool IsPlaying() const;

	Position GetPosition() const;
	Position GetLength() const;
	void SetPosition(Position position);

	int GetBaseTempo() const; // tempo at the start of the file, in BPM
	void SetTempo(int bpm);
	void SetTranspose(int semitones);

	void SetLoop(Position begin, Position end);
	void ResetLoop();

	// A muted channel (1..16) plays silently: its notes are not sent, but its controllers
	// and program changes are, so it sounds right when it is switched on again.
	// Used for the Mixer channel switches and the right/left/backing part buttons.
	void SetChannelMuted(int channel, bool muted);

	// MIDI channels (1..16) that contain notes
	std::vector<int> GetUsedChannels() const;

private:
	struct Event
	{
		int tick;
		double seconds; // at the original tempo
		MidiMessage message;
		bool setup;     // sent only once, when the song is loaded
	};

	struct TempoPoint
	{
		int tick;
		double seconds;
		double secondsPerTick;
	};

	struct TimeSignaturePoint
	{
		int tick;
		int measure;      // 0-based index of the measure starting at this tick
		int numerator;
		int ticksPerBeat;
	};

	static const int NumChannels = 16;
	static const int DrumChannel = 10;
	static const int NoValue = -1;

	mutable CriticalSection m_lock;

	// song data (read-only while playing)
	std::vector<Event> m_events;
	std::vector<TempoPoint> m_tempoMap;
	std::vector<TimeSignaturePoint> m_timeSignatures;
	int m_ticksPerQuarter = 480;
	int m_endTick = 0;
	double m_endSeconds = 0.0;
	double m_baseBpm = 120.0;
	bool m_usedChannels[NumChannels] = {};
	bool m_muted[NumChannels] = {};
	bool m_loaded = false;

	// controllers/programs that change during the song (not only in the setup part)
	bool m_midSongController[NumChannels][128] = {};
	bool m_midSongProgram[NumChannels] = {};
	bool m_midSongPitchWheel[NumChannels] = {};
	int m_setupController[NumChannels][128];
	int m_setupProgram[NumChannels];
	int m_setupPitchWheel[NumChannels];

	// playback state
	bool m_playing = false;
	double m_songSeconds = 0.0;      // current position, in original-tempo seconds
	double m_startMs = 0.0;          // wall clock when playback (re)started
	double m_startSongSeconds = 0.0; // song position when playback (re)started
	double m_tempoFactor = 1.0;
	size_t m_nextEvent = 0;
	int m_transpose = 0;
	int m_sounding[NumChannels][128]; // note actually sent for each original note, or NoValue
	int m_loopBeginTick = NoValue;
	int m_loopEndTick = NoValue;
	Position m_reportedPosition{1, 1};

	void hiResTimerCallback() override;

	void Send(const MidiMessage& message);
	void SendEvent(const MidiMessage& message);
	void SilenceAll();
	void Chase(int tick);
	void Rebase();
	void SeekToTick(int tick);
	bool UpdateReportedPosition();
	void Notify(bool positionChanged, bool playingChanged);

	double TickToSeconds(double tick) const;
	double SecondsToTick(double seconds) const;
	Position TickToPosition(int tick) const;
	int PositionToTick(Position position) const;
	const TimeSignaturePoint& TimeSignatureAt(int tick) const;
};
