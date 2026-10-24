//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Groove Pool & Advanced Humanize Groove Templates
//
// MPC-60 / MPC-3000 Swing, J Dilla Drunken Swing, Afrobeat, Funk Pocket,
// Micro-Timing Extraction and Non-Destructive Groove Quantization.
//*****************************************************************************

#pragma once

#include <string>
#include <vector>
#include <algorithm>

class JZTrack;

struct JZGrooveStep
{
  int tickOffset;            // Micro-timing displacement in ticks (e.g. -12 to +16)
  float velocityScale;       // Accent multiplier (e.g. 0.85 = ghost note, 1.25 = accent)
  float probability;         // Probability of playing (1.0 = always)

  JZGrooveStep(int offset = 0, float vel = 1.0f, float prob = 1.0f)
    : tickOffset(offset),
      velocityScale(vel),
      probability(prob)
  {
  }
};

struct JZGrooveTemplate
{
  std::string name;
  std::string category;      // MPC Swing, Neo-Soul / Dilla, Funk Pocket, Afrobeat, Latin
  int resolutionSteps;       // Default 16 (16th note grid)
  std::vector<JZGrooveStep> steps;

  JZGrooveTemplate(
    const std::string& n = "Default Groove",
    const std::string& cat = "Standard",
    int res = 16)
    : name(n),
      category(cat),
      resolutionSteps(res)
  {
  }
};

class JZGroovePool
{
  public:

    static JZGroovePool& Instance();

    // Built-in library of legendary producer grooves
    const std::vector<JZGrooveTemplate>& GetTemplates() const { return mTemplates; }
    int GetTemplateCount() const { return (int)mTemplates.size(); }
    const JZGrooveTemplate* GetTemplate(int index) const;
    const JZGrooveTemplate* FindTemplateByName(const std::string& name) const;

    // Apply groove template timing & velocity offsets to a track
    static int ApplyGroove(
      JZTrack* pTrack,
      const JZGrooveTemplate& tmpl,
      int fromClock = 0,
      int toClock = -1,
      float timingAmount = 1.0f,
      float velocityAmount = 1.0f);

    // Extract groove micro-timing & velocity variations from a live played track
    static JZGrooveTemplate ExtractGroove(
      JZTrack* pTrack,
      int fromClock,
      int toClock,
      const std::string& templateName = "Extracted Live Groove",
      int ticksPerQuarter = 96);

  private:

    JZGroovePool();
    void InitializePresets();

    std::vector<JZGrooveTemplate> mTemplates;
};
