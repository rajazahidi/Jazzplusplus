//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Professional Multi-Channel Virtual Mixing Console Desk
//
// Vertical Channel Strips, dB Faders (-inf to +6dB), Stereo Peak Meter Ladders,
// Rotary Pan Pots, Mute/Solo/Arm, Auxiliary Send Busses & Master Brickwall Limiter.
//*****************************************************************************

#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

// Individual Track Channel Strip
struct JZChannelStrip
{
  int trackIndex;
  std::string name;
  float faderDb;           // -60.0 dB to +6.0 dB (or -inf)
  int pan;                 // 0 (Hard Left) to 127 (Hard Right), 64 = Center
  bool isMuted;
  bool isSolo;
  bool isArmed;
  float sendA;             // Aux Send A (Reverb Bus) [0.0, 1.0]
  float sendB;             // Aux Send B (Delay Bus)  [0.0, 1.0]

  // Peak Meters (Peak Hold & RMS Decay)
  float peakL;             // [0.0, 1.0+]
  float peakR;             // [0.0, 1.0+]

  JZChannelStrip(int idx = 0, const std::string& n = "Track")
    : trackIndex(idx),
      name(n),
      faderDb(0.0f),
      pan(64),
      isMuted(false),
      isSolo(false),
      isArmed(false),
      sendA(0.0f),
      sendB(0.0f),
      peakL(0.0f),
      peakR(0.0f)
  {
  }

  float GetLinearGain() const
  {
    if (isMuted) return 0.0f;
    if (faderDb <= -60.0f) return 0.0f;
    return std::pow(10.0f, faderDb / 20.0f);
  }

  void GetPanGains(float& leftGain, float& rightGain) const
  {
    float panNorm = (float)pan / 127.0f; // 0.0 to 1.0
    float angle = panNorm * 1.5707963f;  // 0 to pi/2
    leftGain = std::cos(angle);
    rightGain = std::sin(angle);
  }
};

// Virtual Mixing Console Engine
class JZVirtualMixer
{
  public:

    static JZVirtualMixer& Instance();

    void Initialize(int numTracks = 16);
    void Clear();

    int GetChannelCount() const { return (int)mChannels.size(); }
    JZChannelStrip* GetChannel(int index);
    const JZChannelStrip* GetChannel(int index) const;

    // Master Bus Output
    float GetMasterFaderDb() const { return mMasterFaderDb; }
    void SetMasterFaderDb(float db) { mMasterFaderDb = std::max(-60.0f, std::min(6.0f, db)); }
    float GetMasterPeakL() const { return mMasterPeakL; }
    float GetMasterPeakR() const { return mMasterPeakR; }

    bool IsMasterLimiterEnabled() const { return mMasterLimiter; }
    void SetMasterLimiter(bool enable) { mMasterLimiter = enable; }

    // Mixdown and metering processor for an audio block
    void ProcessMasterBlock(
      float* pLeftOut,
      float* pRightOut,
      int numFrames);

    void ResetMeters();

    // Utility decibel converters
    static float DbToGain(float db);
    static float GainToDb(float gain);

  private:

    JZVirtualMixer();

    std::vector<JZChannelStrip> mChannels;
    float mMasterFaderDb;
    float mMasterPeakL;
    float mMasterPeakR;
    bool mMasterLimiter;
};
