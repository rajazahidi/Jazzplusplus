//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Copyright (C) 2008-2013 Peter J. Stieber, all rights reserved.
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

#include "TrackDialog.h"

#include "../Configuration.h"
#include "../Globals.h"
#include "../Help.h"
#include "../Knob.h"
#include "../Resources.h"
#include "../Track.h"

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/listbox.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

#include <sstream>

using namespace std;

//*****************************************************************************
//*****************************************************************************
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(JZTrackDialog, wxDialog)

  EVT_KNOB_CHANGED(IDC_KB_CHANNEL, JZTrackDialog::OnChannelChange)

  EVT_BUTTON(wxID_HELP, JZTrackDialog::OnHelp)

END_EVENT_TABLE()

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZTrackDialog::JZTrackDialog(JZTrack& Track, wxWindow* pParent)
  : wxDialog(pParent, wxID_ANY, wxString("Track Settings")),
    mTrack(Track),
    mLastTrackChannelWasDrums(Track.IsDrumTrack()),
    mpTrackNameEdit(0),
    mpPatchListBox(0),
    mpChannelValue(0),
    mpChannelKnob(0),
    mpAudioModeCheckBox(0)
{
  mpTrackNameEdit = new wxTextCtrl(this, wxID_ANY);

  mpPatchListBox = new wxListBox(this, wxID_ANY);
  SetPatchListEntries();

  mpChannelValue = new wxStaticText(this, wxID_ANY, "00");

  mpChannelKnob = new JZKnob(this, IDC_KB_CHANNEL, 0, 1, 16);

  mpAudioModeCheckBox = new wxCheckBox(this, wxID_ANY, "Audio Track");

  wxButton* pOkButton = new wxButton(this, wxID_OK, "&OK");
  wxButton* pCancelButton = new wxButton(this, wxID_CANCEL, "Cancel");
  wxButton* pHelpButton = new wxButton(this, wxID_HELP, "Help");
  pOkButton->SetDefault();

  wxBoxSizer* pTopSizer = new wxBoxSizer(wxVERTICAL);

  pTopSizer->Add(
    new wxStaticText(this, wxID_ANY, "Track Name:"),
    0,
    wxALL,
    4);
  pTopSizer->Add(mpTrackNameEdit, 0, wxGROW | wxALL, 4);

  pTopSizer->Add(
    new wxStaticText(this, wxID_ANY, "Patch:"),
    0,
    wxALL,
    4);
  pTopSizer->Add(mpPatchListBox, 0, wxGROW | wxALL, 4);

  wxFlexGridSizer* pFlexGridSizer = new wxFlexGridSizer(1, 3, 4, 2);

  pFlexGridSizer->Add(
    new wxStaticText(this, wxID_ANY, "Channel:"),
    0,
    wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
  pFlexGridSizer->Add(
    mpChannelValue,
    0,
    wxALIGN_CENTER_VERTICAL | wxFIXED_MINSIZE);
  pFlexGridSizer->Add(mpChannelKnob, 0, wxALIGN_CENTER_VERTICAL);

  pTopSizer->Add(pFlexGridSizer, 0, wxCENTER | wxALL, 2);

  pTopSizer->Add(mpAudioModeCheckBox, 0, wxALL, 2);

  wxBoxSizer* pButtonSizer = new wxBoxSizer(wxHORIZONTAL);
  pButtonSizer->Add(pOkButton, 0, wxALL, 5);
  pButtonSizer->Add(pCancelButton, 0, wxALL, 5);
  pButtonSizer->Add(pHelpButton, 0, wxALL, 5);

  pTopSizer->Add(pButtonSizer, 0, wxALIGN_CENTER | wxBOTTOM, 6);

  SetAutoLayout(true);
  SetSizer(pTopSizer);

  pTopSizer->SetSizeHints(this);
  pTopSizer->Fit(this);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZTrackDialog::SetPatchListEntries()
{
  mpPatchListBox->Clear();
  mPatchValues.clear();

  int CurrentPatchValue = mTrack.GetPatch() + (mTrack.GetBank() << 8);
  int SelectionIndex = wxNOT_FOUND;

  if (mTrack.IsDrumTrack())
  {
    const vector<pair<string, int> >& DrumSets = gpConfig->GetDrumSets();
    for (
      vector<pair<string, int> >::const_iterator iDrumSet =
        DrumSets.begin();
      iDrumSet != DrumSets.end();
      ++iDrumSet)
    {
      const string& DrumSet = iDrumSet->first;

      if (!DrumSet.empty())
      {
        int val = iDrumSet->second;
        int idx = mpPatchListBox->Append(DrumSet.c_str());
        mPatchValues.push_back(val);
        if (val == CurrentPatchValue)
        {
          SelectionIndex = idx;
        }
      }
    }
  }
  else
  {
    const vector<pair<string, int> >& VoiceNames = gpConfig->GetVoiceNames();
    for (
      vector<pair<string, int> >::const_iterator iVoiceName =
        VoiceNames.begin();
      iVoiceName != VoiceNames.end();
      ++iVoiceName)
    {
      const string& VoiceName = iVoiceName->first;

      if (!VoiceName.empty())
      {
        int val = iVoiceName->second;
        int idx = mpPatchListBox->Append(VoiceName.c_str());
        mPatchValues.push_back(val);
        if (val == CurrentPatchValue)
        {
          SelectionIndex = idx;
        }
      }
    }
  }

  if (SelectionIndex != wxNOT_FOUND && SelectionIndex >= 0 && SelectionIndex < (int)mpPatchListBox->GetCount())
  {
    mpPatchListBox->SetSelection(SelectionIndex);
  }
  else if (mpPatchListBox->GetCount() > 0)
  {
    mpPatchListBox->SetSelection(0);
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool JZTrackDialog::TransferDataToWindow()
{
  mpTrackNameEdit->ChangeValue(mTrack.GetName());

  SetPatchListEntries();

  int channel = mTrack.mChannel;
  if (channel < 1)
  {
    channel = 1;
  }
  else if (channel > 16)
  {
    channel = 16;
  }

  ostringstream Oss;
  Oss << channel;
  mpChannelValue->SetLabel(Oss.str().c_str());

  mpChannelKnob->SetValue(channel);

  mpAudioModeCheckBox->SetValue(mTrack.GetAudioMode());

  return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool JZTrackDialog::TransferDataFromWindow()
{
  wxString Trackname = mpTrackNameEdit->GetValue();
  mTrack.SetName(Trackname.c_str());

  mTrack.SetChannel(mpChannelKnob->GetValue());
  mTrack.SetAudioMode(mpAudioModeCheckBox->GetValue());

  int Selection = mpPatchListBox->GetSelection();
  if (Selection != wxNOT_FOUND && Selection >= 0 && Selection < (int)mPatchValues.size())
  {
    int PatchVal = mPatchValues[Selection];
    int Bank = (PatchVal & 0x0000ff00) >> 8;
    int Patch = PatchVal & 0x000000ff;
    mTrack.SetBank(Bank);
    mTrack.SetPatch(Patch);
  }

  return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZTrackDialog::OnChannelChange(JZKnobEvent& Event)
{
  int Value = Event.GetValue();
  ostringstream Oss;
  Oss << Value;
  mpChannelValue->SetLabel(Oss.str().c_str());
  mTrack.mChannel = Value;

  // Test to determine if the track channel toggled in or out of drum mode.
  if (mLastTrackChannelWasDrums != mTrack.IsDrumTrack())
  {
    // If it did switch, update the patch list entries safely.
    SetPatchListEntries();
  }

  // Record if the current value for the channel indicates drums.
  mLastTrackChannelWasDrums = mTrack.IsDrumTrack();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZTrackDialog::OnHelp(wxCommandEvent& Event)
{
  JZHelp::Instance().ShowTopic("Track Dialog");
}
