//*****************************************************************************
// The JAZZ++ Midi Sequencer — JUCE 8 Edition
//
// Copyright (C) 2026 Raja Zahidi
// Audio & MIDI Device Engine Implementation
//*****************************************************************************

#include "AudioMidiEngine.h"
#include <cmath>
#include <algorithm>

AudioMidiEngine::AudioMidiEngine()
{
    voicePool.resize(kMaxVoices);

    // Default EQ: gentle low warm boost, flat mids, slight high sparkle
    eqSettings.bassGainDB = 2.0f;
    eqSettings.midGainDB = 0.0f;
    eqSettings.trebleGainDB = 1.5f;
    equalizer.SetSettings(eqSettings);

    // Default Reverb: subtle studio room
    reverbSettings.roomSize = 0.45f;
    reverbSettings.damping = 0.35f;
    reverbSettings.width = 1.0f;
    reverbSettings.wet = 0.25f;
    reverbSettings.dry = 0.85f;
    reverb.SetSettings(reverbSettings);

    // Default Delay: 250ms left, 350ms right
    delaySettings.delayTimeMsL = 250.0f;
    delaySettings.delayTimeMsR = 350.0f;
    delaySettings.feedback = 0.35f;
    delaySettings.damp = 0.2f;
    delaySettings.wet = 0.3f;
    delaySettings.dry = 0.8f;
    delay.SetSettings(delaySettings);

    // Default Chorus
    chorusSettings.rateHz = 1.2f;
    chorusSettings.depthMs = 4.0f;
    chorusSettings.feedback = 0.2f;
    chorusSettings.mix = 0.3f;
    chorus.SetSettings(chorusSettings);

    // Default Limiter
    limiterSettings.drive = 1.0f;
    limiterSettings.ceilingDB = -0.1f;
    limiterSettings.enableOverdrive = false;
    limiter.SetSettings(limiterSettings);
}

AudioMidiEngine::~AudioMidiEngine()
{
    shutdown();
}

juce::Result AudioMidiEngine::initialize(int numInputChannels, int numOutputChannels)
{
    auto error = deviceManager.initialiseWithDefaultDevices(numInputChannels, numOutputChannels);
    if (error.isNotEmpty())
    {
        // Try fallback to just 2 output channels
        error = deviceManager.initialise(0, 2, nullptr, true);
        if (error.isNotEmpty())
            return juce::Result::fail("Audio initialization failed: " + error);
    }

    deviceManager.addAudioCallback(this);

    // Automatically select the best MIDI output available (e.g. FluidSynth or Midi Through)
    auto midiOutputs = juce::MidiOutput::getAvailableDevices();
    juce::String selectedDeviceId;

    for (const auto& dev : midiOutputs)
    {
        if (dev.name.containsIgnoreCase("FLUID") || dev.name.containsIgnoreCase("Synth"))
        {
            selectedDeviceId = dev.identifier;
            break;
        }
    }

    if (selectedDeviceId.isEmpty() && !midiOutputs.isEmpty())
    {
        for (const auto& dev : midiOutputs)
        {
            if (dev.name.containsIgnoreCase("Midi Through"))
            {
                selectedDeviceId = dev.identifier;
                break;
            }
        }
    }

    if (selectedDeviceId.isEmpty() && !midiOutputs.isEmpty())
    {
        selectedDeviceId = midiOutputs[0].identifier;
    }

    if (selectedDeviceId.isNotEmpty())
    {
        setMidiOutputDevice(selectedDeviceId);
    }

    // Initialize VST3 & CLAP plugin hosting
    pluginFormatManager.addDefaultFormats();
    pluginFormatManager.addFormat(new ClapPluginFormat());

    auto cacheFile = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                         .getChildFile("jazz").getChildFile("plugins.xml");
    if (cacheFile.existsAsFile())
    {
        if (auto xml = juce::parseXML(cacheFile))
            knownPluginList.recreateFromXml(*xml);
    }

    return juce::Result::ok();
}

void AudioMidiEngine::shutdown()
{
    stopPlayback();
    deviceManager.removeAudioCallback(this);
    deviceManager.closeAudioDevice();
    midiOutput.reset();
}

void AudioMidiEngine::startPlayback()
{
    playing.store(true);
    listeners.call(&Listener::playbackStateChanged, true);
}

void AudioMidiEngine::pausePlayback()
{
    playing.store(false);
    allNotesOff();
    listeners.call(&Listener::playbackStateChanged, false);
}

void AudioMidiEngine::stopPlayback()
{
    playing.store(false);
    playheadClock.store(0);
    lastStep16th = -1;
    allNotesOff();
    listeners.call(&Listener::playbackStateChanged, false);

    int bar = 1, beat = 1, tick = 0;
    double sec = 0.0;
    getPlayheadPosition(bar, beat, tick, sec);
    listeners.call(&Listener::playheadMoved, bar, beat, tick, sec);
}

void AudioMidiEngine::setTempo(double bpm)
{
    bpm = juce::jlimit(20.0, 300.0, bpm);
    tempoBpm.store(bpm);
    listeners.call(&Listener::tempoChanged, bpm);
}

void AudioMidiEngine::setTimeSignature(int num, int den)
{
    timeSigNumerator = juce::jlimit(1, 32, num);
    timeSigDenominator = juce::jlimit(1, 32, den);
}

void AudioMidiEngine::setPlayheadClock(int64_t tick)
{
    playheadClock.store(std::max<int64_t>(0, tick));
    int bar = 1, beat = 1, t = 0;
    double sec = 0.0;
    getPlayheadPosition(bar, beat, t, sec);
    listeners.call(&Listener::playheadMoved, bar, beat, t, sec);
}

void AudioMidiEngine::setLoopRange(int64_t startTick, int64_t endTick)
{
    loopStartTick.store(std::max<int64_t>(0, startTick));
    loopEndTick.store(std::max<int64_t>(startTick + 1, endTick));
}

void AudioMidiEngine::getPlayheadPosition(int& bar, int& beat, int& tick, double& seconds) const
{
    int64_t curClock = playheadClock.load();
    int ticksPerBeat = ticksPerQuarterNote * 4 / timeSigDenominator;
    int ticksPerBar = ticksPerBeat * timeSigNumerator;

    bar = static_cast<int>(curClock / ticksPerBar) + 1;
    int rem = static_cast<int>(curClock % ticksPerBar);
    beat = (rem / ticksPerBeat) + 1;
    tick = rem % ticksPerBeat;

    double bpm = tempoBpm.load();
    double beats = static_cast<double>(curClock) / static_cast<double>(ticksPerQuarterNote);
    seconds = (beats / bpm) * 60.0;
}

void AudioMidiEngine::setMidiOutputDevice(const juce::String& deviceId)
{
    midiOutput = juce::MidiOutput::openDevice(deviceId);
}

juce::String AudioMidiEngine::getMidiOutputDeviceName() const
{
    if (midiOutput != nullptr)
        return midiOutput->getName();
    return "None";
}

void AudioMidiEngine::sendNoteOn(int channel, int noteNumber, float velocity)
{
    auto msg = juce::MidiMessage::noteOn(juce::jlimit(1, 16, channel),
                                         juce::jlimit(0, 127, noteNumber),
                                         static_cast<juce::uint8>(juce::jlimit(0.0f, 1.0f, velocity) * 127.0f));
    if (midiOutput != nullptr)
    {
        midiOutput->sendMessageNow(msg);
    }

    {
        std::lock_guard<std::mutex> lock(pluginMutex);
        pendingPluginMidi.addEvent(msg, 0);
    }
}

void AudioMidiEngine::sendNoteOff(int channel, int noteNumber, float velocity)
{
    auto msg = juce::MidiMessage::noteOff(juce::jlimit(1, 16, channel),
                                          juce::jlimit(0, 127, noteNumber),
                                          static_cast<juce::uint8>(juce::jlimit(0.0f, 1.0f, velocity) * 127.0f));
    if (midiOutput != nullptr)
    {
        midiOutput->sendMessageNow(msg);
    }

    {
        std::lock_guard<std::mutex> lock(pluginMutex);
        pendingPluginMidi.addEvent(msg, 0);
    }
}

void AudioMidiEngine::allNotesOff(int channel)
{
    if (midiOutput != nullptr)
    {
        if (channel < 0)
        {
            for (int ch = 1; ch <= 16; ++ch)
                midiOutput->sendMessageNow(juce::MidiMessage::allNotesOff(ch));
        }
        else
        {
            midiOutput->sendMessageNow(juce::MidiMessage::allNotesOff(juce::jlimit(1, 16, channel)));
        }
    }
}

void AudioMidiEngine::triggerDrum(JZSynthDrumType type, float velocity, float pan)
{
    auto params = JZSynthDrumGenerator::GetPreset(type);
    params.volume = juce::jlimit(0.0f, 1.0f, velocity);
    auto pcm = JZSynthDrumGenerator::Generate(params, static_cast<int>(sampleRate));
    triggerSample(pcm, velocity, pan);
}

void AudioMidiEngine::triggerRetroSFX(JZRetroSFXType type, float velocity)
{
    auto params = JZRetroSFXGenerator::GetPreset(type);
    params.volume = juce::jlimit(0.0f, 1.0f, velocity);
    auto pcm = JZRetroSFXGenerator::Generate(params, static_cast<int>(sampleRate));
    triggerSample(pcm, velocity, 0.5f);
}

void AudioMidiEngine::triggerSample(const std::vector<short>& pcm, float velocity, float pan)
{
    if (pcm.empty())
        return;

    std::lock_guard<std::mutex> lock(voiceMutex);
    // Find free voice or steal oldest
    size_t targetIdx = 0;
    size_t maxPlayhead = 0;

    for (size_t i = 0; i < voicePool.size(); ++i)
    {
        if (!voicePool[i].active)
        {
            targetIdx = i;
            break;
        }
        if (voicePool[i].playhead > maxPlayhead)
        {
            maxPlayhead = voicePool[i].playhead;
            targetIdx = i;
        }
    }

    voicePool[targetIdx].pcmData = pcm;
    voicePool[targetIdx].playhead = 0;
    voicePool[targetIdx].volume = juce::jlimit(0.0f, 1.0f, velocity);
    voicePool[targetIdx].pan = juce::jlimit(0.0f, 1.0f, pan);
    voicePool[targetIdx].active = true;
}

void AudioMidiEngine::updateEqualizer()
{
    std::lock_guard<std::mutex> lock(dspMutex);
    equalizer.SetSettings(eqSettings);
}

void AudioMidiEngine::updateReverb()
{
    std::lock_guard<std::mutex> lock(dspMutex);
    reverb.SetSettings(reverbSettings);
}

void AudioMidiEngine::updateDelay()
{
    std::lock_guard<std::mutex> lock(dspMutex);
    delay.SetSettings(delaySettings);
}

void AudioMidiEngine::updateChorus()
{
    std::lock_guard<std::mutex> lock(dspMutex);
    chorus.SetSettings(chorusSettings);
}

void AudioMidiEngine::updateLimiter()
{
    std::lock_guard<std::mutex> lock(dspMutex);
    limiter.SetSettings(limiterSettings);
}

void AudioMidiEngine::setFxEnabled(bool eq, bool rev, bool del, bool cho, bool lim)
{
    std::lock_guard<std::mutex> lock(dspMutex);
    enableEq = eq;
    enableReverb = rev;
    enableDelay = del;
    enableChorus = cho;
    enableLimiter = lim;
}

void AudioMidiEngine::addEngineListener(Listener* listener)
{
    listeners.add(listener);
}

void AudioMidiEngine::removeEngineListener(Listener* listener)
{
    listeners.remove(listener);
}

void AudioMidiEngine::scanPluginDirectory(const juce::FileSearchPath& searchPath)
{
    juce::File deadMansPedalFile = juce::File::getSpecialLocation(juce::File::tempDirectory)
                                       .getChildFile("jazz_plugin_scan_crash.txt");
    for (int i = 0; i < pluginFormatManager.getNumFormats(); ++i)
    {
        if (auto* format = pluginFormatManager.getFormat(i))
        {
            juce::PluginDirectoryScanner scanner(knownPluginList,
                                                 *format,
                                                 searchPath,
                                                 true, // recursive
                                                 deadMansPedalFile);
            juce::String pluginBeingScanned;
            while (scanner.scanNextFile(true, pluginBeingScanned)) {}
        }
    }

    // Save updated cache
    auto cacheDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("jazz");
    cacheDir.createDirectory();
    if (auto xml = knownPluginList.createXml())
        xml->writeTo(cacheDir.getChildFile("plugins.xml"));
}

void AudioMidiEngine::loadPluginAsync(const juce::PluginDescription& desc, bool asInstrument,
                                     std::function<void(bool success, const juce::String& error)> onComplete)
{
    pluginFormatManager.createPluginInstanceAsync(
        desc,
        sampleRate,
        512,
        [this, asInstrument, onComplete](std::unique_ptr<juce::AudioPluginInstance> instance, const juce::String& error) {
            if (instance != nullptr)
            {
                instance->prepareToPlay(sampleRate, 512);
                std::lock_guard<std::mutex> lock(pluginMutex);
                if (asInstrument)
                    activeInstrumentPlugin = std::move(instance);
                else
                    activeEffectPlugin = std::move(instance);

                if (onComplete) onComplete(true, {});
            }
            else
            {
                if (onComplete) onComplete(false, error);
            }
        }
    );
}

void AudioMidiEngine::unloadPlugin(bool isInstrument)
{
    std::lock_guard<std::mutex> lock(pluginMutex);
    if (isInstrument)
        activeInstrumentPlugin.reset();
    else
        activeEffectPlugin.reset();
}

void AudioMidiEngine::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    if (device != nullptr)
    {
        sampleRate = device->getCurrentSampleRate();
        equalizer = JZ3BandEqualizer(static_cast<float>(sampleRate));
        equalizer.SetSettings(eqSettings);
        delay = JZStereoDelay(static_cast<float>(sampleRate));
        delay.SetSettings(delaySettings);
        chorus = JZStereoChorus(static_cast<float>(sampleRate));
        chorus.SetSettings(chorusSettings);
        limiter = JZAudioLimiterDistortion();
        limiter.SetSettings(limiterSettings);

        std::lock_guard<std::mutex> lock(pluginMutex);
        if (activeInstrumentPlugin != nullptr)
            activeInstrumentPlugin->prepareToPlay(sampleRate, 512);
        if (activeEffectPlugin != nullptr)
            activeEffectPlugin->prepareToPlay(sampleRate, 512);
    }
}

void AudioMidiEngine::audioDeviceStopped()
{
    allNotesOff();
    std::lock_guard<std::mutex> lock(pluginMutex);
    if (activeInstrumentPlugin != nullptr)
        activeInstrumentPlugin->releaseResources();
    if (activeEffectPlugin != nullptr)
        activeEffectPlugin->releaseResources();
}

void AudioMidiEngine::handleIncomingMidiMessage(juce::MidiInput* /*source*/, const juce::MidiMessage& message)
{
    std::lock_guard<std::mutex> lock(pluginMutex);
    pendingPluginMidi.addEvent(message, 0);
}

void AudioMidiEngine::advanceClock(int numSamples)
{
    double bpm = tempoBpm.load();
    double samplesPerBeat = (sampleRate * 60.0) / bpm;
    double ticksPerSample = static_cast<double>(ticksPerQuarterNote) / samplesPerBeat;

    int64_t addedTicks = static_cast<int64_t>(numSamples * ticksPerSample);
    int64_t nextClock = playheadClock.load() + addedTicks;

    if (looping.load() && nextClock >= loopEndTick.load())
    {
        int64_t loopSpan = loopEndTick.load() - loopStartTick.load();
        if (loopSpan > 0)
            nextClock = loopStartTick.load() + ((nextClock - loopStartTick.load()) % loopSpan);
    }

    playheadClock.store(nextClock);

    int ticksPer16th = ticksPerQuarterNote / 4;
    int currentStep16th = static_cast<int>((nextClock / ticksPer16th) % 16);

    if (currentStep16th != lastStep16th)
    {
        lastStep16th = currentStep16th;
        juce::MessageManager::callAsync([this, currentStep16th]() {
            listeners.call(&Listener::stepTriggered, currentStep16th);
        });
    }

    int bar = 1, beat = 1, tick = 0;
    double sec = 0.0;
    getPlayheadPosition(bar, beat, tick, sec);

    juce::MessageManager::callAsync([this, bar, beat, tick, sec]() {
        listeners.call(&Listener::playheadMoved, bar, beat, tick, sec);
    });
}

void AudioMidiEngine::audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                                      int numInputChannels,
                                                      float* const* outputChannelData,
                                                      int numOutputChannels,
                                                      int numSamples,
                                                      const juce::AudioIODeviceCallbackContext& context)
{
    if (numOutputChannels < 2)
        return;

    float* outL = outputChannelData[0];
    float* outR = outputChannelData[1];

    std::fill_n(outL, numSamples, 0.0f);
    std::fill_n(outR, numSamples, 0.0f);

    // 1. Route MIDI & Process Active Instrument Plugin (VST3 / CLAP)
    juce::MidiBuffer localMidi;
    {
        std::lock_guard<std::mutex> lock(pluginMutex);
        localMidi.swapWith(pendingPluginMidi);
    }

    if (activeInstrumentPlugin != nullptr && !instrumentBypassed.load())
    {
        juce::AudioBuffer<float> instBuf(outputChannelData, 2, numSamples);
        activeInstrumentPlugin->processBlock(instBuf, localMidi);
    }

    // 1b. Mix active procedural voices
    {
        std::lock_guard<std::mutex> lock(voiceMutex);
        for (auto& voice : voicePool)
        {
            if (!voice.active)
                continue;

            float panL = std::cos(voice.pan * 1.5707963f);
            float panR = std::sin(voice.pan * 1.5707963f);

            for (int i = 0; i < numSamples; ++i)
            {
                if (voice.playhead < voice.pcmData.size())
                {
                    float s = (static_cast<float>(voice.pcmData[voice.playhead++]) / 32768.0f) * voice.volume;
                    outL[i] += s * panL;
                    outR[i] += s * panR;
                }
                else
                {
                    voice.active = false;
                    break;
                }
            }
        }
    }

    // 2. Advance transport clock if playing
    if (playing.load())
    {
        advanceClock(numSamples);
    }

    // 3. DSP Effects Chain
    {
        std::lock_guard<std::mutex> lock(dspMutex);

        if (enableEq)
            equalizer.Process(outL, outR, numSamples);

        if (enableDelay)
            delay.Process(outL, outR, outL, outR, numSamples);

        if (enableChorus)
            chorus.Process(outL, outR, numSamples);

        if (enableReverb)
            reverb.Process(outL, outR, outL, outR, numSamples);

        if (enableLimiter)
            limiter.Process(outL, outR, numSamples);
    }

    // 3b. Active Master Effect Plugin (VST3 / CLAP)
    if (activeEffectPlugin != nullptr && !effectBypassed.load())
    {
        juce::AudioBuffer<float> fxBuf(outputChannelData, 2, numSamples);
        juce::MidiBuffer emptyMidi;
        activeEffectPlugin->processBlock(fxBuf, emptyMidi);
    }

    // 4. Master Volume & Metering
    float mVol = masterVolume.load();
    float maxL = 0.0f;
    float maxR = 0.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        outL[i] *= mVol;
        outR[i] *= mVol;

        float absL = std::abs(outL[i]);
        float absR = std::abs(outR[i]);
        if (absL > maxL) maxL = absL;
        if (absR > maxR) maxR = absR;
    }

    currentLeftPeak.store(maxL);
    currentRightPeak.store(maxR);
}
