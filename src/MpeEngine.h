//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// MPE (MIDI Polyphonic Expression) & MusicXML Interoperability Engine
//
// Per-Note Pitch Bend, Pressure, Slide (CC74), Multidimensional Polyphonic
// Touch Allocation and Comprehensive MusicXML Score Import / Export Suite.
//*****************************************************************************

#pragma once

#include <string>
#include <vector>
#include <map>

class JZSong;
class JZTrack;

// MPE Active Note Voice Record
struct JZMpeVoice
{
  int pitch;                 // MIDI note number (0 - 127)
  int channel;               // Allocated member channel (2 - 16)
  int velocity;              // Note-on strike velocity
  int pitchBend;             // Per-note pitch bend (-8192 to +8191)
  int pressure;              // Per-note channel pressure (0 - 127)
  int slideCC74;             // Per-note vertical slide / timbre (0 - 127)
  bool isActive;

  JZMpeVoice(int p = 60, int ch = 2, int vel = 80)
    : pitch(p),
      channel(ch),
      velocity(vel),
      pitchBend(0),
      pressure(0),
      slideCC74(64),
      isActive(true)
  {
  }
};

// MPE Polyphonic Voice Allocator & Controller
class JZMpeManager
{
  public:

    static JZMpeManager& Instance();

    void Reset();

    // Configuration
    int GetMasterChannel() const { return mMasterChannel; }
    void SetMasterChannel(int ch) { mMasterChannel = ch; }
    int GetZoneStartChannel() const { return mZoneStart; }
    int GetZoneEndChannel() const { return mZoneEnd; }
    void SetZone(int startCh, int endCh);

    // Voice lifecycle
    int NoteOn(int pitch, int velocity);
    int NoteOff(int pitch);

    // Per-note multidimensional controllers
    bool SetPerNotePitchBend(int pitch, int bendValue);
    bool SetPerNotePressure(int pitch, int pressure);
    bool SetPerNoteSlide(int pitch, int cc74Value);

    // Query channel for a specific playing note
    int GetChannelForPitch(int pitch) const;
    int GetActiveVoiceCount() const;

  private:

    JZMpeManager();

    int mMasterChannel;      // Channel 1 standard
    int mZoneStart;          // Channel 2
    int mZoneEnd;            // Channel 16
    int mLastAllocatedChannel;
    std::map<int, JZMpeVoice> mActiveVoices;
};

// MusicXML Professional Score Import / Export Engine
class JZMusicXmlEngine
{
  public:

    // Export entire song structure to standard MusicXML (.musicxml / .xml)
    static bool ExportMusicXml(
      JZSong* pSong,
      const std::string& filePath,
      const std::string& songTitle = "Untitled Jazz Project",
      const std::string& composer = "Jazz++ Arranger");

    // Import MusicXML score into JZSong tracks, notes, tempo and meter
    static bool ImportMusicXml(
      JZSong* pSong,
      const std::string& filePath);
};
