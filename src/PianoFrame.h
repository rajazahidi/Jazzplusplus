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
class tFilter;
class tCtrlEditBase;
class tTrack;
class JZPianoWindow;

//*****************************************************************************
//*****************************************************************************
class JZPianoFrame : public wxFrame, public tButtonLabelInterface
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

    // Overridden tButtonLabelInterface finction.
    void ButtonLabelDisplay(const wxString& Text, bool IsButtonDown);

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
    long xPiano, wPiano;

    int  nSnaps;
    enum TESizes
    {
      MaxSnaps = 500
    };

    long xSnaps[MaxSnaps];

    int  FromLines[MaxTracks];

    int  IsVisible(JZEvent *e);
    int  IsVisible(tTrack *t);
    void VisibleDialog();
    bool VisibleKeyOn;
    bool VisiblePitch;
    bool VisibleController;
    bool VisibleProgram;
    bool VisibleTempo;
    bool VisibleSysex;
    bool VisiblePlayTrack;
    bool VisibleDrumNames;
    bool VisibleAllTracks;
    bool VisibleHBChord;
    bool VisibleMono;

    long MouseLine;

    wxFont *DrumFont;

    long SnapClocks();
    long SnapDenomiator;	// 16 fuer 16-tel
    long SnapClock(long Clock, int up = 0);


    void CreateMenu();
    void Setup();
    void NewPosition(int TrackNr, long Clock);
    void ShowPitch(int pitch);
    int  Channel();	// Channel of actual track 0..15

    void OnMenuCommand(int Id);
    void OnPaintSub(wxDC* dc,long x, long y);
    void SnapSelStart(wxMouseEvent &e);
    void SnapSelStop(wxMouseEvent &e);

    void SnapDlg(wxCommandEvent& Event);

    int TrackNr;	// aktueller Track
    tTrack *Track;

    tEventArray PasteBuffer;
    void DrawEvents(wxDC* dc, tTrack *t, int Stat, const wxBrush* Brush, int force_colors);
    void DrawEvent(wxDC* dc, JZEvent *, const wxBrush* Brush, int xoor, int force_color=0);
    void DrawPianoRoll(wxDC* dc);

    int OnMouseEvent(wxMouseEvent &e);
    bool OnKeyEvent(wxKeyEvent &e);
    void MouseEvents(wxMouseEvent &e);
    void MousePiano(wxMouseEvent &e);
    void MouseCutPaste(wxMouseEvent &e, bool cut);

    JZGuitarFrame* GetGuitarFrame()
    {
      return mpGuitarFrame;
    }

    // Utils
    int y2Pitch(long y);
    long Pitch2y(int Pitch);
    JZEvent *FindEvent(tTrack *t, long Clock, int Pitch);
    void Copy(tTrack *t, JZEvent *e, int Kill);
    void Paste(tTrack *t, long Clock, int Pitch);
    // SN++ Key_Aftertouch Utils
    void paste_keys_aftertouch(tTrack *t, JZEvent *e);
    void kill_keys_aftertouch(tTrack *t, JZEvent *e);
    int  nKeyOnEvents();
    void LogicalMousePosition(wxMouseEvent &e, long *x, long *y);

    bool OnClose();

    virtual void OnSize(wxSizeEvent& Event);

    void SetSnapDenom(long value);
    void PressRadio(int id = 0);
    void SetVisibleAllTracks(bool value);
// SN++ made public for mouse keylength dragger
     tCtrlEditBase *CtrlEdit;
    void CutOrCopy(int id);

    void OnFilter(wxCommandEvent& Event);

    // Actions (Called from menu items, toolbar buttons, and wx events)
    void ActCloseEvent(wxCloseEvent& Event);
    void ActClose(wxCommandEvent& Event);
    void ActHelpMouse(wxCommandEvent& Event);
    void ActSettingsDialog(wxCommandEvent& Event);
    void ActMidiDelayDialog(wxCommandEvent& Event);
    void ActSequenceLengthDialog(wxCommandEvent& Event);
    void ActVelocityDialog(wxCommandEvent& Event);

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

  // Functions slurped from JZEventFrame
  public:

    void NewPlayPosition(long Clock);
    void DrawPlayPosition(wxDC* dc);
    void Redraw();
    void CreateCanvas();
    long Clock2x(long clk);
    long x2Clock(long x);
    long Line2y(long Line);
    long y2Line(long y, int up = 0);
    int EventsSelected(const char *msg = 0);
    void ZoomIn();
    void ZoomOut();
    void LineText(
      wxDC *dc,
      long x,
      long y,
      long w,
      long h,
      wxString str = "",
      bool down = FALSE);
    long x2BarClock(long x, int next);
    int OnEventWinMouseEvent(wxMouseEvent &e);
    void OnEventWinPaintSub(long x, long y);
    long y2yLine(long y, int up = 0);
    void GetVirtSize(long *w, long *h);
    bool OnCharHook(wxKeyEvent& e);

    JZPianoWindow *Canvas;
    tFilter* mpFilter;

    long FontSize;
    long ClocksPerPixel;
    JZSong* Song;
    wxFont* mpFont;
    wxFont* mpFixedFont;	//remains with 12pt/ bleibt bei 12pt
    int hFixedFont;	//Height letters/ Hoehe eines Buchstaben
    long hTop;
    long wLeft;
    long LittleBit;
    long FromLine, ToLine;
    long hLine;
    long xEvents, yEvents, wEvents, hEvents;
    long CanvasX, CanvasY, CanvasW, CanvasH;	// canvas coords
    long FromClock, ToClock;
    tSnapSelection* SnapSel;
    bool UseColors;
    tMouseAction *MouseAction;
    long PlayClock;
    wxDialog* DialogBox;
    wxDialog* MixerForm;

  private:

    // Next 3 statements are "Patrick Approved."
    enum { NUM_COLORS = 24 }; // Number of colors to use for velocity
                              // representation.
    wxBrush color_brush[NUM_COLORS];
    void InitColors();

//    tCtrlEditBase *CtrlEdit;

    tMouseMapper MousePlay;
    tMouseMapper MouseEvnt;

  private:

    void OnGuitar(wxCommandEvent& Event);

  private:

    JZToolBar* mpToolBar;

    JZGuitarFrame* mpGuitarFrame;

  DECLARE_EVENT_TABLE()
};

#endif // !defined(JZ_PIANOFRAME_H)
