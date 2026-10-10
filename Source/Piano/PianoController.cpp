/*
 *  This file is part of ConPianist. See <https://github.com/Viktor318/conpianist>.
 *  Fork of the original project <https://github.com/hugbug/conpianist>.
 *
 *  Copyright (C) 2018-2020 Andrey Prygunkov <hugbug@users.sourceforge.net>
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

#include "PianoController.h"
#include "PianoMessage.h"
#include "Presets.h"

const std::vector<PianoController::Channel> PianoController::AllChannels = {
	chMain, chLayer, chLeft,
	chMidi1, chMidi2, chMidi3, chMidi4, chMidi5, chMidi6, chMidi7, chMidi8,
	chMidi9, chMidi10, chMidi11, chMidi12, chMidi13, chMidi14, chMidi15, chMidi16,
	chMic, chAuxIn, chWave, chMidiMaster, chStyle };

const std::vector<PianoController::Channel> PianoController::MidiChannels = {
	chMidi1, chMidi2, chMidi3, chMidi4, chMidi5, chMidi6, chMidi7, chMidi8,
	chMidi9, chMidi10, chMidi11, chMidi12, chMidi13, chMidi14, chMidi15, chMidi16 };

PianoController::PianoController()
{
	// set internal state for channels
	for (Channel ch : AllChannels)
	{
		m_channels[ch].enabled = (ch < chMidi1 || ch > chMidi16) && ch != chMidiMaster;
		m_channels[ch].active = m_channels[ch].enabled;
	}
	ClearConvertedVoices();
	for (int i = 0; i < 16; i++)
	{
		m_liveOriginalVoice[i] = -1;
		m_liveConvertedVoice[i] = -1;
	}
	for (int i = 0; i <= NumStyleParts; i++)
	{
		for (int which = 0; which < 3; which++)
		{
			m_styleDefaults[i][which] = NoStyleDefault;
			m_styleDefaultPending[i][which] = false;
		}
	}
}

PianoController::~PianoController()
{
}

void PianoController::SetPianoConnector(PianoConnector* pianoConnector)
{
	m_pianoConnector = pianoConnector;
	m_pianoConnector->SetListener(this);
}

void PianoController::Connect()
{
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::PianoModel));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::FirmwareVersion));
}

void PianoController::InitEvents()
{
	// Activate feedback events from piano
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Events, Property::Length));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Events, Property::Position));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Events, Property::Play));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Events, Property::Part));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Events, Property::PartChannel));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Events, Property::PartAuto));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Events, Property::Guide));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Events, Property::GuideType));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Events, Property::StreamLights));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Events, Property::StreamSpeed));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Events, Property::Volume));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Events, Property::Pan));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Events, Property::Reverb));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Events, Property::Octave));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Events, Property::Tempo));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Events, Property::Transpose));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Events, Property::ReverbEffect));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Events, Property::Loop));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Events, Property::VoicePreset));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Events, Property::Active));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Events, Property::Present));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Events, Property::VoiceMidi));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Events, Property::SongName));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Events, Property::SplitPoint));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Events, Property::StyleName));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Events, Property::StylePlay));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Events, Property::StylePosition));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Events, Property::StyleSyncStart));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Events, Property::StyleSection));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Events, Property::StyleChord));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Events, Property::StyleChordArea));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Events, Property::Metronome));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Events, Property::MetronomeCount));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Events, Property::MetronomeBeat));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Events, Property::MetronomeBell));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Events, Property::MetronomeVolume));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Events, Property::LidPosition));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Events, Property::Environment));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Events, Property::Brightness));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Events, Property::TouchCurve));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Events, Property::FixedCurve));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Events, Property::FixedVelocity));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Events, Property::MasterTune));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Events, Property::Vrm));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Events, Property::DamperResonance));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Events, Property::StringResonance));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Events, Property::KeyOffSampling));
}

void PianoController::Sync()
{
	InitEvents();
	SetLocalControl(true);
	if (!m_localPlayback)
	{
		m_songLoaded = false;
	}
	ResyncStateFromPiano();
}

void PianoController::ResyncStateFromPiano()
{
	for (Channel ch : AllChannels)
	{
		if (chMidi1 <= ch && ch <= chMidi16 && ch != chMidiMaster)
		{
			m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::Present, ch, 0));
			m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::VoiceMidi, ch, 0));
		}
		if (ch != chAuxIn)
		{
			m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::Active, ch, 0));
			m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::Pan, ch, 0));
			m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::Reverb, ch, 0));
		}

		m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::Volume, ch, 0));
	}

	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::Guide));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::GuideType));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::StreamLights));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::StreamSpeed));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::ReverbEffect));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::Tempo));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::StyleName));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::StylePlay));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::StyleSyncStart));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::StyleSection, 0, 0));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::StyleSection, 1, 0));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::StyleChordArea));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::StyleLeftSound));
	QueryStyleParts();
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::Metronome));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::MetronomeBeat));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::MetronomeBell));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::MetronomeVolume));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::Transpose, 2, 0));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::Transpose, 1, 0));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::VoicePreset, chMain, 0));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::VoicePreset, chLayer, 0));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::VoicePreset, chLeft, 0));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::Octave, chMain, 0));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::Octave, chLayer, 0));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::Octave, chLeft, 0));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::Play));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::Part, paRight, 0));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::Part, paLeft, 0));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::Part, paBacking, 0));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::PartChannel, paRight, 0));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::PartChannel, paLeft, 0));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::PartAuto));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::SplitPoint));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::SplitPoint, 1, 0));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::LidPosition));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::Environment));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::Brightness));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::TouchCurve));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::FixedCurve, chMain, 0));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::FixedCurve, chLeft, 0));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::FixedCurve, chLayer, 0));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::FixedVelocity));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::MasterTune));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::Vrm));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::DamperResonance));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::StringResonance));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::KeyOffSampling));

	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::SongName));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::Length));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::Position));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::Loop));
}

void PianoController::Reset()
{
	// set internal state for channels
	for (Channel ch : AllChannels)
	{
		m_channels[ch].enabled = (ch < chMidi1 || ch > chMidi16) && ch != chMidiMaster;
		m_channels[ch].active = true;
	}

	InitEvents();

	Stop();

	ResetSong();

	SetLocalControl(true);

	SetGuide(false);
	SetStreamLights(true);
	SetStreamFast(true);

	for (Channel ch : AllChannels)
	{
		ResetVolume(ch);
		ResetPan(ch);
		ResetReverb(ch);
	}

	ResetTempo();
	SetTranspose(DefaultTranspose);

	SetReverbEffect(DefaultReverbEffect);
	SetLidPosition(DefaultLidPosition);
	SetEnvironment(DefaultEnvironment);
	SetBrightness(DefaultBrightness);
	SetTouchCurve(DefaultTouchCurve);
	SetFixedCurve(chMain, false);
	SetFixedCurve(chLayer, false);
	SetFixedCurve(chLeft, false);
	SetFixedVelocity(DefaultFixedVelocity);
	SetMasterTune(0);
	SetKeyboardTranspose(0);
	SetVrm(true);
	SetDamperResonance(DefaultResonance);
	SetStringResonance(DefaultResonance);
	SetKeyOffSampling(DefaultKeyOffSampling);

	SetActive(chMain, true);
	SetActive(chLayer, false);
	SetActive(chLeft, false);
	SetActive(chMic, true);

	SetVoice(chMain, "PRESET:/VOICE/Piano/Grand Piano/CFX Grand.T542.VRM");
	SetVoice(chLayer, "PRESET:/VOICE/Strings & Vocal/String Ensemble/Real Strings.T250.SAR");
	SetVoice(chLeft, "PRESET:/VOICE/Piano/FM E.Piano/Sweet DX.T232.CLV");
	// after the voices: a voice may bring its own octave
	SetOctave(chMain, DefaultOctave);
	SetOctave(chLayer, DefaultOctave);
	SetOctave(chLeft, DefaultLeftOctave);
	SetSplitPoint(DefaultSplitPoint);

	// The accompaniment: the default style with its own tempo and mixer, chords detected
	// below the split point. The values of the parts are the ones of this style; they are
	// sent since the piano keeps changed values while the style is not loaded again.
	if (m_stylePlaying)
	{
		SetStylePlaying(false);
	}
	ForgetStyleOffParts();
	StyleState style;
	style.style = "PRESET:/STYLE/Pop & Rock/Pop/Standard 8Beat.T308.prs";
	style.tempo = DefaultStyleTempo;
	style.chordArea = caLower;
	style.leftSound = 0; // "Main voice below" off (Smart Pianist starts with it on)
	style.splitPoint = DefaultSplitPoint;
	style.hasMixer = true;
	style.volume = DefaultVolume;
	style.pan = DefaultPan;
	style.reverb = 64;
	static const int partDefaults[NumStyleParts][3] = { // volume, pan, reverb
		{54, 0, 26}, {76, 0, 26}, {56, 0, 0}, {31, -28, 26}, {68, 0, 30}, {44, 0, 36}, {52, 27, 36}, {100, 0, 40}};
	for (int i = 0; i < NumStyleParts; i++)
	{
		style.parts[i].active = true;
		style.parts[i].volume = partDefaults[i][0];
		style.parts[i].pan = partDefaults[i][1];
		style.parts[i].reverb = partDefaults[i][2];
	}
	RestoreStyleState(style);

	if (m_localPlayback)
	{
		ResetLocalMixState();
	}

	ResyncStateFromPiano();
}

void PianoController::Disconnect()
{
	if (m_connected)
	{
		m_connected = false;
		NotifyChanged(apConnection);
	}
}

void PianoController::SetLocalControl(bool enabled)
{
	m_localControl = enabled;
	MidiMessage localControlMessage = MidiMessage::controllerEvent(1, 122, enabled ? 127 : 0);
	m_pianoConnector->SendMidiMessage(localControlMessage);
	NotifyChanged(apLocalControl);
}

bool PianoController::UploadSong(const File& file)
{
	if (m_localPlayback)
	{
		return LoadLocalSong(file);
	}

	String headerHex = "01 00 00 06 00 00 00 01 00 00 00 00 00 00 00 01 00 00 00 00 00 00 00";

	MemoryBlock message;
	message.loadFromHexString(headerHex);

	// The name is sent with a one-byte length prefix, so it must fit into 255 bytes
	// including the terminating zero. The limit is in UTF-8 bytes, not characters:
	// accented letters (e.g. in Hungarian file names) take two bytes each.
	const size_t MaxNameBytes = 255;
	String filename = "EXTERNAL:" + file.getFullPathName();
	if (filename.getNumBytesAsUTF8() + 1 > MaxNameBytes)
	{
		filename = "EXTERNAL:" + file.getFileName();
		while (filename.getNumBytesAsUTF8() + 1 > MaxNameBytes)
		{
			filename = filename.dropLastCharacters(1);
		}
	}
	const char* namebuf = filename.toRawUTF8();
	size_t namelen = strlen(namebuf) + 1;
	uint8_t namelenbuf = (uint8_t)namelen;
	message.append(&namelenbuf, 1);
	message.append(namebuf, namelen);

	int fileSize = (int)file.getSize();
	int payloadSize = (int)message.getSize() - 8 + fileSize;
	message[8] = ((payloadSize >> 8*3) & 0xFF);
	message[9] = ((payloadSize >> 8*2) & 0xFF);
	message[10] = ((payloadSize >> 8*1) & 0xFF);
	message[11] = ((payloadSize >> 8*0) & 0xFF);

	uint8_t sizeBuf[4] = {
		(uint8_t)((fileSize >> 8*3) & 0xFF),
		(uint8_t)((fileSize >> 8*2) & 0xFF),
		(uint8_t)((fileSize >> 8*1) & 0xFF),
		(uint8_t)((fileSize >> 8*0) & 0xFF)};
	message.append(sizeBuf, 4);

	if (!file.loadFileAsData(message))
	{
		return false;
	}

	Pause();

	Thread::sleep(100);

	// The upload runs on the UI thread, so every step must have a time limit;
	// otherwise the whole window freezes if the piano does not answer.
	// Note: write() and read() return -1 on error, which would count as "true"
	// in a boolean expression, so their results are compared explicitly.
	const int ConnectTimeoutMs = 3000;
	const int ResponseTimeoutMs = 10000;
	const int messageSize = (int)message.getSize();

	m_songLoading = true;
	ClearSentSongVoices(); // the new song sets its own voices
	char response[16];
	StreamingSocket socket;
	bool ok = socket.connect(m_remoteIp, UploadPort, ConnectTimeoutMs) &&
		socket.write(message.getData(), messageSize) == messageSize &&
		socket.waitUntilReady(true, ResponseTimeoutMs) == 1 &&
		socket.read(response, (int)sizeof(response), false) > 0;

	m_songLoading &= ok;
	return ok;
}

// The piano is asked to load one of its own songs by its path, the way the song
// name is reported by the piano (e.g. PRESET:/SONG/Popular/Pop/Pop01.S000.mid). If the
// piano loads it, it reports the new song name (apSongName, apSongLoaded).
bool PianoController::LoadPresetSong(const String& path)
{
	if (!m_connected || path.isEmpty())
	{
		return false;
	}
	Logger::writeToLog("Loading the piano's song " + path);
	m_songLoading = true;
	ClearSentSongVoices();
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Set, Property::SongName, 0, path));
	return true;
}

String PianoController::DecodeSongName(String rawValue)
{
	// utf8 conversion
	int len = rawValue.length();
	std::vector<char> utf8name(len + 1);
	for (int i = 0; i < len; i++)
	{
		utf8name[i] = rawValue[i];
	}
	utf8name[len] = '\0';
	String name = String::fromUTF8(utf8name.data());

	// strip "EXTERNAL:" or "PRESET:"
	if (name.startsWith("EXTERNAL:"))
	{
		name = name.substring(9);
	}
	else if (name.startsWith("PRESET:"))
	{
		name = name.substring(7);
	}

	// Windows <-> Unix compatibility
	name = name.replaceCharacter('\\', File::getSeparatorChar())
		.replaceCharacter('/', File::getSeparatorChar());

	return name;
}

void PianoController::ResetSong()
{
	if (m_localPlayback)
	{
		if (m_localPlayer)
		{
			m_localPlayer->Unload();
		}
		ClearSongState();
		return;
	}
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Set, Property::SongReset));
}

void PianoController::Play()
{
	SuspendStyleSyncStart(); // the notes of the song must not start the accompaniment
	if (m_localPlayback)
	{
		if (m_localPlayer) m_localPlayer->Play();
		return;
	}
	m_stopRequested = false;
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Set, Property::Play, 1));
}

void PianoController::Pause()
{
	if (m_localPlayback)
	{
		if (m_localPlayer) m_localPlayer->Pause();
		return;
	}
	m_stopRequested = true;
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Set, Property::Play, 2));
}

void PianoController::Stop()
{
	if (m_localPlayback)
	{
		if (m_localPlayer) m_localPlayer->Stop();
		return;
	}
	m_stopRequested = true;
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Set, Property::Play, 0));
}

void PianoController::SetGuide(bool enable)
{
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Set, Property::Guide, enable ? 1 : 0));
}

void PianoController::SetGuideType(GuideType type)
{
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Set, Property::GuideType, type));
}

void PianoController::SetStreamLights(bool enable)
{
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Set, Property::StreamLights, enable ? 1 : 0));
}

void PianoController::SetStreamFast(bool fast)
{
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Set, Property::StreamSpeed, fast ? 1 : 0));
}

void PianoController::SetPosition(const Position position)
{
	if (m_localPlayback)
	{
		if (m_localPlayer) m_localPlayer->SetPosition({position.measure, position.beat});
		return;
	}
	uint8_t data[4];
	data[0] = (position.measure >> 7) & 0x7f;
	data[1] = (position.measure >> 0) & 0x7f;
	data[2] = (position.beat >> 7) & 0x7f;
	data[3] = (position.beat >> 0) & 0x7f;
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Set, Property::Position, 0, data, 4));
}

void PianoController::SetVolume(Channel ch, int volume)
{
	const bool liveOnly = IsLiveOnlyChannel(ch);
	if (liveOnly)
	{
		LiveStateFor(ch).volume = volume;
	}

	if (m_genericDevice)
	{
		// the song's own volume changes (CC7) are kept, scaled to the mixer setting
		m_channels[ch].volume = volume;
		if (liveOnly)
		{
			// not played by the song player: sent directly
			SendMidiMessage(MidiMessage::controllerEvent(ch - chMidi0, 7, jlimit(0, 127, volume)));
		}
		else if (m_localPlayer && ch == chMidiMaster)
		{
			m_localPlayer->SetMasterVolumeScale(volume / double(DefaultVolume));
		}
		else if (m_localPlayer && IsSongChannel(ch))
		{
			const int base = std::max(1, GenericSetupValue(ch, 7, DefaultVolume));
			m_localPlayer->SetVolumeScale(ch - chMidi0, volume / double(base));
		}
		NotifyChanged(apVolume, ch);
		return;
	}

	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Set, Property::Volume, ch, volume));
}

void PianoController::ResetVolume(Channel ch)
{
	if (m_genericDevice)
	{
		SetVolume(ch, ch == chMidiMaster ? DefaultVolume : GenericSetupValue(ch, 7, DefaultVolume));
		return;
	}

	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Reset, Property::Volume, ch, 0));
}

void PianoController::SetPan(Channel ch, int pan)
{
	const bool liveOnly = IsLiveOnlyChannel(ch);
	if (liveOnly)
	{
		LiveStateFor(ch).pan = pan;
	}

	if (m_genericDevice)
	{
		m_channels[ch].pan = pan;
		if (liveOnly)
		{
			SendMidiMessage(MidiMessage::controllerEvent(ch - chMidi0, 10, jlimit(0, 127, pan + PanBase)));
		}
		else if (m_localPlayer && IsSongChannel(ch))
		{
			m_localPlayer->SetControllerOverride(ch - chMidi0, 10, pan + PanBase);
		}
		NotifyChanged(apPan, ch);
		return;
	}

	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Set, Property::Pan, ch, pan + PanBase));
}

void PianoController::ResetPan(Channel ch)
{
	if (m_genericDevice)
	{
		SetPan(ch, GenericSetupValue(ch, 10, PanBase) - PanBase);
		return;
	}

	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Reset, Property::Pan, ch, 0));
}

void PianoController::SetReverb(Channel ch, int reverb)
{
	const bool liveOnly = IsLiveOnlyChannel(ch);
	if (liveOnly)
	{
		LiveStateFor(ch).reverb = reverb;
	}

	if (m_genericDevice)
	{
		m_channels[ch].reverb = reverb;
		if (liveOnly)
		{
			SendMidiMessage(MidiMessage::controllerEvent(ch - chMidi0, 91, jlimit(0, 127, reverb)));
		}
		else if (m_localPlayer && IsSongChannel(ch))
		{
			m_localPlayer->SetControllerOverride(ch - chMidi0, 91, reverb);
		}
		NotifyChanged(apReverb, ch);
		return;
	}

	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Set, Property::Reverb, ch, reverb));
}

void PianoController::ResetReverb(Channel ch)
{
	if (m_genericDevice)
	{
		SetReverb(ch, GenericSetupValue(ch, 91, GenericDefaultReverb));
		return;
	}

	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Reset, Property::Reverb, ch, 0));
}

void PianoController::SetOctave(Channel ch, int octave)
{
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Set, Property::Octave, ch, octave + OctaveBase));
}

void PianoController::SetTempo(int tempo)
{
	if (m_localPlayback)
	{
		SetLocalTempo(tempo);
		SendLocalTempo();
		return;
	}
	m_networkTempoSet = true;
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Set, Property::Tempo, tempo));
}

void PianoController::SetSpeed(int percent)
{
	if (!IsLocalSongLoaded())
	{
		return;
	}
	m_speedFactor = jlimit(MinSpeed, MaxSpeed, percent) / 100.0;
	ApplyLocalSpeed();
	SendLocalTempo();
}

void PianoController::SetSpeedFactor(double factor)
{
	if (!IsLocalSongLoaded() || factor <= 0)
	{
		return;
	}
	m_speedFactor = jlimit(0.02, 20.0, factor);
	ApplyLocalSpeed();
	SendLocalTempo();
}

// The tempo is set (Accompaniment or Recording window, Tempo of the left panel, the piano):
// exactly this tempo at the current position; the speed is calculated from it.
void PianoController::SetLocalTempo(int tempo)
{
	tempo = jlimit((int)MinTempo, (int)MaxTempo, tempo);
	if (!IsLocalSongLoaded())
	{
		m_tempo = tempo;
		NotifyChanged(apTempo);
		return;
	}
	const double fileTempo = m_localPlayer->GetFileTempo();
	m_speedFactor = jlimit(0.02, 20.0, fileTempo > 0 ? tempo / fileTempo : 1.0);
	ApplyLocalSpeed(tempo);
}

void PianoController::ApplyLocalSpeed(int tempo)
{
	if (m_localPlayer)
	{
		m_localPlayer->SetSpeed(m_speedFactor);
		m_tempo = tempo > 0 ? tempo :
			jlimit((int)MinTempo, (int)MaxTempo, roundToInt(m_localPlayer->GetFileTempo() * m_speedFactor));
	}
	NotifyChanged(apTempo);
}

void PianoController::UpdateLocalTempo()
{
	if (!IsLocalSongLoaded())
	{
		return;
	}
	const int tempo = jlimit((int)MinTempo, (int)MaxTempo, roundToInt(m_localPlayer->GetFileTempo() * m_speedFactor));
	if (tempo != m_tempo)
	{
		m_tempo = tempo;
		NotifyChanged(apTempo);
		SendLocalTempo();
	}
}

void PianoController::SendLocalTempo()
{
	if (!m_connected)
	{
		m_recorder.AddTempo(m_tempo); // no piano that reports it: recorded from here
		return;
	}
	// the piano's metronome and accompaniment follow the tempo of the own player
	m_tempoSentMs = Time::getMillisecondCounter();
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Set, Property::Tempo, m_tempo));
}

void PianoController::ResetTempo()
{
	if (m_localPlayback)
	{
		if (IsLocalSongLoaded())
		{
			// the song's own tempo: the speed 100%
			SetSpeed(DefaultSpeed);
		}
		else
		{
			SetTempo(DefaultTempo);
		}
		return;
	}
	m_networkTempoSet = false; // the song's own tempo again
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Reset, Property::Tempo));
}

void PianoController::SetTranspose(int transpose)
{
	if (m_localPlayback)
	{
		if (m_localPlayer) m_localPlayer->SetTranspose(transpose);
		m_transpose = transpose;
		NotifyChanged(apTranspose);
		return;
	}
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Set, Property::Transpose, 2, transpose + TransposeBase));
}

void PianoController::SetReverbEffect(int effect)
{
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Set, Property::ReverbEffect, 0, effect));
}

void PianoController::SetPart(Part part, bool enable)
{
	if (m_localPlayback)
	{
		m_parts[part] = enable;
		NotifyChanged(apPart);
		UpdateLocalMutes();
		return;
	}
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Set, Property::Part, part, enable ? 1 : 0));
}

void PianoController::SetPartChannel(Part part, Channel channel)
{
	if (m_localPlayback)
	{
		const Channel oldCh = m_partChannels[part];
		m_partChannels[part] = channel;
		NotifyChanged(apPartChannel, oldCh);
		NotifyChanged(apPartChannel, channel);
		UpdateLocalMutes();
		return;
	}
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Set, Property::PartChannel, part, channel - chMidi0));
}

void PianoController::SetPartAuto(bool enable)
{
	if (m_localPlayback)
	{
		m_partAuto = enable;
		NotifyChanged(apPartAuto);
		return;
	}
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Set, Property::PartAuto, 0, enable ? 1 : 0));
}

void PianoController::SetLoop(Loop loop)
{
	if (m_localPlayback)
	{
		if (m_localPlayer) m_localPlayer->SetLoop({loop.begin.measure, loop.begin.beat}, {loop.end.measure, loop.end.beat});
		m_loop = loop;
		m_loopStart = {0,0};
		NotifyChanged(apLoop);
		return;
	}
	m_loopStart = {0,0};
	uint8_t data[9] = {1,
		(uint8_t)((loop.begin.measure >> 7) & 0x7f),
		(uint8_t)((loop.begin.measure >> 0) & 0x7f),
		(uint8_t)((loop.begin.beat >> 7) & 0x7f),
		(uint8_t)((loop.begin.beat >> 0) & 0x7f),
		(uint8_t)((loop.end.measure >> 7) & 0x7f),
		(uint8_t)((loop.end.measure >> 0) & 0x7f),
		(uint8_t)((loop.end.beat >> 7) & 0x7f),
		(uint8_t)((loop.end.beat >> 0) & 0x7f)};
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Set, Property::Loop, 0, data, 9));
}

void PianoController::ResetLoop()
{
	if (m_localPlayback)
	{
		if (m_localPlayer) m_localPlayer->ResetLoop();
		m_loop = {{0,0},{0,0}};
		m_loopStart = {0,0};
		NotifyChanged(apLoop);
		return;
	}
	m_loopStart = {0,0};
	uint8_t data[9] = {0,0,1,0,1,0,2,0,1};
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Set, Property::Loop, 0, data, 9));
}

void PianoController::SetLoopStart(const Position loopStart)
{
	m_loopStart = loopStart;
	NotifyChanged(apLoop);
}

void PianoController::SetVoice(Channel ch, const String& voice)
{
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Set, Property::VoicePreset, ch, voice));
}

// Sets the voice of a song channel (Midi1..Midi16) the same way a MIDI file does it:
// with standard Bank Select (CC0 MSB, CC32 LSB) and Program Change messages on that
// MIDI channel. The piano rejects both the VoicePreset (status 02 02) and the
// VoiceMidi (status 02 05) properties for song channels.
// voiceNum has the format 0x00MMLLPP (MSB, LSB, program), as in Presets::Voices().
int PianoController::RealSongVoice(int voiceNum)
{
	// 119 / 119 / program is not a voice: it is the answer of the piano to a GS voice
	// that was sent to it in GS mode (saved by an earlier version). The GS voice of the
	// program (variation 0) is used instead.
	if (((voiceNum >> 16) & 0x7f) == GsBank && ((voiceNum >> 8) & 0x7f) == GsBank)
	{
		return (GsBank << 16) | (voiceNum & 0x7f);
	}
	return voiceNum;
}

void PianoController::ClearSentSongVoices()
{
	std::fill(std::begin(m_sentSongVoice), std::end(m_sentSongVoice), -1);
}

void PianoController::SetSongChannelVoice(Channel ch, int voiceNum)
{
	const int midiChannel = ch - chMidi0; // 1..16
	if (midiChannel < 1 || midiChannel > 16)
	{
		return;
	}

	voiceNum = RealSongVoice(voiceNum);
	// remembered: the piano answers with its own form of the number (see RealSongVoice)
	m_sentSongVoice[midiChannel - 1] = m_genericDevice ? -1 : voiceNum;

	if (IsLiveOnlyChannel(ch))
	{
		LiveChannelState& state = LiveStateFor(ch);
		state.voice = voiceNum;
		state.gmVoice = m_genericDevice;
	}

	SendMidiMessage(MidiMessage::controllerEvent(midiChannel, 0, (voiceNum >> 16) & 0x7f));
	SendMidiMessage(MidiMessage::controllerEvent(midiChannel, 32, (voiceNum >> 8) & 0x7f));
	SendMidiMessage(MidiMessage::programChange(midiChannel, voiceNum & 0x7f));

	if (m_genericDevice)
	{
		// a general MIDI device does not report its voices: the name is shown from here
		m_genericBank[midiChannel - 1] = (voiceNum >> 8) & 0x7f7f;
		SetVoiceOf(ch, String(voiceNum));
		NotifyChanged(apVoice, ch);
	}
}

void PianoController::SetSavedSongChannelVoice(Channel ch, int voiceNum, bool gmVoice)
{
	if (ch < chMidi1 || ch > chMidi16)
	{
		return;
	}
	SetSongChannelVoice(ch, ConvertSongChannelVoice(ch, voiceNum, gmVoice, m_genericDevice));
}

int PianoController::GetOriginalSongChannelVoice(Channel ch) const
{
	if (ch < chMidi1 || ch > chMidi16 || !m_genericDevice)
	{
		return -1;
	}
	const int index = ch - chMidi1;
	const String voice = VoiceOf(ch);
	return m_originalVoice[index] >= 0 && voice.isNotEmpty() && voice.getIntValue() == m_convertedVoice[index] ?
		m_originalVoice[index] : -1;
}

// Converts a song channel voice between Yamaha and General MIDI voices. A Yamaha voice
// converted to General MIDI is remembered, so that converting back gives exactly the
// same voice (if the channel still has the converted voice).
int PianoController::ConvertSongChannelVoice(Channel ch, int voiceNum, bool fromGm, bool toGm)
{
	const int index = ch - chMidi1;
	const bool drums = ch == chMidi10;
	if (!fromGm && toGm)
	{
		const int gmVoice = Presets::GmVoiceForYamahaVoice(voiceNum, drums);
		m_originalVoice[index] = voiceNum;
		m_convertedVoice[index] = gmVoice;
		return gmVoice;
	}
	if (fromGm && !toGm)
	{
		if (m_originalVoice[index] >= 0 && m_convertedVoice[index] == voiceNum)
		{
			return m_originalVoice[index];
		}
		return Presets::YamahaVoiceForGmVoice(voiceNum, drums);
	}
	return voiceNum;
}

void PianoController::ClearConvertedVoices()
{
	for (int i = 0; i < 16; i++)
	{
		m_originalVoice[i] = -1;
		m_convertedVoice[i] = -1;
	}
}

void PianoController::SetActive(Channel ch, bool active)
{
	if (ch == chStyle)
	{
		SetStyleOn(active);
		return;
	}
	if (m_localPlayback && (IsSongChannel(ch) || ch == chMidiMaster))
	{
		// song channels are switched on and off by the local player, not by the piano
		m_channels[ch].active = active;
		NotifyChanged(apActive, ch);
		UpdateLocalMutes();
		return;
	}
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Set, Property::Active, ch, active ? 1 : 0));
}

void PianoController::SetSplitPoint(int splitPoint)
{
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Set, Property::SplitPoint, 0, splitPoint));
}

void PianoController::SetStyleSplitPoint(int splitPoint)
{
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Set, Property::SplitPoint, 1, splitPoint));
}

void PianoController::SetLidPosition(LidPosition position)
{
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Set, Property::LidPosition, 0, position));
}

void PianoController::SetEnvironment(int environment)
{
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Set, Property::Environment, 0, environment));
}

void PianoController::SetBrightness(int brightness)
{
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Set, Property::Brightness, 0, brightness));
}

void PianoController::SetTouchCurve(TouchCurve touchCurve)
{
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Set, Property::TouchCurve, 0, touchCurve));
}

void PianoController::SetFixedCurve(Channel ch, bool active)
{
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Set, Property::FixedCurve, ch, active ? 1 : 0));
}

void PianoController::SetFixedVelocity(int fixedVelocity)
{
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Set, Property::FixedVelocity, 0, fixedVelocity));
}

// The piano's Transpose property: index 1 is the keyboard, index 2 the MIDI song
// (index 0 transposes the whole instrument and is not used).
void PianoController::SetKeyboardTranspose(int transpose)
{
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Set, Property::Transpose, 1,
		jlimit(MinTranspose, MaxTranspose, transpose) + TransposeBase));
}

void PianoController::SetMasterTune(int masterTune)
{
	int tune = masterTune * MasterTuneFactor + MasterTuneBase;
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Set, Property::MasterTune, 0, tune));
}

void PianoController::SetVrm(bool vrm)
{
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Set, Property::Vrm, 0, vrm ? 1 : 0));
}

void PianoController::SetDamperResonance(int damperResonance)
{
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Set, Property::DamperResonance, 0, damperResonance));
}

void PianoController::SetStringResonance(int stringResonance)
{
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Set, Property::StringResonance, 0, stringResonance));
}

void PianoController::SetKeyOffSampling(int keyOffSampling)
{
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Set, Property::KeyOffSampling, 0, keyOffSampling));
}

void PianoController::IncomingMidiMessage(const MidiMessage& message)
{
	if (message.isNoteOnOrOff())
	{
		NotifyNoteMessage(message);
	}

	// recording: the piano sends its own keys on channel 1 (Main), 2 (Layer) and 3 (Left),
	// and its accompaniment on channel 9..16 (with XG system exclusive messages)
	if (message.isSysEx())
	{
		m_recorder.AddStyle(message);
	}
	else
	{
		const int channel = message.getChannel();
		if (channel >= 1 && channel <= 3)
		{
			m_recorder.Add(LiveRecorder::srcMain + (channel - 1), message);
		}
		else if (channel >= 9 && channel <= 16)
		{
			m_recorder.AddStyle(message);
		}
	}
}

void PianoController::IncomingPianoMessage(const PianoMessage& message)
{
	std::unique_ptr<PianoMessage> pm = std::make_unique<PianoMessage>(message);

	const Action action = pm->GetAction();

	if ((action != Action::Info && action != Action::Response) ||
		(lastMessage && pm->DataEqualsTo(*lastMessage)))
	{
		return;
	}

	if (action == Action::Response && pm->GetResponseStatus() != 0)
	{
		// the piano rejected the request; the message contains no valid value
		Logger::writeToLog("Piano rejected request, status " + String::toHexString(pm->GetResponseStatus()));
		return;
	}

	const Property property = pm->GetProperty();

	if (m_genericDevice &&
		(property == Property::Volume || property == Property::Pan || property == Property::Reverb ||
		property == Property::VoiceMidi) &&
		(IsSongChannel((Channel)pm->GetIndex()) || pm->GetIndex() == chMidiMaster))
	{
		// the song is played by a MIDI device: these belong to the piano's (unused) mixer
		return;
	}

	if (property == Property::Tempo && pm->GetIndex() != 1)
	{
		// the tempo of the piano itself (index 1 is the tempo of its song): the
		// accompaniment follows it, also when ConPianist plays a song itself
		const int pianoTempo = pm->GetIntValue();
		const bool changed = m_pianoTempoKnown && pianoTempo != m_pianoTempo;
		m_pianoTempo = pianoTempo;
		m_recorder.AddTempo(pianoTempo); // a change of the tempo is recorded with the music
		m_pianoTempoKnown = true;
		NotifyChanged(apStyle);

		// ConPianist plays the song itself: its player follows a change of the piano's
		// tempo (e.g. a new style, or the tempo set on the piano). Not the first report
		// (the song keeps its tempo when the piano connects), and not the echo of a
		// tempo that was just sent to the piano.
		if (changed && m_localPlayback && pianoTempo != m_tempo &&
			Time::getMillisecondCounter() - m_tempoSentMs > 700)
		{
			std::weak_ptr<bool> alive = m_alive;
			MessageManager::callAsync([this, alive, pianoTempo]()
				{
					if (alive.lock() && m_localPlayback && m_tempo != pianoTempo &&
						Time::getMillisecondCounter() - m_tempoSentMs > 700)
					{
						// as if it was set in the Accompaniment window: the speed goes to 100%
						SetLocalTempo(pianoTempo);
					}
				});
		}
	}

	if (m_localPlayback &&
		(property == Property::Position || property == Property::Length ||
		property == Property::Play || property == Property::SongName ||
		property == Property::Loop || property == Property::Tempo ||
		(property == Property::Transpose && pm->GetIndex() != 1) || property == Property::Present ||
		property == Property::Part || property == Property::PartChannel ||
		property == Property::PartAuto ||
		(property == Property::Active &&
			(IsSongChannel((Channel)pm->GetIndex()) || pm->GetIndex() == chMidiMaster))))
	{
		// ConPianist plays the song itself; these values belong to the piano's own
		// (unused) song player and would overwrite the local playback state
		return;
	}

	const uint8_t* data = pm->GetRawValue();
	const int size = pm->GetSize();
	const int index = pm->GetIndex();
	const int intValue = pm->GetIntValue();
	const bool boolValue = intValue == 1;
	Channel ch = (Channel)index;

	if (property == Property::Position && size == 4)
	{
		m_position = {(data[0] << 7) + data[1], (data[2] << 7) + data[3]};
		if (m_playing && m_songLoaded)
		{
			// the settings while playing; not after jumping back (at the end of the song
			// the piano resets them and goes back to the beginning)
			if (m_position.measure >= m_lastPlayedMeasure)
			{
				m_playingSnapshot = TakeSnapshot();
			}
			m_lastPlayedMeasure = m_position.measure;
		}
		NotifyChanged(apPosition);
	}
	else if (property == Property::Length && size == 4)
	{
		m_length = {(data[0] << 7) + data[1], (data[2] << 7) + data[3]};
		m_songLoaded = m_length.measure > 1 || m_length.beat > 1;
		m_channels[chMidiMaster].enabled = m_songLoaded;
		m_channels[chMidiMaster].active = m_songLoaded;
		NotifyChanged(apLength);
		NotifyChanged(apEnable, chMidiMaster);
	}
	else if (property == Property::Play)
	{
		const bool wasPlaying = m_playing;
		m_playing = boolValue;
		if (wasPlaying && !m_playing)
		{
			ResumeStyleSyncStartLater();
		}
		if (m_playing && !wasPlaying)
		{
			m_stopRequested = false;
			m_lastPlayedMeasure = 0;
			m_playingSnapshot.valid = false;
		}
		else if (wasPlaying && !m_playing && !m_stopRequested && m_playingSnapshot.valid &&
			m_lastPlayedMeasure >= m_length.measure - 1)
		{
			// The song has ended: the piano goes back to the beginning and sets the song's
			// own settings (voices, volumes etc.) again. The settings made in ConPianist
			// are restored, after a short delay that lets the piano finish.
			Logger::writeToLog("End of the song: restoring the settings");
			const MixSnapshot snapshot = m_playingSnapshot;
			m_playingSnapshot.valid = false;
			std::weak_ptr<bool> alive = m_alive;
			MessageManager::callAsync([this, alive, snapshot]()
				{
					Timer::callAfterDelay(800, [this, alive, snapshot]()
						{
							if (alive.lock() && !m_localPlayback && m_songLoaded && !m_playing)
							{
								ApplySnapshot(snapshot);
							}
						});
				});
		}
		NotifyChanged(apPlayback);
	}
	else if (property == Property::Guide)
	{
		m_guide = boolValue;
		NotifyChanged(apGuide);
	}
	else if (property == Property::GuideType)
	{
		m_guideType = (GuideType)intValue;
		NotifyChanged(apGuide);
	}
	else if (property == Property::StreamLights)
	{
		m_streamLights = boolValue;
		NotifyChanged(apStreamLights);
	}
	else if (property == Property::StreamSpeed)
	{
		m_streamFast = boolValue;
		NotifyChanged(apStreamLights);
	}
	else if (property == Property::Part && index < numElementsInArray(m_parts))
	{
		m_parts[(Part)index] = boolValue;
		NotifyChanged(apPart);
	}
	else if (property == Property::PartChannel && index < numElementsInArray(m_partChannels))
	{
		Channel newCh = (Channel)(chMidi0 + intValue);
		Channel oldCh = m_partChannels[index];
		m_partChannels[index] = newCh;
		NotifyChanged(apPartChannel, oldCh);
		NotifyChanged(apPartChannel, newCh);
	}
	else if (property == Property::PartAuto)
	{
		m_partAuto = boolValue;
		NotifyChanged(apPartAuto);
	}
	else if (property == Property::Volume)
	{
		m_channels[ch].volume = intValue;
		NoteStyleDefault(ch, 0, intValue);
		NotifyChanged(apVolume, ch);
	}
	else if (property == Property::Pan)
	{
		m_channels[ch].pan = intValue - PanBase;
		NoteStyleDefault(ch, 1, intValue - PanBase);
		NotifyChanged(apPan, ch);
	}
	else if (property == Property::Reverb)
	{
		m_channels[ch].reverb = intValue;
		NoteStyleDefault(ch, 2, intValue);
		NotifyChanged(apReverb, ch);
	}
	else if (property == Property::Octave)
	{
		m_channels[ch].octave = intValue - OctaveBase;
		NotifyChanged(apOctave, ch);
	}
	else if (property == Property::Active && ch != chMidiMaster && ch != chStyle)
	{
		// (the song as a whole cannot be switched on and off; the accompaniment as a whole
		// is on while any of its parts is on)
		m_channels[ch].active = boolValue;
		NotifyChanged(apActive, ch);
		if (ch >= chStylePart1 && ch < chStylePart1 + NumStyleParts)
		{
			UpdateStyleOn();
		}
	}
	else if (property == Property::Present && ch != chMidiMaster)
	{
		m_channels[ch].enabled = boolValue;
		NotifyChanged(apEnable, ch);
		if (!boolValue && IsLiveOnlyChannel(ch))
		{
			// a Live Play channel not used in the (new) song: its own settings again
			std::weak_ptr<bool> alive = m_alive;
			MessageManager::callAsync([this, alive, ch]()
				{
					if (alive.lock())
					{
						ApplyLiveChannel(ch);
					}
				});
		}
	}
	else if (property == Property::VoiceMidi && size == 4)
	{
		int voice = (data[0] << 7 * 3) + (data[1] << 7 * 2) + (data[2] << 7) + data[3];
		if (ch >= chMidi1 && ch <= chMidi16)
		{
			// In GS mode (after a GS reset, e.g. of a song or another program) the bank MSB
			// is the variation of a GS voice: the piano reports a voice sent to the channel
			// as 119 / bank MSB / program. The voice that was sent is kept, it has a name
			// and can be saved. Any other answer is a voice set by the song itself.
			int& sent = m_sentSongVoice[ch - chMidi1];
			const int reported = (GsBank << 16) | (((sent >> 16) & 0x7f) << 8) | (sent & 0x7f);
			if (sent >= 0 && (voice == sent || voice == reported))
			{
				voice = sent;
			}
			else
			{
				sent = -1;
			}
		}
		SetVoiceOf(ch, String(voice));
		NotifyChanged(apVoice, ch);
	}
	else if (property == Property::Tempo)
	{
		m_tempo = intValue;
		NotifyChanged(apTempo);
	}
	else if (property == Property::StyleName)
	{
		// the path as it is, in UTF-8
		const String raw = pm->GetStrValue();
		std::vector<char> utf8(raw.length() + 1);
		for (int i = 0; i < raw.length(); i++)
		{
			utf8[i] = (char)raw[i];
		}
		utf8[raw.length()] = 0;
		bool changed = false;
		{
			const ScopedLock lock(m_styleLock);
			const String name = String::fromUTF8(utf8.data());
			changed = name != m_styleName;
			m_styleName = name;
			m_styleNameKnown = true;
			// a state is being restored (RestoreStyleState): it goes on now that the
			// style of the piano is known, or that the piano has loaded the restored one
			const int stage = m_styleRestoreStage;
			if (stage == srWaitName)
			{
				StyleRestoreAfter(0, srWaitName);
			}
			else if (stage == srWaitLoad && name == m_styleRestoreName)
			{
				StyleRestoreAfter(500, srWaitLoad);
			}
		}
		if (changed)
		{
			ForgetStyleOffParts(); // the parts remembered belong to the old style
		}
		NotifyChanged(apStyle);
		if (changed)
		{
			// Another style: its parts have their own settings. They are asked for soon,
			// before anything changes them (a registration memory sets its own values half
			// a second after the style is loaded): these are the defaults of the style.
			// Asked for once more later, in case the piano was not ready with them.
			std::weak_ptr<bool> alive = m_alive;
			MessageManager::callAsync([this, alive]()
				{
					Timer::callAfterDelay(300, [this, alive]()
						{
							if (alive.lock())
							{
								QueryStyleParts(true);
							}
						});
					Timer::callAfterDelay(1500, [this, alive]()
						{
							if (alive.lock())
							{
								QueryStyleParts(false);
							}
						});
				});
		}
	}
	else if (property == Property::StyleChordArea)
	{
		m_styleChordArea = intValue;
		NotifyChanged(apStyle);
	}
	else if (property == Property::StyleLeftSound)
	{
		m_styleLeftSound = intValue != 0 ? 1 : 0;
		NotifyChanged(apStyle);
	}
	else if (property == Property::StylePlay)
	{
		const bool stopped = m_stylePlaying && !boolValue;
		const bool playChanged = m_stylePlaying != boolValue;
		m_stylePlaying = boolValue;
		NotifyChanged(apStyle);
		if (playChanged)
		{
			// not reported by the piano when it is changed elsewhere
			m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::StyleLeftSound));
		}
		if (stopped && m_styleSyncWanted && !m_styleSyncSuspended)
		{
			// Sync Start was on before the accompaniment started: on again, a moment
			// later, when the piano has finished stopping
			std::weak_ptr<bool> alive = m_alive;
			MessageManager::callAsync([this, alive]()
				{
					Timer::callAfterDelay(300, [this, alive]()
						{
							if (alive.lock() && m_connected && m_styleSyncWanted && !m_styleSyncSuspended &&
								!m_stylePlaying && !m_styleSyncStart)
							{
								m_pianoConnector->SendPianoMessage(PianoMessage(Action::Set, Property::StyleSyncStart, 0, 1));
							}
						});
				});
		}
	}
	else if (property == Property::StyleSyncStart)
	{
		m_styleSyncStart = boolValue;
		if (boolValue)
		{
			// also when it was switched on at the piano; switching off is remembered only
			// from the program (the piano switches it off itself when the style starts)
			m_styleSyncWanted = true;
		}
		NotifyChanged(apStyle);
	}
	else if (property == Property::StyleSection)
	{
		const int previous = m_styleSection;
		(index == 1 ? m_styleNextSection : m_styleSection) = intValue;
		NotifyChanged(apStyle);

		// the fill in of a change of the main section has ended: if the piano went back
		// to the old main section, the new one is chosen again
		const int target = m_styleFillTarget;
		const bool wasFill = previous >= ssFillInAA && previous < ssFillInAA + 4;
		const bool isFill = intValue >= ssFillInAA && intValue < ssFillInAA + 4;
		if (index == 0 && target != ssNone && wasFill && !isFill)
		{
			m_styleFillTarget = ssNone;
			if (intValue != target && Time::getMillisecondCounter() - m_styleFillTargetMs < 20000)
			{
				std::weak_ptr<bool> alive = m_alive;
				MessageManager::callAsync([this, alive, target]()
					{
						if (alive.lock() && m_stylePlaying)
						{
							SendStyleSection(target, true);
						}
					});
			}
		}
	}
	else if (property == Property::StylePosition && size == 4)
	{
		m_stylePosition = (((data[0] << 7) + data[1]) << 16) | ((data[2] << 7) + data[3]);
		NotifyChanged(apStyle);
	}
	else if (property == Property::StyleChord && size == 4)
	{
		m_styleChord = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
		m_recorder.AddChord(m_styleChord); // recorded with the music
		NotifyChanged(apStyle);
	}
	else if (property == Property::Metronome)
	{
		if (!m_genericDevice)
		{
			m_metronome = boolValue;
			NotifyChanged(apMetronome);
		}
	}
	else if (property == Property::MetronomeVolume)
	{
		if (!m_genericDevice)
		{
			m_metronomeVolume = jlimit(0, 127, intValue);
			NotifyChanged(apMetronome);
		}
	}
	else if (property == Property::MetronomeBell)
	{
		if (!m_genericDevice)
		{
			m_metronomeBell = boolValue;
			NotifyChanged(apMetronome);
		}
	}
	else if (property == Property::MetronomeBeat && size == 2)
	{
		if (data[0] > 0 && data[1] > 0)
		{
			m_metronomeNumerator = data[0];
			m_metronomeDenominator = data[1];
			m_pianoCountPeriod = 0;
			// (the piano sets the time signature of a style when it is loaded: a change
			// while recording is written into the recording, only if it changed)
			m_recorder.AddTimeSignature(data[0], data[1]);
			NotifyChanged(apMetronome);
		}
	}
	else if (property == Property::MetronomeCount)
	{
		if (!m_genericDevice)
		{
			// The piano reports a beat when it ends, i.e. together with the click of the
			// next beat: the count of the last beat of the measure arrives on the downbeat
			// (on the bell). The length of the measure is learned from the wrap of the
			// count; until then the beat setting of the metronome is used.
			const uint32 now = Time::getMillisecondCounter();
			if (intValue == 1 && m_pianoCount > 1 && now - m_lastBeatMs < 2500)
			{
				m_pianoCountPeriod = m_pianoCount;
			}
			m_pianoCount = intValue;
			const int period = m_pianoCountPeriod > 0 ? m_pianoCountPeriod : jmax(1, (int)m_metronomeNumerator);
			OnBeat(intValue >= period ? 1 : intValue + 1);
		}
	}
	else if (property == Property::Transpose && index == 1)
	{
		m_keyboardTranspose = intValue - TransposeBase;
		NotifyChanged(apKeyboardTranspose);
	}
	else if (property == Property::Transpose && index == 2)
	{
		m_transpose = intValue - TransposeBase;
		NotifyChanged(apTranspose);
	}
	else if (property == Property::ReverbEffect)
	{
		m_reverbEffect = intValue;
		NotifyChanged(apReverbEffect);
	}
	else if (property == Property::Loop && size == 9)
	{
		bool enabled = data[0] == 1;
		if (enabled)
		{
			m_loop = {{(data[1] << 7) + data[2], (data[3] << 7) + data[4]},
				{(data[5] << 7) + data[6], (data[7] << 7) + data[8]}};
			m_loopStart = {0,0};
		}
		else
		{
			m_loop = {{0,0},{0,0}};
		}
		NotifyChanged(apLoop);
	}
	else if (property == Property::SplitPoint)
	{
		// index 0: the split point of the Left part, 1: of the accompaniment
		(index == 1 ? m_styleSplitPoint : m_splitPoint) = intValue;
		NotifyChanged(apSplitPoint);
	}
	else if (property == Property::LidPosition)
	{
		m_lidPosition = (LidPosition)intValue;
		NotifyChanged(apLidPosition);
	}
	else if (property == Property::Environment)
	{
		m_environment = intValue;
		NotifyChanged(apEnvironment);
	}
	else if (property == Property::Brightness)
	{
		m_brightness = intValue;
		NotifyChanged(apBrightness);
	}
	else if (property == Property::TouchCurve)
	{
		m_touchCurve = (TouchCurve)intValue;
		NotifyChanged(apTouchCurve);
	}
	else if (property == Property::FixedCurve && index < numElementsInArray(m_fixedCurve))
	{
		m_fixedCurve[index] = boolValue;
		NotifyChanged(apFixedCurve);
	}
	else if (property == Property::FixedVelocity)
	{
		m_fixedVelocity = intValue;
		NotifyChanged(apFixedVelocity);
	}
	else if (property == Property::MasterTune)
	{
		m_masterTune = (intValue - MasterTuneBase) / MasterTuneFactor;
		NotifyChanged(apMasterTune);
	}
	else if (property == Property::Vrm)
	{
		m_vrm = boolValue;
		NotifyChanged(apVrm);
	}
	else if (property == Property::DamperResonance)
	{
		m_damperResonance = intValue;
		NotifyChanged(apDamperResonance);
	}
	else if (property == Property::StringResonance)
	{
		m_stringResonance = intValue;
		NotifyChanged(apStringResonance);
	}
	else if (property == Property::KeyOffSampling)
	{
		m_keyOffSampling = intValue;
		NotifyChanged(apKeyOffSampling);
	}
	else if (property == Property::VoicePreset)
	{
		SetVoiceOf(ch, pm->GetStrValue());
		NotifyChanged(apVoice, ch);
	}
	else if (property == Property::PianoModel)
	{
		{
			const ScopedLock lock(m_stringLock);
			m_model = pm->GetStrValue();
		}
	}
	else if (property == Property::FirmwareVersion)
	{
		{
			const ScopedLock lock(m_stringLock);
			m_version = pm->GetStrValue();
		}
		if (!m_connected)
		{
			m_connected = true;
			NotifyChanged(apConnection);

			// The song may have been loaded while the piano was switched off; its voices and
			// settings are sent only at loading, so it is loaded again. Asked on the message
			// thread: this thread (of the connection) may hold its own lock, and the own
			// player may be sending to the same connection with its lock held.
			{
				std::weak_ptr<bool> alive = m_alive;
				MessageManager::callAsync([this, alive]()
					{
						if (alive.lock() && IsLocalSongLoaded() && m_playbackSource == psLocal)
						{
							ReloadSong();
						}
					});
			}
		}
	}
	else if (property == Property::SongName)
	{
		String name = DecodeSongName(pm->GetStrValue());
		SetSongNameValue(name);
		NotifyChanged(apSongName);
		if (m_songLoading)
		{
			m_songLoading = false;
			if (!m_localPlayback)
			{
				m_networkTempoSet = false; // a new song of the piano's player: its own tempo
			}
			NotifyChanged(apSongLoaded);

			if (m_pendingMeasure > 1 || m_pendingSnapshot.valid)
			{
				// the song was loaded again after switching the player: go back to the
				// measure where it was and restore the settings, when the piano has
				// finished loading (see RestorePendingSongState)
				const int measure = m_pendingMeasure;
				std::weak_ptr<bool> alive = m_alive;
				MessageManager::callAsync([this, alive, measure]()
					{
						Timer::callAfterDelay(500, [this, alive, measure]()
							{
								if (alive.lock())
								{
									RestorePendingSongState(measure, 0);
								}
							});
					});
			}
			m_pendingMeasure = 0;
		}
	}

	lastMessage = std::move(pm);
}

//==============================================================================
// Local playback

void PianoController::SetLocalPlayback(bool enabled)
{
	if (enabled != m_localPlayback)
	{
		ApplyPlaybackSource(enabled ? psLocal : psPiano);
	}
}

void PianoController::ApplyPlaybackSource(PlaybackSource source)
{
	if (m_localPlayer)
	{
		// silence the old output before switching
		m_localPlayer->Unload();
		m_localPlayer->SetMasterVolumeScale(1.0);
	}
	if (m_softMetronome)
	{
		// the own metronome sounds on the MIDI device only
		m_softMetronome.reset();
		m_metronome = false;
	}

	{
		// The Live Play notes and pedal are released on the old output (even if the
		// channels stay the same, the output may change), and the output is switched, in
		// one step: a note played meanwhile (MIDI In 2) is not started on the old output
		// and released on the new one. The pedal held down is sent again when the switch
		// is complete (ResumeLivePedal).
		const ScopedLock lock(m_liveLock);
		SuspendLive();
		m_playbackSource = source;
		m_localPlayback = source != psPiano;
		m_genericDevice = source == psMidiDevice;
		UpdateLiveTarget(); // MIDI device playback: Live Play on the Mixer channels (MIDI Out)
	}
	const bool enabled = m_localPlayback;
	Logger::writeToLog("Playback: " + String(source == psPiano ? "piano" : source == psLocal ? "own player (USB)" : "own player (MIDI device)"));

	if (enabled && !m_localPlayer)
	{
		m_localPlayer = std::make_unique<LocalSongPlayer>();
		// notes are sent directly (not through the message queue) for exact timing
		m_localPlayer->sendMidi = [this](const MidiMessage& message)
			{
				SendToOutput(message);
				OnLocalMessage(message);
			};
		// the chords written into the song are shown like the chords the piano recognizes
		m_localPlayer->onChord = [this](int chord)
			{
				m_styleChord = chord;
				NotifyChanged(apStyle);
			};
		m_localPlayer->onChanged = [this](bool positionChanged, bool playingChanged)
			{
				if (positionChanged)
				{
					NotifyChanged(apPosition);
					// the song may change its tempo: the shown tempo follows it
					std::weak_ptr<bool> alive = m_alive;
					MessageManager::callAsync([this, alive]()
						{
							if (alive.lock())
							{
								UpdateLocalTempo();
							}
						});
					if (m_localPlayer && m_localPlayer->IsPlaying())
					{
						OnBeat(m_localPlayer->GetPosition().beat);
					}
				}
				if (playingChanged)
				{
					NotifyChanged(apPlayback);
					if (m_localPlayer && !m_localPlayer->IsPlaying())
					{
						ResumeStyleSyncStartLater();
					}
				}
			};
	}

	ClearSongState();

	if (enabled)
	{
		ResetLocalMixState();
	}
}

// Song playback of the own player: to the piano (USB) or to the MIDI device.
void PianoController::SendToOutput(const MidiMessage& message)
{
	if (m_genericDevice)
	{
		if (sendToMidiDevice) sendToMidiDevice(message);
	}
	else
	{
		m_pianoConnector->SendMidiMessageNow(message);
	}
}

void PianoController::SendMidiMessage(const MidiMessage& message)
{
	if (m_genericDevice)
	{
		if (sendToMidiDevice) sendToMidiDevice(message);
	}
	else
	{
		m_pianoConnector->SendMidiMessage(message);
	}
}

// Messages from MIDI In 2: played like the virtual keyboard (Live Play), and their
// notes are shown on the virtual keyboard.
void PianoController::IncomingMidiDeviceMessage(const MidiMessage& message)
{
	if (message.isNoteOnOrOff())
	{
		NotifyNoteMessage(message);
	}
	PlayLive(message);
}

void PianoController::SetLiveChannels(int channelMask)
{
	channelMask &= 0xFFFF;
	if (channelMask == 0)
	{
		channelMask = 1; // channel 1 by default
	}

	int added;
	{
		const ScopedLock lock(m_liveLock);
		added = channelMask & ~m_liveMixerChannels;
		m_liveMixerChannels = channelMask;
		ApplyLiveChannels(IsLivePlayOnPianoKeyboard() ? LiveKeyboardBit : m_liveMixerChannels);
	}

	// channels not used in the song, now used for Live Play: their own settings
	for (Channel ch : MidiChannels)
	{
		if (added & (1 << (ch - chMidi1)))
		{
			ApplyLiveChannel(ch);
		}
	}
}

bool PianoController::IsLiveOnlyChannel(Channel ch) const
{
	return IsSongChannel(ch) && (m_liveMixerChannels & (1 << (ch - chMidi1))) != 0 &&
		!m_channels[ch].enabled;
}

// The state of a live-only channel; the defaults are filled in when it is used first.
PianoController::LiveChannelState& PianoController::LiveStateFor(Channel ch)
{
	LiveChannelState& state = m_liveState[ch - chMidi1];
	if (!state.set)
	{
		state.set = true;
		state.gmVoice = m_genericDevice;
		// piano (CFX Grand); on the drum channel the standard drum kit
		state.voice = m_genericDevice ? 0 :
			ch == chMidi10 ? Presets::YamahaVoiceForGmVoice(0, true) : Presets::Voices().front().num;
		state.volume = DefaultVolume;
		state.pan = DefaultPan;
		state.reverb = m_genericDevice ? GenericDefaultReverb : DefaultReverb;
	}
	return state;
}

// The state to save: a Yamaha voice converted to General MIDI is saved as the original.
PianoController::LiveChannelState PianoController::GetLiveChannelState(Channel ch) const
{
	if (!IsSongChannel(ch))
	{
		return {};
	}
	const int index = ch - chMidi1;
	LiveChannelState state = m_liveState[index];
	if (state.set && state.gmVoice && m_liveOriginalVoice[index] >= 0 &&
		m_liveConvertedVoice[index] == state.voice)
	{
		state.voice = m_liveOriginalVoice[index];
		state.gmVoice = false;
	}
	return state;
}

void PianoController::SetLiveChannelState(Channel ch, const LiveChannelState& state)
{
	if (!IsSongChannel(ch))
	{
		return;
	}
	const int index = ch - chMidi1;
	m_liveState[index] = state;
	m_liveOriginalVoice[index] = -1;
	m_liveConvertedVoice[index] = -1;
	ApplyLiveChannel(ch);
}

// Semitones added to the Live Play notes of a Mixer channel (1..16; not the drum channel
// and not the piano's keyboard parts, which have their own octaves).
int PianoController::LiveOctaveShift(int midiChannel) const
{
	return midiChannel >= 1 && midiChannel <= 16 && midiChannel != 10 ? m_liveOctave[midiChannel - 1] * 12 : 0;
}

void PianoController::SetLiveOctave(Channel ch, int octave)
{
	if (!IsSongChannel(ch) || ch == chMidi10)
	{
		return;
	}
	octave = jlimit(-2, 2, octave);
	const int midiChannel = ch - chMidi0;
	{
		const ScopedLock lock(m_liveLock);
		if (m_liveOctave[midiChannel - 1] == octave)
		{
			return;
		}
		// the notes held on the channel are released with the old octave
		const int bit = 1 << (midiChannel - 1);
		for (int note = 0; note < 128; note++)
		{
			if (m_liveNoteChannels[note] & bit)
			{
				const int sounding = note + m_liveNoteTranspose[note] + LiveOctaveShift(midiChannel);
				if (sounding >= 0 && sounding <= 127)
				{
					SendLive(midiChannel, MidiMessage::noteOff(1, sounding));
				}
				m_liveNoteChannels[note] &= ~bit;
			}
		}
		m_liveOctave[midiChannel - 1] = octave;
	}
	NotifyChanged(apOctave, ch);
}

bool PianoController::CanTakeKeyboardPart(Channel part) const
{
	return Presets::FindVoice(VoiceOf(part)) != nullptr;
}

void PianoController::TakeKeyboardPart(Channel mixerChannel, Channel part)
{
	if (!IsSongChannel(mixerChannel) || mixerChannel == chMidi10)
	{
		return; // the drum channel plays drum kits only
	}

	Voice* preset = Presets::FindVoice(VoiceOf(part));
	if (preset)
	{
		if (m_genericDevice)
		{
			// the nearest General MIDI voice; the Yamaha voice is used again on the piano
			const bool liveOnly = IsLiveOnlyChannel(mixerChannel);
			const int gmVoice = liveOnly ? Presets::GmVoiceForYamahaVoice(preset->num, false) :
				ConvertSongChannelVoice(mixerChannel, preset->num, false, true);
			SetSongChannelVoice(mixerChannel, gmVoice);
			if (liveOnly)
			{
				m_liveOriginalVoice[mixerChannel - chMidi1] = preset->num;
				m_liveConvertedVoice[mixerChannel - chMidi1] = gmVoice;
			}
		}
		else
		{
			SetSongChannelVoice(mixerChannel, preset->num);
		}
	}

	SetVolume(mixerChannel, m_channels[part].volume);
	SetPan(mixerChannel, m_channels[part].pan);
	SetReverb(mixerChannel, m_channels[part].reverb);
	SetLiveOctave(mixerChannel, m_channels[part].octave);
}

// The voice (0x00MMLLPP) a MIDI file sets first on a channel (1..16), or -1.
static int SongSetupVoice(const File& file, int midiChannel)
{
	FileInputStream stream(file);
	MidiFile midiFile;
	if (!stream.openedOk() || !LocalSongPlayer::ReadMidiFile(stream, midiFile))
	{
		return -1;
	}

	int voice = -1;
	double voiceTime = 0;
	for (int t = 0; t < midiFile.getNumTracks(); t++)
	{
		const MidiMessageSequence* track = midiFile.getTrack(t);
		int msb = 0;
		int lsb = 0;
		for (int i = 0; i < track->getNumEvents(); i++)
		{
			const MidiMessage& msg = track->getEventPointer(i)->message;
			if (msg.getChannel() != midiChannel)
			{
				continue;
			}
			if (msg.isControllerOfType(0))
			{
				msb = msg.getControllerValue();
			}
			else if (msg.isControllerOfType(32))
			{
				lsb = msg.getControllerValue();
			}
			else if (msg.isProgramChange())
			{
				if (voice < 0 || msg.getTimeStamp() < voiceTime)
				{
					voice = (msb << 16) | (lsb << 8) | msg.getProgramChangeNumber();
					voiceTime = msg.getTimeStamp();
				}
				break; // the first voice of the channel in this track
			}
		}
	}
	return voice;
}

void PianoController::ResetChannelSettings(Channel ch)
{
	if (!IsSongChannel(ch))
	{
		return;
	}
	const int index = ch - chMidi1;

	if (IsLiveOnlyChannel(ch))
	{
		// the defaults of a live-only channel (also remembered as its settings)
		m_liveState[index] = {};
		m_liveOriginalVoice[index] = -1;
		m_liveConvertedVoice[index] = -1;
		ApplyLiveChannel(ch);
	}
	else if (m_channels[ch].enabled)
	{
		// the values of the song
		if (m_localPlayback)
		{
			// sent by ConPianist's own player when the song was loaded
			const int volume = GenericSetupValue(ch, 7, -1);
			const int pan = GenericSetupValue(ch, 10, -1);
			const int reverb = GenericSetupValue(ch, 91, -1);
			if (volume >= 0) SetVolume(ch, volume); else ResetVolume(ch);
			if (pan >= 0) SetPan(ch, pan - PanBase); else ResetPan(ch);
			if (reverb >= 0) SetReverb(ch, reverb); else ResetReverb(ch);
		}
		else
		{
			ResetVolume(ch); // the piano's own player knows the values of the song
			ResetPan(ch);
			ResetReverb(ch);
		}

		// the piano cannot reset the voice: it is read from the MIDI file
		const String songName = GetSongName();
		const File file(File::isAbsolutePath(songName) ? File(songName) : File());
		const int voice = file.existsAsFile() ? SongSetupVoice(file, ch - chMidi0) : -1;
		if (voice >= 0)
		{
			m_originalVoice[index] = -1;
			m_convertedVoice[index] = -1;
			SetSongChannelVoice(ch, voice);
		}
	}

	SetLiveOctave(ch, 0);
}

// Sends the settings of all live-only channels (e.g. after a song is loaded: the player
// or the piano may have reset them).
void PianoController::RestoreLiveChannels()
{
	for (Channel ch : MidiChannels)
	{
		ApplyLiveChannel(ch);
	}
}

// Sends the voice, volume, pan and reverb of a live-only channel to the current player.
// The voice is converted between Yamaha and General MIDI voices if it was set for the
// other kind of player.
void PianoController::ApplyLiveChannel(Channel ch)
{
	if (!IsLiveOnlyChannel(ch) || !IsReady())
	{
		return;
	}

	const int index = ch - chMidi1;
	const bool drums = ch == chMidi10;
	const bool toGm = m_genericDevice;
	const LiveChannelState state = LiveStateFor(ch); // a copy: the setters store it again

	int voice = state.voice;
	if (!state.gmVoice && toGm)
	{
		voice = Presets::GmVoiceForYamahaVoice(state.voice, drums);
		m_liveOriginalVoice[index] = state.voice;
		m_liveConvertedVoice[index] = voice;
	}
	else if (state.gmVoice && !toGm)
	{
		voice = m_liveOriginalVoice[index] >= 0 && m_liveConvertedVoice[index] == state.voice ?
			m_liveOriginalVoice[index] : Presets::YamahaVoiceForGmVoice(state.voice, drums);
	}

	SetSongChannelVoice(ch, voice);
	SetVolume(ch, state.volume);
	SetPan(ch, state.pan);
	SetReverb(ch, state.reverb);
}

// Uses the piano's keyboard or the Mixer channels for Live Play, as the settings and the
// player (MIDI device playback: always the Mixer channels) require now.
void PianoController::UpdateLiveTarget()
{
	const ScopedLock lock(m_liveLock);
	ApplyLiveChannels(IsLivePlayOnPianoKeyboard() ? LiveKeyboardBit : m_liveMixerChannels);
}

void PianoController::SetLivePianoKeyboard(bool enabled, bool available)
{
	{
		const ScopedLock lock(m_liveLock);
		if (enabled == m_liveKeyboardEnabled && available == m_liveKeyboardAvailable)
		{
			return;
		}
		m_liveKeyboardEnabled = enabled;
		m_liveKeyboardAvailable = available;
		ApplyLiveChannels(IsLivePlayOnPianoKeyboard() ? LiveKeyboardBit : m_liveMixerChannels);
	}
	NotifyChanged(apPlaybackSource); // the UI shows where Live Play sounds
}

// Sends a Live Play message: on a Mixer channel (1..16) like the song, or on the piano's
// own keyboard parts (17: the second MIDI port, channel 1).
void PianoController::SendLive(int channel, const MidiMessage& message)
{
	MidiMessage copy(message);
	if (channel == 17)
	{
		RecordLiveKeyboard(message);
		copy.setChannel(1);
		if (sendToPianoKeyboard)
		{
			sendToPianoKeyboard(copy);
		}
		return;
	}
	m_recorder.Add(LiveRecorder::srcMixer1 + (channel - 1), message);
	copy.setChannel(channel);
	if (m_liveSendNow && !m_genericDevice)
	{
		m_pianoConnector->SendMidiMessageNow(copy);
		return;
	}
	SendMidiMessage(copy);
}

// Recording of the notes played on the piano's keyboard parts from here (the piano does
// not send them back): they sound on the parts that are switched on, like the piano's
// own keys - Left below the split point, Main and Layer above it - and the piano adds
// its keyboard transpose and the octave of the part.
void PianoController::RecordLiveKeyboard(const MidiMessage& message)
{
	static const Channel parts[3] = {chMain, chLayer, chLeft};

	if (!message.isNoteOnOrOff())
	{
		for (int i = 0; i < 3; i++)
		{
			m_recorder.Add(LiveRecorder::srcMain + i, message);
		}
		return;
	}

	const int note = message.getNoteNumber();
	int sources = 0;
	if (message.isNoteOn())
	{
		const bool leftSide = m_channels[chLeft].active && note <= m_splitPoint;
		if (leftSide)
		{
			sources = 1 << 2;
		}
		else
		{
			if (m_channels[chMain].active) sources |= 1 << 0;
			if (m_channels[chLayer].active) sources |= 1 << 1;
		}
		m_recKeyboardSources[note] |= sources;
	}
	else
	{
		// released on the parts it was started on
		sources = m_recKeyboardSources[note];
		m_recKeyboardSources[note] = 0;
	}

	for (int i = 0; i < 3; i++)
	{
		if (sources & (1 << i))
		{
			const int sounding = note + m_keyboardTranspose + m_channels[parts[i]].octave * 12;
			if (sounding >= 0 && sounding <= 127)
			{
				MidiMessage copy(message);
				copy.setNoteNumber(sounding);
				m_recorder.Add(LiveRecorder::srcMain + i, copy);
			}
		}
	}
}

LiveRecorder::Setup PianoController::RecorderSetup(Channel ch)
{
	LiveRecorder::Setup setup;
	const ChannelInfo& info = m_channels[ch];
	const String voice = VoiceOf(ch);
	if (voice.startsWith("PRESET:"))
	{
		Voice* preset = Presets::FindVoice(voice);
		if (preset) setup.voice = preset->num;
	}
	else if (voice.isNotEmpty())
	{
		setup.voice = RealSongVoice(voice.getIntValue());
	}
	setup.volume = jlimit(0, 127, info.volume);
	setup.pan = jlimit(0, 127, info.pan + PanBase);
	setup.reverb = jlimit(0, 127, info.reverb);
	return setup;
}

void PianoController::UpdateRecorderSetups()
{
	// the tempo and the time signature of the recording: as they are when it starts
	m_recordedTempo = jlimit((int)MinTempo, (int)MaxTempo, m_tempo);
	m_recordedNumerator = m_metronomeNumerator;
	m_recordedDenominator = m_metronomeDenominator;

	for (int i = 0; i < 16; i++)
	{
		m_recorder.SetSetup(LiveRecorder::srcMixer1 + i, RecorderSetup((Channel)(chMidi1 + i)));
	}
	m_recorder.SetSetup(LiveRecorder::srcMain, RecorderSetup(chMain));
	m_recorder.SetSetup(LiveRecorder::srcLayer, RecorderSetup(chLayer));
	m_recorder.SetSetup(LiveRecorder::srcLeft, RecorderSetup(chLeft));

	// The voices of the accompaniment parts: the piano sends them on MIDI only when a
	// style is started for the first time after it was selected, so they are asked for
	// (the answers are used when the recording is saved).
	if (!m_genericDevice && m_connected)
	{
		for (int i = 0; i < NumStyleParts; i++)
		{
			const int ch = chStylePart1 + i;
			m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::VoiceMidi, ch, 0));
			m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::Volume, ch, 0));
			m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::Pan, ch, 0));
			m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::Reverb, ch, 0));
		}
	}
}

// The settings of the accompaniment parts (MIDI channel 9..16), as the piano reported
// them, for the file of the recording.
void PianoController::UpdateRecorderStyleSetups()
{
	if (m_genericDevice)
	{
		return;
	}
	for (int i = 0; i < NumStyleParts; i++)
	{
		const ChannelInfo& info = m_channels[chStylePart1 + i];
		const String voice = VoiceOf((Channel)(chStylePart1 + i));
		if (voice.isEmpty() || voice.startsWith("PRESET:"))
		{
			continue; // not known: what was heard on MIDI is used
		}
		LiveRecorder::Setup setup;
		setup.voice = voice.getIntValue();
		// the volume of a part on MIDI: scaled by the volume of the whole accompaniment
		setup.volume = jlimit(0, 127, roundToInt(info.volume * jlimit(0, 127, m_channels[chStyle].volume) / 127.0));
		setup.pan = jlimit(0, 127, info.pan + PanBase);
		setup.reverb = jlimit(0, 127, info.reverb);
		m_recorder.SetStyleSetup(i, setup);
	}
}

// The own metronome (on the MIDI device): a click on the drum channel on every beat.
class PianoController::SoftMetronome : public HighResolutionTimer
{
public:
	SoftMetronome(PianoController& owner) : owner(owner) {}
	~SoftMetronome() override { stopTimer(); }
	void hiResTimerCallback() override { owner.SoftMetronomeTick(); }
private:
	PianoController& owner;
};

void PianoController::SoftMetronomeTick()
{
	const double now = Time::getMillisecondCounterHiRes();
	if (now < m_softNextMs)
	{
		return;
	}
	const int numerator = jmax(1, (int)m_metronomeNumerator);
	const int denominator = jmax(1, (int)m_metronomeDenominator);
	m_softBeat = m_softBeat % numerator + 1;
	// a beat is one note of the denominator; the tempo counts quarter notes
	const double beatMs = 60000.0 / jlimit((int)MinTempo, (int)MaxTempo, m_tempo) * 4.0 / denominator;
	m_softNextMs = (m_softNextMs == 0 || now - m_softNextMs > beatMs ? now : m_softNextMs) + beatMs;

	// low wood block; with the bell the first beat is a triangle
	const bool bell = m_softBeat == 1 && m_metronomeBell;
	const int note = bell ? 81 : 77;
	// the volume is the velocity of the click (the drum channel has no volume of its own here)
	const int velocity = jlimit(1, 127, (int)m_metronomeVolume * (bell ? 127 : 105) / 127);
	if (m_metronomeVolume > 0)
	{
		SendToOutput(MidiMessage::noteOn(10, note, (uint8)velocity));
	}
	SendToOutput(MidiMessage::noteOff(10, note));
	OnBeat(m_softBeat);
}

// A beat of the metronome or of the playing song (any thread); beat 1 is the downbeat.
void PianoController::OnBeat(int beat)
{
	m_lastBeatMs = Time::getMillisecondCounter();
	m_recorder.Beat(beat == 1);
}

bool PianoController::HasBeats() const
{
	return m_lastBeatMs != 0 && Time::getMillisecondCounter() - m_lastBeatMs < 2500;
}

void PianoController::SetMetronome(bool on)
{
	if (m_genericDevice)
	{
		m_metronome = on;
		m_softMetronome.reset();
		if (on)
		{
			m_softBeat = 0;
			m_softNextMs = 0;
			m_softMetronome = std::make_unique<SoftMetronome>(*this);
			m_softMetronome->startTimer(2);
		}
		NotifyChanged(apMetronome);
		return;
	}
	m_softMetronome.reset();
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Set, Property::Metronome, on ? 1 : 0));
}

void PianoController::SetMetronomeVolume(int volume)
{
	volume = jlimit(0, 127, volume);
	if (m_genericDevice)
	{
		m_metronomeVolume = volume;
		NotifyChanged(apMetronome);
		return;
	}
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Set, Property::MetronomeVolume, volume));
}

void PianoController::SetMetronomeBell(bool on)
{
	if (m_genericDevice)
	{
		m_metronomeBell = on;
		NotifyChanged(apMetronome);
		return;
	}
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Set, Property::MetronomeBell, on ? 1 : 0));
}

void PianoController::SetMetronomeBeat(int numerator, int denominator)
{
	if (m_genericDevice)
	{
		m_metronomeNumerator = numerator;
		m_metronomeDenominator = denominator;
		m_recorder.AddTimeSignature(numerator, denominator);
		NotifyChanged(apMetronome);
		return;
	}
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Set, Property::MetronomeBeat,
		(numerator << 7) + denominator));
}

String PianoController::GetStyleName()
{
	const ScopedLock lock(m_styleLock);
	return m_styleName;
}

void PianoController::SetStyle(const String& path)
{
	if (m_connected && path.isNotEmpty())
	{
		m_pianoConnector->SendPianoMessage(PianoMessage(Action::Set, Property::StyleName, 0, path));
	}
}

PianoController::StyleState PianoController::GetStyleState()
{
	StyleState state;
	if (m_styleNameKnown)
	{
		state.style = GetStyleName();
	}
	state.tempo = m_pianoTempoKnown ? (int)m_pianoTempo : 0;
	state.chordArea = m_styleChordArea;
	state.leftSound = m_styleLeftSound;
	state.splitPoint = m_styleSplitPoint;
	state.hasMixer = true;
	state.volume = GetVolume(chStyle);
	state.pan = GetPan(chStyle);
	state.reverb = GetReverb(chStyle);
	for (int i = 0; i < NumStyleParts; i++)
	{
		const Channel ch = StylePartChannel(i);
		state.parts[i].active = GetActive(ch);
		state.parts[i].volume = GetVolume(ch);
		state.parts[i].pan = GetPan(ch);
		state.parts[i].reverb = GetReverb(ch);
	}
	return state;
}

void PianoController::RestoreStyleState(const StyleState& state)
{
	// the key (a setting of the program) may have been loaded with it
	NotifyChanged(apStyle);
	if (!m_connected)
	{
		return;
	}

	// what does not depend on the style is sent at once
	if (state.chordArea == caLower || state.chordArea == caFull)
	{
		SetStyleChordArea(state.chordArea);
	}
	if (state.leftSound >= 0)
	{
		SetStyleLeftSound(state.leftSound == 1);
	}
	if (state.splitPoint > 0)
	{
		SetStyleSplitPoint(state.splitPoint);
	}

	m_styleRestore = state;
	m_styleRestoreSerial++;
	{
		const ScopedLock lock(m_styleLock);
		m_styleRestoreName = state.style;
	}
	if (m_styleNameKnown)
	{
		StyleRestoreSendStyle();
	}
	else
	{
		// right after connecting: the style of the piano is reported soon; if it is not,
		// the saved style is sent anyway
		m_styleRestoreStage = srWaitName;
		StyleRestoreAfter(3000, srWaitName);
	}
}

// Goes on with the restoring after a delay, if it is still at the given stage (and no
// other restoring was started meanwhile). Called on any thread.
void PianoController::StyleRestoreAfter(int ms, int stage)
{
	std::weak_ptr<bool> alive = m_alive;
	MessageManager::callAsync([this, alive, ms, stage]()
		{
			if (!alive.lock())
			{
				return;
			}
			const int serial = m_styleRestoreSerial;
			auto goOn = [this, alive, stage, serial]()
				{
					if (!alive.lock() || serial != m_styleRestoreSerial || m_styleRestoreStage != stage)
					{
						return;
					}
					if (stage == srWaitName)
					{
						StyleRestoreSendStyle();
					}
					else
					{
						StyleRestoreFinish();
					}
				};
			if (ms > 0)
			{
				Timer::callAfterDelay(ms, goOn);
			}
			else
			{
				goOn();
			}
		});
}

void PianoController::StyleRestoreSendStyle()
{
	const String style = m_styleRestore.style;
	if (style.isNotEmpty() && (!m_styleNameKnown || style != GetStyleName()))
	{
		// the rest follows when the piano reports the style (or a little later, if it
		// does not, e.g. because it does not know the style)
		m_styleRestoreStage = srWaitLoad;
		SetStyle(style);
		StyleRestoreAfter(3000, srWaitLoad);
	}
	else
	{
		StyleRestoreFinish();
	}
}

void PianoController::StyleRestoreFinish()
{
	m_styleRestoreStage = srNone;
	if (!m_connected)
	{
		return;
	}
	const StyleState& state = m_styleRestore;
	if (state.tempo > 0)
	{
		SetStyleTempo(state.tempo);
	}
	if (state.hasMixer)
	{
		SetVolume(chStyle, state.volume);
		SetPan(chStyle, state.pan);
		SetReverb(chStyle, state.reverb);
		for (int i = 0; i < NumStyleParts; i++)
		{
			const Channel ch = StylePartChannel(i);
			SetVolume(ch, state.parts[i].volume);
			SetPan(ch, state.parts[i].pan);
			SetReverb(ch, state.parts[i].reverb);
			SetActive(ch, state.parts[i].active);
		}
	}
}

void PianoController::SetStyleChordArea(int area)
{
	if (m_connected && (area == caLower || area == caFull))
	{
		m_pianoConnector->SendPianoMessage(PianoMessage(Action::Set, Property::StyleChordArea, 0, area));
	}
}

void PianoController::SetStyleLeftSound(bool on)
{
	if (m_connected)
	{
		m_pianoConnector->SendPianoMessage(PianoMessage(Action::Set, Property::StyleLeftSound, 0, on ? 1 : 0));
		// the piano sends no event for it: the new value is asked for
		m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::StyleLeftSound));
	}
}

void PianoController::SetStyleOn(bool on)
{
	if (!m_connected)
	{
		return;
	}
	int parts = 0;
	for (int i = 0; i < NumStyleParts; i++)
	{
		if (m_channels[chStylePart1 + i].active)
		{
			parts |= 1 << i;
		}
	}
	if (!on)
	{
		if (parts != 0)
		{
			m_styleOffParts = parts; // switched on again later
		}
		for (int i = 0; i < NumStyleParts; i++)
		{
			if (parts & (1 << i))
			{
				m_pianoConnector->SendPianoMessage(PianoMessage(Action::Set, Property::Active, chStylePart1 + i, 0));
			}
		}
	}
	else if (parts == 0)
	{
		const int wanted = m_styleOffParts != 0 ? m_styleOffParts : (1 << NumStyleParts) - 1;
		for (int i = 0; i < NumStyleParts; i++)
		{
			if (wanted & (1 << i))
			{
				m_pianoConnector->SendPianoMessage(PianoMessage(Action::Set, Property::Active, chStylePart1 + i, 1));
			}
		}
	}
}

void PianoController::ForgetStyleOffParts()
{
	m_styleOffParts = 0;
	for (int& parts : m_styleGroupOffParts)
	{
		parts = 0;
	}
}

int PianoController::StyleGroupParts(int group)
{
	// bit 0: Rhythm 1 ... bit 7: Phrase 2
	return group == sgRhythm ? 0x03 : group == sgBass ? 0x04 : group == sgOthers ? 0xf8 : 0;
}

bool PianoController::GetStyleGroupOn(int group)
{
	const int groupParts = StyleGroupParts(group);
	for (int i = 0; i < NumStyleParts; i++)
	{
		if ((groupParts & (1 << i)) && m_channels[chStylePart1 + i].active)
		{
			return true;
		}
	}
	return false;
}

void PianoController::SetStyleGroupOn(int group, bool on)
{
	if (!m_connected || group < 0 || group >= NumStyleGroups)
	{
		return;
	}
	const int groupParts = StyleGroupParts(group);
	int parts = 0; // the parts of the group that are on
	for (int i = 0; i < NumStyleParts; i++)
	{
		if ((groupParts & (1 << i)) && m_channels[chStylePart1 + i].active)
		{
			parts |= 1 << i;
		}
	}
	if (!on)
	{
		if (parts != 0)
		{
			m_styleGroupOffParts[group] = parts; // switched on again later
		}
		for (int i = 0; i < NumStyleParts; i++)
		{
			if (parts & (1 << i))
			{
				m_pianoConnector->SendPianoMessage(PianoMessage(Action::Set, Property::Active, chStylePart1 + i, 0));
			}
		}
	}
	else if (parts == 0)
	{
		const int wanted = m_styleGroupOffParts[group] != 0 ? m_styleGroupOffParts[group] : groupParts;
		for (int i = 0; i < NumStyleParts; i++)
		{
			if (wanted & (1 << i))
			{
				m_pianoConnector->SendPianoMessage(PianoMessage(Action::Set, Property::Active, chStylePart1 + i, 1));
			}
		}
	}
}

// Called when a part of the accompaniment was switched (from any thread).
void PianoController::UpdateStyleOn()
{
	bool on = false;
	for (int i = 0; i < NumStyleParts; i++)
	{
		on = on || m_channels[chStylePart1 + i].active;
	}
	if (m_channels[chStyle].active != on)
	{
		m_channels[chStyle].active = on;
		NotifyChanged(apActive, chStyle);
	}
}

int PianoController::StyleDefaultIndex(Channel ch)
{
	return ch == chStyle ? NumStyleParts :
		ch >= chStylePart1 && ch < chStylePart1 + NumStyleParts ? ch - chStylePart1 : -1;
}

// which: 0 volume, 1 pan, 2 reverb. Called when the piano reports the value.
void PianoController::NoteStyleDefault(Channel ch, int which, int value)
{
	const int index = StyleDefaultIndex(ch);
	if (index >= 0 && m_styleDefaultPending[index][which].exchange(false))
	{
		m_styleDefaults[index][which] = value;
	}
}

bool PianoController::ResetStyleValue(Channel ch, Aspect aspect)
{
	const int index = StyleDefaultIndex(ch);
	const int which = aspect == apVolume ? 0 : aspect == apPan ? 1 : aspect == apReverb ? 2 : -1;
	if (index < 0 || which < 0 || !m_connected)
	{
		return false;
	}
	int value = m_styleDefaults[index][which];
	if (ch == chStyle && which == 0)
	{
		value = DefaultVolume; // of the whole accompaniment: always 100
	}
	else if (ch == chStyle && which == 1)
	{
		value = DefaultPan;
	}
	if (value == NoStyleDefault)
	{
		return false;
	}
	if (which == 0)
	{
		SetVolume(ch, value);
	}
	else if (which == 1)
	{
		SetPan(ch, value);
	}
	else
	{
		SetReverb(ch, value);
	}
	return true;
}

const char* PianoController::StylePartName(int part)
{
	static const char* const names[NumStyleParts] = {
		"Rhythm 1", "Rhythm 2", "Bass", "Chord 1", "Chord 2", "Pad", "Phrase 1", "Phrase 2"};
	return part >= 0 && part < NumStyleParts ? names[part] : "";
}

void PianoController::QueryStyleParts(bool defaults)
{
	if (!m_pianoConnector)
	{
		return;
	}
	// asked for when connecting and after a style was loaded: the answers are the values
	// the style came with (the defaults of ResetStyleValue)
	for (int i = 0; defaults && i <= NumStyleParts; i++)
	{
		for (int which = 0; which < 3; which++)
		{
			// the whole accompaniment keeps its settings when the style changes: only once
			if (i < NumStyleParts || m_styleDefaults[i][which] == NoStyleDefault)
			{
				m_styleDefaultPending[i][which] = true;
			}
		}
	}
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::Volume, chStyle, 0));
	for (int i = 0; i < NumStyleParts; i++)
	{
		const int ch = chStylePart1 + i;
		m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::Active, ch, 0));
		m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::Volume, ch, 0));
		m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::Pan, ch, 0));
		m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::Reverb, ch, 0));
		m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::VoiceMidi, ch, 0));
	}
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::Pan, chStyle, 0));
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::Reverb, chStyle, 0));
}

void PianoController::SetStylePlaying(bool playing)
{
	if (m_connected)
	{
		m_pianoConnector->SendPianoMessage(PianoMessage(Action::Set, Property::StylePlay, 0, playing ? 1 : 0));
	}
}

// With Sync Start on, the piano does not load a song into its player, and the notes of a
// song played by ConPianist would start the accompaniment. So Sync Start is switched off
// while a song is loaded or played, and on again afterwards.
void PianoController::SuspendStyleSyncStart()
{
	if (m_connected && !IsMidiDevicePlayback() && (m_styleSyncStart || m_styleSyncWanted))
	{
		m_styleSyncSuspended = true;
		if (m_styleSyncStart)
		{
			m_pianoConnector->SendPianoMessage(PianoMessage(Action::Set, Property::StyleSyncStart, 0, 0));
		}
	}
}

// Called from any thread, when a song was loaded or the playback has stopped.
void PianoController::ResumeStyleSyncStartLater()
{
	if (!m_styleSyncSuspended)
	{
		return;
	}
	std::weak_ptr<bool> alive = m_alive;
	MessageManager::callAsync([this, alive]()
		{
			Timer::callAfterDelay(2000, [this, alive]()
				{
					if (!alive.lock() || !m_styleSyncSuspended || GetPlaying())
					{
						return; // playing (again): the next stop brings Sync Start back
					}
					m_styleSyncSuspended = false;
					if (m_connected && m_styleSyncWanted && !m_stylePlaying && !m_styleSyncStart)
					{
						m_pianoConnector->SendPianoMessage(PianoMessage(Action::Set, Property::StyleSyncStart, 0, 1));
					}
				});
		});
}

void PianoController::SetStyleSyncStart(bool on)
{
	m_styleSyncWanted = on;
	m_styleSyncSuspended = false; // set by the user: as it was asked for
	if (m_connected)
	{
		m_pianoConnector->SendPianoMessage(PianoMessage(Action::Set, Property::StyleSyncStart, 0, on ? 1 : 0));
	}
}

// The piano rejects setting the section property; the section is changed with the
// Section Control system exclusive message of the Yamaha style format, as an arranger
// keyboard connected to the piano does it.
void PianoController::SendStyleSection(int section, bool on)
{
	if (m_connected && section >= 0 && section < 0x7f)
	{
		const uint8 data[] = {0x43, 0x7e, 0x00, (uint8)section, (uint8)(on ? 0x7f : 0x00)};
		m_pianoConnector->SendMidiMessage(MidiMessage::createSysExMessage(data, sizeof(data)));
	}
}

void PianoController::SetStyleSection(int section)
{
	m_styleFillTarget = ssNone; // chosen by hand: nothing is waiting for a fill in
	SendStyleSection(section, true);
}

// A section that stays switched on is repeated (a fill in or the break again and again,
// like holding its button on an arranger keyboard): it is switched off a moment later,
// so it is played once. Called on the message thread.
void PianoController::PlayStyleSection(int section)
{
	static const int ReleaseMs = 150;
	SendStyleSection(section, true);
	std::weak_ptr<bool> alive = m_alive;
	Timer::callAfterDelay(ReleaseMs, [this, alive, section]()
		{
			if (alive.lock())
			{
				SendStyleSection(section, false);
			}
		});
}

// The new main section is chosen first (it becomes the next section), then its fill in
// is played once: a main section chosen while the fill in is playing would cut the fill
// in. If the piano goes back to the old main section after the fill in, the new one is
// chosen again then (see StyleSection in IncomingPianoMessage). Called on the message
// thread.
void PianoController::ChangeStyleMainWithFill(int main)
{
	if (main < ssMainA || main >= ssMainA + 4)
	{
		return;
	}
	static const int FillDelayMs = 60;
	m_styleFillTarget = main;
	m_styleFillTargetMs = Time::getMillisecondCounter();
	SendStyleSection(main, true);
	std::weak_ptr<bool> alive = m_alive;
	Timer::callAfterDelay(FillDelayMs, [this, alive, main]()
		{
			if (alive.lock() && m_styleFillTarget == main)
			{
				PlayStyleSection(ssFillInAA + (main - ssMainA));
			}
		});
}

PianoController::Position PianoController::GetStylePosition() const
{
	const int value = m_stylePosition;
	return {value >> 16, value & 0xffff};
}

PianoController::StyleChord PianoController::GetStyleChord() const
{
	const int value = m_styleChord;
	StyleChord chord;
	chord.root = (value >> 24) & 0x7f;
	chord.type = (value >> 16) & 0x7f;
	chord.bassRoot = (value >> 8) & 0x7f;
	chord.bassType = value & 0x7f;
	return chord;
}

void PianoController::SetStyleTempo(int tempo)
{
	// one tempo: the song player (of the piano or of ConPianist) and the accompaniment
	SetTempo(jlimit((int)MinTempo, (int)MaxTempo, tempo));
}

void PianoController::StartRecordingWithCountIn(int measures)
{
	UpdateRecorderSetups();
	m_recorder.StartCountIn(measures);
}

void PianoController::StartRecording(bool autoStart)
{
	UpdateRecorderSetups();
	if (autoStart)
	{
		m_recorder.Arm();
	}
	else
	{
		m_recorder.Start();
	}
}

void PianoController::StopRecording()
{
	m_recorder.Stop();
}

bool PianoController::SaveRecording(const File& file, bool includeStyle, String& error, bool keep,
	int quantizeTicks, bool quantizeEnds, int tripletTicks, bool fillGaps)
{
	// the reverb type of the piano as XG reverb type; not known on a general MIDI device
	const int reverbType = m_genericDevice || m_reverbEffect <= 0 ? LiveRecorder::NoValue : m_reverbEffect;
	if (includeStyle)
	{
		UpdateRecorderStyleSetups();
	}
	const bool ok = m_recorder.Save(file, m_recordedTempo, m_recordedNumerator, m_recordedDenominator,
		includeStyle, reverbType, error, quantizeTicks, quantizeEnds, tripletTicks, fillGaps);
	if (ok && keep)
	{
		m_recorder.MarkSaved();
	}
	return ok;
}

void PianoController::ReleaseLive()
{
	const ScopedLock lock(m_liveLock);

	// every channel with a held note or pedal, also one that is no longer used
	int held = m_liveSustainChannels;
	for (int note = 0; note < 128; note++)
	{
		held |= m_liveNoteChannels[note];
	}
	if (held == 0)
	{
		return;
	}

	m_liveSendNow = true;
	ReleaseLiveChannels(held);
	m_liveSendNow = false;
	m_liveSustainValue = 0;
}

// Sets the channels used for Live Play (with m_liveLock held). Held notes and the pedal
// are released on the channels that are no longer used; a sustain pedal still held down
// goes on with the new channels, so it need not be pressed again.
void PianoController::ApplyLiveChannels(int channelMask)
{
	const int removed = m_liveChannels & ~channelMask;
	const int added = channelMask & ~m_liveChannels;
	m_liveChannels = channelMask;

	if (removed != 0)
	{
		ReleaseLiveChannels(removed);
	}

	if (added != 0)
	{
		PressLivePedal(added);
	}
}

// Releases the held notes and the pedal of Live Play on the current output, but keeps
// the state of the sustain pedal, so that it can go on with the new output.
void PianoController::SuspendLive()
{
	const ScopedLock lock(m_liveLock);
	int held = m_liveSustainChannels;
	for (int note = 0; note < 128; note++)
	{
		held |= m_liveNoteChannels[note];
	}
	if (held != 0)
	{
		ReleaseLiveChannels(held);
		(m_genericDevice ? m_liveSuspendedDevice : m_liveSuspendedPiano) |= held;
	}
}

// Sends the sustain pedal still held down on the Live Play channels again, after the
// output has changed or the song has been loaded (the player releases the pedal on the
// channels of the song when it loads, stops or jumps).
void PianoController::ResumeLivePedal()
{
	const ScopedLock lock(m_liveLock);
	PressLivePedal(m_liveChannels);
	m_liveSuspendedPiano = 0;
	m_liveSuspendedDevice = 0;
}

// Sends the sustain pedal held down on the given Live Play channels (with m_liveLock
// held). On a channel where the pedal was just released on the same output (e.g. the
// same piano channel after a network <-> USB switch), the old notes would still be
// sounding and would be held again, so they are silenced first (All Sound Off).
void PianoController::PressLivePedal(int channelMask)
{
	if (m_liveSustainValue <= 0)
	{
		return;
	}
	const int sameOutput = channelMask &
		(m_genericDevice ? m_liveSuspendedDevice : m_liveSuspendedPiano);
	for (int ch = 1; ch <= 17; ch++)
	{
		if (channelMask & (1 << (ch - 1)))
		{
			if (sameOutput & (1 << (ch - 1)))
			{
				SendLive(ch, MidiMessage::allSoundOff(1));
			}
			SendLive(ch, MidiMessage::controllerEvent(1, 64, m_liveSustainValue));
		}
	}
	m_liveSustainChannels |= channelMask;
}

// Releases the held notes and the pedal on the given channels (with m_liveLock held).
void PianoController::ReleaseLiveChannels(int removed)
{
	for (int note = 0; note < 128; note++)
	{
		const int channels = m_liveNoteChannels[note] & removed;
		for (int ch = 1; ch <= 17; ch++)
		{
			const int sounding = ch == 10 ? note : note + m_liveNoteTranspose[note] + LiveOctaveShift(ch);
			if ((channels & (1 << (ch - 1))) && sounding >= 0 && sounding <= 127)
			{
				SendLive(ch, MidiMessage::noteOff(1, sounding));
			}
		}
		m_liveNoteChannels[note] &= ~removed;
	}
	for (int ch = 1; ch <= 17; ch++)
	{
		if (m_liveSustainChannels & removed & (1 << (ch - 1)))
		{
			SendLive(ch, MidiMessage::controllerEvent(1, 64, 0));
		}
	}
	m_liveSustainChannels &= ~removed;
}

void PianoController::PlayLive(const MidiMessage& message)
{
	const bool isNote = message.isNoteOnOrOff();
	if (!isNote && !message.isController() && !message.isPitchWheel() &&
		!message.isChannelPressure() && !message.isAftertouch())
	{
		// e.g. program changes (would change the voices), system messages, clock
		return;
	}

	const ScopedLock lock(m_liveLock);
	int channels = m_liveChannels;

	// the notes are transposed like the song (not on the drum channel); a note is
	// released with the transposition it was started with
	const bool hasNote = isNote || message.isAftertouch();
	const int note = hasNote ? message.getNoteNumber() : 0;
	int transpose = m_transpose;

	if (isNote)
	{
		if (message.isNoteOn())
		{
			if (m_liveNoteChannels[note] != 0 && m_liveNoteTranspose[note] != transpose)
			{
				transpose = m_liveNoteTranspose[note]; // same key again while held
			}
			m_liveNoteChannels[note] |= channels;
			m_liveNoteTranspose[note] = transpose;
		}
		else
		{
			// released where it was started, even if the channels have changed since
			channels = m_liveNoteChannels[note];
			transpose = m_liveNoteTranspose[note];
			m_liveNoteChannels[note] = 0;
		}
	}
	else if (message.isAftertouch() && m_liveNoteChannels[note] != 0)
	{
		transpose = m_liveNoteTranspose[note];
	}
	else if (message.isControllerOfType(64))
	{
		// any value above 0 counts as down (half pedal too): released on a switch
		m_liveSustainValue = message.getControllerValue();
		if (m_liveSustainValue > 0)
			m_liveSustainChannels |= channels;
		else
			m_liveSustainChannels &= ~channels;
	}

	for (int ch = 1; ch <= 17; ch++)
	{
		if (channels & (1 << (ch - 1)))
		{
			MidiMessage copy(message);
			// not on the drum channel; on the piano's keyboard parts (17) the piano adds
			// its own keyboard transpose (Piano Room) to this
			if (hasNote && ch != 10)
			{
				const int sounding = note + transpose + LiveOctaveShift(ch);
				if (sounding < 0 || sounding > 127)
				{
					continue;
				}
				copy.setNoteNumber(sounding);
			}
			SendLive(ch, copy);
		}
	}
}

void PianoController::SetPlaybackAvailability(bool network, bool local, bool midiDevice)
{
	m_networkPlaybackAvailable = network;
	m_localPlaybackAvailable = local;
	m_midiDevicePlaybackAvailable = midiDevice;
	NotifyChanged(apPlaybackSource);
}

void PianoController::SetPlaybackSource(PlaybackSource source, bool automatic)
{
	if (m_switchPending)
	{
		if (source == m_pendingSource)
		{
			return; // on its way: switched when the piano is at the beginning of the measure
		}
		// another player was chosen meanwhile: the delayed switch is cancelled (otherwise it
		// would switch to the player chosen first after this one)
		m_switchPending = false;
		m_switchRequest++;
	}

	const bool available = source == psPiano ? m_networkPlaybackAvailable :
		source == psLocal ? m_localPlaybackAvailable : m_midiDevicePlaybackAvailable;
	if (source == m_playbackSource || !available)
	{
		if (source == m_playbackSource)
		{
			m_playbackSourceAutomatic = automatic;
		}
		// nothing to do; lets the UI show the actual state again
		NotifyChanged(apPlaybackSource);
		return;
	}

	// the loaded song is loaded again into the other player, at the beginning of the same
	// measure
	const String songName = IsSongLoaded() ? GetSongName() : String();
	const int measure = GetPosition().measure;

	if (!m_localPlayback && source != psPiano && m_networkTempoSet && m_connected &&
		songName.isNotEmpty() && !m_switchAtMeasureStart)
	{
		// the tempo was set on the piano's player: the piano goes to the beginning of the
		// measure first, its tempo there is compared with the file's (after its report)
		m_switchPending = true;
		m_pendingSource = source;
		const int request = ++m_switchRequest;
		Stop();
		SetPosition({jmax(1, measure), 1});
		std::weak_ptr<bool> alive = m_alive;
		Timer::callAfterDelay(600, [this, alive, source, automatic, request]()
			{
				if (alive.lock() && m_switchPending && request == m_switchRequest)
				{
					m_switchPending = false;
					m_switchAtMeasureStart = true;
					SetPlaybackSource(source, automatic);
					m_switchAtMeasureStart = false;
				}
			});
		return;
	}

	if (!m_localPlayback)
	{
		Stop(); // the piano's own player
	}

	const PlaybackSource previousSource = m_playbackSource;
	// the song file; for one of the piano's own songs its MIDI file in its folder
	File songFile;
	if (songName.isNotEmpty())
	{
		if (File::isAbsolutePath(songName) && File(songName).existsAsFile())
		{
			songFile = File(songName);
		}
		else if (findSongFile)
		{
			songFile = findSongFile(songName);
		}
	}
	const bool reload = songFile.existsAsFile();
	// to the piano's player: the MIDI file of one of its own songs loads that song
	const String pianoSong = reload && source == psPiano && findPianoSong ? findPianoSong(songFile) : String();
	if (reload)
	{
		// the settings of the Mixer and the Playback panel are kept
		m_pendingSnapshot = TakeSnapshot();
		if (previousSource == psPiano && !m_networkTempoSet)
		{
			// the tempo was not set on the piano's player: the song keeps its own tempo
			m_pendingSnapshot.speedFactor = 1.0;
		}
	}

	m_playbackSourceAutomatic = automatic;
	ApplyPlaybackSource(source);
	NotifyChanged(apPlaybackSource);

	if (reload)
	{
		m_pendingMeasure = measure;
		m_skipRegistrationMemory = true;
		// (the measure and the settings: when the piano reports the song, see SongName)
		const bool loaded = pianoSong.isNotEmpty() ? LoadPresetSong(pianoSong) : LoadSongInternal(songFile);
		if (!loaded)
		{
			m_pendingSnapshot.valid = false;
			m_skipRegistrationMemory = false;
		}
	}

	if (m_connected && (!m_localPlayback || previousSource == psMidiDevice))
	{
		// the piano's own player (or its mixer) is used again: read its song, parts
		// and channels.
		// This is done after the upload, so that the answers cannot be mistaken
		// for the confirmation of the new song.
		ResyncStateFromPiano();
	}

	RestoreLiveChannels();
	ResumeLivePedal();
}

// Called for every message sent by the local player (with the player's lock held).
void PianoController::OnLocalMessage(const MidiMessage& message)
{
	ShowLocalNote(message);

	if (m_genericDevice && (message.isProgramChange() ||
		(message.isController() && (message.getControllerNumber() == 0 || message.getControllerNumber() == 32))))
	{
		// keep track of the voices of the song, to show their names in the mixer
		const int index = message.getChannel() - 1;
		const Channel ch = (Channel)(chMidi0 + message.getChannel());
		if (message.isProgramChange())
		{
			SetVoiceOf(ch, String((m_genericBank[index] << 8) | message.getProgramChangeNumber()));
			NotifyChangedLater(apVoice, ch); // the player's lock is held: see NotifyChangedLater
		}
		else if (message.getControllerNumber() == 0)
		{
			m_genericBank[index] = (m_genericBank[index] & 0x7f) | (message.getControllerValue() << 8);
		}
		else
		{
			m_genericBank[index] = (m_genericBank[index] & 0x7f00) | message.getControllerValue();
		}
	}
}

// Shows the notes of the right and left hand parts played by the local player on the
// virtual keyboard (the same way as the notes coming from the piano). Called by the
// player, always with the player's lock held, so m_shownNotes needs no extra lock.
void PianoController::ShowLocalNote(const MidiMessage& message)
{
	if (!message.isNoteOnOrOff())
	{
		return;
	}

	const int channel = message.getChannel(); // 1..16
	const int note = message.getNoteNumber();
	bool& shown = m_shownNotes[channel - 1][note];

	if (message.isNoteOn())
	{
		const Channel ch = (Channel)(chMidi0 + channel);
		const bool handPart = channel != 10 && // drums: the keys are drum sounds
			(ch == m_partChannels[paRight] || ch == m_partChannels[paLeft]);
		if (handPart)
		{
			shown = true;
			NotifyNoteMessageLater(message);
		}
	}
	else if (shown)
	{
		// released even if the part assignment has changed in the meantime
		shown = false;
		NotifyNoteMessageLater(message);
	}
}

// Loads the current song again into the own player (at the same measure), unless it
// is playing.
void PianoController::ReloadSong()
{
	if (!IsLocalSongLoaded() || GetPlaying())
	{
		return;
	}

	const File file(GetSongName());
	if (file.existsAsFile())
	{
		m_pendingSnapshot = TakeSnapshot();
		m_pendingMeasure = GetPosition().measure;
		m_skipRegistrationMemory = true;
		if (!LoadSongInternal(file))
		{
			m_pendingSnapshot.valid = false;
			m_skipRegistrationMemory = false;
		}
	}
}

bool PianoController::LoadSong(const File& file)
{
	// a song loaded by the user starts with its own settings (and registration memory)
	m_pendingMeasure = 0;
	m_pendingSnapshot.valid = false;
	m_skipRegistrationMemory = false;
	ClearConvertedVoices();
	return LoadSongInternal(file);
}

PianoController::MixSnapshot PianoController::TakeSnapshot()
{
	MixSnapshot snapshot;
	snapshot.valid = true;
	snapshot.source = m_playbackSource;
	snapshot.speedFactor = IsLocalSongLoaded() ? m_speedFactor : 0.0;
	for (Channel ch : MidiChannels)
	{
		const ChannelInfo& info = m_channels[ch];
		String voice = VoiceOf(ch);
		if (voice.startsWith("PRESET:"))
		{
			Voice* preset = Presets::FindVoice(voice);
			voice = preset ? String(preset->num) : String();
		}
		snapshot.channels[ch - chMidi1] = {info.enabled, info.active, info.volume, info.pan, info.reverb, voice};
	}
	snapshot.masterVolume = m_channels[chMidiMaster].volume;
	snapshot.masterActive = m_channels[chMidiMaster].active;
	for (int i = 0; i < 3; i++) snapshot.parts[i] = m_parts[i];
	for (int i = 0; i < 2; i++) snapshot.partChannels[i] = m_partChannels[i];
	snapshot.tempo = m_tempo;
	snapshot.transpose = m_transpose;
	snapshot.loop = m_loop;
	return snapshot;
}

// Restores the settings from before the player was switched, sent to the new player.
// The voices are converted between Yamaha and General MIDI voices when switching
// between the piano and a MIDI device.
void PianoController::ApplySnapshot(const MixSnapshot& snapshot)
{
	const bool toDevice = m_playbackSource == psMidiDevice;
	const bool fromDevice = snapshot.source == psMidiDevice;

	for (Channel ch : MidiChannels)
	{
		// the same song: the channels used in it are known from before the switch
		// (with the piano's own player they are reported only a little later)
		const MixSnapshot::ChannelState& state = snapshot.channels[ch - chMidi1];
		if (!state.enabled)
		{
			continue; // not used in the song
		}
		SetActive(ch, state.active);
		SetVolume(ch, state.volume);
		SetPan(ch, state.pan);
		SetReverb(ch, state.reverb);

		if (state.voice.isNotEmpty())
		{
			SetSongChannelVoice(ch, ConvertSongChannelVoice(ch, state.voice.getIntValue(), fromDevice, toDevice));
		}
	}

	SetVolume(chMidiMaster, snapshot.masterVolume);
	if (m_localPlayback)
	{
		SetActive(chMidiMaster, snapshot.masterActive);
	}
	SetPartChannel(paRight, snapshot.partChannels[paRight]);
	SetPartChannel(paLeft, snapshot.partChannels[paLeft]);
	for (int i = 0; i < 3; i++)
	{
		SetPart((Part)i, snapshot.parts[i]);
	}
	if (snapshot.speedFactor > 0 && m_localPlayback)
	{
		SetSpeedFactor(snapshot.speedFactor); // own player to own player: the same speed
	}
	else if (snapshot.speedFactor <= 0 && IsLocalSongLoaded())
	{
		// from the piano's player to the own one (the tempo was set there): the speed from
		// the piano's tempo and the file's at the beginning of the measure, both in whole
		// numbers; one unit is the rounding of the piano's tempo, the song keeps its own
		// tempo then
		const int fileTempo = roundToInt(m_localPlayer->GetFileTempo());
		double speed = fileTempo > 0 ? snapshot.tempo / (double)fileTempo : 1.0;
		if (std::abs(snapshot.tempo - fileTempo) <= 1)
		{
			speed = 1.0;
		}
		SetSpeedFactor(speed);
	}
	else if (snapshot.speedFactor <= 0)
	{
		SetTempo(snapshot.tempo);
	}
	// (own player to the piano's: the speed is set when the piano reports its tempo at the
	// measure, see SongName)
	SetTranspose(snapshot.transpose);
	if (snapshot.loop.begin.measure > 0)
	{
		SetLoop(snapshot.loop);
	}

	RestoreLiveChannels();
}

bool PianoController::LoadSongInternal(const File& file)
{
	// the player releases the pedal on the channels of the song: the Live Play notes
	// are released, and the pedal held down is sent again afterwards (ResumeLivePedal)
	SuspendLive();
	// the piano does not load a song while Sync Start is on
	SuspendStyleSyncStart();

	bool ok = false;
	if (m_localPlayback)
	{
		ok = LoadLocalSong(file);
	}
	else
	{
		ok = UploadSong(file);
		if (!ok && m_localPlaybackAvailable)
		{
			// the piano cannot be reached over the network: continue with ConPianist's
			// own player until the next start (or until the connection settings change)
			Logger::writeToLog("Song upload failed, switching to local playback");
			m_networkPlaybackAvailable = false;
			ApplyPlaybackSource(psLocal);
			NotifyChanged(apPlaybackSource);
			if (onNetworkPlaybackFailed)
			{
				onNetworkPlaybackFailed();
			}
			ok = LoadLocalSong(file);
		}
	}

	// first the measure, then the settings: the tempo of the snapshot belongs to that
	// measure (the speed of the own player is calculated from the tempo of the song there)
	if (ok && m_localPlayback && m_pendingMeasure > 1)
	{
		SetPosition({m_pendingMeasure, 1});
	}
	if (ok && m_localPlayback && m_pendingSnapshot.valid)
	{
		ApplySnapshot(m_pendingSnapshot);
		m_pendingSnapshot.valid = false;
	}
	if (!ok || m_localPlayback)
	{
		m_pendingMeasure = 0;
	}
	ResumeLivePedal(); // also if the song could not be loaded
	ResumeStyleSyncStartLater(); // on again, if the song is not played
	return ok;
}

// Stops the local player before the connectors are destroyed (on application exit).
void PianoController::ShutdownLocalPlayer()
{
	m_softMetronome.reset();
	if (m_localPlayer)
	{
		m_localPlayer->Unload();
		m_localPlayer.reset();
	}
	m_localPlayback = false;
}

bool PianoController::LoadLocalSong(const File& file)
{
	ClearSentSongVoices(); // the new song sets its own voices
	if (m_genericDevice)
	{
		// General MIDI default voice (program 0) until the song selects another one
		for (Channel ch : MidiChannels)
		{
			m_genericBank[ch - chMidi1] = 0;
			SetVoiceOf(ch, "0");
		}
	}

	if (!m_localPlayer || !m_localPlayer->Load(file))
	{
		return false;
	}

	SetSongNameValue(file.getFullPathName());
	m_songLoaded = true;
	m_loop = {{0,0},{0,0}};
	m_loopStart = {0,0};
	// a new song starts with its own tempo (speed 100%)
	m_speedFactor = 1.0;
	m_localPlayer->SetSpeed(1.0);
	m_tempo = jlimit((int)MinTempo, (int)MaxTempo, roundToInt(m_localPlayer->GetFileTempo()));
	m_localPlayer->SetTranspose(m_transpose);

	const std::vector<int> usedChannels = m_localPlayer->GetUsedChannels();
	for (Channel ch : MidiChannels)
	{
		const int midiChannel = ch - chMidi0;
		m_channels[ch].enabled = std::find(usedChannels.begin(), usedChannels.end(), midiChannel) != usedChannels.end();
		NotifyChanged(apEnable, ch);
	}
	m_channels[chMidiMaster].enabled = true;
	m_channels[chMidiMaster].active = true;
	NotifyChanged(apEnable, chMidiMaster);
	NotifyChanged(apActive, chMidiMaster);

	// a new song starts with all channels and parts switched on, like on the piano;
	// the registration memory of the song (if any) is applied after this
	ResetLocalMixState();

	if (m_genericDevice)
	{
		InitGenericMixer();
	}

	// the Live Play channels not used in the song keep their own settings
	RestoreLiveChannels();

	NotifyChanged(apSongName);
	NotifyChanged(apLength);
	NotifyChanged(apPosition);
	NotifyChanged(apPlayback);
	NotifyChanged(apTempo);
	NotifyChanged(apTranspose);
	NotifyChanged(apLoop);
	NotifyChanged(apSongLoaded);
	return true;
}

// Value of a controller in the setup part of the loaded song (general MIDI devices).
int PianoController::GenericSetupValue(Channel ch, int controller, int defaultValue)
{
	const int value = m_localPlayer && IsSongChannel(ch) ?
		m_localPlayer->GetSetupController(ch - chMidi0, controller) : -1;
	return value >= 0 ? value : defaultValue;
}

// The mixer of a general MIDI device starts with the values of the song.
void PianoController::InitGenericMixer()
{
	for (Channel ch : MidiChannels)
	{
		m_channels[ch].volume = GenericSetupValue(ch, 7, DefaultVolume);
		m_channels[ch].pan = GenericSetupValue(ch, 10, PanBase) - PanBase;
		m_channels[ch].reverb = GenericSetupValue(ch, 91, GenericDefaultReverb);
		NotifyChanged(apVolume, ch);
		NotifyChanged(apPan, ch);
		NotifyChanged(apReverb, ch);
		NotifyChanged(apVoice, ch);
	}
	m_localPlayer->SetMasterVolumeScale(m_channels[chMidiMaster].volume / double(DefaultVolume));
	NotifyChanged(apVolume, chMidiMaster);
}

void PianoController::ClearSongState()
{
	m_songLoaded = false;
	m_songLoading = false;
	SetSongNameValue(String());
	m_playing = false;
	m_speedFactor = 1.0;
	m_position = {0,0};
	m_length = {0,0};
	m_loop = {{0,0},{0,0}};
	m_loopStart = {0,0};

	for (Channel ch : MidiChannels)
	{
		m_channels[ch].enabled = false;
		NotifyChanged(apEnable, ch);
	}
	m_channels[chMidiMaster].enabled = false;
	m_channels[chMidiMaster].active = false;
	NotifyChanged(apEnable, chMidiMaster);

	NotifyChanged(apSongName);
	NotifyChanged(apLength);
	NotifyChanged(apPosition);
	NotifyChanged(apPlayback);
	NotifyChanged(apLoop);
}

// Switches all song channels and parts on (local playback only). With automatic part
// selection, channel 1 is the right hand and channel 2 the left hand, as on the piano.
void PianoController::ResetLocalMixState()
{
	for (Channel ch : MidiChannels)
	{
		m_channels[ch].active = true;
		NotifyChanged(apActive, ch);
	}

	for (bool& part : m_parts)
	{
		part = true;
	}
	NotifyChanged(apPart);

	if (m_partAuto)
	{
		const Channel autoChannels[2] = {chMidi1, chMidi2};
		for (int part = paRight; part <= paLeft; part++)
		{
			const Channel oldCh = m_partChannels[part];
			m_partChannels[part] = autoChannels[part];
			NotifyChanged(apPartChannel, oldCh);
			NotifyChanged(apPartChannel, autoChannels[part]);
		}
	}

	UpdateLocalMutes();
}

// Tells the local player which song channels must be silent: a channel plays only if
// the song (Balance), the channel itself (Mixer) and its part (right/left/backing) are on.
void PianoController::UpdateLocalMutes()
{
	if (!m_localPlayer)
	{
		return;
	}

	const bool songActive = m_channels[chMidiMaster].active;
	for (Channel ch : MidiChannels)
	{
		const Part part = m_partChannels[paRight] == ch ? paRight :
			m_partChannels[paLeft] == ch ? paLeft : paBacking;
		const bool audible = songActive && m_channels[ch].active && m_parts[part];
		m_localPlayer->SetChannelMuted(ch - chMidi0, !audible);
	}
}

bool PianoController::IsSongLoaded()
{
	return m_localPlayback ? m_songLoaded && IsLocalSongLoaded() : m_songLoaded;
}

bool PianoController::GetPlaying()
{
	return m_localPlayback ? IsLocalSongLoaded() && m_localPlayer->IsPlaying() : m_playing;
}

PianoController::Position PianoController::GetPosition()
{
	if (IsLocalSongLoaded())
	{
		const LocalSongPlayer::Position position = m_localPlayer->GetPosition();
		return {position.measure, position.beat};
	}
	return m_position;
}

PianoController::Position PianoController::GetLength()
{
	if (IsLocalSongLoaded())
	{
		const LocalSongPlayer::Position length = m_localPlayer->GetLength();
		return {length.measure, length.beat};
	}
	return m_length;
}

void PianoController::AddListener(Listener* listener)
{
	m_listeners.add(listener);
}

void PianoController::RemoveListener(Listener* listener)
{
	m_listeners.remove(listener);
}

void PianoController::NotifyChanged(Aspect aspect, Channel channel)
{
	m_listeners.call([aspect, channel](Listener& listener) { listener.PianoStateChanged(aspect, channel); });
}

void PianoController::NotifyNoteMessage(const MidiMessage& message)
{
	m_listeners.call([&message](Listener& listener) { listener.PianoNoteMessage(message); });
}

// After the piano's player loaded the song again (switching the player): the measure where
// it was and the settings. The piano reports the name of the song before it has finished
// loading it; until then it reports the length as 0 and the song counts as not loaded, and
// a jump would be lost. So the length is waited for, at most about 5 seconds.
void PianoController::RestorePendingSongState(int measure, int attempt)
{
	std::weak_ptr<bool> alive = m_alive;
	if (!m_localPlayback && !m_songLoaded && attempt < 18)
	{
		Timer::callAfterDelay(250, [this, alive, measure, attempt]()
			{
				if (alive.lock())
				{
					RestorePendingSongState(measure, attempt + 1);
				}
			});
		return;
	}

	double speed = 0.0;
	if (!m_localPlayback && m_songLoaded)
	{
		if (m_pendingSnapshot.valid)
		{
			ApplySnapshot(m_pendingSnapshot);
			speed = m_pendingSnapshot.speedFactor;
		}
		if (measure > 1)
		{
			SetPosition({measure, 1});
		}
	}
	else if (!m_localPlayback)
	{
		Logger::writeToLog("The piano has not finished loading the song: its measure and settings are not restored");
	}
	m_pendingSnapshot.valid = false;
	if (speed > 0 && std::abs(speed - 1.0) > 0.001)
	{
		// from the own player: the same speed, from the piano's own tempo at the measure
		// (reported after the jump)
		Timer::callAfterDelay(500, [this, alive, speed]()
			{
				if (alive.lock() && !m_localPlayback && m_songLoaded)
				{
					SetTempo(jlimit((int)MinTempo, (int)MaxTempo, roundToInt(m_tempo * speed)));
				}
			});
	}
}

// The listeners are called with the lock of the listener list held, and some of them ask
// the controller for the state of the own player (which takes the player's lock). The
// player's thread holds its lock while it sends the notes of the song: if it called the
// listeners directly, the two threads could wait for each other forever (the program
// froze). So from there the listeners are called later, on the message thread.
void PianoController::NotifyChangedLater(Aspect aspect, Channel channel)
{
	std::weak_ptr<bool> alive = m_alive;
	MessageManager::callAsync([this, alive, aspect, channel]()
		{
			if (alive.lock())
			{
				NotifyChanged(aspect, channel);
			}
		});
}

void PianoController::NotifyNoteMessageLater(const MidiMessage& message)
{
	std::weak_ptr<bool> alive = m_alive;
	MessageManager::callAsync([this, alive, message]()
		{
			if (alive.lock())
			{
				NotifyNoteMessage(message);
			}
		});
}

