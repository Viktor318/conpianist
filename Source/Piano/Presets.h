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
