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
        // This is called when the app is being asked to quit: you can ignore this
        // request and let the app carry on running, or call quit() to allow the app to close.
        quit();
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
    class MainWindow    : public DocumentWindow
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
			Rectangle<int> bounds = settings.windowPos;
			if (bounds.getX() > -100 && bounds.getY() > -100 &&
				bounds.getWidth() > 200 && bounds.getHeight() > 100)
			{
				setBounds(bounds);
			}
#endif

            setVisible (true);
        }

		~MainWindow()
		{
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
