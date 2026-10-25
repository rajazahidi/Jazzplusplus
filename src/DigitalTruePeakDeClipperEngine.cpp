//=============================================================================
// DigitalTruePeakDeClipperEngine.cpp
//
// Master Studio Pro Suite v31.0 - Apex Transcendence Sovereign Pinnacle Release
// Digital True-Peak De-Clipper & Cubic Hermite Spline Waveform Inpainter.
//=============================================================================

#include "DigitalTruePeakDeClipperEngine.h"

namespace Jazz {

JZDigitalTruePeakDeClipperEngine::JZDigitalTruePeakDeClipperEngine(float sampleRate)
    : mSampleRate(sampleRate > 1000.0f ? sampleRate : 48000.0f)
    , mThresholdDbFS(-0.1f)
    , mLinearThreshold(0.988553f) // ~ -0.1 dBFS
    , mAutoHeadroom(true)
    , mReconstructionPurity(0.85f)
    , mClippedEventsCount(0)
    , mRepairedPeaksCount(0)
    , mMaxReconstructedPeakDbFS(-90.0f)
    , mHeadroomGainDb(0.0f)
    , mLinearHeadroomGain(1.0f)
    , mWriteIdx(0)
{
    setClippingThresholdDbFS(mThresholdDbFS);
}

void JZDigitalTruePeakDeClipperEngine::setSampleRate(float sampleRate) {
    if (sampleRate > 1000.0f) {
        mSampleRate = sampleRate;
        reset();
    }
}

void JZDigitalTruePeakDeClipperEngine::setClippingThresholdDbFS(float dbFS) {
    mThresholdDbFS = std::max(-6.0f, std::min(0.0f, dbFS));
    mLinearThreshold = std::pow(10.0f, mThresholdDbFS / 20.0f);
}

void JZDigitalTruePeakDeClipperEngine::setReconstructionPurity(float purity) {
    mReconstructionPurity = std::max(0.1f, std::min(1.0f, purity));
}

void JZDigitalTruePeakDeClipperEngine::reset() {
    mClippedEventsCount = 0;
    mRepairedPeaksCount = 0;
    mMaxReconstructedPeakDbFS = -90.0f;
    mHeadroomGainDb = 0.0f;
    mLinearHeadroomGain = 1.0f;
    mWriteIdx = 0;
}

float JZDigitalTruePeakDeClipperEngine::inpaintCubicHermite(float p0, float p1, float m0, float m1, float t) {
    float t2 = t * t;
    float t3 = t2 * t;

    float h00 = 2.0f * t3 - 3.0f * t2 + 1.0f;
    float h10 = t3 - 2.0f * t2 + t;
    float h01 = -2.0f * t3 + 3.0f * t2;
    float h11 = t3 - t2;

    return h00 * p0 + h10 * m0 + h01 * p1 + h11 * m1;
}

void JZDigitalTruePeakDeClipperEngine::process(const float* inL, const float* inR,
                                             float* outL, float* outR, size_t numSamples) {
    if (!inL || !inR || !outL || !outR || numSamples == 0) return;

    // Process channel L & R with lookahead boundary inpainting
    std::vector<float> workL(inL, inL + numSamples);
    std::vector<float> workR(inR, inR + numSamples);

    auto repairChannel = [&](std::vector<float>& s) {
        size_t n = s.size();
        size_t i = 0;

        while (i < n) {
            float absVal = std::abs(s[i]);

            // Detect flat-topped clipping: sample exceeds threshold
            if (absVal >= mLinearThreshold) {
                size_t clipStart = i;
                float sign = (s[i] >= 0.0f) ? 1.0f : -1.0f;

                // Find duration of consecutive clipped samples
                while (i < n && std::abs(s[i]) >= (mLinearThreshold - 0.005f)) {
                    i++;
                }
                size_t clipEnd = i; // exclusive
                size_t clipLen = clipEnd - clipStart;

                // If clipped for 1 to 16 samples, reconstruct trajectory
                if (clipLen >= 1 && clipLen <= 16) {
                    mClippedEventsCount++;

                    // Boundary sample values
                    float p0 = (clipStart > 0) ? s[clipStart - 1] : s[clipStart];
                    float p_prev = (clipStart > 1) ? s[clipStart - 2] : p0;
                    float p1 = (clipEnd < n) ? s[clipEnd] : s[clipEnd - 1];
                    float p_next = (clipEnd + 1 < n) ? s[clipEnd + 1] : p1;

                    // Slopes scaled to interval length
                    float interval = static_cast<float>(clipLen + 1);
                    float m0 = (p0 - p_prev) * interval * 1.5f;
                    float m1 = (p_next - p1) * interval * 1.5f;

                    // Ensure slope direction arches towards the true peak
                    if (sign > 0.0f) {
                        if (m0 < 0.0f) m0 = 0.1f * interval;
                        if (m1 > 0.0f) m1 = -0.1f * interval;
                    } else {
                        if (m0 > 0.0f) m0 = -0.1f * interval;
                        if (m1 < 0.0f) m1 = 0.1f * interval;
                    }

                    // Inpaint samples inside the clipped region
                    for (size_t k = 0; k < clipLen; ++k) {
                        float t = static_cast<float>(k + 1) / interval;
                        float inpainted = inpaintCubicHermite(p0, p1, m0, m1, t);

                        // Blend between clipped flat and pure Hermite reconstruction
                        float blended = (s[clipStart + k] * (1.0f - mReconstructionPurity)) +
                                        (inpainted * mReconstructionPurity);

                        // Ensure reconstructed peak exceeds the original flat top
                        if (sign > 0.0f && blended < mLinearThreshold) blended = mLinearThreshold + 0.05f;
                        if (sign < 0.0f && blended > -mLinearThreshold) blended = -mLinearThreshold - 0.05f;

                        s[clipStart + k] = blended;

                        float peakDb = 20.0f * std::log10(std::abs(blended) + 1e-6f);
                        mMaxReconstructedPeakDbFS = std::max(mMaxReconstructedPeakDbFS, peakDb);
                    }

                    mRepairedPeaksCount++;
                }
            } else {
                i++;
            }
        }
    };

    repairChannel(workL);
    repairChannel(workR);

    // Dynamic Headroom Compensation
    if (mAutoHeadroom && mMaxReconstructedPeakDbFS > 0.0f) {
        // Drop master gain by max reconstructed overage + 0.5 dB safety margin
        float targetGainDb = -mMaxReconstructedPeakDbFS - 0.5f;
        targetGainDb = std::max(-12.0f, std::min(0.0f, targetGainDb));
        mHeadroomGainDb = targetGainDb;
        mLinearHeadroomGain = std::pow(10.0f, mHeadroomGainDb / 20.0f);
    } else {
        mHeadroomGainDb = 0.0f;
        mLinearHeadroomGain = 1.0f;
    }

    for (size_t i = 0; i < numSamples; ++i) {
        outL[i] = workL[i] * mLinearHeadroomGain;
        outR[i] = workR[i] * mLinearHeadroomGain;
    }
}

} // namespace Jazz
