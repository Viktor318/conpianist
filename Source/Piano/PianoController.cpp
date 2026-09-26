/*
 *  This file is part of ConPianist. See <https://github.com/hugbug/conpianist>.
 *
 *  Copyright (C) 2018-2020 Andrey Prygunkov <hugbug@users.sourceforge.net>
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
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Get, Property::Transpose, 2, 0));
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
	SetVrm(true);
	SetDamperResonance(DefaultResonance);
	SetStringResonance(DefaultResonance);
	SetKeyOffSampling(DefaultKeyOffSampling);

	SetOctave(chMain, 0);
	SetOctave(chLayer, 0);
	SetOctave(chLeft, 0);

	SetActive(chMain, true);
	SetActive(chLayer, false);
	SetActive(chLeft, false);
	SetActive(chMic, true);

	SetVoice(chMain, "PRESET:/VOICE/Piano/Grand Piano/CFX Grand.T542.VRM");
	SetVoice(chLayer, "PRESET:/VOICE/Strings & Vocal/String Ensemble/Real Strings.T250.SAR");
	SetVoice(chLeft, "PRESET:/VOICE/Piano/FM E.Piano/Sweet DX.T232.CLV");

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
	char response[16];
	StreamingSocket socket;
	bool ok = socket.connect(m_remoteIp, UploadPort, ConnectTimeoutMs) &&
		socket.write(message.getData(), messageSize) == messageSize &&
		socket.waitUntilReady(true, ResponseTimeoutMs) == 1 &&
		socket.read(response, (int)sizeof(response), false) > 0;

	m_songLoading &= ok;
	return ok;
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
	if (m_localPlayback)
	{
		if (m_localPlayer) m_localPlayer->Play();
		return;
	}
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Set, Property::Play, 1));
}

void PianoController::Pause()
{
	if (m_localPlayback)
	{
		if (m_localPlayer) m_localPlayer->Pause();
		return;
	}
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Set, Property::Play, 2));
}

void PianoController::Stop()
{
	if (m_localPlayback)
	{
		if (m_localPlayer) m_localPlayer->Stop();
		return;
	}
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
	if (m_genericDevice)
	{
		// the song's own volume changes (CC7) are kept, scaled to the mixer setting
		m_channels[ch].volume = volume;
		if (m_localPlayer && ch == chMidiMaster)
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
	if (m_genericDevice)
	{
		m_channels[ch].pan = pan;
		if (m_localPlayer && IsSongChannel(ch))
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
	if (m_genericDevice)
	{
		m_channels[ch].reverb = reverb;
		if (m_localPlayer && IsSongChannel(ch))
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
		if (m_localPlayer) m_localPlayer->SetTempo(tempo);
		m_tempo = tempo;
		NotifyChanged(apTempo);
		return;
	}
	m_pianoConnector->SendPianoMessage(PianoMessage(Action::Set, Property::Tempo, tempo));
}

void PianoController::ResetTempo()
{
	if (m_localPlayback)
	{
		SetTempo(IsLocalSongLoaded() ? m_localPlayer->GetBaseTempo() : DefaultTempo);
		return;
	}
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
void PianoController::SetSongChannelVoice(Channel ch, int voiceNum)
{
	const int midiChannel = ch - chMidi0; // 1..16
	if (midiChannel < 1 || midiChannel > 16)
	{
		return;
	}

	m_pianoConnector->SendMidiMessage(MidiMessage::controllerEvent(midiChannel, 0, (voiceNum >> 16) & 0x7f));
	m_pianoConnector->SendMidiMessage(MidiMessage::controllerEvent(midiChannel, 32, (voiceNum >> 8) & 0x7f));
	m_pianoConnector->SendMidiMessage(MidiMessage::programChange(midiChannel, voiceNum & 0x7f));

	if (m_genericDevice)
	{
		// a general MIDI device does not report its voices: the name is shown from here
		m_genericBank[midiChannel - 1] = (voiceNum >> 8) & 0x7f7f;
		m_channels[ch].voice = String(voiceNum);
		NotifyChanged(apVoice, ch);
	}
}

void PianoController::SetActive(Channel ch, bool active)
{
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
	if (m_genericDevice)
	{
		// e.g. loopMIDI sends everything back: these are our own notes
		return;
	}

	if (message.isNoteOnOrOff())
	{
		NotifyNoteMessage(message);
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

	if (m_localPlayback &&
		(property == Property::Position || property == Property::Length ||
		property == Property::Play || property == Property::SongName ||
		property == Property::Loop || property == Property::Tempo ||
		property == Property::Transpose || property == Property::Present ||
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
		m_playing = boolValue;
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
		NotifyChanged(apVolume, ch);
	}
	else if (property == Property::Pan)
	{
		m_channels[ch].pan = intValue - PanBase;
		NotifyChanged(apPan, ch);
	}
	else if (property == Property::Reverb)
	{
		m_channels[ch].reverb = intValue;
		NotifyChanged(apReverb, ch);
	}
	else if (property == Property::Octave)
	{
		m_channels[ch].octave = intValue - OctaveBase;
		NotifyChanged(apOctave, ch);
	}
	else if (property == Property::Active && ch != chMidiMaster)
	{
		m_channels[ch].active = boolValue;
		NotifyChanged(apActive, ch);
	}
	else if (property == Property::Present && ch != chMidiMaster)
	{
		m_channels[ch].enabled = boolValue;
		NotifyChanged(apEnable, ch);
	}
	else if (property == Property::VoiceMidi && size == 4)
	{
		m_channels[ch].voice = String((data[0] << 7 * 3) + (data[1] << 7 * 2) + (data[2] << 7) + data[3]);
		NotifyChanged(apVoice, ch);
	}
	else if (property == Property::Tempo)
	{
		m_tempo = intValue;
		NotifyChanged(apTempo);
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
		m_splitPoint = intValue;
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
		//TODO: make thread safe
		m_channels[ch].voice = pm->GetStrValue();
		NotifyChanged(apVoice, ch);
	}
	else if (property == Property::PianoModel)
	{
		//TODO: make thread safe
		m_model = pm->GetStrValue();
	}
	else if (property == Property::FirmwareVersion)
	{
		//TODO: make thread safe
		m_version = pm->GetStrValue();
		if (!m_connected)
		{
			m_connected = true;
			NotifyChanged(apConnection);

			if (IsLocalSongLoaded())
			{
				// The song may have been loaded while the piano was switched off; its
				// voices and settings are sent only at loading, so it is loaded again.
				std::weak_ptr<bool> alive = m_alive;
				MessageManager::callAsync([this, alive]()
					{
						if (alive.lock())
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
		//TODO: make thread safe
		m_songName = name;
		NotifyChanged(apSongName);
		if (m_songLoading)
		{
			m_songLoading = false;
			NotifyChanged(apSongLoaded);

			if (m_pendingMeasure > 1)
			{
				// the song was loaded again after switching the player: go back to the
				// measure where it was; a short delay lets the piano finish loading
				const int measure = m_pendingMeasure;
				std::weak_ptr<bool> alive = m_alive;
				MessageManager::callAsync([this, alive, measure]()
					{
						Timer::callAfterDelay(500, [this, alive, measure]()
							{
								if (alive.lock() && !m_localPlayback && m_songLoaded)
								{
									SetPosition({measure, 1});
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
	if (enabled == m_localPlayback)
	{
		return;
	}

	if (m_localPlayer)
	{
		m_localPlayer->Unload();
	}

	m_localPlayback = enabled;

	if (enabled && !m_localPlayer)
	{
		m_localPlayer = std::make_unique<LocalSongPlayer>();
		// notes are sent directly (not through the message queue) for exact timing
		m_localPlayer->sendMidi = [this](const MidiMessage& message)
			{
				m_pianoConnector->SendMidiMessageNow(message);
				OnLocalMessage(message);
			};
		m_localPlayer->onChanged = [this](bool positionChanged, bool playingChanged)
			{
				if (positionChanged) NotifyChanged(apPosition);
				if (playingChanged) NotifyChanged(apPlayback);
			};
	}

	ClearSongState();

	if (enabled)
	{
		ResetLocalMixState();
	}
}

void PianoController::SetPlaybackAvailability(bool network, bool local)
{
	m_networkPlaybackAvailable = network;
	m_localPlaybackAvailable = local;
	NotifyChanged(apPlaybackSource);
}

void PianoController::SetPlaybackSource(bool local)
{
	if (local == m_localPlayback ||
		(local ? !m_localPlaybackAvailable : !m_networkPlaybackAvailable))
	{
		// nothing to do; lets the UI show the actual state again
		NotifyChanged(apPlaybackSource);
		return;
	}

	// the loaded song is loaded again into the other player, at the same measure
	const String songName = IsSongLoaded() ? m_songName : String();
	const int measure = GetPosition().measure;

	if (!m_localPlayback)
	{
		Stop(); // the piano's own player
	}

	SetLocalPlayback(local);
	NotifyChanged(apPlaybackSource);

	if (File::isAbsolutePath(songName) && File(songName).existsAsFile())
	{
		m_pendingMeasure = measure;
		LoadSongInternal(File(songName));
	}

	if (!m_localPlayback && m_connected)
	{
		// the piano's own player is used again: read its song, parts and channels.
		// This is done after the upload, so that the answers cannot be mistaken
		// for the confirmation of the new song.
		ResyncStateFromPiano();
	}
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
			m_channels[ch].voice = String((m_genericBank[index] << 8) | message.getProgramChangeNumber());
			NotifyChanged(apVoice, ch);
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
			NotifyNoteMessage(message);
		}
	}
	else if (shown)
	{
		// released even if the part assignment has changed in the meantime
		shown = false;
		NotifyNoteMessage(message);
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

	const File file(m_songName);
	if (file.existsAsFile())
	{
		m_pendingMeasure = GetPosition().measure;
		LoadSongInternal(file);
	}
}

bool PianoController::LoadSong(const File& file)
{
	m_pendingMeasure = 0;
	return LoadSongInternal(file);
}

bool PianoController::LoadSongInternal(const File& file)
{
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
			SetLocalPlayback(true);
			NotifyChanged(apPlaybackSource);
			if (onNetworkPlaybackFailed)
			{
				onNetworkPlaybackFailed();
			}
			ok = LoadLocalSong(file);
		}
	}

	if (ok && m_localPlayback && m_pendingMeasure > 1)
	{
		SetPosition({m_pendingMeasure, 1});
	}
	if (!ok || m_localPlayback)
	{
		m_pendingMeasure = 0;
	}
	return ok;
}

// Stops the local player before the connectors are destroyed (on application exit).
void PianoController::ShutdownLocalPlayer()
{
	if (m_localPlayer)
	{
		m_localPlayer->Unload();
		m_localPlayer.reset();
	}
	m_localPlayback = false;
}

bool PianoController::LoadLocalSong(const File& file)
{
	if (m_genericDevice)
	{
		// General MIDI default voice (program 0) until the song selects another one
		for (Channel ch : MidiChannels)
		{
			m_genericBank[ch - chMidi1] = 0;
			m_channels[ch].voice = "0";
		}
	}

	if (!m_localPlayer || !m_localPlayer->Load(file))
	{
		return false;
	}

	m_songName = file.getFullPathName();
	m_songLoaded = true;
	m_loop = {{0,0},{0,0}};
	m_loopStart = {0,0};
	m_tempo = m_localPlayer->GetBaseTempo();
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

void PianoController::SetGenericDevice(bool generic)
{
	if (generic == m_genericDevice)
	{
		return;
	}

	m_genericDevice = generic;
	Logger::writeToLog(generic ? "General MIDI device mode" : "Piano mode");

	if (m_localPlayer)
	{
		m_localPlayer->SetMasterVolumeScale(1.0);
	}

	// the song is loaded again, so that the mixer and the voices fit the device
	ReloadSong();

	NotifyChanged(apConnection);
}

void PianoController::ClearSongState()
{
	m_songLoaded = false;
	m_songLoading = false;
	m_songName = "";
	m_playing = false;
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
	RemoveListener(listener);
	m_listeners.push_back(listener);
}

void PianoController::RemoveListener(Listener* listener)
{
	std::vector<Listener*>::iterator pos = std::find(m_listeners.begin(), m_listeners.end(), listener);
	if (pos != m_listeners.end())
	{
		m_listeners.erase(pos);
	}
}

void PianoController::NotifyChanged(Aspect aspect, Channel channel)
{
	for (auto listener : m_listeners)
	{
		listener->PianoStateChanged(aspect, channel);
	}
}

void PianoController::NotifyNoteMessage(const MidiMessage& message)
{
	for (auto listener : m_listeners)
	{
		listener->PianoNoteMessage(message);
	}
}

