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

// The content of the Accompaniment mixer window: the eight parts of the accompaniment
// (style) of the piano, each with on/off, voice name, pan, reverb and volume, and a
// master strip (the volume, pan and reverb of the whole accompaniment). The controls look
// and work like those of the Balance window: a double click sets a value back to what
// the style came with, a part that is off is greyed out and cannot be set. The type of
// the reverb is the same setting as in the Balance window and in Piano Room.
class StyleMixerComponent : public Component,
                            public PianoController::Listener
{
public:
	StyleMixerComponent(Settings& settings, PianoController& pianoController);
	~StyleMixerComponent() override;

	void paint(Graphics& g) override;
	void resized() override;
	void mouseDoubleClick(const MouseEvent& event) override;
	void PianoStateChanged(PianoController::Aspect aspect, PianoController::Channel channel) override;

private:
	Settings& settings;
	PianoController& pianoController;

	static const int NumParts = PianoController::NumStyleParts;
	static const int MasterStrip = NumParts; // index of the master strip
	struct Strip
	{
		PianoController::Channel channel = PianoController::chNone;
		TextButton onButton;  // the name of the part; lit when the part is on
		Label nameLabel;      // the master strip has a label instead of the button
		Label voiceLabel;
		Slider panSlider;
		Slider reverbSlider;
		Slider volumeSlider;
	};
	Strip strips[NumParts + 1];
	Label panCaption;
	Label reverbCaption;
	Label volumeCaption;
	ComboBox reverbEffectCombo; // item id: number of the effect + ReverbEffectIdBase
	Label hintLabel;            // a message, e.g. that the piano is not connected
	static const int ReverbEffectIdBase = 1000000;

	void update();
	static bool isStripChannel(PianoController::Channel channel);
};

// The Accompaniment mixer window: not modal and always on top, like the Accompaniment
// window. Closing only hides it.
class StyleMixerWindow : public DocumentWindow
{
public:
	StyleMixerWindow(Settings& settings, PianoController& pianoController);
	void closeButtonPressed() override;
	void moved() override;
	// Moves the window where it was the last time; false if that is not known (or not
	// on a screen any more).
	bool RestorePosition();

private:
	Settings& settings;
};

// A button of the top bar whose icon is drawn in code, in the style of the image
// buttons next to it (white, fainter when disabled or under the mouse).
class TopBarButton : public Button
{
public:
	enum Icon
	{
		iconRecord,        // Recording window: a dot in a ring
		iconAccompaniment, // Accompaniment window: a drum with two sticks
		iconStyleMixer     // Accompaniment mixer window: a drum below three faders
	};

	TopBarButton(const String& name, Icon icon) : Button(name), m_icon(icon) {}
	void paintButton(Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

private:
	// A drum seen from the side, between the given edges.
	static void drawDrum(Graphics& g, float left, float right, float top, float bottom,
		float rim, float shell, float lug, float lugInset);

	Icon m_icon;
};
