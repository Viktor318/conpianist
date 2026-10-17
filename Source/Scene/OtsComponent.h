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
#include "OneTouchSettings.h"
#include "Settings.h"

// The content of the OTS window: the eight own One Touch Settings of the style of the
// piano, the buttons that store, delete, save and discard them; the four factory settings
// with OTS Link and the buttons that copy them to the own ones; and the voice settings
// of the Main, Layer and Left parts (the same ones as on the Voice tab and in the
// Balance window, which it follows).
class OtsComponent : public Component,
                     public PianoController::Listener,
                     private ChangeListener,
                     private Timer
{
public:
	OtsComponent(Settings& settings, PianoController& pianoController, OneTouchSettings& ots);
	~OtsComponent() override;

	void paint(Graphics& g) override;
	void resized() override;
	bool keyPressed(const KeyPress& key) override;
	void PianoStateChanged(PianoController::Aspect aspect, PianoController::Channel channel) override;

	// A button of the window, filled grey (the look of the OTS window); while blinking,
	// filled orange.
	class FilledButton : public TextButton
	{
	public:
		bool blinkOn = false;
		void paintButton(Graphics& g, bool highlighted, bool down) override;
	};

	// A place of a setting: lemon yellow when active, a white frame when it holds a
	// setting, a red frame when the active setting is modified.
	class SlotButton : public TextButton
	{
	public:
		bool active = false;
		bool used = false;
		bool modified = false;
		void paintButton(Graphics& g, bool highlighted, bool down) override;
	};

	// A place of a factory setting: orange when active, a white frame when it holds a
	// setting, a red frame when it is modified (active or not: the changed versions are
	// kept until another style is loaded).
	class FactorySlotButton : public TextButton
	{
	public:
		bool active = false;
		bool used = false;
		bool modified = false;
		void paintButton(Graphics& g, bool highlighted, bool down) override;
	};

private:
	Settings& settings;
	PianoController& pianoController;
	OneTouchSettings& ots;

	Label factoryLabel;
	TextButton linkButton; // OTS Link, orange when on
	FilledButton factorySaveButton;    // the active factory setting: Save (after asking)
	FilledButton factoryDiscardButton; // and Discard
	FactorySlotButton factoryButtons[OneTouchSettings::NumFactorySettings];
	FilledButton copyFirstButton;  // to the own 1-4
	FilledButton copySecondButton; // to the own 5-8

	Label styleLabel;
	Label styleNameLabel;
	Label ownLabel;
	SlotButton slotButtons[OneTouchSettings::NumSettings];
	FilledButton storeButton;
	FilledButton deleteButton;
	FilledButton saveButton;
	FilledButton discardButton;

	struct PartControls
	{
		TextButton onButton;  // the name of the part, on / off
		Label voiceLabel;
		FilledButton changeButton;
		Label volumeLabel;
		Slider volumeSlider;
		Label octaveLabel;
		FilledButton octaveDownButton;
		Label octaveValueLabel;
		FilledButton octaveUpButton;
	};
	// in the order of the window: Left, Main, Layer
	PartControls parts[OneTouchSettings::NumParts];
	static const PianoController::Channel PartChannels[OneTouchSettings::NumParts];

	// Store or Delete was pressed: the next own button gets the setting or is emptied
	enum class Mode { none, store, erase };
	Mode mode = Mode::none;
	bool blinkOn = false;
	void setMode(Mode newMode);

	void slotClicked(int index);
	void askDelete(int index);
	// copies the factory settings to the own places first..first + 3, after asking if
	// any of them holds a setting
	void askCopy(int first);
	// writes the changed version of the active factory setting into its file, after asking
	// (the original is lost)
	void askSaveFactory();
	void showVoiceMenu(int part);
	void update();
	void updateParts();
	void changeListenerCallback(ChangeBroadcaster*) override;
	void timerCallback() override;
};

// The OTS window: not modal and always on top, like the Accompaniment window. Closing
// only hides it.
class OtsWindow : public DocumentWindow
{
public:
	OtsWindow(Settings& settings, PianoController& pianoController, OneTouchSettings& ots);
	void closeButtonPressed() override;
	void visibilityChanged() override;
	// called when the window is shown or hidden (also by its close button)
	std::function<void()> onVisibilityChanged;
	void moved() override;
	// Moves the window where it was the last time; false if that is not known (or not
	// on a screen any more).
	bool RestorePosition();

private:
	Settings& settings;
};
