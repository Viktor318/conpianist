/*
  ==============================================================================

  This is an automatically generated GUI class created by the Projucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Projucer version: 5.4.7

  ------------------------------------------------------------------------------

  The Projucer is part of the JUCE library.
  Copyright (c) 2017 - ROLI Ltd.

  ==============================================================================
*/

//[Headers] You can add your own extra header files here...
#include "ConnectionComponent.h"
#include "BalanceComponent.h"
#include "RegistrationMemory.h"
#include "GuiHelper.h"
//[/Headers]

#include "SceneComponent.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
static File PianoSongMidi(const Settings& settings, const String& songName); // see below

// A thin bar on the upper edge of the virtual keyboard: dragging it with the mouse
// changes the height of the keyboard, a double click restores the default height.
class KeyboardResizer : public Component
{
public:
	std::function<int()> getHeightFunc;
	std::function<void(int height, bool save)> setHeightFunc;
	std::function<void()> resetFunc;

	KeyboardResizer()
	{
		setMouseCursor(MouseCursor::UpDownResizeCursor);
		setRepaintsOnMouseActivity(true);
	}

	void paint(Graphics& g) override
	{
		if (isMouseOverOrDragging())
		{
			g.setColour(Colour(0x80EE6C0A));
			g.fillRect(0, getHeight() / 2 - 1, getWidth(), 3);
		}
	}

	void mouseDown(const MouseEvent&) override
	{
		startHeight = getHeightFunc();
	}

	void mouseDrag(const MouseEvent& event) override
	{
		// dragging upwards makes the keyboard higher
		setHeightFunc(startHeight - event.getDistanceFromDragStartY(), false);
	}

	void mouseUp(const MouseEvent& event) override
	{
		if (event.mouseWasDraggedSinceMouseDown())
		{
			setHeightFunc(getHeightFunc(), true);
		}
	}

	void mouseDoubleClick(const MouseEvent&) override
	{
		resetFunc();
	}

private:
	int startHeight = 0;
};
//[/MiscUserDefs]

//==============================================================================
SceneComponent::SceneComponent (Settings& settings)
    : settings(settings)
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    topbarPanel.reset (new GroupComponent ("Top Bar",
                                           TRANS("Top Bar")));
    addAndMakeVisible (topbarPanel.get());
    topbarPanel->setTextLabelPosition (Justification::centred);

    playbackPanel.reset (new Component());
    addAndMakeVisible (playbackPanel.get());
    playbackPanel->setName ("Playback Panel");

    largeContentPanel.reset (new Component());
    addAndMakeVisible (largeContentPanel.get());
    largeContentPanel->setName ("Large Content");

    muteButton.reset (new ImageButton ("Mute Button"));
    addAndMakeVisible (muteButton.get());
    muteButton->setTooltip (TRANS("Local Control on/off"));
    muteButton->setButtonText (TRANS("Mute"));
    muteButton->addListener (this);

    muteButton->setImages (false, true, true,
                           ImageCache::getFromMemory (BinaryData::buttonvolume_png, BinaryData::buttonvolume_pngSize), 1.000f, Colour (0x00000000),
                           Image(), 0.750f, Colour (0x00000000),
                           Image(), 1.000f, Colour (0x00000000));
    zoomInButton.reset (new ImageButton ("Zoom In UI Button"));
    addAndMakeVisible (zoomInButton.get());
    zoomInButton->setTooltip (TRANS("Zoom In UI"));
    zoomInButton->setButtonText (TRANS("Mute"));
    zoomInButton->addListener (this);

    zoomInButton->setImages (false, true, true,
                             ImageCache::getFromMemory (BinaryData::buttonzoomin_png, BinaryData::buttonzoomin_pngSize), 1.000f, Colour (0x00000000),
                             Image(), 0.750f, Colour (0x00000000),
                             Image(), 1.000f, Colour (0x00000000));
    zoomOutButton.reset (new ImageButton ("Zoom Out UI Button"));
    addAndMakeVisible (zoomOutButton.get());
    zoomOutButton->setTooltip (TRANS("Zoom Out UI"));
    zoomOutButton->setButtonText (TRANS("Mute"));
    zoomOutButton->addListener (this);

    zoomOutButton->setImages (false, true, true,
                              ImageCache::getFromMemory (BinaryData::buttonzoomout_png, BinaryData::buttonzoomout_pngSize), 1.000f, Colour (0x00000000),
                              Image(), 0.750f, Colour (0x00000000),
                              Image(), 1.000f, Colour (0x00000000));
    keyboardPanel.reset (new Component());
    addAndMakeVisible (keyboardPanel.get());
    keyboardPanel->setName ("Keyboard Panel");

    keyboardButton.reset (new ImageButton ("Virtual Keyboard Button"));
    addAndMakeVisible (keyboardButton.get());
    keyboardButton->setTooltip (TRANS("Virtual Keyboard"));
    keyboardButton->setButtonText (TRANS("Virtual Keyboard"));
    keyboardButton->addListener (this);

    keyboardButton->setImages (false, true, true,
                               ImageCache::getFromMemory (BinaryData::buttonkeyboard_png, BinaryData::buttonkeyboard_pngSize), 1.000f, Colour (0x00000000),
                               Image(), 0.750f, Colour (0x00000000),
                               Image(), 1.000f, Colour (0x00000000));
    balanceButton.reset (new ImageButton ("Balance Button"));
    addAndMakeVisible (balanceButton.get());
    balanceButton->setTooltip (TRANS("Balance"));
    balanceButton->setButtonText (TRANS("Balance"));
    balanceButton->addListener (this);

    balanceButton->setImages (false, true, true,
                              ImageCache::getFromMemory (BinaryData::buttonbalance_png, BinaryData::buttonbalance_pngSize), 1.000f, Colour (0x00000000),
                              Image(), 0.750f, Colour (0x00000000),
                              Image(), 1.000f, Colour (0x00000000));
    voiceButton.reset (new TextButton ("Voice Button"));
    addAndMakeVisible (voiceButton.get());
    voiceButton->setTooltip (TRANS("Voice Selection"));
    voiceButton->setButtonText (TRANS("Voice"));
    voiceButton->addListener (this);

    scoreButton.reset (new TextButton ("Score Button"));
    addAndMakeVisible (scoreButton.get());
    scoreButton->setTooltip (TRANS("Score View"));
    scoreButton->setButtonText (TRANS("Score"));
    scoreButton->addListener (this);

    statusLabel.reset (new Label ("Status Label",
                                  TRANS("Looking for the instrument")));
    addAndMakeVisible (statusLabel.get());
    statusLabel->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    statusLabel->setJustificationType (Justification::centredLeft);
    statusLabel->setEditable (false, false, false);
    statusLabel->setColour (TextEditor::textColourId, Colours::black);
    statusLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    statusLabel->setBounds (48, 11, 240, 24);

    mixerButton.reset (new TextButton ("Mixer Button"));
    addAndMakeVisible (mixerButton.get());
    mixerButton->setTooltip (TRANS("Channel Mixer"));
    mixerButton->setButtonText (TRANS("Mixer"));
    mixerButton->addListener (this);

    menuButton.reset (new ImageButton ("Menu Button"));
    addAndMakeVisible (menuButton.get());
    menuButton->setTooltip (TRANS("Main Menu"));
    menuButton->setButtonText (TRANS("Menu"));
    menuButton->addListener (this);

    menuButton->setImages (false, true, true,
                           ImageCache::getFromMemory (BinaryData::buttonmenu_png, BinaryData::buttonmenu_pngSize), 1.000f, Colour (0x00000000),
                           Image(), 0.750f, Colour (0x00000000),
                           Image(), 1.000f, Colour (0x00000000));
    menuButton->setBounds (8, 8, 32, 28);


    //[UserPreSize]
    topbarPanel->setColour(GroupComponent::outlineColourId, Colours::transparentBlack);
    topbarPanel->setText("");

	playbackComponent.reset(new PlaybackComponent(settings, pianoController));
	playbackComponent->onChooseSong = [this]() { showSongSelector(); };
	// a song of the piano is played from its MIDI file when the player is switched
	pianoController.findSongFile = [this](const String& songName) { return PianoSongMidi(this->settings, songName); };
	pianoController.findPianoSong = [this](const File& file)
		{
			const File songs = this->settings.GetSongsDirectory();
			for (const Song& song : Presets::Songs())
			{
				// only the songs of the folder of the file are looked at
				if (file.getParentDirectory() == songs.getChildFile(song.folder) &&
					this->settings.GetSongMidi(song) == file)
				{
					return song.path;
				}
			}
			return String();
		};
	playbackViewport.reset(new Viewport());
	playbackViewport->setViewedComponent(playbackComponent.get(), false);
	playbackViewport->setScrollBarsShown(true, false);
	playbackViewport->setScrollBarThickness(8);
	playbackPanel->addAndMakeVisible(playbackViewport.get());

	keyboardComponent.reset(new KeyboardComponent(settings, pianoController));
	keyboardPanel->addAndMakeVisible(keyboardComponent.get());

	voiceComponent.reset(new VoiceComponent(settings, pianoController));
	scoreComponent.reset(ScoreComponent::Create(settings, pianoController));
	mixerComponent.reset(new MixerComponent(settings, pianoController));
	largeContentPanel->addChildComponent(voiceComponent.get());
    largeContentPanel->addChildComponent(scoreComponent.get());
	largeContentPanel->addChildComponent(mixerComponent.get());
	voiceButton->getProperties().set("tab", "yes");
	scoreButton->getProperties().set("tab", "yes");
	mixerButton->getProperties().set("tab", "yes");
    keyboardButton->getProperties().set("toggle", "yes");

    KeyboardResizer* resizer = new KeyboardResizer();
    resizer->getHeightFunc = [this]() { return getKeyboardHeight(); };
    resizer->setHeightFunc = [this](int height, bool save) { setKeyboardHeight(height, save); };
    resizer->resetFunc = [this]() { setKeyboardHeight(DefaultKeyboardHeight, true); };
    keyboardResizer.reset(resizer);
    addChildComponent(keyboardResizer.get());
    //[/UserPreSize]

    setSize (850, 550);


    //[Constructor] You can add your own custom stuff here..
	songSelectorButton.reset(new TopBarButton("Song Selector Button", TopBarButton::iconSongs));
	addAndMakeVisible(songSelectorButton.get());
	songSelectorButton->setTooltip(TRANS("Song Selector"));
	songSelectorButton->onClick = [this]() { showSongSelector(); };
	songSelectorButton->setBounds(getWidth() - 290 - 32, 8, 32, 28);
	recorderButton.reset(new TopBarButton("Recorder Button", TopBarButton::iconRecord));
	addAndMakeVisible(recorderButton.get());
	recorderButton->setTooltip(TRANS("Recording"));
	recorderButton->onClick = [this]() { showRecorder(); };
	recorderButton->setBounds(getWidth() - 255 - 32, 8, 32, 28);
	accompanimentButton.reset(new TopBarButton("Accompaniment Button", TopBarButton::iconAccompaniment));
	addAndMakeVisible(accompanimentButton.get());
	accompanimentButton->setTooltip(TRANS("Accompaniment"));
	accompanimentButton->onClick = [this]() { showAccompaniment(); };
	accompanimentButton->setBounds(getWidth() - 220 - 32, 8, 32, 28);
	styleMixerButton.reset(new TopBarButton("Style Mixer Button", TopBarButton::iconStyleMixer));
	addAndMakeVisible(styleMixerButton.get());
	styleMixerButton->setTooltip(TRANS("Accompaniment mixer"));
	styleMixerButton->onClick = [this]() { showStyleMixer(); };
	styleMixerButton->setBounds(getWidth() - 150 - 32, 8, 32, 28);
	pianoConnector.startThread();
    pianoController.SetPianoConnector(&pianoConnector);
    pianoController.AddListener(this);
    pianoController.sendToMidiDevice = [this](const MidiMessage& message) { midiDevice.Send(message); };
    pianoController.sendToPianoKeyboard = [this](const MidiMessage& message)
    	{
    		if (pianoKeyboardShared) midiDevice.Send(message); else pianoKeyboardPort.Send(message);
    	};
    midiDevice.onIncoming = [this](const MidiMessage& message) { pianoController.IncomingMidiDeviceMessage(message); };
    pianoController.onNetworkPlaybackFailed = [this]()
    	{
    		networkReachable = false;
    		AlertWindow::showMessageBoxAsync(MessageBoxIconType::WarningIcon, "ConPianist",
    			TRANS("The piano cannot be reached over the network. Playback continues via USB."));
    	};
    settings.addChangeListener(this);
	applySettings();
	updateSettingsState();

	switchLargePanel(scoreButton.get());

	startTimer(250);
    //[/Constructor]
}

SceneComponent::~SceneComponent()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    recorderWindow = nullptr;
    accompanimentWindow = nullptr;
    styleMixerWindow = nullptr;
    balanceWindow = nullptr;
    songSelectorWindow = nullptr;
    styleMixerButton = nullptr;
    accompanimentButton = nullptr;
    recorderButton = nullptr;
    songSelectorButton = nullptr;
    saveLastState(); // restored at the next start
    // MIDI In 2 is closed first (the output stays open), so no more notes arrive;
    // then the notes still held there or on the virtual keyboard are released
    midiDevice.SetPorts("", midiDevice.GetOutputName());
    midiDevice.onIncoming = nullptr;
    pianoController.ReleaseLive();
    pianoController.ShutdownLocalPlayer();
    pianoController.sendToPianoKeyboard = nullptr;
    pianoKeyboardPort.SetPorts("", "");
    midiDevice.SetPorts("", "");
    pianoConnector.SetMidiConnector(nullptr); // nothing is sent to the connectors any more
    if (rtpMidiConnector)
    {
		rtpMidiConnector->stopThread(3000);
	}
	pianoConnector.stopThread(1000);

    //[/Destructor_pre]

    topbarPanel = nullptr;
    playbackPanel = nullptr;
    largeContentPanel = nullptr;
    muteButton = nullptr;
    zoomInButton = nullptr;
    zoomOutButton = nullptr;
    keyboardPanel = nullptr;
    keyboardButton = nullptr;
    balanceButton = nullptr;
    voiceButton = nullptr;
    scoreButton = nullptr;
    statusLabel = nullptr;
    mixerButton = nullptr;
    menuButton = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void SceneComponent::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.fillAll (Colour (0xff323e44));

    {
        int x = 0, y = 43, width = getWidth() - 0, height = 1;
        Colour fillColour = Colour (0xff4e5b62);
        //[UserPaintCustomArguments] Customize the painting arguments here..
        //[/UserPaintCustomArguments]
        g.setColour (fillColour);
        g.fillRect (x, y, width, height);
    }

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void SceneComponent::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    topbarPanel->setBounds (0, -8, getWidth() - 0, 52);
    playbackPanel->setBounds (0, (-8) + 52, 290, getHeight() - 111);
    largeContentPanel->setBounds (0 + 290, (-8) + 52, getWidth() - 290, getHeight() - 111);
    muteButton->setBounds (getWidth() - 80 - 32, 8, 32, 28);
    zoomInButton->setBounds (getWidth() - 10 - 32, 8, 32, 28);
    zoomOutButton->setBounds (getWidth() - 45 - 32, 8, 32, 28);
    keyboardPanel->setBounds (0, getHeight() - 67, getWidth() - 0, 67);
    keyboardButton->setBounds (getWidth() - 115 - 32, 8, 32, 28);
    balanceButton->setBounds (getWidth() - 185 - 32, 8, 32, 28);
    voiceButton->setBounds (0 + 378, (-8) + 18, 80, 34);
    scoreButton->setBounds (0 + 298, (-8) + 18, 80, 34);
    mixerButton->setBounds (0 + 458, (-8) + 18, 80, 34);
    //[UserResized] Add your own custom resize handling here..
    if (styleMixerButton) // created after the first layout
    {
        // Song Selector, Recording, Accompaniment, (Balance), Accompaniment mixer, (keyboard, ...)
        songSelectorButton->setBounds(getWidth() - 290 - 32, 8, 32, 28);
        recorderButton->setBounds(getWidth() - 255 - 32, 8, 32, 28);
        accompanimentButton->setBounds(getWidth() - 220 - 32, 8, 32, 28);
        styleMixerButton->setBounds(getWidth() - 150 - 32, 8, 32, 28);
    }
    // the height of the virtual keyboard can be changed by the user
    const int keyboardHeight = getKeyboardHeight();
    keyboardPanel->setBounds (0, getHeight() - keyboardHeight, getWidth(), keyboardHeight);
    const int contentHeight = getHeight() - 44 - (keyboardPanel->isVisible() ? keyboardHeight : 0);
    playbackPanel->setBounds(playbackPanel->getX(), playbackPanel->getY(), playbackPanel->getWidth(), contentHeight);
    largeContentPanel->setBounds(largeContentPanel->getX(), largeContentPanel->getY(), largeContentPanel->getWidth(), contentHeight);
    keyboardResizer->setBounds(0, keyboardPanel->getY() - KeyboardResizerHeight / 2, getWidth(), KeyboardResizerHeight);
    keyboardResizer->setVisible(keyboardPanel->isVisible());
    keyboardResizer->toFront(false);
	// the left panel scrolls if it is lower than its controls need
	playbackViewport->setBounds(0, 0, playbackPanel->getWidth(), playbackPanel->getHeight());
	const bool scroll = playbackPanel->getHeight() < PlaybackComponent::MinimumHeight;
	playbackComponent->setSize(playbackPanel->getWidth() - (scroll ? playbackViewport->getScrollBarThickness() : 0),
		jmax(playbackPanel->getHeight(), (int)PlaybackComponent::MinimumHeight));
    keyboardComponent->setBounds(0, 0, keyboardPanel->getWidth(), keyboardPanel->getHeight());

	if (voiceComponent->isVisible())
	{
		voiceComponent->setBounds(0, 0, largeContentPanel->getWidth(), largeContentPanel->getHeight());
	}
	if (scoreComponent->isVisible())
	{
    	scoreComponent->setBounds(0, 0, largeContentPanel->getWidth(), largeContentPanel->getHeight());
	}
	if (mixerComponent->isVisible())
	{
		mixerComponent->setBounds(0, 0, largeContentPanel->getWidth(), largeContentPanel->getHeight());
	}
    //[/UserResized]
}

void SceneComponent::buttonClicked (Button* buttonThatWasClicked)
{
    //[UserbuttonClicked_Pre]
    //[/UserbuttonClicked_Pre]

    if (buttonThatWasClicked == muteButton.get())
    {
        //[UserButtonCode_muteButton] -- add your button handler code here..
        pianoController.SetLocalControl(!pianoController.GetLocalControl());
        //[/UserButtonCode_muteButton]
    }
    else if (buttonThatWasClicked == zoomInButton.get())
    {
        //[UserButtonCode_zoomInButton] -- add your button handler code here..
        zoomUi(true);
        //[/UserButtonCode_zoomInButton]
    }
    else if (buttonThatWasClicked == zoomOutButton.get())
    {
        //[UserButtonCode_zoomOutButton] -- add your button handler code here..
        zoomUi(false);
        //[/UserButtonCode_zoomOutButton]
    }
    else if (buttonThatWasClicked == keyboardButton.get())
    {
        //[UserButtonCode_keyboardButton] -- add your button handler code here..
        toggleKeyboard();
        //[/UserButtonCode_keyboardButton]
    }
    else if (buttonThatWasClicked == balanceButton.get())
    {
        //[UserButtonCode_balanceButton] -- add your button handler code here..
		showBalance();
        //[/UserButtonCode_balanceButton]
    }
    else if (buttonThatWasClicked == voiceButton.get())
    {
        //[UserButtonCode_voiceButton] -- add your button handler code here..
		switchLargePanel(buttonThatWasClicked);
        //[/UserButtonCode_voiceButton]
    }
    else if (buttonThatWasClicked == scoreButton.get())
    {
        //[UserButtonCode_scoreButton] -- add your button handler code here..
		switchLargePanel(buttonThatWasClicked);
        //[/UserButtonCode_scoreButton]
    }
    else if (buttonThatWasClicked == mixerButton.get())
    {
        //[UserButtonCode_mixerButton] -- add your button handler code here..
		switchLargePanel(buttonThatWasClicked);
        //[/UserButtonCode_mixerButton]
    }
    else if (buttonThatWasClicked == menuButton.get())
    {
        //[UserButtonCode_menuButton] -- add your button handler code here..
        showMenu();
        //[/UserButtonCode_menuButton]
    }

    //[UserbuttonClicked_Post]
    //[/UserbuttonClicked_Post]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...
void SceneComponent::PianoStateChanged(PianoController::Aspect aspect, PianoController::Channel channel)
{
	if (aspect == PianoController::apConnection || aspect == PianoController::apLocalControl)
	{
		GuiHelper::CallAsync(this, [=](){updateSettingsState();});
	}
	if (aspect == PianoController::apConnection && pianoController.IsConnected())
	{
		GuiHelper::CallAsync(this, [=]()
			{
				pianoController.Sync();
				connectedSince = Time::getCurrentTime();
				restorePianoState();
				pianoController.RestoreLiveChannels(); // Live Play channels not used in the song
				updatePlaybackAvailability();
				chooseDefaultPlaybackSource();
			});
	}
	else if (aspect == PianoController::apActive && channel == PianoController::chLeft)
	{
		GuiHelper::CallAsync(this, [=](){updateKeyboard();});
	}
	else if (aspect == PianoController::apSongLoaded)
	{
		GuiHelper::CallAsync(this, [=](){loadSongState();});
	}
	else if (aspect == PianoController::apPlaybackSource)
	{
		GuiHelper::CallAsync(this, [=](){updateSettingsState();});
	}
	else if (aspect == PianoController::apPlayback)
	{
		// a switch back to the chosen player that waited for the end of playback; the state
		// is asked later, not here (see PianoController::NotifyChangedLater)
		GuiHelper::CallAsync(this, [=]()
			{
				if (!pianoController.GetPlaying())
				{
					chooseDefaultPlaybackSource();
				}
			});
	}
}

void SceneComponent::updateSettingsState()
{
	bool mute = !pianoController.GetLocalControl() && pianoController.IsConnected();
	muteButton->setImages(false, true, true, ImageCache::getFromMemory(
			mute ? BinaryData::buttonmute_png : BinaryData::buttonvolume_png,
			mute ? BinaryData::buttonmute_pngSize : BinaryData::buttonvolume_pngSize),
			1.000f, Colour (0x00000000), Image(), 0.750f, Colour (0x00000000), Image(), 1.000f, Colour (0x00000000));

	balanceButton->setEnabled(pianoController.IsConnected());
	if (styleMixerButton)
	{
		styleMixerButton->setEnabled(pianoController.IsConnected());
	}
	keyboardButton->setEnabled(pianoController.IsReady());
	muteButton->setEnabled(pianoController.IsConnected());
}

void SceneComponent::showMenu()
{
	PopupMenu menu;
	menu.addSectionHeader(TRANS("INSTRUMENT"));
	if (pianoController.IsConnected())
	{
		menu.addItem(10, pianoController.GetModel() + " " + String(CharPointer_UTF8("\xe2\x80\xa2")) + " " + TRANS("Firmware") + " " + pianoController.GetVersion(), false, false);
	}
	menu.addItem(1, TRANS("Connection Settings"));
	menu.addItem(4, TRANS("Reset Connection"));
	menu.addItem(2, TRANS("Resync State from Piano"));
	menu.addItem(3, TRANS("Reset Piano to Default State"));
	menu.addSectionHeader(TRANS("REGISTRATION MEMORY"));
	menu.addItem(101, TRANS("Load Piano State"));
	menu.addItem(102, TRANS("Save Piano State"));
	menu.addSectionHeader(TRANS("LIVE PLAY"));
	menu.addItem(301, TRANS("Recording..."));
	menu.addItem(302, TRANS("Accompaniment..."));
	menu.addSectionHeader(TRANS("LANGUAGE"));
	// language names are intentionally not translated: each is shown in its own language
	menu.addItem(201, "English", true, settings.GetEffectiveLanguage() == "en");
	menu.addItem(202, "Magyar", true, settings.GetEffectiveLanguage() == "hu");
	menu.addSectionHeader(TRANS("ABOUT"));
	menu.addItem(998, TRANS("Version:") + " \t" + JUCEApplication::getInstance()->getApplicationVersion(), false, false);
	menu.addItem(999, TRANS("Homepage"));
	menu.addItem(997, TRANS("About ConPianist..."));

	GuiHelper::ShowMenuAsync(menu, menuButton.get(),
		[this, self = Component::SafePointer<Component>(this)](int result)
		{
			if (self == nullptr) return; // deleted meanwhile
			switch (result)
			{
				case 1:
					ConnectionComponent::showDialog(settings);
					break;
				case 2:
					GuiHelper::CallAsync(this, [=](){pianoController.Sync();});
					break;
				case 3:
					GuiHelper::CallAsync(this, [=](){pianoController.Reset();});
					break;
				case 4:
					resetConnection();
					break;
				case 101:
					loadState();
					break;
				case 102:
					saveState();
					break;
				case 201:
				case 202:
					changeLanguage(result == 202 ? "hu" : "en");
					break;
				case 301:
					showRecorder();
					break;
				case 302:
					showAccompaniment();
					break;
				case 997:
					showAbout();
					break;
				case 999:
					URL("https://github.com/Viktor318/conpianist").launchInDefaultBrowser();
					break;
			}
		});
}

void SceneComponent::timerCallback()
{
	checkConnection();
}

void SceneComponent::checkConnection()
{
	if (!midiConnector->IsConnected())
	{
		pianoController.Disconnect();
	}

	Time curTime = Time::getCurrentTime();

	// the MIDI device may be started later (e.g. loopMIDI): try to open it every 2 s
	if (++midiDeviceRefreshCounter % 8 == 0)
	{
		midiDevice.Refresh();
		updatePianoKeyboardPort(false); // the piano may be switched on or plugged in later
	}
	updatePlaybackAvailability();
	checkPianoAvailability(curTime);
	recheckNetwork(curTime);
	restoreSongInPiano(curTime);

	if (!pianoController.IsConnected())
	{
		String prefix = midiConnector->IsConnected() ? TRANS("Connecting to the instrument") : TRANS("Looking for the instrument");
		String status = statusLabel->getText();
		if (pianoController.IsMidiDevicePlayback())
		{
			// playing on the MIDI device; the piano is still looked for in the background
			status = TRANS("MIDI device: NAME").replace("NAME", midiDevice.GetOutputName());
		}
		else if (status.startsWith(prefix) && status.length() < prefix.length() + 10)
		{
			status += ".";
		}
		else
		{
			status = prefix;
		}
		statusLabel->setText(status, NotificationType::dontSendNotification);
		statusLabel->setColour(Label::textColourId, Colours::white);

		if (midiConnector->IsConnected() && pianoConnector.QueueSize() == 0)
		{
			pianoController.Connect();
		}

		if ((pianoConnector.GetAttempt() > 2 && (curTime - pianoConnector.GetStallTime()).inSeconds() > ResetStalledInterval) ||
			(curTime - lastResetTime).inSeconds() > ResetConnectingInterval)
		{
			resetMidiConnector();
		}
	}
	else if (pianoConnector.GetAttempt() > 2 && (curTime - pianoConnector.GetStallTime()).inSeconds() > ResetStalledInterval)
	{
		resetMidiConnector();
	}
	else
	{
		int queueSize = pianoConnector.QueueSize();
		if (queueSize > 0)
		{
			String status = statusLabel->getText();
			int pos = status.indexOf(".");
			int numDots = pos == -1 ? 0 : status.length() - pos;
			numDots = numDots < 10 ? numDots + 1 : 0;
			status = TRANS("Exchanging NUMBER messages").replace("NUMBER", String(queueSize)) + String::repeatedString(".", numDots);
			statusLabel->setText(status, NotificationType::dontSendNotification);
		}
		else
		{
			// with MIDI device playback the device is shown (the piano is connected too)
			statusLabel->setText(pianoController.IsMidiDevicePlayback() ?
				TRANS("MIDI device: NAME").replace("NAME", midiDevice.GetOutputName()) :
				TRANS("Connected and ready"), NotificationType::dontSendNotification);
		}
		bool stalled = queueSize > 0 && pianoConnector.GetAttempt() > 1 &&
			(curTime - pianoConnector.GetStallTime()).inSeconds() > IndicateStalledInterval;
		statusLabel->setColour(Label::textColourId, stalled ? Colours::red : Colours::white);
	}
}

void SceneComponent::applySettings()
{
	if (currentPianoIp != settings.pianoIp ||
		currentMidiPort != settings.midiPort ||
		!midiConnector)
	{
		resetMidiConnector();
		currentPianoIp = settings.pianoIp;
		currentMidiPort = settings.midiPort;
		updatePlaybackSource();
	}

	if (currentMidiIn2 != settings.midiIn2 || currentMidiOut != settings.midiOut)
	{
		currentMidiIn2 = settings.midiIn2;
		currentMidiOut = settings.midiOut;
		midiDevice.SetPorts(settings.midiIn2, settings.midiOut);
		updatePlaybackAvailability();
	}

	// the port may change with the piano's port or the MIDI Out; Live Play setting
	updatePianoKeyboardPort(false);

	float scale = settings.zoomUi;
	scale = std::min(std::max(scale, 0.25f), 4.0f);
	scale = round(scale * 20) / 20;
	Desktop::getInstance().setGlobalScaleFactor(scale);

#if JUCE_ANDROID || JUCE_IOS
	if (getParentComponent())
	{
		Rectangle<int> r = Desktop::getInstance().getDisplays().getMainDisplay().userArea;
		Component* win = (DocumentWindow*)getParentComponent();
		win->setBounds(r.getX(), r.getY(), r.getWidth(), r.getHeight());
	}
#endif

	updateKeyboard();
}

void SceneComponent::resetMidiConnector()
{
	Logger::writeToLog(midiConnector ? "Reset Midi-Connector" : "Init Midi-Connector");

	pianoController.Disconnect();
	pianoConnector.ClearQueue();

	// Nothing is sent to the old connector any more (a message being sent is waited
	// for), then the old connector is stopped and deleted, and only then is the new
	// one created: the two never run at the same time.
	pianoConnector.SetMidiConnector(nullptr);
	if (midiConnector)
	{
		midiConnector->SetListener(nullptr);
	}
	midiConnector = nullptr;
	if (rtpMidiConnector)
	{
		rtpMidiConnector->stopThread(3000);
		rtpMidiConnector.reset();
	}
	localMidiConnector.reset();

	pianoController.SetRemoteIp(settings.pianoIp);

	if (settings.midiPort == "")
	{
		rtpMidiConnector = std::make_unique<RtpMidiConnector>(settings.pianoIp, settings.rtpLogging);
		midiConnector = rtpMidiConnector.get();
		pianoConnector.SetMidiConnector(midiConnector);
		rtpMidiConnector->startThread();
	}
	else
	{
		// The ports are closed first and opened again: after the USB cable was unplugged
		// and plugged in again, the device usually has the same identifier, and JUCE would
		// keep the old (no longer working) port open instead of opening it again.
		for (auto& device : MidiInput::getAvailableDevices())
		{
			audioDeviceManager.setMidiInputDeviceEnabled(device.identifier, false);
		}
		audioDeviceManager.setDefaultMidiOutputDevice("");

		// only the input with the same name as the output is used (the piano has one of
		// each; a general MIDI device may have no input)
		for (auto& device : MidiInput::getAvailableDevices())
		{
			audioDeviceManager.setMidiInputDeviceEnabled(device.identifier, device.name == settings.midiPort);
		}
		String outputId;
		for (auto& device : MidiOutput::getAvailableDevices())
		{
			if (device.name == settings.midiPort)
			{
				outputId = device.identifier;
			}
		}
		audioDeviceManager.setDefaultMidiOutputDevice(outputId);
		localMidiConnector = std::make_unique<LocalMidiConnector>(&audioDeviceManager);
		midiConnector = localMidiConnector.get();
		pianoConnector.SetMidiConnector(midiConnector);
	}

	// opened again too (after unplugging the USB cable the old port does not work)
	updatePianoKeyboardPort(true);

	lastResetTime = Time::getCurrentTime();
}

// The piano's second MIDI port, next to its first one (settings.midiPort). Its name
// depends on the system and the driver, e.g. "CSP-170" and "CSP-170-2", "CSP-170-1" and
// "CSP-170-2", "CSP-170" and "MIDIOUT2 (CSP-170)": the name of the first port without
// its number ("CSP-170"), followed or enclosed by the number 2.
String SceneComponent::findPianoKeyboardPort() const
{
	const String port = settings.midiPort;
	if (port == "")
	{
		return String(); // network connection: no second port
	}

	String base = port;
	if (base.startsWith("MIDIOUT1 (") || base.startsWith("MIDIIN1 ("))
	{
		base = base.fromFirstOccurrenceOf("(", false, false).upToLastOccurrenceOf(")", false, false);
	}
	else if (base.endsWith("-1") || base.endsWith(" 1"))
	{
		base = base.dropLastCharacters(2);
	}
	base = base.trim();

	for (auto& device : MidiOutput::getAvailableDevices())
	{
		const String name = device.name;
		if (name == port || !name.contains(base))
		{
			continue;
		}
		// what remains without the base name must be the number 2 (with separators)
		const String rest = name.replace(base, "").removeCharacters(" -_()[]:#");
		if (rest == "2" || rest.equalsIgnoreCase("MIDIOUT2") || rest.equalsIgnoreCase("Port2"))
		{
			return name;
		}
	}
	return String();
}

// Opens the piano's second port (again, if "reopen") and tells the piano controller
// whether Live Play can sound on the piano's own keyboard parts.
void SceneComponent::updatePianoKeyboardPort(bool reopen)
{
	const String name = findPianoKeyboardPort();
	const bool shared = name.isNotEmpty() && name == settings.midiOut;
	pianoKeyboardShared = shared;

	const String portName = shared ? String() : name;
	if (portName != pianoKeyboardPortName || reopen)
	{
		pianoKeyboardPortName = portName;
		pianoKeyboardPort.SetPorts("", "");
		pianoKeyboardPort.SetPorts("", portName);
	}
	else if (portName.isNotEmpty())
	{
		pianoKeyboardPort.Refresh();
	}

	const bool available = name.isNotEmpty() &&
		(shared ? midiDevice.IsOutputOpen() : pianoKeyboardPort.IsOutputOpen());
	if (available != pianoController.IsLivePianoKeyboardAvailable() || reopen)
	{
		Logger::writeToLog("Piano keyboard port: " + (name.isEmpty() ? String("not found (piano port: ") +
			settings.midiPort + ")" : name + (available ? " available" : " cannot be opened")));
	}
	pianoController.SetLivePianoKeyboard(settings.livePlayOnPiano, available);
}

// Decides which player plays the songs, after the start and after the connection
// settings were changed. The piano's own player (song uploaded over the network) is
// preferred, because only it supports Stream Lights and Guide. With a network
// connection it is the only choice; with a MIDI port (USB) it is used if the piano
// can be reached over the network, otherwise ConPianist plays the songs itself.
void SceneComponent::updatePlaybackSource()
{
	networkCheckId++; // results of earlier checks are no longer relevant
	networkReachable = true; // with a USB connection: assumed until the check is finished
	pianoMissing = false;

	if (settings.midiPort != "")
	{
		checkNetworkPlayback();
	}

	updatePlaybackAvailability();
	chooseDefaultPlaybackSource();
}

// Menu "Reset Connection": opens the piano's connection and the MIDI device's ports
// (MIDI Out, MIDI In 2) again and checks again which outputs are available (the network
// is checked on the piano's upload port).
void SceneComponent::resetConnection()
{
	Logger::writeToLog("Reset connection");

	resetMidiConnector();

	// a port of a device that was unplugged and plugged in again does not work any more
	midiDevice.SetPorts("", "");
	midiDevice.SetPorts(settings.midiIn2, settings.midiOut);
	midiDevice.Refresh();

	networkCheckId++;
	networkReachable = true; // until the check is finished
	pianoMissing = false;
	if (settings.midiPort != "")
	{
		checkNetworkPlayback();
	}

	lastAvailability = -1;
	updatePlaybackAvailability();
}

// With a USB connection: if the piano cannot be reached over the network, it is checked
// again every few seconds, so network playback becomes available (and is used again, if
// it was chosen) as soon as the piano's Wi-Fi is up, e.g. after switching the piano on.
void SceneComponent::recheckNetwork(Time curTime)
{
	if (settings.midiPort == "" || networkReachable || !pianoController.IsConnected() ||
		isNetworkCheckRunning() ||
		(curTime - lastNetworkCheck).inMilliseconds() < NetworkRecheckIntervalMs)
	{
		return;
	}
	networkCheckId++;
	checkNetworkPlayback();
}

// Which players can be chosen now.
void SceneComponent::updatePlaybackAvailability()
{
	const bool connected = pianoController.IsConnected();
	const bool network = connected && networkReachable;
	const bool usb = connected && settings.midiPort != "";
	const bool device = midiDevice.IsOutputOpen();
	const int availability = (network ? 1 : 0) + (usb ? 2 : 0) + (device ? 4 : 0);
	if (availability != lastAvailability)
	{
		lastAvailability = availability;
		pianoController.SetPlaybackAvailability(network, usb, device);
		chooseDefaultPlaybackSource();
	}
}

// The player chosen by the user (saved in the settings, also used at the next start).
PianoController::PlaybackSource SceneComponent::getPreferredPlaybackSource() const
{
	return settings.playbackSource == "usb" ? PianoController::psLocal :
		settings.playbackSource == "device" ? PianoController::psMidiDevice :
		PianoController::psPiano;
}

// Chooses the player, whenever the available players change: the one chosen by the
// user, if it is available; otherwise another one, until the chosen one is available
// again. With the piano: its own player (network) or ConPianist's own player (USB);
// without the piano the MIDI device is chosen a few seconds later (see
// checkPianoAvailability).
void SceneComponent::chooseDefaultPlaybackSource()
{
	const PianoController::PlaybackSource preferred = getPreferredPlaybackSource();
	const bool network = pianoController.IsNetworkPlaybackAvailable();
	const bool usb = pianoController.IsLocalPlaybackAvailable();

	PianoController::PlaybackSource source;
	if (preferred == PianoController::psMidiDevice && pianoController.IsMidiDevicePlaybackAvailable())
	{
		source = PianoController::psMidiDevice;
	}
	else if (!pianoController.IsConnected())
	{
		return;
	}
	else if (preferred == PianoController::psLocal && usb)
	{
		source = PianoController::psLocal;
	}
	else if (network)
	{
		source = PianoController::psPiano;
	}
	else if (usb)
	{
		source = PianoController::psLocal;
	}
	else
	{
		return;
	}

	const PianoController::PlaybackSource current = pianoController.GetPlaybackSource();
	const bool currentAvailable = current == PianoController::psPiano ? network :
		current == PianoController::psLocal ? usb : pianoController.IsMidiDevicePlaybackAvailable();
	if (source != current && currentAvailable && pianoController.GetPlaying())
	{
		// the player works: it is not switched during playback, only when it stops
		return;
	}

	if (source != current)
	{
		pianoController.SetPlaybackSource(source, source != preferred);
	}

	if (source != PianoController::psPiano)
	{
		loadLastSong();
	}
}

// If the piano is not available (switched off, not connected) for a few seconds, the
// songs are played on the MIDI device (MIDI Out). When the piano is available again,
// its player is used again, unless the user has chosen the MIDI device.
void SceneComponent::checkPianoAvailability(Time curTime)
{
	if (pianoController.IsConnected())
	{
		pianoMissing = false;
		return;
	}

	if (!pianoMissing)
	{
		pianoMissing = true;
		pianoMissingSince = curTime;
	}

	const int delay = settings.midiPort == "" ? PianoMissingDelayNetworkMs :
		midiConnector != nullptr && midiConnector->IsConnected() ? PianoMissingDelayUsbMs : PianoMissingDelayUsbNoPortMs;
	if ((curTime - pianoMissingSince).inMilliseconds() < delay || pianoController.IsMidiDevicePlayback())
	{
		return;
	}

	if (midiDevice.IsOutputOpen())
	{
		Logger::writeToLog("Piano not available, playing on MIDI device " + midiDevice.GetOutputName());
		networkCheckId++; // a running network check is no longer relevant
		pianoController.SetPlaybackSource(PianoController::psMidiDevice, true);
		loadLastSong();
		updateSettingsState();
	}
	else if (!noMidiOutMessageShown)
	{
		noMidiOutMessageShown = true;
		AlertWindow::showMessageBoxAsync(MessageBoxIconType::InfoIcon, "ConPianist",
			TRANS("The piano is not available. To play songs without the piano, choose a MIDI Out port in the Connection Settings."));
	}
}

// Checks in the background, if the piano accepts connections on its upload port.
void SceneComponent::checkNetworkPlayback()
{
	lastNetworkCheck = Time::getCurrentTime();
	const int checkId = networkCheckId;
	networkCheckPendingId = checkId;
	const String pianoIp = settings.pianoIp;
	Component::SafePointer<SceneComponent> self(this);

	Thread::launch([self, pianoIp, checkId]()
		{
			StreamingSocket socket;
			const bool reachable = pianoIp.isNotEmpty() &&
				socket.connect(pianoIp, PianoController::UploadPort, NetworkCheckTimeoutMs);
			socket.close();

			MessageManager::callAsync([self, reachable, checkId]()
				{
					if (self != nullptr)
					{
						self->networkCheckFinished(reachable, checkId);
					}
				});
		});
}

void SceneComponent::networkCheckFinished(bool reachable, int checkId)
{
	if (checkId == networkCheckPendingId)
	{
		networkCheckPendingId = -1;
	}
	if (checkId != networkCheckId || settings.midiPort == "")
	{
		return;
	}

	if (reachable != networkReachable || reachable)
	{
		// the regular checks while it cannot be reached are not logged every time
		Logger::writeToLog(String("Piano ") + (reachable ? "can" : "cannot") + " be reached over the network");
	}

	networkReachable = reachable;
	updatePlaybackAvailability();
	chooseDefaultPlaybackSource();
}

// The path of one of the piano's own songs ("/SONG/Popular/Pop/Pop01.S000.mid"; the piano
// reports it with the separator of the system), or empty for a file.
static String PianoSongPath(const String& songName)
{
	const String path = songName.replaceCharacter('\\', '/');
	return path.startsWith("/SONG/") ? path : String();
}

// Two song names are the same song: the same song of the piano or the same file.
// (On Windows "\SONG\..." would count as an absolute path, so the piano's songs come first.)
static bool IsSameSong(const String& a, const String& b)
{
	if (PianoSongPath(a).isNotEmpty() || PianoSongPath(b).isNotEmpty())
	{
		return PianoSongPath(a) == PianoSongPath(b);
	}
	return File::isAbsolutePath(a) && File::isAbsolutePath(b) && File(a) == File(b);
}

// The MIDI file of one of the piano's own songs in its folder (for playback via USB or
// MIDI device, see Settings::GetSongMidi), or a non-existing file.
static File PianoSongMidi(const Settings& settings, const String& songName)
{
	const String path = PianoSongPath(songName);
	const Song* song = path.isNotEmpty() ? Presets::FindSong("PRESET:" + path) : nullptr;
	return song != nullptr ? settings.GetSongMidi(*song) : File();
}

// With ConPianist's own player the song is not kept by the piano, so the last song
// is loaded again: at the start the song of the last state (restored with its
// settings, see loadSongState), later the last loaded song (with its registration memory).
void SceneComponent::loadLastSong()
{
	if (!pianoController.IsLocalPlayback() || pianoController.IsSongLoaded())
	{
		return;
	}

	String song = songStateRestored ? String() : getLastStateSong();
	if (song.isEmpty())
	{
		song = settings.lastSong;
	}
	if (PianoSongPath(song).isNotEmpty())
	{
		// one of the piano's own songs: its MIDI file in its folder, as the Load button of
		// the Song Selector loads it; nothing if it has none
		const File midi = PianoSongMidi(settings, song);
		if (!midi.existsAsFile())
		{
			songStateRestored = true;
			return;
		}
		song = midi.getFullPathName();
	}

	if (File::isAbsolutePath(song) && File(song).existsAsFile())
	{
		pianoController.LoadSong(File(song));
	}
	else
	{
		songStateRestored = true; // nothing to restore
	}
}

// The song saved in the last state (full path), or empty.
String SceneComponent::getLastStateSong() const
{
	const File file = settings.GetLastStateFile();
	return file.existsAsFile() ? RegistrationMemory::GetSongName(file) : String();
}

// Restores the piano's own settings of the last state (voices, balance, Piano Room,
// accompaniment), once, when the piano is connected for the first time.
void SceneComponent::restorePianoState()
{
	if (pianoStateRestored)
	{
		return;
	}
	pianoStateRestored = true;

	const File file = settings.GetLastStateFile();
	if (file.existsAsFile())
	{
		Logger::writeToLog("Restoring the piano settings of the last state");
		RegistrationMemory::Options opts;
		opts.songname = false;
		opts.mixer = false;
		opts.playback = false;
		opts.settings = false;
		opts.key = false; // at the start the key of the settings file is used
		RegistrationMemory regmem(pianoController, settings, opts, file);
		regmem.Load();
	}
}

// Restores the settings of the song of the last state (Mixer, Playback panel, position).
void SceneComponent::restoreSongState()
{
	const File file = settings.GetLastStateFile();
	if (file.existsAsFile())
	{
		Logger::writeToLog("Restoring the song settings of the last state");
		RegistrationMemory::Options opts;
		opts.songname = false;
		opts.voices = false;
		opts.balance = false;
		opts.pianoroom = false;
		opts.settings = false;
		opts.style = false;
		RegistrationMemory regmem(pianoController, settings, opts, file);
		regmem.Load();
	}
}

// The settings of the loaded song from the beginning: its own tempo and volume, no
// transposition, no A-B loop, all parts on. (The Mixer channels are set by the song.)
void SceneComponent::resetSongSettings()
{
	Logger::writeToLog("The song of the piano starts with its own settings");
	pianoController.ResetTempo();
	pianoController.ResetVolume(PianoController::chMidiMaster);
	pianoController.SetTranspose(0);
	pianoController.ResetLoop();
	for (PianoController::Part part : {PianoController::paRight, PianoController::paLeft, PianoController::paBacking})
	{
		pianoController.SetPart(part, true);
	}
}

// With the piano's own player (network), the song of the last state is loaded into the
// piano at the start, unless the piano still has it. This is done after the piano's
// state was read and the network was checked.
void SceneComponent::restoreSongInPiano(Time curTime)
{
	if (songStateRestored || songRestoreRequested || !pianoController.IsConnected() ||
		pianoController.IsLocalPlayback() || isNetworkCheckRunning() ||
		pianoConnector.QueueSize() > 0 || (curTime - connectedSince).inMilliseconds() < 1500)
	{
		return;
	}
	songRestoreRequested = true;

	// a file, or one of the piano's own songs (loaded by its path, see the Song Selector)
	const String song = getLastStateSong();
	const String pianoSong = PianoSongPath(song);
	if (pianoSong.isEmpty() && (!File::isAbsolutePath(song) || !File(song).existsAsFile()))
	{
		songStateRestored = true; // nothing to restore
		return;
	}

	// one of the piano's own songs is always loaded again, as with the Load button of the
	// Song Selector, whatever the piano has now; its settings: see loadSongState
	if (pianoSong.isNotEmpty())
	{
		Logger::writeToLog("Loading the piano's song of the last state");
		if (!pianoController.LoadPresetSong("PRESET:" + pianoSong))
		{
			songStateRestored = true;
		}
		return;
	}

	const String loaded = pianoController.IsSongLoaded() ? pianoController.GetSongName() : String();
	if (IsSameSong(loaded, song))
	{
		// the piano still has the song
		songStateRestored = true;
		restoreSongState();
		return;
	}

	Logger::writeToLog("Loading the song of the last state");
	pianoController.LoadSong(File(song)); // its settings: see loadSongState
}

// Saves the current state, restored at the next start. Parts that were not known in
// this session (the piano was not connected, the song was not loaded yet) are kept
// from the previously saved state.
void SceneComponent::saveLastState()
{
	const File file = settings.GetLastStateFile();
	RegistrationMemory regmem(pianoController, settings, {}, file);
	std::unique_ptr<XmlElement> state = regmem.CreateXml();

	std::unique_ptr<XmlElement> previous = file.existsAsFile() ? XmlDocument::parse(file) : nullptr;
	if (previous)
	{
		auto keep = [](XmlElement& parent, const XmlElement* previousParent, const String& name)
			{
				if (XmlElement* el = parent.getChildByName(name))
				{
					parent.removeChildElement(el, true);
				}
				if (const XmlElement* old = previousParent ? previousParent->getChildByName(name) : nullptr)
				{
					parent.addChildElement(new XmlElement(*old));
				}
			};
		XmlElement* channels = state->getChildByName("Channels");
		if (!channels)
		{
			channels = state->createNewChildElement("Channels");
		}
		const XmlElement* previousChannels = previous->getChildByName("Channels");

		if (!pianoStateRestored)
		{
			keep(*state, previous.get(), "Voices");
			keep(*state, previous.get(), "PianoRoom");
			keep(*state, previous.get(), "Style");
			for (const char* name : {"Main", "Left", "Layer", "Mic", "AuxIn"})
			{
				keep(*channels, previousChannels, name);
			}
		}
		if (!songStateRestored)
		{
			keep(*state, previous.get(), "Song");
			keep(*state, previous.get(), "Playback");
			keep(*channels, previousChannels, "MidiMaster");
			for (int i = 1; i <= 16; i++)
			{
				keep(*channels, previousChannels, "Midi" + String(i));
			}
		}
		if (!pianoStateRestored && !songStateRestored)
		{
			// the Live Play channels were not restored in this session either
			keep(*state, previous.get(), "LiveOctaves");
			keep(*state, previous.get(), "LiveChannels");
		}
	}

	file.getParentDirectory().createDirectory();
	if (!state->writeTo(file))
	{
		Logger::writeToLog("Cannot save the last state: " + file.getFullPathName());
	}
}

// Saves the state a little later (after a song was loaded and its settings applied),
// so that it is not lost if the program does not end normally.
void SceneComponent::scheduleLastStateSave()
{
	Component::SafePointer<SceneComponent> self(this);
	Timer::callAfterDelay(3000, [self]()
		{
			if (self != nullptr)
			{
				self->saveLastState();
			}
		});
}

void SceneComponent::zoomUi(bool zoomIn)
{
	float scale = Desktop::getInstance().getGlobalScaleFactor();
	scale += zoomIn ? + 0.05 : -0.05;
	scale = std::min(std::max(scale, 0.25f), 4.0f);
	scale = round(scale * 20) / 20;

	settings.zoomUi = scale;
	settings.Save();
}

void SceneComponent::toggleKeyboard()
{
	if (keyboardPanel->isVisible() && voiceComponent->isVisible() &&
		pianoController.GetActive(PianoController::chLeft))
	{
		keyboardManuallyHidden = true;
	}

	settings.keyboardVisible = !keyboardPanel->isVisible();
	settings.Save();
}

void SceneComponent::updateKeyboard()
{
	keyboardButton->setToggleState(settings.keyboardVisible, NotificationType::dontSendNotification);
	bool keyboardMustBeShown = settings.keyboardVisible ||
		(voiceComponent->isVisible() && pianoController.GetActive(PianoController::chLeft) && !keyboardManuallyHidden);
	if (keyboardPanel->isVisible() != keyboardMustBeShown ||
		keyboardComponent->GetSplitMode() != voiceComponent->isVisible())
	{
		keyboardComponent->SetSplitMode(voiceComponent->isVisible());
		keyboardPanel->setVisible(keyboardMustBeShown);
		keyboardButton->setToggleState(keyboardPanel->isVisible(), NotificationType::dontSendNotification);
		resized();
    }
}

// The keyboard can be made higher only as long as the keys can grow with it, i.e. the
// whole 88-key keyboard still fits into the width of the window (the keys keep their
// proportions, see KeyboardComponent::resized), and at most up to half of the window.
// It cannot be lower than the default height.
int SceneComponent::getMaxKeyboardHeight() const
{
	const int DefaultKeyWidth = 16; // as in KeyboardComponent
	const int NumWhiteKeys = 52;
	const int fullWidthHeight = DefaultKeyboardHeight * getWidth() / (DefaultKeyWidth * NumWhiteKeys);
	return std::max(DefaultKeyboardHeight, std::min(fullWidthHeight, getHeight() / 2));
}

int SceneComponent::getKeyboardHeight() const
{
	return jlimit(DefaultKeyboardHeight, getMaxKeyboardHeight(), settings.keyboardHeight);
}

void SceneComponent::setKeyboardHeight(int height, bool save)
{
	height = jlimit(DefaultKeyboardHeight, getMaxKeyboardHeight(), height);
	if (height != settings.keyboardHeight)
	{
		settings.keyboardHeight = height;
		resized();
	}
	if (save)
	{
		settings.Save();
	}
}

void SceneComponent::switchLargePanel(Button* button)
{
	voiceComponent->setVisible(button == voiceButton.get());
	scoreComponent->setVisible(button == scoreButton.get());
	mixerComponent->setVisible(button == mixerButton.get());

	voiceButton->setToggleState(button == voiceButton.get(), NotificationType::dontSendNotification);
	scoreButton->setToggleState(button == scoreButton.get(), NotificationType::dontSendNotification);
	mixerButton->setToggleState(button == mixerButton.get(), NotificationType::dontSendNotification);

	resized();
	updateKeyboard();
}

void SceneComponent::saveState()
{
	// Offered next to the loaded song if it is a MIDI file (then it is loaded with the song
	// next time), otherwise in the folder of the piano states. (A song of the piano has no
	// file: its name, e.g. "\SONG\...", is not a path; it was taken as one on the desktop.)
	String initialLocation = settings.memoryDirectory + File::getSeparatorString() + "Untitled.conmem";
	const String songname = pianoController.GetSongName();
	if (File::isAbsolutePath(songname) && File(songname).existsAsFile())
	{
		initialLocation = File(songname).withFileExtension(".conmem").getFullPathName();
	}

	GuiHelper::ShowFileSaveDialogAsync(TRANS("Please select the name for registration memory file..."),
		initialLocation, "*.conmem",
		[this, self = Component::SafePointer<Component>(this)](const URL& url)
		{
			if (self == nullptr) return; // deleted meanwhile
			settings.memoryDirectory = url.getLocalFile().getParentDirectory().getFullPathName();
			settings.Save();
			GuiHelper::CallAsync(this, [=](){
				// generate access token on sandboxed platforms (iOS)
				std::unique_ptr<OutputStream> outp(url.createOutputStream());
				outp.reset();

				RegistrationMemory::Options opts;
				RegistrationMemory regmem(pianoController, settings, opts, url.getLocalFile());
				regmem.Save();
			});
		});
}

void SceneComponent::loadState()
{
	GuiHelper::ShowFileOpenDialogAsync(TRANS("Please select the registration memory file to load..."),
		settings.memoryDirectory, "*.conmem",
		[this, self = Component::SafePointer<Component>(this)](const URL& url)
		{
			if (self == nullptr) return; // deleted meanwhile
			settings.memoryDirectory = url.getLocalFile().getParentDirectory().getFullPathName();
			settings.Save();
			GuiHelper::CallAsync(this, [=](){
				// generate access token on sandboxed platforms (iOS)
				std::unique_ptr<InputStream> inp(url.createInputStream(false));

				RegistrationMemory regmem(pianoController, settings, {}, url.getLocalFile());
				regmem.Load();
			});
    	});
}

// Main menu "About ConPianist...": the purpose and the main functions of the program,
// its authors and license, in the language of the user interface.
void SceneComponent::showAbout()
{
	const String version = JUCEApplication::getInstance()->getApplicationVersion();
	const String text = settings.GetEffectiveLanguage() == "hu" ?
		String(CharPointer_UTF8(
		"Vez\xc3\xa9" "rl\xc5\x91" "program Yamaha Clavinova CSP digit\xc3\xa1" "lis zongor\xc3\xa1"
		"khoz (fejlesztve \xc3\xa9" "s tesztelve: CSP-170), a Yamaha Smart Pianist alkalmaz\xc3\xa1"
		"s asztali alternat\xc3\xad" "v\xc3\xa1" "ja.\n\nMIRE J\xc3\x93" "?\nA zongora hangsz\xc3\xad"
		"neinek, kever\xc5\x91" "j\xc3\xa9" "nek \xc3\xa9" "s Piano Room be\xc3\xa1" "ll\xc3\xad" "t\xc3\xa1"
		"sainak kezel\xc3\xa9" "se; MIDI-dalok lej\xc3\xa1" "tsz\xc3\xa1" "sa kott\xc3\xa1"
		"val, gyakorl\xc3\xa1" "shoz (sz\xc3\xb3" "lamok, temp\xc3\xb3" ", transzpon\xc3\xa1" "l\xc3\xa1"
		"s, ism\xc3\xa9" "tl\xc3\xa9" "s).\n\nLEJ\xc3\x81" "TSZ\xc3\x81" "SI M\xc3\x93" "DOK\n\xe2\x80\x93"
		" H\xc3\xa1" "l\xc3\xb3" "zaton: a dal a zongora saj\xc3\xa1" "t lej\xc3\xa1" "tsz\xc3\xb3"
		"j\xc3\xa1" "val sz\xc3\xb3" "l (Stream Lights, Seg\xc3\xa9" "d).\n\xe2\x80\x93"
		" USB-n: a ConPianist maga j\xc3\xa1" "tssza a dalt, Wi-Fi n\xc3\xa9" "lk\xc3\xbc" "l.\n\xe2\x80\x93"
		" MIDI-eszk\xc3\xb6" "z\xc3\xb6" "n: a dal m\xc3\xa1" "s MIDI-eszk\xc3\xb6" "z\xc3\xb6"
		"n sz\xc3\xb3" "l (pl. szoftveres hangszeren), zongora n\xc3\xa9" "lk\xc3\xbc" "l is.\n\n\xc3\x89"
		"L\xc5\x90" " J\xc3\x81" "T\xc3\x89" "K\nA virtu\xc3\xa1" "lis billenty\xc5\xb1" "zet \xc3\xa9"
		"s a MIDI In 2 a zongora saj\xc3\xa1" "t hangj\xc3\xa1" "n vagy a Kever\xc5\x91" " csatorn\xc3\xa1"
		"in sz\xc3\xb3" "l.\n\nK\xc3\x89" "SZ\xc3\x8d" "T\xc5\x90"
		"K\nEredeti program: Andrey Prygunkov (hugbug/conpianist)\nFork \xc3\xa9" "s tov\xc3\xa1"
		"bbfejleszt\xc3\xa9" "s: Viktor Oszk\xc3\xb3"
		" (Viktor318/conpianist)\n\nLICENC\nGNU General Public License v3. Felhaszn\xc3\xa1" "lt k\xc3\xb6"
		"nyvt\xc3\xa1" "rak: JUCE, Lomse, Arduino AppleMIDI Library.")) :
		String(CharPointer_UTF8(
		"Control app for Yamaha Clavinova CSP digital pianos (developed and tested with the CSP-170), a desktop alternative to Yamaha's Smart Pianist app.\n\nWHAT IS IT FOR?\nManaging the piano's voices, mixer and Piano Room settings; playing MIDI songs with scores for practice (parts, tempo, transpose, loop).\n\nPLAYBACK MODES\n- Via network: the song is played by the piano's own player (Stream Lights, Guide).\n- Via USB: ConPianist plays the song itself, without Wi-Fi.\n- Via MIDI device: the song is played on another MIDI device (e.g. a software instrument), even without the piano.\n\nLIVE PLAY\nThe virtual keyboard and MIDI In 2 sound with the piano's own voices or on the Mixer channels.\n\nAUTHORS\nOriginal program: Andrey Prygunkov (hugbug/conpianist)\nFork and further development: Viktor Oszk\xc3\xb3"
		" (Viktor318/conpianist)\n\nLICENSE\nGNU General Public License v3. Libraries used: JUCE, Lomse, Arduino AppleMIDI Library."));
	AlertWindow::showMessageBoxAsync(MessageBoxIconType::NoIcon, "ConPianist " + version, text);
}

// The Recording window is not modal: it stays open (on top) while the program is used.
void SceneComponent::showRecorder()
{
	if (!recorderWindow)
	{
		recorderWindow = std::make_unique<RecorderWindow>(settings, pianoController);
		if (!recorderWindow->RestorePosition())
		{
			recorderWindow->centreAroundComponent(this, recorderWindow->getWidth(), recorderWindow->getHeight());
		}
	}
	recorderWindow->setVisible(true);
	recorderWindow->setMinimised(false); // opened from the menu while it is minimised
	recorderWindow->toFront(true);
}

// The Accompaniment window is not modal either; its keyboard shortcuts work while it is
// the active window.
void SceneComponent::showAccompaniment()
{
	if (!accompanimentWindow)
	{
		accompanimentWindow = std::make_unique<AccompanimentWindow>(settings, pianoController);
		if (!accompanimentWindow->RestorePosition())
		{
			accompanimentWindow->centreAroundComponent(this, accompanimentWindow->getWidth(), accompanimentWindow->getHeight());
		}
	}
	accompanimentWindow->setVisible(true);
	accompanimentWindow->setMinimised(false); // opened from the menu while it is minimised
	accompanimentWindow->toFront(true);
	if (Component* content = accompanimentWindow->getContentComponent())
	{
		content->grabKeyboardFocus();
	}
}

// The Song Selector stays open after a song is loaded; it can be minimised to see the score.
void SceneComponent::showSongSelector()
{
	if (!songSelectorWindow)
	{
		songSelectorWindow = std::make_unique<SongSelectorWindow>(settings, pianoController);
		if (!songSelectorWindow->RestoreBounds())
		{
			songSelectorWindow->centreAroundComponent(this, songSelectorWindow->getWidth(), songSelectorWindow->getHeight());
		}
		if (SongSelectorComponent* selector = songSelectorWindow->GetSelector())
		{
			selector->onLoadFile = [this](const File& file) { playbackComponent->loadSongFile(file); };
			selector->onOpenFile = [this]() { playbackComponent->chooseSongFile(); };
		}
	}
	else if (SongSelectorComponent* selector = songSelectorWindow->GetSelector())
	{
		selector->Refresh(); // files may have been added to the folders meanwhile
	}
	songSelectorWindow->setVisible(true);
	songSelectorWindow->setMinimised(false);
	songSelectorWindow->toFront(true);
}

void SceneComponent::showStyleMixer()
{
	if (!styleMixerWindow)
	{
		styleMixerWindow = std::make_unique<StyleMixerWindow>(settings, pianoController);
		if (!styleMixerWindow->RestorePosition())
		{
			styleMixerWindow->centreAroundComponent(this, styleMixerWindow->getWidth(), styleMixerWindow->getHeight());
		}
	}
	styleMixerWindow->setVisible(true);
	styleMixerWindow->setMinimised(false); // opened again while it is minimised
	styleMixerWindow->toFront(true);
}

void SceneComponent::showBalance()
{
	if (!balanceWindow)
	{
		balanceWindow = std::make_unique<BalanceWindow>(settings, pianoController);
		if (!balanceWindow->RestorePosition())
		{
			balanceWindow->centreAroundComponent(this, balanceWindow->getWidth(), balanceWindow->getHeight());
		}
	}
	balanceWindow->setVisible(true);
	balanceWindow->setMinimised(false); // opened again while it is minimised
	balanceWindow->toFront(true);
}

void SceneComponent::requestExit()
{
	if (!pianoController.GetRecorder().IsUnsaved())
	{
		JUCEApplication::quit();
		return;
	}

	AlertWindow::showAsync(MessageBoxOptions()
			.withIconType(MessageBoxIconType::WarningIcon)
			.withTitle("ConPianist")
			.withMessage(TRANS("There is a recording that has not been saved. Exit without saving it?"))
			.withButton(TRANS("Exit"))
			.withButton(TRANS("Cancel"))
			.withAssociatedComponent(this),
		[this, self = Component::SafePointer<Component>(this)](int result)
		{
			if (self == nullptr)
			{
				return;
			}
			if (result == 1)
			{
				JUCEApplication::quit();
			}
			else
			{
				showRecorder(); // to save it
			}
		});
}

void SceneComponent::changeLanguage(const String& language)
{
	if (language == settings.GetEffectiveLanguage())
	{
		return;
	}

	settings.language = language;
	settings.Save();

	// The UI texts are translated when the components are created, so the new
	// language takes effect on the next start. The message is shown in both
	// languages, because the user may not understand the current one.
	AlertWindow::showMessageBoxAsync(MessageBoxIconType::InfoIcon, "ConPianist",
		String(CharPointer_UTF8("The new language will be applied after restarting ConPianist.\n\nAz \xc3\xba" "j nyelv a ConPianist \xc3\xba" "jraind\xc3\xad" "t\xc3\xa1" "sa ut\xc3\xa1" "n l\xc3\xa9" "p \xc3\xa9" "letbe.")));
}

void SceneComponent::loadSongState()
{
	const bool switched = pianoController.TakeSkipRegistrationMemory();
	const String song = pianoController.GetSongName();
	if (PianoSongPath(song).isNotEmpty() && settings.lastSong != song)
	{
		// the last loaded song is one of the piano's own songs: with USB or MIDI device
		// playback its MIDI file is loaded, not an earlier file
		settings.lastSong = song;
		settings.Save();
	}

	if (!songStateRestored)
	{
		// the first song after the start: if it is the song of the last state, its
		// settings are restored instead of its own registration memory
		songStateRestored = true;
		const String lastSong = getLastStateSong();
		if (PianoSongPath(lastSong).isNotEmpty())
		{
			// one of the piano's own songs (with USB or MIDI device playback from its MIDI
			// file): it starts as if it was loaded with the Load button of the Song
			// Selector, with its own settings; only a registration memory next to the
			// MIDI file is loaded (as for every song)
			const File lastSongMidi = PianoSongMidi(settings, lastSong);
			if (IsSameSong(song, lastSong) ||
				(lastSongMidi.existsAsFile() && File::isAbsolutePath(song) && File(song) == lastSongMidi))
			{
				const File conmem = PianoSongPath(song).isEmpty() ? File(song).withFileExtension(".conmem") : File();
				if (conmem.existsAsFile() && conmem.getSize() > 0)
				{
					RegistrationMemory regmem(pianoController, settings, {}, conmem);
					regmem.Load();
				}
				else
				{
					resetSongSettings();
				}
				scheduleLastStateSave();
				return;
			}
		}
		else if (IsSameSong(song, lastSong))
		{
			restoreSongState();
			scheduleLastStateSave();
			return;
		}
	}

	if (switched)
	{
		// the player was switched: the settings from before the switch are kept
		return;
	}

	if (PianoSongPath(song).isEmpty() && File::isAbsolutePath(song))
	{
		File file = File(song).withFileExtension(".conmem");
		if (file.existsAsFile() && file.getSize() > 0)
		{
			RegistrationMemory regmem(pianoController, settings, {}, file);
			regmem.Load();
		}
	}
	scheduleLastStateSave();
}
//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Projucer information section --

    This is where the Projucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="SceneComponent" componentName=""
                 parentClasses="public Component, public PianoController::Listener, public ChangeListener, public Timer"
                 constructorParams="Settings&amp; settings" variableInitialisers="settings(settings)"
                 snapPixels="8" snapActive="1" snapShown="1" overlayOpacity="0.330"
                 fixedSize="0" initialWidth="850" initialHeight="550">
  <BACKGROUND backgroundColour="ff323e44">
    <RECT pos="0 43 0M 1" fill="solid: ff4e5b62" hasStroke="0"/>
  </BACKGROUND>
  <GROUPCOMPONENT name="Top Bar" id="69305d91c2150486" memberName="topbarPanel"
                  virtualName="" explicitFocusOrder="0" pos="0 -8 0M 52" title="Top Bar"
                  textpos="36"/>
  <GENERICCOMPONENT name="Playback Panel" id="cf6dcbcdc3b17ace" memberName="playbackPanel"
                    virtualName="" explicitFocusOrder="0" pos="0 52 290 111M" posRelativeY="69305d91c2150486"
                    class="Component" params=""/>
  <GENERICCOMPONENT name="Large Content" id="5d00b51e97f2c31f" memberName="largeContentPanel"
                    virtualName="" explicitFocusOrder="0" pos="0R 52 290M 111M" posRelativeX="cf6dcbcdc3b17ace"
                    posRelativeY="69305d91c2150486" class="Component" params=""/>
  <IMAGEBUTTON name="Mute Button" id="ca510a4be11fdde2" memberName="muteButton"
               virtualName="" explicitFocusOrder="0" pos="80Rr 8 32 28" posRelativeX="c7b94b60aa96c6e2"
               posRelativeY="c7b94b60aa96c6e2" tooltip="Local Control on/off"
               buttonText="Mute" connectedEdges="0" needsCallback="1" radioGroupId="0"
               keepProportions="1" resourceNormal="BinaryData::buttonvolume_png"
               opacityNormal="1.0" colourNormal="0" resourceOver="" opacityOver="0.75"
               colourOver="0" resourceDown="" opacityDown="1.0" colourDown="0"/>
  <IMAGEBUTTON name="Zoom In UI Button" id="8f2ba3f851b38bd8" memberName="zoomInButton"
               virtualName="" explicitFocusOrder="0" pos="10Rr 8 32 28" posRelativeX="c7b94b60aa96c6e2"
               posRelativeY="c7b94b60aa96c6e2" tooltip="Zoom In UI" buttonText="Mute"
               connectedEdges="0" needsCallback="1" radioGroupId="0" keepProportions="1"
               resourceNormal="BinaryData::buttonzoomin_png" opacityNormal="1.0"
               colourNormal="0" resourceOver="" opacityOver="0.75" colourOver="0"
               resourceDown="" opacityDown="1.0" colourDown="0"/>
  <IMAGEBUTTON name="Zoom Out UI Button" id="9c93ecb0c87ce0c4" memberName="zoomOutButton"
               virtualName="" explicitFocusOrder="0" pos="45Rr 8 32 28" posRelativeX="c7b94b60aa96c6e2"
               posRelativeY="c7b94b60aa96c6e2" tooltip="Zoom Out UI" buttonText="Mute"
               connectedEdges="0" needsCallback="1" radioGroupId="0" keepProportions="1"
               resourceNormal="BinaryData::buttonzoomout_png" opacityNormal="1.0"
               colourNormal="0" resourceOver="" opacityOver="0.75" colourOver="0"
               resourceDown="" opacityDown="1.0" colourDown="0"/>
  <GENERICCOMPONENT name="Keyboard Panel" id="d578dbfb8bf47c83" memberName="keyboardPanel"
                    virtualName="" explicitFocusOrder="0" pos="0 0Rr 0M 67" class="Component"
                    params=""/>
  <IMAGEBUTTON name="Virtual Keyboard Button" id="802f8ad4daaeee49" memberName="keyboardButton"
               virtualName="" explicitFocusOrder="0" pos="115Rr 8 32 28" posRelativeX="c7b94b60aa96c6e2"
               posRelativeY="c7b94b60aa96c6e2" tooltip="Virtual Keyboard" buttonText="Virtual Keyboard"
               connectedEdges="0" needsCallback="1" radioGroupId="0" keepProportions="1"
               resourceNormal="BinaryData::buttonkeyboard_png" opacityNormal="1.0"
               colourNormal="0" resourceOver="" opacityOver="0.75" colourOver="0"
               resourceDown="" opacityDown="1.0" colourDown="0"/>
  <IMAGEBUTTON name="Balance Button" id="b26d1a0a73e9171c" memberName="balanceButton"
               virtualName="" explicitFocusOrder="0" pos="185Rr 8 32 28" tooltip="Balance"
               buttonText="Balance" connectedEdges="0" needsCallback="1" radioGroupId="0"
               keepProportions="1" resourceNormal="BinaryData::buttonbalance_png"
               opacityNormal="1.0" colourNormal="0" resourceOver="" opacityOver="0.75"
               colourOver="0" resourceDown="" opacityDown="1.0" colourDown="0"/>
  <TEXTBUTTON name="Voice Button" id="f0432a7d7961584d" memberName="voiceButton"
              virtualName="" explicitFocusOrder="0" pos="378 18 80 34" posRelativeX="69305d91c2150486"
              posRelativeY="69305d91c2150486" tooltip="Voice Selection" buttonText="Voice"
              connectedEdges="0" needsCallback="1" radioGroupId="0"/>
  <TEXTBUTTON name="Score Button" id="470fdf4dc9f8f0cd" memberName="scoreButton"
              virtualName="" explicitFocusOrder="0" pos="298 18 80 34" posRelativeX="69305d91c2150486"
              posRelativeY="69305d91c2150486" tooltip="Score View" buttonText="Score"
              connectedEdges="0" needsCallback="1" radioGroupId="0"/>
  <LABEL name="Status Label" id="71086bde8935001" memberName="statusLabel"
         virtualName="" explicitFocusOrder="0" pos="48 11 240 24" edTextCol="ff000000"
         edBkgCol="0" labelText="Looking for the instrument" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15.0" kerning="0.0" bold="0" italic="0" justification="33"/>
  <TEXTBUTTON name="Mixer Button" id="e43c013391cb868" memberName="mixerButton"
              virtualName="" explicitFocusOrder="0" pos="458 18 80 34" posRelativeX="69305d91c2150486"
              posRelativeY="69305d91c2150486" tooltip="Channel Mixer" buttonText="Mixer"
              connectedEdges="0" needsCallback="1" radioGroupId="0"/>
  <IMAGEBUTTON name="Menu Button" id="c87eaad1c0559e4c" memberName="menuButton"
               virtualName="" explicitFocusOrder="0" pos="8 8 32 28" posRelativeX="c7b94b60aa96c6e2"
               posRelativeY="c7b94b60aa96c6e2" tooltip="Main Menu" buttonText="Menu"
               connectedEdges="0" needsCallback="1" radioGroupId="0" keepProportions="1"
               resourceNormal="BinaryData::buttonmenu_png" opacityNormal="1.0"
               colourNormal="0" resourceOver="" opacityOver="0.75" colourOver="0"
               resourceDown="" opacityDown="1.0" colourDown="0"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
//[/EndFile]

