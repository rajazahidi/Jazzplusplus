//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Convolution Reverb & Impulse Response (IR) Cabinet Simulator
//
// High-fidelity convolution engine supporting acoustic spaces (Cathedral,
// Concert Hall, Jazz Club) and Guitar/Bass Cabinet speaker emulations
// (4x12 Vintage 30, 8x10 Bass Cab) via time-domain impulse response convolution.
//*****************************************************************************

#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

// Preset types of Impulse Responses
enum JZImpulseType
{
  IR_CONCERT_HALL = 0,
  IR_JAZZ_CLUB,
  IR_VINTAGE_SPRING,
  IR_GUITAR_CAB_4X12,
  IR_BASS_CAB_8X10,
  IR_CUSTOM
};

// Convolution Parameters
struct JZConvolutionConfig
{
  JZImpulseType irType;
  float dryGain;              // 0.0 to 1.0 (e.g. 1.0)
  float wetGain;              // 0.0 to 1.0 (e.g. 0.35)
  float preDelayMs;           // 0.0 to 150.0 ms
  float decayScale;           // 0.2 to 2.0 (length multiplier)
  float highCutHz;            // 1000.0 Hz to 20000.0 Hz (damping / high frequency roll-off)
  bool normalizeIR;           // Normalize impulse response energy

  JZConvolutionConfig()
    : irType(IR_CONCERT_HALL),
      dryGain(1.0f),
      wetGain(0.35f),
      preDelayMs(10.0f),
      decayScale(1.0f),
      highCutHz(12000.0f),
      normalizeIR(true)
  {
  }
};

// Convolution Reverb & IR Cabinet Engine
class JZConvolutionEngine
{
public:
  JZConvolutionEngine(float sampleRate = 44100.0f);
  ~JZConvolutionEngine() = default;

  void SetSampleRate(float sampleRate);
  float GetSampleRate() const { return mSampleRate; }

  void SetConfig(const JZConvolutionConfig& config);
  const JZConvolutionConfig& GetConfig() const { return mConfig; }
  JZConvolutionConfig& GetConfig() { return mConfig; }

  // Load preset impulse response
  void LoadPreset(JZImpulseType type);

  // Load custom impulse response audio buffers
  void LoadCustomIR(const std::vector<float>& irL, const std::vector<float>& irR);

  // Reset internal delay line and history buffers
  void Reset();

  // Process Stereo Audio Block
  void Process(float* inOutL, float* inOutR, int numSamples);

  // Process Mono Audio Block
  void ProcessMono(float* inOut, int numSamples);

  // Get current IR length in samples
  int GetIRLength() const { return (int)mIrLeft.size(); }

  // Static helper to generate acoustic and cabinet impulse responses procedurally
  static void GenerateImpulseResponse(
    JZImpulseType type,
    float sampleRate,
    float decayScale,
    std::vector<float>& outIrL,
    std::vector<float>& outIrR);

private:
  float mSampleRate;
  JZConvolutionConfig mConfig;

  // Active Impulse Responses
  std::vector<float> mIrLeft;
  std::vector<float> mIrRight;

  // Circular history buffers for convolution
  std::vector<float> mHistoryL;
  std::vector<float> mHistoryR;
  int mHistoryPos;

  // Pre-delay buffer
  std::vector<float> mPreDelayBufL;
  std::vector<float> mPreDelayBufR;
  int mPreDelayWritePos;

  void RebuildImpulse();
};
