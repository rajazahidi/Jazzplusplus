#ifndef JAZZ_ANALOG_TAPE_FLUTTER_MODELER_H
#define JAZZ_ANALOG_TAPE_FLUTTER_MODELER_H

#include <vector>
#include <cmath>
#include <cstdint>

namespace Jazz {

enum class TapeSpeedIPS {
    IPS_7_5,  // Lo-Fi consumer reel (high scrape flutter)
    IPS_15,   // Standard studio master (warm punchy balance)
    IPS_30    // High-end audiophile master (ultra-tight transient)
};

class JZAnalogTapeFlutterModeler {
public:
    JZAnalogTapeFlutterModeler(float sampleRate = 44100.0f);
    ~JZAnalogTapeFlutterModeler() = default;

    void setSampleRate(float sampleRate);
    float getSampleRate() const { return m_sampleRate; }

    void setTapeSpeed(TapeSpeedIPS speed);
    TapeSpeedIPS getTapeSpeed() const { return m_tapeSpeed; }

    void setScrapeFlutter(float amount); // 0.0 to 1.0 (2.5kHz - 4kHz head micro-friction)
    float getScrapeFlutter() const { return m_scrapeFlutter; }

    void setPinchRollerEccentricity(float amount); // 0.0 to 1.0 (4Hz - 8Hz rotational wobble)
    float getPinchRollerEccentricity() const { return m_pinchRoller; }

    void setTensionSag(float amount); // 0.0 to 1.0 (transient tape stretch)
    float getTensionSag() const { return m_tensionSag; }

    void setDropoutRate(float rate); // 0.0 to 1.0 (micro-dropouts from magnetic oxide loss)
    float getDropoutRate() const { return m_dropoutRate; }

    void setMix(float mix); // 0.0 to 1.0
    float getMix() const { return m_mix; }

    // Real-time audio processing
    void process(float& left, float& right);
    void processBlock(float* left, float* right, int numSamples);

    // Current mechanical speed variance telemetry (in cents)
    float getCurrentSpeedVarianceCents() const { return m_currentVarianceCents; }

    void reset();

private:
    float fractionalDelay(const float* buffer, int bufSize, int writeIdx, float delaySamples) const;

    float m_sampleRate;
    TapeSpeedIPS m_tapeSpeed;
    float m_scrapeFlutter;
    float m_pinchRoller;
    float m_tensionSag;
    float m_dropoutRate;
    float m_mix;

    // Fractional delay buffer for mechanical tape movement
    static const int DELAY_SIZE = 2048;
    float m_delayBufL[DELAY_SIZE];
    float m_delayBufR[DELAY_SIZE];
    int m_writeIdx;

    // Physical oscillator phases
    float m_scrapePhase;
    float m_pinchPhase;
    float m_chaoticNoise;
    float m_transientSagEnv;

    // Dropout simulation state
    float m_dropoutGainL;
    float m_dropoutGainR;
    float m_dropoutFilterL;
    float m_dropoutFilterR;

    float m_currentVarianceCents;
    uint32_t m_rng;
    uint32_t xorshift32();
    float randomFloat();
};

} // namespace Jazz

#endif // JAZZ_ANALOG_TAPE_FLUTTER_MODELER_H
