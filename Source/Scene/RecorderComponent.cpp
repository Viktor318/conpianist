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

	initLabel(metronomeVolumeLabel, TRANS("Metronome volume"));

	metronomeVolumeSlider.setSliderStyle(Slider::LinearHorizontal);
	metronomeVolumeSlider.setTextBoxStyle(Slider::TextBoxLeft, false, 40, 24);
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
	listenButton.setTooltip(TRANS("Loads the recording into the player and plays it (it replaces the loaded song); pressed again, it stops the playing and then continues it"));
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

	folderButton.setButtonText(TRANS("Folder"));
	folderButton.setTooltip(TRANS("Opens the folder of the recordings (with the saved recording selected)"));
	folderButton.addListener(this);
	addAndMakeVisible(folderButton);
	initLabel(positionCaption, TRANS("Position"));

	autoButton.setToggleState(settings.recorderAutomatic, dontSendNotification);
	manualButton.setToggleState(!settings.recorderAutomatic, dontSendNotification);
	silenceSlider.setValue(settings.recorderSilence, dontSendNotification);
	styleButton.setToggleState(settings.recorderStyle, dontSendNotification);
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

	setSize(440, 544);

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
static const int SeparatorY[] = {122, 238, 318, 452};

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
	// start and stop, what is recorded: the two buttons as far apart as before; the last
	// letter of the Manual button stands above the left edge of the + button of the silence
	// (value box 40, then the - and + buttons of 40 each, drawn 3 pixels narrower on the
	// side of the value box; the text of a toggle button begins after its 17 pixel tick box
	// and 10 pixels)
	{
		const String manualText = manualButton.getButtonText();
		const float lastLetter = (textWidth(manualText.dropLastCharacters(1)) + textWidth(manualText)) / 2.0f;
		const int plusLeft = silenceSlider.getX() + 40 + 40 + 3;
		const int manualX = plusLeft - 27 - roundToInt(lastLetter);
		const int minLabelWidth = textWidth(modeLabel.getText()) + 10;
		const int labelWidth = jmax(minLabelWidth, manualX - 6 - 124 - 16);
		modeLabel.setBounds(16, 16, labelWidth, 24);
		autoButton.setBounds(16 + labelWidth, 16, 124, 24);
		manualButton.setBounds(autoButton.getRight() + 6, 16, 124, 24);
	}
	styleButton.setBounds(12, 88, 412, 24);
	// metronome
	metronomeButton.setBounds(12, 132, 120, 24);
	bellButton.setBounds(136, 132, 100, 24);
	countInCombo.setBounds(240, 132, 184, 24);
	{
		const int labelWidth = jlimit(60, 170, textWidth(tempoLabel.getText()) + 10);
		tempoLabel.setBounds(16, 168, labelWidth, 24);
		tempoSlider.setBounds(16 + labelWidth, 168, 120, 24);
		beatCombo.setBounds(16 + labelWidth + 120 + 8, 168, 110, 24);
	}
	{
		// the value box right after the text, the slider to the end of the row
		const int labelWidth = jlimit(60, 170, textWidth(metronomeVolumeLabel.getText()) + 10);
		metronomeVolumeLabel.setBounds(16, 204, labelWidth, 24);
		metronomeVolumeSlider.setBounds(16 + labelWidth, 204, 424 - (16 + labelWidth), 24);
	}
	// quantization
	{
		// the tick box and the text of the button, then the two lists
		const int buttonWidth = jlimit(70, 140, textWidth(quantizeButton.getButtonText()) + 40);
		quantizeButton.setBounds(12, 248, buttonWidth, 24);
		quantizeCombo.setBounds(12 + buttonWidth, 248, 126, 24);
		tripletCombo.setBounds(12 + buttonWidth + 126 + 8, 248, 150, 24);
	}
	quantizeEndsButton.setBounds(12, 284, 222, 24);
	quantizeFillButton.setBounds(240, 284, 184, 24);
	// file and state
	{
		// the name right after its label; the Folder button at the end of the row
		const int labelWidth = jlimit(40, 120, textWidth(nameLabel.getText()) + 10);
		const int folderWidth = 84;
		nameLabel.setBounds(16, 328, labelWidth, 24);
		nameEditor.setBounds(16 + labelWidth, 328, 424 - folderWidth - 8 - (16 + labelWidth), 24);
		folderButton.setBounds(424 - folderWidth, 326, folderWidth, 28);
	}
	statusLabel.setBounds(16, 364, 408, 36);
	// buttons, listening back
	recordButton.setBounds(16, 408, 128, 32);
	stopButton.setBounds(156, 408, 128, 32);
	saveButton.setBounds(296, 408, 128, 32);
	listenButton.setBounds(16, 464, 160, 32);
	positionLabel.setBounds(188, 464, 236, 32);
	{
		const int labelWidth = jlimit(40, 120, textWidth(positionCaption.getText()) + 6);
		positionCaption.setBounds(16, 504, labelWidth, 24);
		positionSlider.setBounds(16 + labelWidth, 504, 428 - (16 + labelWidth), 24);
	}
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
		if (isListening())
		{
			// the same button stops the listening back; the position is kept
			pianoController.Pause();
		}
		else
		{
			listen();
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
			const File directory = settings.GetDefaultSongDirectory();
			directory.createDirectory();
			directory.startAsProcess();
		}
	}
	else if (button == &autoButton || button == &manualButton || button == &styleButton ||
		button == &quantizeButton || button == &quantizeEndsButton || button == &quantizeFillButton)
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
	const int triplets = tripletTicks();
	const bool ends = quantizeEndsButton.getToggleState();
	const bool fill = quantizeFillButton.getToggleState();
	if (automatic != settings.recorderAutomatic || silence != settings.recorderSilence ||
		style != settings.recorderStyle || countIn != settings.recorderCountIn ||
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
	listenSettings = ""; // the recording in the player is the previous one
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
	applyKeySignature();
	if (pianoController.SaveRecording(file, styleButton.getToggleState(), error, true,
		quantizeTicks(), quantizeEndsButton.getToggleState(), tripletTicks(),
		quantizeFillButton.getToggleState()))
	{
		saved = true;
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

	if (listenLoaded && listenSettings == listenKey() && !pianoController.GetPlaying())
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
	if (!pianoController.SaveRecording(file, styleButton.getToggleState(), error, false,
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
	listenSettings = listenKey();
	playPending = true; // played when the player reports that the song is loaded
	if (!pianoController.LoadSong(file))
	{
		playPending = false;
		listenSettings = "";
		message = TRANS("The recording could not be played.");
	}
	updateControls();
	updateStatus();
}

String RecorderComponent::listenKey() const
{
	return String(quantizeTicks()) + "/" + String(tripletTicks()) + "/" +
		String((int)quantizeEndsButton.getToggleState()) + "/" + String((int)quantizeFillButton.getToggleState()) +
		"/" + String((int)styleButton.getToggleState()) + "/" + settings.accompanimentKey +
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
	if (recorder.HasData())
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
	recordButton.setEnabled(idle);
	stopButton.setEnabled(!idle || listening);
	listenButton.setEnabled(hasData);
	listenButton.setButtonText(listening ? TRANS("Stop listening") : TRANS("Listen"));
	positionSlider.setEnabled(listenLoaded);
	if (!listenLoaded)
	{
		positionSlider.setValue(1, dontSendNotification);
	}
	saveButton.setEnabled(hasData);
	nameEditor.setEnabled(hasData);
	countInCombo.setEnabled(idle && manualButton.getToggleState());
	quantizeCombo.setEnabled(quantizeButton.getToggleState());
	tripletCombo.setEnabled(quantizeButton.getToggleState());
	quantizeEndsButton.setEnabled(quantizeButton.getToggleState());
	quantizeFillButton.setEnabled(quantizeButton.getToggleState());
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
