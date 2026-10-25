#include "GuitarAmpProfiler.h"
#include <algorithm>
#include <cmath>

namespace Jazz {

static const float PI = 3.14159265358979323846f;

JZGuitarAmpProfiler::JZGuitarAmpProfiler(float sampleRate)
    : m_sampleRate(sampleRate > 1000.0f ? sampleRate : 44100.0f)
    , m_ampModel(AmpTubeModel::CRUNCH_BRITISH_EL34)
    , m_gain(6.0f)
    , m_masterVolume(7.0f)
    , m_powerSag(0.45f)
    , m_bassVal(5.5f)
    , m_midVal(6.0f)
    , m_trebleVal(6.5f)
    , m_presenceVal(5.0f)
    , m_cabBlend(0.35f)
    , m_cabBypass(false)
    , m_sagEnv(0.0f)
    , m_currentSagAmount(0.0f)
    , m_irWriteA(0)
    , m_irWriteB(0)
{
    m_slotA = {CabinetProfile::CAB_4x12_VINTAGE_30, MicPlacement::CENTER_CAP, 2.5f, 1.0f, false};
    m_slotB = {CabinetProfile::CAB_2x12_GREENBACK, MicPlacement::OFF_AXIS_45, 5.0f, 1.0f, false};

    reset();
    updateFilters();
}

void JZGuitarAmpProfiler::setSampleRate(float sampleRate) {
    if (sampleRate > 1000.0f) {
        m_sampleRate = sampleRate;
        updateFilters();
    }
}

void JZGuitarAmpProfiler::setAmpModel(AmpTubeModel model) {
    m_ampModel = model;
    updateFilters();
}

void JZGuitarAmpProfiler::setGain(float gain) {
    m_gain = std::max(0.0f, std::min(10.0f, gain));
}

void JZGuitarAmpProfiler::setMasterVolume(float master) {
    m_masterVolume = std::max(0.0f, std::min(10.0f, master));
}

void JZGuitarAmpProfiler::setPowerSag(float sag) {
    m_powerSag = std::max(0.0f, std::min(1.0f, sag));
}

void JZGuitarAmpProfiler::setBass(float bass) {
    m_bassVal = std::max(0.0f, std::min(10.0f, bass));
    updateFilters();
}

void JZGuitarAmpProfiler::setMiddle(float mid) {
    m_midVal = std::max(0.0f, std::min(10.0f, mid));
    updateFilters();
}

void JZGuitarAmpProfiler::setTreble(float treble) {
    m_trebleVal = std::max(0.0f, std::min(10.0f, treble));
    updateFilters();
}

void JZGuitarAmpProfiler::setPresence(float presence) {
    m_presenceVal = std::max(0.0f, std::min(10.0f, presence));
    updateFilters();
}

void JZGuitarAmpProfiler::setCabinetA(CabinetProfile profile, MicPlacement placement, float distCm) {
    m_slotA.profile = profile;
    m_slotA.placement = placement;
    m_slotA.distanceCm = std::max(0.0f, std::min(10.0f, distCm));
}

void JZGuitarAmpProfiler::setCabinetB(CabinetProfile profile, MicPlacement placement, float distCm) {
    m_slotB.profile = profile;
    m_slotB.placement = placement;
    m_slotB.distanceCm = std::max(0.0f, std::min(10.0f, distCm));
}

void JZGuitarAmpProfiler::setCabinetBlend(float blend) {
    m_cabBlend = std::max(0.0f, std::min(1.0f, blend));
}

void JZGuitarAmpProfiler::reset() {
    m_sagEnv = 0.0f;
    m_currentSagAmount = 0.0f;
    for (int i = 0; i < 2; ++i) {
        m_bassL[i] = m_midL[i] = m_trebleL[i] = m_presenceL[i] = 0.0f;
    }
    for (int i = 0; i < IR_LEN; ++i) {
        m_irBufA[i] = 0.0f;
        m_irBufB[i] = 0.0f;
    }
    m_irWriteA = 0;
    m_irWriteB = 0;
}

void JZGuitarAmpProfiler::updateFilters() {
    // Helper lambda for peaking/shelving biquad
    auto makePeaking = [this](float freq, float gainDb, float Q, float b[3], float a[3]) {
        float w0 = 2.0f * PI * freq / m_sampleRate;
        float alpha = std::sin(w0) / (2.0f * Q);
        float A = std::pow(10.0f, gainDb / 40.0f);
        float cosw0 = std::cos(w0);

        float a0 = 1.0f + alpha / A;
        b[0] = (1.0f + alpha * A) / a0;
        b[1] = (-2.0f * cosw0) / a0;
        b[2] = (1.0f - alpha * A) / a0;
        a[0] = 1.0f;
        a[1] = (-2.0f * cosw0) / a0;
        a[2] = (1.0f - alpha / A) / a0;
    };

    // Bass Peak/Shelf at 110 Hz
    float bassDb = (m_bassVal - 5.0f) * 2.4f; // -12dB to +12dB
    makePeaking(110.0f, bassDb, 0.707f, m_bBass, m_aBass);

    // Mid Peak at 650 Hz
    float midDb = (m_midVal - 5.0f) * 2.4f;
    makePeaking(650.0f, midDb, 1.2f, m_bMid, m_aMid);

    // Treble Shelf at 3200 Hz
    float trebDb = (m_trebleVal - 5.0f) * 2.4f;
    makePeaking(3200.0f, trebDb, 0.8f, m_bTreb, m_aTreb);

    // Presence Peak at 5500 Hz
    float presDb = (m_presenceVal - 5.0f) * 1.8f;
    makePeaking(5500.0f, presDb, 1.0f, m_bPres, m_aPres);
}

float JZGuitarAmpProfiler::triodePreampSaturate(float in, float drive) const {
    float x = in * drive;
    // Asymmetric 12AX7 tube triode transfer function:
    // Positive grid conducting softly, negative cutoff sharp
    if (x > 0.0f) {
        return std::tanh(x * 0.85f);
    } else {
        float neg = -x;
        float shaped = neg / (1.0f + neg * 0.75f);
        return -std::tanh(shaped);
    }
}

float JZGuitarAmpProfiler::powerAmpSaturate(float in, float drive) const {
    // Dynamic tube sag reduces rail voltage
    float railVoltage = 1.0f - m_currentSagAmount * 0.55f;
    railVoltage = std::max(0.35f, railVoltage);

    float scaled = (in * drive) / railVoltage;
    float out = std::tanh(scaled) * railVoltage;
    return out;
}

float JZGuitarAmpProfiler::convolveCabinet(float in, const CabinetSlotConfig& slot, float* delayLine, int& writeIdx) {
    delayLine[writeIdx] = in;

    // Synthesize 64-tap impulse response based on cabinet acoustic resonance
    float resonanceFreq = 120.0f;
    float brightDecay = 0.08f;
    if (slot.profile == CabinetProfile::CAB_4x12_VINTAGE_30) {
        resonanceFreq = 115.0f; brightDecay = 0.07f;
    } else if (slot.profile == CabinetProfile::CAB_2x12_GREENBACK) {
        resonanceFreq = 95.0f; brightDecay = 0.12f;
    } else if (slot.profile == CabinetProfile::CAB_1x12_AMERICAN_CLEAN) {
        resonanceFreq = 85.0f; brightDecay = 0.05f;
    } else {
        resonanceFreq = 130.0f; brightDecay = 0.09f;
    }

    // Mic placement damping
    float damping = 1.0f;
    if (slot.placement == MicPlacement::CONE) damping = 1.6f;
    else if (slot.placement == MicPlacement::OFF_AXIS_45) damping = 2.2f;

    // Proximity effect bass boost (0 to 3 cm)
    float proxBoost = (slot.distanceCm < 3.0f) ? (3.0f - slot.distanceCm) * 0.15f : 0.0f;

    float sum = 0.0f;
    for (int t = 0; t < IR_LEN; ++t) {
        int readIdx = (writeIdx - t + IR_LEN) % IR_LEN;
        float timeSec = static_cast<float>(t) / m_sampleRate;
        float env = std::exp(-timeSec * (brightDecay * 1000.0f * damping));
        float osc = std::sin(2.0f * PI * resonanceFreq * timeSec) + proxBoost * std::cos(2.0f * PI * 80.0f * timeSec);
        float tapCoeff = (t == 0) ? 0.35f : (osc * env * 0.15f);

        sum += delayLine[readIdx] * tapCoeff;
    }

    writeIdx = (writeIdx + 1) % IR_LEN;

    if (slot.phaseInvert) {
        sum = -sum;
    }
    return sum * slot.level;
}

float JZGuitarAmpProfiler::processSample(float in) {
    // Envelope tracker for power sag
    float absIn = std::abs(in);
    float attack = 0.02f;
    float release = 0.002f;
    if (absIn > m_sagEnv) {
        m_sagEnv += attack * (absIn - m_sagEnv);
    } else {
        m_sagEnv += release * (absIn - m_sagEnv);
    }
    m_currentSagAmount = m_sagEnv * m_powerSag;

    // Preamp drive calculation
    float preampDrive = 1.0f + m_gain * 2.8f;
    if (m_ampModel == AmpTubeModel::HIGH_GAIN_LEAD_6505) preampDrive *= 2.2f;
    else if (m_ampModel == AmpTubeModel::CLEAN_AMERICAN_6L6) preampDrive *= 0.65f;

    // Preamp Tube Stage
    float v1 = triodePreampSaturate(in, preampDrive);
    float v2 = triodePreampSaturate(v1, preampDrive * 0.7f);

    // Tonestack filtering (Biquads in series)
    auto runBiquad = [](float sample, float b[3], float a[3], float state[2]) -> float {
        float out = b[0] * sample + state[0];
        state[0] = b[1] * sample - a[1] * out + state[1];
        state[1] = b[2] * sample - a[2] * out;
        return out;
    };

    float eqBass = runBiquad(v2, m_bBass, m_aBass, m_bassL);
    float eqMid = runBiquad(eqBass, m_bMid, m_aMid, m_midL);
    float eqTreb = runBiquad(eqMid, m_bTreb, m_aTreb, m_trebleL);
    float eqPres = runBiquad(eqTreb, m_bPres, m_aPres, m_presenceL);

    // Power Amp Stage
    float masterDrive = 0.5f + m_masterVolume * 0.35f;
    float pwr = powerAmpSaturate(eqPres, masterDrive);

    if (m_cabBypass) {
        return pwr * 0.8f;
    }

    // Dual-IR Cabinet Convolution
    float cabA = convolveCabinet(pwr, m_slotA, m_irBufA, m_irWriteA);
    float cabB = convolveCabinet(pwr, m_slotB, m_irBufB, m_irWriteB);

    // Blend Cabinet A and B
    float cabOut = (1.0f - m_cabBlend) * cabA + m_cabBlend * cabB;
    return cabOut * 1.25f;
}

void JZGuitarAmpProfiler::processStereo(float inL, float inR, float& outL, float& outR) {
    // Process dual mono / stereo with independent cab convolutions
    outL = processSample(inL);
    outR = processSample(inR);
}

void JZGuitarAmpProfiler::processBlock(float* buffer, int numSamples) {
    if (!buffer || numSamples <= 0) return;
    for (int i = 0; i < numSamples; ++i) {
        buffer[i] = processSample(buffer[i]);
    }
}

} // namespace Jazz
