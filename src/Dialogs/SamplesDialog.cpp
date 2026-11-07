//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Copyright (C) 2010 Peter J. Stieber, all rights reserved.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//*****************************************************************************

#include "SamplesDialog.h"

#include "../Audio.h"
#include "../Globals.h"
#include "../Help.h"
#include "../Resources.h"
#include "../Sample.h"
#include "SoundGeneratorDialog.h"

#include <wx/button.h>
#include <wx/filedlg.h>
#include <wx/filename.h>
#include <wx/listbox.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/slider.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

enum
{
  IDC_BN_PLAY_SAMPLE = wxID_HIGHEST + 401,
  IDC_BN_CLEAR_SAMPLE
};

//*****************************************************************************
//*****************************************************************************
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(JZSamplesDialog, wxDialog)
  EVT_BUTTON(IDC_BN_SD_FILE_SELECT_BROWSE, JZSamplesDialog::OnSelectSampleFile)
  EVT_LISTBOX(wxID_ANY, JZSamplesDialog::OnListBoxSelect)
  EVT_BUTTON(IDC_BN_PLAY_SAMPLE, JZSamplesDialog::OnPlaySample)
  EVT_BUTTON(IDC_BN_CLEAR_SAMPLE, JZSamplesDialog::OnClearSample)
  EVT_BUTTON(wxID_OK, JZSamplesDialog::OnOk)
  EVT_BUTTON(wxID_CANCEL, JZSamplesDialog::OnCancel)
  EVT_CLOSE(JZSamplesDialog::OnClose)
  EVT_BUTTON(wxID_HELP, JZSamplesDialog::OnHelp)
END_EVENT_TABLE()

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZSamplesDialog::JZSamplesDialog(wxWindow* pParent, JZSampleSet& SampleSet)
  : wxDialog(
      pParent,
      wxID_ANY,
      wxString("Sample Set Settings"),
      wxDefaultPosition,
      wxDefaultSize,
      wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
    mSampleSet(SampleSet),
    mCurrentIndex(0),
    mpListBox(0),
    mpLabelEdit(0),
    mpFileNameEdit(0),
    mpFileNameBrowseButton(0),
    mpPlayButton(0),
    mpClearButton(0),
    mpVolumeSlider(0),
    mpPanSlider(0),
    mpPitchSlider(0)
{
  wxArrayString SampleNames;
  for (int Index = 0; Index < JZSampleSet::eSampleCount; ++Index)
  {
    wxString Entry;
    std::string label = SampleSet.GetSampleLabel(Index);
    if (label.empty())
    {
      label = "(empty)";
    }
    Entry << Index + 1 << ": " << label;
    SampleNames.push_back(Entry);
  }

  mpListBox = new wxListBox(
    this,
    wxID_ANY,
    wxDefaultPosition,
    wxSize(240, 200),
    SampleNames,
    wxLB_SINGLE | wxLB_NEEDED_SB);

  mpLabelEdit = new wxTextCtrl(this, wxID_ANY);
  mpFileNameEdit = new wxTextCtrl(this, wxID_ANY);

  mpFileNameBrowseButton = new wxButton(
    this,
    IDC_BN_SD_FILE_SELECT_BROWSE,
    "Browse...");

  mpPlayButton = new wxButton(this, IDC_BN_PLAY_SAMPLE, "Audition / Play");
  mpClearButton = new wxButton(this, IDC_BN_CLEAR_SAMPLE, "Clear Slot");

  mpVolumeSlider = new wxSlider(
    this,
    wxID_ANY,
    64,
    0,
    127,
    wxDefaultPosition,
    wxDefaultSize,
    wxSL_LABELS);

  mpPanSlider = new wxSlider(
    this,
    wxID_ANY,
    0,
    -63,
    63,
    wxDefaultPosition,
    wxDefaultSize,
    wxSL_LABELS);

  mpPitchSlider = new wxSlider(
    this,
    wxID_ANY,
    0,
    -12,
    12,
    wxDefaultPosition,
    wxDefaultSize,
    wxSL_LABELS);

  wxButton* pOkButton = new wxButton(this, wxID_OK, "&OK");
  wxButton* pCancelButton = new wxButton(this, wxID_CANCEL, "Cancel");
  wxButton* pHelpButton = new wxButton(this, wxID_HELP, "Help");
  pOkButton->SetDefault();

  wxBoxSizer* pTopSizer = new wxBoxSizer(wxVERTICAL);
  wxBoxSizer* pMainRow = new wxBoxSizer(wxHORIZONTAL);

  // Left column: Sample slot list
  wxBoxSizer* pListSizer = new wxBoxSizer(wxVERTICAL);
  pListSizer->Add(new wxStaticText(this, wxID_ANY, "Sample Slots (1..128):"), 0, wxBOTTOM, 4);
  pListSizer->Add(mpListBox, 1, wxEXPAND);
  pMainRow->Add(pListSizer, 0, wxEXPAND | wxRIGHT, 12);

  // Right column: Sample editor controls
  wxBoxSizer* pEditorSizer = new wxBoxSizer(wxVERTICAL);

  wxFlexGridSizer* pFlexGridSizer = new wxFlexGridSizer(2, 3, 6, 6);
  pFlexGridSizer->AddGrowableCol(1);

  pFlexGridSizer->Add(new wxStaticText(this, wxID_ANY, "Label:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
  pFlexGridSizer->Add(mpLabelEdit, 1, wxEXPAND | wxALIGN_CENTER_VERTICAL);
  pFlexGridSizer->Add(mpPlayButton, 0, wxALIGN_CENTER_VERTICAL);

  pFlexGridSizer->Add(new wxStaticText(this, wxID_ANY, "File:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
  pFlexGridSizer->Add(mpFileNameEdit, 1, wxEXPAND | wxALIGN_CENTER_VERTICAL);
  pFlexGridSizer->Add(mpFileNameBrowseButton, 0, wxALIGN_CENTER_VERTICAL);

  pEditorSizer->Add(pFlexGridSizer, 0, wxEXPAND | wxBOTTOM, 8);

  pEditorSizer->Add(new wxStaticText(this, wxID_ANY, "Volume (0..127):"), 0, wxTOP, 4);
  pEditorSizer->Add(mpVolumeSlider, 0, wxEXPAND | wxBOTTOM, 6);

  pEditorSizer->Add(new wxStaticText(this, wxID_ANY, "Pan (-63..+63):"), 0, wxTOP, 4);
  pEditorSizer->Add(mpPanSlider, 0, wxEXPAND | wxBOTTOM, 6);

  pEditorSizer->Add(new wxStaticText(this, wxID_ANY, "Pitch Transpose (-12..+12 semitones):"), 0, wxTOP, 4);
  pEditorSizer->Add(mpPitchSlider, 0, wxEXPAND | wxBOTTOM, 8);

  pEditorSizer->Add(mpClearButton, 0, wxALIGN_LEFT);

  pMainRow->Add(pEditorSizer, 1, wxEXPAND);
  pTopSizer->Add(pMainRow, 1, wxEXPAND | wxALL, 12);

  wxBoxSizer* pButtonSizer = new wxBoxSizer(wxHORIZONTAL);
  pButtonSizer->Add(pOkButton, 0, wxRIGHT, 8);
  pButtonSizer->Add(pCancelButton, 0, wxRIGHT, 8);
  pButtonSizer->Add(pHelpButton, 0);

  pTopSizer->Add(pButtonSizer, 0, wxALIGN_RIGHT | wxLEFT | wxRIGHT | wxBOTTOM, 12);

  SetSizer(pTopSizer);
  pTopSizer->Fit(this);

  wxRect workArea = wxGetClientDisplayRect();
  int targetW = FromDIP(680);
  int targetH = FromDIP(480);
  if (targetW > workArea.GetWidth() - FromDIP(30)) targetW = workArea.GetWidth() - FromDIP(30);
  if (targetH > workArea.GetHeight() - FromDIP(50)) targetH = workArea.GetHeight() - FromDIP(50);
  SetSize(targetW, targetH);
  SetMinSize(FromDIP(wxSize(600, 420)));
  CentreOnScreen();

  if (mpListBox->GetCount() > 0)
  {
    mpListBox->SetSelection(0);
    LoadSampleToControls(0);
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZSamplesDialog::LoadSampleToControls(int Index)
{
  if (Index < 0 || Index >= JZSampleSet::eSampleCount) return;

  mCurrentIndex = Index;
  JZSample& sample = mSampleSet[Index];

  mpLabelEdit->ChangeValue(sample.GetLabel());
  mpFileNameEdit->ChangeValue(sample.GetFileName());
  mpVolumeSlider->SetValue(sample.GetVolume());
  mpPanSlider->SetValue(sample.GetPan());
  mpPitchSlider->SetValue(sample.GetPitch());
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZSamplesDialog::SaveControlsToSample(int Index)
{
  if (Index < 0 || Index >= JZSampleSet::eSampleCount) return;

  JZSample& sample = mSampleSet[Index];
  sample.SetLabel(mpLabelEdit->GetValue().ToStdString());
  sample.SetFileName(mpFileNameEdit->GetValue().ToStdString());
  sample.SetVolume(mpVolumeSlider->GetValue());
  sample.SetPan(mpPanSlider->GetValue());
  sample.SetPitch(mpPitchSlider->GetValue());

  wxString entry;
  std::string label = sample.GetLabel();
  if (label.empty())
  {
    label = "(empty)";
  }
  entry << Index + 1 << ": " << label;
  mpListBox->SetString(Index, entry);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZSamplesDialog::OnListBoxSelect(wxCommandEvent& Event)
{
  int sel = mpListBox->GetSelection();
  if (sel != wxNOT_FOUND && sel != mCurrentIndex)
  {
    SaveControlsToSample(mCurrentIndex);
    LoadSampleToControls(sel);
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZSamplesDialog::OnSelectSampleFile(wxCommandEvent& Event)
{
  int Selection = mpListBox->GetSelection();
  if (Selection != wxNOT_FOUND)
  {
    wxFileDialog FileOpenDialog(
      this,
      "Choose a sound file",
      wxEmptyString,
      wxEmptyString,
      "WAV files (*.wav)|*.wav|All files (*.*)|*.*",
      wxFD_OPEN | wxFD_FILE_MUST_EXIST);

    if (FileOpenDialog.ShowModal() == wxID_OK)
    {
      wxFileName FileName = FileOpenDialog.GetPath();
      mSampleSet[Selection].SetFileName(FileName.GetFullPath().ToStdString());
      mSampleSet[Selection].SetLabel(FileName.GetName().ToStdString());
      mSampleSet[Selection].LoadWav();

      mpFileNameEdit->ChangeValue(FileName.GetFullPath());
      mpLabelEdit->ChangeValue(FileName.GetName());

      wxString Label;
      Label << Selection + 1 << ": " << FileName.GetName();
      mpListBox->SetString(Selection, Label);
    }
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZSamplesDialog::OnPlaySample(wxCommandEvent& Event)
{
  wxString path = mpFileNameEdit->GetValue();
  if (!path.empty() && wxFileExists(path))
  {
    JZSoundIO::PlayWav(path.ToStdString());
  }
  else
  {
    wxMessageBox("No valid WAV audio file loaded in this slot.", "Audition", wxOK | wxICON_INFORMATION, this);
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZSamplesDialog::OnClearSample(wxCommandEvent& Event)
{
  if (mCurrentIndex >= 0 && mCurrentIndex < JZSampleSet::eSampleCount)
  {
    mSampleSet[mCurrentIndex].Clear();
    LoadSampleToControls(mCurrentIndex);

    wxString entry;
    entry << mCurrentIndex + 1 << ": (empty)";
    mpListBox->SetString(mCurrentIndex, entry);
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZSamplesDialog::OnOk(wxCommandEvent& Event)
{
  SaveControlsToSample(mCurrentIndex);
  EndModal(wxID_OK);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZSamplesDialog::OnCancel(wxCommandEvent& Event)
{
  EndModal(wxID_CANCEL);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZSamplesDialog::OnClose(wxCloseEvent& Event)
{
  EndModal(wxID_CANCEL);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZSamplesDialog::OnHelp(wxCommandEvent& Event)
{
  JZHelp::Instance().ShowTopic("Sample Set Settings");
}
