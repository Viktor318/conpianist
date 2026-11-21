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

#include "OneTouchSettings.h"
#include "Presets.h"

const PianoController::Channel OneTouchSettings::PartChannels[NumParts] = {
	PianoController::chMain, PianoController::chLayer, PianoController::chLeft};
const char* const OneTouchSettings::PartNames[NumParts] = {"Main", "Layer", "Left"};

static const char* const RootTag = "OneTouchSettings";
static const char* const FileExtension = ".ots";
static const char* const FactoryFolderName = "Yamaha"; // in the OTS folder
static const char* const VariantChoicesName = "FactoryVariants.xml"; // in the OTS folder

bool OneTouchSettings::Part::operator==(const Part& other) const
{
	return voice == other.voice && active == other.active &&
		volume == other.volume && octave == other.octave &&
		pan == other.pan && reverb == other.reverb; // (the chorus is not used)
}

OneTouchSettings::OneTouchSettings(const File& folder, PianoController& pianoController) :
	m_folder(folder), m_pianoController(pianoController),
	m_factoryFolder(folder.getChildFile(FactoryFolderName))
{
	ScanFolder();
	ScanFactoryFolder();
	LoadVariantChoices();
}

OneTouchSettings::~OneTouchSettings()
{
	stopTimer();
}

// A name as Windows allows it in a file name (as the files of the songs are looked
// for): without the characters < > : " / \ | ? * and without dots and spaces at the end.
static String LegalName(const String& name)
{
	String legal = name.removeCharacters("<>:\"/\\|?*").trim();
	while (legal.endsWithChar('.') || legal.endsWithChar(' '))
	{
		legal = legal.dropLastCharacters(1);
	}
	return legal.isEmpty() ? String("_") : legal;
}

// The category, the subcategory and the name of a style: from the list of the program,
// or from its preset path (PRESET:/STYLE/<category>/<subcategory>/<name>.<id>.<extension>).
static void StyleNames(const String& stylePath, String& category, String& group, String& name)
{
	if (const Style* style = Presets::FindStyle(stylePath))
	{
		category = style->category;
		group = style->group;
		name = style->title;
		return;
	}
	StringArray parts;
	parts.addTokens(stylePath.replaceCharacter('\\', '/'), "/", "");
	parts.removeEmptyStrings();
	const int styleIndex = parts.indexOf("STYLE");
	name = parts.isEmpty() ? String() : parts[parts.size() - 1].upToFirstOccurrenceOf(".", false, false);
	category = styleIndex >= 0 && styleIndex + 2 < parts.size() ? parts[styleIndex + 1] : String();
	group = styleIndex >= 0 && styleIndex + 3 < parts.size() ? parts[styleIndex + 2] : String();
}

// The style a file of settings belongs to (empty if it is not such a file).
static String StyleOfFile(const File& file)
{
	if (!file.existsAsFile())
	{
		return {};
	}
	std::unique_ptr<XmlElement> root = XmlDocument::parse(file);
	return root && root->hasTagName(RootTag) ? root->getStringAttribute("style") : String();
}

File OneTouchSettings::FileOf(const String& stylePath) const
{
	return FileIn(m_folder, stylePath);
}

File OneTouchSettings::FileIn(const File& baseFolder, const String& stylePath)
{
	String category, group, name;
	StyleNames(stylePath, category, group, name);
	File folder = baseFolder;
	if (category.isNotEmpty())
	{
		folder = folder.getChildFile(LegalName(category));
		if (group.isNotEmpty())
		{
			folder = folder.getChildFile(LegalName(group));
		}
	}
	const File file = folder.getChildFile(LegalName(name) + FileExtension);
	// two styles of the same name in a subcategory: the other one gets its number too
	const String owner = StyleOfFile(file);
	if (owner.isNotEmpty() && owner != stylePath)
	{
		const String id = stylePath.fromLastOccurrenceOf("/", false, false)
			.fromFirstOccurrenceOf(".", false, false).upToFirstOccurrenceOf(".", false, false);
		return folder.getChildFile(LegalName(name + " " + id) + FileExtension);
	}
	return file;
}

void OneTouchSettings::CreateFolders(const File& otsFolder)
{
	for (const Style& style : Presets::Styles())
	{
		File folder = otsFolder.getChildFile(LegalName(style.category));
		if (style.group.isNotEmpty())
		{
			folder = folder.getChildFile(LegalName(style.group));
		}
		if (!folder.isDirectory())
		{
			folder.createDirectory();
		}
	}
}

// The styles that have settings: the style of every file in the folder (not in the
// folder of the factory settings).
void OneTouchSettings::ScanFolder()
{
	m_stylesWithSettings.clear();
	for (const DirectoryEntry& entry : RangedDirectoryIterator(m_folder, true,
		String("*") + FileExtension, File::findFiles))
	{
		if (entry.getFile().isAChildOf(m_factoryFolder))
		{
			continue;
		}
		const String style = StyleOfFile(entry.getFile());
		if (style.isNotEmpty())
		{
			m_stylesWithSettings.insert(style);
		}
	}
}

bool OneTouchSettings::HasSettings(const String& stylePath) const
{
	return m_stylesWithSettings.count(stylePath) > 0;
}

void OneTouchSettings::ScanFactoryFolder()
{
	m_stylesWithFactory.clear();
	if (!m_factoryFolder.isDirectory())
	{
		return;
	}
	for (const DirectoryEntry& entry : RangedDirectoryIterator(m_factoryFolder, true,
		String("*") + FileExtension, File::findFiles))
	{
		const String style = StyleOfFile(entry.getFile());
		if (style.isNotEmpty())
		{
			m_stylesWithFactory.insert(style);
		}
	}
}

bool OneTouchSettings::HasFactorySettings(const String& stylePath) const
{
	return m_stylesWithFactory.count(stylePath) > 0;
}

bool OneTouchSettings::IsFactoryUsed(int index) const
{
	return index >= 0 && index < NumFactorySettings && m_factory[index].used;
}

bool OneTouchSettings::HasAnyFactory() const
{
	for (const Setting& setting : m_factory)
	{
		if (setting.used)
		{
			return true;
		}
	}
	return false;
}

void OneTouchSettings::ReloadFactory()
{
	ScanFactoryFolder();
	m_factoryActive = -1;
	LoadFactory();
	Changed();
}

// The Main section of the accompaniment (0..3: A..D): the one playing, or the one that
// comes after a fill in; -1 if none is known.
int OneTouchSettings::CurrentMain() const
{
	for (const int section : { m_pianoController.GetStyleSection(), m_pianoController.GetStyleNextSection() })
	{
		if (section >= PianoController::ssMainA && section < PianoController::ssMainA + NumFactorySettings)
		{
			return section - PianoController::ssMainA;
		}
	}
	return -1;
}

bool OneTouchSettings::IsUsed(int index) const
{
	return index >= 0 && index < NumSettings && m_settings[index].used;
}

String OneTouchSettings::VoicePath(const String& voice)
{
	Voice* preset = Presets::FindVoice(voice);
	return preset ? preset->path : voice;
}

void OneTouchSettings::Changed()
{
	sendSynchronousChangeMessage();
}

void OneTouchSettings::Update()
{
	if (!m_pianoController.IsConnected() || !m_pianoController.IsStyleKnown())
	{
		// The voices of the piano are not known: nothing is active. The style is forgotten
		// too: after connecting again (e.g. the piano was switched off and on) the style
		// the piano reports becomes known (see m_linkOnConnect).
		if (m_active >= 0 || m_factoryActive >= 0 || m_style.isNotEmpty())
		{
			m_active = -1;
			m_factoryActive = -1;
			m_recallPending = false;
			m_style.clear();
			m_main = -1;
			Changed();
		}
		return;
	}

	const String style = m_pianoController.GetStyleName();
	const String previousStyle = m_style;
	const bool styleChanged = style != m_style;
	if (styleChanged)
	{
		// another style: its own settings; the active setting belonged to the previous one
		m_style = style;
		m_active = -1;
		m_factoryActive = -1;
		m_modified = false;
		m_recallPending = false;
		Load();
		LoadFactory();
	}

	// The style has become known (the program has started, or the piano is connected
	// again): with OTS Link on, the factory setting of the Main section is recalled as soon
	// as the state of the last start has been restored (it suspends OTS Link for a while,
	// see RegistrationMemory::Load), so the voices follow the Main section from the start.
	// A style chosen meanwhile (not by the restoring) and an OTS recalled meanwhile take
	// its place.
	const bool suspended = m_pianoController.IsOtsLinkSuspended();
	if (styleChanged && previousStyle.isEmpty())
	{
		m_linkOnConnect = true;
	}
	else if ((styleChanged && !suspended) || !m_link || m_active >= 0 || m_factoryActive >= 0)
	{
		m_linkOnConnect = false;
	}

	// OTS Link: the factory setting of the Main section, when the style or the section
	// changes (when they become known: see above). The last Main section
	// known is kept while no Main section is reported: during a fill in the piano may
	// report the fill in as the section playing and as the next one for a moment, before
	// it reports the new Main section as the next one; forgetting the old one then would
	// make the new one look like the first one known, and it would not be recalled.
	const int main = CurrentMain();
	const int previousMain = m_main;
	if (main >= 0)
	{
		m_main = main;
	}
	const bool linked = styleChanged ? previousStyle.isNotEmpty() && style.isNotEmpty() :
		main >= 0 && main != previousMain && previousMain >= 0;
	if (m_link && linked && main >= 0 && !suspended && IsFactoryUsed(main))
	{
		m_linkOnConnect = false;
		RecallFactory(main);
		return;
	}
	if (m_linkOnConnect && main >= 0 && IsFactoryUsed(main))
	{
		if (!suspended)
		{
			m_linkOnConnect = false;
			Logger::writeToLog("OTS Link: factory OTS " + String(main + 1) + " recalled at the start");
			RecallFactory(main);
			return;
		}
		if (!isTimerRunning())
		{
			startTimer(250); // tried again when the restoring is over
		}
	}
	if (styleChanged)
	{
		Changed();
		return;
	}

	if (m_factoryActive >= 0)
	{
		UpdateFactoryModified();
		return;
	}
	if (m_active < 0)
	{
		return;
	}
	const bool matches = Matches(m_settings[m_active]);
	if (m_recallPending && (matches || Time::getMillisecondCounter() - m_recallTime >= (uint32)RecallSettleMs))
	{
		m_recallPending = false;
	}
	const bool modified = !m_recallPending && !matches;
	if (modified != m_modified)
	{
		m_modified = modified;
		Changed();
	}
}

// The active factory setting. When the recall has settled, the values the piano has not
// taken are logged (they make it modified too: e.g. the piano did not get the setting).
// After that, when the piano differs from the original factory setting (in memory, as
// read from its file with the style), the values of the piano become its changed version;
// when they are the original ones again, it is not modified any more.
void OneTouchSettings::UpdateFactoryModified()
{
	const int index = m_factoryActive;
	const Setting& target = GetFactory(index);
	if (m_recallPending)
	{
		if (!Matches(target) && Time::getMillisecondCounter() - m_recallTime < (uint32)RecallSettleMs)
		{
			return;
		}
		m_recallPending = false;
		LogNotTaken(index, target);
	}
	Setting changed = Follow(target);
	const Setting& original = m_factory[index];
	if (Same(changed, original))
	{
		changed = Setting();
	}
	if (!Same(changed, m_factoryChanged[index]))
	{
		m_factoryChanged[index] = changed;
		Changed();
	}
}

void OneTouchSettings::LogNotTaken(int index, const Setting& target)
{
	const Setting current = Follow(target);
	for (int i = 0; i < NumParts; i++)
	{
		const Part& sent = target.parts[i];
		const Part& now = current.parts[i];
		const auto check = [&](bool differs, const char* what, const String& value, const String& pianoValue)
			{
				if (differs)
				{
					Logger::writeToLog("Factory OTS " + String(index + 1) + ": " + PartNames[i] + " " + what + " " +
						value + ", the piano has " + pianoValue);
				}
			};
		check(sent.voice != now.voice, "voice", sent.voice, now.voice);
		check(sent.activeSet && sent.active != now.active, "on", String((int)sent.active), String((int)now.active));
		check(sent.valuesSet && sent.volume != now.volume, "volume", String(sent.volume), String(now.volume));
		check(sent.valuesSet && sent.octave != now.octave, "octave", String(sent.octave), String(now.octave));
		check(sent.effectsSet && sent.pan != now.pan, "pan", String(sent.pan), String(now.pan));
		check(sent.effectsSet && sent.reverb != now.reverb, "reverb", String(sent.reverb), String(now.reverb));
	}
}

OneTouchSettings::Setting OneTouchSettings::Follow(const Setting& setting) const
{
	const Setting current = Capture();
	Setting result = setting;
	for (int i = 0; i < NumParts; i++)
	{
		Part& part = result.parts[i];
		const Part& now = current.parts[i];
		if (part.voice.isNotEmpty() && now.voice.isNotEmpty())
		{
			part.voice = now.voice;
		}
		if (part.activeSet)
		{
			part.active = now.active;
		}
		if (part.valuesSet)
		{
			part.volume = now.volume;
			part.octave = now.octave;
		}
		if (part.effectsSet)
		{
			part.pan = now.pan;
			part.reverb = now.reverb;
		}
	}
	return result;
}

bool OneTouchSettings::Same(const Setting& a, const Setting& b)
{
	if (a.used != b.used)
	{
		return false;
	}
	if (!a.used)
	{
		return true;
	}
	for (int i = 0; i < NumParts; i++)
	{
		const Part& x = a.parts[i];
		const Part& y = b.parts[i];
		if (x.voice != y.voice || x.activeSet != y.activeSet || x.valuesSet != y.valuesSet ||
			x.effectsSet != y.effectsSet || x.chorusSet != y.chorusSet ||
			(x.activeSet && x.active != y.active) ||
			(x.valuesSet && (x.volume != y.volume || x.octave != y.octave)) ||
			(x.effectsSet && (x.pan != y.pan || x.reverb != y.reverb)) ||
			(x.chorusSet && x.chorus != y.chorus))
		{
			return false;
		}
	}
	return true;
}

void OneTouchSettings::timerCallback()
{
	stopTimer();
	Update();
}

OneTouchSettings::Setting OneTouchSettings::Capture() const
{
	Setting setting;
	setting.used = true;
	for (int i = 0; i < NumParts; i++)
	{
		const PianoController::Channel ch = PartChannels[i];
		Part& part = setting.parts[i];
		part.voice = VoicePath(m_pianoController.GetVoice(ch));
		part.active = m_pianoController.GetActive(ch);
		part.volume = m_pianoController.GetVolume(ch);
		part.octave = m_pianoController.GetOctave(ch);
		part.pan = m_pianoController.GetPan(ch);
		part.reverb = m_pianoController.GetReverb(ch);
	}
	return setting;
}

bool OneTouchSettings::Matches(const Setting& setting) const
{
	const Setting current = Capture();
	for (int i = 0; i < NumParts; i++)
	{
		// a setting without a voice (not known when it was stored) does not change it, nor
		// the values it does not set
		Part stored = setting.parts[i];
		if (stored.voice.isEmpty())
		{
			stored.voice = current.parts[i].voice;
		}
		if (!stored.activeSet)
		{
			stored.active = current.parts[i].active;
		}
		if (!stored.valuesSet)
		{
			stored.volume = current.parts[i].volume;
			stored.octave = current.parts[i].octave;
		}
		if (!stored.effectsSet)
		{
			stored.pan = current.parts[i].pan;
			stored.reverb = current.parts[i].reverb;
		}
		if (!(stored == current.parts[i]))
		{
			return false;
		}
	}
	return true;
}

// The voice is sent first (it may bring its own octave), only if it differs from the one
// the channel will have (loading a voice takes time; a voice sent just before may not be
// reported back yet). The other values are always sent: the reported ones may still be
// the ones before a previous recall.
void OneTouchSettings::Apply(const Setting& setting)
{
	// a voice or an octave changes: the notes still sounding are silenced first (a sudden
	// change of the voice under them sounds strange); the accompaniment goes on
	bool changes = false;
	for (int i = 0; i < NumParts; i++)
	{
		const PianoController::Channel ch = PartChannels[i];
		const Part& part = setting.parts[i];
		changes = changes ||
			(part.voice.isNotEmpty() && part.voice != VoicePath(m_pianoController.GetTargetVoice(ch))) ||
			(part.valuesSet && part.octave != m_pianoController.GetOctave(ch));
	}
	if (changes)
	{
		m_pianoController.SilenceKeyboardParts();
		Thread::sleep(10); // the piano silences them before the new voice comes
	}

	for (int i = 0; i < NumParts; i++)
	{
		const PianoController::Channel ch = PartChannels[i];
		const Part& part = setting.parts[i];
		if (part.voice.isNotEmpty() && part.voice != VoicePath(m_pianoController.GetTargetVoice(ch)))
		{
			m_pianoController.SetVoice(ch, part.voice);
		}
		if (part.valuesSet)
		{
			m_pianoController.SetOctave(ch, jlimit(PianoController::MinOctave, PianoController::MaxOctave, part.octave));
			m_pianoController.SetVolume(ch, jlimit(PianoController::MinVolume, PianoController::MaxVolume, part.volume));
		}
		if (part.effectsSet)
		{
			m_pianoController.SetPan(ch, jlimit(PianoController::MinPan, PianoController::MaxPan, part.pan));
			m_pianoController.SetReverb(ch, jlimit(PianoController::MinReverb, PianoController::MaxReverb, part.reverb));
		}
		if (part.activeSet)
		{
			m_pianoController.SetActive(ch, part.active);
		}
	}
}

void OneTouchSettings::Store(int index)
{
	if (index < 0 || index >= NumSettings || !m_pianoController.IsConnected() || m_style.isEmpty())
	{
		return;
	}
	m_settings[index] = Capture();
	m_active = index;
	m_factoryActive = -1;
	m_modified = false;
	m_recallPending = false;
	Save();
	Changed();
}

void OneTouchSettings::Recall(int index)
{
	if (!IsUsed(index) || !m_pianoController.IsConnected())
	{
		return;
	}
	Apply(m_settings[index]);
	m_active = index;
	m_factoryActive = -1;
	m_modified = false;
	m_recallPending = true;
	m_recallTime = Time::getMillisecondCounter();
	startTimer(RecallSettleMs + 100); // decides then if the piano has not taken every value
	Changed();
}

void OneTouchSettings::Delete(int index)
{
	if (!IsUsed(index))
	{
		return;
	}
	m_settings[index] = Setting();
	if (m_active == index)
	{
		// the voices of the piano do not change
		m_active = -1;
		m_modified = false;
		m_recallPending = false;
	}
	Save();
	Changed();
}

void OneTouchSettings::SaveActive()
{
	if (m_active >= 0)
	{
		Store(m_active);
	}
}

void OneTouchSettings::DiscardActive()
{
	if (m_active >= 0)
	{
		Recall(m_active);
	}
}

void OneTouchSettings::RecallFactory(int index)
{
	if (!IsFactoryUsed(index) || !m_pianoController.IsConnected())
	{
		return;
	}
	Apply(GetFactory(index));
	m_factoryActive = index;
	m_active = -1;
	m_modified = false;
	m_recallPending = true;
	m_recallTime = Time::getMillisecondCounter();
	startTimer(RecallSettleMs + 100); // decides then if the piano has not taken every value
	Changed();
}

int OneTouchSettings::StoredValue(PianoController::Channel ch, Value which) const
{
	const Setting* setting = m_factoryActive >= 0 ? &m_factory[m_factoryActive] :
		m_active >= 0 ? &m_settings[m_active] : nullptr;
	for (int i = 0; i < NumParts; i++)
	{
		if (PartChannels[i] != ch || setting == nullptr || !setting->used)
		{
			continue;
		}
		const Part& part = setting->parts[i];
		if (which == Value::volume && part.valuesSet)
		{
			return part.volume;
		}
		if (which == Value::pan && part.effectsSet)
		{
			return part.pan;
		}
		if (which == Value::reverb && part.effectsSet)
		{
			return part.reverb;
		}
	}
	return which == Value::volume ? (int)PianoController::DefaultVolume :
		which == Value::pan ? (int)PianoController::DefaultPan : (int)PianoController::DefaultReverb;
}

const OneTouchSettings::Setting& OneTouchSettings::GetFactory(int index) const
{
	return m_factoryChanged[index].used ? m_factoryChanged[index] : m_factory[index];
}

bool OneTouchSettings::IsFactoryModified(int index) const
{
	return index >= 0 && index < NumFactorySettings && m_factoryChanged[index].used;
}

void OneTouchSettings::SaveActiveFactory()
{
	const int index = m_factoryActive;
	if (!IsFactoryModified(index) || m_style.isEmpty())
	{
		return;
	}
	m_factory[index] = m_factoryChanged[index];
	m_factoryChanged[index] = Setting();
	// into the version chosen; the other versions stay as they are
	if (!m_factoryVariants[index].empty())
	{
		m_factoryVariants[index][(size_t)m_factorySelected[index]].setting = m_factory[index];
	}
	// only the original settings go into the file (the other changed versions stay changed)
	if (WriteFile(FileIn(m_factoryFolder, m_style), m_style, m_factory, NumFactorySettings, m_factoryVariants))
	{
		m_stylesWithFactory.insert(m_style);
	}
	Changed();
}

void OneTouchSettings::DiscardActiveFactory()
{
	const int index = m_factoryActive;
	if (!IsFactoryModified(index))
	{
		return;
	}
	m_factoryChanged[index] = Setting();
	RecallFactory(index);
}

void OneTouchSettings::CopyFactory(int first)
{
	if (first < 0 || first + NumFactorySettings > NumSettings || m_style.isEmpty())
	{
		return;
	}
	bool any = false;
	for (int i = 0; i < NumFactorySettings; i++)
	{
		if (m_factory[i].used)
		{
			m_settings[first + i] = GetFactory(i);
			if (m_active == first + i)
			{
				m_active = -1; // its content has changed
				m_modified = false;
			}
			any = true;
		}
	}
	if (any)
	{
		Save();
		Changed();
	}
}

String OneTouchSettings::Describe(int index) const
{
	return IsUsed(index) ? DescribeSetting(m_settings[index]) : String();
}

String OneTouchSettings::DescribeFactory(int index) const
{
	return IsFactoryUsed(index) ? DescribeSetting(GetFactory(index)) : String();
}

String OneTouchSettings::DescribeSetting(const Setting& setting)
{
	String text;
	for (int i = 0; i < NumParts; i++)
	{
		const Part& part = setting.parts[i];
		// the names of the parts as on the Voice tab
		text << (text.isEmpty() ? "" : "\n") << TRANS(PartNames[i]) << ": "
			<< (part.voice.isEmpty() ? TRANS("unchanged") : Presets::VoiceTitle(part.voice));
		if (part.activeSet && !part.active)
		{
			text << " (" << TRANS("off") << ")";
		}
	}
	return text;
}

void OneTouchSettings::Load()
{
	for (Setting& setting : m_settings)
	{
		setting = Setting();
	}
	if (m_style.isNotEmpty())
	{
		ReadFile(FileOf(m_style), m_style, m_settings, NumSettings);
	}
}

void OneTouchSettings::LoadFactory()
{
	for (int i = 0; i < NumFactorySettings; i++)
	{
		m_factory[i] = Setting();
		m_factoryChanged[i] = Setting(); // the changed versions are kept only for the style
		m_factoryVariants[i].clear();
		m_factorySelected[i] = 0;
	}
	if (m_style.isEmpty() || !HasFactorySettings(m_style))
	{
		return;
	}
	ReadFile(FileIn(m_factoryFolder, m_style), m_style, m_factory, NumFactorySettings, m_factoryVariants);
	// the versions chosen for the style (the default one if the instrument chosen has none
	// any more, e.g. after importing the style files again)
	for (int i = 0; i < NumFactorySettings; i++)
	{
		const auto choice = m_variantChoices.find(m_style + "|" + String(i + 1));
		if (choice == m_variantChoices.end())
		{
			continue;
		}
		for (size_t v = 0; v < m_factoryVariants[i].size(); v++)
		{
			if (m_factoryVariants[i][v].sources.contains(choice->second))
			{
				m_factorySelected[i] = (int)v;
				m_factory[i] = m_factoryVariants[i][v].setting;
				break;
			}
		}
	}
}

int OneTouchSettings::GetFactoryVariantCount(int index) const
{
	return index >= 0 && index < NumFactorySettings ? (int)m_factoryVariants[index].size() : 0;
}

const OneTouchSettings::Variant& OneTouchSettings::GetFactoryVariant(int index, int variant) const
{
	jassert(variant >= 0 && variant < GetFactoryVariantCount(index));
	return m_factoryVariants[index][(size_t)variant];
}

int OneTouchSettings::GetFactoryVariantSelected(int index) const
{
	return index >= 0 && index < NumFactorySettings ? m_factorySelected[index] : 0;
}

void OneTouchSettings::SelectFactoryVariant(int index, int variant)
{
	if (variant < 0 || variant >= GetFactoryVariantCount(index) || m_style.isEmpty())
	{
		return;
	}
	if (variant == m_factorySelected[index] && !IsFactoryModified(index))
	{
		return;
	}
	m_factorySelected[index] = variant;
	m_factory[index] = m_factoryVariants[index][(size_t)variant].setting;
	m_factoryChanged[index] = Setting(); // the changes of the previous version are dropped
	const String key = m_style + "|" + String(index + 1);
	const StringArray& sources = m_factoryVariants[index][(size_t)variant].sources;
	if (variant == 0 || sources.isEmpty())
	{
		m_variantChoices.erase(key); // the default one
	}
	else
	{
		m_variantChoices[key] = sources[0];
	}
	SaveVariantChoices();
	Logger::writeToLog("Factory OTS " + String(index + 1) + " of " + m_style + ": the version of " +
		(sources.isEmpty() ? String("-") : sources.joinIntoString(", ")));
	if (m_factoryActive == index)
	{
		RecallFactory(index); // the new version to the piano
	}
	else
	{
		Changed();
	}
}

bool OneTouchSettings::SameVariant(const Setting& a, const Setting& b)
{
	Setting x = a;
	Setting y = b;
	for (int i = 0; i < NumParts; i++)
	{
		x.parts[i].chorusSet = y.parts[i].chorusSet = false;
	}
	return Same(x, y);
}

File OneTouchSettings::VariantChoicesFile() const
{
	return m_folder.getChildFile(VariantChoicesName);
}

void OneTouchSettings::LoadVariantChoices()
{
	m_variantChoices.clear();
	std::unique_ptr<XmlElement> root = XmlDocument::parse(VariantChoicesFile());
	if (!root || !root->hasTagName("FactoryVariants"))
	{
		return;
	}
	for (XmlElement* element : root->getChildWithTagNameIterator("Choice"))
	{
		const String style = element->getStringAttribute("style");
		const int number = element->getIntAttribute("number");
		const String source = element->getStringAttribute("source");
		if (style.isNotEmpty() && number >= 1 && number <= NumFactorySettings && source.isNotEmpty())
		{
			m_variantChoices[style + "|" + String(number)] = source;
		}
	}
}

void OneTouchSettings::SaveVariantChoices() const
{
	const File file = VariantChoicesFile();
	if (m_variantChoices.empty())
	{
		file.deleteFile();
		return;
	}
	XmlElement root("FactoryVariants");
	for (const auto& choice : m_variantChoices)
	{
		XmlElement* element = root.createNewChildElement("Choice");
		element->setAttribute("style", choice.first.upToLastOccurrenceOf("|", false, false));
		element->setAttribute("number", choice.first.fromLastOccurrenceOf("|", false, false).getIntValue());
		element->setAttribute("source", choice.second);
	}
	if (!root.writeTo(file))
	{
		Logger::writeToLog("Cannot save the choice of the factory OTS versions: " + file.getFullPathName());
	}
}

void OneTouchSettings::Save()
{
	if (m_style.isEmpty())
	{
		return;
	}
	const File file = FileOf(m_style);
	if (WriteFile(file, m_style, m_settings, NumSettings))
	{
		m_stylesWithSettings.insert(m_style);
		return;
	}
	const bool any = std::any_of(std::begin(m_settings), std::end(m_settings),
		[](const Setting& setting) { return setting.used; });
	if (!any)
	{
		m_stylesWithSettings.erase(m_style); // all places empty: the file is deleted
		return;
	}
	// it could not be written (e.g. a read-only folder): kept in memory for this session
	Logger::writeToLog("Cannot save the OTS: " + file.getFullPathName());
	AlertWindow::showMessageBoxAsync(MessageBoxIconType::WarningIcon, "ConPianist",
		TRANS("The OTS could not be saved:") + "\n" + file.getFullPathName());
}

// The parts of a setting from its element (a setting or a version of it).
static void ReadParts(const XmlElement& element, OneTouchSettings::Setting& setting)
{
	using Part = OneTouchSettings::Part;
	setting.used = true;
	for (XmlElement* partElement : element.getChildWithTagNameIterator("Part"))
	{
		for (int i = 0; i < OneTouchSettings::NumParts; i++)
		{
			if (partElement->getStringAttribute("name") == OneTouchSettings::PartNames[i])
			{
				Part& part = setting.parts[i];
				part.voice = partElement->getStringAttribute("voice");
				// a missing value is not changed by the setting
				part.activeSet = partElement->hasAttribute("active");
				part.active = partElement->getBoolAttribute("active", true);
				part.valuesSet = partElement->hasAttribute("volume");
				part.volume = jlimit(PianoController::MinVolume, PianoController::MaxVolume,
					partElement->getIntAttribute("volume", PianoController::DefaultVolume));
				part.octave = jlimit(PianoController::MinOctave, PianoController::MaxOctave,
					partElement->getIntAttribute("octave", PianoController::DefaultOctave));
				// (not in the files of earlier versions: not changed then)
				part.effectsSet = partElement->hasAttribute("pan");
				part.pan = jlimit(PianoController::MinPan, PianoController::MaxPan,
					partElement->getIntAttribute("pan", PianoController::DefaultPan));
				part.reverb = jlimit(PianoController::MinReverb, PianoController::MaxReverb,
					partElement->getIntAttribute("reverb", PianoController::DefaultReverb));
				part.chorusSet = partElement->hasAttribute("chorus");
				part.chorus = jlimit(0, 127, partElement->getIntAttribute("chorus", 0));
			}
		}
	}
}

static void WriteParts(XmlElement& element, const OneTouchSettings::Setting& setting)
{
	using Part = OneTouchSettings::Part;
	for (int i = 0; i < OneTouchSettings::NumParts; i++)
	{
		const Part& part = setting.parts[i];
		XmlElement* partElement = element.createNewChildElement("Part");
		partElement->setAttribute("name", OneTouchSettings::PartNames[i]);
		partElement->setAttribute("voice", part.voice);
		if (part.activeSet)
		{
			partElement->setAttribute("active", part.active ? 1 : 0);
		}
		if (part.valuesSet)
		{
			partElement->setAttribute("volume", part.volume);
			partElement->setAttribute("octave", part.octave);
		}
		if (part.effectsSet)
		{
			partElement->setAttribute("pan", part.pan);
			partElement->setAttribute("reverb", part.reverb);
		}
		if (part.chorusSet)
		{
			partElement->setAttribute("chorus", part.chorus);
		}
	}
}

bool OneTouchSettings::ReadFile(const File& file, const String& stylePath, Setting* settings, int count,
	std::vector<Variant>* variants)
{
	std::unique_ptr<XmlElement> root = XmlDocument::parse(file);
	if (!root || !root->hasTagName(RootTag) || root->getStringAttribute("style") != stylePath)
	{
		return false;
	}
	for (XmlElement* element : root->getChildWithTagNameIterator("Setting"))
	{
		const int index = element->getIntAttribute("number") - 1;
		if (index < 0 || index >= count)
		{
			continue;
		}
		Setting& setting = settings[index];
		ReadParts(*element, setting);
		if (variants != nullptr)
		{
			// the default version first
			std::vector<Variant>& list = variants[index];
			list.insert(list.begin(), {setting, StringArray::fromTokens(element->getStringAttribute("sources"), ",", "")});
		}
	}
	if (variants != nullptr)
	{
		// the other versions (only with a setting: a version of nothing is not used)
		for (XmlElement* element : root->getChildWithTagNameIterator("Variant"))
		{
			const int index = element->getIntAttribute("number") - 1;
			if (index < 0 || index >= count || !settings[index].used)
			{
				continue;
			}
			Variant variant;
			ReadParts(*element, variant.setting);
			variant.sources = StringArray::fromTokens(element->getStringAttribute("sources"), ",", "");
			variants[index].push_back(variant);
		}
	}
	return true;
}

bool OneTouchSettings::WriteFile(const File& file, const String& stylePath, const Setting* settings, int count,
	const std::vector<Variant>* variants)
{
	bool any = false;
	XmlElement root(RootTag);
	root.setAttribute("style", stylePath);
	for (int index = 0; index < count; index++)
	{
		const bool withVariants = variants != nullptr && !variants[index].empty();
		const Setting& setting = withVariants ? variants[index].front().setting : settings[index];
		if (!setting.used)
		{
			continue;
		}
		any = true;
		XmlElement* element = root.createNewChildElement("Setting");
		element->setAttribute("number", index + 1);
		if (withVariants && !variants[index].front().sources.isEmpty())
		{
			element->setAttribute("sources", variants[index].front().sources.joinIntoString(","));
		}
		WriteParts(*element, setting);
	}
	// the other versions after the settings (a program that does not know them skips them)
	for (int index = 0; variants != nullptr && index < count; index++)
	{
		for (size_t v = 1; v < variants[index].size(); v++)
		{
			const Variant& variant = variants[index][v];
			if (!variant.setting.used || !variants[index].front().setting.used)
			{
				continue;
			}
			XmlElement* element = root.createNewChildElement("Variant");
			element->setAttribute("number", index + 1);
			element->setAttribute("sources", variant.sources.joinIntoString(","));
			WriteParts(*element, variant.setting);
		}
	}

	if (!any)
	{
		file.deleteFile();
		return false;
	}
	file.getParentDirectory().createDirectory();
	return root.writeTo(file);
}
