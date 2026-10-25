//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// RealDrumsEngine.cpp - Band-in-a-Box Style Acoustic RealDrums Engine
//*****************************************************************************

#include "RealDrumsEngine.h"
#include <cmath>
#include <cstring>
#include <algorithm>

namespace Jazz {

static constexpr float PI_F = 3.14159265358979323846f;
static constexpr float TWO_PI_F = 6.28318530717958647692f;

JZRealDrumsEngine::JZRealDrumsEngine(float sampleRate)
    : m_sampleRate(sampleRate)
{
    initialize(sampleRate);
}

void JZRealDrumsEngine::initialize(float sampleRate)
{
    m_sampleRate = (sampleRate > 8000.0f) ? sampleRate : 48000.0f;
    reset();

    // Early reflection room delay line (~40ms stereo)
    size_t roomDelaySize = static_cast<size_t>(m_sampleRate * 0.05f);
    m_roomDelayBufferL.assign(roomDelaySize, 0.0f);
    m_roomDelayBufferR.assign(roomDelaySize, 0.0f);
    m_roomDelayPos = 0;

    buildSampleLibrary();
}

void JZRealDrumsEngine::reset()
{
    for (size_t i = 0; i < MAX_VOICES; ++i) {
        m_voices[i].active = false;
        m_voices[i].midiNote = -1;
        m_voices[i].sample = nullptr;
        m_voices[i].playPos = 0.0f;
        m_voices[i].playInc = 1.0f;
        m_voices[i].gainSoft = 0.0f;
        m_voices[i].gainHard = 0.0f;
        m_voices[i].choking = false;
        m_voices[i].chokeGain = 1.0f;
        m_voices[i].snareBuzzGain = 0.0f;
        m_voices[i].snareBuzzPos = 0.0f;
    }
    m_totalHits = 0;
    m_lastVelocity = 0.0f;
    m_lastNote = 0;
    if (!m_roomDelayBufferL.empty()) {
        std::fill(m_roomDelayBufferL.begin(), m_roomDelayBufferL.end(), 0.0f);
        std::fill(m_roomDelayBufferR.begin(), m_roomDelayBufferR.end(), 0.0f);
    }
}

void JZRealDrumsEngine::setParameters(const Parameters& params)
{
    m_params = params;
    buildSampleLibrary(); // Rebuild kit if kit type changed
}

void JZRealDrumsEngine::setKitType(JZDrumKitType kit)
{
    if (m_params.kitType != kit) {
        m_params.kitType = kit;
        buildSampleLibrary();
    }
}

void JZRealDrumsEngine::setVelocitySensitivity(float sens)
{
    m_params.velocitySensitivity = std::clamp(sens, 0.1f, 1.0f);
}

void JZRealDrumsEngine::setChokeGroupEnabled(bool enable)
{
    m_params.enableChokeGroup = enable;
}

void JZRealDrumsEngine::setRoundRobinEnabled(bool enable)
{
    m_params.enableRoundRobin = enable;
}

void JZRealDrumsEngine::setSnareBuzzAmount(float amount)
{
    m_params.snareBuzzAmount = std::clamp(amount, 0.0f, 1.0f);
}

void JZRealDrumsEngine::setRoomAmbianceSend(float send)
{
    m_params.roomAmbianceSend = std::clamp(send, 0.0f, 1.0f);
}

void JZRealDrumsEngine::setMasterLevel(float level)
{
    m_params.masterLevel = std::clamp(level, 0.0f, 2.0f);
}

void JZRealDrumsEngine::setBypass(bool bypass)
{
    m_params.bypassed = bypass;
}

float JZRealDrumsEngine::xorshiftRandom()
{
    m_rngState ^= (m_rngState << 13);
    m_rngState ^= (m_rngState >> 17);
    m_rngState ^= (m_rngState << 5);
    return static_cast<float>(m_rngState) * (1.0f / 4294967296.0f);
}

std::string JZRealDrumsEngine::getKitName() const
{
    switch (m_params.kitType) {
        case JZDrumKitType::VINTAGE_MAPLE: return "Vintage Maple Studio";
        case JZDrumKitType::MODERN_BIRCH:  return "Modern Birch Recording";
        case JZDrumKitType::JAZZ_CUSTOM:   return "Jazz Club Custom";
        case JZDrumKitType::ROCK_STADIUM:  return "Rock Stadium Punch";
        default: return "Custom Drum Kit";
    }
}

size_t JZRealDrumsEngine::getActiveVoicesCount() const
{
    size_t count = 0;
    for (size_t i = 0; i < MAX_VOICES; ++i) {
        if (m_voices[i].active) {
            count++;
        }
    }
    return count;
}

JZDrumInstrument JZRealDrumsEngine::midiNoteToInstrument(int note) const
{
    switch (note) {
        case 35: // Acoustic Bass Drum
        case 36: // Bass Drum 1
            return JZDrumInstrument::KICK;
        case 38: // Acoustic Snare
        case 40: // Electric Snare
            return JZDrumInstrument::SNARE;
        case 37: // Side Stick
            return JZDrumInstrument::SIDE_STICK;
        case 42: // Closed Hi-Hat
            return JZDrumInstrument::HIHAT_CLOSED;
        case 44: // Pedal Hi-Hat
            return JZDrumInstrument::HIHAT_PEDAL;
        case 46: // Open Hi-Hat
            return JZDrumInstrument::HIHAT_OPEN;
        case 41: // Low Floor Tom
        case 43: // High Floor Tom
            return JZDrumInstrument::TOM_LOW;
        case 45: // Low Tom
        case 47: // Low-Mid Tom
            return JZDrumInstrument::TOM_MID;
        case 48: // Hi-Mid Tom
        case 50: // High Tom
            return JZDrumInstrument::TOM_HIGH;
        case 49: // Crash Cymbal 1
            return JZDrumInstrument::CRASH_1;
        case 57: // Crash Cymbal 2
            return JZDrumInstrument::CRASH_2;
        case 51: // Ride Cymbal 1
        case 59: // Ride Cymbal 2
            return JZDrumInstrument::RIDE;
        case 53: // Ride Bell
            return JZDrumInstrument::RIDE_BELL;
        case 54: // Tambourine
            return JZDrumInstrument::TAMBOURINE;
        case 56: // Cowbell
            return JZDrumInstrument::COWBELL;
        default:
            return JZDrumInstrument::COUNT;
    }
}

void JZRealDrumsEngine::buildSampleLibrary()
{
    for (int i = 0; i < static_cast<int>(JZDrumInstrument::COUNT); ++i) {
        generateInstrumentWaveforms(static_cast<JZDrumInstrument>(i), m_samples[i]);
    }

    // Pre-generate sympathetic snare wire buzz (150ms buffer)
    size_t buzzSamples = static_cast<size_t>(m_sampleRate * 0.15f);
    m_snareWireNoise.assign(buzzSamples, 0.0f);
    for (size_t i = 0; i < buzzSamples; ++i) {
        float t = static_cast<float>(i) / m_sampleRate;
        float decay = std::exp(-t * 28.0f);
        float noise = (xorshiftRandom() * 2.0f - 1.0f);
        m_snareWireNoise[i] = noise * decay * 0.45f;
    }
}

void JZRealDrumsEngine::generateInstrumentWaveforms(JZDrumInstrument inst, DrumSamplePair& pair)
{
    float durSec = 0.5f;
    float pan = 0.0f;

    switch (inst) {
        case JZDrumInstrument::KICK:
            durSec = 0.45f;
            pan = 0.0f;
            break;
        case JZDrumInstrument::SNARE:
            durSec = 0.55f;
            pan = -0.15f;
            break;
        case JZDrumInstrument::SIDE_STICK:
            durSec = 0.15f;
            pan = -0.2f;
            break;
        case JZDrumInstrument::HIHAT_CLOSED:
            durSec = 0.12f;
            pan = -0.45f;
            break;
        case JZDrumInstrument::HIHAT_PEDAL:
            durSec = 0.18f;
            pan = -0.35f;
            break;
        case JZDrumInstrument::HIHAT_OPEN:
            durSec = 0.90f;
            pan = -0.45f;
            break;
        case JZDrumInstrument::TOM_LOW:
            durSec = 0.65f;
            pan = 0.55f;
            break;
        case JZDrumInstrument::TOM_MID:
            durSec = 0.55f;
            pan = 0.20f;
            break;
        case JZDrumInstrument::TOM_HIGH:
            durSec = 0.45f;
            pan = -0.25f;
            break;
        case JZDrumInstrument::CRASH_1:
            durSec = 1.6f;
            pan = -0.60f;
            break;
        case JZDrumInstrument::CRASH_2:
            durSec = 1.8f;
            pan = 0.60f;
            break;
        case JZDrumInstrument::RIDE:
            durSec = 1.9f;
            pan = 0.55f;
            break;
        case JZDrumInstrument::RIDE_BELL:
            durSec = 1.2f;
            pan = 0.50f;
            break;
        case JZDrumInstrument::TAMBOURINE:
            durSec = 0.35f;
            pan = 0.30f;
            break;
        case JZDrumInstrument::COWBELL:
            durSec = 0.40f;
            pan = 0.10f;
            break;
        default:
            durSec = 0.3f;
            pan = 0.0f;
            break;
    }

    pair.basePan = pan;
    size_t numSamples = static_cast<size_t>(m_sampleRate * durSec);
    pair.soft.assign(numSamples, 0.0f);
    pair.hard.assign(numSamples, 0.0f);

    float kitTuningMult = 1.0f;
    if (m_params.kitType == JZDrumKitType::MODERN_BIRCH) kitTuningMult = 1.05f;
    else if (m_params.kitType == JZDrumKitType::JAZZ_CUSTOM) kitTuningMult = 1.12f;
    else if (m_params.kitType == JZDrumKitType::ROCK_STADIUM) kitTuningMult = 0.92f;

    for (size_t i = 0; i < numSamples; ++i) {
        float t = static_cast<float>(i) / m_sampleRate;

        switch (inst) {
            case JZDrumInstrument::KICK: {
                // Soft: deep sub body, warm decay
                float f0_soft = (52.0f * kitTuningMult) * std::exp(-t * 22.0f) + 40.0f;
                float phase_soft = TWO_PI_F * f0_soft * t;
                float env_soft = std::exp(-t * 12.0f);
                pair.soft[i] = std::sin(phase_soft) * env_soft;

                // Hard: beater click transient + high attack punch
                float f0_hard = (95.0f * kitTuningMult) * std::exp(-t * 35.0f) + 46.0f;
                float phase_hard = TWO_PI_F * f0_hard * t;
                float env_hard = std::exp(-t * 9.0f);
                float click = (i < 120) ? (std::sin(TWO_PI_F * 2800.0f * t) * std::exp(-t * 250.0f) * 0.8f) : 0.0f;
                pair.hard[i] = (std::sin(phase_hard) * env_hard) + click;
                break;
            }

            case JZDrumInstrument::SNARE: {
                // Soft: warm 185Hz body + soft wire noise
                float f0_soft = 185.0f * kitTuningMult;
                float body_soft = std::sin(TWO_PI_F * f0_soft * t) * std::exp(-t * 24.0f);
                float noise_soft = (xorshiftRandom() * 2.0f - 1.0f) * std::exp(-t * 22.0f) * 0.55f;
                pair.soft[i] = (body_soft * 0.6f + noise_soft * 0.4f);

                // Hard: 215Hz sharp attack, 4.5kHz rimshot crack, dense wire sizzle
                float f0_hard = 215.0f * kitTuningMult;
                float body_hard = std::sin(TWO_PI_F * f0_hard * t) * std::exp(-t * 16.0f);
                float rimshot = (i < 200) ? (std::sin(TWO_PI_F * 4200.0f * t) * std::exp(-t * 180.0f) * 0.9f) : 0.0f;
                float noise_hard = (xorshiftRandom() * 2.0f - 1.0f) * std::exp(-t * 14.0f) * 0.8f;
                pair.hard[i] = (body_hard * 0.45f + rimshot + noise_hard * 0.55f);
                break;
            }

            case JZDrumInstrument::SIDE_STICK: {
                float f_pop = 1680.0f * kitTuningMult;
                float env = std::exp(-t * 55.0f);
                float click = (i < 80) ? (xorshiftRandom() * 2.0f - 1.0f) * 0.7f : 0.0f;
                float val = (std::sin(TWO_PI_F * f_pop * t) * env) + click;
                pair.soft[i] = val * 0.7f;
                pair.hard[i] = val * 1.0f;
                break;
            }

            case JZDrumInstrument::HIHAT_CLOSED: {
                // High-frequency inharmonic metallic clusters
                float f1 = 2800.0f * kitTuningMult, f2 = 4700.0f, f3 = 7600.0f, f4 = 11200.0f;
                float metal = (std::sin(TWO_PI_F * f1 * t) + std::sin(TWO_PI_F * f2 * t) +
                               std::sin(TWO_PI_F * f3 * t) + std::sin(TWO_PI_F * f4 * t)) * 0.25f;
                float noise = (xorshiftRandom() * 2.0f - 1.0f) * 0.35f;
                float env_soft = std::exp(-t * 62.0f);
                float env_hard = std::exp(-t * 45.0f);
                pair.soft[i] = (metal + noise) * env_soft;
                pair.hard[i] = (metal * 0.8f + noise * 1.2f) * env_hard;
                break;
            }

            case JZDrumInstrument::HIHAT_PEDAL: {
                float f1 = 1800.0f * kitTuningMult, f2 = 3400.0f, f3 = 6500.0f;
                float metal = (std::sin(TWO_PI_F * f1 * t) + std::sin(TWO_PI_F * f2 * t) +
                               std::sin(TWO_PI_F * f3 * t)) * 0.33f;
                float noise = (xorshiftRandom() * 2.0f - 1.0f) * 0.25f;
                float env = std::exp(-t * 38.0f);
                pair.soft[i] = (metal + noise) * env * 0.7f;
                pair.hard[i] = (metal + noise) * env * 1.0f;
                break;
            }

            case JZDrumInstrument::HIHAT_OPEN: {
                float f1 = 2900.0f * kitTuningMult, f2 = 4800.0f, f3 = 7800.0f, f4 = 10500.0f;
                float metal = (std::sin(TWO_PI_F * f1 * t) + std::sin(TWO_PI_F * f2 * t) +
                               std::sin(TWO_PI_F * f3 * t) + std::sin(TWO_PI_F * f4 * t)) * 0.25f;
                float noise = (xorshiftRandom() * 2.0f - 1.0f) * 0.4f;
                float env_soft = std::exp(-t * 5.5f);
                float env_hard = std::exp(-t * 4.2f);
                pair.soft[i] = (metal + noise) * env_soft;
                pair.hard[i] = (metal + noise * 1.1f) * env_hard;
                break;
            }

            case JZDrumInstrument::TOM_LOW:
            case JZDrumInstrument::TOM_MID:
            case JZDrumInstrument::TOM_HIGH: {
                float basePitch = (inst == JZDrumInstrument::TOM_LOW) ? 88.0f :
                                  ((inst == JZDrumInstrument::TOM_MID) ? 135.0f : 185.0f);
                basePitch *= kitTuningMult;
                float sweepRate = (inst == JZDrumInstrument::TOM_LOW) ? 16.0f : 20.0f;
                float decayRate = (inst == JZDrumInstrument::TOM_LOW) ? 8.0f : 10.0f;

                float f_soft = basePitch * (0.8f + 0.2f * std::exp(-t * sweepRate));
                float f_hard = basePitch * (0.75f + 0.35f * std::exp(-t * (sweepRate * 1.5f)));

                float body_soft = std::sin(TWO_PI_F * f_soft * t) * std::exp(-t * decayRate);
                float body_hard = std::sin(TWO_PI_F * f_hard * t) * std::exp(-t * (decayRate * 0.85f));
                float stick = (i < 150) ? (std::sin(TWO_PI_F * 1400.0f * t) * std::exp(-t * 180.0f) * 0.5f) : 0.0f;

                pair.soft[i] = body_soft;
                pair.hard[i] = body_hard + stick;
                break;
            }

            case JZDrumInstrument::CRASH_1:
            case JZDrumInstrument::CRASH_2: {
                float fBase = (inst == JZDrumInstrument::CRASH_1) ? 580.0f : 520.0f;
                float metal = (std::sin(TWO_PI_F * fBase * t) +
                               std::sin(TWO_PI_F * fBase * 1.48f * t) +
                               std::sin(TWO_PI_F * fBase * 2.31f * t) +
                               std::sin(TWO_PI_F * fBase * 3.75f * t) +
                               std::sin(TWO_PI_F * fBase * 5.92f * t)) * 0.2f;
                float noise = (xorshiftRandom() * 2.0f - 1.0f);
                float env = std::exp(-t * 3.2f);
                pair.soft[i] = (metal * 0.4f + noise * 0.6f) * env * 0.7f;
                pair.hard[i] = (metal * 0.5f + noise * 0.75f) * env * 1.0f;
                break;
            }

            case JZDrumInstrument::RIDE: {
                float ping = std::sin(TWO_PI_F * 3400.0f * kitTuningMult * t) * std::exp(-t * 12.0f);
                float wash = (std::sin(TWO_PI_F * 720.0f * t) + std::sin(TWO_PI_F * 1150.0f * t)) * 0.5f * std::exp(-t * 2.8f);
                float noise = (xorshiftRandom() * 2.0f - 1.0f) * 0.15f * std::exp(-t * 3.5f);
                pair.soft[i] = (ping * 0.5f + wash * 0.4f + noise) * 0.75f;
                pair.hard[i] = (ping * 0.85f + wash * 0.55f + noise * 1.5f);
                break;
            }

            case JZDrumInstrument::RIDE_BELL: {
                float b1 = std::sin(TWO_PI_F * 630.0f * t);
                float b2 = std::sin(TWO_PI_F * 1280.0f * t);
                float b3 = std::sin(TWO_PI_F * 2450.0f * t);
                float bell = (b1 * 0.3f + b2 * 0.4f + b3 * 0.3f) * std::exp(-t * 4.5f);
                pair.soft[i] = bell * 0.7f;
                pair.hard[i] = bell * 1.0f;
                break;
            }

            case JZDrumInstrument::TAMBOURINE: {
                float jingle = (std::sin(TWO_PI_F * 4500.0f * t) + std::sin(TWO_PI_F * 8800.0f * t)) * 0.5f;
                float noise = (xorshiftRandom() * 2.0f - 1.0f) * 0.5f;
                float env = std::exp(-t * 18.0f);
                float val = (jingle + noise) * env;
                pair.soft[i] = val * 0.65f;
                pair.hard[i] = val * 1.0f;
                break;
            }

            case JZDrumInstrument::COWBELL: {
                float tone = (std::sin(TWO_PI_F * 565.0f * t) + std::sin(TWO_PI_F * 845.0f * t)) * 0.5f;
                float env = std::exp(-t * 16.0f);
                float val = tone * env;
                pair.soft[i] = val * 0.7f;
                pair.hard[i] = val * 1.0f;
                break;
            }

            default:
                pair.soft[i] = 0.0f;
                pair.hard[i] = 0.0f;
                break;
        }
    }
}

void JZRealDrumsEngine::chokeHiHat()
{
    for (size_t i = 0; i < MAX_VOICES; ++i) {
        if (m_voices[i].active && m_voices[i].instrument == JZDrumInstrument::HIHAT_OPEN) {
            m_voices[i].choking = true;
        }
    }
}

void JZRealDrumsEngine::noteOn(int midiNote, float velocity)
{
    if (m_params.bypassed) return;

    JZDrumInstrument inst = midiNoteToInstrument(midiNote);
    if (inst == JZDrumInstrument::COUNT) {
        return; // Unmapped drum note
    }

    // Hi-Hat Choke Group processing
    if (m_params.enableChokeGroup) {
        if (inst == JZDrumInstrument::HIHAT_CLOSED || inst == JZDrumInstrument::HIHAT_PEDAL) {
            chokeHiHat();
        }
    }

    // Velocity scaling & non-linear crossfade
    float scaledVel = std::clamp(velocity * m_params.velocitySensitivity, 0.02f, 1.0f);
    float angle = 0.5f * PI_F * scaledVel;
    float gainSoft = std::cos(angle);
    float gainHard = std::sin(angle);

    // Round-robin human micro-pitch detuning (+/- 12 cents)
    float pitchRatio = 1.0f;
    if (m_params.enableRoundRobin) {
        float semitones = (xorshiftRandom() - 0.5f) * 0.24f; // +/- 0.12 semitones
        pitchRatio = std::pow(2.0f, semitones / 12.0f);
    }

    // Find available voice slot (or steal quietest)
    int targetSlot = -1;
    float minGain = 1e9f;

    for (size_t i = 0; i < MAX_VOICES; ++i) {
        if (!m_voices[i].active) {
            targetSlot = static_cast<int>(i);
            break;
        }
        float voiceEnergy = m_voices[i].gainSoft + m_voices[i].gainHard;
        if (m_voices[i].choking) voiceEnergy *= 0.1f;
        if (voiceEnergy < minGain) {
            minGain = voiceEnergy;
            targetSlot = static_cast<int>(i);
        }
    }

    if (targetSlot < 0) targetSlot = 0;

    DrumVoice& v = m_voices[targetSlot];
    v.active = true;
    v.midiNote = midiNote;
    v.instrument = inst;
    v.sample = &m_samples[static_cast<size_t>(inst)];
    v.playPos = 0.0f;
    v.playInc = pitchRatio;
    v.gainSoft = gainSoft;
    v.gainHard = gainHard;
    v.choking = false;
    v.chokeGain = 1.0f;

    // Pan law
    float pan = v.sample->basePan;
    float panAngle = 0.5f * PI_F * (pan * 0.5f + 0.5f);
    v.panL = std::cos(panAngle);
    v.panR = std::sin(panAngle);

    // Sympathetic snare buzz on kick and toms
    if (m_params.snareBuzzAmount > 0.01f &&
        (inst == JZDrumInstrument::KICK || inst == JZDrumInstrument::TOM_LOW ||
         inst == JZDrumInstrument::TOM_MID || inst == JZDrumInstrument::TOM_HIGH)) {
        v.snareBuzzGain = scaledVel * m_params.snareBuzzAmount * 0.35f;
        v.snareBuzzPos = 0.0f;
    } else {
        v.snareBuzzGain = 0.0f;
        v.snareBuzzPos = 0.0f;
    }

    m_totalHits++;
    m_lastVelocity = scaledVel;
    m_lastNote = midiNote;
}

void JZRealDrumsEngine::noteOff(int midiNote)
{
    // Cymbals and drums ring naturally, but allow manual dampening if desired
    (void)midiNote;
}

void JZRealDrumsEngine::renderAudio(float* outL, float* outR, size_t numFrames)
{
    if (!outL || !outR || numFrames == 0) return;

    if (m_params.bypassed) {
        return;
    }

    float masterVol = m_params.masterLevel;
    float roomSend = m_params.roomAmbianceSend;
    size_t roomDelaySize = m_roomDelayBufferL.size();

    // Fast choke fade factor per sample (~6ms choke decay at 48kHz)
    float chokeDecay = std::exp(-1.0f / (m_sampleRate * 0.006f));

    for (size_t f = 0; f < numFrames; ++f) {
        float dryL = 0.0f;
        float dryR = 0.0f;

        for (size_t i = 0; i < MAX_VOICES; ++i) {
            DrumVoice& v = m_voices[i];
            if (!v.active || !v.sample) continue;

            const auto& softVec = v.sample->soft;
            const auto& hardVec = v.sample->hard;
            size_t sampleLen = softVec.size();

            size_t idx = static_cast<size_t>(v.playPos);
            if (idx + 1 >= sampleLen) {
                v.active = false;
                continue;
            }

            float frac = v.playPos - static_cast<float>(idx);
            float s0 = softVec[idx];
            float s1 = softVec[idx + 1];
            float softVal = s0 + frac * (s1 - s0);

            float h0 = hardVec[idx];
            float h1 = hardVec[idx + 1];
            float hardVal = h0 + frac * (h1 - h0);

            float sampleVal = (softVal * v.gainSoft + hardVal * v.gainHard);

            // Handle choking
            if (v.choking) {
                sampleVal *= v.chokeGain;
                v.chokeGain *= chokeDecay;
                if (v.chokeGain < 0.001f) {
                    v.active = false;
                    continue;
                }
            }

            // Snare buzz contribution
            if (v.snareBuzzGain > 0.001f && !m_snareWireNoise.empty()) {
                size_t buzzIdx = static_cast<size_t>(v.snareBuzzPos);
                if (buzzIdx < m_snareWireNoise.size()) {
                    sampleVal += m_snareWireNoise[buzzIdx] * v.snareBuzzGain;
                    v.snareBuzzPos += 1.0f;
                }
            }

            dryL += sampleVal * v.panL;
            dryR += sampleVal * v.panR;

            v.playPos += v.playInc;
        }

        // Room ambiance reflection
        float wetL = 0.0f;
        float wetR = 0.0f;
        if (roomDelaySize > 0) {
            float delayedL = m_roomDelayBufferL[m_roomDelayPos];
            float delayedR = m_roomDelayBufferR[m_roomDelayPos];

            m_roomDelayBufferL[m_roomDelayPos] = dryL + delayedR * 0.35f;
            m_roomDelayBufferR[m_roomDelayPos] = dryR + delayedL * 0.35f;

            m_roomDelayPos = (m_roomDelayPos + 1) % roomDelaySize;

            wetL = delayedL * roomSend;
            wetR = delayedR * roomSend;
        }

        outL[f] += (dryL + wetL) * masterVol;
        outR[f] += (dryR + wetR) * masterVol;
    }
}

bool JZRealDrumsEngine::loadCustomWavSample(int midiNote, const std::string& wavPath)
{
    (void)midiNote;
    (void)wavPath;
    // Returns false if file not found or invalid, falling back safely to studio procedural sample
    return false;
}

} // namespace Jazz
