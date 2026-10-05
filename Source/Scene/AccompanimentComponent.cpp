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

	initLabel(currentLabel, TRANS("Current style:"));
	initLabel(currentNameLabel, "");
	currentNameLabel.setMinimumHorizontalScale(0.7f);
	// bold, in the colour of the chord, on a dark field (see paint)
	currentNameLabel.setFont(Font(FontOptions(15.00f, Font::bold)));
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

	setWantsKeyboardFocus(true);
	setSize(440, 536);

	pianoController.AddListener(this);
	update();
}

AccompanimentComponent::~AccompanimentComponent()
{
	pianoController.RemoveListener(this);
}

// vertical positions of the lines between the groups of the controls
static const int AccompanimentSeparatorY[] = {204, 258, 310, 442, 486};

void AccompanimentComponent::paint(Graphics& g)
{
	g.fillAll(Colour(0xff323e44));
	g.setColour(Colours::white.withAlpha(0.25f));
	for (int y : AccompanimentSeparatorY)
	{
		g.fillRect(12, y, getWidth() - 24, 1);
	}
	// the name of the current style on a dark field, like a display
	g.setColour(Colour(0xff171d20));
	g.fillRoundedRectangle(currentNameLabel.getBounds().expanded(0, 3).toFloat(), 5.0f);
	// the frame of the chord
	g.setColour(Colours::white.withAlpha(0.7f));
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
	{
		// the name right after the text of the label
		const int textWidth = GlyphArrangement::getStringWidthInt(currentLabel.getFont(), currentLabel.getText());
		const int labelWidth = jlimit(60, 200, textWidth + 10);
		currentLabel.setBounds(16, 88, labelWidth, 24);
		currentNameLabel.setBounds(16 + labelWidth + 4, 88, 408 - labelWidth - 4, 24);
	}
	{
		// the key list right after its label, then major or minor
		auto widthOf = [](const Label& label)
			{
				return GlyphArrangement::getStringWidthInt(label.getFont(), label.getText()) + 10;
			};
		const int keyWidth = jlimit(30, 90, widthOf(keyLabel));
		keyLabel.setBounds(16, 128, keyWidth, 24);
		// wide enough for every name (e.g. "F#", "Bb") at the normal size of the text
		keyCombo.setBounds(16 + keyWidth, 128, 76, 24);
		keyModeCombo.setBounds(16 + keyWidth + 76 + 6, 128, 84, 24);
	}
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
	// start and stop
	startButton.setBounds(16, 268, 128, 32);
	syncStartButton.setBounds(156, 268, 128, 32);
	positionLabel.setBounds(296, 268, 128, 32);
	// sections
	introLabel.setBounds(16, 320, 70, 32);
	mainLabel.setBounds(16, 360, 70, 32);
	endingLabel.setBounds(16, 400, 70, 32);
	for (int i = 0; i < NumIntros; i++)
	{
		introButtons[i].setBounds(90 + i * 48, 320, 44, 32);
	}
	for (int i = 0; i < NumMains; i++)
	{
		mainButtons[i].setBounds(90 + i * 48, 360, 44, 32);
	}
	for (int i = 0; i < NumEndings; i++)
	{
		endingButtons[i].setBounds(90 + i * 48, 400, 44, 32);
	}
	autoFillButton.setBounds(290, 320, 134, 32);
	fillInButton.setBounds(290, 360, 64, 32);
	breakButton.setBounds(360, 360, 64, 32);
	// volume
	volumeLabel.setBounds(16, 452, 170, 24);
	volumeSlider.setBounds(186, 452, 238, 24);
	shortcutsButton.setBounds(16, 496, 190, 28);
	hintLabel.setBounds(214, 498, 210, 24);
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
	else
	{
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
	if (!pianoController.IsConnected())
	{
		return false;
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
		(aspect == PianoController::apVolume && channel == PianoController::chStyle))
	{
		GuiHelper::CallAsync(this, [this]() { update(); });
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

	shortcutsButton.setEnabled(true); // the list can be read without the piano too
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
