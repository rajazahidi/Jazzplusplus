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

#include <wx/wx.h>
#include <windows.h>
#include <wincon.h>

#include <stdio.h>
#include <fcntl.h>
#include <io.h>

#include <iostream>
#include <fstream>

using namespace std;

// Maximum mumber of lines the output console should have.
static const WORD MAX_CONSOLE_LINES = 500;

//*****************************************************************************
//*****************************************************************************
void RedirectIoToConsole()
{
  CONSOLE_SCREEN_BUFFER_INFO coninfo;
  FILE *fp = NULL;

  // allocate a console for this app
  if (!AllocConsole())
  {
    return;
  }

  // set the screen buffer to be big enough to let us scroll text
  GetConsoleScreenBufferInfo(
    GetStdHandle(STD_OUTPUT_HANDLE),
    &coninfo);
  coninfo.dwSize.Y = MAX_CONSOLE_LINES;
  SetConsoleScreenBufferSize(
    GetStdHandle(STD_OUTPUT_HANDLE),
    coninfo.dwSize);

  // redirect unbuffered STDOUT, STDIN, and STDERR to the console
  freopen_s(&fp, "CONOUT$", "w", stdout);
  freopen_s(&fp, "CONIN$", "r", stdin);
  freopen_s(&fp, "CONOUT$", "w", stderr);

  // make cout, wcout, cin, wcin, wcerr, cerr, wclog and clog
  // point to console as well
  ios::sync_with_stdio();
}
