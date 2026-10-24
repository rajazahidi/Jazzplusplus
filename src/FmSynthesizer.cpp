//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// 4-Operator Frequency Modulation (FM) Synthesizer Implementation
//*****************************************************************************

#include "FmSynthesizer.h"

JZFmSynthesizer::JZFmSynthesizer(float sampleRate)
  : mSampleRate(sampleRate > 1000.0f ? sampleRate : 44100.0f)
{
  Reset();
}

void JZFmSynthesizer::SetSampleRate(float sampleRate)
{
  if (sampleRate > 1000.0f)
  {
    mSampleRate = sampleRate;
  }
}

void JZFmSynthesizer::SetConfig(const JZFmSynthConfig& config)
{
  mConfig = config;
  mConfig.routingAlgorithm = std::max(1, std::min(8, mConfig.routingAlgorithm));
}

void JZFmSynthesizer::Reset()
{
  for (int v = 0; v < MAX_POLYPHONY; ++v)
  {
    mVoices[v].active = false;
    mVoices[v].prevOp4Out = 0.0f;
    for (int i = 0; i < 4; ++i)
    {
      mVoices[v].phase[i] = 0.0;
      mVoices[v].envLevel[i] = 0.0f;
      mVoices[v].envStage[i] = 0;
    }
  }
}

int JZFmSynthesizer::AllocateVoice()
{
  for (int i = 0; i < MAX_POLYPHONY; ++i)
  {
    if (!mVoices[i].active) return i;
  }
  // Voice stealing: voice with lowest carrier envelope
  int best = 0;
  float minEnv = 999.0f;
  for (int i = 0; i < MAX_POLYPHONY; ++i)
  {
    if (mVoices[i].envStage[0] == 4 && mVoices[i].envLevel[0] < minEnv)
    {
      minEnv = mVoices[i].envLevel[0];
      best = i;
    }
  }
  return best;
}

void JZFmSynthesizer::NoteOn(int note, int velocity)
{
  if (velocity <= 0) { NoteOff(note); return; }

  int v = AllocateVoice();
  auto& voice = mVoices[v];
  voice.active = true;
  voice.note = note;
  voice.velocity = velocity;
  voice.prevOp4Out = 0.0f;

  for (int i = 0; i < 4; ++i)
  {
    voice.phase[i] = 0.0;
    voice.envLevel[i] = 0.0f;
    voice.envStage[i] = 1; // Attack
  }
}

void JZFmSynthesizer::NoteOff(int note)
{
  for (int i = 0; i < MAX_POLYPHONY; ++i)
  {
    if (mVoices[i].active && mVoices[i].note == note)
    {
      for (int op = 0; op < 4; ++op)
      {
        if (mVoices[i].envStage[op] != 4)
        {
          mVoices[i].envStage[op] = 4; // Release
        }
      }
    }
  }
}

void JZFmSynthesizer::AllNotesOff()
{
  for (int i = 0; i < MAX_POLYPHONY; ++i)
  {
    mVoices[i].active = false;
    for (int op = 0; op < 4; ++op)
    {
      mVoices[i].envStage[op] = 0;
      mVoices[i].envLevel[op] = 0.0f;
    }
  }
}

int JZFmSynthesizer::GetActiveVoiceCount() const
{
  int count = 0;
  for (int i = 0; i < MAX_POLYPHONY; ++i) if (mVoices[i].active) count++;
  return count;
}

float JZFmSynthesizer::ComputeVoiceSample(JZFmVoice& voice, float baseFreq)
{
  const float twoPi = 6.28318530717958647692f;
  float opOutputs[4];

  // 1. Update Envelopes and Advance Phases for all 4 operators
  for (int op = 0; op < 4; ++op)
  {
    const auto& cfg = mConfig.ops[op];
    float attStep = 1000.0f / (std::max(0.5f, cfg.attackMs) * mSampleRate);
    float decStep = 1000.0f / (std::max(1.0f, cfg.decayMs) * mSampleRate);
    float relStep = 1000.0f / (std::max(1.0f, cfg.releaseMs) * mSampleRate);

    if (voice.envStage[op] == 1) // Attack
    {
      voice.envLevel[op] += attStep;
      if (voice.envLevel[op] >= 1.0f) { voice.envLevel[op] = 1.0f; voice.envStage[op] = 2; }
    }
    else if (voice.envStage[op] == 2) // Decay
    {
      voice.envLevel[op] -= decStep;
      if (voice.envLevel[op] <= cfg.sustainLevel) { voice.envLevel[op] = cfg.sustainLevel; voice.envStage[op] = 3; }
    }
    else if (voice.envStage[op] == 4) // Release
    {
      voice.envLevel[op] -= relStep;
      if (voice.envLevel[op] <= 0.0001f) { voice.envLevel[op] = 0.0f; }
    }

    float freq = (baseFreq * cfg.ratio) + cfg.detuneHz;
    voice.phase[op] += (double)freq / (double)mSampleRate;
    if (voice.phase[op] >= 1.0) voice.phase[op] -= 1.0;
  }

  // Deactivate voice if carrier envelope is fully released
  if (voice.envStage[0] == 4 && voice.envLevel[0] <= 0.0001f)
  {
    voice.active = false;
    return 0.0f;
  }

  // 2. Compute Operator 4 with Self-Feedback
  float fbMod = voice.prevOp4Out * mConfig.ops[3].feedback * 3.14159f;
  float op4ModPhase = (float)(voice.phase[3] * twoPi) + fbMod;
  float op4Raw = std::sin(op4ModPhase);
  voice.prevOp4Out = op4Raw;
  opOutputs[3] = op4Raw * mConfig.ops[3].outputLevel * voice.envLevel[3];

  // 3. Routing Algorithms (Modulation matrix)
  int alg = mConfig.routingAlgorithm;
  float finalCarrierOut = 0.0f;

  if (alg == 1) // 4 -> 3 -> 2 -> 1
  {
    float mod3 = opOutputs[3] * 4.0f;
    opOutputs[2] = std::sin((float)(voice.phase[2] * twoPi) + mod3) * mConfig.ops[2].outputLevel * voice.envLevel[2];
    float mod2 = opOutputs[2] * 4.0f;
    opOutputs[1] = std::sin((float)(voice.phase[1] * twoPi) + mod2) * mConfig.ops[1].outputLevel * voice.envLevel[1];
    float mod1 = opOutputs[1] * 4.0f;
    finalCarrierOut = std::sin((float)(voice.phase[0] * twoPi) + mod1) * mConfig.ops[0].outputLevel * voice.envLevel[0];
  }
  else if (alg == 3) // (4 -> 3) + (2 -> 1)
  {
    float mod3 = opOutputs[3] * 4.0f;
    opOutputs[2] = std::sin((float)(voice.phase[2] * twoPi) + mod3) * mConfig.ops[2].outputLevel * voice.envLevel[2];
    float mod1 = opOutputs[1] * 4.0f;
    float car1 = std::sin((float)(voice.phase[0] * twoPi) + mod1) * mConfig.ops[0].outputLevel * voice.envLevel[0];
    finalCarrierOut = (car1 + opOutputs[2]) * 0.5f;
  }
  else if (alg == 8) // All parallel carriers (Additive)
  {
    for (int op = 0; op < 3; ++op)
    {
      opOutputs[op] = std::sin((float)(voice.phase[op] * twoPi)) * mConfig.ops[op].outputLevel * voice.envLevel[op];
    }
    finalCarrierOut = (opOutputs[0] + opOutputs[1] + opOutputs[2] + opOutputs[3]) * 0.25f;
  }
  else // Standard 2-modulator to 2-carrier (Alg 5 default)
  {
    float mod3 = opOutputs[3] * 4.0f;
    opOutputs[2] = std::sin((float)(voice.phase[2] * twoPi) + mod3) * mConfig.ops[2].outputLevel * voice.envLevel[2];
    float mod1 = opOutputs[2] * 3.0f;
    float car1 = std::sin((float)(voice.phase[0] * twoPi) + mod1) * mConfig.ops[0].outputLevel * voice.envLevel[0];
    float car2 = std::sin((float)(voice.phase[1] * twoPi)) * mConfig.ops[1].outputLevel * voice.envLevel[1];
    finalCarrierOut = (car1 * 0.7f + car2 * 0.3f);
  }

  float velScale = (float)voice.velocity / 127.0f;
  return finalCarrierOut * velScale;
}

void JZFmSynthesizer::Process(float* outL, float* outR, int numSamples)
{
  if (!outL || !outR || numSamples <= 0) return;

  std::fill(outL, outL + numSamples, 0.0f);
  std::fill(outR, outR + numSamples, 0.0f);

  float masterGain = std::pow(10.0f, mConfig.masterGainDB / 20.0f);

  for (int s = 0; s < numSamples; ++s)
  {
    float mix = 0.0f;

    for (int v = 0; v < MAX_POLYPHONY; ++v)
    {
      auto& voice = mVoices[v];
      if (!voice.active) continue;

      float baseFreq = 440.0f * std::pow(2.0f, (voice.note - 69) / 12.0f);
      mix += ComputeVoiceSample(voice, baseFreq);
    }

    float out = mix * masterGain;
    outL[s] = out;
    outR[s] = out;
  }
}

void JZFmSynthesizer::LoadPreset(int presetId)
{
  Reset();

  switch (presetId)
  {
    case 1: // Lately FM Bass
      mConfig.routingAlgorithm = 1;
      mConfig.ops[0] = JZFmOperatorConfig(1.0f, 1.0f, 1.0f, 180.0f, 0.2f, 100.0f); // Carrier
      mConfig.ops[1] = JZFmOperatorConfig(1.0f, 0.85f, 1.0f, 90.0f, 0.0f, 50.0f);
      mConfig.ops[2] = JZFmOperatorConfig(2.0f, 0.65f, 1.0f, 60.0f, 0.0f, 40.0f);
      mConfig.ops[3] = JZFmOperatorConfig(4.0f, 0.40f, 1.0f, 40.0f, 0.0f, 30.0f);
      mConfig.ops[3].feedback = 0.45f;
      break;

    case 2: // Crystal Bell
      mConfig.routingAlgorithm = 3;
      mConfig.ops[0] = JZFmOperatorConfig(1.0f, 0.8f, 2.0f, 1200.0f, 0.0f, 800.0f);
      mConfig.ops[1] = JZFmOperatorConfig(3.5f, 0.7f, 2.0f, 400.0f, 0.0f, 300.0f);
      mConfig.ops[2] = JZFmOperatorConfig(7.0f, 0.5f, 1.0f, 200.0f, 0.0f, 150.0f);
      mConfig.ops[3] = JZFmOperatorConfig(11.0f, 0.3f, 1.0f, 100.0f, 0.0f, 80.0f);
      mConfig.ops[3].feedback = 0.15f;
      break;

    case 0: // DX7 E-Piano
    default:
      mConfig = JZFmSynthConfig();
      break;
  }
}
