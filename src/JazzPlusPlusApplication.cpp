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

#include "JazzPlusPlusApplication.h"
#include "TrackFrame.h"
#include "Project.h"
#include "Globals.h"

#ifdef _MSC_VER

#ifdef _DEBUG
// This code provides a console window for a GUI windows application.  This
// allows the use of cout for debug or informational messages.
#include "mswin/WindowsConsole.h"
#endif

// This include allows Microsoft leak detection calls like _CrtSetBreakAlloc.
#include <crtdbg.h>

#endif // _MSC_VER

#ifdef __LINUX__

// The following include is required to call feenableexcept on Linux.
#include <fenv.h>

#endif

//*****************************************************************************
// Description:
//   This is the JazzPlusPlus application class definition.
//*****************************************************************************
//-----------------------------------------------------------------------------
// Description:
//   Create a new application object using the wxWidgets macro.  This macro
// will allow wxWidgets to create the application object during program
// execution (it's better than using a static object for many reasons) and
// also declares the accessor function wxGetApp() which will return the
// reference of the right type (i.e. JZJazzPlusPlusApplication and not wxApp).
//-----------------------------------------------------------------------------
IMPLEMENT_APP(JZJazzPlusPlusApplication)

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(JZJazzPlusPlusApplication, wxApp)
END_EVENT_TABLE()

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZJazzPlusPlusApplication::JZJazzPlusPlusApplication()
  : wxApp(),
    mpProject(0),
    mHelp(wxHF_DEFAULT_STYLE | wxHF_OPEN_FILES)
{
#ifdef _MSC_VER
  // When using the Microsoft C++ compiler in debug mode, each heap allocation
  // (i.e. calling new) is counted. The following line will cause the code to
  // generate a user defined break point when the passed allocation index is
  // hit. To find leaks, run in debug and look for the following type of line
  // in the Debug output window.
  //
  // {1494} normal block at 0x00B98FC8, 32 bytes long.
  //  Data: <            Jazz> 01 00 00 00 04 00 00 00 13 00 00 00 4A 61 7A 7A
  //
  // Then use the following line to cause a break point when this allocation
  // occurs:
  //
  // _CrtSetBreakAlloc(1494);
#endif // _MSC_VER

#ifdef __LINUX__
  // This code enables floating point exceptions for
  // 1. Division by zero.
  // 2. Invalid arguments (for example sqrt of a negative number).
  // 3. Overflow.
  // on a Linux box.
  feenableexcept(FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW);
#endif // __LINUX__
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZJazzPlusPlusApplication::~JZJazzPlusPlusApplication()
{
}

//-----------------------------------------------------------------------------
// Description:
//   This function is called on application startup and is a good place
// for application initialization.  Initializing here and not in the
// constructor allows an error return.  If OnInit() returns false, the
// application terminates.
//-----------------------------------------------------------------------------
bool JZJazzPlusPlusApplication::OnInit()
{
#if defined(_MSC_VER) && defined(_DEBUG)
  RedirectIoToConsole();
#endif // _MSC_VER

  SetVendorName("Jazz");
  SetAppName("Jazz");

  // Create the one and only top-level Jazz++ project.
  mpProject = new JZProject;
  gpProject = mpProject;

  wxConfigBase* pConfig = wxConfigBase::Get();

  // Let the help system store the Jazz++ help configuration info.
  mHelp.UseConfig(pConfig);

  // Call base class function.  This is needed for command line parsing.
  wxApp::OnInit();

  // Create the main application window.
  mpTrackFrame = new JZTrackFrame(
    0,
    "Jazz++",
    gpSong,
    wxPoint(10, 10),
    wxSize(600, 400));

  // Show it and tell the application that it's our main window
  mpTrackFrame->Show(true);
  SetTopWindow(mpTrackFrame);

  return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int JZJazzPlusPlusApplication::OnExit()
{
  delete mpProject;

  // GetFrame returns NULL if there is no help frame active.
  if (mHelp.GetFrame())
  {
    // Close the help frame; this will cause the config data to get written.
    mHelp.GetFrame()->Close(true);
  }

  // Prevent reported leaks from the configuration class.
  delete wxConfigBase::Set(0);

  return 0;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZTrackFrame* JZJazzPlusPlusApplication::GetMainFrame() const
{
  return mpTrackFrame;
}

//-----------------------------------------------------------------------------
// Description:
//   Display the table of contents for Jazz++ help.
//-----------------------------------------------------------------------------
void JZJazzPlusPlusApplication::DisplayHelpContents() const
{
  mHelp.DisplayContents();
}

//-----------------------------------------------------------------------------
// Description:
//   Provide context sensitive help for Jazz++.
//-----------------------------------------------------------------------------
void JZJazzPlusPlusApplication::GetHelp(const wxString& TopicString) const
{
  mHelp.DisplaySection(TopicString);
}
