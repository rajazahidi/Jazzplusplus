//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Modern Low-Latency Audio Driver & WASAPI Engine
//
// Sub-10ms Low-Latency Buffering, Exclusive & Shared Mode Architecture,
// Buffer Size Profiling & Audio Performance Diagnostics.
//*****************************************************************************

#pragma once

#include <string>
#include <vector>

enum JZAudioDriverType
{
  DRIVER_WASAPI_EXCLUSIVE = 0,   // Ultra low-latency direct hardware bypass (sub-5ms)
  DRIVER_WASAPI_SHARED,          // Studio default Windows audio engine (low latency 10-15ms)
  DRIVER_ASIO_VIRTUAL,           // ASIO low-latency interface
  DRIVER_MMSYSTEM_LEGACY,        // Legacy waveOut / waveIn fallback
  DRIVER_TYPE_COUNT
};

struct JZAudioDriverConfig
{
  JZAudioDriverType driverType;
  int sampleRate;                // 44100, 48000, 88200, 96000
  int bufferSizeFrames;          // 64, 128, 256, 512, 1024
  int numChannels;               // 2 (Stereo)
  bool exclusiveMode;            // Direct hardware access
  std::string deviceName;

  JZAudioDriverConfig()
    : driverType(DRIVER_WASAPI_EXCLUSIVE),
      sampleRate(44100),
      bufferSizeFrames(128),
      numChannels(2),
      exclusiveMode(true),
      deviceName("Default Output Device")
  {
  }

  // Calculate round-trip hardware buffer latency in milliseconds
  double GetBufferLatencyMs() const
  {
    if (sampleRate <= 0) return 0.0;
    return ((double)bufferSizeFrames / (double)sampleRate) * 1000.0;
  }
};

struct JZAudioDeviceInfo
{
  int deviceId;
  std::string name;
  std::string idString;
  bool isDefault;
  int minSampleRate;
  int maxSampleRate;
  int defaultBufferSize;
};

class JZWasapiAudioDriver
{
  public:

    static JZWasapiAudioDriver& Instance();

    // Query available system audio devices
    std::vector<JZAudioDeviceInfo> EnumerateOutputDevices();

    // Initialize audio driver with desired configuration
    bool Initialize(const JZAudioDriverConfig& config);
    void Shutdown();

    bool IsActive() const { return mIsActive; }
    const JZAudioDriverConfig& GetConfig() const { return mConfig; }

    // Performance metrics & diagnostics
    double GetCurrentLatencyMs() const { return mConfig.GetBufferLatencyMs(); }
    int GetUnderrunCount() const { return mUnderrunCount; }
    void ResetUnderrunCount() { mUnderrunCount = 0; }

    static const char* GetDriverTypeName(JZAudioDriverType type);

  private:

    JZWasapiAudioDriver();
    ~JZWasapiAudioDriver();

    bool mIsActive;
    JZAudioDriverConfig mConfig;
    int mUnderrunCount;
};
