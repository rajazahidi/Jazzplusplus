//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Self-Contained Project Bundle & Asset Archiver (.jazzbundle)
// Implementation
//*****************************************************************************

#include "ProjectBundle.h"
#include "Project.h"
#include "Song.h"
#include "Track.h"
#include "Events.h"
#include "AutoAccompaniment.h"
#include "AutomationEngine.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <ctime>

using namespace std;

//-----------------------------------------------------------------------------
// JZProjectBundle Implementation
//-----------------------------------------------------------------------------
bool JZProjectBundle::ExportBundle(
  JZProject* pProject,
  const string& bundleFilePath,
  const JZAccompanimentConfig* pArrangementConfig)
{
  if (!pProject || !pProject->GetSong())
  {
    return false;
  }

  ofstream out(bundleFilePath.c_str(), ios::binary);
  if (!out.is_open())
  {
    return false;
  }

  JZSong* pSong = pProject->GetSong();
  int trackCount = pSong->GetTrackCount();

  // 1. Magic Header
  out << "[JAZZBUNDLE_V1]\n";

  // 2. Project Manifest
  out << "ProjectName=JazzStudioProject\n";
  out << "Composer=Jazz++ Producer Suite\n";
  out << "TempoBpm=120\n";
  out << "TrackCount=" << trackCount << "\n";
  out << "TotalMeasures=" << pSong->NumBars() << "\n";
  out << "TicksPerQuarter=" << pSong->GetTicksPerQuarter() << "\n";

  // 3. Embedded Arrangement Style Configuration (.jazzstyle format)
  out << "[SECTION:STYLE_PRESET]\n";
  if (pArrangementConfig)
  {
    out << "mode=" << (int)pArrangementConfig->mode << "\n";
    out << "style=" << (int)pArrangementConfig->style << "\n";
    out << "swingPercent=" << pArrangementConfig->swingPercent << "\n";
    out << "generateBass=" << (pArrangementConfig->generateBass ? 1 : 0) << "\n";
    out << "generateDrums=" << (pArrangementConfig->generateDrums ? 1 : 0) << "\n";
    out << "generatePad=" << (pArrangementConfig->generatePad ? 1 : 0) << "\n";
    out << "reharmonizeMode=" << (int)pArrangementConfig->reharmonizeMode << "\n";
    out << "enableAudioClickTrack=" << (pArrangementConfig->enableAudioClickTrack ? 1 : 0) << "\n";
  }
  else
  {
    out << "style=0\n";
    out << "swingPercent=62\n";
  }

  // 4. Embedded Track Summary & Events
  out << "[SECTION:TRACKS]\n";
  for (int t = 0; t < trackCount; ++t)
  {
    JZTrack* trk = pSong->GetTrack(t);
    if (!trk || trk->IsKilled()) continue;

    out << "TrackIndex=" << t << "\n";
    out << "TrackName=" << trk->GetTrackName() << "\n";
    out << "Channel=" << trk->GetChannel() << "\n";
    out << "Volume=" << trk->GetVolume() << "\n";
    out << "Pan=" << trk->GetPan() << "\n";

    // Write note summary
    int noteCount = 0;
    for (int i = 0; i < trk->mEventCount; ++i)
    {
      if (trk->mppEvents[i] && trk->mppEvents[i]->IsKeyOn()) noteCount++;
    }
    out << "NoteCount=" << noteCount << "\n";
  }

  // 5. Embedded Automation Curves
  out << "[SECTION:AUTOMATION]\n";
  for (int t = 0; t < trackCount; ++t)
  {
    vector<JZAutomationLane*> lanes = JZAutomationManager::Instance().GetTrackLanes(t);
    for (size_t l = 0; l < lanes.size(); ++l)
    {
      JZAutomationLane* pLane = lanes[l];
      out << "AutoTrack=" << t << ";Type=" << (int)pLane->GetType() << ";NodeCount=" << pLane->GetCurve().GetNodeCount() << "\n";
      for (int n = 0; n < pLane->GetCurve().GetNodeCount(); ++n)
      {
        const JZAutomationNode& node = pLane->GetCurve().GetNode(n);
        out << "Node=" << node.clock << "," << node.value << "," << (int)node.interp << "," << node.tension << "\n";
      }
    }
  }

  out << "[END_OF_BUNDLE]\n";
  out.close();
  return true;
}

bool JZProjectBundle::ReadManifest(const string& bundleFilePath, JZBundleManifest& outManifest)
{
  ifstream in(bundleFilePath.c_str());
  if (!in.is_open()) return false;

  string line;
  getline(in, line);
  if (line.find("[JAZZBUNDLE_V1]") == string::npos)
  {
    in.close();
    return false;
  }

  while (getline(in, line))
  {
    if (line.empty() || line[0] == '[')
    {
      if (line == "[SECTION:STYLE_PRESET]") break; // Manifest done
      continue;
    }
    size_t eq = line.find('=');
    if (eq == string::npos) continue;

    string key = line.substr(0, eq);
    string val = line.substr(eq + 1);

    if (key == "ProjectName") outManifest.projectName = val;
    else if (key == "Composer") outManifest.composer = val;
    else if (key == "TempoBpm") outManifest.tempoBpm = atoi(val.c_str());
    else if (key == "TrackCount") outManifest.trackCount = atoi(val.c_str());
    else if (key == "TotalMeasures") outManifest.totalMeasures = atoi(val.c_str());
  }

  in.close();
  return true;
}

bool JZProjectBundle::ImportBundle(
  JZProject* pProject,
  const string& bundleFilePath,
  JZAccompanimentConfig* pOutArrangementConfig)
{
  if (!pProject) return false;

  ifstream in(bundleFilePath.c_str());
  if (!in.is_open()) return false;

  string line;
  getline(in, line);
  if (line.find("[JAZZBUNDLE_V1]") == string::npos)
  {
    in.close();
    return false;
  }

  string section = "MANIFEST";

  while (getline(in, line))
  {
    if (line.empty()) continue;
    if (line[0] == '[')
    {
      if (line == "[SECTION:STYLE_PRESET]") section = "STYLE";
      else if (line == "[SECTION:TRACKS]") section = "TRACKS";
      else if (line == "[SECTION:AUTOMATION]") section = "AUTOMATION";
      else if (line == "[END_OF_BUNDLE]") break;
      continue;
    }

    size_t eq = line.find('=');
    if (eq == string::npos) continue;
    string key = line.substr(0, eq);
    string val = line.substr(eq + 1);

    if (section == "STYLE" && pOutArrangementConfig)
    {
      int iVal = atoi(val.c_str());
      if (key == "style") pOutArrangementConfig->style = (JZAccompanimentStyle)iVal;
      else if (key == "swingPercent") pOutArrangementConfig->swingPercent = iVal;
      else if (key == "generateBass") pOutArrangementConfig->generateBass = (iVal != 0);
      else if (key == "generateDrums") pOutArrangementConfig->generateDrums = (iVal != 0);
      else if (key == "generatePad") pOutArrangementConfig->generatePad = (iVal != 0);
      else if (key == "reharmonizeMode") pOutArrangementConfig->reharmonizeMode = (JZReharmonizeMode)iVal;
    }
    else if (section == "AUTOMATION")
    {
      if (key == "Node")
      {
        // Parse node clk,val,interp,tension
        stringstream ss(val);
        string sc, sv, si, st;
        if (getline(ss, sc, ',') && getline(ss, sv, ',') && getline(ss, si, ',') && getline(ss, st, ','))
        {
          int clk = atoi(sc.c_str());
          double v = atof(sv.c_str());
          JZInterpolationType it = (JZInterpolationType)atoi(si.c_str());
          double tension = atof(st.c_str());
          JZAutomationLane* pLane = JZAutomationManager::Instance().AddLane(0, AUTO_VOLUME);
          if (pLane) pLane->GetCurve().AddNode(clk, v, it, tension);
        }
      }
    }
  }

  in.close();
  return true;
}
