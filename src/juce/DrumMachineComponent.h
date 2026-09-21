//*****************************************************************************
// The JAZZ++ Midi Sequencer — JUCE 8 Edition
//
// Copyright (C) 2026 Raja Zahidi
// 808 Drum Machine & 16-Step Sequencer with Live MPC Trigger Pads
//*****************************************************************************

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "AudioMidiEngine.h"
#include "SoundGenerator.h"
#include "JazzLookAndFeel.h"

#include <array>
#include <vector>

class DrumMachineComponent : public juce::Component,
                             public AudioMidiEngine::Listener
{
public:
    static constexpr int kNumVoices = 8;
    static constexpr int kNumSteps = 16;

    struct DrumVoice
    {
        juce::String name;
        JZSynthDrumType type;
        bool isRetroSFX = false;
        JZRetroSFXType sfxType = eSFXLaser;
        juce::Colour color;
        float volume = 0.9f;
        float pan = 0.5f;
    };

    DrumMachineComponent(AudioMidiEngine& engine);
    ~DrumMachineComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& e) override;

    // AudioMidiEngine::Listener overrides
    void stepTriggered(int step16th) override;
    void playbackStateChanged(bool isPlaying) override;

    void loadPreset(int presetIdx);
    void clearPattern();

private:
    AudioMidiEngine& engine;
    std::array<DrumVoice, kNumVoices> voices;
    std::array<std::array<bool, kNumSteps>, kNumVoices> pattern {};

    int currentPlayStep = -1;
    int padFlashIdx = -1;

    // Preset buttons
    juce::TextButton btnPresetTrap { "808 Trap" };
    juce::TextButton btnPresetHouse { "Classic House" };
    juce::TextButton btnPresetTechno { "Techno Pulse" };
    juce::TextButton btnPresetArcade { "Retro Chiptune" };
    juce::TextButton btnClear { "Clear" };

    void drawSequencerGrid(juce::Graphics& g, juce::Rectangle<int> area);
    void drawTriggerPads(juce::Graphics& g, juce::Rectangle<int> area);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrumMachineComponent)
};
