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

#include "KeyDialog.h"
#include "KeyStringConverters.h"
#include "DeprecatedStringUtils.h"
#include "DeprecatedWx/proplist.h"

tKeyDlg::tKeyDlg(const char* pTitle, int Key)
{
  char buf[50];
  Key2Str(Key, buf);
  mpString = copystring(buf);
  mpTitle = pTitle;
}


tKeyDlg::~tKeyDlg()
{
  delete mpString;
}

#ifdef OBSOLETE
wxFormItem *tKeyDlg::mkFormItem(int w)
{
  return wxMakeFormString(mpTitle, &mpString, wxFORM_DEFAULT, 0, 0, 0, w);
}
#endif

wxProperty* tKeyDlg::mkProperty()
{
  return new wxProperty(mpTitle, wxPropertyValue((char**)&mpString), "string");
}

int tKeyDlg::GetKey()
{
  return Str2Key(mpString);
}

