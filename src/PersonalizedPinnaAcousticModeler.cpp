//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// PersonalizedPinnaAcousticModeler.cpp - Anthropometric HRTF & Ear Canal Modeler
//*****************************************************************************

#include "PersonalizedPinnaAcousticModeler.h"
#include <cmath>
#include <algorithm>

namespace Jazz {

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

JZPersonalizedPinnaAcousticModeler::JZPersonalizedPinnaAcousticModeler(float sampleRate) {
    initialize(sampleRate);
}

void JZPersonalizedPinnaAcousticModeler::initialize(float sampleRate) {
    if (sampleRate < 8000.0f) sampleRate = 48000.0f;
    m_sampleRate = sampleRate;

    m_itdDelayBufL.assign(ITD_BUF_SIZE, 0.0f);
    m_itdDelayBufR.assign(ITD_BUF_SIZE, 0.0f);

    reset();
    updateAcousticFilters();
}

void JZPersonalizedPinnaAcousticModeler::reset() {
    std::fill(m_itdDelayBufL.begin(), m_itdDelayBufL.end(), 0.0f);
    std::fill(m_itdDelayBufR.begin(), m_itdDelayBufR.end(), 0.0f);
    m_itdWritePos = 0;

    m_earCanalFilterL.reset(); m_earCanalFilterR.reset();
    m_pinnaNotchFilterL.reset(); m_pinnaNotchFilterR.reset();
    m_headShadowFilterL.reset(); m_headShadowFilterR.reset();
}

void JZPersonalizedPinnaAcousticModeler::setEarGeometry(const EarGeometry& geom) {
    m_geom = geom;
    m_geom.headCircumferenceCm = std::max(48.0f, std::min(68.0f, m_geom.headCircumferenceCm));
    m_geom.pinnaHeightMm = std::max(40.0f, std::min(85.0f, m_geom.pinnaHeightMm));
    m_geom.conchaDepthMm = std::max(6.0f, std::min(30.0f, m_geom.conchaDepthMm));
    m_geom.earCanalLengthMm = std::max(16.0f, std::min(38.0f, m_geom.earCanalLengthMm));
    updateAcousticFilters();
}

void JZPersonalizedPinnaAcousticModeler::setVirtualSource(const VirtualSource& src) {
    m_source = src;
    updateAcousticFilters();
}

void JZPersonalizedPinnaAcousticModeler::updateAcousticFilters() {
    const float speedOfSound = 343.0f; // m/s

    // 1. Ear canal 1/4-wave acoustic resonance: f = c / (4 * L)
    float canalLengthMeters = (m_geom.earCanalLengthMm * 1e-3f);
    m_earCanalResonanceHz = speedOfSound / (4.0f * canalLengthMeters);
    if (m_earCanalResonanceHz >= m_sampleRate * 0.45f) m_earCanalResonanceHz = m_sampleRate * 0.45f;

    // Design Ear Canal Resonant Peaking Biquad (+7.5 dB boost)
    float canalA = std::pow(10.0f, 7.5f / 40.0f);
    float canalW0 = 2.0f * static_cast<float>(M_PI) * (m_earCanalResonanceHz / m_sampleRate);
    float canalAlpha = std::sin(canalW0) / (2.0f * 2.2f);
    float cos_cw = std::cos(canalW0);

    float b0_c = 1.0f + canalAlpha * canalA;
    float b1_c = -2.0f * cos_cw;
    float b2_c = 1.0f - canalAlpha * canalA;
    float a0_c = 1.0f + canalAlpha / canalA;
    float a1_c = -2.0f * cos_cw;
    float a2_c = 1.0f - canalAlpha / canalA;

    m_earCanalFilterL.b0 = b0_c / a0_c; m_earCanalFilterL.b1 = b1_c / a0_c; m_earCanalFilterL.b2 = b2_c / a0_c;
    m_earCanalFilterL.a1 = a1_c / a0_c; m_earCanalFilterL.a2 = a2_c / a0_c;
    m_earCanalFilterR.b0 = m_earCanalFilterL.b0; m_earCanalFilterR.b1 = m_earCanalFilterL.b1; m_earCanalFilterR.b2 = m_earCanalFilterL.b2;
    m_earCanalFilterR.a1 = m_earCanalFilterL.a1; m_earCanalFilterR.a2 = m_earCanalFilterL.a2;

    // 2. Pinna acoustic notch: shifts upward with source elevation
    float pinnaDimMeters = (m_geom.pinnaHeightMm * 1e-3f) * 0.28f;
    float baseNotch = speedOfSound / (4.0f * pinnaDimMeters);
    // Elevation shift cue: higher elevation shifts notch upward
    float elevNorm = (m_source.elevationDeg + 45.0f) / 135.0f;
    m_pinnaNotchHz = baseNotch * (0.85f + 0.35f * elevNorm);
    if (m_pinnaNotchHz >= m_sampleRate * 0.45f) m_pinnaNotchHz = m_sampleRate * 0.45f;

    // Design Pinna Notch Biquad (-12.0 dB cut)
    float notchA = std::pow(10.0f, -12.0f / 40.0f);
    float notchW0 = 2.0f * static_cast<float>(M_PI) * (m_pinnaNotchHz / m_sampleRate);
    float notchAlpha = std::sin(notchW0) / (2.0f * 3.5f);
    float cos_nw = std::cos(notchW0);

    float b0_n = 1.0f + notchAlpha * notchA;
    float b1_n = -2.0f * cos_nw;
    float b2_n = 1.0f - notchAlpha * notchA;
    float a0_n = 1.0f + notchAlpha / notchA;
    float a1_n = -2.0f * cos_nw;
    float a2_n = 1.0f - notchAlpha / notchA;

    m_pinnaNotchFilterL.b0 = b0_n / a0_n; m_pinnaNotchFilterL.b1 = b1_n / a0_n; m_pinnaNotchFilterL.b2 = b2_n / a0_n;
    m_pinnaNotchFilterL.a1 = a1_n / a0_n; m_pinnaNotchFilterL.a2 = a2_n / a0_n;
    m_pinnaNotchFilterR.b0 = m_pinnaNotchFilterL.b0; m_pinnaNotchFilterR.b1 = m_pinnaNotchFilterL.b1; m_pinnaNotchFilterR.b2 = m_pinnaNotchFilterL.b2;
    m_pinnaNotchFilterR.a1 = m_pinnaNotchFilterL.a1; m_pinnaNotchFilterR.a2 = m_pinnaNotchFilterL.a2;

    // 3. Woodworth ITD calculation based on head radius
    float headRadius = (m_geom.headCircumferenceCm * 0.01f) / (2.0f * static_cast<float>(M_PI));
    float azRad = m_source.azimuthDeg * (static_cast<float>(M_PI) / 180.0f);
    float sinAz = std::sin(std::abs(azRad));
    float itdSec = (headRadius / speedOfSound) * (sinAz + std::abs(azRad) * sinAz * 0.5f);
    m_itdMicroseconds = itdSec * 1e6f;
}

void JZPersonalizedPinnaAcousticModeler::process(
    const float* inL, const float* inR,
    float* outL, float* outR,
    size_t numSamples) {

    if (!inL || !outL || numSamples == 0) return;
    const bool stereo = (inR != nullptr && outR != nullptr);

    float itdSamples = (m_itdMicroseconds / 1e6f) * m_sampleRate;
    if (itdSamples > static_cast<float>(ITD_BUF_SIZE / 2 - 2)) {
        itdSamples = static_cast<float>(ITD_BUF_SIZE / 2 - 2);
    }

    bool delayRight = (m_source.azimuthDeg >= 0.0f); // Source on right delays left ear

    for (size_t i = 0; i < numSamples; ++i) {
        float sampleL = inL[i];
        float sampleR = stereo ? inR[i] : sampleL;

        // Apply Ear Canal Resonance (3.4 kHz boost)
        float canalL = m_earCanalFilterL.processSample(sampleL);
        float canalR = m_earCanalFilterR.processSample(sampleR);

        // Apply Pinna Elevation Notch (6-10 kHz elevation cue)
        float pinnaL = m_pinnaNotchFilterL.processSample(canalL);
        float pinnaR = m_pinnaNotchFilterR.processSample(canalR);

        // Push to ITD ring buffers
        m_itdDelayBufL[m_itdWritePos] = pinnaL;
        m_itdDelayBufR[m_itdWritePos] = pinnaR;

        // Read with fractional delay
        float readPosL = static_cast<float>(m_itdWritePos);
        float readPosR = static_cast<float>(m_itdWritePos);

        if (delayRight) {
            readPosL -= itdSamples;
        } else {
            readPosR -= itdSamples;
        }

        while (readPosL < 0.0f) readPosL += static_cast<float>(ITD_BUF_SIZE);
        while (readPosR < 0.0f) readPosR += static_cast<float>(ITD_BUF_SIZE);

        size_t idxL0 = static_cast<size_t>(readPosL) % ITD_BUF_SIZE;
        size_t idxL1 = (idxL0 + 1) % ITD_BUF_SIZE;
        float fracL = readPosL - static_cast<float>(static_cast<size_t>(readPosL));
        float delayedL = m_itdDelayBufL[idxL0] * (1.0f - fracL) + m_itdDelayBufL[idxL1] * fracL;

        size_t idxR0 = static_cast<size_t>(readPosR) % ITD_BUF_SIZE;
        size_t idxR1 = (idxR0 + 1) % ITD_BUF_SIZE;
        float fracR = readPosR - static_cast<float>(static_cast<size_t>(readPosR));
        float delayedR = m_itdDelayBufR[idxR0] * (1.0f - fracR) + m_itdDelayBufR[idxR1] * fracR;

        m_itdWritePos = (m_itdWritePos + 1) % ITD_BUF_SIZE;

        outL[i] = delayedL;
        if (outR) outR[i] = delayedR;
    }
}

} // namespace Jazz
