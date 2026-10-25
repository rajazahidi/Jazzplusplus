//=============================================================================
// TransauralCrosstalkCancellerEngine.cpp
//
// Master Studio Pro Suite v31.0 - Apex Transcendence Sovereign Pinnacle Release
// Stereo Transaural Acoustic Crosstalk Cancellation (XTC) & 3D Monitor Spatializer.
//=============================================================================

#include "TransauralCrosstalkCancellerEngine.h"

namespace Jazz {

static const float kPi = 3.14159265358979323846f;

JZTransauralCrosstalkCancellerEngine::JZTransauralCrosstalkCancellerEngine(float sampleRate)
    : mSampleRate(sampleRate > 1000.0f ? sampleRate : 48000.0f)
    , mSpeakerAngleDeg(60.0f)
    , mXtcDepth(80.0f)
    , mListenerDistMeters(1.5f)
    , mSoundstageWidth(1.5f)
    , mItdMicroseconds(260.0f)
    , mDelaySamples(12)
    , mWriteIdx(0)
    , mHeadShadowStateL(0.0f)
    , mHeadShadowStateR(0.0f)
{
    mDelayBufL.assign(XTC_BUFFER_SIZE, 0.0f);
    mDelayBufR.assign(XTC_BUFFER_SIZE, 0.0f);
    updateAcousticGeometry();
}

void JZTransauralCrosstalkCancellerEngine::setSampleRate(float sampleRate) {
    if (sampleRate > 1000.0f) {
        mSampleRate = sampleRate;
        updateAcousticGeometry();
        reset();
    }
}

void JZTransauralCrosstalkCancellerEngine::setSpeakerAngleDegrees(float degrees) {
    mSpeakerAngleDeg = std::max(15.0f, std::min(120.0f, degrees));
    updateAcousticGeometry();
}

void JZTransauralCrosstalkCancellerEngine::setCancellationDepthPercent(float percent) {
    mXtcDepth = std::max(0.0f, std::min(100.0f, percent));
}

void JZTransauralCrosstalkCancellerEngine::setListenerDistanceMeters(float meters) {
    mListenerDistMeters = std::max(0.3f, std::min(5.0f, meters));
    updateAcousticGeometry();
}

void JZTransauralCrosstalkCancellerEngine::setSoundstageWidth(float width) {
    mSoundstageWidth = std::max(0.5f, std::min(3.0f, width));
}

void JZTransauralCrosstalkCancellerEngine::updateAcousticGeometry() {
    // Woodworth spherical head model:
    // a = 0.0875m (head radius), c = 343 m/s (speed of sound)
    const float a = 0.0875f;
    const float c = 343.0f;

    float thetaRad = (mSpeakerAngleDeg * 0.5f) * (kPi / 180.0f);
    float deltaT = (a / c) * (thetaRad + std::sin(thetaRad)); // seconds

    mItdMicroseconds = deltaT * 1000000.0f;
    float delaySamplesFloat = deltaT * mSampleRate;

    mDelaySamples = static_cast<size_t>(std::round(delaySamplesFloat));
    mDelaySamples = std::max<size_t>(1, std::min<size_t>(XTC_BUFFER_SIZE - 2, mDelaySamples));
}

void JZTransauralCrosstalkCancellerEngine::reset() {
    std::fill(mDelayBufL.begin(), mDelayBufL.end(), 0.0f);
    std::fill(mDelayBufR.begin(), mDelayBufR.end(), 0.0f);
    mWriteIdx = 0;
    mHeadShadowStateL = mHeadShadowStateR = 0.0f;
}

void JZTransauralCrosstalkCancellerEngine::process(const float* inL, const float* inR,
                                                 float* outL, float* outR, size_t numSamples) {
    if (!inL || !inR || !outL || !outR || numSamples == 0) return;

    // Head-shadow single-pole lowpass filter (~3.8 kHz cutoff)
    float shadowAlpha = std::exp(-2.0f * kPi * 3800.0f / mSampleRate);
    float depthFactor = (mXtcDepth * 0.01f) * 0.85f; // keep stability margin

    for (size_t i = 0; i < numSamples; ++i) {
        float sL = inL[i];
        float sR = inR[i];

        // 1. Write to cross-talk path delay buffers
        mDelayBufL[mWriteIdx] = sL;
        mDelayBufR[mWriteIdx] = sR;

        // 2. Read Woodworth delayed contralateral signal
        size_t readIdx = (mWriteIdx + XTC_BUFFER_SIZE - mDelaySamples) % XTC_BUFFER_SIZE;
        float delayedL = mDelayBufL[readIdx];
        float delayedR = mDelayBufR[readIdx];

        mWriteIdx = (mWriteIdx + 1) % XTC_BUFFER_SIZE;

        // 3. Head shadow acoustic filtering
        mHeadShadowStateL = (1.0f - shadowAlpha) * delayedL + shadowAlpha * mHeadShadowStateL;
        mHeadShadowStateR = (1.0f - shadowAlpha) * delayedR + shadowAlpha * mHeadShadowStateR;

        // 4. Recursive Cooper-Bauck Crosstalk Cancellation:
        // Subtract filtered delayed contralateral path from ipsilateral speaker
        float xtcL = sL - (mHeadShadowStateR * depthFactor);
        float xtcR = sR - (mHeadShadowStateL * depthFactor);

        // 5. Soundstage Width Enhancement
        float mid = 0.5f * (xtcL + xtcR);
        float side = 0.5f * (xtcR - xtcL) * mSoundstageWidth;

        float wideL = mid - side;
        float wideR = mid + side;

        outL[i] = wideL;
        outR[i] = wideR;
    }
}

} // namespace Jazz
