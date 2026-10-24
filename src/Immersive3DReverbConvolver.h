#ifndef JAZZ_IMMERSIVE_3D_REVERB_CONVOLVER_H
#define JAZZ_IMMERSIVE_3D_REVERB_CONVOLVER_H

#include <vector>
#include <cmath>

namespace Jazz {

enum class ReverbSpacePreset {
    CONCERT_HALL,     // 24m x 16m x 10m (RT60 ~2.4s)
    CATHEDRAL,        // 35m x 20m x 18m (RT60 ~4.8s)
    STUDIO_LIVE_ROOM, // 9m x 6m x 3.5m (RT60 ~0.8s)
    AMBIENT_ARENA     // 50m x 35m x 22m (RT60 ~6.0s)
};

class JZImmersive3DReverbConvolver {
public:
    JZImmersive3DReverbConvolver(float sampleRate = 44100.0f);
    ~JZImmersive3DReverbConvolver() = default;

    void setSampleRate(float sampleRate);
    float getSampleRate() const { return m_sampleRate; }

    void setPreset(ReverbSpacePreset preset);
    ReverbSpacePreset getPreset() const { return m_preset; }

    void setRoomDimensions(float lengthM, float widthM, float heightM);
    void getRoomDimensions(float& lengthM, float& widthM, float& heightM) const;

    void setDecayTimeRt60(float seconds); // 0.3s to 10.0s
    float getDecayTimeRt60() const { return m_rt60; }

    void setDamping(float damping); // 0.0 (reflective) to 1.0 (warm/dark)
    float getDamping() const { return m_damping; }

    void setEarlyLateMix(float ratio); // 0.0 (early reflections only) to 1.0 (late tail only)
    float getEarlyLateMix() const { return m_earlyLateRatio; }

    void setWetMix(float mix); // 0.0 to 1.0
    float getWetMix() const { return m_wetMix; }

    // Core 7.1.4 Dolby Atmos processing:
    // Takes stereo/mono input and generates full 12-channel 3D immersive acoustic reverb
    // Channels: 0:L, 1:R, 2:C, 3:LFE, 4:Lss, 5:Rss, 6:Lsr, 7:Rsr, 8:Ltf, 9:Rtf, 10:Ltr, 11:Rtr
    void process7_1_4(float inL, float inR, float outChannels[12]);

    // Stereo downmixed reverb processing
    void processStereo(float& left, float& right);

    void reset();

private:
    void updateFdnCoeffs();

    float m_sampleRate;
    ReverbSpacePreset m_preset;
    float m_roomLength, m_roomWidth, m_roomHeight;
    float m_rt60;
    float m_damping;
    float m_earlyLateRatio;
    float m_wetMix;

    // 3D Early Reflections delay lines for 6 room surfaces (Front, Back, Left, Right, Floor, Ceiling)
    static const int ER_BUFFER_SIZE = 8192;
    float m_erBuffer[ER_BUFFER_SIZE];
    int m_erWriteIdx;

    int m_tapFront, m_tapBack, m_tapLeft, m_tapRight, m_tapFloor, m_tapCeil;
    float m_gainFront, m_gainBack, m_gainLeft, m_gainRight, m_gainFloor, m_gainCeil;

    // 8-channel 3D Feedback Delay Network (FDN) for diffuse late isotropic tail
    static const int FDN_LINES = 8;
    std::vector<float> m_fdnBuffers[FDN_LINES];
    int m_fdnWriteIdx[FDN_LINES];
    int m_fdnLengths[FDN_LINES];
    float m_fdnFeedbackGains[FDN_LINES];
    float m_fdnFilterState[FDN_LINES];
};

} // namespace Jazz

#endif // JAZZ_IMMERSIVE_3D_REVERB_CONVOLVER_H
