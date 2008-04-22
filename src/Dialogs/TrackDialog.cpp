//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Copyright (C) 2008 Peter J. Stieber, all rights reserved.
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

#include "WxWidgets.h"

#include "TrackDialog.h"
#include "../Track.h"
#include "../Configuration.h"
#include "../Globals.h"

using namespace std;

//*****************************************************************************
//*****************************************************************************
JZTrackDialog::JZTrackDialog(JZTrack& Track, wxWindow* pParent)
  : wxDialog(pParent, wxID_ANY, wxString("Track Settings")),
    mTrack(Track),
    mpTrackNameEdit(0),
    mpPatchListBox(0)
{
  mpTrackNameEdit = new wxTextCtrl(this, wxID_ANY);

  mpPatchListBox = new wxListBox(this, wxID_ANY);
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
        mpPatchListBox->Append(DrumSet.c_str());
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
        mpPatchListBox->Append(VoiceName.c_str());
      }
    }
  }

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
bool JZTrackDialog::TransferDataToWindow()
{
  mpTrackNameEdit->ChangeValue(mTrack.GetName());

  int PatchIndex = mTrack.GetPatch() + (mTrack.GetBank() << 8);
  mpPatchListBox->SetSelection(PatchIndex);

  return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool JZTrackDialog::TransferDataFromWindow()
{
  wxString Trackname = mpTrackNameEdit->GetValue();
  mTrack.SetName(Trackname.c_str());

  int Selection = mpPatchListBox->GetSelection();
  if (Selection != wxNOT_FOUND)
  {
    int Patch = Selection & 0x000000ff;;
    int Bank = (Selection & 0x0000ff00) >> 8;
    mTrack.SetPatch(Patch);
    mTrack.SetBank(Bank);
  }

  return true;
}
