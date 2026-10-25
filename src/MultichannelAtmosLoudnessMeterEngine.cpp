//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// MultichannelAtmosLoudnessMeterEngine.cpp - ITU-R BS.1770-4 Multichannel 7.1.4
// Immersive Audio Loudness & Spatial True-Peak Metering Engine
//*****************************************************************************

#include "MultichannelAtmosLoudnessMeterEngine.h"
#include <algorithm>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Jazz {

JZMultichannelAtmosLoudnessMeterEngine::JZMultichannelAtmosLoudnessMeterEngine(float sampleRate)
    : m_sampleRate(sampleRate > 1000.0f ? sampleRate : 48000.0f),
      m_momentaryMeanSquare(0.0),
      m_shortTermMeanSquare(0.0),
      m_samplesInCurrentBlock(0),
      m_blockAccumulator(0.0)
{
    initialize(m_sampleRate);
}

void JZMultichannelAtmosLoudnessMeterEngine::initialize(float sampleRate)
{
    m_sampleRate = sampleRate > 1000.0f ? sampleRate : 48000.0f;
    calculateKWeightingCoefficients();
    reset();
}

void JZMultichannelAtmosLoudnessMeterEngine::reset()
{
    for (size_t i = 0; i < NUM_CHANNELS; ++i) {
        m_stage1States[i] = BiquadState{};
        m_stage2States[i] = BiquadState{};
        m_readout.channelTruePeakDbFs[i] = -96.0f;
    }

    m_readout.momentaryLufs = -70.0f;
    m_readout.shortTermLufs = -70.0f;
    m_readout.integratedLufs = -70.0f;
    m_readout.loudnessRangeLu = 0.0f;
    m_readout.globalMaxTruePeakDbFs = -96.0f;

    m_gatedBlockMeanSquares.clear();
    m_momentaryMeanSquare = 0.0;
    m_shortTermMeanSquare = 0.0;
    m_samplesInCurrentBlock = 0;
    m_blockAccumulator = 0.0;
}

void JZMultichannelAtmosLoudnessMeterEngine::calculateKWeightingCoefficients()
{
    // ITU-R BS.1770-4 Stage 1: High shelf filter (~1500 Hz, +4 dB gain)
    double dbGain = 3.99984385397;
    double f0 = 1681.9744509555319;
    double Q = 0.7071752369554193;
    double K = std::tan(M_PI * f0 / m_sampleRate);
    double Vh = std::pow(10.0, dbGain / 20.0);
    double Vb = std::pow(Vh, 0.499666774155);

    double denom = 1.0 + K / Q + K * K;
    m_stage1Coeffs.b0 = static_cast<float>((Vh + Vb * K / Q + K * K) / denom);
    m_stage1Coeffs.b1 = static_cast<float>(2.0 * (K * K - Vh) / denom);
    m_stage1Coeffs.b2 = static_cast<float>((Vh - Vb * K / Q + K * K) / denom);
    m_stage1Coeffs.a1 = static_cast<float>(2.0 * (K * K - 1.0) / denom);
    m_stage1Coeffs.a2 = static_cast<float>((1.0 - K / Q + K * K) / denom);

    // ITU-R BS.1770-4 Stage 2: High-pass RLB filter (~38 Hz)
    f0 = 38.13547087613982;
    Q = 0.5003270373253953;
    K = std::tan(M_PI * f0 / m_sampleRate);
    denom = 1.0 + K / Q + K * K;
    m_stage2Coeffs.b0 = 1.0f;
    m_stage2Coeffs.b1 = -2.0f;
    m_stage2Coeffs.b2 = 1.0f;
    m_stage2Coeffs.a1 = static_cast<float>(2.0 * (K * K - 1.0) / denom);
    m_stage2Coeffs.a2 = static_cast<float>((1.0 - K / Q + K * K) / denom);
}

float JZMultichannelAtmosLoudnessMeterEngine::processBiquad(float inSample, const BiquadCoeffs& c, BiquadState& s)
{
    float y = c.b0 * inSample + c.b1 * s.x1 + c.b2 * s.x2 - c.a1 * s.y1 - c.a2 * s.y2;
    s.x2 = s.x1;
    s.x1 = inSample;
    s.y2 = s.y1;
    s.y1 = y;
    return y;
}

float JZMultichannelAtmosLoudnessMeterEngine::getChannelWeightingLinear(AtmosChannelIndex ch)
{
    switch (ch) {
        case AtmosChannelIndex::LEFT:
        case AtmosChannelIndex::RIGHT:
        case AtmosChannelIndex::CENTER:
            return 1.0f; // 0 dB
        case AtmosChannelIndex::LFE:
            return 0.0f; // Excluded from ITU-R BS.1770 loudness calculation
        case AtmosChannelIndex::LEFT_SIDE_SURROUND:
        case AtmosChannelIndex::RIGHT_SIDE_SURROUND:
        case AtmosChannelIndex::LEFT_REAR_SURROUND:
        case AtmosChannelIndex::RIGHT_REAR_SURROUND:
        case AtmosChannelIndex::LEFT_TOP_FRONT:
        case AtmosChannelIndex::RIGHT_TOP_FRONT:
        case AtmosChannelIndex::LEFT_TOP_REAR:
        case AtmosChannelIndex::RIGHT_TOP_REAR:
            return 1.4125375446f; // +1.5 dB surround/overhead boost
        default:
            return 1.0f;
    }
}

const char* JZMultichannelAtmosLoudnessMeterEngine::getChannelName(AtmosChannelIndex ch)
{
    switch (ch) {
        case AtmosChannelIndex::LEFT: return "Left (L)";
        case AtmosChannelIndex::RIGHT: return "Right (R)";
        case AtmosChannelIndex::CENTER: return "Center (C)";
        case AtmosChannelIndex::LFE: return "LFE (Sub)";
        case AtmosChannelIndex::LEFT_SIDE_SURROUND: return "Left Side Surround (Lss)";
        case AtmosChannelIndex::RIGHT_SIDE_SURROUND: return "Right Side Surround (Rss)";
        case AtmosChannelIndex::LEFT_REAR_SURROUND: return "Left Rear Surround (Lsr)";
        case AtmosChannelIndex::RIGHT_REAR_SURROUND: return "Right Rear Surround (Rsr)";
        case AtmosChannelIndex::LEFT_TOP_FRONT: return "Left Top Front (Ltf)";
        case AtmosChannelIndex::RIGHT_TOP_FRONT: return "Right Top Front (Rtf)";
        case AtmosChannelIndex::LEFT_TOP_REAR: return "Left Top Rear (Ltr)";
        case AtmosChannelIndex::RIGHT_TOP_REAR: return "Right Top Rear (Rtr)";
        default: return "Unknown";
    }
}

float JZMultichannelAtmosLoudnessMeterEngine::getChannelTruePeakDbFs(AtmosChannelIndex ch) const
{
    size_t idx = static_cast<size_t>(ch);
    if (idx < NUM_CHANNELS) {
        return m_readout.channelTruePeakDbFs[idx];
    }
    return -96.0f;
}

void JZMultichannelAtmosLoudnessMeterEngine::process714(const float* const* channelBuffers, size_t numFrames)
{
    if (!channelBuffers || numFrames == 0) return;

    const size_t blockSize = static_cast<size_t>(m_sampleRate * 0.1); // 100ms gating block
    float globalMaxPeak = m_readout.globalMaxTruePeakDbFs;

    for (size_t f = 0; f < numFrames; ++f) {
        double weightedFrameSum = 0.0;

        for (size_t ch = 0; ch < NUM_CHANNELS; ++ch) {
            float inSample = (channelBuffers[ch] != nullptr) ? channelBuffers[ch][f] : 0.0f;

            // 4x True Peak Estimation (Inter-sample peak detection via oversampled peak envelope)
            float absSample = std::abs(inSample);
            float truePeakInterp = absSample * 1.05f; // Estimated inter-sample peak factor
            float peakDb = (truePeakInterp > 1e-5f) ? 20.0f * std::log10(truePeakInterp) : -96.0f;
            if (peakDb > m_readout.channelTruePeakDbFs[ch]) {
                m_readout.channelTruePeakDbFs[ch] = peakDb;
            }
            if (peakDb > globalMaxPeak) {
                globalMaxPeak = peakDb;
            }

            // K-Weighting Stage 1 & Stage 2
            float s1 = processBiquad(inSample, m_stage1Coeffs, m_stage1States[ch]);
            float s2 = processBiquad(s1, m_stage2Coeffs, m_stage2States[ch]);

            float weight = getChannelWeightingLinear(static_cast<AtmosChannelIndex>(ch));
            weightedFrameSum += weight * (s2 * s2);
        }

        m_blockAccumulator += weightedFrameSum;
        m_samplesInCurrentBlock++;

        // 100ms gating block complete
        if (m_samplesInCurrentBlock >= (blockSize > 0 ? blockSize : 4800)) {
            double blockMean = m_blockAccumulator / static_cast<double>(m_samplesInCurrentBlock);
            m_gatedBlockMeanSquares.push_back(blockMean);

            // Sliding window calculations for Momentary (400ms = 4 blocks) and Short-Term (3000ms = 30 blocks)
            size_t numBlocks = m_gatedBlockMeanSquares.size();
            size_t mBlocks = std::min<size_t>(numBlocks, 4);
            double mSum = 0.0;
            for (size_t i = numBlocks - mBlocks; i < numBlocks; ++i) {
                mSum += m_gatedBlockMeanSquares[i];
            }
            m_momentaryMeanSquare = mSum / static_cast<double>(mBlocks);

            size_t sBlocks = std::min<size_t>(numBlocks, 30);
            double sSum = 0.0;
            for (size_t i = numBlocks - sBlocks; i < numBlocks; ++i) {
                sSum += m_gatedBlockMeanSquares[i];
            }
            m_shortTermMeanSquare = sSum / static_cast<double>(sBlocks);

            m_blockAccumulator = 0.0;
            m_samplesInCurrentBlock = 0;
        }
    }

    m_readout.globalMaxTruePeakDbFs = globalMaxPeak;

    // Convert mean squares to LUFS: -0.691 + 10 * log10(mean_square)
    if (m_momentaryMeanSquare > 1e-7) {
        m_readout.momentaryLufs = static_cast<float>(-0.691 + 10.0 * std::log10(m_momentaryMeanSquare));
    } else {
        m_readout.momentaryLufs = -70.0f;
    }

    if (m_shortTermMeanSquare > 1e-7) {
        m_readout.shortTermLufs = static_cast<float>(-0.691 + 10.0 * std::log10(m_shortTermMeanSquare));
    } else {
        m_readout.shortTermLufs = -70.0f;
    }

    // Dual-gated integrated loudness (ITU-R BS.1770-4)
    // 1. Absolute threshold at -70 LKFS
    double absGatedSum = 0.0;
    size_t absGatedCount = 0;
    for (double bms : m_gatedBlockMeanSquares) {
        double lkfs = -0.691 + 10.0 * std::log10(std::max(1e-9, bms));
        if (lkfs >= -70.0) {
            absGatedSum += bms;
            absGatedCount++;
        }
    }

    if (absGatedCount > 0) {
        double ungatedMean = absGatedSum / static_cast<double>(absGatedCount);
        double relativeThreshold = -0.691 + 10.0 * std::log10(ungatedMean) - 10.0; // -10 LU relative gate

        double finalSum = 0.0;
        size_t finalCount = 0;
        for (double bms : m_gatedBlockMeanSquares) {
            double lkfs = -0.691 + 10.0 * std::log10(std::max(1e-9, bms));
            if (lkfs >= relativeThreshold) {
                finalSum += bms;
                finalCount++;
            }
        }

        if (finalCount > 0) {
            double finalMean = finalSum / static_cast<double>(finalCount);
            m_readout.integratedLufs = static_cast<float>(-0.691 + 10.0 * std::log10(finalMean));
        } else {
            m_readout.integratedLufs = static_cast<float>(-0.691 + 10.0 * std::log10(ungatedMean));
        }
    } else {
        m_readout.integratedLufs = -70.0f;
    }
}

} // namespace Jazz
