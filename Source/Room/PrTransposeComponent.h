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

#pragma once

//[Headers]     -- You can add your own extra header files here --
#include "../JuceLibraryCode/JuceHeader.h"
#include "PrBaseComponent.h"
#include "GuiHelper.h"
//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Projucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class PrTransposeComponent  : public PrBaseComponent,
                               public Slider::Listener
{
public:
    //==============================================================================
    PrTransposeComponent (Settings& settings, PianoController& pianoController);
    ~PrTransposeComponent() override;

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.
    void PianoStateChanged(PianoController::Aspect ap, PianoController::Channel ch) override
		{ if (ap == PianoController::apKeyboardTranspose || ap == PianoController::apConnection)
			GuiHelper::CallAsync(this, [=](){updatePianoState(ap);}); }
	void updatePianoState(PianoController::Aspect aspect);
    void mouseDoubleClick (const MouseEvent& e) override;
    //[/UserMethods]

    void paint (Graphics& g) override;
    void resized() override;
    void sliderValueChanged (Slider* sliderThatWasMoved) override;



private:
    //[UserVariables]   -- You can add your own custom variables in this section.
    int inSliderChange = 0;
    //[/UserVariables]

    //==============================================================================
    std::unique_ptr<Slider> slider;
    std::unique_ptr<Label> titleLabel;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PrTransposeComponent)
};

//[EndFile] You can add extra defines here...
//[/EndFile]

