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

#include "Settings.h"

Settings::Settings()
{
#if JUCE_IOS
	resourcesPath = File::getSpecialLocation(File::currentApplicationFile).getFullPathName();
#elif __APPLE__
	resourcesPath = File::getSpecialLocation(File::currentApplicationFile).getFullPathName() + "/Contents/Resources";
#elif JUCE_ANDROID
	PrepareResources();
	resourcesPath = File::getSpecialLocation(File::userApplicationDataDirectory).getChildFile("assets").getFullPathName();
#else
	resourcesPath = File::getSpecialLocation(File::currentApplicationFile).getParentDirectory().getFullPathName() + "/Resources";
#endif

	opt.storageFormat = PropertiesFile::StorageFormat::storeAsXML;
	opt.applicationName = "ConPianist";
	opt.osxLibrarySubFolder = "Application Support";
	opt.filenameSuffix = "settings";

	workingDirectory = File::getSpecialLocation(File::userHomeDirectory).getFullPathName();
}

void Settings::Save()
{
	PropertiesFile prop(opt);

	prop.setValue("PianoIp", pianoIp);
	prop.setValue("MidiPort", midiPort);
	prop.setValue("ZoomUi", zoomUi);
	prop.setValue("Window.X", windowPos.getX());
	prop.setValue("Window.Y", windowPos.getY());
	prop.setValue("Window.Width", windowPos.getWidth());
	prop.setValue("Window.Height", windowPos.getHeight());
	prop.setValue("Keyboard.Visible", keyboardVisible);
	prop.setValue("Keyboard.Channel", keyboardChannel);
	prop.setValue("Keyboard.Height", keyboardHeight);
	prop.setValue("Score.InstrumentNames", scoreInstrumentNames);
	prop.setValue("Score.Part", scorePart);
	prop.setValue("Score.ShowMidiChannel", scoreShowMidiChannel);
	prop.setValue("WorkingDirectory", workingDirectory);
	prop.setValue("Logging", logging);
	prop.setValue("RtpLogging", rtpLogging);
	prop.setValue("Language", language);
	prop.setValue("LastSong", lastSong);
	prop.setValue("MidiIn2", midiIn2);
	prop.setValue("MidiOut", midiOut);

	prop.save();
	sendChangeMessage();
}

void Settings::Load()
{
	PropertiesFile prop(opt);

	pianoIp = prop.getValue("PianoIp", pianoIp);
	midiPort = prop.getValue("MidiPort", midiPort);
	windowPos.setX(prop.getIntValue("Window.X", windowPos.getX()));
	windowPos.setY(prop.getIntValue("Window.Y", windowPos.getY()));
	windowPos.setWidth(prop.getIntValue("Window.Width", windowPos.getWidth()));
	windowPos.setHeight(prop.getIntValue("Window.Height", windowPos.getHeight()));
	zoomUi = prop.getDoubleValue("ZoomUi", zoomUi);
	keyboardVisible = prop.getIntValue("Keyboard.Visible", keyboardVisible);
	keyboardChannel = prop.getIntValue("Keyboard.Channel", keyboardChannel);
	keyboardHeight = prop.getIntValue("Keyboard.Height", keyboardHeight);
	scoreInstrumentNames = (ScoreInstrumentNames)prop.getIntValue("Score.InstrumentNames", scoreInstrumentNames);
	scoreShowMidiChannel = prop.getIntValue("Score.ShowMidiChannel", scoreShowMidiChannel);
	scorePart = (ScorePart)prop.getIntValue("Score.Part", scorePart);
	workingDirectory = prop.getValue("WorkingDirectory", workingDirectory);
	logging = prop.getIntValue("Logging", logging);
	rtpLogging = prop.getIntValue("RtpLogging", rtpLogging);
	language = prop.getValue("Language", language);
	lastSong = prop.getValue("LastSong", lastSong);
	midiIn2 = prop.getValue("MidiIn2", midiIn2);
	midiOut = prop.getValue("MidiOut", midiOut);
}

// Returns the UI language actually in use: "hu" or "en".
// If no language was chosen yet, the system language decides.
String Settings::GetEffectiveLanguage() const
{
	String lang = language.isNotEmpty() ? language : SystemStats::getUserLanguage();
	return lang.startsWithIgnoreCase("hu") ? "hu" : "en";
}

// Installs the translation for the effective language. Must be called before
// the UI components are created, because TRANS() is evaluated in their constructors.
// English is the original language of the source code, so it needs no translation file.
void Settings::ApplyLanguage() const
{
	if (GetEffectiveLanguage() == "hu")
	{
		LocalisedStrings::setCurrentMappings(new LocalisedStrings(
			String::fromUTF8(BinaryData::translation_hu_txt, BinaryData::translation_hu_txtSize), false));
	}
	else
	{
		LocalisedStrings::setCurrentMappings(nullptr);
	}
}

// For Android:
// Copies the resource files into the app data directory upon first launch.
// Subsequent launches will only copy over new files, if there are any.
void Settings::PrepareResources()
{
	const File dataDirectory = File::getSpecialLocation(File::userApplicationDataDirectory);
	// Directory to store resource files
	if (!dataDirectory.exists())
	{
		dataDirectory.createDirectory();
	}
	// See if there are any files installed from previous launches of the app
	Array<File> installedData = dataDirectory.getChildFile("assets").findChildFiles(File::findFiles, true, "*");
	StringArray installedDataFilenames;
	for (auto data : installedData)
	{
		installedDataFilenames.add(data.getFileName());
	}
	// Extract entries from the APK file under the assets folder (generated by Projucer)
	ZipFile zip(File::getSpecialLocation(File::currentApplicationFile));
	for (int i = 0; i < zip.getNumEntries(); i++)
	{
		if (auto* entry = zip.getEntry(i))
		{
			// Will only copy resource over once
			if (entry->filename.startsWith("assets/") && !installedDataFilenames.contains(entry->filename))
			{
				// File not installed yet, unpack it.
				zip.uncompressEntry (i, dataDirectory, true);
			}
		}
	}
}
