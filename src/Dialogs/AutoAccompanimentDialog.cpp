//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Auto-Accompaniment & Smart Music Intelligence Dialog Implementation
//*****************************************************************************

#include "AutoAccompanimentDialog.h"
#include "../Project.h"
#include "../Song.h"
#include "../Track.h"
#include "../Globals.h"
#include "../Player.h"
#include "../TrackWindow.h"

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/notebook.h>
#include <wx/panel.h>
#include <wx/radiobut.h>
#include <wx/sizer.h>
#include <wx/slider.h>
#include <wx/spinctrl.h>
#include <wx/statbox.h>
#include <wx/stattext.h>

#include <sstream>

using namespace std;

enum
{
  ID_BTN_GENERATE = wxID_HIGHEST + 301,
  ID_BTN_AUDITION,
  ID_BTN_STOP_AUDITION,
  ID_BTN_EXPORT_WAV,
  ID_RANGE_ALL_RADIO,
  ID_RANGE_CUSTOM_RADIO,
  ID_NOTEBOOK_HUB,
  ID_SIMULTANEOUS_CHECK
};

BEGIN_EVENT_TABLE(JZAutoAccompanimentDialog, wxDialog)
  EVT_BUTTON(ID_BTN_GENERATE, JZAutoAccompanimentDialog::OnGenerateButton)
  EVT_BUTTON(ID_BTN_AUDITION, JZAutoAccompanimentDialog::OnAuditionPreview)
  EVT_BUTTON(ID_BTN_STOP_AUDITION, JZAutoAccompanimentDialog::OnStopPreview)
  EVT_BUTTON(ID_BTN_EXPORT_WAV, JZAutoAccompanimentDialog::OnExportWav)
  EVT_BUTTON(wxID_CANCEL, JZAutoAccompanimentDialog::OnCancelButton)
  EVT_RADIOBUTTON(ID_RANGE_ALL_RADIO, JZAutoAccompanimentDialog::OnRangeRadio)
  EVT_RADIOBUTTON(ID_RANGE_CUSTOM_RADIO, JZAutoAccompanimentDialog::OnRangeRadio)
  EVT_NOTEBOOK_PAGE_CHANGED(ID_NOTEBOOK_HUB, JZAutoAccompanimentDialog::OnNotebookPageChanged)
  EVT_CHECKBOX(ID_SIMULTANEOUS_CHECK, JZAutoAccompanimentDialog::OnSimultaneousCheck)
END_EVENT_TABLE()

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZAutoAccompanimentDialog::JZAutoAccompanimentDialog(
  wxWindow* pParent,
  JZProject* pProject,
  int defaultSourceTrack)
  : wxDialog(
      pParent,
      wxID_ANY,
      "Smart Auto-Accompaniment & Session Band Hub",
      wxDefaultPosition,
      wxDefaultSize,
      wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
    mpProject(pProject),
    mDefaultSourceTrack(defaultSourceTrack),
    mIsAuditioning(false),
    mpSourceTrackChoice(0),
    mpKeyCenterLabel(0),
    mpRangeAllRadio(0),
    mpRangeCustomRadio(0),
    mpFromBarSpin(0),
    mpToBarSpin(0),
    mpNotebook(0),
    mpBandStyleChoice(0),
    mpSectionChoice(0),
    mpBassFeelChoice(0),
    mpBassCheckBox(0),
    mpDrumsCheckBox(0),
    mpPadCheckBox(0),
    mpStrumStyleChoice(0),
    mpStrumTrackChoice(0),
    mpStrumPatchChoice(0),
    mpStrumSpeedSlider(0),
    mpRiffStyleChoice(0),
    mpRiffTrackChoice(0),
    mpRiffPatchChoice(0),
    mpRiffOctaveSpin(0),
    mpSoloStyleChoice(0),
    mpSoloDensityChoice(0),
    mpSoloTrackChoice(0),
    mpSoloPatchChoice(0),
    mpSoloOctaveSpin(0),
    mpSwingSlider(0),
    mpDrumFillsCheckBox(0),
    mpSoloBendsCheckBox(0),
    mpHumanizeVelCheckBox(0),
    mpSmoothVoiceLeadingCheckBox(0),
    mpGhostNotesCheckBox(0),
    mpMicroPocketCheckBox(0),
    mpScaleLockCheckBox(0),
    mpMixerPresetsCheckBox(0),
    mpOverwriteCheckBox(0),
    mpSimultaneousCheckBox(0),
    mpAuditionButton(0),
    mpStopAuditionButton(0),
    mpExportWavButton(0),
    mpGenerateButton(0),
    mpCancelButton(0)
{
  CreateControls();
  SetMinSize(FromDIP(wxSize(560, 680)));
  Fit();
  CentreOnParent();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZAutoAccompanimentDialog::~JZAutoAccompanimentDialog()
{
  StopAudition();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZAutoAccompanimentDialog::StopAudition()
{
  if (gpMidiPlayer)
  {
    gpMidiPlayer->AllNotesOff(true);
  }
  mIsAuditioning = false;
  if (mpStopAuditionButton)
  {
    mpStopAuditionButton->Enable(false);
  }
  if (mpAuditionButton)
  {
    mpAuditionButton->Enable(true);
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZAutoAccompanimentDialog::CreateControls()
{
  wxBoxSizer* pMainSizer = new wxBoxSizer(wxVERTICAL);

  // Top Section: Source Track & Measure Range
  wxBoxSizer* pTopRow = new wxBoxSizer(wxHORIZONTAL);

  // Source Track Box
  wxStaticBoxSizer* pSourceBox = new wxStaticBoxSizer(wxVERTICAL, this, "Source Track (Piano / Lead)");
  mpSourceTrackChoice = new wxChoice(this, wxID_ANY);

  int maxTracks = mpProject ? min(mpProject->GetTrackCount(), 32) : 16;
  for (int i = 0; i < maxTracks; ++i)
  {
    ostringstream oss;
    oss << "Track " << (i + 1);
    if (mpProject)
    {
      JZTrack* t = mpProject->GetTrack(i);
      if (t && t->GetName() && t->GetName()[0] != '\0')
      {
        oss << " (" << t->GetName() << ")";
      }
      else if (i == 0)
      {
        oss << " (Piano Lead)";
      }
    }
    mpSourceTrackChoice->Append(oss.str());
  }
  mpSourceTrackChoice->SetSelection((mDefaultSourceTrack >= 0 && mDefaultSourceTrack < maxTracks) ? mDefaultSourceTrack : 0);
  pSourceBox->Add(mpSourceTrackChoice, 0, wxALL | wxEXPAND, FromDIP(4));

  JZKeyCenter detectedKey;
  if (mpProject)
  {
    JZTrack* src = mpProject->GetTrack(mDefaultSourceTrack);
    if (src) detectedKey = JZAutoAccompaniment::DetectSongKeyCenter(src);
  }
  wxString keyText = wxString::Format("Key Center: %s (%d%% match)", detectedKey.name.c_str(), (int)(detectedKey.confidence * 100));
  mpKeyCenterLabel = new wxStaticText(this, wxID_ANY, keyText);
  pSourceBox->Add(mpKeyCenterLabel, 0, wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(4));

  pTopRow->Add(pSourceBox, 1, wxALL | wxEXPAND, FromDIP(5));

  // Measure Range Box
  wxStaticBoxSizer* pRangeBox = new wxStaticBoxSizer(wxVERTICAL, this, "Measure Range");
  mpRangeAllRadio = new wxRadioButton(this, ID_RANGE_ALL_RADIO, "Entire Song", wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
  mpRangeAllRadio->SetValue(true);
  pRangeBox->Add(mpRangeAllRadio, 0, wxALL, FromDIP(2));

  wxBoxSizer* pCustomRangeSizer = new wxBoxSizer(wxHORIZONTAL);
  mpRangeCustomRadio = new wxRadioButton(this, ID_RANGE_CUSTOM_RADIO, "Bars:");
  pCustomRangeSizer->Add(mpRangeCustomRadio, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(4));

  mpFromBarSpin = new wxSpinCtrl(this, wxID_ANY, "1", wxDefaultPosition, FromDIP(wxSize(60, -1)));
  mpFromBarSpin->SetRange(1, 999);
  mpFromBarSpin->Enable(false);
  pCustomRangeSizer->Add(mpFromBarSpin, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(6));

  pCustomRangeSizer->Add(new wxStaticText(this, wxID_ANY, "to"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(6));

  int songLastBar = 16;
  if (mpProject)
  {
    int lastClock = mpProject->GetLastClock();
    if (lastClock > 0)
    {
      JZBarInfo bi(*mpProject);
      bi.SetClock(lastClock);
      songLastBar = max(1, bi.GetBarIndex() + 1);
    }
  }

  mpToBarSpin = new wxSpinCtrl(this, wxID_ANY, wxString::Format("%d", songLastBar), wxDefaultPosition, FromDIP(wxSize(60, -1)));
  mpToBarSpin->SetRange(1, 999);
  mpToBarSpin->Enable(false);
  pCustomRangeSizer->Add(mpToBarSpin, 0, wxALIGN_CENTER_VERTICAL, 0);

  pRangeBox->Add(pCustomRangeSizer, 0, wxALL, FromDIP(2));
  pTopRow->Add(pRangeBox, 1, wxALL | wxEXPAND, FromDIP(5));

  pMainSizer->Add(pTopRow, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, FromDIP(5));

  // Notebook Tabs
  mpNotebook = new wxNotebook(this, ID_NOTEBOOK_HUB);
  mpNotebook->AddPage(CreateBandTab(mpNotebook), "🥁 Session Band", true);
  mpNotebook->AddPage(CreateStrumPluckTab(mpNotebook), "🎸 Strum & Pluck", false);
  mpNotebook->AddPage(CreateRiffTab(mpNotebook), "⚡ Riffs & Hooks", false);
  mpNotebook->AddPage(CreateSoloTab(mpNotebook), "🎷 Auto-Solo", false);

  pMainSizer->Add(mpNotebook, 1, wxEXPAND | wxALL, FromDIP(6));

  // Musical Expression & Groove Box
  wxStaticBoxSizer* pGrooveBox = new wxStaticBoxSizer(wxVERTICAL, this, "Groove & Musical Expression");

  wxBoxSizer* pSwingRow = new wxBoxSizer(wxHORIZONTAL);
  pSwingRow->Add(new wxStaticText(this, wxID_ANY, "Swing Ratio: Straight (50%)"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(6));
  mpSwingSlider = new wxSlider(this, wxID_ANY, 62, 50, 70, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_AUTOTICKS);
  pSwingRow->Add(mpSwingSlider, 1, wxEXPAND | wxRIGHT, FromDIP(6));
  pSwingRow->Add(new wxStaticText(this, wxID_ANY, "Hard (70%)"), 0, wxALIGN_CENTER_VERTICAL);
  pGrooveBox->Add(pSwingRow, 0, wxALL | wxEXPAND, FromDIP(4));

  wxBoxSizer* pExprRow = new wxBoxSizer(wxHORIZONTAL);
  mpDrumFillsCheckBox = new wxCheckBox(this, wxID_ANY, "Smart Drum Fills (Bar 4/8/12/16)");
  mpDrumFillsCheckBox->SetValue(true);
  pExprRow->Add(mpDrumFillsCheckBox, 1, wxRIGHT, FromDIP(8));

  mpSoloBendsCheckBox = new wxCheckBox(this, wxID_ANY, "Expressive Solo Bends & CC1 Vibrato");
  mpSoloBendsCheckBox->SetValue(true);
  pExprRow->Add(mpSoloBendsCheckBox, 1, 0, 0);
  pGrooveBox->Add(pExprRow, 0, wxALL | wxEXPAND, FromDIP(4));

  wxBoxSizer* pAdvRow = new wxBoxSizer(wxHORIZONTAL);
  mpSmoothVoiceLeadingCheckBox = new wxCheckBox(this, wxID_ANY, "Smooth Voice Leading (Minimal Leaps)");
  mpSmoothVoiceLeadingCheckBox->SetValue(true);
  pAdvRow->Add(mpSmoothVoiceLeadingCheckBox, 1, wxRIGHT, FromDIP(8));

  mpGhostNotesCheckBox = new wxCheckBox(this, wxID_ANY, "Bass Ghost Notes & Articulations");
  mpGhostNotesCheckBox->SetValue(true);
  pAdvRow->Add(mpGhostNotesCheckBox, 1, 0, 0);
  pGrooveBox->Add(pAdvRow, 0, wxALL | wxEXPAND, FromDIP(4));

  wxBoxSizer* pPocketRow = new wxBoxSizer(wxHORIZONTAL);
  mpMicroPocketCheckBox = new wxCheckBox(this, wxID_ANY, "Micro-Timing Pocket (Laid-Back Snare & Push Bass)");
  mpMicroPocketCheckBox->SetValue(true);
  pPocketRow->Add(mpMicroPocketCheckBox, 1, wxRIGHT, FromDIP(8));

  mpScaleLockCheckBox = new wxCheckBox(this, wxID_ANY, "Smart Scale Lock (Snap-to-Key)");
  mpScaleLockCheckBox->SetValue(false);
  pPocketRow->Add(mpScaleLockCheckBox, 1, 0, 0);
  pGrooveBox->Add(pPocketRow, 0, wxALL | wxEXPAND, FromDIP(4));

  wxBoxSizer* pMixRow = new wxBoxSizer(wxHORIZONTAL);
  mpMixerPresetsCheckBox = new wxCheckBox(this, wxID_ANY, "Apply Smart Auto-Mixer Studio Presets (Stereo Pan, Levels & FX)");
  mpMixerPresetsCheckBox->SetValue(true);
  pMixRow->Add(mpMixerPresetsCheckBox, 1, 0, 0);
  pGrooveBox->Add(pMixRow, 0, wxALL | wxEXPAND, FromDIP(4));

  pMainSizer->Add(pGrooveBox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(6));

  // Bottom Options & Preview
  wxBoxSizer* pOptSizer = new wxBoxSizer(wxHORIZONTAL);

  wxBoxSizer* pCheckSizer = new wxBoxSizer(wxVERTICAL);
  mpOverwriteCheckBox = new wxCheckBox(this, wxID_ANY, "Overwrite existing notes in target tracks");
  mpOverwriteCheckBox->SetValue(true);
  pCheckSizer->Add(mpOverwriteCheckBox, 0, wxBOTTOM, FromDIP(3));

  mpSimultaneousCheckBox = new wxCheckBox(this, ID_SIMULTANEOUS_CHECK, "Generate all tab layers simultaneously (Full Band + Guitar + Riff + Solo)");
  mpSimultaneousCheckBox->SetValue(false);
  pCheckSizer->Add(mpSimultaneousCheckBox, 0, 0, 0);
  pOptSizer->Add(pCheckSizer, 1, wxEXPAND, 0);

  // Audition Preview Buttons
  wxBoxSizer* pAuditionSizer = new wxBoxSizer(wxHORIZONTAL);
  mpAuditionButton = new wxButton(this, ID_BTN_AUDITION, "▶ Preview");
  pAuditionSizer->Add(mpAuditionButton, 0, wxRIGHT, FromDIP(4));

  mpStopAuditionButton = new wxButton(this, ID_BTN_STOP_AUDITION, "■ Stop");
  mpStopAuditionButton->Enable(false);
  pAuditionSizer->Add(mpStopAuditionButton, 0, 0, 0);
  pOptSizer->Add(pAuditionSizer, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(10));

  pMainSizer->Add(pOptSizer, 0, wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(8));

  // Final Action Buttons
  wxBoxSizer* pBtnSizer = new wxBoxSizer(wxHORIZONTAL);
  mpExportWavButton = new wxButton(this, ID_BTN_EXPORT_WAV, "🎧 Export Audio WAV...");
  pBtnSizer->Add(mpExportWavButton, 0, wxRIGHT, FromDIP(12));

  mpGenerateButton = new wxButton(this, ID_BTN_GENERATE, "Generate Session Band");
  mpGenerateButton->SetDefault();
  pBtnSizer->Add(mpGenerateButton, 0, wxRIGHT, FromDIP(10));

  mpCancelButton = new wxButton(this, wxID_CANCEL, "Close");
  pBtnSizer->Add(mpCancelButton, 0, 0, 0);

  pMainSizer->Add(pBtnSizer, 0, wxALIGN_RIGHT | wxALL, FromDIP(10));

  SetSizer(pMainSizer);
}

//-----------------------------------------------------------------------------
// Tab 1: Full Band Accompaniment
//-----------------------------------------------------------------------------
wxPanel* JZAutoAccompanimentDialog::CreateBandTab(wxWindow* pParent)
{
  wxPanel* pPanel = new wxPanel(pParent);
  wxBoxSizer* pSizer = new wxBoxSizer(wxVERTICAL);

  wxStaticBoxSizer* pStyleBox = new wxStaticBoxSizer(wxVERTICAL, pPanel, "Accompaniment Musical Style");
  mpBandStyleChoice = new wxChoice(pPanel, wxID_ANY);
  for (int s = 0; s < STYLE_COUNT; ++s)
  {
    mpBandStyleChoice->Append(JZAutoAccompaniment::GetStyleName((JZAccompanimentStyle)s));
  }
  mpBandStyleChoice->SetSelection(STYLE_JAZZ_SWING);
  pStyleBox->Add(mpBandStyleChoice, 0, wxALL | wxEXPAND, FromDIP(5));
  pSizer->Add(pStyleBox, 0, wxALL | wxEXPAND, FromDIP(6));

  wxStaticBoxSizer* pInstBox = new wxStaticBoxSizer(wxVERTICAL, pPanel, "Instruments to Generate");
  mpBassCheckBox = new wxCheckBox(pPanel, wxID_ANY, "Auto Bass (Track 2, Channel 2 - Walking / Groove)");
  mpBassCheckBox->SetValue(true);
  pInstBox->Add(mpBassCheckBox, 0, wxALL, FromDIP(4));

  mpDrumsCheckBox = new wxCheckBox(pPanel, wxID_ANY, "Auto Drums (Track 10, Channel 10 - Standard Drum Kit)");
  mpDrumsCheckBox->SetValue(true);
  pInstBox->Add(mpDrumsCheckBox, 0, wxALL, FromDIP(4));

  mpPadCheckBox = new wxCheckBox(pPanel, wxID_ANY, "Auto Strings / Pad (Track 3, Channel 3 - Warm Chords)");
  mpPadCheckBox->SetValue(true);
  pInstBox->Add(mpPadCheckBox, 0, wxALL, FromDIP(4));

  pSizer->Add(pInstBox, 0, wxALL | wxEXPAND, FromDIP(6));

  wxStaticBoxSizer* pFormBox = new wxStaticBoxSizer(wxVERTICAL, pPanel, "Song Form & Dynamic Arrangement");
  wxFlexGridSizer* pFormGrid = new wxFlexGridSizer(2, 2, FromDIP(6), FromDIP(10));
  pFormGrid->AddGrowableCol(1);

  pFormGrid->Add(new wxStaticText(pPanel, wxID_ANY, "Song Section:"), 0, wxALIGN_CENTER_VERTICAL);
  mpSectionChoice = new wxChoice(pPanel, wxID_ANY);
  for (int s = 0; s < SECTION_COUNT; ++s)
  {
    mpSectionChoice->Append(JZAutoAccompaniment::GetSectionName((JZSongSection)s));
  }
  mpSectionChoice->SetSelection(SECTION_AUTO);
  pFormGrid->Add(mpSectionChoice, 1, wxEXPAND);

  pFormGrid->Add(new wxStaticText(pPanel, wxID_ANY, "Bass Articulation:"), 0, wxALIGN_CENTER_VERTICAL);
  mpBassFeelChoice = new wxChoice(pPanel, wxID_ANY);
  for (int f = 0; f < 3; ++f)
  {
    mpBassFeelChoice->Append(JZAutoAccompaniment::GetBassFeelName((JZBassFeel)f));
  }
  mpBassFeelChoice->SetSelection(BASS_FEEL_AUTO);
  pFormGrid->Add(mpBassFeelChoice, 1, wxEXPAND);

  pFormBox->Add(pFormGrid, 0, wxALL | wxEXPAND, FromDIP(5));
  pSizer->Add(pFormBox, 0, wxALL | wxEXPAND, FromDIP(6));
  pPanel->SetSizer(pSizer);
  return pPanel;
}

//-----------------------------------------------------------------------------
// Tab 2: Strumming & Plucking
//-----------------------------------------------------------------------------
wxPanel* JZAutoAccompanimentDialog::CreateStrumPluckTab(wxWindow* pParent)
{
  wxPanel* pPanel = new wxPanel(pParent);
  wxBoxSizer* pSizer = new wxBoxSizer(wxVERTICAL);

  wxStaticBoxSizer* pStyleBox = new wxStaticBoxSizer(wxVERTICAL, pPanel, "Strumming & Plucking Style");
  mpStrumStyleChoice = new wxChoice(pPanel, wxID_ANY);
  for (int s = 0; s < STRUM_PLUCK_COUNT; ++s)
  {
    mpStrumStyleChoice->Append(JZAutoAccompaniment::GetStrumPluckStyleName((JZStrumPluckStyle)s));
  }
  mpStrumStyleChoice->SetSelection(STRUM_ACOUSTIC_FOLK);
  pStyleBox->Add(mpStrumStyleChoice, 0, wxALL | wxEXPAND, FromDIP(5));
  pSizer->Add(pStyleBox, 0, wxALL | wxEXPAND, FromDIP(6));

  wxStaticBoxSizer* pTargetBox = new wxStaticBoxSizer(wxVERTICAL, pPanel, "Target Track & Sound");
  wxFlexGridSizer* pGrid = new wxFlexGridSizer(2, 2, FromDIP(6), FromDIP(10));
  pGrid->AddGrowableCol(1);

  pGrid->Add(new wxStaticText(pPanel, wxID_ANY, "Destination Track:"), 0, wxALIGN_CENTER_VERTICAL);
  mpStrumTrackChoice = new wxChoice(pPanel, wxID_ANY);
  for (int i = 0; i < 16; ++i)
  {
    mpStrumTrackChoice->Append(wxString::Format("Track %d", i + 1));
  }
  mpStrumTrackChoice->SetSelection(3); // Track 4
  pGrid->Add(mpStrumTrackChoice, 1, wxEXPAND);

  pGrid->Add(new wxStaticText(pPanel, wxID_ANY, "Guitar Sound / Patch:"), 0, wxALIGN_CENTER_VERTICAL);
  mpStrumPatchChoice = new wxChoice(pPanel, wxID_ANY);
  mpStrumPatchChoice->Append("Acoustic Steel Guitar (Patch 26)");
  mpStrumPatchChoice->Append("Acoustic Nylon Guitar (Patch 25)");
  mpStrumPatchChoice->Append("Electric Clean Guitar (Patch 28)");
  mpStrumPatchChoice->Append("Electric Jazz Guitar (Patch 27)");
  mpStrumPatchChoice->Append("Electric Piano / Rhodes (Patch 5)");
  mpStrumPatchChoice->SetSelection(0);
  pGrid->Add(mpStrumPatchChoice, 1, wxEXPAND);

  pTargetBox->Add(pGrid, 0, wxALL | wxEXPAND, FromDIP(5));
  pSizer->Add(pTargetBox, 0, wxALL | wxEXPAND, FromDIP(6));

  wxStaticBoxSizer* pSpeedBox = new wxStaticBoxSizer(wxVERTICAL, pPanel, "Strum Speed & Humanization");
  wxBoxSizer* pSpeedRow = new wxBoxSizer(wxHORIZONTAL);
  pSpeedRow->Add(new wxStaticText(pPanel, wxID_ANY, "Fast (10ms)"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(6));
  mpStrumSpeedSlider = new wxSlider(pPanel, wxID_ANY, 18, 10, 35, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_AUTOTICKS);
  pSpeedRow->Add(mpStrumSpeedSlider, 1, wxEXPAND | wxRIGHT, FromDIP(6));
  pSpeedRow->Add(new wxStaticText(pPanel, wxID_ANY, "Relaxed (35ms)"), 0, wxALIGN_CENTER_VERTICAL);
  pSpeedBox->Add(pSpeedRow, 0, wxALL | wxEXPAND, FromDIP(5));
  pSizer->Add(pSpeedBox, 0, wxALL | wxEXPAND, FromDIP(6));

  pPanel->SetSizer(pSizer);
  return pPanel;
}

//-----------------------------------------------------------------------------
// Tab 3: Riffs & Hooks
//-----------------------------------------------------------------------------
wxPanel* JZAutoAccompanimentDialog::CreateRiffTab(wxWindow* pParent)
{
  wxPanel* pPanel = new wxPanel(pParent);
  wxBoxSizer* pSizer = new wxBoxSizer(wxVERTICAL);

  wxStaticBoxSizer* pStyleBox = new wxStaticBoxSizer(wxVERTICAL, pPanel, "Riff / Hook Style");
  mpRiffStyleChoice = new wxChoice(pPanel, wxID_ANY);
  for (int s = 0; s < RIFF_COUNT; ++s)
  {
    mpRiffStyleChoice->Append(JZAutoAccompaniment::GetRiffStyleName((JZRiffStyle)s));
  }
  mpRiffStyleChoice->SetSelection(RIFF_BLUES_ROCK);
  pStyleBox->Add(mpRiffStyleChoice, 0, wxALL | wxEXPAND, FromDIP(5));
  pSizer->Add(pStyleBox, 0, wxALL | wxEXPAND, FromDIP(6));

  wxStaticBoxSizer* pTargetBox = new wxStaticBoxSizer(wxVERTICAL, pPanel, "Target Track & Sound");
  wxFlexGridSizer* pGrid = new wxFlexGridSizer(3, 2, FromDIP(6), FromDIP(10));
  pGrid->AddGrowableCol(1);

  pGrid->Add(new wxStaticText(pPanel, wxID_ANY, "Destination Track:"), 0, wxALIGN_CENTER_VERTICAL);
  mpRiffTrackChoice = new wxChoice(pPanel, wxID_ANY);
  for (int i = 0; i < 16; ++i)
  {
    mpRiffTrackChoice->Append(wxString::Format("Track %d", i + 1));
  }
  mpRiffTrackChoice->SetSelection(4); // Track 5
  pGrid->Add(mpRiffTrackChoice, 1, wxEXPAND);

  pGrid->Add(new wxStaticText(pPanel, wxID_ANY, "Instrument Sound:"), 0, wxALIGN_CENTER_VERTICAL);
  mpRiffPatchChoice = new wxChoice(pPanel, wxID_ANY);
  mpRiffPatchChoice->Append("Overdriven Guitar (Patch 30)");
  mpRiffPatchChoice->Append("Brass Section Stabs (Patch 62)");
  mpRiffPatchChoice->Append("Synth Bass 1 (Patch 39)");
  mpRiffPatchChoice->Append("Distortion Guitar (Patch 31)");
  mpRiffPatchChoice->Append("Trumpet Solo (Patch 57)");
  mpRiffPatchChoice->SetSelection(0);
  pGrid->Add(mpRiffPatchChoice, 1, wxEXPAND);

  pGrid->Add(new wxStaticText(pPanel, wxID_ANY, "Base Octave:"), 0, wxALIGN_CENTER_VERTICAL);
  mpRiffOctaveSpin = new wxSpinCtrl(pPanel, wxID_ANY, "3", wxDefaultPosition, FromDIP(wxSize(70, -1)));
  mpRiffOctaveSpin->SetRange(2, 5);
  pGrid->Add(mpRiffOctaveSpin, 0);

  pTargetBox->Add(pGrid, 0, wxALL | wxEXPAND, FromDIP(5));
  pSizer->Add(pTargetBox, 0, wxALL | wxEXPAND, FromDIP(6));

  pPanel->SetSizer(pSizer);
  return pPanel;
}

//-----------------------------------------------------------------------------
// Tab 4: Auto-Solo Lead
//-----------------------------------------------------------------------------
wxPanel* JZAutoAccompanimentDialog::CreateSoloTab(wxWindow* pParent)
{
  wxPanel* pPanel = new wxPanel(pParent);
  wxBoxSizer* pSizer = new wxBoxSizer(wxVERTICAL);

  wxStaticBoxSizer* pStyleBox = new wxStaticBoxSizer(wxVERTICAL, pPanel, "Solo Improvisation Style");
  mpSoloStyleChoice = new wxChoice(pPanel, wxID_ANY);
  for (int s = 0; s < SOLO_COUNT; ++s)
  {
    mpSoloStyleChoice->Append(JZAutoAccompaniment::GetSoloStyleName((JZSoloStyle)s));
  }
  mpSoloStyleChoice->SetSelection(SOLO_MELODIC_LYRICAL);
  pStyleBox->Add(mpSoloStyleChoice, 0, wxALL | wxEXPAND, FromDIP(5));
  pSizer->Add(pStyleBox, 0, wxALL | wxEXPAND, FromDIP(6));

  wxStaticBoxSizer* pDensityBox = new wxStaticBoxSizer(wxVERTICAL, pPanel, "Phrasing Density & Breath");
  mpSoloDensityChoice = new wxChoice(pPanel, wxID_ANY);
  mpSoloDensityChoice->Append("Sparse / Soulful (Singing quarter notes, spacious rests)");
  mpSoloDensityChoice->Append("Balanced Phrasing (2-Measure Call-and-Response)");
  mpSoloDensityChoice->Append("Virtuoso (Dense 8th/16th runs, turnarounds, enclosure)");
  mpSoloDensityChoice->SetSelection(1);
  pDensityBox->Add(mpSoloDensityChoice, 0, wxALL | wxEXPAND, FromDIP(5));
  pSizer->Add(pDensityBox, 0, wxALL | wxEXPAND, FromDIP(6));

  wxStaticBoxSizer* pTargetBox = new wxStaticBoxSizer(wxVERTICAL, pPanel, "Target Track & Sound");
  wxFlexGridSizer* pGrid = new wxFlexGridSizer(3, 2, FromDIP(6), FromDIP(10));
  pGrid->AddGrowableCol(1);

  pGrid->Add(new wxStaticText(pPanel, wxID_ANY, "Destination Track:"), 0, wxALIGN_CENTER_VERTICAL);
  mpSoloTrackChoice = new wxChoice(pPanel, wxID_ANY);
  for (int i = 0; i < 16; ++i)
  {
    mpSoloTrackChoice->Append(wxString::Format("Track %d", i + 1));
  }
  mpSoloTrackChoice->SetSelection(5); // Track 6
  pGrid->Add(mpSoloTrackChoice, 1, wxEXPAND);

  pGrid->Add(new wxStaticText(pPanel, wxID_ANY, "Solo Instrument:"), 0, wxALIGN_CENTER_VERTICAL);
  mpSoloPatchChoice = new wxChoice(pPanel, wxID_ANY);
  mpSoloPatchChoice->Append("Alto Sax (Patch 66)");
  mpSoloPatchChoice->Append("Tenor Sax (Patch 67)");
  mpSoloPatchChoice->Append("Trumpet (Patch 57)");
  mpSoloPatchChoice->Append("Electric Lead Guitar (Patch 30)");
  mpSoloPatchChoice->Append("Lead Synth (Square) (Patch 81)");
  mpSoloPatchChoice->Append("Flute (Patch 74)");
  mpSoloPatchChoice->SetSelection(0);
  pGrid->Add(mpSoloPatchChoice, 1, wxEXPAND);

  pGrid->Add(new wxStaticText(pPanel, wxID_ANY, "Lead Octave:"), 0, wxALIGN_CENTER_VERTICAL);
  mpSoloOctaveSpin = new wxSpinCtrl(pPanel, wxID_ANY, "5", wxDefaultPosition, FromDIP(wxSize(70, -1)));
  mpSoloOctaveSpin->SetRange(4, 6);
  pGrid->Add(mpSoloOctaveSpin, 0);

  pTargetBox->Add(pGrid, 0, wxALL | wxEXPAND, FromDIP(5));
  pSizer->Add(pTargetBox, 0, wxALL | wxEXPAND, FromDIP(6));

  pPanel->SetSizer(pSizer);
  return pPanel;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZAutoAccompanimentDialog::UpdateGenerateButtonLabel()
{
  if (!mpGenerateButton || !mpNotebook)
  {
    return;
  }

  if (mpSimultaneousCheckBox && mpSimultaneousCheckBox->GetValue())
  {
    mpGenerateButton->SetLabel("Generate All Layers Simultaneously");
    return;
  }

  int page = mpNotebook->GetSelection();
  switch (page)
  {
    case 0: mpGenerateButton->SetLabel("Generate Session Band"); break;
    case 1: mpGenerateButton->SetLabel("Generate Strum / Pluck"); break;
    case 2: mpGenerateButton->SetLabel("Generate Riff / Hook"); break;
    case 3: mpGenerateButton->SetLabel("Generate Auto-Solo"); break;
    default: mpGenerateButton->SetLabel("Generate Selected"); break;
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZAutoAccompanimentDialog::OnNotebookPageChanged(wxBookCtrlEvent& Event)
{
  StopAudition();
  UpdateGenerateButtonLabel();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZAutoAccompanimentDialog::OnRangeRadio(wxCommandEvent& Event)
{
  bool isCustom = mpRangeCustomRadio->GetValue();
  mpFromBarSpin->Enable(isCustom);
  mpToBarSpin->Enable(isCustom);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZAutoAccompanimentDialog::OnSimultaneousCheck(wxCommandEvent& Event)
{
  UpdateGenerateButtonLabel();
}

//-----------------------------------------------------------------------------
// Audition / Live Preview
//-----------------------------------------------------------------------------
void JZAutoAccompanimentDialog::OnAuditionPreview(wxCommandEvent& Event)
{
  if (!gpMidiPlayer)
  {
    wxMessageBox("MIDI Output device is not available for preview.", "Preview", wxOK | wxICON_INFORMATION, this);
    return;
  }

  StopAudition();
  mIsAuditioning = true;
  mpAuditionButton->Enable(false);
  mpStopAuditionButton->Enable(true);

  // Play a representative 2-bar preview chord / groove on track
  int page = mpNotebook->GetSelection();
  JZTrack dummyTrack;

  if (page == 0) // Band Preview: C Major / G7 preview
  {
    // Bass note
    JZKeyOnEvent bass1(0, 1, 36 /* C2 */, 95, 400);
    gpMidiPlayer->OutNow(&dummyTrack, &bass1);

    // Pad chord
    JZKeyOnEvent pad1(0, 2, 60 /* C4 */, 70, 700);
    JZKeyOnEvent pad2(0, 2, 64 /* E4 */, 66, 700);
    JZKeyOnEvent pad3(0, 2, 67 /* G4 */, 68, 700);
    gpMidiPlayer->OutNow(&dummyTrack, &pad1);
    gpMidiPlayer->OutNow(&dummyTrack, &pad2);
    gpMidiPlayer->OutNow(&dummyTrack, &pad3);

    // Drum ride
    JZKeyOnEvent ride(0, 9, 51 /* Ride */, 88, 100);
    gpMidiPlayer->OutNow(&dummyTrack, &ride);
  }
  else if (page == 1) // Strum Preview
  {
    // Guitar steel string chord strum
    int strumSpeed = mpStrumSpeedSlider->GetValue();
    int delay = max(2, (120 * strumSpeed) / 500);
    int pitches[] = {48, 55, 60, 64, 67}; // C Major guitar chord
    for (int k = 0; k < 5; ++k)
    {
      JZKeyOnEvent str(k * delay, 3, (unsigned char)pitches[k], (unsigned char)(90 - k * 2), 600);
      gpMidiPlayer->OutNow(&dummyTrack, &str);
    }
  }
  else if (page == 2) // Riff Preview
  {
    int oct = mpRiffOctaveSpin->GetValue();
    int root = oct * 12; // C
    JZKeyOnEvent r1(0, 4, (unsigned char)root, 105, 120);
    JZKeyOnEvent r2(120, 4, (unsigned char)(root + 3), 98, 120);
    JZKeyOnEvent r3(240, 4, (unsigned char)(root + 5), 102, 120);
    JZKeyOnEvent r4(360, 4, (unsigned char)(root + 6), 108, 250); // Blue note
    gpMidiPlayer->OutNow(&dummyTrack, &r1);
    gpMidiPlayer->OutNow(&dummyTrack, &r2);
    gpMidiPlayer->OutNow(&dummyTrack, &r3);
    gpMidiPlayer->OutNow(&dummyTrack, &r4);
  }
  else if (page == 3) // Solo Preview
  {
    int oct = mpSoloOctaveSpin->GetValue();
    int root = oct * 12; // C
    // Melodic phrase with vibrato
    JZKeyOnEvent s1(0, 5, (unsigned char)(root + 4), 95, 180);
    JZKeyOnEvent s2(180, 5, (unsigned char)(root + 7), 100, 180);
    JZKeyOnEvent s3(360, 5, (unsigned char)(root + 12), 110, 500);
    gpMidiPlayer->OutNow(&dummyTrack, &s1);
    gpMidiPlayer->OutNow(&dummyTrack, &s2);
    gpMidiPlayer->OutNow(&dummyTrack, &s3);

    // CC 1 Vibrato
    JZControlEvent vib(480, 5, 1, 55);
    gpMidiPlayer->OutNow(&dummyTrack, &vib);
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZAutoAccompanimentDialog::OnStopPreview(wxCommandEvent& Event)
{
  StopAudition();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZAutoAccompanimentDialog::OnCancelButton(wxCommandEvent& Event)
{
  StopAudition();
  EndModal(wxID_CANCEL);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZAutoAccompanimentDialog::OnExportWav(wxCommandEvent& Event)
{
  StopAudition();

  if (!mpProject || !mpProject->GetSong())
  {
    wxMessageBox("No active song project to export.", "Export Error", wxOK | wxICON_ERROR, this);
    return;
  }

  wxFileDialog saveDlg(
    this,
    "Export Direct Audio WAV Mixdown",
    "",
    "JazzAccompanimentMix.wav",
    "WAV Audio Files (*.wav)|*.wav",
    wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

  if (saveDlg.ShowModal() != wxID_OK)
  {
    return;
  }

  string path = saveDlg.GetPath().ToStdString();
  bool success = JZMidiAudioRenderer::ExportWav(mpProject->GetSong(), path);

  if (success)
  {
    wxMessageBox(
      wxString::Format("WAV audio mixdown successfully exported to:\n%s", saveDlg.GetPath().c_str()),
      "Audio Export Complete",
      wxOK | wxICON_INFORMATION,
      this);
  }
  else
  {
    wxMessageBox("Failed to export audio mixdown. Please check directory permissions.", "Export Failed", wxOK | wxICON_ERROR, this);
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZAutoAccompanimentDialog::OnGenerateButton(wxCommandEvent& Event)
{
  StopAudition();

  if (!mpProject)
  {
    wxMessageBox("No active project found.", "Error", wxOK | wxICON_ERROR, this);
    return;
  }

  mConfig.sourceTrackIndex = mpSourceTrackChoice->GetSelection();
  mConfig.overwriteExisting = mpOverwriteCheckBox->GetValue();

  // Range
  if (mpRangeCustomRadio->GetValue())
  {
    int fromB = mpFromBarSpin->GetValue() - 1;
    int toB = mpToBarSpin->GetValue();
    if (toB <= fromB)
    {
      wxMessageBox("End measure must be greater than or equal to start measure.", "Invalid Range", wxOK | wxICON_WARNING, this);
      return;
    }
    mConfig.fromBar = fromB;
    mConfig.toBar = toB;
  }
  else
  {
    mConfig.fromBar = 0;
    mConfig.toBar = -1;
  }

  // Musicality Enhancements
  mConfig.swingPercent = mpSwingSlider->GetValue();
  mConfig.enableDrumFills = mpDrumFillsCheckBox->GetValue();
  mConfig.enableSoloBendsAndVibrato = mpSoloBendsCheckBox->GetValue();
  mConfig.enableHumanizeVelocity = true;
  if (mpSmoothVoiceLeadingCheckBox) mConfig.enableSmoothVoiceLeading = mpSmoothVoiceLeadingCheckBox->GetValue();
  if (mpGhostNotesCheckBox) mConfig.enableGhostNotes = mpGhostNotesCheckBox->GetValue();
  if (mpMicroPocketCheckBox) mConfig.enableMicroPocket = mpMicroPocketCheckBox->GetValue();
  if (mpScaleLockCheckBox) mConfig.enableScaleLock = mpScaleLockCheckBox->GetValue();
  if (mpMixerPresetsCheckBox) mConfig.applyMixerPresets = mpMixerPresetsCheckBox->GetValue();
  if (mpSectionChoice) mConfig.songSection = (JZSongSection)mpSectionChoice->GetSelection();
  if (mpBassFeelChoice) mConfig.bassFeel = (JZBassFeel)mpBassFeelChoice->GetSelection();

  // Determine mode
  if (mpSimultaneousCheckBox->GetValue())
  {
    mConfig.mode = MODE_ALL_SIMULTANEOUS;
  }
  else
  {
    int selPage = mpNotebook->GetSelection();
    switch (selPage)
    {
      case 0: mConfig.mode = MODE_FULL_BAND; break;
      case 1: mConfig.mode = MODE_STRUM_PLUCK; break;
      case 2: mConfig.mode = MODE_RIFF_MAKER; break;
      case 3: mConfig.mode = MODE_AUTO_SOLO; break;
      default: mConfig.mode = MODE_FULL_BAND; break;
    }
  }

  // Tab 1 (Band)
  mConfig.style = (JZAccompanimentStyle)mpBandStyleChoice->GetSelection();
  mConfig.generateBass = mpBassCheckBox->GetValue();
  mConfig.generateDrums = mpDrumsCheckBox->GetValue();
  mConfig.generatePad = mpPadCheckBox->GetValue();

  // Tab 2 (Strum & Pluck)
  mConfig.strumPluck.style = (JZStrumPluckStyle)mpStrumStyleChoice->GetSelection();
  mConfig.strumPluck.targetTrackIndex = mpStrumTrackChoice->GetSelection();
  static const int sStrumPatches[] = {26, 25, 28, 27, 5};
  int sPatchIdx = mpStrumPatchChoice->GetSelection();
  mConfig.strumPluck.patchNumber = sStrumPatches[max(0, min(4, sPatchIdx))];
  mConfig.strumPluck.strumSpeedMs = mpStrumSpeedSlider->GetValue();

  // Tab 3 (Riffs & Hooks)
  mConfig.riff.style = (JZRiffStyle)mpRiffStyleChoice->GetSelection();
  mConfig.riff.targetTrackIndex = mpRiffTrackChoice->GetSelection();
  static const int sRiffPatches[] = {30, 62, 39, 31, 57};
  int rPatchIdx = mpRiffPatchChoice->GetSelection();
  mConfig.riff.patchNumber = sRiffPatches[max(0, min(4, rPatchIdx))];
  mConfig.riff.octave = mpRiffOctaveSpin->GetValue();

  // Tab 4 (Auto-Solo)
  mConfig.solo.style = (JZSoloStyle)mpSoloStyleChoice->GetSelection();
  mConfig.solo.density = (JZSoloDensity)mpSoloDensityChoice->GetSelection();
  mConfig.solo.targetTrackIndex = mpSoloTrackChoice->GetSelection();
  static const int sSoloPatches[] = {66, 67, 57, 30, 81, 74};
  int solPatchIdx = mpSoloPatchChoice->GetSelection();
  mConfig.solo.patchNumber = sSoloPatches[max(0, min(5, solPatchIdx))];
  mConfig.solo.octave = mpSoloOctaveSpin->GetValue();

  // Save undo point
  mpProject->NewUndoBuffer();

  // Execute generation
  JZAccompanimentResult res = JZAutoAccompaniment::Generate(mpProject, mConfig);

  if (!res.success)
  {
    wxMessageBox(res.message, "Smart Music Intelligence", wxOK | wxICON_WARNING, this);
    return;
  }

  // Refresh windows
  if (gpTrackWindow)
  {
    gpTrackWindow->Refresh();
  }

  wxMessageBox(
    wxString(res.message.c_str(), wxConvUTF8),
    "Generation Completed Successfully",
    wxOK | wxICON_INFORMATION,
    this);

  EndModal(wxID_OK);
}
