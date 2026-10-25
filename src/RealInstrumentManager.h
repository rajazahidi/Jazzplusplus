//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// RealInstrumentManager.h - Studio RealAudio & SoundFont Coordinator
//
// Central manager uniting RealDrums, RealTracks, SoundFont acoustic samples,
// and physical acoustic modeling (Helmholtz guitar body, piano bridge coupling,
// binaural 3D room ambiance).
//*****************************************************************************

#ifndef JAZZ_REAL_INSTRUMENT_MANAGER_H
#define JAZZ_REAL_INSTRUMENT_MANAGER_H

#include "RealDrumsEngine.h"
#include "RealTracksEngine.h"
#include "AcousticGuitarBodyImpulseConvolverEngine.h"
#include "GrandPianoSympatheticResonanceEngine.h"
#include "BinauralAcousticRoomConvolverEngine.h"

#include <string>
#include <vector>
#include <memory>

namespace Jazz {

enum class JZRealAudioMode {
    STANDARD_MIDI = 0,         // Pure MIDI synthesizer output
    REALDRUMS_ENHANCED = 1,    // Real acoustic studio drums replacing MIDI drum track
    FULL_REALTRACKS_BAND = 2   // RealDrums + RealTracks (Bass, Guitar, Piano) + Physical Modeling
};

class JZRealInstrumentManager {
public:
    static JZRealInstrumentManager& getInstance();

    explicit JZRealInstrumentManager(float sampleRate = 48000.0f);
    ~JZRealInstrumentManager() = default;

    void initialize(float sampleRate);
    void reset();

    // Mode & Routing Controls
    void setMode(JZRealAudioMode mode) { m_mode = mode; }
    JZRealAudioMode getMode() const { return m_mode; }

    void setRealAudioBlend(float blend); // 0.0 = pure MIDI, 1.0 = pure RealAudio
    float getRealAudioBlend() const { return m_realAudioBlend; }

    // Physical Acoustic Enhancement Switches
    void setGuitarBodyResonanceEnabled(bool enable) { m_enableGuitarBody = enable; }
    bool isGuitarBodyResonanceEnabled() const { return m_enableGuitarBody; }

    void setPianoSympatheticResonanceEnabled(bool enable) { m_enablePianoResonance = enable; }
    bool isPianoSympatheticResonanceEnabled() const { return m_enablePianoResonance; }

    void setBinauralRoomEnabled(bool enable) { m_enableBinauralRoom = enable; }
    bool isBinauralRoomEnabled() const { return m_enableBinauralRoom; }

    // SoundFont & Sample Libraries
    void setSoundFontPath(const std::string& path);
    const std::string& getSoundFontPath() const { return m_soundFontPath; }

    void setSamplePackPath(const std::string& path);
    const std::string& getSamplePackPath() const { return m_samplePackPath; }

    bool isSoundFontLoaded() const { return m_soundFontLoaded; }

    // Sub-engine Accessors
    JZRealDrumsEngine& getRealDrums() { return m_drumsEngine; }
    const JZRealDrumsEngine& getRealDrums() const { return m_drumsEngine; }

    JZRealTracksEngine& getRealTracks() { return m_tracksEngine; }
    const JZRealTracksEngine& getRealTracks() const { return m_tracksEngine; }

    JZAcousticGuitarBodyImpulseConvolverEngine& getGuitarConvolver() { return m_guitarConvolver; }
    JZGrandPianoSympatheticResonanceEngine& getPianoResonance() { return m_pianoResonance; }
    JZBinauralAcousticRoomConvolverEngine& getRoomConvolver() { return m_roomConvolver; }

    // MIDI Interception
    bool handleMidiNoteOn(int channel, int note, int velocity);
    bool handleMidiNoteOff(int channel, int note);
    bool handleMidiControlChange(int channel, int controller, int value);

    // Master Audio Processing Hook
    void processMasterAudio(float* outL, float* outR, size_t numFrames);

private:
    float m_sampleRate = 48000.0f;
    JZRealAudioMode m_mode = JZRealAudioMode::REALDRUMS_ENHANCED;
    float m_realAudioBlend = 1.0f;

    bool m_enableGuitarBody = true;
    bool m_enablePianoResonance = true;
    bool m_enableBinauralRoom = true;

    std::string m_soundFontPath;
    std::string m_samplePackPath;
    bool m_soundFontLoaded = false;

    // Component Engines
    JZRealDrumsEngine m_drumsEngine;
    JZRealTracksEngine m_tracksEngine;
    JZAcousticGuitarBodyImpulseConvolverEngine m_guitarConvolver;
    JZGrandPianoSympatheticResonanceEngine m_pianoResonance;
    JZBinauralAcousticRoomConvolverEngine m_roomConvolver;

    // Scratch buffers for physical processing
    std::vector<float> m_scratchGuitarL;
    std::vector<float> m_scratchGuitarR;
    std::vector<float> m_scratchPianoL;
    std::vector<float> m_scratchPianoR;
    std::vector<float> m_scratchRoomL;
    std::vector<float> m_scratchRoomR;

    // Simple chord recognition state
    int m_activeChordNotes[12] = {0};
    void updateDetectedChord();
};

} // namespace Jazz

namespace jazz {
    using JZRealInstrumentManager = Jazz::JZRealInstrumentManager;
    using JZRealAudioMode = Jazz::JZRealAudioMode;
}

#endif // JAZZ_REAL_INSTRUMENT_MANAGER_H
