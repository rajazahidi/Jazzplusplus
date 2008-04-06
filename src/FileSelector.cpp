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

//*****************************************************************************
// Description:
//   Add a supplied extension to a file name if the file name doesn't already
// have an extension.
//   ext is assumed to have a leading dot. (.mid for example)
//*****************************************************************************
wxString add_default_ext(const wxString fn, const wxString ext)

{
  // is any extension already there?
  {
    const wxString x = wxFileNameFromPath(fn);
    if (x.find( '.', TRUE) != 0)
    {
      //if there is a dot, assume its an extension and return
      return fn;
    }
  }

  // otherwise append the supplied extension and return
  wxString RevisedFileName = fn;
  RevisedFileName += ext;
  return RevisedFileName;
}

//*****************************************************************************
// Description:
//   Presents the user with a file selector.  Some default values are used.
// This can be used to present a load or save selector.  If save, handles
// situations like showing a "really save over..." question box.
//
// Returns:
//   wxString:
//     This is the file name.
//*****************************************************************************
wxString file_selector(
  wxString deffile,
  const wxString title,
  bool save,
  bool changed,
  const wxString ext)
{
   wxString s;
   wxString file;
   wxString path;

   if (save)
   {
     file = wxFileNameFromPath(deffile);
   }

   path = wxPathOnly(deffile);

   int flags = save ? wxFD_SAVE : wxFD_OPEN;
   s = wxFileSelector(title, path, file, 0, ext, flags);

  // add extension if missing
  if (s && ext)
  {
    s = add_default_ext(s, ext);
  }

  // warn if overwriting existent file
  if (s && save)
  {
    if (wxFileExists(s))
    {
      wxString buf;
      //sprintf(buf, "overwrite %s?", (char*)s);
      buf << "overwrite "<<s<<"?";
      if (wxMessageBox(buf, "Save ?", wxYES_NO) == wxNO) {

        s = wxEmptyString;
      }
    }
  }

  if (s && !save && changed)
  {
    wxString buf;
    buf<<deffile;
    buf <<" has changed. Load anyway?";
    if (wxMessageBox(buf, "Load ?", wxYES_NO) == wxNO) {
      s = wxEmptyString;
    }
  }

  if (s!=wxEmptyString && !save)
  {
    if (!wxFileExists(s))
    {
      wxString buf;
      buf<< "Cannot find file ";
      buf<< s;
      wxMessageBox( buf, "Error", wxOK );
      s = wxEmptyString;
    }
  }

  if (s!=wxEmptyString)
  {
    //delete [] deffile;
    //deffile = s;
    //return deffile;
    //i dont understand the point of the above original construct
//     wxString rv=*(new wxString(s));
//     return rv;//copy the local string and return it
    return s; //should be safe to return locally allocated string
  }

  return wxEmptyString;
}
