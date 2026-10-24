//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Timeline Audio Waveform Rendering & Non-Destructive Clip Editing
// Implementation
//*****************************************************************************

#include "AudioWaveform.h"

#include <fstream>
#include <iostream>
#include <cmath>
#include <cstring>
#include <cstdint>

using namespace std;

//-----------------------------------------------------------------------------
// JZWaveformThumbnail Implementation
//-----------------------------------------------------------------------------
JZWaveformThumbnail::JZWaveformThumbnail()
  : mTotalSamples(0),
    mSampleRate(44100),
    mChannels(2)
{
}

JZWaveformThumbnail::~JZWaveformThumbnail()
{
  Clear();
}

void JZWaveformThumbnail::Clear()
{
  mTotalSamples = 0;
  mPeaksHighRes.clear();
  mPeaksMedRes.clear();
  mPeaksLowRes.clear();
}

const vector<JZWaveformPeak>& JZWaveformThumbnail::GetPeaks(int level) const
{
  if (level == 0 && !mPeaksHighRes.empty()) return mPeaksHighRes;
  if (level == 1 && !mPeaksMedRes.empty()) return mPeaksMedRes;
  return mPeaksLowRes;
}

bool JZWaveformThumbnail::GenerateFromSamples(
  const float* pSamples,
  int totalSamples,
  int numChannels,
  int sampleRate)
{
  if (!pSamples || totalSamples <= 0)
  {
    return false;
  }

  Clear();
  mTotalSamples = totalSamples;
  mSampleRate = (sampleRate > 0) ? sampleRate : 44100;
  mChannels = (numChannels > 0) ? numChannels : 1;

  auto buildLevel = [&](int blockSize, vector<JZWaveformPeak>& outPeaks) {
    outPeaks.clear();
    int numBlocks = (totalSamples + blockSize - 1) / blockSize;
    outPeaks.reserve(numBlocks);

    for (int b = 0; b < numBlocks; ++b)
    {
      int startIdx = b * blockSize;
      int endIdx = min(totalSamples, startIdx + blockSize);

      float minV = 0.0f;
      float maxV = 0.0f;
      double sumSq = 0.0;
      int count = 0;

      for (int s = startIdx; s < endIdx; ++s)
      {
        float val = pSamples[s];
        if (val < minV) minV = val;
        if (val > maxV) maxV = val;
        sumSq += (val * val);
        count++;
      }

      float rms = (count > 0) ? (float)sqrt(sumSq / (double)count) : 0.0f;
      outPeaks.push_back(JZWaveformPeak(minV, maxV, rms));
    }
  };

  buildLevel(64, mPeaksHighRes);
  buildLevel(256, mPeaksMedRes);
  buildLevel(1024, mPeaksLowRes);

  return true;
}

bool JZWaveformThumbnail::GenerateFromWav(const string& wavFilePath)
{
  ifstream in(wavFilePath.c_str(), ios::binary);
  if (!in.is_open())
  {
    return false;
  }

  char riff[4];
  in.read(riff, 4);
  if (memcmp(riff, "RIFF", 4) != 0) return false;

  uint32_t chunkSize = 0;
  in.read(reinterpret_cast<char*>(&chunkSize), 4);

  char wave[4];
  in.read(wave, 4);
  if (memcmp(wave, "WAVE", 4) != 0) return false;

  // Search for 'fmt ' chunk
  uint16_t audioFormat = 1;
  uint16_t numChannels = 2;
  uint32_t sRate = 44100;
  uint16_t bitsPerSample = 16;
  bool fmtFound = false;
  bool dataFound = false;
  uint32_t dataBytes = 0;
  streampos dataPos = 0;

  while (in.good() && (!fmtFound || !dataFound))
  {
    char subchunkId[4];
    in.read(subchunkId, 4);
    if (!in.good()) break;

    uint32_t subchunkSize = 0;
    in.read(reinterpret_cast<char*>(&subchunkSize), 4);
    if (!in.good()) break;

    if (memcmp(subchunkId, "fmt ", 4) == 0)
    {
      in.read(reinterpret_cast<char*>(&audioFormat), 2);
      in.read(reinterpret_cast<char*>(&numChannels), 2);
      in.read(reinterpret_cast<char*>(&sRate), 4);
      uint32_t byteRate = 0;
      in.read(reinterpret_cast<char*>(&byteRate), 4);
      uint16_t blockAlign = 0;
      in.read(reinterpret_cast<char*>(&blockAlign), 2);
      in.read(reinterpret_cast<char*>(&bitsPerSample), 2);
      if (subchunkSize > 16)
      {
        in.seekg(subchunkSize - 16, ios::cur);
      }
      fmtFound = true;
    }
    else if (memcmp(subchunkId, "data", 4) == 0)
    {
      dataBytes = subchunkSize;
      dataPos = in.tellg();
      dataFound = true;
      break;
    }
    else
    {
      in.seekg(subchunkSize, ios::cur);
    }
  }

  if (!fmtFound || !dataFound || bitsPerSample != 16)
  {
    return false;
  }

  in.seekg(dataPos);
  int totalFrames = dataBytes / (numChannels * 2);
  if (totalFrames <= 0) return false;

  vector<float> monoSamples(totalFrames, 0.0f);
  int bufferFrames = 4096;
  vector<int16_t> rawBuf(bufferFrames * numChannels);

  int framesRead = 0;
  while (framesRead < totalFrames && in.good())
  {
    int toRead = min(bufferFrames, totalFrames - framesRead);
    in.read(reinterpret_cast<char*>(rawBuf.data()), toRead * numChannels * 2);
    int actuallyRead = (int)in.gcount() / (numChannels * 2);
    if (actuallyRead <= 0) break;

    for (int f = 0; f < actuallyRead; ++f)
    {
      float sum = 0.0f;
      for (int ch = 0; ch < numChannels; ++ch)
      {
        sum += (float)rawBuf[f * numChannels + ch] / 32768.0f;
      }
      monoSamples[framesRead + f] = sum / (float)numChannels;
    }
    framesRead += actuallyRead;
  }

  in.close();
  return GenerateFromSamples(monoSamples.data(), framesRead, 1, sRate);
}

//-----------------------------------------------------------------------------
// JZAudioClip Implementation
//-----------------------------------------------------------------------------
JZAudioClip::JZAudioClip(
  const string& clipId,
  const string& name,
  const string& wavFilePath,
  int startClock,
  int lengthClock)
  : mClipId(clipId),
    mName(name),
    mWavFilePath(wavFilePath),
    mStartClock(startClock),
    mLengthClock(lengthClock),
    mSampleOffset(0),
    mFadeInClocks(0),
    mFadeOutClocks(0),
    mGain(1.0f),
    mIsMuted(false)
{
  if (!wavFilePath.empty())
  {
    mWaveform.GenerateFromWav(wavFilePath);
  }
}

JZAudioClip* JZAudioClip::SplitAt(int splitClock, const string& newClipId)
{
  if (splitClock <= mStartClock || splitClock >= (mStartClock + mLengthClock))
  {
    return 0;
  }

  int leftLength = splitClock - mStartClock;
  int rightLength = mLengthClock - leftLength;

  JZAudioClip* pRight = new JZAudioClip(*this);
  pRight->mStartClock = splitClock;
  pRight->mLengthClock = rightLength;
  if (!newClipId.empty()) pRight->mClipId = newClipId;
  else pRight->mClipId = mClipId + "_split";

  // Compute sample offset shift based on 44.1kHz standard ratio
  int sampleDelta = (int)(((double)leftLength / 768.0) * 44100.0);
  pRight->mSampleOffset = mSampleOffset + sampleDelta;

  this->mLengthClock = leftLength;

  return pRight;
}

bool JZAudioClip::TrimStart(int newStartClock)
{
  if (newStartClock < 0 || newStartClock >= (mStartClock + mLengthClock))
  {
    return false;
  }
  int delta = newStartClock - mStartClock;
  int sampleDelta = (int)(((double)delta / 768.0) * 44100.0);
  mSampleOffset += sampleDelta;
  mLengthClock -= delta;
  mStartClock = newStartClock;
  return true;
}

bool JZAudioClip::TrimEnd(int newEndClock)
{
  if (newEndClock <= mStartClock)
  {
    return false;
  }
  mLengthClock = newEndClock - mStartClock;
  return true;
}

//-----------------------------------------------------------------------------
// JZAudioClipTrack Implementation
//-----------------------------------------------------------------------------
JZAudioClipTrack::JZAudioClipTrack(int trackIndex)
  : mTrackIndex(trackIndex)
{
}

JZAudioClipTrack::~JZAudioClipTrack()
{
  Clear();
}

void JZAudioClipTrack::Clear()
{
  mClips.clear();
}

JZAudioClip* JZAudioClipTrack::GetClip(int index)
{
  if (index < 0 || index >= (int)mClips.size()) return 0;
  return &mClips[index];
}

const JZAudioClip* JZAudioClipTrack::GetClip(int index) const
{
  if (index < 0 || index >= (int)mClips.size()) return 0;
  return &mClips[index];
}

JZAudioClip* JZAudioClipTrack::AddClip(const JZAudioClip& clip)
{
  mClips.push_back(clip);
  return &mClips.back();
}

bool JZAudioClipTrack::RemoveClip(int index)
{
  if (index < 0 || index >= (int)mClips.size()) return false;
  mClips.erase(mClips.begin() + index);
  return true;
}

JZAudioClip* JZAudioClipTrack::FindClipAtClock(int clock)
{
  for (size_t i = 0; i < mClips.size(); ++i)
  {
    if (clock >= mClips[i].GetStartClock() && clock < mClips[i].GetEndClock())
    {
      return &mClips[i];
    }
  }
  return 0;
}
