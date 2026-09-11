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

#include <wx/app.h>
#include <wx/config.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>

#include <iostream>
#include <vector>

using namespace std;

//*****************************************************************************
// Description:
//   This function attempts to find a file.  It checks for the existence of
// the file across various standard and relative candidate directories:
//
// 1. the passed file name directly
// 2. user configuration directory (where user copies reside)
// 3. /Paths/Conf registered in wxConfig
// 4. conf/ subfolder and base folder next to the executable
// 5. conf/ subfolder and base folder in application data directory
// 6. conf/ subfolder and base folder in resources directory
// 7. current working directory (with and without conf/ subfolder)
// 8. paths specified by JAZZ or HOME environment variables
// 9. standard Linux system installation paths
//
// Returns:
//   wxString:
//     A complete path and file name for the found file or wxEmptyString if
//     the file was not found.
//*****************************************************************************
wxString FindFile(const wxString& FileName)
{
  if (::wxFileExists(FileName))
  {
    cout << "FindFile: Immediate hit on file \"" << FileName << '"' << endl;
    return FileName;
  }

  vector<wxString> searchDirs;

  // 1. User config directory
  wxString userDir = wxStandardPaths::Get().GetUserDataDir();
  if (!userDir.empty())
  {
    searchDirs.push_back(userDir);
  }

  // 2. Configured /Paths/Conf if registered
  wxConfigBase* pConfig = wxConfigBase::Get();
  if (pConfig)
  {
    wxString confPath;
    if (pConfig->Read("/Paths/Conf", &confPath) && !confPath.empty())
    {
      searchDirs.push_back(confPath);
    }
  }

  // 3. Executable directory (conf/ subfolder and base)
  wxString exeDir = ::wxPathOnly(wxStandardPaths::Get().GetExecutablePath());
  if (exeDir.empty() && wxTheApp && wxTheApp->argv)
  {
    exeDir = ::wxPathOnly(wxTheApp->argv[0]);
  }
  if (!exeDir.empty())
  {
    searchDirs.push_back(exeDir + wxFileName::GetPathSeparator() + "conf");
    searchDirs.push_back(exeDir);
  }

  // 4. Application data directory (conf/ subfolder and base)
  wxString dataDir = wxStandardPaths::Get().GetDataDir();
  if (!dataDir.empty())
  {
    searchDirs.push_back(dataDir + wxFileName::GetPathSeparator() + "conf");
    searchDirs.push_back(dataDir);
  }

  // 5. Resources directory
  wxString resDir = wxStandardPaths::Get().GetResourcesDir();
  if (!resDir.empty())
  {
    searchDirs.push_back(resDir + wxFileName::GetPathSeparator() + "conf");
    searchDirs.push_back(resDir);
  }

  // 6. Current working directory
  searchDirs.push_back(wxGetCwd() + wxFileName::GetPathSeparator() + "conf");
  searchDirs.push_back(wxGetCwd());

  // 7. JAZZ environment variable
  if (getenv("JAZZ") != 0)
  {
    wxString jazzEnv = getenv("JAZZ");
    searchDirs.push_back(jazzEnv + wxFileName::GetPathSeparator() + "conf");
    searchDirs.push_back(jazzEnv);
  }

  // 8. HOME environment variable
  if (getenv("HOME") != 0)
  {
    wxString home = getenv("HOME");
    searchDirs.push_back(home + wxFileName::GetPathSeparator() + ".jazz");
    searchDirs.push_back(home);
  }

  // 9. Standard system paths
  searchDirs.push_back("/usr/local/share/jazz/conf");
  searchDirs.push_back("/usr/share/jazz/conf");
  searchDirs.push_back("/usr/local/share/jazz");
  searchDirs.push_back("/usr/share/jazz");

  for (size_t i = 0; i < searchDirs.size(); ++i)
  {
    wxString candidate = searchDirs[i] + wxFileName::GetPathSeparator() + FileName;
    if (::wxFileExists(candidate))
    {
      cout << "FindFile: Found \"" << FileName << "\" at \"" << candidate << '"' << endl;
      return candidate;
    }
  }

  cout << "FindFile: File not found: \"" << FileName << '"' << endl;
  return wxEmptyString;
}
