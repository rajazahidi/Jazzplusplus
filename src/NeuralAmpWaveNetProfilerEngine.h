//=============================================================================
// NeuralAmpWaveNetProfilerEngine.h
//
// Master Studio Pro Suite v30.0 - Apex Colossus Sovereign Diamond Jubilee Release
// Real-Time Non-Linear Dilated Temporal Convolutional Network (WaveNet)
// Guitar Tube Amplifier Profiler & Dynamic Tube Transformer Modeler.
//
// Features:
// - 4-stage dilated 1D temporal convolution layers (dilations: 1, 2, 4, 8)
// - Non-linear gated activations: tanh(W_f * x) * sigmoid(W_g * x)
// - Preamp 12AX7 dual-triode asymmetric bias clipping & cathode memory dynamics
// - Push-pull power amplifier (6L6 / EL34) with transformer core hysteresis
// - Real-time power supply rail dynamic sag intermodulation
// - Sub-0.5ms zero-latency execution optimized for real-time live performance
// - 4 factory amp profiles: Clean Boutique, British Crunch, High-Gain Lead, Vintage Tweed
//=============================================================================

#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace Jazz {

enum class AmpProfile {
    CLEAN_BOUTIQUE = 0,
    BRITISH_CRUNCH,
    HIGH_GAIN_LEAD,
    VINTAGE_TWEED
};

class JZNeuralAmpWaveNetProfilerEngine {
public:
    explicit JZNeuralAmpWaveNetProfilerEngine(float sampleRate = 48000.0f);
    ~JZNeuralAmpWaveNetProfilerEngine() = default;

    void setSampleRate(float sampleRate);
    float getSampleRate() const { return mSampleRate; }

    // Amp Profile Selection
    void setAmpProfile(AmpProfile profile);
    AmpProfile getAmpProfile() const { return mProfile; }
    std::string getAmpProfileName() const;

    // Core Amp Controls
    void setGainDb(float gainDb);         // -24.0 dB to +24.0 dB
    float getGainDb() const { return mGainDb; }

    void setMasterVolumeDb(float volDb);  // -24.0 dB to +12.0 dB
    float getMasterVolumeDb() const { return mMasterVolumeDb; }

    // 4-Band Tonestack
    void setBass(float bass);             // 0.0 to 1.0 (nominally 0.5)
    float getBass() const { return mBass; }
    void setMiddle(float mid);            // 0.0 to 1.0 (nominally 0.5)
    float getMiddle() const { return mMiddle; }
    void setTreble(float treble);         // 0.0 to 1.0 (nominally 0.5)
    float getTreble() const { return mTreble; }
    void setPresence(float pres);         // 0.0 to 1.0 (nominally 0.5)
    float getPresence() const { return mPresence; }

    // Physical Dynamics
    void setPowerSag(float sag);          // 0.0 to 1.0 (power rail droop)
    float getPowerSag() const { return mPowerSag; }
    void setTubeBias(float bias);         // -0.5 to +0.5 (asymmetric grid bias)
    float getTubeBias() const { return mTubeBias; }

    // Telemetry
    float getCurrentSagVoltage() const { return mCurrentSagVoltage; }
    float getThdPercent() const { return mThdPercent; }
    bool isClipping() const { return mIsClipping; }

    // Audio Processing
    void process(const float* inL, const float* inR,
                 float* outL, float* outR, size_t numSamples);

    void reset();

private:
    float mSampleRate;
    AmpProfile mProfile;
    float mGainDb;
    float mMasterVolumeDb;
    float mBass;
    float mMiddle;
    float mTreble;
    float mPresence;
    float mPowerSag;
    float mTubeBias;

    // Computed linear gains
    float mInputLinearGain;
    float mMasterLinearGain;

    // Telemetry state
    float mCurrentSagVoltage;
    float mThdPercent;
    bool mIsClipping;

    // DC Blocker states
    float mDcInL, mDcOutL;
    float mDcInR, mDcOutR;

    // Tonestack biquad / IIR state
    struct TonestackState {
        float x1, x2, y1, y2;
        TonestackState() : x1(0), x2(0), y1(0), y2(0) {}
        void reset() { x1 = x2 = y1 = y2 = 0.0f; }
    };
    TonestackState mBassFilterL, mBassFilterR;
    TonestackState mMidFilterL, mMidFilterR;
    TonestackState mTrebleFilterL, mTrebleFilterR;
    TonestackState mPresenceFilterL, mPresenceFilterR;

    // WaveNet 1D temporal dilated layer ring buffers
    static constexpr size_t NUM_LAYERS = 4;
    static constexpr size_t MAX_DILATION = 8;
    std::vector<float> mDelayLineL[NUM_LAYERS];
    std::vector<float> mDelayLineR[NUM_LAYERS];
    size_t mWriteIndex[NUM_LAYERS];

    // Power Sag Envelope
    float mSagEnvelopeL;
    float mSagEnvelopeR;

    // Internal helpers
    void updateProfileParameters();
    float processPreampStage(float inSample, float bias);
    float processWaveNetLayers(float x, size_t channelIndex);
    float processPowerAmpStage(float inSample, float& sagEnv);
    float processTonestack(float x, TonestackState& bassF, TonestackState& midF,
                           TonestackState& trebF, TonestackState& presF);
    float runBiquad(float in, TonestackState& s, float b0, float b1, float b2, float a1, float a2);
};

} // namespace Jazz
