//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Universal Parameter Modulation Matrix & Multi-Shape LFO Implementation
//*****************************************************************************

#include "ModulationMatrix.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>

JZModulationMatrix::JZModulationMatrix(float sampleRate)
  : mSampleRate(sampleRate > 1000.0f ? sampleRate : 44100.0f),
    mProjectBpm(120.0f),
    mEnvelopeFollowerState(0.0f)
{
  mSlots.resize(8);
  for (int i = 0; i < 8; ++i)
  {
    mSlots[i].id = i;
    mSlots[i].shape = static_cast<JZLfoShape>(i % 5);
    mSlots[i].rateHz = 0.5f * static_cast<float>(i + 1);
  }
}

void JZModulationMatrix::SetSampleRate(float sampleRate)
{
  if (sampleRate > 1000.0f)
  {
    mSampleRate = sampleRate;
  }
}

void JZModulationMatrix::SetProjectBpm(float bpm)
{
  mProjectBpm = std::clamp(bpm, 20.0f, 999.0f);
}

void JZModulationMatrix::ConfigureSlot(
  int slotIdx,
  JZLfoShape shape,
  float rateHz,
  bool bpmSync,
  float syncDivision)
{
  if (slotIdx >= 0 && slotIdx < static_cast<int>(mSlots.size()))
  {
    mSlots[slotIdx].shape = shape;
    mSlots[slotIdx].rateHz = std::clamp(rateHz, 0.01f, 100.0f);
    mSlots[slotIdx].bpmSync = bpmSync;
    mSlots[slotIdx].syncDivision = syncDivision > 0.0f ? syncDivision : 1.0f;
  }
}

JZModulatorSlot& JZModulationMatrix::GetSlot(int slotIdx)
{
  static JZModulatorSlot dummy;
  if (slotIdx >= 0 && slotIdx < static_cast<int>(mSlots.size()))
  {
    return mSlots[slotIdx];
  }
  return dummy;
}

void JZModulationMatrix::AddRoute(int sourceModId, int targetParamId, float depth)
{
  mRoutes.emplace_back(sourceModId, targetParamId, std::clamp(depth, -1.0f, 1.0f));
}

void JZModulationMatrix::ClearRoutes()
{
  mRoutes.clear();
}

void JZModulationMatrix::FeedAudioEnvelope(float audioSample)
{
  float envIn = std::abs(audioSample);
  float coeff = 0.99f;
  mEnvelopeFollowerState = coeff * mEnvelopeFollowerState + (1.0f - coeff) * envIn;
}

void JZModulationMatrix::AdvanceSample()
{
  const float twoPi = 6.2831853f;

  for (auto& slot : mSlots)
  {
    float effectiveHz = slot.rateHz;
    if (slot.bpmSync)
    {
      // 1 beat = mProjectBpm / 60 Hz. syncDivision scales beat length.
      effectiveHz = (mProjectBpm / 60.0f) / slot.syncDivision;
    }

    float phaseStep = effectiveHz / mSampleRate;
    slot.phase += phaseStep;
    if (slot.phase >= 1.0f)
    {
      slot.phase -= 1.0f;
      // Trigger new random value for S&H on phase cycle wrap
      slot.lastRandomVal = (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX)) * 2.0f - 1.0f;
    }

    switch (slot.shape)
    {
      case JZLfoShape::Sine:
        slot.currentOutput = std::sin(slot.phase * twoPi);
        break;

      case JZLfoShape::Triangle:
        if (slot.phase < 0.25f)
        {
          slot.currentOutput = slot.phase * 4.0f;
        }
        else if (slot.phase < 0.75f)
        {
          slot.currentOutput = 1.0f - (slot.phase - 0.25f) * 4.0f;
        }
        else
        {
          slot.currentOutput = -1.0f + (slot.phase - 0.75f) * 4.0f;
        }
        break;

      case JZLfoShape::SawUp:
        slot.currentOutput = 2.0f * slot.phase - 1.0f;
        break;

      case JZLfoShape::SawDown:
        slot.currentOutput = 1.0f - 2.0f * slot.phase;
        break;

      case JZLfoShape::Square:
        slot.currentOutput = slot.phase < 0.5f ? 1.0f : -1.0f;
        break;

      case JZLfoShape::RandomSampleAndHold:
        slot.currentOutput = slot.lastRandomVal;
        break;

      case JZLfoShape::EnvelopeFollower:
        slot.currentOutput = std::clamp(mEnvelopeFollowerState * 2.0f - 1.0f, -1.0f, 1.0f);
        break;

      case JZLfoShape::ChaosWander:
        slot.currentOutput = 0.7f * std::sin(slot.phase * twoPi) + 0.3f * std::sin(slot.phase * twoPi * 2.37f);
        break;
    }
  }
}

float JZModulationMatrix::GetModulatedValue(int targetParamId, float baseNormalizedValue) const
{
  float accumulatedMod = 0.0f;

  for (const auto& route : mRoutes)
  {
    if (route.targetParamId == targetParamId)
    {
      if (route.sourceModulatorId >= 0 && route.sourceModulatorId < static_cast<int>(mSlots.size()))
      {
        float modOut = mSlots[route.sourceModulatorId].currentOutput;
        accumulatedMod += modOut * route.bipolarDepth;
      }
    }
  }

  // Modulate normalized value around base point and clamp [0.0, 1.0]
  return std::clamp(baseNormalizedValue + accumulatedMod, 0.0f, 1.0f);
}
