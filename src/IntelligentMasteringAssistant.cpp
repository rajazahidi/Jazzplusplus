#include "IntelligentMasteringAssistant.h"
#include <algorithm>
#include <cmath>

namespace Jazz {

JZIntelligentMasteringAssistant::JZIntelligentMasteringAssistant(MasteringTargetProfile target)
    : m_target(target)
{
}

float JZIntelligentMasteringAssistant::getTargetLufs(MasteringTargetProfile target)
{
    switch (target) {
        case MasteringTargetProfile::STREAMING_SPOTIFY: return -14.0f;
        case MasteringTargetProfile::STREAMING_APPLE:   return -16.0f;
        case MasteringTargetProfile::VINYL_RECORD:      return -18.0f;
        case MasteringTargetProfile::CD_AUDIOPHILE:     return -9.0f;
        case MasteringTargetProfile::CINEMA_BROADCAST:  return -23.0f;
    }
    return -14.0f;
}

float JZIntelligentMasteringAssistant::getTargetCeilingDb(MasteringTargetProfile target)
{
    switch (target) {
        case MasteringTargetProfile::STREAMING_SPOTIFY: return -1.0f;
        case MasteringTargetProfile::STREAMING_APPLE:   return -1.0f;
        case MasteringTargetProfile::VINYL_RECORD:      return -1.5f;
        case MasteringTargetProfile::CD_AUDIOPHILE:     return -0.1f;
        case MasteringTargetProfile::CINEMA_BROADCAST:  return -1.0f;
    }
    return -1.0f;
}

MasteringAnalysisResult JZIntelligentMasteringAssistant::analyzeMix(
    const float* left, const float* right, int numSamples, float sampleRate)
{
    (void)sampleRate;
    MasteringAnalysisResult res;
    res.integratedLufs = -18.0f;
    res.peakDbfs = -1.0f;
    res.crestFactorDb = 12.0f;
    res.spectralCentroidHz = 2200.0f;
    res.lowEndEnergyRatio = 0.35f;
    res.highEndEnergyRatio = 0.20f;

    if (!left || !right || numSamples <= 0) return res;

    float peakVal = 0.00001f;
    double sumSquares = 0.0;

    for (int i = 0; i < numSamples; ++i) {
        float mono = 0.5f * (left[i] + right[i]);
        float absVal = std::abs(mono);
        if (absVal > peakVal) peakVal = absVal;
        sumSquares += mono * mono;
    }

    float rmsVal = static_cast<float>(std::sqrt(sumSquares / static_cast<double>(numSamples)));
    if (rmsVal < 0.00001f) rmsVal = 0.00001f;

    float peakDb = 20.0f * std::log10(peakVal);
    float rmsDb = 20.0f * std::log10(rmsVal);

    res.peakDbfs = std::clamp(peakDb, -90.0f, 6.0f);
    res.crestFactorDb = std::clamp(peakDb - rmsDb, 1.0f, 30.0f);

    // Approximate Integrated LUFS from RMS with K-weighting offset (~ -0.69 dB)
    res.integratedLufs = std::clamp(rmsDb - 0.69f, -70.0f, 0.0f);

    return res;
}

MasteringRecommendation JZIntelligentMasteringAssistant::generateRecommendations(
    const MasteringAnalysisResult& analysis) const
{
    MasteringRecommendation rec;
    float targetLufs = getTargetLufs(m_target);
    rec.limiterCeilingDb = getTargetCeilingDb(m_target);

    // 1. Gain leveling to match target LUFS
    rec.targetGainCorrectionDb = targetLufs - analysis.integratedLufs;

    // 2. Spectral Balance Recommendations
    // If Spectral Centroid is below 1800 Hz, the mix is dark -> boost high air shelf
    if (analysis.spectralCentroidHz < 1800.0f) {
        rec.highAirShelfGainDb = +1.5f;
    } else if (analysis.spectralCentroidHz > 3200.0f) {
        // Overly bright / harsh -> gentle air dip
        rec.highAirShelfGainDb = -1.0f;
    } else {
        rec.highAirShelfGainDb = 0.0f;
    }

    // Low shelf adjustment based on sub-bass ratio
    if (analysis.lowEndEnergyRatio > 0.45f) {
        rec.lowShelfGainDb = -1.5f; // Tame excessive low end mud
    } else if (analysis.lowEndEnergyRatio < 0.20f) {
        rec.lowShelfGainDb = +1.5f; // Add low end warmth
    } else {
        rec.lowShelfGainDb = 0.0f;
    }

    rec.midParametricGainDb = 0.0f;

    // 3. Dynamic Crest & Limiter Threshold
    // If crest factor is large (> 14 dB), apply gentle limiting to catch peaks
    if (analysis.crestFactorDb > 14.0f) {
        rec.limiterThresholdDb = -3.0f;
    } else {
        rec.limiterThresholdDb = -1.5f;
    }

    // Vinyl mastering requires strict mono bass guard to prevent needle skipping
    rec.requireMonoBassGuard = (m_target == MasteringTargetProfile::VINYL_RECORD);

    return rec;
}

} // namespace Jazz
