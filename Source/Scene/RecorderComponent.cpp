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
#include "GuiHelper.h"

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

	bellButton.setButtonText(TRANS("Bell"));
	bellButton.setTooltip(TRANS("A bell sounds on the first beat of every measure"));
	bellButton.onClick = [this]()
		{
			this->pianoController.SetMetronomeBell(bellButton.getToggleState());
		};
	addAndMakeVisible(bellButton);

	initLabel(metronomeVolumeLabel, TRANS("Metronome volume:"));

	metronomeVolumeSlider.setSliderStyle(Slider::LinearHorizontal);
	metronomeVolumeSlider.setTextBoxStyle(Slider::TextBoxRight, false, 40, 24);
	metronomeVolumeSlider.setRange(0, 127, 1);
	metronomeVolumeSlider.onValueChange = [this]()
		{
			const int volume = roundToInt(metronomeVolumeSlider.getValue());
			if (volume != this->pianoController.GetMetronomeVolume())
			{
				this->pianoController.SetMetronomeVolume(volume);
			}
		};
	addAndMakeVisible(metronomeVolumeSlider);

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

	quantizeButton.setButtonText(TRANS("Quantize"));
	quantizeButton.setTooltip(TRANS("The notes of the saved (and listened) recording are moved to the nearest beat division; useful for making a score from a recording played with the metronome or with a song"));
	quantizeButton.addListener(this);
	addAndMakeVisible(quantizeButton);

	quantizeCombo.addItem(TRANS("Quarter note"), 480);
	quantizeCombo.addItem(TRANS("Eighth note"), 240);
	quantizeCombo.addItem(TRANS("Sixteenth note"), 120);
	quantizeCombo.addItem(TRANS("Eighth-note triplet"), 160);
	quantizeCombo.addItem(TRANS("Sixteenth-note triplet"), 80);
	quantizeCombo.setTooltip(TRANS("The smallest note value: the beginnings of the notes are moved to this grid"));
	quantizeCombo.onChange = [this]() { saveOptions(); };
	addAndMakeVisible(quantizeCombo);

	quantizeEndsButton.setButtonText(TRANS("Align the ends of the notes too"));
	quantizeEndsButton.setTooltip(TRANS("The ends of the notes are moved to the grid too (cleaner score); otherwise the notes keep their length"));
	quantizeEndsButton.addListener(this);
	addAndMakeVisible(quantizeEndsButton);

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

	listenButton.setButtonText(TRANS("Listen"));
	listenButton.setTooltip(TRANS("Loads the recording into the player and plays it (it replaces the loaded song)"));
	listenButton.addListener(this);
	addAndMakeVisible(listenButton);

	positionSlider.setSliderStyle(Slider::LinearHorizontal);
	positionSlider.setTextBoxStyle(Slider::NoTextBox, true, 0, 0);
	positionSlider.setRange(1, 2, 1);
	positionSlider.setTooltip(TRANS("Position in the recording while listening"));
	positionSlider.onValueChange = [this]()
		{
			if (listenLoaded)
			{
				this->pianoController.SetPosition({roundToInt(positionSlider.getValue()), 1});
			}
		};
	addAndMakeVisible(positionSlider);

	saveButton.setButtonText(TRANS("Save"));
	saveButton.addListener(this);
	addAndMakeVisible(saveButton);

	autoButton.setToggleState(settings.recorderAutomatic, dontSendNotification);
	manualButton.setToggleState(!settings.recorderAutomatic, dontSendNotification);
	silenceSlider.setValue(settings.recorderSilence, dontSendNotification);
	styleButton.setToggleState(settings.recorderStyle, dontSendNotification);
	countInCombo.setSelectedId(settings.recorderCountIn + 1, dontSendNotification);
	quantizeButton.setToggleState(settings.recorderQuantize, dontSendNotification);
	quantizeCombo.setSelectedId(settings.recorderQuantizeTicks, dontSendNotification);
	if (quantizeCombo.getSelectedId() == 0)
	{
		quantizeCombo.setSelectedId(120, dontSendNotification);
	}
	quantizeEndsButton.setToggleState(settings.recorderQuantizeEnds, dontSendNotification);

	setSize(400, 486);

	pianoController.AddListener(this);
	updateMetronome();

	lastState = pianoController.GetRecorder().GetState();
	updateControls();
	updateStatus();
	startTimer(RecorderTimerMs);
}

RecorderComponent::~RecorderComponent()
{
	stopTimer();
	pianoController.RemoveListener(this);
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
	metronomeButton.setBounds(12, 124, 112, 24);
	bellButton.setBounds(124, 124, 92, 24);
	countInCombo.setBounds(220, 124, 164, 24);
	metronomeVolumeLabel.setBounds(16, 160, 170, 24);
	metronomeVolumeSlider.setBounds(186, 160, 198, 24);
	styleButton.setBounds(12, 196, 372, 24);
	quantizeButton.setBounds(12, 232, 140, 24);
	quantizeCombo.setBounds(160, 232, 224, 24);
	quantizeEndsButton.setBounds(36, 268, 348, 24);
	nameLabel.setBounds(16, 304, 100, 24);
	nameEditor.setBounds(120, 304, 264, 24);
	statusLabel.setBounds(16, 342, 368, 36);
	recordButton.setBounds(16, 394, 112, 32);
	stopButton.setBounds(144, 394, 112, 32);
	saveButton.setBounds(272, 394, 112, 32);
	listenButton.setBounds(16, 438, 112, 32);
	positionSlider.setBounds(140, 442, 248, 24);
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
		if (pianoController.GetRecorder().GetState() == LiveRecorder::stIdle)
		{
			// nothing is being recorded: the button stops the listening back
			if (isListening())
			{
				pianoController.Stop();
			}
		}
		else
		{
			stopRecording();
		}
	}
	else if (button == &listenButton)
	{
		listen();
	}
	else if (button == &saveButton)
	{
		save();
	}
	else if (button == &autoButton || button == &manualButton || button == &styleButton ||
		button == &quantizeButton || button == &quantizeEndsButton)
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
	const bool quantize = quantizeButton.getToggleState();
	const int ticks = quantizeCombo.getSelectedId() > 0 ? quantizeCombo.getSelectedId() : settings.recorderQuantizeTicks;
	const bool ends = quantizeEndsButton.getToggleState();
	if (automatic != settings.recorderAutomatic || silence != settings.recorderSilence ||
		style != settings.recorderStyle || countIn != settings.recorderCountIn ||
		quantize != settings.recorderQuantize || ticks != settings.recorderQuantizeTicks ||
		ends != settings.recorderQuantizeEnds)
	{
		settings.recorderQuantize = quantize;
		settings.recorderQuantizeTicks = ticks;
		settings.recorderQuantizeEnds = ends;
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
	if (isListening())
	{
		// the recording played back would be recorded again as a song
		pianoController.Stop();
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
	if (pianoController.SaveRecording(file, styleButton.getToggleState(), error, true,
		quantizeTicks(), quantizeEndsButton.getToggleState()))
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

// Listening back: the recording is written into a temporary file as it would be saved
// now (quantized or not, with or without the accompaniment), loaded into the player as a
// song and played. This does not count as saving.
void RecorderComponent::listen()
{
	LiveRecorder& recorder = pianoController.GetRecorder();
	if (recorder.GetState() != LiveRecorder::stIdle || !recorder.HasData())
	{
		return;
	}

	const File directory = File::getSpecialLocation(File::tempDirectory).getChildFile("ConPianist");
	directory.createDirectory();
	const File file = directory.getChildFile(File::createLegalFileName(TRANS("Recording (listening)")) + ".mid");
	String error;
	if (!pianoController.SaveRecording(file, styleButton.getToggleState(), error, false,
		quantizeTicks(), quantizeEndsButton.getToggleState()))
	{
		message = TRANS("The recording could not be played.");
		updateStatus();
		return;
	}

	if (pianoController.GetPlaying())
	{
		pianoController.Stop();
	}

	message = "";
	listenLoaded = false;
	playPending = true; // played when the player reports that the song is loaded
	if (!pianoController.LoadSong(file))
	{
		playPending = false;
		message = TRANS("The recording could not be played.");
	}
	updateControls();
	updateStatus();
}

int RecorderComponent::quantizeTicks() const
{
	return quantizeButton.getToggleState() ? jmax(0, quantizeCombo.getSelectedId()) : 0;
}

bool RecorderComponent::isListening() const
{
	return listenLoaded && pianoController.GetPlaying();
}

// Called from any thread.
void RecorderComponent::PianoStateChanged(PianoController::Aspect aspect, PianoController::Channel channel)
{
	if (aspect == PianoController::apSongLoaded)
	{
		// the song loaded for listening back, or another song that replaces it
		const bool mine = playPending.exchange(false);
		listenLoaded = mine;
		if (mine)
		{
			GuiHelper::CallAsync(this, [this]()
				{
					if (listenLoaded)
					{
						pianoController.Play();
					}
				});
		}
	}
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
	const bool listening = isListening();
	if (listening != lastListening || positionSlider.isEnabled() != (bool)listenLoaded)
	{
		lastListening = listening;
		updateControls();
	}
	if (listenLoaded && !positionSlider.isMouseButtonDown())
	{
		// the position of the player in the recording
		const int length = jmax(2, pianoController.GetLength().measure);
		if (roundToInt(positionSlider.getMaximum()) != length)
		{
			positionSlider.setRange(1, length, 1);
		}
		positionSlider.setValue(pianoController.GetPosition().measure, dontSendNotification);
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
	const bool listening = isListening();
	recordButton.setEnabled(idle);
	stopButton.setEnabled(!idle || listening);
	listenButton.setEnabled(hasData && !listening);
	positionSlider.setEnabled(listenLoaded);
	if (!listenLoaded)
	{
		positionSlider.setValue(1, dontSendNotification);
	}
	saveButton.setEnabled(hasData);
	nameEditor.setEnabled(hasData);
	countInCombo.setEnabled(idle && manualButton.getToggleState());
	quantizeCombo.setEnabled(quantizeButton.getToggleState());
	quantizeEndsButton.setEnabled(quantizeButton.getToggleState());
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
	bellButton.setToggleState(pianoController.GetMetronomeBell(), dontSendNotification);
	if (!metronomeVolumeSlider.isMouseButtonDown(true) && !metronomeVolumeSlider.hasKeyboardFocus(true))
	{
		metronomeVolumeSlider.setValue(pianoController.GetMetronomeVolume(), dontSendNotification);
	}
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
			// the count-in begins on the next downbeat (the metronome may be running already)
			text = recorder.GetCountInMeasuresLeft() > jlimit(0, 2, countInCombo.getSelectedId() - 1) ?
				TRANS("The count-in begins with the next measure...") : TRANS("Count-in...");
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
			else if (isListening())
			{
				text = TRANS("Playing the recording:") + " " + info;
				colour = Colours::lightblue;
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
