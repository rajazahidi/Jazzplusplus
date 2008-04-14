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

#ifndef JZ_PIANOWINDOW_H
#define JZ_PIANOWINDOW_H

#include "EventWindow.h"
#include "MouseAction.h"
#include "Track.h"
#include "Globals.h"

class JZPianoFrame;
class JZSong;
class JZTrack;
class JZEvent;
class JZFilter;
class tCtrlEditBase;
class tSnapSelection;

//*****************************************************************************
//*****************************************************************************
class JZListen : public wxTimer
{
  public:

    JZListen();

    void KeyOn(
      JZTrack *t,
      int Pitch,
      int Channel,
      int Velocity = 64,
      int MilliSeconds = 100);

    void Notify();

  private:

    bool mActive;
    int mPitch, mChannel;

    JZTrack* mpTrack;
};

//*****************************************************************************
//*****************************************************************************
class JZPianoWindow : public JZEventWindow, public tButtonLabelInterface
{
  public:

    JZPianoWindow(
      JZPianoFrame* pFrame,
      JZSong* pSong,
      const wxPoint& Position = wxDefaultPosition,
      const wxSize& Size = wxDefaultSize);

    virtual ~JZPianoWindow();

    JZFilter* GetFilter();

    JZTrack* GetTrack();

    void ShowPitch(int Pitch);

    // Channel of actual track 0..15
    int Channel();

    const wxFont* GetFont() const;

    const wxFont* GetFixedFont() const;

    void UpdateControl();

    void ApplyToTrack(JZEvent* pEvent1, JZEvent* pEvent2);

    void KillTrackEvent(JZEvent* pEvent);

    void DrawPlayPosition(wxDC& Dc);

    void NewPosition(int TrackIndex, int Clock);

    void NewPlayPosition(int Clock);

    void ZoomIn();

    void ZoomOut();

    int TrackIndex2y(int TrackIndex);

    int IsVisible(JZEvent* pEvent);

    int IsVisible(JZTrack* pTrack);

    void SnapSelStart(wxMouseEvent& Event);

    int SnapClock(int Clock, int up = 0);

    void SnapSelStop(wxMouseEvent& Event);

    int SnapClocks();

    void SetSnapDenom(int Value);

    void SnapDialog();

    int y2Pitch(int y);

    void CtrlVelocity();

    void CtrlChannelAftertouchEdit();

    void CtrlPolyAftertouchEdit();

    void CtrlNone();

    void CtrlTempo();

    void EditFilter();

    void SelectController();

    void CtrlModulation();

    void CtrlPitch();

    void Redo();

    void Undo();

    void Quantize();

    void ExchangeUpDown();

    void ExchangeLeftRight();

    void ShiftLeft();

    void ShiftRight();

    void CutOrCopy(int Id);

    void Erase();

    void ToggleVisibleAllTracks();

    void MSelect();

    void MLength();

    void MDialog();

    void MCutPaste();

    void Snap8();

    void Snap8D();

    void Snap16();

    void Snap16D();

    void SetVisibleAllTracks(bool Value);

    void ActivateSettingsDialog();

    void ActivateMidiDelayDialog();

    void ActivateSequenceLengthDialog();

    void ActivateVelocityDialog();

  private:

    JZPianoFrame* mpPianoFrame;

  public:

    int mFromLines[eMaxTrackCount];

    int mPlayClock;

    enum TESizes
    {
      eMaxSnaps = 500
    };

    int mSnapCount;

    int mSnapsX[eMaxSnaps];

    tMouseAction* mpMouseAction;

    tEventArray mPasteBuffer;

  public:

    void SetScrollRanges();

    void DrawEvent(
      wxDC& Dc,
      JZEvent* pEvent,
      const wxBrush* Brush,
      int xoor,
      int force_color=0);

  private:

    void Setup();

    // Utils
    int Pitch2y(int Pitch);
    JZEvent *FindEvent(JZTrack *t, int Clock, int Pitch);
    void Copy(JZTrack* pTrack, JZEvent* pEvent, int Kill);
    void Paste(JZTrack* pTrack, int Clock, int Pitch);

    // SN++ Key_Aftertouch Utils
    void paste_keys_aftertouch(JZTrack *t, JZEvent *e);
    void kill_keys_aftertouch(JZTrack *t, JZEvent *e);
    int GetKeyOnEventCount();

    int y2TrackIndex(int y);
    int EventsSelected(const char *msg = 0);
    int OnEventWinMouseEvent(wxMouseEvent &e);

    void DrawEvents(
      wxDC& Dc,
      JZTrack *t,
      int Stat,
      const wxBrush* Brush,
      int force_colors);

    void DrawPianoRoll(wxDC& Dc);

    void LineText(
      wxDC& Dc,
      int x,
      int y,
      int w,
      int h,
      wxString str = "",
      bool down = FALSE);

    void OnSize(wxSizeEvent& Event);

    void OnEraseBackground(wxEraseEvent& Event);

    void OnDraw(wxDC& Dc);

    void OnPaint(wxMouseEvent& Event);

    void OnMouseEvent(wxMouseEvent& Event);

    void OnScroll(wxScrollWinEvent& Event);

    void HorizontalScroll(wxScrollWinEvent& Event);

    void VerticalScroll(wxScrollWinEvent& Event);

    void MouseCutPaste(wxMouseEvent& Event, bool Cut);

    void MouseEvents(wxMouseEvent& Event);

    void MousePiano(wxMouseEvent& Event);

    bool OnCharHook(wxKeyEvent& Event);

    void OnPaint(wxPaintEvent& Event);

    void OnChar(wxKeyEvent& Event);

    bool OnKeyEvent(wxKeyEvent& Event);

    void OnMenuCommand(int Id);

    void InitColors();

    void Draw(wxDC& Dc);

    // Overridden tButtonLabelInterface finction.
    void ButtonLabelDisplay(const wxString& Text, bool IsButtonDown);

  private:

    static JZListen mListen;

    JZTrack* mpTrack;

    int mTrackIndex;

    tCtrlEditBase* mpCtrlEdit;

    tMouseMapper mMousePlay;
    tMouseMapper mMouseEvent;

    // Number of colors to use for velocity representation.
    enum
    {
      NUM_COLORS = 24
    };

    wxBrush mpColorBrush[NUM_COLORS];

    int mPianoX, mPianoWidth;
    bool mUseColors;

    int mMouseLine;

    int mFontSize;
    wxFont* mpFont;

    // remains with 12pt/ bleibt bei 12pt
    wxFont* mpFixedFont;
    int mFixedFontHeight;        //Height letters/ Hoehe eines Buchstaben

    wxFont* mpDrumFont;

    int mSnapDenomiator;        // 16 for 16-tel

    bool mVisibleKeyOn;
    bool mVisiblePitch;
    bool mVisibleController;
    bool mVisibleProgram;
    bool mVisibleTempo;
    bool mVisibleSysex;
    bool mVisiblePlayTrack;
    bool mVisibleDrumNames;
    bool mVisibleAllTracks;
    bool mVisibleHBChord;
    bool mVisibleMono;

    bool mDrawing;

    wxBitmap* mpFrameBuffer;

  DECLARE_EVENT_TABLE()
};

inline
JZFilter* JZPianoWindow::GetFilter()
{
  return mpFilter;
}

inline
JZTrack* JZPianoWindow::GetTrack()
{
  return mpTrack;
}

inline
const wxFont* JZPianoWindow::GetFont() const
{
  return mpFont;
}

inline
const wxFont* JZPianoWindow::GetFixedFont() const
{
  return mpFixedFont;
}

#endif // !defined(JZ_PIANOWINDOW_H)
