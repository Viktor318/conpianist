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
#include <functional>

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

	void LoadDocument(String filename);
	void PrepareImage();
	LUnits ScaledUnits(int pixels);
	unsigned GetMouseFlags(const MouseEvent& event);
	void UpdateTempoLine(bool scroll);
	static TimeUnits BeatLocation(ImoScore* score, int measure, int beat);
	static TimeUnits BeatTimepos(ImoScore* score, int measure, int beat);
	int MeasureAtPoint(SpInteractor& interactor, ImoScore* score, int x, int y);
	void SetViewport(int y);
	void LimitViewport();
	int ScoreBottom();
	void UpdateABMarks(bool force);
	void BuildControls();
	void LoadScore(const File& file);
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

// Lomse does not show the chord symbols of MusicXML (<harmony>): they are written into the
// text as words above the staff (<direction>), at the same place, so they are shown like
// the other texts of the score. The file itself is not changed.
static String ChordSymbolsAsWords(const String& content)
{
	static const String openTag = "<harmony";
	static const String closeTag = "</harmony>";
	String result;
	result.preallocateBytes(content.getNumBytesAsUTF8() + 4096);
	int position = 0; // the text before this position is in the result
	for (int start = content.indexOf(openTag); start >= 0; start = content.indexOf(start + 1, openTag))
	{
		const juce_wchar next = content[start + openTag.length()];
		if (next != '>' && next != '/' && !CharacterFunctions::isWhitespace(next))
		{
			continue; // another element whose name begins the same way
		}
		const int tagEnd = content.indexOf(start, ">");
		if (tagEnd < 0)
		{
			break;
		}
		int end = tagEnd + 1;
		if (content[tagEnd - 1] != '/')
		{
			const int close = content.indexOf(tagEnd, closeTag);
			if (close < 0)
			{
				break;
			}
			end = close + closeTag.length();
		}
		std::unique_ptr<XmlElement> harmony = parseXML(content.substring(start, end));
		result += content.substring(position, start);
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
		const XmlElement* offset = harmony->getChildByName("offset");
		const XmlElement* staff = harmony->getChildByName("staff");
		// above the staff, higher than the notes on the top line usually reach
		result << "<direction placement=\"above\"><direction-type><words relative-y=\"40\" font-weight=\"bold\">"
			<< name.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")
			<< "</words></direction-type>"
			<< (offset != nullptr ? "<offset>" + offset->getAllSubText().trim() + "</offset>" : String())
			<< "<staff>" << (staff != nullptr ? staff->getAllSubText().trim() : String("1")) << "</staff></direction>";
	}
	result += content.substring(position);
	return result;
}

// A note of a chord (<chord/>) may leave out its voice (Sibelius writes it so): the voice is
// that of the first note of the chord. Lomse crashes when laying out such a score, so the
// voice of the note before is written into these notes. Returns false (and leaves the text
// as it is) if there is no such note.
static bool AddVoiceToChordNotes(String& content)
{
	static const String openTag = "<note";
	static const String closeTag = "</note>";
	// the elements after <voice> in a note, in their order (the voice is put before the first)
	static const char* const after[] = { "<type", "<dot", "<accidental", "<time-modification",
		"<stem", "<notehead", "<staff", "<beam", "<notations", "<lyric", "<play", "<listen" };
	String result;
	bool changed = false;
	String voice = "1";
	int position = 0; // the text before this position is in the result
	for (int start = content.indexOf(openTag); start >= 0; start = content.indexOf(start + 1, openTag))
	{
		const juce_wchar next = content[start + openTag.length()];
		if (next != '>' && !CharacterFunctions::isWhitespace(next))
		{
			continue; // another element (e.g. <notehead>, <notations>)
		}
		const int close = content.indexOf(start, closeTag);
		if (close < 0)
		{
			break;
		}
		const String note = content.substring(start, close);
		const int voiceStart = note.indexOf("<voice>");
		if (voiceStart >= 0)
		{
			voice = note.substring(voiceStart + 7, note.indexOf(voiceStart, "</voice>")).trim();
			continue;
		}
		if (!note.contains("<chord"))
		{
			continue;
		}
		int insert = note.length();
		for (const char* tag : after)
		{
			const int found = note.indexOf(tag);
			if (found >= 0 && found < insert)
			{
				insert = found;
			}
		}
		result += content.substring(position, start + insert);
		result += "<voice>" + voice + "</voice>";
		position = start + insert;
		changed = true;
	}
	if (changed)
	{
		result += content.substring(position);
		content = result;
	}
	return changed;
}

void LomseScoreComponent::LoadDocument(String filename)
{
	//first, we will create a 'presenter'. It takes care of creating and maintaining
	//all objects and relationships between the document, its views and the interactors
	//to interact with the view
	if (filename.isNotEmpty())
	{
		// load from file
		// The file is read here and passed to Lomse as text (MusicXML) in these cases:
		// - the file is a compressed MusicXML (.mxl): it is unpacked here;
		// - the path has non-ASCII characters (e.g. accented letters): Lomse cannot open
		//   such a file on Windows;
		// - the score has part groups (brackets joining the instruments, written e.g. by
		//   Dorico): Lomse cannot draw them if an instrument of the group is not shown,
		//   so the groups are left out;
		// - the score has chord symbols: Lomse does not show them, they are written into
		//   the text as words above the staff (see ChordSymbolsAsWords);
		// - a note of a chord has no voice (Sibelius): it is added (see AddVoiceToChordNotes).
		const bool compressed = filename.endsWithIgnoreCase(".mxl");
		String content = compressed ? ReadCompressedMusicXml(File(filename)) : File(filename).loadFileAsString();
		if (compressed && content.isEmpty())
		{
			// not a compressed MusicXML file: nothing is shown
			m_presenter.reset();
			return;
		}
		const bool asciiPath = CharPointer_ASCII::isValidString(filename.toRawUTF8(), (int)filename.getNumBytesAsUTF8());
		const bool hasGroups = content.contains("<part-group");
		const bool hasChords = content.contains("<harmony");
		if (hasChords)
		{
			content = ChordSymbolsAsWords(content);
		}
		const bool voicesAdded = AddVoiceToChordNotes(content);
		if (asciiPath && !hasGroups && !hasChords && !voicesAdded && !compressed)
		{
			m_presenter.reset(m_lomse.open_document(lomse::k_view_vertical_book, filename.toStdString()));
		}
		else
		{
			for (int start = content.indexOf("<part-group"); start >= 0; start = content.indexOf(start, "<part-group"))
			{
				const int tagEnd = content.indexOf(start, ">");
				if (tagEnd < 0)
				{
					break;
				}
				int end = tagEnd + 1;
				if (content[tagEnd - 1] != '/')
				{
					const int close = content.indexOf(tagEnd, "</part-group>");
					if (close < 0)
					{
						break;
					}
					end = close + (int)strlen("</part-group>");
				}
				content = content.substring(0, start) + content.substring(end);
			}
			m_presenter.reset(m_lomse.new_document(lomse::k_view_vertical_book,
				content.toStdString(), lomse::Document::k_format_mxl));
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

	PrepareInstruments();
	ConfigureInstruments();

	loop = {{0,0},{0,0}};
}

void LomseScoreComponent::PrepareImage()
{
	try
	{
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
void LomseScoreComponent::LomseRequest(Request* request)
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
	fontRequest->set_font_fullname(NativePath(m_settings.resourcesPath + "/fonts/" + family + "-" + style + ".ttf"));
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
// (with repeats in the score: to its first occurrence).
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

	m_pianoController.SetPosition({measure + 1, 1});
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
	const int measure = songPosition.measure - 1;
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
	return entry != nullptr ? entry->get_bottom_ts_beat_duration() * beat : 0.0;
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
				loop.begin.measure > 0 ? loop.begin.measure - 1 : loopStart.measure - 1,
				loop.begin.measure > 0 ? loop.begin.beat - 1 : loopStart.beat - 1);
			loopStartMark = interactor->add_fragment_mark_at_note_rest(m_scoreId, timepos);
			loopStartMark->color(Color(15, 90, 235, 128)); // light orange
			loopStartMark->type(k_mark_open_rounded);
			loopStartMark->x_shift(-5);
		}

		if (loop.end.measure > 0)
		{
			TimeUnits timepos = BeatTimepos(score, loop.end.measure - 1, loop.end.beat - 1);
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

void LomseScoreComponent::LoadScore(const File& file)
{
	Cleanup();

	LoadDocument(file.getFullPathName());

	if (m_presenter)
	{
		PrepareImage();
	}

	loadButton->setVisible(m_presenter == nullptr);
	repaint();
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
