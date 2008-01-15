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

#ifndef JZ_TRACKFRAME_H
#define JZ_TRACKFRAME_H

class JZToolBar;
class JZTrackWindow;
class JZPianoFrame;

//*****************************************************************************
//*****************************************************************************
class JZTrackFrame : public wxFrame
{
  public:

    JZTrackFrame(
      wxWindow* pParent,
      const wxString& Title,
      const wxPoint& Position = wxDefaultPosition,
      const wxSize& Size = wxDefaultSize);

    virtual ~JZTrackFrame();

  private:

    bool OnClose();

    void CreateToolBar();

    void CreateMenu();

    void OnFileExit(wxCommandEvent& Event);

    void OnPianoWindow(wxCommandEvent& Event);

    void OnHelpContents(wxCommandEvent& Event);

    void OnHelpAbout(wxCommandEvent& Event);

  private:

    JZToolBar* mpToolBar;

    wxMenu* mpFileMenu;

    wxMenu* mpEditMenu;

    JZTrackWindow* mpTrackWindow;

//    JZPianoFrame* mpPianoFrame;

  DECLARE_EVENT_TABLE()
};

#endif // !defined(JZ_TRACKFRAME_H)
