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

#include "NamedValueChoice.h"

using namespace std;

//*****************************************************************************
//*****************************************************************************
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
tNamedValueChoice::tNamedValueChoice(
  wxWindow* pParent,
  const vector<pair<string, int> >& Pairs)
  : wxChoice(pParent, wxID_ANY),
    mPairs(Pairs)
{
  for (
    vector<pair<string, int> >::const_iterator iPair = mPairs.begin();
    iPair != mPairs.end();
    ++iPair)
  {
    Append(iPair->first);
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int tNamedValueChoice::GetValue()
{
  int i = GetSelection();
  if (i >= 0)
  {
    return mPairs[i].second;
  }
  return 16;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void tNamedValueChoice::SetValue(int Measure)
{
  int i = 0;
  for (
    vector<pair<string, int> >::const_iterator iPair = mPairs.begin();
    iPair != mPairs.end();
    ++iPair)
  {
    if (iPair->second == Measure)
    {
      SetSelection(i);
    }
    ++i;
  }
}
