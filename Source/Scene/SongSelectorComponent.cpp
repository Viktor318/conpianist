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

#include "SongSelectorComponent.h"
#include "GuiHelper.h"

#include <algorithm>

namespace
{
	const Colour BackgroundColour(0xff323e44);
	const Colour ListColour(0xff263238);       // the background of the tree and the list
	const Colour OutlineColour(0xff8e989a);
	const Colour SelectedColour(0xff42555d);   // the selected row of the list
	const Colour OrangeColour(0xffee6c0a);     // the colour of the buttons that are on
	const Colour GreyTextColour(0xffaab4ba);   // number, composer
	const Colour DimTextColour(0xff69737a);    // a song that cannot be loaded now

	const int Margin = 10;
	const int RowHeight = 26;

	// the folder that is shown first: Score > 50 Popular > Pop
	const char* const DefaultFolder = "Score/50 Popular/Pop";
	// these folders are closed at first (many songs, rarely used)
	const char* const ClosedFolder = "PDF Score/Lesson";
}

//==============================================================================
// A folder in the tree; its unique name is its path in the songs folder.
class SongSelectorComponent::FolderItem : public TreeViewItem
{
public:
	FolderItem(SongSelectorComponent& owner, const String& path, const String& name, int depth) :
		owner(owner), path(path), name(name), depth(depth) {}

	const String& GetPath() const { return path; }

	// the item of a subfolder; created if it does not exist yet
	FolderItem* GetChild(const String& childName)
	{
		for (int i = 0; i < getNumSubItems(); i++)
		{
			auto* item = static_cast<FolderItem*>(getSubItem(i));
			if (item->name == childName)
			{
				return item;
			}
		}
		auto* item = new FolderItem(owner, path.isEmpty() ? childName : path + "/" + childName, childName, depth + 1);
		addSubItem(item);
		return item;
	}

	bool mightContainSubItems() override { return getNumSubItems() > 0; }
	String getUniqueName() const override { return path; }
	int getItemHeight() const override { return 24; }

	void paintItem(Graphics& g, int width, int height) override
	{
		g.setColour(Colours::white);
		g.setFont(Font(14.0f, depth <= 1 ? Font::bold : Font::plain));
		g.drawText(name, 4, 0, width - 6, height, Justification::centredLeft, true);
	}

	void paintOpenCloseButton(Graphics& g, const Rectangle<float>& area, Colour, bool) override
	{
		// a small white triangle: pointing down if the folder is open
		Path triangle;
		const float size = 8.0f;
		const auto c = area.getCentre();
		if (isOpen())
		{
			triangle.addTriangle(c.x - size / 2, c.y - size / 4, c.x + size / 2, c.y - size / 4, c.x, c.y + size / 3);
		}
		else
		{
			triangle.addTriangle(c.x - size / 4, c.y - size / 2, c.x - size / 4, c.y + size / 2, c.x + size / 3, c.y);
		}
		g.setColour(Colours::white.withAlpha(0.8f));
		g.fillPath(triangle);
	}

	void itemSelectionChanged(bool isNowSelected) override
	{
		if (isNowSelected)
		{
			owner.FolderSelected(path);
		}
	}

private:
	SongSelectorComponent& owner;
	String path;
	String name;
	int depth; // 1: the folders directly in the songs folder (the root item is 0)
};

//==============================================================================
SongSelectorComponent::SongSelectorComponent(Settings& settings, PianoController& pianoController) :
	settings(settings), pianoController(pianoController)
{
	// the folders: the tree is built from their paths
	rootItem = std::make_unique<FolderItem>(*this, String(), String(), 0);
	for (const String& folder : Presets::SongFolders())
	{
		FolderItem* item = rootItem.get();
		for (const String& part : StringArray::fromTokens(folder, "/", ""))
		{
			item = item->GetChild(part);
		}
	}
	folderTree.setRootItem(rootItem.get());
	folderTree.setRootItemVisible(false);
	folderTree.setDefaultOpenness(true);
	if (FolderItem* closed = FindFolderItem(ClosedFolder))
	{
		closed->setOpen(false);
	}
	folderTree.setIndentSize(14);
	folderTree.setColour(TreeView::backgroundColourId, ListColour);
	folderTree.setColour(TreeView::selectedItemBackgroundColourId, OrangeColour);
	folderTree.setColour(TreeView::linesColourId, Colours::transparentBlack);
	folderTree.setWantsKeyboardFocus(true);
	addAndMakeVisible(folderTree);

	headerLabel.setFont(Font(15.0f, Font::bold));
	headerLabel.setColour(Label::textColourId, Colours::white);
	headerLabel.setMinimumHorizontalScale(0.7f);
	addAndMakeVisible(headerLabel);

	searchLabel.setText(TRANS("Search"), dontSendNotification);
	searchLabel.setFont(Font(14.0f));
	searchLabel.setColour(Label::textColourId, GreyTextColour);
	searchLabel.setJustificationType(Justification::centredRight);
	addAndMakeVisible(searchLabel);

	searchEditor.setFont(Font(14.0f));
	searchEditor.setColour(TextEditor::backgroundColourId, ListColour);
	searchEditor.setColour(TextEditor::outlineColourId, OutlineColour);
	searchEditor.setTooltip(TRANS("Searches the title and the composer of the songs in every folder"));
	searchEditor.onTextChange = [this]()
		{
			songTable.deselectAllRows();
			UpdateEntries();
		};
	searchEditor.onEscapeKey = [this]() { searchEditor.clear(); UpdateEntries(); };
	searchEditor.onReturnKey = [this]()
		{
			// the first result is chosen; Enter in the list loads it
			if (!entries.empty())
			{
				songTable.selectRow(0);
				songTable.grabKeyboardFocus();
			}
		};
	addAndMakeVisible(searchEditor);

	TableHeaderComponent& header = songTable.getHeader();
	header.addColumn("#", colNumber, 40, 30, 60, TableHeaderComponent::notSortable);
	header.addColumn(TRANS("Title"), colTitle, 300, 100, -1, TableHeaderComponent::notSortable);
	header.addColumn(TRANS("Composer"), colComposer, 220, 80, -1, TableHeaderComponent::notSortable);
	header.addColumn(TRANS("Score"), colScore, 56, 56, 56, TableHeaderComponent::notSortable);
	header.setStretchToFitActive(true);
	header.setPopupMenuActive(false);
	header.setColour(TableHeaderComponent::backgroundColourId, ListColour);
	header.setColour(TableHeaderComponent::textColourId, GreyTextColour);
	header.setColour(TableHeaderComponent::outlineColourId, BackgroundColour.interpolatedWith(Colours::white, 0.25f));
	songTable.setHeaderHeight(24);
	songTable.setRowHeight(RowHeight);
	songTable.setColour(ListBox::backgroundColourId, ListColour);
	songTable.setColour(ListBox::outlineColourId, OutlineColour);
	songTable.setOutlineThickness(1);
	songTable.setMultipleSelectionEnabled(false);
	addAndMakeVisible(songTable);

	infoLabel.setFont(Font(13.0f));
	infoLabel.setColour(Label::textColourId, GreyTextColour);
	infoLabel.setJustificationType(Justification::centredLeft);
	infoLabel.setMinimumHorizontalScale(0.8f);
	addAndMakeVisible(infoLabel);

	openFileButton.setButtonText(TRANS("Open File..."));
	openFileButton.setTooltip(TRANS("Loads a MIDI file from another folder"));
	openFileButton.onClick = [this]() { if (onOpenFile) onOpenFile(); };
	addAndMakeVisible(openFileButton);

	loadButton.setButtonText(TRANS("Load"));
	loadButton.getProperties().set("toggle", true);
	loadButton.setToggleState(true, dontSendNotification);
	loadButton.onClick = [this]() { LoadSelected(); };
	addAndMakeVisible(loadButton);

	closeButton.setButtonText(TRANS("Close Window"));
	closeButton.onClick = [this]()
		{
			if (auto* window = findParentComponentOfClass<DocumentWindow>())
			{
				window->closeButtonPressed();
			}
		};
	addAndMakeVisible(closeButton);

	// the folder of the loaded song of the piano, otherwise Score > 50 Popular > Pop
	String folder = DefaultFolder;
	const String songPath = pianoController.GetSongName().replaceCharacter('\\', '/');
	if (const Song* song = songPath.startsWith("/SONG/") ? Presets::FindSong("PRESET:" + songPath) : nullptr)
	{
		folder = song->folder;
	}
	if (FolderItem* item = FindFolderItem(folder))
	{
		item->setSelected(true, true); // calls FolderSelected
	}

	pianoController.AddListener(this);
	setSize(DefaultWidth, DefaultHeight);
}

SongSelectorComponent::~SongSelectorComponent()
{
	pianoController.RemoveListener(this);
	folderTree.setRootItem(nullptr);
}

void SongSelectorComponent::paint(Graphics& g)
{
	g.fillAll(BackgroundColour);
	// the frame of the tree
	g.setColour(OutlineColour);
	g.drawRect(folderTree.getBounds().expanded(1), 1);
}

void SongSelectorComponent::resized()
{
	const int treeWidth = jlimit(180, 260, getWidth() * 30 / 100);
	const int bottomHeight = 30;
	const int bottomY = getHeight() - Margin - bottomHeight;

	folderTree.setBounds(Margin + 1, Margin + 1, treeWidth - 2, bottomY - 2 * Margin - 2);

	const int listX = Margin + treeWidth + Margin;
	const int listWidth = getWidth() - listX - Margin;
	const int searchWidth = jmin(185, listWidth / 3);
	searchEditor.setBounds(getWidth() - Margin - searchWidth, Margin, searchWidth, 24);
	searchLabel.setBounds(searchEditor.getX() - 3 - 70, Margin - 1, 70, 26);
	headerLabel.setBounds(listX - 4, Margin - 1, searchLabel.getX() - 3 - (listX - 4), 26);
	songTable.setBounds(listX, Margin + 24 + 6, listWidth, bottomY - Margin - (Margin + 24 + 6));

	// buttons on the right, the info line takes the rest
	closeButton.setBounds(getWidth() - Margin - 108, bottomY, 108, bottomHeight);
	loadButton.setBounds(closeButton.getX() - 8 - 104, bottomY, 104, bottomHeight);
	openFileButton.setBounds(loadButton.getX() - 8 - 130, bottomY, 130, bottomHeight);
	infoLabel.setBounds(Margin - 4, bottomY - 2, openFileButton.getX() - 8 - (Margin - 4), bottomHeight + 4);
}

void SongSelectorComponent::PianoStateChanged(PianoController::Aspect aspect, PianoController::Channel)
{
	if (aspect == PianoController::apPlaybackSource || aspect == PianoController::apConnection)
	{
		GuiHelper::CallAsync(this, [this]() { UpdateEntries(); });
	}
	else if (aspect == PianoController::apSongName)
	{
		GuiHelper::CallAsync(this, [this]() { songTable.repaint(); });
	}
	else if (aspect == PianoController::apSongLoaded)
	{
		GuiHelper::CallAsync(this, [this]() { StartPendingPlayback(); });
	}
}

void SongSelectorComponent::StartPendingPlayback()
{
	if (!playWhenLoaded)
	{
		return;
	}
	playWhenLoaded = false;
	if ((Time::getCurrentTime() - playRequestTime).inSeconds() > 20)
	{
		return;
	}
	// a short delay lets the piano finish loading (and the song state be restored)
	Timer::callAfterDelay(500, [self = Component::SafePointer<SongSelectorComponent>(this)]()
		{
			if (self != nullptr && self->pianoController.IsSongLoaded() && !self->pianoController.GetPlaying())
			{
				self->pianoController.Play();
			}
		});
}

void SongSelectorComponent::Refresh()
{
	UpdateEntries();
}

void SongSelectorComponent::FolderSelected(const String& folder)
{
	currentFolder = folder;
	if (searchEditor.getText().isNotEmpty())
	{
		searchEditor.clear(); // choosing a folder ends the search
	}
	songTable.deselectAllRows();
	UpdateEntries();
	songTable.scrollToEnsureRowIsOnscreen(0);
}

bool SongSelectorComponent::IsPresetFolder(const String& folder) const
{
	for (const Song& song : Presets::Songs())
	{
		if (song.folder == folder)
		{
			return true;
		}
	}
	return false;
}

SongSelectorComponent::FolderItem* SongSelectorComponent::FindFolderItem(const String& folder) const
{
	FolderItem* item = rootItem.get();
	for (const String& part : StringArray::fromTokens(folder, "/", ""))
	{
		FolderItem* found = nullptr;
		for (int i = 0; i < item->getNumSubItems() && found == nullptr; i++)
		{
			auto* sub = static_cast<FolderItem*>(item->getSubItem(i));
			if (sub->GetPath().fromLastOccurrenceOf("/", false, false) == part)
			{
				found = sub;
			}
		}
		if (found == nullptr)
		{
			return nullptr;
		}
		item = found;
	}
	return item;
}

// The score of a MIDI file: a file with the same name and the extension .musicxml, .xml
// or .mxl (as the Score tab looks for it).
File SongSelectorComponent::FindScore(const File& midiFile)
{
	for (const char* extension : {".musicxml", ".xml", ".mxl"})
	{
		const File score = midiFile.withFileExtension(extension);
		if (score.existsAsFile() && score.getSize() > 0)
		{
			return score;
		}
	}
	return {};
}

void SongSelectorComponent::UpdateEntries()
{
	// the selected song stays selected if it is still in the list
	const int selected = songTable.getSelectedRow();
	const String selectedTitle = selected >= 0 && selected < (int)entries.size() ? entries[selected].title : String();

	entries.clear();
	const String search = searchEditor.getText().trim();
	const bool searching = search.isNotEmpty();
	const bool pianoPlayback = pianoController.GetPlaybackSource() == PianoController::psPiano;

	auto inFolder = [&](const String& folder)
		{
			return searching || (currentFolder.isNotEmpty() &&
				(folder == currentFolder || folder.startsWith(currentFolder + "/")));
		};

	// the piano's own songs
	for (const Song& song : Presets::Songs())
	{
		if (!inFolder(song.folder) ||
			(searching && !song.title.containsIgnoreCase(search) && !song.composer.containsIgnoreCase(search)))
		{
			continue;
		}
		Entry entry;
		entry.song = &song;
		entry.title = song.title;
		entry.composer = song.composer;
		entry.file = settings.GetSongMidi(song);
		entry.score = settings.GetSongScore(song);
		entry.available = pianoPlayback || entry.file.existsAsFile();
		entries.push_back(entry);
	}

	// the MIDI files of the other folders (with their subfolders)
	const File songsDirectory = settings.GetSongsDirectory();
	for (const String& folder : Presets::SongFolders())
	{
		if (IsPresetFolder(folder) || !inFolder(folder))
		{
			continue;
		}
		Array<File> files = songsDirectory.getChildFile(folder).findChildFiles(File::findFiles, true, "*.mid;*.midi");
		std::sort(files.begin(), files.end(), [](const File& a, const File& b)
			{
				return a.getFileName().compareNatural(b.getFileName()) < 0;
			});
		for (const File& file : files)
		{
			const String title = file.getFileNameWithoutExtension();
			if (searching && !title.containsIgnoreCase(search))
			{
				continue;
			}
			Entry entry;
			entry.file = file;
			entry.title = title;
			entry.score = FindScore(file);
			entries.push_back(entry);
		}
	}

	if (searching)
	{
		headerLabel.setText(TRANS("Search results") + " (" + String((int)entries.size()) + ")", dontSendNotification);
	}
	else
	{
		headerLabel.setText(currentFolder.replace("/", String(CharPointer_UTF8("  \xe2\x80\xba  "))), dontSendNotification);
	}

	songTable.updateContent();
	songTable.deselectAllRows();
	if (selectedTitle.isNotEmpty())
	{
		for (int i = 0; i < (int)entries.size(); i++)
		{
			if (entries[i].title == selectedTitle)
			{
				songTable.selectRow(i);
				break;
			}
		}
	}
	songTable.repaint();
	message.clear();
	UpdateInfo();
	UpdateButtons();
}

void SongSelectorComponent::UpdateInfo()
{
	String text;
	if (message.isNotEmpty())
	{
		text = message;
	}
	else
	{
		switch (pianoController.GetPlaybackSource())
		{
			case PianoController::psPiano:
				text = TRANS("Playback via network: every song of the piano can be chosen.");
				break;
			case PianoController::psLocal:
				text = TRANS("Playback via USB: the grey songs have no MIDI file in their folder.");
				break;
			case PianoController::psMidiDevice:
				text = TRANS("Playback via MIDI device: the grey songs have no MIDI file in their folder.");
				break;
		}
		text += "  " + TRANS("Double click = load");
	}
	infoLabel.setText(text, dontSendNotification);
}

void SongSelectorComponent::UpdateButtons()
{
	const int row = songTable.getSelectedRow();
	loadButton.setEnabled(row >= 0 && row < (int)entries.size() && entries[row].available);
}

bool SongSelectorComponent::IsCurrentSong(const Entry& entry) const
{
	const String songName = pianoController.GetSongName();
	if (songName.isEmpty())
	{
		return false;
	}
	if (entry.song != nullptr &&
		songName.replaceCharacter('\\', '/') == entry.song->path.fromFirstOccurrenceOf("PRESET:", false, false))
	{
		return true;
	}
	return entry.file != File() && File::isAbsolutePath(songName) && File(songName) == entry.file;
}

void SongSelectorComponent::LoadSelected(bool play)
{
	playWhenLoaded = false;
	const int row = songTable.getSelectedRow();
	if (row < 0 || row >= (int)entries.size())
	{
		return;
	}
	const Entry& entry = entries[row];
	if (!entry.available)
	{
		return;
	}
	playWhenLoaded = play;
	playRequestTime = Time::getCurrentTime();

	if (entry.song != nullptr && pianoController.GetPlaybackSource() == PianoController::psPiano)
	{
		// the piano loads its own song
		if (!pianoController.LoadPresetSong(entry.song->path))
		{
			playWhenLoaded = false;
			message = TRANS("The piano is not connected.");
			UpdateInfo();
		}
		return;
	}

	if (onLoadFile)
	{
		onLoadFile(entry.file);
	}
}

int SongSelectorComponent::getNumRows()
{
	return (int)entries.size();
}

void SongSelectorComponent::paintRowBackground(Graphics& g, int rowNumber, int width, int height, bool rowIsSelected)
{
	if (rowIsSelected && rowNumber >= 0 && rowNumber < (int)entries.size())
	{
		g.setColour(entries[rowNumber].available ? SelectedColour : SelectedColour.withAlpha(0.5f));
		g.fillRoundedRectangle(2.0f, 1.0f, width - 4.0f, height - 2.0f, 3.0f);
	}
}

void SongSelectorComponent::paintCell(Graphics& g, int rowNumber, int columnId, int width, int height, bool)
{
	if (rowNumber < 0 || rowNumber >= (int)entries.size())
	{
		return;
	}
	const Entry& entry = entries[rowNumber];
	const bool current = IsCurrentSong(entry);
	const Colour mainColour = !entry.available ? DimTextColour : current ? OrangeColour : Colours::white;
	const Colour sideColour = !entry.available ? DimTextColour : GreyTextColour;

	switch (columnId)
	{
		case colNumber:
			g.setColour(sideColour);
			g.setFont(Font(14.0f));
			g.drawText(String(rowNumber + 1), 6, 0, width - 8, height, Justification::centredLeft, true);
			break;
		case colTitle:
			g.setColour(mainColour);
			g.setFont(Font(14.0f, current ? Font::bold : Font::plain));
			g.drawText(entry.title, 3, 0, width - 6, height, Justification::centredLeft, true);
			break;
		case colComposer:
			g.setColour(sideColour);
			g.setFont(Font(13.0f));
			g.drawText(entry.composer, 3, 0, width - 6, height, Justification::centredLeft, true);
			break;
		case colScore:
			if (entry.score != File())
			{
				DrawScoreIcon(g, (width - 11) / 2.0f, (height - 14) / 2.0f, entry.available ? Colours::white : DimTextColour);
			}
			break;
	}
}

// A sheet of music: a frame with three staff lines and a note head.
void SongSelectorComponent::DrawScoreIcon(Graphics& g, float x, float y, Colour colour)
{
	g.setColour(colour);
	g.drawRoundedRectangle(x, y, 11.0f, 14.0f, 1.0f, 1.2f);
	for (int i = 0; i < 3; i++)
	{
		g.fillRect(x + 2.5f, y + 2.6f + i * 2.4f, 6.0f, 0.9f);
	}
	g.fillEllipse(x + 3.0f, y + 9.4f, 4.0f, 3.2f);
}

void SongSelectorComponent::cellDoubleClicked(int rowNumber, int, const MouseEvent&)
{
	songTable.selectRow(rowNumber);
	LoadSelected(true);
}

void SongSelectorComponent::returnKeyPressed(int)
{
	LoadSelected();
}

void SongSelectorComponent::selectedRowsChanged(int)
{
	message.clear();
	UpdateInfo();
	UpdateButtons();
}

String SongSelectorComponent::getCellTooltip(int rowNumber, int)
{
	if (rowNumber < 0 || rowNumber >= (int)entries.size())
	{
		return {};
	}
	const Entry& entry = entries[rowNumber];
	if (!entry.available)
	{
		return TRANS("There is no MIDI file for this song in its folder; the piano's own songs can be played via network");
	}
	if (searchEditor.getText().trim().isNotEmpty())
	{
		// where the song was found
		const String folder = entry.song != nullptr ? entry.song->folder :
			entry.file.getParentDirectory().getRelativePathFrom(settings.GetSongsDirectory()).replaceCharacter('\\', '/');
		return folder.replace("/", String(CharPointer_UTF8(" \xe2\x80\xba ")));
	}
	return {};
}

//==============================================================================
SongSelectorWindow::SongSelectorWindow(Settings& settings, PianoController& pianoController) :
	DocumentWindow(TRANS("Song Selector"), Colour(0xff323e44),
		DocumentWindow::minimiseButton | DocumentWindow::closeButton),
	settings(settings)
{
	const bool usingNativeTitleBar = (SystemStats::getOperatingSystemType() & SystemStats::Windows) ||
		(SystemStats::getOperatingSystemType() & SystemStats::MacOSX);
	setUsingNativeTitleBar(usingNativeTitleBar);
	setContentOwned(new SongSelectorComponent(settings, pianoController), true);
	setResizable(true, !usingNativeTitleBar);
	setResizeLimits(SongSelectorComponent::MinimumWidth, SongSelectorComponent::MinimumHeight, 10000, 10000);
	setAlwaysOnTop(true);
}

void SongSelectorWindow::closeButtonPressed()
{
	setVisible(false);
}

// The position and the size are remembered (saved with the settings when the program exits).
void SongSelectorWindow::StoreBounds()
{
	if (isShowing() && !isMinimised() && getX() > -10000 && getY() > -10000)
	{
		settings.songSelectorWindowBounds = getBounds();
	}
}

void SongSelectorWindow::moved()
{
	DocumentWindow::moved();
	StoreBounds();
}

void SongSelectorWindow::resized()
{
	DocumentWindow::resized();
	StoreBounds();
}

bool SongSelectorWindow::RestoreBounds()
{
	const Rectangle<int> bounds = settings.songSelectorWindowBounds;
	if (!Settings::IsWindowPosUsable(bounds.getPosition(), bounds.getWidth()) ||
		bounds.getWidth() < SongSelectorComponent::MinimumWidth / 2 || bounds.getHeight() < SongSelectorComponent::MinimumHeight / 2)
	{
		return false;
	}
	setBounds(bounds);
	return true;
}
