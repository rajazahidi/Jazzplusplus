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

#ifndef toolbar_h
#define toolbar_h

#include "wx/wx.h"

/* This struct is used to initialize the tToolBar class, which is a wxToolBar
   wrapper.  An array of tToolDef items must be created, and the ID of the
   last entry in the array must be set to TOOLDEF_END. */
struct tToolDef
{
  int  id;
  bool sticky;
  const void *resid;
  const char *tooltip;
};

#define TOOLDEF_END_ID -1        // The id of the last tToolDef entry.
#define TOOLDEF_SEPARATOR_ID -2  // The id representing a separator.

// Use these entries in the tToolDef list.
#define TOOLDEF_END { TOOLDEF_END_ID }
#define TOOLDEF_SEPARATOR { TOOLDEF_SEPARATOR_ID }


class tToolBar
{
  public:
    // Pass in the frame on which the toolbar is to be created.  Also pass in
    // the list of toolbar definitions terminated by TOOLDEF_END.
    tToolBar(wxFrame *frame, tToolDef *tdef);

    // To retrieve the wxToolBar we're delegating to
    wxToolBar *GetDelegateToolBar();

    // Delegated functions from wxToolBar
    void ToggleTool(int toolId, const bool toggle);
    wxSize GetSize() const;
    bool GetToolState(int toolId) const;

  private:
    wxToolBar* toolbar;
};

#endif
