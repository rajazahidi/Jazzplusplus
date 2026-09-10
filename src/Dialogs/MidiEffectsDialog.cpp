//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// MIDI Effects Dialog Implementation
//*****************************************************************************

#include "MidiEffectsDialog.h"

#include "../ChordScaleData.h"
#include "../Events.h"
#include "../Globals.h"
#include "../Project.h"
#include "../Song.h"
#include "../Track.h"

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/msgdlg.h>
#include <wx/notebook.h>
#include <wx/panel.h>
#include <wx/radiobox.h>
#include <wx/sizer.h>
#include <wx/slider.h>
#include <wx/spinctrl.h>
#include <wx/statbox.h>
#include <wx/stattext.h>

#include <algorithm>
#include <vector>

enum
{
  ID_NOTEBOOK = 11001,
  ID_TRACK_CHOICE,
  ID_SCOPE_RADIO,
  ID_BTN_APPLY,
  ID_BTN_CLOSE
};

BEGIN_EVENT_TABLE(JZMidiEffectsDialog, wxDialog)
  EVT_BUTTON(ID_BTN_APPLY, JZMidiEffectsDialog::OnApplyButton)
  EVT_BUTTON(ID_BTN_CLOSE, JZMidiEffectsDialog::OnCloseButton)
END_EVENT_TABLE()

JZMidiEffectsDialog::JZMidiEffectsDialog(
  wxWindow* pParent,
  JZProject* pProject,
  int DefaultTrack,
  long SelectionFromClock,
  long SelectionToClock)
  : wxDialog(
      pParent,
      wxID_ANY,
      wxString("MIDI Effects Suite"),
      wxDefaultPosition,
      wxSize(540, 520),
      wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
    mpProject(pProject),
    mTargetTrackIndex(DefaultTrack),
    mFromClock(SelectionFromClock),
    mToClock(SelectionToClock),
    mpNotebook(0),
    mpTrackChoice(0),
    mpScopeRadio(0),
    mpArpPatternChoice(0),
    mpArpRateChoice(0),
    mpArpOctavesSpin(0),
    mpArpGateSlider(0),
    mpArpScaleLockCheck(0),
    mpArpRootChoice(0),
    mpArpScaleChoice(0),
    mpHumTimingSlider(0),
    mpHumVelocitySlider(0),
    mpHumSwingSlider(0),
    mpHarmIntervalChoice(0),
    mpHarmRootChoice(0),
    mpHarmScaleChoice(0),
    mpHarmVelScaleSlider(0),
    mpEchoDelayChoice(0),
    mpEchoRepeatsSpin(0),
    mpEchoFeedbackSlider(0),
    mpEchoPitchShiftSpin(0),
    mpApplyButton(0),
    mpCloseButton(0)
{
  CreateControls();
  CentreOnParent();
}

JZMidiEffectsDialog::~JZMidiEffectsDialog()
{
}

void JZMidiEffectsDialog::CreateControls()
{
  wxBoxSizer* pTopSizer = new wxBoxSizer(wxVERTICAL);

  // 1. Target Track and Scope Header
  wxBoxSizer* pHeaderSizer = new wxBoxSizer(wxHORIZONTAL);

  pHeaderSizer->Add(
    new wxStaticText(this, wxID_ANY, "Target Track:"),
    0,
    wxALIGN_CENTER_VERTICAL | wxRIGHT,
    8);

  wxArrayString trackNames;
  if (mpProject)
  {
    for (int i = 0; i < mpProject->GetTrackCount(); ++i)
    {
      JZTrack* pTrk = mpProject->GetTrack(i);
      wxString name = wxString::Format("Track %d", i + 1);
      if (pTrk)
      {
        const char* pName = pTrk->GetName();
        if (pName && pName[0] != '\0')
        {
          name += wxString::Format(" (%s)", pName);
        }
      }
      trackNames.Add(name);
    }
  }
  if (trackNames.IsEmpty())
  {
    trackNames.Add("Track 1");
  }

  mpTrackChoice = new wxChoice(this, ID_TRACK_CHOICE, wxDefaultPosition, wxDefaultSize, trackNames);
  if (mTargetTrackIndex >= 0 && mTargetTrackIndex < static_cast<int>(trackNames.GetCount()))
  {
    mpTrackChoice->SetSelection(mTargetTrackIndex);
  }
  else
  {
    mpTrackChoice->SetSelection(0);
  }
  pHeaderSizer->Add(mpTrackChoice, 1, wxEXPAND | wxRIGHT, 15);

  wxArrayString scopeOptions;
  scopeOptions.Add("Selected Range");
  scopeOptions.Add("Entire Track");
  mpScopeRadio = new wxRadioBox(
    this,
    ID_SCOPE_RADIO,
    "Scope",
    wxDefaultPosition,
    wxDefaultSize,
    scopeOptions,
    1,
    wxRA_SPECIFY_ROWS);

  if (mFromClock >= mToClock)
  {
    // No valid range selected, default to entire track
    mpScopeRadio->SetSelection(1);
  }
  else
  {
    mpScopeRadio->SetSelection(0);
  }
  pHeaderSizer->Add(mpScopeRadio, 0, wxALIGN_CENTER_VERTICAL);

  pTopSizer->Add(pHeaderSizer, 0, wxEXPAND | wxALL, 10);

  // 2. Notebook for 4 MIDI Effects
  mpNotebook = new wxNotebook(this, ID_NOTEBOOK);

  // Common root notes and scales
  wxArrayString rootNames;
  const char* roots[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
  for (int i = 0; i < 12; ++i) rootNames.Add(roots[i]);

  wxArrayString scaleNames;
  const auto& allScales = JZChordScaleLibrary::GetScales();
  for (size_t i = 0; i < allScales.size(); ++i)
  {
    scaleNames.Add(allScales[i].mName.c_str());
  }
  if (scaleNames.IsEmpty())
  {
    scaleNames.Add("Major (Ionian)");
    scaleNames.Add("Natural Minor (Aeolian)");
    scaleNames.Add("Pentatonic Major");
    scaleNames.Add("Pentatonic Minor");
    scaleNames.Add("Blues");
    scaleNames.Add("Chromatic");
  }

  // --------------------------------------------------------------------------
  // Tab 1: Arpeggiator
  // --------------------------------------------------------------------------
  wxPanel* pArpPanel = new wxPanel(mpNotebook);
  wxBoxSizer* pArpSizer = new wxBoxSizer(wxVERTICAL);

  wxFlexGridSizer* pArpGrid = new wxFlexGridSizer(5, 2, 8, 12);
  pArpGrid->AddGrowableCol(1);

  pArpGrid->Add(new wxStaticText(pArpPanel, wxID_ANY, "Pattern:"), 0, wxALIGN_CENTER_VERTICAL);
  wxArrayString arpPatterns;
  arpPatterns.Add("Up");
  arpPatterns.Add("Down");
  arpPatterns.Add("Up & Down");
  arpPatterns.Add("Down & Up");
  arpPatterns.Add("Random");
  arpPatterns.Add("Chord Pulse (Rhythmic)");
  mpArpPatternChoice = new wxChoice(pArpPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, arpPatterns);
  mpArpPatternChoice->SetSelection(0);
  pArpGrid->Add(mpArpPatternChoice, 1, wxEXPAND);

  pArpGrid->Add(new wxStaticText(pArpPanel, wxID_ANY, "Rate (Subdivision):"), 0, wxALIGN_CENTER_VERTICAL);
  wxArrayString arpRates;
  arpRates.Add("1/4 Note (Quarter)");
  arpRates.Add("1/8 Note (Eighth)");
  arpRates.Add("1/16 Note (Sixteenth)");
  arpRates.Add("1/8 Triplet");
  arpRates.Add("1/16 Triplet");
  mpArpRateChoice = new wxChoice(pArpPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, arpRates);
  mpArpRateChoice->SetSelection(2); // 1/16 default
  pArpGrid->Add(mpArpRateChoice, 1, wxEXPAND);

  pArpGrid->Add(new wxStaticText(pArpPanel, wxID_ANY, "Octave Span:"), 0, wxALIGN_CENTER_VERTICAL);
  mpArpOctavesSpin = new wxSpinCtrl(pArpPanel, wxID_ANY, "1", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 4, 1);
  pArpGrid->Add(mpArpOctavesSpin, 0);

  pArpGrid->Add(new wxStaticText(pArpPanel, wxID_ANY, "Gate Length (%):"), 0, wxALIGN_CENTER_VERTICAL);
  mpArpGateSlider = new wxSlider(pArpPanel, wxID_ANY, 80, 10, 100, wxDefaultPosition, wxDefaultSize, wxSL_LABELS);
  pArpGrid->Add(mpArpGateSlider, 1, wxEXPAND);

  pArpSizer->Add(pArpGrid, 0, wxEXPAND | wxALL, 10);

  // Scale Lock Box
  wxStaticBoxSizer* pScaleLockBox = new wxStaticBoxSizer(wxVERTICAL, pArpPanel, "Scale-Aware Quantization");
  mpArpScaleLockCheck = new wxCheckBox(pArpPanel, wxID_ANY, "Lock Generated Notes to Active Scale");
  mpArpScaleLockCheck->SetValue(false);
  pScaleLockBox->Add(mpArpScaleLockCheck, 0, wxBOTTOM, 6);

  wxBoxSizer* pScaleRow = new wxBoxSizer(wxHORIZONTAL);
  pScaleRow->Add(new wxStaticText(pArpPanel, wxID_ANY, "Key:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
  mpArpRootChoice = new wxChoice(pArpPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, rootNames);
  mpArpRootChoice->SetSelection(0); // C
  pScaleRow->Add(mpArpRootChoice, 0, wxRIGHT, 10);

  pScaleRow->Add(new wxStaticText(pArpPanel, wxID_ANY, "Scale:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
  mpArpScaleChoice = new wxChoice(pArpPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, scaleNames);
  mpArpScaleChoice->SetSelection(0);
  pScaleRow->Add(mpArpScaleChoice, 1, wxEXPAND);
  pScaleLockBox->Add(pScaleRow, 0, wxEXPAND);

  pArpSizer->Add(pScaleLockBox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
  pArpPanel->SetSizer(pArpSizer);
  mpNotebook->AddPage(pArpPanel, "Smart Arpeggiator");

  // --------------------------------------------------------------------------
  // Tab 2: Humanizer
  // --------------------------------------------------------------------------
  wxPanel* pHumPanel = new wxPanel(mpNotebook);
  wxBoxSizer* pHumSizer = new wxBoxSizer(wxVERTICAL);

  wxFlexGridSizer* pHumGrid = new wxFlexGridSizer(3, 2, 10, 12);
  pHumGrid->AddGrowableCol(1);

  pHumGrid->Add(new wxStaticText(pHumPanel, wxID_ANY, "Timing Jitter (+/- ticks):"), 0, wxALIGN_CENTER_VERTICAL);
  mpHumTimingSlider = new wxSlider(pHumPanel, wxID_ANY, 8, 0, 40, wxDefaultPosition, wxDefaultSize, wxSL_LABELS);
  pHumGrid->Add(mpHumTimingSlider, 1, wxEXPAND);

  pHumGrid->Add(new wxStaticText(pHumPanel, wxID_ANY, "Velocity Jitter (+/- delta):"), 0, wxALIGN_CENTER_VERTICAL);
  mpHumVelocitySlider = new wxSlider(pHumPanel, wxID_ANY, 12, 0, 40, wxDefaultPosition, wxDefaultSize, wxSL_LABELS);
  pHumGrid->Add(mpHumVelocitySlider, 1, wxEXPAND);

  pHumGrid->Add(new wxStaticText(pHumPanel, wxID_ANY, "Groove Swing (%):"), 0, wxALIGN_CENTER_VERTICAL);
  mpHumSwingSlider = new wxSlider(pHumPanel, wxID_ANY, 50, 50, 75, wxDefaultPosition, wxDefaultSize, wxSL_LABELS);
  pHumGrid->Add(mpHumSwingSlider, 1, wxEXPAND);

  pHumSizer->Add(pHumGrid, 0, wxEXPAND | wxALL, 15);

  wxStaticText* pHumHint = new wxStaticText(
    pHumPanel,
    wxID_ANY,
    "Tip: Swing at 50% = Straight timing. 66% = Triplet shuffle feel. 75% = Hard dotted swing.\n"
    "Timing & Velocity jitter introduces natural organic variation found in live human performance.");
  pHumSizer->Add(pHumHint, 0, wxALL, 15);

  pHumPanel->SetSizer(pHumSizer);
  mpNotebook->AddPage(pHumPanel, "Humanizer & Swing");

  // --------------------------------------------------------------------------
  // Tab 3: Harmonizer
  // --------------------------------------------------------------------------
  wxPanel* pHarmPanel = new wxPanel(mpNotebook);
  wxBoxSizer* pHarmSizer = new wxBoxSizer(wxVERTICAL);

  wxFlexGridSizer* pHarmGrid = new wxFlexGridSizer(4, 2, 10, 12);
  pHarmGrid->AddGrowableCol(1);

  pHarmGrid->Add(new wxStaticText(pHarmPanel, wxID_ANY, "Harmony Interval:"), 0, wxALIGN_CENTER_VERTICAL);
  wxArrayString harmIntervals;
  harmIntervals.Add("Octave Above (+12)");
  harmIntervals.Add("Octave Below (-12)");
  harmIntervals.Add("Diatonic 3rd Above");
  harmIntervals.Add("Diatonic 3rd Below");
  harmIntervals.Add("Diatonic 5th Above");
  harmIntervals.Add("Diatonic 5th Below");
  harmIntervals.Add("Diatonic Triad (Root + 3rd + 5th)");
  harmIntervals.Add("Octave Pair (+12 & -12)");
  mpHarmIntervalChoice = new wxChoice(pHarmPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, harmIntervals);
  mpHarmIntervalChoice->SetSelection(2); // Diatonic 3rd Above
  pHarmGrid->Add(mpHarmIntervalChoice, 1, wxEXPAND);

  pHarmGrid->Add(new wxStaticText(pHarmPanel, wxID_ANY, "Key Root:"), 0, wxALIGN_CENTER_VERTICAL);
  mpHarmRootChoice = new wxChoice(pHarmPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, rootNames);
  mpHarmRootChoice->SetSelection(0);
  pHarmGrid->Add(mpHarmRootChoice, 1, wxEXPAND);

  pHarmGrid->Add(new wxStaticText(pHarmPanel, wxID_ANY, "Scale / Mode:"), 0, wxALIGN_CENTER_VERTICAL);
  mpHarmScaleChoice = new wxChoice(pHarmPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, scaleNames);
  mpHarmScaleChoice->SetSelection(0);
  pHarmGrid->Add(mpHarmScaleChoice, 1, wxEXPAND);

  pHarmGrid->Add(new wxStaticText(pHarmPanel, wxID_ANY, "Harmony Velocity (%):"), 0, wxALIGN_CENTER_VERTICAL);
  mpHarmVelScaleSlider = new wxSlider(pHarmPanel, wxID_ANY, 85, 20, 100, wxDefaultPosition, wxDefaultSize, wxSL_LABELS);
  pHarmGrid->Add(mpHarmVelScaleSlider, 1, wxEXPAND);

  pHarmSizer->Add(pHarmGrid, 0, wxEXPAND | wxALL, 15);
  pHarmPanel->SetSizer(pHarmSizer);
  mpNotebook->AddPage(pHarmPanel, "Intelligent Harmonizer");

  // --------------------------------------------------------------------------
  // Tab 4: Echo / Delay
  // --------------------------------------------------------------------------
  wxPanel* pEchoPanel = new wxPanel(mpNotebook);
  wxBoxSizer* pEchoSizer = new wxBoxSizer(wxVERTICAL);

  wxFlexGridSizer* pEchoGrid = new wxFlexGridSizer(4, 2, 10, 12);
  pEchoGrid->AddGrowableCol(1);

  pEchoGrid->Add(new wxStaticText(pEchoPanel, wxID_ANY, "Delay Time:"), 0, wxALIGN_CENTER_VERTICAL);
  wxArrayString echoTimes;
  echoTimes.Add("1/16 Note (60 ticks)");
  echoTimes.Add("1/8 Note (120 ticks)");
  echoTimes.Add("1/8 Note Dotted (180 ticks)");
  echoTimes.Add("1/4 Note (240 ticks)");
  echoTimes.Add("1/2 Note (480 ticks)");
  mpEchoDelayChoice = new wxChoice(pEchoPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, echoTimes);
  mpEchoDelayChoice->SetSelection(1); // 1/8 note default
  pEchoGrid->Add(mpEchoDelayChoice, 1, wxEXPAND);

  pEchoGrid->Add(new wxStaticText(pEchoPanel, wxID_ANY, "Echo Repeats:"), 0, wxALIGN_CENTER_VERTICAL);
  mpEchoRepeatsSpin = new wxSpinCtrl(pEchoPanel, wxID_ANY, "3", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 8, 3);
  pEchoGrid->Add(mpEchoRepeatsSpin, 0);

  pEchoGrid->Add(new wxStaticText(pEchoPanel, wxID_ANY, "Feedback Decay (%):"), 0, wxALIGN_CENTER_VERTICAL);
  mpEchoFeedbackSlider = new wxSlider(pEchoPanel, wxID_ANY, 65, 10, 95, wxDefaultPosition, wxDefaultSize, wxSL_LABELS);
  pEchoGrid->Add(mpEchoFeedbackSlider, 1, wxEXPAND);

  pEchoGrid->Add(new wxStaticText(pEchoPanel, wxID_ANY, "Pitch Shift per Repeat:"), 0, wxALIGN_CENTER_VERTICAL);
  mpEchoPitchShiftSpin = new wxSpinCtrl(pEchoPanel, wxID_ANY, "0", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, -12, 12, 0);
  pEchoGrid->Add(mpEchoPitchShiftSpin, 0);

  pEchoSizer->Add(pEchoGrid, 0, wxEXPAND | wxALL, 15);
  pEchoPanel->SetSizer(pEchoSizer);
  mpNotebook->AddPage(pEchoPanel, "Tempo-Synced Echo");

  pTopSizer->Add(mpNotebook, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);

  // 3. Action Buttons
  wxBoxSizer* pButtonSizer = new wxBoxSizer(wxHORIZONTAL);
  pButtonSizer->AddStretchSpacer();

  mpApplyButton = new wxButton(this, ID_BTN_APPLY, "Apply Effect");
  mpApplyButton->SetDefault();
  pButtonSizer->Add(mpApplyButton, 0, wxRIGHT, 10);

  mpCloseButton = new wxButton(this, ID_BTN_CLOSE, "Close");
  pButtonSizer->Add(mpCloseButton, 0);

  pTopSizer->Add(pButtonSizer, 0, wxEXPAND | wxALL, 10);

  SetSizer(pTopSizer);
}

void JZMidiEffectsDialog::OnApplyButton(wxCommandEvent& Event)
{
  if (!mpProject)
  {
    wxMessageBox("No active song available.", "Error", wxOK | wxICON_ERROR, this);
    return;
  }

  int trkIndex = mpTrackChoice->GetSelection();
  if (trkIndex < 0 || trkIndex >= mpProject->GetTrackCount())
  {
    wxMessageBox("Invalid track selected.", "Error", wxOK | wxICON_ERROR, this);
    return;
  }

  JZTrack* pTrack = mpProject->GetTrack(trkIndex);
  if (!pTrack)
  {
    wxMessageBox("Track not found.", "Error", wxOK | wxICON_ERROR, this);
    return;
  }

  long fromClock = 0;
  long toClock = mpProject->GetLastClock();
  if (mpScopeRadio->GetSelection() == 0 && mFromClock < mToClock)
  {
    fromClock = mFromClock;
    toClock = mToClock;
  }

  int activeTab = mpNotebook->GetSelection();
  switch (activeTab)
  {
    case 0:
      ApplyArpeggiator(pTrack, fromClock, toClock);
      break;
    case 1:
      ApplyHumanizer(pTrack, fromClock, toClock);
      break;
    case 2:
      ApplyHarmonizer(pTrack, fromClock, toClock);
      break;
    case 3:
      ApplyEcho(pTrack, fromClock, toClock);
      break;
  }

  mpProject->NewUndoBuffer();
  wxMessageBox("MIDI Effect successfully applied!", "Jazz++ MIDI Effects", wxOK | wxICON_INFORMATION, this);
}

void JZMidiEffectsDialog::OnCloseButton(wxCommandEvent& Event)
{
  EndModal(wxID_CANCEL);
}

void JZMidiEffectsDialog::ApplyArpeggiator(JZTrack* pTrack, long fromClock, long toClock)
{
  JZArpSettings settings;
  settings.pattern = static_cast<JZArpPattern>(mpArpPatternChoice->GetSelection());

  switch (mpArpRateChoice->GetSelection())
  {
    case 0: settings.rate = eArpRate_Quarter; break;
    case 1: settings.rate = eArpRate_Eighth; break;
    case 2: settings.rate = eArpRate_Sixteenth; break;
    case 3: settings.rate = eArpRate_EighthTriplet; break;
    case 4: settings.rate = eArpRate_SixteenthTriplet; break;
    default: settings.rate = eArpRate_Sixteenth; break;
  }

  settings.octaves = mpArpOctavesSpin->GetValue();
  settings.gatePercent = mpArpGateSlider->GetValue();
  if (mpArpScaleLockCheck->IsChecked())
  {
    settings.scaleName = mpArpScaleChoice->GetStringSelection().ToStdString();
    settings.scaleRoot = mpArpRootChoice->GetSelection();
  }
  else
  {
    settings.scaleName = "";
  }

  int tpq = mpProject ? mpProject->GetTicksPerQuarter() : 480;
  JZMidiArpeggiator::Apply(pTrack, fromClock, toClock, tpq, settings);
}

void JZMidiEffectsDialog::ApplyHumanizer(JZTrack* pTrack, long fromClock, long toClock)
{
  JZHumanizeSettings settings;
  settings.timingJitterTicks = mpHumTimingSlider->GetValue();
  settings.velocityJitter = mpHumVelocitySlider->GetValue();
  settings.swingPercent = mpHumSwingSlider->GetValue();
  int tpq = mpProject ? mpProject->GetTicksPerQuarter() : 480;
  settings.swingGridTicks = tpq / 2;

  JZMidiHumanizer::Apply(pTrack, fromClock, toClock, settings);
}

void JZMidiEffectsDialog::ApplyHarmonizer(JZTrack* pTrack, long fromClock, long toClock)
{
  JZHarmonizeSettings settings;
  settings.interval = static_cast<JZHarmonizeInterval>(mpHarmIntervalChoice->GetSelection());
  settings.scaleRoot = mpHarmRootChoice->GetSelection();
  settings.scaleName = mpHarmScaleChoice->GetStringSelection().ToStdString();
  settings.velocityRatioPercent = mpHarmVelScaleSlider->GetValue();

  JZMidiHarmonizer::Apply(pTrack, fromClock, toClock, settings);
}

void JZMidiEffectsDialog::ApplyEcho(JZTrack* pTrack, long fromClock, long toClock)
{
  JZMidiEchoSettings settings;
  int tpq = mpProject ? mpProject->GetTicksPerQuarter() : 480;
  switch (mpEchoDelayChoice->GetSelection())
  {
    case 0: settings.delayTicks = tpq / 4; break;
    case 1: settings.delayTicks = tpq / 2; break;
    case 2: settings.delayTicks = (tpq / 2) + (tpq / 4); break;
    case 3: settings.delayTicks = tpq; break;
    case 4: settings.delayTicks = tpq * 2; break;
    default: settings.delayTicks = tpq / 2; break;
  }

  settings.repeatCount = mpEchoRepeatsSpin->GetValue();
  settings.feedback = static_cast<double>(mpEchoFeedbackSlider->GetValue()) / 100.0;
  settings.pitchShiftPerRepeat = mpEchoPitchShiftSpin->GetValue();

  JZMidiEcho::Apply(pTrack, fromClock, toClock, settings);
}
