//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// PolyphonicNeuralAudioToMidiEngine.cpp - Polyphonic DNA Audio-to-MIDI Transcriber
//*****************************************************************************

#include "PolyphonicNeuralAudioToMidiEngine.h"
#include <cmath>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace Jazz {

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

JZPolyphonicNeuralAudioToMidiEngine::JZPolyphonicNeuralAudioToMidiEngine(float sampleRate) {
    initialize(sampleRate);
}

void JZPolyphonicNeuralAudioToMidiEngine::initialize(float sampleRate) {
    if (sampleRate < 8000.0f) sampleRate = 48000.0f;
    m_sampleRate = sampleRate;
    reset();
}

void JZPolyphonicNeuralAudioToMidiEngine::reset() {
    m_audioBuffer.clear();
    m_notes.clear();
    m_detectedChords = 0;
}

void JZPolyphonicNeuralAudioToMidiEngine::setConfiguration(const Configuration& config) {
    m_config = config;
}

void JZPolyphonicNeuralAudioToMidiEngine::ingestAudio(const float* buffer, size_t numSamples) {
    if (buffer && numSamples > 0) {
        m_audioBuffer.insert(m_audioBuffer.end(), buffer, buffer + numSamples);
    }
}

float JZPolyphonicNeuralAudioToMidiEngine::hzToMidiNote(float hz, float& outPitchBendCents) const {
    if (hz < 10.0f) {
        outPitchBendCents = 0.0f;
        return 0.0f;
    }
    float midiFloat = 69.0f + 12.0f * std::log2(hz / 440.0f);
    int noteInt = static_cast<int>(std::round(midiFloat));
    outPitchBendCents = (midiFloat - static_cast<float>(noteInt)) * 100.0f;
    return static_cast<float>(noteInt);
}

void JZPolyphonicNeuralAudioToMidiEngine::extractPolyphonicPeaks(const float* window, size_t windowSize, float timestampSec) {
    if (!window || windowSize < 128) return;

    // Harmonic Sieve / DFT peak detection across musical notes C2 (65.4 Hz) to B5 (987.7 Hz)
    float minFreq = 65.4f;
    float maxFreq = 1000.0f;

    struct PeakCandidate {
        float freq;
        float power;
    };
    std::vector<PeakCandidate> candidates;

    // Search 48 semitones
    for (int semi = 36; semi <= 84; ++semi) { // C2 to C6
        float f0 = 440.0f * std::pow(2.0f, (semi - 69) / 12.0f);
        if (f0 < minFreq || f0 > maxFreq) continue;

        // Discrete correlation at f0
        float sumCos = 0.0f;
        float sumSin = 0.0f;
        float w0 = 2.0f * static_cast<float>(M_PI) * (f0 / m_sampleRate);

        for (size_t i = 0; i < windowSize; ++i) {
            float angle = w0 * static_cast<float>(i);
            float s = window[i];
            sumCos += s * std::cos(angle);
            sumSin += s * std::sin(angle);
        }

        float power = (sumCos * sumCos + sumSin * sumSin) / static_cast<float>(windowSize);
        float powerDb = (power > 1e-9f) ? (10.0f * std::log10(power)) : -90.0f;

        if (powerDb >= m_config.energyThresholdDb) {
            candidates.push_back({ f0, powerDb });
        }
    }

    // Sort by power descending and select top polyphonic voices
    std::sort(candidates.begin(), candidates.end(), [](const PeakCandidate& a, const PeakCandidate& b) {
        return a.power > b.power;
    });

    size_t voiceCount = 0;
    for (const auto& cand : candidates) {
        if (voiceCount >= static_cast<size_t>(m_config.maxPolyphony)) break;

        float bendCents = 0.0f;
        int noteNum = static_cast<int>(hzToMidiNote(cand.freq, bendCents));

        // Avoid duplicate notes in same chord
        bool duplicate = false;
        for (const auto& n : m_notes) {
            if (n.noteNumber == noteNum && std::abs(n.onsetSec - timestampSec) < 0.08f) {
                duplicate = true;
                break;
            }
        }

        if (!duplicate && noteNum >= 21 && noteNum <= 108) {
            float velNorm = (cand.power - m_config.energyThresholdDb) / (-m_config.energyThresholdDb);
            velNorm = std::max(0.1f, std::min(1.0f, velNorm));
            uint8_t vel = static_cast<uint8_t>(velNorm * 127.0f);

            TranscribedNote note;
            note.noteNumber = noteNum;
            note.pitchHz = cand.freq;
            note.onsetSec = timestampSec;
            note.durationSec = m_config.minNoteDurationMs / 1000.0f;
            note.velocity = vel;
            note.pitchBendCents = m_config.trackMpePitchBend ? bendCents : 0.0f;
            note.voiceIndex = static_cast<int>(voiceCount);

            m_notes.push_back(note);
            voiceCount++;
        }
    }

    if (voiceCount > 1) {
        m_detectedChords++;
    }
}

size_t JZPolyphonicNeuralAudioToMidiEngine::transcribe() {
    m_notes.clear();
    m_detectedChords = 0;

    if (m_audioBuffer.size() < 512) return 0;

    size_t hopSize = static_cast<size_t>(0.04f * m_sampleRate); // 40 ms hop
    size_t windowSize = static_cast<size_t>(0.08f * m_sampleRate); // 80 ms window
    if (hopSize < 128) hopSize = 128;
    if (windowSize < 256) windowSize = 256;

    for (size_t pos = 0; pos + windowSize <= m_audioBuffer.size(); pos += hopSize) {
        float timestampSec = static_cast<float>(pos) / m_sampleRate;
        extractPolyphonicPeaks(&m_audioBuffer[pos], windowSize, timestampSec);
    }

    return m_notes.size();
}

std::string JZPolyphonicNeuralAudioToMidiEngine::exportMusicXmlString() const {
    std::ostringstream ss;
    ss << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    ss << "<score-partwise version=\"3.1\">\n";
    ss << "  <part-list>\n";
    ss << "    <score-part id=\"P1\"><part-name>Polyphonic DNA Transcription</part-name></score-part>\n";
    ss << "  </part-list>\n";
    ss << "  <part id=\"P1\">\n";
    ss << "    <measure number=\"1\">\n";

    static const char* stepNames[12] = { "C", "C", "D", "D", "E", "F", "F", "G", "G", "A", "A", "B" };
    static const int alterValues[12] = { 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0 };

    for (const auto& note : m_notes) {
        int noteIndex = note.noteNumber % 12;
        int octave = (note.noteNumber / 12) - 1;

        ss << "      <note>\n";
        ss << "        <pitch>\n";
        ss << "          <step>" << stepNames[noteIndex] << "</step>\n";
        if (alterValues[noteIndex] != 0) {
            ss << "          <alter>" << alterValues[noteIndex] << "</alter>\n";
        }
        ss << "          <octave>" << octave << "</octave>\n";
        ss << "        </pitch>\n";
        ss << "        <duration>" << static_cast<int>(note.durationSec * 1000) << "</duration>\n";
        ss << "        <voice>" << (note.voiceIndex + 1) << "</voice>\n";
        ss << "        <type>quarter</type>\n";
        ss << "      </note>\n";
    }

    ss << "    </measure>\n";
    ss << "  </part>\n";
    ss << "</score-partwise>\n";
    return ss.str();
}

} // namespace Jazz
