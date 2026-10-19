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
#include "AccompanimentComponent.h"

static const int RecorderTimerMs = 100;
static const int MaxNameLength = 100; // characters of the file name typed (the path is limited)

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

	initLabel(modeLabel, TRANS("Start and stop"));

	autoButton.setButtonText(TRANS("Automatic"));
	autoButton.setTooltip(TRANS("The recording starts with the first played note and stops after the given silence; if the accompaniment is recorded too, its start also starts the recording, and it does not stop while the accompaniment is playing"));
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

	initLabel(silenceLabel, TRANS("Stop after silence (seconds)"));

	silenceSlider.setSliderStyle(Slider::IncDecButtons);
	silenceSlider.setTextBoxStyle(Slider::TextBoxLeft, false, 40, 24);
	silenceSlider.setRange(Settings::MinRecorderSilence, Settings::MaxRecorderSilence, 1);
	silenceSlider.onValueChange = [this]() { saveOptions(); };
	addAndMakeVisible(silenceSlider);

	initLabel(tempoLabel, TRANS("Tempo and beat"));

	tempoSlider.setSliderStyle(Slider::IncDecButtons);
	tempoSlider.setTextBoxStyle(Slider::TextBoxLeft, false, 40, 24);
	tempoSlider.setRange(PianoController::MinTempo, PianoController::MaxTempo, 1);
	tempoSlider.onValueChange = [this]()
		{
			const int tempo = roundToInt(tempoSlider.getValue());
			// the tempo of the accompaniment and the recording, not the song's
			if (tempo != this->pianoController.GetStyleTempo())
			{
				this->pianoController.SetStyleTempo(tempo);
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

	initLabel(metronomeVolumeLabel, TRANS("Metronome volume"));

	metronomeVolumeSlider.setSliderStyle(Slider::LinearHorizontal);
	metronomeVolumeSlider.setTextBoxStyle(Slider::TextBoxLeft, false, 40, 24);
	metronomeVolumeSlider.setRange(0, 127, 1);
	metronomeVolumeSlider.setDoubleClickReturnValue(true, 100); // like the other volumes
	silenceLabel.addMouseListener(this, false);
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

	// what is recorded: one of three (like Automatic / Manual)
	noneButton.setButtonText(TRANS("Playing without accompaniment"));
	noneButton.setTooltip(TRANS("Only what you play is saved. (The accompaniment playing meanwhile is kept too: choosing Playing with accompaniment before saving puts it into the file.)"));
	styleButton.setButtonText(TRANS("Playing with accompaniment"));
	styleButton.setTooltip(TRANS("What you play and the accompaniment (style) of the piano are saved; with automatic start and stop the accompaniment also starts the recording and keeps it going"));
	playbackButton.setButtonText(TRANS("Playing over the song/playback"));
	playbackButton.setTooltip(TRANS("What the program's own player plays (via USB or MIDI device) is recorded too: the loaded song, and a recording played with the Play button too, so a new layer can be recorded to a recording"));
	for (ToggleButton* button : {&noneButton, &styleButton, &playbackButton})
	{
		button->setRadioGroupId(2);
		button->setClickingTogglesState(true);
		button->addListener(this);
		addAndMakeVisible(*button);
	}
	separateButton.setButtonText(TRANS("Playing and accompaniment in separate files too"));
	separateButton.setTooltip(TRANS("Besides the whole recording, the playing (... playing) and the accompaniment (... accompaniment) are saved into files of their own too, with the same name, e.g. to practise with the accompaniment alone"));
	separateButton.addListener(this);
	addAndMakeVisible(separateButton);

	quantizeButton.setButtonText(TRANS("Quantize"));
	quantizeButton.setTooltip(TRANS("The notes of the saved (and listened) recording are moved to the nearest beat division; useful for making a score from a recording played with the metronome or with a song"));
	quantizeButton.addListener(this);
	addAndMakeVisible(quantizeButton);

	quantizeCombo.addItem(TRANS("Quarter note"), 480);
	quantizeCombo.addItem(TRANS("Eighth note"), 240);
	quantizeCombo.addItem(TRANS("Sixteenth note"), 120);
	quantizeCombo.setTooltip(TRANS("The smallest note value: the beginnings of the notes are moved to this grid"));
	quantizeCombo.onChange = [this]() { saveOptions(); };
	addAndMakeVisible(quantizeCombo);

	tripletCombo.addItem(TRANS("No triplets"), NoTripletId);
	tripletCombo.addItem(TRANS("Quarter-note triplet"), 320);
	tripletCombo.addItem(TRANS("Eighth-note triplet"), 160);
	tripletCombo.addItem(TRANS("Sixteenth-note triplet"), 80);
	tripletCombo.setTooltip(TRANS("Triplets recognized too: the program decides for every part of the measure whether it was played in the chosen note value or in these triplets. Usual pairs: eighth note with eighth-note triplet, sixteenth note with eighth-note triplet"));
	tripletCombo.onChange = [this]() { saveOptions(); };
	addAndMakeVisible(tripletCombo);

	quantizeEndsButton.setButtonText(TRANS("Align the ends of the notes too"));
	quantizeEndsButton.setTooltip(TRANS("The ends of the notes are moved to the grid too (cleaner score); otherwise the notes keep their length"));
	quantizeEndsButton.addListener(this);
	addAndMakeVisible(quantizeEndsButton);

	quantizeFillButton.setButtonText(TRANS("Fill the gaps"));
	quantizeFillButton.setTooltip(TRANS("A note lasts until the next note if it was held for at least half of the time between them (no short rests in the score); notes played short stay short"));
	quantizeFillButton.addListener(this);
	addAndMakeVisible(quantizeFillButton);

	initLabel(nameLabel, TRANS("File name"));

	nameEditor.setMultiLine(false);
	nameEditor.setSelectAllWhenFocused(true);
	nameEditor.setInputRestrictions(MaxNameLength);
	nameEditor.setTooltip(TRANS("The name of the recording; the date and the time are added as set below. The name is remembered"));
	nameEditor.setText(settings.recorderName.isNotEmpty() ? settings.recorderName : TRANS("Recording"), false);
	nameEditor.onTextChange = [this]() { updatePreview(); updateControls(); };
	nameEditor.onReturnKey = [this]() { storeName(); };
	nameEditor.onFocusLost = [this]() { storeName(); };
	addAndMakeVisible(nameEditor);

	dateFoldersButton.setButtonText(TRANS("Subfolders by date"));
	dateFoldersButton.setTooltip(TRANS("The recordings are saved in a subfolder named after the date of the recording (e.g. 2026-10-18)"));
	dateFoldersButton.addListener(this);
	addAndMakeVisible(dateFoldersButton);

	timestampButton.setButtonText(TRANS("Time stamp in the file name"));
	timestampButton.setTooltip(TRANS("The time of the start of the recording is added to the file name (without subfolders by date: the date and the time)"));
	timestampButton.addListener(this);
	addAndMakeVisible(timestampButton);

	initLabel(previewLabel, "");
	previewLabel.setFont(Font(FontOptions(13.00f, Font::plain)));
	previewLabel.setColour(Label::textColourId, Colours::white.withAlpha(0.55f));
	previewLabel.setMinimumHorizontalScale(1.0f);

	initLabel(statusLabel, "");
	statusLabel.setMinimumHorizontalScale(0.7f);

	recordButton.setButtonText(TRANS("Record"));
	recordButton.setTooltip(TRANS("Starts the recording (automatic: with the first note); pressed again, it stops the recording"));
	recordButton.addListener(this);
	addAndMakeVisible(recordButton);

	discardButton.setButtonText(TRANS("Discard"));
	discardButton.setTooltip(TRANS("Drops the last recording without saving it (after asking)"));
	discardButton.addListener(this);
	addAndMakeVisible(discardButton);

	listenButton.setButtonText(TRANS("Play"));
	listenButton.setTooltip(TRANS("Plays the last recording, or the loaded one, in the player (it replaces the loaded song); pressed again, it stops the playing, and then continues it"));
	listenButton.addListener(this);
	addAndMakeVisible(listenButton);

	initLabel(positionLabel, "");

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

	folderButton.setButtonText(TRANS("Open folder"));
	folderButton.setTooltip(TRANS("Opens the folder of the recordings (with the saved recording selected)"));
	folderButton.addListener(this);
	addAndMakeVisible(folderButton);

	loadButton.setButtonText(TRANS("Load recording"));
	loadButton.setTooltip(TRANS("Loads an earlier recording (or any MIDI file) into the player, to play it with the Play button, or to record a new layer to it (with Record playback/song; via USB or MIDI device)"));
	loadButton.addListener(this);
	addAndMakeVisible(loadButton);
	initLabel(positionCaption, TRANS("Position"));

	autoButton.setToggleState(settings.recorderAutomatic, dontSendNotification);
	manualButton.setToggleState(!settings.recorderAutomatic, dontSendNotification);
	silenceSlider.setValue(settings.recorderSilence, dontSendNotification);
	styleButton.setToggleState(settings.recorderStyle, dontSendNotification);
	playbackButton.setToggleState(settings.recorderPlayback && !settings.recorderStyle, dontSendNotification);
	noneButton.setToggleState(!styleButton.getToggleState() && !playbackButton.getToggleState(), dontSendNotification);
	separateButton.setToggleState(settings.recorderSeparateStyle, dontSendNotification);
	pianoController.SetRecordPlayback(settings.recorderPlayback);
	countInCombo.setSelectedId(settings.recorderCountIn + 1, dontSendNotification);
	quantizeButton.setToggleState(settings.recorderQuantize, dontSendNotification);
	quantizeCombo.setSelectedId(settings.recorderQuantizeTicks, dontSendNotification);
	tripletCombo.setSelectedId(settings.recorderQuantizeTripletTicks > 0 ? settings.recorderQuantizeTripletTicks : NoTripletId, dontSendNotification);
	if (tripletCombo.getSelectedId() == 0)
	{
		tripletCombo.setSelectedId(NoTripletId, dontSendNotification);
	}
	if (quantizeCombo.getSelectedId() == 0)
	{
		quantizeCombo.setSelectedId(120, dontSendNotification);
	}
	quantizeEndsButton.setToggleState(settings.recorderQuantizeEnds, dontSendNotification);
	quantizeFillButton.setToggleState(settings.recorderQuantizeFill, dontSendNotification);
	dateFoldersButton.setToggleState(settings.recorderDateFolders, dontSendNotification);
	timestampButton.setToggleState(settings.recorderTimestamp, dontSendNotification);

	setSize(440, 704);

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

// vertical positions of the lines between the groups of the controls
static const int SeparatorY[] = {178, 294, 374, 526};

void RecorderComponent::paint(Graphics& g)
{
	g.fillAll(Colour(0xff323e44));
	g.setColour(Colours::white.withAlpha(0.25f));
	for (int y : SeparatorY)
	{
		g.fillRect(12, y, getWidth() - 24, 1);
	}
}

void RecorderComponent::resized()
{
	// (the controls of a row right after the text of its label, without a gap)
	auto textWidth = [](const String& text)
		{
			return GlyphArrangement::getStringWidthInt(Font(FontOptions(15.00f, Font::plain)), text);
		};
	{
		// the value right after the text of the label
		const int silenceTextWidth = GlyphArrangement::getStringWidthInt(silenceLabel.getFont(), silenceLabel.getText());
		const int labelWidth = jlimit(100, 288, silenceTextWidth + 14);
		silenceLabel.setBounds(16, 52, labelWidth, 24);
		silenceSlider.setBounds(16 + labelWidth, 52, 120, 24);
	}
	// start and stop, what is recorded: the two buttons a little after the text
	{
		const int labelWidth = jlimit(60, 200, textWidth(modeLabel.getText()) + 24);
		modeLabel.setBounds(16, 16, labelWidth, 24);
		autoButton.setBounds(16 + labelWidth, 16, 124, 24);
		manualButton.setBounds(autoButton.getRight() + 6, 16, 124, 24);
	}
	{
		// what is recorded: three rows; the separate files beside the accompaniment
		noneButton.setBounds(12, 84, 300, 24);
		const int x = jlimit(180, 240, 12 + textWidth(styleButton.getButtonText()) + 48);
		styleButton.setBounds(12, 112, x - 12 - 4, 24);
		separateButton.setBounds(x, 112, 424 - x, 24);
		playbackButton.setBounds(12, 140, 300, 24);
	}
	// metronome
	metronomeButton.setBounds(12, 188, 120, 24);
	bellButton.setBounds(136, 188, 100, 24);
	countInCombo.setBounds(240, 188, 184, 24);
	{
		const int labelWidth = jlimit(60, 170, textWidth(tempoLabel.getText()) + 10);
		tempoLabel.setBounds(16, 224, labelWidth, 24);
		tempoSlider.setBounds(16 + labelWidth, 224, 120, 24);
		beatCombo.setBounds(16 + labelWidth + 120 + 8, 224, 110, 24);
	}
	{
		// the value box right after the text, the slider to the end of the row
		const int labelWidth = jlimit(60, 170, textWidth(metronomeVolumeLabel.getText()) + 10);
		metronomeVolumeLabel.setBounds(16, 260, labelWidth, 24);
		metronomeVolumeSlider.setBounds(16 + labelWidth, 260, 424 - (16 + labelWidth), 24);
	}
	// quantization
	{
		// the tick box and the text of the button, then the two lists
		const int buttonWidth = jlimit(70, 140, textWidth(quantizeButton.getButtonText()) + 40);
		quantizeButton.setBounds(12, 304, buttonWidth, 24);
		quantizeCombo.setBounds(12 + buttonWidth, 304, 126, 24);
		tripletCombo.setBounds(12 + buttonWidth + 126 + 8, 304, 150, 24);
	}
	quantizeEndsButton.setBounds(12, 340, 222, 24);
	quantizeFillButton.setBounds(240, 340, 184, 24);
	// file and state
	{
		// the name right after its label; the Folder button at the end of the row
		const int labelWidth = jlimit(40, 120, textWidth(nameLabel.getText()) + 10);
		nameLabel.setBounds(16, 384, labelWidth, 24);
		nameEditor.setBounds(16 + labelWidth, 384, 424 - (16 + labelWidth), 24);
	}
	// where the recording goes (like the tick boxes of the quantization), and the preview
	dateFoldersButton.setBounds(12, 416, 222, 24);
	timestampButton.setBounds(240, 416, 184, 24);
	previewLabel.setBounds(16, 446, 408, 20);
	updatePreview();
	folderButton.setBounds(16, 480, 200, 32);
	loadButton.setBounds(224, 480, 200, 32);
	// recording, saving, discarding; the messages; playing back
	recordButton.setBounds(16, 538, 128, 32);
	saveButton.setBounds(156, 538, 128, 32);
	discardButton.setBounds(296, 538, 128, 32);
	statusLabel.setBounds(16, 578, 408, 28);
	listenButton.setBounds(16, 616, 140, 32);
	positionLabel.setBounds(164, 616, 260, 32);
	positionLabel.setJustificationType(Justification::centredRight);
	{
		const int labelWidth = jlimit(40, 120, textWidth(positionCaption.getText()) + 6);
		positionCaption.setBounds(16, 664, labelWidth, 24);
		positionSlider.setBounds(16 + labelWidth, 664, 428 - (16 + labelWidth), 24);
	}
}

void RecorderComponent::mouseDoubleClick(const MouseEvent& event)
{
	if (event.eventComponent == &silenceLabel && silenceSlider.isEnabled())
	{
		silenceSlider.setValue(Settings::DefaultRecorderSilence); // saved by onValueChange
	}
}

void RecorderComponent::buttonClicked(Button* button)
{
	if (button == &recordButton && pianoController.GetRecorder().GetState() != LiveRecorder::stIdle)
	{
		// the same button stops the recording (or cancels the waiting for its start)
		stopRecording();
	}
	else if (button == &recordButton)
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
	else if (button == &discardButton)
	{
		askDiscard();
	}
	else if (button == &loadButton)
	{
		chooseFile();
	}
	else if (button == &listenButton)
	{
		if (isListening())
		{
			// the same button stops the playing; the position is kept
			pianoController.Pause();
		}
		else if (recordingNewest && pianoController.GetRecorder().HasData())
		{
			listen();
		}
		else
		{
			playLoadedFile();
		}
	}
	else if (button == &saveButton)
	{
		save();
	}
	else if (button == &folderButton)
	{
		// the saved recording selected in its folder, or the folder where it will be saved
		if (lastSavedFile.existsAsFile())
		{
			lastSavedFile.revealToUser();
		}
		else
		{
			// with subfolders by date: today's, if there is one
			const File directory = todayFolder();
			directory.createDirectory();
			directory.startAsProcess();
		}
	}
	else if (button == &autoButton || button == &manualButton || button == &noneButton ||
		button == &styleButton || button == &playbackButton || button == &separateButton ||
		button == &quantizeButton || button == &quantizeEndsButton || button == &quantizeFillButton ||
		button == &dateFoldersButton || button == &timestampButton)
	{
		// (the accompaniment or the playback is recorded, not both: radio buttons)
		pianoController.SetRecordPlayback(playbackButton.getToggleState());
		// a recorded accompaniment starts and keeps the automatic recording going
		pianoController.GetRecorder().SetStyleCounts(withStyle());
		saveOptions();
		updateControls();
	}
}

void RecorderComponent::saveOptions()
{
	const bool automatic = autoButton.getToggleState();
	const int silence = roundToInt(silenceSlider.getValue());
	const bool style = styleButton.getToggleState();
	const bool playback = playbackButton.getToggleState();
	const int countIn = jlimit(0, 2, countInCombo.getSelectedId() - 1);
	const bool quantize = quantizeButton.getToggleState();
	const int ticks = quantizeCombo.getSelectedId() > 0 ? quantizeCombo.getSelectedId() : settings.recorderQuantizeTicks;
	const int triplets = tripletTicks();
	const bool ends = quantizeEndsButton.getToggleState();
	const bool fill = quantizeFillButton.getToggleState();
	const bool separate = separateButton.getToggleState();
	const bool dateFolders = dateFoldersButton.getToggleState();
	const bool timestamp = timestampButton.getToggleState();
	if (separate != settings.recorderSeparateStyle || dateFolders != settings.recorderDateFolders || timestamp != settings.recorderTimestamp ||
		automatic != settings.recorderAutomatic || silence != settings.recorderSilence ||
		style != settings.recorderStyle || playback != settings.recorderPlayback ||
		countIn != settings.recorderCountIn ||
		quantize != settings.recorderQuantize || ticks != settings.recorderQuantizeTicks ||
		triplets != settings.recorderQuantizeTripletTicks || ends != settings.recorderQuantizeEnds ||
		fill != settings.recorderQuantizeFill)
	{
		settings.recorderQuantize = quantize;
		settings.recorderQuantizeTicks = ticks;
		settings.recorderQuantizeTripletTicks = triplets;
		settings.recorderQuantizeFill = fill;
		settings.recorderQuantizeEnds = ends;
		settings.recorderCountIn = countIn;
		settings.recorderAutomatic = automatic;
		settings.recorderSilence = silence;
		settings.recorderStyle = style;
		settings.recorderPlayback = playback;
		settings.recorderDateFolders = dateFolders;
		settings.recorderSeparateStyle = separate;
		settings.recorderTimestamp = timestamp;
		settings.Save();
	}
	updatePreview();
	updateControls(); // (Save: another file after a change)
}

void RecorderComponent::startRecording()
{
	if (pianoController.GetRecorder().GetState() != LiveRecorder::stIdle)
	{
		return;
	}
	if (isListening() && !(playbackButton.getToggleState() && canRecordPlayback()))
	{
		// the recording played back would be recorded again as a song (with Record
		// playback/song it goes on: a new layer is recorded to it)
		pianoController.Stop();
	}
	listenSettings = ""; // the recording in the player is the previous one
	saved = false;
	savedName = "";
	message = "";
	// the date and the time of the start go into the file name (see targetFile)
	recordingTime = Time::getCurrentTime();
	hasRecordingTime = true;
	updatePreview();
	const int countIn = jlimit(0, 2, countInCombo.getSelectedId() - 1);
	// a recorded accompaniment starts and keeps the automatic recording going
	pianoController.GetRecorder().SetStyleCounts(withStyle());
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

	storeName();
	// (an existing file is not overwritten: a number is added to the name)
	if (withStyle() && separateButton.getToggleState())
	{
		// the whole recording, and the playing and the accompaniment in files of their own too
		const Array<File> files = targetFiles(true, {String(), TRANS("playing"), TRANS("accompaniment")});
		if (files.isEmpty())
		{
			message = TRANS("Enter a file name.");
			updateStatus();
			return;
		}
		files[0].getParentDirectory().createDirectory();
		writeFiles(files[0], files[1], files[2]);
	}
	else
	{
		const File file = targetFile();
		if (file == File())
		{
			message = TRANS("Enter a file name.");
			updateStatus();
			return;
		}
		file.getParentDirectory().createDirectory();
		writeFile(file);
	}
	updatePreview();
}

File RecorderComponent::targetFile(bool ofRecordingTime) const
{
	const Array<File> files = targetFiles(ofRecordingTime, {String()});
	return files.isEmpty() ? File() : files[0];
}

Array<File> RecorderComponent::targetFiles(bool ofRecordingTime, const StringArray& suffixes) const
{
	String name = File::createLegalFileName(nameEditor.getText().trim());
	if (name.endsWithIgnoreCase(".mid"))
	{
		name = name.dropLastCharacters(4).trim();
	}
	if (name.isEmpty() || suffixes.isEmpty())
	{
		return {};
	}
	// the start of the recording (while it is recorded, or kept to be saved); else: now
	LiveRecorder& recorder = pianoController.GetRecorder();
	const bool ofRecording = ofRecordingTime && hasRecordingTime &&
		(recorder.GetState() != LiveRecorder::stIdle || recorder.HasData());
	const Time time = ofRecording ? recordingTime : Time::getCurrentTime();
	File directory = settings.GetRecordingsDirectory();
	if (dateFoldersButton.getToggleState())
	{
		directory = directory.getChildFile(time.formatted("%Y-%m-%d"));
		if (timestampButton.getToggleState())
		{
			name << " " << time.formatted("%H-%M-%S");
		}
	}
	else if (timestampButton.getToggleState())
	{
		name << " " << time.formatted("%Y-%m-%d %H-%M-%S");
	}
	// the same number for all of them (they belong together)
	Array<File> files;
	for (int number = 1; number < 1000; number++)
	{
		files.clear();
		bool free = true;
		for (const String& suffix : suffixes)
		{
			String fileName = name;
			if (suffix.isNotEmpty())
			{
				fileName << " " << suffix;
			}
			if (number > 1)
			{
				fileName << " (" << number << ")";
			}
			const File file = directory.getChildFile(File::createLegalFileName(fileName) + ".mid");
			free = free && !file.exists();
			files.add(file);
		}
		if (free)
		{
			break;
		}
	}
	return files;
}

File RecorderComponent::todayFolder() const
{
	const File directory = settings.GetRecordingsDirectory();
	const File today = directory.getChildFile(Time::getCurrentTime().formatted("%Y-%m-%d"));
	return dateFoldersButton.getToggleState() && today.isDirectory() ? today : directory;
}

void RecorderComponent::storeName()
{
	const String name = nameEditor.getText().trim();
	if (name != settings.recorderName)
	{
		settings.recorderName = name;
		settings.Save();
	}
}

// The folder and the name of the file the recording is saved to, from the folder of the
// recordings on; shortened in the middle if it does not fit (the end, with the time, is
// kept). The whole path is in the tooltip.
void RecorderComponent::updatePreview()
{
	previewMs = Time::getMillisecondCounter();
	// the folder only (the name with the time would change every second before a recording);
	// the date of the recording that has not been saved yet, otherwise of today
	const File file = targetFile(!saved);
	const File base = settings.GetRecordingsDirectory();
	const File folder = file != File() ? file.getParentDirectory() : base;
	String shown = base.getFileName();
	if (folder != base)
	{
		shown << File::getSeparatorString() << folder.getRelativePathFrom(base);
	}
	const String caption = TRANS("Save to:") + " ";
	const String text = caption + shown;
	if (text == previewText && previewLabel.getWidth() > 0)
	{
		return;
	}
	previewText = text;
	previewLabel.setTooltip(folder.getFullPathName());
	const Font font = previewLabel.getFont();
	const int width = previewLabel.getWidth() - 8;
	String result = text;
	if (width > 0 && GlyphArrangement::getStringWidthInt(font, text) > width)
	{
		// characters are left out in the middle of the path, in the place of an ellipsis
		const String ellipsis = String(CharPointer_UTF8(" \xe2\x80\xa6 "));
		int keepStart = shown.length() / 2;
		int keepEnd = shown.length() - keepStart;
		while (keepStart > 0 || keepEnd > 0)
		{
			if (keepStart >= keepEnd && keepStart > 0)
			{
				keepStart--;
			}
			else if (keepEnd > 0)
			{
				keepEnd--;
			}
			result = caption + shown.substring(0, keepStart).trimEnd() + ellipsis +
				shown.substring(shown.length() - keepEnd).trimStart();
			if (GlyphArrangement::getStringWidthInt(font, result) <= width)
			{
				break;
			}
		}
	}
	previewLabel.setText(result, dontSendNotification);
}

// The whole recording (playing and accompaniment), and the playing and the accompaniment in
// files of their own too (the accompaniment alone, with the same times). If no
// accompaniment was recorded, its file is not written, with a message.
void RecorderComponent::writeFiles(const File& wholeFile, const File& playingFile, const File& styleFile)
{
	writeFile(wholeFile);
	if (!saved || savedName != wholeFile.getFileName())
	{
		return; // (could not be saved: the message is shown)
	}
	String error;
	const bool playingOk = pianoController.SaveRecording(playingFile, false, error, true,
		quantizeTicks(), quantizeEndsButton.getToggleState(), tripletTicks(),
		quantizeFillButton.getToggleState());
	if (playingOk)
	{
		savedName << ", " << playingFile.getFileName();
	}
	error.clear();
	if (pianoController.SaveRecording(styleFile, true, error, true,
		quantizeTicks(), quantizeEndsButton.getToggleState(), tripletTicks(),
		quantizeFillButton.getToggleState(), true))
	{
		savedName << ", " << styleFile.getFileName();
	}
	if (!playingOk)
	{
		message = TRANS("The file could not be saved.");
	}
	else if (error == "No accompaniment was recorded")
	{
		message = TRANS("No accompaniment was recorded: only the playing was saved.");
	}
	else if (error.isNotEmpty())
	{
		message = TRANS("The accompaniment file could not be saved.");
	}
	updateControls();
	updateStatus();
}

void RecorderComponent::writeFile(const File& file)
{
	String error;
	applyKeySignature();
	if (pianoController.SaveRecording(file, withStyle(), error, true,
		quantizeTicks(), quantizeEndsButton.getToggleState(), tripletTicks(),
		quantizeFillButton.getToggleState()))
	{
		saved = true;
		savedKey = saveKey();
		savedName = file.getFileName();
		lastSavedFile = file;
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

	if (listenLoaded && !fileLoaded && listenSettings == listenKey() && !pianoController.GetPlaying())
	{
		// the recording is in the player as it would be written now: continues where it
		// was stopped (from the beginning if it was played to the end)
		if (pianoController.GetPosition().measure >= pianoController.GetLength().measure)
		{
			pianoController.SetPosition({1, 1});
		}
		pianoController.Play();
		return;
	}

	const File directory = File::getSpecialLocation(File::tempDirectory).getChildFile("ConPianist");
	directory.createDirectory();
	const File file = directory.getChildFile(File::createLegalFileName(TRANS("Recording (listening)")) + ".mid");
	String error;
	applyKeySignature();
	if (!pianoController.SaveRecording(file, withStyle(), error, false,
		quantizeTicks(), quantizeEndsButton.getToggleState(), tripletTicks(),
		quantizeFillButton.getToggleState()))
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
	fileLoaded = false;
	listenSettings = listenKey();
	playPending = pendingRecording; // played when the player reports that the song is loaded
	if (!pianoController.LoadSong(file))
	{
		playPending = pendingNone;
		listenSettings = "";
		message = TRANS("The recording could not be played.");
	}
	updateControls();
	updateStatus();
}

// Play: the loaded file, from where it was stopped (from the beginning if it was played
// to the end); loaded again if another song has replaced it in the player.
void RecorderComponent::playLoadedFile()
{
	if (!loadedFile.existsAsFile())
	{
		return;
	}
	if (listenLoaded && fileLoaded)
	{
		if (pianoController.GetPosition().measure >= pianoController.GetLength().measure)
		{
			pianoController.SetPosition({1, 1});
		}
		pianoController.Play();
		return;
	}
	loadFile(loadedFile, true);
}

// Load recording: an earlier recording (any MIDI file) from the folder of the recordings.
void RecorderComponent::chooseFile()
{
	const File directory = todayFolder(); // with subfolders by date: today's, if there is one
	directory.createDirectory();
	GuiHelper::ShowFileOpenDialogAsync(TRANS("Please select the recording you want to load..."),
		directory.getFullPathName(), "*.mid;*.midi",
		[this, self = Component::SafePointer<Component>(this)](const URL& url)
		{
			if (self == nullptr)
			{
				return; // deleted meanwhile
			}
			const File file = url.getLocalFile();
			if (file.existsAsFile() && pianoController.GetRecorder().GetState() == LiveRecorder::stIdle)
			{
				loadFile(file, false);
			}
		});
}

// Loads a file into the player (in the place of the loaded song); it is played with the
// Play button (or at once: play).
void RecorderComponent::loadFile(const File& file, bool play)
{
	if (pianoController.GetPlaying())
	{
		pianoController.Stop();
	}
	loadedFile = file;
	recordingNewest = false; // Play plays the loaded file
	message = "";
	listenLoaded = false;
	fileLoaded = false;
	listenSettings = "";
	playPending = play ? pendingFilePlay : pendingFile;
	if (!pianoController.LoadSong(file))
	{
		playPending = pendingNone;
		loadedFile = File();
		message = TRANS("The recording could not be played.");
	}
	updateControls();
	updateStatus();
}

// Discard: the recording that has not been saved is dropped (after asking).
void RecorderComponent::askDiscard()
{
	LiveRecorder& recorder = pianoController.GetRecorder();
	if (!recorder.HasData() || saved)
	{
		return;
	}
	AlertWindow::showAsync(MessageBoxOptions()
			.withIconType(MessageBoxIconType::QuestionIcon)
			.withTitle("ConPianist")
			.withMessage(TRANS("Discard the last recording? It will not be saved."))
			.withButton(TRANS("Yes"))
			.withButton(TRANS("Cancel"))
			.withAssociatedComponent(this),
		[this, self = Component::SafePointer<Component>(this)](int result)
		{
			if (self == nullptr || result != 1)
			{
				return;
			}
			LiveRecorder& recorder = pianoController.GetRecorder();
			if (recorder.GetState() != LiveRecorder::stIdle)
			{
				return; // a new recording was started meanwhile
			}
			if (listenLoaded && !fileLoaded)
			{
				// the dropped recording is in the player: it is not played any more
				if (pianoController.GetPlaying())
				{
					pianoController.Stop();
				}
				listenLoaded = false;
			}
			recorder.Clear();
			saved = false;
			savedName = "";
			recordingNewest = false; // Play plays the loaded file again, if there is one
			listenSettings = "";
			message = "";
			updateControls();
			updateStatus();
		});
}

// The accompaniment is recorded: chosen, and the piano is connected (the accompaniment
// comes from it). A recording made while it was connected keeps its accompaniment.
bool RecorderComponent::withStyle() const
{
	return styleButton.getToggleState() &&
		(pianoController.IsConnected() || pianoController.GetRecorder().HasData());
}

bool RecorderComponent::canRecordPlayback() const
{
	return pianoController.GetPlaybackSource() != PianoController::psPiano;
}

String RecorderComponent::saveKey() const
{
	return listenKey() + "|" + nameEditor.getText().trim() + "|" +
		String((int)dateFoldersButton.getToggleState()) + String((int)timestampButton.getToggleState()) +
		String((int)(withStyle() && separateButton.getToggleState()));
}

String RecorderComponent::listenKey() const
{
	return String(quantizeTicks()) + "/" + String(tripletTicks()) + "/" +
		String((int)quantizeEndsButton.getToggleState()) + "/" + String((int)quantizeFillButton.getToggleState()) +
		"/" + String((int)withStyle()) + "/" + settings.accompanimentKey +
		(settings.accompanimentMinor ? "m" : "");
}

// The key chosen in the Accompaniment window is written into the file as its key
// signature (none, if no key is chosen).
void RecorderComponent::applyKeySignature()
{
	// the names of the recorded chords, with the sharps or flats of the key
	int keySharps = 0;
	const int accidentals = !settings.GetKeySignature(keySharps) ? 0 : keySharps > 0 ? +1 : keySharps < 0 ? -1 : 0;
	pianoController.GetRecorder().chordName = [accidentals](int chord)
		{
			PianoController::StyleChord styleChord;
			styleChord.root = (chord >> 24) & 0x7f;
			styleChord.type = (chord >> 16) & 0x7f;
			styleChord.bassRoot = (chord >> 8) & 0x7f;
			styleChord.bassType = chord & 0x7f;
			return AccompanimentComponent::chordName(styleChord, accidentals);
		};

	int sharps = 0;
	if (settings.GetKeySignature(sharps))
	{
		pianoController.GetRecorder().SetKeySignature(sharps, settings.accompanimentMinor);
	}
	else
	{
		pianoController.GetRecorder().SetNoKeySignature();
	}
}

int RecorderComponent::quantizeTicks() const
{
	return quantizeButton.getToggleState() ? jmax(0, quantizeCombo.getSelectedId()) : 0;
}

int RecorderComponent::tripletTicks() const
{
	const int id = tripletCombo.getSelectedId();
	return id > NoTripletId ? id : 0;
}

// The time and the measure where the listening back is (the beginning, if the recording
// is not in the player), and the length of the recording.
void RecorderComponent::updatePosition()
{
	LiveRecorder& recorder = pianoController.GetRecorder();
	String text;
	if (listenLoaded && fileLoaded)
	{
		// the loaded file: its measure (its tempo is its own)
		const PianoController::Position position = pianoController.GetPosition();
		const String bullet = " " + String(CharPointer_UTF8("\xe2\x80\xa2")) + " ";
		text = TRANS("Measure NUMBER").replace("NUMBER", String(jmax(1, position.measure))) +
			bullet + String(jmax(1, position.beat));
	}
	else if (recorder.HasData())
	{
		PianoController::Position position = {1, 1};
		if (listenLoaded)
		{
			position = pianoController.GetPosition();
		}
		const int measure = jmax(1, position.measure);
		const int beat = jmax(1, position.beat);
		// the tempo counts quarter notes; a beat is one note of the denominator
		const double beats = (measure - 1) * (double)jmax(1, pianoController.GetRecordedNumerator()) + (beat - 1);
		const double seconds = beats * 4.0 / jmax(1, pianoController.GetRecordedDenominator()) *
			60.0 / jmax(1, pianoController.GetRecordedTempo());
		const String bullet = " " + String(CharPointer_UTF8("\xe2\x80\xa2")) + " ";
		text = formatTime(seconds) + " / " + formatTime(recorder.GetLengthSeconds()) + bullet +
			TRANS("Measure NUMBER").replace("NUMBER", String(measure));
	}
	if (positionLabel.getText() != text)
	{
		positionLabel.setText(text, dontSendNotification);
	}
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
		// the song loaded for playing back (a recording or the loaded file), or another
		// song that replaces it
		const int pending = playPending.exchange(pendingNone);
		listenLoaded = pending != pendingNone;
		fileLoaded = pending == pendingFile || pending == pendingFilePlay;
		if (pending == pendingRecording || pending == pendingFilePlay)
		{
			GuiHelper::CallAsync(this, [this]()
				{
					if (listenLoaded)
					{
						pianoController.Play();
					}
				});
		}
		GuiHelper::CallAsync(this, [this]() { updateControls(); updateStatus(); });
	}
	else if (aspect == PianoController::apPlaybackSource || aspect == PianoController::apConnection)
	{
		GuiHelper::CallAsync(this, [this]() { updateControls(); });
	}
}

void RecorderComponent::timerCallback()
{
	LiveRecorder& recorder = pianoController.GetRecorder();
	const LiveRecorder::State state = recorder.GetState();
	if (Time::getMillisecondCounter() - previewMs >= 500)
	{
		updatePreview(); // (the time shown goes on before a recording)
	}

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
	if (newState != lastState && newState == LiveRecorder::stIdle && recorder.HasData())
	{
		recordingNewest = true; // Play plays the new recording
	}
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
	// the Record button blinks while the recording waits for its start
	const bool blinkOn = (Time::getMillisecondCounter() / 500) % 2 == 0;
	if (recordButton.look == RecordButton::Look::waiting && recordButton.blinkOn != blinkOn)
	{
		recordButton.blinkOn = blinkOn;
		recordButton.repaint();
	}
	updatePosition();
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
	const bool canPlayback = canRecordPlayback();
	const bool hasFile = loadedFile.existsAsFile();

	// Record / Stop
	const LiveRecorder::State state = recorder.GetState();
	const RecordButton::Look look = state == LiveRecorder::stRecording ? RecordButton::Look::recording :
		idle ? RecordButton::Look::idle : RecordButton::Look::waiting;
	if (recordButton.look != look)
	{
		recordButton.look = look;
		recordButton.blinkOn = true;
		recordButton.repaint();
	}
	// waiting for the start (armed, count-in): "Record", blinking; pressed, it cancels
	recordButton.setButtonText(state == LiveRecorder::stRecording ? TRANS("Stop") : TRANS("Record"));
	recordButton.setEnabled(true);

	// Play / Stop: the new recording, or the loaded file; a loaded file can be started
	// during a recording too (a new layer recorded to it)
	const bool playsRecording = recordingNewest && hasData;
	listenButton.setEnabled(listening || playsRecording || (hasFile && (idle || canPlayback)));
	listenButton.setButtonText(listening ? TRANS("Stop playing") : TRANS("Play"));
	positionSlider.setEnabled(listenLoaded);
	if (!listenLoaded)
	{
		positionSlider.setValue(1, dontSendNotification);
	}
	// after saving, only when something was changed that gives another file (another
	// quantization, name, folder): e.g. a plain and a quantized version can be saved
	saveButton.setEnabled(hasData && (!saved || saveKey() != savedKey));
	discardButton.setEnabled(hasData && !saved);
	// (the name can be changed at any time: it is used when the recording is saved)
	// loading replaces the song of the player: not during a recording. Via network it is
	// only played (the piano's player plays the file): a new layer cannot be recorded to it
	// (Record playback/song is off then, and Record stops the playback, see startRecording)
	loadButton.setEnabled(idle);
	noneButton.setEnabled(idle);
	// without the piano there is no accompaniment (greyed; it is valid again when the
	// piano is connected), like the playback via network
	styleButton.setEnabled(idle && pianoController.IsConnected());
	playbackButton.setEnabled(idle && canPlayback);
	separateButton.setEnabled(withStyle()); // a choice of the saving
	countInCombo.setEnabled(idle && manualButton.getToggleState());
	quantizeCombo.setEnabled(quantizeButton.getToggleState());
	tripletCombo.setEnabled(quantizeButton.getToggleState());
	quantizeEndsButton.setEnabled(quantizeButton.getToggleState());
	quantizeFillButton.setEnabled(quantizeButton.getToggleState());
}

// Shows the tempo and the time signature of the piano (of the accompaniment and the
// recording; the song has its own tempo, on the Playback panel) and the state of the
// metronome as they are now (they can change on the piano and with the song too).
void RecorderComponent::updateMetronome()
{
	if (!tempoSlider.hasKeyboardFocus(true) && !tempoSlider.isMouseButtonDown(true))
	{
		tempoSlider.setValue(pianoController.GetStyleTempo(), dontSendNotification);
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

// The length of the recording in measures: of the recording in the player while it is
// played back, otherwise from its length in seconds, with its tempo and time signature
// (as the position is counted in updatePosition).
int RecorderComponent::recordingMeasures()
{
	if (isListening() && !fileLoaded)
	{
		const int measures = pianoController.GetLength().measure;
		if (measures > 0)
		{
			return measures;
		}
	}
	const double measureSeconds = jmax(1, pianoController.GetRecordedNumerator()) * 4.0 /
		jmax(1, pianoController.GetRecordedDenominator()) * 60.0 / jmax(1, pianoController.GetRecordedTempo());
	return jmax(1, (int)std::ceil(pianoController.GetRecorder().GetLengthSeconds() / measureSeconds - 0.001));
}

void RecorderComponent::updateStatus()
{
	LiveRecorder& recorder = pianoController.GetRecorder();
	const String bullet = " " + String(CharPointer_UTF8("\xe2\x80\xa2")) + " ";
	const String info = formatTime(recorder.GetLengthSeconds()) + bullet +
		TRANS("NUMBER measures").replace("NUMBER", String(recordingMeasures()));
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
			else if (isListening() && fileLoaded)
			{
				text = TRANS("Playing:") + " " + loadedFile.getFileName();
				colour = Colours::lightblue;
			}
			else if (isListening())
			{
				text = TRANS("Playing the recording:") + " " + info;
				colour = Colours::lightblue;
			}
			else if (!(recordingNewest && recorder.HasData()) && loadedFile.existsAsFile())
			{
				text = TRANS("Loaded:") + " " + loadedFile.getFileName();
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

void RecorderComponent::RecordButton::paintButton(Graphics& g, bool highlighted, bool down)
{
	const Colour red(0xffe53935);
	const Colour dim(0xff7a2a2a);
	const float alpha = isEnabled() ? 1.0f : 0.5f;
	const Rectangle<float> r = getLocalBounds().toFloat().reduced(1.0f);
	const bool recording = look == Look::recording;
	const bool lit = look != Look::waiting || blinkOn;

	if (recording)
	{
		g.setColour((down ? red.darker(0.2f) : highlighted ? red.brighter(0.1f) : red).withMultipliedAlpha(alpha));
		g.fillRoundedRectangle(r, 5.0f);
	}
	else
	{
		if (highlighted || down)
		{
			g.setColour(Colour(down ? 0xff3d484e : 0xff3d484e).withMultipliedAlpha(alpha));
			g.fillRoundedRectangle(r, 5.0f);
		}
		g.setColour((lit ? red : dim).withMultipliedAlpha(alpha));
		g.drawRoundedRectangle(r.reduced(1.0f), 5.0f, 2.0f);
	}

	// the sign (a dot, or a square while recording) and the bold text, in the middle
	const Font font(FontOptions(15.0f, Font::bold));
	const String text = getButtonText();
	const float textWidth = GlyphArrangement::getStringWidth(font, text);
	const float sign = 12.0f;
	const float gap = 7.0f;
	const float x = r.getCentreX() - (sign + gap + textWidth) / 2.0f;
	const float cy = r.getCentreY();
	if (recording)
	{
		g.setColour(Colours::white.withMultipliedAlpha(alpha));
		g.fillRect(x + 1.0f, cy - 5.0f, 10.0f, 10.0f);
	}
	else
	{
		g.setColour((lit ? red : dim).withMultipliedAlpha(alpha));
		g.fillEllipse(x, cy - sign / 2.0f, sign, sign);
	}
	g.setColour(Colours::white.withMultipliedAlpha(alpha));
	g.setFont(font);
	g.drawText(text, Rectangle<float>(x + sign + gap, r.getY(), textWidth + 2.0f, r.getHeight()), Justification::centredLeft, false);
}

//==============================================================================

RecorderWindow::RecorderWindow(Settings& settings, PianoController& pianoController) :
	DocumentWindow(TRANS("Recording"), Colour(0xff323e44),
		DocumentWindow::minimiseButton | DocumentWindow::closeButton),
	settings(settings)
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

// The position is remembered (saved with the settings when the program exits).
void RecorderWindow::moved()
{
	DocumentWindow::moved();
	if (isShowing() && !isMinimised() && getX() > -10000 && getY() > -10000)
	{
		settings.recorderWindowPos = getPosition();
	}
}

bool RecorderWindow::RestorePosition()
{
	if (!Settings::IsWindowPosUsable(settings.recorderWindowPos, getWidth()))
	{
		return false;
	}
	setTopLeftPosition(settings.recorderWindowPos);
	return true;
}
