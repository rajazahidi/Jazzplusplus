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

#include "KeyDialog.h"

#include "KeyStringConverters.h"
#include "DeprecatedStringUtils.h"
#include "DeprecatedWx/proplist.h"

using namespace std;

JZKeyDialog::JZKeyDialog(const char* pTitle, int Key)
{
  string KeyString;
  KeyToString(Key, KeyString);
  mpString = copystring(KeyString.c_str());
  mpTitle = pTitle;
}


JZKeyDialog::~JZKeyDialog()
{
  delete mpString;
}

#ifdef OBSOLETE
wxFormItem *JZKeyDialog::mkFormItem(int w)
{
  return wxMakeFormString(mpTitle, &mpString, wxFORM_DEFAULT, 0, 0, 0, w);
}
#endif

wxProperty* JZKeyDialog::mkProperty()
{
  return new wxProperty(mpTitle, wxPropertyValue((char**)&mpString), "string");
}

int JZKeyDialog::GetKey()
{
  return StringToKey(mpString);
}
