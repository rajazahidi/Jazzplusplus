//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// BowedStringAcousticBodyConvolverEngine.h - Bowed String Acoustic Body Impulse
// & Chladni Wood Resonator
//
// Physical acoustic modeling of bowed string instrument soundboxes:
// Violin, Viola, Cello, and Double Bass:
// Spruce top-plate & maple back-plate Chladni eigenmodes (B1- and B1+ modes),
// bridge rock/bounce transmission filter, F-hole Helmholtz bass air cavity coupling (A0 mode),
// soundpost (anima) asymmetric energy transfer, and varnish acoustic damping.
//*****************************************************************************

#ifndef JAZZ_BOWED_STRING_ACOUSTIC_BODY_CONVOLVER_ENGINE_H
#define JAZZ_BOWED_STRING_ACOUSTIC_BODY_CONVOLVER_ENGINE_H

#include <vector>
#include <string>
#include <cmath>
#include <cstddef>
#include <array>
#include <algorithm>

namespace Jazz {

enum class BowedInstrumentType {
    VIOLIN = 0,       // 4/4 Concert Master Violin (A0 ~280Hz, B1 ~460Hz/560Hz)
    VIOLA,            // 16.5-inch Viola (A0 ~225Hz, B1 ~360Hz/440Hz)
    CELLO,            // 4/4 Violoncello (A0 ~98Hz, B1 ~175Hz/210Hz)
    DOUBLE_BASS       // 3/4 Orchestral Contrabass (A0 ~60Hz, B1 ~90Hz/120Hz)
};

class JZBowedStringAcousticBodyConvolverEngine {
public:
    struct Parameters {
        BowedInstrumentType instrumentType = BowedInstrumentType::VIOLIN;
        float bodyWoodAge = 0.85f;           // Wood maturity & dryness: 0.0 (new) to 1.0 (vintage Stradivarius)
        float varnishDamping = 0.35f;        // Oil varnish high-frequency damping (0.0 to 1.0)
        float helmholtzA0DepthDb = 4.5f;     // F-hole air cavity resonance boost (0.0 to +10.0 dB)
        float bridgeRockingPeakDb = 3.0f;    // Bridge high-frequency transmission peak (-6.0 to +8.0 dB)
        float bodyMix = 0.8f;                // Acoustic body wet ratio (0.0 to 1.0)
        bool bypass = false;
    };

    JZBowedStringAcousticBodyConvolverEngine(float sampleRate = 48000.0f);
    ~JZBowedStringAcousticBodyConvolverEngine() = default;

    void initialize(float sampleRate);
    void reset();

    void setParameters(const Parameters& params);
    const Parameters& getParameters() const { return m_params; }

    void setInstrumentType(BowedInstrumentType type);
    BowedInstrumentType getInstrumentType() const { return m_params.instrumentType; }
    std::string getInstrumentTypeName() const;

    void setBodyWoodAge(float age0to1);
    float getBodyWoodAge() const { return m_params.bodyWoodAge; }

    void setVarnishDamping(float damp0to1);
    float getVarnishDamping() const { return m_params.varnishDamping; }

    void setHelmholtzA0DepthDb(float db);
    float getHelmholtzA0DepthDb() const { return m_params.helmholtzA0DepthDb; }

    void setBridgeRockingPeakDb(float db);
    float getBridgeRockingPeakDb() const { return m_params.bridgeRockingPeakDb; }

    void setBodyMix(float mix);
    float getBodyMix() const { return m_params.bodyMix; }

    void setBypass(bool bp);
    bool isBypassed() const { return m_params.bypass; }

    /**
     * @brief Process stereo audio buffer through bowed string body resonator.
     */
    void process(const float* inL, const float* inR,
                 float* outL, float* outR,
                 size_t numSamples);

    // Acoustic Telemetry
    float getHelmholtzA0FrequencyHz() const { return m_helmholtzA0Hz; }
    float getPlateB1FrequencyHz() const { return m_plateB1Hz; }
    float getBridgeHillFrequencyHz() const { return m_bridgeHillHz; }

private:
    float m_sampleRate = 48000.0f;
    Parameters m_params;

    float m_helmholtzA0Hz = 280.0f;
    float m_plateB1Hz = 480.0f;
    float m_bridgeHillHz = 3000.0f;

    // Resonator filter states (Stereo 2nd-order Biquads)
    // 1. F-hole Helmholtz A0 Air Cavity Resonator
    float m_a0B0 = 1.0f, m_a0B1 = 0.0f, m_a0B2 = 0.0f, m_a0A1 = 0.0f, m_a0A2 = 0.0f;
    float m_a0X1L = 0.0f, m_a0X2L = 0.0f, m_a0Y1L = 0.0f, m_a0Y2L = 0.0f;
    float m_a0X1R = 0.0f, m_a0X2R = 0.0f, m_a0Y1R = 0.0f, m_a0Y2R = 0.0f;

    // 2. Chladni B1 Wood Plate Resonator
    float m_b1B0 = 1.0f, m_b1B1 = 0.0f, m_b1B2 = 0.0f, m_b1A1 = 0.0f, m_b1A2 = 0.0f;
    float m_b1X1L = 0.0f, m_b1X2L = 0.0f, m_b1Y1L = 0.0f, m_b1Y2L = 0.0f;
    float m_b1X1R = 0.0f, m_b1X2R = 0.0f, m_b1Y1R = 0.0f, m_b1Y2R = 0.0f;

    // 3. Bridge Hill High-Frequency Transmission Peak
    float m_brgB0 = 1.0f, m_brgB1 = 0.0f, m_brgB2 = 0.0f, m_brgA1 = 0.0f, m_brgA2 = 0.0f;
    float m_brgX1L = 0.0f, m_brgX2L = 0.0f, m_brgY1L = 0.0f, m_brgY2L = 0.0f;
    float m_brgX1R = 0.0f, m_brgX2R = 0.0f, m_brgY1R = 0.0f, m_brgY2R = 0.0f;

    void updateModalAcoustics();
};

} // namespace Jazz

namespace jazz {
    using JZBowedStringAcousticBodyConvolverEngine = Jazz::JZBowedStringAcousticBodyConvolverEngine;
}

#endif // JAZZ_BOWED_STRING_ACOUSTIC_BODY_CONVOLVER_ENGINE_H
