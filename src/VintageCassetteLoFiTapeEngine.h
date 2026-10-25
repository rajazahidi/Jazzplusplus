//=============================================================================
// VintageCassetteLoFiTapeEngine.h
//
// Master Studio Pro Suite v31.0 - Apex Transcendence Sovereign Pinnacle Release
// Vintage Compact Cassette Lo-Fi Tape & Mechanical Flutter/Dropouts Engine.
//
// Features:
// - Physical Compact Cassette modeling (1-7/8 IPS tape speed, 4.76 cm/s)
// - 3 tape formulations: Type I Ferric, Type II Chrome, Type IV Metal
// - Chaotic dual-motor mechanical Wow (0.5 - 3.5 Hz) & Flutter (20 - 50 Hz)
// - Head-gap loss Bessel frequency rolloff (gap width 1.2 um)
// - Magnetic oxide saturation & 120us / 70us NAB playback EQ
// - Random magnetic oxide dropouts and continuous shaped tape hiss
//=============================================================================

#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace Jazz {

enum class CassetteTapeType {
    TYPE_I_FERRIC = 0,
    TYPE_II_CHROME,
    TYPE_IV_METAL
};

class JZVintageCassetteLoFiTapeEngine {
public:
    explicit JZVintageCassetteLoFiTapeEngine(float sampleRate = 48000.0f);
    ~JZVintageCassetteLoFiTapeEngine() = default;

    void setSampleRate(float sampleRate);
    float getSampleRate() const { return mSampleRate; }

    // Formulation Selection
    void setTapeType(CassetteTapeType type);
    CassetteTapeType getTapeType() const { return mTapeType; }
    std::string getTapeTypeName() const;

    // Controls
    void setWowAmount(float percent);         // 0.0% to 100.0%
    float getWowAmount() const { return mWowAmount; }

    void setFlutterAmount(float percent);     // 0.0% to 100.0%
    float getFlutterAmount() const { return mFlutterAmount; }

    void setTapeDriveDb(float db);            // -12.0 dB to +18.0 dB
    float getTapeDriveDb() const { return mTapeDriveDb; }

    void setTapeHissLevelDb(float db);        // -80.0 dB to -30.0 dB
    float getTapeHissLevelDb() const { return mHissLevelDb; }

    void setDropoutsAmount(float percent);    // 0.0% to 100.0%
    float getDropoutsAmount() const { return mDropoutsAmount; }

    // Telemetry
    float getCurrentWowDeviationCents() const { return mCurrentWowCents; }
    float getCurrentFlutterDeviationCents() const { return mCurrentFlutterCents; }
    size_t getTriggeredDropoutsCount() const { return mDropoutsCount; }

    // Processing
    void process(const float* inL, const float* inR,
                 float* outL, float* outR, size_t numSamples);

    void reset();

private:
    float mSampleRate;
    CassetteTapeType mTapeType;
    float mWowAmount;
    float mFlutterAmount;
    float mTapeDriveDb;
    float mLinearDrive;
    float mHissLevelDb;
    float mLinearHiss;
    float mDropoutsAmount;

    // Telemetry
    float mCurrentWowCents;
    float mCurrentFlutterCents;
    size_t mDropoutsCount;

    // Wow & Flutter oscillator phases
    float mWowPhase1;
    float mWowPhase2;
    float mFlutterPhase1;
    float mFlutterPhase2;

    // Fractional delay line for mechanical pitch warble
    std::vector<float> mDelayLineL;
    std::vector<float> mDelayLineR;
    size_t mDelayWriteIdx;
    static constexpr size_t DELAY_BUF_SIZE = 2048;

    // Head-gap loss IIR filter
    float mGapFilterL;
    float mGapFilterR;

    // Dropout state
    float mDropoutGainL;
    float mDropoutGainR;
    size_t mDropoutCountdown;

    // PRNG for hiss and dropouts
    uint32_t mPrngState;
    float getRandomFloat();

    void updateFormulationProfile();
};

} // namespace Jazz
