//*****************************************************************************
// The JAZZ++ Midi Sequencer — JUCE 8 Edition
//
// Copyright (C) 2026 Raja Zahidi
// Main Application Component with Top Transport Bar & View Switcher
//*****************************************************************************

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_utils/juce_audio_utils.h>

#include "AudioMidiEngine.h"
#include "ArrangerComponent.h"
#include "PianoRollComponent.h"
#include "DrumMachineComponent.h"
#include "FretboardComponent.h"
#include "AudioEffectsRackComponent.h"
#include "PluginHostComponent.h"
#include "JazzLookAndFeel.h"

class MainComponent : public juce::Component,
                      public AudioMidiEngine::Listener,
                      public juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // AudioMidiEngine::Listener overrides
    void playbackStateChanged(bool isPlaying) override;
    void tempoChanged(double newBpm) override;
    void playheadMoved(int bar, int beat, int tick, double seconds) override;

    // Timer override for 30Hz smooth UI meters & clock refresh
    void timerCallback() override;

    enum ViewIndex
    {
        eViewArranger = 0,
        eViewPianoRoll,
        eViewDrumMachine,
        eViewFretboard,
        eViewAudioFx,
        eViewPluginHost
    };

    void setActiveView(ViewIndex view);

private:
    AudioMidiEngine engine;

    // Sub-components
    ArrangerComponent arrangerComp;
    PianoRollComponent pianoRollComp;
    DrumMachineComponent drumMachineComp;
    FretboardComponent fretboardComp;
    AudioEffectsRackComponent fxRackComp;
    PluginHostComponent pluginHostComp;

    ViewIndex currentView = eViewArranger;

    // Top Transport Bar Controls
    juce::TextButton btnRewind { "|<" };
    juce::TextButton btnPlay { "> PLAY" };
    juce::TextButton btnPause { "||" };
    juce::TextButton btnStop { "[]" };
    juce::TextButton btnRecord { "REC" };
    juce::TextButton btnLoop { "LOOP" };

    juce::Slider slTempo;
    juce::Label lblTempo { {}, "BPM" };
    juce::ComboBox cmbTimeSig;

    juce::Label lblTimeDisplay;
    juce::TextButton btnAudioSettings { "AUDIO / MIDI" };

    // View switcher buttons
    juce::TextButton btnTabArranger { "ARRANGER" };
    juce::TextButton btnTabPianoRoll { "PIANO ROLL" };
    juce::TextButton btnTabDrums { "808 DRUMS" };
    juce::TextButton btnTabFretboard { "FRETBOARD" };
    juce::TextButton btnTabFx { "AUDIO FX" };
    juce::TextButton btnTabPlugins { "PLUGINS (VST3 / CLAP)" };

    // Bottom Bar Controls
    juce::Slider slMasterVolume;
    juce::Label lblMasterVolume { {}, "MASTER" };
    juce::Label lblStatusBar;

    float meterLeftDecay = 0.0f;
    float meterRightDecay = 0.0f;

    void updateTabButtons();
    void showAudioSettingsModal();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
