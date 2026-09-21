//*****************************************************************************
// The JAZZ++ Midi Sequencer — JUCE 8 Edition
//
// Copyright (C) 2026 Raja Zahidi
// Interactive High-DPI Piano Roll Editor with Scale Highlighting
//*****************************************************************************

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "AudioMidiEngine.h"
#include "ChordScaleData.h"
#include "JazzLookAndFeel.h"

#include <vector>

struct PianoRollNote
{
    int pitch = 60; // Middle C
    int64_t startTick = 0;
    int64_t lengthTicks = 240; // 8th note default (480 PPQN)
    int velocity = 100;
    bool selected = false;
};

class PianoRollComponent : public juce::Component,
                           public AudioMidiEngine::Listener
{
public:
    PianoRollComponent(AudioMidiEngine& engine);
    ~PianoRollComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;

    // AudioMidiEngine::Listener overrides
    void playheadMoved(int bar, int beat, int tick, double seconds) override;
    void playbackStateChanged(bool isPlaying) override;

    // Scale & Key settings
    void setScale(const juce::String& rootNote, const juce::String& scaleName);
    bool isPitchInScale(int pitch) const;
    bool isPitchRoot(int pitch) const;

    std::vector<PianoRollNote>& getNotes() { return notes; }

private:
    AudioMidiEngine& engine;
    std::vector<PianoRollNote> notes;

    // Dimensions
    static constexpr int kKeyboardWidth = 70;
    static constexpr int kVelocityHeight = 65;
    static constexpr int kRulerHeight = 26;
    static constexpr int kKeyHeight = 16;
    static constexpr int kLowestPitch = 24;  // C1
    static constexpr int kHighestPitch = 96; // C7
    static constexpr int kNumPitches = kHighestPitch - kLowestPitch + 1;
    static constexpr int kTicksPerQuarter = 480;

    float pixelsPerBar = 160.0f;
    int64_t currentTick = 0;

    // Scale highlight data
    int scaleRootPitch = 0; // 0 = C
    std::vector<int> scaleIntervals; // semitone offsets

    // Interactive state
    int auditionPitch = -1;
    int hoveredPitch = -1;
    int selectedNoteIdx = -1;
    bool isDraggingPlayhead = false;
    bool isDraggingNoteMove = false;
    bool isDraggingNoteResize = false;
    bool isDraggingVelocity = false;
    int64_t dragStartTick = 0;
    int dragPitchOffset = 0;

    int64_t xToTick(float x) const;
    float tickToX(int64_t tick) const;
    int yToPitch(float y) const;
    float pitchToY(int pitch) const;

    void drawRuler(juce::Graphics& g, juce::Rectangle<int> area);
    void drawKeyboard(juce::Graphics& g, juce::Rectangle<int> area);
    void drawGrid(juce::Graphics& g, juce::Rectangle<int> area);
    void drawNotes(juce::Graphics& g, juce::Rectangle<int> area);
    void drawVelocityLane(juce::Graphics& g, juce::Rectangle<int> area);
    void drawPlayhead(juce::Graphics& g, juce::Rectangle<int> area);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoRollComponent)
};
