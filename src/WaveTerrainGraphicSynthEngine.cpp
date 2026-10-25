//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// JZWaveTerrainGraphicSynthEngine.cpp - 3D Wave-Terrain & Optical Synthesizer
//*****************************************************************************

#include "WaveTerrainGraphicSynthEngine.h"
#include <algorithm>
#include <cmath>

namespace Jazz {

JZWaveTerrainGraphicSynthEngine::JZWaveTerrainGraphicSynthEngine(float sampleRate)
    : m_sampleRate(sampleRate > 1000.0f ? sampleRate : 48000.0f)
    , m_frequencyHz(130.81f) // C3 default
    , m_surface(WaveTerrainSurface::RIPPLE_TORUS)
    , m_trajectory(WaveTerrainTrajectory::CIRCULAR_ORBIT)
    , m_orbitRadius(1.2f)
    , m_ratioA(3)
    , m_ratioB(2)
    , m_phaseOffset(0.785398f) // pi / 4
    , m_orbitPhase(0.0)
    , m_bitmapWidth(0)
    , m_bitmapHeight(0)
{
}

void JZWaveTerrainGraphicSynthEngine::setSampleRate(float sampleRate) {
    if (sampleRate > 1000.0f) {
        m_sampleRate = sampleRate;
        reset();
    }
}

void JZWaveTerrainGraphicSynthEngine::setFrequencyHz(float f0Hz) {
    m_frequencyHz = std::clamp(f0Hz, 10.0f, 4000.0f);
}

void JZWaveTerrainGraphicSynthEngine::setSurfaceType(WaveTerrainSurface surface) {
    m_surface = surface;
}

void JZWaveTerrainGraphicSynthEngine::setTrajectoryType(WaveTerrainTrajectory trajectory) {
    m_trajectory = trajectory;
}

void JZWaveTerrainGraphicSynthEngine::setOrbitRadius(float radius) {
    m_orbitRadius = std::clamp(radius, 0.05f, 4.0f);
}

void JZWaveTerrainGraphicSynthEngine::setLissajousRatio(int a, int b) {
    m_ratioA = std::max(1, a);
    m_ratioB = std::max(1, b);
}

void JZWaveTerrainGraphicSynthEngine::setPhaseOffsetRad(float phaseRad) {
    m_phaseOffset = phaseRad;
}

bool JZWaveTerrainGraphicSynthEngine::loadBitmapSurface(const std::vector<float>& grayscaleData,
                                                      size_t width, size_t height) {
    if (grayscaleData.size() < width * height || width == 0 || height == 0) {
        return false;
    }
    m_bitmapData = grayscaleData;
    m_bitmapWidth = width;
    m_bitmapHeight = height;
    m_surface = WaveTerrainSurface::OPTICAL_BITMAP;
    return true;
}

void JZWaveTerrainGraphicSynthEngine::reset() {
    m_orbitPhase = 0.0;
}

float JZWaveTerrainGraphicSynthEngine::evaluateSurface(float x, float y) const {
    switch (m_surface) {
        case WaveTerrainSurface::RIPPLE_TORUS: {
            float r2 = x * x + y * y;
            return std::sin(r2 * 2.5f) * std::cos(std::atan2(y, x) * 2.0f);
        }

        case WaveTerrainSurface::HYPERBOLIC_SADDLE: {
            float z = (x * x - y * y) * 0.5f;
            return std::tanh(z);
        }

        case WaveTerrainSurface::CHAOTIC_ATTRACTOR: {
            return std::sin(x * 2.0f) * std::cos(y * 2.0f) + 0.4f * std::sin(3.0f * x * y);
        }

        case WaveTerrainSurface::OPTICAL_BITMAP: {
            if (m_bitmapData.empty() || m_bitmapWidth == 0 || m_bitmapHeight == 0) {
                return 0.0f;
            }
            // Normalize [-2.0, 2.0] into bitmap coordinates [0, width-1]
            float u = (x + 2.0f) * 0.25f * (m_bitmapWidth - 1);
            float v = (y + 2.0f) * 0.25f * (m_bitmapHeight - 1);

            int x0 = std::clamp(static_cast<int>(std::floor(u)), 0, static_cast<int>(m_bitmapWidth - 1));
            int y0 = std::clamp(static_cast<int>(std::floor(v)), 0, static_cast<int>(m_bitmapHeight - 1));
            int x1 = std::min(x0 + 1, static_cast<int>(m_bitmapWidth - 1));
            int y1 = std::min(y0 + 1, static_cast<int>(m_bitmapHeight - 1));

            float fx = u - x0;
            float fy = v - y0;

            float p00 = m_bitmapData[y0 * m_bitmapWidth + x0];
            float p10 = m_bitmapData[y0 * m_bitmapWidth + x1];
            float p01 = m_bitmapData[y1 * m_bitmapWidth + x0];
            float p11 = m_bitmapData[y1 * m_bitmapWidth + x1];

            // Bilinear interpolation
            float top = p00 * (1.0f - fx) + p10 * fx;
            float bottom = p01 * (1.0f - fx) + p11 * fx;
            float z = top * (1.0f - fy) + bottom * fy;
            return (z * 2.0f) - 1.0f; // Scale [0, 1] to [-1, 1]
        }
    }
    return 0.0f;
}

void JZWaveTerrainGraphicSynthEngine::generate(float* outAudio, size_t numSamples) {
    if (!outAudio || numSamples == 0) return;

    double omega = 2.0 * 3.141592653589793 * (m_frequencyHz / m_sampleRate);

    for (size_t i = 0; i < numSamples; ++i) {
        m_orbitPhase += omega;
        if (m_orbitPhase >= 2.0 * 3.141592653589793) {
            m_orbitPhase -= 2.0 * 3.141592653589793;
        }

        float x = 0.0f;
        float y = 0.0f;

        switch (m_trajectory) {
            case WaveTerrainTrajectory::CIRCULAR_ORBIT:
                x = m_orbitRadius * static_cast<float>(std::cos(m_orbitPhase));
                y = m_orbitRadius * static_cast<float>(std::sin(m_orbitPhase));
                break;

            case WaveTerrainTrajectory::ELLIPSE_PRECESSION:
                x = m_orbitRadius * static_cast<float>(std::cos(m_orbitPhase));
                y = (m_orbitRadius * 0.5f) * static_cast<float>(std::sin(m_orbitPhase));
                break;

            case WaveTerrainTrajectory::LISSAJOUS_FIGURE:
                x = m_orbitRadius * static_cast<float>(std::sin(m_ratioA * m_orbitPhase + m_phaseOffset));
                y = m_orbitRadius * static_cast<float>(std::sin(m_ratioB * m_orbitPhase));
                break;

            case WaveTerrainTrajectory::SPIRAL_GALAXY: {
                float pulseR = m_orbitRadius * (0.8f + 0.4f * static_cast<float>(std::sin(m_orbitPhase * 0.25)));
                x = pulseR * static_cast<float>(std::cos(m_orbitPhase));
                y = pulseR * static_cast<float>(std::sin(m_orbitPhase));
                break;
            }
        }

        // Evaluate surface height z at (x, y)
        float z = evaluateSurface(x, y);
        outAudio[i] = std::clamp(z, -1.0f, 1.0f);
    }
}

} // namespace Jazz
