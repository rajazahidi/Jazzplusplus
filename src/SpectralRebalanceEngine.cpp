#include "SpectralRebalanceEngine.h"
#include <algorithm>
#include <cmath>

namespace Jazz {

static const float PI = 3.14159265358979323846f;

JZSpectralRebalanceEngine::JZSpectralRebalanceEngine(float sampleRate)
    : m_sampleRate(sampleRate > 1000.0f ? sampleRate : 44100.0f)
    , m_transientEnv(0.0f)
    , m_slowEnv(0.0f)
{
    for (int i = 0; i < static_cast<int>(RebalanceStem::COUNT); ++i) {
        m_stems[i].gainDb = 0.0f;
        m_stems[i].mute = false;
        m_stems[i].solo = false;
        m_stems[i].pan = 0.0f;
        m_stemRms[i] = 0.0f;
    }

    reset();
    updateFilterCoeffs();
}

void JZSpectralRebalanceEngine::setSampleRate(float sampleRate) {
    if (sampleRate > 1000.0f) {
        m_sampleRate = sampleRate;
        updateFilterCoeffs();
    }
}

float JZSpectralRebalanceEngine::dbToLinear(float db) const {
    return std::pow(10.0f, db / 20.0f);
}

void JZSpectralRebalanceEngine::setStemGain(RebalanceStem stem, float gainDb) {
    int idx = static_cast<int>(stem);
    if (idx >= 0 && idx < static_cast<int>(RebalanceStem::COUNT)) {
        m_stems[idx].gainDb = std::max(-24.0f, std::min(12.0f, gainDb));
    }
}

float JZSpectralRebalanceEngine::getStemGain(RebalanceStem stem) const {
    int idx = static_cast<int>(stem);
    if (idx >= 0 && idx < static_cast<int>(RebalanceStem::COUNT)) {
        return m_stems[idx].gainDb;
    }
    return 0.0f;
}

void JZSpectralRebalanceEngine::setStemMute(RebalanceStem stem, bool mute) {
    int idx = static_cast<int>(stem);
    if (idx >= 0 && idx < static_cast<int>(RebalanceStem::COUNT)) {
        m_stems[idx].mute = mute;
    }
}

bool JZSpectralRebalanceEngine::isStemMuted(RebalanceStem stem) const {
    int idx = static_cast<int>(stem);
    if (idx >= 0 && idx < static_cast<int>(RebalanceStem::COUNT)) {
        return m_stems[idx].mute;
    }
    return false;
}

void JZSpectralRebalanceEngine::setStemSolo(RebalanceStem stem, bool solo) {
    int idx = static_cast<int>(stem);
    if (idx >= 0 && idx < static_cast<int>(RebalanceStem::COUNT)) {
        m_stems[idx].solo = solo;
    }
}

bool JZSpectralRebalanceEngine::isStemSolo(RebalanceStem stem) const {
    int idx = static_cast<int>(stem);
    if (idx >= 0 && idx < static_cast<int>(RebalanceStem::COUNT)) {
        return m_stems[idx].solo;
    }
    return false;
}

void JZSpectralRebalanceEngine::setStemPan(RebalanceStem stem, float pan) {
    int idx = static_cast<int>(stem);
    if (idx >= 0 && idx < static_cast<int>(RebalanceStem::COUNT)) {
        m_stems[idx].pan = std::max(-1.0f, std::min(1.0f, pan));
    }
}

float JZSpectralRebalanceEngine::getStemPan(RebalanceStem stem) const {
    int idx = static_cast<int>(stem);
    if (idx >= 0 && idx < static_cast<int>(RebalanceStem::COUNT)) {
        return m_stems[idx].pan;
    }
    return 0.0f;
}

float JZSpectralRebalanceEngine::getStemRms(RebalanceStem stem) const {
    int idx = static_cast<int>(stem);
    if (idx >= 0 && idx < static_cast<int>(RebalanceStem::COUNT)) {
        return m_stemRms[idx];
    }
    return 0.0f;
}

void JZSpectralRebalanceEngine::reset() {
    for (int i = 0; i < 2; ++i) {
        m_lpBassL[i] = m_lpBassR[i] = 0.0f;
        m_hpBassL[i] = m_hpBassR[i] = 0.0f;
        m_lpMidL[i]  = m_lpMidR[i]  = 0.0f;
        m_hpMidL[i]  = m_hpMidR[i]  = 0.0f;
    }
    m_transientEnv = 0.0f;
    m_slowEnv = 0.0f;
    for (int i = 0; i < static_cast<int>(RebalanceStem::COUNT); ++i) {
        m_stemRms[i] = 0.0f;
    }
}

void JZSpectralRebalanceEngine::updateFilterCoeffs() {
    // 2nd-order Butterworth low-pass for Bass crossover at 250 Hz
    float fcBass = 250.0f / m_sampleRate;
    float omegaBass = std::tan(PI * fcBass);
    float omega2Bass = omegaBass * omegaBass;
    float cBass = 1.0f + 1.41421356f * omegaBass + omega2Bass;

    m_b0Bass = omega2Bass / cBass;
    m_b1Bass = 2.0f * m_b0Bass;
    m_b2Bass = m_b0Bass;
    m_a1Bass = 2.0f * (omega2Bass - 1.0f) / cBass;
    m_a2Bass = (1.0f - 1.41421356f * omegaBass + omega2Bass) / cBass;

    // 2nd-order Butterworth low-pass for Vocal Mid crossover at 3500 Hz
    float fcMid = 3500.0f / m_sampleRate;
    float omegaMid = std::tan(PI * fcMid);
    float omega2Mid = omegaMid * omegaMid;
    float cMid = 1.0f + 1.41421356f * omegaMid + omega2Mid;

    m_b0Mid = omega2Mid / cMid;
    m_b1Mid = 2.0f * m_b0Mid;
    m_b2Mid = m_b0Mid;
    m_a1Mid = 2.0f * (omega2Mid - 1.0f) / cMid;
    m_a2Mid = (1.0f - 1.41421356f * omegaMid + omega2Mid) / cMid;
}

void JZSpectralRebalanceEngine::process(float& left, float& right) {
    float inL = left;
    float inR = right;

    // Direct form biquad filter for Low/Bass (250 Hz)
    float lowL = m_b0Bass * inL + m_lpBassL[0];
    m_lpBassL[0] = m_b1Bass * inL - m_a1Bass * lowL + m_lpBassL[1];
    m_lpBassL[1] = m_b2Bass * inL - m_a2Bass * lowL;

    float lowR = m_b0Bass * inR + m_lpBassR[0];
    m_lpBassR[0] = m_b1Bass * inR - m_a1Bass * lowR + m_lpBassR[1];
    m_lpBassR[1] = m_b2Bass * inR - m_a2Bass * lowR;

    // High portion = in - low (perfect reconstruction crossover)
    float highL = inL - lowL;
    float highR = inR - lowR;

    // Filter high portion through Mid biquad (3500 Hz)
    float midL = m_b0Mid * highL + m_lpMidL[0];
    m_lpMidL[0] = m_b1Mid * highL - m_a1Mid * midL + m_lpMidL[1];
    m_lpMidL[1] = m_b2Mid * highL - m_a2Mid * midL;

    float midR = m_b0Mid * highR + m_lpMidR[0];
    m_lpMidR[0] = m_b1Mid * highR - m_a1Mid * midR + m_lpMidR[1];
    m_lpMidR[1] = m_b2Mid * highR - m_a2Mid * midR;

    // Air / High = high portion - mid portion
    float airL = highL - midL;
    float airR = highR - midR;

    // Transient detection for Drum separation
    float mag = 0.5f * (std::abs(inL) + std::abs(inR));
    float fastAlpha = 0.05f; // Fast tracker
    float slowAlpha = 0.005f; // Slow tracker
    m_transientEnv += fastAlpha * (mag - m_transientEnv);
    m_slowEnv += slowAlpha * (mag - m_slowEnv);

    float transientDelta = std::max(0.0f, m_transientEnv - m_slowEnv);
    float drumWeight = std::min(1.0f, transientDelta * 4.0f);

    // Mid/Side analysis in the vocal band
    float midChannel = 0.5f * (midL + midR);
    float sideChannel = 0.5f * (midL - midR);

    // Bass Stem: Mono low-frequency + sub transient
    float bassL = lowL * (1.0f - 0.4f * drumWeight);
    float bassR = lowR * (1.0f - 0.4f * drumWeight);

    // Drum Stem: Percussive transients in low and air bands
    float drumL = (lowL * 0.4f + airL * 0.6f + midL * 0.3f) * drumWeight;
    float drumR = (lowR * 0.4f + airR * 0.6f + midR * 0.3f) * drumWeight;

    // Vocal Stem: Center-panned mid content with smooth transient rejection
    float vocalL = midChannel * (1.0f - drumWeight * 0.5f);
    float vocalR = midChannel * (1.0f - drumWeight * 0.5f);

    // Other Stem: Stereo side content, melodic air, diffuse textures
    float otherL = sideChannel + airL * (1.0f - 0.6f * drumWeight) + midL * (drumWeight * 0.2f);
    float otherR = -sideChannel + airR * (1.0f - 0.6f * drumWeight) + midR * (drumWeight * 0.2f);

    // Determine Solo State
    bool anySolo = false;
    for (int i = 0; i < static_cast<int>(RebalanceStem::COUNT); ++i) {
        if (m_stems[i].solo) {
            anySolo = true;
            break;
        }
    }

    // Apply Gains, Solo, Mute, Pan
    float stemSignalsL[4] = {vocalL, drumL, bassL, otherL};
    float stemSignalsR[4] = {vocalR, drumR, bassR, otherR};

    float outL = 0.0f;
    float outR = 0.0f;

    for (int i = 0; i < 4; ++i) {
        bool active = true;
        if (anySolo) {
            active = m_stems[i].solo;
        } else if (m_stems[i].mute) {
            active = false;
        }

        float gain = active ? dbToLinear(m_stems[i].gainDb) : 0.0f;
        float pan = m_stems[i].pan;
        float panL = (pan <= 0.0f) ? 1.0f : (1.0f - pan);
        float panR = (pan >= 0.0f) ? 1.0f : (1.0f + pan);

        float sL = stemSignalsL[i] * gain * panL;
        float sR = stemSignalsR[i] * gain * panR;

        outL += sL;
        outR += sR;

        // Update RMS meter
        float energy = 0.5f * (sL * sL + sR * sR);
        m_stemRms[i] = 0.95f * m_stemRms[i] + 0.05f * std::sqrt(energy);
    }

    left = outL;
    right = outR;
}

void JZSpectralRebalanceEngine::processBlock(float* left, float* right, int numSamples) {
    if (!left || !right || numSamples <= 0) return;
    for (int i = 0; i < numSamples; ++i) {
        process(left[i], right[i]);
    }
}

} // namespace Jazz
