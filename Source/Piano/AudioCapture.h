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

// The audio of a recording: keeps the stereo input of the audio interface (the sound of
// the piano) while a MIDI recording is made, and writes it into a WAV file that begins
// exactly where the MIDI file begins.
//
// While the recording waits for its start (armed) only the last seconds are kept, in a
// ring buffer in memory. When the MIDI recording begins, a thread writes the ring buffer
// (what was heard before the first note too) and what comes after it into a temporary
// file. After the MIDI recording has ended, the audio goes on until the sound has faded
// (the tail). The WAV file is cut from the temporary file when the recording is saved:
// from the beginning of the MIDI file (which is known only then: it may be moved to a
// downbeat or to the first note) to the end of the tail.
//
// The audio thread only copies the samples into the ring buffer (Feed): it never waits
// for a lock or for the disk. The times are those of Time::getMillisecondCounterHiRes(),
// like in LiveRecorder; the latency of the input is taken into account.
class AudioCapture : private Thread
{
public:
	AudioCapture();
	~AudioCapture() override;

	enum State
	{
		stIdle,      // nothing is kept
		stArmed,     // the last seconds are kept in memory
		stWriting,   // written into the temporary file
		stTail,      // the MIDI recording has ended: waiting for the sound to fade
		stReady      // finished: can be saved
	};

	static const int NumChannels = 2;
	static constexpr double RingSeconds = 20.0;     // kept in memory while armed
	static constexpr float SilenceLevel = 0.001f;   // -60 dBFS: the sound has faded below it ...
	static constexpr double SilenceSeconds = 1.0;   // ... for this long
	static constexpr double MaxTailSeconds = 15.0;  // the tail is not longer than this
	static constexpr double KeptSilenceSeconds = 0.5; // of the silence at the end of the file
	static constexpr double FadeSeconds = 0.03;     // fade out at the end (no click)

	// The format of the input: called when the device starts (and by the tests).
	// latencySamples: the input latency the driver reports.
	void Prepare(double sampleRate, int latencySamples);
	// The device has stopped: a recording that is going on cannot be continued.
	void DeviceStopped();
	// (audio thread) A block of the input; nowMs: the time of its arrival. A missing
	// second channel is the copy of the first.
	void Feed(const float* const* input, int numChannels, int numSamples, double nowMs);

	// The steps of a recording (message thread).
	bool Arm();              // false: no input (the device is not running)
	bool BeginWriting();     // the MIDI recording has begun; false: the temporary file cannot be written
	void EndAt(double midiEndMs); // the MIDI recording has ended at this time: the tail begins
	void Poll();             // to be called regularly: ends the tail when the sound has faded
	void Clear();            // drops the recording (and its temporary file)

	State GetState() const { return m_state.load(); }
	bool HasFailed() const { return m_failed.load(); } // the device stopped, or the disk is full
	double GetSampleRate() const { return m_sampleRate; }

	// Writes the WAV file (24 bit): from the beginning of the MIDI recording (startMs) to
	// the end of the tail. If the audio begins later than the MIDI recording (it began
	// more than RingSeconds before the first note), the beginning is silence, so the two
	// files stay together. The file is written under a temporary name first.
	bool Save(const File& file, double startMs, String& error) const;
	// The length of that file in seconds.
	double GetLengthSeconds(double startMs) const;

private:
	std::atomic<State> m_state{stIdle};
	std::atomic<bool> m_failed{false};
	// the format of the device as it is now, and of the recording (taken when it is armed)
	std::atomic<double> m_deviceRate{0.0};
	std::atomic<int> m_deviceLatency{0};
	double m_sampleRate = 0.0;
	int m_latencySamples = 0;

	// The ring buffer: sample n of the input (counted from the first Feed after Prepare)
	// is at n % m_ringSize. Written by the audio thread only.
	HeapBlock<float> m_ring[NumChannels];
	int64 m_ringSize = 0;
	SpinLock m_ringLock;               // held while the ring buffer is set up or freed
	std::atomic<bool> m_capturing{false};
	std::atomic<int64> m_total{0};      // the number of samples fed
	std::atomic<int64> m_lastLoud{0};   // the end of the last block that was not silent
	// arrival time of sample n = m_clockOffsetMs + n * 1000 / sample rate (the earliest
	// arrivals count: a block may come late, never early)
	std::atomic<double> m_clockOffsetMs{0.0};
	std::atomic<bool> m_hasClock{false};

	// The temporary file: raw 32 bit float samples, interleaved, from sample m_fileFirst.
	File m_tempFile;
	std::unique_ptr<FileOutputStream> m_out;
	int64 m_fileFirst = 0;
	std::atomic<int64> m_filePos{0};    // the next sample to be written
	std::atomic<int64> m_fileEnd{-1};   // the writing ends here (-1: not known yet)
	double m_midiEndMs = 0.0;
	int64 m_endSample = 0;              // the end of the recording (stReady)

	void run() override;                // the writing thread
	bool WriteAvailable(int64 upTo);    // false: could not be written
	void FinishWriting(int64 endSample);
	int64 SampleAt(double soundMs) const; // the sample that sounded at this time
};
