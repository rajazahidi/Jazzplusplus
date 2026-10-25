//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// JZEuclideanPolyrhythmEngine.cpp - Bjorklund Euclidean & Polymetric Sequencer
//*****************************************************************************

#include "EuclideanPolyrhythmEngine.h"
#include <algorithm>
#include <cmath>

namespace Jazz {

JZEuclideanPolyrhythmEngine::JZEuclideanPolyrhythmEngine(int ticksPerQuarterNote)
    : m_tpqn(ticksPerQuarterNote > 0 ? ticksPerQuarterNote : 96)
    , m_ticksPerStep(24) // 16th note step = 96 / 4 = 24 ticks
    , m_swingPercent(50.0f) // 50% = straight
    , m_nextLaneId(1)
    , m_rngSeed(123456789)
{
}

float JZEuclideanPolyrhythmEngine::getRandomFloat() {
    // Linear congruential generator (LCG)
    m_rngSeed = (1103515245U * m_rngSeed + 12345U) & 0x7fffffffU;
    return static_cast<float>(m_rngSeed) / static_cast<float>(0x7fffffffU);
}

std::vector<bool> JZEuclideanPolyrhythmEngine::generateEuclidean(int pulses, int steps, int rotation) {
    if (steps <= 0) return {};
    if (pulses <= 0) return std::vector<bool>(steps, false);
    if (pulses >= steps) return std::vector<bool>(steps, true);

    // Bjorklund's algorithm via recursive vector groupings
    std::vector<std::vector<bool>> groups;
    for (int i = 0; i < pulses; ++i) {
        groups.push_back({true});
    }
    for (int i = 0; i < steps - pulses; ++i) {
        groups.push_back({false});
    }

    int countOnes = pulses;
    int countZeros = steps - pulses;

    while (countZeros > 1) {
        int pairs = std::min(countOnes, countZeros);
        for (int i = 0; i < pairs; ++i) {
            groups[i].insert(groups[i].end(), groups.back().begin(), groups.back().end());
            groups.pop_back();
        }
        countZeros = static_cast<int>(groups.size()) - pairs;
        countOnes = pairs;
    }

    std::vector<bool> result;
    result.reserve(steps);
    for (const auto& grp : groups) {
        result.insert(result.end(), grp.begin(), grp.end());
    }

    // Apply rotation
    if (rotation != 0 && !result.empty()) {
        int rot = ((rotation % steps) + steps) % steps;
        std::rotate(result.begin(), result.begin() + rot, result.end());
    }

    return result;
}

int JZEuclideanPolyrhythmEngine::addLane(const std::string& name, uint8_t midiNote,
                                        int pulses, int steps, int rotation, uint8_t channel) {
    EuclideanLaneConfig lane;
    lane.laneId = m_nextLaneId++;
    lane.name = name;
    lane.midiNote = midiNote;
    lane.midiChannel = channel;
    lane.pulses = std::max(0, std::min(steps, pulses));
    lane.totalSteps = std::max(1, steps);
    lane.rotation = rotation;
    lane.probability = 1.0f;
    lane.baseVelocity = 100;
    lane.velocityJitter = 0;
    lane.stepRatchets.assign(lane.totalSteps, 1);
    lane.mute = false;
    lane.solo = false;
    lane.currentStep = 0;
    lane.pattern = generateEuclidean(lane.pulses, lane.totalSteps, lane.rotation);

    m_lanes.push_back(lane);
    return lane.laneId;
}

bool JZEuclideanPolyrhythmEngine::removeLane(int laneId) {
    auto it = std::remove_if(m_lanes.begin(), m_lanes.end(),
                             [laneId](const EuclideanLaneConfig& l) { return l.laneId == laneId; });
    if (it != m_lanes.end()) {
        m_lanes.erase(it, m_lanes.end());
        return true;
    }
    return false;
}

const EuclideanLaneConfig* JZEuclideanPolyrhythmEngine::getLane(int laneId) const {
    for (const auto& lane : m_lanes) {
        if (lane.laneId == laneId) return &lane;
    }
    return nullptr;
}

bool JZEuclideanPolyrhythmEngine::setLaneEuclidean(int laneId, int pulses, int steps, int rotation) {
    for (auto& lane : m_lanes) {
        if (lane.laneId == laneId) {
            lane.totalSteps = std::max(1, steps);
            lane.pulses = std::max(0, std::min(lane.totalSteps, pulses));
            lane.rotation = rotation;
            lane.pattern = generateEuclidean(lane.pulses, lane.totalSteps, lane.rotation);
            lane.stepRatchets.resize(lane.totalSteps, 1);
            lane.currentStep %= lane.totalSteps;
            return true;
        }
    }
    return false;
}

bool JZEuclideanPolyrhythmEngine::setLaneProbability(int laneId, float prob) {
    for (auto& lane : m_lanes) {
        if (lane.laneId == laneId) {
            lane.probability = std::clamp(prob, 0.0f, 1.0f);
            return true;
        }
    }
    return false;
}

bool JZEuclideanPolyrhythmEngine::setLaneRatchet(int laneId, int stepIndex, int ratchetCount) {
    for (auto& lane : m_lanes) {
        if (lane.laneId == laneId) {
            if (stepIndex >= 0 && stepIndex < static_cast<int>(lane.stepRatchets.size())) {
                lane.stepRatchets[stepIndex] = std::clamp(ratchetCount, 1, 8);
                return true;
            }
        }
    }
    return false;
}

bool JZEuclideanPolyrhythmEngine::setLaneMute(int laneId, bool mute) {
    for (auto& lane : m_lanes) {
        if (lane.laneId == laneId) {
            lane.mute = mute;
            return true;
        }
    }
    return false;
}

bool JZEuclideanPolyrhythmEngine::setLaneSolo(int laneId, bool solo) {
    for (auto& lane : m_lanes) {
        if (lane.laneId == laneId) {
            lane.solo = solo;
            return true;
        }
    }
    return false;
}

void JZEuclideanPolyrhythmEngine::setSwingPercent(float swing) {
    m_swingPercent = std::clamp(swing, 50.0f, 75.0f);
}

bool JZEuclideanPolyrhythmEngine::processTick(uint64_t masterTick, std::vector<EuclideanTriggerEvent>& outEvents) {
    outEvents.clear();
    if (m_ticksPerStep <= 0) return false;

    // Check if this master tick falls on a step boundary
    if ((masterTick % static_cast<uint64_t>(m_ticksPerStep)) != 0) {
        return false;
    }

    uint64_t stepNumber = masterTick / static_cast<uint64_t>(m_ticksPerStep);

    // Calculate swing offset for even/odd 16th steps
    int swingOffset = 0;
    if ((stepNumber % 2) != 0 && m_swingPercent > 50.0f) {
        float swingRatio = (m_swingPercent - 50.0f) / 50.0f; // 0.0 to 0.5
        swingOffset = static_cast<int>(swingRatio * m_ticksPerStep * 0.5f);
    }

    bool anySolo = std::any_of(m_lanes.begin(), m_lanes.end(), [](const EuclideanLaneConfig& l) { return l.solo; });

    for (auto& lane : m_lanes) {
        if (lane.mute || (anySolo && !lane.solo)) {
            continue;
        }

        // Each polymetric lane advances modulo its own totalSteps!
        int stepIdx = static_cast<int>(stepNumber % static_cast<uint64_t>(lane.totalSteps));
        lane.currentStep = stepIdx;

        if (stepIdx < static_cast<int>(lane.pattern.size()) && lane.pattern[stepIdx]) {
            // Check probability
            if (lane.probability < 1.0f && getRandomFloat() > lane.probability) {
                continue; // Suppressed by probability
            }

            // Velocity calculation with optional jitter
            int vel = lane.baseVelocity;
            if (lane.velocityJitter > 0) {
                int jitter = static_cast<int>(getRandomFloat() * lane.velocityJitter * 2.0f) - lane.velocityJitter;
                vel = std::clamp(vel + jitter, 1, 127);
            }

            int ratchets = (stepIdx < static_cast<int>(lane.stepRatchets.size())) ? lane.stepRatchets[stepIdx] : 1;

            for (int r = 0; r < ratchets; ++r) {
                EuclideanTriggerEvent ev;
                ev.laneId = lane.laneId;
                ev.laneName = lane.name;
                ev.midiNote = lane.midiNote;
                ev.midiChannel = lane.midiChannel;
                ev.velocity = static_cast<uint8_t>(vel);
                ev.ratchetBurstIndex = r;
                ev.ratchetTotalCount = ratchets;
                ev.microTimeOffsetTicks = swingOffset + (r * (m_ticksPerStep / ratchets));

                outEvents.push_back(ev);
            }
        }
    }

    return !outEvents.empty();
}

void JZEuclideanPolyrhythmEngine::resetLanes() {
    for (auto& lane : m_lanes) {
        lane.currentStep = 0;
    }
}

} // namespace Jazz
