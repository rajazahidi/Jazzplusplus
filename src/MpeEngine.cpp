//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// MPE (MIDI Polyphonic Expression) & MusicXML Interoperability Engine
// Implementation
//*****************************************************************************

#include "MpeEngine.h"
#include "Song.h"
#include "Track.h"
#include "Events.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

using namespace std;

//-----------------------------------------------------------------------------
// Singleton Instance
//-----------------------------------------------------------------------------
JZMpeManager& JZMpeManager::Instance()
{
  static JZMpeManager sInstance;
  return sInstance;
}

JZMpeManager::JZMpeManager()
  : mMasterChannel(1),
    mZoneStart(2),
    mZoneEnd(16),
    mLastAllocatedChannel(1)
{
}

void JZMpeManager::Reset()
{
  mActiveVoices.clear();
  mLastAllocatedChannel = mMasterChannel;
}

void JZMpeManager::SetZone(int startCh, int endCh)
{
  mZoneStart = max(1, min(16, startCh));
  mZoneEnd = max(mZoneStart, min(16, endCh));
}

int JZMpeManager::NoteOn(int pitch, int velocity)
{
  // Release any existing voice on same pitch
  NoteOff(pitch);

  // Round-robin channel allocation in MPE member zone
  int numChannels = mZoneEnd - mZoneStart + 1;
  int candidate = mLastAllocatedChannel + 1;
  if (candidate > mZoneEnd || candidate < mZoneStart)
  {
    candidate = mZoneStart;
  }

  // Find a free channel if possible
  for (int i = 0; i < numChannels; ++i)
  {
    bool inUse = false;
    for (auto it = mActiveVoices.begin(); it != mActiveVoices.end(); ++it)
    {
      if (it->second.channel == candidate)
      {
        inUse = true;
        break;
      }
    }
    if (!inUse)
    {
      break;
    }
    candidate++;
    if (candidate > mZoneEnd) candidate = mZoneStart;
  }

  mLastAllocatedChannel = candidate;
  mActiveVoices[pitch] = JZMpeVoice(pitch, candidate, velocity);
  return candidate;
}

int JZMpeManager::NoteOff(int pitch)
{
  auto it = mActiveVoices.find(pitch);
  if (it != mActiveVoices.end())
  {
    int ch = it->second.channel;
    mActiveVoices.erase(it);
    return ch;
  }
  return 0;
}

bool JZMpeManager::SetPerNotePitchBend(int pitch, int bendValue)
{
  auto it = mActiveVoices.find(pitch);
  if (it != mActiveVoices.end())
  {
    it->second.pitchBend = max(-8192, min(8191, bendValue));
    return true;
  }
  return false;
}

bool JZMpeManager::SetPerNotePressure(int pitch, int pressure)
{
  auto it = mActiveVoices.find(pitch);
  if (it != mActiveVoices.end())
  {
    it->second.pressure = max(0, min(127, pressure));
    return true;
  }
  return false;
}

bool JZMpeManager::SetPerNoteSlide(int pitch, int cc74Value)
{
  auto it = mActiveVoices.find(pitch);
  if (it != mActiveVoices.end())
  {
    it->second.slideCC74 = max(0, min(127, cc74Value));
    return true;
  }
  return false;
}

int JZMpeManager::GetChannelForPitch(int pitch) const
{
  auto it = mActiveVoices.find(pitch);
  if (it != mActiveVoices.end())
  {
    return it->second.channel;
  }
  return 0;
}

int JZMpeManager::GetActiveVoiceCount() const
{
  return (int)mActiveVoices.size();
}

//-----------------------------------------------------------------------------
// JZMusicXmlEngine Implementation
//-----------------------------------------------------------------------------
bool JZMusicXmlEngine::ExportMusicXml(
  JZSong* pSong,
  const string& filePath,
  const string& songTitle,
  const string& composer)
{
  if (!pSong) return false;

  ofstream out(filePath.c_str());
  if (!out.is_open()) return false;

  int tpq = pSong->GetTicksPerQuarter();
  if (tpq <= 0) tpq = 96;

  out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
  out << "<!DOCTYPE score-partwise PUBLIC \"-//Recordare//DTD MusicXML 3.1 Partwise//EN\" \"http://www.musicxml.org/dtds/partwise.dtd\">\n";
  out << "<score-partwise version=\"3.1\">\n";
  out << "  <work>\n";
  out << "    <work-title>" << songTitle << "</work-title>\n";
  out << "  </work>\n";
  out << "  <identification>\n";
  out << "    <creator type=\"composer\">" << composer << "</creator>\n";
  out << "    <encoding>\n";
  out << "      <software>Jazz++ MIDI Sequencer Studio Arranger Pro</software>\n";
  out << "    </encoding>\n";
  out << "  </identification>\n";

  // Part list
  out << "  <part-list>\n";
  int trackCount = pSong->GetTrackCount();
  for (int t = 0; t < trackCount; ++t)
  {
    JZTrack* trk = pSong->GetTrack(t);
    if (!trk || trk->IsKilled()) continue;
    string name = trk->GetTrackName();
    if (name.empty())
    {
      ostringstream oss;
      oss << "Track " << (t + 1);
      name = oss.str();
    }
    out << "    <score-part id=\"P" << (t + 1) << "\">\n";
    out << "      <part-name>" << name << "</part-name>\n";
    out << "    </score-part>\n";
  }
  out << "  </part-list>\n";

  static const char* sPitchSteps[12] = {"C", "C", "D", "D", "E", "F", "F", "G", "G", "A", "A", "B"};
  static const int sAlterTable[12] =   { 0,   1,   0,   1,   0,   0,   1,   0,   1,   0,   1,   0};

  for (int t = 0; t < trackCount; ++t)
  {
    JZTrack* trk = pSong->GetTrack(t);
    if (!trk || trk->IsKilled()) continue;

    out << "  <part id=\"P" << (t + 1) << "\">\n";
    int numBars = pSong->NumBars();
    if (numBars <= 0) numBars = 4;

    for (int b = 0; b < numBars; ++b)
    {
      out << "    <measure number=\"" << (b + 1) << "\">\n";
      if (b == 0)
      {
        out << "      <attributes>\n";
        out << "        <divisions>" << tpq << "</divisions>\n";
        out << "        <key>\n";
        out << "          <fifths>0</fifths>\n";
        out << "          <mode>major</mode>\n";
        out << "        </key>\n";
        out << "        <time>\n";
        out << "          <beats>4</beats>\n";
        out << "          <beat-type>4</beat-type>\n";
        out << "        </time>\n";
        out << "        <clef>\n";
        out << "          <sign>" << (t == 1 ? "F" : "G") << "</sign>\n";
        out << "          <line>" << (t == 1 ? "4" : "2") << "</line>\n";
        out << "        </clef>\n";
        out << "      </attributes>\n";
      }

      int bStart = b * tpq * 4;
      int bEnd = (b + 1) * tpq * 4;
      int notesInBar = 0;

      for (int i = 0; i < trk->mEventCount; ++i)
      {
        JZEvent* ev = trk->mppEvents[i];
        if (!ev || ev->IsKilled()) continue;

        JZKeyOnEvent* kOn = ev->IsKeyOn();
        if (!kOn) continue;

        int clk = kOn->GetClock();
        if (clk >= bStart && clk < bEnd)
        {
          int pitch = kOn->GetPitch();
          int len = kOn->GetLength();
          if (len <= 0) len = tpq;

          int pc = pitch % 12;
          int octave = (pitch / 12) - 1;
          const char* stepName = sPitchSteps[pc];
          int alter = sAlterTable[pc];

          out << "      <note>\n";
          out << "        <pitch>\n";
          out << "          <step>" << stepName << "</step>\n";
          if (alter != 0)
          {
            out << "          <alter>" << alter << "</alter>\n";
          }
          out << "          <octave>" << octave << "</octave>\n";
          out << "        </pitch>\n";
          out << "        <duration>" << len << "</duration>\n";
          out << "        <voice>1</voice>\n";
          out << "        <type>" << (len >= tpq * 4 ? "whole" : (len >= tpq * 2 ? "half" : (len >= tpq ? "quarter" : "eighth"))) << "</type>\n";
          out << "      </note>\n";
          notesInBar++;
        }
      }

      if (notesInBar == 0)
      {
        // Emit full measure rest
        out << "      <note>\n";
        out << "        <rest/>\n";
        out << "        <duration>" << (tpq * 4) << "</duration>\n";
        out << "        <voice>1</voice>\n";
        out << "        <type>whole</type>\n";
        out << "      </note>\n";
      }

      out << "    </measure>\n";
    }

    out << "  </part>\n";
  }

  out << "</score-partwise>\n";
  out.close();
  return true;
}

bool JZMusicXmlEngine::ImportMusicXml(
  JZSong* pSong,
  const string& filePath)
{
  if (!pSong) return false;

  ifstream in(filePath.c_str());
  if (!in.is_open()) return false;

  string line;
  int currentPart = 0;
  int currentMeasure = 0;
  int currentClock = 0;
  int divisions = 96;

  JZTrack* pTrack = pSong->GetTrack(0);
  if (!pTrack) return false;

  string step = "C";
  int octave = 4;
  int alter = 0;
  int duration = divisions;
  bool isRest = false;

  while (getline(in, line))
  {
    if (line.find("<divisions>") != string::npos)
    {
      size_t s1 = line.find("<divisions>") + 11;
      size_t s2 = line.find("</divisions>");
      if (s2 != string::npos) divisions = atoi(line.substr(s1, s2 - s1).c_str());
      if (divisions <= 0) divisions = 96;
    }
    else if (line.find("<measure") != string::npos)
    {
      currentMeasure++;
      currentClock = (currentMeasure - 1) * divisions * 4;
    }
    else if (line.find("<rest") != string::npos)
    {
      isRest = true;
    }
    else if (line.find("<step>") != string::npos)
    {
      size_t s1 = line.find("<step>") + 6;
      size_t s2 = line.find("</step>");
      if (s2 != string::npos) step = line.substr(s1, s2 - s1);
      isRest = false;
    }
    else if (line.find("<alter>") != string::npos)
    {
      size_t s1 = line.find("<alter>") + 7;
      size_t s2 = line.find("</alter>");
      if (s2 != string::npos) alter = atoi(line.substr(s1, s2 - s1).c_str());
    }
    else if (line.find("<octave>") != string::npos)
    {
      size_t s1 = line.find("<octave>") + 8;
      size_t s2 = line.find("</octave>");
      if (s2 != string::npos) octave = atoi(line.substr(s1, s2 - s1).c_str());
    }
    else if (line.find("<duration>") != string::npos)
    {
      size_t s1 = line.find("<duration>") + 10;
      size_t s2 = line.find("</duration>");
      if (s2 != string::npos) duration = atoi(line.substr(s1, s2 - s1).c_str());
      if (duration <= 0) duration = divisions;
    }
    else if (line.find("</note>") != string::npos)
    {
      if (!isRest)
      {
        int basePc = 0;
        char c = step.empty() ? 'C' : step[0];
        switch (c)
        {
          case 'C': basePc = 0; break;
          case 'D': basePc = 2; break;
          case 'E': basePc = 4; break;
          case 'F': basePc = 5; break;
          case 'G': basePc = 7; break;
          case 'A': basePc = 9; break;
          case 'B': basePc = 11; break;
          default: basePc = 0; break;
        }
        int pitch = (octave + 1) * 12 + basePc + alter;
        pitch = max(0, min(127, pitch));

        pTrack->Put(new JZKeyOnEvent(currentClock, 0, (unsigned char)pitch, 85, (unsigned short)duration));
      }
      currentClock += duration;
      isRest = false;
      alter = 0;
    }
  }

  in.close();
  return true;
}
