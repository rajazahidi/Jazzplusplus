#ifndef JAZZ_GONIOMETER_VECTORSCOPE_H
#define JAZZ_GONIOMETER_VECTORSCOPE_H

#include <vector>
#include <cmath>

namespace Jazz {

enum class GoniometerDisplayMode {
    LISSAJOUS_XY,    // Standard X=L, Y=R rotated 45 deg: X=(L-R)/sqrt(2), Y=(L+R)/sqrt(2)
    POLAR_RADAR,     // Radius = Magnitude, Angle = Stereo position
    LINEAR_MID_SIDE  // Direct M/S balance representation
};

struct GoniometerPoint {
    float x;         // Normalized horizontal position [-1.0, 1.0]
    float y;         // Normalized vertical position [-1.0, 1.0]
    float intensity; // Phosphor decay brightness [0.0, 1.0]
};

class JZGoniometerVectorscope {
public:
    JZGoniometerVectorscope(int historySize = 1024, double sampleRate = 44100.0);
    ~JZGoniometerVectorscope() = default;

    void setSampleRate(double sampleRate);
    void setHistorySize(int numPoints);
    void setDisplayMode(GoniometerDisplayMode mode) { m_displayMode = mode; }
    GoniometerDisplayMode getDisplayMode() const { return m_displayMode; }

    void setPersistence(double persistence); // 0.0 (no trail) to 0.99 (long phosphor glow)
    double getPersistence() const { return m_persistence; }

    void setCorrelationSmoothing(double timeMs);

    // Audio ingestion
    void processSample(double left, double right);
    void processBlock(const double* left, const double* right, int numSamples);

    // Analytical metrics
    double getPhaseCorrelation() const { return m_smoothedCorrelation; }
    double getInstantaneousCorrelation() const { return m_instantCorrelation; }
    double getStereoBalance() const { return m_stereoBalance; } // -1.0 (Full L) to +1.0 (Full R)
    double getStereoWidth() const { return m_stereoWidth; }     // Side / Mid ratio
    bool isMonoCancellationWarning() const { return m_monoCancellationWarning; }

    double getRmsLeft() const { return m_rmsLeft; }
    double getRmsRight() const { return m_rmsRight; }
    double getRmsMid() const { return m_rmsMid; }
    double getRmsSide() const { return m_rmsSide; }

    // Retrieve visual trajectory buffer
    int getPoints(std::vector<GoniometerPoint>& outPoints) const;

    // Reset meters and display points
    void reset();

private:
    void updateBallistics();

    double m_sampleRate;
    int m_historySize;
    GoniometerDisplayMode m_displayMode;
    double m_persistence;
    double m_smoothingCoeff;

    // Ring buffer of display points
    std::vector<GoniometerPoint> m_points;
    int m_writeIndex;
    int m_pointCount;

    // Correlation accumulators (short term window)
    double m_sumLR;
    double m_sumLL;
    double m_sumRR;
    double m_sumM;
    double m_sumS;
    int m_windowCount;
    int m_windowSize;

    double m_instantCorrelation;
    double m_smoothedCorrelation;
    double m_stereoBalance;
    double m_stereoWidth;
    bool m_monoCancellationWarning;
    int m_outOfPhaseCounter;

    double m_rmsLeft;
    double m_rmsRight;
    double m_rmsMid;
    double m_rmsSide;
};

} // namespace Jazz

#endif // JAZZ_GONIOMETER_VECTORSCOPE_H
