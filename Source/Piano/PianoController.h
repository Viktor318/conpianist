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

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

#include "PianoConnector.h"
#include "LocalSongPlayer.h"
#include "LiveRecorder.h"

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
		apKeyboardTranspose,
		apVrm,
		apDamperResonance,
		apStringResonance,
		apKeyOffSampling,
		apMetronome,
		apPlaybackSource,
		apStyle
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
	// Live Play: notes and controllers played on the virtual keyboard or on MIDI In 2 are
	// sent on every Live Play channel (bit 0 = MIDI channel 1), to the piano or, with
	// playback via the MIDI device, to MIDI Out. The channel of the message is ignored.
	void SetLiveChannels(int channelMask);
	// Live-only Mixer channels: song channels (Midi1..Midi16) not used in the song but
	// chosen for Live Play. Their voice, volume, pan and reverb are kept here (also while
	// the channel is not used for Live Play) and sent again when the channel is used for
	// Live Play again, after a song is loaded and when the player changes. A song using
	// the channel overrides them (then it is a song channel).
	struct LiveChannelState
	{
		bool set = false;      // false: the defaults (piano voice, volume 100, pan centre)
		int voice = 0;         // 0x00MMLLPP
		bool gmVoice = false;  // a General MIDI voice (MIDI device playback)
		int volume = DefaultVolume;
		int pan = DefaultPan;
		int reverb = DefaultReverb;
	};
	bool IsLiveOnlyChannel(Channel ch) const;
	LiveChannelState GetLiveChannelState(Channel ch) const;
	void SetLiveChannelState(Channel ch, const LiveChannelState& state);
	void RestoreLiveChannels();
	// Live Play octave of a Mixer channel (-2..+2): only for the notes played live on
	// the channel, not for the song; not on the drum channel (10).
	int GetLiveOctave(Channel ch) const { return IsSongChannel(ch) ? m_liveOctave[ch - chMidi1] : 0; }
	void SetLiveOctave(Channel ch, int octave);
	// Copies the voice, volume, pan, reverb and octave of a keyboard part (Voice tab:
	// Main, Layer, Left) to a Mixer channel; possible if the voice of the part is known.
	bool CanTakeKeyboardPart(Channel part) const;
	void TakeKeyboardPart(Channel mixerChannel, Channel part);
	// Resets the voice, volume, pan, reverb and Live Play octave of a Mixer channel: to
	// the values of the song on a channel used in the song, otherwise to the defaults of
	// a live-only channel. On/off, the part and the Live Play selection are kept.
	void ResetChannelSettings(Channel ch);
	void PlayLive(const MidiMessage& message);
	// Recording of what is played live (Live Play, the piano's own keys and the piano's
	// accompaniment). autoStart: the recording starts with the first played note.
	// Accompaniment (style) of the piano. It needs the piano: nothing happens without it.
	enum StyleSection
	{
		ssIntro1 = 0x00,    // .. Intro 4 = 0x03
		ssMainA = 0x08,     // .. Main D = 0x0b
		ssFillInAA = 0x10,  // .. Fill In DD = 0x13
		ssBreak = 0x18,
		ssEnding1 = 0x20,   // .. Ending 4 = 0x23
		ssNone = 0x7f
	};
	struct StyleChord
	{
		int root = 0x7f;     // 0fffnnnn: fff - 0 bbb, 1 bb, 2 b, 3 natural, 4 #, 5 ##, 6 ###;
		                     // nnnn - 1 C, 2 D, 3 E, 4 F, 5 G, 6 A, 7 B; 0x7f - no chord
		int type = 0x7f;     // 0 Maj, 8 min, 19 7th, ... (Yamaha chord types)
		int bassRoot = 0x7f; // bass note, like the root
		int bassType = 0x7f;
	};
	// Preset path of the style, e.g. PRESET:/STYLE/Pop & Rock/Pop/Contemp Gtr Pop.T308.prs
	String GetStyleName();
	void SetStyle(const String& path);
	bool GetStylePlaying() const { return m_stylePlaying; }
	void SetStylePlaying(bool playing);
	// Sync Start is kept: the piano switches it off when the accompaniment starts; if it
	// was on, it is switched on again when the accompaniment stops (or ends).
	bool GetStyleSyncStart() const { return m_styleSyncStart; }
	void SetStyleSyncStart(bool on);
	int GetStyleSection() const { return m_styleSection; }         // playing now
	int GetStyleNextSection() const { return m_styleNextSection; } // played after it
	void SetStyleSection(int section);
	Position GetStylePosition() const;
	StyleChord GetStyleChord() const;
	// The tempo of the piano (also when ConPianist plays a song itself).
	int GetStyleTempo() const { return m_pianoTempo; }
	void SetStyleTempo(int tempo);

	LiveRecorder& GetRecorder() { return m_recorder; }
	void StartRecording(bool autoStart);
	void StopRecording();
	// Passes the current voices and mixer settings to the recorder (message thread).
	void UpdateRecorderSetups();
	// keep: the recording counts as saved (false for the temporary file of listening back)
	// quantizeTicks, quantizeEnds, tripletTicks, fillGaps: see LiveRecorder::Save
	bool SaveRecording(const File& file, bool includeStyle, String& error, bool keep = true,
		int quantizeTicks = 0, bool quantizeEnds = false, int tripletTicks = 0, bool fillGaps = false);
	// tempo (quarter notes per minute) and time signature of the recording
	int GetRecordedTempo() const { return m_recordedTempo; }
	int GetRecordedNumerator() const { return m_recordedNumerator; }
	int GetRecordedDenominator() const { return m_recordedDenominator; }
	// Manual recording with a count-in: starts on the downbeat after the given number
	// of measures (counted by the metronome or the playing song).
	void StartRecordingWithCountIn(int measures);

	// Metronome: the piano's own metronome; ConPianist's own clicks on the MIDI device
	// (drum channel) when the piano is not used for playing.
	bool GetMetronome() const { return m_metronome; }
	void SetMetronome(bool on);
	// bell on the first beat of the measure
	bool GetMetronomeBell() const { return m_metronomeBell; }
	void SetMetronomeBell(bool on);
	int GetMetronomeVolume() const { return m_metronomeVolume; } // 0..127
	void SetMetronomeVolume(int volume);
	int GetMetronomeBeatNumerator() const { return m_metronomeNumerator; }
	int GetMetronomeBeatDenominator() const { return m_metronomeDenominator; }
	void SetMetronomeBeat(int numerator, int denominator);
	// true if the beats of the metronome or of a playing song are known now
	bool HasBeats() const;
	// Releases every note and the sustain pedal still held by Live Play, at once (not
	// through the queue): on application exit the note-offs from MIDI In 2 or the
	// virtual keyboard would never arrive, and the notes would sound on.
	void ReleaseLive();
	// Live Play on the piano's own keyboard parts (the piano's second MIDI port, channel 1:
	// the Voice tab settings) instead of the Mixer channels; used when "enabled" and the
	// port is "available", but not with MIDI device playback (then Live Play sounds on the
	// MIDI Out, on the Mixer channels, as the song); otherwise the Mixer channels are used
	void SetLivePianoKeyboard(bool enabled, bool available);
	bool GetLivePianoKeyboard() const { return m_liveKeyboardEnabled; }
	bool IsLivePianoKeyboardAvailable() const { return m_liveKeyboardAvailable && !m_genericDevice; }
	bool IsLivePlayOnPianoKeyboard() const { return m_liveKeyboardEnabled && IsLivePianoKeyboardAvailable(); }
	std::function<void(const MidiMessage&)> sendToPianoKeyboard;
	// True once after the song was loaded again because the player was switched: the
	// settings are then restored from before the switch, not from the registration memory.
	bool TakeSkipRegistrationMemory() { const bool skip = m_skipRegistrationMemory; m_skipRegistrationMemory = false; return skip; }
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
	// sets a song channel voice from a saved state: a Yamaha voice ("gmVoice" false) or a
	// General MIDI voice, converted to the kind of voices of the current player
	void SetSavedSongChannelVoice(Channel ch, int voiceNum, bool gmVoice);
	// the Yamaha voice that was converted to the current General MIDI voice of the
	// channel (MIDI device playback), or -1: saved instead of the converted voice
	int GetOriginalSongChannelVoice(Channel ch) const;
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
	// Transposition of the piano's own keyboard, in semitones (Piano Room). The piano
	// adds it to Live Play on its keyboard parts, too. The transposition of the Playback
	// panel (SetTranspose) is for the song and all Live Play, not for the piano's own keys.
	int GetKeyboardTranspose() { return m_keyboardTranspose; }
	void SetKeyboardTranspose(int transpose);
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
	// Notifications come from several threads (piano messages, the local player, the UI),
	// while the UI adds and removes listeners: the list is locked while it is used, so a
	// listener cannot be removed (and deleted) while it is being notified.
	ListenerList<Listener, Array<Listener*, CriticalSection>> m_listeners;
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
	int m_keyboardTranspose = 0;
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
	CriticalSection m_liveLock;
	int m_liveChannels = 1;       // the channels used now (bit 16: the piano's keyboard)
	int m_liveMixerChannels = 1;  // the Mixer channels chosen for Live Play
	bool m_liveKeyboardEnabled = false;
	bool m_liveKeyboardAvailable = false;
	void UpdateLiveTarget();
	const static int LiveKeyboardBit = 1 << 16;
	void ApplyLiveChannels(int channelMask);
	void ReleaseLiveChannels(int channelMask);
	void SuspendLive();
	void ResumeLivePedal();
	void PressLivePedal(int channelMask);
	void SendLive(int channel, const MidiMessage& message);
	LiveRecorder m_recorder;
	int m_recordedTempo = DefaultTempo;   // tempo and time signature when the recording started
	int m_recordedNumerator = 4;
	int m_recordedDenominator = 4;
	std::atomic<bool> m_metronome{false};
	std::atomic<bool> m_metronomeBell{false};
	std::atomic<int> m_metronomeVolume{100};
	std::atomic<int> m_metronomeNumerator{4};
	std::atomic<int> m_metronomeDenominator{4};
	std::atomic<uint32> m_lastBeatMs{0};
	class SoftMetronome;
	std::unique_ptr<HighResolutionTimer> m_softMetronome;
	int m_softBeat = 0;
	std::atomic<bool> m_stylePlaying{false};
	std::atomic<bool> m_styleSyncStart{false};
	std::atomic<bool> m_styleSyncWanted{false}; // switched on again when the style stops
	std::atomic<int> m_styleSection{ssNone};
	std::atomic<int> m_styleNextSection{ssNone};
	std::atomic<int> m_stylePosition{0}; // measure << 16 | beat
	std::atomic<int> m_styleChord{0x7f7f7f7f}; // root, type, bass root, bass type
	std::atomic<int> m_pianoTempo{DefaultTempo};
	CriticalSection m_styleLock; // guards m_styleName
	String m_styleName;
	int m_pianoCount = 0;
	int m_pianoCountPeriod = 0;
	double m_softNextMs = 0;
	void SoftMetronomeTick();
	void OnBeat(int beat);
	int m_recKeyboardSources[128] = {}; // keyboard parts a live note was recorded on (bits 0..2)
	void RecordLiveKeyboard(const MidiMessage& message);
	LiveRecorder::Setup RecorderSetup(Channel ch);
	int m_liveNoteChannels[128] = {}; // channels on which each held note was started
	int m_liveNoteTranspose[128] = {}; // transposition used when each held note was started
	int m_liveSustainChannels = 0;    // channels on which the sustain pedal is down
	int m_liveSustainValue = 0;       // the last sustain pedal value played (0: up)
	int m_liveSuspendedPiano = 0;     // channels released by SuspendLive on the piano
	int m_liveSuspendedDevice = 0;    // ... and on the MIDI device (MIDI Out)
	bool m_liveSendNow = false;       // ReleaseLive: send without the queue
	LiveChannelState m_liveState[16];
	int m_liveOctave[16] = {};        // Live Play octave of each Mixer channel
	int LiveOctaveShift(int midiChannel) const;
	// a Yamaha voice of a live-only channel converted to General MIDI (-1: none): used
	// again when switching back to the piano, if the voice was not changed meanwhile
	int m_liveOriginalVoice[16];
	int m_liveConvertedVoice[16];
	LiveChannelState& LiveStateFor(Channel ch);
	void ApplyLiveChannel(Channel ch);

	// Mixer and playback settings kept when the player is switched (the song is loaded
	// again into the other player and would otherwise start with its own settings).
	struct MixSnapshot
	{
		bool valid = false;
		PlaybackSource source = psPiano;
		struct ChannelState { bool enabled; bool active; int volume; int pan; int reverb; String voice; } channels[16];
		int masterVolume = DefaultVolume;
		bool masterActive = true;
		bool parts[3];
		Channel partChannels[2];
		int tempo = DefaultTempo;
		int transpose = 0;
		Loop loop{{0,0},{0,0}};
	};
	MixSnapshot m_pendingSnapshot;
	// The piano's own player sets the song's own settings again when the song ends (and
	// jumps back to the beginning): the settings during playback are kept here and
	// restored then.
	MixSnapshot m_playingSnapshot;
	int m_lastPlayedMeasure = 0;
	bool m_stopRequested = false; // stopped or paused by the user (not the end of the song)
	bool m_skipRegistrationMemory = false;
	MixSnapshot TakeSnapshot();
	void ApplySnapshot(const MixSnapshot& snapshot);
	bool m_playbackSourceAutomatic = false;
	bool m_midiDevicePlaybackAvailable = false;
	int m_genericBank[16] = {};       // bank select (MSB << 8 | LSB) sent on each channel
	// Yamaha voices converted to General MIDI voices (per song channel, -1: none): when
	// switching back to the piano, the original voice is used if it was not changed
	int m_originalVoice[16];
	int m_convertedVoice[16];
	int ConvertSongChannelVoice(Channel ch, int voiceNum, bool fromGm, bool toGm);
	void ClearConvertedVoices();
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
