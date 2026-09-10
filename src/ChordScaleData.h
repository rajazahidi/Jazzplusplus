//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Chord and Scale Library Definitions
//*****************************************************************************

#pragma once

#include <string>
#include <vector>

struct JZChordDefinition
{
  std::string mName;
  std::string mShortName;
  std::string mCategory;
  std::vector<int> mIntervals; // Semitone offsets from root (e.g. {0, 7} for power chord)

  std::string GetFormulaString() const;
};

struct JZScaleDefinition
{
  std::string mName;
  std::string mCategory;
  std::vector<int> mIntervals; // Semitone offsets from root (e.g. {0, 2, 4, 5, 7, 9, 11})

  std::string GetFormulaString() const;
};

class JZChordScaleLibrary
{
  public:

    static const std::vector<JZChordDefinition>& GetChords();

    static const std::vector<JZScaleDefinition>& GetScales();

    static std::vector<std::string> GetChordCategories();

    static std::vector<std::string> GetScaleCategories();

    static const std::vector<std::string>& GetRootNoteNames();

    static int NoteNameToSemitone(const std::string& Name);

    static std::string PitchToNoteName(int Pitch);
};
