//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// BinauralAcousticRoomConvolverEngine.cpp - Virtual Acoustic Room Convolver
//*****************************************************************************

#include "BinauralAcousticRoomConvolverEngine.h"
#include <cmath>
#include <algorithm>
#include <numeric>

namespace Jazz {

static constexpr float SPEED_OF_SOUND = 343.0f; // m/s in air at 20°C
static constexpr float HEAD_RADIUS = 0.0875f;    // Average adult head radius (m)
static constexpr float PI_F = 3.14159265358979323846f;

JZBinauralAcousticRoomConvolverEngine::JZBinauralAcousticRoomConvolverEngine(float sampleRate)
    : m_sampleRate(sampleRate)
    , m_bufferSize(0)
    , m_writePos(0)
    , m_crossfeedL2R(0.0f)
    , m_crossfeedR2L(0.0f)
{
    for (int i = 0; i < NUM_COMBS; ++i) {
        m_combIndices[i] = 0;
        m_combSizes[i] = 0;
        m_combFeedback[i] = 0.0f;
        m_combFilterState[i] = 0.0f;
    }
    for (int i = 0; i < NUM_ALLPASS; ++i) {
        m_allpassIndices[i] = 0;
        m_allpassSizes[i] = 0;
    }

    initialize(sampleRate);
}

void JZBinauralAcousticRoomConvolverEngine::initialize(float sampleRate)
{
    m_sampleRate = (sampleRate > 8000.0f) ? sampleRate : 48000.0f;

    // Allocate delay buffer: 250ms maximum early reflection delay
    m_bufferSize = static_cast<int>(m_sampleRate * 0.25f) + 16;
    m_delayBufferL.assign(m_bufferSize, 0.0f);
    m_delayBufferR.assign(m_bufferSize, 0.0f);
    m_writePos = 0;

    // Initialize Comb & Allpass delay buffers
    static const int BASE_COMB_TUNINGS[NUM_COMBS] = { 1116, 1188, 1277, 1356 };
    static const int BASE_ALLPASS_TUNINGS[NUM_ALLPASS] = { 225, 556 };
    float srScale = m_sampleRate / 44100.0f;

    for (int i = 0; i < NUM_COMBS; ++i) {
        m_combSizes[i] = static_cast<int>(BASE_COMB_TUNINGS[i] * srScale);
        m_combBuffers[i].assign(m_combSizes[i], 0.0f);
        m_combIndices[i] = 0;
        m_combFilterState[i] = 0.0f;
    }

    for (int i = 0; i < NUM_ALLPASS; ++i) {
        m_allpassSizes[i] = static_cast<int>(BASE_ALLPASS_TUNINGS[i] * srScale);
        m_allpassBuffers[i].assign(m_allpassSizes[i], 0.0f);
        m_allpassIndices[i] = 0;
    }

    m_crossfeedL2R = 0.0f;
    m_crossfeedR2L = 0.0f;

    rebuildAcousticModel();
}

void JZBinauralAcousticRoomConvolverEngine::reset()
{
    std::fill(m_delayBufferL.begin(), m_delayBufferL.end(), 0.0f);
    std::fill(m_delayBufferR.begin(), m_delayBufferR.end(), 0.0f);
    m_writePos = 0;

    for (int i = 0; i < NUM_COMBS; ++i) {
        std::fill(m_combBuffers[i].begin(), m_combBuffers[i].end(), 0.0f);
        m_combIndices[i] = 0;
        m_combFilterState[i] = 0.0f;
    }

    for (int i = 0; i < NUM_ALLPASS; ++i) {
        std::fill(m_allpassBuffers[i].begin(), m_allpassBuffers[i].end(), 0.0f);
        m_allpassIndices[i] = 0;
    }

    for (auto& tap : m_tapsL) {
        tap.filterStateL = 0.0f;
        tap.filterStateR = 0.0f;
    }
    for (auto& tap : m_tapsR) {
        tap.filterStateL = 0.0f;
        tap.filterStateR = 0.0f;
    }

    m_crossfeedL2R = 0.0f;
    m_crossfeedR2L = 0.0f;
}

void JZBinauralAcousticRoomConvolverEngine::setParameters(const Parameters& params)
{
    m_params = params;
    m_params.roomSizeFactor = std::max(0.5f, std::min(m_params.roomSizeFactor, 2.0f));
    m_params.damping = std::max(0.05f, std::min(m_params.damping, 0.95f));
    m_params.stereoSpread = std::max(0.0f, std::min(m_params.stereoSpread, 1.5f));
    m_params.dryWet = std::max(0.0f, std::min(m_params.dryWet, 1.0f));
    rebuildAcousticModel();
}

void JZBinauralAcousticRoomConvolverEngine::setRoomDimensions(float lengthM, float widthM, float heightM)
{
    m_params.roomLengthMeters = std::max(2.0f, lengthM);
    m_params.roomWidthMeters = std::max(2.0f, widthM);
    m_params.roomHeightMeters = std::max(2.0f, heightM);
    rebuildAcousticModel();
}

void JZBinauralAcousticRoomConvolverEngine::setRoomSizeFactor(float factor)
{
    m_params.roomSizeFactor = std::max(0.5f, std::min(factor, 2.0f));
    rebuildAcousticModel();
}

void JZBinauralAcousticRoomConvolverEngine::setDamping(float damping)
{
    m_params.damping = std::max(0.05f, std::min(damping, 0.95f));
    updateCombFilterGains();
}

void JZBinauralAcousticRoomConvolverEngine::setEarlyReflectionLevelDb(float db)
{
    m_params.earlyReflectionLevelDb = std::max(-60.0f, std::min(db, 0.0f));
}

void JZBinauralAcousticRoomConvolverEngine::setLateReverbLevelDb(float db)
{
    m_params.lateReverbLevelDb = std::max(-60.0f, std::min(db, 0.0f));
}

void JZBinauralAcousticRoomConvolverEngine::setStereoSpread(float spread)
{
    m_params.stereoSpread = std::max(0.0f, std::min(spread, 1.5f));
    rebuildAcousticModel();
}

void JZBinauralAcousticRoomConvolverEngine::setDryWet(float wet)
{
    m_params.dryWet = std::max(0.0f, std::min(wet, 1.0f));
}

void JZBinauralAcousticRoomConvolverEngine::setHeadphonesCrossfeed(bool enabled)
{
    m_params.headphonesCrossfeed = enabled;
}

void JZBinauralAcousticRoomConvolverEngine::setBypass(bool bypass)
{
    m_params.bypassed = bypass;
}

float JZBinauralAcousticRoomConvolverEngine::getRoomVolumeCubicMeters() const
{
    float L = m_params.roomLengthMeters * m_params.roomSizeFactor;
    float W = m_params.roomWidthMeters * m_params.roomSizeFactor;
    float H = m_params.roomHeightMeters * m_params.roomSizeFactor;
    return L * W * H;
}

float JZBinauralAcousticRoomConvolverEngine::getTotalSurfaceAreaSquareMeters() const
{
    float L = m_params.roomLengthMeters * m_params.roomSizeFactor;
    float W = m_params.roomWidthMeters * m_params.roomSizeFactor;
    float H = m_params.roomHeightMeters * m_params.roomSizeFactor;
    return 2.0f * (L * W + L * H + W * H);
}

int JZBinauralAcousticRoomConvolverEngine::getEarlyReflectionCount() const
{
    return static_cast<int>(m_tapsL.size() + m_tapsR.size());
}

float JZBinauralAcousticRoomConvolverEngine::getAverageAbsorption() const
{
    return 0.15f + 0.65f * m_params.damping;
}

float JZBinauralAcousticRoomConvolverEngine::getRt60Seconds() const
{
    float V = getRoomVolumeCubicMeters();
    float S = getTotalSurfaceAreaSquareMeters();
    float alpha = getAverageAbsorption();
    if (alpha >= 0.99f) alpha = 0.99f;
    // Eyring formula: RT60 = 0.161 * V / (-S * ln(1 - alpha))
    float rt60 = (0.161f * V) / (-S * std::log(1.0f - alpha));
    return std::max(0.1f, std::min(rt60, 5.0f));
}

void JZBinauralAcousticRoomConvolverEngine::updateCombFilterGains()
{
    float rt60 = getRt60Seconds();
    for (int i = 0; i < NUM_COMBS; ++i) {
        float delaySec = static_cast<float>(m_combSizes[i]) / m_sampleRate;
        m_combFeedback[i] = std::pow(10.0f, -3.0f * delaySec / rt60);
        m_combFeedback[i] = std::max(0.1f, std::min(m_combFeedback[i], 0.96f));
    }
}

void JZBinauralAcousticRoomConvolverEngine::rebuildAcousticModel()
{
    m_tapsL.clear();
    m_tapsR.clear();

    float L = m_params.roomLengthMeters * m_params.roomSizeFactor;
    float W = m_params.roomWidthMeters * m_params.roomSizeFactor;
    float H = m_params.roomHeightMeters * m_params.roomSizeFactor;

    // Listener at sweet spot: center width (X=0), 1/4 length back (Y=-L*0.25f), seated height (Z=1.2m)
    float lx = 0.0f;
    float ly = -L * 0.25f;
    float lz = 1.2f;

    // Stereo monitor speakers at 60-degree equilateral triangle
    float speakerSpread = (W * 0.28f) * m_params.stereoSpread;
    float speakerY = ly + speakerSpread * 1.732f * 0.5f; // ~60 deg equilateral apex
    float speakerZ = 1.2f; // ear level

    struct Point3D { float x, y, z; };
    Point3D spkL = { -speakerSpread, speakerY, speakerZ };
    Point3D spkR = {  speakerSpread, speakerY, speakerZ };

    float directDistL = std::sqrt((spkL.x - lx)*(spkL.x - lx) + (spkL.y - ly)*(spkL.y - ly) + (spkL.z - lz)*(spkL.z - lz));
    float directDistR = std::sqrt((spkR.x - lx)*(spkR.x - lx) + (spkR.y - ly)*(spkR.y - ly) + (spkR.z - lz)*(spkR.z - lz));
    if (directDistL < 0.1f) directDistL = 0.1f;
    if (directDistR < 0.1f) directDistR = 0.1f;

    // Wall positions relative to room center
    float minX = -W * 0.5f, maxX = W * 0.5f;
    float minY = -L * 0.5f, maxY = L * 0.5f;
    float minZ = 0.0f,      maxZ = H;

    float alpha = getAverageAbsorption();
    float reflCoeff1 = std::sqrt(std::max(0.01f, 1.0f - alpha));
    float reflCoeff2 = reflCoeff1 * reflCoeff1;

    // Generator lambda for 12 image sources per speaker
    auto generateTaps = [&](const Point3D& spk, std::vector<ReflectionTap>& taps, float directDist) {
        struct ImageSource {
            Point3D pos;
            float coeff;
        };

        std::vector<ImageSource> sources = {
            // 1st order reflections (6 boundary walls)
            { { spk.x, spk.y, 2.0f * minZ - spk.z }, reflCoeff1 },          // Floor
            { { spk.x, spk.y, 2.0f * maxZ - spk.z }, reflCoeff1 },          // Ceiling
            { { spk.x, 2.0f * maxY - spk.y, spk.z }, reflCoeff1 },          // Front wall
            { { spk.x, 2.0f * minY - spk.y, spk.z }, reflCoeff1 },          // Back wall
            { { 2.0f * minX - spk.x, spk.y, spk.z }, reflCoeff1 },          // Left wall
            { { 2.0f * maxX - spk.x, spk.y, spk.z }, reflCoeff1 },          // Right wall

            // 2nd order reflections (6 corner & axial combinations)
            { { 2.0f * minX - spk.x, 2.0f * maxY - spk.y, spk.z }, reflCoeff2 }, // Left + Front
            { { 2.0f * maxX - spk.x, 2.0f * maxY - spk.y, spk.z }, reflCoeff2 }, // Right + Front
            { { 2.0f * minX - spk.x, 2.0f * minY - spk.y, spk.z }, reflCoeff2 }, // Left + Back
            { { 2.0f * maxX - spk.x, 2.0f * minY - spk.y, spk.z }, reflCoeff2 }, // Right + Back
            { { 2.0f * minX - spk.x, spk.y, 2.0f * minZ - spk.z }, reflCoeff2 }, // Left + Floor
            { { 2.0f * maxX - spk.x, spk.y, 2.0f * minZ - spk.z }, reflCoeff2 }  // Right + Floor
        };

        for (const auto& isrc : sources) {
            float dx = isrc.pos.x - lx;
            float dy = isrc.pos.y - ly;
            float dz = isrc.pos.z - lz;
            float dist = std::sqrt(dx*dx + dy*dy + dz*dz);
            if (dist < 0.2f) dist = 0.2f;

            // Propagation delay in seconds
            float delaySec = dist / SPEED_OF_SOUND;
            float delaySamples = delaySec * m_sampleRate;
            if (delaySamples >= m_bufferSize - 4.0f) {
                delaySamples = m_bufferSize - 4.0f;
            }

            // Azimuth angle relative to listener orientation (facing +Y)
            float azimuth = std::atan2(dx, dy); // 0 = front, +pi/2 = right, -pi/2 = left

            // Binaural Interaural Time Difference (Woodworth spherical model approximation)
            float itdSec = (HEAD_RADIUS / SPEED_OF_SOUND) * std::sin(azimuth);
            float itdSamples = itdSec * m_sampleRate;

            float delaySamplesL = std::max(1.0f, delaySamples + itdSamples * 0.5f);
            float delaySamplesR = std::max(1.0f, delaySamples - itdSamples * 0.5f);

            // Distance attenuation 1/d relative to direct distance
            float distAtten = directDist / dist;
            if (distAtten > 1.0f) distAtten = 1.0f;

            // Head shadow ILD (Interaural Level Difference)
            float ildL = 1.0f - 0.35f * std::max(0.0f, std::sin(azimuth));
            float ildR = 1.0f - 0.35f * std::max(0.0f, -std::sin(azimuth));

            ReflectionTap tap;
            tap.delaySamplesL = delaySamplesL;
            tap.delaySamplesR = delaySamplesR;
            tap.gainL = distAtten * isrc.coeff * ildL;
            tap.gainR = distAtten * isrc.coeff * ildR;
            tap.hfDamping = std::max(0.1f, 1.0f - 0.55f * m_params.damping);
            tap.filterStateL = 0.0f;
            tap.filterStateR = 0.0f;

            taps.push_back(tap);
        }
    };

    generateTaps(spkL, m_tapsL, directDistL);
    generateTaps(spkR, m_tapsR, directDistR);

    updateCombFilterGains();
}

void JZBinauralAcousticRoomConvolverEngine::processStereo(
    const float* inL, const float* inR, float* outL, float* outR, int numSamples)
{
    if (!inL || !inR || !outL || !outR || numSamples <= 0) return;

    if (m_params.bypassed) {
        std::copy(inL, inL + numSamples, outL);
        std::copy(inR, inR + numSamples, outR);
        return;
    }

    float erGain = std::pow(10.0f, m_params.earlyReflectionLevelDb / 20.0f);
    float lateGain = std::pow(10.0f, m_params.lateReverbLevelDb / 20.0f);
    float dryGain = 1.0f - m_params.dryWet;
    float wetGain = m_params.dryWet;

    // Headphone crossfeed parameters: 0.28ms delay with 2kHz 1-pole lowpass
    float crossfeedFilterCoeff = 0.35f;

    for (int i = 0; i < numSamples; ++i) {
        float xL = inL[i];
        float xR = inR[i];

        // Write stereo input to ring buffer
        m_delayBufferL[m_writePos] = xL;
        m_delayBufferR[m_writePos] = xR;

        // 1. Process Early Reflections
        float erAccumL = 0.0f;
        float erAccumR = 0.0f;

        // Reflections from Left Speaker
        for (auto& tap : m_tapsL) {
            // Read left ear tap
            float rPosL = m_writePos - tap.delaySamplesL;
            if (rPosL < 0.0f) rPosL += m_bufferSize;
            int idx0L = static_cast<int>(rPosL);
            int idx1L = (idx0L + 1) % m_bufferSize;
            float fracL = rPosL - idx0L;
            float smpL = m_delayBufferL[idx0L] * (1.0f - fracL) + m_delayBufferL[idx1L] * fracL;
            tap.filterStateL += tap.hfDamping * (smpL - tap.filterStateL);
            erAccumL += tap.filterStateL * tap.gainL;

            // Read right ear tap
            float rPosR = m_writePos - tap.delaySamplesR;
            if (rPosR < 0.0f) rPosR += m_bufferSize;
            int idx0R = static_cast<int>(rPosR);
            int idx1R = (idx0R + 1) % m_bufferSize;
            float fracR = rPosR - idx0R;
            float smpR = m_delayBufferL[idx0R] * (1.0f - fracR) + m_delayBufferL[idx1R] * fracR;
            tap.filterStateR += tap.hfDamping * (smpR - tap.filterStateR);
            erAccumR += tap.filterStateR * tap.gainR;
        }

        // Reflections from Right Speaker
        for (auto& tap : m_tapsR) {
            // Read left ear tap
            float rPosL = m_writePos - tap.delaySamplesL;
            if (rPosL < 0.0f) rPosL += m_bufferSize;
            int idx0L = static_cast<int>(rPosL);
            int idx1L = (idx0L + 1) % m_bufferSize;
            float fracL = rPosL - idx0L;
            float smpL = m_delayBufferR[idx0L] * (1.0f - fracL) + m_delayBufferR[idx1L] * fracL;
            tap.filterStateL += tap.hfDamping * (smpL - tap.filterStateL);
            erAccumL += tap.filterStateL * tap.gainL;

            // Read right ear tap
            float rPosR = m_writePos - tap.delaySamplesR;
            if (rPosR < 0.0f) rPosR += m_bufferSize;
            int idx0R = static_cast<int>(rPosR);
            int idx1R = (idx0R + 1) % m_bufferSize;
            float fracR = rPosR - idx0R;
            float smpR = m_delayBufferR[idx0R] * (1.0f - fracR) + m_delayBufferR[idx1R] * fracR;
            tap.filterStateR += tap.hfDamping * (smpR - tap.filterStateR);
            erAccumR += tap.filterStateR * tap.gainR;
        }

        // 2. Process Diffuse Late Reverb Tail (Comb + Allpass)
        float lateIn = (xL + xR) * 0.5f;
        float combSumL = 0.0f;
        float combSumR = 0.0f;

        for (int c = 0; c < NUM_COMBS; ++c) {
            float delayed = m_combBuffers[c][m_combIndices[c]];
            m_combFilterState[c] += (1.0f - m_params.damping * 0.5f) * (delayed - m_combFilterState[c]);
            m_combBuffers[c][m_combIndices[c]] = lateIn + m_combFilterState[c] * m_combFeedback[c];

            m_combIndices[c]++;
            if (m_combIndices[c] >= m_combSizes[c]) m_combIndices[c] = 0;

            if (c % 2 == 0) {
                combSumL += delayed;
                combSumR -= delayed * 0.5f;
            } else {
                combSumR += delayed;
                combSumL -= delayed * 0.5f;
            }
        }

        // Pass through Allpass filters for diffusion
        float allpassOutL = combSumL;
        float allpassOutR = combSumR;

        for (int a = 0; a < NUM_ALLPASS; ++a) {
            float bufValL = m_allpassBuffers[a][m_allpassIndices[a]];
            float apOutL = -allpassOutL + bufValL;
            m_allpassBuffers[a][m_allpassIndices[a]] = allpassOutL + bufValL * 0.5f;
            allpassOutL = apOutL;

            m_allpassIndices[a]++;
            if (m_allpassIndices[a] >= m_allpassSizes[a]) m_allpassIndices[a] = 0;
        }
        allpassOutR = combSumR; // Decorrelated late tail

        // Direct sound with optional binaural crossfeed
        float directL = xL;
        float directR = xR;
        if (m_params.headphonesCrossfeed) {
            m_crossfeedL2R += crossfeedFilterCoeff * (xL * 0.22f - m_crossfeedL2R);
            m_crossfeedR2L += crossfeedFilterCoeff * (xR * 0.22f - m_crossfeedR2L);
            directL = directL * 0.88f + m_crossfeedR2L;
            directR = directR * 0.88f + m_crossfeedL2R;
        }

        float wetL = erAccumL * erGain + allpassOutL * lateGain;
        float wetR = erAccumR * erGain + allpassOutR * lateGain;

        outL[i] = directL * dryGain + wetL * wetGain;
        outR[i] = directR * dryGain + wetR * wetGain;

        // Advance write pointer
        m_writePos++;
        if (m_writePos >= m_bufferSize) m_writePos = 0;
    }
}

} // namespace Jazz
