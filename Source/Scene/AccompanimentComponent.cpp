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
	categoryCombo.onChange = [this]()
		{
			const int index = categoryCombo.getSelectedId() - 1;
			if (index >= 0 && index < categories.size())
			{
				fillStyleCombo(categories[index]);
			}
		};
	addAndMakeVisible(categoryCombo);

	styleCombo.setWantsKeyboardFocus(false);
	styleCombo.onChange = [this]()
		{
			const int index = styleCombo.getSelectedId() - 1;
			if (index >= 0 && index < (int)styles.size() &&
				styles[index].path != this->pianoController.GetStyleName())
			{
				this->pianoController.SetStyle(styles[index].path);
			}
		};
	addAndMakeVisible(styleCombo);

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

	initLabel(chordLabel, TRANS("Chord:"));
	initLabel(chordNameLabel, "");
	chordNameLabel.setFont(Font(FontOptions(18.00f, Font::bold)));
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
	volumeSlider.onValueChange = [this]()
		{
			const int volume = roundToInt(volumeSlider.getValue());
			if (volume != this->pianoController.GetVolume(PianoController::chStyle))
			{
				this->pianoController.SetVolume(PianoController::chStyle, volume);
			}
		};
	addAndMakeVisible(volumeSlider);

	initLabel(hintLabel, "");
	hintLabel.setJustificationType(Justification::centred);
	hintLabel.setMinimumHorizontalScale(0.7f);

	setWantsKeyboardFocus(true);
	setSize(440, 364);

	pianoController.AddListener(this);
	update();
}

AccompanimentComponent::~AccompanimentComponent()
{
	pianoController.RemoveListener(this);
}

// vertical positions of the lines between the groups of the controls
static const int AccompanimentSeparatorY[] = {86, 138, 270, 314};

void AccompanimentComponent::paint(Graphics& g)
{
	g.fillAll(Colour(0xff323e44));
	g.setColour(Colours::white.withAlpha(0.25f));
	for (int y : AccompanimentSeparatorY)
	{
		g.fillRect(12, y, getWidth() - 24, 1);
	}
}

void AccompanimentComponent::resized()
{
	// style, tempo, chord
	styleLabel.setBounds(16, 16, 70, 24);
	categoryCombo.setBounds(90, 16, 150, 24);
	styleCombo.setBounds(246, 16, 178, 24);
	tempoLabel.setBounds(16, 52, 70, 24);
	tempoSlider.setBounds(90, 52, 120, 24);
	chordLabel.setBounds(234, 52, 66, 24);
	chordNameLabel.setBounds(300, 52, 124, 24);
	// start and stop
	startButton.setBounds(16, 96, 128, 32);
	syncStartButton.setBounds(156, 96, 128, 32);
	positionLabel.setBounds(296, 96, 128, 32);
	// sections
	introLabel.setBounds(16, 148, 70, 32);
	mainLabel.setBounds(16, 188, 70, 32);
	endingLabel.setBounds(16, 228, 70, 32);
	for (int i = 0; i < NumIntros; i++)
	{
		introButtons[i].setBounds(90 + i * 48, 148, 44, 32);
	}
	for (int i = 0; i < NumMains; i++)
	{
		mainButtons[i].setBounds(90 + i * 48, 188, 44, 32);
	}
	for (int i = 0; i < NumEndings; i++)
	{
		endingButtons[i].setBounds(90 + i * 48, 228, 44, 32);
	}
	autoFillButton.setBounds(290, 148, 134, 32);
	fillInButton.setBounds(290, 188, 64, 32);
	breakButton.setBounds(360, 188, 64, 32);
	// volume
	volumeLabel.setBounds(16, 280, 170, 24);
	volumeSlider.setBounds(186, 280, 238, 24);
	hintLabel.setBounds(16, 326, 408, 24);
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
	if (!pianoController.IsConnected() || key.getModifiers().isAnyModifierKeyDown())
	{
		return false;
	}
	const juce_wchar character = CharacterFunctions::toLowerCase(key.getTextCharacter());
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

	// the style of the piano in the lists; a style that is not in the list file (or
	// without the file) is shown by its name only
	// (only when the style of the piano changes, so another category can be looked at
	// in the lists meanwhile)
	const String path = pianoController.GetStyleName();
	if (path != shownStyle && !categoryCombo.isPopupActive() && !styleCombo.isPopupActive())
	{
		shownStyle = path;
		int found = -1;
		for (int i = 0; i < (int)styles.size(); i++)
		{
			if (styles[i].path == path)
			{
				found = i;
				break;
			}
		}
		if (found >= 0)
		{
			const int category = categories.indexOf(styles[found].category);
			if (categoryCombo.getSelectedId() != category + 1)
			{
				categoryCombo.setSelectedId(category + 1, dontSendNotification);
				fillStyleCombo(styles[found].category);
			}
			styleCombo.setSelectedId(found + 1, dontSendNotification);
		}
		else
		{
			const String title = styleTitle(path);
			styleCombo.setTextWhenNothingSelected(title.isEmpty() ? String("-") : title);
			styleCombo.setSelectedId(0, dontSendNotification);
		}
	}

	if (!tempoSlider.hasKeyboardFocus(true) && !tempoSlider.isMouseButtonDown(true))
	{
		tempoSlider.setValue(pianoController.GetStyleTempo(), dontSendNotification);
	}
	chordNameLabel.setText(connected ? chordName(pianoController.GetStyleChord()) : String(), dontSendNotification);

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
	categoryCombo.setEnabled(connected && !styles.empty());
	styleCombo.setEnabled(connected && !styles.empty());

	hintLabel.setText(connected ?
		TRANS("Keys: Space - start and stop, 1-4 - Main A-D, F - Fill In, A - Auto Fill, B - Break") :
		TRANS("The accompaniment is played by the piano: the piano is not connected."),
		dontSendNotification);
	hintLabel.setColour(Label::textColourId, connected ? Colours::white.withAlpha(0.6f) : Colours::orange);
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
	styleCombo.clear(dontSendNotification);
	if (styles.empty())
	{
		const String tip = TRANS("The list of the styles (styles.csv) was not found in the data folder of the program");
		categoryCombo.setTooltip(tip);
		styleCombo.setTooltip(tip);
	}
}

// The styles of a category, under the headings of their groups.
void AccompanimentComponent::fillStyleCombo(const String& category)
{
	styleCombo.clear(dontSendNotification);
	String group;
	for (int i = 0; i < (int)styles.size(); i++)
	{
		if (styles[i].category == category)
		{
			if (styles[i].group != group)
			{
				group = styles[i].group;
				if (group.isNotEmpty())
				{
					styleCombo.addSectionHeading(group);
				}
			}
			styleCombo.addItem(styles[i].title, i + 1);
		}
	}
}

String AccompanimentComponent::chordName(const PianoController::StyleChord& chord)
{
	static const char* const notes[] = {"", "C", "D", "E", "F", "G", "A", "B"};
	static const char* const accidentals[] = {"bbb", "bb", "b", "", "#", "##", "###", ""};
	static const char* const types[] = {
		"", "6", "maj7", "maj7(#11)", "(9)", "maj7(9)", "6(9)", "aug",
		"m", "m6", "m7", "m7b5", "m(9)", "m7(9)", "m7(11)", "mMaj7",
		"mMaj7(9)", "dim", "dim7", "7", "7sus4", "7b5", "7(9)", "7(#11)",
		"7(13)", "7(b9)", "7(b13)", "7(#9)", "maj7aug", "7aug", "1+8", "1+5",
		"sus4", "1+2+5", ""};
	auto noteName = [](int value)
		{
			const int note = value & 0x0f;
			if (value >= 0x7f || note < 1 || note > 7)
			{
				return String();
			}
			return String(notes[note]) + accidentals[(value >> 4) & 0x07];
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
	DocumentWindow(TRANS("Accompaniment"), Colour(0xff323e44), DocumentWindow::closeButton)
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
