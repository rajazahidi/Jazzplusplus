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

#include "Help.h"

#include <wx/filedlg.h>
#include <wx/filefn.h>
#include <wx/filename.h>
#include <wx/html/helpctrl.h>
#include <wx/msgdlg.h>
#include <wx/stdpaths.h>

#include <fstream>
#include <vector>

using namespace std;

//*****************************************************************************
// Description:
//   This is the help class definition.
//*****************************************************************************
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
wxString JZHelp::mHelpFileName = "jazz.hhp";

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZHelp::JZHelp()
  : mpHelp(0)
{
  mpHelp = new wxHtmlHelpController(wxHF_DEFAULT_STYLE | wxHF_OPEN_FILES);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZHelp::~JZHelp()
{
  CloseHelp();
  delete mpHelp;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZHelp::ShowTopic(const wxString& TopicString)
{
  ifstream test(mHelpFileName.mb_str());
  if (!test.is_open())
  {
    ConfigureHelp();
    ifstream test2(mHelpFileName.mb_str());
    if (!test2.is_open())
    {
      wxString HelpFilePath;
      if (FindAndRegisterHelpFilePath(HelpFilePath))
      {
        mHelpFileName = HelpFilePath + "jazz.hhp";
        mpHelp->AddBook(mHelpFileName);
      }
      else
      {
        return;
      }
    }
  }
  mpHelp->LoadFile(mHelpFileName);
  mpHelp->KeywordSearch(TopicString);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZHelp::DisplayHelpContents()
{
  ifstream test(mHelpFileName.mb_str());
  if (!test.is_open())
  {
    ConfigureHelp();
    ifstream test2(mHelpFileName.mb_str());
    if (!test2.is_open())
    {
      wxString HelpFilePath;
      if (FindAndRegisterHelpFilePath(HelpFilePath))
      {
        mHelpFileName = HelpFilePath + "jazz.hhp";
        mpHelp->AddBook(mHelpFileName);
      }
      else
      {
        return;
      }
    }
  }
  mpHelp->LoadFile(mHelpFileName);
  mpHelp->DisplayContents();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZHelp::CloseHelp()
{
  // GetFrame returns NULL if there is no help frame active.
  if (mpHelp->GetFrame())
  {
    // Close the help frame; this will cause the config data to get written.
    mpHelp->GetFrame()->Close(true);
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZHelp::ConfigureHelp()
{
  wxConfigBase* pConfig = wxConfigBase::Get();

  // Let the help system store the Jazz++ help configuration info.
  mpHelp->UseConfig(pConfig);
  mpHelp->SetTempDir(wxStandardPaths::Get().GetUserDataDir());

  wxString HelpFilePath;
  if (pConfig)
  {
    pConfig->Read("/Paths/Help", &HelpFilePath);
  }

  std::vector<wxString> candidatePaths;
  if (!HelpFilePath.empty())
  {
    if (HelpFilePath.Last() != wxFileName::GetPathSeparator())
    {
      HelpFilePath += wxFileName::GetPathSeparator();
    }
    candidatePaths.push_back(HelpFilePath);
  }

  // 1. Data dir
  candidatePaths.push_back(
    wxStandardPaths::Get().GetDataDir() +
    wxFileName::GetPathSeparator() +
    "HelpFiles" +
    wxFileName::GetPathSeparator());

  // 2. Relative to executable
  wxString exeDir = ::wxPathOnly(wxStandardPaths::Get().GetExecutablePath());
  candidatePaths.push_back(exeDir + wxFileName::GetPathSeparator() + "HelpFiles" + wxFileName::GetPathSeparator());
  candidatePaths.push_back(exeDir + wxFileName::GetPathSeparator() + ".." + wxFileName::GetPathSeparator() + "src" + wxFileName::GetPathSeparator() + "HelpFiles" + wxFileName::GetPathSeparator());
  candidatePaths.push_back(exeDir + wxFileName::GetPathSeparator() + ".." + wxFileName::GetPathSeparator() + "HelpFiles" + wxFileName::GetPathSeparator());

  // 3. Current working directory
  candidatePaths.push_back(wxGetCwd() + wxFileName::GetPathSeparator() + "src" + wxFileName::GetPathSeparator() + "HelpFiles" + wxFileName::GetPathSeparator());
  candidatePaths.push_back(wxGetCwd() + wxFileName::GetPathSeparator() + "HelpFiles" + wxFileName::GetPathSeparator());

  // 4. Source root / standard install paths
  candidatePaths.push_back("/usr/share/jazz/HelpFiles/");
  candidatePaths.push_back("/usr/local/share/jazz/HelpFiles/");

  bool HelpFileFound = false;
  wxString foundPath;
  for (size_t i = 0; i < candidatePaths.size(); ++i)
  {
    wxString testPath = candidatePaths[i] + "jazz.hhp";
    ifstream testFile(testPath.mb_str());
    if (testFile.is_open())
    {
      HelpFileFound = true;
      foundPath = candidatePaths[i];
      break;
    }
  }

  if (HelpFileFound)
  {
    HelpFilePath = foundPath;
    mHelpFileName = HelpFilePath + "jazz.hhp";
    mpHelp->AddBook(mHelpFileName);

    if (pConfig)
    {
      pConfig->Write("/Paths/Help", HelpFilePath);
    }
  }
}

//-----------------------------------------------------------------------------
// Description:
//   This function walks the user through a top-level help file search.  If
// the help file is found, create a configuration entry so the code can find
// the help file path the next time the application starts.
//
// Outputs:
//   wxString& HelpFilePath:
//     A user-selected path to the help file.  The calling code should check
//     to insure the help file is actually in this path.
//-----------------------------------------------------------------------------
bool JZHelp::FindAndRegisterHelpFilePath(wxString& HelpFilePath) const
{
  wxString Message;
  Message =
    "Unable to find " + mHelpFileName + "\n" +
    "Would you like to locate this file?";
  int Response = ::wxMessageBox(
    Message,
    "Cannnot Find Help File",
    wxOK | wxCANCEL);

  if (Response == wxOK)
  {
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
      // Generate a string that contains a path to the help file.
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
  }

  return false;
}
