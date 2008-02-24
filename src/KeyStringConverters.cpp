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

#include <string.h>
#include <stdio.h>
#include <ctype.h>

void Key2Str(int Key, char* pString)
{
  static char* pNames[] =
  {
    "C",
    "C#",
    "D",
    "D#",
    "E",
    "F",
    "F#",
    "G",
    "G#",
    "A",
    "A#",
    "B"
  };

  strcpy(pString, pNames[Key % 12]);
  sprintf(pString + strlen(pString), "%d", Key / 12);
}

int Str2Key(const char *pString)
{
  static char sKey[] = "cCdDeEfFgGaAbB";
  static int  nKey[] =
  {
    0,
    0,
    2,
    2,
    4,
    4,
    5,
    5,
    7,
    7,
    9,
    9,
    11,
    11
  };

  int Key = 0;

  while (*pString)
  {
    if (*pString == '#')
    {
      Key += 1;
      ++pString;
    }
    else if (isdigit(*pString))
    {
      int n = 0;
      while (isdigit(*pString))
      {
        n = 10 * n + *pString++ - '0';
      }
      Key += 12 * n;
    }
    else
    {
      int i;
      for (i = 0; sKey[i]; ++i)
      {
        if (pString[0] == sKey[i])
        {
          Key += nKey[i];
          ++pString;
          break;
        }
      }
      if (!sKey[i])
      {
        // error
        ++pString;
      }
    }
  }
  return Key;
}
