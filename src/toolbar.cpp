/*
**  The JAZZ++ Midi Sequencer
**
** Copyright (C) 1994-2000 Andreas Voss and Per Sigmond, all rights reserved.
** Modifications Copyright (C) 2004 Patrick Earl
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
*/

#include "wx/wx.h"
#pragma hdrstop

#include "toolbar.h"


tToolBar::tToolBar(wxFrame *frame, tToolDef *td)
{
  toolbar=frame->CreateToolBar(wxTB_FLAT | wxTB_HORIZONTAL | wxTB_DOCKABLE);

  toolbar->SetMargins(5, 5);

  while(td->id != TOOLDEF_END_ID)
  {
    if(td->id == TOOLDEF_SEPARATOR_ID) {
      toolbar->AddSeparator();
    } else {
      wxBitmap bitmap =  wxBitmap((char **)td->resid);

      if(td->sticky)
	toolbar->AddCheckTool(td->id, "", bitmap, wxNullBitmap, td->tooltip);
      else
	toolbar->AddTool(td->id, bitmap, td->tooltip);
    }

    td++;
  }

  toolbar->Realize();
}

wxToolBar *tToolBar::GetDelegateToolBar() {
  return toolbar;
}

// Delegated Methods

wxSize tToolBar::GetSize() const {
  return toolbar->GetSize();
}

bool tToolBar::GetToolState(int toolId) const {
  return toolbar->GetToolState(toolId);
}

void tToolBar::ToggleTool(int toolId, const bool toggle) {
  toolbar->ToggleTool(toolId, toggle);
}
