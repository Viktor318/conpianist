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

class Settings : public ChangeBroadcaster
{
public:
	Settings();
	void Save();
	void Load();
	String GetEffectiveLanguage() const;
	File GetLastStateFile() const;
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
	bool keyboardVisible = false;
	// Live Play channels (virtual keyboard, MIDI In 2): bit 0 = MIDI channel 1 etc.
	int keyboardChannels = 1;
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

private:
	PropertiesFile::Options opt;

	void PrepareResources();
};
