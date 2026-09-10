//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Insert Chord and Scale Dialog Implementation
//*****************************************************************************

#include "InsertChordScaleDialog.h"

#include "../Events.h"
#include "../Globals.h"
#include "../Player.h"
#include "../Project.h"
#include "../ProjectManager.h"
#include "../Track.h"

#include <wx/button.h>
#include <wx/choice.h>
#include <wx/listbox.h>
#include <wx/msgdlg.h>
#include <wx/radiobox.h>
#include <wx/sizer.h>
#include <wx/spinctrl.h>
#include <wx/statbox.h>
#include <wx/stattext.h>

#include <algorithm>
#include <cstdint>
#include <sstream>

using namespace std;

enum
{
  ID_MODE_RADIO = 10001,
  ID_CATEGORY_CHOICE,
  ID_ITEM_LIST,
  ID_ROOT_CHOICE,
  ID_OCTAVE_CHOICE,
  ID_STYLE_CHOICE,
  ID_DURATION_CHOICE,
  ID_TRACK_CHOICE,
  ID_BTN_PREVIEW,
  ID_BTN_INSERT,
  ID_BTN_CLOSE
};

BEGIN_EVENT_TABLE(JZInsertChordScaleDialog, wxDialog)
  EVT_RADIOBOX(ID_MODE_RADIO, JZInsertChordScaleDialog::OnModeChange)
  EVT_CHOICE(ID_CATEGORY_CHOICE, JZInsertChordScaleDialog::OnCategoryChange)
  EVT_LISTBOX(ID_ITEM_LIST, JZInsertChordScaleDialog::OnItemSelect)
  EVT_CHOICE(ID_ROOT_CHOICE, JZInsertChordScaleDialog::OnRootOrOctaveChange)
  EVT_CHOICE(ID_OCTAVE_CHOICE, JZInsertChordScaleDialog::OnRootOrOctaveChange)
  EVT_BUTTON(ID_BTN_PREVIEW, JZInsertChordScaleDialog::OnPreviewButton)
  EVT_BUTTON(ID_BTN_INSERT, JZInsertChordScaleDialog::OnInsertButton)
  EVT_BUTTON(ID_BTN_CLOSE, JZInsertChordScaleDialog::OnCloseButton)
END_EVENT_TABLE()

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZInsertChordScaleDialog::JZInsertChordScaleDialog(
  wxWindow* pParent,
  JZProject* pProject,
  int DefaultTrack,
  int DefaultClock)
  : wxDialog(
      pParent,
      wxID_ANY,
      "Insert Chord / Scale into Track",
      wxDefaultPosition,
      wxSize(540, 520),
      wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
    mpProject(pProject),
    mTargetTrackIndex(DefaultTrack),
    mStartClock(DefaultClock),
    mpModeRadio(0),
    mpCategoryChoice(0),
    mpItemListBox(0),
    mpRootChoice(0),
    mpOctaveChoice(0),
    mpStyleChoice(0),
    mpDurationChoice(0),
    mpTrackChoice(0),
    mpVelocitySpin(0),
    mpInfoText(0),
    mpPreviewButton(0),
    mpInsertButton(0),
    mpCloseButton(0)
{
  CreateControls();
  PopulateCategories();
  PopulateItems();
  UpdateInfoLabel();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZInsertChordScaleDialog::~JZInsertChordScaleDialog()
{
  if (gpMidiPlayer)
  {
    gpMidiPlayer->AllNotesOff(true);
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZInsertChordScaleDialog::CreateControls()
{
  wxBoxSizer* pMainSizer = new wxBoxSizer(wxVERTICAL);

  // Top Mode selection (Chord vs Scale)
  wxArrayString modes;
  modes.Add("Chords (Harmonic / Power Chords)");
  modes.Add("Scales (Melodic Runs)");
  mpModeRadio = new wxRadioBox(
    this,
    ID_MODE_RADIO,
    "Type",
    wxDefaultPosition,
    wxDefaultSize,
    modes,
    2,
    wxRA_SPECIFY_COLS);
  pMainSizer->Add(mpModeRadio, 0, wxEXPAND | wxALL, 6);

  // Category and Search/Filter
  wxBoxSizer* pCatSizer = new wxBoxSizer(wxHORIZONTAL);
  pCatSizer->Add(new wxStaticText(this, wxID_ANY, "Category:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
  mpCategoryChoice = new wxChoice(this, ID_CATEGORY_CHOICE);
  pCatSizer->Add(mpCategoryChoice, 1, wxEXPAND);
  pMainSizer->Add(pCatSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);

  // List of Chords / Scales
  mpItemListBox = new wxListBox(this, ID_ITEM_LIST, wxDefaultPosition, wxSize(-1, 140), wxArrayString(), wxLB_SINGLE | wxLB_NEEDED_SB);
  pMainSizer->Add(mpItemListBox, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);

  // Parameters Grid
  wxStaticBoxSizer* pParamBox = new wxStaticBoxSizer(wxVERTICAL, this, "Music & Placement Settings");
  wxFlexGridSizer* pGrid = new wxFlexGridSizer(4, 4, 6, 8);
  pGrid->AddGrowableCol(1);
  pGrid->AddGrowableCol(3);

  // Row 1: Root Key & Octave
  pGrid->Add(new wxStaticText(this, wxID_ANY, "Root Note:"), 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
  const vector<string>& rootNames = JZChordScaleLibrary::GetRootNoteNames();
  wxArrayString roots;
  for (size_t i = 0; i < rootNames.size(); ++i)
  {
    roots.Add(rootNames[i]);
  }
  mpRootChoice = new wxChoice(this, ID_ROOT_CHOICE, wxDefaultPosition, wxDefaultSize, roots);
  mpRootChoice->SetSelection(0); // C
  pGrid->Add(mpRootChoice, 1, wxEXPAND);

  pGrid->Add(new wxStaticText(this, wxID_ANY, "Octave:"), 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
  wxArrayString octaves;
  octaves.Add("1 (Bass / Sub)");
  octaves.Add("2 (Low)");
  octaves.Add("3 (Tenor)");
  octaves.Add("4 (Middle C)");
  octaves.Add("5 (Treble)");
  octaves.Add("6 (High)");
  octaves.Add("7 (Very High)");
  mpOctaveChoice = new wxChoice(this, ID_OCTAVE_CHOICE, wxDefaultPosition, wxDefaultSize, octaves);
  mpOctaveChoice->SetSelection(3); // Octave 4
  pGrid->Add(mpOctaveChoice, 1, wxEXPAND);

  // Row 2: Target Track & Velocity
  pGrid->Add(new wxStaticText(this, wxID_ANY, "Track:"), 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
  wxArrayString tracks;
  int trackCount = mpProject ? mpProject->GetTrackCount() : 16;
  for (int t = 0; t < trackCount; ++t)
  {
    JZTrack* trk = mpProject ? mpProject->GetTrack(t) : 0;
    string name = trk ? trk->GetName() : "";
    if (name.empty())
    {
      ostringstream oss;
      oss << "Track " << t;
      name = oss.str();
    }
    ostringstream oss;
    oss << t << ": " << name;
    tracks.Add(oss.str());
  }
  mpTrackChoice = new wxChoice(this, ID_TRACK_CHOICE, wxDefaultPosition, wxDefaultSize, tracks);
  if (mTargetTrackIndex >= 0 && mTargetTrackIndex < (int)tracks.GetCount())
  {
    mpTrackChoice->SetSelection(mTargetTrackIndex);
  }
  else
  {
    mpTrackChoice->SetSelection(1 < (int)tracks.GetCount() ? 1 : 0);
  }
  pGrid->Add(mpTrackChoice, 1, wxEXPAND);

  pGrid->Add(new wxStaticText(this, wxID_ANY, "Velocity:"), 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
  mpVelocitySpin = new wxSpinCtrl(this, wxID_ANY, "90", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 127, 90);
  pGrid->Add(mpVelocitySpin, 1, wxEXPAND);

  // Row 3: Play Style & Duration
  pGrid->Add(new wxStaticText(this, wxID_ANY, "Playback Style:"), 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
  mpStyleChoice = new wxChoice(this, ID_STYLE_CHOICE);
  pGrid->Add(mpStyleChoice, 1, wxEXPAND);

  pGrid->Add(new wxStaticText(this, wxID_ANY, "Duration:"), 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
  wxArrayString durations;
  durations.Add("1 Bar (Whole Note)");
  durations.Add("1/2 Note (Half)");
  durations.Add("1/4 Note (Quarter)");
  durations.Add("1/8 Note");
  durations.Add("1/16 Note");
  mpDurationChoice = new wxChoice(this, ID_DURATION_CHOICE, wxDefaultPosition, wxDefaultSize, durations);
  mpDurationChoice->SetSelection(2); // 1/4 note
  pGrid->Add(mpDurationChoice, 1, wxEXPAND);

  pParamBox->Add(pGrid, 0, wxEXPAND | wxALL, 4);

  // Formula & Notes details label
  mpInfoText = new wxStaticText(this, wxID_ANY, "Selected: None");
  wxFont infoFont = mpInfoText->GetFont();
  infoFont.SetWeight(wxFONTWEIGHT_BOLD);
  mpInfoText->SetFont(infoFont);
  pParamBox->Add(mpInfoText, 0, wxEXPAND | wxTOP | wxLEFT | wxRIGHT, 4);

  pMainSizer->Add(pParamBox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);

  // Action Buttons
  wxBoxSizer* pBtnSizer = new wxBoxSizer(wxHORIZONTAL);
  mpPreviewButton = new wxButton(this, ID_BTN_PREVIEW, "&Preview Audio");
  pBtnSizer->Add(mpPreviewButton, 0, wxRIGHT, 6);

  pBtnSizer->AddStretchSpacer();

  mpInsertButton = new wxButton(this, ID_BTN_INSERT, "&Insert to Track");
  mpInsertButton->SetDefault();
  pBtnSizer->Add(mpInsertButton, 0, wxRIGHT, 6);

  mpCloseButton = new wxButton(this, ID_BTN_CLOSE, "&Close");
  pBtnSizer->Add(mpCloseButton, 0);

  pMainSizer->Add(pBtnSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);

  SetSizer(pMainSizer);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZInsertChordScaleDialog::PopulateCategories()
{
  mpCategoryChoice->Clear();
  mpCategoryChoice->Append("All Categories");

  bool isChord = (mpModeRadio->GetSelection() == 0);
  if (isChord)
  {
    vector<string> cats = JZChordScaleLibrary::GetChordCategories();
    for (size_t i = 0; i < cats.size(); ++i)
    {
      mpCategoryChoice->Append(cats[i].c_str());
    }

    mpStyleChoice->Clear();
    mpStyleChoice->Append("Block Chord (Simultaneous)");
    mpStyleChoice->Append("Arpeggio Up");
    mpStyleChoice->Append("Arpeggio Down");
    mpStyleChoice->Append("Arpeggio Up & Down");
    mpStyleChoice->SetSelection(0);
  }
  else
  {
    vector<string> cats = JZChordScaleLibrary::GetScaleCategories();
    for (size_t i = 0; i < cats.size(); ++i)
    {
      mpCategoryChoice->Append(cats[i].c_str());
    }

    mpStyleChoice->Clear();
    mpStyleChoice->Append("Scale Run Up (1 Octave)");
    mpStyleChoice->Append("Scale Run Down (1 Octave)");
    mpStyleChoice->Append("Scale Run Up (2 Octaves)");
    mpStyleChoice->Append("Scale Run Down (2 Octaves)");
    mpStyleChoice->Append("Scale Run Up & Down");
    mpStyleChoice->SetSelection(0);
  }
  mpCategoryChoice->SetSelection(0);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZInsertChordScaleDialog::PopulateItems()
{
  mpItemListBox->Clear();
  bool isChord = (mpModeRadio->GetSelection() == 0);
  string selectedCat = mpCategoryChoice->GetStringSelection().ToStdString();

  if (isChord)
  {
    const vector<JZChordDefinition>& chords = JZChordScaleLibrary::GetChords();
    for (size_t i = 0; i < chords.size(); ++i)
    {
      if (selectedCat == "All Categories" || chords[i].mCategory == selectedCat)
      {
        ostringstream oss;
        oss << chords[i].mName << "  [" << chords[i].GetFormulaString() << "]";
        mpItemListBox->Append(oss.str().c_str(), reinterpret_cast<void*>(static_cast<uintptr_t>(i)));
      }
    }
  }
  else
  {
    const vector<JZScaleDefinition>& scales = JZChordScaleLibrary::GetScales();
    for (size_t i = 0; i < scales.size(); ++i)
    {
      if (selectedCat == "All Categories" || scales[i].mCategory == selectedCat)
      {
        ostringstream oss;
        oss << scales[i].mName << "  [" << scales[i].GetFormulaString() << "]";
        mpItemListBox->Append(oss.str().c_str(), reinterpret_cast<void*>(static_cast<uintptr_t>(i)));
      }
    }
  }

  if (mpItemListBox->GetCount() > 0)
  {
    mpItemListBox->SetSelection(0);
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZInsertChordScaleDialog::UpdateInfoLabel()
{
  int sel = mpItemListBox->GetSelection();
  if (sel < 0)
  {
    mpInfoText->SetLabel("Selected: None");
    return;
  }

  uintptr_t idx = reinterpret_cast<uintptr_t>(mpItemListBox->GetClientData(sel));
  int rootKey = mpRootChoice->GetSelection();
  int octave = mpOctaveChoice->GetSelection() + 1;
  int basePitch = octave * 12 + rootKey;

  bool isChord = (mpModeRadio->GetSelection() == 0);
  ostringstream oss;

  if (isChord)
  {
    const vector<JZChordDefinition>& chords = JZChordScaleLibrary::GetChords();
    if (idx < chords.size())
    {
      const JZChordDefinition& chord = chords[idx];
      oss << chord.mName << " (" << chord.mCategory << ")\nNotes: ";
      for (size_t i = 0; i < chord.mIntervals.size(); ++i)
      {
        if (i > 0) oss << ", ";
        oss << JZChordScaleLibrary::PitchToNoteName(basePitch + chord.mIntervals[i]);
      }
    }
  }
  else
  {
    const vector<JZScaleDefinition>& scales = JZChordScaleLibrary::GetScales();
    if (idx < scales.size())
    {
      const JZScaleDefinition& scale = scales[idx];
      oss << scale.mName << " (" << scale.mCategory << ")\nNotes: ";
      for (size_t i = 0; i < scale.mIntervals.size(); ++i)
      {
        if (i > 0) oss << ", ";
        oss << JZChordScaleLibrary::PitchToNoteName(basePitch + scale.mIntervals[i]);
      }
    }
  }

  mpInfoText->SetLabel(oss.str());
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZInsertChordScaleDialog::OnModeChange(wxCommandEvent&)
{
  PopulateCategories();
  PopulateItems();
  UpdateInfoLabel();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZInsertChordScaleDialog::OnCategoryChange(wxCommandEvent&)
{
  PopulateItems();
  UpdateInfoLabel();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZInsertChordScaleDialog::OnItemSelect(wxCommandEvent&)
{
  UpdateInfoLabel();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZInsertChordScaleDialog::OnRootOrOctaveChange(wxCommandEvent&)
{
  UpdateInfoLabel();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZInsertChordScaleDialog::PlayPreview()
{
  if (!gpMidiPlayer)
  {
    return;
  }

  int sel = mpItemListBox->GetSelection();
  if (sel < 0)
  {
    return;
  }

  uintptr_t idx = reinterpret_cast<uintptr_t>(mpItemListBox->GetClientData(sel));
  int rootKey = mpRootChoice->GetSelection();
  int octave = mpOctaveChoice->GetSelection() + 1;
  int basePitch = octave * 12 + rootKey;
  int velocity = mpVelocitySpin->GetValue();

  int trackIdx = mpTrackChoice->GetSelection();
  JZTrack* pTrack = mpProject ? mpProject->GetTrack(trackIdx) : 0;
  int channel = pTrack ? pTrack->GetChannel() - 1 : 0;
  if (channel < 0) channel = 0;

  bool isChord = (mpModeRadio->GetSelection() == 0);
  vector<int> pitches;

  if (isChord)
  {
    const vector<JZChordDefinition>& chords = JZChordScaleLibrary::GetChords();
    if (idx < chords.size())
    {
      for (size_t i = 0; i < chords[idx].mIntervals.size(); ++i)
      {
        pitches.push_back(basePitch + chords[idx].mIntervals[i]);
      }
    }
  }
  else
  {
    const vector<JZScaleDefinition>& scales = JZChordScaleLibrary::GetScales();
    if (idx < scales.size())
    {
      for (size_t i = 0; i < scales[idx].mIntervals.size(); ++i)
      {
        pitches.push_back(basePitch + scales[idx].mIntervals[i]);
      }
    }
  }

  gpMidiPlayer->AllNotesOff(true);
  for (size_t i = 0; i < pitches.size(); ++i)
  {
    int p = pitches[i];
    if (p >= 0 && p <= 127)
    {
      JZKeyOnEvent keyOn(0, channel, p, velocity, 500);
      gpMidiPlayer->OutNow(pTrack, &keyOn);
    }
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZInsertChordScaleDialog::ExecuteInsert()
{
  if (!mpProject)
  {
    return;
  }

  int sel = mpItemListBox->GetSelection();
  if (sel < 0)
  {
    wxMessageBox("Please select a chord or scale first.", "Insert", wxOK | wxICON_INFORMATION, this);
    return;
  }

  int trackIdx = mpTrackChoice->GetSelection();
  JZTrack* pTrack = mpProject->GetTrack(trackIdx);
  if (!pTrack)
  {
    wxMessageBox("Selected track is invalid.", "Error", wxOK | wxICON_ERROR, this);
    return;
  }

  uintptr_t idx = reinterpret_cast<uintptr_t>(mpItemListBox->GetClientData(sel));
  int rootKey = mpRootChoice->GetSelection();
  int octave = mpOctaveChoice->GetSelection() + 1;
  int basePitch = octave * 12 + rootKey;
  int velocity = mpVelocitySpin->GetValue();
  int channel = pTrack->GetChannel() - 1;
  if (channel < 0) channel = 0;

  // Calculate note length in clocks
  int tpq = mpProject->GetTicksPerQuarter();
  if (tpq <= 0) tpq = 120;

  int durationSel = mpDurationChoice->GetSelection();
  int noteClocks = tpq; // Default quarter note
  switch (durationSel)
  {
    case 0: noteClocks = tpq * 4; break; // Whole note / 1 bar
    case 1: noteClocks = tpq * 2; break; // Half note
    case 2: noteClocks = tpq; break;     // Quarter note
    case 3: noteClocks = tpq / 2; break; // 8th note
    case 4: noteClocks = tpq / 4; break; // 16th note
    default: noteClocks = tpq; break;
  }
  if (noteClocks < 4) noteClocks = 4;

  bool isChord = (mpModeRadio->GetSelection() == 0);
  int styleSel = mpStyleChoice->GetSelection();

  vector<int> noteSequence;

  if (isChord)
  {
    const vector<JZChordDefinition>& chords = JZChordScaleLibrary::GetChords();
    if (idx >= chords.size()) return;
    const vector<int>& ivs = chords[idx].mIntervals;

    if (styleSel == 0) // Block Chord
    {
      // Insert simultaneous notes
      mpProject->NewUndoBuffer();
      for (size_t i = 0; i < ivs.size(); ++i)
      {
        int p = basePitch + ivs[i];
        if (p >= 0 && p <= 127)
        {
          JZKeyOnEvent* pEvent = new JZKeyOnEvent(mStartClock, channel, p, velocity, noteClocks);
          pTrack->Put(pEvent);
        }
      }
      pTrack->Cleanup();
      mStartClock += noteClocks;
      JZProjectManager::Instance()->UpdateAllViews();
      wxString msg = wxString::Format("Chord inserted successfully into %s.", pTrack->GetName());
      wxMessageBox(msg, "Insert Chord", wxOK | wxICON_INFORMATION, this);
      return;
    }
    else if (styleSel == 1) // Arpeggio Up
    {
      noteSequence = ivs;
    }
    else if (styleSel == 2) // Arpeggio Down
    {
      noteSequence = ivs;
      reverse(noteSequence.begin(), noteSequence.end());
    }
    else // Arpeggio Up & Down
    {
      noteSequence = ivs;
      for (int i = (int)ivs.size() - 2; i > 0; --i)
      {
        noteSequence.push_back(ivs[i]);
      }
    }
  }
  else
  {
    const vector<JZScaleDefinition>& scales = JZChordScaleLibrary::GetScales();
    if (idx >= scales.size()) return;
    const vector<int>& ivs = scales[idx].mIntervals;

    if (styleSel == 0) // Scale Run Up (1 Octave)
    {
      noteSequence = ivs;
      noteSequence.push_back(12); // Add octave note
    }
    else if (styleSel == 1) // Scale Run Down (1 Octave)
    {
      noteSequence = ivs;
      noteSequence.push_back(12);
      reverse(noteSequence.begin(), noteSequence.end());
    }
    else if (styleSel == 2) // Scale Run Up (2 Octaves)
    {
      noteSequence = ivs;
      for (size_t i = 0; i < ivs.size(); ++i)
      {
        noteSequence.push_back(ivs[i] + 12);
      }
      noteSequence.push_back(24);
    }
    else if (styleSel == 3) // Scale Run Down (2 Octaves)
    {
      noteSequence = ivs;
      for (size_t i = 0; i < ivs.size(); ++i)
      {
        noteSequence.push_back(ivs[i] + 12);
      }
      noteSequence.push_back(24);
      reverse(noteSequence.begin(), noteSequence.end());
    }
    else // Scale Run Up & Down
    {
      noteSequence = ivs;
      noteSequence.push_back(12);
      for (int i = (int)ivs.size() - 1; i > 0; --i)
      {
        noteSequence.push_back(ivs[i]);
      }
    }
  }

  // Insert sequential notes (Arpeggio or Scale Run)
  mpProject->NewUndoBuffer();
  int currentClock = mStartClock;
  for (size_t i = 0; i < noteSequence.size(); ++i)
  {
    int p = basePitch + noteSequence[i];
    if (p >= 0 && p <= 127)
    {
      JZKeyOnEvent* pEvent = new JZKeyOnEvent(currentClock, channel, p, velocity, noteClocks);
      pTrack->Put(pEvent);
    }
    currentClock += noteClocks;
  }
  pTrack->Cleanup();
  mStartClock = currentClock;

  JZProjectManager::Instance()->UpdateAllViews();
  wxString msg = wxString::Format("Notes inserted successfully into %s.", pTrack->GetName());
  wxMessageBox(msg, "Insert", wxOK | wxICON_INFORMATION, this);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZInsertChordScaleDialog::OnPreviewButton(wxCommandEvent&)
{
  PlayPreview();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZInsertChordScaleDialog::OnInsertButton(wxCommandEvent&)
{
  ExecuteInsert();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZInsertChordScaleDialog::OnCloseButton(wxCommandEvent&)
{
  Close();
}
