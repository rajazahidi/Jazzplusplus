//*****************************************************************************
// The JAZZ++ Midi Sequencer — JUCE 8 Edition
//
// Copyright (C) 2026 Raja Zahidi
// Application Entry Point & Top-Level Window Management
//*****************************************************************************

#include <juce_gui_basics/juce_gui_basics.h>
#include "JazzLookAndFeel.h"
#include "MainComponent.h"

class JazzJuceApp : public juce::JUCEApplication
{
public:
    JazzJuceApp() = default;

    const juce::String getApplicationName() override       { return "Jazz++ JUCE"; }
    const juce::String getApplicationVersion() override    { return "6.0.0"; }
    bool moreThanOneInstanceAllowed() override             { return true; }

    void initialise(const juce::String& commandLine) override
    {
        juce::LookAndFeel::setDefaultLookAndFeel(&lookAndFeel);
        mainWindow = std::make_unique<MainWindow>(getApplicationName());

        if (auto* mainComp = dynamic_cast<MainComponent*>(mainWindow->getContentComponent()))
        {
            if (commandLine.containsIgnoreCase("piano"))
                mainComp->setActiveView(MainComponent::eViewPianoRoll);
            else if (commandLine.containsIgnoreCase("drum"))
                mainComp->setActiveView(MainComponent::eViewDrumMachine);
            else if (commandLine.containsIgnoreCase("fret"))
                mainComp->setActiveView(MainComponent::eViewFretboard);
            else if (commandLine.containsIgnoreCase("fx"))
                mainComp->setActiveView(MainComponent::eViewAudioFx);
            else if (commandLine.containsIgnoreCase("plugin"))
                mainComp->setActiveView(MainComponent::eViewPluginHost);
        }
    }

    void shutdown() override
    {
        mainWindow.reset();
        juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
    }

    void systemRequestedQuit() override
    {
        quit();
    }

    void anotherInstanceStarted(const juce::String& /*commandLine*/) override
    {
    }

    class MainWindow : public juce::DocumentWindow
    {
    public:
        MainWindow(juce::String name)
            : DocumentWindow(name,
                             JazzLookAndFeel::Colors::backgroundDark,
                             DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar(true);
            setContentOwned(new MainComponent(), true);

           #if JUCE_IOS || JUCE_ANDROID
            setFullScreen(true);
           #else
            setResizable(true, true);
            setResizeLimits(960, 600, 3840, 2160);
            centreWithSize(1280, 800);
           #endif

            setVisible(true);
        }

        void closeButtonPressed() override
        {
            JUCEApplication::getInstance()->systemRequestedQuit();
        }

    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
    };

private:
    JazzLookAndFeel lookAndFeel;
    std::unique_ptr<MainWindow> mainWindow;
};

// Start the JUCE Application
START_JUCE_APPLICATION(JazzJuceApp)
