//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Audio Effects DSP Dialog Implementation
//*****************************************************************************

#include "AudioEffectsDialog.h"

#include "../Audio.h"
#include "../Globals.h"
#include "../Player.h"
#include "../Project.h"
#include "../Sample.h"
#include "../SoundGenerator.h"

namespace
{
  JZSampleSet* GetProjectSampleSet(JZProject* pProject)
  {
    if (pProject && pProject->GetPlayer())
    {
      return pProject->GetPlayer()->GetSampleSet();
    }
    if (gpMidiPlayer)
    {
      return gpMidiPlayer->GetSampleSet();
    }
    return 0;
  }
}

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/filename.h>
#include <wx/msgdlg.h>
#include <wx/notebook.h>
#include <wx/panel.h>
#include <wx/scrolwin.h>
#include <wx/sizer.h>
#include <wx/slider.h>
#include <wx/statbox.h>
#include <wx/stattext.h>
#include <wx/stdpaths.h>

#include <vector>

enum
{
  ID_SAMPLE_CHOICE = 13001,
  ID_BTN_PREVIEW,
  ID_BTN_APPLY,
  ID_BTN_CLOSE
};

BEGIN_EVENT_TABLE(JZAudioEffectsDialog, wxDialog)
  EVT_CHOICE(ID_SAMPLE_CHOICE, JZAudioEffectsDialog::OnSampleSelect)
  EVT_BUTTON(ID_BTN_PREVIEW, JZAudioEffectsDialog::OnPreviewButton)
  EVT_BUTTON(ID_BTN_APPLY, JZAudioEffectsDialog::OnApplyButton)
  EVT_BUTTON(ID_BTN_CLOSE, JZAudioEffectsDialog::OnCloseButton)
END_EVENT_TABLE()

JZAudioEffectsDialog::JZAudioEffectsDialog(
  wxWindow* pParent,
  JZProject* pProject,
  int DefaultSampleIndex)
  : wxDialog(
      pParent,
      wxID_ANY,
      wxString("Audio Effects & DSP Suite"),
      wxDefaultPosition,
      wxSize(720, 620),
      wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
    mpProject(pProject),
    mSelectedSampleIndex(DefaultSampleIndex),
    mpNotebook(0),
    mpSampleChoice(0),
    mpEqBassSlider(0),
    mpEqMidSlider(0),
    mpEqTrebleSlider(0),
    mpRevRoomSlider(0),
    mpRevDampSlider(0),
    mpRevWidthSlider(0),
    mpRevWetSlider(0),
    mpRevDrySlider(0),
    mpDelayTimeSlider(0),
    mpDelayFeedbackSlider(0),
    mpDelayDampSlider(0),
    mpDelayWetSlider(0),
    mpDelayDrySlider(0),
    mpChoRateSlider(0),
    mpChoDepthSlider(0),
    mpChoFeedbackSlider(0),
    mpChoMixSlider(0),
    mpLimDriveSlider(0),
    mpLimCeilingSlider(0),
    mpLimSoftClipCheck(0),
    mpPreviewButton(0),
    mpApplyButton(0),
    mpCloseButton(0)
{
  SetMinSize(wxSize(680, 560));
  CreateControls();
  CentreOnParent();
}

JZAudioEffectsDialog::~JZAudioEffectsDialog()
{
}

void JZAudioEffectsDialog::CreateControls()
{
  wxBoxSizer* pTopSizer = new wxBoxSizer(wxVERTICAL);

  // 1. Sample Selector Row
  wxBoxSizer* pSampleRow = new wxBoxSizer(wxHORIZONTAL);
  pSampleRow->Add(new wxStaticText(this, wxID_ANY, "Target Sample / Slot:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

  wxArrayString sampleNames;
  JZSampleSet* pSet = GetProjectSampleSet(mpProject);
  if (pSet)
  {
    for (int i = 0; i < JZSampleSet::eSampleCount; ++i)
    {
      std::string label = pSet->GetSampleLabel(i);
      wxString entry = wxString::Format("Slot %d (Key %d)", i + 1, i);
      if (!label.empty())
      {
        entry += wxString::Format(" - %s", label.c_str());
      }
      sampleNames.Add(entry);
    }
  }
  else
  {
    for (int i = 0; i < 16; ++i)
    {
      sampleNames.Add(wxString::Format("Slot %d (Key %d)", i + 1, i));
    }
  }

  mpSampleChoice = new wxChoice(this, ID_SAMPLE_CHOICE, wxDefaultPosition, wxDefaultSize, sampleNames);
  if (mSelectedSampleIndex >= 0 && mSelectedSampleIndex < static_cast<int>(sampleNames.GetCount()))
  {
    mpSampleChoice->SetSelection(mSelectedSampleIndex);
  }
  else
  {
    mpSampleChoice->SetSelection(0);
  }
  pSampleRow->Add(mpSampleChoice, 1, wxEXPAND);
  pTopSizer->Add(pSampleRow, 0, wxEXPAND | wxALL, 10);

  // 2. DSP Notebook
  mpNotebook = new wxNotebook(this, wxID_ANY);

  // --------------------------------------------------------------------------
  // Tab 1: 3-Band Parametric Equalizer
  // --------------------------------------------------------------------------
  wxScrolledWindow* pEqPanel = new wxScrolledWindow(mpNotebook, wxID_ANY);
  pEqPanel->SetScrollRate(5, 10);
  wxBoxSizer* pEqSizer = new wxBoxSizer(wxVERTICAL);

  wxFlexGridSizer* pEqGrid = new wxFlexGridSizer(3, 2, 10, 12);
  pEqGrid->AddGrowableCol(1);

  pEqGrid->Add(new wxStaticText(pEqPanel, wxID_ANY, "Bass Gain (dB @ 120Hz):"), 0, wxALIGN_CENTER_VERTICAL);
  mpEqBassSlider = new wxSlider(pEqPanel, wxID_ANY, 0, -18, 18, wxDefaultPosition, wxDefaultSize, wxSL_LABELS);
  pEqGrid->Add(mpEqBassSlider, 1, wxEXPAND);

  pEqGrid->Add(new wxStaticText(pEqPanel, wxID_ANY, "Mid Gain (dB @ 1.2kHz):"), 0, wxALIGN_CENTER_VERTICAL);
  mpEqMidSlider = new wxSlider(pEqPanel, wxID_ANY, 0, -18, 18, wxDefaultPosition, wxDefaultSize, wxSL_LABELS);
  pEqGrid->Add(mpEqMidSlider, 1, wxEXPAND);

  pEqGrid->Add(new wxStaticText(pEqPanel, wxID_ANY, "Treble Gain (dB @ 7kHz):"), 0, wxALIGN_CENTER_VERTICAL);
  mpEqTrebleSlider = new wxSlider(pEqPanel, wxID_ANY, 0, -18, 18, wxDefaultPosition, wxDefaultSize, wxSL_LABELS);
  pEqGrid->Add(mpEqTrebleSlider, 1, wxEXPAND);

  pEqSizer->Add(pEqGrid, 0, wxEXPAND | wxALL, 15);
  pEqPanel->SetSizer(pEqSizer);
  pEqSizer->FitInside(pEqPanel);
  mpNotebook->AddPage(pEqPanel, "3-Band EQ");

  // --------------------------------------------------------------------------
  // Tab 2: Freeverb Algorithmic Reverb
  // --------------------------------------------------------------------------
  wxScrolledWindow* pRevPanel = new wxScrolledWindow(mpNotebook, wxID_ANY);
  pRevPanel->SetScrollRate(5, 10);
  wxBoxSizer* pRevSizer = new wxBoxSizer(wxVERTICAL);

  wxFlexGridSizer* pRevGrid = new wxFlexGridSizer(5, 2, 8, 12);
  pRevGrid->AddGrowableCol(1);

  pRevGrid->Add(new wxStaticText(pRevPanel, wxID_ANY, "Room Size (%):"), 0, wxALIGN_CENTER_VERTICAL);
  mpRevRoomSlider = new wxSlider(pRevPanel, wxID_ANY, 60, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_LABELS);
  pRevGrid->Add(mpRevRoomSlider, 1, wxEXPAND);

  pRevGrid->Add(new wxStaticText(pRevPanel, wxID_ANY, "High Damping (%):"), 0, wxALIGN_CENTER_VERTICAL);
  mpRevDampSlider = new wxSlider(pRevPanel, wxID_ANY, 35, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_LABELS);
  pRevGrid->Add(mpRevDampSlider, 1, wxEXPAND);

  pRevGrid->Add(new wxStaticText(pRevPanel, wxID_ANY, "Stereo Width (%):"), 0, wxALIGN_CENTER_VERTICAL);
  mpRevWidthSlider = new wxSlider(pRevPanel, wxID_ANY, 100, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_LABELS);
  pRevGrid->Add(mpRevWidthSlider, 1, wxEXPAND);

  pRevGrid->Add(new wxStaticText(pRevPanel, wxID_ANY, "Wet Mix (%):"), 0, wxALIGN_CENTER_VERTICAL);
  mpRevWetSlider = new wxSlider(pRevPanel, wxID_ANY, 40, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_LABELS);
  pRevGrid->Add(mpRevWetSlider, 1, wxEXPAND);

  pRevGrid->Add(new wxStaticText(pRevPanel, wxID_ANY, "Dry Mix (%):"), 0, wxALIGN_CENTER_VERTICAL);
  mpRevDrySlider = new wxSlider(pRevPanel, wxID_ANY, 75, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_LABELS);
  pRevGrid->Add(mpRevDrySlider, 1, wxEXPAND);

  pRevSizer->Add(pRevGrid, 0, wxEXPAND | wxALL, 12);
  pRevPanel->SetSizer(pRevSizer);
  pRevSizer->FitInside(pRevPanel);
  mpNotebook->AddPage(pRevPanel, "Reverb (Freeverb)");

  // --------------------------------------------------------------------------
  // Tab 3: Stereo Delay & Echo
  // --------------------------------------------------------------------------
  wxScrolledWindow* pDelayPanel = new wxScrolledWindow(mpNotebook, wxID_ANY);
  pDelayPanel->SetScrollRate(5, 10);
  wxBoxSizer* pDelaySizer = new wxBoxSizer(wxVERTICAL);

  wxFlexGridSizer* pDelayGrid = new wxFlexGridSizer(5, 2, 8, 12);
  pDelayGrid->AddGrowableCol(1);

  pDelayGrid->Add(new wxStaticText(pDelayPanel, wxID_ANY, "Delay Time (ms):"), 0, wxALIGN_CENTER_VERTICAL);
  mpDelayTimeSlider = new wxSlider(pDelayPanel, wxID_ANY, 320, 20, 1500, wxDefaultPosition, wxDefaultSize, wxSL_LABELS);
  pDelayGrid->Add(mpDelayTimeSlider, 1, wxEXPAND);

  pDelayGrid->Add(new wxStaticText(pDelayPanel, wxID_ANY, "Feedback (%):"), 0, wxALIGN_CENTER_VERTICAL);
  mpDelayFeedbackSlider = new wxSlider(pDelayPanel, wxID_ANY, 50, 0, 95, wxDefaultPosition, wxDefaultSize, wxSL_LABELS);
  pDelayGrid->Add(mpDelayFeedbackSlider, 1, wxEXPAND);

  pDelayGrid->Add(new wxStaticText(pDelayPanel, wxID_ANY, "High Damp (%):"), 0, wxALIGN_CENTER_VERTICAL);
  mpDelayDampSlider = new wxSlider(pDelayPanel, wxID_ANY, 25, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_LABELS);
  pDelayGrid->Add(mpDelayDampSlider, 1, wxEXPAND);

  pDelayGrid->Add(new wxStaticText(pDelayPanel, wxID_ANY, "Wet Mix (%):"), 0, wxALIGN_CENTER_VERTICAL);
  mpDelayWetSlider = new wxSlider(pDelayPanel, wxID_ANY, 45, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_LABELS);
  pDelayGrid->Add(mpDelayWetSlider, 1, wxEXPAND);

  pDelayGrid->Add(new wxStaticText(pDelayPanel, wxID_ANY, "Dry Mix (%):"), 0, wxALIGN_CENTER_VERTICAL);
  mpDelayDrySlider = new wxSlider(pDelayPanel, wxID_ANY, 80, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_LABELS);
  pDelayGrid->Add(mpDelayDrySlider, 1, wxEXPAND);

  pDelaySizer->Add(pDelayGrid, 0, wxEXPAND | wxALL, 12);
  pDelayPanel->SetSizer(pDelaySizer);
  pDelaySizer->FitInside(pDelayPanel);
  mpNotebook->AddPage(pDelayPanel, "Stereo Delay");

  // --------------------------------------------------------------------------
  // Tab 4: Stereo Chorus / Flanger
  // --------------------------------------------------------------------------
  wxScrolledWindow* pChoPanel = new wxScrolledWindow(mpNotebook, wxID_ANY);
  pChoPanel->SetScrollRate(5, 10);
  wxBoxSizer* pChoSizer = new wxBoxSizer(wxVERTICAL);

  wxFlexGridSizer* pChoGrid = new wxFlexGridSizer(4, 2, 10, 12);
  pChoGrid->AddGrowableCol(1);

  pChoGrid->Add(new wxStaticText(pChoPanel, wxID_ANY, "LFO Rate (Hz x10):"), 0, wxALIGN_CENTER_VERTICAL);
  mpChoRateSlider = new wxSlider(pChoPanel, wxID_ANY, 12, 1, 80, wxDefaultPosition, wxDefaultSize, wxSL_LABELS); // 1.2 Hz
  pChoGrid->Add(mpChoRateSlider, 1, wxEXPAND);

  pChoGrid->Add(new wxStaticText(pChoPanel, wxID_ANY, "Modulation Depth (ms):"), 0, wxALIGN_CENTER_VERTICAL);
  mpChoDepthSlider = new wxSlider(pChoPanel, wxID_ANY, 4, 1, 15, wxDefaultPosition, wxDefaultSize, wxSL_LABELS);
  pChoGrid->Add(mpChoDepthSlider, 1, wxEXPAND);

  pChoGrid->Add(new wxStaticText(pChoPanel, wxID_ANY, "Feedback (%):"), 0, wxALIGN_CENTER_VERTICAL);
  mpChoFeedbackSlider = new wxSlider(pChoPanel, wxID_ANY, 25, 0, 85, wxDefaultPosition, wxDefaultSize, wxSL_LABELS);
  pChoGrid->Add(mpChoFeedbackSlider, 1, wxEXPAND);

  pChoGrid->Add(new wxStaticText(pChoPanel, wxID_ANY, "Chorus Mix (%):"), 0, wxALIGN_CENTER_VERTICAL);
  mpChoMixSlider = new wxSlider(pChoPanel, wxID_ANY, 50, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_LABELS);
  pChoGrid->Add(mpChoMixSlider, 1, wxEXPAND);

  pChoSizer->Add(pChoGrid, 0, wxEXPAND | wxALL, 15);
  pChoPanel->SetSizer(pChoSizer);
  pChoSizer->FitInside(pChoPanel);
  mpNotebook->AddPage(pChoPanel, "Stereo Chorus");

  // --------------------------------------------------------------------------
  // Tab 5: Master Limiter & Distortion
  // --------------------------------------------------------------------------
  wxScrolledWindow* pLimPanel = new wxScrolledWindow(mpNotebook, wxID_ANY);
  pLimPanel->SetScrollRate(5, 10);
  wxBoxSizer* pLimSizer = new wxBoxSizer(wxVERTICAL);

  wxFlexGridSizer* pLimGrid = new wxFlexGridSizer(3, 2, 10, 12);
  pLimGrid->AddGrowableCol(1);

  pLimGrid->Add(new wxStaticText(pLimPanel, wxID_ANY, "Drive / Boost (dB):"), 0, wxALIGN_CENTER_VERTICAL);
  mpLimDriveSlider = new wxSlider(pLimPanel, wxID_ANY, 0, 0, 24, wxDefaultPosition, wxDefaultSize, wxSL_LABELS);
  pLimGrid->Add(mpLimDriveSlider, 1, wxEXPAND);

  pLimGrid->Add(new wxStaticText(pLimPanel, wxID_ANY, "Ceiling (dB):"), 0, wxALIGN_CENTER_VERTICAL);
  mpLimCeilingSlider = new wxSlider(pLimPanel, wxID_ANY, -1, -12, 0, wxDefaultPosition, wxDefaultSize, wxSL_LABELS);
  pLimGrid->Add(mpLimCeilingSlider, 1, wxEXPAND);

  pLimGrid->Add(new wxStaticText(pLimPanel, wxID_ANY, "Warm Saturation:"), 0, wxALIGN_CENTER_VERTICAL);
  mpLimSoftClipCheck = new wxCheckBox(pLimPanel, wxID_ANY, "Enable Soft-Clipping Overdrive");
  mpLimSoftClipCheck->SetValue(true);
  pLimGrid->Add(mpLimSoftClipCheck, 0);

  pLimSizer->Add(pLimGrid, 0, wxEXPAND | wxALL, 15);
  pLimPanel->SetSizer(pLimSizer);
  pLimSizer->FitInside(pLimPanel);
  mpNotebook->AddPage(pLimPanel, "Limiter & Overdrive");

  pTopSizer->Add(mpNotebook, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);

  // 3. Action Buttons
  wxBoxSizer* pBtnSizer = new wxBoxSizer(wxHORIZONTAL);
  mpPreviewButton = new wxButton(this, ID_BTN_PREVIEW, "Play Preview");
  pBtnSizer->Add(mpPreviewButton, 0, wxRIGHT, 10);

  pBtnSizer->AddStretchSpacer();

  mpApplyButton = new wxButton(this, ID_BTN_APPLY, "Apply DSP Effect");
  mpApplyButton->SetDefault();
  pBtnSizer->Add(mpApplyButton, 0, wxRIGHT, 10);

  mpCloseButton = new wxButton(this, ID_BTN_CLOSE, "Close");
  pBtnSizer->Add(mpCloseButton, 0);

  pTopSizer->Add(pBtnSizer, 0, wxEXPAND | wxALL, 10);

  SetSizer(pTopSizer);
  pTopSizer->SetSizeHints(this);
}

void JZAudioEffectsDialog::OnSampleSelect(wxCommandEvent& Event)
{
  mSelectedSampleIndex = mpSampleChoice->GetSelection();
}

void JZAudioEffectsDialog::ProcessAudio(
  short* pBuffer,
  int numSamples,
  int channels,
  int sampleRate)
{
  int activeTab = mpNotebook->GetSelection();

  switch (activeTab)
  {
    case 0: // 3-Band EQ
    {
      JZEqualizerSettings eqSettings;
      eqSettings.bassGainDB = static_cast<float>(mpEqBassSlider->GetValue());
      eqSettings.midGainDB = static_cast<float>(mpEqMidSlider->GetValue());
      eqSettings.trebleGainDB = static_cast<float>(mpEqTrebleSlider->GetValue());

      JZ3BandEqualizer eq(static_cast<float>(sampleRate));
      eq.SetSettings(eqSettings);
      eq.Process(pBuffer, numSamples, channels);
      break;
    }

    case 1: // Reverb
    {
      JZReverbSettings revSettings;
      revSettings.roomSize = static_cast<float>(mpRevRoomSlider->GetValue()) / 100.0f;
      revSettings.damping = static_cast<float>(mpRevDampSlider->GetValue()) / 100.0f;
      revSettings.width = static_cast<float>(mpRevWidthSlider->GetValue()) / 100.0f;
      revSettings.wet = static_cast<float>(mpRevWetSlider->GetValue()) / 100.0f;
      revSettings.dry = static_cast<float>(mpRevDrySlider->GetValue()) / 100.0f;

      JZFreeverb reverb(static_cast<float>(sampleRate));
      reverb.SetSettings(revSettings);
      reverb.Process(pBuffer, numSamples, channels);
      break;
    }

    case 2: // Stereo Delay
    {
      JZDelaySettings delaySettings;
      float dMs = static_cast<float>(mpDelayTimeSlider->GetValue());
      delaySettings.delayTimeMsL = dMs;
      delaySettings.delayTimeMsR = dMs * 1.25f;
      delaySettings.feedback = static_cast<float>(mpDelayFeedbackSlider->GetValue()) / 100.0f;
      delaySettings.damp = static_cast<float>(mpDelayDampSlider->GetValue()) / 100.0f;
      delaySettings.wet = static_cast<float>(mpDelayWetSlider->GetValue()) / 100.0f;
      delaySettings.dry = static_cast<float>(mpDelayDrySlider->GetValue()) / 100.0f;

      JZStereoDelay delay(static_cast<float>(sampleRate));
      delay.SetSettings(delaySettings);
      delay.Process(pBuffer, numSamples, channels);
      break;
    }

    case 3: // Stereo Chorus
    {
      JZChorusSettings choSettings;
      choSettings.rateHz = static_cast<float>(mpChoRateSlider->GetValue()) / 10.0f;
      choSettings.depthMs = static_cast<float>(mpChoDepthSlider->GetValue());
      choSettings.feedback = static_cast<float>(mpChoFeedbackSlider->GetValue()) / 100.0f;
      choSettings.mix = static_cast<float>(mpChoMixSlider->GetValue()) / 100.0f;

      JZStereoChorus chorus(static_cast<float>(sampleRate));
      chorus.SetSettings(choSettings);
      chorus.Process(pBuffer, numSamples, channels);
      break;
    }

    case 4: // Limiter & Overdrive
    {
      JZLimiterSettings limSettings;
      float driveVal = static_cast<float>(mpLimDriveSlider->GetValue());
      limSettings.drive = 1.0f + (driveVal / 24.0f) * 4.0f;
      limSettings.ceilingDB = static_cast<float>(mpLimCeilingSlider->GetValue());
      limSettings.enableOverdrive = mpLimSoftClipCheck->IsChecked();

      JZAudioLimiterDistortion limiter;
      limiter.SetSettings(limSettings);
      limiter.Process(pBuffer, numSamples, channels);
      break;
    }
  }
}

void JZAudioEffectsDialog::OnPreviewButton(wxCommandEvent& Event)
{
  std::vector<short> previewSamples;
  int sampleRate = 44100;
  int channels = 1;

  JZSample* pSample = 0;
  JZSampleSet* pSet = GetProjectSampleSet(mpProject);
  if (pSet && mSelectedSampleIndex >= 0 && mSelectedSampleIndex < JZSampleSet::eSampleCount)
  {
    pSample = &((*pSet)[mSelectedSampleIndex]);
  }

  if (pSample && !pSample->IsEmpty() && pSample->GetData())
  {
    sampleRate = pSample->GetSamplingRate();
    channels = pSample->GetChannelCount();
    previewSamples.assign(pSample->GetData(), pSample->GetData() + pSample->GetLength());
  }
  else
  {
    // Generate an analog synth test sound for preview
    JZSynthDrumParams drumP = JZSynthDrumGenerator::GetPreset(eDrum808Snare);
    previewSamples = JZSynthDrumGenerator::Generate(drumP, sampleRate);
  }

  // Process DSP
  ProcessAudio(previewSamples.data(), static_cast<int>(previewSamples.size()), channels, sampleRate);

  // Write and play temp WAV
  wxString tempDir = wxStandardPaths::Get().GetTempDir();
  wxFileName tempWav(tempDir, "jazz_dsp_preview", "wav");
  std::string filePath = std::string(tempWav.GetFullPath().mb_str());

  if (JZSoundIO::SaveWav(filePath, previewSamples, sampleRate, channels))
  {
    JZSoundIO::PlayWav(filePath);
  }
}

void JZAudioEffectsDialog::OnApplyButton(wxCommandEvent& Event)
{
  JZSample* pSample = 0;
  JZSampleSet* pSet = GetProjectSampleSet(mpProject);
  if (pSet && mSelectedSampleIndex >= 0 && mSelectedSampleIndex < JZSampleSet::eSampleCount)
  {
    pSample = &((*pSet)[mSelectedSampleIndex]);
  }

  if (!pSample || pSample->IsEmpty() || !pSample->GetData())
  {
    wxMessageBox(
      "The selected sample slot is empty. Please load or generate audio into this slot first.",
      "No Sample Data",
      wxOK | wxICON_WARNING,
      this);
    return;
  }

  // Apply effect directly to sample buffer
  ProcessAudio(
    pSample->GetData(),
    pSample->GetLength(),
    pSample->GetChannelCount(),
    pSample->GetSamplingRate());

  pSample->Rescale();
  pSample->SaveWave();

  wxMessageBox("DSP Audio Effect applied successfully!", "Jazz++ Audio DSP", wxOK | wxICON_INFORMATION, this);
}

void JZAudioEffectsDialog::OnCloseButton(wxCommandEvent& Event)
{
  EndModal(wxID_CANCEL);
}
