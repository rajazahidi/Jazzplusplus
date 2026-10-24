#ifndef JAZZ_BINAURAL_HRTF_SPATIALIZER_H
#define JAZZ_BINAURAL_HRTF_SPATIALIZER_H

#include <vector>
#include <cmath>

namespace Jazz {

struct SpatialPoint3D {
    float azimuthDeg;   // -180 to +180 (0 = front, 90 = right, -90 = left)
    float elevationDeg; // -90 to +90 (0 = eye level, +90 = top)
    float distance;     // meters (default 1.5m)
};

class JZBinauralHrtfSpatializer {
public:
    JZBinauralHrtfSpatializer(float sampleRate = 44100.0f);
    ~JZBinauralHrtfSpatializer() = default;

    void setSampleRate(float sampleRate);
    float getSampleRate() const { return m_sampleRate; }

    void setSourcePosition(float azimuthDeg, float elevationDeg, float distanceMeters = 1.5f);
    SpatialPoint3D getSourcePosition() const { return m_sourcePos; }

    // Head orientation for head-tracking simulation
    void setHeadOrientation(float yawDeg, float pitchDeg, float rollDeg);
    void getHeadOrientation(float& yawDeg, float& pitchDeg, float& rollDeg) const;

    // Headphone tuning & cross-talk
    void setCrossTalkCancellation(bool enable) { m_crossTalkCancel = enable; }
    bool isCrossTalkCancellation() const { return m_crossTalkCancel; }

    void setRoomAmbience(float levelPercent); // 0% (anechoic) to 100% (studio room)
    float getRoomAmbience() const { return m_roomAmbience; }

    // Core Binaural Rendering
    void processPointSource(float inMono, float& outLeft, float& outRight);
    void processStereo(float inL, float inR, float& outL, float& outR);

    // 7.1.4 Immersive Bed Downmix to Binaural
    // Order: L, R, C, LFE, Lss, Rss, Lsr, Rsr, Ltf, Rtf, Ltr, Rtr
    void process7_1_4(const float channels[12], float& outL, float& outR);

    void reset();

    // Woodworth max ITD in milliseconds
    static float getMaxItdMs();

private:
    void updateHrtfFilters();
    float fractionalDelay(const float* buffer, int bufSize, int writeIdx, float delaySamples) const;

    float m_sampleRate;
    SpatialPoint3D m_sourcePos;
    float m_yaw, m_pitch, m_roll;
    bool m_crossTalkCancel;
    float m_roomAmbience;

    // Fractional delay lines for left and right ear (Woodworth ITD)
    static const int DELAY_SIZE = 256;
    float m_delayBufL[DELAY_SIZE];
    float m_delayBufR[DELAY_SIZE];
    int m_writeIdx;

    float m_itdSamplesL;
    float m_itdSamplesR;

    // Head-shadowing ILD filter states (1-pole lowpass for far ear)
    float m_ildL, m_ildR;
    float m_ildCoeffL, m_ildCoeffR;
    float m_ildGainL, m_ildGainR;

    // Pinna elevation notch biquads
    float m_pinnaL[2], m_pinnaR[2];
    float m_pb0L, m_pb1L, m_pb2L, m_pa1L, m_pa2L;
    float m_pb0R, m_pb1R, m_pb2R, m_pa1R, m_pa2R;

    // Room early reflections delay taps (4 stereo taps)
    float m_roomDelayL[1024];
    float m_roomDelayR[1024];
    int m_roomWriteIdx;
};

} // namespace Jazz

#endif // JAZZ_BINAURAL_HRTF_SPATIALIZER_H
