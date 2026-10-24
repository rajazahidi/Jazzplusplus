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
#include <wx/dirdlg.h>
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
  ID_BTN_EXPORT_STEMS,
  ID_BTN_EXPORT_LEADSHEET,
  ID_BTN_INJECT_PROGRESSION,
  ID_BTN_SAVE_PRESET,
  ID_BTN_LOAD_PRESET,
  ID_BTN_HARMONIZER,
  ID_RANGE_ALL_RADIO,
  ID_RANGE_CUSTOM_RADIO,
  ID_NOTEBOOK_HUB,
  ID_SIMULTANEOUS_CHECK,
  ID_CHORD_PAD_BASE
};

BEGIN_EVENT_TABLE(JZAutoAccompanimentDialog, wxDialog)
  EVT_BUTTON(ID_BTN_GENERATE, JZAutoAccompanimentDialog::OnGenerateButton)
  EVT_BUTTON(ID_BTN_AUDITION, JZAutoAccompanimentDialog::OnAuditionPreview)
  EVT_BUTTON(ID_BTN_STOP_AUDITION, JZAutoAccompanimentDialog::OnStopPreview)
  EVT_BUTTON(ID_BTN_EXPORT_WAV, JZAutoAccompanimentDialog::OnExportWav)
  EVT_BUTTON(ID_BTN_EXPORT_STEMS, JZAutoAccompanimentDialog::OnExportStems)
  EVT_BUTTON(ID_BTN_EXPORT_LEADSHEET, JZAutoAccompanimentDialog::OnExportLeadSheet)
  EVT_BUTTON(ID_BTN_INJECT_PROGRESSION, JZAutoAccompanimentDialog::OnInjectProgression)
  EVT_BUTTON(ID_BTN_SAVE_PRESET, JZAutoAccompanimentDialog::OnSavePreset)
  EVT_BUTTON(ID_BTN_LOAD_PRESET, JZAutoAccompanimentDialog::OnLoadPreset)
  EVT_BUTTON(ID_BTN_HARMONIZER, JZAutoAccompanimentDialog::OnHarmonizerButton)
  EVT_BUTTON(wxID_CANCEL, JZAutoAccompanimentDialog::OnCancelButton)
  EVT_COMMAND_RANGE(ID_CHORD_PAD_BASE, ID_CHORD_PAD_BASE + 7, wxEVT_COMMAND_BUTTON_CLICKED, JZAutoAccompanimentDialog::OnChordPadClicked)
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
    mpProgressionChoice(0),
    mpInjectProgressionButton(0),
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
    mpVelocityCurveChoice(0),
    mpOutroRitardandoCheckBox(0),
    mpMicrotonalCheckBox(0),
    mpMicrotonalChoice(0),
    mpMixerPresetsCheckBox(0),
    mpReharmonizeChoice(0),
    mpAudioClickTrackCheckBox(0),
    mpCountInCheckBox(0),
    mpCountInBarsSpin(0),
    mpOverwriteCheckBox(0),
    mpSimultaneousCheckBox(0),
    mpAuditionButton(0),
    mpStopAuditionButton(0),
    mpHarmonizerButton(0),
    mpSavePresetButton(0),
    mpLoadPresetButton(0),
    mpExportLeadSheetButton(0),
    mpExportStemsButton(0),
    mpExportWavButton(0),
    mpGenerateButton(0),
    mpCancelButton(0)
{
  for (int p = 0; p < 8; ++p) mpChordPadButtons[p] = 0;
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

  // Song Starter: 1-Click Chord Progression Generator Box
  wxStaticBoxSizer* pProgBox = new wxStaticBoxSizer(wxHORIZONTAL, this, "Song Starter: 1-Click Chord Progression Builder");
  mpProgressionChoice = new wxChoice(this, wxID_ANY);
  for (int p = 0; p < PROGRESSION_COUNT; ++p)
  {
    mpProgressionChoice->Append(JZAutoAccompaniment::GetProgressionName((JZChordProgressionPreset)p));
  }
  mpProgressionChoice->SetSelection(0);
  pProgBox->Add(mpProgressionChoice, 1, wxALIGN_CENTER_VERTICAL | wxALL, FromDIP(4));

  mpInjectProgressionButton = new wxButton(this, ID_BTN_INJECT_PROGRESSION, "🎹 Inject Chords to Track 1");
  pProgBox->Add(mpInjectProgressionButton, 0, wxALIGN_CENTER_VERTICAL | wxALL, FromDIP(4));
  pMainSizer->Add(pProgBox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(6));

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

  wxBoxSizer* pVelCurveRow = new wxBoxSizer(wxHORIZONTAL);
  pVelCurveRow->Add(new wxStaticText(this, wxID_ANY, "Velocity Dynamic Response:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(8));
  mpVelocityCurveChoice = new wxChoice(this, wxID_ANY);
  for (int vc = 0; vc < VELOCITY_CURVE_COUNT; ++vc)
  {
    mpVelocityCurveChoice->Append(JZAutoAccompaniment::GetVelocityCurveName((JZVelocityCurve)vc));
  }
  mpVelocityCurveChoice->SetSelection(0);
  pVelCurveRow->Add(mpVelocityCurveChoice, 1, wxEXPAND);
  pGrooveBox->Add(pVelCurveRow, 0, wxALL | wxEXPAND, FromDIP(4));

  wxBoxSizer* pRitMicroRow = new wxBoxSizer(wxHORIZONTAL);
  mpOutroRitardandoCheckBox = new wxCheckBox(this, wxID_ANY, "Outro Ritardando (Slow Cadence)");
  mpOutroRitardandoCheckBox->SetValue(true);
  pRitMicroRow->Add(mpOutroRitardandoCheckBox, 1, wxRIGHT, FromDIP(8));

  mpMicrotonalCheckBox = new wxCheckBox(this, wxID_ANY, "Microtonal Eastern / Tradisi Tuning");
  mpMicrotonalCheckBox->SetValue(false);
  pRitMicroRow->Add(mpMicrotonalCheckBox, 1, 0, 0);
  pGrooveBox->Add(pRitMicroRow, 0, wxALL | wxEXPAND, FromDIP(4));

  // Re-Harmonizer & Jazz Substitution Dropdown
  wxBoxSizer* pReharmRow = new wxBoxSizer(wxHORIZONTAL);
  pReharmRow->Add(new wxStaticText(this, wxID_ANY, "Re-Harmonizer & Substitutions:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(8));
  mpReharmonizeChoice = new wxChoice(this, wxID_ANY);
  for (int rm = 0; rm < REHARM_COUNT; ++rm)
  {
    mpReharmonizeChoice->Append(JZAutoAccompaniment::GetReharmonizeModeName((JZReharmonizeMode)rm));
  }
  mpReharmonizeChoice->SetSelection(0);
  pReharmRow->Add(mpReharmonizeChoice, 1, wxEXPAND);
  pGrooveBox->Add(pReharmRow, 0, wxALL | wxEXPAND, FromDIP(4));

  // Studio Metronome Audio Click Track & Count-in
  wxBoxSizer* pClickRow = new wxBoxSizer(wxHORIZONTAL);
  mpAudioClickTrackCheckBox = new wxCheckBox(this, wxID_ANY, "Woodblock Click Track (900Hz/580Hz)");
  mpAudioClickTrackCheckBox->SetValue(false);
  pClickRow->Add(mpAudioClickTrackCheckBox, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(12));

  mpCountInCheckBox = new wxCheckBox(this, wxID_ANY, "Count-in:");
  mpCountInCheckBox->SetValue(false);
  pClickRow->Add(mpCountInCheckBox, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(4));

  mpCountInBarsSpin = new wxSpinCtrl(this, wxID_ANY, "1", wxDefaultPosition, FromDIP(wxSize(50, -1)), wxSP_ARROW_KEYS, 1, 4, 1);
  pClickRow->Add(mpCountInBarsSpin, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(4));
  pClickRow->Add(new wxStaticText(this, wxID_ANY, "bar(s)"), 0, wxALIGN_CENTER_VERTICAL);
  pGrooveBox->Add(pClickRow, 0, wxALL | wxEXPAND, FromDIP(4));

  pMainSizer->Add(pGrooveBox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(6));

  // Interactive Virtual Chord Pads Matrix (8 Diatonic Pads)
  wxStaticBoxSizer* pChordPadsBox = new wxStaticBoxSizer(wxVERTICAL, this, "Interactive Chord Pads Matrix (Instant Live Audition)");
  wxBoxSizer* pPadsGrid = new wxBoxSizer(wxHORIZONTAL);
  static const char* sPadLabels[8] = {
    "I (Tonic)", "ii (Supertonic)", "iii (Mediant)", "IV (Subdominant)",
    "V7 (Dominant)", "vi (Submediant)", "vii° (Leading)", "bII7 (Tritone Sub)"
  };
  for (int pi = 0; pi < 8; ++pi)
  {
    mpChordPadButtons[pi] = new wxButton(this, ID_CHORD_PAD_BASE + pi, sPadLabels[pi]);
    pPadsGrid->Add(mpChordPadButtons[pi], 1, wxALL | wxEXPAND, FromDIP(2));
  }
  pChordPadsBox->Add(pPadsGrid, 0, wxEXPAND | wxALL, FromDIP(2));
  pMainSizer->Add(pChordPadsBox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(6));

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
  mpSavePresetButton = new wxButton(this, ID_BTN_SAVE_PRESET, "💾 Save...");
  pBtnSizer->Add(mpSavePresetButton, 0, wxRIGHT, FromDIP(6));

  mpLoadPresetButton = new wxButton(this, ID_BTN_LOAD_PRESET, "📂 Load...");
  pBtnSizer->Add(mpLoadPresetButton, 0, wxRIGHT, FromDIP(6));

  mpHarmonizerButton = new wxButton(this, ID_BTN_HARMONIZER, "🎺 Harmonizer...");
  pBtnSizer->Add(mpHarmonizerButton, 0, wxRIGHT, FromDIP(6));

  mpExportLeadSheetButton = new wxButton(this, ID_BTN_EXPORT_LEADSHEET, "📄 Lead Sheet...");
  pBtnSizer->Add(mpExportLeadSheetButton, 0, wxRIGHT, FromDIP(6));

  mpExportStemsButton = new wxButton(this, ID_BTN_EXPORT_STEMS, "📦 Stems (WAV)...");
  pBtnSizer->Add(mpExportStemsButton, 0, wxRIGHT, FromDIP(6));

  mpExportWavButton = new wxButton(this, ID_BTN_EXPORT_WAV, "🎧 Mix (WAV)...");
  pBtnSizer->Add(mpExportWavButton, 0, wxRIGHT, FromDIP(10));

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
  bool clickTrack = mpAudioClickTrackCheckBox ? mpAudioClickTrackCheckBox->GetValue() : false;
  bool countIn = mpCountInCheckBox ? mpCountInCheckBox->GetValue() : false;
  int countBars = mpCountInBarsSpin ? mpCountInBarsSpin->GetValue() : 1;
  bool success = JZMidiAudioRenderer::ExportWav(mpProject->GetSong(), path, 44100, 0, -1, clickTrack, countIn, countBars);

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
void JZAutoAccompanimentDialog::OnExportStems(wxCommandEvent& Event)
{
  StopAudition();
  if (!mpProject || !mpProject->GetSong())
  {
    wxMessageBox("No active song project to export.", "Export Error", wxOK | wxICON_ERROR, this);
    return;
  }

  wxDirDialog dirDlg(
    this,
    "Choose Destination Folder for Multi-Track Audio Stems",
    "",
    wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);

  if (dirDlg.ShowModal() != wxID_OK)
  {
    return;
  }

  string folder = dirDlg.GetPath().ToStdString();
  vector<string> files;
  bool clickTrack = mpAudioClickTrackCheckBox ? mpAudioClickTrackCheckBox->GetValue() : false;
  bool ok = JZMidiAudioRenderer::ExportStems(mpProject->GetSong(), folder, files, 44100, 0, -1, clickTrack);

  if (ok && !files.empty())
  {
    ostringstream oss;
    oss << "Successfully exported " << files.size() << " multi-track stems to folder:\n" << folder << "\n\nFiles generated:\n";
    for (size_t i = 0; i < files.size(); ++i)
    {
      size_t slash = files[i].find_last_of("/\\");
      string fn = (slash != string::npos) ? files[i].substr(slash + 1) : files[i];
      oss << " • " << fn << "\n";
    }
    wxMessageBox(wxString(oss.str().c_str(), wxConvUTF8), "Multi-Track Stems Exported", wxOK | wxICON_INFORMATION, this);
  }
  else
  {
    wxMessageBox("No active tracks found to export as stems.", "Export Warning", wxOK | wxICON_WARNING, this);
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZAutoAccompanimentDialog::OnExportLeadSheet(wxCommandEvent& Event)
{
  if (!mpProject || !mpProject->GetSong())
  {
    wxMessageBox("No active song project to export.", "Export Error", wxOK | wxICON_ERROR, this);
    return;
  }

  wxFileDialog saveDlg(
    this,
    "Export Printable Musician's Lead Sheet (HTML)",
    "",
    "JazzLeadSheet.html",
    "HTML Files (*.html;*.htm)|*.html;*.htm",
    wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

  if (saveDlg.ShowModal() != wxID_OK)
  {
    return;
  }

  string path = saveDlg.GetPath().ToStdString();
  string title = "Jazz Project Lead Sheet";
  if (mpProject->GetTrack(0) && mpProject->GetTrack(0)->GetName())
  {
    string tn = mpProject->GetTrack(0)->GetName();
    if (!tn.empty()) title = tn;
  }

  bool ok = JZAutoAccompaniment::ExportLeadSheetHtml(mpProject->GetSong(), path, title, "Jazz++ Arranger");
  if (ok)
  {
    wxMessageBox(
      wxString::Format("Musician's Lead Sheet successfully generated at:\n%s\n\nYou can open this in any web browser to view, format, or print (PDF).", saveDlg.GetPath().c_str()),
      "Lead Sheet Export Complete",
      wxOK | wxICON_INFORMATION,
      this);
  }
  else
  {
    wxMessageBox("Failed to write HTML lead sheet file.", "Export Error", wxOK | wxICON_ERROR, this);
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZAutoAccompanimentDialog::OnInjectProgression(wxCommandEvent& Event)
{
  if (!mpProject || !mpProject->GetSong())
  {
    wxMessageBox("No active song project.", "Error", wxOK | wxICON_ERROR, this);
    return;
  }

  int sel = mpProgressionChoice->GetSelection();
  if (sel < 0) sel = 0;
  JZChordProgressionPreset preset = (JZChordProgressionPreset)sel;

  mpProject->NewUndoBuffer();

  int targetTrack = mpSourceTrackChoice->GetSelection();
  if (targetTrack < 0) targetTrack = 0;

  int tonic = 0;
  JZTrack* pSrc = mpProject->GetTrack(targetTrack);
  if (pSrc && !pSrc->IsEmpty())
  {
    JZKeyCenter kc = JZAutoAccompaniment::DetectSongKeyCenter(pSrc);
    if (kc.confidence > 0.3f) tonic = kc.tonicPc;
  }

  int notes = JZAutoAccompaniment::GenerateChordProgression(
    mpProject->GetSong(),
    targetTrack,
    preset,
    tonic,
    0,
    2,
    false);

  if (gpTrackWindow) gpTrackWindow->Refresh();

  JZKeyCenter updatedKey = JZAutoAccompaniment::DetectSongKeyCenter(mpProject->GetTrack(targetTrack));
  mpKeyCenterLabel->SetLabel(wxString::Format("Key Center: %s (%d%% match)", updatedKey.name.c_str(), (int)(updatedKey.confidence * 100)));

  wxMessageBox(
    wxString::Format("Successfully injected %d chord notes for:\n'%s'\ninto Track %d!\n\nYou can now click 'Generate Session Band' to build accompaniment.",
      notes,
      JZAutoAccompaniment::GetProgressionName(preset),
      targetTrack + 1),
    "Song Starter Injected",
    wxOK | wxICON_INFORMATION,
    this);
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
  if (mpVelocityCurveChoice) mConfig.velocityCurve = (JZVelocityCurve)mpVelocityCurveChoice->GetSelection();
  if (mpOutroRitardandoCheckBox) mConfig.enableOutroRitardando = mpOutroRitardandoCheckBox->GetValue();
  if (mpMicrotonalCheckBox)
  {
    mConfig.enableMicrotonalTuning = mpMicrotonalCheckBox->GetValue();
    mConfig.microtonalTuning = (mConfig.style == STYLE_NUSANTARA_ZAPIN || mConfig.style == STYLE_NUSANTARA_INANG)
                                ? MICROTONAL_BAYATI_RAST
                                : (mConfig.style == STYLE_NUSANTARA_JOGET ? MICROTONAL_SLENDRO_PELOG : MICROTONAL_BAYATI_RAST);
  }
  if (mpMixerPresetsCheckBox) mConfig.applyMixerPresets = mpMixerPresetsCheckBox->GetValue();
  if (mpReharmonizeChoice) mConfig.reharmonizeMode = (JZReharmonizeMode)mpReharmonizeChoice->GetSelection();
  if (mpAudioClickTrackCheckBox) mConfig.enableAudioClickTrack = mpAudioClickTrackCheckBox->GetValue();
  if (mpCountInCheckBox) mConfig.enableCountIn = mpCountInCheckBox->GetValue();
  if (mpCountInBarsSpin) mConfig.countInBars = mpCountInBarsSpin->GetValue();
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

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZAutoAccompanimentDialog::OnChordPadClicked(wxCommandEvent& Event)
{
  int padIndex = Event.GetId() - ID_CHORD_PAD_BASE;
  if (padIndex < 0 || padIndex >= 8) return;

  int tonic = 0;
  bool isMinor = false;
  if (mpProject && mpProject->GetSong() && mpProject->GetSong()->GetTrack(0))
  {
    JZKeyCenter kc = JZAutoAccompaniment::DetectSongKeyCenter(mpProject->GetSong()->GetTrack(0));
    tonic = kc.tonicPc;
    isMinor = kc.isMinor;
  }

  JZDetectedChord ch = JZAutoAccompaniment::GetChordPadChord(tonic, isMinor, padIndex);
  static const char* sPadsNames[8] = { "I (Tonic)", "ii (Supertonic)", "iii (Mediant)", "IV (Subdominant)",
                                       "V7 (Dominant)", "vi (Submediant)", "vii° (Leading)", "bII7 (Tritone Sub)" };
  if (mpKeyCenterLabel)
  {
    wxString msg;
    msg.Printf("Pad %d: %s (%s)", padIndex + 1, sPadsNames[padIndex], JZAutoAccompaniment::GetPitchClassName(ch.rootPitchClass));
    mpKeyCenterLabel->SetLabel(msg);
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZAutoAccompanimentDialog::OnSavePreset(wxCommandEvent& Event)
{
  wxFileDialog dlg(
    this,
    "Save Arrangement Preset & Style Template",
    "",
    "ArrangementStyle.jazzstyle",
    "Jazz++ Style Preset (*.jazzstyle)|*.jazzstyle",
    wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

  if (dlg.ShowModal() != wxID_OK) return;

  if (mpReharmonizeChoice) mConfig.reharmonizeMode = (JZReharmonizeMode)mpReharmonizeChoice->GetSelection();
  if (mpAudioClickTrackCheckBox) mConfig.enableAudioClickTrack = mpAudioClickTrackCheckBox->GetValue();
  if (mpCountInCheckBox) mConfig.enableCountIn = mpCountInCheckBox->GetValue();
  if (mpCountInBarsSpin) mConfig.countInBars = mpCountInBarsSpin->GetValue();
  if (mpSwingSlider) mConfig.swingPercent = mpSwingSlider->GetValue();
  if (mpDrumFillsCheckBox) mConfig.enableDrumFills = mpDrumFillsCheckBox->GetValue();
  if (mpSoloBendsCheckBox) mConfig.enableSoloBendsAndVibrato = mpSoloBendsCheckBox->GetValue();
  if (mpSmoothVoiceLeadingCheckBox) mConfig.enableSmoothVoiceLeading = mpSmoothVoiceLeadingCheckBox->GetValue();
  if (mpGhostNotesCheckBox) mConfig.enableGhostNotes = mpGhostNotesCheckBox->GetValue();
  if (mpMicroPocketCheckBox) mConfig.enableMicroPocket = mpMicroPocketCheckBox->GetValue();
  if (mpScaleLockCheckBox) mConfig.enableScaleLock = mpScaleLockCheckBox->GetValue();

  string path = dlg.GetPath().ToStdString();
  if (JZAutoAccompaniment::SaveArrangementPreset(mConfig, path))
  {
    wxMessageBox("Arrangement preset saved successfully!", "Preset Saved", wxOK | wxICON_INFORMATION, this);
  }
  else
  {
    wxMessageBox("Failed to save arrangement preset.", "Save Error", wxOK | wxICON_ERROR, this);
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZAutoAccompanimentDialog::OnLoadPreset(wxCommandEvent& Event)
{
  wxFileDialog dlg(
    this,
    "Load Arrangement Preset & Style Template",
    "",
    "",
    "Jazz++ Style Preset (*.jazzstyle)|*.jazzstyle",
    wxFD_OPEN | wxFD_FILE_MUST_EXIST);

  if (dlg.ShowModal() != wxID_OK) return;

  string path = dlg.GetPath().ToStdString();
  if (JZAutoAccompaniment::LoadArrangementPreset(mConfig, path))
  {
    if (mpReharmonizeChoice) mpReharmonizeChoice->SetSelection((int)mConfig.reharmonizeMode);
    if (mpAudioClickTrackCheckBox) mpAudioClickTrackCheckBox->SetValue(mConfig.enableAudioClickTrack);
    if (mpCountInCheckBox) mpCountInCheckBox->SetValue(mConfig.enableCountIn);
    if (mpCountInBarsSpin) mpCountInBarsSpin->SetValue(mConfig.countInBars);
    if (mpSwingSlider) mpSwingSlider->SetValue(mConfig.swingPercent);
    if (mpDrumFillsCheckBox) mpDrumFillsCheckBox->SetValue(mConfig.enableDrumFills);
    if (mpSoloBendsCheckBox) mpSoloBendsCheckBox->SetValue(mConfig.enableSoloBendsAndVibrato);
    if (mpSmoothVoiceLeadingCheckBox) mpSmoothVoiceLeadingCheckBox->SetValue(mConfig.enableSmoothVoiceLeading);
    if (mpGhostNotesCheckBox) mpGhostNotesCheckBox->SetValue(mConfig.enableGhostNotes);
    if (mpMicroPocketCheckBox) mpMicroPocketCheckBox->SetValue(mConfig.enableMicroPocket);
    if (mpScaleLockCheckBox) mpScaleLockCheckBox->SetValue(mConfig.enableScaleLock);
    wxMessageBox("Arrangement preset loaded successfully!", "Preset Loaded", wxOK | wxICON_INFORMATION, this);
  }
  else
  {
    wxMessageBox("Failed to load arrangement preset.", "Load Error", wxOK | wxICON_ERROR, this);
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZAutoAccompanimentDialog::OnHarmonizerButton(wxCommandEvent& Event)
{
  if (!mpProject || !mpProject->GetSong())
  {
    wxMessageBox("No active song project found.", "Harmonizer Error", wxOK | wxICON_ERROR, this);
    return;
  }

  wxArrayString styles;
  for (int s = 0; s < HARMONY_STYLE_COUNT; ++s)
  {
    styles.Add(JZAutoAccompaniment::GetHarmonizerStyleName((JZHarmonizerStyle)s));
  }

  wxSingleChoiceDialog styleDlg(this, "Select Vocal / Horn Harmonizer Style:", "Intelligent Harmonizer", styles);
  if (styleDlg.ShowModal() != wxID_OK) return;

  int sel = styleDlg.GetSelection();
  JZHarmonizerConfig harmCfg;
  harmCfg.sourceTrackIndex = mpSourceTrackChoice ? mpSourceTrackChoice->GetSelection() : 0;
  harmCfg.targetTrackIndex = 4;
  harmCfg.style = (JZHarmonizerStyle)sel;

  int fromB = 0;
  int toB = -1;
  if (mpRangeCustomRadio && mpRangeCustomRadio->GetValue())
  {
    fromB = mpFromBarSpin->GetValue() - 1;
    toB = mpToBarSpin->GetValue();
  }

  int notes = JZAutoAccompaniment::GenerateMelodicHarmony(mpProject->GetSong(), harmCfg, fromB, toB);
  wxString msg;
  msg.Printf("Generated %d harmony voice notes in Track 5 using style '%s'!", notes, JZAutoAccompaniment::GetHarmonizerStyleName(harmCfg.style));
  wxMessageBox(msg, "Harmonizer Complete", wxOK | wxICON_INFORMATION, this);
}
