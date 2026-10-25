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

#include "AboutDialog.h"

#include "JazzPlusPlusVersion.h"
#include "Help.h"
#include "Resources.h"

#include <wx/button.h>
#include <wx/html/htmlwin.h>
#include <wx/dcclient.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/statbmp.h>
#include <wx/version.h>

#include <sstream>

using namespace std;

#include "Bitmaps/JazzLogo.xpm"

//*****************************************************************************
// Description:
//   This is the Jazz++ application about dialog box definion.
//*****************************************************************************
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(JZAboutDialog, wxDialog)

  EVT_BUTTON(IDC_BN_VISIT_WEB_SITE, JZAboutDialog::OnVisitWebSite)

  EVT_BUTTON(wxID_HELP, JZAboutDialog::OnHelp)

END_EVENT_TABLE()

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZAboutDialog::JZAboutDialog(wxWindow* pParent)
  : wxDialog(pParent, wxID_ANY, wxString("About Jazz++")),
    mpLogo(0),
    mpBitmap(0)
{
  SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
  mpLogo = new wxBitmap((const char **) JazzLogo_xpm);
  mpBitmap = new wxStaticBitmap(this, wxID_ANY, *mpLogo);

  wxHtmlWindow* pHtmlWindow = new wxHtmlWindow(
    this,
    wxID_ANY,
    wxDefaultPosition,
    wxSize(450, 180),
    wxBORDER_NONE | wxHW_SCROLLBAR_AUTO);
  pHtmlWindow->SetDoubleBuffered(true);
  pHtmlWindow->SetBackgroundColour(*wxWHITE);
  pHtmlWindow->Bind(wxEVT_ERASE_BACKGROUND, [pHtmlWindow](wxEraseEvent& event) {
    wxDC* pDc = event.GetDC();
    if (pDc)
    {
      pDc->SetBackground(*wxWHITE_BRUSH);
      pDc->Clear();
    }
    else
    {
      wxClientDC clientDc(pHtmlWindow);
      clientDc.SetBackground(*wxWHITE_BRUSH);
      clientDc.Clear();
    }
  });

  pHtmlWindow->Bind(wxEVT_HTML_LINK_CLICKED, [](wxHtmlLinkEvent& event) {
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

  wxString LocaleString = wxLocale::GetSystemEncodingName();

  wxString VersionString;
  VersionString
    << JZJazzPlusPlusVersion::Instance().GetMajorVersion()
    << '.' << JZJazzPlusPlusVersion::Instance().GetMinorVersion()
    << '.' << JZJazzPlusPlusVersion::Instance().GetBuildNumber();

  // Indicate the wxWidgets version used in the build.
  wxString InformationString = "Jazz++ uses ";
  InformationString += wxVERSION_STRING;
  InformationString += "<br>";

  // Indicate the build date.
  InformationString += "Program build date: " __DATE__ "<br>";

  wxString Paragraph1String = "Jazz++ is an advanced MIDI &amp; Audio Sequencer workstation.";

//  wxString Paragraph2String = "Jazz++ is a MIDI sequencer program.";

  wxString HtmlString =
    "<html>"
    "<head><META http-equiv=\"Content-Type\" content=\"text/html; charset=" +
    LocaleString + "\">"
    "<style>"
    "body { background-color: #ffffff; color: #1e293b; margin: 8px 12px; font-family: -apple-system, BlinkMacSystemFont, Segoe UI, sans-serif; line-height: 1.4; }"
    "hr { display: none; }"
    "h3 { margin: 4px 0 2px 0; color: #0f172a; text-align: center; }"
    "h5 { margin: 10px 0 4px 0; color: #475569; text-align: center; text-transform: uppercase; font-size: 11px; }"
    "p { margin: 4px 0; font-size: 11px; text-align: center; }"
    "table { border: 0; width: 100%; border-collapse: collapse; margin-top: 4px; }"
    "td { border: 0; padding: 3px 6px; vertical-align: top; font-size: 11px; }"
    "</style>"
    "</head>"
    "<body bgcolor=\"#ffffff\" text=\"#1e293b\">"
    "<font size=2>"

    "<center><h3>" "Jazz++ " + VersionString + "</h3></center>"

    "<center><h5>" + "Information" + "</h5></center>"
    "<p>"
    + Paragraph1String +
    "</p>"

//    "<p>"
//    + Paragraph2String +
//    "</p>"

    "<p><center>" + InformationString + "</center></p>"

    "<center><h5>" + "Credits" + "</h5></center>"

    "<table border=0 cellpadding=2 cellspacing=0>"
    "<tr>"
     "<td><b>Pete Stieber</b></td>"
     "<td>" + "Lead Developer" + "</td>"
    "</tr>"
    "<tr>"
     "<td><b>Patrick Earl</b></td>"
     "<td>" + "Developer" + "</td>"
    "</tr>"
    "<tr>"
     "<td><b>Kevin Cosgrove</b></td>"
     "<td>" + "Developer. Also maintains RPM packages." + "</td>"
    "</tr>"
    "<tr>"
     "<td><b>Dave Fancella</b></td>"
     "<td>" +
       "Developer. Also wrote the first version of the JazzPlusPlus website "
       "hosted on SourceForge." +
     "</td>"
    "</tr>"
    "<tr>"
      "<td><b>Joakim Verona</b></td>"
      "<td>" +
        "Developer. Wrote most of the original wxWidgets 2.4 port of Jazz++." +
      "</td>"
    "</tr>"
    "<tr>"
      "<td><b>Raja Zahidi</b><br>&lt;rajazahidi@aol.com&gt;</td>"
      "<td>" +
        "Developer &amp; Maintainer. Core codebase fixes, stability &amp; crash repairs, modern ALSA audio subsystem, "
        "MIDI Effects Suite, Procedural Sound FX &amp; Drums, Audio DSP Engine, "
        "automated test suite (2026)." +
      "</td>"
    "</tr>"
    "</table>"

    "<center>"
    "<h5>" "Other contributors" "</h5>"
    "<p>"
    "Mark Constable<br>"
    "Matt Kelly<br>"
    "<p>"
    "<h5>" + "Special thanks" + "</h5>"
    "<p>"
    "The wxWidgets Team<br>"
    "</p>"
    "</center>"

    "</font>"
    "</body>"
    "</html>";

  pHtmlWindow->SetPage(HtmlString);

  wxBoxSizer* pTopSizer = new wxBoxSizer(wxVERTICAL);

  pTopSizer->Add(mpBitmap, 0, wxALIGN_CENTER | wxALL, 8);

  pTopSizer->Add(pHtmlWindow, 0, wxALIGN_CENTER | wxALL, 8);

  wxBoxSizer* pButtonsSizer = new wxBoxSizer(wxHORIZONTAL);

  pButtonsSizer->Add(new wxButton(this, wxID_OK, "OK"), 0, wxALL, 10);
  pButtonsSizer->Add(
    new wxButton(this, IDC_BN_VISIT_WEB_SITE, "Visit Web Site..."),
    0,
    wxALL,
    10);
  pButtonsSizer->Add(new wxButton(this, wxID_HELP, "Help"), 0, wxALL, 10);

  pTopSizer->Add(pButtonsSizer, 0, wxALIGN_CENTER);

  SetAutoLayout(true);
  SetSizer(pTopSizer);

  pTopSizer->SetSizeHints(this);
  pTopSizer->Fit(this);

  Centre();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZAboutDialog::~JZAboutDialog()
{
  delete mpLogo;
  delete mpBitmap;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZAboutDialog::OnVisitWebSite(wxCommandEvent&)
{
  wxLaunchDefaultBrowser("https://rajazahidi.github.io/Jazzplusplus/");
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZAboutDialog::OnHelp(wxCommandEvent& Event)
{
  JZHelp::Instance().DisplayHelpContents();
}
