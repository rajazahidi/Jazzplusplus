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

#include "Events.h"
#include "Synth.h"
#include "Globals.h"
#include "JazzPlusPlusApplication.h"
#include "ErrorMessage.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>


int tReadBase::Open(const char *fname)
{
  if (fname == NULL)
    fd = stdin;
  else
  {
    fd = fopen(fname, "rb");
    if (fd == NULL) {
      Error("Error opening file %s", fname);
      return 0;
    }
  }
  return 1;
}


void tReadBase::Close()
{
  if (fd != stdin)
    fclose(fd);
}




int tWriteBase::Open(const char *fname, int nTracks, int TicksPerQuarter)
{
  if (fname == NULL)
    fd = stdout;
  else
  {
#ifndef __WXMSW__
    FILE *testfd = fopen(fname, "r");
    if (testfd) {
	fclose( testfd );
    	char *syscmd;
    	syscmd = new char[ strlen( "cp" ) + 2*strlen( fname ) + strlen( ".backup" ) + 3 ];
    	sprintf( syscmd, "cp %s %s.backup", fname, fname );
    	if (system( syscmd ) != 0) {
		fprintf(stderr, "Could not make backup file %s.backup\n", fname );
    	}
    	delete syscmd;
    }
#endif
    fd = fopen(fname, "wb");
    if (fd == NULL)
    {
      Error("Error opening file %s", fname);
      return 0;
    }
  }
  return nTracks;
}


void tWriteBase::Close()
{
  if (fd != stdout)
    fclose(fd);
}


// -------------------------------------------------------
// tGetMidiBytes
// -------------------------------------------------------

int tGetMidiBytes::Write(JZEvent *e, unsigned char *s, int len)
{
  int Stat = e->Stat;

  switch (Stat)
  {
    case StatKeyOff:
// SN-- want key off veloc      Stat = StatKeyOn;  // better RunningStatus
    case StatKeyOn:
    case StatKeyPressure:
    case StatControl:
    case StatProgram:
    case StatChnPressure:
    case StatPitch:
      nBytes = 0;

      Buffer[nBytes++] = Stat | ((tChannelEvent *)e)->Channel;
      while(len--)
        Buffer[nBytes++] = *s++;
      return 0;

    default:
      return 1;
  }
}


int tSysEx::GetPitch()
{
   edb();
   int id = Synth->GetSysexId( this );

   if ((id >= SX_GM_ON) && (id < SX_GS_ON))
   {
      return SX_GROUP_GM;
   }
   else if ((id >= SX_GS_ON) && (id < SX_XG_ON))
   {
      return SX_GROUP_GS;
   }
   else if ((id >= SX_XG_ON) && (id < NumSysexIds))
   {
      return SX_GROUP_XG;
   }
   else
   {
      return SX_GROUP_UNKNOWN;
   }
}
