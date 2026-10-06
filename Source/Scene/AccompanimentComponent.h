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

// The content of the Accompaniment window: controls the accompaniment (style) of the
// piano: start and stop, sections (intro, main, fill in, break, ending), tempo, volume.
// The accompaniment is played by the piano itself; the window needs the piano.
class AccompanimentComponent : public Component,
                               public Button::Listener,
                               public PianoController::Listener,
                               private Timer
{
public:
	AccompanimentComponent(Settings& settings, PianoController& pianoController);
	~AccompanimentComponent() override;

	void paint(Graphics& g) override;
	void resized() override;
	void buttonClicked(Button* button) override;
	bool keyPressed(const KeyPress& key) override;
	void PianoStateChanged(PianoController::Aspect aspect, PianoController::Channel channel) override;
	void PianoNoteMessage(const MidiMessage& message) override;
	// Name of a note as the piano names it (middle C, note 60, is C3).
	static String noteName(int note);

	// Name of a chord as the piano reports it (e.g. "Cmaj7/E"); empty if there is none.
	// accidentals: 0 - the notes as the piano names them, +1 - the black keys with
	// sharps (C#), -1 - with flats (Db).
	static String chordName(const PianoController::StyleChord& chord, int accidentals = 0);
	// A style as it is shown: its name, type and default time signature, e.g.
	// "Standard 8Beat - Pro - (4/4)" (with middle dots), by its preset path or as a list item.
	static String styleTitle(const String& path);
	static String styleListName(const Style& style);

private:
	Settings& settings;
	PianoController& pianoController;

	// The styles of the piano: the list built into the program.
	const std::vector<Style>& styles = Presets::Styles();
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
	Rectangle<int> chordFrame; // the frame around the chord, in a row of its own
	Label keyLabel;
	ComboBox keyCombo;    // the key of the music: the chords are named with its sharps or flats
	ComboBox keyModeCombo; // major or minor
	void fillKeyCombo();
	// Filter of the style lists: only the styles of the chosen time signature are listed
	// (and only the categories and groups that have such styles). It is not saved.
	// The time signatures are the ones the styles are shown with. Item id: 1 no filter,
	// otherwise note value * 100 + beats (e.g. 403: 3/4, 806: 6/8).
	Label meterLabel;
	ComboBox meterCombo;
	static int meterId(const Style& style) { return style.beatUnit * 100 + style.beats; }
	bool matchesMeter(const Style& style) const;
	std::vector<double> taps; // times of the last presses of Tap Tempo (ms)
	// chord detection: where the chords are recognized, whether the keys below the split
	// point sound while the accompaniment is playing, and the split point of the
	// accompaniment (not the one of the Left part, which is on the Voice tab)
	Label chordAreaLabel;
	TextButton chordFullButton;
	TextButton chordLowerButton;
	TextButton leftSoundButton;
	Label splitLabel;
	TextButton splitDownButton;
	Label splitNameLabel;
	TextButton splitUpButton;
	TextButton splitLearnButton; // the next key played becomes the split point
	std::atomic<bool> splitLearning{false};
	void stepSplitPoint(int delta);
	void setBothSplitPoints(int note);
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

	// Registration memories: the style, the tempo, the key and the keyboard parts are
	// saved with the Memory button and a number, and recalled with the number (or F1..F8).
	// They are kept in a file of their own in the data folder of the program.
	static const int NumRegistrations = 8;
	struct Registration
	{
		bool used = false;
		String name;        // given by the user, may be empty
		String style;       // preset path of the style
		int tempo = PianoController::DefaultTempo;
		int styleVolume = PianoController::DefaultVolume;
		String key;         // as Settings::accompanimentKey (empty: no key)
		bool minor = false;
		int transpose = 0;         // of the song and Live Play (left panel)
		int keyboardTranspose = 0; // of the piano's own keys (Piano Room)
		int splitPoint = 0;        // of the Left part
		int styleSplitPoint = 0;   // of the accompaniment (0: not saved)
		struct Part
		{
			String voice;   // preset path of the voice
			bool active = false;
			int volume = PianoController::DefaultVolume;
			int pan = PianoController::DefaultPan;
			int reverb = PianoController::DefaultReverb;
			int octave = PianoController::DefaultOctave;
		};
		Part parts[3];      // Main, Layer, Left
		int chordArea = PianoController::caUnknown; // chord detection area
		int leftSound = -1; // sound of the keys below the split point (-1: not saved)
		// the parts of the accompaniment (saved since they can be mixed)
		bool hasStyleParts = false;
		struct StylePart
		{
			bool active = true;
			int volume = PianoController::DefaultVolume;
			int pan = PianoController::DefaultPan;
			int reverb = PianoController::DefaultReverb;
		};
		StylePart styleParts[PianoController::NumStyleParts];
	};
	// A number button: a click with the right mouse button opens its menu instead of
	// pressing it.
	class MemoryButton : public TextButton
	{
	public:
		std::function<void()> onMenu;
		void mouseDown(const MouseEvent& e) override;
		void mouseDrag(const MouseEvent& e) override;
		void mouseUp(const MouseEvent& e) override;
	private:
		bool menuClick = false;
	};
	TextButton memoryButton;
	MemoryButton registrationButtons[NumRegistrations];
	Registration registrations[NumRegistrations];
	bool memoryArmed = false;      // Memory was pressed: the next number saves
	bool memoryBlink = false;
	int currentRegistration = -1;  // recalled or saved last, while its style is on the piano
	// the style of a recalled registration is being loaded by the piano; its tempo and
	// volume are sent when the piano has loaded it
	String pendingStyle;
	int pendingTempo = 0;
	int pendingVolume = 0;
	int pendingRegistration = -1; // its parts of the accompaniment are sent with them
	bool pendingStyleChanged = false;
	int recallSerial = 0;
	File registrationsFile() const;
	void loadRegistrations();
	void saveRegistrations();
	Registration captureRegistration() const;
	void registrationPressed(int index);
	void storeRegistration(int index);
	void recallRegistration(int index);
	void finishRecall();
	void showRegistrationMenu(int index);
	void askRegistrationName(int index, const String& title, std::function<void(const String&)> done);
	String registrationTooltip(int index) const;
	void setMemoryArmed(bool armed);
	void updateRegistrationButtons();
	void timerCallback() override;
	Label hintLabel;            // a message, e.g. that the piano is not connected

	void update();
	void loadStyles();      // the categories that have styles (of the chosen time signature)
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
