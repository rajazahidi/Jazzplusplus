//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Auto-Accompaniment (Smart Band) Dialog Implementation
//*****************************************************************************

#include "AutoAccompanimentDialog.h"
#include "../Project.h"
#include "../Song.h"
#include "../Track.h"
#include "../Globals.h"
#include "../TrackWindow.h"

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/msgdlg.h>
#include <wx/radiobut.h>
#include <wx/sizer.h>
#include <wx/spinctrl.h>
#include <wx/statbox.h>
#include <wx/stattext.h>

#include <sstream>

using namespace std;

enum
{
  ID_BTN_GENERATE = wxID_HIGHEST + 301,
  ID_RANGE_ALL_RADIO,
  ID_RANGE_CUSTOM_RADIO
};

BEGIN_EVENT_TABLE(JZAutoAccompanimentDialog, wxDialog)
  EVT_BUTTON(ID_BTN_GENERATE, JZAutoAccompanimentDialog::OnGenerateButton)
  EVT_BUTTON(wxID_CANCEL, JZAutoAccompanimentDialog::OnCancelButton)
  EVT_RADIOBUTTON(ID_RANGE_ALL_RADIO, JZAutoAccompanimentDialog::OnRangeRadio)
  EVT_RADIOBUTTON(ID_RANGE_CUSTOM_RADIO, JZAutoAccompanimentDialog::OnRangeRadio)
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
      "Auto-Accompaniment (Smart Band)",
      wxDefaultPosition,
      wxDefaultSize,
      wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
    mpProject(pProject),
    mDefaultSourceTrack(defaultSourceTrack),
    mpSourceTrackChoice(0),
    mpStyleChoice(0),
    mpBassCheckBox(0),
    mpDrumsCheckBox(0),
    mpPadCheckBox(0),
    mpRangeAllRadio(0),
    mpRangeCustomRadio(0),
    mpFromBarSpin(0),
    mpToBarSpin(0),
    mpOverwriteCheckBox(0)
{
  CreateControls();
  SetMinSize(FromDIP(wxSize(480, 520)));
  Fit();
  CentreOnParent();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZAutoAccompanimentDialog::~JZAutoAccompanimentDialog()
{
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZAutoAccompanimentDialog::CreateControls()
{
  wxBoxSizer* pMainSizer = new wxBoxSizer(wxVERTICAL);

  // Description banner
  wxStaticText* pDesc = new wxStaticText(
    this,
    wxID_ANY,
    "Generate intelligent Bass, Drums, and Pad accompaniment\n"
    "tracks based on the chords and rhythm of your piano track.");
  pMainSizer->Add(pDesc, 0, wxALL | wxEXPAND, FromDIP(10));

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
        oss << " (Piano)";
      }
    }
    mpSourceTrackChoice->Append(oss.str());
  }

  if (mDefaultSourceTrack >= 0 && mDefaultSourceTrack < maxTracks)
  {
    mpSourceTrackChoice->SetSelection(mDefaultSourceTrack);
  }
  else
  {
    mpSourceTrackChoice->SetSelection(0);
  }
  pSourceBox->Add(mpSourceTrackChoice, 0, wxALL | wxEXPAND, FromDIP(5));
  pMainSizer->Add(pSourceBox, 0, wxALL | wxEXPAND, FromDIP(8));

  // Style Box
  wxStaticBoxSizer* pStyleBox = new wxStaticBoxSizer(wxVERTICAL, this, "Accompaniment Musical Style");
  mpStyleChoice = new wxChoice(this, wxID_ANY);
  for (int s = 0; s < STYLE_COUNT; ++s)
  {
    mpStyleChoice->Append(JZAutoAccompaniment::GetStyleName((JZAccompanimentStyle)s));
  }
  mpStyleChoice->SetSelection(STYLE_JAZZ_SWING);
  pStyleBox->Add(mpStyleChoice, 0, wxALL | wxEXPAND, FromDIP(5));
  pMainSizer->Add(pStyleBox, 0, wxALL | wxEXPAND, FromDIP(8));

  // Instruments to generate Box
  wxStaticBoxSizer* pInstBox = new wxStaticBoxSizer(wxVERTICAL, this, "Instruments to Generate");
  mpBassCheckBox = new wxCheckBox(this, wxID_ANY, "Auto Bass (Track 2, Channel 2 - Walking / Groove)");
  mpBassCheckBox->SetValue(true);
  pInstBox->Add(mpBassCheckBox, 0, wxALL, FromDIP(4));

  mpDrumsCheckBox = new wxCheckBox(this, wxID_ANY, "Auto Drums (Track 10, Channel 10 - Standard Drum Kit)");
  mpDrumsCheckBox->SetValue(true);
  pInstBox->Add(mpDrumsCheckBox, 0, wxALL, FromDIP(4));

  mpPadCheckBox = new wxCheckBox(this, wxID_ANY, "Auto Strings / Pad (Track 3, Channel 3 - Warm Chords)");
  mpPadCheckBox->SetValue(true);
  pInstBox->Add(mpPadCheckBox, 0, wxALL, FromDIP(4));

  pMainSizer->Add(pInstBox, 0, wxALL | wxEXPAND, FromDIP(8));

  // Measure Range Box
  wxStaticBoxSizer* pRangeBox = new wxStaticBoxSizer(wxVERTICAL, this, "Measure Range");
  mpRangeAllRadio = new wxRadioButton(this, ID_RANGE_ALL_RADIO, "Entire Song (from Bar 1 to End)", wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
  mpRangeAllRadio->SetValue(true);
  pRangeBox->Add(mpRangeAllRadio, 0, wxALL, FromDIP(4));

  wxBoxSizer* pCustomRangeSizer = new wxBoxSizer(wxHORIZONTAL);
  mpRangeCustomRadio = new wxRadioButton(this, ID_RANGE_CUSTOM_RADIO, "Measure:");
  pCustomRangeSizer->Add(mpRangeCustomRadio, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(5));

  mpFromBarSpin = new wxSpinCtrl(this, wxID_ANY, "1", wxDefaultPosition, FromDIP(wxSize(65, -1)));
  mpFromBarSpin->SetRange(1, 999);
  mpFromBarSpin->Enable(false);
  pCustomRangeSizer->Add(mpFromBarSpin, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(8));

  pCustomRangeSizer->Add(new wxStaticText(this, wxID_ANY, "to"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(8));

  // Calculate default end bar
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

  mpToBarSpin = new wxSpinCtrl(this, wxID_ANY, wxString::Format("%d", songLastBar), wxDefaultPosition, FromDIP(wxSize(65, -1)));
  mpToBarSpin->SetRange(1, 999);
  mpToBarSpin->Enable(false);
  pCustomRangeSizer->Add(mpToBarSpin, 0, wxALIGN_CENTER_VERTICAL, 0);

  pRangeBox->Add(pCustomRangeSizer, 0, wxALL, FromDIP(4));
  pMainSizer->Add(pRangeBox, 0, wxALL | wxEXPAND, FromDIP(8));

  // Options Box
  mpOverwriteCheckBox = new wxCheckBox(this, wxID_ANY, "Overwrite existing notes in target accompaniment tracks");
  mpOverwriteCheckBox->SetValue(true);
  pMainSizer->Add(mpOverwriteCheckBox, 0, wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(10));

  // Button Sizer
  wxBoxSizer* pBtnSizer = new wxBoxSizer(wxHORIZONTAL);
  wxButton* pGenBtn = new wxButton(this, ID_BTN_GENERATE, "Generate Accompaniment");
  pGenBtn->SetDefault();
  pBtnSizer->Add(pGenBtn, 0, wxRIGHT, FromDIP(10));

  wxButton* pCancelBtn = new wxButton(this, wxID_CANCEL, "Cancel");
  pBtnSizer->Add(pCancelBtn, 0, 0, 0);

  pMainSizer->Add(pBtnSizer, 0, wxALIGN_RIGHT | wxALL, FromDIP(12));

  SetSizer(pMainSizer);
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
void JZAutoAccompanimentDialog::OnCancelButton(wxCommandEvent& Event)
{
  EndModal(wxID_CANCEL);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZAutoAccompanimentDialog::OnGenerateButton(wxCommandEvent& Event)
{
  if (!mpProject)
  {
    wxMessageBox("No active project found.", "Error", wxOK | wxICON_ERROR, this);
    return;
  }

  // Populate config
  mConfig.sourceTrackIndex = mpSourceTrackChoice->GetSelection();
  mConfig.style = (JZAccompanimentStyle)mpStyleChoice->GetSelection();
  mConfig.generateBass = mpBassCheckBox->GetValue();
  mConfig.generateDrums = mpDrumsCheckBox->GetValue();
  mConfig.generatePad = mpPadCheckBox->GetValue();
  mConfig.overwriteExisting = mpOverwriteCheckBox->GetValue();

  if (!mConfig.generateBass && !mConfig.generateDrums && !mConfig.generatePad)
  {
    wxMessageBox("Please select at least one instrument track to generate.", "Information", wxOK | wxICON_INFORMATION, this);
    return;
  }

  if (mpRangeCustomRadio->GetValue())
  {
    int fromB = mpFromBarSpin->GetValue() - 1; // 0-based
    int toB = mpToBarSpin->GetValue();          // 0-based exclusive
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
    mConfig.toBar = -1; // to end of song
  }

  // Save undo point
  mpProject->NewUndoBuffer();

  // Execute generation
  JZAccompanimentResult res = JZAutoAccompaniment::Generate(mpProject, mConfig);

  if (!res.success)
  {
    wxMessageBox(res.message, "Auto-Accompaniment", wxOK | wxICON_WARNING, this);
    return;
  }

  // Refresh windows
  if (gpTrackWindow)
  {
    gpTrackWindow->Refresh();
  }

  wxMessageBox(
    wxString(res.message.c_str(), wxConvUTF8),
    "Accompaniment Generated Successfully",
    wxOK | wxICON_INFORMATION,
    this);

  EndModal(wxID_OK);
}
