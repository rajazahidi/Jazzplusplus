//*****************************************************************************
// The JAZZ++ Midi Sequencer — JUCE 8 Edition
//
// Copyright (C) 2026 Raja Zahidi
// Audio & MIDI Device Engine, Transport Clock & Procedural DSP Mixer
//*****************************************************************************

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "SoundGenerator.h"
#include "AudioEffects.h"
#include "ClapPluginFormat.h"

#include <vector>
#include <mutex>
#include <atomic>

class AudioMidiEngine : public juce::AudioIODeviceCallback,
                        public juce::MidiInputCallback,
                        public juce::ChangeBroadcaster
{
public:
    struct Listener
    {
        virtual ~Listener() = default;
        virtual void playbackStateChanged(bool isPlaying) {}
        virtual void tempoChanged(double newBpm) {}
        virtual void playheadMoved(int bar, int beat, int tick, double seconds) {}
        virtual void stepTriggered(int step16th) {}
        virtual void audioLevelsUpdated(float leftPeak, float rightPeak) {}
    };

    struct ActiveVoice
    {
        std::vector<short> pcmData;
        size_t playhead = 0;
        float volume = 1.0f;
        float pan = 0.5f; // 0.0 left, 1.0 right
        bool active = false;
    };

    AudioMidiEngine();
    ~AudioMidiEngine() override;

    // Initialization
    juce::Result initialize(int numInputChannels = 0, int numOutputChannels = 2);
    void shutdown();

    juce::AudioDeviceManager& getDeviceManager() { return deviceManager; }

    // Transport & Playback Clock
    void startPlayback();
    void pausePlayback();
    void stopPlayback();
    bool isPlaying() const { return playing.load(); }

    void setTempo(double bpm);
    double getTempo() const { return tempoBpm.load(); }

    void setTimeSignature(int num, int den);
    int getTimeSigNumerator() const { return timeSigNumerator; }
    int getTimeSigDenominator() const { return timeSigDenominator; }

    void setPlayheadClock(int64_t tick);
    int64_t getPlayheadClock() const { return playheadClock.load(); }
    void getPlayheadPosition(int& bar, int& beat, int& tick, double& seconds) const;

    void setLooping(bool shouldLoop) { looping.store(shouldLoop); }
    bool isLooping() const { return looping.load(); }
    void setLoopRange(int64_t startTick, int64_t endTick);
    int64_t getLoopStart() const { return loopStartTick.load(); }
    int64_t getLoopEnd() const { return loopEndTick.load(); }

    // MIDI Output
    void setMidiOutputDevice(const juce::String& deviceId);
    juce::String getMidiOutputDeviceName() const;
    void sendNoteOn(int channel, int noteNumber, float velocity);
    void sendNoteOff(int channel, int noteNumber, float velocity = 0.0f);
    void allNotesOff(int channel = -1);

    // Procedural Synth & Drum Triggering
    void triggerDrum(JZSynthDrumType type, float velocity = 1.0f, float pan = 0.5f);
    void triggerRetroSFX(JZRetroSFXType type, float velocity = 1.0f);
    void triggerSample(const std::vector<short>& pcm, float velocity = 1.0f, float pan = 0.5f);

    // Audio Effects & Master controls
    void setMasterVolume(float vol) { masterVolume.store(vol); }
    float getMasterVolume() const { return masterVolume.load(); }

    JZEqualizerSettings& getEqualizerSettings() { return eqSettings; }
    void updateEqualizer();

    JZReverbSettings& getReverbSettings() { return reverbSettings; }
    void updateReverb();

    JZDelaySettings& getDelaySettings() { return delaySettings; }
    void updateDelay();

    JZChorusSettings& getChorusSettings() { return chorusSettings; }
    void updateChorus();

    JZLimiterSettings& getLimiterSettings() { return limiterSettings; }
    void updateLimiter();

    void setFxEnabled(bool eq, bool reverb, bool delay, bool chorus, bool limiter);

    // VST3 & CLAP Plugin Hosting
    juce::AudioPluginFormatManager& getPluginFormatManager() { return pluginFormatManager; }
    juce::KnownPluginList& getKnownPluginList() { return knownPluginList; }

    void scanPluginDirectory(const juce::FileSearchPath& searchPath);
    void loadPluginAsync(const juce::PluginDescription& desc, bool asInstrument,
                         std::function<void(bool success, const juce::String& error)> onComplete);
    void unloadPlugin(bool isInstrument);

    juce::AudioPluginInstance* getActiveInstrumentPlugin() const { return activeInstrumentPlugin.get(); }
    juce::AudioPluginInstance* getActiveEffectPlugin() const { return activeEffectPlugin.get(); }

    void setInstrumentBypassed(bool b) { instrumentBypassed.store(b); }
    bool isInstrumentBypassed() const { return instrumentBypassed.load(); }

    void setEffectBypassed(bool b) { effectBypassed.store(b); }
    bool isEffectBypassed() const { return effectBypassed.load(); }

    // Peak levels for UI meters (0.0 to 1.0)
    float getLeftPeak() const { return currentLeftPeak.load(); }
    float getRightPeak() const { return currentRightPeak.load(); }

    // Listeners
    void addEngineListener(Listener* listener);
    void removeEngineListener(Listener* listener);

    // AudioIODeviceCallback overrides
    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                         int numInputChannels,
                                         float* const* outputChannelData,
                                         int numOutputChannels,
                                         int numSamples,
                                         const juce::AudioIODeviceCallbackContext& context) override;
    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;

    // MidiInputCallback override
    void handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message) override;

private:
    juce::AudioDeviceManager deviceManager;
    std::unique_ptr<juce::MidiOutput> midiOutput;

    // Transport state
    std::atomic<bool> playing { false };
    std::atomic<bool> looping { false };
    std::atomic<double> tempoBpm { 120.0 };
    int timeSigNumerator = 4;
    int timeSigDenominator = 4;
    std::atomic<int64_t> playheadClock { 0 };
    std::atomic<int64_t> loopStartTick { 0 };
    std::atomic<int64_t> loopEndTick { 480 * 4 * 4 }; // 4 bars default
    double sampleRate = 44100.0;
    int ticksPerQuarterNote = 480;

    int lastStep16th = -1;

    // Master & metering
    std::atomic<float> masterVolume { 0.85f };
    std::atomic<float> currentLeftPeak { 0.0f };
    std::atomic<float> currentRightPeak { 0.0f };

    // Procedural voice pool
    static constexpr size_t kMaxVoices = 32;
    std::vector<ActiveVoice> voicePool;
    std::mutex voiceMutex;

    // DSP Effects
    std::mutex dspMutex;
    JZ3BandEqualizer equalizer;
    JZEqualizerSettings eqSettings;
    bool enableEq = true;

    JZFreeverb reverb;
    JZReverbSettings reverbSettings;
    bool enableReverb = true;

    JZStereoDelay delay;
    JZDelaySettings delaySettings;
    bool enableDelay = false;

    JZStereoChorus chorus;
    JZChorusSettings chorusSettings;
    bool enableChorus = false;

    JZAudioLimiterDistortion limiter;
    JZLimiterSettings limiterSettings;
    bool enableLimiter = true;

    // VST3 & CLAP Hosting members
    juce::AudioPluginFormatManager pluginFormatManager;
    juce::KnownPluginList knownPluginList;
    std::unique_ptr<juce::AudioPluginInstance> activeInstrumentPlugin;
    std::unique_ptr<juce::AudioPluginInstance> activeEffectPlugin;
    std::atomic<bool> instrumentBypassed { false };
    std::atomic<bool> effectBypassed { false };
    juce::MidiBuffer pendingPluginMidi;
    std::mutex pluginMutex;

    // Listeners
    juce::ListenerList<Listener> listeners;

    void advanceClock(int numSamples);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioMidiEngine)
};
