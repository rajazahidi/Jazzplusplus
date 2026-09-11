//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Copyright (C) 1994-2000 Andreas Voss and Per Sigmond, all rights reserved.
// Modifications Copyright (C) 2004 Patrick Earl
// Modifications Copyright (C) 2008-2013 Peter J. Stieber
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

#include "GuitarSettingsDialog.h"
#include "GuitarWindow.h"

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(JZGuitarSettingsDialog, wxDialog)
  EVT_BUTTON(wxID_OK, JZGuitarSettingsDialog::OnOk)
END_EVENT_TABLE()

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZGuitarSettingsDialog::JZGuitarSettingsDialog(wxWindow* pParent)
  : wxDialog(pParent, wxID_ANY, wxString("Guitar Fretboard & Tab Settings")),
    mpChordModeCheckBox(0),
    mpBassGuitarCheckBox(0),
    mpShowOctavesCheckBox(0),
    mpFretCountChoice(0)
{
  mpChordModeCheckBox = new wxCheckBox(this, wxID_ANY, "Chord Mode (simultaneous buffer insert)");
  mpChordModeCheckBox->SetValue(JZGuitarWindow::GetChordMode());

  mpBassGuitarCheckBox = new wxCheckBox(this, wxID_ANY, "4-String Bass Guitar (E1 A1 D2 G2)");
  mpBassGuitarCheckBox->SetValue(JZGuitarWindow::GetBassGuitar());

  mpShowOctavesCheckBox = new wxCheckBox(this, wxID_ANY, "Show All Octaves Across Fretboard");
  mpShowOctavesCheckBox->SetValue(JZGuitarWindow::GetShowOctaves());

  wxArrayString fretChoices;
  fretChoices.Add("17 Frets (Vintage Compact)");
  fretChoices.Add("21 Frets (Standard Stratocaster)");
  fretChoices.Add("22 Frets (Modern Standard)");
  fretChoices.Add("24 Frets (Full Range 2-Octave)");

  mpFretCountChoice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, fretChoices);
  int curFrets = JZGuitarWindow::GetFretCount();
  if (curFrets <= 17) mpFretCountChoice->SetSelection(0);
  else if (curFrets == 21) mpFretCountChoice->SetSelection(1);
  else if (curFrets == 22) mpFretCountChoice->SetSelection(2);
  else mpFretCountChoice->SetSelection(3);

  wxBoxSizer* pTopSizer = new wxBoxSizer(wxVERTICAL);
  wxBoxSizer* pCheckBoxSizer = new wxBoxSizer(wxVERTICAL);

  pCheckBoxSizer->Add(mpChordModeCheckBox, 0, wxALL, 6);
  pCheckBoxSizer->Add(mpBassGuitarCheckBox, 0, wxALL, 6);
  pCheckBoxSizer->Add(mpShowOctavesCheckBox, 0, wxALL, 6);

  wxBoxSizer* pFretRow = new wxBoxSizer(wxHORIZONTAL);
  pFretRow->Add(new wxStaticText(this, wxID_ANY, "Fretboard Length:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
  pFretRow->Add(mpFretCountChoice, 1, wxEXPAND);
  pCheckBoxSizer->Add(pFretRow, 0, wxEXPAND | wxALL, 6);

  pTopSizer->Add(pCheckBoxSizer, 1, wxGROW | wxALL, 12);

  wxBoxSizer* pButtonsSizer = new wxBoxSizer(wxHORIZONTAL);
  wxButton* pOkButton = new wxButton(this, wxID_OK, "&OK");
  pOkButton->SetDefault();
  pButtonsSizer->Add(pOkButton, 0, wxALL, 10);
  pButtonsSizer->Add(new wxButton(this, wxID_CANCEL, "Cancel"), 0, wxALL, 10);

  pTopSizer->Add(pButtonsSizer, 0, wxALIGN_CENTER);

  SetAutoLayout(true);
  SetSizer(pTopSizer);

  pTopSizer->Fit(this);
  CentreOnParent();
}

void JZGuitarSettingsDialog::OnOk(wxCommandEvent&)
{
  JZGuitarWindow::SetChordMode(mpChordModeCheckBox->GetValue());
  JZGuitarWindow::SetBassGuitar(mpBassGuitarCheckBox->GetValue());
  JZGuitarWindow::SetShowOctaves(mpShowOctavesCheckBox->GetValue());

  int sel = mpFretCountChoice->GetSelection();
  int frets = 21;
  switch (sel)
  {
    case 0: frets = 17; break;
    case 1: frets = 21; break;
    case 2: frets = 22; break;
    case 3: frets = 24; break;
    default: frets = 21; break;
  }
  JZGuitarWindow::SetFretCount(frets);

  EndModal(wxID_OK);
}
