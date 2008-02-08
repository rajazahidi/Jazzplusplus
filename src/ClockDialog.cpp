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

#include "ClockDialog.h"

#include "Song.h"
#include "DeprecatedStringUtils.h"
#include "DeprecatedWx/proplist.h"

//*****************************************************************************
// tClockDlg
//*****************************************************************************
tClockDlg::tClockDlg(JZSong *s, char *t, long c)
{
  char buf[500];
  s->Clock2String(c, buf);
  String = copystring(buf);
  Title = t;
  Song = s;
}

tClockDlg::~tClockDlg()
{
  delete String;
}

wxProperty* tClockDlg::mkProperty()
{
  return new wxProperty(Title, wxPropertyValue((char**)&String), "string"); 
}

long tClockDlg::GetClock()
{
  return Song->String2Clock(String);
}
