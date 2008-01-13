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

//*****************************************************************************
// Description:
//   This is the JazzPlusPlus application class declaration.
//*****************************************************************************
class JZJazzPlusPlusApplication : public wxApp
{
  public:

    JZJazzPlusPlusApplication();

    // Override the base class virtual functions.

    // Description:
    //   This function is called on application startup and is a good place
    // for application initialization.  Initializing here and not in the
    // constructor allows an error return.  If OnInit() returns false, the
    // application terminates.
    virtual bool OnInit();

    virtual int OnExit();

    void DisplayHelpContents() const;

    void GetHelp(const wxString& TopicString) const;

    // Description:
    //   This function returns the application's major version number.
    //
    // Returns:
    //   int:
    //     The application's major version number.
    virtual int GetMajorVersion() const;

    // Description:
    //   This function returns the application's minor version number.
    //
    // Returns:
    //   int:
    //     The application's minor version number.
    virtual int GetMinorVersion() const;

    // Description:
    //   This function returns the application's build number.
    //
    // Returns:
    //   int:
    //     The application's build number.
    virtual int GetBuildNumber() const;

  private:

    mutable wxHtmlHelpController mHelp;

  DECLARE_EVENT_TABLE()
};

//*****************************************************************************
// Description:
//   This is a declaration of the one-and-only Jazz++ application pointer.
//*****************************************************************************
extern JZJazzPlusPlusApplication* gpGlobalJazzApplication;

//*****************************************************************************
// Description:
//   This is a global function that returns the Jazz++ application.
//
// Returns:
//   const JZJazzPlusPlusApplication&:
//     A constant reference to the one-and-only Jazz++ application.
//*****************************************************************************
const JZJazzPlusPlusApplication& GetJazzApplication();

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
//
// Returns:
//   int:
//     The application's major version number.
//-----------------------------------------------------------------------------
inline
int JZJazzPlusPlusApplication::GetMajorVersion() const
{
  return 4;
}

//-----------------------------------------------------------------------------
// Description:
//   This function returns the application's minor version number.
//
// Ver Date       Description
//  ?  ?          Unknown for all prior versions.  Might be filled in by
//                looking at old commit messages.
//  2  1/11/2008  Major refactoring here to get the code compiling with
//                wxWidgets 2.8.7 and recent compilers including Visual Studio
//                .NET 2005 and GCC 4.
// Returns:
//   int:
//     The application's minor version number.
//-----------------------------------------------------------------------------
inline
int JZJazzPlusPlusApplication::GetMinorVersion() const
{
  return 2;
}

//-----------------------------------------------------------------------------
// Description:
//   This function returns the application's build number.
//
// Ver Date       Description
//  ?  ?          Unknown for all prior versions.  Might be filled in by
//                looking at old commit messages.
//  11 1/11/2008  See minor version 2.
//-----------------------------------------------------------------------------
inline
int JZJazzPlusPlusApplication::GetBuildNumber() const
{
  return 11;
}

#endif // !defined(JZ_JAZZPLUSPLUSAPPLICATION_H)
