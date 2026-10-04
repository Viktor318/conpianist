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
// and saves it into a MIDI file in the songs folder. The recording can be listened back:
// it is loaded into the player as a song (from the saved file, or from a temporary one).
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
	ToggleButton styleButton;
	ToggleButton quantizeButton;
	ComboBox quantizeCombo;      // the grid; the item ids are ticks
	ComboBox tripletCombo;       // the triplet grid; the item ids are ticks, NoTripletId: none
	ToggleButton quantizeEndsButton;
	ToggleButton quantizeFillButton;
	Label nameLabel;
	TextEditor nameEditor;
	Label statusLabel;
	TextButton recordButton;
	TextButton stopButton;
	TextButton listenButton;
	Label positionLabel;     // time and measure of the listening back
	Slider positionSlider;   // position in the recording while it is listened back (measures)
	TextButton saveButton;
	// (the tooltips are shown by the tooltip window of the application, see Main.cpp)

	LiveRecorder::State lastState = LiveRecorder::stIdle;
	bool saved = false;      // the recording in the recorder has been saved
	String savedName;        // ... under this name
	std::atomic<bool> playPending{false}; // the recording is being loaded for listening back
	std::atomic<bool> listenLoaded{false}; // the song in the player is the recording
	bool lastListening = false;
	String listenSettings;   // how the recording in the player was written (see listenKey)
	String message;          // shown until the next recording (e.g. an error)
	bool metronomeStarted = false; // the metronome was switched on for the count-in

	void startRecording();
	void stopRecording();
	void save();
	void listen();
	String listenKey() const; // the settings that change what is listened back
	static const int NoTripletId = 1;
	int quantizeTicks() const; // 0: not quantized
	int tripletTicks() const;  // 0: no triplets
	void updatePosition();
	bool isListening() const; // the recording is being played back
	void writeFile(const File& file);
	void saveOptions();
	void updateControls();
	void updateMetronome();
	void updateStatus();
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
};
