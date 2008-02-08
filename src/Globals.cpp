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
#include "Globals.h"
#include "Song.h"
#include "Synth.h"
#include "Help.h"
#include "NamedValue.h"
#include "Project.h"

char* pgStartUpSong = 0;

JZSong* TheSong = 0;

JZSynth* Synth = 0;

tHelp* HelpInstance = 0;

tNamedValue limitSteps[] =
{
  tNamedValue( "1/8",   8 ),
  tNamedValue( "1/12", 12 ),
  tNamedValue( "1/16", 16 ),
  tNamedValue( "1/24", 24 ),
  tNamedValue( "1/32", 32 ),
  tNamedValue( "1/48", 48 ),
  tNamedValue( "1/96", 96 ),
  tNamedValue( "1/192", 192 ),
  tNamedValue(   0,      1  )
};

jppProject* gProject = 0;