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

#include "MetronomeSettingsDialog.h"

#include "../Metronome.h"
#include "../Configuration.h"
#include "../Globals.h"
#include "../Knob.h"
#include "../Resources.h"

#include <vector>
#include <sstream>

using namespace std;

//*****************************************************************************
//*****************************************************************************
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(JZMetronomeSettingsDialog, wxDialog)

  EVT_KNOB_CHANGED(IDC_KB_VOLUME, JZMetronomeSettingsDialog::OnVolumeChange)

END_EVENT_TABLE()

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZMetronomeSettingsDialog::JZMetronomeSettingsDialog(
  wxWindow* pParent,
  JZMetronomeInfo& MetronomeInfo)
  : wxDialog(pParent, wxID_ANY, wxString("Metronome Settings")),
    mMetronomeInfo(MetronomeInfo),
    mpVelocityKnob(0),
    mpVelocityValue(0),
    mpAccentedCheckBox(0),
    mpNormalListbox(0),
    mpAccentedListbox(0)
{
  mpVelocityKnob = new JZKnob(this, IDC_KB_VOLUME, 100, 0, 127);

  mpVelocityValue = new wxStaticText(this, wxID_ANY, "127");

  mpAccentedCheckBox = new wxCheckBox(this, wxID_ANY, "Use Accented Click");

  mpNormalListbox = new wxListBox(this, wxID_ANY);

  int Selection = 0;
  int Index = 0;
  for (
    vector<pair<string, int> >::const_iterator iPair = gSynthesizerTypes.begin();
    iPair != gSynthesizerTypes.end();
    ++iPair, ++Index)
  {
    mpNormalListbox->Append(iPair->first.c_str());
    if (strcmp(iPair->first.c_str(), gpConfig->StrValue(C_SynthType)) == 0)
    {
      Selection = Index;
    }
  }
  mpNormalListbox->SetSelection(Selection);

  mpAccentedListbox = new wxListBox(this, wxID_ANY);

  mpAccentedListbox->Append("Never");
  mpAccentedListbox->Append("Song Start");
  mpAccentedListbox->Append("Start Play");

  mpAccentedListbox->SetSelection(gpConfig->GetValue(C_SendSynthReset));

  wxButton* pOkButton = new wxButton(this, wxID_OK, "&OK");
  wxButton* pCancelButton = new wxButton(this, wxID_CANCEL, "Cancel");
  wxButton* pHelpButton = new wxButton(this, wxID_HELP, "Help");
  pOkButton->SetDefault();

  wxBoxSizer* pTopSizer = new wxBoxSizer(wxVERTICAL);
  wxBoxSizer* pListControlSizer = new wxBoxSizer(wxHORIZONTAL);
  wxBoxSizer* pButtonSizer = new wxBoxSizer(wxHORIZONTAL);

  pTopSizer->Add(
    new wxStaticText(this, wxID_ANY, "Velocity"),
    0,
    wxCENTER | wxALL,
    2);

  pTopSizer->Add(mpVelocityKnob, 0, wxCENTER | wxALL, 2);

  pTopSizer->Add(mpVelocityValue, 0, wxCENTER | wxALL, 2);

  pTopSizer->Add(mpAccentedCheckBox, 0, wxCENTER | wxALL, 2);

  wxBoxSizer* pLeftSizer = new wxBoxSizer(wxVERTICAL);
  wxBoxSizer* pRightSizer = new wxBoxSizer(wxVERTICAL);

  pLeftSizer->Add(
    new wxStaticText(this, wxID_ANY, "Normal Click"),
    0,
    wxALL,
    2);
  pLeftSizer->Add(mpNormalListbox, 0, wxGROW | wxALL, 2);

  pRightSizer->Add(
    new wxStaticText(this, wxID_ANY, "Accented Click:"),
    0,
    wxALL,
    2);
  pRightSizer->Add(mpAccentedListbox, 0, wxALL, 2);

  pListControlSizer->Add(pLeftSizer, 0, wxALL, 3);
  pListControlSizer->Add(pRightSizer, 0, wxALL, 3);

  pTopSizer->Add(pListControlSizer, 0, wxCENTER);

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
void JZMetronomeSettingsDialog::OnVolumeChange(JZKnobEvent& Event)
{
  int Value = Event.GetValue();
  ostringstream Oss;
  Oss << Value;
  mpVelocityValue->SetLabel(Oss.str().c_str());
}