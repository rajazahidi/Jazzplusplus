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

#ifndef JZ_JAZZPLUSPLUSAPPLICATION_H
#define JZ_JAZZPLUSPLUSAPPLICATION_H

#include <wx/html/helpctrl.h>

class JZProject;
class JZTrackFrame;

//*****************************************************************************
// Description:
//   This is the JazzPlusPlus application class declaration.
//*****************************************************************************
class JZJazzPlusPlusApplication : public wxApp
{
  public:

    JZJazzPlusPlusApplication();

    ~JZJazzPlusPlusApplication();

    // Override the base class virtual functions.

    // Description:
    //   This function is called on application startup and is a good place
    // for application initialization.  Initializing here and not in the
    // constructor allows an error return.  If OnInit() returns false, the
    // application terminates.
    virtual bool OnInit();

    virtual int OnExit();

    // Description:
    //   This virtual function returns a pointer to the application's main
    // frame.
    //
    // Returns:
    //   JZTrackFrame*:
    //     A pointer to the application's main frame.
    JZTrackFrame* GetMainFrame() const;

    void DisplayHelpContents() const;

    void GetHelp(const wxString& TopicString) const;

    // Description:
    //   This function returns the application's major version number.
    //
    // Returns:
    //   int:
    //     The application's major version number.
    int GetMajorVersion() const;

    // Description:
    //   This function returns the application's minor version number.
    //
    // Returns:
    //   int:
    //     The application's minor version number.
    int GetMinorVersion() const;

    // Description:
    //   This function returns the application's build number.
    //
    // Returns:
    //   int:
    //     The application's build number.
    int GetBuildNumber() const;

  private:

    void InsureConfigurationFileExistence() const;

    void FindAndRegisterHelpFilePath(wxString& HelpFilePath);

  private:

    static wxString mHelpFileName;

    JZProject* mpProject;

    JZTrackFrame* mpTrackFrame;

    mutable wxHtmlHelpController mHelp;

  DECLARE_EVENT_TABLE()
};

DECLARE_APP(JZJazzPlusPlusApplication)

//*****************************************************************************
// Description:
//   These are the Jazz++ application class inline member functions.
//*****************************************************************************
//-----------------------------------------------------------------------------
// Description:
//   This function returns the application's major version number.
// Version 1 was the MFC version developed on the Phase II ONR SBIR.
//
// Ver Date       Description
//  4  ?          Unknown for all prior versions.  Might be filled in by
//                looking at old commit messages.
//  5  1/21/2008  Start of the Jazz++ development revival.
//
// Returns:
//   int:
//     The application's major version number.
//-----------------------------------------------------------------------------
inline
int JZJazzPlusPlusApplication::GetMajorVersion() const
{
  return 5;
}

//-----------------------------------------------------------------------------
// Description:
//   This function returns the application's minor version number.
//
// Ver Date       Description
//  ?  ?          Unknown for all prior versions.  Might be filled in by
//                looking at old commit messages.
//  3  1/21/2008  Major refactoring here to get the code compiling with
//                wxWidgets 2.8.7 and recent compilers including Visual Studio
//                .NET 2005 and GCC 4.
//  4  9/1/2008   Updated to wxWidgets version 2.8.8.
//
// Returns:
//   int:
//     The application's minor version number.
//-----------------------------------------------------------------------------
inline
int JZJazzPlusPlusApplication::GetMinorVersion() const
{
  return 4;
}

//-----------------------------------------------------------------------------
// Description:
//   This function returns the application's build number.
//
// Ver Date       Description
//  ?  ?          Unknown for all prior versions.  Might be filled in by
//                looking at old commit messages.
//  11 1/21/2008  See minor version 3.
//  12 9/1/2008   See minor version 4.
//-----------------------------------------------------------------------------
inline
int JZJazzPlusPlusApplication::GetBuildNumber() const
{
  return 12;
}

#endif // !defined(JZ_JAZZPLUSPLUSAPPLICATION_H)
