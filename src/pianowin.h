/*
**  The JAZZ++ Midi Sequencer
**
** Copyright (C) 1994-2000 Andreas Voss and Per Sigmond, all rights reserved.
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
*/                                                                              


#ifndef pianowin_h
#define pianowin_h

#ifndef eventwin_h
#include "eventwin.h"
#endif

#ifndef track_h
#include "track.h"
#endif

#ifndef song_h
#include "song.h"
#endif

#define MaxSnaps 500

class tCtrlEditBase;

class tPianoWin : public tEventWin
{
  friend class tGuitarWin;
 public:
  tPianoWin(wxFrame *frame, char *title, tSong *song, int x, int y, int width, int height);
  virtual ~tPianoWin();
    


    void OnMSelect();
    void OnMLength();
    void OnMDialog();
    void OnMCutPaste();




    void OnZoomIn();
    void OnZoomOut();
    void OnSnap8();
    void OnSnap8D();
    void OnSnap16();
    void OnSnap16D();
    long xPiano, wPiano;

    int  nSnaps;
    long xSnaps[MaxSnaps];

    int  FromLines[MaxTracks];

    int  IsVisible(tEvent *e);
    int  IsVisible(tTrack *t);
    void VisibleDialog();
    Bool VisibleKeyOn;
    Bool VisiblePitch;
    Bool VisibleController;
    Bool VisibleProgram;
    Bool VisibleTempo;
    Bool VisibleSysex;
    Bool VisiblePlayTrack;
    Bool VisibleDrumNames;
    Bool VisibleAllTracks;
    Bool VisibleHBChord;
    // SN++
    Bool VisibleMono;

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
    void SnapDlg();

    int TrackNr;	// aktueller Track
    tTrack *Track;

    tEventArray PasteBuffer;
    void DrawEvents(wxDC* dc, tTrack *t, int Stat, wxBrush *Brush, int force_colors);
    void DrawEvent(wxDC* dc, tEvent *, wxBrush *Brush, int xoor, int force_color=0);
    void DrawPianoRoll(wxDC* dc);

    int OnMouseEvent(wxMouseEvent &e);
    int OnKeyEvent(wxKeyEvent &e);
    void MouseEvents(wxMouseEvent &e);
    void MousePiano(wxMouseEvent &e);
    void MouseCutPaste(wxMouseEvent &e, Bool cut);

    void OnGuitar();

    // Utils
    int y2Pitch(long y);
    long Pitch2y(int Pitch);
    tEvent *FindEvent(tTrack *t, long Clock, int Pitch);
    void Copy(tTrack *t, tEvent *e, int Kill);
    void Paste(tTrack *t, long Clock, int Pitch);
    // SN++ Key_Aftertouch Utils
    void paste_keys_aftertouch(tTrack *t, tEvent *e);
    void kill_keys_aftertouch(tTrack *t, tEvent *e);
    int  nKeyOnEvents();
    //

#ifndef __PORTING

    bool OnClose();
#endif // __PORTING
    virtual void OnSize(wxSizeEvent& event);
    tGuitarWin    *GetGuitarWin()	{ return GuitarWin; }

    void SetSnapDenom(long value);
    void PressRadio(int id = 0);
    void SetVisibleAllTracks(Bool value);
// SN++ made public for mouse keylength dragger
   tCtrlEditBase *CtrlEdit;
  void CutOrCopy(int id);
  DECLARE_EVENT_TABLE()
void OnFilter();
  void OnSettingsDialog();
void CtrlChannelAftertouchEdit();
  void OnCtrlPolyAftertouchEdit();
  void OnCtrlNone();
  void OnCtrlTempo();
  void OnCtrlVelocity();
  void OnSelectController();
  void OnCtrlModulation();
  void OnCtrlPitch();
  void OnRedo();
  void OnUndo();
  void OnQuantize();
  void OnExchangeUpDown();
  void OnExchangeLeftRight();
  void OnShiftLeft();
  void OnShiftRight();
  void OnShift();
  void OnCut();
  void OnCopy();
  void OnErase();
  void OnVisibleAllTracks();
  void OnReset();


  private:
    // Next 3 statements are "Patrick Approved."
    enum { NUM_COLORS = 24 }; // Number of colors to use for velocity
                              // representation.
    wxBrush color_brush[NUM_COLORS];
    void InitColors();

//    tCtrlEditBase *CtrlEdit;
    tGuitarWin    *GuitarWin;

    tMouseMapper MousePlay;
    tMouseMapper MouseEvnt;


};

#endif

