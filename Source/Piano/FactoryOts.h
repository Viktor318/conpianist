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
#include "OneTouchSettings.h"

// Reads the factory One Touch Settings (OTS) of the styles of the CSP-170 from the style
// files of Yamaha arranger keyboards, copied by the user into the data folder of the
// program: Styles/PSR-SX600 and Styles/PSR-S970 (with their subfolders). A style of the
// piano gets the four OTS of the style file of the same name (letters and digits only,
// "&" as "and"; first PSR-SX600, then PSR-S970; the Pianist styles have none).
//
// A style file is a MIDI file with Yamaha chunks after it; the OTSc chunk holds the four
// OTS as tracks of MIDI messages: channel 1 Right 1, 2 Right 2, 3 Right 3, 4 Left (bank
// select and program change for the voice, Yamaha messages F0 43 73 01 50 08 cc pp vv F7
// for part pp of channel cc: 00 on / off, 03 octave (40H: 0), 04 volume). Right 1 is the
// Main part, Right 2 the Layer, Left the Left part; Right 3 is not used (the PSR-SX600 and
// the PSR-S970 have no Right 3). The on / off state of the Left part is not taken.
//
// A voice is a panel voice of the CSP-170 with the same number, or one of VoiceTable (the
// same name, a similar name, or chosen by Viktor; see docs/Gyari_OTS_hangszinek.xlsx). An
// OTS is taken if its Main voice is known, and its Layer voice too or the Layer is off;
// an unknown Left voice (or Layer voice of a Layer that is off) is not changed by the OTS.
// The others are left for a later version.
class FactoryOts
{
public:
	struct Result
	{
		bool sourcesFound = false; // a folder of the style files is there
		int styles = 0;            // styles with factory settings
		int settings = 0;          // factory settings taken
		int skipped = 0;           // factory settings left out (a voice not on the CSP-170)
		int missing = 0;           // styles without a style file of the same name
		StringArray details;       // for the log: what is left out, and why
	};

	// The folders of the style files in the Styles folder, in the order they are searched.
	static const StringArray& Sources();

	// Reads every style, then replaces the content of factoryFolder (see
	// OneTouchSettings::FileIn). Nothing is deleted if no folder of the style files is there.
	// progress (if given) gets how much is done (0..1): reading up to 0.9, then writing.
	static Result Import(const File& stylesFolder, const File& factoryFolder,
		const std::function<void(double)>& progress = nullptr);

	// The four OTS of a style file; false if it has none. Every left out setting is counted
	// in skipped and explained in log.
	static bool ReadStyleFile(const File& file, OneTouchSettings::Setting* settings, int& skipped, StringArray& log);

	// The panel voice of the CSP-170 (preset path) for a voice of a style file
	// (0x00MMLLPP, as Voice::num); empty if there is none.
	static String CspVoice(int yamahaVoice);

	// The letters and digits of a name, lower case ("&" as "and"), to find the style file
	// of a style.
	static String NameKey(const String& name);

	// The volume of a part: the value of the Yamaha message (true) or Control Change 7.
	// To be checked on a PSR-SX700 (October 17, 2026).
	static const bool VolumeFromPartMessage = true;
};
