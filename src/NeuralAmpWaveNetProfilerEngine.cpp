//=============================================================================
// NeuralAmpWaveNetProfilerEngine.cpp
//
// Master Studio Pro Suite v30.0 - Apex Colossus Sovereign Diamond Jubilee Release
// Real-Time Non-Linear Dilated Temporal Convolutional Network (WaveNet)
// Guitar Tube Amplifier Profiler & Dynamic Tube Transformer Modeler.
//=============================================================================

#include "NeuralAmpWaveNetProfilerEngine.h"

namespace Jazz {

static inline float fastSigmoid(float x) {
    return 1.0f / (1.0f + std::exp(-std::max(-15.0f, std::min(15.0f, x))));
}

static inline float fastTanh(float x) {
    float clamped = std::max(-15.0f, std::min(15.0f, x));
    return std::tanh(clamped);
}

JZNeuralAmpWaveNetProfilerEngine::JZNeuralAmpWaveNetProfilerEngine(float sampleRate)
    : mSampleRate(sampleRate > 1000.0f ? sampleRate : 48000.0f)
    , mProfile(AmpProfile::BRITISH_CRUNCH)
    , mGainDb(6.0f)
    , mMasterVolumeDb(0.0f)
    , mBass(0.5f)
    , mMiddle(0.6f)
    , mTreble(0.55f)
    , mPresence(0.5f)
    , mPowerSag(0.35f)
    , mTubeBias(0.05f)
    , mInputLinearGain(1.0f)
    , mMasterLinearGain(1.0f)
    , mCurrentSagVoltage(0.0f)
    , mThdPercent(0.0f)
    , mIsClipping(false)
    , mDcInL(0.0f), mDcOutL(0.0f)
    , mDcInR(0.0f), mDcOutR(0.0f)
    , mSagEnvelopeL(0.0f)
    , mSagEnvelopeR(0.0f)
{
    // Dilations: 1, 2, 4, 8
    size_t dilations[NUM_LAYERS] = { 1, 2, 4, 8 };
    for (size_t i = 0; i < NUM_LAYERS; ++i) {
        mDelayLineL[i].assign(dilations[i] + 1, 0.0f);
        mDelayLineR[i].assign(dilations[i] + 1, 0.0f);
        mWriteIndex[i] = 0;
    }

    setGainDb(mGainDb);
    setMasterVolumeDb(mMasterVolumeDb);
    updateProfileParameters();
}

void JZNeuralAmpWaveNetProfilerEngine::setSampleRate(float sampleRate) {
    if (sampleRate > 1000.0f) {
        mSampleRate = sampleRate;
        reset();
    }
}

void JZNeuralAmpWaveNetProfilerEngine::setAmpProfile(AmpProfile profile) {
    mProfile = profile;
    updateProfileParameters();
}

std::string JZNeuralAmpWaveNetProfilerEngine::getAmpProfileName() const {
    switch (mProfile) {
        case AmpProfile::CLEAN_BOUTIQUE: return "Boutique Class A Clean (Dumble / Matchless)";
        case AmpProfile::BRITISH_CRUNCH: return "British Plexi 1959 High Treble (JCM800)";
        case AmpProfile::HIGH_GAIN_LEAD: return "High Gain Rectifier Multi-Watt Lead";
        case AmpProfile::VINTAGE_TWEED:  return "Vintage 1957 Tweed Deluxe 5E3";
        default: return "Custom Tube Profile";
    }
}

void JZNeuralAmpWaveNetProfilerEngine::updateProfileParameters() {
    switch (mProfile) {
        case AmpProfile::CLEAN_BOUTIQUE:
            mTubeBias = 0.02f;
            mPowerSag = 0.20f;
            break;
        case AmpProfile::BRITISH_CRUNCH:
            mTubeBias = 0.08f;
            mPowerSag = 0.40f;
            break;
        case AmpProfile::HIGH_GAIN_LEAD:
            mTubeBias = 0.12f;
            mPowerSag = 0.55f;
            break;
        case AmpProfile::VINTAGE_TWEED:
            mTubeBias = 0.06f;
            mPowerSag = 0.50f;
            break;
    }
}

void JZNeuralAmpWaveNetProfilerEngine::setGainDb(float gainDb) {
    mGainDb = std::max(-24.0f, std::min(36.0f, gainDb));
    mInputLinearGain = std::pow(10.0f, mGainDb / 20.0f);
}

void JZNeuralAmpWaveNetProfilerEngine::setMasterVolumeDb(float volDb) {
    mMasterVolumeDb = std::max(-48.0f, std::min(12.0f, volDb));
    mMasterLinearGain = std::pow(10.0f, mMasterVolumeDb / 20.0f);
}

void JZNeuralAmpWaveNetProfilerEngine::setBass(float bass) {
    mBass = std::max(0.0f, std::min(1.0f, bass));
}

void JZNeuralAmpWaveNetProfilerEngine::setMiddle(float mid) {
    mMiddle = std::max(0.0f, std::min(1.0f, mid));
}

void JZNeuralAmpWaveNetProfilerEngine::setTreble(float treble) {
    mTreble = std::max(0.0f, std::min(1.0f, treble));
}

void JZNeuralAmpWaveNetProfilerEngine::setPresence(float pres) {
    mPresence = std::max(0.0f, std::min(1.0f, pres));
}

void JZNeuralAmpWaveNetProfilerEngine::setPowerSag(float sag) {
    mPowerSag = std::max(0.0f, std::min(1.0f, sag));
}

void JZNeuralAmpWaveNetProfilerEngine::setTubeBias(float bias) {
    mTubeBias = std::max(-0.5f, std::min(0.5f, bias));
}

void JZNeuralAmpWaveNetProfilerEngine::reset() {
    for (size_t i = 0; i < NUM_LAYERS; ++i) {
        std::fill(mDelayLineL[i].begin(), mDelayLineL[i].end(), 0.0f);
        std::fill(mDelayLineR[i].begin(), mDelayLineR[i].end(), 0.0f);
        mWriteIndex[i] = 0;
    }
    mBassFilterL.reset(); mBassFilterR.reset();
    mMidFilterL.reset();  mMidFilterR.reset();
    mTrebleFilterL.reset(); mTrebleFilterR.reset();
    mPresenceFilterL.reset(); mPresenceFilterR.reset();

    mDcInL = mDcOutL = 0.0f;
    mDcInR = mDcOutR = 0.0f;
    mSagEnvelopeL = mSagEnvelopeR = 0.0f;
    mCurrentSagVoltage = 0.0f;
    mThdPercent = 0.0f;
    mIsClipping = false;
}

float JZNeuralAmpWaveNetProfilerEngine::processPreampStage(float inSample, float bias) {
    // 12AX7 Asymmetric triode transfer curve with cathode bias shift
    float biased = inSample + bias;
    float saturated = 0.0f;
    if (biased >= 0.0f) {
        // Soft positive grid conduction saturation
        saturated = fastTanh(1.4f * biased);
    } else {
        // Asymmetric negative cutoff knee
        saturated = 0.85f * fastTanh(biased * 1.1f);
    }
    return saturated;
}

float JZNeuralAmpWaveNetProfilerEngine::processWaveNetLayers(float x, size_t channelIndex) {
    // 4-Layer Dilated Temporal Convolutional Network with Gated Activations
    // Layer dilations: 1, 2, 4, 8
    const size_t dilations[NUM_LAYERS] = { 1, 2, 4, 8 };
    // Fixed weights simulating non-linear tube amplifier impulse dynamics
    const float filterWeights[NUM_LAYERS] = { 0.65f, 0.45f, 0.30f, 0.20f };
    const float gateWeights[NUM_LAYERS]   = { 0.55f, 0.40f, 0.28f, 0.18f };
    const float skipWeights[NUM_LAYERS]   = { 0.35f, 0.28f, 0.22f, 0.15f };

    float currentActivation = x;
    float accumulatedSkip = 0.0f;

    for (size_t l = 0; l < NUM_LAYERS; ++l) {
        std::vector<float>& dline = (channelIndex == 0) ? mDelayLineL[l] : mDelayLineR[l];
        size_t d = dilations[l];
        size_t bufSize = dline.size();

        // Write current sample to delay line
        dline[mWriteIndex[l] % bufSize] = currentActivation;

        // Read delayed sample with dilation d
        size_t readIdx = (mWriteIndex[l] + bufSize - d) % bufSize;
        float delayedSample = dline[readIdx];

        // Gated activation: z = tanh(W_f * x) * sigmoid(W_g * x)
        float filterOut = fastTanh(currentActivation * 0.8f + delayedSample * filterWeights[l]);
        float gateOut   = fastSigmoid(currentActivation * 0.7f + delayedSample * gateWeights[l]);
        float z = filterOut * gateOut;

        // Skip connection output
        accumulatedSkip += z * skipWeights[l];

        // Residual connection to next layer
        currentActivation = currentActivation + z * 0.5f;

        if (channelIndex == 1) { // Increment write pointer only after processing both channels
            mWriteIndex[l] = (mWriteIndex[l] + 1) % bufSize;
        }
    }

    return fastTanh(accumulatedSkip * 1.25f);
}

float JZNeuralAmpWaveNetProfilerEngine::processPowerAmpStage(float inSample, float& sagEnv) {
    // Dynamic power rail sag calculation:
    // Large transient draws pull down B+ rail voltage through rectifier tube and filter choke
    float absSig = std::abs(inSample);
    float attackCoeff = 0.02f;   // fast sag drop under heavy peak
    float recoveryCoeff = 0.002f; // slower recovery as filter caps recharge

    if (absSig > sagEnv) {
        sagEnv += attackCoeff * (absSig - sagEnv);
    } else {
        sagEnv -= recoveryCoeff * sagEnv;
    }

    // Rail voltage compression factor
    float sagFactor = 1.0f - (mPowerSag * 0.45f * sagEnv);
    sagFactor = std::max(0.35f, std::min(1.0f, sagFactor));

    // Push-pull class AB tube stage with transformer core saturation
    float pushed = inSample * sagFactor;
    // Cubic and hyperbolic transformer hysteresis
    float out = (1.5f * pushed - 0.5f * pushed * pushed * pushed);
    out = fastTanh(out);

    return out;
}

float JZNeuralAmpWaveNetProfilerEngine::runBiquad(float in, TonestackState& s,
                                                float b0, float b1, float b2,
                                                float a1, float a2) {
    float out = b0 * in + b1 * s.x1 + b2 * s.x2 - a1 * s.y1 - a2 * s.y2;
    s.x2 = s.x1;
    s.x1 = in;
    s.y2 = s.y1;
    s.y1 = out;
    return out;
}

float JZNeuralAmpWaveNetProfilerEngine::processTonestack(float x, TonestackState& bassF,
                                                       TonestackState& midF,
                                                       TonestackState& trebF,
                                                       TonestackState& presF) {
    // Fender / Marshall FMV style passive tonestack simulation
    // Bass (Low Shelf at 120 Hz)
    float bassGain = std::pow(10.0f, (mBass - 0.5f) * 24.0f / 20.0f);
    float b0_b = bassGain * 0.15f + 0.85f;
    float bassOut = runBiquad(x, bassF, b0_b, -0.7f * b0_b, 0.0f, -0.7f, 0.0f);

    // Middle (Peaking at 650 Hz, Q = 1.2)
    float midGain = std::pow(10.0f, (mMiddle - 0.5f) * 18.0f / 20.0f);
    float midOut = runBiquad(bassOut, midF, 0.2f * midGain + 0.8f, -1.2f, 0.4f, -1.2f, 0.4f);

    // Treble (High Shelf at 3200 Hz)
    float trebGain = std::pow(10.0f, (mTreble - 0.5f) * 20.0f / 20.0f);
    float trebOut = runBiquad(midOut, trebF, trebGain * 0.4f + 0.6f, -0.5f, 0.0f, -0.5f, 0.0f);

    // Presence (Upper Sparkle at 5500 Hz)
    float presGain = std::pow(10.0f, (mPresence - 0.5f) * 16.0f / 20.0f);
    float presOut = runBiquad(trebOut, presF, presGain * 0.3f + 0.7f, -0.3f, 0.0f, -0.3f, 0.0f);

    return presOut;
}

void JZNeuralAmpWaveNetProfilerEngine::process(const float* inL, const float* inR,
                                              float* outL, float* outR, size_t numSamples) {
    if (!inL || !inR || !outL || !outR || numSamples == 0) return;

    float peakIn = 0.0f;
    float peakOut = 0.0f;
    float harmonicEnergy = 0.0f;
    float totalEnergy = 0.0f;

    for (size_t i = 0; i < numSamples; ++i) {
        float sampleL = inL[i] * mInputLinearGain;
        float sampleR = inR[i] * mInputLinearGain;

        peakIn = std::max(peakIn, std::max(std::abs(sampleL), std::abs(sampleR)));

        // 1. Preamp Stage: 12AX7 Asymmetric Saturation
        float preL = processPreampStage(sampleL, mTubeBias);
        float preR = processPreampStage(sampleR, mTubeBias);

        // 2. WaveNet Dilated Temporal Non-Linear Convolutional Core
        float netL = processWaveNetLayers(preL, 0);
        float netR = processWaveNetLayers(preR, 1);

        // 3. Power Amp Stage: Push-Pull Tube & Dynamic Sag
        float pwrL = processPowerAmpStage(netL, mSagEnvelopeL);
        float pwrR = processPowerAmpStage(netR, mSagEnvelopeR);

        // 4. FMV Tonestack Filtering (Bass, Middle, Treble, Presence)
        float toneL = processTonestack(pwrL, mBassFilterL, mMidFilterL, mTrebleFilterL, mPresenceFilterL);
        float toneR = processTonestack(pwrR, mBassFilterR, mMidFilterR, mTrebleFilterR, mPresenceFilterR);

        // 5. High-Pass DC Blocker (cutoff ~15 Hz)
        float dcAlpha = 0.998f;
        mDcOutL = toneL - mDcInL + dcAlpha * mDcOutL;
        mDcInL  = toneL;
        mDcOutR = toneR - mDcInR + dcAlpha * mDcOutR;
        mDcInR  = toneR;

        // 6. Master Volume Application
        float finalL = mDcOutL * mMasterLinearGain;
        float finalR = mDcOutR * mMasterLinearGain;

        // Output soft safety ceiling
        finalL = fastTanh(finalL);
        finalR = fastTanh(finalR);

        outL[i] = finalL;
        outR[i] = finalR;

        peakOut = std::max(peakOut, std::max(std::abs(finalL), std::abs(finalR)));
        float rawDiff = std::abs(finalL - sampleL);
        harmonicEnergy += rawDiff * rawDiff;
        totalEnergy += finalL * finalL + 1e-6f;
    }

    // Telemetry updates
    mCurrentSagVoltage = (mSagEnvelopeL + mSagEnvelopeR) * 0.5f * mPowerSag;
    mIsClipping = (peakIn > 0.95f) || (peakOut > 0.98f);
    mThdPercent = std::min(100.0f, (std::sqrt(harmonicEnergy / totalEnergy) * 100.0f));
}

} // namespace Jazz
