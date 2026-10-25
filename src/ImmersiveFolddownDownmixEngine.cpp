//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// ImmersiveFolddownDownmixEngine.cpp - Immersive Audio Folddown & Downmixing
//*****************************************************************************

#include "ImmersiveFolddownDownmixEngine.h"
#include <cmath>
#include <algorithm>

namespace Jazz {

static const char* CHANNEL_NAMES[JZImmersiveFolddownDownmixEngine::NUM_INPUT_CHANNELS] = {
    "L", "R", "C", "LFE",
    "Lss", "Rss", "Lsr", "Rsr",
    "Ltf", "Rtf", "Ltm", "Rtm",
    "Ltr", "Rtr", "Lw", "Rw"
};

JZImmersiveFolddownDownmixEngine::JZImmersiveFolddownDownmixEngine(float sampleRate)
    : m_sampleRate(sampleRate)
{
    initialize(sampleRate);
}

void JZImmersiveFolddownDownmixEngine::initialize(float sampleRate)
{
    m_sampleRate = (sampleRate > 8000.0f) ? sampleRate : 48000.0f;
    reset();
}

void JZImmersiveFolddownDownmixEngine::reset()
{
    for (int i = 0; i < 4; ++i) {
        m_hilbertState[i] = 0.0f;
    }
    m_outputPeakLevelDb = -60.0f;
}

void JZImmersiveFolddownDownmixEngine::setParameters(const Parameters& params)
{
    m_params = params;
    m_params.centerTrimDb = std::max(-6.0f, std::min(m_params.centerTrimDb, -3.0f));
    m_params.surroundTrimDb = std::max(-9.0f, std::min(m_params.surroundTrimDb, -3.0f));
    m_params.heightTrimDb = std::max(-9.0f, std::min(m_params.heightTrimDb, -3.0f));
}

void JZImmersiveFolddownDownmixEngine::setTargetMode(DownmixTargetMode mode)
{
    m_params.targetMode = mode;
}

void JZImmersiveFolddownDownmixEngine::setCenterTrimDb(float db)
{
    m_params.centerTrimDb = std::max(-6.0f, std::min(db, -3.0f));
}

void JZImmersiveFolddownDownmixEngine::setSurroundTrimDb(float db)
{
    m_params.surroundTrimDb = std::max(-9.0f, std::min(db, -3.0f));
}

void JZImmersiveFolddownDownmixEngine::setHeightTrimDb(float db)
{
    m_params.heightTrimDb = std::max(-9.0f, std::min(db, -3.0f));
}

void JZImmersiveFolddownDownmixEngine::setLfeMode(LfeFolddownMode mode)
{
    m_params.lfeMode = mode;
}

void JZImmersiveFolddownDownmixEngine::setHilbertPhaseSteering(bool enable)
{
    m_params.enableHilbertPhaseSteering = enable;
}

void JZImmersiveFolddownDownmixEngine::setOutputLimiter(bool enable)
{
    m_params.enableOutputLimiter = enable;
}

void JZImmersiveFolddownDownmixEngine::setBypass(bool bypass)
{
    m_params.bypassed = bypass;
}

std::string JZImmersiveFolddownDownmixEngine::getChannelName(size_t index) const
{
    if (index < NUM_INPUT_CHANNELS) {
        return CHANNEL_NAMES[index];
    }
    return "Unknown";
}

void JZImmersiveFolddownDownmixEngine::processToStereo(
    const float* const* in16Channels,
    float* outL, float* outR,
    size_t numSamples)
{
    if (!in16Channels || !outL || !outR || numSamples == 0) return;

    if (m_params.bypassed) {
        // Pass Left (0) and Right (1) directly
        if (in16Channels[0]) std::copy(in16Channels[0], in16Channels[0] + numSamples, outL);
        if (in16Channels[1]) std::copy(in16Channels[1], in16Channels[1] + numSamples, outR);
        return;
    }

    float cGain = std::pow(10.0f, m_params.centerTrimDb / 20.0f);
    float sGain = std::pow(10.0f, m_params.surroundTrimDb / 20.0f);
    float hGain = std::pow(10.0f, m_params.heightTrimDb / 20.0f);
    float lfeGain = (m_params.lfeMode == LfeFolddownMode::LFE_ATTENUATE_10DB)
                    ? (std::pow(10.0f, -10.0f / 20.0f) * 0.7071f) : 0.0f;

    float peakMax = 0.0f;

    for (size_t i = 0; i < numSamples; ++i) {
        float inL   = in16Channels[0]  ? in16Channels[0][i]  : 0.0f;
        float inR   = in16Channels[1]  ? in16Channels[1][i]  : 0.0f;
        float inC   = in16Channels[2]  ? in16Channels[2][i]  : 0.0f;
        float inLfe = in16Channels[3]  ? in16Channels[3][i]  : 0.0f;
        float inLss = in16Channels[4]  ? in16Channels[4][i]  : 0.0f;
        float inRss = in16Channels[5]  ? in16Channels[5][i]  : 0.0f;
        float inLsr = in16Channels[6]  ? in16Channels[6][i]  : 0.0f;
        float inRsr = in16Channels[7]  ? in16Channels[7][i]  : 0.0f;
        float inLtf = in16Channels[8]  ? in16Channels[8][i]  : 0.0f;
        float inRtf = in16Channels[9]  ? in16Channels[9][i]  : 0.0f;
        float inLtm = in16Channels[10] ? in16Channels[10][i] : 0.0f;
        float inRtm = in16Channels[11] ? in16Channels[11][i] : 0.0f;
        float inLtr = in16Channels[12] ? in16Channels[12][i] : 0.0f;
        float inRtr = in16Channels[13] ? in16Channels[13][i] : 0.0f;
        float inLw  = in16Channels[14] ? in16Channels[14][i] : 0.0f;
        float inRw  = in16Channels[15] ? in16Channels[15][i] : 0.0f;

        // Surround and height sub-mix
        float surrL = inLss * sGain + inLsr * (sGain * 0.7071f);
        float surrR = inRss * sGain + inRsr * (sGain * 0.7071f);

        float heightL = inLtf * hGain + inLtm * (hGain * 0.7071f) + inLtr * (hGain * 0.5f);
        float heightR = inRtf * hGain + inRtm * (hGain * 0.7071f) + inRtr * (hGain * 0.5f);

        // Optional 90-degree Hilbert phase allpass network on diffuse spatial elements
        if (m_params.enableHilbertPhaseSteering) {
            float phaseL = (surrL + heightL);
            float outPhaseL = -0.6f * phaseL + m_hilbertState[0];
            m_hilbertState[0] = phaseL + 0.6f * outPhaseL;
            surrL = outPhaseL * 0.85f;

            float phaseR = (surrR + heightR);
            float outPhaseR = -0.6f * phaseR + m_hilbertState[1];
            m_hilbertState[1] = phaseR + 0.6f * outPhaseR;
            surrR = outPhaseR * 0.85f;
            heightL = 0.0f;
            heightR = 0.0f;
        }

        // Stereo folddown summing
        float sumL = inL + inC * cGain + inLw * 0.5f + surrL + heightL + inLfe * lfeGain;
        float sumR = inR + inC * cGain + inRw * 0.5f + surrR + heightR + inLfe * lfeGain;

        // Output safety limiter
        if (m_params.enableOutputLimiter) {
            if (std::abs(sumL) > 0.95f) sumL = std::tanh(sumL);
            if (std::abs(sumR) > 0.95f) sumR = std::tanh(sumR);
        }

        outL[i] = sumL;
        outR[i] = sumR;

        peakMax = std::max(peakMax, std::max(std::abs(sumL), std::abs(sumR)));
    }

    m_outputPeakLevelDb = 20.0f * std::log10(peakMax + 1e-6f);
}

void JZImmersiveFolddownDownmixEngine::processTo51(
    const float* const* in16Channels,
    float* const* out6Channels,
    size_t numSamples)
{
    if (!in16Channels || !out6Channels || numSamples == 0) return;

    float hGain = std::pow(10.0f, m_params.heightTrimDb / 20.0f);

    for (size_t i = 0; i < numSamples; ++i) {
        float inL   = in16Channels[0]  ? in16Channels[0][i]  : 0.0f;
        float inR   = in16Channels[1]  ? in16Channels[1][i]  : 0.0f;
        float inC   = in16Channels[2]  ? in16Channels[2][i]  : 0.0f;
        float inLfe = in16Channels[3]  ? in16Channels[3][i]  : 0.0f;
        float inLss = in16Channels[4]  ? in16Channels[4][i]  : 0.0f;
        float inRss = in16Channels[5]  ? in16Channels[5][i]  : 0.0f;
        float inLsr = in16Channels[6]  ? in16Channels[6][i]  : 0.0f;
        float inRsr = in16Channels[7]  ? in16Channels[7][i]  : 0.0f;
        float inLtf = in16Channels[8]  ? in16Channels[8][i]  : 0.0f;
        float inRtf = in16Channels[9]  ? in16Channels[9][i]  : 0.0f;
        float inLtm = in16Channels[10] ? in16Channels[10][i] : 0.0f;
        float inRtm = in16Channels[11] ? in16Channels[11][i] : 0.0f;
        float inLtr = in16Channels[12] ? in16Channels[12][i] : 0.0f;
        float inRtr = in16Channels[13] ? in16Channels[13][i] : 0.0f;
        float inLw  = in16Channels[14] ? in16Channels[14][i] : 0.0f;
        float inRw  = in16Channels[15] ? in16Channels[15][i] : 0.0f;

        // Out 0: Left
        if (out6Channels[0]) {
            out6Channels[0][i] = inL + inLw * 0.7071f + inLtf * hGain + inLtm * (hGain * 0.5f);
        }
        // Out 1: Right
        if (out6Channels[1]) {
            out6Channels[1][i] = inR + inRw * 0.7071f + inRtf * hGain + inRtm * (hGain * 0.5f);
        }
        // Out 2: Center (pass through unmodified)
        if (out6Channels[2]) {
            out6Channels[2][i] = inC;
        }
        // Out 3: LFE
        if (out6Channels[3]) {
            out6Channels[3][i] = inLfe;
        }
        // Out 4: Left Surround
        if (out6Channels[4]) {
            out6Channels[4][i] = inLss + inLsr * 0.7071f + inLtr * hGain + inLtm * (hGain * 0.5f);
        }
        // Out 5: Right Surround
        if (out6Channels[5]) {
            out6Channels[5][i] = inRss + inRsr * 0.7071f + inRtr * hGain + inRtm * (hGain * 0.5f);
        }
    }
}

} // namespace Jazz
