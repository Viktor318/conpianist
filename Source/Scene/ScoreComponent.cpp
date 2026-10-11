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

#ifdef DISABLE_LOMSE

#include "ScoreComponent.h"
ScoreComponent* ScoreComponent::Create(Settings& settings, PianoController& pianoController)
{
	return new ScoreComponent();
}

int ScoreComponent::CheckScoreFile(const File& file, const File& resultFile)
{
	return 0;
}
#else

#include "Presets.h"

#include <filesystem>
#include <map>

#include <lomse_doorway.h>
#include <lomse_document.h>
#include <lomse_graphic_view.h>
#include <lomse_interactor.h>
#include <lomse_presenter.h>
#include <lomse_tasks.h>
#include <lomse_tempo_line.h>
#include <lomse_score_algorithms.h>
#include <lomse_fragment_mark.h>
#include <lomse_gm_basic.h>
#include <lomse_graphical_model.h>
#include <lomse_im_measures_table.h>
#include <lomse_midi_table.h>
#include <pugixml/pugixml.hpp>
#include <functional>
#include <string>
#include <string_view>
#include <cstdlib>
#include <algorithm>

#include "GuiHelper.h"
#include "ScoreComponent.h"

using namespace lomse;

// A file path in the form Lomse and FreeType can open. On Windows they open files with
// the narrow-character functions, which expect the path in the code page of the system,
// not in UTF-8: with a UTF-8 path the fonts were not found when the program was in a
// folder with accented letters in its path. (A path with characters the code page of the
// system does not have cannot be converted; it is passed on unchanged then.)
static std::string NativePath(const String& path)
{
#if JUCE_WINDOWS
	try
	{
		return std::filesystem::path(std::wstring(path.toWideCharPointer())).string();
	}
	catch (const std::exception&)
	{
	}
#endif
	return path.toStdString();
}

class LomseScoreComponent : public ScoreComponent, public PianoController::Listener,
	public Button::Listener, public ChangeListener
{
public:
	LomseScoreComponent(Settings& settings, PianoController& pianoController);
	~LomseScoreComponent() override;

	void paint (Graphics& g) override;
	void resized() override;
	void mouseDown(const MouseEvent& event) override;
	void mouseUp(const MouseEvent& event) override;
	void mouseMove(const MouseEvent& event) override;
	void mouseDrag(const MouseEvent& event) override;
	void mouseDoubleClick(const MouseEvent& event) override;
	void mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& details) override;
    void buttonClicked (Button* buttonThatWasClicked) override;

	void PianoStateChanged(PianoController::Aspect aspect, PianoController::Channel channel) override;
    void changeListenerCallback(ChangeBroadcaster* source) override { if (source == &m_settings) ApplySettings(); }

private:
	Settings& m_settings;
	PianoController& m_pianoController;
	// Lomse writes its logs into this stream: it has no buffer, so nothing is written
	// anywhere (by default Lomse creates log files in the home and the current folders).
	static std::ostream& NullLog() { static std::ostream stream(nullptr); return stream; }
	lomse::LomseDoorway m_lomse{&NullLog(), &NullLog()};
	std::unique_ptr<Presenter> m_presenter;
	std::unique_ptr<juce::Image> m_image;
	float m_scale = 1;
	float m_docScale = 1;
	int m_scoreId = 0;
	PianoController::Loop loop{{0,0},{0,0}};
	PianoController::Position loopStart{0,0};
	FragmentMark* loopStartMark = nullptr;
	FragmentMark* loopEndMark = nullptr;
    std::unique_ptr<Button> loadButton;
    std::unique_ptr<ImageButton> menuButton;
    std::vector<std::string> m_instrNames;
    std::vector<std::string> m_instrAbbrevs;
    std::vector<ImoInstrument*> m_instruments;
    std::vector<ImoInstrGroup*> m_instrgroups;
    PianoController::Channel m_rightChannel = PianoController::chMidi0;
    PianoController::Channel m_leftChannel = PianoController::chMidi0;
	Settings::ScoreInstrumentNames m_scoreInstrumentNames = Settings::siMixed;
	Settings::ScorePart m_scorePart = Settings::spRightAndLeft;
	bool m_scoreShowMidiChannel = true;
	String m_error;
	bool m_firstSync = true;
	File m_chosenScore; // score chosen by the user, shown when its song has been loaded
	// Checking a score before it is shown (see ScoreCheck): the score being checked, the
	// number of the check (a result of an earlier check is dropped) and the last faulty score.
	class ScoreCheck;
	std::unique_ptr<ScoreCheck> m_check;
	File m_checkedScore;
	int m_checkNumber = 0;
	String m_faultyScore;
	StringArray m_checkedScores; // see IsCheckedScore
	bool m_checkedScoresRead = false;
	// The playback order of the measures of a score with repeats (see ReadPlaybackOrder),
	// and the number of measures of the score
	std::vector<int> m_playOrder;
	std::vector<int> m_passStarts;
	int m_scoreMeasures = 0;

	void LoadDocument(String filename);
	void PrepareImage();
	LUnits ScaledUnits(int pixels);
	unsigned GetMouseFlags(const MouseEvent& event);
	void UpdateTempoLine(bool scroll);
	static TimeUnits BeatLocation(ImoScore* score, int measure, int beat);
	static TimeUnits BeatTimepos(ImoScore* score, int measure, int beat);
	int MeasureAtPoint(SpInteractor& interactor, ImoScore* score, int x, int y);
	bool FollowsPlaybackOrder();
	int ScoreMeasure(int playedMeasure);
	int PlayedMeasure(int scoreMeasure);
	void SetViewport(int y);
	void LimitViewport();
	int ScoreBottom();
	void UpdateABMarks(bool force);
	void BuildControls();
	void LoadScore(const File& file);
	void ShowScore(const File& file);
	void ScoreChecked(int number, const File& file, bool ok, const String& reason, const String& details);
	void ReportFaultyScore(const File& file, const String& reason, const String& details);
	String CheckedScoreKey(const File& file) const;
	bool IsCheckedScore(const File& file);
	void AddCheckedScore(const File& file);
	void LoadScore(const URL& url);
	void ChooseScoreFile();
	void ShowMenu();
	void ApplySettings();
	void PrepareInstruments();
	void ConfigureInstruments();
	void UpdateInstruments(bool force);
	int MidiChannelOfInstrument(ImoInstrument* instr);
	void Cleanup();

	// Piano controller callbacks
	void UpdateSongState();
	void LoadSong();

	// Lomse callbacks
	void UpdateWindow(SpEventInfo event);
	void LomseEvent(SpEventInfo event);
	static void UpdateWindowWrapper(void* obj, SpEventInfo event)
		{ static_cast<LomseScoreComponent*>(obj)->UpdateWindow(event); }
	static void LomseEventWrapper(void* obj, SpEventInfo event)
		{ static_cast<LomseScoreComponent*>(obj)->LomseEvent(event); }
	void LomseRequest(Request* request);
	static void LomseRequestWrapper(void* obj, Request* request)
		{ static_cast<LomseScoreComponent*>(obj)->LomseRequest(request); }
};


ScoreComponent* ScoreComponent::Create(Settings& settings, PianoController& pianoController)
{
	return new LomseScoreComponent(settings, pianoController);
}

LomseScoreComponent::LomseScoreComponent(Settings& settings, PianoController& pianoController) :
	m_settings(settings), m_pianoController(pianoController)
{
	m_scale = m_settings.zoomUi * Desktop::getInstance().getDisplays().getMainDisplay().scale;
	int resolution = int(96 * m_scale);

	lomse::glogger.set_logging_mode(lomse::Logger::k_trace_mode);

	// Lomse knows nothing about windows. It renders everything on a bitmap and the
	// user application uses this bitmap. For instance, to display it on a window.
	// Lomse supports a lot of bitmap formats and pixel formats. Therefore, before
	// using the Lomse library you MUST specify which bitmap formap to use.

	//the pixel format
	int pixel_format = k_pix_format_rgba32;

	//Lomse default y axis direction is 0 coordinate at top and increases
	//downwards. For JUCE the Lomse default behaviour is the right behaviour.
	bool reverse_y_axis = false;

	//initialize the Lomse library with these values
	m_lomse.init_library(pixel_format, resolution, reverse_y_axis);

	m_lomse.set_default_fonts_path(NativePath(m_settings.resourcesPath + "/fonts/"));

	//set required callbacks
	m_lomse.set_notify_callback(this, LomseEventWrapper);
	m_lomse.set_request_callback(this, LomseRequestWrapper);

	BuildControls();

	pianoController.AddListener(this);
	settings.addChangeListener(this);
}

LomseScoreComponent::~LomseScoreComponent()
{
	m_check.reset();
	Cleanup();
}

void LomseScoreComponent::BuildControls()
{
    loadButton.reset(new TextButton("Load Button"));
    addAndMakeVisible(loadButton.get());
    loadButton->setButtonText(TRANS("Load Score"));
    loadButton->addListener(this);

	menuButton.reset(new ImageButton("Menu Button"));
	addAndMakeVisible(menuButton.get());
	menuButton->setTooltip(TRANS("Context Menu"));
	menuButton->setButtonText("Menu");
	menuButton->addListener(this);
	menuButton->setImages(false, true, true,
		ImageCache::getFromMemory(BinaryData::buttoncontextmenuscore_png,
			BinaryData::buttoncontextmenuscore_pngSize), 1.000f, Colour (0x00000000),
		juce::Image(), 0.750f, Colour (0x00000000),
		juce::Image(), 1.000f, Colour (0x00000000));
    menuButton->setBounds(8, 8, 28, 28);
}

// The text of the score in a compressed MusicXML file (.mxl): a zip archive in which
// META-INF/container.xml names the score file. An empty string if there is no score in it.
static String ReadCompressedMusicXml(const File& file)
{
	ZipFile zip(file);
	int index = -1;

	const int containerIndex = zip.getIndexOfFileName("META-INF/container.xml", true);
	if (containerIndex >= 0)
	{
		std::unique_ptr<juce::InputStream> stream(zip.createStreamForEntry(containerIndex));
		std::unique_ptr<XmlElement> container = stream != nullptr ? parseXML(stream->readEntireStreamAsString()) : nullptr;
		XmlElement* rootfiles = container != nullptr ? container->getChildByName("rootfiles") : nullptr;
		XmlElement* rootfile = rootfiles != nullptr ? rootfiles->getChildByName("rootfile") : nullptr;
		if (rootfile != nullptr)
		{
			index = zip.getIndexOfFileName(rootfile->getStringAttribute("full-path"), true);
		}
	}

	// no usable container: the first score file outside the META-INF folder
	for (int i = 0; i < zip.getNumEntries() && index < 0; i++)
	{
		const String name = zip.getEntry(i)->filename;
		if (!name.startsWithIgnoreCase("META-INF") &&
			(name.endsWithIgnoreCase(".xml") || name.endsWithIgnoreCase(".musicxml")))
		{
			index = i;
		}
	}
	if (index < 0)
	{
		return {};
	}

	std::unique_ptr<juce::InputStream> stream(zip.createStreamForEntry(index));
	if (stream == nullptr)
	{
		return {};
	}
	MemoryBlock data;
	stream->readIntoMemoryBlock(data);
	return String::createStringFromData(data.getData(), (int)data.getSize()); // UTF-8 or UTF-16
}

// The name of a chord symbol of MusicXML (<harmony>), as it is written above the staff:
// root, kind (the text given by the notation program, e.g. "maj7", or the usual
// abbreviation of the kind), added or altered degrees and the bass ("C/G").
static String ChordSymbolName(const XmlElement& harmony)
{
	auto accidental = [](const XmlElement* alter)
		{
			const int value = alter != nullptr ? alter->getAllSubText().trim().getIntValue() : 0;
			return value > 0 ? String::repeatedString("#", value) : value < 0 ? String::repeatedString("b", -value) : String();
		};
	auto step = [&accidental](const XmlElement* parent, const char* stepName, const char* alterName)
		{
			if (parent == nullptr)
			{
				return String();
			}
			const XmlElement* stepElement = parent->getChildByName(stepName);
			if (stepElement == nullptr)
			{
				return String();
			}
			// a text given for the step (e.g. "Do" or "H") is used as it is
			const String text = stepElement->getStringAttribute("text");
			return (text.isNotEmpty() ? text : stepElement->getAllSubText().trim()) +
				accidental(parent->getChildByName(alterName));
		};

	String name = step(harmony.getChildByName("root"), "root-step", "root-alter");
	if (name.isEmpty())
	{
		// a harmony without a root (e.g. a function or a Roman numeral): its text
		if (const XmlElement* function = harmony.getChildByName("function"))
		{
			name = function->getAllSubText().trim();
		}
		else if (const XmlElement* numeral = harmony.getChildByName("numeral"))
		{
			if (const XmlElement* root = numeral->getChildByName("numeral-root"))
			{
				name = root->getStringAttribute("text", root->getAllSubText().trim());
			}
		}
	}

	if (const XmlElement* kind = harmony.getChildByName("kind"))
	{
		const bool hasText = kind->hasAttribute("text");
		if (kind->getAllSubText().trim() == "none")
		{
			// "No chord": the root is there only for the position, the symbol is the text of
			// the kind ("N.C."). Without a text MuseScore means the plain chord of the root
			// (it writes e.g. "D" like this), so the root is shown alone.
			if (hasText)
			{
				return kind->getStringAttribute("text");
			}
		}
		else if (hasText)
		{
			// a suspended chord is always shown short ("A4", "A2"), whatever its text is,
			// so that it does not run into the next chord symbol
			const String kindName = kind->getAllSubText().trim();
			name += kindName == "suspended-fourth" ? String("4") :
				kindName == "suspended-second" ? String("2") : kind->getStringAttribute("text");
		}
		else
		{
			static const std::map<String, String> kinds = {
				{"major", ""}, {"minor", "m"}, {"augmented", "+"}, {"diminished", "dim"},
				{"dominant", "7"}, {"major-seventh", "maj7"}, {"minor-seventh", "m7"},
				{"diminished-seventh", "dim7"}, {"augmented-seventh", "+7"},
				{"half-diminished", "m7b5"}, {"major-minor", "m(maj7)"}, {"major-sixth", "6"},
				{"minor-sixth", "m6"}, {"dominant-ninth", "9"}, {"major-ninth", "maj9"},
				{"minor-ninth", "m9"}, {"dominant-11th", "11"}, {"major-11th", "maj11"},
				{"minor-11th", "m11"}, {"dominant-13th", "13"}, {"major-13th", "maj13"},
				{"minor-13th", "m13"}, {"suspended-second", "2"}, {"suspended-fourth", "4"},
				{"power", "5"}, {"pedal", "ped"}, {"Neapolitan", "N6"}, {"Italian", "It+6"},
				{"French", "Fr+6"}, {"German", "Ger+6"}, {"Tristan", "Tristan"}};
			auto it = kinds.find(kind->getAllSubText().trim());
			if (it != kinds.end())
			{
				name += it->second;
			}
		}

		// the added, altered or left out degrees (not if the text of the kind is given:
		// it shows them already)
		for (auto* degree : harmony.getChildWithTagNameIterator("degree"))
		{
			if (hasText || degree->getStringAttribute("print-object") == "no")
			{
				continue;
			}
			const XmlElement* type = degree->getChildByName("degree-type");
			const XmlElement* value = degree->getChildByName("degree-value");
			const String typeName = type != nullptr ? type->getAllSubText().trim() : String("add");
			const String number = value != nullptr ? value->getAllSubText().trim() : String();
			const String alter = accidental(degree->getChildByName("degree-alter"));
			name += typeName == "subtract" ? "no" + number :
				typeName == "alter" ? alter + number : "add" + alter + number;
		}
	}

	const String bass = step(harmony.getChildByName("bass"), "bass-step", "bass-alter");
	if (bass.isNotEmpty())
	{
		name += "/" + bass;
	}
	return name;
}

// The highest voice number used by the notes of a MusicXML text (0 if none).
static int HighestVoice(const std::string& text)
{
	static const std::string voiceTag = "<voice>";
	int highest = 0;
	for (size_t start = text.find(voiceTag); start != std::string::npos; start = text.find(voiceTag, start + 1))
	{
		highest = jmax(highest, std::atoi(text.c_str() + start + voiceTag.size()));
	}
	return highest;
}

// The duration (in divisions) of the first note after the given position of a MusicXML
// text, or 0 if it is a grace note or it has no whole number as duration.
static int NextNoteDuration(const std::string& text, size_t position)
{
	static const std::string openTag = "<note";
	static const std::string closeTag = "</note>";
	for (size_t start = text.find(openTag, position); start != std::string::npos; start = text.find(openTag, start + 1))
	{
		const char next = start + openTag.size() < text.size() ? text[start + openTag.size()] : '\0';
		if (next != '>' && !CharacterFunctions::isWhitespace(next))
		{
			continue; // another element (e.g. <notehead>, <notations>)
		}
		const size_t close = text.find(closeTag, start);
		if (close == std::string::npos)
		{
			return 0;
		}
		const std::string_view note(text.data() + start, close - start);
		const size_t duration = note.find("<duration>");
		if (note.find("<grace") != std::string_view::npos || duration == std::string_view::npos)
		{
			return 0;
		}
		return std::atoi(std::string(note.substr(duration + 10, 12)).c_str());
	}
	return 0;
}

// Lomse does not show the chord symbols of MusicXML (<harmony>): they are written into the
// text as words above the staff (<direction>), at the same place, so they are shown like
// the other texts of the score. The file itself is not changed.
// A chord symbol may be later than the note it is written before (<offset>, e.g. more chord
// symbols over one long note). Lomse does not use the offset of a direction, so these would
// be drawn over each other. The chord symbols written before the same note are taken
// together, and those with an offset are moved forward in a voice of their own (one that
// no note uses; Lomse fills it with invisible rests) up to the end of that note, so that
// there is room for them; then the time goes back to the note.
static String ChordSymbolsAsWords(const String& content)
{
	// (searched in the bytes of the text: the positions of a juce::String are counted from
	// its beginning each time, which made a large score take very long)
	static const std::string openTag = "<harmony";
	static const std::string closeTag = "</harmony>";
	const std::string text = content.toStdString();
	// the voice for the moved chord symbols (Lomse allows 64 voices in a part)
	const int highestVoice = HighestVoice(text);
	const String offsetVoice = highestVoice < 63 ? String(highestVoice + 1) : String();
	auto words = [](const String& name, const String& staffNumber)
		{
			// above the staff, higher than the notes on the top line usually reach
			String direction;
			direction << "<direction placement=\"above\"><direction-type><words relative-y=\"40\" font-weight=\"bold\">"
				<< name << "</words></direction-type><staff>" << staffNumber << "</staff></direction>";
			return direction;
		};
	// the chord symbols with an offset of the current group (offset, words), and their staff
	std::vector<std::pair<int, String>> moved;
	String movedStaff;
	std::string result;
	result.reserve(text.size() + 4096);
	size_t position = 0; // the text before this position is in the result
	// writes the moved chord symbols of the group that ends at the position
	auto writeMoved = [&]()
		{
			if (moved.empty())
			{
				return;
			}
			std::stable_sort(moved.begin(), moved.end(),
				[](const auto& a, const auto& b) { return a.first < b.first; });
			String forward = "</duration><voice>" + offsetVoice + "</voice><staff>" + movedStaff + "</staff></forward>";
			String chain;
			int time = 0;
			for (const auto& [offset, direction] : moved)
			{
				if (offset > time)
				{
					chain << "<forward><duration>" << (offset - time) << forward;
					time = offset;
				}
				chain << direction;
			}
			// up to the end of the note, with an invisible word at its end (a non-breaking
			// space): Lomse fills the voice with a rest only up to a following object
			const int rest = NextNoteDuration(text, position) - time;
			if (rest > 0)
			{
				chain << "<forward><duration>" << rest << forward << words(CharPointer_UTF8("\xc2\xa0"), movedStaff);
				time += rest;
			}
			chain << "<backup><duration>" << time << "</duration></backup>";
			result += chain.toStdString();
			moved.clear();
		};
	for (size_t start = text.find(openTag); start != std::string::npos; start = text.find(openTag, start + 1))
	{
		const char next = start + openTag.size() < text.size() ? text[start + openTag.size()] : '\0';
		if (next != '>' && next != '/' && !CharacterFunctions::isWhitespace(next))
		{
			continue; // another element whose name begins the same way
		}
		const size_t tagEnd = text.find('>', start);
		if (tagEnd == std::string::npos)
		{
			break;
		}
		size_t end = tagEnd + 1;
		if (text[tagEnd - 1] != '/')
		{
			const size_t close = text.find(closeTag, tagEnd);
			if (close == std::string::npos)
			{
				break;
			}
			end = close + closeTag.size();
		}
		// a group ends where something else than white space comes after a chord symbol
		if (text.find_first_not_of(" \t\r\n", position) < start)
		{
			writeMoved();
		}
		std::unique_ptr<XmlElement> harmony = parseXML(String::fromUTF8(text.data() + start, (int)(end - start)));
		result.append(text, position, start - position);
		position = end;
		if (harmony == nullptr || harmony->getStringAttribute("print-object") == "no")
		{
			continue; // not shown
		}
		const String name = ChordSymbolName(*harmony);
		if (name.isEmpty())
		{
			continue;
		}
		const XmlElement* staff = harmony->getChildByName("staff");
		const String staffNumber = staff != nullptr ? staff->getAllSubText().trim() : String("1");
		const String direction = words(name.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;"), staffNumber);
		// the offset in divisions, if it is a whole positive number (only such a <forward>
		// can be written)
		int offset = 0;
		if (const XmlElement* offsetElement = harmony->getChildByName("offset"))
		{
			const String value = offsetElement->getAllSubText().trim();
			if (value.containsOnly("0123456789") && value.length() <= 6)
			{
				offset = value.getIntValue();
			}
		}
		if (offset > 0 && offsetVoice.isNotEmpty())
		{
			if (moved.empty())
			{
				movedStaff = staffNumber;
			}
			moved.emplace_back(offset, direction);
		}
		else
		{
			result += direction.toStdString();
		}
	}
	writeMoved();
	result.append(text, position, std::string::npos);
	return String::fromUTF8(result.data(), (int)result.size());
}

// A note of a chord (<chord/>) may leave out its voice (Sibelius writes it so): the voice is
// that of the first note of the chord. Lomse crashes when laying out such a score, so the
// voice of the note before is written into these notes. Returns false (and leaves the text
// as it is) if there is no such note.
static bool AddVoiceToChordNotes(String& content)
{
	// (searched in the bytes of the text, see ChordSymbolsAsWords)
	static const std::string openTag = "<note";
	static const std::string closeTag = "</note>";
	// the elements after <voice> in a note, in their order (the voice is put before the first)
	static const char* const after[] = { "<type", "<dot", "<accidental", "<time-modification",
		"<stem", "<notehead", "<staff", "<beam", "<notations", "<lyric", "<play", "<listen" };
	const std::string text = content.toStdString();
	std::string result;
	bool changed = false;
	std::string voice = "1";
	size_t position = 0; // the text before this position is in the result
	for (size_t start = text.find(openTag); start != std::string::npos; start = text.find(openTag, start + 1))
	{
		const char next = start + openTag.size() < text.size() ? text[start + openTag.size()] : '\0';
		if (next != '>' && !CharacterFunctions::isWhitespace(next))
		{
			continue; // another element (e.g. <notehead>, <notations>)
		}
		const size_t close = text.find(closeTag, start);
		if (close == std::string::npos)
		{
			break;
		}
		const std::string_view note(text.data() + start, close - start);
		const size_t voiceStart = note.find("<voice>");
		if (voiceStart != std::string_view::npos)
		{
			const size_t voiceEnd = note.find("</voice>", voiceStart);
			if (voiceEnd != std::string_view::npos)
			{
				voice = String(std::string(note.substr(voiceStart + 7, voiceEnd - voiceStart - 7))).trim().toStdString();
			}
			continue;
		}
		if (note.find("<chord") == std::string_view::npos)
		{
			continue;
		}
		size_t insert = note.size();
		for (const char* tag : after)
		{
			const size_t found = note.find(tag);
			if (found != std::string_view::npos && found < insert)
			{
				insert = found;
			}
		}
		if (!changed)
		{
			result.reserve(text.size() + 4096);
		}
		result.append(text, position, start + insert - position);
		result += "<voice>" + voice + "</voice>";
		position = start + insert;
		changed = true;
	}
	if (changed)
	{
		result.append(text, position, std::string::npos);
		content = String::fromUTF8(result.data(), (int)result.size());
	}
	return changed;
}

// The side of a fingering (above or below its note) as MuseScore shows it. MuseScore writes
// its position relative to the note: default-y is where MuseScore placed it (it is left out
// if the side was set by hand, then the placement attribute gives the side), relative-y is
// how far it was moved by hand. Lomse does not read these positions, and the placement
// attribute is not the side shown when the fingering was moved to the other side, so
// e.g. the fingerings of the lower voice in the upper staff were above, among those of
// the upper voice. The side shown is written as placement into the fingerings.
// Returns false (and leaves the text as it is) if there is no fingering to change.
static bool SetFingeringPlacement(String& content)
{
	if (!content.contains("<fingering") || !content.contains("<software>MuseScore"))
	{
		return false;
	}
	// (searched in the bytes of the text, see ChordSymbolsAsWords)
	static const std::string openTag = "<fingering";
	// the value of an attribute of the tag (empty if the tag has no such attribute)
	const auto attribute = [](std::string_view tag, const std::string& name) -> std::string_view
	{
		const std::string key = " " + name + "=\"";
		const size_t start = tag.find(key);
		if (start == std::string_view::npos)
		{
			return std::string_view();
		}
		const size_t valueStart = start + key.size();
		const size_t valueEnd = tag.find('"', valueStart);
		return valueEnd == std::string_view::npos ? std::string_view() : tag.substr(valueStart, valueEnd - valueStart);
	};
	const auto number = [](std::string_view value)
	{
		return std::atof(std::string(value).c_str());
	};
	const std::string text = content.toStdString();
	std::string result;
	bool changed = false;
	size_t position = 0; // the text before this position is in the result
	for (size_t start = text.find(openTag); start != std::string::npos; start = text.find(openTag, start + 1))
	{
		const size_t attributes = start + openTag.size();
		const size_t tagEnd = text.find('>', attributes);
		if (tagEnd == std::string::npos)
		{
			break;
		}
		std::string_view tag(text.data() + attributes, tagEnd - attributes);
		if (tag.empty() || !CharacterFunctions::isWhitespace(tag[0]))
		{
			continue; // no attributes
		}
		if (tag.back() == '/')
		{
			tag.remove_suffix(1); // an empty element
		}
		const std::string_view defaultY = attribute(tag, "default-y");
		const std::string_view relativeY = attribute(tag, "relative-y");
		const std::string_view placement = attribute(tag, "placement");
		if (defaultY.empty() && relativeY.empty())
		{
			continue; // the side is the placement, if given
		}
		// MuseScore puts a fingering about 1.5 spaces from the note
		double y = !defaultY.empty() ? number(defaultY)
			: placement == "above" ? 15.0 : placement == "below" ? -15.0 : 0.0;
		y += relativeY.empty() ? 0.0 : number(relativeY);
		if (y == 0.0)
		{
			continue;
		}
		if (!changed)
		{
			result.reserve(text.size() + 4096);
		}
		// the tag without its placement attribute, and the new one
		result.append(text, position, attributes - position);
		std::string newTag(tag);
		const size_t oldPlacement = newTag.find(" placement=\"");
		if (oldPlacement != std::string::npos)
		{
			const size_t valueEnd = newTag.find('"', oldPlacement + 12);
			newTag.erase(oldPlacement, valueEnd == std::string::npos ? std::string::npos : valueEnd + 1 - oldPlacement);
		}
		result += y > 0.0 ? " placement=\"above\"" : " placement=\"below\"";
		result += newTag;
		position = attributes + tag.size();
		changed = true;
	}
	if (changed)
	{
		result.append(text, position, std::string::npos);
		content = String::fromUTF8(result.data(), (int)result.size());
	}
	return changed;
}

// Removes the part groups (the brackets joining instruments) from the text of a score.
static void RemovePartGroups(String& content)
{
	if (!content.contains("<part-group"))
	{
		return;
	}
	// (searched in the bytes of the text, see ChordSymbolsAsWords)
	const std::string text = content.toStdString();
	std::string result;
	result.reserve(text.size());
	size_t position = 0; // the text before this position is in the result
	for (size_t start = text.find("<part-group"); start != std::string::npos; start = text.find("<part-group", position))
	{
		const size_t tagEnd = text.find('>', start);
		if (tagEnd == std::string::npos)
		{
			break;
		}
		size_t end = tagEnd + 1;
		if (text[tagEnd - 1] != '/')
		{
			const size_t close = text.find("</part-group>", tagEnd);
			if (close == std::string::npos)
			{
				break;
			}
			end = close + strlen("</part-group>");
		}
		result.append(text, position, start - position);
		position = end;
	}
	result.append(text, position, std::string::npos);
	content = String::fromUTF8(result.data(), (int)result.size());
}

// Reads a score file and prepares its text for Lomse. The file is passed to Lomse as text
// (asText) in these cases, otherwise Lomse opens the file itself:
// - the file is a compressed MusicXML (.mxl): it is unpacked here;
// - the path has non-ASCII characters (e.g. accented letters): Lomse cannot open
//   such a file on Windows;
// - the score has part groups (brackets joining the instruments, written e.g. by
//   Dorico): Lomse cannot draw them if an instrument of the group is not shown,
//   so the groups are left out;
// - the score has chord symbols: Lomse does not show them, they are written into
//   the text as words above the staff (see ChordSymbolsAsWords);
// - a note of a chord has no voice (Sibelius): it is added (see AddVoiceToChordNotes).
// - the side of the fingerings is given by their position (MuseScore): it is written into
//   them (see SetFingeringPlacement).
// Returns false if a compressed file has no score in it.
static bool PrepareScoreText(const String& filename, String& content, bool& asText)
{
	const bool compressed = filename.endsWithIgnoreCase(".mxl");
	content = compressed ? ReadCompressedMusicXml(File(filename)) : File(filename).loadFileAsString();
	if (compressed && content.isEmpty())
	{
		return false;
	}
	const bool asciiPath = CharPointer_ASCII::isValidString(filename.toRawUTF8(), (int)filename.getNumBytesAsUTF8());
	const bool hasGroups = content.contains("<part-group");
	const bool hasChords = content.contains("<harmony");
	if (hasChords)
	{
		content = ChordSymbolsAsWords(content);
	}
	const bool voicesAdded = AddVoiceToChordNotes(content);
	const bool placementsSet = SetFingeringPlacement(content);
	asText = !asciiPath || hasGroups || hasChords || voicesAdded || placementsSet || compressed;
	if (asText)
	{
		RemovePartGroups(content);
	}
	return true;
}

// The playback order of the measures of a score with repeats (repeat signs, voltas, D.C.,
// D.S., Fine, Coda), as Lomse works it out: for each played measure (0..) the measure of
// the score (0..). The passes (the parts played straight through, between two jumps) are
// given by their first played measure. Both are empty if the score is played straight
// through. The measures are counted as in the song positions: the first measure of the
// score (a pickup measure too) is the first. numMeasures: the number of measures of the
// score.
static void ReadPlaybackOrder(ImoScore* score, std::vector<int>& order, std::vector<int>& passStarts,
	int& numMeasures)
{
	order.clear();
	passStarts.clear();
	numMeasures = 0;
	if (score == nullptr)
	{
		return;
	}
	SoundEventsTable* table = score->get_midi_table();
	numMeasures = table->get_num_measures();
	for (MeasuresJumpsEntry* pass : table->get_measures_jumps())
	{
		const int from = pass->get_from_measure();
		const int to = pass->get_to_measure() == 0 ? numMeasures : pass->get_to_measure(); // 0: to the end
		if (from < 1 || to < from || to > numMeasures)
		{
			continue; // e.g. the jump of a D.S. without a segno
		}
		passStarts.push_back((int)order.size());
		for (int measure = from; measure <= to; measure++)
		{
			order.push_back(measure - 1);
		}
		if (order.size() > 100000)
		{
			break; // (jumps that would never end)
		}
	}
	bool straight = (int)order.size() == numMeasures;
	for (int i = 0; straight && i < (int)order.size(); i++)
	{
		straight = order[i] == i;
	}
	if (straight)
	{
		order.clear();
		passStarts.clear();
	}
}

void LomseScoreComponent::LoadDocument(String filename)
{
	//first, we will create a 'presenter'. It takes care of creating and maintaining
	//all objects and relationships between the document, its views and the interactors
	//to interact with the view
	if (filename.isNotEmpty())
	{
		// load from file (see PrepareScoreText)
		String content;
		bool asText = false;
		if (!PrepareScoreText(filename, content, asText))
		{
			// not a compressed MusicXML file: nothing is shown
			m_presenter.reset();
			return;
		}
		if (asText)
		{
			m_presenter.reset(m_lomse.new_document(lomse::k_view_vertical_book,
				content.toStdString(), lomse::Document::k_format_mxl));
		}
		else
		{
			m_presenter.reset(m_lomse.open_document(lomse::k_view_vertical_book, filename.toStdString()));
		}
	}
	else
	{
		// empty document
		m_presenter.reset(m_lomse.new_document(lomse::k_view_vertical_book));
	}

	//get the pointer to the interactor and register for receiving desired events
	//(the rendering buffer is set in PrepareImage, when the image is created)
	SpInteractor interactor = m_presenter->get_interactor(0).lock();
	//ask to receive desired events
	interactor->add_event_handler(k_update_window_event, this, UpdateWindowWrapper);

	// visuals
	interactor->set_view_background(Color(68,62,50)); // dark grey
	interactor->set_visual_tracking_mode(k_tracking_tempo_line);

	TempoLine* tempoLine = static_cast<TempoLine*>(interactor->get_tracking_effect(k_tracking_tempo_line));
	tempoLine->set_color(Color(15, 90, 235, 128));   // light orange

	interactor->switch_task(TaskFactory::k_task_drag_view);

	// the playback order of the measures, while all instruments are in the score
	ReadPlaybackOrder(dynamic_cast<ImoScore*>(m_presenter->get_document_raw_ptr()->get_im_root()->get_content_item(0)),
		m_playOrder, m_passStarts, m_scoreMeasures);
	if (!m_playOrder.empty())
	{
		String passes;
		for (size_t i = 0; i < m_passStarts.size(); i++)
		{
			const int first = m_passStarts[i];
			const int last = (i + 1 < m_passStarts.size() ? m_passStarts[i + 1] : (int)m_playOrder.size()) - 1;
			passes << (i > 0 ? ", " : "") << (m_playOrder[first] + 1) << "-" << (m_playOrder[last] + 1);
		}
		juce::Logger::writeToLog("[SCORE] Playback order " + passes + " (" + String((int)m_playOrder.size()) +
			" measures, the score has " + String(m_scoreMeasures) + ")");
	}

	PrepareInstruments();
	ConfigureInstruments();

	loop = {{0,0},{0,0}};
}

void LomseScoreComponent::PrepareImage()
{
	try
	{
		const uint32 start = Time::getMillisecondCounter();
		m_image.reset();

		int width = int(getWidth() * m_scale);
		int height = int(getHeight() * m_scale);

		//adjust the number of measures to fit the area size
		//adjust page size
		SpInteractor interactor = m_presenter->get_interactor(0).lock();
		Document* doc = m_presenter->get_document_raw_ptr();
		ImoDocument* imoDoc = doc->get_im_root();
		ImoPageInfo* pageInfo = imoDoc->get_page_info();

		imoDoc->set_page_content_scale(1.0); // reset scale
		pageInfo->set_page_width(ScaledUnits(width));
		pageInfo->set_page_height(ScaledUnits(height));
		// the same margins on the odd and the even pages, no binding margin
		pageInfo->set_top_margin_odd(500);
		pageInfo->set_left_margin_odd(300);
		pageInfo->set_right_margin_odd(300);
		pageInfo->set_bottom_margin_odd(500);
		pageInfo->set_top_margin_even(500);
		pageInfo->set_left_margin_even(300);
		pageInfo->set_right_margin_even(300);
		pageInfo->set_bottom_margin_even(500);

		interactor->on_document_updated();  //This rebuilds GraphicModel

		m_docScale = imoDoc->get_page_content_scale(); // Scale is calculated when rebuliding GraphicModel

		// create image to fit the whole page
		m_image.reset(new juce::Image(juce::Image::PixelFormat::ARGB,
			int(width / m_docScale), int(height / m_docScale), false, SoftwareImageType()));
		//connect the view with the pixels of the image. Lomse takes the address and the
		//size of the buffer when this is called, so it is done for every new image
		juce::Image::BitmapData bitmap(*m_image, juce::Image::BitmapData::readWrite);
		jassert(bitmap.lineStride == m_image->getWidth() * 4);
		interactor->set_rendering_buffer(bitmap.data, (unsigned)m_image->getWidth(), (unsigned)m_image->getHeight());

		interactor->redraw_bitmap();

		// after resizing, the old scroll position may lie outside the score
		LimitViewport();

		UpdateABMarks(true);
		UpdateTempoLine(false);

		// (drawing a large score can take long, mainly in the Debug build)
		juce::Logger::writeToLog("[SCORE] Drawn in " + String(Time::getMillisecondCounter() - start) +
			" ms (" + String(width) + " x " + String(height) + " pixels)");
	}
	catch (runtime_error e)
	{
		m_error = e.what();
		juce::Logger::writeToLog("[SCORE] runtime_error: " + m_error);
	}
}

LUnits LomseScoreComponent::ScaledUnits(int pixels)
{
	return LUnits(pixels) * 26.5f / m_scale;
}

void LomseScoreComponent::UpdateWindow(SpEventInfo event)
{
	repaint();
}

void LomseScoreComponent::LomseEvent(SpEventInfo event)
{
	if (event->get_event_type() == k_update_viewport_event)
	{
		SpEventUpdateViewport viewportEvent(static_pointer_cast<EventUpdateViewport>(event));
		SpInteractor interactor = m_presenter->get_interactor(0).lock();
		const int OFFSET_CORRECTION = 19; // empirical value
		SetViewport(viewportEvent->get_new_viewport_y() - OFFSET_CORRECTION);
	}
}

// Scrolls the score vertically, but only as far as the score reaches: the top of the
// score cannot move below the top of the window, and the bottom of the score cannot
// move above the bottom of the window. There is no horizontal scrolling (the page is
// as wide as the window).
// Lomse asks for the file of a text font it does not know (e.g. the "FreeSerif" named
// in the scores exported by MuseScore). Without an answer the texts of the score (the
// instrument names, the measure numbers, the tempo) are not drawn at all, so one of the
// fonts shipped with the program is given.
static void AnswerFontRequest(Request* request, const String& resourcesPath)
{
	if (request == nullptr || !request->is_get_font_filename())
	{
		return;
	}
	RequestFont* fontRequest = static_cast<RequestFont*>(request);
	const String name(fontRequest->get_fontname());
	const String family = name.containsIgnoreCase("sans") || name.containsIgnoreCase("arial") ||
		name.containsIgnoreCase("helvetica") ? "LiberationSans" : "LiberationSerif";
	const String style = fontRequest->get_bold() ? (fontRequest->get_italic() ? "BoldItalic" : "Bold") :
		(fontRequest->get_italic() ? "Italic" : "Regular");
	fontRequest->set_font_fullname(NativePath(resourcesPath + "/fonts/" + family + "-" + style + ".ttf"));
}

void LomseScoreComponent::LomseRequest(Request* request)
{
	AnswerFontRequest(request, m_settings.resourcesPath);
}

void LomseScoreComponent::SetViewport(int y)
{
	if (!m_presenter || !m_image) return;

	SpInteractor interactor = m_presenter->get_interactor(0).lock();
	int bottom = ScoreBottom();
	if (bottom < 0)
	{
		Pixels viewWidth = 0, viewHeight = 0;
		interactor->get_view_size(&viewWidth, &viewHeight);
		bottom = int(viewHeight);
	}
	const int maxY = std::max(0, bottom - m_image->getHeight());
	y = jlimit(0, maxY, y);

	Pixels curX = 0, curY = 0;
	interactor->get_viewport(&curX, &curY);
	if (curX != 0 || curY != y)
	{
		interactor->new_viewport(0, y);
	}
}

// Returns the position (in view pixels) of the bottom of the last system of the score,
// plus a small margin, or -1 if unknown. The last page is as high as the window, so its
// empty part below the last system (and the gap after the pages) is not scrolled to.
int LomseScoreComponent::ScoreBottom()
{
	SpInteractor interactor = m_presenter->get_interactor(0).lock();
	GraphicModel* model = interactor->get_graphic_model();
	if (!model || model->get_num_pages() == 0) return -1;

	const int lastPage = model->get_num_pages() - 1;
	LUnits bottom = 0.0f;
	std::function<void(GmoBox*)> findLastSystem = [&](GmoBox* box)
		{
			for (GmoBox* child : box->get_child_boxes())
			{
				if (child->is_box_system())
					bottom = std::max(bottom, child->get_bottom());
				else
					findLastSystem(child);
			}
		};
	findLastSystem(model->get_page(lastPage));
	if (bottom <= 0.0f) return -1;

	const LUnits BottomMargin = 500.0f; // the same as the bottom margin of the page
	double x = 0.0;
	double y = double(bottom + BottomMargin);
	interactor->model_point_to_device(&x, &y, lastPage); // relative to the current viewport

	Pixels viewportX = 0, viewportY = 0;
	interactor->get_viewport(&viewportX, &viewportY);
	return int(y) + int(viewportY);
}

// Moves the current scroll position back into the allowed range (after dragging).
void LomseScoreComponent::LimitViewport()
{
	if (!m_presenter || !m_image) return;

	SpInteractor interactor = m_presenter->get_interactor(0).lock();
	Pixels x = 0, y = 0;
	interactor->get_viewport(&x, &y);
	SetViewport(int(y));
}

void LomseScoreComponent::resized()
{
	if (m_presenter)
	{
		PrepareImage();
	}

	loadButton->setBounds(getWidth() / 2 - 50, 30, 100, 30);
}

void LomseScoreComponent::paint(Graphics& g)
{
	if (m_presenter && m_image)
	{
		g.drawImage(*m_image, 0, 0, getWidth(), getHeight(), 0, 0, m_image->getWidth(), m_image->getHeight());
	}
	else if (m_presenter && !m_image)
	{
		String text = TRANS("An error occured when drawing the score:") + "\n" + m_error;
		g.setColour(Colours::white);
		g.fillRect(0, 0, getWidth(), getHeight());
		g.setColour(Colours::red);
		g.setFont(16);
		juce::Rectangle<int> rec(20, 80, getWidth() - 40, getHeight() - 100);
		g.drawFittedText(text, rec, Justification::centredTop, 100, 1);
	}
	else if (m_checkedScore != File())
	{
		g.setColour(Colour(167,172,176));
		g.setFont(16);
		juce::Rectangle<int> rec(20, 80, getWidth() - 40, getHeight() - 100);
		g.drawFittedText(TRANS("Checking the score..."), rec, Justification::centredTop, 100, 1);
	}
	else if (m_faultyScore.isNotEmpty())
	{
		g.setColour(Colour(230,120,90));
		g.setFont(16);
		juce::Rectangle<int> rec(20, 80, getWidth() - 40, getHeight() - 100);
		g.drawFittedText(TRANS("Faulty score file: NAME").replace("NAME", m_faultyScore),
			rec, Justification::centredTop, 100, 1);
	}
	else
	{
		String text = TRANS(
			"To automatically load score for a song put the score-file in MusicXML format near MIDI-file. "
			"The score-file should have the same name as MIDI-file and extension .musicxml, .xml or .mxl.");
		g.setColour(Colour(167,172,176));
		g.setFont(16);
		juce::Rectangle<int> rec(20, 80, getWidth() - 40, getHeight() - 100);
		g.drawFittedText(text, rec, Justification::centredTop, 100, 1);
	}
}

void LomseScoreComponent::buttonClicked(Button* buttonThatWasClicked)
{
	if (buttonThatWasClicked == loadButton.get())
	{
		ChooseScoreFile();
	}
	else if (buttonThatWasClicked == menuButton.get())
	{
		ShowMenu();
	}
}

void LomseScoreComponent::ChooseScoreFile()
{
	const String songPath = m_pianoController.GetSongName().replaceCharacter('\\', '/');
	const Song* presetSong = songPath.startsWith("/SONG/") ? Presets::FindSong("PRESET:" + songPath) : nullptr;
	String songName = presetSong != nullptr ? presetSong->title :
		File::createFileWithoutCheckingPath(m_pianoController.GetSongName()).getFileNameWithoutExtension();
	String title = songName == "" ? TRANS("Please select the score") :
		TRANS("Please select the score for SONGNAME").replace("SONGNAME", songName);

	GuiHelper::ShowFileOpenDialogAsync(title, m_settings.workingDirectory, "*.xml;*.musicxml;*.mxl",
		[this, self = Component::SafePointer<Component>(this)](const URL& url)
		{
			if (self == nullptr) return; // deleted meanwhile
			m_settings.workingDirectory = url.getLocalFile().getParentDirectory().getFullPathName();
			m_settings.Save();
			GuiHelper::CallAsync(this, [=](){LoadScore(url);});
    	});
}

void LomseScoreComponent::LoadScore(const URL& url)
{
	// generate access token on sandboxed platforms (iOS)
	std::unique_ptr<juce::InputStream> inp(url.createInputStream(false));

	// A score chosen by the user brings its song with it: if a MIDI file with the same
	// name is next to the score and it is not the loaded song, it is loaded too. The
	// score is shown when the song has been loaded (see LoadSong).
	const File score = url.getLocalFile();
	File song = score.withFileExtension(".mid");
	if (!song.existsAsFile())
	{
		song = score.withFileExtension(".midi");
	}
	if (song.existsAsFile() && File::createFileWithoutCheckingPath(m_pianoController.GetSongName()) != song)
	{
		m_chosenScore = score;
		if (m_pianoController.LoadSong(song))
		{
			// remembered for the next start, as when the song is opened in the left panel
			m_settings.lastSong = song.getFullPathName();
			m_settings.Save();
			return;
		}
		m_chosenScore = File();
	}

	LoadScore(score);
}

void LomseScoreComponent::mouseDown(const MouseEvent& event)
{
	if (!m_presenter || !m_image) return;

	SpInteractor interactor = m_presenter->get_interactor(0).lock();
	interactor->on_mouse_button_down(int(event.getMouseDownScreenX() * m_scale),
		int(event.getScreenY() * m_scale), GetMouseFlags(event));
}

void LomseScoreComponent::mouseUp(const MouseEvent& event)
{
	if (!m_presenter || !m_image) return;

	SpInteractor interactor = m_presenter->get_interactor(0).lock();
	interactor->on_mouse_button_up(int(event.getMouseDownScreenX() * m_scale),
		int(event.getScreenY() * m_scale), GetMouseFlags(event));
}

void LomseScoreComponent::mouseMove(const MouseEvent& event)
{
	if (!m_presenter || !m_image) return;

	SpInteractor interactor = m_presenter->get_interactor(0).lock();
	interactor->on_mouse_move(int(event.getMouseDownScreenX() * m_scale),
		int(event.getScreenY() * m_scale), GetMouseFlags(event));
}

// The measure (0..n) at a point of the score image, -1 if the point is not on a staff.
// In the first measure of a system Lomse gives measure 0 with the time position counted
// from the beginning of the score (and nothing at all on the clef and the key signature),
// so this case is worked out here: the first point to the right with a time position is
// taken and its measure is looked up by that position.
int LomseScoreComponent::MeasureAtPoint(SpInteractor& interactor, ImoScore* score, int x, int y)
{
	for (int probe = x; probe < m_image->getWidth(); probe += 6)
	{
		const MeasureLocator locator = interactor->find_click_info_at(probe, y).ml;
		if (locator.iMeasure != 0 || score == nullptr)
		{
			return locator.iMeasure;
		}
		if (locator.location > 0.0)
		{
			const TimeUnits secondMeasure = ScoreAlgorithms::get_timepos_for(score, 1, 0);
			return secondMeasure <= 0.0 || locator.location <= secondMeasure ? 0 :
				ScoreAlgorithms::get_locator_for(score, locator.location).iMeasure;
		}
	}
	return 0;
}

// Double click on the score: the playback jumps to the beginning of the clicked measure
// (with repeats in the score: to its occurrence chosen by PlayedMeasure).
void LomseScoreComponent::mouseDoubleClick(const MouseEvent& event)
{
	if (!m_presenter || !m_image || getWidth() <= 0 || getHeight() <= 0) return;

	SpInteractor interactor = m_presenter->get_interactor(0).lock();

	// position in the pixels of the score image
	const int x = event.x * m_image->getWidth() / getWidth();
	const int y = event.y * m_image->getHeight() / getHeight();

	// Lomse finds the measure only on the staves themselves; for a click between or near
	// the staves the nearest staff above or below is taken
	Document* doc = m_presenter->get_document_raw_ptr();
	ImoScore* score = dynamic_cast<ImoScore*>(doc->get_im_root()->get_content_item(0));
	const int range = jmax(20, m_image->getHeight() / 12);
	int measure = -1;
	for (int distance = 0; distance <= range && measure < 0; distance += 3)
	{
		measure = MeasureAtPoint(interactor, score, x, y - distance);
		if (measure < 0 && distance > 0)
		{
			measure = MeasureAtPoint(interactor, score, x, y + distance);
		}
	}
	if (measure < 0) return;

	const int played = PlayedMeasure(measure);
	if (played < 0) return; // the measure is not played (e.g. after Fine)

	m_pianoController.SetPosition({played + 1, 1});
}

// The song is played in the playback order of the score (with its repeats) if its length
// is nearer to the length of the playback order than to the number of measures of the
// score. A MIDI file may also have been made without the repeats: then its measures are
// those of the score.
bool LomseScoreComponent::FollowsPlaybackOrder()
{
	if (m_playOrder.empty())
	{
		return false;
	}
	const int length = m_pianoController.IsSongLoaded() ? m_pianoController.GetLength().measure : 0;
	if (length <= 0)
	{
		return true; // not known
	}
	return std::abs(length - (int)m_playOrder.size()) < std::abs(length - m_scoreMeasures);
}

// The measure of the score (0..) shown for a measure of the song (0..).
int LomseScoreComponent::ScoreMeasure(int playedMeasure)
{
	if (playedMeasure < 0 || !FollowsPlaybackOrder())
	{
		// the song may go on after the score (e.g. an empty measure at its end): the
		// position line stays in the last measure of the score
		return m_scoreMeasures > 0 ? std::min(playedMeasure, m_scoreMeasures - 1) : playedMeasure;
	}
	return m_playOrder[std::min(playedMeasure, (int)m_playOrder.size() - 1)];
}

// The measure of the song (0..) to jump to for a measure of the score (0..), or -1 if the
// measure is not played. A measure played more than once (with repeats): its occurrence in
// the current pass (the part played straight through, in which the playback position is),
// otherwise the occurrence nearest to the playback position (the later one of two equally
// near).
int LomseScoreComponent::PlayedMeasure(int scoreMeasure)
{
	if (!FollowsPlaybackOrder())
	{
		return scoreMeasure;
	}
	const int count = (int)m_playOrder.size();
	const int current = jlimit(0, count - 1, m_pianoController.GetPosition().measure - 1);

	const auto nextPass = std::upper_bound(m_passStarts.begin(), m_passStarts.end(), current);
	const int passStart = nextPass == m_passStarts.begin() ? 0 : *(nextPass - 1);
	const int passEnd = nextPass == m_passStarts.end() ? count : *nextPass;
	for (int i = passStart; i < passEnd; i++)
	{
		if (m_playOrder[i] == scoreMeasure)
		{
			return i;
		}
	}

	int nearest = -1;
	for (int i = 0; i < count; i++)
	{
		if (m_playOrder[i] == scoreMeasure && (nearest < 0 || std::abs(i - current) <= std::abs(nearest - current)))
		{
			nearest = i;
		}
	}
	return nearest;
}

void LomseScoreComponent::mouseDrag(const MouseEvent& event)
{
	mouseMove(event);
	LimitViewport();
}

void LomseScoreComponent::mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& details)
{
	if (!m_presenter || !m_image) return;

	// the same distance as before, when the wheel was simulated as dragging the view,
	// but limited to the extent of the score
	float scrollY = details.deltaY * 256;

	SpInteractor interactor = m_presenter->get_interactor(0).lock();
	Pixels x = 0, y = 0;
	interactor->get_viewport(&x, &y);
	SetViewport(int(y) - int(scrollY * m_scale));
}

unsigned LomseScoreComponent::GetMouseFlags(const MouseEvent& event)
{
	unsigned mouseFlags = 0;
	if (event.mods.isLeftButtonDown()) mouseFlags |= k_mouse_left;
	if (event.mods.isRightButtonDown()) mouseFlags |= k_mouse_right;
	if (event.mods.isMiddleButtonDown()) mouseFlags |= k_mouse_middle;
	if (event.mods.isShiftDown()) mouseFlags |= k_kbd_shift;
	if (event.mods.isAltDown()) mouseFlags |= k_kbd_alt;
	if (event.mods.isCtrlDown()) mouseFlags |= k_kbd_ctrl;
	return mouseFlags;
}

void LomseScoreComponent::PianoStateChanged(PianoController::Aspect aspect, PianoController::Channel channel)
{
	if (aspect == PianoController::apPosition || aspect == PianoController::apLoop)
	{
		GuiHelper::CallAsync(this, [=](){UpdateSongState();});
	}
	else if (aspect == PianoController::apSongLoaded ||
		(aspect == PianoController::apSongName && m_firstSync))
	{
		m_firstSync = false;
		GuiHelper::CallAsync(this, [=](){LoadSong();});
	}
	else if (aspect == PianoController::apPartChannel &&
		m_settings.scorePart != Settings::spAll &&
		(channel == m_pianoController.GetPartChannel(PianoController::paRight) ||
		channel == m_pianoController.GetPartChannel(PianoController::paLeft)))
	{
		GuiHelper::CallAsync(this, [=](){UpdateInstruments(false);});
	}
}

void LomseScoreComponent::UpdateSongState()
{
	if (!m_presenter || !m_image) return;

	UpdateABMarks(false);
	UpdateTempoLine(true);
}

void LomseScoreComponent::UpdateTempoLine(bool scroll)
{
	// highlight playback position
	SpInteractor interactor = m_presenter->get_interactor(0).lock();
	PianoController::Position songPosition = m_pianoController.GetPosition();
	Document* doc = m_presenter->get_document_raw_ptr();
	ImoScore* score = dynamic_cast<ImoScore*>(doc->get_im_root()->get_content_item(0));
	const int measure = ScoreMeasure(songPosition.measure - 1);
	const TimeUnits location = BeatLocation(score, measure, songPosition.beat - 1);
	if (scroll)
	{
		interactor->move_tempo_line_and_scroll_if_necessary(m_scoreId, measure, location);
	}
	else
	{
		interactor->move_tempo_line(m_scoreId, measure, location);
	}
}

// The piano counts the beats by the bottom number of the time signature (six beats in
// 6/8), Lomse by the implied beat (two beats in 6/8). Returns the place of a beat of the
// piano inside its measure, in Lomse time units.
TimeUnits LomseScoreComponent::BeatLocation(ImoScore* score, int measure, int beat)
{
	if (score == nullptr || score->get_num_instruments() == 0 || measure < 0 || beat <= 0)
	{
		return 0.0;
	}
	ImMeasuresTable* table = score->get_instrument(0)->get_measures_table();
	ImMeasuresTableEntry* entry = table != nullptr ? table->get_measure(measure) : nullptr;
	if (entry == nullptr)
	{
		return 0.0;
	}
	TimeUnits location = entry->get_bottom_ts_beat_duration() * beat;

	// A pickup (an incomplete first measure) is the end of a whole measure for the piano:
	// a MIDI file starts its first measure with the missing beats (Yamaha files are like
	// that), so the beats of the piano are counted from the start of the whole measure.
	// Without this the position line ran past the pickup into the second measure and
	// jumped back when the piano reached it.
	ColStaffObjs* staffObjs = score->get_staffobjs_table();
	if (measure == 0 && staffObjs != nullptr && staffObjs->is_anacrusis_start())
	{
		location = std::max(0.0, location - staffObjs->anacrusis_missing_time());
	}
	return location;
}

// Time position of a beat of the piano in the score.
TimeUnits LomseScoreComponent::BeatTimepos(ImoScore* score, int measure, int beat)
{
	if (score == nullptr)
	{
		return 0.0;
	}
	return ScoreAlgorithms::get_timepos_for(score, measure, 0) + BeatLocation(score, measure, beat);
}

void LomseScoreComponent::UpdateABMarks(bool force)
{
	SpInteractor interactor = m_presenter->get_interactor(0).lock();
	Document* doc = m_presenter->get_document_raw_ptr();
	ImoScore* score = dynamic_cast<ImoScore*>(doc->get_im_root()->get_content_item(0));

	// highlight AB-Loop
	PianoController::Loop curLoop = m_pianoController.GetLoop();
	PianoController::Position curLoopStart = m_pianoController.GetLoopStart();
	if (!(loop.begin == curLoop.begin && loop.end == curLoop.end && loopStart == curLoopStart) || force)
	{
		loop = curLoop;
		loopStart = curLoopStart;

		interactor->remove_mark(loopStartMark);
		interactor->remove_mark(loopEndMark);

		if (loop.begin.measure > 0 || loopStart.measure > 0)
		{
			TimeUnits timepos = BeatTimepos(score,
				ScoreMeasure(loop.begin.measure > 0 ? loop.begin.measure - 1 : loopStart.measure - 1),
				loop.begin.measure > 0 ? loop.begin.beat - 1 : loopStart.beat - 1);
			loopStartMark = interactor->add_fragment_mark_at_note_rest(m_scoreId, timepos);
			loopStartMark->color(Color(15, 90, 235, 128)); // light orange
			loopStartMark->type(k_mark_open_rounded);
			loopStartMark->x_shift(-5);
		}

		if (loop.end.measure > 0)
		{
			TimeUnits timepos = BeatTimepos(score, ScoreMeasure(loop.end.measure - 1), loop.end.beat - 1);
			timepos -= 1;
			loopEndMark = interactor->add_fragment_mark_at_note_rest(m_scoreId, timepos);
			loopEndMark->color(Color(15, 90, 235, 128)); // light orange
			loopEndMark->type(k_mark_close_rounded);
		}
	}
}

void LomseScoreComponent::LoadSong()
{
	Cleanup();
	m_check.reset(); // the check of the score of the previous song
	m_checkNumber++;
	m_checkedScore = File();
	m_faultyScore = String();

	// the score chosen by the user for this song comes before the scores found by name
	const File chosenScore = m_chosenScore;
	m_chosenScore = File();
	if (chosenScore.existsAsFile() && chosenScore.getFileNameWithoutExtension() ==
		File::createFileWithoutCheckingPath(m_pianoController.GetSongName()).getFileNameWithoutExtension())
	{
		LoadScore(chosenScore);
		return;
	}

	// a song of the piano (loaded by its path): its score is looked for in the songs folder
	const String songPath = m_pianoController.GetSongName().replaceCharacter('\\', '/');
	if (songPath.startsWith("/SONG/"))
	{
		const Song* song = Presets::FindSong("PRESET:" + songPath);
		const File score = song != nullptr ? m_settings.GetSongScore(*song) : File();
		if (score.existsAsFile())
		{
			LoadScore(score);
		}
		else
		{
			loadButton->setVisible(m_presenter == nullptr);
			repaint();
		}
		return;
	}

	File file = File(m_pianoController.GetSongName()).withFileExtension(".musicxml");
	if (!file.existsAsFile())
	{
		file = File(m_pianoController.GetSongName()).withFileExtension(".xml");
	}
	if (!file.existsAsFile())
	{
		file = File(m_pianoController.GetSongName()).withFileExtension(".mxl");
	}

	if (file.existsAsFile() && file.getSize() > 0)
	{
		LoadScore(file);
		return;
	}
	else
	{
		loadButton->setVisible(m_presenter == nullptr);
		repaint();
	}
}

// The check of a score before it is shown: the score is drawn by a separate process (the
// program started with --check-score, see ScoreComponent::CheckScoreFile). If Lomse freezes
// or crashes on the score, only that process stops, and the score is not loaded. The
// callback is called on the thread of the check, not on the message thread.
class LomseScoreComponent::ScoreCheck : public Thread
{
public:
	using Callback = std::function<void(const File& file, bool ok, const String& reason, const String& details)>;

	ScoreCheck(const File& file, Callback callback) :
		Thread("Score check"), m_file(file), m_callback(std::move(callback))
	{
		startThread();
	}

	~ScoreCheck() override
	{
		stopThread(3000); // the check process is stopped (see run)
	}

	void run() override
	{
		// the reason of an error is written into this file by the check process (its standard
		// output is not read: Lomse writes many notes there, a full pipe would stop the process)
		TemporaryFile resultFile(".txt");
		ChildProcess process;
		const StringArray arguments{ File::getSpecialLocation(File::currentExecutableFile).getFullPathName(),
			ScoreComponent::CheckScoreArgument, m_file.getFullPathName(), resultFile.getFile().getFullPathName() };
		if (!process.start(arguments, 0))
		{
			m_callback(m_file, true, String(), "the check process could not be started");
			return;
		}

		const uint32 start = Time::getMillisecondCounter();
		while (!process.waitForProcessToFinish(50))
		{
			if (threadShouldExit())
			{
				process.kill(); // another score is loaded, or the program is closed
				return;
			}
			if (Time::getMillisecondCounter() - start > (uint32)TimeoutMs)
			{
				process.kill();
				m_callback(m_file, false,
					TRANS("Drawing the score did not finish in SECONDS seconds (it froze).")
						.replace("SECONDS", String(TimeoutMs / 1000)),
					"drawing did not finish in " + String(TimeoutMs) + " ms (frozen), the check process was stopped");
				return;
			}
		}

		const uint32 exitCode = process.getExitCode();
		const String output = resultFile.getFile().loadFileAsString().trim();
		juce::Logger::writeToLog("[SCORE] Checked " + m_file.getFileName() + " in " +
			String(Time::getMillisecondCounter() - start) + " ms, exit code " + String((int)exitCode));
		if (exitCode == 0)
		{
			m_callback(m_file, true, String(), String());
		}
		else if (exitCode == ScoreComponent::CheckScoreError && output.isNotEmpty())
		{
			m_callback(m_file, false, TRANS("Drawing the score failed: ERROR").replace("ERROR", output),
				"Lomse error: " + output);
		}
		else
		{
			m_callback(m_file, false, TRANS("The score viewer crashed while drawing the score."),
				"the check process crashed, exit code 0x" + String::toHexString((int)exitCode) +
				(output.isNotEmpty() ? ", output: " + output : String()));
		}
	}

private:
	// the time a score may take to be drawn (only a limit: a score drawn sooner is shown at
	// once; a large score may take several seconds on a slower computer); the Debug build is
	// much slower
#if JUCE_DEBUG
	static constexpr int TimeoutMs = 45000;
#else
	static constexpr int TimeoutMs = 15000;
#endif

	File m_file;
	Callback m_callback;
};

// The check process (the program started with --check-score): draws the score as the score
// window does and returns 0 if it went well; the reason of an error is written into
// resultFile. It writes nothing anywhere else.
int ScoreComponent::CheckScoreFile(const File& file, const File& resultFile)
{
	if (!file.existsAsFile())
	{
		resultFile.replaceWithText("the score file was not found");
		return CheckScoreError;
	}
	const String resourcesPath = Settings().resourcesPath;
	try
	{
		static std::ostream nullLog(nullptr);
		LomseDoorway lomse(&nullLog, &nullLog);
		lomse.init_library(k_pix_format_rgba32, 96, false);
		lomse.set_default_fonts_path(NativePath(resourcesPath + "/fonts/"));
		static String fontsPath;
		fontsPath = resourcesPath;
		lomse.set_request_callback(nullptr, [](void*, Request* request) { AnswerFontRequest(request, fontsPath); });

		String content;
		bool asText = false;
		if (!PrepareScoreText(file.getFullPathName(), content, asText))
		{
			resultFile.replaceWithText("the compressed file has no score in it");
			return CheckScoreError;
		}
		std::unique_ptr<Presenter> presenter(asText ?
			lomse.new_document(k_view_vertical_book, content.toStdString(), Document::k_format_mxl) :
			lomse.open_document(k_view_vertical_book, file.getFullPathName().toStdString()));
		SpInteractor interactor = presenter->get_interactor(0).lock();

		// the playback order of the measures is worked out, and the instruments are taken
		// out of the score and put back, as the score window does
		ImoDocument* imoDoc = presenter->get_document_raw_ptr()->get_im_root();
		if (ImoScore* score = dynamic_cast<ImoScore*>(imoDoc->get_content_item(0)))
		{
			std::vector<int> order, passStarts;
			int numMeasures = 0;
			ReadPlaybackOrder(score, order, passStarts, numMeasures);

			std::vector<ImoInstrument*> instruments;
			while (score->get_num_instruments() > 0)
			{
				ImoInstrument* instr = score->get_instrument(0);
				instr->set_measures_numbering(ImoInstrument::k_system);
				instruments.push_back(instr);
				score->get_instruments()->remove_child(instr);
			}
			score->end_of_changes();
			for (ImoInstrument* instr : instruments)
			{
				score->add_instrument(instr);
			}
			score->end_of_changes();
		}

		// a page of a usual window size (1200 x 800 pixels), as in PrepareImage
		const int width = 1200;
		const int height = 800;
		ImoPageInfo* pageInfo = imoDoc->get_page_info();
		imoDoc->set_page_content_scale(1.0);
		pageInfo->set_page_width(LUnits(width) * 26.5f);
		pageInfo->set_page_height(LUnits(height) * 26.5f);
		pageInfo->set_top_margin_odd(500);
		pageInfo->set_left_margin_odd(300);
		pageInfo->set_right_margin_odd(300);
		pageInfo->set_bottom_margin_odd(500);
		pageInfo->set_top_margin_even(500);
		pageInfo->set_left_margin_even(300);
		pageInfo->set_right_margin_even(300);
		pageInfo->set_bottom_margin_even(500);
		interactor->on_document_updated();

		const float scale = std::max(0.01f, imoDoc->get_page_content_scale());
		const unsigned bitmapWidth = (unsigned)std::max(1, int(width / scale));
		const unsigned bitmapHeight = (unsigned)std::max(1, int(height / scale));
		std::vector<unsigned char> bitmap((size_t)bitmapWidth * bitmapHeight * 4);
		interactor->set_rendering_buffer(bitmap.data(), bitmapWidth, bitmapHeight);
		interactor->redraw_bitmap();
	}
	catch (const std::exception& e)
	{
		resultFile.replaceWithText(String(e.what()).substring(0, 500));
		return CheckScoreError;
	}
	catch (...)
	{
		resultFile.replaceWithText("unknown exception");
		return CheckScoreError;
	}
	return 0;
}

// A score is checked before it is shown (see ScoreCheck): a faulty score file is not
// loaded, a window tells about it and the reason is written into the log.
void LomseScoreComponent::LoadScore(const File& file)
{
	Cleanup();
	m_check.reset(); // an earlier check is not needed any more
	m_checkNumber++;
	m_checkedScore = File();
	m_faultyScore = String();

	// a file that is not well-formed XML is found without drawing it
	const bool compressed = file.hasFileExtension(".mxl");
	const String content = compressed ? ReadCompressedMusicXml(file) : file.loadFileAsString();
	if (compressed && content.isEmpty())
	{
		ReportFaultyScore(file, TRANS("The compressed score file (.mxl) has no score in it."),
			"no score in the zip archive");
		return;
	}
	pugi::xml_document xml;
	const std::string text = content.toStdString();
	const pugi::xml_parse_result parsed = xml.load_buffer(text.data(), text.size());
	if (!parsed)
	{
		const int line = 1 + (int)std::count(text.begin(), text.begin() +
			std::min((std::ptrdiff_t)text.size(), (std::ptrdiff_t)parsed.offset), '\n');
		ReportFaultyScore(file, TRANS("The file is not a readable MusicXML file (error in line LINE).")
			.replace("LINE", String(line)), "XML error in line " + String(line) + ": " + parsed.description());
		return;
	}

	if (IsCheckedScore(file))
	{
		ShowScore(file);
		return;
	}

	// drawn first by a check process (see ScoreCheck); the score is shown when it has finished
	m_checkedScore = file;
	loadButton->setVisible(false);
	repaint();
	const int number = m_checkNumber;
	m_check.reset(new ScoreCheck(file,
		[this, number, self = Component::SafePointer<Component>(this)](const File& checked, bool ok,
			const String& reason, const String& details)
		{
			MessageManager::callAsync([=]()
				{
					if (self != nullptr)
					{
						ScoreChecked(number, checked, ok, reason, details);
					}
				});
		}));
}

void LomseScoreComponent::ShowScore(const File& file)
{
	Cleanup();

	const uint32 start = Time::getMillisecondCounter();
	LoadDocument(file.getFullPathName());
	juce::Logger::writeToLog("[SCORE] Loaded " + file.getFileName() + " in " +
		String(Time::getMillisecondCounter() - start) + " ms");

	if (m_presenter)
	{
		PrepareImage();
	}

	loadButton->setVisible(m_presenter == nullptr);
	repaint();
}

void LomseScoreComponent::ScoreChecked(int number, const File& file, bool ok, const String& reason,
	const String& details)
{
	if (number != m_checkNumber)
	{
		return; // another score has been loaded meanwhile
	}
	m_check.reset();
	m_checkedScore = File();
	if (ok)
	{
		if (details.isNotEmpty())
		{
			// the check could not be done (e.g. the check process could not be started):
			// the score is shown as before the check existed
			juce::Logger::writeToLog("[SCORE] The score could not be checked: " + file.getFullPathName() + ": " + details);
		}
		else
		{
			AddCheckedScore(file);
		}
		ShowScore(file);
	}
	else
	{
		ReportFaultyScore(file, reason, details);
	}
}

void LomseScoreComponent::ReportFaultyScore(const File& file, const String& reason, const String& details)
{
	Cleanup();
	m_faultyScore = file.getFileName();
	loadButton->setVisible(true);
	repaint();

	juce::Logger::writeToLog("[SCORE] Faulty score file, not loaded: " + file.getFullPathName() + ": " + details);
	AlertWindow::showMessageBoxAsync(MessageBoxIconType::WarningIcon, TRANS("Faulty score file"),
		TRANS("The score could not be shown, so it was not loaded.") + "\n\n" +
		file.getFileName() + "\n" + reason);
}

// The scores that have been checked without a fault are remembered (with their size, date
// and the version of the program) in a file next to the settings, so a score is checked
// only when it is new or has changed.
static File CheckedScoresFile(const Settings& settings)
{
	return settings.GetLastStateFile().getSiblingFile("CheckedScores.txt");
}

String LomseScoreComponent::CheckedScoreKey(const File& file) const
{
	// the revision of the check: increased when the check does more (2: the playback order)
	// or the score is prepared differently (3: chord symbols moved by their offset), so the
	// scores already checked are checked again
	const int checkRevision = 3;
	return JUCEApplication::getInstance()->getApplicationVersion() + "/" + String(checkRevision) + "\t" + file.getFullPathName() + "\t" +
		String(file.getSize()) + "\t" + String(file.getLastModificationTime().toMilliseconds());
}

bool LomseScoreComponent::IsCheckedScore(const File& file)
{
	if (!m_checkedScoresRead)
	{
		m_checkedScoresRead = true;
		CheckedScoresFile(m_settings).readLines(m_checkedScores);
		m_checkedScores.removeEmptyStrings();
	}
	return m_checkedScores.contains(CheckedScoreKey(file));
}

void LomseScoreComponent::AddCheckedScore(const File& file)
{
	IsCheckedScore(file); // reads the file if not yet read
	const String key = CheckedScoreKey(file);
	const String prefix = key.upToLastOccurrenceOf("\t", false, false).upToLastOccurrenceOf("\t", false, false) + "\t";
	for (int i = m_checkedScores.size() - 1; i >= 0; i--)
	{
		// an earlier state of the same file
		if (m_checkedScores[i].startsWith(prefix))
		{
			m_checkedScores.remove(i);
		}
	}
	m_checkedScores.add(key);
	const int MaxCheckedScores = 1000;
	if (m_checkedScores.size() > MaxCheckedScores)
	{
		m_checkedScores.removeRange(0, m_checkedScores.size() - MaxCheckedScores);
	}
	CheckedScoresFile(m_settings).replaceWithText(m_checkedScores.joinIntoString("\n") + "\n");
}

void LomseScoreComponent::ShowMenu()
{
	PopupMenu menu;
	menu.addSectionHeader(TRANS("SCORE"));
	menu.addItem(1, TRANS("Load Score"));
	menu.addSectionHeader(TRANS("PARTS"));
	menu.addItem(300 + Settings::spRight, TRANS("Right"), true, m_settings.scorePart == Settings::spRight);
	menu.addItem(300 + Settings::spLeft, TRANS("Left"), true, m_settings.scorePart == Settings::spLeft);
	menu.addItem(300 + Settings::spRightAndLeft, TRANS("Right and Left"), true, m_settings.scorePart == Settings::spRightAndLeft);
	menu.addItem(300 + Settings::spAll, TRANS("All"), true, m_settings.scorePart == Settings::spAll);
	menu.addSectionHeader(TRANS("INSTRUMENT NAMES"));
	menu.addItem(100 + Settings::siHidden, TRANS("Hidden"), true, m_settings.scoreInstrumentNames == Settings::siHidden);
	menu.addItem(100 + Settings::siShort, TRANS("Short"), true, m_settings.scoreInstrumentNames == Settings::siShort);
	menu.addItem(100 + Settings::siMixed, TRANS("Mixed"), true, m_settings.scoreInstrumentNames == Settings::siMixed);
	menu.addItem(100 + Settings::siFull, TRANS("Full"), true, m_settings.scoreInstrumentNames == Settings::siFull);
	menu.addSeparator();
	menu.addItem(201, TRANS("Show MIDI-Channel"), true, m_settings.scoreShowMidiChannel);

	GuiHelper::ShowMenuAsync(menu, menuButton.get(),
		[this, self = Component::SafePointer<Component>(this)](int result)
		{
			if (self == nullptr) return; // deleted meanwhile
			const int group = result / 100;

			if (result == 1)
			{
				ChooseScoreFile();
			}
			else if (group == 1)
			{
				m_settings.scoreInstrumentNames = Settings::ScoreInstrumentNames(result - 100);
			}
			else if (group == 2)
			{
				m_settings.scoreShowMidiChannel = !m_settings.scoreShowMidiChannel;
			}
			else if (group == 3)
			{
				m_settings.scorePart = Settings::ScorePart(result - 300);
			}

			if (group == 1 || group == 2 || group == 3)
			{
				m_settings.Save();
			}
		});
}

void LomseScoreComponent::UpdateInstruments(bool force)
{
	if (m_presenter && (force ||
		m_rightChannel != m_pianoController.GetPartChannel(PianoController::paRight) ||
		m_leftChannel != m_pianoController.GetPartChannel(PianoController::paLeft)))
	{
		ConfigureInstruments();

		m_rightChannel = m_pianoController.GetPartChannel(PianoController::paRight);
		m_leftChannel = m_pianoController.GetPartChannel(PianoController::paLeft);

		PrepareImage();
		repaint();
	}
}

void LomseScoreComponent::PrepareInstruments()
{
	m_instrNames.clear();
	m_instrAbbrevs.clear();
	m_instruments.clear();
	m_instrgroups.clear();

	ImoDocument* imoDoc = m_presenter->get_document_raw_ptr()->get_im_root();
	ImoScore* score = dynamic_cast<ImoScore*>(imoDoc->get_content_item(0));
	if (!score) return;

	m_scoreId = score->get_id();
	while (score->get_num_instruments() > 0)
	{
		ImoInstrument* instr = score->get_instrument(0);

		m_instruments.push_back(instr);
		m_instrNames.push_back(instr->get_name().text);
		m_instrAbbrevs.push_back(instr->get_abbrev().text);

		//show measure numbers
		instr->set_measures_numbering(ImoInstrument::k_system);

		// hide instrument
		score->get_instruments()->remove_child(instr);
	}

	while (score->get_instrument_groups() &&
		score->get_instrument_groups()->get_num_items() > 0)
	{
		ImoInstrGroup* group = (ImoInstrGroup*)score->get_instrument_groups()->get_child(0);
		m_instrgroups.push_back(group);
		score->get_instrument_groups()->remove_child(group);
	}

	score->end_of_changes();
}

void LomseScoreComponent::ConfigureInstruments()
{
	m_scoreInstrumentNames = m_settings.scoreInstrumentNames;
	m_scorePart = m_settings.scorePart;
	m_scoreShowMidiChannel = m_settings.scoreShowMidiChannel;

	ImoDocument* imoDoc = m_presenter->get_document_raw_ptr()->get_im_root();
	ImoScore* score = dynamic_cast<ImoScore*>(imoDoc->get_content_item(0));
	if (!score) return;

	// hide all instruments
	while (score->get_num_instruments() > 0)
	{
		ImoInstrument* instr = score->get_instrument(0);
		score->get_instruments()->remove_child(instr);
	}

	// hide all groups
	while (score->get_instrument_groups() &&
		score->get_instrument_groups()->get_num_items() > 0)
	{
		ImoInstrGroup* group = (ImoInstrGroup*)score->get_instrument_groups()->get_child(0);
		score->get_instrument_groups()->remove_child(group);
	}

	int num = 0;
	for (ImoInstrument* instr : m_instruments)
	{
		//hide instrument names if necessary or restore names if were previously hidden
		switch (m_settings.scoreInstrumentNames)
		{
			case Settings::siHidden:
				instr->set_name("");
				instr->set_abbrev("");
				break;
			case Settings::siShort:
				instr->set_name(m_instrAbbrevs[num]);
				instr->set_abbrev(m_instrAbbrevs[num]);
				break;
			case Settings::siMixed:
				instr->set_name(m_instrNames[num]);
				instr->set_abbrev(m_instrAbbrevs[num]);
				break;
			case Settings::siFull:
				instr->set_name(m_instrNames[num]);
				instr->set_abbrev(m_instrNames[num]);
				break;
		}

		// add midi-channel(s) to instrument name
		int midiChannel = MidiChannelOfInstrument(instr);
		if (midiChannel > 0 && m_settings.scoreShowMidiChannel)
		{
			instr->set_name((String("#") + String(midiChannel) + " " + String(instr->get_name().text)).toStdString());
			instr->set_abbrev((String("#") + String(midiChannel) + " " + String(instr->get_abbrev().text)).toStdString());
		}

		num++;
	}

	int rightChannel = m_pianoController.GetPartChannel(PianoController::paRight) - PianoController::chMidi0;
	int leftChannel = m_pianoController.GetPartChannel(PianoController::paLeft) - PianoController::chMidi0;

	// add right hand part to score
	for (ImoInstrument* instr : m_instruments)
	{
		int midiChannel = MidiChannelOfInstrument(instr);
		if (midiChannel == rightChannel &&
			(m_settings.scorePart == Settings::spRight || m_settings.scorePart == Settings::spRightAndLeft))
		{
			score->add_instrument(instr);
		}
	}

	// add left hand part to score
	for (ImoInstrument* instr : m_instruments)
	{
		int midiChannel = MidiChannelOfInstrument(instr);
		if (midiChannel == leftChannel &&
			(m_settings.scorePart == Settings::spLeft || m_settings.scorePart == Settings::spRightAndLeft))
		{
			score->add_instrument(instr);
		}
	}

	// add other parts to score
	for (ImoInstrument* instr : m_instruments)
	{
		if (m_settings.scorePart == Settings::spAll ||
			// add at least one instrument
			score->get_num_instruments() == 0)
		{
			score->add_instrument(instr);
		}
	}

	// add groups for added instruments
	for (ImoInstrGroup* group : m_instrgroups)
	{
		for (int i = 0; i < score->get_num_instruments(); i++)
		{
			ImoInstrument* instr = score->get_instrument(i);
			if (group->get_first_instrument() == instr)
			{
				score->add_instruments_group(group);
				break;
			}
		}
	}

	score->end_of_changes();
}

int LomseScoreComponent::MidiChannelOfInstrument(ImoInstrument* instr)
{
	int midiChannel = instr->get_num_sounds() > 0 && instr->get_sound_info(0)->get_midi_info() ?
		instr->get_sound_info(0)->get_midi_info()->get_midi_channel() + 1 : 0;

	return midiChannel;
}

void LomseScoreComponent::Cleanup()
{
	if (!m_presenter) return;

	ImoDocument* imoDoc = m_presenter->get_document_raw_ptr()->get_im_root();
	ImoScore* score = dynamic_cast<ImoScore*>(imoDoc->get_content_item(0));
	if (score)
	{
		// move all instruments back to score to ensure they are deleted with the score
		while (score->get_num_instruments() > 0)
		{
			score->get_instruments()->remove_child(score->get_instrument(0));
		}
		for (ImoInstrument* instr : m_instruments)
		{
			score->add_instrument(instr);
		}
	}

	m_presenter = nullptr; // this destroys the score object
}

void LomseScoreComponent::ApplySettings()
{
	if (m_scoreInstrumentNames != m_settings.scoreInstrumentNames ||
		m_scorePart != m_settings.scorePart ||
		m_scoreShowMidiChannel != m_settings.scoreShowMidiChannel)
	{
		UpdateInstruments(true);
	}
}

#endif
