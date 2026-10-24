//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Studio Spectral Audio Denoiser & Noise Repair Implementation
//*****************************************************************************

#include "SpectralDenoise.h"

JZSpectralDenoise::JZSpectralDenoise(float sampleRate)
  : mSampleRate(sampleRate > 1000.0f ? sampleRate : 44100.0f),
    mProfileFramesCount(0),
    mCurrentNoiseLevelDB(-96.0f)
{
  InitializeBands();
}

void JZSpectralDenoise::SetSampleRate(float sampleRate)
{
  if (sampleRate > 1000.0f)
  {
    mSampleRate = sampleRate;
  }
}

void JZSpectralDenoise::SetConfig(const JZSpectralDenoiseConfig& config)
{
  mConfig = config;
}

void JZSpectralDenoise::InitializeBands()
{
  mNoiseProfile.assign(NUM_BANDS, 1.0e-5f);
  mBandGainL.assign(NUM_BANDS, 1.0f);
  mBandGainR.assign(NUM_BANDS, 1.0f);
}

void JZSpectralDenoise::Reset()
{
  mBandGainL.assign(NUM_BANDS, 1.0f);
  mBandGainR.assign(NUM_BANDS, 1.0f);
  mCurrentNoiseLevelDB = -96.0f;
}

void JZSpectralDenoise::StartLearningProfile()
{
  mConfig.learnProfileMode = true;
  mProfileFramesCount = 0;
  mNoiseProfile.assign(NUM_BANDS, 1.0e-6f);
}

void JZSpectralDenoise::StopLearningProfile()
{
  mConfig.learnProfileMode = false;
}

void JZSpectralDenoise::ClearProfile()
{
  mNoiseProfile.assign(NUM_BANDS, 1.0e-5f);
  mProfileFramesCount = 0;
}

void JZSpectralDenoise::EstimateBandEnergies(
  const float* signal,
  int numSamples,
  std::vector<float>& bandEnergies) const
{
  bandEnergies.assign(NUM_BANDS, 1.0e-6f);
  if (!signal || numSamples <= 0) return;

  // Split audio block into log-spaced frequency bands (30 Hz to 20,000 Hz)
  float minF = 30.0f;
  float maxF = std::min(mSampleRate * 0.48f, 20000.0f);
  float logMin = std::log10(minF);
  float logMax = std::log10(maxF);

  for (int b = 0; b < NUM_BANDS; ++b)
  {
    float fCenter = std::pow(10.0f, logMin + (logMax - logMin) * ((float)b / (float)(NUM_BANDS - 1)));
    float k = (float)numSamples * fCenter / mSampleRate;
    float omega = 2.0f * 3.14159265f * k / (float)numSamples;
    float coeff = 2.0f * std::cos(omega);

    // Goertzel single-bin energy
    float q0 = 0.0f, q1 = 0.0f, q2 = 0.0f;
    for (int i = 0; i < numSamples; ++i)
    {
      q0 = coeff * q1 - q2 + signal[i];
      q2 = q1;
      q1 = q0;
    }

    float mag = std::sqrt(std::max(0.0f, q1 * q1 + q2 * q2 - q1 * q2 * coeff)) / (float)numSamples;
    bandEnergies[b] = std::max(1.0e-6f, mag);
  }
}

void JZSpectralDenoise::Process(float* inOutL, float* inOutR, int numSamples)
{
  if (!inOutL || numSamples <= 0) return;

  std::vector<float> energyL, energyR;
  EstimateBandEnergies(inOutL, numSamples, energyL);
  if (inOutR) EstimateBandEnergies(inOutR, numSamples, energyR);
  else energyR = energyL;

  // 1. Noise Profile Learning Mode
  if (mConfig.learnProfileMode)
  {
    mProfileFramesCount++;
    float alphaLearn = 1.0f / (float)mProfileFramesCount;
    float sumProfile = 0.0f;

    for (int b = 0; b < NUM_BANDS; ++b)
    {
      float currentNoiseMag = 0.5f * (energyL[b] + energyR[b]);
      mNoiseProfile[b] = (1.0f - alphaLearn) * mNoiseProfile[b] + alphaLearn * currentNoiseMag;
      sumProfile += mNoiseProfile[b];
    }

    float avgNoise = sumProfile / (float)NUM_BANDS;
    mCurrentNoiseLevelDB = (avgNoise > 1e-6f) ? (20.0f * std::log10(avgNoise)) : -96.0f;
    return;
  }

  // 2. Spectral Subtraction Gain Calculation
  float maxReductionLin = std::pow(10.0f, -mConfig.reductionDB / 20.0f);
  float sensitivityOffsetLin = std::pow(10.0f, mConfig.thresholdOffsetDB / 20.0f);
  float floorLimit = std::max(0.001f, mConfig.spectralFloor);

  float overallAvgGain = 0.0f;

  for (int b = 0; b < NUM_BANDS; ++b)
  {
    float noiseThreshold = mNoiseProfile[b] * sensitivityOffsetLin;

    // Gain for Left channel
    float snrL = energyL[b] / std::max(1.0e-6f, noiseThreshold);
    float targetGainL = 1.0f;
    if (snrL < 1.0f)
    {
      targetGainL = maxReductionLin;
    }
    else if (snrL < 4.0f)
    {
      // Soft transition knee
      float frac = (snrL - 1.0f) / 3.0f;
      targetGainL = maxReductionLin + frac * (1.0f - maxReductionLin);
    }
    targetGainL = std::max(floorLimit, targetGainL);
    mBandGainL[b] = 0.7f * mBandGainL[b] + 0.3f * targetGainL;

    // Gain for Right channel
    float snrR = energyR[b] / std::max(1.0e-6f, noiseThreshold);
    float targetGainR = 1.0f;
    if (snrR < 1.0f)
    {
      targetGainR = maxReductionLin;
    }
    else if (snrR < 4.0f)
    {
      float frac = (snrR - 1.0f) / 3.0f;
      targetGainR = maxReductionLin + frac * (1.0f - maxReductionLin);
    }
    targetGainR = std::max(floorLimit, targetGainR);
    mBandGainR[b] = 0.7f * mBandGainR[b] + 0.3f * targetGainR;

    overallAvgGain += 0.5f * (mBandGainL[b] + mBandGainR[b]);
  }

  overallAvgGain /= (float)NUM_BANDS;

  // 3. Apply Spectral Subtraction Gains
  for (int i = 0; i < numSamples; ++i)
  {
    float origL = inOutL[i];
    float origR = inOutR ? inOutR[i] : origL;

    float cleanL = origL * overallAvgGain;
    float cleanR = origR * overallAvgGain;

    if (mConfig.listenNoiseOnly)
    {
      inOutL[i] = origL - cleanL;
      if (inOutR) inOutR[i] = origR - cleanR;
    }
    else
    {
      inOutL[i] = cleanL;
      if (inOutR) inOutR[i] = cleanR;
    }
  }
}

void JZSpectralDenoise::ProcessMono(float* inOut, int numSamples)
{
  Process(inOut, nullptr, numSamples);
}
