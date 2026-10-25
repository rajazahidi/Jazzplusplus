//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// VocalVibratoHumanizerEngine.cpp - Vocal Vibrato & Pitch Drift Humanizer
//*****************************************************************************

#include "VocalVibratoHumanizerEngine.h"
#include <cmath>
#include <algorithm>

namespace Jazz {

static constexpr float PI_F = 3.14159265358979323846f;

JZVocalVibratoHumanizerEngine::JZVocalVibratoHumanizerEngine(float sampleRate)
    : m_sampleRate(sampleRate)
{
    initialize(sampleRate);
}

void JZVocalVibratoHumanizerEngine::initialize(float sampleRate)
{
    m_sampleRate = (sampleRate > 8000.0f) ? sampleRate : 48000.0f;
    m_delayBufL.assign(DELAY_BUF_SIZE, 0.0f);
    m_delayBufR.assign(DELAY_BUF_SIZE, 0.0f);
    reset();
}

void JZVocalVibratoHumanizerEngine::reset()
{
    std::fill(m_delayBufL.begin(), m_delayBufL.end(), 0.0f);
    std::fill(m_delayBufR.begin(), m_delayBufR.end(), 0.0f);
    m_writePos = 0;
    m_readPos = 0.0f;
    m_vibratoPhase = 0.0f;
    m_driftPhase = 0.0f;
    m_noteTimeSec = 0.0f;
    m_onsetEnvelope = 0.0f;
    m_currentPitchOffsetCents = 0.0f;
    m_pinkB0 = m_pinkB1 = m_pinkB2 = 0.0f;
}

void JZVocalVibratoHumanizerEngine::setParameters(const Parameters& params)
{
    m_params = params;
    m_params.vibratoRateHz = std::max(3.5f, std::min(m_params.vibratoRateHz, 8.5f));
    m_params.vibratoDepthCents = std::max(0.0f, std::min(m_params.vibratoDepthCents, 120.0f));
    m_params.onsetDelayMs = std::max(0.0f, std::min(m_params.onsetDelayMs, 1200.0f));
    m_params.pitchDriftCents = std::max(0.0f, std::min(m_params.pitchDriftCents, 25.0f));
    m_params.tremoloAmount = std::max(0.0f, std::min(m_params.tremoloAmount, 0.35f));
    m_params.dryWet = std::max(0.0f, std::min(m_params.dryWet, 1.0f));
}

void JZVocalVibratoHumanizerEngine::setVibratoRateHz(float hz)
{
    m_params.vibratoRateHz = std::max(3.5f, std::min(hz, 8.5f));
}

void JZVocalVibratoHumanizerEngine::setVibratoDepthCents(float cents)
{
    m_params.vibratoDepthCents = std::max(0.0f, std::min(cents, 120.0f));
}

void JZVocalVibratoHumanizerEngine::setOnsetDelayMs(float ms)
{
    m_params.onsetDelayMs = std::max(0.0f, std::min(ms, 1200.0f));
}

void JZVocalVibratoHumanizerEngine::setPitchDriftCents(float cents)
{
    m_params.pitchDriftCents = std::max(0.0f, std::min(cents, 25.0f));
}

void JZVocalVibratoHumanizerEngine::setTremoloAmount(float tremolo)
{
    m_params.tremoloAmount = std::max(0.0f, std::min(tremolo, 0.35f));
}

void JZVocalVibratoHumanizerEngine::setDryWet(float wet)
{
    m_params.dryWet = std::max(0.0f, std::min(wet, 1.0f));
}

void JZVocalVibratoHumanizerEngine::setBypass(bool bypass)
{
    m_params.bypassed = bypass;
}

void JZVocalVibratoHumanizerEngine::triggerNoteOn()
{
    m_noteTimeSec = 0.0f;
    m_onsetEnvelope = 0.0f;
}

void JZVocalVibratoHumanizerEngine::process(
    const float* inL, const float* inR,
    float* outL, float* outR,
    size_t numSamples)
{
    if (!inL || !inR || !outL || !outR || numSamples == 0) return;

    if (m_params.bypassed) {
        std::copy(inL, inL + numSamples, outL);
        std::copy(inR, inR + numSamples, outR);
        return;
    }

    float phaseInc = 2.0f * PI_F * m_params.vibratoRateHz / m_sampleRate;
    float driftPhaseInc = 2.0f * PI_F * 0.45f / m_sampleRate;
    float onsetSec = m_params.onsetDelayMs * 0.001f;
    float dt = 1.0f / m_sampleRate;
    static constexpr float BASE_DELAY_SAMPLES = 512.0f;

    for (size_t i = 0; i < numSamples; ++i) {
        float xL = inL[i];
        float xR = inR[i];

        // Write stereo input into circular buffer
        m_delayBufL[m_writePos] = xL;
        m_delayBufR[m_writePos] = xR;

        // Onset envelope ramp
        m_noteTimeSec += dt;
        if (onsetSec > 0.01f) {
            float ramp = (m_noteTimeSec - onsetSec * 0.25f) / (onsetSec * 0.75f);
            m_onsetEnvelope = std::max(0.0f, std::min(ramp, 1.0f));
        } else {
            m_onsetEnvelope = 1.0f;
        }

        // Fractal 1/f slow pitch drift
        m_driftPhase += driftPhaseInc;
        if (m_driftPhase >= 2.0f * PI_F) m_driftPhase -= 2.0f * PI_F;
        float slowDrift = std::sin(m_driftPhase) + 0.35f * std::sin(m_driftPhase * 2.3f);

        // Sinusoidal vibrato LFO
        m_vibratoPhase += phaseInc;
        if (m_vibratoPhase >= 2.0f * PI_F) m_vibratoPhase -= 2.0f * PI_F;
        float vibSin = std::sin(m_vibratoPhase);

        // Instantaneous pitch offset in cents
        float vibCents = vibSin * m_params.vibratoDepthCents * m_onsetEnvelope;
        float driftCents = slowDrift * m_params.pitchDriftCents * m_onsetEnvelope;
        m_currentPitchOffsetCents = vibCents + driftCents;

        // Convert cents modulation to delay sample modulation:
        // delta_t = (2^(cents/1200) - 1) / (2*pi*f_vib) * fs
        float semitones = m_currentPitchOffsetCents / 100.0f;
        float delayModSamples = -(semitones / 12.0f) * (m_sampleRate / (2.0f * PI_F * m_params.vibratoRateHz));

        float readDelay = BASE_DELAY_SAMPLES + delayModSamples;
        readDelay = std::max(4.0f, std::min(readDelay, static_cast<float>(DELAY_BUF_SIZE - 8)));

        // Read fractional sample with linear interpolation
        float rPos = static_cast<float>(m_writePos) - readDelay;
        if (rPos < 0.0f) rPos += DELAY_BUF_SIZE;
        int idx0 = static_cast<int>(rPos);
        int idx1 = (idx0 + 1) % DELAY_BUF_SIZE;
        float frac = rPos - static_cast<float>(idx0);

        float modL = m_delayBufL[idx0] * (1.0f - frac) + m_delayBufL[idx1] * frac;
        float modR = m_delayBufR[idx0] * (1.0f - frac) + m_delayBufR[idx1] * frac;

        // Amplitude tremolo coupling
        if (m_params.tremoloAmount > 0.0f) {
            float trem = 1.0f + m_params.tremoloAmount * vibSin * m_onsetEnvelope;
            modL *= trem;
            modR *= trem;
        }

        // Dry/Wet blend
        outL[i] = xL * (1.0f - m_params.dryWet) + modL * m_params.dryWet;
        outR[i] = xR * (1.0f - m_params.dryWet) + modR * m_params.dryWet;

        m_writePos = (m_writePos + 1) % DELAY_BUF_SIZE;
    }
}

} // namespace Jazz
