//=============================================================================
// TransauralCrosstalkCancellerEngine.h
//
// Master Studio Pro Suite v31.0 - Apex Transcendence Sovereign Pinnacle Release
// Stereo Transaural Acoustic Crosstalk Cancellation (XTC) & 3D Monitor Spatializer.
//
// Features:
// - Physical Cooper-Bauck / Woodworth Acoustic Crosstalk Cancellation (XTC)
// - Expands stereo soundstage across physical monitor speakers to 180 degrees
// - Eliminates inter-aural speaker bleed without comb-filtering coloration
// - Adjustable monitor aperture angle (20 to 90 degrees) and listener distance
// - Recursive 2nd-order crosstalk matrix with head shadow acoustic modeling
//=============================================================================

#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace Jazz {

class JZTransauralCrosstalkCancellerEngine {
public:
    explicit JZTransauralCrosstalkCancellerEngine(float sampleRate = 48000.0f);
    ~JZTransauralCrosstalkCancellerEngine() = default;

    void setSampleRate(float sampleRate);
    float getSampleRate() const { return mSampleRate; }

    // Configuration
    void setSpeakerAngleDegrees(float degrees); // 20.0 to 90.0 degrees (nominally 60.0 degrees)
    float getSpeakerAngleDegrees() const { return mSpeakerAngleDeg; }

    void setCancellationDepthPercent(float percent); // 0.0% to 100.0% (nominally 80%)
    float getCancellationDepthPercent() const { return mXtcDepth; }

    void setListenerDistanceMeters(float meters);    // 0.5 to 4.0 meters
    float getListenerDistanceMeters() const { return mListenerDistMeters; }

    void setSoundstageWidth(float width);            // 1.0x to 2.5x (nominally 1.5x)
    float getSoundstageWidth() const { return mSoundstageWidth; }

    // Telemetry
    float getInterauralTimeDelayMicroseconds() const { return mItdMicroseconds; }
    size_t getInterauralDelaySamples() const { return mDelaySamples; }
    bool isCancellationActive() const { return mXtcDepth > 5.0f; }

    // Processing
    void process(const float* inL, const float* inR,
                 float* outL, float* outR, size_t numSamples);

    void reset();

private:
    float mSampleRate;
    float mSpeakerAngleDeg;
    float mXtcDepth;
    float mListenerDistMeters;
    float mSoundstageWidth;

    // Telemetry
    float mItdMicroseconds;
    size_t mDelaySamples;

    // Delay ring buffers for cross-talk path (L -> R ear and R -> L ear)
    static constexpr size_t XTC_BUFFER_SIZE = 512;
    std::vector<float> mDelayBufL;
    std::vector<float> mDelayBufR;
    size_t mWriteIdx;

    // Recursive 2nd-order cancellation filter state
    float mHeadShadowStateL;
    float mHeadShadowStateR;

    void updateAcousticGeometry();
};

} // namespace Jazz
