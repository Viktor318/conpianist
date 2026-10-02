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

#include "RecorderComponent.h"

static const int RecorderTimerMs = 100;

RecorderComponent::RecorderComponent(Settings& settings, PianoController& pianoController) :
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

	initLabel(modeLabel, TRANS("Start and stop:"));

	autoButton.setButtonText(TRANS("Automatic"));
	autoButton.setTooltip(TRANS("The recording starts with the first played note and stops after the given silence"));
	autoButton.setRadioGroupId(1);
	autoButton.setClickingTogglesState(true);
	autoButton.addListener(this);
	addAndMakeVisible(autoButton);

	manualButton.setButtonText(TRANS("Manual"));
	manualButton.setTooltip(TRANS("The recording is started and stopped with the buttons"));
	manualButton.setRadioGroupId(1);
	manualButton.setClickingTogglesState(true);
	manualButton.addListener(this);
	addAndMakeVisible(manualButton);

	initLabel(silenceLabel, TRANS("Stop after silence (seconds):"));

	silenceSlider.setSliderStyle(Slider::IncDecButtons);
	silenceSlider.setTextBoxStyle(Slider::TextBoxLeft, false, 40, 24);
	silenceSlider.setRange(Settings::MinRecorderSilence, Settings::MaxRecorderSilence, 1);
	silenceSlider.onValueChange = [this]() { saveOptions(); };
	addAndMakeVisible(silenceSlider);

	initLabel(tempoLabel, TRANS("Tempo and beat:"));

	tempoSlider.setSliderStyle(Slider::IncDecButtons);
	tempoSlider.setTextBoxStyle(Slider::TextBoxLeft, false, 40, 24);
	tempoSlider.setRange(PianoController::MinTempo, PianoController::MaxTempo, 1);
	tempoSlider.onValueChange = [this]()
		{
			const int tempo = roundToInt(tempoSlider.getValue());
			if (tempo != this->pianoController.GetTempo())
			{
				this->pianoController.SetTempo(tempo);
			}
		};
	addAndMakeVisible(tempoSlider);

	static const int beats[][2] = {{2, 4}, {3, 4}, {4, 4}, {5, 4}, {6, 4}, {3, 8}, {6, 8}, {9, 8}, {12, 8}};
	for (const auto& beat : beats)
	{
		beatCombo.addItem(String(beat[0]) + "/" + String(beat[1]), beat[0] * 100 + beat[1]);
	}
	beatCombo.setTooltip(TRANS("Time signature of the metronome and of the recording"));
	beatCombo.onChange = [this]()
		{
			const int id = beatCombo.getSelectedId();
			if (id > 0)
			{
				this->pianoController.SetMetronomeBeat(id / 100, id % 100);
			}
		};
	addAndMakeVisible(beatCombo);

	metronomeButton.setButtonText(TRANS("Metronome"));
	metronomeButton.setTooltip(TRANS("The metronome sounds on the piano, or on the MIDI device when that is used for playing; a recording made with it is aligned to the measures"));
	metronomeButton.onClick = [this]()
		{
			metronomeStarted = false;
			this->pianoController.SetMetronome(metronomeButton.getToggleState());
		};
	addAndMakeVisible(metronomeButton);

	countInCombo.addItem(TRANS("No count-in"), 1);
	countInCombo.addItem(TRANS("Count-in: 1 measure"), 2);
	countInCombo.addItem(TRANS("Count-in: 2 measures"), 3);
	countInCombo.setTooltip(TRANS("Manual recording starts after the count-in of the metronome"));
	countInCombo.onChange = [this]() { saveOptions(); };
	addAndMakeVisible(countInCombo);

	styleButton.setButtonText(TRANS("Record accompaniment"));
	styleButton.setTooltip(TRANS("The accompaniment (style) of the piano is saved into the file too"));
	styleButton.addListener(this);
	addAndMakeVisible(styleButton);

	initLabel(nameLabel, TRANS("File name:"));

	nameEditor.setMultiLine(false);
	nameEditor.setSelectAllWhenFocused(true);
	addAndMakeVisible(nameEditor);

	initLabel(statusLabel, "");
	statusLabel.setJustificationType(Justification::centred);
	statusLabel.setMinimumHorizontalScale(0.7f);

	recordButton.setButtonText(TRANS("Record"));
	recordButton.addListener(this);
	addAndMakeVisible(recordButton);

	stopButton.setButtonText(TRANS("Stop"));
	stopButton.addListener(this);
	addAndMakeVisible(stopButton);

	saveButton.setButtonText(TRANS("Save"));
	saveButton.addListener(this);
	addAndMakeVisible(saveButton);

	autoButton.setToggleState(settings.recorderAutomatic, dontSendNotification);
	manualButton.setToggleState(!settings.recorderAutomatic, dontSendNotification);
	silenceSlider.setValue(settings.recorderSilence, dontSendNotification);
	styleButton.setToggleState(settings.recorderStyle, dontSendNotification);
	countInCombo.setSelectedId(settings.recorderCountIn + 1, dontSendNotification);

	setSize(400, 334);

	updateMetronome();

	lastState = pianoController.GetRecorder().GetState();
	updateControls();
	updateStatus();
	startTimer(RecorderTimerMs);
}

RecorderComponent::~RecorderComponent()
{
	stopTimer();
}

void RecorderComponent::paint(Graphics& g)
{
	g.fillAll(Colour(0xff323e44));
}

void RecorderComponent::resized()
{
	modeLabel.setBounds(16, 16, 120, 24);
	autoButton.setBounds(140, 16, 120, 24);
	manualButton.setBounds(264, 16, 120, 24);
	silenceLabel.setBounds(16, 52, 240, 24);
	silenceSlider.setBounds(264, 52, 120, 24);
	tempoLabel.setBounds(16, 88, 120, 24);
	tempoSlider.setBounds(140, 88, 112, 24);
	beatCombo.setBounds(264, 88, 120, 24);
	metronomeButton.setBounds(12, 124, 180, 24);
	countInCombo.setBounds(200, 124, 184, 24);
	styleButton.setBounds(12, 160, 372, 24);
	nameLabel.setBounds(16, 196, 100, 24);
	nameEditor.setBounds(120, 196, 264, 24);
	statusLabel.setBounds(16, 234, 368, 36);
	recordButton.setBounds(16, 286, 112, 32);
	stopButton.setBounds(144, 286, 112, 32);
	saveButton.setBounds(272, 286, 112, 32);
}

void RecorderComponent::buttonClicked(Button* button)
{
	if (button == &recordButton)
	{
		LiveRecorder& recorder = pianoController.GetRecorder();
		if (recorder.HasData() && !saved)
		{
			// asks first: the previous recording would be lost
			AlertWindow::showAsync(MessageBoxOptions()
					.withIconType(MessageBoxIconType::QuestionIcon)
					.withTitle("ConPianist")
					.withMessage(TRANS("The previous recording is not saved and will be lost. Continue?"))
					.withButton(TRANS("Yes"))
					.withButton(TRANS("Cancel"))
					.withAssociatedComponent(this),
				[this, self = Component::SafePointer<Component>(this)](int result)
				{
					if (self != nullptr && result == 1)
					{
						startRecording();
					}
				});
		}
		else
		{
			startRecording();
		}
	}
	else if (button == &stopButton)
	{
		stopRecording();
	}
	else if (button == &saveButton)
	{
		save();
	}
	else if (button == &autoButton || button == &manualButton || button == &styleButton)
	{
		saveOptions();
		updateControls();
	}
}

void RecorderComponent::saveOptions()
{
	const bool automatic = autoButton.getToggleState();
	const int silence = roundToInt(silenceSlider.getValue());
	const bool style = styleButton.getToggleState();
	const int countIn = jlimit(0, 2, countInCombo.getSelectedId() - 1);
	if (automatic != settings.recorderAutomatic || silence != settings.recorderSilence ||
		style != settings.recorderStyle || countIn != settings.recorderCountIn)
	{
		settings.recorderCountIn = countIn;
		settings.recorderAutomatic = automatic;
		settings.recorderSilence = silence;
		settings.recorderStyle = style;
		settings.Save();
	}
}

void RecorderComponent::startRecording()
{
	if (pianoController.GetRecorder().GetState() != LiveRecorder::stIdle)
	{
		return;
	}
	saved = false;
	savedName = "";
	message = "";
	// automatic file name with the date and time; it can be changed until it is saved
	nameEditor.setText(TRANS("Recording") + " " + Time::getCurrentTime().formatted("%Y-%m-%d %H-%M-%S"), false);
	const int countIn = jlimit(0, 2, countInCombo.getSelectedId() - 1);
	if (autoButton.getToggleState())
	{
		pianoController.StartRecording(true);
	}
	else if (countIn > 0)
	{
		// counted by the metronome, unless a song (or the metronome) is giving the beats
		if (!pianoController.HasBeats() && !pianoController.GetMetronome())
		{
			pianoController.SetMetronome(true);
			metronomeStarted = true;
		}
		pianoController.StartRecordingWithCountIn(countIn);
	}
	else
	{
		pianoController.StartRecording(false);
	}
	timerCallback();
}

void RecorderComponent::stopRecording()
{
	pianoController.StopRecording();
	timerCallback();
}

void RecorderComponent::save()
{
	LiveRecorder& recorder = pianoController.GetRecorder();
	if (recorder.GetState() != LiveRecorder::stIdle || !recorder.HasData())
	{
		return;
	}

	String name = File::createLegalFileName(nameEditor.getText().trim());
	if (name.endsWithIgnoreCase(".mid"))
	{
		name = name.dropLastCharacters(4).trim();
	}
	if (name.isEmpty())
	{
		message = TRANS("Enter a file name.");
		updateStatus();
		return;
	}

	File directory = settings.GetDefaultSongDirectory();
	directory.createDirectory();
	const File file = directory.getChildFile(name + ".mid");

	if (file.existsAsFile())
	{
		AlertWindow::showAsync(MessageBoxOptions()
				.withIconType(MessageBoxIconType::QuestionIcon)
				.withTitle("ConPianist")
				.withMessage(TRANS("A file with this name already exists. Overwrite it?"))
				.withButton(TRANS("Yes"))
				.withButton(TRANS("Cancel"))
				.withAssociatedComponent(this),
			[this, file, self = Component::SafePointer<Component>(this)](int result)
			{
				if (self != nullptr && result == 1)
				{
					writeFile(file);
				}
			});
	}
	else
	{
		writeFile(file);
	}
}

void RecorderComponent::writeFile(const File& file)
{
	String error;
	if (pianoController.SaveRecording(file, styleButton.getToggleState(), error))
	{
		saved = true;
		savedName = file.getFileName();
		message = "";
	}
	else
	{
		message = TRANS("The file could not be saved.");
	}
	updateControls();
	updateStatus();
}

void RecorderComponent::timerCallback()
{
	LiveRecorder& recorder = pianoController.GetRecorder();
	const LiveRecorder::State state = recorder.GetState();

	if (state == LiveRecorder::stRecording &&
		(lastState == LiveRecorder::stArmed || lastState == LiveRecorder::stCountIn))
	{
		// started by the first note or after the count-in: the voices as they are now
		pianoController.UpdateRecorderSetups();
	}

	// automatic stop: nothing is held and nothing was played for the given time
	if (state == LiveRecorder::stRecording && autoButton.getToggleState() &&
		recorder.GetNoteCount() > 0 && recorder.GetHeldNoteCount() == 0 &&
		recorder.GetSecondsSinceLastEvent() >= silenceSlider.getValue())
	{
		pianoController.StopRecording();
	}

	const LiveRecorder::State newState = recorder.GetState();
	if (newState != lastState)
	{
		if (newState == LiveRecorder::stIdle && metronomeStarted)
		{
			// switched on for the count-in only
			metronomeStarted = false;
			pianoController.SetMetronome(false);
		}
		lastState = newState;
		updateControls();
	}
	updateMetronome();
	updateStatus();
}

void RecorderComponent::updateControls()
{
	LiveRecorder& recorder = pianoController.GetRecorder();
	const bool idle = recorder.GetState() == LiveRecorder::stIdle;
	const bool hasData = idle && recorder.HasData();

	autoButton.setEnabled(idle);
	manualButton.setEnabled(idle);
	silenceLabel.setEnabled(autoButton.getToggleState());
	silenceSlider.setEnabled(autoButton.getToggleState());
	recordButton.setEnabled(idle);
	stopButton.setEnabled(!idle);
	saveButton.setEnabled(hasData);
	nameEditor.setEnabled(hasData);
	countInCombo.setEnabled(idle && manualButton.getToggleState());
}

// Shows the tempo, the time signature and the state of the metronome as they are now
// (they can change on the piano and with the song too).
void RecorderComponent::updateMetronome()
{
	if (!tempoSlider.hasKeyboardFocus(true) && !tempoSlider.isMouseButtonDown(true))
	{
		tempoSlider.setValue(pianoController.GetTempo(), dontSendNotification);
	}

	if (!beatCombo.isPopupActive())
	{
		const int numerator = pianoController.GetMetronomeBeatNumerator();
		const int denominator = pianoController.GetMetronomeBeatDenominator();
		const int id = numerator * 100 + denominator;
		if (beatCombo.indexOfItemId(id) >= 0)
		{
			beatCombo.setSelectedId(id, dontSendNotification);
		}
		else
		{
			beatCombo.setText(String(numerator) + "/" + String(denominator), dontSendNotification);
		}
	}

	metronomeButton.setToggleState(pianoController.GetMetronome(), dontSendNotification);
}

String RecorderComponent::formatTime(double seconds)
{
	const int total = jmax(0, (int)seconds);
	return String(total / 60) + ":" + String(total % 60).paddedLeft('0', 2);
}

void RecorderComponent::updateStatus()
{
	LiveRecorder& recorder = pianoController.GetRecorder();
	const String bullet = " " + String(CharPointer_UTF8("\xe2\x80\xa2")) + " ";
	const String info = formatTime(recorder.GetLengthSeconds()) + bullet +
		TRANS("NUMBER notes").replace("NUMBER", String(recorder.GetNoteCount()));
	String text;
	Colour colour = Colours::white;

	switch (recorder.GetState())
	{
		case LiveRecorder::stArmed:
			text = TRANS("Waiting for the first note...");
			colour = Colours::orange;
			break;
		case LiveRecorder::stCountIn:
			text = TRANS("Count-in...");
			colour = Colours::orange;
			break;
		case LiveRecorder::stRecording:
			text = TRANS("Recording:") + " " + info;
			colour = Colour(0xffff6060);
			break;
		default:
			if (message.isNotEmpty())
			{
				text = message;
				colour = Colours::orange;
			}
			else if (!recorder.HasData())
			{
				text = TRANS("Press Record to start a recording.");
			}
			else if (saved)
			{
				text = TRANS("Saved:") + " " + savedName;
				colour = Colours::lightgreen;
			}
			else
			{
				text = TRANS("Recorded, not saved:") + " " + info;
			}
			break;
	}

	if (statusLabel.getText() != text)
	{
		statusLabel.setText(text, dontSendNotification);
	}
	statusLabel.setColour(Label::textColourId, colour);
}

//==============================================================================

RecorderWindow::RecorderWindow(Settings& settings, PianoController& pianoController) :
	DocumentWindow(TRANS("Recording"), Colour(0xff323e44), DocumentWindow::closeButton)
{
	const bool usingNativeTitleBar = (SystemStats::getOperatingSystemType() & SystemStats::Windows) ||
		(SystemStats::getOperatingSystemType() & SystemStats::MacOSX);
	setUsingNativeTitleBar(usingNativeTitleBar);
	setContentOwned(new RecorderComponent(settings, pianoController), true);
	setResizable(false, false);
	setAlwaysOnTop(true);
}

void RecorderWindow::closeButtonPressed()
{
	setVisible(false);
}
