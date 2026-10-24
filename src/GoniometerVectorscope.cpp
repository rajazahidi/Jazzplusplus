#include "GoniometerVectorscope.h"
#include <algorithm>
#include <cmath>

namespace Jazz {

static const double INV_SQRT2 = 0.7071067811865475;

JZGoniometerVectorscope::JZGoniometerVectorscope(int historySize, double sampleRate)
    : m_sampleRate(sampleRate)
    , m_historySize(historySize > 64 ? historySize : 1024)
    , m_displayMode(GoniometerDisplayMode::LISSAJOUS_XY)
    , m_persistence(0.85)
    , m_smoothingCoeff(0.92)
    , m_writeIndex(0)
    , m_pointCount(0)
    , m_sumLR(0.0)
    , m_sumLL(0.0)
    , m_sumRR(0.0)
    , m_sumM(0.0)
    , m_sumS(0.0)
    , m_windowCount(0)
    , m_windowSize(256)
    , m_instantCorrelation(1.0)
    , m_smoothedCorrelation(1.0)
    , m_stereoBalance(0.0)
    , m_stereoWidth(0.0)
    , m_monoCancellationWarning(false)
    , m_outOfPhaseCounter(0)
    , m_rmsLeft(0.0)
    , m_rmsRight(0.0)
    , m_rmsMid(0.0)
    , m_rmsSide(0.0)
{
    m_points.resize(m_historySize);
    for (int i = 0; i < m_historySize; ++i) {
        m_points[i] = {0.0f, 0.0f, 0.0f};
    }
    setCorrelationSmoothing(150.0);
}

void JZGoniometerVectorscope::setSampleRate(double sampleRate) {
    if (sampleRate > 1000.0) {
        m_sampleRate = sampleRate;
        setCorrelationSmoothing(150.0);
    }
}

void JZGoniometerVectorscope::setHistorySize(int numPoints) {
    if (numPoints >= 64 && numPoints <= 65536) {
        m_historySize = numPoints;
        m_points.assign(m_historySize, {0.0f, 0.0f, 0.0f});
        m_writeIndex = 0;
        m_pointCount = 0;
    }
}

void JZGoniometerVectorscope::setPersistence(double persistence) {
    m_persistence = std::max(0.0, std::min(0.999, persistence));
}

void JZGoniometerVectorscope::setCorrelationSmoothing(double timeMs) {
    if (timeMs <= 1.0) {
        m_smoothingCoeff = 0.0;
    } else {
        double windowsPerSec = m_sampleRate / static_cast<double>(m_windowSize);
        double tau = (timeMs * 0.001) * windowsPerSec;
        if (tau <= 0.01) {
            m_smoothingCoeff = 0.0;
        } else {
            m_smoothingCoeff = std::exp(-1.0 / tau);
        }
    }
}

void JZGoniometerVectorscope::reset() {
    m_writeIndex = 0;
    m_pointCount = 0;
    for (int i = 0; i < m_historySize; ++i) {
        m_points[i] = {0.0f, 0.0f, 0.0f};
    }
    m_sumLR = 0.0;
    m_sumLL = 0.0;
    m_sumRR = 0.0;
    m_sumM = 0.0;
    m_sumS = 0.0;
    m_windowCount = 0;
    m_instantCorrelation = 1.0;
    m_smoothedCorrelation = 1.0;
    m_stereoBalance = 0.0;
    m_stereoWidth = 0.0;
    m_monoCancellationWarning = false;
    m_outOfPhaseCounter = 0;
    m_rmsLeft = 0.0;
    m_rmsRight = 0.0;
    m_rmsMid = 0.0;
    m_rmsSide = 0.0;
}

void JZGoniometerVectorscope::processSample(double left, double right) {
    // Lissajous / Polar transformation
    float px = 0.0f;
    float py = 0.0f;

    double mid = (left + right) * INV_SQRT2;
    double side = (left - right) * INV_SQRT2;

    switch (m_displayMode) {
        case GoniometerDisplayMode::LISSAJOUS_XY:
            // Standard 45-degree rotated display:
            // X = Side (L - R), Y = Mid (L + R)
            px = static_cast<float>(side);
            py = static_cast<float>(mid);
            break;

        case GoniometerDisplayMode::POLAR_RADAR: {
            double radius = std::sqrt(left * left + right * right);
            double angle = std::atan2(right, left) - (3.14159265358979323846 / 4.0);
            px = static_cast<float>(radius * std::sin(angle));
            py = static_cast<float>(radius * std::cos(angle));
            break;
        }

        case GoniometerDisplayMode::LINEAR_MID_SIDE:
            px = static_cast<float>(side);
            py = static_cast<float>(mid);
            break;
    }

    // Clamp coordinates to [-1.0, 1.0]
    px = std::max(-1.0f, std::min(1.0f, px));
    py = std::max(-1.0f, std::min(1.0f, py));

    // Store in ring buffer
    m_points[m_writeIndex] = {px, py, 1.0f};
    m_writeIndex = (m_writeIndex + 1) % m_historySize;
    if (m_pointCount < m_historySize) {
        m_pointCount++;
    }

    // Accumulate metrics
    m_sumLL += left * left;
    m_sumRR += right * right;
    m_sumLR += left * right;
    m_sumM += mid * mid;
    m_sumS += side * side;
    m_windowCount++;

    if (m_windowCount >= m_windowSize) {
        double denom = std::sqrt(m_sumLL * m_sumRR);
        if (denom > 1e-9) {
            m_instantCorrelation = m_sumLR / denom;
        } else {
            // Silence or one channel zero
            m_instantCorrelation = 0.0;
        }
        m_instantCorrelation = std::max(-1.0, std::min(1.0, m_instantCorrelation));

        // Smooth correlation
        m_smoothedCorrelation = m_smoothingCoeff * m_smoothedCorrelation + (1.0 - m_smoothingCoeff) * m_instantCorrelation;

        // RMS calculations
        m_rmsLeft = std::sqrt(m_sumLL / m_windowSize);
        m_rmsRight = std::sqrt(m_sumRR / m_windowSize);
        m_rmsMid = std::sqrt(m_sumM / m_windowSize);
        m_rmsSide = std::sqrt(m_sumS / m_windowSize);

        // Stereo balance: -1.0 (L) to +1.0 (R)
        double totalRMS = m_rmsLeft + m_rmsRight;
        if (totalRMS > 1e-6) {
            m_stereoBalance = (m_rmsRight - m_rmsLeft) / totalRMS;
        } else {
            m_stereoBalance = 0.0;
        }

        // Stereo width: Side / Mid
        if (m_rmsMid > 1e-6) {
            m_stereoWidth = m_rmsSide / m_rmsMid;
        } else if (m_rmsSide > 1e-6) {
            m_stereoWidth = 10.0; // fully out of phase side signal
        } else {
            m_stereoWidth = 0.0;
        }

        // Mono cancellation warning threshold: smoothed correlation < -0.25
        if (m_smoothedCorrelation < -0.25) {
            m_outOfPhaseCounter++;
            if (m_outOfPhaseCounter >= 2) {
                m_monoCancellationWarning = true;
            }
        } else {
            m_outOfPhaseCounter = 0;
            m_monoCancellationWarning = false;
        }

        // Reset accumulators
        m_sumLL = 0.0;
        m_sumRR = 0.0;
        m_sumLR = 0.0;
        m_sumM = 0.0;
        m_sumS = 0.0;
        m_windowCount = 0;
    }
}

void JZGoniometerVectorscope::processBlock(const double* left, const double* right, int numSamples) {
    if (!left || !right || numSamples <= 0) return;

    for (int i = 0; i < numSamples; ++i) {
        processSample(left[i], right[i]);
    }
}

int JZGoniometerVectorscope::getPoints(std::vector<GoniometerPoint>& outPoints) const {
    outPoints.clear();
    outPoints.reserve(m_pointCount);

    // Read points starting from oldest
    int startIdx = (m_writeIndex - m_pointCount + m_historySize) % m_historySize;
    for (int i = 0; i < m_pointCount; ++i) {
        int idx = (startIdx + i) % m_historySize;
        GoniometerPoint pt = m_points[idx];
        // Calculate age-based phosphor decay intensity
        float ageFactor = static_cast<float>(i) / static_cast<float>(m_pointCount);
        pt.intensity = std::pow(static_cast<float>(m_persistence), (1.0f - ageFactor) * 20.0f);
        if (pt.intensity > 0.02f) {
            outPoints.push_back(pt);
        }
    }
    return static_cast<int>(outPoints.size());
}

} // namespace Jazz
