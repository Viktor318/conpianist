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

struct Voice
{
	Voice(int num, String path, String type);

	int num;
	String path;
	String type;
	String title;
	String category1;
	String category2;
};

using VoiceList = std::vector<Voice>;

// A voice of the piano that is not on its panel (not in the voice tree): the XG, GM2 and
// GS voices and the Mega Voices of the styles, as in the Data List of the piano.
struct ExtraVoice
{
	int num;              // 0x00MMLLPP, as Voice::num
	const char* set;      // "XG", "GM2", "GS" or "Mega Voice"
	const char* category;
	const char* title;

	// a drum kit (GM2: bank MSB 120, GS: 118): only for the drum channel
	bool IsKit() const { const int msb = (num >> 16) & 0x7f; return msb == 120 || msb == 118; }
};

struct ReverbEffect
{
	ReverbEffect(String title, String description, int msb, int lsb) :
		num((msb << 8) + lsb), title(title), description(description) {}

	int num;
	String title;
	String description;
};

using ReverbEffectList = std::vector<ReverbEffect>;

class Presets
{
public:
	static VoiceList& Voices();
	static String VoiceTitle(String voice);
	static Voice* FindVoice(String voice);
	static const std::vector<ExtraVoice>& ExtraVoices();
	static const ExtraVoice* FindExtraVoice(int num);
	// The name of a voice by its number, for the track names of a recording: its own name,
	// or the category of the nearest panel voice; empty if the voice is not known.
	static String VoiceName(int num);
	static ReverbEffectList& ReverbEffects();
	static String ReverbEffectTitle(int num);

	// General MIDI voices, for MIDI devices that are not Yamaha pianos.
	// Voice numbers have the same format as Voice::num (0x00MMLLPP), with bank 0.
	static const StringArray& GmVoiceNames();   // 128 names, by program number
	static const StringArray& GmFamilies();     // 16 families of 8 programs each
	static const std::vector<std::pair<int, String>>& GmDrumKits(); // program, name
	static String GmVoiceTitle(String voice, bool drums);

	// Nearest General MIDI voice of a Yamaha voice (by its category and program number),
	// and the other way round. If there is no suitable voice, a piano is returned.
	static int GmVoiceForYamahaVoice(int yamahaVoice, bool drums);
	static int YamahaVoiceForGmVoice(int gmVoice, bool drums);
};
