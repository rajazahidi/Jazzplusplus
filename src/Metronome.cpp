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

#include "Metronome.h"
#include "Configuration.h"
#include "Globals.h"
#include "Events.h"

tMetronomeInfo::tMetronomeInfo()
  : KeyAcc(36),
    KeyNorm(37),
    Veloc(127),
    IsOn(0),
    IsAccented(1)
{
}

tKeyOn* tMetronomeInfo::Normal(int clock)
{
  return new tKeyOn(
    clock,
    gpConfig->GetValue(C_DrumChannel) - 1,
    KeyNorm,
    Veloc,
    15);
}

tKeyOn* tMetronomeInfo::Accented(int clock)
{
  return new tKeyOn(
    clock,
    gpConfig->GetValue(C_DrumChannel) - 1,
    KeyAcc,
    Veloc,
    15);
}
