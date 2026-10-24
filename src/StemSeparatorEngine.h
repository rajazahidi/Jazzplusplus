//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// AI Spectral Stem Separator & Audio Unmixing Engine
//
// Separates stereo mixed audio into 4 isolated stems:
// 1. Vocals (mid-band harmonic speech/singing formant concentration)
// 2. Drums (transient onset / percussive noise components)
// 3. Bass (fundamental sub-bass harmonics < 250Hz)
// 4. Others (melodic guitars, keyboards, synthesizers, brass, strings)
//*****************************************************************************

#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

enum class JZStemType
{
  Vocal = 0,
  Drums,
  Bass,
  Others
};

struct JZSeparatedStems
{
  std::vector<float> vocalL;
  std::vector<float> vocalR;

  std::vector<float> drumsL;
  std::vector<float> drumsR;

  std::vector<float> bassL;
  std::vector<float> bassR;

  std::vector<float> othersL;
  std::vector<float> othersR;

  void Clear()
  {
    vocalL.clear(); vocalR.clear();
    drumsL.clear(); drumsR.clear();
    bassL.clear(); bassR.clear();
    othersL.clear(); othersR.clear();
  }
};

class JZStemSeparatorEngine
{
public:
  JZStemSeparatorEngine(float sampleRate = 44100.0f);
  ~JZStemSeparatorEngine() = default;

  void SetSampleRate(float sampleRate);
  float GetSampleRate() const { return mSampleRate; }

  void SetIsolationPurity(float purity); // 0.0 (gentle separation) to 1.0 (strict isolation)
  float GetIsolationPurity() const { return mIsolationPurity; }

  // Separate stereo mixed tracks into 4 stems
  bool SeparateStems(
    const std::vector<float>& mixedLeft,
    const std::vector<float>& mixedRight,
    JZSeparatedStems& outStems);

private:
  float mSampleRate;
  float mIsolationPurity;

  void ProcessChannel(
    const std::vector<float>& inChannel,
    std::vector<float>& outVocal,
    std::vector<float>& outDrums,
    std::vector<float>& outBass,
    std::vector<float>& outOthers);
};
