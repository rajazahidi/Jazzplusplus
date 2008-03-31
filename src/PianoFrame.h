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

#ifndef JZ_PIANOFRAME_H
#define JZ_PIANOFRAME_H

#include "MouseAction.h"
#include "Song.h"

class JZGuitarFrame;
class JZSong;
class JZToolBar;
class tCtrlEditBase;
class JZTrack;
class JZPianoWindow;

//*****************************************************************************
//*****************************************************************************
class JZPianoFrame : public wxFrame
{
   friend class JZGuitarFrame;

  public:

    JZPianoFrame(
      wxWindow* pParent,
      const wxString& Title,
      JZSong* pSong,
      const wxPoint& Position = wxDefaultPosition,
      const wxSize& Size = wxDefaultSize);

    virtual ~JZPianoFrame();

    void CreateToolBar();

    void SetToolbarButtonState(int Id);

    void OnMSelect(wxCommandEvent& Event);
    void OnMLength(wxCommandEvent& Event);
    void OnMDialog(wxCommandEvent& Event);
    void OnMCutPaste(wxCommandEvent& Event);

    void OnZoomIn(wxCommandEvent& Event);
    void OnZoomOut(wxCommandEvent& Event);
    void OnSnap8(wxCommandEvent& Event);
    void OnSnap8D(wxCommandEvent& Event);
    void OnSnap16(wxCommandEvent& Event);
    void OnSnap16D(wxCommandEvent& Event);

    void VisibleDialog();


    void CreateMenu();

    void OnPaintSub(wxDC* dc, int x, int y);

    void OnSnapDlg(wxCommandEvent& Event);

    // Current track.
    int mTrackIndex;
    JZTrack* Track;

    void MouseCutPaste(wxMouseEvent &e, bool cut);

    JZGuitarFrame* GetGuitarFrame();

    bool OnClose();

    void PressRadio(int id = 0);
    void SetVisibleAllTracks(bool value);
// SN++ made public for mouse keylength dragger
    tCtrlEditBase* mpCtrlEdit;
    void CutOrCopy(int id);

    void OnFilter(wxCommandEvent& Event);

    // Actions (Called from menu items, toolbar buttons, and wx events)
    void ActCloseEvent(wxCloseEvent& Event);
    void ActClose(wxCommandEvent& Event);
    void ActHelpMouse(wxCommandEvent& Event);
    void OnActivateSettingsDialog(wxCommandEvent& Event);
    void OnActivateMidiDelayDialog(wxCommandEvent& Event);
    void OnActivateSequenceLengthDialog(wxCommandEvent& Event);
    void OnActivateVelocityDialog(wxCommandEvent& Event);

    void CtrlChannelAftertouchEdit(wxCommandEvent& Event);
    void OnCtrlPolyAftertouchEdit(wxCommandEvent& Event);
    void OnCtrlNone(wxCommandEvent& Event);
    void OnCtrlTempo(wxCommandEvent& Event);
    void OnCtrlVelocity(wxCommandEvent& Event);
    void OnSelectController(wxCommandEvent& Event);
    void OnCtrlModulation(wxCommandEvent& Event);
    void OnCtrlPitch(wxCommandEvent& Event);
    void OnRedo(wxCommandEvent& Event);
    void OnUndo(wxCommandEvent& Event);
    void OnQuantize(wxCommandEvent& Event);
    void OnExchangeUpDown(wxCommandEvent& Event);
    void OnExchangeLeftRight(wxCommandEvent& Event);
    void OnShiftLeft(wxCommandEvent& Event);
    void OnShiftRight(wxCommandEvent& Event);
    void OnShift(wxCommandEvent& Event);
    void OnCut(wxCommandEvent& Event);
    void OnCopy(wxCommandEvent& Event);
    void OnErase(wxCommandEvent& Event);
    void OnVisibleAllTracks(wxCommandEvent& Event);
    void OnReset(wxCommandEvent& Event);

  public:

    void NewPlayPosition(int Clock);
    void Redraw();

    JZPianoWindow* mpPianoWindow;

    int mClockTicsPerPixel;
    JZSong* mpSong;
    wxDialog* DialogBox;
    wxDialog* MixerForm;

  private:

    void OnGuitar(wxCommandEvent& Event);

  private:

    JZToolBar* mpToolBar;

  DECLARE_EVENT_TABLE()
};

#endif // !defined(JZ_PIANOFRAME_H)
