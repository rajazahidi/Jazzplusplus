//=============================================================================
// VintageCassetteLoFiTapeEngine.cpp
//
// Master Studio Pro Suite v31.0 - Apex Transcendence Sovereign Pinnacle Release
// Vintage Compact Cassette Lo-Fi Tape & Mechanical Flutter/Dropouts Engine.
//=============================================================================

#include "VintageCassetteLoFiTapeEngine.h"

namespace Jazz {

static const float kPi = 3.14159265358979323846f;

JZVintageCassetteLoFiTapeEngine::JZVintageCassetteLoFiTapeEngine(float sampleRate)
    : mSampleRate(sampleRate > 1000.0f ? sampleRate : 48000.0f)
    , mTapeType(CassetteTapeType::TYPE_I_FERRIC)
    , mWowAmount(35.0f)
    , mFlutterAmount(25.0f)
    , mTapeDriveDb(3.0f)
    , mLinearDrive(1.4125f)
    , mHissLevelDb(-54.0f)
    , mLinearHiss(0.002f)
    , mDropoutsAmount(20.0f)
    , mCurrentWowCents(0.0f)
    , mCurrentFlutterCents(0.0f)
    , mDropoutsCount(0)
    , mWowPhase1(0.0f)
    , mWowPhase2(0.0f)
    , mFlutterPhase1(0.0f)
    , mFlutterPhase2(0.0f)
    , mDelayWriteIdx(0)
    , mGapFilterL(0.0f)
    , mGapFilterR(0.0f)
    , mDropoutGainL(1.0f)
    , mDropoutGainR(1.0f)
    , mDropoutCountdown(0)
    , mPrngState(123456789)
{
    mDelayLineL.assign(DELAY_BUF_SIZE, 0.0f);
    mDelayLineR.assign(DELAY_BUF_SIZE, 0.0f);

    setTapeDriveDb(mTapeDriveDb);
    setTapeHissLevelDb(mHissLevelDb);
}

void JZVintageCassetteLoFiTapeEngine::setSampleRate(float sampleRate) {
    if (sampleRate > 1000.0f) {
        mSampleRate = sampleRate;
        reset();
    }
}

void JZVintageCassetteLoFiTapeEngine::setTapeType(CassetteTapeType type) {
    mTapeType = type;
    updateFormulationProfile();
}

std::string JZVintageCassetteLoFiTapeEngine::getTapeTypeName() const {
    switch (mTapeType) {
        case CassetteTapeType::TYPE_I_FERRIC:  return "Type I Normal Bias (Fe2O3 Gamma Ferric 120us)";
        case CassetteTapeType::TYPE_II_CHROME: return "Type II High Bias (CrO2 Chrome 70us)";
        case CassetteTapeType::TYPE_IV_METAL:  return "Type IV Metal Particle (Pure Fe 70us)";
        default: return "Compact Cassette";
    }
}

void JZVintageCassetteLoFiTapeEngine::updateFormulationProfile() {
    switch (mTapeType) {
        case CassetteTapeType::TYPE_I_FERRIC:
            mHissLevelDb = -54.0f;
            break;
        case CassetteTapeType::TYPE_II_CHROME:
            mHissLevelDb = -62.0f;
            break;
        case CassetteTapeType::TYPE_IV_METAL:
            mHissLevelDb = -68.0f;
            break;
    }
    mLinearHiss = std::pow(10.0f, mHissLevelDb / 20.0f);
}

void JZVintageCassetteLoFiTapeEngine::setWowAmount(float percent) {
    mWowAmount = std::max(0.0f, std::min(100.0f, percent));
}

void JZVintageCassetteLoFiTapeEngine::setFlutterAmount(float percent) {
    mFlutterAmount = std::max(0.0f, std::min(100.0f, percent));
}

void JZVintageCassetteLoFiTapeEngine::setTapeDriveDb(float db) {
    mTapeDriveDb = std::max(-24.0f, std::min(24.0f, db));
    mLinearDrive = std::pow(10.0f, mTapeDriveDb / 20.0f);
}

void JZVintageCassetteLoFiTapeEngine::setTapeHissLevelDb(float db) {
    mHissLevelDb = std::max(-90.0f, std::min(-20.0f, db));
    mLinearHiss = std::pow(10.0f, mHissLevelDb / 20.0f);
}

void JZVintageCassetteLoFiTapeEngine::setDropoutsAmount(float percent) {
    mDropoutsAmount = std::max(0.0f, std::min(100.0f, percent));
}

void JZVintageCassetteLoFiTapeEngine::reset() {
    std::fill(mDelayLineL.begin(), mDelayLineL.end(), 0.0f);
    std::fill(mDelayLineR.begin(), mDelayLineR.end(), 0.0f);
    mDelayWriteIdx = 0;
    mGapFilterL = mGapFilterR = 0.0f;
    mDropoutGainL = mDropoutGainR = 1.0f;
    mDropoutCountdown = 0;
    mCurrentWowCents = mCurrentFlutterCents = 0.0f;
    mDropoutsCount = 0;
    mWowPhase1 = mWowPhase2 = 0.0f;
    mFlutterPhase1 = mFlutterPhase2 = 0.0f;
}

float JZVintageCassetteLoFiTapeEngine::getRandomFloat() {
    // 32-bit xorshift PRNG
    mPrngState ^= mPrngState << 13;
    mPrngState ^= mPrngState >> 17;
    mPrngState ^= mPrngState << 5;
    return static_cast<float>(mPrngState) / 4294967296.0f;
}

void JZVintageCassetteLoFiTapeEngine::process(const float* inL, const float* inR,
                                            float* outL, float* outR, size_t numSamples) {
    if (!inL || !inR || !outL || !outR || numSamples == 0) return;

    // Head-gap loss single-pole lowpass filter coefficient
    // Type I cut ~11.5 kHz, Type II cut ~15 kHz, Type IV cut ~18 kHz
    float cutoffHz = 11500.0f;
    if (mTapeType == CassetteTapeType::TYPE_II_CHROME) cutoffHz = 15000.0f;
    else if (mTapeType == CassetteTapeType::TYPE_IV_METAL) cutoffHz = 18000.0f;
    float gapAlpha = std::exp(-2.0f * kPi * cutoffHz / mSampleRate);

    // Modulation frequencies:
    // Wow: 1.2 Hz and 0.73 Hz (slow capstan out-of-roundness)
    float wowInc1 = 2.0f * kPi * 1.2f / mSampleRate;
    float wowInc2 = 2.0f * kPi * 0.73f / mSampleRate;
    // Flutter: 33 Hz and 47 Hz (fast roller vibration)
    float flutterInc1 = 2.0f * kPi * 33.0f / mSampleRate;
    float flutterInc2 = 2.0f * kPi * 47.0f / mSampleRate;

    float centerDelay = 120.0f; // nominal delay in samples
    float maxWowDevSamples = (mWowAmount * 0.01f) * 18.0f;
    float maxFlutterDevSamples = (mFlutterAmount * 0.01f) * 3.5f;

    for (size_t i = 0; i < numSamples; ++i) {
        float inSampleL = inL[i];
        float inSampleR = inR[i];

        // 1. Write to fractional delay buffer
        mDelayLineL[mDelayWriteIdx] = inSampleL;
        mDelayLineR[mDelayWriteIdx] = inSampleR;

        // 2. Compute mechanical Wow & Flutter delay modulation
        float wowOsc = 0.65f * std::sin(mWowPhase1) + 0.35f * std::sin(mWowPhase2);
        float flutterOsc = 0.60f * std::sin(mFlutterPhase1) + 0.40f * std::sin(mFlutterPhase2);

        mWowPhase1 += wowInc1; if (mWowPhase1 >= 2.0f * kPi) mWowPhase1 -= 2.0f * kPi;
        mWowPhase2 += wowInc2; if (mWowPhase2 >= 2.0f * kPi) mWowPhase2 -= 2.0f * kPi;
        mFlutterPhase1 += flutterInc1; if (mFlutterPhase1 >= 2.0f * kPi) mFlutterPhase1 -= 2.0f * kPi;
        mFlutterPhase2 += flutterInc2; if (mFlutterPhase2 >= 2.0f * kPi) mFlutterPhase2 -= 2.0f * kPi;

        float totalDelaySamples = centerDelay + (wowOsc * maxWowDevSamples) + (flutterOsc * maxFlutterDevSamples);
        totalDelaySamples = std::max(2.0f, std::min(static_cast<float>(DELAY_BUF_SIZE - 2), totalDelaySamples));

        // Linear interpolation readout
        float readPos = static_cast<float>(mDelayWriteIdx) - totalDelaySamples;
        while (readPos < 0.0f) readPos += static_cast<float>(DELAY_BUF_SIZE);
        size_t idx0 = static_cast<size_t>(readPos) % DELAY_BUF_SIZE;
        size_t idx1 = (idx0 + 1) % DELAY_BUF_SIZE;
        float frac = readPos - std::floor(readPos);

        float warbledL = (1.0f - frac) * mDelayLineL[idx0] + frac * mDelayLineL[idx1];
        float warbledR = (1.0f - frac) * mDelayLineR[idx0] + frac * mDelayLineR[idx1];

        mDelayWriteIdx = (mDelayWriteIdx + 1) % DELAY_BUF_SIZE;

        // 3. Head-Gap Loss High-Frequency Rolloff
        mGapFilterL = (1.0f - gapAlpha) * warbledL + gapAlpha * mGapFilterL;
        mGapFilterR = (1.0f - gapAlpha) * warbledR + gapAlpha * mGapFilterR;

        // 4. Oxide Saturation (Asymmetrical magnetic hysteresis curve)
        float drivenL = mGapFilterL * mLinearDrive;
        float drivenR = mGapFilterR * mLinearDrive;
        float satL = std::tanh(drivenL + 0.03f) - 0.02f;
        float satR = std::tanh(drivenR + 0.03f) - 0.02f;

        // 5. Magnetic Oxide Random Dropouts
        if (mDropoutsCountdown > 0) {
            mDropoutsCountdown--;
            // Recovering
            mDropoutGainL += (1.0f - mDropoutGainL) * 0.02f;
            mDropoutGainR += (1.0f - mDropoutGainR) * 0.02f;
        } else {
            // Check for new dropout trigger
            if (mDropoutsAmount > 0.0f && (getRandomFloat() < (mDropoutsAmount * 0.000008f))) {
                mDropoutGainL = 0.25f + getRandomFloat() * 0.35f; // dip by 6-12 dB
                mDropoutGainR = 0.35f + getRandomFloat() * 0.30f;
                mDropoutsCountdown = static_cast<size_t>(mSampleRate * (0.01f + getRandomFloat() * 0.025f)); // 10-35ms
                mDropoutsCount++;
            }
        }

        float tapeOutL = satL * mDropoutGainL;
        float tapeOutR = satR * mDropoutGainR;

        // 6. Analog Tape Hiss Injection (pink-like noise)
        float noise = (getRandomFloat() * 2.0f - 1.0f) * mLinearHiss;

        outL[i] = tapeOutL + noise;
        outR[i] = tapeOutR + noise;
    }

    mCurrentWowCents = (maxWowDevSamples / centerDelay) * 100.0f;
    mCurrentFlutterCents = (maxFlutterDevSamples / centerDelay) * 100.0f;
}

} // namespace Jazz
