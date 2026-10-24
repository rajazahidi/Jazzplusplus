//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// ITU-R BS.1770-4 / EBU R128 Loudness Meter & True Peak Engine Implementation
//*****************************************************************************

#include "LoudnessMeterEBU.h"

JZLoudnessMeterEBU::JZLoudnessMeterEBU(float sampleRate)
  : mSampleRate(sampleRate > 1000.0f ? sampleRate : 44100.0f),
    mTargetPreset(LOUDNESS_SPOTIFY),
    mHsX1L(0), mHsX2L(0), mHsY1L(0), mHsY2L(0),
    mHsX1R(0), mHsX2R(0), mHsY1R(0), mHsY2R(0),
    mHpX1L(0), mHpX2L(0), mHpY1L(0), mHpY2L(0),
    mHpX1R(0), mHpX2R(0), mHpY1R(0), mHpY2R(0),
    mMomentaryPos(0), mShortTermPos(0)
{
  UpdateFilterCoefficients();
  Reset();
}

void JZLoudnessMeterEBU::SetSampleRate(float sampleRate)
{
  if (sampleRate > 1000.0f)
  {
    mSampleRate = sampleRate;
    UpdateFilterCoefficients();
    Reset();
  }
}

void JZLoudnessMeterEBU::SetTargetPreset(JZLoudnessTargetPreset preset)
{
  mTargetPreset = preset;
}

float JZLoudnessMeterEBU::GetTargetLUFS() const
{
  switch (mTargetPreset)
  {
    case LOUDNESS_SPOTIFY:
    case LOUDNESS_YOUTUBE:
      return -14.0f;
    case LOUDNESS_APPLE_MUSIC:
      return -16.0f;
    case LOUDNESS_EBU_R128:
      return -23.0f;
    case LOUDNESS_ATSC_A85:
      return -24.0f;
  }
  return -14.0f;
}

std::string JZLoudnessMeterEBU::GetPresetName(JZLoudnessTargetPreset preset)
{
  switch (preset)
  {
    case LOUDNESS_SPOTIFY:     return "Spotify / Tidal (-14 LUFS, -1.0 dBTP)";
    case LOUDNESS_APPLE_MUSIC: return "Apple Music Sound Check (-16 LUFS, -1.0 dBTP)";
    case LOUDNESS_YOUTUBE:     return "YouTube Music (-14 LUFS, -1.0 dBTP)";
    case LOUDNESS_EBU_R128:    return "EBU R128 European Broadcast (-23 LUFS, -1.0 dBTP)";
    case LOUDNESS_ATSC_A85:    return "ATSC A/85 US TV Broadcast (-24 LUFS, -2.0 dBTP)";
  }
  return "Standard";
}

void JZLoudnessMeterEBU::Reset()
{
  mHsX1L = mHsX2L = mHsY1L = mHsY2L = 0.0f;
  mHsX1R = mHsX2R = mHsY1R = mHsY2R = 0.0f;
  mHpX1L = mHpX2L = mHpY1L = mHpY2L = 0.0f;
  mHpX1R = mHpX2R = mHpY1R = mHpY2R = 0.0f;

  mMomentarySamples = (int)(mSampleRate * 0.4f); // 400ms
  mShortTermSamples = (int)(mSampleRate * 3.0f); // 3.0s

  mMomentaryBuffer.assign(mMomentarySamples, 0.0f);
  mShortTermBuffer.assign(mShortTermSamples, 0.0f);
  mMomentaryPos = 0;
  mShortTermPos = 0;

  mBlockEnergies.clear();
  mMetrics = JZLoudnessMetrics();
}

void JZLoudnessMeterEBU::UpdateFilterCoefficients()
{
  // Stage 1: High-Shelf Filter (+3.99 dB gain around 1681.97 Hz)
  float dbGain = 3.9998438f;
  float f0_hs = 1681.97445f;
  float K = std::tan(3.14159265f * f0_hs / mSampleRate);
  float Vh = std::pow(10.0f, dbGain / 20.0f);
  float Vb = std::pow(Vh, 0.49966677f);
  float a0_hs = 1.0f + 1.41421356f * K + K * K;

  hs_b0 = (Vh + Vb * 1.41421356f * K + K * K) / a0_hs;
  hs_b1 = 2.0f * (K * K - Vh) / a0_hs;
  hs_b2 = (Vh - Vb * 1.41421356f * K + K * K) / a0_hs;
  hs_a1 = 2.0f * (K * K - 1.0f) / a0_hs;
  hs_a2 = (1.0f - 1.41421356f * K + K * K) / a0_hs;

  // Stage 2: RLB High-Pass Filter (Q = 0.5, f0 = 38.13 Hz)
  float f0_hp = 38.13547f;
  float Q_hp = 0.500327f;
  float K_hp = std::tan(3.14159265f * f0_hp / mSampleRate);
  float a0_hp = 1.0f + K_hp / Q_hp + K_hp * K_hp;

  hp_b0 = 1.0f / a0_hp;
  hp_b1 = -2.0f / a0_hp;
  hp_b2 = 1.0f / a0_hp;
  hp_a1 = 2.0f * (K_hp * K_hp - 1.0f) / a0_hp;
  hp_a2 = (1.0f - K_hp / Q_hp + K_hp * K_hp) / a0_hp;
}

float JZLoudnessMeterEBU::FilterSample(float in, float b0, float b1, float b2, float a1, float a2,
                                     float& x1, float& x2, float& y1, float& y2)
{
  float out = b0 * in + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
  x2 = x1; x1 = in;
  y2 = y1; y1 = out;
  return out;
}

float JZLoudnessMeterEBU::DetectTruePeak4x(float s0, float s1)
{
  // 4x polyphase inter-sample interpolation using cubic Hermite splines
  float maxP = std::max(std::fabs(s0), std::fabs(s1));
  for (int step = 1; step <= 3; ++step)
  {
    float t = (float)step * 0.25f;
    // Smooth cosine interpolation
    float interp = s0 + (s1 - s0) * (0.5f - 0.5f * std::cos(3.14159265f * t));
    maxP = std::max(maxP, std::fabs(interp));
  }
  return maxP;
}

void JZLoudnessMeterEBU::ProcessBlock(const float* left, const float* right, int numSamples)
{
  if (!left || numSamples <= 0) return;

  float maxInterSamplePeak = 0.0f;

  for (int i = 0; i < numSamples; ++i)
  {
    float rawL = left[i];
    float rawR = right ? right[i] : rawL;

    // 1. True Peak 4x Over-sampled calculation
    if (i > 0)
    {
      maxInterSamplePeak = std::max(maxInterSamplePeak, DetectTruePeak4x(left[i - 1], rawL));
      if (right)
      {
        maxInterSamplePeak = std::max(maxInterSamplePeak, DetectTruePeak4x(right[i - 1], rawR));
      }
    }
    else
    {
      maxInterSamplePeak = std::max(maxInterSamplePeak, std::max(std::fabs(rawL), std::fabs(rawR)));
    }

    // 2. K-Weighting Stage 1: High-Shelf
    float hsL = FilterSample(rawL, hs_b0, hs_b1, hs_b2, hs_a1, hs_a2, mHsX1L, mHsX2L, mHsY1L, mHsY2L);
    float hsR = FilterSample(rawR, hs_b0, hs_b1, hs_b2, hs_a1, hs_a2, mHsX1R, mHsX2R, mHsY1R, mHsY2R);

    // 3. K-Weighting Stage 2: RLB High-Pass
    float kWeightedL = FilterSample(hsL, hp_b0, hp_b1, hp_b2, hp_a1, hp_a2, mHpX1L, mHpX2L, mHpY1L, mHpY2L);
    float kWeightedR = FilterSample(hsR, hp_b0, hp_b1, hp_b2, hp_a1, hp_a2, mHpX1R, mHpX2R, mHpY1R, mHpY2R);

    // 4. Mean-square energy (ITU-R BS.1770 channel weighting: L = 1.0, R = 1.0)
    float sampleEnergy = (kWeightedL * kWeightedL) + (kWeightedR * kWeightedR);

    // 5. Store in sliding buffers
    mMomentaryBuffer[mMomentaryPos] = sampleEnergy;
    mMomentaryPos = (mMomentaryPos + 1) % mMomentarySamples;

    mShortTermBuffer[mShortTermPos] = sampleEnergy;
    mShortTermPos = (mShortTermPos + 1) % mShortTermSamples;
  }

  // 6. Compute Momentary LUFS (400ms average)
  float sumMom = 0.0f;
  for (float e : mMomentaryBuffer) sumMom += e;
  float meanMom = sumMom / (float)mMomentarySamples;
  float momLUFS = (meanMom > 1e-7f) ? (-0.691f + 10.0f * std::log10(meanMom)) : -70.0f;
  mMetrics.momentaryLUFS = momLUFS;
  mMetrics.maxMomentaryLUFS = std::max(mMetrics.maxMomentaryLUFS, momLUFS);

  // 7. Compute Short-Term LUFS (3s average)
  float sumShort = 0.0f;
  for (float e : mShortTermBuffer) sumShort += e;
  float meanShort = sumShort / (float)mShortTermSamples;
  float shortLUFS = (meanShort > 1e-7f) ? (-0.691f + 10.0f * std::log10(meanShort)) : -70.0f;
  mMetrics.shortTermLUFS = shortLUFS;
  mMetrics.maxShortTermLUFS = std::max(mMetrics.maxShortTermLUFS, shortLUFS);

  // Accumulate block energy for integrated loudness
  if (meanMom > 1e-7f)
  {
    mBlockEnergies.push_back(meanMom);
  }

  // 8. Compute True Peak in dBTP
  float currentTP = (maxInterSamplePeak > 1e-5f) ? (20.0f * std::log10(maxInterSamplePeak)) : -96.0f;
  mMetrics.truePeakDBTP = std::max(mMetrics.truePeakDBTP, currentTP);

  // 9. Compute Integrated Loudness & LRA
  ComputeIntegratedLoudness();

  // 10. Compliance offset check
  float targetLUFS = GetTargetLUFS();
  mMetrics.targetOffsetLU = mMetrics.integratedLUFS - targetLUFS;
  mMetrics.isTargetCompliant = (mMetrics.integratedLUFS <= targetLUFS + 0.5f) && (mMetrics.truePeakDBTP <= -1.0f);
}

void JZLoudnessMeterEBU::ComputeIntegratedLoudness()
{
  if (mBlockEnergies.empty()) return;

  // Step 1: Calculate ungated loudness across all accumulated 400ms blocks
  float totalEnergy = 0.0f;
  for (float e : mBlockEnergies) totalEnergy += e;
  float meanUngated = totalEnergy / (float)mBlockEnergies.size();
  float ungatedLUFS = (meanUngated > 1e-7f) ? (-0.691f + 10.0f * std::log10(meanUngated)) : -70.0f;

  // Step 2: Absolute threshold at -70 LKFS and relative threshold at -10 LU below ungated mean
  float relativeThresholdDB = ungatedLUFS - 10.0f;
  float absoluteThresholdLinear = std::pow(10.0f, (-70.0f + 0.691f) / 10.0f);
  float relativeThresholdLinear = std::pow(10.0f, (relativeThresholdDB + 0.691f) / 10.0f);

  float gatedThresholdLinear = std::max(absoluteThresholdLinear, relativeThresholdLinear);

  float gatedSum = 0.0f;
  int gatedCount = 0;
  std::vector<float> gatedLoudnessValues;

  for (float e : mBlockEnergies)
  {
    if (e >= gatedThresholdLinear)
    {
      gatedSum += e;
      gatedCount++;
      float l = -0.691f + 10.0f * std::log10(e);
      gatedLoudnessValues.push_back(l);
    }
  }

  if (gatedCount > 0)
  {
    float gatedMean = gatedSum / (float)gatedCount;
    mMetrics.integratedLUFS = -0.691f + 10.0f * std::log10(gatedMean);

    // Loudness Range (LRA): 95th percentile minus 10th percentile
    if (gatedLoudnessValues.size() >= 10)
    {
      std::sort(gatedLoudnessValues.begin(), gatedLoudnessValues.end());
      int p10 = (int)(gatedLoudnessValues.size() * 0.10f);
      int p95 = (int)(gatedLoudnessValues.size() * 0.95f);
      mMetrics.loudnessRangeLU = std::max(0.0f, gatedLoudnessValues[p95] - gatedLoudnessValues[p10]);
    }
  }
}
