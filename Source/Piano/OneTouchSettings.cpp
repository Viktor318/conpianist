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

bool OneTouchSettings::Part::operator==(const Part& other) const
{
	return voice == other.voice && active == other.active &&
		volume == other.volume && octave == other.octave;
}

OneTouchSettings::OneTouchSettings(const File& folder, PianoController& pianoController) :
	m_folder(folder), m_pianoController(pianoController)
{
	ScanFolder();
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
	String category, group, name;
	StyleNames(stylePath, category, group, name);
	File folder = m_folder;
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

// The styles that have settings: the style of every file in the folder.
void OneTouchSettings::ScanFolder()
{
	m_stylesWithSettings.clear();
	for (const DirectoryEntry& entry : RangedDirectoryIterator(m_folder, true,
		String("*") + FileExtension, File::findFiles))
	{
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
	if (!m_pianoController.IsConnected())
	{
		// the voices of the piano are not known: nothing is active
		if (m_active >= 0)
		{
			m_active = -1;
			m_recallPending = false;
			Changed();
		}
		return;
	}

	const String style = m_pianoController.GetStyleName();
	if (style != m_style)
	{
		// another style: its own settings; the active setting belonged to the previous one
		m_style = style;
		m_active = -1;
		m_modified = false;
		m_recallPending = false;
		Load();
		Changed();
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
	}
	return setting;
}

bool OneTouchSettings::Matches(const Setting& setting) const
{
	const Setting current = Capture();
	for (int i = 0; i < NumParts; i++)
	{
		// a setting without a voice (not known when it was stored) does not change it
		Part stored = setting.parts[i];
		if (stored.voice.isEmpty())
		{
			stored.voice = current.parts[i].voice;
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
	for (int i = 0; i < NumParts; i++)
	{
		const PianoController::Channel ch = PartChannels[i];
		const Part& part = setting.parts[i];
		if (part.voice.isNotEmpty() && part.voice != VoicePath(m_pianoController.GetTargetVoice(ch)))
		{
			m_pianoController.SetVoice(ch, part.voice);
		}
		m_pianoController.SetOctave(ch, jlimit(PianoController::MinOctave, PianoController::MaxOctave, part.octave));
		m_pianoController.SetVolume(ch, jlimit(PianoController::MinVolume, PianoController::MaxVolume, part.volume));
		m_pianoController.SetActive(ch, part.active);
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

String OneTouchSettings::Describe(int index) const
{
	if (!IsUsed(index))
	{
		return {};
	}
	String text;
	for (int i = 0; i < NumParts; i++)
	{
		const Part& part = m_settings[index].parts[i];
		// the names of the parts as on the Voice tab
		text << (text.isEmpty() ? "" : "\n") << TRANS(PartNames[i]) << ": " << Presets::VoiceTitle(part.voice);
		if (!part.active)
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
	if (m_style.isEmpty())
	{
		return;
	}
	std::unique_ptr<XmlElement> root = XmlDocument::parse(FileOf(m_style));
	if (!root || !root->hasTagName(RootTag) || root->getStringAttribute("style") != m_style)
	{
		return;
	}
	for (XmlElement* element : root->getChildWithTagNameIterator("Setting"))
	{
		const int index = element->getIntAttribute("number") - 1;
		if (index < 0 || index >= NumSettings)
		{
			continue;
		}
		Setting& setting = m_settings[index];
		setting.used = true;
		for (XmlElement* partElement : element->getChildWithTagNameIterator("Part"))
		{
			for (int i = 0; i < NumParts; i++)
			{
				if (partElement->getStringAttribute("name") == PartNames[i])
				{
					Part& part = setting.parts[i];
					part.voice = partElement->getStringAttribute("voice");
					part.active = partElement->getBoolAttribute("active", true);
					part.volume = jlimit(PianoController::MinVolume, PianoController::MaxVolume,
						partElement->getIntAttribute("volume", PianoController::DefaultVolume));
					part.octave = jlimit(PianoController::MinOctave, PianoController::MaxOctave,
						partElement->getIntAttribute("octave", PianoController::DefaultOctave));
				}
			}
		}
	}
}

void OneTouchSettings::Save()
{
	if (m_style.isEmpty())
	{
		return;
	}
	const File file = FileOf(m_style);
	bool any = false;
	XmlElement root(RootTag);
	root.setAttribute("style", m_style);
	for (int index = 0; index < NumSettings; index++)
	{
		const Setting& setting = m_settings[index];
		if (!setting.used)
		{
			continue;
		}
		any = true;
		XmlElement* element = root.createNewChildElement("Setting");
		element->setAttribute("number", index + 1);
		for (int i = 0; i < NumParts; i++)
		{
			const Part& part = setting.parts[i];
			XmlElement* partElement = element->createNewChildElement("Part");
			partElement->setAttribute("name", PartNames[i]);
			partElement->setAttribute("voice", part.voice);
			partElement->setAttribute("active", part.active ? 1 : 0);
			partElement->setAttribute("volume", part.volume);
			partElement->setAttribute("octave", part.octave);
		}
	}

	if (!any)
	{
		file.deleteFile();
		m_stylesWithSettings.erase(m_style);
		return;
	}
	file.getParentDirectory().createDirectory();
	if (root.writeTo(file))
	{
		m_stylesWithSettings.insert(m_style);
	}
}
