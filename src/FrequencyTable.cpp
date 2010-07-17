//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Copyright (C) 1994-2000 Andreas Voss and Per Sigmond, all rights reserved.
// Modifications Copyright (C) 2004 Patrick Earl
// Modifications Copyright (C) 2008-2010 Peter J. Stieber
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

#include "FrequencyTable.h"

#include <cmath>
#include <algorithm>

using namespace std;

FreqTab::FreqTab()
{
  double fac = pow(2.0, 1.0 / 12.0);
  double frq = 440 * pow(fac, 3.0) / 32.0;
  tab.reserve(128);
  for (int i = 0; i < 128; i++)
  {
    tab.push_back(frq);
    frq *= fac;
  }
}

inline double dabs(double a, double b)
{
  return a > b ? a - b : b - a;
}

int FreqTab::key(double f)
{
  // do a binary search
  vector<double>::iterator pos = lower_bound(tab.begin(), tab.end(), f);
  // todo: if f is only a very little bigger than *pos, f is nearer to pos-1
  return pos - tab.begin();
}
