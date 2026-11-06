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

#include <wx/app.h>
#include <wx/dialog.h>
#include <wx/filedlg.h>
#include <wx/filefn.h>
#include <wx/filename.h>
#include <wx/html/helpctrl.h>
#include <wx/html/helpwnd.h>
#include <wx/html/htmlwin.h>
#include <wx/dcclient.h>
#include <wx/msgdlg.h>
#include <wx/stdpaths.h>
#include <wx/toplevel.h>
#include <wx/utils.h>

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
static void ConfigureHelpWindowBackground(wxHtmlHelpController* pHelpCtrl)
{
  if (!pHelpCtrl)
  {
    return;
  }
  wxHtmlHelpWindow* pHelpWin = pHelpCtrl->GetHelpWindow();
  if (pHelpWin)
  {
    wxHtmlWindow* pHtml = pHelpWin->GetHtmlWindow();
    if (pHtml)
    {
      pHtml->SetDoubleBuffered(true);
      pHtml->SetBackgroundColour(*wxWHITE);
      if (pHtml->GetName() != "JazzHelpConfigured")
      {
        pHtml->SetName("JazzHelpConfigured");

        pHtml->Bind(wxEVT_ERASE_BACKGROUND, [](wxEraseEvent& event) {
          wxDC* pDc = event.GetDC();
          if (pDc)
          {
            pDc->SetBackground(*wxWHITE_BRUSH);
            pDc->Clear();
          }
          else
          {
            event.Skip();
          }
        });

        pHtml->Bind(wxEVT_HTML_LINK_CLICKED, [](wxHtmlLinkEvent& event) {
          const wxString href = event.GetLinkInfo().GetHref();
          if (href.StartsWith("http://") || href.StartsWith("https://") || href.StartsWith("mailto:"))
          {
            wxLaunchDefaultBrowser(href);
          }
          else
          {
            event.Skip();
          }
        });
      }
      pHtml->Refresh();
    }
  }
}

//-----------------------------------------------------------------------------
// Helper to locate active modal dialog
//-----------------------------------------------------------------------------
static wxWindow* ResolveModalParent(wxWindow* pParent, bool& outIsModal)
{
  outIsModal = false;
  if (!pParent)
  {
    pParent = wxGetActiveWindow();
  }
  if (pParent)
  {
    wxDialog* pDialog = wxDynamicCast(pParent, wxDialog);
    if (pDialog && pDialog->IsModal())
    {
      outIsModal = true;
      return pParent;
    }
  }
  // Check top level windows for any active modal dialog
  if (wxTheApp)
  {
    for (wxWindowList::compatibility_iterator node = wxTopLevelWindows.GetFirst(); node; node = node->GetNext())
    {
      wxDialog* dlg = wxDynamicCast(node->GetData(), wxDialog);
      if (dlg && dlg->IsModal())
      {
        outIsModal = true;
        return dlg;
      }
    }
  }
  return pParent;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool JZHelp::EnsureHelpBookLoaded(wxHtmlHelpController* pHelpCtrl)
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
        if (pHelpCtrl)
        {
          pHelpCtrl->AddBook(mHelpFileName);
        }
        return true;
      }
      return false;
    }
  }
  if (pHelpCtrl)
  {
    pHelpCtrl->AddBook(mHelpFileName);
  }
  return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZHelp::ShowTopic(const wxString& TopicString, wxWindow* pParent)
{
  bool isModal = false;
  pParent = ResolveModalParent(pParent, isModal);

  if (isModal && pParent)
  {
    if (!EnsureHelpBookLoaded(nullptr))
    {
      return;
    }
    // Launch a modal help controller with wxHF_DIALOG | wxHF_MODAL.
    // This creates an interactive wxHtmlHelpDialog as a direct modal child of
    // the active modal dialog, preventing Windows from disabling the window,
    // allowing it to be clicked, navigated, and closed via [X] or Close button
    // without hanging or crashing.
    // (int style, wxWindow* parent) matches wxWidgets 3.0, 3.1, and 3.2+.
    wxHtmlHelpController modalHelp(wxHF_DEFAULT_STYLE | wxHF_DIALOG | wxHF_MODAL, pParent);
    modalHelp.UseConfig(wxConfigBase::Get());
    modalHelp.SetTempDir(wxStandardPaths::Get().GetUserDataDir());
    modalHelp.AddBook(mHelpFileName);
    ConfigureHelpWindowBackground(&modalHelp);

    bool displayed = false;
    if (!TopicString.IsEmpty())
    {
      displayed = modalHelp.KeywordSearch(TopicString);
      if (!displayed)
      {
        displayed = modalHelp.Display(TopicString);
      }
    }
    if (!displayed)
    {
      modalHelp.DisplayContents();
    }
    return;
  }

  // Modeless invocation (from main application frame or piano roll)
  if (!EnsureHelpBookLoaded(mpHelp))
  {
    return;
  }
  bool displayed = false;
  if (!TopicString.IsEmpty())
  {
    displayed = mpHelp->KeywordSearch(TopicString);
    if (!displayed)
    {
      displayed = mpHelp->Display(TopicString);
    }
  }
  if (!displayed)
  {
    mpHelp->DisplayContents();
  }
  ConfigureHelpWindowBackground(mpHelp);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZHelp::DisplayHelpContents(wxWindow* pParent)
{
  bool isModal = false;
  pParent = ResolveModalParent(pParent, isModal);

  if (isModal && pParent)
  {
    if (!EnsureHelpBookLoaded(nullptr))
    {
      return;
    }
    wxHtmlHelpController modalHelp(wxHF_DEFAULT_STYLE | wxHF_DIALOG | wxHF_MODAL, pParent);
    modalHelp.UseConfig(wxConfigBase::Get());
    modalHelp.SetTempDir(wxStandardPaths::Get().GetUserDataDir());
    modalHelp.AddBook(mHelpFileName);
    ConfigureHelpWindowBackground(&modalHelp);
    modalHelp.DisplayContents();
    return;
  }

  if (!EnsureHelpBookLoaded(mpHelp))
  {
    return;
  }
  mpHelp->DisplayContents();
  ConfigureHelpWindowBackground(mpHelp);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZHelp::CloseHelp()
{
  if (!mpHelp)
  {
    return;
  }
  if (mpHelp->GetFrame())
  {
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
