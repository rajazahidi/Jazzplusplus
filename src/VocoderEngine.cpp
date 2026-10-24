//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Dynamic 24-Band Vocoder & Robotic Speech Implementation
//*****************************************************************************

#include "VocoderEngine.h"

JZVocoderEngine::JZVocoderEngine(float sampleRate)
  : mSampleRate(sampleRate > 1000.0f ? sampleRate : 44100.0f),
    mCarrierMidiNote(48), // C3
    mCarrierPhase(0.0),
    mAlphaAttack(0.0f),
    mAlphaRelease(0.0f)
{
  for (int b = 0; b < NUM_BANDS; ++b) mBandEnvelopes[b] = 0.0f;
  InitializeFilterbanks();
  UpdateCoefficients();
}

void JZVocoderEngine::SetSampleRate(float sampleRate)
{
  if (sampleRate > 1000.0f)
  {
    mSampleRate = sampleRate;
    InitializeFilterbanks();
    UpdateCoefficients();
  }
}

void JZVocoderEngine::SetConfig(const JZVocoderConfig& config)
{
  mConfig = config;
  UpdateCoefficients();
}

void JZVocoderEngine::Reset()
{
  for (int b = 0; b < NUM_BANDS; ++b)
  {
    mAnalysisFilters[b].Reset();
    mSynthesisFiltersL[b].Reset();
    mSynthesisFiltersR[b].Reset();
    mBandEnvelopes[b] = 0.0f;
  }
}

void JZVocoderEngine::SetCarrierPitch(int midiNote)
{
  mCarrierMidiNote = std::max(24, std::min(96, midiNote));
}

void JZVocoderEngine::InitializeFilterbanks()
{
  // 24 logarithmic bandpass frequencies (100 Hz to 8000 Hz)
  float fMin = 100.0f;
  float fMax = std::min(mSampleRate * 0.42f, 8000.0f);
  float logMin = std::log10(fMin);
  float logMax = std::log10(fMax);
  float Q = 8.0f; // Narrow resonant bandpass for distinct vocoder formants

  for (int b = 0; b < NUM_BANDS; ++b)
  {
    float fCenter = std::pow(10.0f, logMin + (logMax - logMin) * ((float)b / (float)(NUM_BANDS - 1)));
    float w0 = 2.0f * 3.14159265f * fCenter / mSampleRate;
    float alpha = std::sin(w0) / (2.0f * Q);

    float a0 = 1.0f + alpha;
    float b0 = (alpha * 1.5f) / a0;
    float b1 = 0.0f;
    float b2 = (-alpha * 1.5f) / a0;
    float a1 = (-2.0f * std::cos(w0)) / a0;
    float a2 = (1.0f - alpha) / a0;

    auto setFilter = [&](JZVocoderBandFilter& filt) {
      filt.b0 = b0; filt.b1 = b1; filt.b2 = b2;
      filt.a1 = a1; filt.a2 = a2;
    };

    setFilter(mAnalysisFilters[b]);
    setFilter(mSynthesisFiltersL[b]);
    setFilter(mSynthesisFiltersR[b]);
  }
}

void JZVocoderEngine::UpdateCoefficients()
{
  float attSec = std::max(0.0005f, mConfig.bandAttackMs * 0.001f);
  float relSec = std::max(0.005f, mConfig.bandReleaseMs * 0.001f);
  mAlphaAttack = std::exp(-1.0f / (mSampleRate * attSec));
  mAlphaRelease = std::exp(-1.0f / (mSampleRate * relSec));
}

void JZVocoderEngine::Process(
  const float* modulatorMono,
  float* outL,
  float* outR,
  int numSamples,
  const float* optCarrierMono)
{
  if (!modulatorMono || !outL || !outR || numSamples <= 0) return;

  float carrierFreq = 440.0f * std::pow(2.0f, (mCarrierMidiNote - 69) / 12.0f);
  double phaseInc = (double)carrierFreq / (double)mSampleRate;

  for (int i = 0; i < numSamples; ++i)
  {
    float modSample = modulatorMono[i];

    // 1. Generate or fetch carrier sample
    float rawCarrier = 0.0f;
    if (mConfig.carrierMode == VOCODER_CARRIER_EXTERNAL && optCarrierMono)
    {
      rawCarrier = optCarrierMono[i];
    }
    else if (mConfig.carrierMode == VOCODER_CARRIER_INTERNAL_PULSE)
    {
      rawCarrier = (mCarrierPhase < 0.5) ? 0.7f : -0.7f;
      mCarrierPhase += phaseInc;
      if (mCarrierPhase >= 1.0) mCarrierPhase -= 1.0;
    }
    else // VOCODER_CARRIER_INTERNAL_SAW
    {
      // Rich dual saw wave
      float saw1 = 2.0f * (float)(mCarrierPhase - std::floor(mCarrierPhase + 0.5));
      float saw2 = 2.0f * (float)((mCarrierPhase * 1.5) - std::floor(mCarrierPhase * 1.5 + 0.5));
      rawCarrier = (saw1 * 0.6f + saw2 * 0.4f);
      mCarrierPhase += phaseInc;
      if (mCarrierPhase >= 1.0) mCarrierPhase -= 1.0;
    }

    float vocodedSumL = 0.0f;
    float vocodedSumR = 0.0f;

    // 2. Process through 24-band analysis & synthesis filterbank
    for (int b = 0; b < NUM_BANDS; ++b)
    {
      // Analysis: filter speech modulator
      float modBand = mAnalysisFilters[b].Process(modSample);
      float bandLevel = std::fabs(modBand);

      // Envelope follower ballistics
      if (bandLevel > mBandEnvelopes[b])
      {
        mBandEnvelopes[b] = mAlphaAttack * mBandEnvelopes[b] + (1.0f - mAlphaAttack) * bandLevel;
      }
      else
      {
        mBandEnvelopes[b] = mAlphaRelease * mBandEnvelopes[b] + (1.0f - mAlphaRelease) * bandLevel;
      }

      // Synthesis: filter carrier and modulate with speech envelope
      int synthBandIdx = b + mConfig.formantShiftBands;
      if (synthBandIdx >= 0 && synthBandIdx < NUM_BANDS)
      {
        float carBandL = mSynthesisFiltersL[synthBandIdx].Process(rawCarrier);
        float carBandR = mSynthesisFiltersR[synthBandIdx].Process(rawCarrier);

        float gain = mBandEnvelopes[b] * 3.5f;
        vocodedSumL += carBandL * gain;
        vocodedSumR += carBandR * gain;
      }
    }

    // 3. Unvoiced sibilance pass-through (adds crispness to 's', 't' speech consonants)
    if (mConfig.unvoicedHighPassGain > 0.001f)
    {
      float sibilance = (modSample > 0.05f || modSample < -0.05f) ? modSample * mConfig.unvoicedHighPassGain : 0.0f;
      vocodedSumL += sibilance;
      vocodedSumR += sibilance;
    }

    // 4. Dry/Wet blend
    outL[i] = (modSample * (1.0f - mConfig.dryWet)) + (vocodedSumL * mConfig.dryWet);
    outR[i] = (modSample * (1.0f - mConfig.dryWet)) + (vocodedSumR * mConfig.dryWet);
  }
}
