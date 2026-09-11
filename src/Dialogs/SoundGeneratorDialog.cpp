//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Sound Generator Dialog Implementation
// Copyright (C) 2026 Raja Zahidi
//*****************************************************************************

#include "SoundGeneratorDialog.h"

#include "../Audio.h"
#include "../Events.h"
#include "../Globals.h"
#include "../Player.h"
#include "../Project.h"
#include "../Sample.h"
#include "../Song.h"
#include "../Track.h"

#include <wx/button.h>
#include <wx/choice.h>
#include <wx/filedlg.h>
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

enum
{
  ID_SFX_PRESET_CHOICE = 12001,
  ID_DRUM_PRESET_CHOICE,
  ID_SFX_BTN_PREVIEW,
  ID_SFX_BTN_EXPORT,
  ID_SFX_BTN_INSERT,
  ID_DRUM_BTN_PREVIEW,
  ID_DRUM_BTN_EXPORT,
  ID_DRUM_BTN_INSERT,
  ID_BTN_CLOSE
};

BEGIN_EVENT_TABLE(JZSoundGeneratorDialog, wxDialog)
  EVT_CHOICE(ID_SFX_PRESET_CHOICE, JZSoundGeneratorDialog::OnSFXPresetChange)
  EVT_CHOICE(ID_DRUM_PRESET_CHOICE, JZSoundGeneratorDialog::OnDrumPresetChange)
  EVT_BUTTON(ID_SFX_BTN_PREVIEW, JZSoundGeneratorDialog::OnSFXPreview)
  EVT_BUTTON(ID_SFX_BTN_EXPORT, JZSoundGeneratorDialog::OnSFXExportWav)
  EVT_BUTTON(ID_SFX_BTN_INSERT, JZSoundGeneratorDialog::OnSFXInsertTrack)
  EVT_BUTTON(ID_DRUM_BTN_PREVIEW, JZSoundGeneratorDialog::OnDrumPreview)
  EVT_BUTTON(ID_DRUM_BTN_EXPORT, JZSoundGeneratorDialog::OnDrumExportWav)
  EVT_BUTTON(ID_DRUM_BTN_INSERT, JZSoundGeneratorDialog::OnDrumInsertTrack)
  EVT_BUTTON(ID_BTN_CLOSE, JZSoundGeneratorDialog::OnCloseButton)
END_EVENT_TABLE()

JZSoundGeneratorDialog::JZSoundGeneratorDialog(
  wxWindow* pParent,
  JZProject* pProject,
  int DefaultTrack,
  long DefaultClock)
  : wxDialog(
      pParent,
      wxID_ANY,
      wxString("Procedural Sound FX & Drum Synthesizer"),
      wxDefaultPosition,
      wxDefaultSize,
      wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
    mpProject(pProject),
    mTargetTrackIndex(DefaultTrack),
    mStartClock(DefaultClock),
    mpNotebook(0),
    mpSFXPresetChoice(0),
    mpSFXWaveformChoice(0),
    mpSFXStartFreqSlider(0),
    mpSFXEndFreqSlider(0),
    mpSFXDurationSlider(0),
    mpSFXDutySlider(0),
    mpSFXVolumeSlider(0),
    mpSFXPreviewBtn(0),
    mpSFXExportBtn(0),
    mpSFXInsertBtn(0),
    mpDrumPresetChoice(0),
    mpDrumPitchSlider(0),
    mpDrumPitchDropSlider(0),
    mpDrumDurationSlider(0),
    mpDrumToneMixSlider(0),
    mpDrumClickSlider(0),
    mpDrumOverdriveSlider(0),
    mpDrumKeyChoice(0),
    mpDrumPreviewBtn(0),
    mpDrumExportBtn(0),
    mpDrumInsertBtn(0),
    mpCloseButton(0)
{
  CreateControls();
}

JZSoundGeneratorDialog::~JZSoundGeneratorDialog()
{
}

void JZSoundGeneratorDialog::CreateControls()
{
  wxBoxSizer* pTopSizer = new wxBoxSizer(wxVERTICAL);

  mpNotebook = new wxNotebook(this, wxID_ANY);

  // ==========================================================================
  // Tab 1: Retro Game SFX Synthesizer
  // ==========================================================================
  wxPanel* pSFXPanel = new wxPanel(mpNotebook, wxID_ANY);
  wxBoxSizer* pSFXSizer = new wxBoxSizer(wxVERTICAL);

  wxFlexGridSizer* pSFXGrid = new wxFlexGridSizer(7, 2, 8, 12);
  pSFXGrid->AddGrowableCol(1);

  pSFXGrid->Add(new wxStaticText(pSFXPanel, wxID_ANY, "SFX Preset:"), 0, wxALIGN_CENTER_VERTICAL);
  wxArrayString sfxPresets;
  sfxPresets.Add("Laser / Blaster");
  sfxPresets.Add("Explosion / Bomb");
  sfxPresets.Add("Power-Up / Level Up");
  sfxPresets.Add("Coin / Pickup Chime");
  sfxPresets.Add("Jump Chirp");
  sfxPresets.Add("Hurt / Hit Impact");
  sfxPresets.Add("Sci-Fi Frequency Sweep");
  sfxPresets.Add("Procedural Random SFX");
  mpSFXPresetChoice = new wxChoice(pSFXPanel, ID_SFX_PRESET_CHOICE, wxDefaultPosition, wxDefaultSize, sfxPresets);
  mpSFXPresetChoice->SetSelection(0);
  pSFXGrid->Add(mpSFXPresetChoice, 1, wxEXPAND);

  pSFXGrid->Add(new wxStaticText(pSFXPanel, wxID_ANY, "Oscillator Waveform:"), 0, wxALIGN_CENTER_VERTICAL);
  wxArrayString waveforms;
  waveforms.Add("Square Wave");
  waveforms.Add("Sawtooth Wave");
  waveforms.Add("Sine Wave");
  waveforms.Add("Triangle Wave");
  waveforms.Add("Filtered Noise");
  mpSFXWaveformChoice = new wxChoice(pSFXPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, waveforms);
  mpSFXWaveformChoice->SetSelection(1); // Sawtooth for laser
  pSFXGrid->Add(mpSFXWaveformChoice, 1, wxEXPAND);

  pSFXGrid->Add(new wxStaticText(pSFXPanel, wxID_ANY, "Start Frequency (Hz):"), 0, wxALIGN_CENTER_VERTICAL);
  mpSFXStartFreqSlider = new wxSlider(pSFXPanel, wxID_ANY, 1400, 40, 3000, wxDefaultPosition, wxDefaultSize, wxSL_LABELS);
  pSFXGrid->Add(mpSFXStartFreqSlider, 1, wxEXPAND);

  pSFXGrid->Add(new wxStaticText(pSFXPanel, wxID_ANY, "End Frequency (Hz):"), 0, wxALIGN_CENTER_VERTICAL);
  mpSFXEndFreqSlider = new wxSlider(pSFXPanel, wxID_ANY, 80, 20, 2000, wxDefaultPosition, wxDefaultSize, wxSL_LABELS);
  pSFXGrid->Add(mpSFXEndFreqSlider, 1, wxEXPAND);

  pSFXGrid->Add(new wxStaticText(pSFXPanel, wxID_ANY, "Duration (ms):"), 0, wxALIGN_CENTER_VERTICAL);
  mpSFXDurationSlider = new wxSlider(pSFXPanel, wxID_ANY, 180, 30, 1000, wxDefaultPosition, wxDefaultSize, wxSL_LABELS);
  pSFXGrid->Add(mpSFXDurationSlider, 1, wxEXPAND);

  pSFXGrid->Add(new wxStaticText(pSFXPanel, wxID_ANY, "Duty Cycle (%):"), 0, wxALIGN_CENTER_VERTICAL);
  mpSFXDutySlider = new wxSlider(pSFXPanel, wxID_ANY, 50, 10, 90, wxDefaultPosition, wxDefaultSize, wxSL_LABELS);
  pSFXGrid->Add(mpSFXDutySlider, 1, wxEXPAND);

  pSFXGrid->Add(new wxStaticText(pSFXPanel, wxID_ANY, "Volume (%):"), 0, wxALIGN_CENTER_VERTICAL);
  mpSFXVolumeSlider = new wxSlider(pSFXPanel, wxID_ANY, 85, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_LABELS);
  pSFXGrid->Add(mpSFXVolumeSlider, 1, wxEXPAND);

  pSFXSizer->Add(pSFXGrid, 0, wxEXPAND | wxALL, 12);

  // SFX Buttons
  wxBoxSizer* pSFXBtnRow = new wxBoxSizer(wxHORIZONTAL);
  mpSFXPreviewBtn = new wxButton(pSFXPanel, ID_SFX_BTN_PREVIEW, "Play Preview");
  pSFXBtnRow->Add(mpSFXPreviewBtn, 0, wxRIGHT, 8);

  mpSFXExportBtn = new wxButton(pSFXPanel, ID_SFX_BTN_EXPORT, "Export WAV...");
  pSFXBtnRow->Add(mpSFXExportBtn, 0, wxRIGHT, 8);

  mpSFXInsertBtn = new wxButton(pSFXPanel, ID_SFX_BTN_INSERT, "Insert into Active Track");
  pSFXBtnRow->Add(mpSFXInsertBtn, 0);

  pSFXSizer->Add(pSFXBtnRow, 0, wxALIGN_CENTER | wxALL, 10);
  pSFXPanel->SetSizer(pSFXSizer);
  mpNotebook->AddPage(pSFXPanel, "Retro Game SFX");

  // ==========================================================================
  // Tab 2: Vintage Analog Drum Synthesizer (808)
  // ==========================================================================
  wxPanel* pDrumPanel = new wxPanel(mpNotebook, wxID_ANY);
  wxBoxSizer* pDrumSizer = new wxBoxSizer(wxVERTICAL);

  wxFlexGridSizer* pDrumGrid = new wxFlexGridSizer(7, 2, 8, 12);
  pDrumGrid->AddGrowableCol(1);

  pDrumGrid->Add(new wxStaticText(pDrumPanel, wxID_ANY, "Drum Voice Preset:"), 0, wxALIGN_CENTER_VERTICAL);
  wxArrayString drumPresets;
  drumPresets.Add("808 Bass Drum (Sub Kick)");
  drumPresets.Add("808 Snare Drum");
  drumPresets.Add("Analog Closed Hi-Hat");
  drumPresets.Add("Analog Open Hi-Hat");
  drumPresets.Add("808 Hand Clap");
  drumPresets.Add("808 Low/Mid Tom");
  mpDrumPresetChoice = new wxChoice(pDrumPanel, ID_DRUM_PRESET_CHOICE, wxDefaultPosition, wxDefaultSize, drumPresets);
  mpDrumPresetChoice->SetSelection(0);
  pDrumGrid->Add(mpDrumPresetChoice, 1, wxEXPAND);

  pDrumGrid->Add(new wxStaticText(pDrumPanel, wxID_ANY, "Base Pitch (Hz):"), 0, wxALIGN_CENTER_VERTICAL);
  mpDrumPitchSlider = new wxSlider(pDrumPanel, wxID_ANY, 48, 30, 400, wxDefaultPosition, wxDefaultSize, wxSL_LABELS);
  pDrumGrid->Add(mpDrumPitchSlider, 1, wxEXPAND);

  pDrumGrid->Add(new wxStaticText(pDrumPanel, wxID_ANY, "Pitch Drop Amount (Hz):"), 0, wxALIGN_CENTER_VERTICAL);
  mpDrumPitchDropSlider = new wxSlider(pDrumPanel, wxID_ANY, 125, 0, 300, wxDefaultPosition, wxDefaultSize, wxSL_LABELS);
  pDrumGrid->Add(mpDrumPitchDropSlider, 1, wxEXPAND);

  pDrumGrid->Add(new wxStaticText(pDrumPanel, wxID_ANY, "Decay Time (ms):"), 0, wxALIGN_CENTER_VERTICAL);
  mpDrumDurationSlider = new wxSlider(pDrumPanel, wxID_ANY, 500, 30, 1000, wxDefaultPosition, wxDefaultSize, wxSL_LABELS);
  pDrumGrid->Add(mpDrumDurationSlider, 1, wxEXPAND);

  pDrumGrid->Add(new wxStaticText(pDrumPanel, wxID_ANY, "Tone / Noise Mix (%):"), 0, wxALIGN_CENTER_VERTICAL);
  mpDrumToneMixSlider = new wxSlider(pDrumPanel, wxID_ANY, 96, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_LABELS);
  pDrumGrid->Add(mpDrumToneMixSlider, 1, wxEXPAND);

  pDrumGrid->Add(new wxStaticText(pDrumPanel, wxID_ANY, "Transient Click (%):"), 0, wxALIGN_CENTER_VERTICAL);
  mpDrumClickSlider = new wxSlider(pDrumPanel, wxID_ANY, 35, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_LABELS);
  pDrumGrid->Add(mpDrumClickSlider, 1, wxEXPAND);

  pDrumGrid->Add(new wxStaticText(pDrumPanel, wxID_ANY, "Analog Overdrive (%):"), 0, wxALIGN_CENTER_VERTICAL);
  mpDrumOverdriveSlider = new wxSlider(pDrumPanel, wxID_ANY, 30, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_LABELS);
  pDrumGrid->Add(mpDrumOverdriveSlider, 1, wxEXPAND);

  pDrumSizer->Add(pDrumGrid, 0, wxEXPAND | wxALL, 12);

  // Trigger mapping row
  wxBoxSizer* pDrumKeyRow = new wxBoxSizer(wxHORIZONTAL);
  pDrumKeyRow->Add(new wxStaticText(pDrumPanel, wxID_ANY, "MIDI Trigger Note:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
  wxArrayString midiKeys;
  midiKeys.Add("Key 36 - Bass Drum 1");
  midiKeys.Add("Key 38 - Acoustic Snare");
  midiKeys.Add("Key 39 - Hand Clap");
  midiKeys.Add("Key 42 - Closed Hi-Hat");
  midiKeys.Add("Key 45 - Low Tom");
  midiKeys.Add("Key 46 - Open Hi-Hat");
  midiKeys.Add("Key 60 - Middle C");
  mpDrumKeyChoice = new wxChoice(pDrumPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, midiKeys);
  mpDrumKeyChoice->SetSelection(0);
  pDrumKeyRow->Add(mpDrumKeyChoice, 1, wxEXPAND);
  pDrumSizer->Add(pDrumKeyRow, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);

  // Drum Buttons
  wxBoxSizer* pDrumBtnRow = new wxBoxSizer(wxHORIZONTAL);
  mpDrumPreviewBtn = new wxButton(pDrumPanel, ID_DRUM_BTN_PREVIEW, "Play Preview");
  pDrumBtnRow->Add(mpDrumPreviewBtn, 0, wxRIGHT, 8);

  mpDrumExportBtn = new wxButton(pDrumPanel, ID_DRUM_BTN_EXPORT, "Export WAV...");
  pDrumBtnRow->Add(mpDrumExportBtn, 0, wxRIGHT, 8);

  mpDrumInsertBtn = new wxButton(pDrumPanel, ID_DRUM_BTN_INSERT, "Insert into Active Track");
  pDrumBtnRow->Add(mpDrumInsertBtn, 0);

  pDrumSizer->Add(pDrumBtnRow, 0, wxALIGN_CENTER | wxALL, 10);
  pDrumPanel->SetSizer(pDrumSizer);
  mpNotebook->AddPage(pDrumPanel, "Analog Synth Drums");

  pTopSizer->Add(mpNotebook, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);

  // Close Button
  wxBoxSizer* pBottomSizer = new wxBoxSizer(wxHORIZONTAL);
  pBottomSizer->AddStretchSpacer();
  mpCloseButton = new wxButton(this, ID_BTN_CLOSE, "Close");
  pBottomSizer->Add(mpCloseButton, 0);
  pTopSizer->Add(pBottomSizer, 0, wxEXPAND | wxALL, 10);

  SetSizer(pTopSizer);
  pTopSizer->Fit(this);

  wxSize sz = GetSize();
  if (sz.x < 740) sz.x = 740;
  if (sz.y < 600) sz.y = 600;
  SetSize(sz);
  SetMinSize(wxSize(680, 540));
  CentreOnParent();
}

void JZSoundGeneratorDialog::OnSFXPresetChange(wxCommandEvent& Event)
{
  int sel = mpSFXPresetChoice->GetSelection();
  JZRetroSFXParams p = JZRetroSFXGenerator::GetPreset(static_cast<JZRetroSFXType>(sel));

  mpSFXWaveformChoice->SetSelection(static_cast<int>(p.waveform));
  mpSFXStartFreqSlider->SetValue(static_cast<int>(p.startFrequency));
  mpSFXEndFreqSlider->SetValue(static_cast<int>(p.endFrequency));
  mpSFXDurationSlider->SetValue(static_cast<int>(p.duration * 1000.0f));
  mpSFXDutySlider->SetValue(static_cast<int>(p.dutyCycle * 100.0f));
  mpSFXVolumeSlider->SetValue(static_cast<int>(p.volume * 100.0f));
}

void JZSoundGeneratorDialog::OnDrumPresetChange(wxCommandEvent& Event)
{
  int sel = mpDrumPresetChoice->GetSelection();
  JZSynthDrumParams p = JZSynthDrumGenerator::GetPreset(static_cast<JZSynthDrumType>(sel));

  mpDrumPitchSlider->SetValue(static_cast<int>(p.baseFrequency));
  mpDrumPitchDropSlider->SetValue(static_cast<int>(p.pitchDropHz));
  mpDrumDurationSlider->SetValue(static_cast<int>(p.duration * 1000.0f));
  mpDrumToneMixSlider->SetValue(static_cast<int>(p.toneMix * 100.0f));
  mpDrumClickSlider->SetValue(static_cast<int>(p.clickTransient * 100.0f));
  mpDrumOverdriveSlider->SetValue(static_cast<int>(p.overdrive * 100.0f));

  // Auto set recommended trigger key
  switch (sel)
  {
    case 0: mpDrumKeyChoice->SetSelection(0); break; // 36 Kick
    case 1: mpDrumKeyChoice->SetSelection(1); break; // 38 Snare
    case 2: mpDrumKeyChoice->SetSelection(3); break; // 42 Closed HH
    case 3: mpDrumKeyChoice->SetSelection(5); break; // 46 Open HH
    case 4: mpDrumKeyChoice->SetSelection(2); break; // 39 Clap
    case 5: mpDrumKeyChoice->SetSelection(4); break; // 45 Tom
  }
}

std::vector<short> JZSoundGeneratorDialog::GenerateCurrentSFX()
{
  JZRetroSFXParams p;
  p.waveform = static_cast<JZWaveform>(mpSFXWaveformChoice->GetSelection());
  p.startFrequency = static_cast<float>(mpSFXStartFreqSlider->GetValue());
  p.endFrequency = static_cast<float>(mpSFXEndFreqSlider->GetValue());
  p.duration = static_cast<float>(mpSFXDurationSlider->GetValue()) / 1000.0f;
  p.dutyCycle = static_cast<float>(mpSFXDutySlider->GetValue()) / 100.0f;
  p.volume = static_cast<float>(mpSFXVolumeSlider->GetValue()) / 100.0f;

  int presetSel = mpSFXPresetChoice->GetSelection();
  if (presetSel == eSFXPowerUp)
  {
    p.frequencyJumpTime = 0.45f;
    p.frequencyJumpFactor = 1.33484f;
  }
  else if (presetSel == eSFXCoin)
  {
    p.frequencyJumpTime = 0.10f;
    p.frequencyJumpFactor = 1.33484f;
  }
  else if (presetSel == eSFXHurt)
  {
    p.vibratoDepth = 45.0f;
    p.vibratoSpeed = 30.0f;
  }
  else if (presetSel == eSFXSweep)
  {
    p.vibratoDepth = 50.0f;
    p.vibratoSpeed = 14.0f;
  }

  return JZRetroSFXGenerator::Generate(p, 44100);
}

std::vector<short> JZSoundGeneratorDialog::GenerateCurrentDrum()
{
  JZSynthDrumParams p;
  p.baseFrequency = static_cast<float>(mpDrumPitchSlider->GetValue());
  p.pitchDropHz = static_cast<float>(mpDrumPitchDropSlider->GetValue());
  p.duration = static_cast<float>(mpDrumDurationSlider->GetValue()) / 1000.0f;
  p.toneMix = static_cast<float>(mpDrumToneMixSlider->GetValue()) / 100.0f;
  p.clickTransient = static_cast<float>(mpDrumClickSlider->GetValue()) / 100.0f;
  p.overdrive = static_cast<float>(mpDrumOverdriveSlider->GetValue()) / 100.0f;
  p.volume = 0.90f;

  int drumSel = mpDrumPresetChoice->GetSelection();
  if (drumSel == eDrum808Snare)
  {
    p.noiseFilterCutoff = 3800.0f;
    p.noiseFilterQ = 1.8f;
  }
  else if (drumSel == eDrumHiHatClosed || drumSel == eDrumHiHatOpen)
  {
    p.noiseFilterCutoff = 7500.0f;
    p.noiseFilterQ = 2.0f;
  }
  else if (drumSel == eDrum808Clap)
  {
    p.noiseFilterCutoff = 2400.0f;
    p.noiseFilterQ = 1.4f;
  }

  return JZSynthDrumGenerator::Generate(p, 44100);
}

void JZSoundGeneratorDialog::OnSFXPreview(wxCommandEvent& Event)
{
  std::vector<short> samples = GenerateCurrentSFX();
  wxString tempDir = wxStandardPaths::Get().GetTempDir();
  wxFileName tempWav(tempDir, "jazz_preview_sfx", "wav");
  std::string filePath = std::string(tempWav.GetFullPath().mb_str());

  if (JZSoundIO::SaveWav(filePath, samples, 44100, 1))
  {
    JZSoundIO::PlayWav(filePath);
  }
}

void JZSoundGeneratorDialog::OnDrumPreview(wxCommandEvent& Event)
{
  std::vector<short> samples = GenerateCurrentDrum();
  wxString tempDir = wxStandardPaths::Get().GetTempDir();
  wxFileName tempWav(tempDir, "jazz_preview_drum", "wav");
  std::string filePath = std::string(tempWav.GetFullPath().mb_str());

  if (JZSoundIO::SaveWav(filePath, samples, 44100, 1))
  {
    JZSoundIO::PlayWav(filePath);
  }
}

void JZSoundGeneratorDialog::OnSFXExportWav(wxCommandEvent& Event)
{
  std::vector<short> samples = GenerateCurrentSFX();
  wxFileDialog saveDlg(
    this,
    "Export Retro SFX WAV",
    "",
    "retro_sfx.wav",
    "WAV files (*.wav)|*.wav",
    wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

  if (saveDlg.ShowModal() == wxID_OK)
  {
    std::string path = std::string(saveDlg.GetPath().mb_str());
    if (JZSoundIO::SaveWav(path, samples, 44100, 1))
    {
      wxMessageBox("WAV file exported successfully!", "Export Success", wxOK | wxICON_INFORMATION, this);
    }
    else
    {
      wxMessageBox("Failed to export WAV file.", "Export Error", wxOK | wxICON_ERROR, this);
    }
  }
}

void JZSoundGeneratorDialog::OnDrumExportWav(wxCommandEvent& Event)
{
  std::vector<short> samples = GenerateCurrentDrum();
  wxFileDialog saveDlg(
    this,
    "Export Analog Synth Drum WAV",
    "",
    "synth_drum.wav",
    "WAV files (*.wav)|*.wav",
    wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

  if (saveDlg.ShowModal() == wxID_OK)
  {
    std::string path = std::string(saveDlg.GetPath().mb_str());
    if (JZSoundIO::SaveWav(path, samples, 44100, 1))
    {
      wxMessageBox("WAV file exported successfully!", "Export Success", wxOK | wxICON_INFORMATION, this);
    }
    else
    {
      wxMessageBox("Failed to export WAV file.", "Export Error", wxOK | wxICON_ERROR, this);
    }
  }
}

void JZSoundGeneratorDialog::OnSFXInsertTrack(wxCommandEvent& Event)
{
  if (!mpProject)
  {
    wxMessageBox("No active song available.", "Error", wxOK | wxICON_ERROR, this);
    return;
  }

  if (mTargetTrackIndex < 0 || mTargetTrackIndex >= mpProject->GetTrackCount())
  {
    mTargetTrackIndex = 0;
  }
  JZTrack* pTrack = mpProject->GetTrack(mTargetTrackIndex);
  if (!pTrack)
  {
    wxMessageBox("Target track not found.", "Error", wxOK | wxICON_ERROR, this);
    return;
  }

  std::vector<short> samples = GenerateCurrentSFX();
  wxString label = mpSFXPresetChoice->GetStringSelection();

  // Save to persistent audio folder in temp
  wxString tempDir = wxStandardPaths::Get().GetTempDir();
  wxString safeName = label.Lower();
  safeName.Replace(" ", "_");
  wxFileName tempWav(tempDir, safeName, "wav");
  std::string filePath = std::string(tempWav.GetFullPath().mb_str());
  JZSoundIO::SaveWav(filePath, samples, 44100, 1);

  // Trigger Note-On (Middle C = 60 by default for SFX)
  JZSoundIO::InsertNoteTrigger(pTrack, mStartClock, 60, 100, 240);
  mpProject->NewUndoBuffer();

  wxString msg = wxString::Format("Sound Effect inserted into Track %d at clock %ld!", mTargetTrackIndex + 1, mStartClock);
  wxMessageBox(msg, "Insert Success", wxOK | wxICON_INFORMATION, this);
}

void JZSoundGeneratorDialog::OnDrumInsertTrack(wxCommandEvent& Event)
{
  if (!mpProject)
  {
    wxMessageBox("No active song available.", "Error", wxOK | wxICON_ERROR, this);
    return;
  }

  if (mTargetTrackIndex < 0 || mTargetTrackIndex >= mpProject->GetTrackCount())
  {
    mTargetTrackIndex = 0;
  }
  JZTrack* pTrack = mpProject->GetTrack(mTargetTrackIndex);
  if (!pTrack)
  {
    wxMessageBox("Target track not found.", "Error", wxOK | wxICON_ERROR, this);
    return;
  }

  std::vector<short> samples = GenerateCurrentDrum();
  wxString label = mpDrumPresetChoice->GetStringSelection();

  // Selected key
  int targetMidiKey = 36;
  switch (mpDrumKeyChoice->GetSelection())
  {
    case 0: targetMidiKey = 36; break;
    case 1: targetMidiKey = 38; break;
    case 2: targetMidiKey = 39; break;
    case 3: targetMidiKey = 42; break;
    case 4: targetMidiKey = 45; break;
    case 5: targetMidiKey = 46; break;
    case 6: targetMidiKey = 60; break;
  }

  // Save WAV
  wxString tempDir = wxStandardPaths::Get().GetTempDir();
  wxString safeName = label.Lower();
  safeName.Replace(" ", "_");
  wxFileName tempWav(tempDir, safeName, "wav");
  std::string filePath = std::string(tempWav.GetFullPath().mb_str());
  JZSoundIO::SaveWav(filePath, samples, 44100, 1);

  // If sample set available, assign sample to targetMidiKey
  JZSampleSet* pSampleSet = 0;
  if (mpProject && mpProject->GetPlayer())
  {
    pSampleSet = mpProject->GetPlayer()->GetSampleSet();
  }
  else if (gpMidiPlayer)
  {
    pSampleSet = gpMidiPlayer->GetSampleSet();
  }

  if (pSampleSet && targetMidiKey >= 0 && targetMidiKey < JZSampleSet::eSampleCount)
  {
    JZSample* pSample = &((*pSampleSet)[targetMidiKey]);
    pSample->SetFileName(filePath);
    pSample->SetLabel(std::string(label.mb_str()));
    pSample->LoadWav();
  }

  // Insert trigger NoteOn
  JZSoundIO::InsertNoteTrigger(pTrack, mStartClock, targetMidiKey, 110, 120);
  mpProject->NewUndoBuffer();

  wxString msg = wxString::Format("Synth Drum inserted into Track %d (Key %d) at clock %ld!", mTargetTrackIndex + 1, targetMidiKey, mStartClock);
  wxMessageBox(msg, "Insert Success", wxOK | wxICON_INFORMATION, this);
}

void JZSoundGeneratorDialog::OnCloseButton(wxCommandEvent& Event)
{
  EndModal(wxID_CANCEL);
}
