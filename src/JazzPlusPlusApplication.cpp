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

#include <wx/stdpaths.h>
#include <wx/fileconf.h>

#include "JazzPlusPlusApplication.h"
#include "TrackFrame.h"
#include "Project.h"
#include "ProjectManager.h"
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

#include <fstream>
#include <vector>

using namespace std;

//*****************************************************************************
// Description:
//   This is the JazzPlusPlus application class definition.
//*****************************************************************************
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
wxString JZJazzPlusPlusApplication::mHelpFileName = "jazz.hhp";

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
    mpTrackFrame(0),
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

  ::wxInitAllImageHandlers();

  SetVendorName("Jazz");
  SetAppName("Jazz");

  InsureConfigurationFileExistence();

  // Create the one and only top-level Jazz++ project.
  mpProject = new JZProject;
  gpProject = mpProject;

  wxConfigBase* pConfig = wxConfigBase::Get();

  // Let the help system store the Jazz++ help configuration info.
  mHelp.UseConfig(pConfig);

  // Call base class function.  This is needed for command line parsing.
  wxApp::OnInit();

  // Create the main application window.
  mpTrackFrame = JZProjectManager::Instance()->CreateTrackView();

  gpTrackFrame = mpTrackFrame;

  // Show it and tell the application that it's our main window
  SetTopWindow(mpTrackFrame);

  // Get the current working directory and append a directory separator.
  wxString CurrentWorkingDirectory =
    ::wxGetCwd() + wxFileName::GetPathSeparator();

  // This code should be distributed with a HelpFiles subdirectory under
  // the directory the executable is stored in.
  wxString HelpFileDirectoryGuess =
    CurrentWorkingDirectory + "HelpFiles" + wxFileName::GetPathSeparator();

  // Attempt to obtain the path to the help file from configuration data.
  wxString HelpFilePath;
  bool WasHelpPathRead = false;
  if (pConfig)
  {
    WasHelpPathRead = pConfig->Read(
      "/Paths/Help",
      &HelpFilePath,
      HelpFileDirectoryGuess);
  }

  // Construct a full file name.
  wxString HelpFileNameAndPath = HelpFilePath + mHelpFileName;

  // Test for the existence of the help file.
  bool HelpFileFound = false;
  ifstream Is;
  Is.open(HelpFileNameAndPath.c_str());
  if (!Is)
  {
    // Ask the user to find the help file.
    if (FindAndRegisterHelpFilePath(HelpFilePath))
    {
      HelpFileNameAndPath = HelpFilePath + mHelpFileName;

      // Try one more time.
      Is.close();
      Is.clear();
      Is.open(HelpFileNameAndPath.c_str());
      if (!Is)
      {
        wxString Message = "Failed to add the IPVT book " + mHelpFileName;
        ::wxMessageBox(Message);
      }
      else
      {
        HelpFileFound = true;
      }
    }
  }
  else
  {
    HelpFileFound = true;
  }

  if (HelpFileFound)
  {
    // GetUserDataDir returns the directory for the user-dependent application
    // data files.  The value is $HOME/.appname on Linux,
    // c:\Documents and Settings\username\Application Data\appname on
    // Windows, and ~/Library/Application Support/appname on the Mac.
    // The cached version of the help file will be placed in this location.
    mHelp.SetTempDir(wxStandardPaths::Get().GetUserDataDir());

    // Add the IPVT help file the the help system.
    mHelp.AddBook(HelpFileNameAndPath);

    if (!WasHelpPathRead && pConfig)
    {
      // Register the help path.
      pConfig->Write("/Paths/Help", HelpFilePath);
    }
  }

  return true;
}

//-----------------------------------------------------------------------------
// Description:
//   This function checks to see if the user's Jazz++ configuration directory
// files exist and creates the directory and copies default versions if they
// do not.
//-----------------------------------------------------------------------------
void JZJazzPlusPlusApplication::InsureConfigurationFileExistence() const
{
  // Determine the expected location of the user's data dir for Jazz++.
  wxString UserConfigDir = wxStandardPaths::Get().GetUserDataDir();

  // Determine if the directory exists.
  if (!wxDirExists(UserConfigDir))
  {
    // Attempt to create the directory.
    if (!wxMkdir(UserConfigDir))
    {
      wxString String;
      String
        << "Unable to create directory \""
        << UserConfigDir << '"';
      ::wxMessageBox(String, "Directory Creation Error");
    }
  }

  // Setup the wxWidgets configuration file.
  wxFileName WxConfigurationFileName(UserConfigDir, ".jazz");

  wxFileConfig* pFileConfig = new wxFileConfig(
    GetAppName(),
    wxEmptyString,
    WxConfigurationFileName.GetFullPath(),
    wxEmptyString,
    wxCONFIG_USE_LOCAL_FILE);

  delete wxConfigBase::Set(pFileConfig);

  // Make sure all of the configuration files are setup.
  vector<wxString> ConfigurationFileNames;
  ConfigurationFileNames.push_back("README");
  ConfigurationFileNames.push_back("jazz.cfg");
  ConfigurationFileNames.push_back("jazz.mid");
  ConfigurationFileNames.push_back("ctrlnam.jzi");
  ConfigurationFileNames.push_back("e26voice.jzi");
  ConfigurationFileNames.push_back("e26.jzi");
  ConfigurationFileNames.push_back("gm.jzi");
  ConfigurationFileNames.push_back("gmdrmnam.jzi");
  ConfigurationFileNames.push_back("gmdrmset.jzi");
  ConfigurationFileNames.push_back("gmvoices.jzi");
  ConfigurationFileNames.push_back("gs.jzi");
  ConfigurationFileNames.push_back("gsdrmset.jzi");
  ConfigurationFileNames.push_back("gsvoices.jzi");
  ConfigurationFileNames.push_back("jv1000.jzi");
  ConfigurationFileNames.push_back("sc88pdrm.jzi");
  ConfigurationFileNames.push_back("sc88pro.jzi");
  ConfigurationFileNames.push_back("sc88pvoi.jzi");
  ConfigurationFileNames.push_back("xg.jzi");
  ConfigurationFileNames.push_back("xgdrmnam.jzi");
  ConfigurationFileNames.push_back("xgdrmset.jzi");
  ConfigurationFileNames.push_back("xgvoices.jzi");

  for (
    vector<wxString>::const_iterator iConfigurationFileName =
      ConfigurationFileNames.begin();
    iConfigurationFileName != ConfigurationFileNames.end();
    ++iConfigurationFileName)
  {
    // Check to see if the user already has a jazz.cfg file in the
    // user configuration directory.
    wxString JazzCfgFile =
      UserConfigDir +
      wxFileName::GetPathSeparator() +
      *iConfigurationFileName;

    if (!::wxFileExists(JazzCfgFile))
    {
      // Attempt to copy the default Jazz++ configuration file to this
      // directory.
      wxString DefaultJazzCfgFile =
        wxStandardPaths::Get().GetDataDir() +
        wxFileName::GetPathSeparator() +
        *iConfigurationFileName;

      if (::wxFileExists(DefaultJazzCfgFile))
      {
        ::wxCopyFile(DefaultJazzCfgFile, JazzCfgFile);
      }
    }
  }
}

//-----------------------------------------------------------------------------
// Description:
//   The help file was not found so let the user search for it. If it is
// found, create a configuration entry so the code can find the help file path
// the next time it starts.
//-----------------------------------------------------------------------------
bool JZJazzPlusPlusApplication::FindAndRegisterHelpFilePath(
  wxString& HelpFilePath) const
{
  wxString Message;
  Message = "Unable to find " + mHelpFileName;
  ::wxMessageBox(Message, "Please Locate This File!");

  // Use an open dialog to find the help file.
  wxFileDialog OpenDialog(
    0,
    "Open the Help File",
    HelpFilePath,
    mHelpFileName,
    "*.hhp",
    wxFD_OPEN);

  if (OpenDialog.ShowModal() == wxID_OK)
  {
    // Generate a c-style string that contains a path to the help file.
    wxString TempHelpFilePath;
    TempHelpFilePath = ::wxPathOnly(OpenDialog.GetPath());
    TempHelpFilePath += ::wxFileName::GetPathSeparator();

    wxConfigBase* pConfig = wxConfigBase::Get();
    if (pConfig)
    {
      pConfig->Write("/Paths/Help", TempHelpFilePath);
    }

    // Return the user selected help file path.
    HelpFilePath = TempHelpFilePath;

    return true;
  }

  return false;
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

  JZProjectManager::Destroy();

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
