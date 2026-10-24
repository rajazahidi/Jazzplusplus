#include "AnalogTapeFlutterModeler.h"
#include <algorithm>
#include <cmath>

namespace Jazz {

static const float PI = 3.14159265358979323846f;

JZAnalogTapeFlutterModeler::JZAnalogTapeFlutterModeler(float sampleRate)
    : m_sampleRate(sampleRate > 1000.0f ? sampleRate : 44100.0f)
    , m_tapeSpeed(TapeSpeedIPS::IPS_15)
    , m_scrapeFlutter(0.35f)
    , m_pinchRoller(0.40f)
    , m_tensionSag(0.30f)
    , m_dropoutRate(0.15f)
    , m_mix(1.0f)
    , m_writeIdx(0)
    , m_scrapePhase(0.0f)
    , m_pinchPhase(0.0f)
    , m_chaoticNoise(0.0f)
    , m_transientSagEnv(0.0f)
    , m_dropoutGainL(1.0f)
    , m_dropoutGainR(1.0f)
    , m_dropoutFilterL(0.0f)
    , m_dropoutFilterR(0.0f)
    , m_currentVarianceCents(0.0f)
    , m_rng(987654321)
{
    reset();
}

uint32_t JZAnalogTapeFlutterModeler::xorshift32() {
    uint32_t x = m_rng;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    m_rng = x;
    return x;
}

float JZAnalogTapeFlutterModeler::randomFloat() {
    return static_cast<float>(xorshift32() & 0x00FFFFFF) / static_cast<float>(0x01000000);
}

void JZAnalogTapeFlutterModeler::setSampleRate(float sampleRate) {
    if (sampleRate > 1000.0f) {
        m_sampleRate = sampleRate;
    }
}

void JZAnalogTapeFlutterModeler::setTapeSpeed(TapeSpeedIPS speed) {
    m_tapeSpeed = speed;
}

void JZAnalogTapeFlutterModeler::setScrapeFlutter(float amount) {
    m_scrapeFlutter = std::max(0.0f, std::min(1.0f, amount));
}

void JZAnalogTapeFlutterModeler::setPinchRollerEccentricity(float amount) {
    m_pinchRoller = std::max(0.0f, std::min(1.0f, amount));
}

void JZAnalogTapeFlutterModeler::setTensionSag(float amount) {
    m_tensionSag = std::max(0.0f, std::min(1.0f, amount));
}

void JZAnalogTapeFlutterModeler::setDropoutRate(float rate) {
    m_dropoutRate = std::max(0.0f, std::min(1.0f, rate));
}

void JZAnalogTapeFlutterModeler::setMix(float mix) {
    m_mix = std::max(0.0f, std::min(1.0f, mix));
}

void JZAnalogTapeFlutterModeler::reset() {
    for (int i = 0; i < DELAY_SIZE; ++i) {
        m_delayBufL[i] = 0.0f;
        m_delayBufR[i] = 0.0f;
    }
    m_writeIdx = 0;
    m_scrapePhase = 0.0f;
    m_pinchPhase = 0.0f;
    m_chaoticNoise = 0.0f;
    m_transientSagEnv = 0.0f;
    m_dropoutGainL = 1.0f;
    m_dropoutGainR = 1.0f;
    m_dropoutFilterL = 0.0f;
    m_dropoutFilterR = 0.0f;
    m_currentVarianceCents = 0.0f;
}

float JZAnalogTapeFlutterModeler::fractionalDelay(const float* buffer, int bufSize, int writeIdx, float delaySamples) const {
    float readPos = static_cast<float>(writeIdx) - delaySamples;
    while (readPos < 0.0f) readPos += static_cast<float>(bufSize);

    int idx0 = static_cast<int>(readPos) % bufSize;
    int idx1 = (idx0 + 1) % bufSize;
    float frac = readPos - std::floor(readPos);

    return buffer[idx0] * (1.0f - frac) + buffer[idx1] * frac;
}

void JZAnalogTapeFlutterModeler::process(float& left, float& right) {
    float inL = left;
    float inR = right;

    // Physical speed constants
    float scrapeFreq = 3100.0f;
    float speedFactor = 1.0f;

    switch (m_tapeSpeed) {
        case TapeSpeedIPS::IPS_7_5:
            scrapeFreq = 2200.0f;
            speedFactor = 1.8f;
            break;
        case TapeSpeedIPS::IPS_15:
            scrapeFreq = 3100.0f;
            speedFactor = 1.0f;
            break;
        case TapeSpeedIPS::IPS_30:
            scrapeFreq = 4200.0f;
            speedFactor = 0.55f;
            break;
    }

    // 1. Scrape Flutter: Micro-friction longitudinal tape oscillation (2.2kHz - 4.2kHz)
    m_scrapePhase += (2.0f * PI * scrapeFreq) / m_sampleRate;
    if (m_scrapePhase >= 2.0f * PI) m_scrapePhase -= 2.0f * PI;
    float scrapeMod = std::sin(m_scrapePhase) * (m_scrapeFlutter * 0.35f);

    // 2. Pinch Roller Eccentricity: 5.5 Hz rotation wobble
    float pinchFreq = 5.5f * (m_tapeSpeed == TapeSpeedIPS::IPS_30 ? 1.5f : (m_tapeSpeed == TapeSpeedIPS::IPS_7_5 ? 0.65f : 1.0f));
    m_pinchPhase += (2.0f * PI * pinchFreq) / m_sampleRate;
    if (m_pinchPhase >= 2.0f * PI) m_pinchPhase -= 2.0f * PI;
    float pinchMod = (std::sin(m_pinchPhase) + 0.3f * std::sin(m_pinchPhase * 2.0f)) * (m_pinchRoller * 2.8f);

    // 3. Chaotic Brownian Noise Motor Drift
    float white = (randomFloat() - 0.5f) * 2.0f;
    m_chaoticNoise = 0.992f * m_chaoticNoise + 0.008f * white;

    // 4. Tape Tension Sag (loud transients stretch elastic tape backing)
    float inputMag = 0.5f * (std::abs(inL) + std::abs(inR));
    if (inputMag > m_transientSagEnv) {
        m_transientSagEnv += 0.05f * (inputMag - m_transientSagEnv);
    } else {
        m_transientSagEnv += 0.001f * (inputMag - m_transientSagEnv);
    }
    float sagMod = m_transientSagEnv * (m_tensionSag * 1.8f);

    // Aggregate mechanical delay modulation around nominal 64 samples
    float totalModSamples = (pinchMod + scrapeMod + m_chaoticNoise * 2.5f + sagMod) * speedFactor;
    float nominalDelay = 64.0f;
    float targetDelayL = std::max(2.0f, std::min(static_cast<float>(DELAY_SIZE - 2), nominalDelay + totalModSamples));
    float targetDelayR = std::max(2.0f, std::min(static_cast<float>(DELAY_SIZE - 2), nominalDelay + totalModSamples * 0.95f));

    // Telemetry: instantaneous speed variance in cents
    m_currentVarianceCents = (totalModSamples / nominalDelay) * 1200.0f;

    // Write input into mechanical delay line
    m_delayBufL[m_writeIdx] = inL;
    m_delayBufR[m_writeIdx] = inR;

    float wetL = fractionalDelay(m_delayBufL, DELAY_SIZE, m_writeIdx, targetDelayL);
    float wetR = fractionalDelay(m_delayBufR, DELAY_SIZE, m_writeIdx, targetDelayR);

    m_writeIdx = (m_writeIdx + 1) % DELAY_SIZE;

    // 5. Magnetic Particle Dropout Simulation
    if (m_dropoutRate > 0.01f) {
        if (randomFloat() < (m_dropoutRate * 0.00015f)) {
            // Trigger micro-dropout
            m_dropoutGainL = 0.35f + 0.3f * randomFloat();
            m_dropoutGainR = 0.35f + 0.3f * randomFloat();
        } else {
            // Recover smoothly
            m_dropoutGainL += 0.004f * (1.0f - m_dropoutGainL);
            m_dropoutGainR += 0.004f * (1.0f - m_dropoutGainR);
        }

        // Apply dropout gain and head-lift loss filter
        m_dropoutFilterL = 0.4f * wetL + 0.6f * m_dropoutFilterL;
        m_dropoutFilterR = 0.4f * wetR + 0.6f * m_dropoutFilterR;

        wetL = (wetL * m_dropoutGainL + m_dropoutFilterL * (1.0f - m_dropoutGainL));
        wetR = (wetR * m_dropoutGainR + m_dropoutFilterR * (1.0f - m_dropoutGainR));
    }

    left  = (1.0f - m_mix) * inL + m_mix * wetL;
    right = (1.0f - m_mix) * inR + m_mix * wetR;
}

void JZAnalogTapeFlutterModeler::processBlock(float* left, float* right, int numSamples) {
    if (!left || !right || numSamples <= 0) return;
    for (int i = 0; i < numSamples; ++i) {
        process(left[i], right[i]);
    }
}

} // namespace Jazz
