//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// JZHardwareInsertDelayCompensator.cpp - Studio Hybrid Outboard Delay Alignment
//*****************************************************************************

#include "HardwareInsertDelayCompensator.h"
#include <algorithm>
#include <cmath>

namespace Jazz {

JZHardwareInsertDelayCompensator::JZHardwareInsertDelayCompensator(float sampleRate)
    : m_sampleRate(sampleRate > 1000.0f ? sampleRate : 48000.0f)
{
}

void JZHardwareInsertDelayCompensator::setSampleRate(float sampleRate) {
    if (sampleRate > 1000.0f) {
        m_sampleRate = sampleRate;
    }
}

void JZHardwareInsertDelayCompensator::generatePingSignal(std::vector<float>& outBuffer, size_t bufferLength, size_t pulseIndex) {
    outBuffer.assign(bufferLength, 0.0f);
    if (pulseIndex < bufferLength) {
        outBuffer[pulseIndex] = 1.0f; // Single Dirac pulse
    }
}

bool JZHardwareInsertDelayCompensator::analyzeReturnSignal(const float* returnSignal, size_t signalLength,
                                                          size_t pulseReferenceIndex, HardwarePingResult& outResult) {
    outResult.detected = false;
    outResult.latencySamples = 0.0;
    outResult.latencyMilliseconds = 0.0;
    outResult.phaseInverted = false;
    outResult.correlationPeak = 0.0f;

    if (!returnSignal || signalLength <= pulseReferenceIndex + 4) return false;

    // Find the maximum absolute peak in the return signal
    size_t peakIdx = pulseReferenceIndex;
    float peakVal = 0.0f;
    float absMax = 0.0f;

    for (size_t i = pulseReferenceIndex; i < signalLength; ++i) {
        float val = returnSignal[i];
        float a = std::abs(val);
        if (a > absMax) {
            absMax = a;
            peakVal = val;
            peakIdx = i;
        }
    }

    if (absMax < 0.05f) { // No recognizable ping response found above noise floor
        return false;
    }

    // Parabolic sub-sample peak refinement using neighbors: peakIdx-1, peakIdx, peakIdx+1
    double delta = 0.0;
    if (peakIdx > 0 && peakIdx + 1 < signalLength) {
        float yA = std::abs(returnSignal[peakIdx - 1]);
        float yB = std::abs(returnSignal[peakIdx]);
        float yC = std::abs(returnSignal[peakIdx + 1]);

        double denom = 2.0 * (yA - 2.0 * yB + yC);
        if (std::abs(denom) > 1e-6) {
            delta = (yA - yC) / denom;
            delta = std::clamp(delta, -0.5, 0.5);
        }
    }

    double totalLatencySamples = static_cast<double>(peakIdx - pulseReferenceIndex) + delta;
    if (totalLatencySamples < 0.0) totalLatencySamples = 0.0;

    outResult.detected = true;
    outResult.latencySamples = totalLatencySamples;
    outResult.latencyMilliseconds = (totalLatencySamples / m_sampleRate) * 1000.0;
    outResult.phaseInverted = (peakVal < 0.0f);
    outResult.correlationPeak = peakVal;

    return true;
}

bool JZHardwareInsertDelayCompensator::registerInsert(int insertId, const std::string& name,
                                                    double latencySamples, bool phaseInverted) {
    HardwareInsertChannel ch;
    ch.insertId = insertId;
    ch.hardwareName = name;
    ch.roundTripLatencySamples = std::max(0.0, latencySamples);
    ch.phaseInverted = phaseInverted;
    ch.enabled = true;
    ch.writeIdx = 0;

    // Pre-allocate delay ring buffers (up to 1 second delay headroom)
    size_t bufferSize = static_cast<size_t>(m_sampleRate) + 4096;
    ch.delayBufferL.assign(bufferSize, 0.0f);
    ch.delayBufferR.assign(bufferSize, 0.0f);

    m_inserts[insertId] = ch;
    return true;
}

bool JZHardwareInsertDelayCompensator::setInsertEnabled(int insertId, bool enabled) {
    auto it = m_inserts.find(insertId);
    if (it != m_inserts.end()) {
        it->second.enabled = enabled;
        return true;
    }
    return false;
}

const HardwareInsertChannel* JZHardwareInsertDelayCompensator::getInsert(int insertId) const {
    auto it = m_inserts.find(insertId);
    if (it != m_inserts.end()) {
        return &it->second;
    }
    return nullptr;
}

double JZHardwareInsertDelayCompensator::calculateMaxDelayCompensationSamples() const {
    double maxLatency = 0.0;
    for (const auto& pair : m_inserts) {
        if (pair.second.enabled && pair.second.roundTripLatencySamples > maxLatency) {
            maxLatency = pair.second.roundTripLatencySamples;
        }
    }
    return maxLatency;
}

void JZHardwareInsertDelayCompensator::processAlignment(int insertId, const float* inL, const float* inR,
                                                       float* outL, float* outR, size_t numSamples) {
    auto it = m_inserts.find(insertId);
    if (it == m_inserts.end() || !it->second.enabled) {
        // Direct passthrough
        std::copy(inL, inL + numSamples, outL);
        std::copy(inR, inR + numSamples, outR);
        return;
    }

    HardwareInsertChannel& ch = it->second;
    double maxLatency = calculateMaxDelayCompensationSamples();
    double neededDelay = maxLatency - ch.roundTripLatencySamples;
    size_t intDelay = static_cast<size_t>(std::round(std::max(0.0, neededDelay)));
    size_t bufSize = ch.delayBufferL.size();

    float polarityMult = ch.phaseInverted ? -1.0f : 1.0f;

    for (size_t i = 0; i < numSamples; ++i) {
        ch.delayBufferL[ch.writeIdx] = inL[i];
        ch.delayBufferR[ch.writeIdx] = inR[i];

        size_t readIdx = (ch.writeIdx + bufSize - intDelay) % bufSize;
        outL[i] = ch.delayBufferL[readIdx] * polarityMult;
        outR[i] = ch.delayBufferR[readIdx] * polarityMult;

        ch.writeIdx = (ch.writeIdx + 1) % bufSize;
    }
}

void JZHardwareInsertDelayCompensator::reset() {
    for (auto& pair : m_inserts) {
        std::fill(pair.second.delayBufferL.begin(), pair.second.delayBufferL.end(), 0.0f);
        std::fill(pair.second.delayBufferR.begin(), pair.second.delayBufferR.end(), 0.0f);
        pair.second.writeIdx = 0;
    }
}

} // namespace Jazz
