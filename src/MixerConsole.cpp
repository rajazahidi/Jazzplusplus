//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Professional Multi-Channel Virtual Mixing Console Desk Implementation
//*****************************************************************************

#include "MixerConsole.h"

#include <cmath>
#include <sstream>
#include <algorithm>

using namespace std;

//-----------------------------------------------------------------------------
// Singleton Instance
//-----------------------------------------------------------------------------
JZVirtualMixer& JZVirtualMixer::Instance()
{
  static JZVirtualMixer sInstance;
  return sInstance;
}

JZVirtualMixer::JZVirtualMixer()
  : mMasterFaderDb(0.0f),
    mMasterPeakL(0.0f),
    mMasterPeakR(0.0f),
    mMasterLimiter(true)
{
  Initialize(16);
}

void JZVirtualMixer::Initialize(int numTracks)
{
  mChannels.clear();
  if (numTracks < 1) numTracks = 16;
  mChannels.reserve(numTracks);

  static const char* sTrackNames[16] = {
    "Track 1 (Lead)", "Track 2 (Bass)", "Track 3 (Pad)", "Track 4 (Rhythm)",
    "Track 5 (Horns)", "Track 6 (Guitar)", "Track 7 (Strings)", "Track 8 (Synth)",
    "Track 9 (Perc)", "Track 10 (Drums)", "Track 11 (Vocal)", "Track 12 (FX)",
    "Track 13 (Aux)", "Track 14 (Sub)", "Track 15 (Key)", "Track 16 (Solo)"
  };

  for (int i = 0; i < numTracks; ++i)
  {
    string name = (i < 16) ? sTrackNames[i] : "Track";
    mChannels.push_back(JZChannelStrip(i, name));
  }
}

void JZVirtualMixer::Clear()
{
  mChannels.clear();
  mMasterPeakL = 0.0f;
  mMasterPeakR = 0.0f;
}

JZChannelStrip* JZVirtualMixer::GetChannel(int index)
{
  if (index < 0 || index >= (int)mChannels.size()) return 0;
  return &mChannels[index];
}

const JZChannelStrip* JZVirtualMixer::GetChannel(int index) const
{
  if (index < 0 || index >= (int)mChannels.size()) return 0;
  return &mChannels[index];
}

void JZVirtualMixer::ResetMeters()
{
  mMasterPeakL = 0.0f;
  mMasterPeakR = 0.0f;
  for (size_t i = 0; i < mChannels.size(); ++i)
  {
    mChannels[i].peakL = 0.0f;
    mChannels[i].peakR = 0.0f;
  }
}

float JZVirtualMixer::DbToGain(float db)
{
  if (db <= -60.0f) return 0.0f;
  return pow(10.0f, db / 20.0f);
}

float JZVirtualMixer::GainToDb(float gain)
{
  if (gain <= 0.00001f) return -60.0f;
  return 20.0f * log10(gain);
}

void JZVirtualMixer::ProcessMasterBlock(
  float* pLeftOut,
  float* pRightOut,
  int numFrames)
{
  if (!pLeftOut || !pRightOut || numFrames <= 0)
  {
    return;
  }

  float masterGain = DbToGain(mMasterFaderDb);

  float maxL = 0.0f;
  float maxR = 0.0f;

  for (int s = 0; s < numFrames; ++s)
  {
    float l = pLeftOut[s] * masterGain;
    float r = pRightOut[s] * masterGain;

    // Master brickwall soft limiter (analog warmth tanh saturation)
    if (mMasterLimiter)
    {
      if (fabs(l) > 0.95f) l = (float)tanh(l * 0.95f);
      if (fabs(r) > 0.95f) r = (float)tanh(r * 0.95f);
    }

    pLeftOut[s] = l;
    pRightOut[s] = r;

    if (fabs(l) > maxL) maxL = fabs(l);
    if (fabs(r) > maxR) maxR = fabs(r);
  }

  // Peak hold with fast attack, smooth decay
  mMasterPeakL = max(maxL, mMasterPeakL * 0.88f);
  mMasterPeakR = max(maxR, mMasterPeakR * 0.88f);
}
