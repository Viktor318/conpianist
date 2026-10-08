/*
 *  This file is part of ConPianist. See <https://github.com/Viktor318/conpianist>.
 *  Fork of the original project <https://github.com/hugbug/conpianist>.
 *
 *  Copyright (C) 2020 Andrey Prygunkov <hugbug@users.sourceforge.net>
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

#include "RegistrationMemory.h"
#include "Presets.h"

void RegistrationMemory::Save()
{
	std::unique_ptr<XmlElement> state = CreateXml();
	state->writeTo(file);
}

std::unique_ptr<XmlElement> RegistrationMemory::CreateXml()
{
	std::unique_ptr<XmlElement> state = std::make_unique<XmlElement>("ConPianistRegistrationMemory");
	root = state.get();

	if (options.songname)
	{
		SaveSongName();
	}

	if (options.voices)
	{
		SaveVoice(PianoController::chMain, "Main");
		SaveVoice(PianoController::chLeft, "Left");
		SaveVoice(PianoController::chLayer, "Layer");
		SaveSplitPoint();
	}

	if (options.balance || options.mixer || options.pianoroom)
	{
		SaveReverbEffect();
	}

	if (options.playback)
	{
		SavePlayback();
	}

	if (options.balance)
	{
		SaveChannel(PianoController::chMain, "Main");
		SaveChannel(PianoController::chLeft, "Left");
		SaveChannel(PianoController::chLayer, "Layer");
		SaveChannel(PianoController::chMidiMaster, "MidiMaster");
		SaveChannel(PianoController::chMic, "Mic");
		SaveChannel(PianoController::chAuxIn, "AuxIn");
	}

	if (options.mixer)
	{
		for (PianoController::Channel ch : PianoController::MidiChannels)
		{
			SaveChannel(ch, String("Midi") + String(ch - PianoController::chMidi0));
		}
		if (!options.balance)
		{
			// the song's volume (Playback panel) belongs to the mixer, too
			SaveChannel(PianoController::chMidiMaster, "MidiMaster");
		}
	}

	if (options.pianoroom)
	{
		SavePianoRoom();
	}

	if (options.settings)
	{
		SaveSettings();
	}

	if (options.livechannels)
	{
		SaveLiveChannels();
	}

	if (options.style)
	{
		SaveStyle();
	}

	root = nullptr;
	return state;
}

// The full path of the loaded song, for information and for restoring the last state.
void RegistrationMemory::SaveSongName()
{
	if (pianoController.IsSongLoaded() && File::isAbsolutePath(pianoController.GetSongName()))
	{
		root->createNewChildElement("Song")->addTextElement(pianoController.GetSongName());
	}
}

String RegistrationMemory::GetSongName(const File& file)
{
	std::unique_ptr<XmlElement> state = XmlDocument::parse(file);
	XmlElement* el = state ? state->getChildByName("Song") : nullptr;
	return el ? el->getAllSubText().trim() : String();
}

void RegistrationMemory::Load()
{
	std::unique_ptr<XmlElement> savedState = XmlDocument::parse(file);
	if (!savedState) return;

	root = savedState.get();

	if (options.settings)
	{
		// first: the Live Play channels decide how the mixer channels are loaded
		LoadSettings();
	}

	if (options.livechannels)
	{
		LoadLiveChannels();
	}

	if (options.voices)
	{
		LoadVoice(PianoController::chMain, "Main");
		LoadVoice(PianoController::chLeft, "Left");
		LoadVoice(PianoController::chLayer, "Layer");
		LoadSplitPoint();
	}

	if (options.balance || options.mixer || options.pianoroom)
	{
		LoadReverbEffect();
	}

	if (options.playback)
	{
		LoadPlayback();
	}

	if (options.balance)
	{
		LoadChannel(PianoController::chMain, "Main");
		LoadChannel(PianoController::chLeft, "Left");
		LoadChannel(PianoController::chLayer, "Layer");
		LoadChannel(PianoController::chMidiMaster, "MidiMaster");
		LoadChannel(PianoController::chMic, "Mic");
		LoadChannel(PianoController::chAuxIn, "AuxIn");
	}

	if (options.mixer)
	{
		for (PianoController::Channel ch : PianoController::MidiChannels)
		{
			LoadChannel(ch, String("Midi") + String(ch - PianoController::chMidi0));
		}
		if (!options.balance)
		{
			// the song's volume (Playback panel) belongs to the mixer, too
			LoadChannel(PianoController::chMidiMaster, "MidiMaster");
		}
	}

	if (options.pianoroom)
	{
		LoadPianoRoom();
	}

	if (options.style)
	{
		// after the voices: the split point of the Left part is sent before the one of
		// the accompaniment (the piano moves one with the other)
		LoadStyle();
	}
}

void RegistrationMemory::SaveChannel(PianoController::Channel channel, String name)
{
	XmlElement* listElement = root->getChildByName("Channels");
	if (!listElement)
	{
		listElement = root->createNewChildElement("Channels");
	}
	XmlElement* chElem = listElement->createNewChildElement(name);
	chElem->createNewChildElement("Active")->addTextElement(pianoController.GetActive(channel) ? "yes" : "no");
	chElem->createNewChildElement("Volume")->addTextElement(String(pianoController.GetVolume(channel)));
	chElem->createNewChildElement("Pan")->addTextElement(String(pianoController.GetPan(channel)));
	chElem->createNewChildElement("Reverb")->addTextElement(String(pianoController.GetReverb(channel)));

	// Song channels: also store the voice, which can be changed in the Mixer.
	// It is stored as the MIDI voice number (0x00MMLLPP) so that exactly the same
	// voice is restored; the title attribute is only for human readers.
	if (IsSongChannel(channel) && pianoController.GetEnabled(channel))
	{
		String voice = pianoController.GetVoice(channel);
		if (voice.startsWith("PRESET:"))
		{
			Voice* vc = Presets::FindVoice(voice);
			voice = vc ? String(vc->num) : String();
		}
		// With MIDI device playback the voices are General MIDI voices ("set" attribute);
		// a voice converted from a Yamaha voice is saved as the original Yamaha voice.
		bool gmVoice = pianoController.IsMidiDevicePlayback();
		const int original = pianoController.GetOriginalSongChannelVoice(channel);
		if (original >= 0)
		{
			voice = String(original);
			gmVoice = false;
		}
		if (voice.isNotEmpty())
		{
			XmlElement* voiceElem = chElem->createNewChildElement("Voice");
			if (gmVoice)
			{
				voiceElem->setAttribute("set", "gm");
			}
			voiceElem->setAttribute("title", gmVoice ?
				Presets::GmVoiceTitle(voice, channel == PianoController::chMidi10) : Presets::VoiceTitle(voice));
			voiceElem->addTextElement(voice);
		}
	}
}

void RegistrationMemory::LoadChannel(PianoController::Channel channel, String name)
{
	XmlElement* listElement = root->getChildByName("Channels");
	if (!listElement) return;

	XmlElement* chElem = listElement->getChildByName(name);
	if (!chElem) return;

	if (IsSongChannel(channel) && !pianoController.GetEnabled(channel) &&
		settings.IsKeyboardChannel(channel - PianoController::chMidi0))
	{
		return; // a Live Play channel not used in the song: its settings are in "LiveChannels"
	}

	XmlElement* el;
	// Aux In and the song master channel cannot be switched on/off (the piano rejects it)
	if ((el = chElem->getChildByName("Active")) &&
		channel != PianoController::chAuxIn && channel != PianoController::chMidiMaster)
	{
		String value = el->getAllSubText();
		pianoController.SetActive(channel, value.equalsIgnoreCase("yes"));
	}
	if ((el = chElem->getChildByName("Volume")))
	{
		int value = el->getAllSubText().getIntValue();
		pianoController.SetVolume(channel, value);
	}
	if ((el = chElem->getChildByName("Pan")))
	{
		int value = el->getAllSubText().getIntValue();
		pianoController.SetPan(channel, value);
	}
	if ((el = chElem->getChildByName("Reverb")))
	{
		int value = el->getAllSubText().getIntValue();
		pianoController.SetReverb(channel, value);
	}
	if (IsSongChannel(channel) && (el = chElem->getChildByName("Voice")))
	{
		String value = el->getAllSubText().trim();
		if (value.isNotEmpty())
		{
			// converted, if saved for another kind of player (Yamaha / General MIDI)
			pianoController.SetSavedSongChannelVoice(channel, value.getIntValue(),
				el->getStringAttribute("set").equalsIgnoreCase("gm"));
		}
	}
}

void RegistrationMemory::SaveVoice(PianoController::Channel channel, String name)
{
	XmlElement* listElement = root->getChildByName("Voices");
	if (!listElement)
	{
		listElement = root->createNewChildElement("Voices");
	}

	XmlElement* chElem = listElement->createNewChildElement(name);

	String voiceNum = pianoController.GetVoice(channel);
	Voice* voice = Presets::FindVoice(voiceNum);
	chElem->createNewChildElement("Path")->addTextElement(voice ? voice->path : voiceNum);

	chElem->createNewChildElement("Octave")->addTextElement(String(pianoController.GetOctave(channel)));
}

void RegistrationMemory::LoadVoice(PianoController::Channel channel, String name)
{
	XmlElement* listElement = root->getChildByName("Voices");
	if (!listElement) return;

	XmlElement* chElem = listElement->getChildByName(name);
	if (!chElem) return;

	XmlElement* el;
	if ((el = chElem->getChildByName("Path")))
	{
		String value = el->getAllSubText();
		pianoController.SetVoice(channel, value);
	}
	if ((el = chElem->getChildByName("Octave")))
	{
		int value = el->getAllSubText().getIntValue();
		pianoController.SetOctave(channel, value);
	}
}

void RegistrationMemory::SaveSplitPoint()
{
	XmlElement* listElement = root->getChildByName("Voices");
	if (!listElement)
	{
		listElement = root->createNewChildElement("Voices");
	}
	listElement->createNewChildElement("SplitPoint")->addTextElement(String(pianoController.GetSplitPoint()));
}

void RegistrationMemory::LoadSplitPoint()
{
	XmlElement* listElement = root->getChildByName("Voices");
	if (!listElement) return;

	XmlElement* chElem = listElement->getChildByName("SplitPoint");
	if (!chElem) return;

	pianoController.SetSplitPoint(chElem->getAllSubText().getIntValue());
}

// The accompaniment: the style, the tempo, the chord detection, its split point and its
// mixer (the whole accompaniment and its parts).
void RegistrationMemory::SaveStyle()
{
	const PianoController::StyleState state = pianoController.GetStyleState();
	XmlElement* styleElem = root->createNewChildElement("Style");
	if (state.style.isNotEmpty())
	{
		styleElem->createNewChildElement("Path")->addTextElement(state.style);
	}
	if (state.tempo > 0)
	{
		styleElem->createNewChildElement("Tempo")->addTextElement(String(state.tempo));
	}
	if (state.chordArea == PianoController::caLower || state.chordArea == PianoController::caFull)
	{
		styleElem->createNewChildElement("ChordDetection")->addTextElement(
			state.chordArea == PianoController::caFull ? "full" : "lower");
	}
	if (state.leftSound >= 0)
	{
		styleElem->createNewChildElement("MainVoiceBelow")->addTextElement(state.leftSound == 1 ? "yes" : "no");
	}
	if (state.splitPoint > 0)
	{
		styleElem->createNewChildElement("SplitPoint")->addTextElement(String(state.splitPoint));
	}
	if (options.key)
	{
		// the key chosen in the Accompaniment window (empty: no key); a setting of the
		// program, which keeps it in its settings file, too
		XmlElement* keyElem = styleElem->createNewChildElement("Key");
		keyElem->setAttribute("mode", settings.accompanimentMinor ? "minor" : "major");
		keyElem->addTextElement(settings.accompanimentKey);
	}
	if (state.hasMixer)
	{
		XmlElement* mixerElem = styleElem->createNewChildElement("Mixer");
		mixerElem->setAttribute("volume", state.volume);
		mixerElem->setAttribute("pan", state.pan);
		mixerElem->setAttribute("reverb", state.reverb);
		for (int i = 0; i < PianoController::NumStyleParts; i++)
		{
			XmlElement* partElem = mixerElem->createNewChildElement("Part");
			partElem->setAttribute("number", i + 1);
			partElem->setAttribute("name", PianoController::StylePartName(i));
			partElem->setAttribute("active", state.parts[i].active ? "yes" : "no");
			partElem->setAttribute("volume", state.parts[i].volume);
			partElem->setAttribute("pan", state.parts[i].pan);
			partElem->setAttribute("reverb", state.parts[i].reverb);
		}
	}
}

void RegistrationMemory::LoadStyle()
{
	XmlElement* styleElem = root->getChildByName("Style");
	if (!styleElem) return; // saved by an older version

	PianoController::StyleState state;
	XmlElement* el;
	if (options.key && (el = styleElem->getChildByName("Key")))
	{
		const bool minor = el->getStringAttribute("mode").equalsIgnoreCase("minor");
		const String key = el->getAllSubText().trim();
		bool valid = key.isEmpty();
		for (int i = 0; !valid && i < Settings::NumKeys; i++)
		{
			valid = key == Settings::KeyName(i, minor);
		}
		if (valid && (key != settings.accompanimentKey || minor != settings.accompanimentMinor))
		{
			// the Accompaniment window shows it when the controller notifies (below)
			settings.accompanimentKey = key;
			settings.accompanimentMinor = minor;
			settings.Save();
		}
	}
	if ((el = styleElem->getChildByName("Path")))
	{
		state.style = el->getAllSubText().trim();
	}
	if ((el = styleElem->getChildByName("Tempo")))
	{
		const int tempo = el->getAllSubText().getIntValue();
		state.tempo = tempo > 0 ? jlimit((int)PianoController::MinTempo, (int)PianoController::MaxTempo, tempo) : 0;
	}
	if ((el = styleElem->getChildByName("ChordDetection")))
	{
		const String value = el->getAllSubText().trim();
		state.chordArea = value.equalsIgnoreCase("full") ? PianoController::caFull :
			value.equalsIgnoreCase("lower") ? PianoController::caLower : PianoController::caUnknown;
	}
	if ((el = styleElem->getChildByName("MainVoiceBelow")))
	{
		state.leftSound = el->getAllSubText().trim().equalsIgnoreCase("yes") ? 1 : 0;
	}
	if ((el = styleElem->getChildByName("SplitPoint")))
	{
		const int splitPoint = el->getAllSubText().getIntValue();
		state.splitPoint = splitPoint > 0 && splitPoint < 128 ? splitPoint : 0;
	}
	if (XmlElement* mixerElem = styleElem->getChildByName("Mixer"))
	{
		state.hasMixer = true;
		state.volume = jlimit(0, 127, mixerElem->getIntAttribute("volume", PianoController::DefaultVolume));
		state.pan = jlimit(-64, 63, mixerElem->getIntAttribute("pan", PianoController::DefaultPan));
		state.reverb = jlimit(0, 127, mixerElem->getIntAttribute("reverb", PianoController::DefaultReverb));
		for (XmlElement* partElem : mixerElem->getChildWithTagNameIterator("Part"))
		{
			const int i = partElem->getIntAttribute("number") - 1;
			if (i < 0 || i >= PianoController::NumStyleParts) continue;
			state.parts[i].active = !partElem->getStringAttribute("active", "yes").equalsIgnoreCase("no");
			state.parts[i].volume = jlimit(0, 127, partElem->getIntAttribute("volume", PianoController::DefaultVolume));
			state.parts[i].pan = jlimit(-64, 63, partElem->getIntAttribute("pan", PianoController::DefaultPan));
			state.parts[i].reverb = jlimit(0, 127, partElem->getIntAttribute("reverb", PianoController::DefaultReverb));
		}
	}
	pianoController.RestoreStyleState(state);
}

void RegistrationMemory::SaveReverbEffect()
{
	XmlElement* listElement = root->getChildByName("PianoRoom");
	if (!listElement)
	{
		listElement = root->createNewChildElement("PianoRoom");
	}
	listElement->createNewChildElement("ReverbEffect")->addTextElement(String(pianoController.GetReverbEffect()));
}

void RegistrationMemory::LoadReverbEffect()
{
	XmlElement* listElement = root->getChildByName("PianoRoom");
	if (!listElement) return;

	XmlElement* chElem = listElement->getChildByName("ReverbEffect");
	if (!chElem) return;

	pianoController.SetReverbEffect(chElem->getAllSubText().getIntValue());
}

void RegistrationMemory::SavePianoRoom()
{
	XmlElement* listElement = root->getChildByName("PianoRoom");
	if (!listElement)
	{
		listElement = root->createNewChildElement("PianoRoom");
	}
	listElement->createNewChildElement("LidPosition")->addTextElement(String(
		pianoController.GetLidPosition() == PianoController::lpOpen ? "open" :
		pianoController.GetLidPosition() == PianoController::lpHalf ? "half" :
		pianoController.GetLidPosition() == PianoController::lpClose ? "close" : ""));
	listElement->createNewChildElement("Environment")->addTextElement(String(pianoController.GetEnvironment()));
	listElement->createNewChildElement("Brightness")->addTextElement(String(pianoController.GetBrightness()));
	listElement->createNewChildElement("TouchCurve")->addTextElement(String(
		pianoController.GetFixedCurve(PianoController::chMain) ? "fixed" :
		pianoController.GetTouchCurve() == PianoController::tcSoft2 ? "soft2" :
		pianoController.GetTouchCurve() == PianoController::tcSoft1 ? "soft1" :
		pianoController.GetTouchCurve() == PianoController::tcMedium ? "medium" :
		pianoController.GetTouchCurve() == PianoController::tcHard1 ? "hard1" :
		pianoController.GetTouchCurve() == PianoController::tcHard2 ? "hard2" : ""));
	listElement->createNewChildElement("FixedVelocity")->addTextElement(String(pianoController.GetFixedVelocity()));
	listElement->createNewChildElement("MasterTune")->addTextElement(String(440.0 + pianoController.GetMasterTune()/10.0));
	listElement->createNewChildElement("Transpose")->addTextElement(String(pianoController.GetKeyboardTranspose()));
	listElement->createNewChildElement("Vrm")->addTextElement(String(pianoController.GetVrm() ? "yes" : "no"));
	listElement->createNewChildElement("DamperResonance")->addTextElement(String(pianoController.GetDamperResonance()));
	listElement->createNewChildElement("StringResonance")->addTextElement(String(pianoController.GetStringResonance()));
	listElement->createNewChildElement("KeyOffSampling")->addTextElement(String(pianoController.GetKeyOffSampling()));
}

void RegistrationMemory::LoadPianoRoom()
{
	XmlElement* listElement = root->getChildByName("PianoRoom");
	if (!listElement) return;

	XmlElement* el;
	if ((el = listElement->getChildByName("LidPosition")))
	{
		String value = el->getAllSubText();
		pianoController.SetLidPosition(value.equalsIgnoreCase("open") ? PianoController::lpOpen :
			value.equalsIgnoreCase("half") ? PianoController::lpHalf : PianoController::lpClose);
	}
	if ((el = listElement->getChildByName("Environment")))
	{
		int value = el->getAllSubText().getIntValue();
		pianoController.SetEnvironment(value);
	}
	if ((el = listElement->getChildByName("Brightness")))
	{
		int value = el->getAllSubText().getIntValue();
		pianoController.SetBrightness(value);
	}
	if ((el = listElement->getChildByName("TouchCurve")))
	{
		String value = el->getAllSubText();
		if (value.equalsIgnoreCase("fixed"))
		{
			pianoController.SetFixedCurve(PianoController::chMain, true);
			pianoController.SetFixedCurve(PianoController::chLeft, true);
			pianoController.SetFixedCurve(PianoController::chLayer, true);
		}
		else
		{
			PianoController::TouchCurve touchCurve =
				value.equalsIgnoreCase("soft2") ? PianoController::tcSoft2 :
				value.equalsIgnoreCase("soft1") ? PianoController::tcSoft1 :
				value.equalsIgnoreCase("hard1") ? PianoController::tcHard1 :
				value.equalsIgnoreCase("hard2") ? PianoController::tcHard2 :
				PianoController::tcMedium;
			pianoController.SetTouchCurve(touchCurve);
			pianoController.SetFixedCurve(PianoController::chMain, false);
			pianoController.SetFixedCurve(PianoController::chLeft, false);
			pianoController.SetFixedCurve(PianoController::chLayer, false);
		}
	}
	if ((el = listElement->getChildByName("FixedVelocity")))
	{
		int value = el->getAllSubText().getIntValue();
		pianoController.SetFixedVelocity(value);
	}
	if ((el = listElement->getChildByName("MasterTune")))
	{
		// round, do not truncate: e.g. 442.3 Hz must give 23, not 22
		int value = roundToInt((el->getAllSubText().getDoubleValue() - 440.0) * 10.0);
		pianoController.SetMasterTune(value);
	}
	if ((el = listElement->getChildByName("Transpose")))
	{
		// transposition of the piano's keyboard (not of the song)
		pianoController.SetKeyboardTranspose(el->getAllSubText().getIntValue());
	}
	if ((el = listElement->getChildByName("Vrm")))
	{
		String value = el->getAllSubText();
		pianoController.SetVrm(value.equalsIgnoreCase("yes"));
	}
	if ((el = listElement->getChildByName("DamperResonance")))
	{
		int value = el->getAllSubText().getIntValue();
		pianoController.SetDamperResonance(value);
	}
	if ((el = listElement->getChildByName("StringResonance")))
	{
		int value = el->getAllSubText().getIntValue();
		pianoController.SetStringResonance(value);
	}
	if ((el = listElement->getChildByName("KeyOffSampling")))
	{
		int value = el->getAllSubText().getIntValue();
		pianoController.SetKeyOffSampling(value);
	}
}

void RegistrationMemory::SavePlayback()
{
	XmlElement* listElement = root->getChildByName("Playback");
	if (!listElement)
	{
		listElement = root->createNewChildElement("Playback");
	}
	listElement->createNewChildElement("StreamLights")->addTextElement(pianoController.GetStreamLights() ? "yes" : "no");
	listElement->createNewChildElement("StreamLightsSpeed")->addTextElement(pianoController.GetStreamFast() ? "fast" : "slow");
	listElement->createNewChildElement("Guide")->addTextElement(pianoController.GetGuide() ? "yes" : "no");
	listElement->createNewChildElement("GuideType")->addTextElement(
		pianoController.GetGuideType() == PianoController::gtAnyKey ? "any-key" :
		pianoController.GetGuideType() == PianoController::gtYourTempo ? "your-tempo" :
		"correct-key");
	listElement->createNewChildElement("Tempo")->addTextElement(String(pianoController.GetTempo()));
	// the speed of ConPianist's own player relative to the song's own tempo (percent, and
	// exactly; 100 with the piano's player); the tempo above is the tempo at the saved position
	listElement->createNewChildElement("Speed")->addTextElement(String(pianoController.GetSpeed()));
	listElement->createNewChildElement("SpeedFactor")->addTextElement(String(pianoController.GetSpeedFactor(), 6));
	listElement->createNewChildElement("Transpose")->addTextElement(String(pianoController.GetTranspose()));
	listElement->createNewChildElement("RightChannel")->addTextElement(
		String(pianoController.GetPartChannel(PianoController::paRight) - PianoController::chMidi0));
	listElement->createNewChildElement("LeftChannel")->addTextElement(
		String(pianoController.GetPartChannel(PianoController::paLeft) - PianoController::chMidi0));
	listElement->createNewChildElement("Right")->addTextElement(pianoController.GetPart(PianoController::paRight) ? "yes" : "no");
	listElement->createNewChildElement("Left")->addTextElement(pianoController.GetPart(PianoController::paLeft) ? "yes" : "no");
	listElement->createNewChildElement("Backing")->addTextElement(pianoController.GetPart(PianoController::paBacking) ? "yes" : "no");
	listElement->createNewChildElement("Position")->addTextElement(
		String(pianoController.GetPosition().measure) + "," + String(pianoController.GetPosition().beat));
	PianoController::Loop loop = pianoController.GetLoop();
	listElement->createNewChildElement("Loop")->addTextElement(
		String(loop.begin.measure) + "," + String(loop.begin.beat) + ":" +
		String(loop.end.measure) + "," + String(loop.end.beat));
}

void RegistrationMemory::LoadPlayback()
{
	XmlElement* listElement = root->getChildByName("Playback");
	if (!listElement) return;

	XmlElement* el;
	if ((el = listElement->getChildByName("StreamLights")))
	{
		String value = el->getAllSubText();
		pianoController.SetStreamLights(value.equalsIgnoreCase("yes"));
	}
	if ((el = listElement->getChildByName("StreamLightsSpeed")))
	{
		String value = el->getAllSubText();
		pianoController.SetStreamFast(value.equalsIgnoreCase("fast"));
	}
	if ((el = listElement->getChildByName("Guide")))
	{
		String value = el->getAllSubText();
		pianoController.SetGuide(value.equalsIgnoreCase("yes"));
	}
	if ((el = listElement->getChildByName("GuideType")))
	{
		String value = el->getAllSubText();
		pianoController.SetGuideType(value.equalsIgnoreCase("any-key") ? PianoController::gtAnyKey :
			value.equalsIgnoreCase("your-tempo") ? PianoController::gtYourTempo :
			PianoController::gtCorrectKey);
	}
	if ((el = listElement->getChildByName("Tempo")))
	{
		int value = el->getAllSubText().getIntValue();
		pianoController.SetTempo(value);
	}
	// with ConPianist's own player the tempo and the speed are restored exactly (the tempo
	// above is only the tempo at the saved position); older files have neither
	if ((el = listElement->getChildByName("Speed")) && pianoController.IsLocalPlayback())
	{
		const int speed = el->getAllSubText().getIntValue();
		XmlElement* factorElement = listElement->getChildByName("SpeedFactor");
		const double factor = factorElement ? factorElement->getAllSubText().getDoubleValue() : 0.0;
		if (factor > 0)
		{
			pianoController.SetSpeedFactor(factor);
		}
		else if (speed > 0)
		{
			pianoController.SetSpeedFactor(speed / 100.0);
		}
	}
	if ((el = listElement->getChildByName("Transpose")))
	{
		int value = el->getAllSubText().getIntValue();
		pianoController.SetTranspose(value);
	}
	if ((el = listElement->getChildByName("RightChannel")))
	{
		int value = el->getAllSubText().getIntValue();
		pianoController.SetPartChannel(PianoController::paRight, PianoController::Channel(value + PianoController::chMidi0));
	}
	if ((el = listElement->getChildByName("LeftChannel")))
	{
		int value = el->getAllSubText().getIntValue();
		pianoController.SetPartChannel(PianoController::paLeft, PianoController::Channel(value + PianoController::chMidi0));
	}
	if ((el = listElement->getChildByName("Right")))
	{
		String value = el->getAllSubText();
		pianoController.SetPart(PianoController::paRight, value.equalsIgnoreCase("yes"));
	}
	if ((el = listElement->getChildByName("Left")))
	{
		String value = el->getAllSubText();
		pianoController.SetPart(PianoController::paLeft, value.equalsIgnoreCase("yes"));
	}
	if ((el = listElement->getChildByName("Backing")))
	{
		String value = el->getAllSubText();
		pianoController.SetPart(PianoController::paBacking, value.equalsIgnoreCase("yes"));
	}
	if ((el = listElement->getChildByName("Loop")))
	{
		String value = el->getAllSubText();
		int delim = value.indexOf(":");
		if (delim > -1)
		{
			String begin = value.substring(0, delim);
			String end = value.substring(delim + 1);

			delim = begin.indexOf(",");
			if (delim > -1)
			{
				PianoController::Loop loop;
				loop.begin.measure = begin.substring(0, delim).getIntValue();
				loop.begin.beat = begin.substring(delim + 1).getIntValue();

				delim = end.indexOf(",");
				if (delim > -1)
				{
					loop.end.measure = end.substring(0, delim).getIntValue();
					loop.end.beat = end.substring(delim + 1).getIntValue();

					if (loop.begin.measure == 0 && loop.begin.beat == 0 &&
						loop.end.measure == 0 && loop.end.beat == 0)
					{
						pianoController.ResetLoop();
					}
					else
					{
						pianoController.SetLoop(loop);
					}
				}
			}
		}
	}
	if ((el = listElement->getChildByName("Position")))
	{
		String value = el->getAllSubText();
		int delim = value.indexOf(",");
		if (delim > -1)
		{
			int measure = value.substring(0, delim).getIntValue();
			int beat = value.substring(delim + 1).getIntValue();
			pianoController.SetPosition({measure, beat});
		}
	}
}

// The Live Play octaves of the Mixer channels and the settings of the Live Play channels
// not used in the song. They are separate from the mixer channels and from the other
// settings, so that they are restored at every start (also without a song).
void RegistrationMemory::SaveLiveChannels()
{
	// the Live Play octaves of the Mixer channels 1..16
	StringArray octaves;
	for (PianoController::Channel ch : PianoController::MidiChannels)
	{
		octaves.add(String(pianoController.GetLiveOctave(ch)));
	}
	root->createNewChildElement("LiveOctaves")->addTextElement(octaves.joinIntoString(","));

	// the settings of the Live Play channels not used in the song (also of the channels
	// not used for Live Play at the moment: they are kept for the next time)
	XmlElement* liveElem = root->createNewChildElement("LiveChannels");
	for (PianoController::Channel ch : PianoController::MidiChannels)
	{
		const PianoController::LiveChannelState state = pianoController.GetLiveChannelState(ch);
		if (!state.set)
		{
			continue;
		}
		XmlElement* chElem = liveElem->createNewChildElement("Channel");
		chElem->setAttribute("number", ch - PianoController::chMidi0);
		chElem->setAttribute("voice", state.voice);
		if (state.gmVoice)
		{
			chElem->setAttribute("set", "gm");
		}
		chElem->setAttribute("title", state.gmVoice ?
			Presets::GmVoiceTitle(String(state.voice), ch == PianoController::chMidi10) :
			Presets::VoiceTitle(String(state.voice)));
		chElem->setAttribute("volume", state.volume);
		chElem->setAttribute("pan", state.pan);
		chElem->setAttribute("reverb", state.reverb);
	}
}

void RegistrationMemory::LoadLiveChannels()
{
	// older files: inside Settings/Keyboard
	XmlElement* settingsElem = root->getChildByName("Settings");
	XmlElement* oldElem = settingsElem ? settingsElem->getChildByName("Keyboard") : nullptr;
	XmlElement* liveElem = root->getChildByName("LiveOctaves") || root->getChildByName("LiveChannels") ? root : oldElem;
	if (!liveElem) return;

	XmlElement* el;
	if ((el = liveElem->getChildByName("LiveOctaves")))
	{
		const StringArray octaves = StringArray::fromTokens(el->getAllSubText(), ",", "");
		for (int i = 0; i < octaves.size() && i < 16; i++)
		{
			pianoController.SetLiveOctave(PianoController::Channel(PianoController::chMidi1 + i),
				octaves[i].trim().getIntValue());
		}
	}
	if ((el = liveElem->getChildByName("LiveChannels")))
	{
		for (auto* chElem : el->getChildWithTagNameIterator("Channel"))
		{
			const int number = chElem->getIntAttribute("number");
			if (number < 1 || number > 16)
			{
				continue;
			}
			PianoController::LiveChannelState state;
			state.set = true;
			state.voice = chElem->getIntAttribute("voice");
			state.gmVoice = chElem->getStringAttribute("set").equalsIgnoreCase("gm");
			state.volume = jlimit(0, 127, chElem->getIntAttribute("volume", PianoController::DefaultVolume));
			state.pan = jlimit(-64, 63, chElem->getIntAttribute("pan", PianoController::DefaultPan));
			state.reverb = jlimit(0, 127, chElem->getIntAttribute("reverb", PianoController::DefaultReverb));
			pianoController.SetLiveChannelState(PianoController::Channel(PianoController::chMidi0 + number), state);
		}
	}
}

void RegistrationMemory::SaveSettings()
{
	XmlElement* listElement = root->getChildByName("Settings");
	if (!listElement)
	{
		listElement = root->createNewChildElement("Settings");
	}

	XmlElement* elem = listElement->createNewChildElement("Keyboard");
	elem->createNewChildElement("Channel")->addTextElement(String(settings.FirstKeyboardChannel())); // older versions
	StringArray channels;
	for (int channel = 1; channel <= 16; channel++)
	{
		if (settings.IsKeyboardChannel(channel)) channels.add(String(channel));
	}
	elem->createNewChildElement("Channels")->addTextElement(channels.joinIntoString(","));
	elem->createNewChildElement("LivePlay")->addTextElement(settings.livePlayOnPiano ? "piano" : "mixer");

	elem = listElement->createNewChildElement("Score");
	elem->createNewChildElement("InstrumentNames")->addTextElement(String(
		settings.scoreInstrumentNames == Settings::siHidden ? "hidden" :
		settings.scoreInstrumentNames == Settings::siShort ? "short" :
		settings.scoreInstrumentNames == Settings::siMixed ? "mixed" :
		"full"));
	elem->createNewChildElement("Part")->addTextElement(String(
		settings.scorePart == Settings::spRight ? "right" :
		settings.scorePart == Settings::spLeft ? "left" :
		settings.scorePart == Settings::spRightAndLeft ? "right-and-left" :
		"all"));
	elem->createNewChildElement("ShowMidiChannel")->addTextElement(settings.scoreShowMidiChannel ? "yes" : "no");
}

void RegistrationMemory::LoadSettings()
{
	XmlElement* listElement = root->getChildByName("Settings");
	if (!listElement) return;

	XmlElement* el;

	XmlElement* keyElem = listElement->getChildByName("Keyboard");
	if (keyElem)
	{
		// The visibility of the keyboard is not part of the registration memory (older
		// files may contain it): it stays as the user has set it, also when a song and
		// its registration memory are loaded automatically.
		if ((el = keyElem->getChildByName("Channel")))
		{
			int value = el->getAllSubText().getIntValue();
			if (value >= 1 && value <= 16) settings.keyboardChannels = 1 << (value - 1);
		}
		if ((el = keyElem->getChildByName("Channels")))
		{
			// several Live Play channels (newer files)
			int channels = 0;
			for (const String& item : StringArray::fromTokens(el->getAllSubText(), ",", ""))
			{
				const int channel = item.trim().getIntValue();
				if (channel >= 1 && channel <= 16) channels |= 1 << (channel - 1);
			}
			if (channels != 0) settings.keyboardChannels = channels;
		}
		if ((el = keyElem->getChildByName("LivePlay")))
		{
			// Live Play on the piano's own keyboard parts or on the Mixer channels
			settings.livePlayOnPiano = !el->getAllSubText().trim().equalsIgnoreCase("mixer");
		}
	}

	XmlElement* scoreElem = listElement->getChildByName("Score");
	if (scoreElem)
	{
		if ((el = scoreElem->getChildByName("InstrumentNames")))
		{
			String value = el->getAllSubText();
			settings.scoreInstrumentNames =
				value.equalsIgnoreCase("hidden") ? Settings::siHidden :
				value.equalsIgnoreCase("short") ? Settings::siShort :
				value.equalsIgnoreCase("mixed") ? Settings::siMixed :
				Settings::siFull;
		}
		if ((el = scoreElem->getChildByName("Part")))
		{
			String value = el->getAllSubText();
			settings.scorePart =
				value.equalsIgnoreCase("right") ? Settings::spRight :
				value.equalsIgnoreCase("left") ? Settings::spLeft :
				value.equalsIgnoreCase("right-and-left") ? Settings::spRightAndLeft :
				Settings::spAll;
		}
		if ((el = scoreElem->getChildByName("ShowMidiChannel")))
		{
			String value = el->getAllSubText();
			settings.scoreShowMidiChannel = value.equalsIgnoreCase("yes");
		}
	}

	settings.sendChangeMessage();
}
