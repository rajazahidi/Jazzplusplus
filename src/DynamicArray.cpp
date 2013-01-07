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

#include "DynamicArray.h"

DEFINE_ARRAY(JZIntArray, int)

JZUniqIds::JZUniqIds()
  : mArray(0)
{
  mArray[0] = 1;         // 0 is an invalid id
}

int JZUniqIds::Get()
{
  int i = 0;
  while (mArray[i])
  {
    ++i;
  }
  mArray[i] = 1;
  return i;
}

void JZUniqIds::Get(int id)
{
  ++mArray[id];
}

int JZUniqIds::Put(int id)
{
  int i = --mArray[id];
  assert(i >= 0);
  return i;
}


DEFINE_ARRAY(JZVoidPtrArray, void *)
