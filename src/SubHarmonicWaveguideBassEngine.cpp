#include "SubHarmonicWaveguideBassEngine.h"
#include <cmath>
#include <algorithm>

namespace Jazz {

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

JZSubHarmonicWaveguideBassEngine::JZSubHarmonicWaveguideBassEngine() {
    initialize(48000.0f);
}

void JZSubHarmonicWaveguideBassEngine::initialize(float sampleRate) {
    if (sampleRate < 8000.0f) sampleRate = 48000.0f;
    m_sampleRate = sampleRate;

    size_t maxLen = static_cast<size_t>(sampleRate / 15.0f) + 16;
    m_waveguideL.assign(maxLen, 0.0f);
    m_waveguideR.assign(maxLen, 0.0f);
    reset();
    updateFilters();
}

void JZSubHarmonicWaveguideBassEngine::reset() {
    m_prevSignL = false;
    m_prevSignR = false;
    m_div1StateL = false;
    m_div1StateR = false;
    m_div2StateL = false;
    m_div2StateR = false;
    m_zeroCrossingCountL = 0;
    m_zeroCrossingCountR = 0;

    m_bpFilterL.reset();
    m_bpFilterR.reset();
    m_hpFilterL.reset();
    m_hpFilterR.reset();

    std::fill(m_waveguideL.begin(), m_waveguideL.end(), 0.0f);
    std::fill(m_waveguideR.begin(), m_waveguideR.end(), 0.0f);
    m_wgIndexL = 0;
    m_wgIndexR = 0;

    m_subEnergyPeak = 0.0f;
    m_subEnergyRms = 0.0f;
    m_lockAcquired = false;
}

void JZSubHarmonicWaveguideBassEngine::setParameters(const Parameters& params) {
    m_params = params;
    m_params.subOctave1Level = std::max(0.0f, std::min(1.0f, m_params.subOctave1Level));
    m_params.subOctave2Level = std::max(0.0f, std::min(1.0f, m_params.subOctave2Level));
    m_params.centerFrequencyHz = std::max(20.0f, std::min(90.0f, m_params.centerFrequencyHz));
    m_params.resonanceQ = std::max(1.0f, std::min(12.0f, m_params.resonanceQ));
    m_params.drive = std::max(1.0f, std::min(5.0f, m_params.drive));
    m_params.lowCutHz = std::max(15.0f, std::min(35.0f, m_params.lowCutHz));
    m_params.dryWet = std::max(0.0f, std::min(1.0f, m_params.dryWet));

    updateFilters();
}

void JZSubHarmonicWaveguideBassEngine::updateFilters() {
    // 1. Resonant Bandpass filter coefficients (constant skirt gain, peak gain = Q)
    float f0 = m_params.centerFrequencyHz;
    float Q = m_params.resonanceQ;
    float w0 = 2.0f * static_cast<float>(M_PI) * (f0 / m_sampleRate);
    float cos_w0 = std::cos(w0);
    float sin_w0 = std::sin(w0);
    float alpha = sin_w0 / (2.0f * Q);

    float b0_bp = sin_w0 / 2.0f;
    float b1_bp = 0.0f;
    float b2_bp = -sin_w0 / 2.0f;
    float a0_bp = 1.0f + alpha;
    float a1_bp = -2.0f * cos_w0;
    float a2_bp = 1.0f - alpha;

    m_bpFilterL.b0 = b0_bp / a0_bp;
    m_bpFilterL.b1 = b1_bp / a0_bp;
    m_bpFilterL.b2 = b2_bp / a0_bp;
    m_bpFilterL.a1 = a1_bp / a0_bp;
    m_bpFilterL.a2 = a2_bp / a0_bp;

    m_bpFilterR.b0 = m_bpFilterL.b0;
    m_bpFilterR.b1 = m_bpFilterL.b1;
    m_bpFilterR.b2 = m_bpFilterL.b2;
    m_bpFilterR.a1 = m_bpFilterL.a1;
    m_bpFilterR.a2 = m_bpFilterL.a2;

    // 2. Highpass Filter at lowCutHz (Butterworth Q = 0.7071)
    float hpF0 = m_params.lowCutHz;
    float hpW0 = 2.0f * static_cast<float>(M_PI) * (hpF0 / m_sampleRate);
    float hpCos = std::cos(hpW0);
    float hpSin = std::sin(hpW0);
    float hpAlpha = hpSin / (2.0f * 0.70710678f);

    float b0_hp = (1.0f + hpCos) / 2.0f;
    float b1_hp = -(1.0f + hpCos);
    float b2_hp = (1.0f + hpCos) / 2.0f;
    float a0_hp = 1.0f + hpAlpha;
    float a1_hp = -2.0f * hpCos;
    float a2_hp = 1.0f - hpAlpha;

    m_hpFilterL.b0 = b0_hp / a0_hp;
    m_hpFilterL.b1 = b1_hp / a0_hp;
    m_hpFilterL.b2 = b2_hp / a0_hp;
    m_hpFilterL.a1 = a1_hp / a0_hp;
    m_hpFilterL.a2 = a2_hp / a0_hp;

    m_hpFilterR.b0 = m_hpFilterL.b0;
    m_hpFilterR.b1 = m_hpFilterL.b1;
    m_hpFilterR.b2 = m_hpFilterL.b2;
    m_hpFilterR.a1 = m_hpFilterL.a1;
    m_hpFilterR.a2 = m_hpFilterL.a2;

    // 3. Waveguide resonant cavity length
    float delaySamples = m_sampleRate / m_params.centerFrequencyHz;
    m_wgLength = static_cast<size_t>(delaySamples);
    if (m_wgLength < 16) m_wgLength = 16;
    if (m_wgLength >= m_waveguideL.size()) m_wgLength = m_waveguideL.size() - 1;

    m_wgFeedback = 0.5f + (m_params.resonanceQ / 12.0f) * 0.45f;
    if (m_wgFeedback > 0.96f) m_wgFeedback = 0.96f;
}

void JZSubHarmonicWaveguideBassEngine::process(const float* inL, const float* inR,
                                             float* outL, float* outR,
                                             size_t numSamples) {
    if (!inL || !outL || numSamples == 0) return;

    const bool stereo = (inR != nullptr && outR != nullptr);
    float subEnergySumSq = 0.0f;
    float maxSubPeak = 0.0f;
    int activeCycles = 0;

    float phaseSign = m_params.invertPhase ? -1.0f : 1.0f;

    for (size_t i = 0; i < numSamples; ++i) {
        float sampleL = inL[i];
        float sampleR = stereo ? inR[i] : sampleL;

        // Step 1: Bandpass the input around resonant sub-band
        float bpL = m_bpFilterL.processSample(sampleL);
        float bpR = stereo ? m_bpFilterR.processSample(sampleR) : bpL;

        // Step 2: Zero-crossing detector & flip-flop octave division for Left
        bool signL = (bpL >= 0.0f);
        if (signL && !m_prevSignL) {
            // Rising zero-crossing detected
            m_div1StateL = !m_div1StateL;
            m_zeroCrossingCountL++;
            if ((m_zeroCrossingCountL & 1) == 0) {
                m_div2StateL = !m_div2StateL;
            }
            activeCycles++;
        }
        m_prevSignL = signL;

        // Amplitude tracking from bandpassed signal
        float envL = std::abs(bpL);

        // Generate Sub Octave 1 (-1 Oct) and Sub Octave 2 (-2 Oct) square-shaped pulse modulated by envelope
        float sub1L = (m_div1StateL ? 1.0f : -1.0f) * envL * m_params.subOctave1Level;
        float sub2L = (m_div2StateL ? 1.0f : -1.0f) * envL * m_params.subOctave2Level;
        float combinedSubL = sub1L + sub2L;

        // Step 3: Zero-crossing & octave division for Right channel
        float combinedSubR = combinedSubL;
        if (stereo) {
            bool signR = (bpR >= 0.0f);
            if (signR && !m_prevSignR) {
                m_div1StateR = !m_div1StateR;
                m_zeroCrossingCountR++;
                if ((m_zeroCrossingCountR & 1) == 0) {
                    m_div2StateR = !m_div2StateR;
                }
            }
            m_prevSignR = signR;
            float envR = std::abs(bpR);
            float sub1R = (m_div1StateR ? 1.0f : -1.0f) * envR * m_params.subOctave1Level;
            float sub2R = (m_div2StateR ? 1.0f : -1.0f) * envR * m_params.subOctave2Level;
            combinedSubR = sub1R + sub2R;
        }

        // Step 4: Inject into Acoustic Waveguide resonator delay line
        float wgReadL = m_waveguideL[m_wgIndexL];
        float wgWriteL = combinedSubL + wgReadL * m_wgFeedback;
        m_waveguideL[m_wgIndexL] = wgWriteL;
        m_wgIndexL = (m_wgIndexL + 1) % m_wgLength;
        float resonantSubL = wgReadL * 0.7f + combinedSubL * 0.3f;

        float resonantSubR = resonantSubL;
        if (stereo) {
            float wgReadR = m_waveguideR[m_wgIndexR];
            float wgWriteR = combinedSubR + wgReadR * m_wgFeedback;
            m_waveguideR[m_wgIndexR] = wgWriteR;
            m_wgIndexR = (m_wgIndexR + 1) % m_wgLength;
            resonantSubR = wgReadR * 0.7f + combinedSubR * 0.3f;
        }

        // Step 5: Soft saturation drive for harmonic projection
        float driveVal = m_params.drive;
        float drivenSubL = std::tanh(resonantSubL * driveVal);
        float drivenSubR = std::tanh(resonantSubR * driveVal);

        // Step 6: Infrasonic lowcut highpass filter
        float cleanSubL = m_hpFilterL.processSample(drivenSubL) * phaseSign;
        float cleanSubR = stereo ? (m_hpFilterR.processSample(drivenSubR) * phaseSign) : cleanSubL;

        // Telemetry accumulation
        float absSub = (std::abs(cleanSubL) + std::abs(cleanSubR)) * 0.5f;
        if (absSub > maxSubPeak) maxSubPeak = absSub;
        subEnergySumSq += (cleanSubL * cleanSubL + cleanSubR * cleanSubR) * 0.5f;

        // Step 7: Output mixing (dry / wet / solo)
        if (m_params.soloSub) {
            outL[i] = cleanSubL;
            if (stereo) outR[i] = cleanSubR;
        } else {
            float dryWeight = 1.0f - m_params.dryWet;
            float wetWeight = m_params.dryWet;
            outL[i] = sampleL * dryWeight + cleanSubL * wetWeight;
            if (stereo) {
                outR[i] = sampleR * dryWeight + cleanSubR * wetWeight;
            }
        }
    }

    // Telemetry update
    m_subEnergyPeak = maxSubPeak;
    m_subEnergyRms = std::sqrt(subEnergySumSq / static_cast<float>(numSamples));
    m_lockAcquired = (activeCycles > 0) && (m_subEnergyRms > 1e-4f);
}

} // namespace Jazz
