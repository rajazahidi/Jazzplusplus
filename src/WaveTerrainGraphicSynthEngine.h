//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// JZWaveTerrainGraphicSynthEngine.h - 3D Wave-Terrain & Optical Synthesizer
//
// Mathematical 3D wave-terrain and optical graphic synthesis (Daphne Oram style):
// 2D/3D topographical surfaces z = f(x, y), orbital scanning trajectories
// (Circular, Elliptical, Lissajous a:b, Chaotic Strange Attractor), and
// direct 2D grayscale optical bitmap-to-audio wave-terrain generation.
//*****************************************************************************

#ifndef JAZZ_WAVE_TERRAIN_GRAPHIC_SYNTH_ENGINE_H
#define JAZZ_WAVE_TERRAIN_GRAPHIC_SYNTH_ENGINE_H

#include <vector>
#include <string>
#include <cstdint>
#include <cmath>

namespace Jazz {

enum class WaveTerrainSurface {
    RIPPLE_TORUS,       // Radial concentric ripple terrain: z = sin(x^2 + y^2)
    HYPERBOLIC_SADDLE,  // Mountain saddle pass: z = x^2 - y^2
    CHAOTIC_ATTRACTOR,  // Non-linear polynomial chaos: z = sin(x)*cos(y) + 0.5*sin(2xy)
    OPTICAL_BITMAP      // 2D optical grayscale image surface
};

enum class WaveTerrainTrajectory {
    CIRCULAR_ORBIT,     // Pure harmonic circular scan
    ELLIPSE_PRECESSION, // Precessing elliptical orbit
    LISSAJOUS_FIGURE,   // Multi-frequency Lissajous ratio a:b
    SPIRAL_GALAXY       // Continuously pulsing radial spiral
};

class JZWaveTerrainGraphicSynthEngine {
public:
    JZWaveTerrainGraphicSynthEngine(float sampleRate = 48000.0f);
    ~JZWaveTerrainGraphicSynthEngine() = default;

    void setSampleRate(float sampleRate);
    float getSampleRate() const { return m_sampleRate; }

    // Pitch & Orbital Controls
    void setFrequencyHz(float f0Hz);
    float getFrequencyHz() const { return m_frequencyHz; }

    void setSurfaceType(WaveTerrainSurface surface);
    WaveTerrainSurface getSurfaceType() const { return m_surface; }

    void setTrajectoryType(WaveTerrainTrajectory trajectory);
    WaveTerrainTrajectory getTrajectoryType() const { return m_trajectory; }

    void setOrbitRadius(float radius); // 0.1 to 3.0
    float getOrbitRadius() const { return m_orbitRadius; }

    void setLissajousRatio(int a, int b);
    void setPhaseOffsetRad(float phaseRad);

    // Optical Graphic Bitmap Surface Loading
    bool loadBitmapSurface(const std::vector<float>& grayscaleData, size_t width, size_t height);
    bool hasBitmapSurface() const { return !m_bitmapData.empty(); }

    // Audio Generation (synthesizes audio waveform)
    void generate(float* outAudio, size_t numSamples);

    void reset();

private:
    float m_sampleRate;
    float m_frequencyHz;
    WaveTerrainSurface m_surface;
    WaveTerrainTrajectory m_trajectory;
    float m_orbitRadius;
    int m_ratioA, m_ratioB;
    float m_phaseOffset;

    // Orbital phase accumulator
    double m_orbitPhase;

    // Optical bitmap memory (normalized float [0.0, 1.0])
    std::vector<float> m_bitmapData;
    size_t m_bitmapWidth;
    size_t m_bitmapHeight;

    // Evaluates the 2D surface z = f(x, y) with bilinear interpolation
    float evaluateSurface(float x, float y) const;
};

} // namespace Jazz

#endif // JAZZ_WAVE_TERRAIN_GRAPHIC_SYNTH_ENGINE_H
