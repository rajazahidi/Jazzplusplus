//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// PolyphonicNeuralAudioToMidiEngine.h - Polyphonic DNA Audio-to-MIDI Transcriber
//
// Polyphonic Direct Note Access (Melodyne DNA / Basic Pitch style):
// Multi-pitch chord extraction, per-note MPE pitch bend tracking, note velocity,
// onset/offset segmentation, and MusicXML / MIDI 2.0 chord score export.
//*****************************************************************************

#ifndef JAZZ_POLYPHONIC_NEURAL_AUDIO_TO_MIDI_ENGINE_H
#define JAZZ_POLYPHONIC_NEURAL_AUDIO_TO_MIDI_ENGINE_H

#include <vector>
#include <string>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace Jazz {

struct TranscribedNote {
    int noteNumber;           // MIDI Note (0 to 127)
    float pitchHz;            // Exact fundamental frequency
    float onsetSec;           // Start time in seconds
    float durationSec;        // Note length in seconds
    uint8_t velocity;         // 1 to 127
    float pitchBendCents;     // Microtonal pitch bend (-50.0 to +50.0 cents)
    int voiceIndex;           // Polyphonic voice assignment (0 to 7)
};

class JZPolyphonicNeuralAudioToMidiEngine {
public:
    struct Configuration {
        float minNoteDurationMs = 50.0f;     // Minimum note duration
        float energyThresholdDb = -45.0f;    // Detection gate threshold
        int maxPolyphony = 8;                // Maximum simultaneous voices
        float pitchToleranceCents = 35.0f;   // Pitch quantization window
        bool trackMpePitchBend = true;       // Real-time per-note microtonal bending
    };

    JZPolyphonicNeuralAudioToMidiEngine(float sampleRate = 48000.0f);
    ~JZPolyphonicNeuralAudioToMidiEngine() = default;

    void initialize(float sampleRate);
    void reset();

    void setConfiguration(const Configuration& config);
    const Configuration& getConfiguration() const { return m_config; }

    /**
     * @brief Ingest audio block into transcription analysis buffer.
     */
    void ingestAudio(const float* buffer, size_t numSamples);

    /**
     * @brief Execute polyphonic DNA pitch transcription on ingested audio.
     */
    size_t transcribe();

    /**
     * @brief Retrieve list of detected polyphonic notes.
     */
    const std::vector<TranscribedNote>& getTranscribedNotes() const { return m_notes; }

    /**
     * @brief Export transcribed chords into MusicXML score notation format.
     */
    std::string exportMusicXmlString() const;

    size_t getDetectedChordCount() const { return m_detectedChords; }

private:
    float m_sampleRate = 48000.0f;
    Configuration m_config;

    std::vector<float> m_audioBuffer;
    std::vector<TranscribedNote> m_notes;
    size_t m_detectedChords = 0;

    void extractPolyphonicPeaks(const float* window, size_t windowSize, float timestampSec);
    float hzToMidiNote(float hz, float& outPitchBendCents) const;
};

} // namespace Jazz

namespace jazz {
    using JZPolyphonicNeuralAudioToMidiEngine = Jazz::JZPolyphonicNeuralAudioToMidiEngine;
}

#endif // JAZZ_POLYPHONIC_NEURAL_AUDIO_TO_MIDI_ENGINE_H
