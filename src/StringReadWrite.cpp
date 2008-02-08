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

#include <iostream>

using namespace std;

//*****************************************************************************
//*****************************************************************************
istream& ReadString(istream& Is, char* pString, int MaximumLength)
{
  // Save space for the trailing 0.
  --MaximumLength;

  int c;
  do
  {
    // Ignore through the first ".
    c = Is.get();
  } while (c != '"' && c != EOF);

  int i;
  for (i = 0; i < MaximumLength; ++i)
  {
    c = Is.get();
    if (c == '"' || c == EOF)
    {
      break;
    }
    pString[i] = c;
  }

  // Terminate the C-style string.
  pString[i] = 0;

  return Is;
}

//*****************************************************************************
//*****************************************************************************
ostream& WriteString(ostream& Os, const char* pString)
{
  Os << '"' << pString << '"';
  return Os;
}
