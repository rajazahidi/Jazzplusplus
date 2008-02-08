//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Copyright (C) 1994-2000 Andreas Voss and Per Sigmond, all rights reserved.
// Modifications Copyright (C) 2004 Patrick Earl
// Modifications Copyright (C) 2008 Peter J. Stieber
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

#include "Configuration.h"

int SelectControllerDlg()
{
  int i, n = 0;
  //  char *names[130];
  // PAT - The following line used to be this:
  //  wxArrayString names=new wxArrayString();
  wxArrayString names;
  int Controllers[130];
  for (i = 0; Config.CtrlName(i).Name; i++)
  {
    if (Config.CtrlName(i).Name[0])
    {
      Controllers[n] = Config.CtrlName(i).Value;
//      names.Add(*(new wxString(Config.CtrlName(i).Name))); //JAVE leaking?
      names.Add(Config.CtrlName(i).Name); //JAVE leaking?
    }
  }

  i = ::wxGetSingleChoiceIndex("Controller", "Select a controller", names);

  if (i >= 0)
  {
    return Controllers[i];
  }

  return -1;
}
