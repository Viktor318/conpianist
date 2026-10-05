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

#include "AccompanimentComponent.h"
#include "GuiHelper.h"
#include "Presets.h"

AccompanimentComponent::AccompanimentComponent(Settings& settings, PianoController& pianoController) :
	settings(settings), pianoController(pianoController)
{
	auto initLabel = [this](Label& label, const String& text)
		{
			label.setText(text, dontSendNotification);
			label.setFont(Font(FontOptions(15.00f, Font::plain)));
			label.setJustificationType(Justification::centredLeft);
			label.setColour(Label::textColourId, Colours::white);
			addAndMakeVisible(label);
		};
	// the buttons do not take the keyboard focus, so the shortcuts of the window always work
	auto initButton = [this](TextButton& button, const String& text)
		{
			button.setButtonText(text);
			button.setWantsKeyboardFocus(false);
			button.addListener(this);
			addAndMakeVisible(button);
		};

	initLabel(styleLabel, TRANS("Style:"));

	// the lists do not take the keyboard focus either
	categoryCombo.setWantsKeyboardFocus(false);
	categoryCombo.setTooltip(TRANS("Category of the style"));
	categoryCombo.onChange = [this]() { fillGroupCombo(); };
	addAndMakeVisible(categoryCombo);

	groupCombo.setWantsKeyboardFocus(false);
	groupCombo.setTooltip(TRANS("Subcategory of the style"));
	groupCombo.onChange = [this]() { fillStyleCombo(); };
	addAndMakeVisible(groupCombo);

	styleCombo.setWantsKeyboardFocus(false);
	styleCombo.onChange = [this]() { update(); };
	addAndMakeVisible(styleCombo);

	initButton(applyButton, TRANS("Apply"));
	applyButton.setTooltip(TRANS("Loads the chosen style on the piano (Enter)"));

	initLabel(currentLabel, TRANS("Current:"));
	initLabel(currentNameLabel, "");
	currentNameLabel.setMinimumHorizontalScale(0.7f);
	// bold, in the colour of the chord, in a frame like the chord (see paint)
	currentNameLabel.setFont(Font(FontOptions(17.00f, Font::bold)));
	currentNameLabel.setColour(Label::textColourId, Colour(0xffee6c0a));

	loadStyles();

	initLabel(tempoLabel, TRANS("Tempo:"));
	tempoSlider.setSliderStyle(Slider::IncDecButtons);
	tempoSlider.setTextBoxStyle(Slider::TextBoxLeft, false, 40, 24);
	tempoSlider.setRange(PianoController::MinTempo, PianoController::MaxTempo, 1);
	tempoSlider.onValueChange = [this]()
		{
			const int tempo = roundToInt(tempoSlider.getValue());
			if (tempo != this->pianoController.GetStyleTempo())
			{
				this->pianoController.SetStyleTempo(tempo);
			}
		};
	addAndMakeVisible(tempoSlider);

	// the key of the music: its note (first the keys with sharps, then the ones with
	// flats) and major or minor
	initLabel(keyLabel, TRANS("Key:"));
	initLabel(chordLabel, TRANS("Chord:"));
	keyCombo.setWantsKeyboardFocus(false);
	keyCombo.setTooltip(TRANS("The key of the music: the chords are named with the sharps or the flats of the key (without a key, as the piano names them), and the key is written into the recording"));
	keyModeCombo.setWantsKeyboardFocus(false);
	keyModeCombo.addItem(TRANS("major"), 1);
	keyModeCombo.addItem(TRANS("minor"), 2);
	keyModeCombo.setTooltip(TRANS("Major or minor key; changing it keeps the key signature (C major - A minor)"));
	keyModeCombo.setSelectedId(settings.accompanimentMinor ? 2 : 1, dontSendNotification);
	fillKeyCombo();
	keyCombo.onChange = [this]()
		{
			this->settings.accompanimentKey = keyCombo.getSelectedId() > 1 ? keyCombo.getText() : String();
			this->settings.Save();
			update();
		};
	keyModeCombo.onChange = [this]()
		{
			// the key with the same key signature in the other mode (the same place in
			// the list)
			const int selected = keyCombo.getSelectedId();
			this->settings.accompanimentMinor = keyModeCombo.getSelectedId() == 2;
			this->settings.accompanimentKey = selected > 1 ?
				String(Settings::KeyName(selected - 2, this->settings.accompanimentMinor)) : String();
			this->settings.Save();
			fillKeyCombo();
			update();
		};
	addAndMakeVisible(keyCombo);
	addAndMakeVisible(keyModeCombo);

	initButton(resetTempoButton, TRANS("Reset"));
	resetTempoButton.setTooltip(TRANS("Back to the default tempo of the style (R)"));
	initButton(tapTempoButton, TRANS("Tap Tempo"));
	tapTempoButton.setTooltip(TRANS("Press it several times in the tempo you want: the tempo is set from the presses (T)"));

	initLabel(chordNameLabel, "");
	chordNameLabel.setFont(Font(FontOptions(26.00f, Font::bold)));
	chordNameLabel.setJustificationType(Justification::centred);
	chordNameLabel.setMinimumHorizontalScale(0.5f);
	chordNameLabel.setColour(Label::textColourId, Colour(0xffee6c0a)); // the colour of the buttons that are on
	chordNameLabel.setTooltip(TRANS("The chord recognized by the piano"));

	initLabel(chordAreaLabel, TRANS("Chord detection:"));
	initButton(chordFullButton, "Full");
	chordFullButton.setTooltip(TRANS("The chords are recognized on the whole keyboard"));
	initButton(chordLowerButton, "Lower");
	chordLowerButton.setTooltip(TRANS("The chords are recognized below the split point"));
	chordFullButton.setConnectedEdges(Button::ConnectedOnRight);
	chordLowerButton.setConnectedEdges(Button::ConnectedOnLeft);
	initButton(leftSoundButton, TRANS("Left-hand sound"));
	leftSoundButton.setTooltip(TRANS("When on, the keys below the split point sound while the accompaniment is playing; when off, they only give the chords. Without the accompaniment the Left part of the Voice tab decides."));
	initLabel(splitLabel, TRANS("Split point:"));
	initButton(splitDownButton, String(CharPointer_UTF8("\xe2\x97\x84")));
	splitDownButton.setTooltip(TRANS("Split point one key lower"));
	initButton(splitUpButton, String(CharPointer_UTF8("\xe2\x96\xba")));
	splitUpButton.setTooltip(TRANS("Split point one key higher"));
	initLabel(splitNameLabel, "");
	splitNameLabel.setJustificationType(Justification::centred);
	splitNameLabel.setColour(Label::outlineColourId, Colour(0xff4e5b62));
	splitNameLabel.setTooltip(TRANS("The split point of the accompaniment: the chords are played below it. The split point of the Left part is on the Voice tab."));
	initButton(splitLearnButton, TRANS("Learn"));
	splitLearnButton.setTooltip(TRANS("Press it, then a key on the piano or on the MIDI keyboard: that key becomes the split point. Press it again (or Esc) to cancel."));

	initButton(startButton, TRANS("Start"));
	startButton.setTooltip(TRANS("Starts and stops the accompaniment (Space)"));
	initButton(syncStartButton, TRANS("Sync Start"));
	syncStartButton.setTooltip(TRANS("The accompaniment starts with the first chord played on the piano"));

	initLabel(positionLabel, "");
	positionLabel.setJustificationType(Justification::centredRight);

	initLabel(introLabel, TRANS("Intro"));
	initLabel(mainLabel, TRANS("Main"));
	initLabel(endingLabel, TRANS("Ending"));
	for (int i = 0; i < NumIntros; i++)
	{
		initButton(introButtons[i], String(i + 1));
	}
	for (int i = 0; i < NumMains; i++)
	{
		initButton(mainButtons[i], String::charToString((juce_wchar)('A' + i)));
		mainButtons[i].setTooltip(TRANS("Main section (key NUMBER)").replace("NUMBER", String(i + 1)));
	}
	for (int i = 0; i < NumEndings; i++)
	{
		initButton(endingButtons[i], String(i + 1));
	}
	initButton(autoFillButton, TRANS("Auto Fill"));
	autoFillButton.setTooltip(TRANS("When on, a fill in is played before the new main section when the main section is changed (A)"));
	initButton(fillInButton, TRANS("Fill In"));
	fillInButton.setTooltip(TRANS("One measure of fill in, staying in the same main section (F)"));
	initButton(breakButton, TRANS("Break"));
	breakButton.setTooltip(TRANS("One measure of break (B)"));

	initLabel(volumeLabel, TRANS("Accompaniment volume:"));
	volumeSlider.setSliderStyle(Slider::LinearHorizontal);
	volumeSlider.setTextBoxStyle(Slider::TextBoxRight, false, 40, 24);
	volumeSlider.setRange(PianoController::MinVolume, PianoController::MaxVolume, 1);
	volumeSlider.setDoubleClickReturnValue(true, PianoController::DefaultVolume); // 100
	volumeSlider.onValueChange = [this]()
		{
			const int volume = roundToInt(volumeSlider.getValue());
			if (volume != this->pianoController.GetVolume(PianoController::chStyle))
			{
				this->pianoController.SetVolume(PianoController::chStyle, volume);
			}
		};
	addAndMakeVisible(volumeSlider);

	initButton(shortcutsButton, TRANS("Keyboard shortcuts"));
	shortcutsButton.setTooltip(TRANS("Shows the keys that control the accompaniment while this window is active (H)"));

	initLabel(hintLabel, "");
	hintLabel.setJustificationType(Justification::centredRight);
	hintLabel.setMinimumHorizontalScale(0.7f);

	initButton(memoryButton, TRANS("Memory"));
	memoryButton.setTooltip(TRANS("Saves the style, the tempo, the key and the voices of the keyboard: press it, then the number of the memory (or F1 - F8). Press it again (or Esc) to cancel."));
	for (int i = 0; i < NumRegistrations; i++)
	{
		initButton(registrationButtons[i], String(i + 1));
		registrationButtons[i].onMenu = [this, i]() { showRegistrationMenu(i); };
	}
	loadRegistrations();

	setWantsKeyboardFocus(true);
	setSize(440, 666);

	pianoController.AddListener(this);
	update();
}

AccompanimentComponent::~AccompanimentComponent()
{
	stopTimer();
	pianoController.RemoveListener(this);
}

// vertical positions of the lines between the groups of the controls
static const int AccompanimentSeparatorY[] = {204, 258, 340, 392, 524, 568, 620};

void AccompanimentComponent::paint(Graphics& g)
{
	g.fillAll(Colour(0xff323e44));
	g.setColour(Colours::white.withAlpha(0.25f));
	for (int y : AccompanimentSeparatorY)
	{
		g.fillRect(12, y, getWidth() - 24, 1);
	}
	// the frames of the current style name and of the chord
	g.setColour(Colours::white.withAlpha(0.7f));
	g.drawRoundedRectangle(currentNameLabel.getBounds().expanded(0, 3).toFloat().reduced(0.75f), 5.0f, 1.5f);
	g.drawRoundedRectangle(chordFrame.toFloat().reduced(0.75f), 5.0f, 1.5f);
}

void AccompanimentComponent::resized()
{
	// style: lists and Apply, the current style, tempo, chord
	styleLabel.setBounds(16, 16, 70, 24);
	categoryCombo.setBounds(90, 16, 164, 24);
	groupCombo.setBounds(260, 16, 164, 24);
	styleCombo.setBounds(90, 52, 220, 24);
	applyButton.setBounds(316, 50, 108, 28);
	// the frame of the name and the key list start where the style lists do
	currentLabel.setBounds(16, 88, 74, 24);
	currentNameLabel.setBounds(90, 88, 334, 24);
	keyLabel.setBounds(16, 128, 74, 24);
	// wide enough for every name (e.g. "F#", "Bb") at the normal size of the text
	keyCombo.setBounds(90, 128, 76, 24);
	keyModeCombo.setBounds(90 + 76 + 6, 128, 84, 24);
	{
		// The chord in a row of its own, between two lines: its label at the left, and
		// the frame in the middle of the window, wide enough for the longest chord name.
		chordLabel.setBounds(16, 219, 74, 24);
		chordFrame = Rectangle<int>((getWidth() - 260) / 2, 212, 260, 38);
		chordNameLabel.setBounds(chordFrame.reduced(4, 1));
	}
	{
		// the tempo right after its label; the two buttons spread evenly in the rest
		const int textWidth = GlyphArrangement::getStringWidthInt(tempoLabel.getFont(), tempoLabel.getText());
		const int labelWidth = jlimit(40, 120, textWidth + 10);
		tempoLabel.setBounds(16, 170, labelWidth, 24);
		tempoSlider.setBounds(16 + labelWidth, 170, 120, 24);
		const int left = 16 + labelWidth + 120;
		const int right = 424;
		const int buttonWidth = jmin(100, (right - left - 16) / 2);
		const int gap = (right - left - 2 * buttonWidth) / 2;
		resetTempoButton.setBounds(left + gap, 168, buttonWidth, 28);
		tapTempoButton.setBounds(right - buttonWidth, 168, buttonWidth, 28);
	}
	// chord detection: the area and the sound of the left hand, then the split point
	chordAreaLabel.setBounds(16, 270, 138, 24);
	chordFullButton.setBounds(156, 268, 63, 28);
	chordLowerButton.setBounds(219, 268, 63, 28);
	leftSoundButton.setBounds(304, 268, 120, 28);
	splitLabel.setBounds(16, 306, 138, 24);
	splitDownButton.setBounds(156, 304, 30, 28);
	splitNameLabel.setBounds(190, 304, 58, 28);
	splitUpButton.setBounds(252, 304, 30, 28);
	splitLearnButton.setBounds(304, 304, 120, 28);
	// start and stop
	startButton.setBounds(16, 350, 128, 32);
	syncStartButton.setBounds(156, 350, 128, 32);
	positionLabel.setBounds(296, 350, 128, 32);
	// sections
	introLabel.setBounds(16, 402, 70, 32);
	mainLabel.setBounds(16, 442, 70, 32);
	endingLabel.setBounds(16, 482, 70, 32);
	for (int i = 0; i < NumIntros; i++)
	{
		introButtons[i].setBounds(90 + i * 48, 402, 44, 32);
	}
	for (int i = 0; i < NumMains; i++)
	{
		mainButtons[i].setBounds(90 + i * 48, 442, 44, 32);
	}
	for (int i = 0; i < NumEndings; i++)
	{
		endingButtons[i].setBounds(90 + i * 48, 482, 44, 32);
	}
	autoFillButton.setBounds(290, 402, 134, 32);
	fillInButton.setBounds(290, 442, 64, 32);
	breakButton.setBounds(360, 442, 64, 32);
	// volume
	volumeLabel.setBounds(16, 534, 170, 24);
	volumeSlider.setBounds(186, 534, 238, 24);
	// registration memories: Memory in the column of the labels, the numbers spread
	// evenly from the line of the lists to the right edge
	memoryButton.setBounds(16, 578, 70, 32);
	for (int i = 0; i < NumRegistrations; i++)
	{
		registrationButtons[i].setBounds(90 + i * 296 / (NumRegistrations - 1), 578, 38, 32);
	}
	shortcutsButton.setBounds(16, 630, 190, 28);
	hintLabel.setBounds(214, 632, 210, 24);
}

void AccompanimentComponent::buttonClicked(Button* button)
{
	if (button == &startButton)
	{
		pianoController.SetStylePlaying(!pianoController.GetStylePlaying());
	}
	else if (button == &syncStartButton)
	{
		pianoController.SetStyleSyncStart(!pianoController.GetStyleSyncStart());
	}
	else if (button == &fillInButton)
	{
		fillIn();
	}
	else if (button == &breakButton)
	{
		pianoController.PlayStyleSection(PianoController::ssBreak);
	}
	else if (button == &autoFillButton)
	{
		toggleAutoFill();
	}
	else if (button == &applyButton)
	{
		applyStyle();
	}
	else if (button == &tapTempoButton)
	{
		tapTempo();
	}
	else if (button == &resetTempoButton)
	{
		resetTempo();
	}
	else if (button == &shortcutsButton)
	{
		showShortcuts();
	}
	else if (button == &memoryButton)
	{
		setMemoryArmed(!memoryArmed);
	}
	else if (button == &chordFullButton)
	{
		pianoController.SetStyleChordArea(PianoController::caFull);
	}
	else if (button == &chordLowerButton)
	{
		pianoController.SetStyleChordArea(PianoController::caLower);
	}
	else if (button == &leftSoundButton)
	{
		pianoController.SetStyleLeftSound(pianoController.GetStyleLeftSound() != 1);
	}
	else if (button == &splitDownButton)
	{
		stepSplitPoint(-1);
	}
	else if (button == &splitUpButton)
	{
		stepSplitPoint(+1);
	}
	else if (button == &splitLearnButton)
	{
		splitLearning = !splitLearning;
		update();
	}
	else
	{
		for (int i = 0; i < NumRegistrations; i++)
		{
			if (button == &registrationButtons[i])
			{
				registrationPressed(i);
			}
		}
		for (int i = 0; i < NumIntros; i++)
		{
			if (button == &introButtons[i])
			{
				pianoController.SetStyleSection(PianoController::ssIntro1 + i);
			}
		}
		for (int i = 0; i < NumMains; i++)
		{
			if (button == &mainButtons[i])
			{
				changeMain(i);
			}
		}
		for (int i = 0; i < NumEndings; i++)
		{
			if (button == &endingButtons[i])
			{
				pianoController.SetStyleSection(PianoController::ssEnding1 + i);
			}
		}
	}
}

// The fill in of the main section that is playing (or that comes next).
void AccompanimentComponent::fillIn()
{
	auto isMain = [](int section)
		{
			return section >= PianoController::ssMainA && section < PianoController::ssMainA + 4;
		};
	const int current = pianoController.GetStyleSection();
	const int next = pianoController.GetStyleNextSection();
	const int main = isMain(current) ? current : isMain(next) ? next : (int)PianoController::ssMainA;
	// played once, then the same main section goes on
	pianoController.PlayStyleSection(PianoController::ssFillInAA + (main - PianoController::ssMainA));
}

// Main A..D (index 0..3). With Auto Fill, while the accompaniment is playing, the fill
// in of the new main section is played first.
void AccompanimentComponent::changeMain(int index)
{
	const int main = PianoController::ssMainA + index;
	if (settings.accompanimentAutoFill && pianoController.GetStylePlaying() &&
		pianoController.GetStyleSection() != main)
	{
		pianoController.ChangeStyleMainWithFill(main);
	}
	else
	{
		pianoController.SetStyleSection(main);
	}
}

// The tempo from the times between the presses: the average of the last (at most four)
// intervals; a pause of more than two seconds begins a new series.
void AccompanimentComponent::tapTempo()
{
	const double now = Time::getMillisecondCounterHiRes();
	if (!taps.empty() && now - taps.back() > 2000.0)
	{
		taps.clear();
	}
	taps.push_back(now);
	if (taps.size() > 5)
	{
		taps.erase(taps.begin());
	}
	if (taps.size() >= 2)
	{
		const double interval = (taps.back() - taps.front()) / (double)(taps.size() - 1);
		if (interval > 0)
		{
			pianoController.SetStyleTempo(roundToInt(60000.0 / interval));
		}
	}
}

void AccompanimentComponent::showShortcuts()
{
	const String dash = " " + String(CharPointer_UTF8("\xe2\x80\x93")) + " ";
	String text;
	text << TRANS("Space") << dash << TRANS("Start / Stop") << "\n"
		<< "1, 2, 3, 4" << dash << "Main A, B, C, D" << "\n"
		<< "F" << dash << TRANS("Fill In") << "\n"
		<< "A" << dash << TRANS("Auto Fill on / off") << "\n"
		<< "B" << dash << TRANS("Break") << "\n"
		<< "T" << dash << TRANS("Tap Tempo") << "\n"
		<< "R" << dash << TRANS("Default tempo of the style (Reset)") << "\n"
		<< "Enter" << dash << TRANS("Apply: loads the chosen style") << "\n"
		<< "F1 " << dash.trim() << " F8" << dash << TRANS("Registration memory 1 - 8") << "\n"
		<< "H" << dash << TRANS("This list") << "\n\n"
		<< TRANS("The keys work while the Accompaniment window is the active window.");
	AlertWindow::showAsync(MessageBoxOptions()
			.withIconType(MessageBoxIconType::NoIcon)
			.withTitle(TRANS("Keyboard shortcuts"))
			.withMessage(text)
			.withButton(TRANS("OK"))
			.withAssociatedComponent(this),
		[self = Component::SafePointer<Component>(this)](int)
		{
			if (self != nullptr)
			{
				self->grabKeyboardFocus(); // the shortcuts work again at once
			}
		});
}

int AccompanimentComponent::defaultTempo() const
{
	const String path = pianoController.GetStyleName();
	for (const StyleEntry& style : styles)
	{
		if (style.path == path)
		{
			return style.tempo;
		}
	}
	return 0;
}

void AccompanimentComponent::resetTempo()
{
	const int tempo = defaultTempo();
	if (tempo > 0)
	{
		taps.clear();
		pianoController.SetStyleTempo(tempo);
	}
}

void AccompanimentComponent::toggleAutoFill()
{
	settings.accompanimentAutoFill = !settings.accompanimentAutoFill;
	settings.Save();
	update();
}

// Shortcuts while the window is active: Space - start and stop, 1..4 - Main A..D,
// F - fill in, B - break.
bool AccompanimentComponent::keyPressed(const KeyPress& key)
{
	if (key.getModifiers().isAnyModifierKeyDown())
	{
		return false;
	}
	const juce_wchar character = CharacterFunctions::toLowerCase(key.getTextCharacter());
	if (character == 'h')
	{
		showShortcuts();
		return true;
	}
	if (key == KeyPress::escapeKey && (memoryArmed || splitLearning))
	{
		if (memoryArmed)
		{
			setMemoryArmed(false);
		}
		splitLearning = false;
		update();
		return true;
	}
	if (!pianoController.IsConnected())
	{
		return false;
	}
	static const int functionKeys[NumRegistrations] = {
		KeyPress::F1Key, KeyPress::F2Key, KeyPress::F3Key, KeyPress::F4Key,
		KeyPress::F5Key, KeyPress::F6Key, KeyPress::F7Key, KeyPress::F8Key};
	for (int i = 0; i < NumRegistrations; i++)
	{
		if (key == KeyPress(functionKeys[i]))
		{
			registrationPressed(i);
			return true;
		}
	}
	if (key == KeyPress::spaceKey)
	{
		pianoController.SetStylePlaying(!pianoController.GetStylePlaying());
		return true;
	}
	if (character >= '1' && character <= '4')
	{
		changeMain((int)(character - '1'));
		return true;
	}
	if (character == 'a')
	{
		toggleAutoFill();
		return true;
	}
	if (key == KeyPress::returnKey)
	{
		applyStyle();
		return true;
	}
	if (character == 't')
	{
		tapTempo();
		return true;
	}
	if (character == 'r')
	{
		resetTempo();
		return true;
	}
	if (character == 'f')
	{
		fillIn();
		return true;
	}
	if (character == 'b')
	{
		pianoController.PlayStyleSection(PianoController::ssBreak);
		return true;
	}
	return false;
}

// Called from any thread.
void AccompanimentComponent::PianoStateChanged(PianoController::Aspect aspect, PianoController::Channel channel)
{
	if (aspect == PianoController::apStyle || aspect == PianoController::apConnection ||
		aspect == PianoController::apSplitPoint ||
		(aspect == PianoController::apVolume && channel == PianoController::chStyle))
	{
		GuiHelper::CallAsync(this, [this]() { update(); });
	}
}

// Called from any thread. While learning, the first key played (on the piano or on the
// MIDI keyboard) becomes the split point.
void AccompanimentComponent::PianoNoteMessage(const MidiMessage& message)
{
	// the keys of the piano are sent on the channels 1..3; the channels 9..16 are the
	// parts of the accompaniment
	if (splitLearning && message.isNoteOn() && message.getChannel() < 9 && !pianoController.GetStylePlaying())
	{
		const int note = message.getNoteNumber();
		GuiHelper::CallAsync(this, [this, note]()
			{
				if (splitLearning.exchange(false))
				{
					pianoController.SetStyleSplitPoint(note);
					update();
				}
			});
	}
}

String AccompanimentComponent::noteName(int note)
{
	static const char* const names[] = {"C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B"};
	if (note < 0 || note > 127)
	{
		return {};
	}
	return String(names[note % 12]) + String(note / 12 - 2);
}

// One key lower or higher, within the keys of the piano.
void AccompanimentComponent::stepSplitPoint(int delta)
{
	const int current = pianoController.GetStyleSplitPoint();
	const int target = jlimit(22, 108, current + delta); // A#-1 .. C7: a key must stay below it
	if (current > 0 && target != current)
	{
		pianoController.SetStyleSplitPoint(target);
	}
}

void AccompanimentComponent::markButton(TextButton& button, bool current, bool next)
{
	// drawn by the look and feel of the program: "toggle" - filled when on,
	// "live" - white outline
	if (current)
	{
		button.getProperties().set("toggle", true);
	}
	else
	{
		button.getProperties().remove("toggle");
	}
	if (next && !current)
	{
		button.getProperties().set("live", true);
	}
	else
	{
		button.getProperties().remove("live");
	}
	if (button.getToggleState() != current)
	{
		button.setToggleState(current, dontSendNotification);
	}
	button.repaint();
}

void AccompanimentComponent::update()
{
	const bool connected = pianoController.IsConnected();
	const bool playing = connected && pianoController.GetStylePlaying();

	// The lists show the style of the piano when it changes (at the start, after Apply,
	// or when it is changed on the piano); otherwise they can be browsed freely.
	const String path = pianoController.GetStyleName();
	if (pendingStyle.isNotEmpty() && path == pendingStyle)
	{
		// The piano has loaded the style of the recalled registration. It sets the default
		// tempo of the style with it: the saved tempo is sent a little later.
		pendingStyle.clear();
		const int serial = recallSerial;
		Timer::callAfterDelay(500, [this, serial, self = Component::SafePointer<Component>(this)]()
			{
				if (self != nullptr && serial == recallSerial)
				{
					finishRecall();
				}
			});
	}
	if (currentRegistration >= 0 && pendingStyle.isEmpty() && registrations[currentRegistration].style != path)
	{
		currentRegistration = -1; // another style was chosen since
	}
	const String title = styleTitle(path);
	currentNameLabel.setText(title.isEmpty() ? String("-") : title, dontSendNotification);
	if (path != shownStyle && !categoryCombo.isPopupActive() && !groupCombo.isPopupActive() &&
		!styleCombo.isPopupActive())
	{
		shownStyle = path;
		for (int i = 0; i < (int)styles.size(); i++)
		{
			if (styles[i].path == path)
			{
				showStyleInLists(i);
				break;
			}
		}
	}

	if (!tempoSlider.hasKeyboardFocus(true) && !tempoSlider.isMouseButtonDown(true))
	{
		tempoSlider.setValue(pianoController.GetStyleTempo(), dontSendNotification);
	}
	chordNameLabel.setText(connected ? chordName(pianoController.GetStyleChord(), keyAccidentals()) : String(), dontSendNotification);

	startButton.setButtonText(playing ? TRANS("Stop") : TRANS("Start"));
	markButton(startButton, playing, false);
	markButton(syncStartButton, connected && pianoController.GetStyleSyncStart(), false);

	String position;
	if (playing)
	{
		const PianoController::Position where = pianoController.GetStylePosition();
		const String bullet = " " + String(CharPointer_UTF8("\xe2\x80\xa2")) + " ";
		position = TRANS("Measure NUMBER").replace("NUMBER", String(jmax(1, where.measure))) + bullet + String(jmax(1, where.beat));
	}
	positionLabel.setText(position, dontSendNotification);

	const int current = connected ? pianoController.GetStyleSection() : (int)PianoController::ssNone;
	const int next = connected ? pianoController.GetStyleNextSection() : (int)PianoController::ssNone;
	for (int i = 0; i < NumIntros; i++)
	{
		markButton(introButtons[i], current == PianoController::ssIntro1 + i, next == PianoController::ssIntro1 + i);
	}
	for (int i = 0; i < NumMains; i++)
	{
		markButton(mainButtons[i], current == PianoController::ssMainA + i, next == PianoController::ssMainA + i);
	}
	for (int i = 0; i < NumEndings; i++)
	{
		markButton(endingButtons[i], current == PianoController::ssEnding1 + i, next == PianoController::ssEnding1 + i);
	}
	const bool fill = current >= PianoController::ssFillInAA && current < PianoController::ssFillInAA + 4;
	const bool nextFill = next >= PianoController::ssFillInAA && next < PianoController::ssFillInAA + 4;
	markButton(fillInButton, fill, nextFill);
	markButton(autoFillButton, settings.accompanimentAutoFill, false);
	markButton(breakButton, current == PianoController::ssBreak, next == PianoController::ssBreak);

	if (!volumeSlider.isMouseButtonDown(true))
	{
		volumeSlider.setValue(pianoController.GetVolume(PianoController::chStyle), dontSendNotification);
	}

	for (Component* child : getChildren())
	{
		if (dynamic_cast<Label*>(child) == nullptr)
		{
			child->setEnabled(connected);
		}
	}
	const bool hasList = connected && !styles.empty();
	categoryCombo.setEnabled(hasList);
	groupCombo.setEnabled(hasList);
	styleCombo.setEnabled(hasList);
	// Apply is marked while the chosen style is not the style of the piano
	const int chosen = styleCombo.getSelectedId() - 1;
	const bool pending = hasList && chosen >= 0 && chosen < (int)styles.size() && styles[chosen].path != path;
	applyButton.setEnabled(pending);
	const int styleTempo = defaultTempo();
	resetTempoButton.setEnabled(connected && styleTempo > 0 && styleTempo != pianoController.GetStyleTempo());
	markButton(applyButton, false, pending);

	// chord detection
	const int chordArea = connected ? pianoController.GetStyleChordArea() : (int)PianoController::caUnknown;
	markButton(chordFullButton, chordArea == PianoController::caFull, false);
	markButton(chordLowerButton, chordArea == PianoController::caLower, false);
	markButton(leftSoundButton, connected && pianoController.GetStyleLeftSound() == 1, false);
	// not while the accompaniment is playing: its notes come from the piano too, and
	// one of them would be taken for the key
	if (!connected || playing)
	{
		splitLearning = false;
	}
	splitLearnButton.setEnabled(connected && !playing);
	markButton(splitLearnButton, splitLearning, false);
	const int splitPoint = pianoController.GetStyleSplitPoint();
	splitNameLabel.setText(connected && splitPoint > 0 ? noteName(splitPoint) : String("-"), dontSendNotification);

	shortcutsButton.setEnabled(true); // the list can be read without the piano too
	if (!connected && memoryArmed)
	{
		setMemoryArmed(false);
	}
	updateRegistrationButtons();
	hintLabel.setText(connected ? String() : TRANS("The piano is not connected."), dontSendNotification);
	hintLabel.setColour(Label::textColourId, Colours::orange);
}

// The list file: text, one style in a line, the fields separated by semicolons:
// id;path;title;category;group;... (the first line is the header).
void AccompanimentComponent::loadStyles()
{
	styles.clear();
	categories.clear();

	const File file = settings.GetLastStateFile().getSiblingFile("styles.csv");
	if (file.existsAsFile())
	{
		StringArray lines;
		lines.addLines(file.loadFileAsString());
		for (const String& line : lines)
		{
			StringArray fields;
			fields.addTokens(line, ";", "");
			if (fields.size() < 5 || !fields[1].trim().startsWith("PRESET:"))
			{
				continue; // header or not a style
			}
			StyleEntry entry;
			entry.path = fields[1].trim();
			entry.title = fields[2].trim();
			entry.category = fields[3].trim();
			entry.group = fields[4].trim();
			entry.tempo = fields.size() > 6 ? fields[6].getIntValue() : 0;
			if (entry.title.isEmpty())
			{
				entry.title = styleTitle(entry.path);
			}
			styles.push_back(entry);
			categories.addIfNotAlreadyThere(entry.category);
		}
	}

	categoryCombo.clear(dontSendNotification);
	for (int i = 0; i < categories.size(); i++)
	{
		categoryCombo.addItem(categories[i], i + 1);
	}
	groupCombo.clear(dontSendNotification);
	styleCombo.clear(dontSendNotification);
	if (styles.empty())
	{
		const String tip = TRANS("The list of the styles (styles.csv) was not found in the data folder of the program");
		categoryCombo.setTooltip(tip);
		groupCombo.setTooltip(tip);
		styleCombo.setTooltip(tip);
	}
	else
	{
		categoryCombo.setSelectedId(1, dontSendNotification);
		fillGroupCombo();
	}
}

// The groups (subcategories) of the chosen category; the first one becomes chosen, and
// the style list shows its styles.
void AccompanimentComponent::fillGroupCombo()
{
	groups.clear();
	const String category = categories[categoryCombo.getSelectedId() - 1];
	for (const StyleEntry& style : styles)
	{
		if (style.category == category)
		{
			groups.addIfNotAlreadyThere(style.group);
		}
	}
	groupCombo.clear(dontSendNotification);
	for (int i = 0; i < groups.size(); i++)
	{
		groupCombo.addItem(groups[i].isEmpty() ? String("-") : groups[i], i + 1);
	}
	groupCombo.setSelectedId(groups.isEmpty() ? 0 : 1, dontSendNotification);
	fillStyleCombo();
}

// The styles of the chosen group; the first one becomes chosen.
void AccompanimentComponent::fillStyleCombo()
{
	const String category = categories[categoryCombo.getSelectedId() - 1];
	const String group = groups[groupCombo.getSelectedId() - 1];
	styleCombo.clear(dontSendNotification);
	int first = 0;
	for (int i = 0; i < (int)styles.size(); i++)
	{
		if (styles[i].category == category && styles[i].group == group)
		{
			styleCombo.addItem(styles[i].title, i + 1);
			if (first == 0)
			{
				first = i + 1;
			}
		}
	}
	styleCombo.setSelectedId(first, dontSendNotification);
	update();
}

// Chooses the style (index in styles) in the three lists.
void AccompanimentComponent::showStyleInLists(int index)
{
	const StyleEntry& style = styles[index];
	categoryCombo.setSelectedId(categories.indexOf(style.category) + 1, dontSendNotification);
	fillGroupCombo();
	groupCombo.setSelectedId(groups.indexOf(style.group) + 1, dontSendNotification);
	fillStyleCombo();
	styleCombo.setSelectedId(index + 1, dontSendNotification);
}

// Loads the style chosen in the lists on the piano.
void AccompanimentComponent::applyStyle()
{
	const int index = styleCombo.getSelectedId() - 1;
	if (index >= 0 && index < (int)styles.size())
	{
		pianoController.SetStyle(styles[index].path);
	}
}

// The notes of the key list in the chosen mode (major or minor); the chosen key stays
// selected.
void AccompanimentComponent::fillKeyCombo()
{
	keyCombo.clear(dontSendNotification);
	keyCombo.addItem("-", 1);
	int selected = 1;
	for (int i = 0; i < Settings::NumKeys; i++)
	{
		const String name = Settings::KeyName(i, settings.accompanimentMinor);
		keyCombo.addItem(name, i + 2);
		if (settings.accompanimentKey == name)
		{
			selected = i + 2;
		}
	}
	keyCombo.setSelectedId(selected, dontSendNotification);
}

// Sharps or flats of the chosen key (0: no key, or a key without accidentals).
int AccompanimentComponent::keyAccidentals() const
{
	int sharps = 0;
	if (!settings.GetKeySignature(sharps))
	{
		return 0;
	}
	return sharps > 0 ? +1 : sharps < 0 ? -1 : 0;
}

String AccompanimentComponent::chordName(const PianoController::StyleChord& chord, int accidentals)
{
	static const char* const notes[] = {"", "C", "D", "E", "F", "G", "A", "B"};
	static const int semitones[] = {0, 0, 2, 4, 5, 7, 9, 11};
	static const char* const marks[] = {"bbb", "bb", "b", "", "#", "##", "###", ""};
	static const char* const withSharps[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
	static const char* const withFlats[] = {"C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B"};
	static const char* const types[] = {
		"", "6", "maj7", "maj7(#11)", "(9)", "maj7(9)", "6(9)", "aug",
		"m", "m6", "m7", "m7b5", "m(9)", "m7(9)", "m7(11)", "mMaj7",
		"mMaj7(9)", "dim", "dim7", "7", "7sus4", "7b5", "7(9)", "7(#11)",
		"7(13)", "7(b9)", "7(b13)", "7(#9)", "maj7aug", "7aug", "1+8", "1+5",
		"sus4", "1+2+5", ""};
	// value: 0fffnnnn - nnnn: 1 C .. 7 B; fff: 0 bbb, 1 bb, 2 b, 3 natural, 4 #, 5 ##, 6 ###
	auto noteName = [accidentals](int value)
		{
			const int note = value & 0x0f;
			const int mark = (value >> 4) & 0x07;
			if (value >= 0x7f || note < 1 || note > 7)
			{
				return String();
			}
			if (accidentals == 0 || mark > 6)
			{
				return String(notes[note]) + marks[mark];
			}
			const int pitch = ((semitones[note] + (mark - 3)) % 12 + 12) % 12;
			return String(accidentals > 0 ? withSharps[pitch] : withFlats[pitch]);
		};

	const String root = noteName(chord.root);
	if (root.isEmpty())
	{
		return {};
	}
	String name = root;
	if (chord.type >= 0 && chord.type < numElementsInArray(types))
	{
		name += types[chord.type];
	}
	const String bass = noteName(chord.bassRoot);
	if (bass.isNotEmpty() && bass != root)
	{
		name += "/" + bass;
	}
	return name;
}

String AccompanimentComponent::styleTitle(const String& path)
{
	// PRESET:/STYLE/<category>/<group>/<name>.<id>.<extension>
	StringArray parts;
	parts.addTokens(path.replaceCharacter('\\', '/'), "/", "");
	parts.removeEmptyStrings();
	if (parts.isEmpty())
	{
		return {};
	}
	const String name = parts[parts.size() - 1].upToFirstOccurrenceOf(".", false, false);
	const int styleIndex = parts.indexOf("STYLE");
	const String category = styleIndex >= 0 && styleIndex + 2 < parts.size() ? parts[styleIndex + 1] : String();
	return category.isEmpty() ? name : name + " (" + category + ")";
}

//==============================================================================
// Registration memories

static const char* const RegistrationPartNames[] = {"Main", "Layer", "Left"};
static const PianoController::Channel RegistrationPartChannels[] = {
	PianoController::chMain, PianoController::chLayer, PianoController::chLeft};

void AccompanimentComponent::MemoryButton::mouseDown(const MouseEvent& e)
{
	menuClick = e.mods.isPopupMenu();
	if (menuClick)
	{
		if (onMenu)
		{
			onMenu();
		}
		return;
	}
	TextButton::mouseDown(e);
}

void AccompanimentComponent::MemoryButton::mouseDrag(const MouseEvent& e)
{
	if (!menuClick)
	{
		TextButton::mouseDrag(e);
	}
}

void AccompanimentComponent::MemoryButton::mouseUp(const MouseEvent& e)
{
	if (menuClick)
	{
		menuClick = false;
		return;
	}
	TextButton::mouseUp(e);
}

File AccompanimentComponent::registrationsFile() const
{
	return settings.GetLastStateFile().getSiblingFile("Registrations.xml");
}

void AccompanimentComponent::loadRegistrations()
{
	std::unique_ptr<XmlElement> root = XmlDocument::parse(registrationsFile());
	if (!root || !root->hasTagName("ConPianistRegistrations"))
	{
		return;
	}
	for (XmlElement* el : root->getChildWithTagNameIterator("Registration"))
	{
		const int index = el->getIntAttribute("number") - 1;
		if (index < 0 || index >= NumRegistrations)
		{
			continue;
		}
		Registration& reg = registrations[index];
		reg = Registration();
		reg.used = true;
		reg.name = el->getStringAttribute("name");
		reg.style = el->getStringAttribute("style");
		reg.tempo = jlimit((int)PianoController::MinTempo, (int)PianoController::MaxTempo,
			el->getIntAttribute("tempo", PianoController::DefaultTempo));
		reg.styleVolume = jlimit(0, 127, el->getIntAttribute("styleVolume", PianoController::DefaultVolume));
		reg.key = el->getStringAttribute("key");
		reg.minor = el->getBoolAttribute("minor");
		reg.transpose = jlimit((int)PianoController::MinTranspose, (int)PianoController::MaxTranspose, el->getIntAttribute("transpose"));
		reg.keyboardTranspose = jlimit((int)PianoController::MinTranspose, (int)PianoController::MaxTranspose, el->getIntAttribute("keyboardTranspose"));
		reg.splitPoint = el->getIntAttribute("splitPoint");
		reg.styleSplitPoint = el->getIntAttribute("styleSplitPoint");
		reg.chordArea = el->getIntAttribute("chordArea", PianoController::caUnknown);
		reg.leftSound = el->getIntAttribute("leftSound", -1);
		for (XmlElement* partEl : el->getChildWithTagNameIterator("StylePart"))
		{
			const int number = partEl->getIntAttribute("number") - 1;
			if (number >= 0 && number < PianoController::NumStyleParts)
			{
				Registration::StylePart& part = reg.styleParts[number];
				part.active = partEl->getBoolAttribute("active", true);
				part.volume = jlimit(0, 127, partEl->getIntAttribute("volume", PianoController::DefaultVolume));
				part.pan = jlimit((int)PianoController::MinPan, (int)PianoController::MaxPan, partEl->getIntAttribute("pan"));
				part.reverb = jlimit(0, 127, partEl->getIntAttribute("reverb"));
				reg.hasStyleParts = true;
			}
		}
		for (int i = 0; i < 3; i++)
		{
			if (XmlElement* partEl = el->getChildByName(RegistrationPartNames[i]))
			{
				Registration::Part& part = reg.parts[i];
				part.voice = partEl->getStringAttribute("voice");
				part.active = partEl->getBoolAttribute("active");
				part.volume = jlimit(0, 127, partEl->getIntAttribute("volume", PianoController::DefaultVolume));
				part.pan = jlimit((int)PianoController::MinPan, (int)PianoController::MaxPan, partEl->getIntAttribute("pan"));
				part.reverb = jlimit(0, 127, partEl->getIntAttribute("reverb"));
				part.octave = jlimit((int)PianoController::MinOctave, (int)PianoController::MaxOctave, partEl->getIntAttribute("octave"));
			}
		}
	}
}

void AccompanimentComponent::saveRegistrations()
{
	XmlElement root("ConPianistRegistrations");
	for (int index = 0; index < NumRegistrations; index++)
	{
		const Registration& reg = registrations[index];
		if (!reg.used)
		{
			continue;
		}
		XmlElement* el = root.createNewChildElement("Registration");
		el->setAttribute("number", index + 1);
		el->setAttribute("name", reg.name);
		el->setAttribute("style", reg.style);
		el->setAttribute("tempo", reg.tempo);
		el->setAttribute("styleVolume", reg.styleVolume);
		el->setAttribute("key", reg.key);
		el->setAttribute("minor", reg.minor);
		el->setAttribute("transpose", reg.transpose);
		el->setAttribute("keyboardTranspose", reg.keyboardTranspose);
		el->setAttribute("splitPoint", reg.splitPoint);
		el->setAttribute("styleSplitPoint", reg.styleSplitPoint);
		el->setAttribute("chordArea", reg.chordArea);
		el->setAttribute("leftSound", reg.leftSound);
		if (reg.hasStyleParts)
		{
			for (int i = 0; i < PianoController::NumStyleParts; i++)
			{
				const Registration::StylePart& part = reg.styleParts[i];
				XmlElement* partEl = el->createNewChildElement("StylePart");
				partEl->setAttribute("number", i + 1);
				partEl->setAttribute("active", part.active);
				partEl->setAttribute("volume", part.volume);
				partEl->setAttribute("pan", part.pan);
				partEl->setAttribute("reverb", part.reverb);
			}
		}
		for (int i = 0; i < 3; i++)
		{
			const Registration::Part& part = reg.parts[i];
			XmlElement* partEl = el->createNewChildElement(RegistrationPartNames[i]);
			partEl->setAttribute("voice", part.voice);
			partEl->setAttribute("active", part.active);
			partEl->setAttribute("volume", part.volume);
			partEl->setAttribute("pan", part.pan);
			partEl->setAttribute("reverb", part.reverb);
			partEl->setAttribute("octave", part.octave);
		}
	}
	root.writeTo(registrationsFile());
}

// The settings as they are now.
AccompanimentComponent::Registration AccompanimentComponent::captureRegistration() const
{
	Registration reg;
	reg.used = true;
	reg.style = pianoController.GetStyleName();
	reg.tempo = pianoController.GetStyleTempo();
	reg.styleVolume = pianoController.GetVolume(PianoController::chStyle);
	reg.key = settings.accompanimentKey;
	reg.minor = settings.accompanimentMinor;
	reg.transpose = pianoController.GetTranspose();
	reg.keyboardTranspose = pianoController.GetKeyboardTranspose();
	reg.splitPoint = pianoController.GetSplitPoint();
	reg.styleSplitPoint = pianoController.GetStyleSplitPoint();
	reg.chordArea = pianoController.GetStyleChordArea();
	reg.leftSound = pianoController.GetStyleLeftSound();
	reg.hasStyleParts = true;
	for (int i = 0; i < PianoController::NumStyleParts; i++)
	{
		const PianoController::Channel ch = PianoController::StylePartChannel(i);
		Registration::StylePart& part = reg.styleParts[i];
		part.active = pianoController.GetActive(ch);
		part.volume = pianoController.GetVolume(ch);
		part.pan = pianoController.GetPan(ch);
		part.reverb = pianoController.GetReverb(ch);
	}
	for (int i = 0; i < 3; i++)
	{
		const PianoController::Channel ch = RegistrationPartChannels[i];
		Registration::Part& part = reg.parts[i];
		const String voice = pianoController.GetVoice(ch);
		Voice* preset = Presets::FindVoice(voice);
		part.voice = preset ? preset->path : voice;
		part.active = pianoController.GetActive(ch);
		part.volume = pianoController.GetVolume(ch);
		part.pan = pianoController.GetPan(ch);
		part.reverb = pianoController.GetReverb(ch);
		part.octave = pianoController.GetOctave(ch);
	}
	return reg;
}

// A number was pressed (button or F1..F8): saves after Memory, recalls otherwise.
void AccompanimentComponent::registrationPressed(int index)
{
	if (!pianoController.IsConnected())
	{
		return;
	}
	if (memoryArmed)
	{
		setMemoryArmed(false);
		storeRegistration(index);
	}
	else if (registrations[index].used)
	{
		recallRegistration(index);
	}
}

// Asks for the (optional) name, then saves the settings as they were when the number
// was pressed. Cancelling the name window keeps the memory as it was.
void AccompanimentComponent::storeRegistration(int index)
{
	const Registration captured = captureRegistration();
	askRegistrationName(index, TRANS("Save to memory NUMBER").replace("NUMBER", String(index + 1)),
		[this, index, captured](const String& name)
		{
			registrations[index] = captured;
			registrations[index].name = name;
			saveRegistrations();
			currentRegistration = index;
			update();
		});
}

void AccompanimentComponent::askRegistrationName(int index, const String& title, std::function<void(const String&)> done)
{
	AlertWindow* window = new AlertWindow(title,
		registrations[index].used ? TRANS("The memory is in use: saving replaces what is in it.") : String(),
		MessageBoxIconType::NoIcon, this);
	window->addTextEditor("name", registrations[index].name, TRANS("Name (not required):"));
	window->addButton(TRANS("OK"), 1, KeyPress(KeyPress::returnKey));
	window->addButton(TRANS("Cancel"), 0, KeyPress(KeyPress::escapeKey));
	if (TextEditor* editor = window->getTextEditor("name"))
	{
		editor->setSelectAllWhenFocused(true);
		editor->setInputRestrictions(40);
	}
	window->enterModalState(true, ModalCallbackFunction::create(
		[window, done, self = Component::SafePointer<Component>(this)](int result)
		{
			if (self != nullptr)
			{
				if (result == 1)
				{
					done(window->getTextEditorContents("name").trim());
				}
				self->grabKeyboardFocus(); // the shortcuts work again at once
			}
		}), true);
	if (TextEditor* editor = window->getTextEditor("name"))
	{
		editor->grabKeyboardFocus(); // the name can be typed at once
	}
}

// Sends what differs from the current state of the piano. The style comes last: the
// piano sets the default tempo of a style when it loads it, so the saved tempo (and the
// volume of the accompaniment) is sent when the piano reports the new style.
void AccompanimentComponent::recallRegistration(int index)
{
	const Registration& reg = registrations[index];
	if (!reg.used || !pianoController.IsConnected())
	{
		return;
	}

	for (int i = 0; i < 3; i++)
	{
		const PianoController::Channel ch = RegistrationPartChannels[i];
		const Registration::Part& part = reg.parts[i];
		const String voice = pianoController.GetVoice(ch);
		Voice* preset = Presets::FindVoice(voice);
		if (part.voice.isNotEmpty() && part.voice != (preset ? preset->path : voice))
		{
			pianoController.SetVoice(ch, part.voice);
			pianoController.SetOctave(ch, part.octave); // a new voice may bring its own octave
		}
		else if (part.octave != pianoController.GetOctave(ch))
		{
			pianoController.SetOctave(ch, part.octave);
		}
		if (part.volume != pianoController.GetVolume(ch))
		{
			pianoController.SetVolume(ch, part.volume);
		}
		if (part.pan != pianoController.GetPan(ch))
		{
			pianoController.SetPan(ch, part.pan);
		}
		if (part.reverb != pianoController.GetReverb(ch))
		{
			pianoController.SetReverb(ch, part.reverb);
		}
		if (part.active != pianoController.GetActive(ch))
		{
			pianoController.SetActive(ch, part.active);
		}
	}
	// The piano keeps the split point of the accompaniment at or below the one of the Left
	// part, moving one with the other: if either differs, both are sent, the Left one first.
	const bool leftSplitDiffers = reg.splitPoint > 0 && reg.splitPoint != pianoController.GetSplitPoint();
	const bool styleSplitDiffers = reg.styleSplitPoint > 0 && reg.styleSplitPoint != pianoController.GetStyleSplitPoint();
	if (leftSplitDiffers || styleSplitDiffers)
	{
		if (reg.splitPoint > 0)
		{
			pianoController.SetSplitPoint(reg.splitPoint);
		}
		if (reg.styleSplitPoint > 0)
		{
			pianoController.SetStyleSplitPoint(reg.styleSplitPoint);
		}
	}
	if (reg.transpose != pianoController.GetTranspose())
	{
		pianoController.SetTranspose(reg.transpose);
	}
	if (reg.keyboardTranspose != pianoController.GetKeyboardTranspose())
	{
		pianoController.SetKeyboardTranspose(reg.keyboardTranspose);
	}
	if (reg.chordArea != PianoController::caUnknown && reg.chordArea != pianoController.GetStyleChordArea())
	{
		pianoController.SetStyleChordArea(reg.chordArea);
	}
	if (reg.leftSound >= 0 && reg.leftSound != pianoController.GetStyleLeftSound())
	{
		pianoController.SetStyleLeftSound(reg.leftSound == 1);
	}

	if (settings.accompanimentKey != reg.key || settings.accompanimentMinor != reg.minor)
	{
		settings.accompanimentKey = reg.key;
		settings.accompanimentMinor = reg.minor;
		settings.Save();
		keyModeCombo.setSelectedId(reg.minor ? 2 : 1, dontSendNotification);
		fillKeyCombo();
	}

	recallSerial++;
	pendingTempo = reg.tempo;
	pendingVolume = reg.styleVolume;
	pendingRegistration = index;
	pendingStyleChanged = reg.style.isNotEmpty() && reg.style != pianoController.GetStyleName();
	currentRegistration = index;
	taps.clear();
	if (pendingStyleChanged)
	{
		pendingStyle = reg.style;
		pianoController.SetStyle(reg.style);
		// if the piano does not report the style (e.g. it does not know it), the rest
		// is sent anyway
		const int serial = recallSerial;
		Timer::callAfterDelay(3000, [this, serial, self = Component::SafePointer<Component>(this)]()
			{
				if (self != nullptr && serial == recallSerial && pendingStyle.isNotEmpty())
				{
					pendingStyle.clear();
					finishRecall();
				}
			});
	}
	else
	{
		pendingStyle.clear();
		finishRecall();
	}
	update();
}

// The tempo and the volume of the accompaniment of the recalled registration.
void AccompanimentComponent::finishRecall()
{
	if (!pianoController.IsConnected())
	{
		return;
	}
	if (pendingTempo > 0 && pendingTempo != pianoController.GetStyleTempo())
	{
		pianoController.SetStyleTempo(pendingTempo);
	}
	if (pendingVolume != pianoController.GetVolume(PianoController::chStyle))
	{
		pianoController.SetVolume(PianoController::chStyle, pendingVolume);
	}
	// The parts of the accompaniment. A new style has brought its own settings, which
	// may not be known here yet: then everything is sent, otherwise what differs.
	if (pendingRegistration >= 0 && registrations[pendingRegistration].used &&
		registrations[pendingRegistration].hasStyleParts)
	{
		const Registration& reg = registrations[pendingRegistration];
		for (int i = 0; i < PianoController::NumStyleParts; i++)
		{
			const PianoController::Channel ch = PianoController::StylePartChannel(i);
			const Registration::StylePart& part = reg.styleParts[i];
			if (pendingStyleChanged || part.volume != pianoController.GetVolume(ch))
			{
				pianoController.SetVolume(ch, part.volume);
			}
			if (pendingStyleChanged || part.pan != pianoController.GetPan(ch))
			{
				pianoController.SetPan(ch, part.pan);
			}
			if (pendingStyleChanged || part.reverb != pianoController.GetReverb(ch))
			{
				pianoController.SetReverb(ch, part.reverb);
			}
			if (pendingStyleChanged || part.active != pianoController.GetActive(ch))
			{
				pianoController.SetActive(ch, part.active);
			}
		}
	}
	pendingRegistration = -1;
}

// Menu of a number button (right mouse button): rename and delete.
void AccompanimentComponent::showRegistrationMenu(int index)
{
	if (!registrations[index].used)
	{
		return;
	}
	PopupMenu menu;
	menu.addItem(1, TRANS("Rename..."));
	menu.addItem(2, TRANS("Delete"));
	menu.showMenuAsync(PopupMenu::Options().withTargetComponent(&registrationButtons[index]),
		[this, index, self = Component::SafePointer<Component>(this)](int result)
		{
			if (self == nullptr)
			{
				return;
			}
			if (result == 1)
			{
				askRegistrationName(index, TRANS("Name of memory NUMBER").replace("NUMBER", String(index + 1)),
					[this, index](const String& name)
					{
						registrations[index].name = name;
						saveRegistrations();
						update();
					});
			}
			else if (result == 2)
			{
				registrations[index] = Registration();
				if (currentRegistration == index)
				{
					currentRegistration = -1;
				}
				saveRegistrations();
				update();
			}
		});
}

// What is in the memory, shown as the tooltip of its button.
String AccompanimentComponent::registrationTooltip(int index) const
{
	const Registration& reg = registrations[index];
	const String dash = " " + String(CharPointer_UTF8("\xe2\x80\x93")) + " ";
	const String number = String(index + 1);
	const String shortcut = " (F" + number + ")";
	if (!reg.used)
	{
		return TRANS("Memory NUMBER").replace("NUMBER", number) + shortcut + dash + TRANS("empty") + "\n" +
			TRANS("To save: Memory, then this button");
	}
	String text = (reg.name.isNotEmpty() ? number + ". " + reg.name : TRANS("Memory NUMBER").replace("NUMBER", number)) + shortcut + "\n";
	text << TRANS("Style:") << " " << styleTitle(reg.style) << "\n";
	text << TRANS("Tempo:") << " " << reg.tempo;
	if (reg.key.isNotEmpty())
	{
		text << "    " << TRANS("Key:") << " " << (reg.minor ? TRANS("KEY minor") : TRANS("KEY major")).replace("KEY", reg.key);
	}
	text << "\n";
	for (int i = 0; i < 3; i++)
	{
		const Registration::Part& part = reg.parts[i];
		String title = Presets::VoiceTitle(part.voice);
		if (title.isEmpty())
		{
			title = "-";
		}
		text << RegistrationPartNames[i] << ": " << title;
		if (!part.active)
		{
			text << " (" << TRANS("Off").toLowerCase() << ")";
		}
		text << "\n";
	}
	if (reg.chordArea == PianoController::caFull || reg.chordArea == PianoController::caLower)
	{
		text << TRANS("Chord detection:") << " " << (reg.chordArea == PianoController::caFull ? "Full" : "Lower");
		if (reg.leftSound >= 0)
		{
			text << ", " << TRANS("Left-hand sound").toLowerCase() << " " << (reg.leftSound == 1 ? TRANS("On") : TRANS("Off")).toLowerCase();
		}
		text << "\n";
	}
	auto signedNumber = [](int value) { return (value > 0 ? "+" : "") + String(value); };
	if (reg.transpose != 0)
	{
		text << TRANS("Transpose") << ": " << signedNumber(reg.transpose) << "\n";
	}
	if (reg.keyboardTranspose != 0)
	{
		text << TRANS("Transpose") << " (Piano Room): " << signedNumber(reg.keyboardTranspose) << "\n";
	}
	return text.trimEnd();
}

// After Memory the button blinks until a number is pressed (or it is cancelled).
void AccompanimentComponent::setMemoryArmed(bool armed)
{
	memoryArmed = armed;
	memoryBlink = armed;
	if (armed)
	{
		startTimer(400);
	}
	else
	{
		stopTimer();
	}
	updateRegistrationButtons();
}

void AccompanimentComponent::timerCallback()
{
	memoryBlink = !memoryBlink;
	markButton(memoryButton, memoryArmed && memoryBlink, false);
}

// The numbers: the ones in use framed, the current one marked, the empty ones faint. They stay usable without
// the piano too (tooltip, renaming, deleting); only saving and recalling need it.
void AccompanimentComponent::updateRegistrationButtons()
{
	markButton(memoryButton, memoryArmed && memoryBlink, false);
	for (int i = 0; i < NumRegistrations; i++)
	{
		MemoryButton& button = registrationButtons[i];
		button.setEnabled(true);
		button.setAlpha(registrations[i].used || memoryArmed ? 1.0f : 0.45f);
		markButton(button, i == currentRegistration && registrations[i].used, false);
		// a white frame around the memories that are in use (the current one too)
		if (registrations[i].used)
		{
			button.getProperties().set("live", true);
			button.repaint();
		}
		button.setTooltip(registrationTooltip(i));
	}
}

//==============================================================================

AccompanimentWindow::AccompanimentWindow(Settings& settings, PianoController& pianoController) :
	DocumentWindow(TRANS("Accompaniment"), Colour(0xff323e44),
		DocumentWindow::minimiseButton | DocumentWindow::closeButton),
	settings(settings)
{
	const bool usingNativeTitleBar = (SystemStats::getOperatingSystemType() & SystemStats::Windows) ||
		(SystemStats::getOperatingSystemType() & SystemStats::MacOSX);
	setUsingNativeTitleBar(usingNativeTitleBar);
	setContentOwned(new AccompanimentComponent(settings, pianoController), true);
	setResizable(false, false);
	setAlwaysOnTop(true);
}

void AccompanimentWindow::closeButtonPressed()
{
	setVisible(false);
}

// The position is remembered (saved with the settings when the program exits).
void AccompanimentWindow::moved()
{
	DocumentWindow::moved();
	if (isShowing() && !isMinimised() && getX() > -10000 && getY() > -10000)
	{
		settings.accompanimentWindowPos = getPosition();
	}
}

bool AccompanimentWindow::RestorePosition()
{
	if (!Settings::IsWindowPosUsable(settings.accompanimentWindowPos, getWidth()))
	{
		return false;
	}
	setTopLeftPosition(settings.accompanimentWindowPos);
	return true;
}
