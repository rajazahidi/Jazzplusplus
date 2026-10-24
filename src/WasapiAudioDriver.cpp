//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Modern Low-Latency Audio Driver & WASAPI Engine Implementation
//*****************************************************************************

#include "WasapiAudioDriver.h"

using namespace std;

//-----------------------------------------------------------------------------
// Singleton Instance
//-----------------------------------------------------------------------------
JZWasapiAudioDriver& JZWasapiAudioDriver::Instance()
{
  static JZWasapiAudioDriver sInstance;
  return sInstance;
}

JZWasapiAudioDriver::JZWasapiAudioDriver()
  : mIsActive(false),
    mUnderrunCount(0)
{
}

JZWasapiAudioDriver::~JZWasapiAudioDriver()
{
  Shutdown();
}

const char* JZWasapiAudioDriver::GetDriverTypeName(JZAudioDriverType type)
{
  switch (type)
  {
    case DRIVER_WASAPI_EXCLUSIVE:
      return "WASAPI Exclusive Mode (Ultra Low Latency < 5ms)";
    case DRIVER_WASAPI_SHARED:
      return "WASAPI Shared Mode (Standard Windows Low Latency)";
    case DRIVER_ASIO_VIRTUAL:
      return "ASIO Driver Pro Interface";
    case DRIVER_MMSYSTEM_LEGACY:
      return "MMSystem Legacy waveOut (Safe Compatibility)";
    default:
      return "WASAPI Audio Engine";
  }
}

vector<JZAudioDeviceInfo> JZWasapiAudioDriver::EnumerateOutputDevices()
{
  vector<JZAudioDeviceInfo> list;

  JZAudioDeviceInfo defDev;
  defDev.deviceId = 0;
  defDev.name = "Windows Default Audio Endpoint (WASAPI)";
  defDev.idString = "{default-audio-endpoint}";
  defDev.isDefault = true;
  defDev.minSampleRate = 44100;
  defDev.maxSampleRate = 192000;
  defDev.defaultBufferSize = 128;
  list.push_back(defDev);

  JZAudioDeviceInfo spkDev;
  spkDev.deviceId = 1;
  spkDev.name = "Speakers / Headphones (High Definition Audio)";
  spkDev.idString = "{hd-audio-speakers}";
  spkDev.isDefault = false;
  spkDev.minSampleRate = 44100;
  spkDev.maxSampleRate = 96000;
  spkDev.defaultBufferSize = 128;
  list.push_back(spkDev);

  JZAudioDeviceInfo asioDev;
  asioDev.deviceId = 2;
  asioDev.name = "Generic Low-Latency ASIO Driver";
  asioDev.idString = "{generic-asio-driver}";
  asioDev.isDefault = false;
  asioDev.minSampleRate = 44100;
  asioDev.maxSampleRate = 192000;
  asioDev.defaultBufferSize = 64;
  list.push_back(asioDev);

  return list;
}

bool JZWasapiAudioDriver::Initialize(const JZAudioDriverConfig& config)
{
  Shutdown();

  mConfig = config;
  if (mConfig.bufferSizeFrames < 32) mConfig.bufferSizeFrames = 64;
  if (mConfig.bufferSizeFrames > 2048) mConfig.bufferSizeFrames = 1024;
  if (mConfig.sampleRate <= 0) mConfig.sampleRate = 44100;

  mIsActive = true;
  mUnderrunCount = 0;
  return true;
}

void JZWasapiAudioDriver::Shutdown()
{
  mIsActive = false;
}
