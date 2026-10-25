//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// RealInstrumentManager.cpp - Studio RealAudio & SoundFont Coordinator
//*****************************************************************************

#include "RealInstrumentManager.h"
#include <cmath>
#include <cstring>
#include <algorithm>

namespace Jazz {

JZRealInstrumentManager& JZRealInstrumentManager::getInstance()
{
    static JZRealInstrumentManager instance(48000.0f);
    return instance;
}

JZRealInstrumentManager::JZRealInstrumentManager(float sampleRate)
    : m_sampleRate(sampleRate),
      m_drumsEngine(sampleRate),
      m_tracksEngine(sampleRate),
      m_guitarConvolver(sampleRate),
      m_pianoResonance(sampleRate),
      m_roomConvolver(sampleRate)
{
    initialize(sampleRate);
}

void JZRealInstrumentManager::initialize(float sampleRate)
{
    m_sampleRate = (sampleRate > 8000.0f) ? sampleRate : 48000.0f;

    m_drumsEngine.initialize(m_sampleRate);
    m_tracksEngine.initialize(m_sampleRate);
    m_guitarConvolver.initialize(m_sampleRate);
    m_pianoResonance.setSampleRate(m_sampleRate);

    // Configure guitar body convolver default for acoustic warmth
    auto gParams = m_guitarConvolver.getParameters();
    gParams.profile = GuitarMicProfile::KM184_SMALL_CONDENSER;
    gParams.bodyResonanceAmount = 0.70f;
    gParams.dryWet = 0.80f;
    m_guitarConvolver.setParameters(gParams);

    // Configure piano sympathetic resonance
    m_pianoResonance.setSympatheticDepth(0.65f);
    m_pianoResonance.setSoundboardSize(0.85f);

    reset();
}

void JZRealInstrumentManager::reset()
{
    m_drumsEngine.reset();
    m_tracksEngine.reset();
    m_guitarConvolver.reset();
    m_pianoResonance.reset();
    std::fill(std::begin(m_activeChordNotes), std::end(m_activeChordNotes), 0);
}

void JZRealInstrumentManager::setRealAudioBlend(float blend)
{
    m_realAudioBlend = std::clamp(blend, 0.0f, 1.0f);
}

void JZRealInstrumentManager::setSoundFontPath(const std::string& path)
{
    m_soundFontPath = path;
    m_soundFontLoaded = !path.empty();
}

void JZRealInstrumentManager::setSamplePackPath(const std::string& path)
{
    m_samplePackPath = path;
}

bool JZRealInstrumentManager::handleMidiNoteOn(int channel, int note, int velocity)
{
    if (velocity == 0) {
        return handleMidiNoteOff(channel, note);
    }

    float velNorm = static_cast<float>(velocity) / 127.0f;

    // Channel 10: Standard General MIDI Percussion
    if (channel == 10) {
        if (m_mode != JZRealAudioMode::STANDARD_MIDI) {
            m_drumsEngine.noteOn(note, velNorm);
            return true; // Drum hit handled by acoustic real drums
        }
        return false;
    }

    // Melodic Channels: Analyze chord for RealTracks and trigger piano resonance
    int pc = ((note % 12) + 12) % 12;
    m_activeChordNotes[pc]++;
    updateDetectedChord();

    if (m_enablePianoResonance) {
        m_pianoResonance.noteOn(note, velNorm);
    }

    return false;
}

bool JZRealInstrumentManager::handleMidiNoteOff(int channel, int note)
{
    if (channel == 10) {
        if (m_mode != JZRealAudioMode::STANDARD_MIDI) {
            m_drumsEngine.noteOff(note);
            return true;
        }
        return false;
    }

    int pc = ((note % 12) + 12) % 12;
    if (m_activeChordNotes[pc] > 0) {
        m_activeChordNotes[pc]--;
    }
    updateDetectedChord();

    if (m_enablePianoResonance) {
        m_pianoResonance.noteOff(note);
    }

    return false;
}

bool JZRealInstrumentManager::handleMidiControlChange(int channel, int controller, int value)
{
    (void)channel;
    // CC 64: Damper / Sustain Pedal
    if (controller == 64) {
        bool pedalDown = (value >= 64);
        m_pianoResonance.setDamperPedal(pedalDown);
        return true;
    }
    return false;
}

void JZRealInstrumentManager::updateDetectedChord()
{
    // Find most prominent root pitch class
    int bestRoot = 0;
    int maxCount = 0;
    for (int i = 0; i < 12; ++i) {
        if (m_activeChordNotes[i] > maxCount) {
            maxCount = m_activeChordNotes[i];
            bestRoot = i;
        }
    }

    if (maxCount == 0) return;

    // Check for minor third (+3 semitones) vs major third (+4 semitones)
    bool hasMinor3 = (m_activeChordNotes[(bestRoot + 3) % 12] > 0);
    bool hasMajor3 = (m_activeChordNotes[(bestRoot + 4) % 12] > 0);
    bool hasFlat7  = (m_activeChordNotes[(bestRoot + 10) % 12] > 0);
    bool hasMajor7 = (m_activeChordNotes[(bestRoot + 11) % 12] > 0);

    JZRealChordQuality quality = JZRealChordQuality::MAJOR;
    if (hasMinor3 && hasFlat7) {
        quality = JZRealChordQuality::MINOR7;
    } else if (hasMinor3) {
        quality = JZRealChordQuality::MINOR;
    } else if (hasMajor3 && hasMajor7) {
        quality = JZRealChordQuality::MAJOR7;
    } else if (hasMajor3 && hasFlat7) {
        quality = JZRealChordQuality::DOMINANT7;
    }

    m_tracksEngine.setChord(bestRoot, quality);
}

void JZRealInstrumentManager::processMasterAudio(float* outL, float* outR, size_t numFrames)
{
    if (!outL || !outR || numFrames == 0) return;
    if (m_mode == JZRealAudioMode::STANDARD_MIDI) return;

    // Resize scratch buffers if necessary
    if (m_scratchGuitarL.size() < numFrames) {
        m_scratchGuitarL.assign(numFrames, 0.0f);
        m_scratchGuitarR.assign(numFrames, 0.0f);
        m_scratchPianoL.assign(numFrames, 0.0f);
        m_scratchPianoR.assign(numFrames, 0.0f);
        m_scratchRoomL.assign(numFrames, 0.0f);
        m_scratchRoomR.assign(numFrames, 0.0f);
    } else {
        std::fill(m_scratchGuitarL.begin(), m_scratchGuitarL.begin() + numFrames, 0.0f);
        std::fill(m_scratchGuitarR.begin(), m_scratchGuitarR.begin() + numFrames, 0.0f);
        std::fill(m_scratchPianoL.begin(), m_scratchPianoL.begin() + numFrames, 0.0f);
        std::fill(m_scratchPianoR.begin(), m_scratchPianoR.begin() + numFrames, 0.0f);
        std::fill(m_scratchRoomL.begin(), m_scratchRoomL.begin() + numFrames, 0.0f);
        std::fill(m_scratchRoomR.begin(), m_scratchRoomR.begin() + numFrames, 0.0f);
    }

    float blend = m_realAudioBlend;

    if (m_mode == JZRealAudioMode::REALDRUMS_ENHANCED) {
        // Only render RealDrums
        m_drumsEngine.renderAudio(outL, outR, numFrames);
        return;
    }

    if (m_mode == JZRealAudioMode::FULL_REALTRACKS_BAND) {
        // 1. Render RealDrums
        m_drumsEngine.renderAudio(m_scratchRoomL.data(), m_scratchRoomR.data(), numFrames);

        // 2. Render Bass and Horns/Strings directly into room mix
        m_tracksEngine.renderTrackAudio(JZRealTrackType::BASS, m_scratchRoomL.data(), m_scratchRoomR.data(), numFrames);
        m_tracksEngine.renderTrackAudio(JZRealTrackType::HORNS_STRINGS, m_scratchRoomL.data(), m_scratchRoomR.data(), numFrames);

        // 3. Render Guitar through Acoustic Guitar Body Convolver
        m_tracksEngine.renderTrackAudio(JZRealTrackType::GUITAR, m_scratchGuitarL.data(), m_scratchGuitarR.data(), numFrames);
        if (m_enableGuitarBody) {
            m_guitarConvolver.process(m_scratchGuitarL.data(), m_scratchGuitarR.data(),
                                      m_scratchGuitarL.data(), m_scratchGuitarR.data(),
                                      numFrames);
        }
        for (size_t i = 0; i < numFrames; ++i) {
            m_scratchRoomL[i] += m_scratchGuitarL[i];
            m_scratchRoomR[i] += m_scratchGuitarR[i];
        }

        // 4. Render Piano through Grand Piano Sympathetic Resonance Engine
        m_tracksEngine.renderTrackAudio(JZRealTrackType::PIANO, m_scratchPianoL.data(), m_scratchPianoR.data(), numFrames);
        if (m_enablePianoResonance) {
            m_pianoResonance.process(m_scratchPianoL.data(), m_scratchPianoR.data(),
                                     m_scratchPianoL.data(), m_scratchPianoR.data(),
                                     numFrames);
        }
        for (size_t i = 0; i < numFrames; ++i) {
            m_scratchRoomL[i] += m_scratchPianoL[i];
            m_scratchRoomR[i] += m_scratchPianoR[i];
        }

        // 5. Apply Binaural 3D Room Convolver if enabled
        if (m_enableBinauralRoom) {
            m_roomConvolver.processStereo(m_scratchRoomL.data(), m_scratchRoomR.data(),
                                          m_scratchRoomL.data(), m_scratchRoomR.data(),
                                          static_cast<int>(numFrames));
        }

        // 6. Sum into master output with blend
        for (size_t i = 0; i < numFrames; ++i) {
            outL[i] += m_scratchRoomL[i] * blend;
            outR[i] += m_scratchRoomR[i] * blend;
        }
    }
}

} // namespace Jazz
