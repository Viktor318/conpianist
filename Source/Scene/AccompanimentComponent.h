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
#include "Settings.h"

// The content of the Accompaniment window: controls the accompaniment (style) of the
// piano: start and stop, sections (intro, main, fill in, break, ending), tempo, volume.
// The accompaniment is played by the piano itself; the window needs the piano.
class AccompanimentComponent : public Component,
                               public Button::Listener,
                               public PianoController::Listener
{
public:
	AccompanimentComponent(Settings& settings, PianoController& pianoController);
	~AccompanimentComponent() override;

	void paint(Graphics& g) override;
	void resized() override;
	void buttonClicked(Button* button) override;
	bool keyPressed(const KeyPress& key) override;
	void PianoStateChanged(PianoController::Aspect aspect, PianoController::Channel channel) override;

	// Name of a chord as the piano reports it (e.g. "Cmaj7/E"); empty if there is none.
	// accidentals: 0 - the notes as the piano names them, +1 - the black keys with
	// sharps (C#), -1 - with flats (Db).
	static String chordName(const PianoController::StyleChord& chord, int accidentals = 0);
	// Name and category of a style from its preset path, e.g. "Contemp Gtr Pop (Pop & Rock)".
	static String styleTitle(const String& path);

private:
	Settings& settings;
	PianoController& pianoController;

	// A style of the piano, from the list file (styles.csv in the data folder of the
	// program; it is not part of the program, the lists stay empty without it).
	struct StyleEntry
	{
		String path;     // preset path, e.g. PRESET:/STYLE/Pop & Rock/Pop/Contemp Gtr Pop.T308.prs
		String title;
		String category; // e.g. Pop & Rock
		String group;    // e.g. Pop
		int tempo = 0;   // default tempo of the style (0: not known)
	};
	std::vector<StyleEntry> styles;
	StringArray categories;
	String shownStyle = "?"; // the style of the piano that the lists show

	// The lists can be browsed freely; the chosen style is loaded with the Apply button
	// (or the Enter key), like on an arranger keyboard.
	Label styleLabel;
	ComboBox categoryCombo; // item id: index in categories + 1
	ComboBox groupCombo;    // groups of the category; item id: index in groups + 1
	ComboBox styleCombo;    // styles of the group; item id: index in styles + 1
	TextButton applyButton;
	Label currentLabel;
	Label currentNameLabel; // the style of the piano
	StringArray groups;     // of the chosen category
	Label tempoLabel;
	Slider tempoSlider;
	TextButton resetTempoButton; // back to the default tempo of the style
	TextButton tapTempoButton;
	Label chordLabel;
	Label chordNameLabel; // the recognized chord, large, in the middle of its frame
	Rectangle<int> chordFrame; // the frame around the chord: the rest of the row
	Label keyLabel;
	ComboBox keyCombo;    // the key of the music: the chords are named with its sharps or flats
	ComboBox keyModeCombo; // major or minor
	void fillKeyCombo();
	std::vector<double> taps; // times of the last presses of Tap Tempo (ms)
	TextButton startButton;
	TextButton syncStartButton;
	Label positionLabel;
	Label introLabel;
	Label mainLabel;
	Label endingLabel;
	// Three intros and endings, as on Yamaha arranger keyboards. The style format knows a
	// fourth one of each, but the piano does not play them (tested on the CSP-170).
	static const int NumIntros = 3;
	static const int NumMains = 4;
	static const int NumEndings = 3;
	TextButton introButtons[NumIntros];
	TextButton mainButtons[NumMains];
	TextButton endingButtons[NumEndings];
	TextButton autoFillButton; // a fill in is played when the main section is changed
	TextButton fillInButton;
	TextButton breakButton;
	Label volumeLabel;
	Slider volumeSlider;
	TextButton shortcutsButton; // shows the list of the keyboard shortcuts
	Label hintLabel;            // a message, e.g. that the piano is not connected

	void update();
	void loadStyles();
	void fillGroupCombo();  // the groups of the chosen category, the first one chosen
	void fillStyleCombo();  // the styles of the chosen group, the first one chosen
	void applyStyle();
	void showStyleInLists(int index);
	void fillIn();
	void changeMain(int index);
	void toggleAutoFill();
	void tapTempo();
	void resetTempo();
	void showShortcuts();
	int keyAccidentals() const; // of the chosen key: 0, +1 (sharps) or -1 (flats)
	int defaultTempo() const; // of the style of the piano; 0 if it is not in the list
	// current: the section is playing; next: it is played after the current one
	static void markButton(TextButton& button, bool current, bool next);
};

// The Accompaniment window: not modal and always on top, like the Recording window.
// Closing only hides it.
class AccompanimentWindow : public DocumentWindow
{
public:
	AccompanimentWindow(Settings& settings, PianoController& pianoController);
	void closeButtonPressed() override;
	void moved() override;
	// Moves the window where it was the last time; false if that is not known (or not
	// on a screen any more).
	bool RestorePosition();

private:
	Settings& settings;
};
