//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Groove Pool & Advanced Humanize Groove Templates Implementation
//*****************************************************************************

#include "GroovePool.h"
#include "Track.h"
#include "Events.h"

#include <cmath>
#include <algorithm>

using namespace std;

//-----------------------------------------------------------------------------
// Singleton Instance
//-----------------------------------------------------------------------------
JZGroovePool& JZGroovePool::Instance()
{
  static JZGroovePool sInstance;
  return sInstance;
}

JZGroovePool::JZGroovePool()
{
  InitializePresets();
}

void JZGroovePool::InitializePresets()
{
  mTemplates.clear();

  // 1. Roger Linn MPC-60 Classic 16th Swing (62%)
  {
    JZGrooveTemplate mpc62("Roger Linn MPC-60 Classic Swing (62%)", "MPC Swing", 16);
    // In 96 TPQ, a quarter note is 96 ticks, a 16th is 24 ticks.
    // 50% = 12 ticks; 62% swing pushes the offbeat 16th by approx +6 ticks.
    for (int i = 0; i < 16; ++i)
    {
      bool isOffbeat = (i % 2 == 1);
      int offset = isOffbeat ? 6 : 0;
      float vel = (i % 4 == 0) ? 1.15f : (isOffbeat ? 0.90f : 1.0f);
      mpc62.steps.push_back(JZGrooveStep(offset, vel));
    }
    mTemplates.push_back(mpc62);
  }

  // 2. Akai MPC-3000 Hip-Hop Hard Swing (66%)
  {
    JZGrooveTemplate mpc66("Akai MPC-3000 Hip-Hop Hard Swing (66%)", "MPC Swing", 16);
    for (int i = 0; i < 16; ++i)
    {
      bool isOffbeat = (i % 2 == 1);
      int offset = isOffbeat ? 8 : -1;
      float vel = (i % 4 == 0) ? 1.20f : (isOffbeat ? 0.88f : 0.95f);
      mpc66.steps.push_back(JZGrooveStep(offset, vel));
    }
    mTemplates.push_back(mpc66);
  }

  // 3. J Dilla / D'Angelo Drunken Swing (Laid-Back Behind The Beat)
  {
    JZGrooveTemplate dilla("J Dilla & Questlove Drunken Loose Swing", "Neo-Soul & Hip-Hop", 16);
    static const int dillaOffsets[16] = {0, 9, 3, 11, -2, 8, 4, 12, 1, 10, 2, 9, -1, 7, 3, 10};
    static const float dillaVels[16] = {1.18f, 0.82f, 0.96f, 0.88f, 1.22f, 0.85f, 0.94f, 0.89f,
                                        1.15f, 0.80f, 0.98f, 0.86f, 1.20f, 0.84f, 0.92f, 0.87f};
    for (int i = 0; i < 16; ++i)
    {
      dilla.steps.push_back(JZGrooveStep(dillaOffsets[i], dillaVels[i]));
    }
    mTemplates.push_back(dilla);
  }

  // 4. James Brown & Clyde Stubblefield Funk Pocket (Tight Ghost Syncopation)
  {
    JZGrooveTemplate funk("Clyde Stubblefield Funky Drummer Pocket", "Funk Pocket", 16);
    static const int funkOffsets[16] = {0, -3, 2, -2, 1, -4, 3, -1, 0, -2, 2, -3, 1, -4, 2, -1};
    static const float funkVels[16] = {1.25f, 0.70f, 0.95f, 0.72f, 1.20f, 0.68f, 1.05f, 0.74f,
                                       1.22f, 0.69f, 0.98f, 0.75f, 1.18f, 0.70f, 1.02f, 0.71f};
    for (int i = 0; i < 16; ++i)
    {
      funk.steps.push_back(JZGrooveStep(funkOffsets[i], funkVels[i]));
    }
    mTemplates.push_back(funk);
  }

  // 5. Afrobeat Tony Allen & Fela Kuti Polyrhythmic Push
  {
    JZGrooveTemplate afro("Fela Kuti & Tony Allen Afrobeat Push", "Afrobeat", 16);
    static const int afroOffsets[16] = {-2, 4, -1, 5, -3, 3, 0, 6, -2, 4, -1, 5, -3, 2, 1, 4};
    static const float afroVels[16] = {1.15f, 0.95f, 1.05f, 0.90f, 1.12f, 0.98f, 1.10f, 0.88f,
                                       1.16f, 0.94f, 1.08f, 0.92f, 1.14f, 0.96f, 1.06f, 0.91f};
    for (int i = 0; i < 16; ++i)
    {
      afro.steps.push_back(JZGrooveStep(afroOffsets[i], afroVels[i]));
    }
    mTemplates.push_back(afro);
  }
}

const JZGrooveTemplate* JZGroovePool::GetTemplate(int index) const
{
  if (index < 0 || index >= (int)mTemplates.size()) return 0;
  return &mTemplates[index];
}

const JZGrooveTemplate* JZGroovePool::FindTemplateByName(const string& name) const
{
  for (size_t i = 0; i < mTemplates.size(); ++i)
  {
    if (mTemplates[i].name == name) return &mTemplates[i];
  }
  return 0;
}

int JZGroovePool::ApplyGroove(
  JZTrack* pTrack,
  const JZGrooveTemplate& tmpl,
  int fromClock,
  int toClock,
  float timingAmount,
  float velocityAmount)
{
  if (!pTrack || tmpl.steps.empty())
  {
    return 0;
  }

  timingAmount = max(0.0f, min(2.0f, timingAmount));
  velocityAmount = max(0.0f, min(2.0f, velocityAmount));

  int ticksPer16th = 24; // 96 TPQ standard: 96 / 4 = 24 ticks
  int ticksPerBar = 24 * tmpl.resolutionSteps;

  int notesModified = 0;

  for (int i = 0; i < pTrack->mEventCount; ++i)
  {
    JZEvent* pEv = pTrack->mppEvents[i];
    if (!pEv || pEv->IsKilled()) continue;

    JZKeyOnEvent* pKeyOn = pEv->IsKeyOn();
    if (!pKeyOn) continue;

    int clk = pKeyOn->GetClock();
    if (fromClock >= 0 && clk < fromClock) continue;
    if (toClock >= 0 && clk >= toClock) continue;

    // Find nearest 16th grid step within the bar
    int clockInBar = clk % ticksPerBar;
    int nearestStep = (clockInBar + (ticksPer16th / 2)) / ticksPer16th;
    nearestStep = max(0, min((int)tmpl.steps.size() - 1, nearestStep));

    const JZGrooveStep& step = tmpl.steps[nearestStep];

    // Apply micro-timing displacement
    int deltaTicks = (int)(step.tickOffset * timingAmount);
    int newClk = max(0, clk + deltaTicks);
    pKeyOn->SetClock(newClk);

    // Apply velocity accent multiplier
    int origVel = pKeyOn->GetVelocity();
    float targetScale = 1.0f + (step.velocityScale - 1.0f) * velocityAmount;
    int newVel = (int)(origVel * targetScale);
    newVel = max(1, min(127, newVel));
    pKeyOn->SetVelocity((unsigned char)newVel);

    notesModified++;
  }

  return notesModified;
}

JZGrooveTemplate JZGroovePool::ExtractGroove(
  JZTrack* pTrack,
  int fromClock,
  int toClock,
  const string& templateName,
  int ticksPerQuarter)
{
  JZGrooveTemplate tmpl(templateName, "User Extracted", 16);
  if (!pTrack || ticksPerQuarter <= 0)
  {
    return tmpl;
  }

  int ticksPer16th = ticksPerQuarter / 4;
  int ticksPerBar = ticksPer16th * 16;

  vector<int> stepOffsetSums(16, 0);
  vector<float> stepVelSums(16, 0.0f);
  vector<int> stepCounts(16, 0);

  for (int i = 0; i < pTrack->mEventCount; ++i)
  {
    JZEvent* pEv = pTrack->mppEvents[i];
    if (!pEv || pEv->IsKilled()) continue;

    JZKeyOnEvent* pKeyOn = pEv->IsKeyOn();
    if (!pKeyOn) continue;

    int clk = pKeyOn->GetClock();
    if (fromClock >= 0 && clk < fromClock) continue;
    if (toClock >= 0 && clk >= toClock) continue;

    int clkInBar = clk % ticksPerBar;
    int stepIdx = (clkInBar + (ticksPer16th / 2)) / ticksPer16th;
    if (stepIdx < 0 || stepIdx >= 16) continue;

    int gridClk = stepIdx * ticksPer16th;
    int offset = clkInBar - gridClk;

    stepOffsetSums[stepIdx] += offset;
    stepVelSums[stepIdx] += ((float)pKeyOn->GetVelocity() / 90.0f);
    stepCounts[stepIdx]++;
  }

  for (int s = 0; s < 16; ++s)
  {
    int avgOffset = (stepCounts[s] > 0) ? (stepOffsetSums[s] / stepCounts[s]) : 0;
    float avgVel = (stepCounts[s] > 0) ? (stepVelSums[s] / (float)stepCounts[s]) : 1.0f;
    tmpl.steps.push_back(JZGrooveStep(avgOffset, avgVel));
  }

  return tmpl;
}
