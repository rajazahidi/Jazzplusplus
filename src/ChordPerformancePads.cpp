//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Performance Chord Trigger Pads & Voicing Engine Implementation
//*****************************************************************************

#include "ChordPerformancePads.h"

JZChordPerformancePads::JZChordPerformancePads()
  : mVoicingMode(VOICING_ROOT_POSITION),
    mStrumSpreadTicks(0)
{
  LoadPreset(PRESET_NEO_SOUL_JAZZ);
}

void JZChordPerformancePads::SetPad(int padIndex, const JZChordPadDefinition& pad)
{
  if (padIndex >= 0 && padIndex < NUM_PADS)
  {
    mPads[padIndex] = pad;
  }
}

const JZChordPadDefinition& JZChordPerformancePads::GetPad(int padIndex) const
{
  static JZChordPadDefinition sEmpty;
  if (padIndex >= 0 && padIndex < NUM_PADS)
  {
    return mPads[padIndex];
  }
  return sEmpty;
}

JZChordPadDefinition& JZChordPerformancePads::GetPad(int padIndex)
{
  static JZChordPadDefinition sEmpty;
  if (padIndex >= 0 && padIndex < NUM_PADS)
  {
    return mPads[padIndex];
  }
  return sEmpty;
}

std::vector<int> JZChordPerformancePads::ApplyVoicing(const std::vector<int>& rawPitches, JZChordVoicingMode mode)
{
  if (rawPitches.size() <= 2 || mode == VOICING_ROOT_POSITION)
  {
    return rawPitches;
  }

  std::vector<int> sorted = rawPitches;
  std::sort(sorted.begin(), sorted.end());

  if (mode == VOICING_DROP_2 && sorted.size() >= 4)
  {
    // Drop 2nd highest note down by 1 octave (-12 semitones)
    int n = (int)sorted.size();
    sorted[n - 2] -= 12;
    std::sort(sorted.begin(), sorted.end());
    return sorted;
  }
  else if (mode == VOICING_OPEN_SPREAD)
  {
    // Spread lowest bass note down 12, highest voice up 12
    sorted.front() -= 12;
    sorted.back() += 12;
    std::sort(sorted.begin(), sorted.end());
    return sorted;
  }

  return sorted;
}

bool JZChordPerformancePads::TriggerPad(int padIndex, int baseVelocity, std::vector<JZMatrixMidiNote>& outNotes) const
{
  if (padIndex < 0 || padIndex >= NUM_PADS) return false;

  const auto& pad = mPads[padIndex];
  if (pad.pitches.empty()) return false;

  std::vector<int> voicedPitches = ApplyVoicing(pad.pitches, mVoicingMode);
  int vel = std::max(1, std::min(127, baseVelocity));

  for (size_t i = 0; i < voicedPitches.size(); ++i)
  {
    int pitch = voicedPitches[i];
    int tickOffset = (int)i * mStrumSpreadTicks;
    int noteVel = std::max(1, std::min(127, vel + (int)(i % 2 == 0 ? 3 : -3))); // subtle velocity humanize

    outNotes.emplace_back(pitch, noteVel, tickOffset, 384);
  }

  return true;
}

void JZChordPerformancePads::LoadPreset(JZChordPadPreset preset)
{
  switch (preset)
  {
    case PRESET_NEO_SOUL_JAZZ:
      // Lush Jazz & Neo-Soul 9th/11th chords
      mPads[0]  = JZChordPadDefinition("Cmaj9", 60, {48, 60, 64, 67, 71, 74});
      mPads[1]  = JZChordPadDefinition("Dm11", 62, {50, 62, 65, 69, 72, 77});
      mPads[2]  = JZChordPadDefinition("Em9", 64, {52, 64, 67, 71, 74, 78});
      mPads[3]  = JZChordPadDefinition("Fmaj9#11", 65, {53, 65, 69, 72, 76, 78});
      mPads[4]  = JZChordPadDefinition("G13", 67, {55, 67, 71, 75, 77, 81});
      mPads[5]  = JZChordPadDefinition("Am11", 69, {57, 69, 72, 76, 79, 82});
      mPads[6]  = JZChordPadDefinition("Bm7b5", 71, {59, 71, 74, 77, 81});
      mPads[7]  = JZChordPadDefinition("E7#9 (Hendrix)", 64, {52, 64, 68, 71, 74, 79});
      mPads[8]  = JZChordPadDefinition("Abmaj7", 56, {44, 56, 60, 63, 67});
      mPads[9]  = JZChordPadDefinition("Bb13sus", 58, {46, 58, 63, 66, 68, 72});
      mPads[10] = JZChordPadDefinition("Ebmaj9", 63, {51, 63, 67, 70, 74, 77});
      mPads[11] = JZChordPadDefinition("Dbmaj7#11", 61, {49, 61, 65, 68, 72, 75});
      mPads[12] = JZChordPadDefinition("Gm9", 67, {55, 67, 70, 74, 77, 81});
      mPads[13] = JZChordPadDefinition("C7alt", 60, {48, 60, 64, 66, 70, 75});
      mPads[14] = JZChordPadDefinition("Fm11", 65, {53, 65, 68, 72, 75, 79});
      mPads[15] = JZChordPadDefinition("Bb7b13", 58, {46, 58, 62, 66, 68, 74});
      break;

    case PRESET_LOFI_CHILLHOP:
      mPads[0]  = JZChordPadDefinition("Dm9", 62, {50, 62, 65, 69, 72, 76});
      mPads[1]  = JZChordPadDefinition("G13", 67, {55, 67, 71, 75, 77, 81});
      mPads[2]  = JZChordPadDefinition("Cmaj7", 60, {48, 60, 64, 67, 71});
      mPads[3]  = JZChordPadDefinition("Am9", 69, {57, 69, 72, 76, 79, 83});
      mPads[4]  = JZChordPadDefinition("Fmaj7", 65, {53, 65, 69, 72, 76});
      mPads[5]  = JZChordPadDefinition("Em7", 64, {52, 64, 67, 71, 74});
      mPads[6]  = JZChordPadDefinition("Bbmaj7", 58, {46, 58, 62, 65, 69});
      mPads[7]  = JZChordPadDefinition("A7b9", 69, {57, 69, 73, 76, 79, 82});
      for (int i = 8; i < 16; ++i) mPads[i] = mPads[i - 8];
      break;

    case PRESET_POP_EDM_ANTHEM:
    default:
      mPads[0]  = JZChordPadDefinition("C Major", 60, {48, 60, 64, 67});
      mPads[1]  = JZChordPadDefinition("G Major", 67, {55, 67, 71, 74});
      mPads[2]  = JZChordPadDefinition("A Minor", 69, {57, 69, 72, 76});
      mPads[3]  = JZChordPadDefinition("F Major", 65, {53, 65, 69, 72});
      mPads[4]  = JZChordPadDefinition("D Minor", 62, {50, 62, 65, 69});
      mPads[5]  = JZChordPadDefinition("E Minor", 64, {52, 64, 67, 71});
      mPads[6]  = JZChordPadDefinition("Bb Major", 58, {46, 58, 62, 65});
      mPads[7]  = JZChordPadDefinition("Ab Major", 56, {44, 56, 60, 63});
      for (int i = 8; i < 16; ++i) mPads[i] = mPads[i - 8];
      break;
  }
}
