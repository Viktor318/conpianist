/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic startup code for a JUCE application.

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "LookAndFeel.h"
#include "GuiHelper.h"
#include "SceneComponent.h"
#include "ScoreComponent.h"

#if JUCE_WINDOWS
 #include <stdlib.h>
 #if defined(_MSC_VER) && defined(_DEBUG)
  #include <crtdbg.h>
 #endif
// from <windows.h>, which is not included here (its names clash with those of JUCE)
extern "C" __declspec(dllimport) unsigned int __stdcall SetErrorMode(unsigned int mode);
// from <dwmapi.h> (dwmapi.lib is linked by JUCE): the window handle as void*, the result
// as HRESULT (long)
extern "C" __declspec(dllimport) long __stdcall DwmSetWindowAttribute(void* hwnd, unsigned long attribute,
	const void* value, unsigned long size);
#endif

//==============================================================================
class ConnectedPianistApplication  : public JUCEApplication
{
public:
    //==============================================================================
    ConnectedPianistApplication() {}

    const String getApplicationName() override       { return ProjectInfo::projectName; }
    const String getApplicationVersion() override    { return ProjectInfo::versionString; }
    bool moreThanOneInstanceAllowed() override       { return true; }
    TooltipWindow tooltipWindow{nullptr, 1000}; // the only tooltip window, for every window of the program
    ::LookAndFeel lookAndFeel;

    //==============================================================================
    void initialise (const String& commandLine) override
    {
        // This method is where you should put your application's initialisation code..

		// the check of a score before it is shown (see ScoreComponent::CheckScoreFile):
		// no window, the result is the exit code
		if (commandLine.contains(ScoreComponent::CheckScoreArgument))
		{
			SuppressErrorWindows();
			StringArray arguments;
			arguments.addTokens(commandLine, true); // the paths are in quotes if they have spaces
			const int index = arguments.indexOf(ScoreComponent::CheckScoreArgument);
			const File score(arguments[index + 1].unquoted());
			const File result(arguments[index + 2].unquoted());
			setApplicationReturnValue(ScoreComponent::CheckScoreFile(score, result));
			quit();
			return;
		}

#if JUCE_IOS
		Desktop::getInstance().setGlobalScaleFactor(1.2);
		CreateSharedDocumenstDirectory();
#endif
		Desktop::getInstance().setDefaultLookAndFeel(&lookAndFeel);
        mainWindow.reset(new MainWindow (getApplicationName()));
    }

    void shutdown() override
    {
        // Add your application's shutdown code here..

        mainWindow = nullptr; // (deletes our window)
    }

    //==============================================================================
    void systemRequestedQuit() override
    {
        // This is called when the app is being asked to quit (e.g. Windows is shut down or
        // the user logs off): it goes the way of the close button, so the program asks
        // first if a recording has not been saved yet (until then Windows waits).
        if (mainWindow)
        {
            mainWindow->closeButtonPressed();
        }
        else
        {
            quit();
        }
    }

    void anotherInstanceStarted (const String& commandLine) override
    {
        // When another instance of the app is launched while this one is running,
        // this method is invoked, and the commandLine parameter tells you what
        // the other instance's command-line arguments were.
    }

	// In the score check process a crash must not open a window ("ConPianist has stopped
	// working", or the assertion window of the Debug build): the program that started the
	// check reports the fault itself.
	static void SuppressErrorWindows()
	{
#if JUCE_WINDOWS
		const unsigned int SemFailCriticalErrors = 0x0001, SemNoGpFaultErrorBox = 0x0002, SemNoOpenFileErrorBox = 0x8000;
		SetErrorMode(SemFailCriticalErrors | SemNoGpFaultErrorBox | SemNoOpenFileErrorBox);
		_set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
 #if defined(_MSC_VER) && defined(_DEBUG)
		_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);
		_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
 #endif
#endif
	}

	void CreateSharedDocumenstDirectory()
	{
		// Create app Documents directory, for use in "Files" app
		File docPath = File::getSpecialLocation(File::userDocumentsDirectory);
		DirectoryIterator iter(docPath, false);
		if (!iter.next())
		{
			File docFlag = docPath.getChildFile("Put your midi files here");
			std::unique_ptr<FileOutputStream> outp(docFlag.createOutputStream());
		}
	}

    class ConFileLogger : public FileLogger
    {
    public:
    	ConFileLogger() : FileLogger(getSystemLogFileFolder()
    	#ifdef WIN32
    		.getChildFile("ConPianist")
		#endif
    		.getChildFile("ConPianist.log"), String("ConPianist ") +
    		JUCEApplication::getInstance()->getApplicationVersion()) {}
	protected:
    	void logMessage(const String& message) override
    	{
			Time time = Time::getCurrentTime();
			String timestr = time.formatted("%Y-%m-%d %H:%M:%S.") + String(time.getMilliseconds()).paddedLeft('0', 3);
			String text(timestr + " " + message);
			FileLogger::logMessage(text);
		}
	};

    //==============================================================================
    /*
        This class implements the desktop window that contains an instance of
        our MainComponent class.
    */
    class MainWindow    : public DocumentWindow, private ComponentListener
    {
    public:
        MainWindow (String name)  : DocumentWindow (name,
                                                    Desktop::getInstance().getDefaultLookAndFeel()
                                                                          .findColour (ResizableWindow::backgroundColourId),
                                                    DocumentWindow::allButtons)
        {
			settings.Load();
			settings.CreateSongFolders();
			settings.ApplyLanguage();

			if (settings.logging)
			{
				logger.reset(new ConFileLogger());
				Logger::setCurrentLogger(logger.get());
			}

            setUsingNativeTitleBar (true);
            content = std::make_unique<SceneComponent>(settings);
            setContentNonOwned(content.get(), true);
            setResizable (true, true);

#if JUCE_ANDROID || JUCE_IOS
			setFullScreen(true);
#else
			centreWithSize (getWidth(), getHeight());
 #if JUCE_WINDOWS
			// Shown maximised, Windows animates the window from its normal size, and the
			// window is black until it has been painted at its new size: it is hidden
			// (cloaked) until then, and appears maximised at once (see componentPainted).
			const bool maximised = settings.windowState.trim().startsWithIgnoreCase("fs");
			if (maximised)
			{
				setCloaked(true);
			}
 #endif
			// The state saved by JUCE: maximised too, and moved onto a screen if the screens
			// have changed since (e.g. a second monitor was disconnected). Without it (saved by
			// an older version): the position, if its top is on a screen.
			// Maximised ("fs x y w h"): first the normal position and size, then maximised.
			// JUCE notes the normal size (saved at exit) before it sets the saved one, and does
			// not update it while the window is not shown: restored maximised at once, the
			// normal size would be the default size of the window.
			const String windowState = settings.windowState.trim();
			if (windowState.startsWithIgnoreCase("fs"))
			{
				restoreWindowStateFromString(windowState.fromFirstOccurrenceOf("fs", false, true).trim());
			}
			if (windowState.isEmpty() || !restoreWindowStateFromString(windowState))
			{
				Rectangle<int> bounds = settings.windowPos;
				if (bounds.getWidth() > 200 && bounds.getHeight() > 100 &&
					Settings::IsWindowPosUsable(bounds.getPosition(), bounds.getWidth()))
				{
					setBounds(bounds);
				}
			}
#endif

            setVisible (true);

#if JUCE_WINDOWS
			if (maximised)
			{
				// shown when the content has been painted; at the latest after the time limit
				content->addComponentListener(this);
				uncloakAfter(UncloakTimeoutMs, "time limit");
			}
#endif
        }

		~MainWindow()
		{
			// an open dialog (e.g. Connection Settings) is closed first: it saves its settings
			// when it is closed, and it must not outlive the settings and the window
			GuiHelper::CloseDialogs();
			settings.windowState = getWindowStateAsString();
			// the window disappears at once: closing the connections and the MIDI ports
			// may take some seconds (e.g. after the piano was switched off)
			setVisible(false);
			settings.windowPos = getBounds();
			settings.Save();
			content = nullptr; // this destroys the SceneComponent
			Logger::writeToLog("Application ended gracefully");
			Logger::setCurrentLogger(nullptr);
			GuiHelper::Final();
		}

        void closeButtonPressed() override
        {
            // This is called when the user tries to close this window. Here, we'll just
            // ask the app to quit when this happens, but you can change this to do
            // whatever you need.
            // The scene asks first if a recording has not been saved yet.
            if (content)
            {
                content->requestExit();
            }
            else
            {
                JUCEApplication::getInstance()->systemRequestedQuit();
            }
        }

        /* Note: Be careful if you override any DocumentWindow methods - the base
           class uses a lot of them, so by overriding you might break its functionality.
           It's best to do all your work in your content component instead, but if
           you really have to override any DocumentWindow methods, make sure your
           subclass also calls the superclass's method.
        */

    private:
#if JUCE_WINDOWS
		// The maximised window is hidden at start until its content has been painted (at its
		// maximised size), and a moment more, until the picture is on the screen; at the
		// latest until the time limit (if no paint comes).
		static const int UncloakAfterPaintMs = 50;
		static const int UncloakTimeoutMs = 1500;
		bool cloaked = false;
		double cloakedAtMs = 0; // for the log: how long the window was hidden

		// Hides or shows the window without changing its state (DWMWA_CLOAK): a cloaked
		// window is laid out and painted, but not drawn on the screen and not animated.
		void setCloaked(bool shouldBeCloaked)
		{
			if (auto* peer = getPeer())
			{
				const unsigned long DwmwaCloak = 13;
				const int value = shouldBeCloaked ? 1 : 0; // BOOL
				DwmSetWindowAttribute(peer->getNativeHandle(), DwmwaCloak, &value, sizeof(value));
				cloaked = shouldBeCloaked;
				if (shouldBeCloaked)
				{
					cloakedAtMs = Time::getMillisecondCounterHiRes();
					Logger::writeToLog("Main window: maximised, hidden until it is painted");
				}
			}
		}

		int getCloakedMs() const
		{
			return roundToInt(Time::getMillisecondCounterHiRes() - cloakedAtMs);
		}

		// shown (and brought to the front: while it was hidden, another program, e.g. the
		// debugger, may have become the active one)
		void uncloakAfter(int delayMs, const String& reason)
		{
			Component::SafePointer<MainWindow> window(this);
			Timer::callAfterDelay(delayMs, [window, reason]
			{
				if (window != nullptr && window->cloaked)
				{
					if (window->content != nullptr)
					{
						window->content->removeComponentListener(window.getComponent());
					}
					window->setCloaked(false);
					window->toFront(true);
					Logger::writeToLog("Main window: shown after " + String(window->getCloakedMs()) +
						" ms (" + reason + ")");
				}
			});
		}

		// the first paint of the content (the window is already maximised then)
		void componentPainted(Component& component, const ComponentPaintDiagnostics&) override
		{
			component.removeComponentListener(this);
			Logger::writeToLog("Main window: first paint after " + String(getCloakedMs()) + " ms");
			uncloakAfter(UncloakAfterPaintMs, "painted");
		}
#endif

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
        Settings settings;
    	std::unique_ptr<FileLogger> logger;
    	std::unique_ptr<SceneComponent> content;
    };

private:
    std::unique_ptr<MainWindow> mainWindow;
};

//==============================================================================
// This macro generates the main() routine that launches the app.
START_JUCE_APPLICATION (ConnectedPianistApplication)
