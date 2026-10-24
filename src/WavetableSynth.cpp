//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Modern Wavetable Synthesizer & Stereo Unison Implementation
//*****************************************************************************

#include "WavetableSynth.h"

JZWavetableSynth::JZWavetableSynth(float sampleRate)
  : mSampleRate(sampleRate > 1000.0f ? sampleRate : 44100.0f)
{
  mWavetableData.resize(NUM_FRAMES * FRAME_SIZE);
  GenerateDefaultWavetable();
  Reset();
}

void JZWavetableSynth::SetSampleRate(float sampleRate)
{
  if (sampleRate > 1000.0f)
  {
    mSampleRate = sampleRate;
  }
}

void JZWavetableSynth::SetConfig(const JZWavetableSynthConfig& config)
{
  mConfig = config;
  mConfig.unisonVoices = std::max(1, std::min(16, mConfig.unisonVoices));
}

void JZWavetableSynth::Reset()
{
  for (int v = 0; v < MAX_POLYPHONY; ++v)
  {
    mVoices[v].active = false;
    mVoices[v].envStage = 0;
    mVoices[v].envLevel = 0.0f;
    for (int u = 0; u < 16; ++u) mVoices[v].phase[u] = (double)u / 16.0;
  }
}

void JZWavetableSynth::GenerateDefaultWavetable()
{
  // Generate 64 frames smoothly morphing:
  // Frame 0-15:   Pure Sine -> Triangle
  // Frame 16-31:  Triangle -> Sawtooth
  // Frame 32-47:  Sawtooth -> Pulse / Square
  // Frame 48-63:  Pulse -> Digital Harmonic Texture
  for (int f = 0; f < NUM_FRAMES; ++f)
  {
    float morph = (float)f / (float)(NUM_FRAMES - 1);
    int frameOffset = f * FRAME_SIZE;

    for (int i = 0; i < FRAME_SIZE; ++i)
    {
      float phase = (float)i / (float)FRAME_SIZE;
      float sinVal = std::sin(phase * 6.2831853f);
      float triVal = 4.0f * std::fabs(phase - 0.5f) - 1.0f;
      float sawVal = 2.0f * (phase - std::floor(phase + 0.5f));
      float pulseVal = (phase < 0.5f) ? 1.0f : -1.0f;
      float metallicVal = sinVal * 0.6f + std::sin(phase * 6.2831853f * 3.0f) * 0.3f + std::sin(phase * 6.2831853f * 7.0f) * 0.2f;

      float sample = 0.0f;
      if (morph < 0.25f)
      {
        float t = morph / 0.25f;
        sample = (1.0f - t) * sinVal + t * triVal;
      }
      else if (morph < 0.50f)
      {
        float t = (morph - 0.25f) / 0.25f;
        sample = (1.0f - t) * triVal + t * sawVal;
      }
      else if (morph < 0.75f)
      {
        float t = (morph - 0.50f) / 0.25f;
        sample = (1.0f - t) * sawVal + t * pulseVal;
      }
      else
      {
        float t = (morph - 0.75f) / 0.25f;
        sample = (1.0f - t) * pulseVal + t * metallicVal;
      }

      mWavetableData[frameOffset + i] = sample;
    }
  }
}

float JZWavetableSynth::SampleWavetable(double phase, float framePos) const
{
  while (phase < 0.0) phase += 1.0;
  while (phase >= 1.0) phase -= 1.0;

  float clampedFrame = std::max(0.0f, std::min((float)(NUM_FRAMES - 1), framePos));
  int f0 = (int)clampedFrame;
  int f1 = std::min(NUM_FRAMES - 1, f0 + 1);
  float frameFrac = clampedFrame - (float)f0;

  double samplePos = phase * (double)FRAME_SIZE;
  int s0 = (int)samplePos;
  int s1 = (s0 + 1) % FRAME_SIZE;
  float sampleFrac = (float)(samplePos - (double)s0);

  // Frame 0 interpolation
  float v0_s0 = mWavetableData[f0 * FRAME_SIZE + s0];
  float v0_s1 = mWavetableData[f0 * FRAME_SIZE + s1];
  float val0 = v0_s0 + sampleFrac * (v0_s1 - v0_s0);

  // Frame 1 interpolation
  float v1_s0 = mWavetableData[f1 * FRAME_SIZE + s0];
  float v1_s1 = mWavetableData[f1 * FRAME_SIZE + s1];
  float val1 = v1_s0 + sampleFrac * (v1_s1 - v1_s0);

  // Cross-frame morph
  return val0 + frameFrac * (val1 - val0);
}

int JZWavetableSynth::AllocateVoice()
{
  for (int i = 0; i < MAX_POLYPHONY; ++i)
  {
    if (!mVoices[i].active) return i;
  }
  // Voice stealing: voice in release stage with lowest volume
  int best = 0;
  float minEnv = 999.0f;
  for (int i = 0; i < MAX_POLYPHONY; ++i)
  {
    if (mVoices[i].envStage == 4 && mVoices[i].envLevel < minEnv)
    {
      minEnv = mVoices[i].envLevel;
      best = i;
    }
  }
  return best;
}

void JZWavetableSynth::NoteOn(int note, int velocity)
{
  if (velocity <= 0) { NoteOff(note); return; }

  int v = AllocateVoice();
  auto& voice = mVoices[v];
  voice.active = true;
  voice.note = note;
  voice.velocity = velocity;
  voice.envStage = 1; // Attack
  voice.envLevel = 0.0f;
  voice.filterStateL1 = voice.filterStateL2 = 0.0f;
  voice.filterStateR1 = voice.filterStateR2 = 0.0f;

  for (int u = 0; u < 16; ++u)
  {
    voice.phase[u] = (double)u / 16.0; // phase decorrelation
  }
}

void JZWavetableSynth::NoteOff(int note)
{
  for (int i = 0; i < MAX_POLYPHONY; ++i)
  {
    if (mVoices[i].active && mVoices[i].note == note && mVoices[i].envStage != 4)
    {
      mVoices[i].envStage = 4; // Release
    }
  }
}

void JZWavetableSynth::AllNotesOff()
{
  for (int i = 0; i < MAX_POLYPHONY; ++i)
  {
    mVoices[i].active = false;
    mVoices[i].envStage = 0;
    mVoices[i].envLevel = 0.0f;
  }
}

int JZWavetableSynth::GetActiveVoiceCount() const
{
  int count = 0;
  for (int i = 0; i < MAX_POLYPHONY; ++i) if (mVoices[i].active) count++;
  return count;
}

void JZWavetableSynth::Process(float* outL, float* outR, int numSamples)
{
  if (!outL || !outR || numSamples <= 0) return;

  std::fill(outL, outL + numSamples, 0.0f);
  std::fill(outR, outR + numSamples, 0.0f);

  float masterGain = std::pow(10.0f, mConfig.masterGainDB / 20.0f);
  int uVoices = std::max(1, std::min(16, mConfig.unisonVoices));

  // Envelope rates
  float attStep = 1000.0f / (std::max(1.0f, mConfig.ampAttackMs) * mSampleRate);
  float decStep = 1000.0f / (std::max(1.0f, mConfig.ampDecayMs) * mSampleRate);
  float relStep = 1000.0f / (std::max(1.0f, mConfig.ampReleaseMs) * mSampleRate);

  for (int s = 0; s < numSamples; ++s)
  {
    float mixL = 0.0f;
    float mixR = 0.0f;

    for (int v = 0; v < MAX_POLYPHONY; ++v)
    {
      auto& voice = mVoices[v];
      if (!voice.active) continue;

      // 1. Envelope update
      if (voice.envStage == 1) // Attack
      {
        voice.envLevel += attStep;
        if (voice.envLevel >= 1.0f) { voice.envLevel = 1.0f; voice.envStage = 2; }
      }
      else if (voice.envStage == 2) // Decay
      {
        voice.envLevel -= decStep;
        if (voice.envLevel <= mConfig.ampSustainLevel) { voice.envLevel = mConfig.ampSustainLevel; voice.envStage = 3; }
      }
      else if (voice.envStage == 4) // Release
      {
        voice.envLevel -= relStep;
        if (voice.envLevel <= 0.0001f) { voice.envLevel = 0.0f; voice.active = false; continue; }
      }

      // Base frequency from MIDI note
      float baseFreq = 440.0f * std::pow(2.0f, (voice.note - 69) / 12.0f);
      float voiceOutL = 0.0f;
      float voiceOutR = 0.0f;

      // 2. Unison oscillators loop
      for (int u = 0; u < uVoices; ++u)
      {
        float detuneFactor = (uVoices > 1) ? ((float)u / (float)(uVoices - 1) - 0.5f) * 2.0f : 0.0f;
        float freq = baseFreq * (1.0f + detuneFactor * (mConfig.unisonDetune * 0.05f));
        double phaseInc = (double)freq / (double)mSampleRate;

        float oscSample = SampleWavetable(voice.phase[u], mConfig.wavetablePosition);
        voice.phase[u] += phaseInc;
        if (voice.phase[u] >= 1.0) voice.phase[u] -= 1.0;

        // Unison stereo panning
        float pan = (uVoices > 1) ? detuneFactor * mConfig.unisonStereoSpread : 0.0f;
        float panNorm = 0.5f * (pan + 1.0f);
        voiceOutL += oscSample * std::cos(panNorm * 1.5707963f);
        voiceOutR += oscSample * std::sin(panNorm * 1.5707963f);
      }

      float normUnison = 1.0f / std::sqrt((float)uVoices);
      voiceOutL *= normUnison;
      voiceOutR *= normUnison;

      // 3. Resonant Filter (2-pole Moog/State-Variable approximation)
      float cutoffMod = mConfig.filterCutoffHz * (1.0f + mConfig.filterEnvAmount * voice.envLevel);
      cutoffMod = std::max(20.0f, std::min(mSampleRate * 0.45f, cutoffMod));
      float f = 2.0f * std::sin(3.14159265f * cutoffMod / mSampleRate);
      float q = 1.0f - mConfig.filterResonance * 0.95f;

      voice.filterStateL1 += f * (voiceOutL - voice.filterStateL1 - q * voice.filterStateL2);
      voice.filterStateL2 += f * voice.filterStateL1;
      voice.filterStateR1 += f * (voiceOutR - voice.filterStateR1 - q * voice.filterStateR2);
      voice.filterStateR2 += f * voice.filterStateR1;

      float filtL = (mConfig.filterMode == FILTER_LOWPASS) ? voice.filterStateL2 : (voiceOutL - voice.filterStateL2);
      float filtR = (mConfig.filterMode == FILTER_LOWPASS) ? voice.filterStateR2 : (voiceOutR - voice.filterStateR2);

      float velScale = (float)voice.velocity / 127.0f;
      mixL += filtL * voice.envLevel * velScale;
      mixR += filtR * voice.envLevel * velScale;
    }

    outL[s] = mixL * masterGain;
    outR[s] = mixR * masterGain;
  }
}
