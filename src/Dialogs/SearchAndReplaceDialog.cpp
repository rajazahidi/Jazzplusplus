//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Copyright (C) 2009-2013 Peter J. Stieber, all rights reserved.
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

#include "SearchAndReplaceDialog.h"

#include "../Configuration.h"
#include "../Globals.h"
#include "../Help.h"

#include <wx/button.h>
#include <wx/listbox.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

using namespace std;

//*****************************************************************************
//*****************************************************************************
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(JZSearchAndReplaceDialog, wxDialog)

  EVT_BUTTON(wxID_HELP, JZSearchAndReplaceDialog::OnHelp)

END_EVENT_TABLE()

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZSearchAndReplaceDialog::JZSearchAndReplaceDialog(
  short From,
  short To,
  wxWindow* pParent)
  : wxDialog(
      pParent,
      wxID_ANY,
      wxString("Search and replace controller types")),
    mFrom(From),
    mTo(To),
    mpFromListBox(0),
    mpToListBox(0)
{
  mpFromListBox = new wxListBox(this, wxID_ANY, wxDefaultPosition, wxSize(200, 160));
  mpToListBox = new wxListBox(this, wxID_ANY, wxDefaultPosition, wxSize(200, 160));

  const vector<pair<string, int> >& ControlNames =
    gpConfig->GetControlNames();
  for (
    vector<pair<string, int> >::const_iterator iControlName =
      ControlNames.begin();
    iControlName != ControlNames.end();
    ++iControlName)
  {
    const string& ControlName = iControlName->first;
    if (!ControlName.empty())
    {
      mControlNumbers.push_back(iControlName->second);
      mpFromListBox->Append(ControlName);
      mpToListBox->Append(ControlName);
    }
  }

  wxButton* pOkButton = new wxButton(this, wxID_OK, "&OK");
  wxButton* pCancelButton = new wxButton(this, wxID_CANCEL, "Cancel");
  wxButton* pHelpButton = new wxButton(this, wxID_HELP, "Help");
  pOkButton->SetDefault();

  wxBoxSizer* pTopSizer = new wxBoxSizer(wxVERTICAL);

  pTopSizer->Add(
    new wxStaticText(this, wxID_ANY, "Search and replace controller types:"),
    0,
    wxALIGN_CENTER | wxALL,
    8);

  wxBoxSizer* pLeftSizer = new wxBoxSizer(wxVERTICAL);
  pLeftSizer->Add(
    new wxStaticText(this, wxID_ANY, "Search for:"),
    0,
    wxALL,
    3);
  pLeftSizer->Add(mpFromListBox, 1, wxGROW | wxALL, 3);

  wxBoxSizer* pRightSizer = new wxBoxSizer(wxVERTICAL);
  pRightSizer->Add(
    new wxStaticText(this, wxID_ANY, "Replace with:"),
    0,
    wxALL,
    3);
  pRightSizer->Add(mpToListBox, 1, wxGROW | wxALL, 3);

  wxBoxSizer* pListBoxSizer = new wxBoxSizer(wxHORIZONTAL);
  pListBoxSizer->Add(pLeftSizer, 1, wxGROW | wxALL, 6);
  pListBoxSizer->Add(pRightSizer, 1, wxGROW | wxALL, 6);

  pTopSizer->Add(pListBoxSizer, 1, wxGROW | wxLEFT | wxRIGHT, 6);

  wxBoxSizer* pButtonSizer = new wxBoxSizer(wxHORIZONTAL);
  pButtonSizer->Add(pOkButton, 0, wxALL, 5);
  pButtonSizer->Add(pCancelButton, 0, wxALL, 5);
  pButtonSizer->Add(pHelpButton, 0, wxALL, 5);

  pTopSizer->Add(pButtonSizer, 0, wxALIGN_CENTER | wxTOP | wxBOTTOM, 8);

  SetAutoLayout(true);
  SetSizer(pTopSizer);

  pTopSizer->SetSizeHints(this);
  pTopSizer->Fit(this);
  CentreOnParent();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool JZSearchAndReplaceDialog::TransferDataToWindow()
{
  int fromSel = 0;
  int toSel = 0;

  for (size_t i = 0; i < mControlNumbers.size(); ++i)
  {
    if (mControlNumbers[i] == mFrom) fromSel = static_cast<int>(i);
    if (mControlNumbers[i] == mTo) toSel = static_cast<int>(i);
  }

  if (mpFromListBox->GetCount() > 0)
  {
    mpFromListBox->SetSelection(fromSel);
  }
  if (mpToListBox->GetCount() > 0)
  {
    mpToListBox->SetSelection(toSel);
  }
  return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool JZSearchAndReplaceDialog::TransferDataFromWindow()
{
  int fromSel = mpFromListBox->GetSelection();
  if (fromSel >= 0 && fromSel < static_cast<int>(mControlNumbers.size()))
  {
    mFrom = static_cast<short>(mControlNumbers[fromSel]);
  }

  int toSel = mpToListBox->GetSelection();
  if (toSel >= 0 && toSel < static_cast<int>(mControlNumbers.size()))
  {
    mTo = static_cast<short>(mControlNumbers[toSel]);
  }
  return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZSearchAndReplaceDialog::OnHelp(wxCommandEvent& Event)
{
  JZHelp::Instance().ShowTopic("Search Replace");
}
