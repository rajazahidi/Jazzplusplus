//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Physical Modeling Wind & Brass Tube Resonator Implementation
//*****************************************************************************

#include "WindTubePhysicalModel.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>

JZWindTubePhysicalModel::JZWindTubePhysicalModel(float sampleRate)
  : mSampleRate(sampleRate > 1000.0f ? sampleRate : 44100.0f),
    mType(JZWindInstrumentType::Saxophone),
    mFrequencyHz(261.63f), // C4
    mBreathPressure(0.75f),
    mBreathNoise(0.08f),
    mBellDamping(0.35f),
    mWriteIdx(0),
    mDelayLength(84),
    mBellFilterState(0.0f)
{
  mForwardDelay.assign(2048, 0.0f);
  mBackwardDelay.assign(2048, 0.0f);
  UpdateDelayLength();
}

void JZWindTubePhysicalModel::SetSampleRate(float sampleRate)
{
  if (sampleRate > 1000.0f)
  {
    mSampleRate = sampleRate;
    UpdateDelayLength();
  }
}

void JZWindTubePhysicalModel::SetInstrumentType(JZWindInstrumentType type)
{
  mType = type;
  UpdateDelayLength();
}

void JZWindTubePhysicalModel::SetFrequencyHz(float freqHz)
{
  mFrequencyHz = std::clamp(freqHz, 40.0f, 2500.0f);
  UpdateDelayLength();
}

void JZWindTubePhysicalModel::SetBreathPressure(float pressure)
{
  mBreathPressure = std::clamp(pressure, 0.0f, 1.0f);
}

void JZWindTubePhysicalModel::SetBreathNoise(float noiseAmount)
{
  mBreathNoise = std::clamp(noiseAmount, 0.0f, 0.5f);
}

void JZWindTubePhysicalModel::SetBellDamping(float damping)
{
  mBellDamping = std::clamp(damping, 0.05f, 0.95f);
}

void JZWindTubePhysicalModel::UpdateDelayLength()
{
  float boreMultiplier = (mType == JZWindInstrumentType::Clarinet) ? 4.0f : 2.0f;
  size_t len = static_cast<size_t>(mSampleRate / (boreMultiplier * mFrequencyHz));

  if (len < 4) len = 4;
  if (len >= mForwardDelay.size() - 2) len = mForwardDelay.size() - 3;
  mDelayLength = len;
}

float JZWindTubePhysicalModel::NonLinearReed(float deltaPressure)
{
  if (mType == JZWindInstrumentType::Flute)
  {
    // Jet excitation cubic polynomial
    float jet = std::clamp(deltaPressure, -1.0f, 1.0f);
    return jet - (jet * jet * jet) * 0.333f;
  }
  else if (mType == JZWindInstrumentType::Clarinet || mType == JZWindInstrumentType::Saxophone)
  {
    // Single-reed woodwind characteristic table
    float dp = std::clamp(deltaPressure, -1.5f, 1.5f);
    float flow = dp * std::max(0.0f, 1.0f - 0.65f * dp);
    return flow;
  }
  else // BrassHorn
  {
    // Lip-reed brass non-linear spring
    float lip = std::tanh(deltaPressure * 1.4f);
    return lip * 0.85f;
  }
}

float JZWindTubePhysicalModel::SynthesizeSample()
{
  if (mBreathPressure <= 0.001f)
  {
    return 0.0f;
  }

  // 1. Mouth breath excitation with turbulence noise
  float noise = ((static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX)) * 2.0f - 1.0f) * mBreathNoise;
  float mouthPressure = mBreathPressure + noise;

  // 2. Read bore reflections
  size_t readIdx = (mWriteIdx + mForwardDelay.size() - mDelayLength) % mForwardDelay.size();
  float backwardWave = mBackwardDelay[readIdx];

  // 3. Differential pressure at reed / mouthpiece
  float deltaP = mouthPressure - backwardWave;
  float reedFlow = NonLinearReed(deltaP);

  // 4. Inject into forward waveguide
  float forwardIn = backwardWave + reedFlow;
  mForwardDelay[mWriteIdx] = forwardIn;

  // 5. Bell reflection and radiation low-pass filter
  float forwardOut = mForwardDelay[readIdx];
  float reflectionSign = (mType == JZWindInstrumentType::Flute) ? -1.0f : -0.92f;

  float alpha = mBellDamping;
  mBellFilterState = alpha * mBellFilterState + (1.0f - alpha) * forwardOut;

  // Reflected back into tube
  mBackwardDelay[mWriteIdx] = mBellFilterState * reflectionSign;

  mWriteIdx = (mWriteIdx + 1) % mForwardDelay.size();

  // Radiated acoustic pressure
  float radiatedPressure = (forwardOut - mBellFilterState) * 1.5f;
  return std::clamp(radiatedPressure, -1.0f, 1.0f);
}

void JZWindTubePhysicalModel::ProcessBuffer(std::vector<float>& outBuffer, size_t numSamples)
{
  outBuffer.resize(numSamples);
  for (size_t i = 0; i < numSamples; ++i)
  {
    outBuffer[i] = SynthesizeSample();
  }
}
