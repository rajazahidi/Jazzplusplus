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

#pragma once

#include <wx/string.h>

class wxHtmlHelpController;
class wxWindow;

//*****************************************************************************
// Description:
//   This is the help class definition.  This class implements a wrapper
// around the wxWidgets help classes.
//*****************************************************************************
class JZHelp
{
  public:

    static JZHelp& Instance();

    void ConfigureHelp();

    void ShowTopic(const wxString& TopicString, wxWindow* pParent = nullptr);

    void DisplayHelpContents(wxWindow* pParent = nullptr);

    void CloseHelp();

  private:

    JZHelp();

    ~JZHelp();

    bool FindAndRegisterHelpFilePath(wxString& HelpFilePath) const;

    bool EnsureHelpBookLoaded(wxHtmlHelpController* pHelpCtrl);

  private:

    static wxString mHelpFileName;

    wxHtmlHelpController* mpHelp;
};

//*****************************************************************************
//*****************************************************************************
inline
JZHelp& JZHelp::Instance()
{
  static JZHelp JazzHelp;
  return JazzHelp;
}
