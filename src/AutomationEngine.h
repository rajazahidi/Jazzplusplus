//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Dynamic Automation Curves & Visual Envelope Lanes
//
// Continuous Bezier, Spline, Exponential, S-Curve & Linear Envelopes
// with Multi-Point Node Editing and Optimized MIDI CC Stream Generation.
//*****************************************************************************

#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

class JZTrack;

// Automation Target Controller Types
enum JZAutomationType
{
  AUTO_VOLUME = 0,         // CC 7 (Channel Volume)
  AUTO_PAN,                // CC 10 (Pan Position)
  AUTO_EXPRESSION,         // CC 11 (Expression)
  AUTO_FILTER_CUTOFF,      // CC 74 (Filter Cutoff / Brightness)
  AUTO_FILTER_RES,         // CC 71 (Filter Resonance / Harmonic Content)
  AUTO_REVERB_SEND,        // CC 91 (Reverb Send Level)
  AUTO_CHORUS_SEND,        // CC 93 (Chorus Send Level)
  AUTO_PITCH_BEND,         // 14-bit Pitch Wheel (-8192 to +8191)
  AUTO_TEMPO,              // BPM Tempo Automation (20 to 300 BPM)
  AUTO_CUSTOM_CC,          // Arbitrary User-Defined MIDI CC
  AUTO_TYPE_COUNT
};

// Curve Interpolation Algorithms
enum JZInterpolationType
{
  INTERP_LINEAR = 0,       // Straight linear ramp
  INTERP_BEZIER,           // Smooth cubic Bezier with adjustable curvature tension
  INTERP_EXPONENTIAL,      // Exponential growth / decay (ideal for filter sweeps)
  INTERP_S_CURVE,          // Smooth sigmoid S-curve (natural organic transition)
  INTERP_HOLD              // Step jump at destination (instantaneous switch)
};

// Automation Curve Node
struct JZAutomationNode
{
  int clock;                     // Timeline clock tick position
  double value;                  // Normalized value [0.0, 1.0]
  JZInterpolationType interp;    // Interpolation type towards the next node
  double tension;                // Curvature tension [-1.0, 1.0] (0.0 = balanced, >0 = ease-out, <0 = ease-in)

  JZAutomationNode(
    int c = 0,
    double v = 0.5,
    JZInterpolationType it = INTERP_BEZIER,
    double t = 0.0)
    : clock(c),
      value(v),
      interp(it),
      tension(t)
  {
  }

  bool operator<(const JZAutomationNode& other) const
  {
    return clock < other.clock;
  }
};

// Automation Curve
class JZAutomationCurve
{
  public:

    JZAutomationCurve();
    ~JZAutomationCurve();

    void Clear();
    int GetNodeCount() const { return (int)mNodes.size(); }
    const JZAutomationNode& GetNode(int index) const { return mNodes[index]; }
    JZAutomationNode& GetNode(int index) { return mNodes[index]; }

    int AddNode(int clock, double value, JZInterpolationType interp = INTERP_BEZIER, double tension = 0.0);
    bool RemoveNode(int index);
    bool SetNodeValue(int index, double value);
    bool SetNodeClock(int index, int clock);
    bool SetNodeInterp(int index, JZInterpolationType interp, double tension = 0.0);

    // Evaluate normalized value [0.0, 1.0] at any arbitrary timeline clock position
    double GetValueAtClock(int clock) const;

    // Evaluate value mapped to specific MIDI range (e.g. 0-127, or pitch bend -8192..8191)
    int GetMidiValueAtClock(int clock, int minVal = 0, int maxVal = 127) const;

    // Bake & Render curve to optimized MIDI CC / Pitch events into target track
    int RenderToTrack(
      JZTrack* pTrack,
      int channel,
      JZAutomationType type,
      int fromClock,
      int toClock,
      int resolutionTicks = 12,
      int customCC = 1);

  private:

    double InterpolateNodes(
      const JZAutomationNode& n1,
      const JZAutomationNode& n2,
      double t) const;

    std::vector<JZAutomationNode> mNodes;
};

// Automation Lane for a Track
class JZAutomationLane
{
  public:

    JZAutomationLane(int trackIndex = 0, JZAutomationType type = AUTO_VOLUME, int customCC = 1);

    int GetTrackIndex() const { return mTrackIndex; }
    JZAutomationType GetType() const { return mType; }
    int GetCustomCC() const { return mCustomCC; }
    const std::string& GetName() const { return mName; }
    void SetName(const std::string& name) { mName = name; }

    bool IsVisible() const { return mIsVisible; }
    void SetVisible(bool v) { mIsVisible = v; }
    bool IsMuted() const { return mIsMuted; }
    void SetMuted(bool m) { mIsMuted = m; }

    JZAutomationCurve& GetCurve() { return mCurve; }
    const JZAutomationCurve& GetCurve() const { return mCurve; }

    static const char* GetAutomationTypeName(JZAutomationType type);

  private:

    int mTrackIndex;
    JZAutomationType mType;
    int mCustomCC;
    std::string mName;
    bool mIsVisible;
    bool mIsMuted;
    JZAutomationCurve mCurve;
};

// Global Automation Registry & Coordinator
class JZAutomationManager
{
  public:

    static JZAutomationManager& Instance();

    void Clear();
    JZAutomationLane* AddLane(int trackIndex, JZAutomationType type, int customCC = 1);
    JZAutomationLane* GetLane(int trackIndex, JZAutomationType type, int customCC = 1);
    bool RemoveLane(int trackIndex, JZAutomationType type, int customCC = 1);
    std::vector<JZAutomationLane*> GetTrackLanes(int trackIndex);
    int RenderAllLanesToTrack(JZTrack* pTrack, int trackIndex, int channel, int fromClock, int toClock);

  private:

    JZAutomationManager() {}
    std::vector<JZAutomationLane> mLanes;
};
