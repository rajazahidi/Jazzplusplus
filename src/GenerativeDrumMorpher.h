#ifndef JAZZ_GENERATIVE_DRUM_MORPHER_H
#define JAZZ_GENERATIVE_DRUM_MORPHER_H

#include <vector>
#include <string>
#include <cstdint>

namespace Jazz {

enum class DrumFillStyle {
    SNARE_ROLL_CRESCENDO,
    TOM_CASCADE_RUN,
    POLYMETRIC_CROSS_BEAT,
    SYNCOPATED_CHOKE
};

struct DrumStep {
    bool active;
    uint8_t velocity; // 1 to 127
    int8_t microTimingTicks; // -10 to +10 ticks
};

struct DrumTrackPattern {
    int midiNote; // e.g. 36 = Kick, 38 = Snare, 42 = Closed HH, etc.
    std::vector<DrumStep> steps; // typically 16 steps per bar
};

struct DrumMidiEvent {
    int midiNote;
    uint8_t velocity;
    int tickOffset;
};

class JZGenerativeDrumMorpher {
public:
    JZGenerativeDrumMorpher(int stepsPerBar = 16);
    ~JZGenerativeDrumMorpher() = default;

    void setStepsPerBar(int steps);
    int getStepsPerBar() const { return m_stepsPerBar; }

    void setMorphRatio(float ratio); // 0.0 (100% Pattern A) to 1.0 (100% Pattern B)
    float getMorphRatio() const { return m_morphRatio; }

    void setFillIntervalBars(int intervalBars); // e.g. 4, 8, 16 bars
    int getFillIntervalBars() const { return m_fillIntervalBars; }

    void setFillLengthBeats(int beats); // 1, 2, or 4 beats
    int getFillLengthBeats() const { return m_fillLengthBeats; }

    void setFillStyle(DrumFillStyle style) { m_fillStyle = style; }
    DrumFillStyle getFillStyle() const { return m_fillStyle; }

    void setGhostNoteProbability(float prob); // 0.0 to 1.0
    float getGhostNoteProbability() const { return m_ghostProb; }

    void setHumanizeJitterTicks(int maxTicks);
    int getHumanizeJitterTicks() const { return m_jitterTicks; }

    // Manual instant fill trigger
    void triggerManualFill() { m_manualFillTriggered = true; }
    bool isFillActive() const { return m_isFillActive; }

    // Pattern Setup
    void loadStandardPresetA();
    void loadStandardPresetB();

    // Sequencer Clock step
    // Evaluates current bar and step index (0..stepsPerBar-1) and populates triggered drum notes
    void processStep(int barNumber, int stepIndex, std::vector<DrumMidiEvent>& outEvents);

    void reset();

private:
    void generateFillEvents(int stepInBar, std::vector<DrumMidiEvent>& outEvents);

    int m_stepsPerBar;
    float m_morphRatio;
    int m_fillIntervalBars;
    int m_fillLengthBeats;
    DrumFillStyle m_fillStyle;
    float m_ghostProb;
    int m_jitterTicks;

    bool m_manualFillTriggered;
    bool m_isFillActive;

    // Pattern A and Pattern B storage (Kick, Snare, ClosedHH, OpenHH, HiTom, LowTom, Crash)
    std::vector<DrumTrackPattern> m_patternA;
    std::vector<DrumTrackPattern> m_patternB;

    uint32_t m_rngState;
    uint32_t xorshift32();
    float randomFloat();
};

} // namespace Jazz

#endif // JAZZ_GENERATIVE_DRUM_MORPHER_H
