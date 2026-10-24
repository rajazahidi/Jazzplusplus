#ifndef JAZZ_INTELLIGENT_MASTERING_ASSISTANT_H
#define JAZZ_INTELLIGENT_MASTERING_ASSISTANT_H

#include <vector>
#include <string>
#include <cmath>

namespace Jazz {

enum class MasteringTargetProfile {
    STREAMING_SPOTIFY,   // -14.0 LUFS, -1.0 dBTP
    STREAMING_APPLE,     // -16.0 LUFS, -1.0 dBTP
    VINYL_RECORD,        // -18.0 LUFS, -1.5 dBTP (Mono bass < 150Hz, tame high sibilance)
    CD_AUDIOPHILE,       // -9.0 LUFS, -0.1 dBTP
    CINEMA_BROADCAST     // -23.0 LUFS, -1.0 dBTP (EBU R128)
};

struct MasteringAnalysisResult {
    float integratedLufs;
    float peakDbfs;
    float crestFactorDb;       // Peak minus RMS (e.g. 14 dB = dynamic, 6 dB = heavily squashed)
    float spectralCentroidHz;  // Center of mass of frequency spectrum (1500-3000 Hz typical)
    float lowEndEnergyRatio;   // Proportion of energy in 20Hz-150Hz
    float highEndEnergyRatio;  // Proportion of energy > 8000Hz
};

struct MasteringRecommendation {
    float targetGainCorrectionDb;
    float lowShelfGainDb;
    float midParametricGainDb;
    float highAirShelfGainDb;
    float limiterThresholdDb;
    float limiterCeilingDb;
    bool requireMonoBassGuard;
};

class JZIntelligentMasteringAssistant {
public:
    JZIntelligentMasteringAssistant(MasteringTargetProfile target = MasteringTargetProfile::STREAMING_SPOTIFY);
    ~JZIntelligentMasteringAssistant() = default;

    void setTargetProfile(MasteringTargetProfile target) { m_target = target; }
    MasteringTargetProfile getTargetProfile() const { return m_target; }

    // Psychoacoustic Analysis of incoming stereo mixdown
    MasteringAnalysisResult analyzeMix(const float* left, const float* right, int numSamples, float sampleRate = 44100.0f);

    // Generates optimal mastering DSP adjustments tailored to analyzed audio & target profile
    MasteringRecommendation generateRecommendations(const MasteringAnalysisResult& analysis) const;

    // Target Profile Standards query
    static float getTargetLufs(MasteringTargetProfile target);
    static float getTargetCeilingDb(MasteringTargetProfile target);

private:
    MasteringTargetProfile m_target;
};

} // namespace Jazz

#endif // JAZZ_INTELLIGENT_MASTERING_ASSISTANT_H
