//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// AI Spectral Stem Separator & Audio Unmixing Engine Implementation
//*****************************************************************************

#include "StemSeparatorEngine.h"
#include <cmath>
#include <algorithm>

JZStemSeparatorEngine::JZStemSeparatorEngine(float sampleRate)
  : mSampleRate(sampleRate > 1000.0f ? sampleRate : 44100.0f),
    mIsolationPurity(0.85f)
{
}

void JZStemSeparatorEngine::SetSampleRate(float sampleRate)
{
  if (sampleRate > 1000.0f)
  {
    mSampleRate = sampleRate;
  }
}

void JZStemSeparatorEngine::SetIsolationPurity(float purity)
{
  mIsolationPurity = std::clamp(purity, 0.1f, 1.0f);
}

void JZStemSeparatorEngine::ProcessChannel(
  const std::vector<float>& inChannel,
  std::vector<float>& outVocal,
  std::vector<float>& outDrums,
  std::vector<float>& outBass,
  std::vector<float>& outOthers)
{
  size_t n = inChannel.size();
  outVocal.resize(n, 0.0f);
  outDrums.resize(n, 0.0f);
  outBass.resize(n, 0.0f);
  outOthers.resize(n, 0.0f);

  if (n == 0)
  {
    return;
  }

  // 1. Low-Pass Filter State for Bass (< 220Hz)
  float bassCutoff = 220.0f;
  float bassAlpha = std::exp(-2.0f * 3.14159265f * bassCutoff / mSampleRate);
  float bassLp = 0.0f;

  // 2. Band-Pass Filter States for Vocal Mid Formants (300Hz to 3800Hz)
  float vocalLp1 = 0.0f, vocalLp2 = 0.0f;
  float vLowAlpha = std::exp(-2.0f * 3.14159265f * 300.0f / mSampleRate);
  float vHighAlpha = std::exp(-2.0f * 3.14159265f * 3800.0f / mSampleRate);

  // 3. Transient / Envelope Follower for Drum Onsets
  float prevSample = 0.0f;
  float drumEnv = 0.0f;
  float attackCoeff = std::exp(-1.0f / (0.003f * mSampleRate)); // 3ms attack
  float releaseCoeff = std::exp(-1.0f / (0.040f * mSampleRate)); // 40ms release

  for (size_t i = 0; i < n; ++i)
  {
    float s = inChannel[i];

    // Bass separation via single-pole low pass
    bassLp = bassAlpha * bassLp + (1.0f - bassAlpha) * s;
    float bassSample = bassLp;

    // Vocal mid-band isolation
    vocalLp1 = vLowAlpha * vocalLp1 + (1.0f - vLowAlpha) * s;
    vocalLp2 = vHighAlpha * vocalLp2 + (1.0f - vHighAlpha) * s;
    float midBand = vocalLp2 - vocalLp1;

    // Transient Onset Detection for Drums
    float diff = std::abs(s - prevSample);
    prevSample = s;
    if (diff > drumEnv)
    {
      drumEnv = attackCoeff * drumEnv + (1.0f - attackCoeff) * diff;
    }
    else
    {
      drumEnv = releaseCoeff * drumEnv + (1.0f - releaseCoeff) * diff;
    }

    // Weighting factor
    float drumWeight = std::clamp(drumEnv * 3.0f, 0.0f, 1.0f);
    float drumSample = s * drumWeight;

    // Vocal weighting: high in midband when percussive energy is low
    float vocalWeight = std::clamp((1.0f - drumWeight * 0.7f), 0.0f, 1.0f);
    float vocalSample = midBand * vocalWeight * 0.85f;

    // Others: Remaining melodic harmonic content
    float othersSample = s - (bassSample * 0.9f + drumSample * 0.7f + vocalSample * 0.8f);

    // Apply purity scaling
    float purity = mIsolationPurity;
    outBass[i]   = bassSample * purity;
    outDrums[i]  = drumSample * purity;
    outVocal[i]  = vocalSample * purity;
    outOthers[i] = othersSample * purity;
  }
}

bool JZStemSeparatorEngine::SeparateStems(
  const std::vector<float>& mixedLeft,
  const std::vector<float>& mixedRight,
  JZSeparatedStems& outStems)
{
  outStems.Clear();
  if (mixedLeft.empty() || mixedRight.empty())
  {
    return false;
  }

  ProcessChannel(mixedLeft, outStems.vocalL, outStems.drumsL, outStems.bassL, outStems.othersL);
  ProcessChannel(mixedRight, outStems.vocalR, outStems.drumsR, outStems.bassR, outStems.othersR);

  return true;
}
