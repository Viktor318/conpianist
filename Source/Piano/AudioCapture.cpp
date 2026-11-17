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


#include "AudioCapture.h"

namespace
{
	const int WriteChunkFrames = 16384;
	const int WriterWaitMs = 50;
}

AudioCapture::AudioCapture() :
	Thread("Audio recording")
{
}

AudioCapture::~AudioCapture()
{
	Clear();
}

void AudioCapture::Prepare(double sampleRate, int latencySamples)
{
	// the device was restarted during a recording: the samples do not follow each other
	// any more
	if (m_capturing.load())
	{
		m_failed = true;
	}
	m_deviceRate = sampleRate;
	m_deviceLatency = jmax(0, latencySamples);
}

void AudioCapture::DeviceStopped()
{
	if (m_capturing.load())
	{
		m_failed = true;
	}
	m_deviceRate = 0.0;
}

// (audio thread) Never waits: if the ring buffer is being set up, the block is dropped
// (nothing is kept then anyway).
void AudioCapture::Feed(const float* const* input, int numChannels, int numSamples, double nowMs)
{
	if (!m_capturing.load() || numSamples <= 0 || numChannels <= 0 || input[0] == nullptr)
	{
		return;
	}
	const SpinLock::ScopedTryLockType lock(m_ringLock);
	if (!lock.isLocked() || !m_capturing.load() || numSamples > m_ringSize)
	{
		return;
	}

	const int64 first = m_total.load();
	const int64 position = first % m_ringSize;
	const int untilEnd = (int)jmin((int64)numSamples, m_ringSize - position);
	float peak = 0.0f;
	for (int ch = 0; ch < NumChannels; ch++)
	{
		const float* source = ch < numChannels && input[ch] != nullptr ? input[ch] : input[0];
		std::memcpy(m_ring[ch].getData() + position, source, sizeof(float) * (size_t)untilEnd);
		if (untilEnd < numSamples)
		{
			std::memcpy(m_ring[ch].getData(), source + untilEnd, sizeof(float) * (size_t)(numSamples - untilEnd));
		}
		for (int i = 0; i < numSamples; i++)
		{
			peak = jmax(peak, std::abs(source[i]));
		}
	}
	const int64 total = first + numSamples;
	if (peak >= SilenceLevel)
	{
		m_lastLoud = total;
	}
	m_total = total;

	// when the last sample of the block arrived; a block may come late, never early: the
	// earliest arrivals give the clock (it may drift a little: it is let rise slowly)
	const double blockMs = numSamples * 1000.0 / m_sampleRate;
	const double offset = nowMs - (double)total * 1000.0 / m_sampleRate;
	if (!m_hasClock.load())
	{
		m_clockOffsetMs = offset;
		m_hasClock = true;
	}
	else
	{
		m_clockOffsetMs = jmin(m_clockOffsetMs.load() + blockMs * 0.0001, offset);
	}
}

bool AudioCapture::Arm()
{
	Clear();
	// (the format of a finished recording is kept until here: it is needed for saving it)
	m_sampleRate = m_deviceRate.load();
	m_latencySamples = m_deviceLatency.load();
	if (m_sampleRate <= 0.0)
	{
		return false;
	}
	{
		const SpinLock::ScopedLockType lock(m_ringLock);
		m_ringSize = (int64)(m_sampleRate * RingSeconds);
		for (auto& channel : m_ring)
		{
			channel.calloc((size_t)m_ringSize);
		}
		m_total = 0;
		m_lastLoud = 0;
		m_hasClock = false;
		m_capturing = true;
	}
	m_state = stArmed;
	return true;
}

bool AudioCapture::BeginWriting()
{
	if (m_state.load() != stArmed)
	{
		return false;
	}
	// what was heard in the last seconds too (the beginning of the MIDI recording may be
	// earlier than the first note); the oldest part of the ring buffer is being
	// overwritten: it is left out
	const int64 total = m_total.load();
	m_fileFirst = jmax((int64)0, total - m_ringSize + (int64)(m_sampleRate * 0.5));
	m_filePos = m_fileFirst;
	m_fileEnd = -1;
	m_tempFile = File::getSpecialLocation(File::tempDirectory)
		.getNonexistentChildFile("ConPianist audio", ".raw", false);
	m_out = m_tempFile.createOutputStream();
	if (m_out == nullptr || m_out->failedToOpen())
	{
		m_out = nullptr;
		Clear();
		return false;
	}
	m_state = stWriting;
	startThread();
	return true;
}

void AudioCapture::EndAt(double midiEndMs, bool untilSilence)
{
	if (m_state.load() != stWriting)
	{
		return;
	}
	m_midiEndMs = midiEndMs;
	if (untilSilence)
	{
		m_state = stTail; // (see Poll)
	}
	else
	{
		EndNow(jmax(m_fileFirst, SampleAt(midiEndMs)));
	}
}

void AudioCapture::StopNow()
{
	if (m_state.load() == stTail)
	{
		EndNow(jmax(m_fileFirst, SampleAt(m_midiEndMs)));
	}
}

// The end is where the samples are now; if the sound ended earlier (after the end of the
// MIDI recording), a little after that.
void AudioCapture::EndNow(int64 midiEnd)
{
	const int64 total = m_total.load();
	const int64 soundEnd = jmax(m_lastLoud.load(), midiEnd) + (int64)(KeptSilenceSeconds * m_sampleRate);
	m_cutWhileSounding = soundEnd >= total;
	FinishWriting(jmin(total, soundEnd));
}

// The sample that sounded at the given time: it arrived later by the latency of the input.
int64 AudioCapture::SampleAt(double soundMs) const
{
	const double latencyMs = m_latencySamples * 1000.0 / m_sampleRate;
	return (int64)std::llround((soundMs + latencyMs - m_clockOffsetMs.load()) * m_sampleRate / 1000.0);
}

void AudioCapture::Poll()
{
	const State state = m_state.load();
	if (state == stArmed && m_failed.load())
	{
		Clear();
		m_failed = true; // (kept, so that it can be told)
		return;
	}
	if (state == stWriting && m_failed.load())
	{
		m_cutWhileSounding = false;
		FinishWriting(m_filePos.load()); // what has been written is kept
		return;
	}
	if (state != stTail)
	{
		return;
	}
	const int64 total = m_total.load();
	if (m_failed.load())
	{
		FinishWriting(jmin(total, m_filePos.load()));
		return;
	}
	// The audio goes on after the MIDI recording until the sound has been below the silence
	// level for a while (it may have been so already); a little of the silence is kept.
	const int64 midiEnd = jmax(m_fileFirst, SampleAt(m_midiEndMs));
	const int64 lastLoud = jmax(m_lastLoud.load(), midiEnd);
	if (total >= midiEnd && total - lastLoud >= (int64)(SilenceSeconds * m_sampleRate))
	{
		m_cutWhileSounding = false;
		FinishWriting(jmin(total, lastLoud + (int64)(KeptSilenceSeconds * m_sampleRate)));
	}
}

// Lets the thread write up to the end, and closes the temporary file.
void AudioCapture::FinishWriting(int64 endSample)
{
	m_fileEnd = jmax(endSample, m_fileFirst);
	notify();
	if (!waitForThreadToExit(10000)) // it exits by itself when the end is written
	{
		stopThread(2000);
	}
	{
		const SpinLock::ScopedLockType lock(m_ringLock);
		m_capturing = false;
	}
	if (m_out != nullptr)
	{
		m_out->flush();
		if (m_out->getStatus().failed())
		{
			m_failed = true;
		}
		m_out = nullptr;
	}
	m_endSample = jmin(m_fileEnd.load(), m_filePos.load());
	m_state = stReady;
}

void AudioCapture::Clear()
{
	signalThreadShouldExit();
	notify();
	stopThread(5000);
	{
		const SpinLock::ScopedLockType lock(m_ringLock);
		m_capturing = false;
		for (auto& channel : m_ring)
		{
			channel.free();
		}
		m_ringSize = 0;
	}
	m_out = nullptr;
	if (m_tempFile != File())
	{
		m_tempFile.deleteFile();
		m_tempFile = File();
	}
	m_failed = false;
	m_state = stIdle;
}

// The writing thread: copies what has arrived from the ring buffer into the temporary
// file, until the end (known when the tail has ended).
void AudioCapture::run()
{
	while (!threadShouldExit())
	{
		const int64 end = m_fileEnd.load();
		const int64 total = m_total.load();
		if (!WriteAvailable(end >= 0 ? jmin(end, total) : total))
		{
			m_failed = true;
			break;
		}
		if (end >= 0 && (m_filePos.load() >= end || m_filePos.load() >= m_total.load()))
		{
			break;
		}
		wait(WriterWaitMs);
	}
}

bool AudioCapture::WriteAvailable(int64 upTo)
{
	HeapBlock<float> frames((size_t)(WriteChunkFrames * NumChannels));
	int64 position = m_filePos.load();
	while (position < upTo)
	{
		const int count = (int)jmin((int64)WriteChunkFrames, upTo - position);
		for (int ch = 0; ch < NumChannels; ch++)
		{
			const float* ring = m_ring[ch].getData();
			int64 index = position % m_ringSize;
			for (int i = 0; i < count; i++)
			{
				frames[(size_t)(i * NumChannels + ch)] = ring[index];
				if (++index == m_ringSize)
				{
					index = 0;
				}
			}
		}
		// the audio thread must not have gone round the ring buffer meanwhile (the disk
		// was too slow by many seconds): what was copied would be newer samples
		if (m_total.load() - position > m_ringSize)
		{
			return false;
		}
		if (!m_out->write(frames.getData(), sizeof(float) * (size_t)(count * NumChannels)))
		{
			return false; // (the disk is full)
		}
		position += count;
		m_filePos = position;
	}
	return true;
}

double AudioCapture::GetLengthSeconds(double startMs) const
{
	if (m_state.load() != stReady || m_sampleRate <= 0.0)
	{
		return 0.0;
	}
	return (double)jmax((int64)0, m_endSample - SampleAt(startMs)) / m_sampleRate;
}

namespace
{
	void WriteWavHeader(OutputStream& out, int sampleRate, int64 frames)
	{
		const int bytesPerFrame = AudioCapture::NumChannels * 3;
		const uint32 dataSize = (uint32)jmin(frames * bytesPerFrame, (int64)0xffffffd0u);
		out.write("RIFF", 4);
		out.writeInt((int)(dataSize + 36));
		out.write("WAVEfmt ", 8);
		out.writeInt(16);
		out.writeShort(1); // PCM
		out.writeShort((short)AudioCapture::NumChannels);
		out.writeInt(sampleRate);
		out.writeInt(sampleRate * bytesPerFrame);
		out.writeShort((short)bytesPerFrame);
		out.writeShort(24);
		out.write("data", 4);
		out.writeInt((int)dataSize);
	}
}

String AudioCapture::GetExtension(Format format)
{
	return format == fmtFlac ? "flac" : format == fmtMp3 ? "mp3" : format == fmtOgg ? "ogg" : "wav";
}

bool AudioCapture::Save(const File& file, double startMs, const SaveOptions& options, String& error) const
{
	if (m_state.load() != stReady || m_sampleRate <= 0.0)
	{
		error = "No audio was recorded";
		return false;
	}
	const int64 start = SampleAt(startMs);
	const int64 end = m_endSample;
	if (end <= start || end <= m_fileFirst)
	{
		error = "No audio was recorded";
		return false;
	}
	FileInputStream in(m_tempFile);
	if (in.failedToOpen())
	{
		error = "The recorded audio cannot be read";
		return false;
	}
	auto setStage = [&options](Stage stage)
		{
			if (options.stage != nullptr)
			{
				*options.stage = stage;
			}
		};
	auto cancelled = [&options, &error]()
		{
			if (options.cancel != nullptr && options.cancel->load())
			{
				error = "Cancelled";
				return true;
			}
			return false;
		};

	// a part of the recording, interleaved; before the beginning of the audio: silence
	HeapBlock<float> frames((size_t)(WriteChunkFrames * NumChannels));
	auto readFrames = [&](int64 position, int count)
		{
			const int silent = (int)jlimit((int64)0, (int64)count, m_fileFirst - position);
			zeromem(frames.getData(), sizeof(float) * (size_t)(count * NumChannels));
			if (silent < count)
			{
				const size_t wanted = sizeof(float) * (size_t)((count - silent) * NumChannels);
				in.setPosition((position + silent - m_fileFirst) * (int64)(sizeof(float) * NumChannels));
				if (in.read(frames.getData() + silent * NumChannels, (int)wanted) != (int)wanted)
				{
					error = "The recorded audio cannot be read";
					return false;
				}
			}
			return true;
		};

	// normalizing: the loudest sample is looked for first
	float gain = 1.0f;
	if (options.normalize)
	{
		setStage(stageNormalizing);
		float peak = 0.0f;
		for (int64 position = start; position < end; position += WriteChunkFrames)
		{
			const int count = (int)jmin((int64)WriteChunkFrames, end - position);
			if (cancelled() || !readFrames(position, count))
			{
				return false;
			}
			const Range<float> range = FloatVectorOperations::findMinAndMax(frames.getData(), count * NumChannels);
			peak = jmax(peak, std::abs(range.getStart()), std::abs(range.getEnd()));
		}
		if (peak >= SilenceLevel)
		{
			gain = NormalizeLevel / peak;
		}
	}
	setStage(options.format == fmtWav ? (options.normalize ? stageNormalizing : stageWriting) : stageConverting);

	TemporaryFile temporary(file);
	{
		std::unique_ptr<OutputStream> out(temporary.getFile().createOutputStream());
		if (out == nullptr)
		{
			error = "The file cannot be written";
			return false;
		}
		// FLAC, Ogg Vorbis and MP3 are written by JUCE (MP3: into a WAV file first, converted
		// by LAME when the writer is closed)
		std::unique_ptr<AudioFormatWriter> writer;
		if (options.format != fmtWav)
		{
			const AudioFormatWriterOptions writerOptions = AudioFormatWriterOptions()
				.withSampleRate(m_sampleRate).withNumChannels(NumChannels).withBitsPerSample(24);
			if (options.format == fmtFlac)
			{
				FlacAudioFormat flac;
				writer = flac.createWriterFor(out, writerOptions);
			}
			else if (options.format == fmtOgg)
			{
				OggVorbisAudioFormat ogg;
				const int quality = ogg.getQualityOptions().indexOf("500 kbps");
				writer = ogg.createWriterFor(out, writerOptions.withBitsPerSample(32).withQualityOptionIndex(quality));
			}
			else
			{
#if JUCE_USE_LAME_AUDIO_FORMAT
				if (!options.lameApp.existsAsFile())
				{
					error = "The LAME encoder is not found";
					return false;
				}
				LAMEEncoderAudioFormat lame(options.lameApp);
				const int quality = lame.getQualityOptions().indexOf("320 Kb/s CBR");
				writer = lame.createWriterFor(out, writerOptions.withQualityOptionIndex(quality));
#endif
			}
			if (writer == nullptr)
			{
				error = "The audio format is not available";
				return false;
			}
		}
		else
		{
			WriteWavHeader(*out, roundToInt(m_sampleRate), end - start);
		}

		const int64 fadeFrames = jmax((int64)1, (int64)((m_cutWhileSounding ? CutFadeSeconds : FadeSeconds) * m_sampleRate));
		HeapBlock<uint8> bytes((size_t)(WriteChunkFrames * NumChannels * 3));
		HeapBlock<float> channelData[NumChannels];
		for (auto& channel : channelData)
		{
			channel.malloc((size_t)WriteChunkFrames);
		}
		for (int64 position = start; position < end; position += WriteChunkFrames)
		{
			const int count = (int)jmin((int64)WriteChunkFrames, end - position);
			if (cancelled() || !readFrames(position, count))
			{
				return false;
			}
			// the gain of the normalizing, and the fade out at the end
			for (int i = 0; i < count; i++)
			{
				const int64 left = end - (position + i);
				const float sampleGain = left < fadeFrames ? gain * (float)left / (float)fadeFrames : gain;
				for (int ch = 0; ch < NumChannels; ch++)
				{
					float& sample = frames[(size_t)(i * NumChannels + ch)];
					sample = jlimit(-1.0f, 1.0f, sample * sampleGain);
				}
			}
			bool written;
			if (writer != nullptr)
			{
				for (int i = 0; i < count; i++)
				{
					for (int ch = 0; ch < NumChannels; ch++)
					{
						channelData[ch][(size_t)i] = frames[(size_t)(i * NumChannels + ch)];
					}
				}
				const float* channels[NumChannels] = {channelData[0].getData(), channelData[1].getData()};
				written = writer->writeFromFloatArrays(channels, NumChannels, count);
			}
			else
			{
				uint8* target = bytes.getData();
				for (int i = 0; i < count * NumChannels; i++)
				{
					const int value = jlimit(-8388608, 8388607, roundToInt(frames[(size_t)i] * 8388607.0f));
					*target++ = (uint8)(value & 0xff);
					*target++ = (uint8)((value >> 8) & 0xff);
					*target++ = (uint8)((value >> 16) & 0xff);
				}
				written = out->write(bytes.getData(), (size_t)(count * NumChannels * 3));
			}
			if (!written)
			{
				error = "The file cannot be written";
				return false;
			}
		}
		if (writer != nullptr)
		{
			writer = nullptr; // (closes the file; MP3: LAME runs now)
		}
		else
		{
			out->flush();
			out = nullptr;
		}
	}
	// (LAME could not be run, or it failed: nothing was written)
	if (temporary.getFile().getSize() <= 0)
	{
		error = options.format == fmtMp3 ? "The LAME encoder failed" : "The file cannot be written";
		return false;
	}
	if (!temporary.overwriteTargetFileWithTemporary())
	{
		error = "The file cannot be written";
		return false;
	}
	return true;
}
