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

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

#include "PianoConnector.h"
#include "LocalSongPlayer.h"

#include <atomic>

class PianoController : public PianoConnector::Listener
{
public:
	struct Position
	{
		int measure;
		int beat;
		bool operator==(const Position& rhs) { return rhs.measure == measure && rhs.beat == beat; }
	};

	struct Loop
	{
		Position begin;
		Position end;
	};

	enum Channel
	{
		chNone = -1,
		chMain = 0x00,
		chLayer = 0x01,
		chLeft = 0x02,
		chMidi0 = 0x0f,
		chMidi1 = 0x10,
		chMidi2, chMidi3, chMidi4, chMidi5, chMidi6, chMidi7, chMidi8,
		chMidi9, chMidi10, chMidi11, chMidi12, chMidi13, chMidi14, chMidi15,
		chMidi16 = 0x1F,
		chMic = 0x40,
		chAuxIn = 0x41,
		chWave = 0x44,
		chMidiMaster = 0x50,
		chStyle = 0x51,
	};

	static const std::vector<Channel> AllChannels;
	static const std::vector<Channel> MidiChannels;

	struct ChannelInfo
	{
		bool enabled = false; // channel is present in current song; only for Midi1..Midi16
		bool active = false; // channel is active for playback
		int volume = DefaultVolume; // 0..127
		int pan = DefaultPan; // 0..127
		int reverb = DefaultReverb; // 0..127
		int octave = DefaultOctave; // -2..+2
		String voice;
	};

	enum Part
	{
		paRight = 0,
		paLeft = 1,
		paBacking = 2
	};

	enum GuideType
	{
		gtCorrectKey = 0,
		gtAnyKey = 1,
		gtYourTempo = 5
	};

	enum LidPosition
	{
		lpOpen = 0,
		lpHalf = 1,
		lpClose = 2
	};

	enum TouchCurve
	{
		tcSoft2 = 0,
		tcSoft1 = 1,
		tcMedium = 2,
		tcHard1 = 3,
		tcHard2 = 4
	};

	enum Aspect
	{
		apConnection,
		apLocalControl,
		apSongName,
		apSongLoaded,
		apLength,
		apPosition,
		apPlayback,
		apGuide,
		apStreamLights,
		apLoop,
		apTranspose,
		apTempo,
		apReverbEffect,
		apPart,
		apPartAuto,
		apPartChannel,
		apVolume,
		apPan,
		apReverb,
		apOctave,
		apEnable,
		apActive,
		apVoice,
		apSplitPoint,
		apLidPosition,
		apEnvironment,
		apBrightness,
		apTouchCurve,
		apFixedCurve,
		apFixedVelocity,
		apMasterTune,
		apVrm,
		apDamperResonance,
		apStringResonance,
		apKeyOffSampling,
		apPlaybackSource
	};

	static const int MinVolume = 0;
	static const int MaxVolume = 127;
	static const int DefaultVolume = 100;
	static const int MinTempo = 5;
	static const int MaxTempo = 280;
	static const int DefaultTempo = 120;
	static const int MinTranspose = -12;
	static const int MaxTranspose = +12;
	static const int DefaultTranspose = 0;
	static const int TransposeBase = 0x40;
	static const int MinPan = -64;
	static const int MaxPan = +63;
	static const int DefaultPan = 0;
	static const int PanBase = 0x40;
	static const int MinReverb = 0;
	static const int MaxReverb = 127;
	static const int DefaultReverb = 0;
	static const int GenericDefaultReverb = 40; // General MIDI default of CC91
	static const int DefaultReverbEffect = 0x0118; // Recital Hall
	static const int MinOctave = -2;
	static const int MaxOctave = +2;
	static const int DefaultOctave = 0;
	static const int OctaveBase = 0x40;
	static const int DefaultEnvironment = 10;
	static const int DefaultBrightness = 0x40;
	static const int MasterTuneBase = 0x400;
	static const int MasterTuneFactor = 4;
	static const int DefaultResonance = 5;
	static const int MaxResonance = 10;
	static const int DefaultKeyOffSampling = 0x40;
	static const int MaxKeyOffSampling = 0x50;
	static const int DefaultFixedVelocity = 100;
	static const LidPosition DefaultLidPosition = lpOpen;
	static const TouchCurve DefaultTouchCurve = tcMedium;
	static const int UploadPort = 10504; // TCP port of the piano for uploading songs

	class Listener
	{
	public:
		virtual ~Listener() {}
		virtual void PianoStateChanged(Aspect aspect, Channel channel) {}
		virtual void PianoNoteMessage(const MidiMessage& message) {}
	};

	PianoController();
	~PianoController();
	void SetPianoConnector(PianoConnector* pianoConnector);
	void AddListener(Listener* listener);
	void RemoveListener(Listener* listener);
	void SetRemoteIp(const String& remoteIp) { m_remoteIp = remoteIp; }
	const String& GetRemoteIp() { return m_remoteIp; }
	void Connect();
	void Disconnect();
	void Reset();
	void Sync();
	// Local playback: ConPianist plays the MIDI file itself over the MIDI port (USB)
	// instead of uploading it to the piano's own player (network connection).
	void SetLocalPlayback(bool enabled);
	bool IsLocalPlayback() const { return m_localPlayback; }
	void ShutdownLocalPlayer();
	// Playback source:
	// - psPiano: the piano's own player (the song is uploaded over the network,
	//   Stream Lights and Guide work),
	// - psLocal: ConPianist's own player, sending to the piano's MIDI port (USB),
	// - psMidiDevice: ConPianist's own player, sending to another MIDI device (MIDI Out,
	//   e.g. loopMIDI to a software instrument); the mixer settings are sent as standard
	//   MIDI controllers and the voices are General MIDI voices.
	// The availability is decided by the caller (connections, network check).
	enum PlaybackSource { psPiano, psLocal, psMidiDevice };
	void SetPlaybackAvailability(bool network, bool local, bool midiDevice);
	bool IsNetworkPlaybackAvailable() const { return m_networkPlaybackAvailable; }
	bool IsLocalPlaybackAvailable() const { return m_localPlaybackAvailable; }
	bool IsMidiDevicePlaybackAvailable() const { return m_midiDevicePlaybackAvailable; }
	// Switches the player; a loaded song is loaded again into the other player
	// at the same measure. Ignored if the requested source is not available.
	// "automatic": chosen by the program (not by the user).
	void SetPlaybackSource(PlaybackSource source, bool automatic = false);
	PlaybackSource GetPlaybackSource() const { return m_playbackSource; }
	bool IsPlaybackSourceAutomatic() const { return m_playbackSourceAutomatic; }
	bool IsMidiDevicePlayback() const { return m_playbackSource == psMidiDevice; }
	// Loads a song into the current player. If the upload to the piano fails and
	// the own player is available, playback continues with the own player.
	bool LoadSong(const File& file);
	// Songs can be played and mixed: a piano is connected or a MIDI device is used
	bool IsReady() const { return m_connected || IsMidiDevicePlayback(); }
	// The MIDI device (MIDI Out): messages to it, and messages from MIDI In 2, which are
	// played through to the MIDI device in psMidiDevice mode.
	std::function<void(const MidiMessage&)> sendToMidiDevice;
	void IncomingMidiDeviceMessage(const MidiMessage& message);
	// Called (on the message thread) when the piano could not be reached over the
	// network and playback switched to ConPianist's own player.
	std::function<void()> onNetworkPlaybackFailed;
	void InitEvents();
	bool UploadSong(const File& file);
	void ResetSong();
	void Play();
	void Pause();
	void Stop();
	const String& GetModel() { return m_model; }
	const String& GetVersion() { return m_version; }
	bool IsConnected() { return m_connected; }
	bool IsSongLoaded();
	bool GetPlaying();
	bool GetGuide() { return m_guide; }
	void SetGuide(bool enable);
	GuideType GetGuideType() { return m_guideType; }
	void SetGuideType(GuideType type);
	bool GetLocalControl() { return m_localControl; }
	void SetLocalControl(bool enabled);
	bool GetStreamLights() { return m_streamLights; }
	void SetStreamLights(bool enable);
	bool GetStreamFast() { return m_streamFast; }
	void SetStreamFast(bool fast);
	Position GetLength();
	Position GetPosition();
	void SetPosition(const Position position);
	Loop GetLoop() { return m_loop; }
	void SetLoop(Loop loop);
	Position GetLoopStart() { return m_loopStart; }
	void SetLoopStart(const Position loopStart);
	void ResetLoop();
	int GetVolume(Channel ch) { return m_channels[ch].volume; }
	void SetVolume(Channel ch, int volume);
	void ResetVolume(Channel ch);
	int GetPan(Channel ch) { return m_channels[ch].pan; }
	void SetPan(Channel ch, int pan);
	void ResetPan(Channel ch);
	int GetReverb(Channel ch) { return m_channels[ch].reverb; }
	void SetReverb(Channel ch, int reverb);
	void ResetReverb(Channel ch);
	int GetOctave(Channel ch) { return m_channels[ch].octave; }
	void SetOctave(Channel ch, int octave);
	int GetTempo() { return m_tempo; }
	void SetTempo(int tempo);
	void ResetTempo();
	int GetTranspose() { return m_transpose; }
	void SetTranspose(int transpose);
	bool GetPart(Part part) { return m_parts[part]; }
	void SetPart(Part part, bool enable);
	Channel GetPartChannel(Part part) { return m_partChannels[part]; }
	void SetPartChannel(Part part, Channel channel);
	int GetPartAuto() { return m_partAuto; }
	void SetPartAuto(bool enable);
	const String& GetVoice(Channel ch) { return m_channels[ch].voice; }
	void SetVoice(Channel ch, const String& voice);
	void SetSongChannelVoice(Channel ch, int voiceNum);
	bool GetActive(Channel ch) { return m_channels[ch].active; }
	void SetActive(Channel ch, bool active);
	bool GetEnabled(Channel ch) { return m_channels[ch].enabled; }
	int GetReverbEffect() { return m_reverbEffect; }
	void SetReverbEffect(int effect);
	int GetSplitPoint() { return m_splitPoint; }
	void SetSplitPoint(int splitPoint);
	const String& GetSongName() { return m_songName; }
	LidPosition GetLidPosition() { return m_lidPosition; }
	void SetLidPosition(LidPosition position);
	int GetEnvironment() { return m_environment; }
	void SetEnvironment(int environment);
	int GetBrightness() { return m_brightness; }
	void SetBrightness(int brightness);
	TouchCurve GetTouchCurve() { return m_touchCurve; }
	void SetTouchCurve(TouchCurve touchCurve);
	bool GetFixedCurve(Channel ch) { return m_fixedCurve[ch]; }
	void SetFixedCurve(Channel ch, bool active);
	int GetFixedVelocity() { return m_fixedVelocity; }
	void SetFixedVelocity(int fixedVelocity);
	int GetMasterTune() { return m_masterTune; }
	void SetMasterTune(int masterTune);
	int GetVrm() { return m_vrm; }
	void SetVrm(bool vrm);
	int GetDamperResonance() { return m_damperResonance; }
	void SetDamperResonance(int damperResonance);
	int GetStringResonance() { return m_stringResonance; }
	void SetStringResonance(int stringResonance);
	int GetKeyOffSampling() { return m_keyOffSampling; }
	void SetKeyOffSampling(int keyOffSampling);

	// e.g. the virtual keyboard: goes to the MIDI device in psMidiDevice mode
	void SendMidiMessage(const MidiMessage& message);
	void IncomingMidiMessage(const MidiMessage& message) override;
	void IncomingPianoMessage(const PianoMessage& message) override;

private:
	PianoConnector* m_pianoConnector;
	std::vector<Listener*> m_listeners;
	String m_remoteIp;
	String m_model;
	String m_version;
	bool m_connected = false;
	bool m_playing = false;
	bool m_localControl = true;
	bool m_guide = false;
	GuideType m_guideType = gtCorrectKey;
	bool m_streamLights = false;
	bool m_streamFast = false;
	Position m_length{0,0};
	Position m_position{0,0};
	bool m_parts[3]{false,false,false};
	Channel m_partChannels[2]{chMidi0,chMidi0};
	bool m_partAuto = true;
	int m_tempo = DefaultTempo;
	int m_transpose = DefaultTranspose;
	Position m_loopStart{0,0};
	Loop m_loop{{0,0},{0,0}};
	ChannelInfo m_channels[128]; // indexed by the channel number sent by the piano (a 7-bit value: 0..127)
	int m_reverbEffect = 0;
	String m_songName;
	bool m_songLoaded = false;
	bool m_songLoading = false;
	int m_splitPoint = 0;
	LidPosition m_lidPosition = DefaultLidPosition;
	int m_environment = 0;
	int m_brightness = DefaultBrightness;
	TouchCurve m_touchCurve = DefaultTouchCurve;
	bool m_fixedCurve[3]{false,false,false};
	int m_fixedVelocity = DefaultFixedVelocity;
	int m_masterTune = 0;
	bool m_vrm = false;
	int m_damperResonance = DefaultResonance;
	int m_stringResonance = DefaultResonance;
	int m_keyOffSampling = DefaultKeyOffSampling;
	std::unique_ptr<PianoMessage> lastMessage;
	bool m_localPlayback = false;
	std::unique_ptr<LocalSongPlayer> m_localPlayer;
	bool m_networkPlaybackAvailable = true;
	bool m_localPlaybackAvailable = false;
	int m_pendingMeasure = 0; // measure to jump to after the song is loaded again
	bool m_shownNotes[16][128] = {}; // notes of the local player shown on the virtual keyboard
	PlaybackSource m_playbackSource = psPiano;
	std::atomic<bool> m_genericDevice{false}; // playback to a MIDI device (psMidiDevice)
	bool m_playbackSourceAutomatic = false;
	bool m_midiDevicePlaybackAvailable = false;
	int m_genericBank[16] = {};       // bank select (MSB << 8 | LSB) sent on each channel
	std::shared_ptr<bool> m_alive = std::make_shared<bool>(true); // for delayed callbacks

	void NotifyChanged(Aspect aspect, Channel channel = chNone);
	void NotifyNoteMessage(const MidiMessage& message);
	void ResyncStateFromPiano();
	String DecodeSongName(String rawValue);
	bool LoadLocalSong(const File& file);
	bool LoadSongInternal(const File& file);
	void ReloadSong();
	void ApplyPlaybackSource(PlaybackSource source);
	void SendToOutput(const MidiMessage& message);
	void ShowLocalNote(const MidiMessage& message);
	void OnLocalMessage(const MidiMessage& message);
	void InitGenericMixer();
	int GenericSetupValue(Channel ch, int controller, int defaultValue);
	void ClearSongState();
	bool IsLocalSongLoaded() const { return m_localPlayback && m_localPlayer && m_localPlayer->IsLoaded(); }
	void ResetLocalMixState();
	void UpdateLocalMutes();
	static bool IsSongChannel(Channel ch) { return chMidi1 <= ch && ch <= chMidi16; }
};
