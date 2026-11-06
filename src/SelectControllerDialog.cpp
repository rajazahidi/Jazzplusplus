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

#include "SelectControllerDialog.h"

#include "Configuration.h"
#include "Globals.h"

#include <wx/arrstr.h>
#include <wx/choicdlg.h>

using namespace std;

//*****************************************************************************
//*****************************************************************************
int SelectControllerDlg()
{
  wxArrayString Names;
  vector<int> Controllers;

  const vector<pair<string, int> >& ControlNames =
    gpConfig->GetControlNames();

  for (
    vector<pair<string, int> >::const_iterator iControlName =
      ControlNames.begin();
    iControlName != ControlNames.end();
    ++iControlName)
  {
    const string& Name = iControlName->first;
    if (!Name.empty())
    {
      Controllers.push_back(iControlName->second);
      Names.Add(Name);
    }
  }

  int i = ::wxGetSingleChoiceIndex("Controller", "Select a controller", Names);

  if (i >= 0 && i < static_cast<int>(Controllers.size()))
  {
    return Controllers[i];
  }

  return -1;
}
