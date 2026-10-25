//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// TonewheelOrganDrawbarEngine.cpp - Electromechanical 91-Tonewheel Generator
// & 9-Drawbar Organ Engine
//*****************************************************************************

#include "TonewheelOrganDrawbarEngine.h"
#include <sstream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Jazz {

constexpr std::array<int, JZTonewheelOrganDrawbarEngine::NUM_DRAWBARS> JZTonewheelOrganDrawbarEngine::DRAWBAR_SEMITONE_OFFSETS;

JZTonewheelOrganDrawbarEngine::JZTonewheelOrganDrawbarEngine(float sampleRate)
    : m_sampleRate(sampleRate > 1000.0f ? sampleRate : 48000.0f)
{
    initialize(m_sampleRate);
}

void JZTonewheelOrganDrawbarEngine::initialize(float sampleRate)
{
    m_sampleRate = sampleRate > 1000.0f ? sampleRate : 48000.0f;
    initTonewheels();
    reset();
}

void JZTonewheelOrganDrawbarEngine::initTonewheels()
{
    // Hammond tonewheel generator has 91 active wheels (wheel 1: C1 32.7Hz to wheel 91: F#8 5919Hz)
    // Synchronous motor turns main shaft at 1200 RPM (20 rev/sec) driving 12 gear sets
    for (int i = 0; i < NUM_TONEWHEELS; ++i) {
        // Approximate equal-temperament tuning matching 12 gear ratios
        // Wheel 0 = C1 (approx 32.703 Hz)
        float freq = 440.0f * std::pow(2.0f, (static_cast<float>(i + 24) - 69.0f) / 12.0f);
        m_tonewheelFrequencies[i] = freq;
        m_tonewheelPhases[i] = 0.0f;
    }
}

void JZTonewheelOrganDrawbarEngine::reset()
{
    for (int i = 0; i < NUM_TONEWHEELS; ++i) {
        m_tonewheelPhases[i] = 0.0f;
    }
    for (auto& v : m_voices) {
        v.active = false;
        v.midiNote = -1;
        v.velocity = 0.0f;
        v.keyClickEnv = 0.0f;
    }
    m_percussionEnv = 0.0f;
    m_percussionTriggered = false;
}

void JZTonewheelOrganDrawbarEngine::setParameters(const Parameters& params)
{
    m_params = params;
    for (int i = 0; i < NUM_DRAWBARS; ++i) {
        m_params.drawbars[i] = std::max(0, std::min(m_params.drawbars[i], 8));
    }
    m_params.keyClickAmount = std::max(0.0f, std::min(m_params.keyClickAmount, 1.0f));
    m_params.leakageCrosstalk = std::max(0.0f, std::min(m_params.leakageCrosstalk, 0.5f));
    m_params.masterVolume = std::max(0.0f, std::min(m_params.masterVolume, 2.0f));
}

void JZTonewheelOrganDrawbarEngine::setDrawbar(int drawbarIndex, int value0to8)
{
    if (drawbarIndex >= 0 && drawbarIndex < NUM_DRAWBARS) {
        m_params.drawbars[drawbarIndex] = std::max(0, std::min(value0to8, 8));
    }
}

int JZTonewheelOrganDrawbarEngine::getDrawbar(int drawbarIndex) const
{
    if (drawbarIndex >= 0 && drawbarIndex < NUM_DRAWBARS) {
        return m_params.drawbars[drawbarIndex];
    }
    return 0;
}

void JZTonewheelOrganDrawbarEngine::setAllDrawbars(const std::array<int, NUM_DRAWBARS>& values)
{
    for (int i = 0; i < NUM_DRAWBARS; ++i) {
        m_params.drawbars[i] = std::max(0, std::min(values[i], 8));
    }
}

void JZTonewheelOrganDrawbarEngine::setPercussionEnabled(bool enabled)
{
    m_params.percussionEnabled = enabled;
}

void JZTonewheelOrganDrawbarEngine::setPercussionHarmonic(OrganPercussionHarmonic harm)
{
    m_params.percussionHarmonic = harm;
}

void JZTonewheelOrganDrawbarEngine::setPercussionDecay(OrganPercussionDecay decay)
{
    m_params.percussionDecay = decay;
}

void JZTonewheelOrganDrawbarEngine::setPercussionSoft(bool soft)
{
    m_params.percussionSoft = soft;
}

void JZTonewheelOrganDrawbarEngine::setKeyClickAmount(float click)
{
    m_params.keyClickAmount = std::max(0.0f, std::min(click, 1.0f));
}

void JZTonewheelOrganDrawbarEngine::setLeakageCrosstalk(float leak)
{
    m_params.leakageCrosstalk = std::max(0.0f, std::min(leak, 0.5f));
}

void JZTonewheelOrganDrawbarEngine::setMasterVolume(float vol)
{
    m_params.masterVolume = std::max(0.0f, std::min(vol, 2.0f));
}

float JZTonewheelOrganDrawbarEngine::getDrawbarGain(int drawbarIndex) const
{
    int val = getDrawbar(drawbarIndex);
    if (val == 0) return 0.0f;
    // Classic Hammond drawbar attenuation: step 8 = 0dB, step 1 = -21dB (~3dB per step)
    float db = (val - 8) * 3.0f;
    return std::pow(10.0f, db / 20.0f);
}

std::string JZTonewheelOrganDrawbarEngine::getPresetRegistrationString() const
{
    std::ostringstream oss;
    for (int i = 0; i < NUM_DRAWBARS; ++i) {
        oss << m_params.drawbars[i];
        if (i == 1 || i == 3) oss << " ";
    }
    return oss.str(); // e.g. "88 80 00000" (Classic Jimmy Smith jazz registration)
}

int JZTonewheelOrganDrawbarEngine::midiNoteToTonewheel(int midiNote, int drawbarIndex) const
{
    int semiOffset = DRAWBAR_SEMITONE_OFFSETS[drawbarIndex];
    int targetMidi = midiNote + semiOffset;
    // Map MIDI note (where C1 is 24) to tonewheel index 0..90
    int wheel = targetMidi - 24;
    // Hammond tonewheel foldback in lowest and highest octaves
    while (wheel < 0) wheel += 12;
    while (wheel >= NUM_TONEWHEELS) wheel -= 12;
    return wheel;
}

void JZTonewheelOrganDrawbarEngine::noteOn(int midiNote, float velocity)
{
    if (midiNote < 0 || midiNote >= 128) return;

    bool hadActiveVoices = (getActiveVoicesCount() > 0);

    Voice& v = m_voices[midiNote];
    v.active = true;
    v.midiNote = midiNote;
    v.velocity = std::max(0.1f, std::min(velocity, 1.0f));
    v.keyClickEnv = 1.0f; // Initial mechanical click pulse
    v.clickSign = (midiNote % 2 == 0) ? 1.0f : -1.0f;

    // Hammond single-trigger percussion triggers only on detached legato (no keys currently held)
    if (m_params.percussionEnabled && !hadActiveVoices) {
        m_percussionEnv = 1.0f;
        m_percussionTriggered = true;
    }
}

void JZTonewheelOrganDrawbarEngine::noteOff(int midiNote)
{
    if (midiNote < 0 || midiNote >= 128) return;
    Voice& v = m_voices[midiNote];
    if (v.active) {
        v.active = false;
        v.keyClickEnv = 0.5f; // Release contact click
    }
}

void JZTonewheelOrganDrawbarEngine::allNotesOff()
{
    for (auto& v : m_voices) {
        v.active = false;
        v.keyClickEnv = 0.0f;
    }
    m_percussionEnv = 0.0f;
    m_percussionTriggered = false;
}

int JZTonewheelOrganDrawbarEngine::getActiveVoicesCount() const
{
    int count = 0;
    for (const auto& v : m_voices) {
        if (v.active) count++;
    }
    return count;
}

void JZTonewheelOrganDrawbarEngine::process(float* outL, float* outR, size_t numSamples)
{
    if (!outL || !outR || numSamples == 0) return;

    // Precalculate drawbar gains
    std::array<float, NUM_DRAWBARS> drawbarGains;
    for (int d = 0; d < NUM_DRAWBARS; ++d) {
        drawbarGains[d] = getDrawbarGain(d);
    }

    // Percussion decay rate
    float percDecayTime = (m_params.percussionDecay == OrganPercussionDecay::FAST) ? 0.15f : 0.80f;
    float percDecayFactor = std::exp(-1.0f / (m_sampleRate * percDecayTime));
    float percGain = m_params.percussionSoft ? 0.707f : 1.0f;
    int percDrawbarIdx = (m_params.percussionHarmonic == OrganPercussionHarmonic::SECOND) ? 3 : 4; // 4' or 2 2/3'

    float keyClickDecay = std::exp(-1.0f / (m_sampleRate * 0.003f)); // 3ms mechanical click

    for (size_t s = 0; s < numSamples; ++s) {
        // 1. Advance all 91 tonewheels
        for (int w = 0; w < NUM_TONEWHEELS; ++w) {
            float phaseInc = (2.0f * static_cast<float>(M_PI) * m_tonewheelFrequencies[w]) / m_sampleRate;
            m_tonewheelPhases[w] += phaseInc;
            if (m_tonewheelPhases[w] >= 2.0f * static_cast<float>(M_PI)) {
                m_tonewheelPhases[w] -= 2.0f * static_cast<float>(M_PI);
            }
        }

        float mixAcc = 0.0f;
        float clickAcc = 0.0f;

        // 2. Synthesize active voices
        for (int n = 0; n < 128; ++n) {
            Voice& v = m_voices[n];
            if (!v.active && v.keyClickEnv < 0.001f) continue;

            if (v.active) {
                // Sum the 9 drawbars for this key
                for (int d = 0; d < NUM_DRAWBARS; ++d) {
                    float gain = drawbarGains[d];
                    if (gain <= 0.0001f) continue;

                    int wheel = midiNoteToTonewheel(n, d);
                    float sinVal = std::sin(m_tonewheelPhases[wheel]);
                    mixAcc += sinVal * gain * 0.12f;

                    // Magnetic crosstalk leakage from adjacent tonewheels
                    if (m_params.leakageCrosstalk > 0.001f) {
                        int adjWheel1 = (wheel + 1) % NUM_TONEWHEELS;
                        int adjWheel2 = (wheel > 0) ? (wheel - 1) : (NUM_TONEWHEELS - 1);
                        float leak1 = std::sin(m_tonewheelPhases[adjWheel1]);
                        float leak2 = std::sin(m_tonewheelPhases[adjWheel2]);
                        mixAcc += (leak1 + leak2) * (gain * m_params.leakageCrosstalk * 0.05f);
                    }
                }

                // Percussion voice
                if (m_params.percussionEnabled && m_percussionEnv > 0.001f) {
                    int pWheel = midiNoteToTonewheel(n, percDrawbarIdx);
                    float pSin = std::sin(m_tonewheelPhases[pWheel]);
                    mixAcc += pSin * (m_percussionEnv * percGain * 0.45f);
                }
            }

            // Key contact mechanical click
            if (v.keyClickEnv > 0.001f && m_params.keyClickAmount > 0.001f) {
                float noise = (static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 2.0f - 1.0f);
                clickAcc += noise * v.keyClickEnv * m_params.keyClickAmount * 0.15f * v.clickSign;
                v.keyClickEnv *= keyClickDecay;
            }
        }

        // Advance percussion envelope
        if (m_percussionEnv > 0.0001f) {
            m_percussionEnv *= percDecayFactor;
        }

        float totalSample = (mixAcc + clickAcc) * m_params.masterVolume;
        // Subtle organ tube preamp warmth
        totalSample = std::tanh(totalSample);

        outL[s] = totalSample;
        outR[s] = totalSample;
    }
}

} // namespace Jazz
