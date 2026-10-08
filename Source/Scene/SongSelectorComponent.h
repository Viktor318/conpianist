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
#include "Presets.h"
#include "Settings.h"

// Song Selector: the folders of the songs folder on the left (the piano's own song
// categories, Bonus Songs, Music Library and the user's folders), the songs of the chosen
// folder on the right. The piano's own songs are loaded by the piano itself with network
// playback; with playback via USB or MIDI device only those can be loaded that have a MIDI
// file in their folder (named like the score, see Settings::GetSongMidi). The other folders
// list their MIDI files. The search looks in every folder (title and composer).
class SongSelectorComponent : public Component,
	public PianoController::Listener,
	public TableListBoxModel
{
public:
	SongSelectorComponent(Settings& settings, PianoController& pianoController);
	~SongSelectorComponent() override;

	void paint(Graphics& g) override;
	void resized() override;
	void PianoStateChanged(PianoController::Aspect aspect, PianoController::Channel channel) override;

	// Reads the folders again (called when the window is shown).
	void Refresh();
	// Called by the tree when a folder is chosen ("Score/50 Popular/Pop" etc.).
	void FolderSelected(const String& folder);

	// TableListBoxModel
	int getNumRows() override;
	void paintRowBackground(Graphics& g, int rowNumber, int width, int height, bool rowIsSelected) override;
	void paintCell(Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected) override;
	void cellDoubleClicked(int rowNumber, int columnId, const MouseEvent& event) override;
	void returnKeyPressed(int lastRowSelected) override;
	void selectedRowsChanged(int lastRowSelected) override;
	String getCellTooltip(int rowNumber, int columnId) override;

	// loads a MIDI file in the player (like the file dialog of the left panel)
	std::function<void(const File& file)> onLoadFile;
	// the file dialog of the left panel, for a song outside the songs folder
	std::function<void()> onOpenFile;

	static const int DefaultWidth = 760;
	static const int DefaultHeight = 520;
	static const int MinimumWidth = 560;
	static const int MinimumHeight = 380;

private:
	class FolderItem;

	enum Column { colNumber = 1, colTitle, colComposer, colScore };

	struct Entry
	{
		const Song* song = nullptr; // one of the piano's songs, or nullptr for a MIDI file
		File file;                  // the MIDI file (for a song of the piano: if there is one)
		File score;                 // the score next to it (if there is one)
		String title;
		String composer;
		bool available = true;      // can be loaded with the current playback source
	};

	void UpdateEntries();
	void UpdateInfo();
	void UpdateButtons();
	void LoadSelected();
	bool IsCurrentSong(const Entry& entry) const;
	bool IsPresetFolder(const String& folder) const;
	FolderItem* FindFolderItem(const String& folder) const;
	static File FindScore(const File& midiFile);
	static void DrawScoreIcon(Graphics& g, float x, float y, Colour colour);

	Settings& settings;
	PianoController& pianoController;

	TreeView folderTree;
	std::unique_ptr<FolderItem> rootItem;
	Label headerLabel;
	Label searchLabel;
	TextEditor searchEditor;
	TableListBox songTable{ {}, this };
	Label infoLabel;
	TextButton openFileButton;
	TextButton loadButton;
	TextButton closeButton;

	String currentFolder;
	std::vector<Entry> entries;
	String message; // shown in the info line instead of the hint until the selection changes

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SongSelectorComponent)
};

// The window of the Song Selector: it stays open after a song is loaded; it can be resized
// and minimised (e.g. to see the score), its position and size are remembered.
class SongSelectorWindow : public DocumentWindow
{
public:
	SongSelectorWindow(Settings& settings, PianoController& pianoController);
	void closeButtonPressed() override;
	void moved() override;
	void resized() override;
	// Puts the window where it was the last time, with its size; false if that is not known
	// (or not on a screen any more).
	bool RestoreBounds();
	SongSelectorComponent* GetSelector() { return dynamic_cast<SongSelectorComponent*>(getContentComponent()); }

private:
	void StoreBounds();

	Settings& settings;
};
