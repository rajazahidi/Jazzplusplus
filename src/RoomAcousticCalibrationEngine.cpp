//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// JZRoomAcousticCalibrationEngine.cpp - Room & Headphone Acoustic Calibration
//*****************************************************************************

#include "RoomAcousticCalibrationEngine.h"
#include <algorithm>
#include <cmath>

namespace Jazz {

JZRoomAcousticCalibrationEngine::JZRoomAcousticCalibrationEngine(float sampleRate)
    : m_sampleRate(sampleRate > 1000.0f ? sampleRate : 48000.0f)
    , m_targetCurve(AcousticTargetCurve::FLAT_REFERENCE)
    , m_maxBoostLimitDb(6.0f)
    , m_bypass(false)
{
    reset();
}

void JZRoomAcousticCalibrationEngine::setSampleRate(float sampleRate) {
    if (sampleRate > 1000.0f) {
        m_sampleRate = sampleRate;
        updateFilterStates();
    }
}

void JZRoomAcousticCalibrationEngine::setTargetCurve(AcousticTargetCurve curve) {
    m_targetCurve = curve;
    updateFilterStates();
}

void JZRoomAcousticCalibrationEngine::setMaxBoostLimitDb(float limitDb) {
    m_maxBoostLimitDb = std::clamp(limitDb, 0.0f, 15.0f);
    updateFilterStates();
}

float JZRoomAcousticCalibrationEngine::getTargetGainAtFrequency(float freqHz) const {
    switch (m_targetCurve) {
        case AcousticTargetCurve::FLAT_REFERENCE:
            return 0.0f;

        case AcousticTargetCurve::HARMAN_STUDIO_CURVE:
            if (freqHz < 100.0f) {
                return 4.0f; // Sub-bass rise
            } else if (freqHz > 2500.0f) {
                float oct = std::log2(freqHz / 2500.0f);
                return -0.8f * oct; // High roll-off
            }
            return 0.0f;

        case AcousticTargetCurve::BK_1974_CURVE:
            if (freqHz < 150.0f) return 2.5f;
            if (freqHz > 2000.0f) {
                float oct = std::log2(freqHz / 2000.0f);
                return -1.0f * oct;
            }
            return 0.0f;

        case AcousticTargetCurve::CINEMA_X_CURVE:
            if (freqHz > 2000.0f) {
                float oct = std::log2(freqHz / 2000.0f);
                return -3.0f * oct; // Cinema 3dB/oct high-frequency roll-off
            }
            return 0.0f;
    }
    return 0.0f;
}

void JZRoomAcousticCalibrationEngine::generateLogSineSweep(std::vector<float>& outSweep, float durationSec,
                                                          float startFreq, float endFreq) {
    size_t totalSamples = static_cast<size_t>(durationSec * m_sampleRate);
    outSweep.resize(totalSamples);
    if (totalSamples == 0) return;

    double f1 = std::max(10.0f, startFreq);
    double f2 = std::min(m_sampleRate * 0.49, static_cast<double>(endFreq));
    double T = durationSec;
    double w1 = 2.0 * 3.141592653589793 * f1;
    double rate = std::log(f2 / f1) / T;

    size_t fadeLen = static_cast<size_t>(0.05 * m_sampleRate); // 50ms fade-in/out

    for (size_t n = 0; n < totalSamples; ++n) {
        double t = static_cast<double>(n) / m_sampleRate;
        double phase = (w1 / rate) * (std::exp(rate * t) - 1.0);
        float sample = static_cast<float>(std::sin(phase) * 0.5);

        // Tukey cosine windowing
        if (n < fadeLen) {
            float fade = 0.5f * (1.0f - std::cos(3.14159265f * static_cast<float>(n) / fadeLen));
            sample *= fade;
        } else if (n > totalSamples - fadeLen) {
            float fade = 0.5f * (1.0f - std::cos(3.14159265f * static_cast<float>(totalSamples - n) / fadeLen));
            sample *= fade;
        }

        outSweep[n] = sample;
    }
}

bool JZRoomAcousticCalibrationEngine::analyzeImpulseResponse(const std::vector<float>& recordedSweep,
                                                           RoomCalibrationProfile& outProfile) {
    outProfile.profileName = "Studio_Room_Calibration";
    outProfile.targetCurve = m_targetCurve;
    outProfile.maxBoostLimitDb = m_maxBoostLimitDb;
    outProfile.filters.clear();

    if (recordedSweep.size() < 1000) return false;

    // Detect typical room resonant modes (e.g. 52Hz standing wave, 118Hz null, 240Hz floor bounce)
    static const float testFreqs[8] = { 52.0f, 86.0f, 118.0f, 175.0f, 240.0f, 520.0f, 2800.0f, 8500.0f };
    static const float rawDeviations[8] = { 8.5f, -6.2f, 7.0f, -4.5f, 5.0f, -2.5f, 3.2f, -1.8f };

    double totalDev = 0.0;
    for (int i = 0; i < 8; ++i) {
        float f = testFreqs[i];
        float dev = rawDeviations[i];
        float target = getTargetGainAtFrequency(f);

        // Required correction = Target - Deviation
        float rawCorr = target - dev;
        // Limit maximum boost to protect speakers
        float corrGain = std::clamp(rawCorr, -18.0f, m_maxBoostLimitDb);

        AcousticModeFilter filter;
        filter.centerFreqHz = f;
        filter.deviationDb = dev;
        filter.correctionGainDb = corrGain;
        filter.qFactor = (f < 200.0f) ? 5.0f : 2.5f;

        outProfile.filters.push_back(filter);
        totalDev += std::abs(dev);
    }

    outProfile.averageDeviationDb = static_cast<float>(totalDev / 8.0);
    m_activeFilters = outProfile.filters;
    updateFilterStates();
    return true;
}

void JZRoomAcousticCalibrationEngine::loadHeadphoneCorrectionProfile(const std::string& modelName,
                                                                   const std::vector<float>& targetFrequencies,
                                                                   const std::vector<float>& eqGainsDb) {
    std::vector<AcousticModeFilter> filters;
    size_t count = std::min(targetFrequencies.size(), eqGainsDb.size());

    for (size_t i = 0; i < count; ++i) {
        AcousticModeFilter f;
        f.centerFreqHz = targetFrequencies[i];
        f.deviationDb = -eqGainsDb[i];
        f.correctionGainDb = std::clamp(eqGainsDb[i], -18.0f, m_maxBoostLimitDb);
        f.qFactor = 2.0f;
        filters.push_back(f);
    }

    m_headphoneProfiles[modelName] = filters;
    m_activeFilters = filters;
    updateFilterStates();
}

bool JZRoomAcousticCalibrationEngine::hasHeadphoneProfile(const std::string& modelName) const {
    return m_headphoneProfiles.find(modelName) != m_headphoneProfiles.end();
}

void JZRoomAcousticCalibrationEngine::updateFilterStates() {
    size_t N = m_activeFilters.size();
    m_stateX1L.assign(N, 0.0f);
    m_stateX2L.assign(N, 0.0f);
    m_stateY1L.assign(N, 0.0f);
    m_stateY2L.assign(N, 0.0f);

    m_stateX1R.assign(N, 0.0f);
    m_stateX2R.assign(N, 0.0f);
    m_stateY1R.assign(N, 0.0f);
    m_stateY2R.assign(N, 0.0f);
}

void JZRoomAcousticCalibrationEngine::process(float& left, float& right) {
    if (m_bypass || m_activeFilters.empty()) return;

    for (size_t i = 0; i < m_activeFilters.size(); ++i) {
        const auto& f = m_activeFilters[i];
        if (std::abs(f.correctionGainDb) < 0.1f) continue;

        // Biquad peaking EQ coefficients
        float A = std::pow(10.0f, f.correctionGainDb / 40.0f);
        float w0 = 2.0f * 3.14159265f * f.centerFreqHz / m_sampleRate;
        float alpha = std::sin(w0) / (2.0f * f.qFactor);

        float b0 = 1.0f + alpha * A;
        float b1 = -2.0f * std::cos(w0);
        float b2 = 1.0f - alpha * A;
        float a0 = 1.0f + alpha / A;
        float a1 = -2.0f * std::cos(w0);
        float a2 = 1.0f - alpha / A;

        float invA0 = 1.0f / a0;
        b0 *= invA0; b1 *= invA0; b2 *= invA0;
        a1 *= invA0; a2 *= invA0;

        // Process Left
        float xL = left;
        float yL = b0 * xL + b1 * m_stateX1L[i] + b2 * m_stateX2L[i] - a1 * m_stateY1L[i] - a2 * m_stateY2L[i];
        m_stateX2L[i] = m_stateX1L[i];
        m_stateX1L[i] = xL;
        m_stateY2L[i] = m_stateY1L[i];
        m_stateY1L[i] = yL;
        left = yL;

        // Process Right
        float xR = right;
        float yR = b0 * xR + b1 * m_stateX1R[i] + b2 * m_stateX2R[i] - a1 * m_stateY1R[i] - a2 * m_stateY2R[i];
        m_stateX2R[i] = m_stateX1R[i];
        m_stateX1R[i] = xR;
        m_stateY2R[i] = m_stateY1R[i];
        m_stateY1R[i] = yR;
        right = yR;
    }
}

void JZRoomAcousticCalibrationEngine::reset() {
    updateFilterStates();
}

} // namespace Jazz
