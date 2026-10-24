#ifndef JAZZ_GUITAR_AMP_PROFILER_H
#define JAZZ_GUITAR_AMP_PROFILER_H

#include <vector>
#include <string>
#include <cmath>

namespace Jazz {

enum class AmpTubeModel {
    CLEAN_AMERICAN_6L6,
    CRUNCH_BRITISH_EL34,
    HIGH_GAIN_LEAD_6505,
    VINTAGE_CLASS_A_EL84
};

enum class CabinetProfile {
    CAB_4x12_VINTAGE_30,
    CAB_2x12_GREENBACK,
    CAB_1x12_AMERICAN_CLEAN,
    CAB_4x10_TWEED_PUNCH
};

enum class MicPlacement {
    CENTER_CAP,
    CAP_EDGE,
    CONE,
    OFF_AXIS_45
};

struct CabinetSlotConfig {
    CabinetProfile profile;
    MicPlacement placement;
    float distanceCm;   // 0.0 to 10.0 cm
    float level;        // 0.0 to 1.0
    bool phaseInvert;
};

class JZGuitarAmpProfiler {
public:
    JZGuitarAmpProfiler(float sampleRate = 44100.0f);
    ~JZGuitarAmpProfiler() = default;

    void setSampleRate(float sampleRate);
    float getSampleRate() const { return m_sampleRate; }

    // Preamp & Power Amp Modeling
    void setAmpModel(AmpTubeModel model);
    AmpTubeModel getAmpModel() const { return m_ampModel; }

    void setGain(float gain); // 0.0 to 10.0
    float getGain() const { return m_gain; }

    void setMasterVolume(float master); // 0.0 to 10.0
    float getMasterVolume() const { return m_masterVolume; }

    void setPowerSag(float sag); // 0.0 (stiff) to 1.0 (heavy tube sag compression)
    float getPowerSag() const { return m_powerSag; }

    // 3-Band Tonestack + Presence + Resonance
    void setBass(float bass);       // 0.0 to 10.0
    void setMiddle(float mid);      // 0.0 to 10.0
    void setTreble(float treble);   // 0.0 to 10.0
    void setPresence(float presence); // 0.0 to 10.0

    // Dual-IR Cabinet Convolver Configuration
    void setCabinetA(CabinetProfile profile, MicPlacement placement, float distCm = 2.0f);
    void setCabinetB(CabinetProfile profile, MicPlacement placement, float distCm = 5.0f);
    void setCabinetBlend(float blend); // 0.0 (100% Cab A) to 1.0 (100% Cab B)
    float getCabinetBlend() const { return m_cabBlend; }

    void setCabinetBypass(bool bypass) { m_cabBypass = bypass; }
    bool isCabinetBypass() const { return m_cabBypass; }

    // Real-time audio processing
    float processSample(float in);
    void processStereo(float inL, float inR, float& outL, float& outR);
    void processBlock(float* buffer, int numSamples);

    // Dynamic Sag meter (0.0 to 1.0)
    float getCurrentSagCompression() const { return m_currentSagAmount; }

    void reset();

private:
    float triodePreampSaturate(float in, float drive) const;
    float powerAmpSaturate(float in, float drive) const;
    float convolveCabinet(float in, const CabinetSlotConfig& slot, float* delayLine, int& writeIdx);
    void updateFilters();

    float m_sampleRate;
    AmpTubeModel m_ampModel;
    float m_gain;
    float m_masterVolume;
    float m_powerSag;

    float m_bassVal, m_midVal, m_trebleVal, m_presenceVal;

    CabinetSlotConfig m_slotA;
    CabinetSlotConfig m_slotB;
    float m_cabBlend;
    bool m_cabBypass;

    // Power supply sag envelope tracker
    float m_sagEnv;
    float m_currentSagAmount;

    // Tonestack biquad filter states
    float m_bassL[2], m_midL[2], m_trebleL[2], m_presenceL[2];
    float m_bBass[3], m_aBass[3];
    float m_bMid[3], m_aMid[3];
    float m_bTreb[3], m_aTreb[3];
    float m_bPres[3], m_aPres[3];

    // Dual-IR mini convolution delay buffers (64 taps FIR per cabinet)
    static const int IR_LEN = 64;
    float m_irBufA[IR_LEN];
    float m_irBufB[IR_LEN];
    int m_irWriteA;
    int m_irWriteB;
};

} // namespace Jazz

#endif // JAZZ_GUITAR_AMP_PROFILER_H
