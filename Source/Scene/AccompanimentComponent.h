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
	static String chordName(const PianoController::StyleChord& chord);
	// Name and category of a style from its preset path, e.g. "Contemp Gtr Pop (Pop & Rock)".
	static String styleTitle(const String& path);

private:
	Settings& settings;
	PianoController& pianoController;

	Label styleLabel;
	Label styleNameLabel;
	Label tempoLabel;
	Slider tempoSlider;
	Label chordLabel;
	Label chordNameLabel;
	TextButton startButton;
	TextButton syncStartButton;
	Label positionLabel;
	Label introLabel;
	Label mainLabel;
	Label endingLabel;
	TextButton introButtons[4];
	TextButton mainButtons[4];
	TextButton endingButtons[4];
	TextButton fillInButton;
	TextButton breakButton;
	Label volumeLabel;
	Slider volumeSlider;
	Label hintLabel;

	void update();
	void fillIn();
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
};
