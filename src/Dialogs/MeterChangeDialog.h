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

#pragma once

#include <wx/dialog.h>

class wxComboBox;
class wxSpinCtrl;

//*****************************************************************************
//*****************************************************************************
class JZMeterChangeDialog : public wxDialog
{
  public:

    JZMeterChangeDialog(wxWindow* pParent, int DefaultBar = 1, int DefaultNum = 4, int DefaultDenom = 4);

    int GetNumerator() const { return mNumerator; }
    int GetDenominator() const { return mDenominator; }
    int GetBar() const { return mBar; }

    virtual bool TransferDataFromWindow();

  private:

    void OnHelp(wxCommandEvent& Event);

  private:

    int mBar;
    int mNumerator;
    int mDenominator;

    wxSpinCtrl* mpBarSpinCtrl;
    wxComboBox* mpNumeratorComboBox;
    wxComboBox* mpDenominatorComboBox;

  DECLARE_EVENT_TABLE();
};
