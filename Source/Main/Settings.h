/*
 *  This file is part of ConPianist. See <https://github.com/Viktor318/conpianist>.
 *  Fork of the original project <https://github.com/hugbug/conpianist>.
 *
 *  Copyright (C) 2018 Andrey Prygunkov <hugbug@users.sourceforge.net>
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

class Settings : public ChangeBroadcaster
{
public:
	Settings();
	void Save();
	void Load();
	String GetEffectiveLanguage() const;
	File GetLastStateFile() const;
	File GetDefaultSongDirectory() const;
	void ApplyLanguage() const;

	enum ScoreInstrumentNames
	{
		siHidden,
		siShort,
		siMixed,
		siFull
	};

	enum ScorePart
	{
		spAll,
		spRightAndLeft,
		spRight,
		spLeft
	};

	String pianoIp = "192.168.0.150";
	String midiPort;
	float zoomUi = 1.0;
	Rectangle<int> windowPos;
	// the position of the Recording and the Accompaniment window (NoWindowPos: not known,
	// the window is centred on the main window)
	static const int NoWindowPos = -100000;
	Point<int> recorderWindowPos{NoWindowPos, NoWindowPos};
	Point<int> accompanimentWindowPos{NoWindowPos, NoWindowPos};
	// true if a window of this size at this (saved) position can be reached on a screen
	static bool IsWindowPosUsable(Point<int> pos, int width);
	bool keyboardVisible = false;
	// Live Play channels (virtual keyboard, MIDI In 2): bit 0 = MIDI channel 1 etc.
	int keyboardChannels = 1;
	// Live Play (virtual keyboard, MIDI In 2) on the piano's own keyboard parts (Voice tab,
	// the piano's second MIDI port) instead of the Mixer channels, when possible
	bool livePlayOnPiano = true;
	bool IsKeyboardChannel(int channel) const { return channel >= 1 && channel <= 16 && (keyboardChannels & (1 << (channel - 1))) != 0; }
	int FirstKeyboardChannel() const;
	int keyboardHeight = 67; // height of the virtual keyboard panel, in pixels
	String resourcesPath;
	ScoreInstrumentNames scoreInstrumentNames = siMixed;
	ScorePart scorePart = spRightAndLeft;
	bool scoreShowMidiChannel = true;
	String workingDirectory;
	bool logging = false;
	bool rtpLogging = false;
	String language; // UI language: "en", "hu" or empty (use the system language)
	String lastSong; // full path of the last loaded song, reloaded at start with USB playback
	String midiIn2;  // secondary MIDI input, played through to MIDI Out in MIDI device mode
	String midiOut;  // MIDI device used when the piano is not available (or chosen)
	// the playback output chosen by the user, used again at the next start:
	// "network" (the piano's own player), "usb" (own player over USB) or "device" (MIDI Out)
	String playbackSource = "network";
	// Recording window: automatic start and stop (first note, silence) or by the buttons;
	// the silence that stops an automatic recording (seconds); the accompaniment is saved too
	static const int MinRecorderSilence = 2;
	static const int MaxRecorderSilence = 60;
	bool recorderAutomatic = true;
	int recorderSilence = 5;
	bool recorderStyle = true;
	int recorderCountIn = 1; // count-in measures of a manual recording (0: none)
	// quantization of the saved recording: the grid in ticks (480 per quarter note), the
	// triplet grid recognized too (ticks, 0: none), and whether the ends of the notes are
	// moved to the grid
	bool recorderQuantize = false;
	int recorderQuantizeTicks = 120;
	int recorderQuantizeTripletTicks = 0;
	bool recorderQuantizeFill = false; // notes are lengthened to the next note
	// Accompaniment window: a fill in is played when the main section is changed
	bool accompanimentAutoFill = false;
	// the (major) key of the music, for the names of the chords: empty - as the piano
	// sends them; a key with sharps (e.g. "E") or with flats (e.g. "Eb")
	String accompanimentKey;
	bool accompanimentMinor = false; // the key is a minor key (e.g. "A" is A minor)
	// The keys that can be chosen, by the number of their sharps (0..6) and then of their
	// flats (1..6); the major and the minor key at the same index have the same key
	// signature (C major and A minor etc.).
	static const int NumKeys = 13;
	static const char* KeyName(int index, bool minor);
	// The key signature of the chosen key: the number of sharps (positive) or flats
	// (negative); false if no key is chosen.
	bool GetKeySignature(int& sharps) const;
	bool recorderQuantizeEnds = false;

private:
	PropertiesFile::Options opt;

	void PrepareResources();
};
