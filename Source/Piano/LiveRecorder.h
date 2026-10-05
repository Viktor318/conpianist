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

// Records what is played live (virtual keyboard, MIDI In 2, the piano's own keys and,
// optionally, the piano's accompaniment) and writes it into a standard MIDI file,
// together with the voices and mixer settings, so that it sounds the same when played.
//
// The played messages come from different "sources"; each source becomes one MIDI
// channel (and one track) of the file.
class LiveRecorder
{
public:
	// Sources
	enum
	{
		srcMixer1 = 1,      // Live Play on Mixer channel 1..16: srcMixer1 + (channel - 1)
		srcMain = 17,       // the piano's keyboard parts
		srcLayer = 18,
		srcLeft = 19,
		srcStyle9 = 20,     // accompaniment, MIDI channel 9..16: srcStyle9 + (channel - 9)
		NumSources = 28
	};

	static const int NoValue = -1;

	// Voice and mixer settings of a source, written at the beginning of the file.
	struct Setup
	{
		int voice = NoValue;    // 0x00MMLLPP (bank MSB, bank LSB, program)
		int volume = NoValue;   // 0..127
		int pan = NoValue;      // 0..127 (64: centre)
		int reverb = NoValue;   // 0..127
		int partMode = NoValue; // XG part mode (accompaniment: drum parts)
	};

	enum State { stIdle, stArmed, stCountIn, stRecording };

	// Automatic start: armed, starts with the first played note.
	void Arm();
	// Starts at once (cancels a previous recording that was not saved).
	void Start();
	// Starts on a downbeat, after the given number of count-in measures (see Beat), or
	// earlier, with the first note played during the count-in.
	void StartCountIn(int measures);
	int GetCountInMeasuresLeft() const;
	// A beat of the metronome or of the playing song. A recording that starts while the
	// beats are running begins at the last downbeat, so its measures match the beats.
	void Beat(bool downbeat);
	// Ends the recording: held notes and the pedal are released in the recording.
	void Stop();
	// Drops the recording.
	void Clear();
	State GetState() const;

	void SetSetup(int source, const Setup& setup);
	// The settings of an accompaniment part (index 0..7: MIDI channel 9..16) as the piano
	// reports them; the values that are not known (NoValue) keep what was heard on MIDI.
	void SetStyleSetup(int index, const Setup& setup);
	// The key signature written into the file: the number of sharps (positive) or flats
	// (negative), major or minor; SetNoKeySignature: none is written.
	void SetKeySignature(int sharps, bool minor) { const ScopedLock lock(m_lock); m_keySharps = jlimit(-7, 7, sharps); m_keyMinor = minor; }
	void SetNoKeySignature() { const ScopedLock lock(m_lock); m_keySharps = NoValue; }

	// A chord recognized by the piano (root, type, bass root, bass type in one number,
	// 0xRRTTBBbb; 0x7f in the root: no chord); thread safe. The chords are written into
	// the file in Yamaha's XF format, and by their names as text if chordName is set.
	void AddChord(int chord);
	std::function<String(int chord)> chordName;

	// A played message of a source (the channel of the message is ignored); thread safe.
	void Add(int source, const MidiMessage& message);
	// A message of the piano's accompaniment (channel 9..16): the voices and settings are
	// always remembered (they are sent when the style is chosen), the rest is recorded.
	void AddStyle(const MidiMessage& message);

	bool HasData() const;
	// A recording is going on, or the finished recording has not been saved yet.
	bool IsUnsaved() const;
	void MarkSaved();
	int GetNoteCount() const;
	double GetLengthSeconds() const;         // from the start to now (recording) or to the end
	double GetSecondsSinceLastEvent() const; // while recording
	int GetHeldNoteCount() const;            // notes and pedals held now

	// Writes the recording; tempo in quarter notes per minute. The accompaniment is left
	// out if includeStyle is false. reverbType: XG reverb type (MSB << 8 | LSB) or NoValue.
	// quantizeTicks: if not 0, the beginning of every played note is moved to the nearest
	// multiple of this many ticks (see TicksPerQuarter), keeping the length of the note;
	// with quantizeEnds the end of the note is moved to the grid too. If tripletTicks is
	// not 0, it is a second grid (320: quarter-note triplets, 160: eighth-note triplets,
	// 80: sixteenth-note triplets): the measure is divided into the shortest parts that
	// both grids fit into, and each part uses the grid that fits its played notes better.
	// With fillGaps a note is lengthened to the beginning of the next note if it was held
	// for at least half of the time between them (no short rests in the score; notes
	// played short stay short). The accompaniment, the pedal and the other controllers
	// are not changed; the recording itself is not changed either.
	static const int TicksPerQuarter = 480;
	bool Save(const File& file, int tempo, int beatNumerator, int beatDenominator,
		bool includeStyle, int reverbType, String& error,
		int quantizeTicks = 0, bool quantizeEnds = false, int tripletTicks = 0,
		bool fillGaps = false) const;

private:
	struct Event
	{
		double time; // milliseconds from the start
		int source;
		MidiMessage message;
	};

	mutable CriticalSection m_lock;
	State m_state = stIdle;
	double m_startMs = 0;
	double m_lastEventMs = 0;
	double m_endMs = 0;
	double m_lastBeatMs = 0;
	double m_lastDownbeatMs = 0;
	int m_downbeatsLeft = 0;
	std::vector<Event> m_events;
	Setup m_setup[NumSources + 1];
	int m_noteCount = 0;
	bool m_saved = false;
	int m_keySharps = NoValue;
	static const int NoChord = 0x7f7f7f7f;
	struct ChordEvent
	{
		double time; // milliseconds from the start
		int chord;
	};
	std::vector<ChordEvent> m_chords;
	int m_currentChord = NoChord;
	bool m_keyMinor = false;
	int m_held[NumSources + 1][128] = {}; // held notes of each source
	int m_pedal[NumSources + 1] = {};     // sustain pedal value of each source

	// the accompaniment channels as the piano has set them (bank, program, controllers)
	int m_styleBankMsb[8] = {};
	int m_styleBankLsb[8] = {};
	Setup m_styleSetup[8];

	void Begin(double nowMs, bool alignToDownbeat);
	void MoveStart(double newStartMs);
	void Store(int source, const MidiMessage& message, double nowMs);
	double AddQuantized(MidiMessageSequence& track, int source, int channel,
		double ticksPerMs, int grid, bool quantizeEnds, int tripletTicks, bool fillGaps,
		double measureTicks) const;
	static bool IsRecordable(const MidiMessage& message);
};
