//*****************************************************************************
// The JAZZ++ Midi Sequencer — JUCE 8 Edition
//
// Copyright (C) 2026 Raja Zahidi
// Arranger Multi-Track Timeline & Clip Arranger
//*****************************************************************************

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "AudioMidiEngine.h"
#include "JazzLookAndFeel.h"

#include <vector>
#include <functional>

struct ArrangerClip
{
    juce::String name;
    int64_t startTick = 0;
    int64_t lengthTicks = 480 * 4; // 1 bar default (480 PPQN * 4 beats)
    juce::Colour color { JazzLookAndFeel::Colors::neonCyan };
    std::vector<std::pair<int, int>> miniNotes; // pitch, tickOffset for preview
};

struct ArrangerTrack
{
    juce::String name;
    juce::Colour color;
    bool isMuted = false;
    bool isSolo = false;
    bool isArmed = false;
    float volume = 0.8f;
    float pan = 0.5f;
    int midiChannel = 1;
    std::vector<ArrangerClip> clips;
};

class ArrangerComponent : public juce::Component,
                          public AudioMidiEngine::Listener
{
public:
    ArrangerComponent(AudioMidiEngine& engine);
    ~ArrangerComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;

    // Callbacks
    std::function<void(int trackIdx, int clipIdx)> onClipDoubleClicked;

    // AudioMidiEngine::Listener overrides
    void playheadMoved(int bar, int beat, int tick, double seconds) override;
    void playbackStateChanged(bool isPlaying) override;

    std::vector<ArrangerTrack>& getTracks() { return tracks; }
    void addTrack(const juce::String& name, juce::Colour color, int channel);

private:
    AudioMidiEngine& engine;
    std::vector<ArrangerTrack> tracks;

    // Layout constants
    static constexpr int kHeaderWidth = 200;
    static constexpr int kRulerHeight = 32;
    static constexpr int kTrackHeight = 56;
    static constexpr int kTicksPerQuarter = 480;

    float pixelsPerBar = 120.0f; // Zoom
    int64_t currentTick = 0;

    int selectedTrackIdx = -1;
    int selectedClipIdx = -1;
    bool isDraggingPlayhead = false;
    bool isDraggingClip = false;
    int64_t dragClipOffsetTick = 0;

    int64_t xToTick(float x) const;
    float tickToX(int64_t tick) const;

    void drawRuler(juce::Graphics& g, juce::Rectangle<int> area);
    void drawTrackHeaders(juce::Graphics& g, juce::Rectangle<int> area);
    void drawLanes(juce::Graphics& g, juce::Rectangle<int> area);
    void drawPlayhead(juce::Graphics& g, juce::Rectangle<int> area);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArrangerComponent)
};
