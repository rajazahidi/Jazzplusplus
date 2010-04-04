//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Copyright (C) 1994-2000 Andreas Voss and Per Sigmond, all rights reserved.
// Modifications Copyright (C) 2004 Patrick Earl
// Modifications Copyright (C) 2008-2010 Peter J. Stieber
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

#include "MouseAction.h"
#include "Metronome.h"

#include "EventFrame.h"

class JZSong;
class JZToolBar;
class JZTrackWindow;
class JZPianoWindow;

//*****************************************************************************
//*****************************************************************************
class JZTrackFrame : public JZEventFrame, public tButtonLabelInterface
{
  public:

    JZTrackFrame(
      wxWindow* pParent,
      const wxString& Title,
      JZSong* pSong,
      const wxPoint& Position = wxDefaultPosition,
      const wxSize& Size = wxDefaultSize);

    virtual ~JZTrackFrame();

    JZPianoWindow* GetPianoWindow()
    {
      return 0;
    }

    void NewPlayPosition(int Clock);

    // Overridden tButtonLabelInterface function.
    virtual void ButtonLabelDisplay(const wxString& Text, bool IsButtonDown);

  private:

    void OnMetroOn(wxCommandEvent& Event);

  private:

    bool OnClose();

    void CreateToolBar();

    void CreateMenu();

    void OnFileNew(wxCommandEvent& Event);

    void OnFileOpen(wxCommandEvent& Event);

    void OnFileSaveAs(wxCommandEvent& Event);

    void OnFileExit(wxCommandEvent& Event);

    void OnZoomIn(wxCommandEvent& Event);

    void OnZoomOut(wxCommandEvent& Event);

    void OnPlay(wxCommandEvent& Event);

    void OnPlayLoop(wxCommandEvent& Event);

    void OnRecord(wxCommandEvent& Event);

    void OnPianoWindow(wxCommandEvent& Event);

    void OnToolsHarmonyBrowser(wxCommandEvent& Event);

    void OnSettingsMetronome(wxCommandEvent& Event);

    void OnSettingsSynthesizerType(wxCommandEvent& Event);

    void OnSettingsMidiDevice(wxCommandEvent& Event);

    void OnAudioGlobalSettings(wxCommandEvent& Event);

    void OnHelpContents(wxCommandEvent& Event);

    void OnHelpAbout(wxCommandEvent& Event);

    void MousePlay(wxMouseEvent& MouseEvent, TEMousePlayMode Mode);

  private:

    JZToolBar* mpToolBar;

    wxMenu* mpFileMenu;

    wxMenu* mpEditMenu;

    wxMenu* mpToolsMenu;

    JZTrackWindow* mpTrackWindow;

    int mPreviousClock;
    bool mPreviouslyRecording;

  DECLARE_EVENT_TABLE()
};

//*****************************************************************************
// Description:
//   These are the track frame class inline member functions.
//*****************************************************************************
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline
void JZTrackFrame::ButtonLabelDisplay(const wxString& Text, bool IsButtonDown)
{
}

#endif // !defined(JZ_TRACKFRAME_H)
