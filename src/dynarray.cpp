/*
**  The JAZZ++ Midi Sequencer
**
** Copyright (C) 1994-2000 Andreas Voss and Per Sigmond, all rights reserved.
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
*/                                                                              

#include "dynarray.h"
#include <assert.h>

DEFINE_ARRAY(tIntArray, int)

tUniqIds::tUniqIds()
  : array(0)
{
  array[0] = 1; 	// 0 is an invalid id
}

int tUniqIds::Get()
{
  int i = 0;
  while (array[i])
    ++ i;
  array[i] = 1;
  return i;
}

void tUniqIds::Get(int id)
{
  ++ array[id];
}

int tUniqIds::Put(int id)
{
  int i = -- array[id];
  assert(i >= 0);
  return i;
}


DEFINE_ARRAY(tVoidPtrArray, void *)


