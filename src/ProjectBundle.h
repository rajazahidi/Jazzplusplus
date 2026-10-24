//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Self-Contained Project Bundle & Asset Archiver (.jazzbundle)
//
// Bundles Project Song Data, Tracks, Arrangement Style Presets,
// Automation Curves, Waveforms & Mixer Settings into a Single Unified Archive.
//*****************************************************************************

#pragma once

#include <string>
#include <vector>
#include <map>

class JZProject;
class JZSong;
struct JZAccompanimentConfig;

struct JZBundleManifest
{
  std::string projectName;
  std::string composer;
  int tempoBpm;
  std::string keySignature;
  int trackCount;
  int totalMeasures;
  std::string creationTimestamp;
  std::vector<std::string> assetFilenames;

  JZBundleManifest()
    : projectName("Untitled Project"),
      composer("Jazz++ Producer"),
      tempoBpm(120),
      keySignature("C Major"),
      trackCount(0),
      totalMeasures(0)
  {
  }
};

class JZProjectBundle
{
  public:

    // Export entire active project to a self-contained .jazzbundle file
    static bool ExportBundle(
      JZProject* pProject,
      const std::string& bundleFilePath,
      const JZAccompanimentConfig* pArrangementConfig = 0);

    // Import a .jazzbundle file and restore project tracks, presets, and automation
    static bool ImportBundle(
      JZProject* pProject,
      const std::string& bundleFilePath,
      JZAccompanimentConfig* pOutArrangementConfig = 0);

    // Read only the manifest info without loading entire project
    static bool ReadManifest(
      const std::string& bundleFilePath,
      JZBundleManifest& outManifest);
};
