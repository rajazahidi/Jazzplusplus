//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// JZEuclideanPolyrhythmEngine.h - Bjorklund Euclidean & Polymetric Sequencer
//
// Mathematical Euclidean rhythm generation via Bjorklund's algorithm E(k, n),
// concurrent multilane polymetric sequencing (independent step lengths),
// probabilistic note triggers, sub-step ratcheting, and micro-timing swing.
//*****************************************************************************

#ifndef JAZZ_EUCLIDEAN_POLYRHYTHM_ENGINE_H
#define JAZZ_EUCLIDEAN_POLYRHYTHM_ENGINE_H

#include <vector>
#include <string>
#include <cstdint>

namespace Jazz {

struct EuclideanTriggerEvent {
    int laneId;
    std::string laneName;
    uint8_t midiNote;
    uint8_t midiChannel;
    uint8_t velocity;
    int ratchetBurstIndex; // 0 for primary hit, 1..N for sub-step bursts
    int ratchetTotalCount;
    int microTimeOffsetTicks;
};

struct EuclideanLaneConfig {
    int laneId;
    std::string name;
    uint8_t midiNote;
    uint8_t midiChannel;
    int pulses;      // k: number of active pulses
    int totalSteps;  // n: total steps in sequence loop
    int rotation;    // step rotation offset
    float probability; // 0.0 to 1.0 trigger chance
    uint8_t baseVelocity;
    uint8_t velocityJitter;
    std::vector<int> stepRatchets; // 1 = normal, 2, 3, 4 = burst rolls
    bool mute;
    bool solo;

    // Runtime state
    int currentStep;
    std::vector<bool> pattern;
};

class JZEuclideanPolyrhythmEngine {
public:
    JZEuclideanPolyrhythmEngine(int ticksPerQuarterNote = 96);
    ~JZEuclideanPolyrhythmEngine() = default;

    // Bjorklund Algorithm: Computes E(k, n) Euclidean sequence
    static std::vector<bool> generateEuclidean(int pulses, int steps, int rotation = 0);

    // Lane Management
    int addLane(const std::string& name, uint8_t midiNote, int pulses, int steps,
                int rotation = 0, uint8_t channel = 10);
    bool removeLane(int laneId);
    size_t getLaneCount() const { return m_lanes.size(); }
    const EuclideanLaneConfig* getLane(int laneId) const;

    // Lane Controls
    bool setLaneEuclidean(int laneId, int pulses, int steps, int rotation = 0);
    bool setLaneProbability(int laneId, float prob);
    bool setLaneRatchet(int laneId, int stepIndex, int ratchetCount);
    bool setLaneMute(int laneId, bool mute);
    bool setLaneSolo(int laneId, bool solo);

    // Global Groove & Timing Controls
    void setSwingPercent(float swing); // 50% = straight, 66% = triplet swing
    float getSwingPercent() const { return m_swingPercent; }

    void setTicksPerStep(int ticks) { m_ticksPerStep = (ticks > 0) ? ticks : 24; }
    int getTicksPerStep() const { return m_ticksPerStep; }

    // Sequencer Clock Tick Processing
    // Call every master tick; outputs triggers when any lane hits an active step
    bool processTick(uint64_t masterTick, std::vector<EuclideanTriggerEvent>& outEvents);

    void resetLanes();

private:
    int m_tpqn;
    int m_ticksPerStep;
    float m_swingPercent;
    std::vector<EuclideanLaneConfig> m_lanes;
    int m_nextLaneId;

    // Pseudo-random generator for probability
    uint32_t m_rngSeed;
    float getRandomFloat();
};

} // namespace Jazz

#endif // JAZZ_EUCLIDEAN_POLYRHYTHM_ENGINE_H
