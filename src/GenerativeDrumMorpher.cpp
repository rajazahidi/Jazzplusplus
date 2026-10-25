#include "GenerativeDrumMorpher.h"
#include <cstdint>
#include <algorithm>
#include <cmath>

namespace Jazz {

JZGenerativeDrumMorpher::JZGenerativeDrumMorpher(int stepsPerBar)
    : m_stepsPerBar(stepsPerBar > 0 ? stepsPerBar : 16)
    , m_morphRatio(0.0f)
    , m_fillIntervalBars(4)
    , m_fillLengthBeats(2)
    , m_fillStyle(DrumFillStyle::SNARE_ROLL_CRESCENDO)
    , m_ghostProb(0.20f)
    , m_jitterTicks(2)
    , m_manualFillTriggered(false)
    , m_isFillActive(false)
    , m_rngState(123456789)
{
    loadStandardPresetA();
    loadStandardPresetB();
}

uint32_t JZGenerativeDrumMorpher::xorshift32() {
    uint32_t x = m_rngState;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    m_rngState = x;
    return x;
}

float JZGenerativeDrumMorpher::randomFloat() {
    return static_cast<float>(xorshift32() & 0x00FFFFFF) / static_cast<float>(0x01000000);
}

void JZGenerativeDrumMorpher::setStepsPerBar(int steps) {
    if (steps >= 8 && steps <= 64) {
        m_stepsPerBar = steps;
        loadStandardPresetA();
        loadStandardPresetB();
    }
}

void JZGenerativeDrumMorpher::setMorphRatio(float ratio) {
    m_morphRatio = std::max(0.0f, std::min(1.0f, ratio));
}

void JZGenerativeDrumMorpher::setFillIntervalBars(int intervalBars) {
    m_fillIntervalBars = std::max(1, std::min(32, intervalBars));
}

void JZGenerativeDrumMorpher::setFillLengthBeats(int beats) {
    m_fillLengthBeats = std::max(1, std::min(4, beats));
}

void JZGenerativeDrumMorpher::setGhostNoteProbability(float prob) {
    m_ghostProb = std::max(0.0f, std::min(1.0f, prob));
}

void JZGenerativeDrumMorpher::setHumanizeJitterTicks(int maxTicks) {
    m_jitterTicks = std::max(0, std::min(12, maxTicks));
}

void JZGenerativeDrumMorpher::reset() {
    m_manualFillTriggered = false;
    m_isFillActive = false;
    m_rngState = 123456789;
}

void JZGenerativeDrumMorpher::loadStandardPresetA() {
    m_patternA.clear();

    // 0: Kick (36) on Beat 1 (0) and Beat 3 (8)
    DrumTrackPattern kick = {36, std::vector<DrumStep>(m_stepsPerBar, {false, 0, 0})};
    kick.steps[0] = {true, 110, 0};
    if (m_stepsPerBar >= 16) kick.steps[8] = {true, 105, 0};
    m_patternA.push_back(kick);

    // 1: Snare (38) on Beat 2 (4) and Beat 4 (12)
    DrumTrackPattern snare = {38, std::vector<DrumStep>(m_stepsPerBar, {false, 0, 0})};
    if (m_stepsPerBar >= 16) {
        snare.steps[4] = {true, 100, 0};
        snare.steps[12] = {true, 105, 0};
    }
    m_patternA.push_back(snare);

    // 2: Closed HH (42) on all 8th notes
    DrumTrackPattern hh = {42, std::vector<DrumStep>(m_stepsPerBar, {false, 0, 0})};
    for (int i = 0; i < m_stepsPerBar; i += 2) {
        hh.steps[i] = {true, (i % 4 == 0) ? uint8_t(85) : uint8_t(65), 0};
    }
    m_patternA.push_back(hh);
}

void JZGenerativeDrumMorpher::loadStandardPresetB() {
    m_patternB.clear();

    // 0: Kick (36) Syncopated Funk Breakbeat: steps 0, 6, 10
    DrumTrackPattern kick = {36, std::vector<DrumStep>(m_stepsPerBar, {false, 0, 0})};
    kick.steps[0] = {true, 115, 0};
    if (m_stepsPerBar >= 16) {
        kick.steps[6] = {true, 95, 0};
        kick.steps[10] = {true, 100, 0};
    }
    m_patternB.push_back(kick);

    // 1: Snare (38) with syncopation on step 14
    DrumTrackPattern snare = {38, std::vector<DrumStep>(m_stepsPerBar, {false, 0, 0})};
    if (m_stepsPerBar >= 16) {
        snare.steps[4] = {true, 108, 0};
        snare.steps[12] = {true, 108, 0};
        snare.steps[14] = {true, 85, 0};
    }
    m_patternB.push_back(snare);

    // 2: Closed HH (42) on all 16th notes with grooved velocity
    DrumTrackPattern hh = {42, std::vector<DrumStep>(m_stepsPerBar, {false, 0, 0})};
    for (int i = 0; i < m_stepsPerBar; ++i) {
        hh.steps[i] = {true, (i % 2 == 0) ? uint8_t(80) : uint8_t(55), 0};
    }
    m_patternB.push_back(hh);
}

void JZGenerativeDrumMorpher::generateFillEvents(int stepInBar, std::vector<DrumMidiEvent>& outEvents) {
    int stepsPerBeat = m_stepsPerBar / 4;
    int fillStartStep = m_stepsPerBar - (m_fillLengthBeats * stepsPerBeat);
    int stepInFill = stepInBar - fillStartStep;
    int totalFillSteps = m_fillLengthBeats * stepsPerBeat;

    float progress = static_cast<float>(stepInFill) / static_cast<float>(totalFillSteps);

    switch (m_fillStyle) {
        case DrumFillStyle::SNARE_ROLL_CRESCENDO: {
            // Snare roll getting louder up to velocity 125
            uint8_t vel = static_cast<uint8_t>(55.0f + progress * 70.0f);
            outEvents.push_back({38, vel, static_cast<int>(xorshift32() % 5) - 2});

            // Occasional kick punch underneath
            if (stepInFill % 2 == 0) {
                outEvents.push_back({36, static_cast<uint8_t>(70.0f + progress * 40.0f), 0});
            }
            break;
        }

        case DrumFillStyle::TOM_CASCADE_RUN: {
            // High Tom (50) -> Mid Tom (47) -> Floor Tom (45)
            if (progress < 0.33f) {
                outEvents.push_back({50, static_cast<uint8_t>(85 + progress * 30), 0});
            } else if (progress < 0.66f) {
                outEvents.push_back({47, static_cast<uint8_t>(90 + progress * 30), 0});
            } else {
                outEvents.push_back({45, static_cast<uint8_t>(100 + progress * 25), 0});
            }
            break;
        }

        case DrumFillStyle::POLYMETRIC_CROSS_BEAT: {
            // 3-against-4 accent pattern
            if (stepInFill % 3 == 0) {
                outEvents.push_back({38, 115, 0}); // Accent snare
                outEvents.push_back({36, 110, 0}); // Accent kick
            } else {
                outEvents.push_back({42, 70, 0}); // Closed HH
            }
            break;
        }

        case DrumFillStyle::SYNCOPATED_CHOKE: {
            if (stepInFill == totalFillSteps - 1) {
                // Crash (49) + Kick (36) accent
                outEvents.push_back({49, 120, 0});
                outEvents.push_back({36, 120, 0});
            } else if (stepInFill % 2 == 0) {
                outEvents.push_back({38, 90, 0});
            }
            break;
        }
    }
}

void JZGenerativeDrumMorpher::processStep(int barNumber, int stepIndex, std::vector<DrumMidiEvent>& outEvents) {
    outEvents.clear();

    int stepsPerBeat = m_stepsPerBar / 4;
    int fillStartStep = m_stepsPerBar - (m_fillLengthBeats * stepsPerBeat);

    bool isFillBar = (barNumber > 0 && (barNumber % m_fillIntervalBars == 0));
    bool isFillTime = (isFillBar || m_manualFillTriggered) && (stepIndex >= fillStartStep);

    if (isFillTime) {
        m_isFillActive = true;
        generateFillEvents(stepIndex, outEvents);

        // Reset manual trigger once fill reaches last step of bar
        if (stepIndex == m_stepsPerBar - 1) {
            m_manualFillTriggered = false;
            m_isFillActive = false;
        }
        return;
    } else {
        m_isFillActive = false;
    }

    // Regular Grooving with Pattern Morphing (Pattern A vs Pattern B)
    size_t numTracks = std::min(m_patternA.size(), m_patternB.size());
    for (size_t t = 0; t < numTracks; ++t) {
        int note = m_patternA[t].midiNote;
        const DrumStep& stepA = m_patternA[t].steps[stepIndex % m_stepsPerBar];
        const DrumStep& stepB = m_patternB[t].steps[stepIndex % m_stepsPerBar];

        bool trigger = false;
        uint8_t vel = 0;

        if (stepA.active && stepB.active) {
            trigger = true;
            vel = static_cast<uint8_t>((1.0f - m_morphRatio) * stepA.velocity + m_morphRatio * stepB.velocity);
        } else if (stepA.active) {
            if (randomFloat() >= m_morphRatio) {
                trigger = true;
                vel = stepA.velocity;
            }
        } else if (stepB.active) {
            if (randomFloat() <= m_morphRatio) {
                trigger = true;
                vel = stepB.velocity;
            }
        } else {
            // Snare Ghost note injection probability
            if (note == 38 && m_ghostProb > 0.01f) {
                if (randomFloat() < m_ghostProb) {
                    trigger = true;
                    vel = static_cast<uint8_t>(25 + (xorshift32() % 15)); // soft 25-40
                }
            }
        }

        if (trigger && vel > 0) {
            int jitter = 0;
            if (m_jitterTicks > 0) {
                jitter = (xorshift32() % (m_jitterTicks * 2 + 1)) - m_jitterTicks;
            }
            outEvents.push_back({note, vel, jitter});
        }
    }
}

} // namespace Jazz
