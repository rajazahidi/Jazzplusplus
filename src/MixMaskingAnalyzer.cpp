//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Intelligent Cross-Track Frequency Masking Analyzer Implementation
//*****************************************************************************

#include "MixMaskingAnalyzer.h"

JZMixMaskingAnalyzer::JZMixMaskingAnalyzer(float sampleRate)
  : mSampleRate(sampleRate > 1000.0f ? sampleRate : 44100.0f)
{
}

void JZMixMaskingAnalyzer::SetSampleRate(float sampleRate)
{
  if (sampleRate > 1000.0f)
  {
    mSampleRate = sampleRate;
  }
}

void JZMixMaskingAnalyzer::Reset()
{
  for (auto& pair : mTracks)
  {
    for (int b = 0; b < NUM_ANALYSIS_BANDS; ++b)
    {
      pair.second.bandEnergiesDB[b] = -96.0f;
    }
  }
}

void JZMixMaskingAnalyzer::RegisterTrack(int trackId, const std::string& name, int priority)
{
  JZTrackSpectrum spec;
  spec.trackId = trackId;
  spec.trackName = name;
  spec.priorityLevel = std::max(1, std::min(4, priority));
  mTracks[trackId] = spec;
}

void JZMixMaskingAnalyzer::RemoveTrack(int trackId)
{
  mTracks.erase(trackId);
}

float JZMixMaskingAnalyzer::BandIndexToFreq(int bandIndex, float sampleRate)
{
  float fMin = 30.0f;
  float fMax = std::min(sampleRate * 0.45f, 18000.0f);
  float logMin = std::log10(fMin);
  float logMax = std::log10(fMax);
  return std::pow(10.0f, logMin + (logMax - logMin) * ((float)bandIndex / (float)(NUM_ANALYSIS_BANDS - 1)));
}

void JZMixMaskingAnalyzer::FeedAudioBlock(int trackId, const float* audio, int numSamples)
{
  auto it = mTracks.find(trackId);
  if (it == mTracks.end() || !audio || numSamples <= 0) return;

  auto& spec = it->second;

  // Discrete band energy estimation using Goertzel binned algorithm
  for (int b = 0; b < NUM_ANALYSIS_BANDS; ++b)
  {
    float fc = BandIndexToFreq(b, mSampleRate);
    float k = (float)numSamples * fc / mSampleRate;
    float omega = 2.0f * 3.14159265f * k / (float)numSamples;
    float coeff = 2.0f * std::cos(omega);

    float q0 = 0.0f, q1 = 0.0f, q2 = 0.0f;
    for (int i = 0; i < numSamples; ++i)
    {
      q0 = coeff * q1 - q2 + audio[i];
      q2 = q1;
      q1 = q0;
    }

    float mag = std::sqrt(std::max(0.0f, q1 * q1 + q2 * q2 - q1 * q2 * coeff)) / (float)numSamples;
    float db = (mag > 1e-5f) ? (20.0f * std::log10(mag * 2.0f)) : -96.0f;

    // Smooth update (instant attack, slow decay)
    if (db > spec.bandEnergiesDB[b])
    {
      spec.bandEnergiesDB[b] = db;
    }
    else
    {
      spec.bandEnergiesDB[b] = std::max(-96.0f, spec.bandEnergiesDB[b] - 0.75f);
    }
  }
}

std::vector<JZMaskingClash> JZMixMaskingAnalyzer::AnalyzeClashes(float minSeverityThreshold)
{
  std::vector<JZMaskingClash> clashes;

  // Compare every track against other tracks
  for (const auto& pairA : mTracks)
  {
    const auto& trackA = pairA.second;

    for (const auto& pairB : mTracks)
    {
      const auto& trackB = pairB.second;
      if (trackA.trackId >= trackB.trackId) continue;

      // Determine priority: track with lower numerical priority has higher precedence (e.g. 1 = Vocal over 3 = Guitars)
      const JZTrackSpectrum* pImportant = &trackA;
      const JZTrackSpectrum* pMasker = &trackB;

      if (trackB.priorityLevel < trackA.priorityLevel)
      {
        pImportant = &trackB;
        pMasker = &trackA;
      }

      // Check for band collisions where the competing masker track overshadows important track
      for (int b = 0; b < NUM_ANALYSIS_BANDS; ++b)
      {
        float dbLead = pImportant->bandEnergiesDB[b];
        float dbMask = pMasker->bandEnergiesDB[b];

        // Only active if both tracks are producing audible sound in this band
        if (dbLead > -48.0f && dbMask > -48.0f)
        {
          float overlapDiff = dbMask - dbLead; // Positive if masker is louder than lead track
          if (overlapDiff > -3.0f) // Competing within 3dB or louder
          {
            float severity = std::min(100.0f, std::max(0.0f, (overlapDiff + 12.0f) * 4.5f));

            if (severity >= minSeverityThreshold)
            {
              JZMaskingClash clash;
              clash.maskedTrackId = pImportant->trackId;
              clash.maskedTrackName = pImportant->trackName;
              clash.maskerTrackId = pMasker->trackId;
              clash.maskerTrackName = pMasker->trackName;
              clash.clashFreqHz = BandIndexToFreq(b, mSampleRate);
              clash.severityPercent = severity;
              clash.recommendedCutDB = -std::min(6.0f, std::max(1.5f, overlapDiff + 2.0f));
              clash.advice = "Apply dynamic EQ notch cut on " + pMasker->trackName + " around " +
                             std::to_string((int)clash.clashFreqHz) + " Hz to unmask " + pImportant->trackName;
              clashes.push_back(clash);
            }
          }
        }
      }
    }
  }

  // Sort by severity descending
  std::sort(clashes.begin(), clashes.end(), [](const JZMaskingClash& a, const JZMaskingClash& b) {
    return a.severityPercent > b.severityPercent;
  });

  return clashes;
}
