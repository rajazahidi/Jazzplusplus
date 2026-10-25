//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// HighOrderAmbisonics3rdOrderEngine.cpp - 3rd-Order HOA Spherical Harmonics
//*****************************************************************************

#include "HighOrderAmbisonics3rdOrderEngine.h"
#include <cmath>
#include <algorithm>

namespace Jazz {

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

JZHighOrderAmbisonics3rdOrderEngine::JZHighOrderAmbisonics3rdOrderEngine(float sampleRate) {
    initialize(sampleRate);
}

void JZHighOrderAmbisonics3rdOrderEngine::initialize(float sampleRate) {
    if (sampleRate < 8000.0f) sampleRate = 48000.0f;
    m_sampleRate = sampleRate;

    reset();
}

void JZHighOrderAmbisonics3rdOrderEngine::reset() {
    m_source = SpatialSource();
    m_orient = Orientation();

    float azRad = m_source.azimuthDeg * (static_cast<float>(M_PI) / 180.0f);
    float elRad = m_source.elevationDeg * (static_cast<float>(M_PI) / 180.0f);
    computeSphericalHarmonics(azRad, elRad, m_shWeights);
}

void JZHighOrderAmbisonics3rdOrderEngine::setSourcePosition(const SpatialSource& src) {
    m_source = src;
    m_source.azimuthDeg = std::max(-180.0f, std::min(180.0f, m_source.azimuthDeg));
    m_source.elevationDeg = std::max(-90.0f, std::min(90.0f, m_source.elevationDeg));
    m_source.distanceMeters = std::max(0.2f, std::min(50.0f, m_source.distanceMeters));

    float azRad = m_source.azimuthDeg * (static_cast<float>(M_PI) / 180.0f);
    float elRad = m_source.elevationDeg * (static_cast<float>(M_PI) / 180.0f);
    computeSphericalHarmonics(azRad, elRad, m_shWeights);
}

void JZHighOrderAmbisonics3rdOrderEngine::setListenerOrientation(const Orientation& ori) {
    m_orient = ori;
}

void JZHighOrderAmbisonics3rdOrderEngine::computeSphericalHarmonics(
    float azRad, float elRad, float* outWeights) {

    if (!outWeights) return;

    // Direction cosines (OpenGL / AES standard: X=Front, Y=Left, Z=Up)
    float cosEl = std::cos(elRad);
    float sinEl = std::sin(elRad);
    float cosAz = std::cos(azRad);
    float sinAz = std::sin(azRad);

    float x = cosEl * cosAz;
    float y = cosEl * sinAz;
    float z = sinEl;

    // Order 0 (1 channel)
    outWeights[0] = 1.0f; // ACN 0: W

    // Order 1 (3 channels)
    outWeights[1] = y;    // ACN 1: Y
    outWeights[2] = z;    // ACN 2: Z
    outWeights[3] = x;    // ACN 3: X

    // Order 2 (5 channels) - SN3D normalized
    float sqrt3 = 1.7320508f;
    outWeights[4] = sqrt3 * x * y;                            // ACN 4: V
    outWeights[5] = sqrt3 * y * z;                            // ACN 5: T
    outWeights[6] = 0.5f * (3.0f * z * z - 1.0f);             // ACN 6: R
    outWeights[7] = sqrt3 * x * z;                            // ACN 7: S
    outWeights[8] = 0.5f * sqrt3 * (x * x - y * y);           // ACN 8: U

    // Order 3 (7 channels) - SN3D normalized
    float sqrt5_8 = 0.7905694f;
    float sqrt15 = 3.8729833f;
    float sqrt3_8 = 0.6123724f;

    outWeights[9]  = sqrt5_8 * y * (3.0f * x * x - y * y);    // ACN 9: Q
    outWeights[10] = sqrt15 * x * y * z;                      // ACN 10: O
    outWeights[11] = sqrt3_8 * y * (5.0f * z * z - 1.0f);     // ACN 11: M
    outWeights[12] = 0.5f * z * (5.0f * z * z - 3.0f);        // ACN 12: K
    outWeights[13] = sqrt3_8 * x * (5.0f * z * z - 1.0f);     // ACN 13: L
    outWeights[14] = 0.5f * sqrt15 * z * (x * x - y * y);     // ACN 14: N
    outWeights[15] = sqrt5_8 * x * (x * x - 3.0f * y * y);    // ACN 15: P
}

void JZHighOrderAmbisonics3rdOrderEngine::encodeMono(
    const float* inMono,
    float* const* hoaOutputs,
    size_t numSamples) {

    if (!inMono || !hoaOutputs || numSamples == 0) return;

    // Distance attenuation (1/r with nearfield damping)
    float distAtten = 1.0f / std::max(1.0f, m_source.distanceMeters);

    for (size_t ch = 0; ch < NUM_HOA_CHANNELS; ++ch) {
        float* chOut = hoaOutputs[ch];
        if (!chOut) continue;
        float weight = m_shWeights[ch] * distAtten;

        for (size_t i = 0; i < numSamples; ++i) {
            chOut[i] = inMono[i] * weight;
        }
    }
}

void JZHighOrderAmbisonics3rdOrderEngine::rotateSoundfield(
    float* const* hoaBuffers, size_t numSamples) {

    if (!hoaBuffers || numSamples == 0) return;

    // Yaw rotation around Z axis (affects X and Y in Order 1)
    float yawRad = m_orient.yawDeg * (static_cast<float>(M_PI) / 180.0f);
    float cosYaw = std::cos(yawRad);
    float sinYaw = std::sin(yawRad);

    float* chY = hoaBuffers[1]; // ACN 1
    float* chX = hoaBuffers[3]; // ACN 3

    if (chY && chX) {
        for (size_t i = 0; i < numSamples; ++i) {
            float y = chY[i];
            float x = chX[i];
            chX[i] = x * cosYaw - y * sinYaw;
            chY[i] = x * sinYaw + y * cosYaw;
        }
    }
}

void JZHighOrderAmbisonics3rdOrderEngine::decodeToBinaural(
    const float* const* hoaInputs,
    float* outL, float* outR,
    size_t numSamples) {

    if (!hoaInputs || !outL || !outR || numSamples == 0) return;

    // Order max-rE energy-preserving decoding weights
    // g0 = 1.0, g1 = 0.838, g2 = 0.577, g3 = 0.301
    static const float maxReWeights[NUM_HOA_CHANNELS] = {
        1.000f,                                                 // Order 0: W
        0.838f, 0.838f, 0.838f,                                 // Order 1: Y, Z, X
        0.577f, 0.577f, 0.577f, 0.577f, 0.577f,                 // Order 2: V, T, R, S, U
        0.301f, 0.301f, 0.301f, 0.301f, 0.301f, 0.301f, 0.301f  // Order 3: Q, O, M, K, L, N, P
    };

    // Virtual headphone ear projection coefficients (Left=+90 deg, Right=-90 deg)
    // Left ear gets positive Y, positive V, positive Q; Right ear gets opposite sign on odd Y harmonics
    for (size_t i = 0; i < numSamples; ++i) {
        float sumL = 0.0f;
        float sumR = 0.0f;

        for (size_t ch = 0; ch < NUM_HOA_CHANNELS; ++ch) {
            if (!hoaInputs[ch]) continue;
            float s = hoaInputs[ch][i] * maxReWeights[ch];

            // W channel is omni
            if (ch == 0) {
                sumL += s * 0.7071f;
                sumR += s * 0.7071f;
            } else if (ch == 1) { // Y channel (Lateral Left-Right)
                sumL += s * 0.85f;
                sumR -= s * 0.85f;
            } else if (ch == 3) { // X channel (Front-Back)
                sumL += s * 0.45f;
                sumR += s * 0.45f;
            } else if (ch == 4 || ch == 9) { // Higher lateral harmonics
                sumL += s * 0.35f;
                sumR -= s * 0.35f;
            } else {
                sumL += s * 0.20f;
                sumR += s * 0.20f;
            }
        }

        outL[i] = sumL;
        outR[i] = sumR;
    }
}

float JZHighOrderAmbisonics3rdOrderEngine::getEncodingWeight(size_t acnIndex) const {
    if (acnIndex < NUM_HOA_CHANNELS) return m_shWeights[acnIndex];
    return 0.0f;
}

std::string JZHighOrderAmbisonics3rdOrderEngine::getChannelAcnName(size_t acnIndex) const {
    static const char* names[NUM_HOA_CHANNELS] = {
        "0: W (0,0)",
        "1: Y (1,-1)", "2: Z (1,0)", "3: X (1,1)",
        "4: V (2,-2)", "5: T (2,-1)", "6: R (2,0)", "7: S (2,1)", "8: U (2,2)",
        "9: Q (3,-3)", "10: O (3,-2)", "11: M (3,-1)", "12: K (3,0)", "13: L (3,1)", "14: N (3,2)", "15: P (3,3)"
    };
    if (acnIndex < NUM_HOA_CHANNELS) return names[acnIndex];
    return "Unknown";
}

} // namespace Jazz
