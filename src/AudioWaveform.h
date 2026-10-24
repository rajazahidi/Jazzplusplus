//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Timeline Audio Waveform Rendering & Non-Destructive Clip Editing
//
// Peak & RMS Waveform Thumbnail Generator, Multi-Resolution Mipmap Cache,
// Non-Destructive Audio Clip Trimming, Splitting, Gain & Crossfades.
//*****************************************************************************

#pragma once

#include <string>
#include <vector>
#include <cmath>
#include <algorithm>

struct JZWaveformPeak
{
  float minVal;
  float maxVal;
  float rmsVal;

  JZWaveformPeak(float mn = 0.0f, float mx = 0.0f, float rms = 0.0f)
    : minVal(mn), maxVal(mx), rmsVal(rms)
  {
  }
};

// Multi-Resolution Waveform Thumbnail Cache
class JZWaveformThumbnail
{
  public:

    JZWaveformThumbnail();
    ~JZWaveformThumbnail();

    void Clear();

    // Generate peak cache from a 16-bit PCM WAV file on disk
    bool GenerateFromWav(const std::string& wavFilePath);

    // Generate peak cache from raw floating-point audio samples
    bool GenerateFromSamples(
      const float* pSamples,
      int totalSamples,
      int numChannels = 2,
      int sampleRate = 44100);

    bool IsLoaded() const { return !mPeaksLowRes.empty(); }
    int GetTotalSamples() const { return mTotalSamples; }
    int GetSampleRate() const { return mSampleRate; }
    double GetDurationSeconds() const { return mSampleRate > 0 ? (double)mTotalSamples / (double)mSampleRate : 0.0; }

    // Retrieve peaks across a sample range
    const std::vector<JZWaveformPeak>& GetPeaks(int level = 0) const;

  private:

    int mTotalSamples;
    int mSampleRate;
    int mChannels;

    // Multi-resolution pyramid (Mipmaps):
    // Level 0: 64 samples per peak (high detail, zoomed in)
    // Level 1: 256 samples per peak (medium overview)
    // Level 2: 1024 samples per peak (entire song bird's eye view)
    std::vector<JZWaveformPeak> mPeaksHighRes;
    std::vector<JZWaveformPeak> mPeaksMedRes;
    std::vector<JZWaveformPeak> mPeaksLowRes;
};

// Non-Destructive Timeline Audio Clip
class JZAudioClip
{
  public:

    JZAudioClip(
      const std::string& clipId = "",
      const std::string& name = "Audio Clip",
      const std::string& wavFilePath = "",
      int startClock = 0,
      int lengthClock = 768);

    const std::string& GetId() const { return mClipId; }
    const std::string& GetName() const { return mName; }
    void SetName(const std::string& name) { mName = name; }
    const std::string& GetWavFilePath() const { return mWavFilePath; }

    int GetStartClock() const { return mStartClock; }
    void SetStartClock(int clock) { mStartClock = std::max(0, clock); }
    int GetLengthClock() const { return mLengthClock; }
    void SetLengthClock(int len) { mLengthClock = std::max(1, len); }
    int GetEndClock() const { return mStartClock + mLengthClock; }

    int GetSampleOffset() const { return mSampleOffset; }
    void SetSampleOffset(int offset) { mSampleOffset = std::max(0, offset); }

    int GetFadeInClocks() const { return mFadeInClocks; }
    void SetFadeInClocks(int clocks) { mFadeInClocks = std::max(0, clocks); }
    int GetFadeOutClocks() const { return mFadeOutClocks; }
    void SetFadeOutClocks(int clocks) { mFadeOutClocks = std::max(0, clocks); }

    float GetGain() const { return mGain; }
    void SetGain(float g) { mGain = std::max(0.0f, std::min(4.0f, g)); }
    bool IsMuted() const { return mIsMuted; }
    void SetMuted(bool m) { mIsMuted = m; }

    JZWaveformThumbnail& GetWaveform() { return mWaveform; }
    const JZWaveformThumbnail& GetWaveform() const { return mWaveform; }

    // Non-destructive slice / split at clock position
    // Returns new right-hand clip on success, nullptr on invalid clock
    JZAudioClip* SplitAt(int splitClock, const std::string& newClipId = "");

    // Trim start or end clock without modifying original audio file
    bool TrimStart(int newStartClock);
    bool TrimEnd(int newEndClock);

  private:

    std::string mClipId;
    std::string mName;
    std::string mWavFilePath;
    int mStartClock;
    int mLengthClock;
    int mSampleOffset;
    int mFadeInClocks;
    int mFadeOutClocks;
    float mGain;
    bool mIsMuted;
    JZWaveformThumbnail mWaveform;
};

// Timeline Audio Track Manager
class JZAudioClipTrack
{
  public:

    JZAudioClipTrack(int trackIndex = 0);
    ~JZAudioClipTrack();

    void Clear();
    int GetTrackIndex() const { return mTrackIndex; }
    int GetClipCount() const { return (int)mClips.size(); }
    JZAudioClip* GetClip(int index);
    const JZAudioClip* GetClip(int index) const;

    JZAudioClip* AddClip(const JZAudioClip& clip);
    bool RemoveClip(int index);
    JZAudioClip* FindClipAtClock(int clock);

  private:

    int mTrackIndex;
    std::vector<JZAudioClip> mClips;
};
