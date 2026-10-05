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

#include "StyleMixerComponent.h"
#include "GuiHelper.h"
#include "Presets.h"

StyleMixerComponent::StyleMixerComponent(Settings& settings, PianoController& pianoController) :
	settings(settings), pianoController(pianoController)
{
	auto initLabel = [this](Label& label, const String& text, float fontSize, Justification justification)
		{
			label.setText(text, dontSendNotification);
			label.setFont(Font(FontOptions(fontSize, Font::plain)));
			label.setJustificationType(justification);
			label.setColour(Label::textColourId, Colours::white);
			label.setMinimumHorizontalScale(0.6f);
			addAndMakeVisible(label);
		};

	for (int i = 0; i <= NumParts; i++)
	{
		Strip& strip = strips[i];
		const bool master = i == MasterStrip;
		const PianoController::Channel ch = master ? PianoController::chStyle : PianoController::StylePartChannel(i);
		strip.channel = ch;

		if (master)
		{
			initLabel(strip.nameLabel, "Master", 15.0f, Justification::centred);
			strip.nameLabel.setFont(Font(FontOptions(15.0f, Font::bold)));
		}
		else
		{
			strip.onButton.setButtonText(PianoController::StylePartName(i));
			strip.onButton.setTooltip(TRANS("Switches the part on and off"));
			strip.onButton.setWantsKeyboardFocus(false);
			strip.onButton.onClick = [this, ch]()
				{
					this->pianoController.SetActive(ch, !this->pianoController.GetActive(ch));
				};
			addAndMakeVisible(strip.onButton);
			initLabel(strip.voiceLabel, "", 12.0f, Justification::centred);
		}

		strip.panSlider.setSliderStyle(Slider::LinearHorizontal);
		strip.panSlider.setTextBoxStyle(Slider::NoTextBox, true, 0, 0);
		strip.panSlider.setRange(PianoController::MinPan, PianoController::MaxPan, 1);
		strip.panSlider.setDoubleClickReturnValue(true, PianoController::DefaultPan);
		strip.panSlider.setPopupDisplayEnabled(true, true, this);
		strip.panSlider.setWantsKeyboardFocus(false);
		strip.panSlider.onValueChange = [this, ch, slider = &strip.panSlider]()
			{
				const int value = roundToInt(slider->getValue());
				if (value != this->pianoController.GetPan(ch))
				{
					this->pianoController.SetPan(ch, value);
				}
			};
		addAndMakeVisible(strip.panSlider);

		strip.reverbSlider.setSliderStyle(Slider::LinearHorizontal);
		strip.reverbSlider.setTextBoxStyle(Slider::NoTextBox, true, 0, 0);
		strip.reverbSlider.setRange(PianoController::MinReverb, PianoController::MaxReverb, 1);
		strip.reverbSlider.setPopupDisplayEnabled(true, true, this);
		strip.reverbSlider.setWantsKeyboardFocus(false);
		strip.reverbSlider.onValueChange = [this, ch, slider = &strip.reverbSlider]()
			{
				const int value = roundToInt(slider->getValue());
				if (value != this->pianoController.GetReverb(ch))
				{
					this->pianoController.SetReverb(ch, value);
				}
			};
		addAndMakeVisible(strip.reverbSlider);

		strip.volumeSlider.setSliderStyle(Slider::LinearVertical);
		strip.volumeSlider.setTextBoxStyle(Slider::TextBoxBelow, false, 44, 20);
		strip.volumeSlider.setRange(PianoController::MinVolume, PianoController::MaxVolume, 1);
		strip.volumeSlider.setWantsKeyboardFocus(false);
		if (master)
		{
			strip.volumeSlider.setDoubleClickReturnValue(true, PianoController::DefaultVolume);
		}
		strip.volumeSlider.onValueChange = [this, ch, slider = &strip.volumeSlider]()
			{
				const int value = roundToInt(slider->getValue());
				if (value != this->pianoController.GetVolume(ch))
				{
					this->pianoController.SetVolume(ch, value);
				}
			};
		addAndMakeVisible(strip.volumeSlider);
	}

	initLabel(panCaption, TRANS("Pan"), 14.0f, Justification::centredLeft);
	initLabel(reverbCaption, TRANS("Reverb"), 14.0f, Justification::centredLeft);
	initLabel(volumeCaption, TRANS("Volume"), 14.0f, Justification::centredLeft);
	for (Label* caption : {&panCaption, &reverbCaption, &volumeCaption})
	{
		caption->setColour(Label::textColourId, Colours::white.withAlpha(0.7f));
	}

	for (ReverbEffect& effect : Presets::ReverbEffects())
	{
		reverbEffectCombo.addItem(effect.title, effect.num + ReverbEffectIdBase);
	}
	reverbEffectCombo.setTooltip(TRANS("The type of the reverb of the piano (the same setting as in the Balance window)"));
	reverbEffectCombo.setWantsKeyboardFocus(false);
	reverbEffectCombo.onChange = [this]()
		{
			const int effect = reverbEffectCombo.getSelectedId() - ReverbEffectIdBase;
			if (reverbEffectCombo.getSelectedId() > 0 && effect != this->pianoController.GetReverbEffect())
			{
				this->pianoController.SetReverbEffect(effect);
			}
		};
	addAndMakeVisible(reverbEffectCombo);

	initLabel(hintLabel, "", 15.0f, Justification::centredRight);
	hintLabel.setColour(Label::textColourId, Colours::orange);

	setSize(654, 470);

	pianoController.AddListener(this);
	update();
}

StyleMixerComponent::~StyleMixerComponent()
{
	pianoController.RemoveListener(this);
}

// the layout: the strips of the parts, a line, the master strip
static const int StripLeft = 12;
static const int StripWidth = 68;
static const int MasterLeft = 12 + 8 * 68 + 14;
static const int MasterWidth = 72;

void StyleMixerComponent::paint(Graphics& g)
{
	g.fillAll(Colour(0xff323e44));
	g.setColour(Colours::white.withAlpha(0.25f));
	g.fillRect(12, 70, StripLeft + NumParts * StripWidth - 12, 1);
	g.fillRect(12, 128, StripLeft + NumParts * StripWidth - 12, 1);
	g.fillRect(12, 190, StripLeft + NumParts * StripWidth - 12, 1);
	g.fillRect(MasterLeft - 8, 12, 1, getHeight() - 52); // before the master strip
}

void StyleMixerComponent::resized()
{
	for (int i = 0; i <= NumParts; i++)
	{
		Strip& strip = strips[i];
		const bool master = i == MasterStrip;
		const int x = master ? MasterLeft : StripLeft + i * StripWidth;
		const int width = master ? MasterWidth : StripWidth;
		strip.onButton.setBounds(x + 2, 12, width - 4, 30);
		strip.nameLabel.setBounds(x, 12, width, 30);
		strip.voiceLabel.setBounds(x, 46, width, 18);
		strip.panSlider.setBounds(x + 2, 98, width - 4, 22);
		strip.reverbSlider.setBounds(x + 2, 160, width - 4, 22);
		strip.volumeSlider.setBounds(x + 2, 220, width - 4, 204);
	}
	panCaption.setBounds(12, 76, 90, 20);
	// the list of the reverb types right after the text of its label
	const int captionWidth = GlyphArrangement::getStringWidthInt(reverbCaption.getFont(), reverbCaption.getText()) + 10;
	reverbCaption.setBounds(12, 134, captionWidth, 24);
	reverbEffectCombo.setBounds(12 + captionWidth + 8, 134, 240, 24);
	volumeCaption.setBounds(12, 196, 90, 20);
	hintLabel.setBounds(12, getHeight() - 34, getWidth() - 24, 24);
}

bool StyleMixerComponent::isStripChannel(PianoController::Channel channel)
{
	return channel == PianoController::chStyle ||
		(channel >= PianoController::chStylePart1 && channel < PianoController::chStylePart1 + NumParts);
}

// Called from any thread.
void StyleMixerComponent::PianoStateChanged(PianoController::Aspect aspect, PianoController::Channel channel)
{
	const bool mixerValue = aspect == PianoController::apVolume || aspect == PianoController::apPan ||
		aspect == PianoController::apReverb || aspect == PianoController::apActive ||
		aspect == PianoController::apVoice;
	if ((mixerValue && isStripChannel(channel)) || aspect == PianoController::apReverbEffect ||
		aspect == PianoController::apConnection)
	{
		GuiHelper::CallAsync(this, [this]() { update(); });
	}
}

void StyleMixerComponent::update()
{
	const bool connected = pianoController.IsConnected();

	for (int i = 0; i <= NumParts; i++)
	{
		Strip& strip = strips[i];
		const bool master = i == MasterStrip;
		const PianoController::Channel ch = strip.channel;
		const bool active = master || pianoController.GetActive(ch);

		if (!master)
		{
			// drawn by the look and feel of the program: "toggle" - filled when on
			if (active && connected)
			{
				strip.onButton.getProperties().set("toggle", true);
			}
			else
			{
				strip.onButton.getProperties().remove("toggle");
			}
			strip.onButton.setToggleState(active && connected, dontSendNotification);
			strip.onButton.setEnabled(connected);
			strip.onButton.repaint();

			const String voice = pianoController.GetVoice(ch);
			String name = voice.isNotEmpty() ? Presets::VoiceName(voice.getIntValue()) : String();
			strip.voiceLabel.setText(connected && name.isNotEmpty() ? name : String("-"), dontSendNotification);
			strip.voiceLabel.setAlpha(active ? 1.0f : 0.45f);
		}

		if (!strip.panSlider.isMouseButtonDown(true))
		{
			strip.panSlider.setValue(pianoController.GetPan(ch), dontSendNotification);
		}
		if (!strip.reverbSlider.isMouseButtonDown(true))
		{
			strip.reverbSlider.setValue(pianoController.GetReverb(ch), dontSendNotification);
		}
		if (!strip.volumeSlider.isMouseButtonDown(true))
		{
			strip.volumeSlider.setValue(pianoController.GetVolume(ch), dontSendNotification);
		}
		for (Slider* slider : {&strip.panSlider, &strip.reverbSlider, &strip.volumeSlider})
		{
			slider->setEnabled(connected);
			slider->setAlpha(active ? 1.0f : 0.45f); // a part that is off is faint, but can be set
		}
	}

	reverbEffectCombo.setEnabled(connected);
	reverbEffectCombo.setSelectedId(pianoController.GetReverbEffect() + ReverbEffectIdBase, dontSendNotification);

	hintLabel.setText(connected ? String() : TRANS("The piano is not connected."), dontSendNotification);
}

//==============================================================================

StyleMixerWindow::StyleMixerWindow(Settings& settings, PianoController& pianoController) :
	DocumentWindow(TRANS("Accompaniment mixer"), Colour(0xff323e44),
		DocumentWindow::minimiseButton | DocumentWindow::closeButton),
	settings(settings)
{
	const bool usingNativeTitleBar = (SystemStats::getOperatingSystemType() & SystemStats::Windows) ||
		(SystemStats::getOperatingSystemType() & SystemStats::MacOSX);
	setUsingNativeTitleBar(usingNativeTitleBar);
	setContentOwned(new StyleMixerComponent(settings, pianoController), true);
	setResizable(false, false);
	setAlwaysOnTop(true);
}

void StyleMixerWindow::closeButtonPressed()
{
	setVisible(false);
}

// The position is remembered (saved with the settings when the program exits).
void StyleMixerWindow::moved()
{
	DocumentWindow::moved();
	if (isShowing() && !isMinimised() && getX() > -10000 && getY() > -10000)
	{
		settings.styleMixerWindowPos = getPosition();
	}
}

bool StyleMixerWindow::RestorePosition()
{
	if (!Settings::IsWindowPosUsable(settings.styleMixerWindowPos, getWidth()))
	{
		return false;
	}
	setTopLeftPosition(settings.styleMixerWindowPos);
	return true;
}

//==============================================================================

// A drum seen from the side: the head, the shell with its lugs, and two sticks.
void StyleMixerButton::paintButton(Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
	const float alpha = !isEnabled() ? 0.4f : shouldDrawButtonAsHighlighted && !shouldDrawButtonAsDown ? 0.75f : 1.0f;
	g.setColour(Colours::white.withAlpha(alpha));

	const Rectangle<float> area = getLocalBounds().toFloat().withSizeKeepingCentre(22.0f, 22.0f);
	const float left = area.getX() + 2.0f;
	const float right = area.getRight() - 2.0f;
	const float top = area.getY() + 9.0f;
	const float bottom = area.getBottom() - 1.0f;

	// the head and the bottom rim
	g.fillRect(left - 1.0f, top, right - left + 2.0f, 2.5f);
	g.fillRect(left - 1.0f, bottom - 2.5f, right - left + 2.0f, 2.5f);
	// the shell
	g.fillRect(left, top, 2.0f, bottom - top);
	g.fillRect(right - 2.0f, top, 2.0f, bottom - top);
	// the lugs
	for (int i = 1; i <= 3; i++)
	{
		const float x = left + (right - left) * (float)i / 4.0f - 0.75f;
		g.fillRect(x, top + 4.0f, 1.5f, bottom - top - 8.0f);
	}
	// the sticks, crossed above the head
	g.drawLine(area.getX() + 1.0f, area.getY() + 1.0f, area.getCentreX() + 3.0f, top - 1.5f, 2.0f);
	g.drawLine(area.getRight() - 1.0f, area.getY() + 1.0f, area.getCentreX() - 3.0f, top - 1.5f, 2.0f);
}
