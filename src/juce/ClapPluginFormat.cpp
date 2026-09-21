//*****************************************************************************
// The JAZZ++ Midi Sequencer — JUCE 8 Edition
//
// Copyright (C) 2026 Raja Zahidi
// CLAP (CLever Audio Plugin) Format Implementation for JUCE 8 Hosting
//*****************************************************************************

#include "ClapPluginFormat.h"

#include <clap/ext/params.h>
#include <clap/ext/state.h>
#include <clap/ext/gui.h>
#include <clap/ext/audio-ports.h>
#include <clap/ext/note-ports.h>

#include <cstring>
#include <algorithm>

// ============================================================================
// CLAP Hosted Parameter Implementation
// ============================================================================
class ClapHostedParameter : public juce::HostedAudioProcessorParameter
{
public:
    ClapHostedParameter(const clap_plugin_t* plugin,
                        const clap_plugin_params_t* paramsExt,
                        const clap_param_info_t& info)
        : clapPlugin(plugin),
          extParams(paramsExt),
          paramInfo(info)
    {
    }

    juce::String getParameterID() const override
    {
        return juce::String(paramInfo.id);
    }

    float getValue() const override
    {
        double val = 0.0;
        if (extParams && clapPlugin && extParams->get_value && extParams->get_value(clapPlugin, paramInfo.id, &val))
        {
            if (paramInfo.max_value > paramInfo.min_value)
                return static_cast<float>((val - paramInfo.min_value) / (paramInfo.max_value - paramInfo.min_value));
        }
        if (paramInfo.max_value > paramInfo.min_value)
            return static_cast<float>((paramInfo.default_value - paramInfo.min_value) / (paramInfo.max_value - paramInfo.min_value));
        return 0.0f;
    }

    void setValue(float newValue) override
    {
        float clamped = juce::jlimit(0.0f, 1.0f, newValue);
        double actualVal = paramInfo.min_value + clamped * (paramInfo.max_value - paramInfo.min_value);
        juce::ignoreUnused(actualVal);
    }

    float getDefaultValue() const override
    {
        if (paramInfo.max_value > paramInfo.min_value)
            return static_cast<float>((paramInfo.default_value - paramInfo.min_value) / (paramInfo.max_value - paramInfo.min_value));
        return 0.0f;
    }

    juce::String getName(int maximumStringLength) const override
    {
        juce::String name(paramInfo.name);
        return name.substring(0, maximumStringLength);
    }

    juce::String getLabel() const override
    {
        return {};
    }

    juce::String getText(float value, int maximumStringLength) const override
    {
        if (extParams && clapPlugin && extParams->value_to_text)
        {
            double actualVal = paramInfo.min_value + value * (paramInfo.max_value - paramInfo.min_value);
            char buf[128] = { 0 };
            if (extParams->value_to_text(clapPlugin, paramInfo.id, actualVal, buf, sizeof(buf)))
                return juce::String::fromUTF8(buf).substring(0, maximumStringLength);
        }
        return juce::AudioProcessorParameter::getText(value, maximumStringLength);
    }

    float getValueForText(const juce::String& text) const override
    {
        if (extParams && clapPlugin && extParams->text_to_value)
        {
            double actualVal = 0.0;
            if (extParams->text_to_value(clapPlugin, paramInfo.id, text.toRawUTF8(), &actualVal))
            {
                if (paramInfo.max_value > paramInfo.min_value)
                    return static_cast<float>((actualVal - paramInfo.min_value) / (paramInfo.max_value - paramInfo.min_value));
            }
        }
        return text.retainCharacters("-0123456789.").getFloatValue();
    }

private:
    const clap_plugin_t* clapPlugin;
    const clap_plugin_params_t* extParams;
    clap_param_info_t paramInfo;
};

// ============================================================================
// ClapPluginInstance Implementation
// ============================================================================
ClapPluginInstance::ClapPluginInstance(const juce::PluginDescription& desc,
                                       std::shared_ptr<juce::DynamicLibrary> lib,
                                       const clap_plugin_t* plugin)
    : AudioPluginInstance(BusesProperties()
                            .withInput("Input", juce::AudioChannelSet::stereo(), true)
                            .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      description(desc),
      library(lib),
      clapPlugin(plugin)
{
    initHostCallbacks();
    queryExtensions();
}

ClapPluginInstance::~ClapPluginInstance()
{
    if (clapPlugin != nullptr)
    {
        clapPlugin->destroy(clapPlugin);
        clapPlugin = nullptr;
    }
}

void ClapPluginInstance::initHostCallbacks()
{
    clapHost.clap_version = CLAP_VERSION;
    clapHost.host_data = this;
    clapHost.name = "Jazz++ JUCE";
    clapHost.vendor = "Raja Zahidi";
    clapHost.url = "https://github.com/rajazahidi/jazzplusplus";
    clapHost.version = "6.0.0";

    clapHost.get_extension = [](const clap_host_t* /*host*/, const char* /*id*/) -> const void* {
        return nullptr;
    };

    clapHost.request_restart = [](const clap_host_t* /*host*/) {};
    clapHost.request_process = [](const clap_host_t* /*host*/) {};
    clapHost.request_callback = [](const clap_host_t* /*host*/) {};
}

void ClapPluginInstance::queryExtensions()
{
    if (clapPlugin == nullptr)
        return;

    extParams = static_cast<const clap_plugin_params_t*>(clapPlugin->get_extension(clapPlugin, CLAP_EXT_PARAMS));
    extState  = static_cast<const clap_plugin_state_t*>(clapPlugin->get_extension(clapPlugin, CLAP_EXT_STATE));
    extGui    = static_cast<const clap_plugin_gui_t*>(clapPlugin->get_extension(clapPlugin, CLAP_EXT_GUI));

    // Enumerate parameters
    if (extParams != nullptr && extParams->count != nullptr && extParams->get_info != nullptr)
    {
        uint32_t numParams = extParams->count(clapPlugin);
        for (uint32_t i = 0; i < numParams; ++i)
        {
            clap_param_info_t info {};
            if (extParams->get_info(clapPlugin, i, &info))
            {
                addHostedParameter(std::make_unique<ClapHostedParameter>(clapPlugin, extParams, info));
            }
        }
    }
}

void ClapPluginInstance::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    if (clapPlugin != nullptr && clapPlugin->activate != nullptr)
    {
        clapPlugin->activate(clapPlugin, sampleRate, 32, static_cast<uint32_t>(samplesPerBlock));
    }
}

void ClapPluginInstance::releaseResources()
{
    if (clapPlugin != nullptr && clapPlugin->deactivate != nullptr)
    {
        clapPlugin->deactivate(clapPlugin);
    }
}

void ClapPluginInstance::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    if (clapPlugin == nullptr || clapPlugin->process == nullptr)
        return;

    const int numSamples = buffer.getNumSamples();
    const int numInChannels = getTotalNumInputChannels();
    const int numOutChannels = getTotalNumOutputChannels();

    float* inChannelPtrs[2] = { nullptr, nullptr };
    float* outChannelPtrs[2] = { nullptr, nullptr };

    for (int ch = 0; ch < std::min(2, numInChannels); ++ch)
        inChannelPtrs[ch] = buffer.getWritePointer(ch);

    for (int ch = 0; ch < std::min(2, numOutChannels); ++ch)
        outChannelPtrs[ch] = buffer.getWritePointer(ch);

    clap_audio_buffer_t inAudio {};
    inAudio.data32 = inChannelPtrs;
    inAudio.data64 = nullptr;
    inAudio.channel_count = static_cast<uint32_t>(std::min(2, numInChannels));
    inAudio.latency = 0;
    inAudio.constant_mask = 0;

    clap_audio_buffer_t outAudio {};
    outAudio.data32 = outChannelPtrs;
    outAudio.data64 = nullptr;
    outAudio.channel_count = static_cast<uint32_t>(std::min(2, numOutChannels));
    outAudio.latency = 0;
    outAudio.constant_mask = 0;

    // Convert JUCE MIDI messages to CLAP note events
    std::vector<clap_event_note_t> noteEvents;
    for (const auto meta : midiMessages)
    {
        auto msg = meta.getMessage();
        if (msg.isNoteOn() || msg.isNoteOff())
        {
            clap_event_note_t evt {};
            evt.header.size = sizeof(clap_event_note_t);
            evt.header.time = static_cast<uint32_t>(meta.samplePosition);
            evt.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
            evt.header.type = msg.isNoteOn() ? CLAP_EVENT_NOTE_ON : CLAP_EVENT_NOTE_OFF;
            evt.header.flags = 0;
            evt.key = static_cast<int16_t>(msg.getNoteNumber());
            evt.channel = static_cast<int16_t>(msg.getChannel() - 1);
            evt.velocity = msg.getFloatVelocity();
            evt.port_index = 0;
            evt.note_id = -1;
            noteEvents.push_back(evt);
        }
    }

    struct EventListContext
    {
        const std::vector<clap_event_note_t>* events;
    } evCtx { &noteEvents };

    clap_input_events_t inEvents {};
    inEvents.ctx = &evCtx;
    inEvents.size = [](const clap_input_events_t* list) -> uint32_t {
        auto* c = static_cast<EventListContext*>(list->ctx);
        return static_cast<uint32_t>(c->events->size());
    };
    inEvents.get = [](const clap_input_events_t* list, uint32_t index) -> const clap_event_header_t* {
        auto* c = static_cast<EventListContext*>(list->ctx);
        if (index < c->events->size())
            return &((*c->events)[index].header);
        return nullptr;
    };

    clap_output_events_t outEvents {};
    outEvents.ctx = nullptr;
    outEvents.try_push = [](const clap_output_events_t* /*list*/, const clap_event_header_t* /*event*/) -> bool {
        return true;
    };

    clap_process_t process {};
    process.steady_time = -1;
    process.frames_count = static_cast<uint32_t>(numSamples);
    process.transport = nullptr;
    process.audio_inputs = (numInChannels > 0) ? &inAudio : nullptr;
    process.audio_inputs_count = (numInChannels > 0) ? 1 : 0;
    process.audio_outputs = &outAudio;
    process.audio_outputs_count = 1;
    process.in_events = &inEvents;
    process.out_events = &outEvents;

    clapPlugin->process(clapPlugin, &process);
}

bool ClapPluginInstance::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* ClapPluginInstance::createEditor()
{
    return new juce::GenericAudioProcessorEditor(*this);
}

void ClapPluginInstance::getStateInformation(juce::MemoryBlock& destData)
{
    if (extState == nullptr || extState->save == nullptr || clapPlugin == nullptr)
        return;

    struct StreamContext
    {
        juce::MemoryOutputStream stream;
    } sCtx;

    clap_ostream_t ostream {};
    ostream.ctx = &sCtx;
    ostream.write = [](const clap_ostream_t* stream, const void* buffer, uint64_t size) -> int64_t {
        auto* c = static_cast<StreamContext*>(stream->ctx);
        return c->stream.write(buffer, static_cast<size_t>(size)) ? static_cast<int64_t>(size) : -1;
    };

    if (extState->save(clapPlugin, &ostream))
    {
        destData.append(sCtx.stream.getData(), sCtx.stream.getDataSize());
    }
}

void ClapPluginInstance::setStateInformation(const void* data, int sizeInBytes)
{
    if (extState == nullptr || extState->load == nullptr || clapPlugin == nullptr || data == nullptr || sizeInBytes <= 0)
        return;

    struct StreamContext
    {
        const char* ptr;
        size_t bytesLeft;
    } sCtx { static_cast<const char*>(data), static_cast<size_t>(sizeInBytes) };

    clap_istream_t istream {};
    istream.ctx = &sCtx;
    istream.read = [](const clap_istream_t* stream, void* buffer, uint64_t size) -> int64_t {
        auto* c = static_cast<StreamContext*>(stream->ctx);
        size_t toRead = std::min(static_cast<size_t>(size), c->bytesLeft);
        if (toRead > 0)
        {
            std::memcpy(buffer, c->ptr, toRead);
            c->ptr += toRead;
            c->bytesLeft -= toRead;
            return static_cast<int64_t>(toRead);
        }
        return 0;
    };

    extState->load(clapPlugin, &istream);
}

// ============================================================================
// ClapPluginFormat Implementation
// ============================================================================
ClapPluginFormat::ClapPluginFormat()
{
}

bool ClapPluginFormat::fileMightContainThisPluginType(const juce::String& fileOrIdentifier)
{
    return fileOrIdentifier.endsWithIgnoreCase(".clap");
}

juce::String ClapPluginFormat::getNameOfPluginFromIdentifier(const juce::String& fileOrIdentifier)
{
    return juce::File(fileOrIdentifier).getFileNameWithoutExtension();
}

bool ClapPluginFormat::pluginNeedsRescanning(const juce::PluginDescription& desc)
{
    juce::File f(desc.fileOrIdentifier);
    return !f.exists() || f.getLastModificationTime() != desc.lastFileModTime;
}

bool ClapPluginFormat::doesPluginStillExist(const juce::PluginDescription& desc)
{
    return juce::File(desc.fileOrIdentifier).exists();
}

juce::FileSearchPath ClapPluginFormat::getDefaultLocationsToSearch()
{
    juce::FileSearchPath path;
#if JUCE_LINUX
    path.add(juce::File("/usr/lib/clap"));
    path.add(juce::File("/usr/local/lib/clap"));
    path.add(juce::File::getSpecialLocation(juce::File::userHomeDirectory).getChildFile(".clap"));
#elif JUCE_MAC
    path.add(juce::File("/Library/Audio/Plug-Ins/CLAP"));
    path.add(juce::File::getSpecialLocation(juce::File::userHomeDirectory).getChildFile("Library/Audio/Plug-Ins/CLAP"));
#elif JUCE_WINDOWS
    path.add(juce::File("C:\\Program Files\\Common Files\\CLAP"));
#endif
    return path;
}

juce::StringArray ClapPluginFormat::searchPathsForPlugins(const juce::FileSearchPath& directoriesToSearch,
                                                         bool recursive,
                                                         bool /*allowPluginsWhichRequireAsynchronousInstantiation*/)
{
    juce::StringArray results;

    for (int i = 0; i < directoriesToSearch.getNumPaths(); ++i)
    {
        auto dir = directoriesToSearch[i];
        if (dir.isDirectory())
        {
            juce::RangedDirectoryIterator iter(dir, recursive, "*.clap");
            for (const auto& entry : iter)
            {
                results.add(entry.getFile().getFullPathName());
            }
        }
    }

    return results;
}

void ClapPluginFormat::findAllTypesForFile(juce::OwnedArray<juce::PluginDescription>& results,
                                          const juce::String& fileOrIdentifier)
{
    juce::File file(fileOrIdentifier);
    if (!file.exists())
        return;

    juce::DynamicLibrary lib;
    if (!lib.open(fileOrIdentifier))
        return;

    auto* entry = reinterpret_cast<const clap_plugin_entry_t*>(lib.getFunction("clap_entry"));
    if (entry == nullptr || entry->init == nullptr || entry->get_factory == nullptr)
        return;

    if (!entry->init(fileOrIdentifier.toRawUTF8()))
        return;

    auto* factory = static_cast<const clap_plugin_factory_t*>(entry->get_factory(CLAP_PLUGIN_FACTORY_ID));
    if (factory != nullptr && factory->get_plugin_count != nullptr && factory->get_plugin_descriptor != nullptr)
    {
        uint32_t count = factory->get_plugin_count(factory);
        for (uint32_t i = 0; i < count; ++i)
        {
            const clap_plugin_descriptor_t* desc = factory->get_plugin_descriptor(factory, i);
            if (desc == nullptr)
                continue;

            auto* pd = new juce::PluginDescription();
            pd->name = desc->name ? desc->name : file.getFileNameWithoutExtension();
            pd->descriptiveName = desc->description ? desc->description : pd->name;
            pd->pluginFormatName = "CLAP";
            pd->manufacturerName = desc->vendor ? desc->vendor : "Unknown";
            pd->version = desc->version ? desc->version : "1.0.0";
            pd->fileOrIdentifier = fileOrIdentifier;
            pd->lastFileModTime = file.getLastModificationTime();
            pd->uniqueId = static_cast<int>(juce::String(desc->id ? desc->id : desc->name).hashCode());

            pd->isInstrument = false;
            pd->category = "Audio Effect";

            if (desc->features != nullptr)
            {
                for (const char* const* f = desc->features; *f != nullptr; ++f)
                {
                    juce::String feat(*f);
                    if (feat.equalsIgnoreCase(CLAP_PLUGIN_FEATURE_INSTRUMENT) ||
                        feat.equalsIgnoreCase(CLAP_PLUGIN_FEATURE_SYNTHESIZER))
                    {
                        pd->isInstrument = true;
                        pd->category = "Instrument / Synth";
                        break;
                    }
                }
            }

            results.add(pd);
        }
    }

    if (entry->deinit != nullptr)
        entry->deinit();
}

void ClapPluginFormat::createPluginInstance(const juce::PluginDescription& desc,
                                            double initialSampleRate,
                                            int initialBufferSize,
                                            PluginCreationCallback callback)
{
    auto lib = std::make_shared<juce::DynamicLibrary>();
    if (!lib->open(desc.fileOrIdentifier))
    {
        callback(nullptr, "Failed to load shared library: " + desc.fileOrIdentifier);
        return;
    }

    auto* entry = reinterpret_cast<const clap_plugin_entry_t*>(lib->getFunction("clap_entry"));
    if (entry == nullptr || entry->init == nullptr || entry->get_factory == nullptr)
    {
        callback(nullptr, "Missing clap_entry symbol in " + desc.fileOrIdentifier);
        return;
    }

    if (!entry->init(desc.fileOrIdentifier.toRawUTF8()))
    {
        callback(nullptr, "clap_entry->init failed for " + desc.fileOrIdentifier);
        return;
    }

    auto* factory = static_cast<const clap_plugin_factory_t*>(entry->get_factory(CLAP_PLUGIN_FACTORY_ID));
    if (factory == nullptr || factory->create_plugin == nullptr)
    {
        callback(nullptr, "Failed to obtain CLAP_PLUGIN_FACTORY_ID");
        return;
    }

    // Find plugin ID matching description
    const char* pluginId = nullptr;
    uint32_t count = factory->get_plugin_count(factory);
    for (uint32_t i = 0; i < count; ++i)
    {
        const auto* d = factory->get_plugin_descriptor(factory, i);
        if (d != nullptr && (juce::String(d->name) == desc.name ||
                             juce::String(d->id).hashCode() == desc.uniqueId))
        {
            pluginId = d->id;
            break;
        }
    }

    if (pluginId == nullptr && count > 0)
    {
        const auto* d = factory->get_plugin_descriptor(factory, 0);
        if (d != nullptr) pluginId = d->id;
    }

    if (pluginId == nullptr)
    {
        callback(nullptr, "Could not find matching plugin in CLAP factory");
        return;
    }

    clap_host_t dummyHost {};
    dummyHost.clap_version = CLAP_VERSION;
    dummyHost.name = "Jazz++ JUCE";
    dummyHost.vendor = "Raja Zahidi";
    dummyHost.url = "https://github.com/rajazahidi/jazzplusplus";
    dummyHost.version = "6.0.0";
    dummyHost.get_extension = [](const clap_host_t*, const char*) -> const void* { return nullptr; };
    dummyHost.request_restart = [](const clap_host_t*) {};
    dummyHost.request_process = [](const clap_host_t*) {};
    dummyHost.request_callback = [](const clap_host_t*) {};

    const clap_plugin_t* clapPlugin = factory->create_plugin(factory, &dummyHost, pluginId);
    if (clapPlugin == nullptr || clapPlugin->init == nullptr)
    {
        callback(nullptr, "CLAP factory->create_plugin failed");
        return;
    }

    if (!clapPlugin->init(clapPlugin))
    {
        clapPlugin->destroy(clapPlugin);
        callback(nullptr, "CLAP plugin->init failed");
        return;
    }

    auto instance = std::make_unique<ClapPluginInstance>(desc, lib, clapPlugin);
    instance->prepareToPlay(initialSampleRate, initialBufferSize);

    callback(std::move(instance), {});
}
