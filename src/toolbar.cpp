/*
**  The JAZZ++ Midi Sequencer
**
** Copyright (C) 1994-2000 Andreas Voss and Per Sigmond, all rights reserved.
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




tToolBar::tToolBar(wxFrame *frame, tToolDef *td, int n)
  //  : wxToolBar(frame, -1, wxDefaultPosition, wxDefaultSize, wxTB_HORIZONTAL|wxTB_3DBUTTONS|wxTB_DOCKABLE)//, 0, 0, -1, -1, wxTB_3DBUTTONS, wxHORIZONTAL, n)
{
  win = frame;
  toolbar=win->CreateToolBar(wxNO_BORDER | wxTB_FLAT|wxTB_HORIZONTAL|wxTB_DOCKABLE);//|wxTB_3DBUTTONS
  //if  you want to go back to subclassing rather than delegating, just set toolbar=this
  
  int i;

  toolbar->SetMargins(5, 5);

  int width = 24;
  int currentX = 5;
  for (i = 0; i < n; i++)
  {
    wxBitmap bitmap =  wxBitmap((char **)td->resid);//, NULL);
    //AddTool(td->id, bitmap, NULL, td->sticky, (float)currentX, -1, NULL);
    //                              
    toolbar->AddTool(td->id, bitmap, wxNullBitmap, td->sticky, currentX, -1, (wxObject *) NULL, td->tooltip);
    //printf(">>%s\n",td->tooltip);
    if (td->sep)
      currentX += 12;
    currentX += width;
    td ++;
  }
  toolbar->Realize();
}

wxToolBar* tToolBar::GetToolBar(){
  return toolbar;
}




//the methods below are redundant when delegating

//this (win->OnMenuCommand(toolIndex))  isnt supposed to be necessary if we do the event macro setup correctly
//i get a mysterious linker error if this function isnt defined
  bool tToolBar::OnLeftClick(int toolIndex, bool toggled)
  {
//    // win->OnMenuCommand(toolIndex);
// //   printf("tToolBar::OnLeftClick FIXME supposed to call the windows event handler somehow");
    return TRUE;
  }

void tToolBar::OnMouseEnter(int toolIndex)
{
}

