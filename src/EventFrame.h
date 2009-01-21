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

#ifndef JZ_EVENTFRAME_H
#define JZ_EVENTFRAME_H

#include <wx/frame.h>

class JZEventWindow;
class JZFilter;
class JZSnapSelection;
class JZSong;
class JZToolBar;
class tMouseAction;
class wxDialog;

//*****************************************************************************
// Description:
//   A frame window that containes a scrolled event window.  This class acts
// as the common base class for the JZTrackFrame class and the JZPianoFrame
// class.
// Functionality:
//   - Settings dialog
//   - Selection via Snapsel
//*****************************************************************************
class JZEventFrame : public wxFrame
{
  public:

    // 2-step initialization: 1) constructor
    JZEventFrame(
      wxWindow* pParent,
      const wxString& Title,
      JZSong* pSong,
      const wxPoint& Position = wxDefaultPosition,
      const wxSize& Size = wxDefaultSize,
      long WindowStyle = wxDEFAULT_FRAME_STYLE);

    virtual ~JZEventFrame();

    virtual void SetEventWindow(JZEventWindow* pEventWindow);

    virtual void SnapSelStart(wxMouseEvent &e);
    virtual void SnapSelStop(wxMouseEvent &e);

    // Events
    virtual int  OnMouseEvent(wxMouseEvent& Event);
    virtual bool OnKeyEvent(wxKeyEvent& Event); // true = processed by eventwin
    virtual bool OnClose();

    // Redraw - nach Aenderungen von Parametern, kein GUI-Event
    virtual void Redraw();




    JZSong* Song;

    JZFilter* mpFilter;

    int mTopInfoHeight;

    int mEventsX, mEventsY, mEventsWidth, mEventsHeight;

    // Mouse handling
    JZSnapSelection* SnapSel;
    tMouseAction* MouseAction;

    // Settings-Dialog
    wxDialog* mpSettingsDialog;

    // Mixer-Dialog
    wxDialog* MixerForm;

  private:

    bool OnCharHook(wxKeyEvent& Event);

    void OnUpdateEditShift(wxUpdateUIEvent& Event);
    void OnEditShift(wxCommandEvent& Event);

    void OnQuantize(wxCommandEvent& Event);

    void MenSetChannel();
    void MenTranspose();
    void MenDelete();
    void MenVelocity();
    void MenLength();
    void MenSeqLength();
    void MenMidiDelay();
    void MenConvertToModulation();
    void MenCleanup();
    void MenSearchReplace();
    void MenMeterChange();

  protected:

    JZToolBar* mpToolBar;

    JZEventWindow* mpEventWindow;

  DECLARE_EVENT_TABLE()
};

#endif // !defined(JZ_EVENTFRAME_H)
