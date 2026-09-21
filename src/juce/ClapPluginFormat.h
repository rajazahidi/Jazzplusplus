//*****************************************************************************
// The JAZZ++ Midi Sequencer — JUCE 8 Edition
//
// Copyright (C) 2026 Raja Zahidi
// CLAP (CLever Audio Plugin) Format Support for JUCE 8 Hosting
//*****************************************************************************

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <clap/clap.h>
#include <memory>
#include <vector>

class ClapPluginInstance : public juce::AudioPluginInstance
{
public:
    ClapPluginInstance(const juce::PluginDescription& desc,
                       std::shared_ptr<juce::DynamicLibrary> lib,
                       const clap_plugin_t* plugin);
    ~ClapPluginInstance() override;

    void fillInPluginDescription(juce::PluginDescription& d) const override { d = description; }
    const juce::String getName() const override { return description.name; }

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    double getTailLengthSeconds() const override { return 0.0; }
    bool acceptsMidi() const override { return description.isInstrument || acceptsMidiFlag; }
    bool producesMidi() const override { return producesMidiFlag; }

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int /*index*/) override {}
    const juce::String getProgramName(int /*index*/) override { return "Default"; }
    void changeProgramName(int /*index*/, const juce::String& /*newName*/) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    const clap_plugin_t* getClapPlugin() const { return clapPlugin; }

private:
    juce::PluginDescription description;
    std::shared_ptr<juce::DynamicLibrary> library;
    const clap_plugin_t* clapPlugin = nullptr;
    clap_host_t clapHost {};

    bool acceptsMidiFlag = true;
    bool producesMidiFlag = false;

    // CLAP Extensions
    const clap_plugin_params_t* extParams = nullptr;
    const clap_plugin_state_t* extState = nullptr;
    const clap_plugin_gui_t* extGui = nullptr;

    void initHostCallbacks();
    void queryExtensions();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClapPluginInstance)
};

class ClapPluginFormat : public juce::AudioPluginFormat
{
public:
    ClapPluginFormat();
    ~ClapPluginFormat() override = default;

    juce::String getName() const override { return "CLAP"; }

    void findAllTypesForFile(juce::OwnedArray<juce::PluginDescription>& results,
                             const juce::String& fileOrIdentifier) override;

    bool fileMightContainThisPluginType(const juce::String& fileOrIdentifier) override;
    juce::String getNameOfPluginFromIdentifier(const juce::String& fileOrIdentifier) override;
    bool pluginNeedsRescanning(const juce::PluginDescription& desc) override;
    bool doesPluginStillExist(const juce::PluginDescription& desc) override;

    bool canScanForPlugins() const override { return true; }
    bool isTrivialToScan() const override { return false; }

    juce::StringArray searchPathsForPlugins(const juce::FileSearchPath& directoriesToSearch,
                                            bool recursive,
                                            bool allowPluginsWhichRequireAsynchronousInstantiation = false) override;

    juce::FileSearchPath getDefaultLocationsToSearch() override;

    bool requiresUnblockedMessageThreadDuringCreation(const juce::PluginDescription&) const override { return false; }

protected:
    void createPluginInstance(const juce::PluginDescription& desc,
                              double initialSampleRate,
                              int initialBufferSize,
                              PluginCreationCallback callback) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClapPluginFormat)
};
