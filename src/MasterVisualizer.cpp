//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Master Spectrum Analyzer & Stereo Goniometer Visualizer Implementation
//*****************************************************************************

#include "MasterVisualizer.h"

JZMasterVisualizer::JZMasterVisualizer(float sampleRate)
  : mSampleRate(sampleRate > 1000.0f ? sampleRate : 44100.0f),
    mDecaySpeedDBPerSec(24.0f)
{
  InitializeBands();
  mRecentPoints.reserve(256);
}

void JZMasterVisualizer::SetSampleRate(float sampleRate)
{
  if (sampleRate > 1000.0f)
  {
    mSampleRate = sampleRate;
  }
}

void JZMasterVisualizer::SetReleaseSpeed(float decaySpeedDBPerSec)
{
  mDecaySpeedDBPerSec = std::max(6.0f, decaySpeedDBPerSec);
}

void JZMasterVisualizer::InitializeBands()
{
  // 31 standard 1/3-octave ISO frequencies
  static const float isoFreqs[31] = {
    20.0f, 25.0f, 31.5f, 40.0f, 50.0f, 63.0f, 80.0f, 100.0f, 125.0f, 160.0f,
    200.0f, 250.0f, 315.0f, 400.0f, 500.0f, 630.0f, 800.0f, 1000.0f, 1250.0f, 1600.0f,
    2000.0f, 2500.0f, 3150.0f, 4000.0f, 5000.0f, 6300.0f, 8000.0f, 10000.0f, 12500.0f, 16000.0f, 20000.0f
  };

  static const char* isoLabels[31] = {
    "20", "25", "31", "40", "50", "63", "80", "100", "125", "160",
    "200", "250", "315", "400", "500", "630", "800", "1k", "1.2k", "1.6k",
    "2k", "2.5k", "3.1k", "4k", "5k", "6.3k", "8k", "10k", "12.5k", "16k", "20k"
  };

  mBands.clear();
  mBands.reserve(31);
  for (int i = 0; i < 31; ++i)
  {
    mBands.emplace_back(isoFreqs[i], isoLabels[i]);
  }
}

void JZMasterVisualizer::ResetPeaks()
{
  for (auto& b : mBands)
  {
    b.magnitudeDB = -96.0f;
    b.peakHoldDB = -96.0f;
    b.peakDecayTimer = 0.0f;
  }
}

std::vector<JZGoniometerPoint> JZMasterVisualizer::GetGoniometerPoints(int maxPoints) const
{
  if (mRecentPoints.empty()) return {};
  int n = (int)mRecentPoints.size();
  int count = std::min(maxPoints, n);
  int start = n - count;
  return std::vector<JZGoniometerPoint>(mRecentPoints.begin() + start, mRecentPoints.end());
}

void JZMasterVisualizer::ProcessBlock(const float* left, const float* right, int numSamples)
{
  if (!left || numSamples <= 0) return;

  const float invSqrt2 = 0.70710678f;
  float sumL2 = 0.0f;
  float sumR2 = 0.0f;
  float sumLR = 0.0f;
  float peakL = 0.0f;
  float peakR = 0.0f;
  float sumMid2 = 0.0f;
  float sumSide2 = 0.0f;

  std::vector<float> monoBuffer(numSamples);
  mRecentPoints.clear();

  // Downsample goniometer points if numSamples is large
  int step = std::max(1, numSamples / 128);

  for (int i = 0; i < numSamples; ++i)
  {
    float l = left[i];
    float r = right ? right[i] : l;

    // Peak levels
    peakL = std::max(peakL, std::fabs(l));
    peakR = std::max(peakR, std::fabs(r));

    // Energy for RMS and Phase correlation
    sumL2 += l * l;
    sumR2 += r * r;
    sumLR += l * r;

    // Mid-Side for goniometer
    float mid = (l + r) * invSqrt2;
    float side = (l - r) * invSqrt2;

    sumMid2 += mid * mid;
    sumSide2 += side * side;

    if (i % step == 0 && mRecentPoints.size() < 128)
    {
      mRecentPoints.emplace_back(side, mid);
    }

    monoBuffer[i] = mid;
  }

  // 1. Phase Correlation: r = sum(L*R) / sqrt(sum(L^2) * sum(R^2))
  float denom = std::sqrt(sumL2 * sumR2);
  if (denom > 1.0e-6f)
  {
    mMetrics.phaseCorrelation = std::max(-1.0f, std::min(1.0f, sumLR / denom));
  }
  else
  {
    mMetrics.phaseCorrelation = 1.0f;
  }

  // 2. RMS and Peak dB levels
  float rmsL = std::sqrt(sumL2 / (float)numSamples);
  float rmsR = std::sqrt(sumR2 / (float)numSamples);
  mMetrics.rmsLeftDB = (rmsL > 1.0e-5f) ? (20.0f * std::log10(rmsL)) : -96.0f;
  mMetrics.rmsRightDB = (rmsR > 1.0e-5f) ? (20.0f * std::log10(rmsR)) : -96.0f;
  mMetrics.peakLeftDB = (peakL > 1.0e-5f) ? (20.0f * std::log10(peakL)) : -96.0f;
  mMetrics.peakRightDB = (peakR > 1.0e-5f) ? (20.0f * std::log10(peakR)) : -96.0f;

  // 3. Stereo spread ratio
  if (sumMid2 > 1.0e-6f)
  {
    mMetrics.stereoSpreadPercent = std::min(200.0f, (std::sqrt(sumSide2 / sumMid2)) * 100.0f);
  }
  else
  {
    mMetrics.stereoSpreadPercent = 0.0f;
  }

  // 4. Estimate 31-band spectrum
  EstimateBandMagnitudes(monoBuffer.data(), numSamples);
}

void JZMasterVisualizer::EstimateBandMagnitudes(const float* mono, int numSamples)
{
  float blockDurationSec = (float)numSamples / mSampleRate;
  float decayAmountDB = mDecaySpeedDBPerSec * blockDurationSec;

  // Bandpass Goertzel / energy approximation for each of the 31 bands
  for (auto& band : mBands)
  {
    float fc = band.centerFreqHz;
    if (fc > mSampleRate * 0.49f)
    {
      band.magnitudeDB = -96.0f;
      continue;
    }

    // Single-frequency discrete Goertzel filter
    float k = 0.5f + ((float)numSamples * fc / mSampleRate);
    float omega = (2.0f * 3.14159265f / (float)numSamples) * k;
    float cosine = std::cos(omega);
    float coeff = 2.0f * cosine;

    float q0 = 0.0f, q1 = 0.0f, q2 = 0.0f;
    for (int i = 0; i < numSamples; ++i)
    {
      q0 = coeff * q1 - q2 + mono[i];
      q2 = q1;
      q1 = q0;
    }

    float magnitude = std::sqrt(std::max(0.0f, q1 * q1 + q2 * q2 - q1 * q2 * coeff)) / (float)numSamples;
    float measuredDB = (magnitude > 1.0e-5f) ? (20.0f * std::log10(magnitude * 2.0f)) : -96.0f;
    measuredDB = std::max(-96.0f, std::min(12.0f, measuredDB));

    // Smooth release ballistics
    if (measuredDB > band.magnitudeDB)
    {
      band.magnitudeDB = measuredDB; // Instant attack
    }
    else
    {
      band.magnitudeDB = std::max(-96.0f, band.magnitudeDB - decayAmountDB);
    }

    // Peak hold with decay
    if (band.magnitudeDB > band.peakHoldDB)
    {
      band.peakHoldDB = band.magnitudeDB;
    }
    else
    {
      band.peakHoldDB = std::max(-96.0f, band.peakHoldDB - (decayAmountDB * 0.5f));
    }
  }
}
