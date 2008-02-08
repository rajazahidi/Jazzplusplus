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

#include <iostream>

using namespace std;

//*****************************************************************************
// Description:
//   Find a file, looking in
// 1. where the HOME environment var is pointing 
// 2. where the JAZZ environment var is pointing
// 3. where the jazz executable was started
//
// This function was converted to use wxString instead of char* because it is
// safe to return localy allocated wxStrings because of reference counting.
//*****************************************************************************
wxString FindFile(const char* pFileName)
{
  wxString buf;

  if (wxFileExists((char *)pFileName))
  {
    cout << "imediate hit " << pFileName << endl;
    return pFileName;
  }

  wxString home;
  if (getenv("HOME") != 0)
  {
    home = getenv("HOME");
    buf << home << "/" << pFileName;
    cout << "home " << buf <<endl;
    if (wxFileExists(buf))
    {
      return buf;
    }
  }
  if (getenv("HOME") != 0)
  {
    buf = "";
    home = getenv("JAZZ");
    buf << home << "/" << pFileName;
    cout << "jazz " << buf <<endl;
    if (wxFileExists(buf))
    {
      return buf;
    }
  }

  // look where the executable was started
  home = wxPathOnly((const char *)wxTheApp->argv[0]);
  buf = "";
  buf << home << "/" << pFileName;
  cout <<"startup " << buf <<endl;
  if (wxFileExists(buf))
  {
    return buf;
  }

  // look in the compiled-in path
//  buf = "";
//  buf << JAZZ_DATADIR << "/" << pFileName;
//  cout << "compiled in path " << buf << "  " << wxFileExists(buf) << endl;
//  if (wxFileExists(buf))
//  {
//    return buf;
//  }
  
  return wxEmptyString;
}
