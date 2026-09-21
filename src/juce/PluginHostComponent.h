//*****************************************************************************
// The JAZZ++ Midi Sequencer — JUCE 8 Edition
//
// Copyright (C) 2026 Raja Zahidi
// VST3 & CLAP Plugin Host Browser, Scanner, and Active Slot Rack
//*****************************************************************************

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "AudioMidiEngine.h"
#include "JazzLookAndFeel.h"

class PluginHostComponent : public juce::Component,
                            public juce::TableListBoxModel
{
public:
    PluginHostComponent(AudioMidiEngine& engine);
    ~PluginHostComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // TableListBoxModel overrides
    int getNumRows() override;
    void paintRowBackground(juce::Graphics& g, int rowNumber, int width, int height, bool rowIsSelected) override;
    void paintCell(juce::Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected) override;
    void cellDoubleClicked(int rowNumber, int columnId, const juce::MouseEvent&) override;

    void scanPlugins();
    void loadPluginFromFile();
    void loadSelectedPlugin(bool asInstrument);
    void openPluginEditor(bool forInstrument);

private:
    AudioMidiEngine& engine;

    // Filter controls
    juce::TextButton btnFilterAll { "ALL" };
    juce::TextButton btnFilterVst3 { "VST3" };
    juce::TextButton btnFilterClap { "CLAP" };
    juce::TextEditor txtSearch;
    juce::TextButton btnScan { "SCAN FOLDERS" };
    juce::TextButton btnLoadFile { "LOAD FILE..." };

    // Plugin list table
    juce::TableListBox tablePlugins;
    std::vector<juce::PluginDescription> filteredPlugins;

    // Active slots controls
    juce::GroupComponent grpInstrument { {}, "ACTIVE INSTRUMENT (VST3 / CLAP)" };
    juce::Label lblInstName;
    juce::ToggleButton btnInstBypass { "BYPASS" };
    juce::TextButton btnInstOpenUI { "OPEN UI" };
    juce::TextButton btnInstUnload { "UNLOAD" };

    juce::GroupComponent grpEffect { {}, "ACTIVE MASTER EFFECT (VST3 / CLAP)" };
    juce::Label lblFxName;
    juce::ToggleButton btnFxBypass { "BYPASS" };
    juce::TextButton btnFxOpenUI { "OPEN UI" };
    juce::TextButton btnFxUnload { "UNLOAD" };

    juce::TextButton btnLoadAsInst { "LOAD AS INSTRUMENT" };
    juce::TextButton btnLoadAsFx { "LOAD AS EFFECT" };

    juce::Label lblStatus;

    enum FilterFormat { eFilterAll = 0, eFilterVst3, eFilterClap } activeFilter = eFilterAll;

    std::unique_ptr<juce::DocumentWindow> instWindow;
    std::unique_ptr<juce::DocumentWindow> fxWindow;

    void updateFilteredList();
    void updateSlotDisplays();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginHostComponent)
};
