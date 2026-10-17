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

#include "OtsComponent.h"
#include "AccompanimentComponent.h"
#include "GuiHelper.h"
#include "Presets.h"

// the colours of the window
static const Colour BackgroundColour(0xff323e44);
static const Colour ButtonColour(0xff4e5b62);        // the grey of the buttons
static const Colour ButtonOverColour(0xff5d6b73);    // under the mouse
static const Colour ButtonDownColour(0xff3d484e);    // pressed
static const Colour OrangeColour(0xffee6c0a);        // the colour of the buttons that are on
static const Colour ActiveColour(0xfffaee46);        // the active own setting: lemon yellow
static const Colour ModifiedColour(0xffe53935);      // the frame of a modified setting

// Spacing of the window: 4 pixels between the elements.
static const int Gap = 4;
static const int Margin = 16;
static const int ContentWidth = 420;
static const int SlotWidth = (ContentWidth - 2 * Margin - 3 * Gap) / 4; // 94
static const int SlotHeight = 30;
static const int SeparatorY = 182;
// the factory settings
static const int FactoryLabelY = 190;
static const int LinkY = 218;
static const int LinkWidth = 145;      // 3/4 of two buttons
static const int FactorySlotsY = 252;
static const int CopyY = 286;
static const int Separator2Y = 326;
static const int PartsY = 338;
static const int PartHeight = 104; // a part with the space after it
static const int LabelWidth = 70;

const PianoController::Channel OtsComponent::PartChannels[OneTouchSettings::NumParts] = {
	PianoController::chLeft, PianoController::chMain, PianoController::chLayer};

//==============================================================================

static void FillButton(Graphics& g, Button& button, Colour fill)
{
	const Rectangle<float> r = button.getLocalBounds().toFloat();
	g.setColour(fill.withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.5f));
	g.fillRoundedRectangle(r, 5.0f);
}

static Colour GreyOf(Button& button, bool highlighted, bool down)
{
	return !button.isEnabled() ? ButtonColour : down ? ButtonDownColour :
		highlighted ? ButtonOverColour : ButtonColour;
}

static void DrawText(Graphics& g, TextButton& button, Colour colour)
{
	g.setFont(Font(FontOptions(15.0f, Font::plain)));
	g.setColour(colour.withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.4f));
	g.drawFittedText(button.getButtonText(), button.getLocalBounds().reduced(4, 2), Justification::centred, 1);
}

void OtsComponent::FilledButton::paintButton(Graphics& g, bool highlighted, bool down)
{
	FillButton(g, *this, blinkOn && isEnabled() ? OrangeColour : GreyOf(*this, highlighted, down));
	DrawText(g, *this, Colours::white);
}

void OtsComponent::SlotButton::paintButton(Graphics& g, bool highlighted, bool down)
{
	const bool lit = active && isEnabled();
	FillButton(g, *this, lit ? (down ? ActiveColour.darker(0.2f) : ActiveColour) : GreyOf(*this, highlighted, down));
	if (used || (active && modified))
	{
		// a frame of 3 pixels inside the button
		const float width = 3.0f;
		g.setColour((active && modified ? ModifiedColour : Colours::white)
			.withMultipliedAlpha(isEnabled() ? 1.0f : 0.5f));
		g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(width / 2), 5.0f, width);
	}
	DrawText(g, *this, lit ? Colours::black : Colours::white);
}

void OtsComponent::FactorySlotButton::paintButton(Graphics& g, bool highlighted, bool down)
{
	const bool lit = active && isEnabled();
	FillButton(g, *this, lit ? (down ? OrangeColour.darker(0.2f) : OrangeColour) : GreyOf(*this, highlighted, down));
	if (used)
	{
		const float width = 3.0f;
		g.setColour((modified ? ModifiedColour : Colours::white).withMultipliedAlpha(isEnabled() ? 1.0f : 0.5f));
		g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(width / 2), 5.0f, width);
	}
	DrawText(g, *this, lit ? Colours::black : Colours::white);
}

//==============================================================================

OtsComponent::OtsComponent(Settings& settings, PianoController& pianoController, OneTouchSettings& ots) :
	settings(settings), pianoController(pianoController), ots(ots)
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
			addAndMakeVisible(button);
		};

	initLabel(styleLabel, TRANS("Style"));
	initLabel(styleNameLabel, "");
	styleNameLabel.setFont(Font(FontOptions(15.00f, Font::bold)));
	styleNameLabel.setColour(Label::textColourId, OrangeColour);
	styleNameLabel.setMinimumHorizontalScale(0.7f);

	initLabel(ownLabel, TRANS("Own OTS"));
	ownLabel.setFont(Font(FontOptions(15.00f, Font::bold)));

	for (int i = 0; i < OneTouchSettings::NumSettings; i++)
	{
		initButton(slotButtons[i], String(i + 1));
		slotButtons[i].onClick = [this, i]() { slotClicked(i); };
	}

	initButton(storeButton, TRANS("Store"));
	storeButton.setTooltip(TRANS("Press it, then an own button: the voice settings are stored there. Press it again (or Esc) to cancel. Shift+click on a button stores at once."));
	storeButton.onClick = [this]() { setMode(mode == Mode::store ? Mode::none : Mode::store); };
	initButton(deleteButton, TRANS("Delete"));
	deleteButton.setTooltip(TRANS("Press it, then an own button: it is emptied. Press it again (or Esc) to cancel. Ctrl+Shift+click on a button deletes it (after asking)."));
	deleteButton.onClick = [this]() { setMode(mode == Mode::erase ? Mode::none : Mode::erase); };
	initButton(saveButton, TRANS("Save"));
	saveButton.setTooltip(TRANS("Saves the changed voice settings into the active OTS"));
	saveButton.onClick = [this]() { setMode(Mode::none); this->ots.SaveActive(); };
	initButton(discardButton, TRANS("Discard"));
	discardButton.setTooltip(TRANS("Discards the changes: the active OTS is recalled again"));
	discardButton.onClick = [this]() { setMode(Mode::none); this->ots.DiscardActive(); };

	initLabel(factoryLabel, TRANS("Factory OTS"));
	factoryLabel.setFont(Font(FontOptions(15.00f, Font::bold)));
	initButton(linkButton, "OTS Link");
	linkButton.getProperties().set("toggle", true); // drawn by the look and feel: orange when on
	linkButton.setTooltip(TRANS("OTS Link: Main A-D recall the factory OTS 1-4, also when the style is changed"));
	linkButton.onClick = [this]()
		{
			this->settings.otsLink = !this->settings.otsLink;
			this->ots.SetLink(this->settings.otsLink);
			update();
			grabKeyboardFocus();
		};
	initButton(factorySaveButton, TRANS("Save"));
	factorySaveButton.setTooltip(TRANS("Saves the changed voice settings into the active factory OTS (the original factory data is lost)"));
	factorySaveButton.onClick = [this]() { setMode(Mode::none); askSaveFactory(); };
	initButton(factoryDiscardButton, TRANS("Discard"));
	factoryDiscardButton.setTooltip(TRANS("Discards the changes: the original factory OTS is recalled again"));
	factoryDiscardButton.onClick = [this]()
		{
			setMode(Mode::none);
			this->ots.DiscardActiveFactory();
			grabKeyboardFocus();
		};
	for (int i = 0; i < OneTouchSettings::NumFactorySettings; i++)
	{
		initButton(factoryButtons[i], String(i + 1));
		factoryButtons[i].onClick = [this, i]()
			{
				setMode(Mode::none);
				this->ots.RecallFactory(i);
				grabKeyboardFocus();
			};
	}
	initButton(copyFirstButton, TRANS("Copy to 1-4"));
	copyFirstButton.setTooltip(TRANS("Copies the factory OTS to the own buttons 1-4 (to change them there)"));
	copyFirstButton.onClick = [this]() { setMode(Mode::none); askCopy(0); };
	initButton(copySecondButton, TRANS("Copy to 5-8"));
	copySecondButton.setTooltip(TRANS("Copies the factory OTS to the own buttons 5-8 (to change them there)"));
	copySecondButton.onClick = [this]() { setMode(Mode::none); askCopy(4); };

	for (int i = 0; i < OneTouchSettings::NumParts; i++)
	{
		PartControls& part = parts[i];
		const PianoController::Channel ch = PartChannels[i];
		// the names of the parts as on the Voice tab
		const char* name = ch == PianoController::chLeft ? "Left" : ch == PianoController::chMain ? "Main" : "Layer";
		initButton(part.onButton, TRANS(name));
		part.onButton.getProperties().set("toggle", true); // drawn by the look and feel: orange when on
		part.onButton.setTooltip(TRANS("Switches the part on and off"));
		part.onButton.onClick = [this, ch]()
			{
				this->pianoController.SetActive(ch, !this->pianoController.GetActive(ch));
			};

		initLabel(part.voiceLabel, "");
		part.voiceLabel.setColour(Label::outlineColourId, Colours::white.withAlpha(0.7f));
		part.voiceLabel.setMinimumHorizontalScale(0.7f);

		initButton(part.changeButton, TRANS("Modify"));
		part.changeButton.setTooltip(TRANS("Chooses another voice for the part"));
		part.changeButton.onClick = [this, i]() { showVoiceMenu(i); };

		initLabel(part.volumeLabel, TRANS("Volume"));
		part.volumeSlider.setSliderStyle(Slider::LinearHorizontal);
		part.volumeSlider.setTextBoxStyle(Slider::TextBoxRight, false, 40, 24);
		part.volumeSlider.setRange(PianoController::MinVolume, PianoController::MaxVolume, 1);
		part.volumeSlider.setDoubleClickReturnValue(true, PianoController::DefaultVolume);
		part.volumeSlider.setWantsKeyboardFocus(false);
		part.volumeSlider.onValueChange = [this, i, ch]()
			{
				const int volume = roundToInt(parts[i].volumeSlider.getValue());
				if (volume != this->pianoController.GetVolume(ch))
				{
					this->pianoController.SetVolume(ch, volume);
				}
			};
		addAndMakeVisible(part.volumeSlider);

		initLabel(part.octaveLabel, TRANS("Octave"));
		initButton(part.octaveDownButton, String(CharPointer_UTF8("\xe2\x88\x92"))); // minus sign
		part.octaveDownButton.setTooltip(TRANS("One octave lower"));
		part.octaveDownButton.onClick = [this, ch]()
			{
				const int octave = this->pianoController.GetOctave(ch);
				if (octave > PianoController::MinOctave)
				{
					this->pianoController.SetOctave(ch, octave - 1);
				}
			};
		initLabel(part.octaveValueLabel, "");
		part.octaveValueLabel.setJustificationType(Justification::centred);
		initButton(part.octaveUpButton, "+");
		part.octaveUpButton.setTooltip(TRANS("One octave higher"));
		part.octaveUpButton.onClick = [this, ch]()
			{
				const int octave = this->pianoController.GetOctave(ch);
				if (octave < PianoController::MaxOctave)
				{
					this->pianoController.SetOctave(ch, octave + 1);
				}
			};
	}

	setWantsKeyboardFocus(true);
	setSize(ContentWidth, PartsY + OneTouchSettings::NumParts * PartHeight);

	ots.addChangeListener(this);
	pianoController.AddListener(this);
	ots.Update();
	update();
}

OtsComponent::~OtsComponent()
{
	stopTimer();
	pianoController.RemoveListener(this);
	ots.removeChangeListener(this);
}

void OtsComponent::paint(Graphics& g)
{
	g.fillAll(BackgroundColour);
	g.setColour(Colours::white.withAlpha(0.25f));
	g.fillRect(12, SeparatorY, getWidth() - 24, 1);
	g.fillRect(12, Separator2Y, getWidth() - 24, 1);
}

void OtsComponent::resized()
{
	const int right = ContentWidth - Margin;
	styleLabel.setBounds(Margin, 12, LabelWidth, 24);
	styleNameLabel.setBounds(Margin + LabelWidth + Gap, 12, right - (Margin + LabelWidth + Gap), 24);
	ownLabel.setBounds(Margin, 46, 200, 24);

	for (int i = 0; i < OneTouchSettings::NumSettings; i++)
	{
		const int row = i / 4;
		const int column = i % 4;
		slotButtons[i].setBounds(Margin + column * (SlotWidth + Gap), 74 + row * (SlotHeight + Gap), SlotWidth, SlotHeight);
	}
	FilledButton* functions[] = {&storeButton, &deleteButton, &saveButton, &discardButton};
	for (int i = 0; i < 4; i++)
	{
		functions[i]->setBounds(Margin + i * (SlotWidth + Gap), 74 + 2 * (SlotHeight + Gap), SlotWidth, 28);
	}

	factoryLabel.setBounds(Margin, FactoryLabelY, 200, 24);
	linkButton.setBounds(Margin, LinkY, LinkWidth, 28);
	// Save and Discard of the factory settings: 1.25 buttons wide each, to the right edge
	const int factorySaveX = Margin + LinkWidth + Gap;
	const int factorySaveWidth = SlotWidth * 5 / 4; // 117
	factorySaveButton.setBounds(factorySaveX, LinkY, factorySaveWidth, 28);
	const int factoryDiscardX = factorySaveX + factorySaveWidth + Gap;
	factoryDiscardButton.setBounds(factoryDiscardX, LinkY, right - factoryDiscardX, 28);
	for (int i = 0; i < OneTouchSettings::NumFactorySettings; i++)
	{
		factoryButtons[i].setBounds(Margin + i * (SlotWidth + Gap), FactorySlotsY, SlotWidth, SlotHeight);
	}
	copyFirstButton.setBounds(Margin, CopyY, 2 * SlotWidth + Gap, 28);
	copySecondButton.setBounds(Margin + 2 * (SlotWidth + Gap), CopyY, 2 * SlotWidth + Gap, 28);

	const int voiceX = Margin + LabelWidth + Gap;      // 90
	const int changeWidth = 110;
	const int changeX = right - changeWidth;           // 294
	for (int i = 0; i < OneTouchSettings::NumParts; i++)
	{
		PartControls& part = parts[i];
		const int y = PartsY + i * PartHeight;
		part.onButton.setBounds(Margin, y, LabelWidth, 28);
		part.voiceLabel.setBounds(voiceX, y, changeX - Gap - voiceX, 28);
		part.changeButton.setBounds(changeX, y, changeWidth, 28);
		part.volumeLabel.setBounds(Margin, y + 32, LabelWidth, 24);
		part.volumeSlider.setBounds(voiceX, y + 32, right - voiceX, 24);
		part.octaveLabel.setBounds(Margin, y + 60, LabelWidth, 28);
		part.octaveDownButton.setBounds(voiceX, y + 60, 30, 28);
		part.octaveValueLabel.setBounds(voiceX + 30 + Gap, y + 60, 40, 28);
		part.octaveUpButton.setBounds(voiceX + 30 + Gap + 40 + Gap, y + 60, 30, 28);
	}
}

// Shift+F1..F8: recall; Ctrl+F1..F4: recall a factory setting; Esc: cancels Store or Delete.
bool OtsComponent::keyPressed(const KeyPress& key)
{
	if (key == KeyPress::escapeKey && mode != Mode::none)
	{
		setMode(Mode::none);
		return true;
	}
	const ModifierKeys mods = key.getModifiers();
	const int keyCode = key.getKeyCode();
	if (mods.isShiftDown() && !mods.isCtrlDown() && !mods.isCommandDown() && !mods.isAltDown() &&
		keyCode >= KeyPress::F1Key && keyCode < KeyPress::F1Key + OneTouchSettings::NumSettings)
	{
		ots.Recall(keyCode - KeyPress::F1Key);
		return true;
	}
	// Ctrl+F1..F4: the factory settings
	if ((mods.isCtrlDown() || mods.isCommandDown()) && !mods.isShiftDown() && !mods.isAltDown() &&
		keyCode >= KeyPress::F1Key && keyCode < KeyPress::F1Key + OneTouchSettings::NumFactorySettings)
	{
		ots.RecallFactory(keyCode - KeyPress::F1Key);
		return true;
	}
	return false;
}

void OtsComponent::PianoStateChanged(PianoController::Aspect aspect, PianoController::Channel channel)
{
	const bool partChannel = channel == PianoController::chMain || channel == PianoController::chLayer ||
		channel == PianoController::chLeft;
	if (aspect == PianoController::apConnection || aspect == PianoController::apPlaybackSource ||
		((aspect == PianoController::apVoice || aspect == PianoController::apActive ||
		  aspect == PianoController::apVolume || aspect == PianoController::apOctave) && partChannel))
	{
		GuiHelper::CallAsync(this, [this]() { update(); });
	}
}

void OtsComponent::changeListenerCallback(ChangeBroadcaster*)
{
	update();
}

void OtsComponent::setMode(Mode newMode)
{
	mode = newMode;
	blinkOn = mode != Mode::none;
	if (mode != Mode::none)
	{
		startTimer(400);
	}
	else
	{
		stopTimer();
	}
	update();
}

void OtsComponent::timerCallback()
{
	blinkOn = !blinkOn;
	storeButton.blinkOn = mode == Mode::store && blinkOn;
	deleteButton.blinkOn = mode == Mode::erase && blinkOn;
	storeButton.repaint();
	deleteButton.repaint();
}

void OtsComponent::slotClicked(int index)
{
	// Shift: stores at once; Ctrl+Shift: deletes (after asking)
	const ModifierKeys mods = ModifierKeys::getCurrentModifiers();
	const bool ctrl = mods.isCtrlDown() || mods.isCommandDown();
	if (mods.isShiftDown() && ctrl)
	{
		setMode(Mode::none);
		askDelete(index);
	}
	else if (mods.isShiftDown())
	{
		setMode(Mode::none);
		ots.Store(index);
	}
	else if (mode == Mode::store)
	{
		setMode(Mode::none);
		ots.Store(index);
	}
	else if (mode == Mode::erase)
	{
		// an empty button: nothing happens, Delete keeps blinking
		if (ots.IsUsed(index))
		{
			setMode(Mode::none);
			ots.Delete(index);
		}
	}
	else
	{
		ots.Recall(index);
	}
	grabKeyboardFocus();
}

void OtsComponent::askDelete(int index)
{
	if (!ots.IsUsed(index))
	{
		return;
	}
	AlertWindow::showAsync(MessageBoxOptions()
			.withIconType(MessageBoxIconType::QuestionIcon)
			.withTitle("ConPianist")
			.withMessage(TRANS("Delete own OTS NUMBER?").replace("NUMBER", String(index + 1)))
			.withButton(TRANS("Yes"))
			.withButton(TRANS("Cancel"))
			.withAssociatedComponent(this),
		[this, index, self = Component::SafePointer<Component>(this)](int button)
		{
			if (self == nullptr)
			{
				return;
			}
			if (button == 1)
			{
				ots.Delete(index);
			}
			self->grabKeyboardFocus(); // the shortcuts work again at once
		});
}

void OtsComponent::askSaveFactory()
{
	if (!ots.IsActiveFactoryModified())
	{
		return;
	}
	AlertWindow::showAsync(MessageBoxOptions()
			.withIconType(MessageBoxIconType::QuestionIcon)
			.withTitle("ConPianist")
			.withMessage(TRANS("The original factory data is lost. Continue?"))
			.withButton(TRANS("Yes"))
			.withButton(TRANS("Cancel"))
			.withAssociatedComponent(this),
		[this, self = Component::SafePointer<Component>(this)](int button)
		{
			if (self == nullptr)
			{
				return;
			}
			if (button == 1)
			{
				ots.SaveActiveFactory();
			}
			self->grabKeyboardFocus(); // the shortcuts work again at once
		});
}

void OtsComponent::askCopy(int first)
{
	if (!ots.HasAnyFactory())
	{
		return;
	}
	StringArray overwritten;
	for (int i = 0; i < OneTouchSettings::NumFactorySettings; i++)
	{
		if (ots.IsFactoryUsed(i) && ots.IsUsed(first + i))
		{
			overwritten.add(String(first + i + 1));
		}
	}
	if (overwritten.isEmpty())
	{
		ots.CopyFactory(first);
		return;
	}
	const String message = TRANS("The factory OTS 1-4 go to the own buttons FIRST-LAST.")
			.replace("FIRST", String(first + 1)).replace("LAST", String(first + OneTouchSettings::NumFactorySettings))
		+ " " + TRANS("The content of these own buttons is replaced: LIST.").replace("LIST", overwritten.joinIntoString(", "))
		+ " " + TRANS("Continue?");
	AlertWindow::showAsync(MessageBoxOptions()
			.withIconType(MessageBoxIconType::QuestionIcon)
			.withTitle("ConPianist")
			.withMessage(message)
			.withButton(TRANS("Yes"))
			.withButton(TRANS("Cancel"))
			.withAssociatedComponent(this),
		[this, first, self = Component::SafePointer<Component>(this)](int button)
		{
			if (self == nullptr)
			{
				return;
			}
			if (button == 1)
			{
				ots.CopyFactory(first);
			}
			self->grabKeyboardFocus();
		});
}

// The voices of the panel in a menu, as in the Mixer ("Change Voice").
void OtsComponent::showVoiceMenu(int partIndex)
{
	static const int VoiceMenuBase = 1000;
	const PianoController::Channel ch = PartChannels[partIndex];
	PopupMenu menu = GuiHelper::BuildVoicesMenu(Presets::FindVoice(pianoController.GetVoice(ch)), VoiceMenuBase);
	GuiHelper::ShowMenuAsync(menu, &parts[partIndex].changeButton,
		[this, ch, self = Component::SafePointer<Component>(this)](int result)
		{
			if (self == nullptr)
			{
				return; // deleted meanwhile
			}
			VoiceList& voices = Presets::Voices();
			const int index = result - VoiceMenuBase;
			if (index >= 0 && index < (int)voices.size())
			{
				pianoController.SetActive(ch, true);
				pianoController.SetVoice(ch, voices[index].path);
			}
			self->grabKeyboardFocus();
		});
}

void OtsComponent::update()
{
	const bool connected = pianoController.IsConnected();
	// The keyboard parts are the piano's: they can be set while it is connected, also when
	// the songs are played via a MIDI device (as the accompaniment).
	const bool usable = connected;
	const String style = ots.GetStyle();
	const bool hasStyle = usable && style.isNotEmpty();

	const String title = AccompanimentComponent::styleTitle(style);
	styleNameLabel.setText(connected && title.isNotEmpty() ? title : String("-"), dontSendNotification);

	bool anyUsed = false;
	for (int i = 0; i < OneTouchSettings::NumSettings; i++)
	{
		SlotButton& button = slotButtons[i];
		button.used = ots.IsUsed(i);
		button.active = ots.GetActive() == i;
		button.modified = button.active && ots.IsModified();
		button.setEnabled(hasStyle);
		String tooltip = ots.Describe(i);
		tooltip << (tooltip.isEmpty() ? "" : "\n\n")
			<< TRANS("Click: recall (Shift+FNUMBER), Shift+click: store, Ctrl+Shift+click: delete")
				.replace("NUMBER", String(i + 1));
		button.setTooltip(tooltip);
		button.repaint();
		anyUsed = anyUsed || button.used;
	}

	if (!hasStyle && mode != Mode::none)
	{
		mode = Mode::none;
		stopTimer();
	}
	if (mode == Mode::erase && !anyUsed)
	{
		mode = Mode::none;
		stopTimer();
	}
	for (int i = 0; i < OneTouchSettings::NumFactorySettings; i++)
	{
		FactorySlotButton& button = factoryButtons[i];
		button.used = ots.IsFactoryUsed(i);
		button.active = ots.GetFactoryActive() == i;
		button.modified = ots.IsFactoryModified(i);
		button.setEnabled(hasStyle);
		String tooltip = ots.DescribeFactory(i);
		tooltip << (tooltip.isEmpty() ? "" : "\n\n")
			<< TRANS("Click: recall the factory OTS (Ctrl+FNUMBER)").replace("NUMBER", String(i + 1));
		button.setTooltip(tooltip);
		button.repaint();
	}
	if (linkButton.getToggleState() != ots.GetLink())
	{
		linkButton.setToggleState(ots.GetLink(), dontSendNotification);
	}
	const bool anyFactory = hasStyle && ots.HasAnyFactory();
	factorySaveButton.setEnabled(hasStyle && ots.IsActiveFactoryModified());
	factoryDiscardButton.setEnabled(hasStyle && ots.IsActiveFactoryModified());
	copyFirstButton.setEnabled(anyFactory);
	copySecondButton.setEnabled(anyFactory);

	storeButton.setEnabled(hasStyle);
	deleteButton.setEnabled(hasStyle && anyUsed);
	saveButton.setEnabled(hasStyle && ots.IsModified());
	discardButton.setEnabled(hasStyle && ots.IsModified());
	storeButton.blinkOn = mode == Mode::store && blinkOn;
	deleteButton.blinkOn = mode == Mode::erase && blinkOn;
	storeButton.repaint();
	deleteButton.repaint();

	updateParts();
}

void OtsComponent::updateParts()
{
	const bool usable = pianoController.IsConnected();
	for (int i = 0; i < OneTouchSettings::NumParts; i++)
	{
		PartControls& part = parts[i];
		const PianoController::Channel ch = PartChannels[i];
		const bool on = usable && pianoController.GetActive(ch);
		if (part.onButton.getToggleState() != on)
		{
			part.onButton.setToggleState(on, dontSendNotification);
		}
		part.voiceLabel.setText(usable ? Presets::VoiceTitle(pianoController.GetVoice(ch)) : String(), dontSendNotification);
		if (!part.volumeSlider.isMouseButtonDown(true))
		{
			part.volumeSlider.setValue(pianoController.GetVolume(ch), dontSendNotification);
		}
		const int octave = pianoController.GetOctave(ch);
		part.octaveValueLabel.setText(usable ? (octave > 0 ? "+" : "") + String(octave) : String(), dontSendNotification);

		for (Component* control : { (Component*)&part.onButton, (Component*)&part.voiceLabel,
			(Component*)&part.changeButton, (Component*)&part.volumeLabel, (Component*)&part.volumeSlider,
			(Component*)&part.octaveLabel, (Component*)&part.octaveValueLabel })
		{
			control->setEnabled(usable);
		}
		part.octaveDownButton.setEnabled(usable && octave > PianoController::MinOctave);
		part.octaveUpButton.setEnabled(usable && octave < PianoController::MaxOctave);
	}
}

//==============================================================================

OtsWindow::OtsWindow(Settings& settings, PianoController& pianoController, OneTouchSettings& ots) :
	DocumentWindow("OTS", BackgroundColour, DocumentWindow::minimiseButton | DocumentWindow::closeButton),
	settings(settings)
{
	const bool usingNativeTitleBar = (SystemStats::getOperatingSystemType() & SystemStats::Windows) ||
		(SystemStats::getOperatingSystemType() & SystemStats::MacOSX);
	setUsingNativeTitleBar(usingNativeTitleBar);
	setContentOwned(new OtsComponent(settings, pianoController, ots), true);
	setResizable(false, false);
	setAlwaysOnTop(true);
}

void OtsWindow::closeButtonPressed()
{
	setVisible(false);
}

void OtsWindow::visibilityChanged()
{
	DocumentWindow::visibilityChanged();
	if (onVisibilityChanged)
	{
		onVisibilityChanged();
	}
}

// The position is remembered (saved with the settings when the program exits).
void OtsWindow::moved()
{
	DocumentWindow::moved();
	if (isShowing() && !isMinimised() && getX() > -10000 && getY() > -10000)
	{
		settings.otsWindowPos = getPosition();
	}
}

bool OtsWindow::RestorePosition()
{
	if (!Settings::IsWindowPosUsable(settings.otsWindowPos, getWidth()))
	{
		return false;
	}
	setTopLeftPosition(settings.otsWindowPos);
	return true;
}
