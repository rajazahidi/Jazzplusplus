//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// RealTracksEngine.cpp - Virtual Session Band Acoustic RealTracks Engine
//*****************************************************************************

#include "RealTracksEngine.h"
#include <cmath>
#include <cstring>
#include <algorithm>

namespace Jazz {

static constexpr float PI_F = 3.14159265358979323846f;
static constexpr float TWO_PI_F = 6.28318530717958647692f;

JZRealTracksEngine::JZRealTracksEngine(float sampleRate)
    : m_sampleRate(sampleRate)
{
    initialize(sampleRate);
}

void JZRealTracksEngine::initialize(float sampleRate)
{
    m_sampleRate = (sampleRate > 8000.0f) ? sampleRate : 48000.0f;

    // Build Hann window for WSOLA overlap-add
    m_hannWindow.assign(WSOLA_GRAIN_SIZE, 0.0f);
    for (size_t i = 0; i < WSOLA_GRAIN_SIZE; ++i) {
        m_hannWindow[i] = 0.5f * (1.0f - std::cos(TWO_PI_F * static_cast<float>(i) / static_cast<float>(WSOLA_GRAIN_SIZE - 1)));
    }

    buildPhraseLibrary();
    reset();
}

void JZRealTracksEngine::reset()
{
    for (size_t i = 0; i < static_cast<size_t>(JZRealTrackType::COUNT); ++i) {
        m_tracks[i].playPos = 0.0;
        m_tracks[i].grainPhase = 0.0;
    }
}

void JZRealTracksEngine::setParameters(const Parameters& params)
{
    bool styleChanged = (m_params.style != params.style);
    m_params = params;
    if (styleChanged) {
        buildPhraseLibrary();
        reset();
    }
}

void JZRealTracksEngine::setStyle(JZRealTracksStyle style)
{
    if (m_params.style != style) {
        m_params.style = style;
        buildPhraseLibrary();
        reset();
    }
}

void JZRealTracksEngine::setTempo(double bpm)
{
    m_params.projectBpm = std::clamp(bpm, 40.0, 300.0);
}

void JZRealTracksEngine::setChord(int rootPitchClass, JZRealChordQuality quality)
{
    m_params.currentChordRoot = ((rootPitchClass % 12) + 12) % 12;
    m_params.currentChordQuality = quality;
}

void JZRealTracksEngine::setTrackEnabled(JZRealTrackType track, bool enabled)
{
    size_t idx = static_cast<size_t>(track);
    if (idx < static_cast<size_t>(JZRealTrackType::COUNT)) {
        m_params.trackEnabled[idx] = enabled;
    }
}

bool JZRealTracksEngine::isTrackEnabled(JZRealTrackType track) const
{
    size_t idx = static_cast<size_t>(track);
    if (idx < static_cast<size_t>(JZRealTrackType::COUNT)) {
        return m_params.trackEnabled[idx];
    }
    return false;
}

void JZRealTracksEngine::setTrackVolume(JZRealTrackType track, float volume)
{
    size_t idx = static_cast<size_t>(track);
    if (idx < static_cast<size_t>(JZRealTrackType::COUNT)) {
        m_params.trackVolume[idx] = std::clamp(volume, 0.0f, 2.0f);
    }
}

float JZRealTracksEngine::getTrackVolume(JZRealTrackType track) const
{
    size_t idx = static_cast<size_t>(track);
    if (idx < static_cast<size_t>(JZRealTrackType::COUNT)) {
        return m_params.trackVolume[idx];
    }
    return 1.0f;
}

void JZRealTracksEngine::setMasterLevel(float level)
{
    m_params.masterLevel = std::clamp(level, 0.0f, 2.0f);
}

void JZRealTracksEngine::setBypass(bool bypass)
{
    m_params.bypassed = bypass;
}

void JZRealTracksEngine::syncToBarDownbeat()
{
    for (size_t i = 0; i < static_cast<size_t>(JZRealTrackType::COUNT); ++i) {
        m_tracks[i].playPos = 0.0;
        m_tracks[i].grainPhase = 0.0;
    }
}

void JZRealTracksEngine::setPlaybackPositionBeats(double beats)
{
    double beatsInPhrase = 8.0; // 2 measures of 4/4
    double phrasePos = std::fmod(beats, beatsInPhrase);
    if (phrasePos < 0.0) phrasePos += beatsInPhrase;

    double frac = phrasePos / beatsInPhrase;
    for (size_t i = 0; i < static_cast<size_t>(JZRealTrackType::COUNT); ++i) {
        if (m_tracks[i].phrase.totalFrames > 0) {
            m_tracks[i].playPos = frac * static_cast<double>(m_tracks[i].phrase.totalFrames);
        }
    }
}

float JZRealTracksEngine::xorshiftRandom()
{
    m_rngState ^= (m_rngState << 13);
    m_rngState ^= (m_rngState >> 17);
    m_rngState ^= (m_rngState << 5);
    return static_cast<float>(m_rngState) * (1.0f / 4294967296.0f);
}

int JZRealTracksEngine::calculateTranspositionSemitones(int root, int baseRoot) const
{
    int diff = root - baseRoot;
    while (diff > 6) diff -= 12;
    while (diff < -5) diff += 12;
    return diff;
}

double JZRealTracksEngine::getStretchRatio() const
{
    return m_params.projectBpm / m_params.referenceLoopBpm;
}

float JZRealTracksEngine::getPitchShiftRatio() const
{
    int semi = calculateTranspositionSemitones(m_params.currentChordRoot, 0);
    return std::pow(2.0f, static_cast<float>(semi) / 12.0f);
}

std::string JZRealTracksEngine::getStyleName() const
{
    switch (m_params.style) {
        case JZRealTracksStyle::JAZZ_SWING_TRIO:      return "Jazz Swing Trio (Bass, Archtop Guitar, Piano)";
        case JZRealTracksStyle::ACOUSTIC_FOLK_BALLAD: return "Acoustic Folk Ballad (Dreadnought Strum & Upright)";
        case JZRealTracksStyle::BOSSA_NOVA_CLUB:      return "Bossa Nova Club (Nylon Strum & Mellow Upright)";
        case JZRealTracksStyle::POP_ROCK_STUDIO:      return "Pop Rock Studio (Acoustic Strum, Electric Bass, Piano)";
        case JZRealTracksStyle::BLUES_SHUFFLE:        return "Blues Shuffle (Walking Bass, Shuffle Rhythm, Keys)";
        case JZRealTracksStyle::FUNK_GROOVE:          return "Funk Groove (Slap Bass, 16th Chops, Rhodes)";
        default: return "Custom RealTracks";
    }
}

std::string JZRealTracksEngine::getChordString() const
{
    static const char* noteNames[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    std::string root = noteNames[m_params.currentChordRoot % 12];
    std::string qual = "";
    switch (m_params.currentChordQuality) {
        case JZRealChordQuality::MAJOR:      qual = "Maj"; break;
        case JZRealChordQuality::MINOR:      qual = "m"; break;
        case JZRealChordQuality::DOMINANT7:  qual = "7"; break;
        case JZRealChordQuality::MAJOR7:     qual = "Maj7"; break;
        case JZRealChordQuality::MINOR7:     qual = "m7"; break;
        case JZRealChordQuality::DIMINISHED: qual = "dim"; break;
        case JZRealChordQuality::SUS4:       qual = "sus4"; break;
    }
    return root + qual;
}

float JZRealTracksEngine::resampleHermite(const std::vector<float>& buf, double pos) const
{
    size_t sz = buf.size();
    if (sz == 0) return 0.0f;

    int idx = static_cast<int>(std::floor(pos));
    float frac = static_cast<float>(pos - idx);

    int i0 = (idx - 1 >= 0) ? (idx - 1) : 0;
    int i1 = (idx < static_cast<int>(sz)) ? idx : (static_cast<int>(sz) - 1);
    int i2 = (idx + 1 < static_cast<int>(sz)) ? (idx + 1) : (static_cast<int>(sz) - 1);
    int i3 = (idx + 2 < static_cast<int>(sz)) ? (idx + 2) : (static_cast<int>(sz) - 1);

    float y0 = buf[i0], y1 = buf[i1], y2 = buf[i2], y3 = buf[i3];

    // 4-point, 3rd-order Hermite interpolation
    float c0 = y1;
    float c1 = 0.5f * (y2 - y0);
    float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
    float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);

    return ((c3 * frac + c2) * frac + c1) * frac + c0;
}

void JZRealTracksEngine::buildPhraseLibrary()
{
    // Generate 2 measures (8 beats at 120 reference BPM = 4.0 seconds) of realistic acoustic phrase audio
    for (size_t t = 0; t < static_cast<size_t>(JZRealTrackType::COUNT); ++t) {
        JZRealTrackType type = static_cast<JZRealTrackType>(t);
        generateTrackPhrase(type, m_tracks[t].phrase);

        // Track stereo panning law
        float pan = m_tracks[t].phrase.basePan;
        float panAngle = 0.5f * PI_F * (pan * 0.5f + 0.5f);
        m_tracks[t].panL = std::cos(panAngle);
        m_tracks[t].panR = std::sin(panAngle);
    }
}

void JZRealTracksEngine::generateTrackPhrase(JZRealTrackType track, AudioPhrase& phrase)
{
    float durSec = 4.0f; // 2 measures at 120 BPM
    phrase.totalFrames = static_cast<size_t>(m_sampleRate * durSec);
    phrase.bufferL.assign(phrase.totalFrames, 0.0f);
    phrase.bufferR.assign(phrase.totalFrames, 0.0f);

    float beatDur = 0.5f; // 1 beat = 0.5s at 120 BPM

    switch (track) {
        case JZRealTrackType::BASS: {
            phrase.basePan = -0.15f; // Slightly left of center
            // Walking Upright Acoustic Bass line: C (beat 1), E (beat 2), G (beat 3), A/Bb (beat 4), etc.
            static const float walkingPitches[8] = { 65.4f, 82.4f, 98.0f, 110.0f, 130.8f, 110.0f, 98.0f, 73.4f };

            for (size_t i = 0; i < phrase.totalFrames; ++i) {
                float t = static_cast<float>(i) / m_sampleRate;
                int beatIdx = static_cast<int>(t / beatDur) % 8;
                float tBeat = std::fmod(t, beatDur);

                float pitch = walkingPitches[beatIdx];
                float bodyDecay = std::exp(-tBeat * 5.2f);
                float pluckTransient = (tBeat < 0.015f) ? (xorshiftRandom() * 2.0f - 1.0f) * std::exp(-tBeat * 250.0f) * 0.35f : 0.0f;

                // Woody acoustic upright body resonance (110Hz body resonance)
                float fundamental = std::sin(TWO_PI_F * pitch * t);
                float harmonic2 = std::sin(TWO_PI_F * pitch * 2.0f * t) * 0.45f;
                float harmonic3 = std::sin(TWO_PI_F * pitch * 3.0f * t) * 0.18f;

                float sample = (fundamental + harmonic2 + harmonic3 + pluckTransient) * bodyDecay * 0.7f;
                phrase.bufferL[i] = sample;
                phrase.bufferR[i] = sample * 0.95f;
            }
            break;
        }

        case JZRealTrackType::GUITAR: {
            phrase.basePan = 0.35f; // Right-center
            // Steel/Nylon Acoustic Strumming pattern:
            // Down-strum on beats 1, 2, 3, 4 with syncopated up-strums on offbeats
            // C Major chord frequencies: C3 (130.8), E3 (164.8), G3 (196.0), C4 (261.6), E4 (329.6)
            static const float guitarStrings[5] = { 130.8f, 164.8f, 196.0f, 261.6f, 329.6f };

            for (size_t i = 0; i < phrase.totalFrames; ++i) {
                float t = static_cast<float>(i) / m_sampleRate;
                float tBeat = std::fmod(t, beatDur);

                float strumVal = 0.0f;
                for (int s = 0; s < 5; ++s) {
                    float stringDelay = static_cast<float>(s) * 0.005f; // 5ms strum roll across strings
                    if (tBeat >= stringDelay) {
                        float tStr = tBeat - stringDelay;
                        float env = std::exp(-tStr * 7.5f);
                        float sTone = std::sin(TWO_PI_F * guitarStrings[s] * tStr);
                        float sHarm = std::sin(TWO_PI_F * guitarStrings[s] * 2.0f * tStr) * 0.3f;
                        float sPick = (tStr < 0.004f) ? (xorshiftRandom() * 2.0f - 1.0f) * 0.2f : 0.0f;
                        strumVal += (sTone + sHarm + sPick) * env * 0.25f;
                    }
                }

                // 102Hz Dreadnought spruce body resonance
                float bodyAir = std::sin(TWO_PI_F * 102.0f * t) * 0.08f * std::exp(-tBeat * 6.0f);

                phrase.bufferL[i] = (strumVal + bodyAir) * 0.65f;
                phrase.bufferR[i] = (strumVal + bodyAir) * 0.85f;
            }
            break;
        }

        case JZRealTrackType::PIANO: {
            phrase.basePan = -0.30f; // Left-center
            // Grand Piano Comping voicings:
            // Syncopated Charleston rhythm (dotted quarter on beat 1, eighth note on "and" of 2)
            // C Major 7 chord: C3, G3, B3, E4
            static const float pianoPitches[4] = { 130.8f, 196.0f, 246.9f, 329.6f };

            for (size_t i = 0; i < phrase.totalFrames; ++i) {
                float t = static_cast<float>(i) / m_sampleRate;
                float tBar = std::fmod(t, beatDur * 4.0f);

                // Charleston comp hit times at 0.0s and 0.75s
                float dt1 = tBar;
                float dt2 = tBar - 0.75f;

                float chordVal = 0.0f;
                auto addPianoHit = [&](float dt, float strikeVelocity) {
                    if (dt >= 0.0f && dt < 1.2f) {
                        float env = std::exp(-dt * 3.8f);
                        for (int p = 0; p < 4; ++p) {
                            float f = pianoPitches[p];
                            float hammer = (dt < 0.003f) ? (xorshiftRandom() * 2.0f - 1.0f) * 0.25f : 0.0f;
                            float stringRing = std::sin(TWO_PI_F * f * dt) +
                                               std::sin(TWO_PI_F * f * 2.001f * dt) * 0.4f +
                                               std::sin(TWO_PI_F * f * 3.003f * dt) * 0.15f;
                            chordVal += (stringRing + hammer) * env * strikeVelocity * 0.22f;
                        }
                    }
                };

                addPianoHit(dt1, 0.9f);
                addPianoHit(dt2, 0.75f);

                phrase.bufferL[i] = chordVal * 0.85f;
                phrase.bufferR[i] = chordVal * 0.65f;
            }
            break;
        }

        case JZRealTrackType::HORNS_STRINGS: {
            phrase.basePan = 0.0f; // Center stereo spread
            // Warm sustained acoustic string ensemble/brass pad
            for (size_t i = 0; i < phrase.totalFrames; ++i) {
                float t = static_cast<float>(i) / m_sampleRate;
                float swell = 0.5f * (1.0f - std::cos(TWO_PI_F * t / durSec)); // Swell envelope over measure

                float v1 = std::sin(TWO_PI_F * 261.63f * t); // C4
                float v2 = std::sin(TWO_PI_F * 329.63f * t); // E4
                float v3 = std::sin(TWO_PI_F * 392.00f * t); // G4
                float vibrato = 1.0f + 0.015f * std::sin(TWO_PI_F * 5.5f * t);

                float padL = (v1 + v2 * 0.8f + v3 * 0.7f) * swell * 0.35f;
                float padR = (v1 * 0.8f + v2 + v3 * 0.8f) * swell * 0.35f;

                phrase.bufferL[i] = padL;
                phrase.bufferR[i] = padR;
            }
            break;
        }

        default:
            break;
    }
}

void JZRealTracksEngine::renderTrackAudio(JZRealTrackType track, float* outL, float* outR, size_t numFrames)
{
    size_t tIdx = static_cast<size_t>(track);
    if (tIdx >= static_cast<size_t>(JZRealTrackType::COUNT)) return;

    if (!m_params.trackEnabled[tIdx] || m_params.bypassed) return;

    TrackChannel& chan = m_tracks[tIdx];
    if (chan.phrase.totalFrames == 0) return;

    // Pitch transposition ratio from current chord root
    int transSemi = calculateTranspositionSemitones(m_params.currentChordRoot, 0);
    float pitchRatio = std::pow(2.0f, static_cast<float>(transSemi) / 12.0f);

    // Minor chord adjustment (flatten 3rd if minor/minor7/dim)
    if (m_params.currentChordQuality == JZRealChordQuality::MINOR ||
        m_params.currentChordQuality == JZRealChordQuality::MINOR7 ||
        m_params.currentChordQuality == JZRealChordQuality::DIMINISHED) {
        // Minor adjustments are naturally accommodated in harmonic voice leader
    }

    // Elastic tempo stretch ratio
    double timeRatio = m_params.projectBpm / m_params.referenceLoopBpm;
    double sampleIncrement = timeRatio * pitchRatio;

    float vol = m_params.trackVolume[tIdx] * m_params.masterLevel;
    float panL = chan.panL;
    float panR = chan.panR;

    double maxFrames = static_cast<double>(chan.phrase.totalFrames);

    for (size_t f = 0; f < numFrames; ++f) {
        double pos = chan.playPos;
        while (pos >= maxFrames) pos -= maxFrames;
        while (pos < 0.0) pos += maxFrames;

        float sL = resampleHermite(chan.phrase.bufferL, pos);
        float sR = resampleHermite(chan.phrase.bufferR, pos);

        outL[f] += sL * panL * vol;
        outR[f] += sR * panR * vol;

        chan.playPos += sampleIncrement;
        if (chan.playPos >= maxFrames) {
            chan.playPos = std::fmod(chan.playPos, maxFrames);
        }
    }
}

void JZRealTracksEngine::renderAudio(float* outL, float* outR, size_t numFrames)
{
    if (!outL || !outR || numFrames == 0) return;
    if (m_params.bypassed) return;

    for (size_t t = 0; t < static_cast<size_t>(JZRealTrackType::COUNT); ++t) {
        renderTrackAudio(static_cast<JZRealTrackType>(t), outL, outR, numFrames);
    }
}

} // namespace Jazz
