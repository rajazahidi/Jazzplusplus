//*****************************************************************************
// The JAZZ++ Midi Sequencer — JUCE 8 Edition
//
// Copyright (C) 2026 Raja Zahidi
// Interactive Guitar & Bass Fretboard with Scale & Chord Visualizer
//*****************************************************************************

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "AudioMidiEngine.h"
#include "ChordScaleData.h"
#include "JazzLookAndFeel.h"

#include <vector>

class FretboardComponent : public juce::Component
{
public:
    FretboardComponent(AudioMidiEngine& engine);
    ~FretboardComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    void setScale(const juce::String& rootNote, const juce::String& scaleName);

private:
    AudioMidiEngine& engine;

    // Guitar tuning (standard 6-string EADGBE)
    // MIDI numbers: E2=40, A2=45, D3=50, G3=55, B3=59, E4=64
    std::vector<int> stringTunings { 64, 59, 55, 50, 45, 40 }; // High E down to Low E
    std::vector<juce::String> stringNames { "E4", "B3", "G3", "D3", "A2", "E2" };

    static constexpr int kNumFrets = 24;

    int scaleRootPitch = 0; // 0 = C
    std::vector<int> scaleIntervals { 0, 2, 4, 5, 7, 9, 11 }; // Major default

    int auditionPitch = -1;

    // Controls
    juce::ComboBox rootSelector;
    juce::ComboBox scaleSelector;

    bool isPitchRoot(int pitch) const;
    bool isPitchInScale(int pitch) const;
    int getPitchAtFret(int stringIdx, int fret) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FretboardComponent)
};
