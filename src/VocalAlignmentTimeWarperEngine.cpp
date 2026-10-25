//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// VocalAlignmentTimeWarperEngine.cpp - Multi-Track Vocal Alignment
//*****************************************************************************

#include "VocalAlignmentTimeWarperEngine.h"
#include <cmath>
#include <limits>
#include <algorithm>

namespace Jazz {

JZVocalAlignmentTimeWarperEngine::JZVocalAlignmentTimeWarperEngine(float sampleRate) {
    initialize(sampleRate);
}

void JZVocalAlignmentTimeWarperEngine::initialize(float sampleRate) {
    if (sampleRate < 8000.0f) sampleRate = 48000.0f;
    m_sampleRate = sampleRate;
    reset();
}

void JZVocalAlignmentTimeWarperEngine::reset() {
    m_avgWarpRatio = 1.0f;
    m_lastGuideSamples = 0;
    m_lastDubSamples = 0;
}

void JZVocalAlignmentTimeWarperEngine::setParameters(const Parameters& params) {
    m_params = params;
    m_params.tightness = std::max(0.0f, std::min(1.0f, m_params.tightness));
    m_params.maxWarpStretchFactor = std::max(1.1f, std::min(3.0f, m_params.maxWarpStretchFactor));
    m_params.maxWarpCompressFactor = std::max(0.3f, std::min(0.95f, m_params.maxWarpCompressFactor));
}

void JZVocalAlignmentTimeWarperEngine::setTightness(float tightness) {
    m_params.tightness = std::max(0.0f, std::min(1.0f, tightness));
}

void JZVocalAlignmentTimeWarperEngine::setMaxWarpStretchFactor(float stretch) {
    m_params.maxWarpStretchFactor = std::max(1.1f, std::min(3.0f, stretch));
}

void JZVocalAlignmentTimeWarperEngine::extractEnergyEnvelope(
    const float* audio, size_t numSamples,
    size_t hopSize, std::vector<float>& outEnvelope) {

    outEnvelope.clear();
    if (!audio || numSamples == 0 || hopSize == 0) return;

    size_t numFrames = numSamples / hopSize;
    if (numFrames == 0) numFrames = 1;
    outEnvelope.resize(numFrames, 0.0f);

    for (size_t f = 0; f < numFrames; ++f) {
        size_t start = f * hopSize;
        size_t end = std::min(start + hopSize, numSamples);
        float rms = 0.0f;
        float hfFlux = 0.0f;

        for (size_t i = start; i < end; ++i) {
            float s = audio[i];
            rms += s * s;
            if (i > start) {
                float diff = std::abs(s - audio[i - 1]);
                hfFlux += diff;
            }
        }
        rms = std::sqrt(rms / static_cast<float>(end - start + 1));
        hfFlux = hfFlux / static_cast<float>(end - start + 1);

        // Combined phoneme energy with transient weighting
        outEnvelope[f] = rms + hfFlux * m_params.transientSensitivity * 0.5f;
    }
}

float JZVocalAlignmentTimeWarperEngine::alignDubToGuide(
    const float* guideAudio, size_t guideSamples,
    const float* dubAudio, size_t dubSamples,
    std::vector<float>& outAlignedDub) {

    outAlignedDub.clear();
    if (!guideAudio || !dubAudio || guideSamples == 0 || dubSamples == 0) {
        return 0.0f;
    }

    m_lastGuideSamples = guideSamples;
    m_lastDubSamples = dubSamples;

    size_t hop = m_params.windowHopSamples;
    if (hop < 64) hop = 256;

    std::vector<float> envG, envD;
    extractEnergyEnvelope(guideAudio, guideSamples, hop, envG);
    extractEnergyEnvelope(dubAudio, dubSamples, hop, envD);

    size_t nG = envG.size();
    size_t nD = envD.size();

    if (nG == 0 || nD == 0) {
        outAlignedDub.assign(dubAudio, dubAudio + dubSamples);
        return 0.0f;
    }

    // 1. Build DTW Cost Matrix
    std::vector<std::vector<float>> cost(nG, std::vector<float>(nD, 0.0f));
    for (size_t i = 0; i < nG; ++i) {
        for (size_t j = 0; j < nD; ++j) {
            float diff = std::abs(envG[i] - envD[j]);
            cost[i][j] = diff;
        }
    }

    // Accumulated distance matrix
    std::vector<std::vector<float>> D(nG, std::vector<float>(nD, std::numeric_limits<float>::max()));
    D[0][0] = cost[0][0];

    for (size_t i = 1; i < nG; ++i) D[i][0] = D[i - 1][0] + cost[i][0];
    for (size_t j = 1; j < nD; ++j) D[0][j] = D[0][j - 1] + cost[0][j];

    for (size_t i = 1; i < nG; ++i) {
        for (size_t j = 1; j < nD; ++j) {
            float minPrev = std::min({ D[i - 1][j], D[i][j - 1], D[i - 1][j - 1] });
            D[i][j] = cost[i][j] + minPrev;
        }
    }

    // 2. Backtrack optimal alignment path
    std::vector<std::pair<size_t, size_t>> path;
    int i = static_cast<int>(nG) - 1;
    int j = static_cast<int>(nD) - 1;

    while (i > 0 || j > 0) {
        path.push_back({ static_cast<size_t>(i), static_cast<size_t>(j) });
        if (i == 0) {
            j--;
        } else if (j == 0) {
            i--;
        } else {
            float diag = D[i - 1][j - 1];
            float up = D[i - 1][j];
            float left = D[i][j - 1];
            if (diag <= up && diag <= left) {
                i--; j--;
            } else if (up <= left) {
                i--;
            } else {
                j--;
            }
        }
    }
    path.push_back({ 0, 0 });
    std::reverse(path.begin(), path.end());

    // 3. Resample Dub into Guide timeline
    outAlignedDub.resize(guideSamples, 0.0f);
    float tight = m_params.tightness;

    // Build frame mapping table from Guide frame to Dub frame
    std::vector<float> guideToDubFrame(nG, 0.0f);
    for (const auto& p : path) {
        guideToDubFrame[p.first] = static_cast<float>(p.second);
    }

    for (size_t s = 0; s < guideSamples; ++s) {
        float guideFrame = static_cast<float>(s) / static_cast<float>(hop);
        size_t gIdx = static_cast<size_t>(guideFrame);
        if (gIdx >= nG) gIdx = nG - 1;

        // DTW target dub frame
        float dtwDubFrame = guideToDubFrame[gIdx];

        // Linear (unwarped) target dub frame
        float linearDubFrame = (static_cast<float>(s) / static_cast<float>(guideSamples)) * static_cast<float>(nD);

        // Blend according to tightness
        float finalDubFrame = linearDubFrame * (1.0f - tight) + dtwDubFrame * tight;
        float finalDubSample = finalDubFrame * static_cast<float>(hop);

        // Clamp within dub buffer bounds
        finalDubSample = std::max(0.0f, std::min(static_cast<float>(dubSamples - 2), finalDubSample));

        // Linear interpolation resampler
        size_t s0 = static_cast<size_t>(finalDubSample);
        size_t s1 = s0 + 1;
        float frac = finalDubSample - static_cast<float>(s0);

        outAlignedDub[s] = dubAudio[s0] * (1.0f - frac) + dubAudio[s1] * frac;
    }

    // 4. Compute correlation metric
    float dotProduct = 0.0f, gNorm = 0.0f, dNorm = 0.0f;
    for (size_t s = 0; s < guideSamples; ++s) {
        float g = guideAudio[s];
        float d = outAlignedDub[s];
        dotProduct += g * d;
        gNorm += g * g;
        dNorm += d * d;
    }

    m_avgWarpRatio = static_cast<float>(dubSamples) / static_cast<float>(guideSamples);
    float denom = std::sqrt(gNorm * dNorm);
    return (denom > 1e-6f) ? std::max(0.0f, dotProduct / denom) : 0.85f;
}

} // namespace Jazz
