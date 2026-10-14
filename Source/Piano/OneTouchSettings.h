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
#include "PianoController.h"

#include <set>

// The own One Touch Settings (OTS) of the styles: up to eight voice settings for each
// style, recalled with one press, as on Yamaha arranger keyboards. A setting holds the
// voice, the on / off state, the volume and the octave of the Main, Layer and Left parts
// (not the split point, nor the tempo or the volume of the accompaniment).
//
// The settings of a style are kept in a file of their own in the data folder of the
// program: OTS/<category>/<subcategory>/<style name>.ots (XML). The file is deleted when
// all eight places are empty.
//
// Only one setting is active at a time: the one recalled or stored last, while the
// style is on the piano. It is "modified" while the voices of the piano differ from it.
//
// Used on the message thread only. The listeners are told of every change of the
// shown state (synchronously).
class OneTouchSettings : public ChangeBroadcaster, private Timer
{
public:
	static const int NumSettings = 8;
	static const int NumParts = 3; // Main, Layer, Left
	static const PianoController::Channel PartChannels[NumParts];
	static const char* const PartNames[NumParts];

	struct Part
	{
		String voice;   // preset path of the voice
		bool active = true;
		int volume = PianoController::DefaultVolume;
		int octave = PianoController::DefaultOctave;
		bool operator==(const Part& other) const;
	};

	struct Setting
	{
		bool used = false;
		Part parts[NumParts];
	};

	// folder: the OTS folder in the data folder of the program
	OneTouchSettings(const File& folder, PianoController& pianoController);
	~OneTouchSettings() override;

	// Creates the OTS folder and in it the folders of the categories and subcategories of
	// the styles of the program (the place of their files), if they do not exist yet.
	// Called when the program starts.
	static void CreateFolders(const File& folder);

	// Follows the style of the piano: when it changes, the settings of the new style are
	// loaded and no setting is active. Called on every change of the piano that may
	// matter (the style, the voices of the keyboard parts, the connection).
	void Update();

	const String& GetStyle() const { return m_style; }
	bool IsUsed(int index) const;
	const Setting& Get(int index) const { return m_settings[index]; }
	int GetActive() const { return m_active; }
	bool IsModified() const { return m_active >= 0 && m_modified; }
	// whether a style (its preset path) has own settings, for the icon in the style lists
	bool HasSettings(const String& stylePath) const;

	// The voice settings of the piano into a place (it becomes the active one).
	void Store(int index);
	// The setting of a place to the piano (it becomes the active one). Nothing happens
	// if the place is empty.
	void Recall(int index);
	void Delete(int index);
	// The active setting: its place gets the voice settings of the piano (Save), or the
	// piano gets its stored values again (Discard).
	void SaveActive();
	void DiscardActive();

	// The file of the settings of a style.
	File FileOf(const String& stylePath) const;
	// A short description of a place for its tooltip: the voices of its parts.
	String Describe(int index) const;

private:
	File m_folder;
	PianoController& m_pianoController;
	String m_style; // preset path of the style whose settings are loaded
	Setting m_settings[NumSettings];
	int m_active = -1;
	bool m_modified = false;
	// after a recall the piano reports the new values one by one: until then (or for
	// at most this long) the setting is not taken for modified
	static const int RecallSettleMs = 2000;
	uint32 m_recallTime = 0;
	bool m_recallPending = false;
	std::set<String> m_stylesWithSettings; // preset paths

	Setting Capture() const;
	bool Matches(const Setting& setting) const;
	void Apply(const Setting& setting);
	void Load();
	void Save();
	void ScanFolder();
	void Changed();
	void timerCallback() override;
	static String VoicePath(const String& voice); // the preset path, if the voice is known
};
