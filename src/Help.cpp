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

#include "wx/html/helpctrl.h"

#include "Help.h"
#include "Globals.h"

#include <iostream>

using namespace std;

//*****************************************************************************
// Description:
//   This is the help class definition.
//*****************************************************************************
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZHelp::JZHelp(const char* pHelpFleName)
  : mpHelp(0),
    mHelpFile(pHelpFleName)
{
  mpHelp = new wxHtmlHelpController();
  mpHelp->Initialize(mHelpFile);
  cout << "JZHelp::JZHelp " << mHelpFile << endl;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZHelp::~JZHelp()
{
  delete mpHelp;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZHelp::ShowTopic(const char* pTopic)
{
  mpHelp->LoadFile(mHelpFile.c_str());
  mpHelp->KeywordSearch(pTopic);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZHelp::DisplayContents()
{
  mpHelp->LoadFile(mHelpFile.c_str());
  mpHelp->DisplayContents();
}
