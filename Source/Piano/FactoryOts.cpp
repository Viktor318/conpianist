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

#include "FactoryOts.h"
#include "Presets.h"

// Voices of the style files that are not on the CSP-170 with the same number, and the
// panel voice of the CSP-170 used for them (0x00MMLLPP; generated from
// docs/Gyari_OTS_hangszinek.xlsx, October 17, 2026).
static const std::pair<int, int> VoiceTable[] = {
	{0x000000, 0x6c0000}, // decision: GrandPiano -> CFX Grand
	{0x000008, 0x007008}, // name: Celesta -> Celesta
	{0x00000f, 0x00700f}, // name: Dulcimer -> Dulcimer
	{0x006a51, 0x68053e}, // similar: RS SawLead2 -> Brassy Lead
	{0x007001, 0x6c1001}, // name: BrightPiano -> Bright Piano
	{0x007003, 0x680d03}, // similar: HonkyTonk -> Honky Tonk Upright
	{0x007009, 0x680009}, // name: Glockenspiel -> Glockenspiel
	{0x00700b, 0x68010b}, // name: Vibraphone -> Vibraphone
	{0x00700c, 0x68000c}, // name: Marimba -> Marimba
	{0x00700d, 0x68000d}, // name: Xylophone -> Xylophone
	{0x007015, 0x007615}, // similar: MusetteAccordion -> Master Accordion
	{0x00702d, 0x00712d}, // name: Pizzicato -> Pizzicato
	{0x007030, 0x007531}, // name: Strings -> Strings
	{0x00703b, 0x00723b}, // name: MutedTrumpet -> Muted Trumpet
	{0x00703c, 0x08213c}, // similar: FrenchHorns -> Warm Horns
	{0x00703e, 0x00743f}, // similar: SynthBrass -> Fat Synth Brass
	{0x007043, 0x680043}, // name: BaritoneSax -> Baritone Sax
	{0x007050, 0x680050}, // similar: SquareLead -> Sub Lead
	{0x007053, 0x007651}, // similar: AeroLead -> Early Lead
	{0x007100, 0x6c0000}, // decision: GrandPiano -> CFX Grand
	{0x007103, 0x6c1f00}, // similar: OctavePiano1 -> Stage Piano
	{0x007115, 0x007217}, // similar: TuttiAccordion -> Tango Accordion
	{0x007130, 0x680631}, // similar: OrchestraStrings2 -> Real Strings p
	{0x007136, 0x007434}, // similar: Voices -> Gospel Voices
	{0x007203, 0x6c1f00}, // similar: OctavePiano2 -> Stage Piano
	{0x007241, 0x082042}, // name: AltoSax -> Alto Sax
	{0x007300, 0x6c0000}, // decision: ConcertGrand -> CFX Grand
	{0x007605, 0x083503}, // similar: JazzChorus -> Light Chorus
	{0x00773c, 0x68023c}, // similar: SymphonyBrass -> Symphony Horns 2
	{0x007951, 0x680f51}, // similar: FunkyLead -> Fusion Lead
	{0x007a00, 0x6c0000}, // decision: ConcertGrand -> CFX Grand
	{0x007a50, 0x007654}, // similar: PanLead -> Hipa Lead
	{0x007d51, 0x082340}, // similar: TrumpetSaw -> Trumpet Shake
	{0x082114, 0x007007}, // similar: Clavi JS -> Clavi Bright
	{0x082127, 0x680835}, // similar: BoysHaaVibrato -> Boys Choir Ooh
	{0x082128, 0x680835}, // similar: OohVibrato -> Boys Choir Ooh
	{0x082528, 0x680835}, // similar: GirlsOohVibrato -> Boys Choir Ooh
	{0x082803, 0x082903}, // name: VintageAmp -> Vintage Amp
	{0x082806, 0x082603}, // similar: SolidJazzGuitar -> 70s Solid Guitar
	{0x082904, 0x082c03}, // similar: CountryPickVintage -> Country Pick
	{0x083805, 0x68041b}, // similar: WahWahGuitar -> Cool Wah Guitar
	{0x083903, 0x082001}, // similar: SixtiesGuitar -> Steel Guitar
	{0x084030, 0x007128}, // similar: JazzViolin -> Violin
	{0x084041, 0x007338}, // similar: SoftTrumpet -> Trumpet
	{0x084050, 0x007542}, // name: BreathyTenorSax -> Breathy Tenor Sax
	{0x084054, 0x007140}, // similar: PopSopranoSax -> Soprano Sax
	{0x08405c, 0x007247}, // similar: BalladClarinet -> Jazz Clarinet
	{0x084068, 0x007016}, // name: Harmonica -> Harmonica
	{0x084130, 0x007028}, // similar: CelticViolin -> Solo Violin
	{0x084141, 0x082140}, // similar: ClassicTrumpet -> Silver Trumpet
	{0x084150, 0x007f42}, // similar: SmoothTenorSax -> Pop Tenor Sax
	{0x084154, 0x007140}, // similar: BalladSopranoSax -> Soprano Sax
	{0x08415c, 0x007047}, // name: Clarinet -> Clarinet
	{0x084168, 0x007016}, // similar: BluesHarmonica -> Harmonica
	{0x084241, 0x007338}, // name: Trumpet -> Trumpet
	{0x680002, 0x680202}, // name: MIDI Grand -> MIDI Grand
	{0x680003, 0x6c1c03}, // name: CocktailPiano -> Cocktail Piano
	{0x680044, 0x082044}, // name: OrchestralOboe -> Orchestral Oboe
	{0x680049, 0x082049}, // name: OrchestralFlute -> Orchestral Flute
	{0x680103, 0x6c1c03}, // name: CocktailPiano -> Cocktail Piano
	{0x680203, 0x6c1f00}, // similar: OctavePiano1 -> Stage Piano
	{0x680300, 0x6c1b03}, // name: AmbientPiano -> Ambient Piano
	{0x680303, 0x6c1f00}, // similar: OctavePiano2 -> Stage Piano
	{0x680400, 0x6c1002}, // name: RockPiano -> Rock Piano
	{0x680500, 0x6c1101}, // name: PopGrand -> Pop Grand
	{0x680704, 0x007604}, // similar: SuitcaseSoft -> Suitcase EP
	{0x680804, 0x007404}, // name: VintageEP -> Vintage EP
	{0x680900, 0x6c1b03}, // name: AmbientPiano -> Ambient Piano
	{0x680a00, 0x6c1002}, // name: RockPiano -> Rock Piano
	{0x680b00, 0x6c1101}, // name: PopGrand -> Pop Grand
	{0x680b04, 0x007604}, // similar: SuitcaseDrive -> Suitcase EP
	{0x680b19, 0x00701b}, // similar: ChickenPickGuitar -> Clean Guitar
	{0x680d00, 0x6c0502}, // name: UprightPiano -> Upright Piano
};

static const char* const StyleExtensions[] = {"prs", "sst", "sty", "fps", "scp"};

const StringArray& FactoryOts::Sources()
{
	static const StringArray sources{"PSR-SX600", "PSR-S970"};
	return sources;
}

String FactoryOts::NameKey(const String& name)
{
	const String lower = name.toLowerCase().replace("&", "and");
	String key;
	for (const juce_wchar c : lower)
	{
		if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))
		{
			key << String::charToString(c);
		}
	}
	return key;
}

String FactoryOts::CspVoice(int yamahaVoice)
{
	int number = yamahaVoice;
	for (int pass = 0; pass < 2; pass++)
	{
		for (const Voice& voice : Presets::Voices())
		{
			if (voice.num == number)
			{
				return voice.path;
			}
		}
		// not with the same number: from the table
		number = -1;
		for (const auto& entry : VoiceTable)
		{
			if (entry.first == yamahaVoice)
			{
				number = entry.second;
				break;
			}
		}
		if (number < 0)
		{
			break;
		}
	}
	return {};
}

//==============================================================================
// Reading the style files

namespace
{
	// A chunk of a Standard MIDI File or of the Yamaha chunks after it.
	struct Chunk
	{
		String tag;
		const uint8* data;
		int size;
	};

	std::vector<Chunk> ReadChunks(const uint8* data, int size)
	{
		std::vector<Chunk> chunks;
		int pos = 0;
		while (pos + 8 <= size)
		{
			const String tag = String::createStringFromData(data + pos, 4);
			const int length = (int)(((uint32)data[pos + 4] << 24) | ((uint32)data[pos + 5] << 16) |
				((uint32)data[pos + 6] << 8) | (uint32)data[pos + 7]);
			if (length < 0 || pos + 8 + length > size)
			{
				break;
			}
			chunks.push_back({tag, data + pos + 8, length});
			pos += 8 + length;
		}
		return chunks;
	}

	bool ReadVlq(const uint8* data, int size, int& pos, int& value)
	{
		value = 0;
		for (int i = 0; i < 4 && pos < size; i++)
		{
			const uint8 c = data[pos++];
			value = (value << 7) | (c & 0x7f);
			if ((c & 0x80) == 0)
			{
				return true;
			}
		}
		return false;
	}

	// The values of a MIDI channel in an OTS track (-1: not set).
	struct ChannelValues
	{
		int msb = -1, lsb = -1, program = -1, volume = -1;
		int on = -1, octave = -1, partVolume = -1;
		int pan = -1, reverb = -1, chorus = -1; // Control Change 10, 91, 93
		int Voice() const { return program < 0 ? -1 : (jmax(0, msb) << 16) | (jmax(0, lsb) << 8) | program; }
	};

	// The channels of one OTS (one track of the OTSc chunk).
	void ReadOtsTrack(const uint8* data, int size, ChannelValues channels[16])
	{
		int pos = 0;
		uint8 status = 0;
		while (pos < size)
		{
			int delta;
			if (!ReadVlq(data, size, pos, delta) || pos >= size)
			{
				return;
			}
			const uint8 byte = data[pos];
			if (byte == 0xff)
			{
				// meta event
				pos += 2;
				int length;
				if (pos > size || !ReadVlq(data, size, pos, length))
				{
					return;
				}
				pos += length;
				continue;
			}
			if (byte == 0xf0 || byte == 0xf7)
			{
				pos++;
				int length;
				if (!ReadVlq(data, size, pos, length) || pos + length > size)
				{
					return;
				}
				// F0 43 73 01 50 08 cc pp vv F7: a value of part pp of channel cc
				const uint8* sysex = data + pos;
				if (byte == 0xf0 && length >= 8 && sysex[0] == 0x43 && sysex[1] == 0x73 &&
					sysex[2] == 0x01 && sysex[3] == 0x50 && sysex[4] == 0x08 && sysex[5] < 16)
				{
					ChannelValues& ch = channels[sysex[5]];
					const int value = sysex[7];
					switch (sysex[6])
					{
						case 0x00: ch.on = value; break;
						case 0x03: ch.octave = value; break;
						case 0x04: ch.partVolume = value; break;
						default: break;
					}
				}
				pos += length;
				continue;
			}
			if (byte & 0x80)
			{
				status = byte;
				pos++;
			}
			if ((status & 0x80) == 0)
			{
				return; // running status without a status
			}
			const int type = status & 0xf0;
			const int length = (type == 0xc0 || type == 0xd0) ? 1 : 2;
			if (pos + length > size)
			{
				return;
			}
			ChannelValues& ch = channels[status & 0x0f];
			if (type == 0xb0)
			{
				const int controller = data[pos], value = data[pos + 1];
				if (controller == 0) ch.msb = value;
				else if (controller == 32) ch.lsb = value;
				else if (controller == 7) ch.volume = value;
				else if (controller == 10) ch.pan = value;
				else if (controller == 91) ch.reverb = value;
				else if (controller == 93) ch.chorus = value;
			}
			else if (type == 0xc0)
			{
				ch.program = data[pos];
			}
			pos += length;
		}
	}

	String VoiceNumber(int voice)
	{
		return String(voice >> 16) + "/" + String((voice >> 8) & 0x7f) + "/" + String((voice & 0x7f) + 1);
	}
}

bool FactoryOts::ReadStyleFile(const File& file, OneTouchSettings::Setting* settings, int& skipped, StringArray& log)
{
	MemoryBlock block;
	if (!file.loadFileAsData(block))
	{
		return false;
	}
	const std::vector<Chunk> chunks = ReadChunks((const uint8*)block.getData(), (int)block.getSize());
	const auto ots = std::find_if(chunks.begin(), chunks.end(), [](const Chunk& c) { return c.tag == "OTSc"; });
	if (ots == chunks.end())
	{
		return false;
	}
	const std::vector<Chunk> tracks = ReadChunks(ots->data, ots->size);
	int index = 0;
	for (const Chunk& track : tracks)
	{
		if (track.tag != "MTrk" || index >= OneTouchSettings::NumFactorySettings)
		{
			continue;
		}
		ChannelValues channels[16];
		ReadOtsTrack(track.data, track.size, channels);
		OneTouchSettings::Setting& setting = settings[index];
		setting = OneTouchSettings::Setting();

		// Main: Right 1 (channel 1), Layer: Right 2 (channel 2), Left: Left (channel 4)
		static const int Channels[OneTouchSettings::NumParts] = {0, 1, 3};
		String problem;
		for (int i = 0; i < OneTouchSettings::NumParts; i++)
		{
			const ChannelValues& ch = channels[Channels[i]];
			OneTouchSettings::Part& part = setting.parts[i];
			const int voice = ch.Voice();
			part.voice = voice < 0 ? String() : CspVoice(voice);
			part.active = ch.on > 0 || (i == 0 && ch.on < 0); // not set: Main on, the others off
			part.activeSet = i != 2;  // the on / off state of the Left part is not taken
			const int volume = VolumeFromPartMessage && ch.partVolume >= 0 ? ch.partVolume : ch.volume;
			part.volume = jlimit(PianoController::MinVolume, PianoController::MaxVolume,
				volume >= 0 ? volume : PianoController::DefaultVolume);
			part.octave = jlimit(PianoController::MinOctave, PianoController::MaxOctave,
				ch.octave >= 0 ? ch.octave - 0x40 : PianoController::DefaultOctave);
			// pan (64: centre), reverb and chorus, if the file has them
			part.effectsSet = ch.pan >= 0 && ch.reverb >= 0;
			part.pan = jlimit(PianoController::MinPan, PianoController::MaxPan,
				ch.pan >= 0 ? ch.pan - PianoController::PanBase : PianoController::DefaultPan);
			part.reverb = jlimit(PianoController::MinReverb, PianoController::MaxReverb,
				ch.reverb >= 0 ? ch.reverb : PianoController::DefaultReverb);
			part.chorusSet = ch.chorus >= 0;
			part.chorus = jlimit(0, 127, ch.chorus >= 0 ? ch.chorus : 0); // kept, not used
			if (part.voice.isEmpty())
			{
				// not changed by the setting
				part.valuesSet = false;
				part.effectsSet = false;
				part.chorusSet = false;
				const bool needed = i == 0 || (i == 1 && part.active);
				if (needed)
				{
					problem << (problem.isEmpty() ? "" : ", ") << OneTouchSettings::PartNames[i]
						<< " " << (voice < 0 ? String("-") : VoiceNumber(voice));
				}
			}
		}
		if (problem.isEmpty())
		{
			setting.used = true;
		}
		else
		{
			setting = OneTouchSettings::Setting();
			skipped++;
			log.add("OTS " + String(index + 1) + ": no voice on the CSP-170 for " + problem);
		}
		index++;
	}
	return index > 0;
}

FactoryOts::Result FactoryOts::Import(const File& stylesFolder, const File& factoryFolder,
	const std::function<void(double)>& progress)
{
	Result result;
	const auto report = [&progress](double done)
		{
			if (progress)
			{
				progress(jlimit(0.0, 1.0, done));
			}
		};
	report(0.0);

	// the style files by their names (the first one of a name)
	std::map<String, File> files[2];
	for (int source = 0; source < Sources().size(); source++)
	{
		const File folder = stylesFolder.getChildFile(Sources()[source]);
		if (!folder.isDirectory())
		{
			continue;
		}
		result.sourcesFound = true;
		Array<File> found = folder.findChildFiles(File::findFiles, true, "*");
		std::sort(found.begin(), found.end(), [](const File& a, const File& b)
			{
				return a.getFullPathName().compareIgnoreCase(b.getFullPathName()) < 0;
			});
		for (const File& file : found)
		{
			const String extension = file.getFileExtension().substring(1).toLowerCase();
			if (std::none_of(std::begin(StyleExtensions), std::end(StyleExtensions),
				[&extension](const char* e) { return extension == e; }))
			{
				continue;
			}
			// <name>.T<number>.<extension>
			const String name = file.getFileNameWithoutExtension();
			const String number = name.fromLastOccurrenceOf(".", false, false);
			if (!name.containsChar('.') || !number.startsWithChar('T') ||
				!number.substring(1).containsOnly("0123456789") || number.length() < 2)
			{
				continue;
			}
			const String key = NameKey(name.upToLastOccurrenceOf(".", false, false));
			if (key.isNotEmpty() && files[source].count(key) == 0)
			{
				files[source][key] = file;
			}
		}
	}
	if (!result.sourcesFound)
	{
		return result;
	}

	struct Item
	{
		String style;
		OneTouchSettings::Setting settings[OneTouchSettings::NumFactorySettings];
	};
	std::vector<Item> items;
	const int styleCount = (int)Presets::Styles().size();
	int styleIndex = 0;
	for (const Style& style : Presets::Styles())
	{
		report(0.9 * styleIndex++ / jmax(1, styleCount));
		if (style.type == "Pianist")
		{
			continue; // only on the Clavinova
		}
		const String key = NameKey(style.title);
		File file;
		for (const auto& source : files)
		{
			const auto it = source.find(key);
			if (it != source.end())
			{
				file = it->second;
				break;
			}
		}
		if (file == File())
		{
			result.missing++;
			continue;
		}
		Item item;
		item.style = style.path;
		StringArray log;
		if (!ReadStyleFile(file, item.settings, result.skipped, log))
		{
			result.details.add(style.title + " (" + file.getRelativePathFrom(stylesFolder) + "): no OTS in the file");
			continue;
		}
		for (const String& line : log)
		{
			result.details.add(style.title + " (" + file.getRelativePathFrom(stylesFolder) + "): " + line);
		}
		int used = 0;
		for (const OneTouchSettings::Setting& setting : item.settings)
		{
			used += setting.used ? 1 : 0;
		}
		if (used > 0)
		{
			result.styles++;
			result.settings += used;
			items.push_back(item);
		}
	}

	report(0.9);
	factoryFolder.deleteRecursively();
	int written = 0;
	for (const Item& item : items)
	{
		OneTouchSettings::WriteFile(OneTouchSettings::FileIn(factoryFolder, item.style), item.style,
			item.settings, OneTouchSettings::NumFactorySettings);
		report(0.9 + 0.1 * ++written / jmax(1, (int)items.size()));
	}
	report(1.0);
	return result;
}
