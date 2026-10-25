//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// TonewheelOrganDrawbarEngine.h - Electromechanical 91-Tonewheel Generator
// & 9-Drawbar Organ Engine
//
// Circuit-accurate physical modeling of the 1935 Hammond Model B3 / C3 organ:
// 91 electromechanical tonewheels running at historical synchronous gear ratios,
// 9 standard harmonic drawbars (16', 5 1/3', 8', 4', 2 2/3', 2', 1 3/5', 1 1/3', 1'),
// touch-response harmonic percussion (2nd/3rd harmonic, Fast/Slow decay, Soft/Normal),
// spring-contact bouncing key click noise, and magnetic pickup leakage crosstalk.
//*****************************************************************************

#ifndef JAZZ_TONEWHEEL_ORGAN_DRAWBAR_ENGINE_H
#define JAZZ_TONEWHEEL_ORGAN_DRAWBAR_ENGINE_H

#include <vector>
#include <string>
#include <cmath>
#include <cstddef>
#include <array>
#include <algorithm>

namespace Jazz {

enum class OrganPercussionDecay {
    FAST = 0,    // ~150ms punchy attack
    SLOW         // ~800ms sustained bell chime
};

enum class OrganPercussionHarmonic {
    SECOND = 2,  // 4' octave pitch
    THIRD = 3    // 2 2/3' fifth pitch
};

class JZTonewheelOrganDrawbarEngine {
public:
    static constexpr int NUM_TONEWHEELS = 91;
    static constexpr int NUM_DRAWBARS = 9;

    struct Parameters {
        std::array<int, NUM_DRAWBARS> drawbars = {8, 8, 8, 0, 0, 0, 0, 0, 0}; // 0 (off) to 8 (full)
        bool percussionEnabled = true;
        OrganPercussionHarmonic percussionHarmonic = OrganPercussionHarmonic::THIRD;
        OrganPercussionDecay percussionDecay = OrganPercussionDecay::FAST;
        bool percussionSoft = false;         // Normal vs Soft (-3dB)
        float keyClickAmount = 0.35f;        // Mechanical spring contact click (0.0 to 1.0)
        float leakageCrosstalk = 0.08f;      // Adjacent magnetic tonewheel leakage (0.0 to 0.3)
        float masterVolume = 0.8f;
    };

    JZTonewheelOrganDrawbarEngine(float sampleRate = 48000.0f);
    ~JZTonewheelOrganDrawbarEngine() = default;

    void initialize(float sampleRate);
    void reset();

    void setParameters(const Parameters& params);
    const Parameters& getParameters() const { return m_params; }

    // Drawbar control: index 0..8, value 0..8
    void setDrawbar(int drawbarIndex, int value0to8);
    int getDrawbar(int drawbarIndex) const;
    void setAllDrawbars(const std::array<int, NUM_DRAWBARS>& values);

    // Percussion controls
    void setPercussionEnabled(bool enabled);
    bool isPercussionEnabled() const { return m_params.percussionEnabled; }

    void setPercussionHarmonic(OrganPercussionHarmonic harm);
    OrganPercussionHarmonic getPercussionHarmonic() const { return m_params.percussionHarmonic; }

    void setPercussionDecay(OrganPercussionDecay decay);
    OrganPercussionDecay getPercussionDecay() const { return m_params.percussionDecay; }

    void setPercussionSoft(bool soft);
    bool isPercussionSoft() const { return m_params.percussionSoft; }

    void setKeyClickAmount(float click);
    float getKeyClickAmount() const { return m_params.keyClickAmount; }

    void setLeakageCrosstalk(float leak);
    float getLeakageCrosstalk() const { return m_params.leakageCrosstalk; }

    void setMasterVolume(float vol);
    float getMasterVolume() const { return m_params.masterVolume; }

    // MIDI Note Events
    void noteOn(int midiNote, float velocity = 1.0f);
    void noteOff(int midiNote);
    void allNotesOff();

    /**
     * @brief Render audio block.
     */
    void process(float* outL, float* outR, size_t numSamples);

    // Telemetry
    int getActiveVoicesCount() const;
    float getDrawbarGain(int drawbarIndex) const;
    std::string getPresetRegistrationString() const;

private:
    float m_sampleRate = 48000.0f;
    Parameters m_params;

    // 91 continuous tonewheel phase accumulators
    std::array<float, NUM_TONEWHEELS> m_tonewheelFrequencies;
    std::array<float, NUM_TONEWHEELS> m_tonewheelPhases;

    // Active key states (MIDI notes 36 [C2] to 96 [C7] typical 61-key manual)
    struct Voice {
        int midiNote = -1;
        float velocity = 0.0f;
        bool active = false;
        float keyClickEnv = 0.0f;
        float clickSign = 1.0f;
    };
    std::array<Voice, 128> m_voices;

    // Percussion single-trigger envelope
    float m_percussionEnv = 0.0f;
    bool m_percussionTriggered = false;

    // Drawbar harmonic semi-tone offsets relative to fundamental (MIDI Note)
    // 16' (-12), 5 1/3' (+7), 8' (0), 4' (+12), 2 2/3' (+19), 2' (+24), 1 3/5' (+28), 1 1/3' (+31), 1' (+36)
    static constexpr std::array<int, NUM_DRAWBARS> DRAWBAR_SEMITONE_OFFSETS = {
        -12, 7, 0, 12, 19, 24, 28, 31, 36
    };

    void initTonewheels();
    int midiNoteToTonewheel(int midiNote, int drawbarIndex) const;
};

} // namespace Jazz

namespace jazz {
    using JZTonewheelOrganDrawbarEngine = Jazz::JZTonewheelOrganDrawbarEngine;
}

#endif // JAZZ_TONEWHEEL_ORGAN_DRAWBAR_ENGINE_H
