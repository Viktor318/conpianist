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
#include "Settings.h"

// The content of the Recording window: records what is played live (see LiveRecorder)
// and saves it into a MIDI file in the songs folder. The recording can be played back:
// it is loaded into the player as a song (from a temporary file). An earlier recording
// (any MIDI file) can be loaded into the player too, and with "Record playback/song" a new
// layer can be recorded while it is playing.
class RecorderComponent : public Component,
                          public Button::Listener,
                          public Timer,
                          public PianoController::Listener
{
public:
	RecorderComponent(Settings& settings, PianoController& pianoController);
	~RecorderComponent() override;

	void paint(Graphics& g) override;
	void resized() override;
	void buttonClicked(Button* button) override;
	void timerCallback() override;
	void PianoStateChanged(PianoController::Aspect aspect, PianoController::Channel channel) override;
	// double click on the label of the silence: back to its default
	void mouseDoubleClick(const MouseEvent& event) override;

private:
	Settings& settings;
	PianoController& pianoController;

	Label modeLabel;
	ToggleButton autoButton;
	ToggleButton manualButton;
	Label silenceLabel;
	Slider silenceSlider;
	Label tempoLabel;
	Slider tempoSlider;
	ComboBox beatCombo;
	ToggleButton metronomeButton;
	ToggleButton bellButton;
	Label metronomeVolumeLabel;
	Slider metronomeVolumeSlider;
	ComboBox countInCombo;
	// what is recorded (one of the three): the playing only, with the accompaniment, or over
	// the playback of the own player (a song, or a recording: a new layer)
	ToggleButton noneButton;
	ToggleButton styleButton;
	ToggleButton playbackButton; // the playback of the own player is recorded (not with styleButton)
	ToggleButton separateButton; // with the accompaniment: two files, the playing and the accompaniment
	ToggleButton quantizeButton;
	ComboBox quantizeCombo;      // the grid; the item ids are ticks
	ComboBox tripletCombo;       // the triplet grid; the item ids are ticks, NoTripletId: none
	ToggleButton quantizeEndsButton;
	ToggleButton quantizeFillButton;
	Label nameLabel;
	TextEditor nameEditor;       // the name of the file, without the date and the time
	ToggleButton dateFoldersButton; // the date of the recording is a subfolder
	ToggleButton timestampButton;   // the time (and the date) is added to the name
	Label previewLabel;          // where and under what name the recording is saved
	String previewText;          // (shown shortened in the middle if it does not fit)
	Label statusLabel;

	// The Record button, the main function of the window, stands out: a red dot and a red
	// frame; it blinks while a recording waits for its start (armed, count-in); while
	// recording it is filled red, with a white square, and it stops the recording.
	class RecordButton : public TextButton
	{
	public:
		enum class Look { idle, waiting, recording };
		Look look = Look::idle;
		bool blinkOn = true;
		void paintButton(Graphics& g, bool highlighted, bool down) override;
	};
	RecordButton recordButton;
	TextButton discardButton;    // drops the recording that has not been saved
	TextButton listenButton;     // Play / Stop: the recording, or the loaded one
	Label positionLabel;     // time and measure of the listening back
	Slider positionSlider;   // position in the recording while it is listened back (measures)
	TextButton saveButton;
	TextButton folderButton; // opens the folder of the recordings
	TextButton loadButton;   // loads an earlier recording into the player
	Label positionCaption;   // the label of the position slider
	File lastSavedFile;      // selected in the folder, if it still exists
	// (the tooltips are shown by the tooltip window of the application, see Main.cpp)

	LiveRecorder::State lastState = LiveRecorder::stIdle;
	bool saved = false;      // the recording in the recorder has been saved
	String savedName;        // ... under this name
	// What is being loaded into the player (see the Pending values), and what is in it: a
	// recording (listenLoaded) or a loaded file (listenLoaded and fileLoaded).
	enum Pending { pendingNone = 0, pendingRecording = 1, pendingFileLoad = 2, pendingFilePlay = 3 };
	std::atomic<int> playPending{pendingNone};
	File pendingFile;     // the file being loaded (only that one counts, see songLoaded)
	uint32 pendingMs = 0; // when it was asked for
	static const uint32 PendingTimeoutMs = 60000; // an upload to the piano may take a while
	void songLoaded();
	std::atomic<bool> listenLoaded{false}; // the song in the player is ours
	std::atomic<bool> fileLoaded{false};   // ... and it is the loaded file
	File loadedFile;            // loaded with "Load recording"
	bool recordingNewest = false; // the recording is newer than the loaded file: Play plays it
	bool lastListening = false;
	String listenSettings;   // how the recording in the player was written (see listenKey)
	String message;          // shown until the next recording (e.g. an error)
	bool metronomeStarted = false; // the metronome was switched on for the count-in
	Time recordingTime;      // the start of the recording (its date and time in the file name)
	bool hasRecordingTime = false;
	uint32 previewMs = 0;    // when the preview was updated last

	void startRecording();
	void stopRecording();
	void save();
	void listen();
	void playLoadedFile();
	void chooseFile();
	void loadFile(const File& file, bool play);
	void askDiscard();
	// the playback can be recorded now (USB or MIDI device: the own player plays)
	bool canRecordPlayback() const;
	bool withStyle() const; // the accompaniment is recorded (chosen, and the piano is there)
	bool recordedWithPiano = false; // the recording in hand was made with the piano connected
	static const int MaxFileNameLength = 128; // (JUCE cuts longer file names)
	String listenKey() const; // the settings that change what is listened back
	void applyKeySignature();
	static const int NoTripletId = 1;
	int quantizeTicks() const; // 0: not quantized
	int tripletTicks() const;  // 0: no triplets
	void updatePosition();
	bool isListening() const; // the recording is being played back
	bool writeFile(const File& file); // false: it could not be saved (the message is shown)
	// three files: the whole recording, the playing, the accompaniment (separateButton)
	void writeFiles(const File& wholeFile, const File& playingFile, const File& styleFile);
	// The file the recording is saved to: in the folder of the recordings (or in the
	// subfolder of its date), with the time (and date) added; a number is added if such a
	// file exists. An invalid file if no name is given.
	// ofRecording: the date and time of the recording (else of now, for the preview)
	File targetFile(bool ofRecording = true) const;
	// The same for several files saved together (e.g. "... playing" and "... accompaniment"):
	// a suffix is added to the name of each; a number is added to all of them if any exists.
	Array<File> targetFiles(bool ofRecording, const StringArray& suffixes) const;
	String saveKey() const;  // what the saved file depends on: the settings and the name
	String savedKey;         // ... when it was saved last (the same again: Save is greyed)
	File todayFolder() const; // the folder of the recordings, or today's subfolder if it exists
	void storeName();         // the name typed is remembered
	void updatePreview();
	void saveOptions();
	void updateControls();
	void updateMetronome();
	void updateStatus();
	int recordingMeasures(); // the length of the recording in measures
	static String formatTime(double seconds);
};

// The Recording window: not modal and always on top, so the program can be used (e.g.
// the virtual keyboard played) while it is open. Closing only hides it: a recording
// goes on, and the recorded music is kept until the program exits.
class RecorderWindow : public DocumentWindow
{
public:
	RecorderWindow(Settings& settings, PianoController& pianoController);
	void closeButtonPressed() override;
	void moved() override;
	// Moves the window where it was the last time; false if that is not known (or not
	// on a screen any more).
	bool RestorePosition();

private:
	Settings& settings;
};
