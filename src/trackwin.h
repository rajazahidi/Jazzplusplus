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


#ifndef trackwin_h
#define trackwin_h

#include "eventwin.h"
#include "song.h"


#define MaxBars 200

class tRecordInfo
{
public:
  tTrack *Track;	// 0 == not recording

  long   FromClock;     // recording from clock
  long   ToClock;	// recording to clock
  long   TrackNr;	// recording on this track
  int    Muted;		// recording track is muted
};

enum tCounterModes
{
  CmProgram,
  CmBank,
  CmVolume,
  CmPan,
  CmReverb,
  CmChorus,
  CmModes
};

enum tNumberModes
{
  NmTrackNr,
  NmMidiChannel,
  NmModes
};

class tPianoWin;
class tGenMelody;
class tEventList;
class tArpeggioWin;

/**
the main window in jazz
*/
class tTrackWin : public tEventWin,
                  public tButtonLabelInterface
{
    // Method in tButtonLabelInterface
    virtual void ButtonLabelDisplay(wxString text, Bool down);

    int xNumber, wNumber;
    int xName,   wName;
    int xState,  wState;
    int xPatch,  wPatch;

    int nBars;
    long xBars[MaxBars];

    // restart play here if space bar hit
    long prev_clock;
    Bool prev_loop;
    Bool prev_muted;
    Bool prev_record;

    tSong *paste_buffer;
    tGenMelody *meldy_win;
    tEventList *eventlst_win;
    tArpeggioWin  *arpeggio_win;

  public:
    wxMenu *file_menu;
    wxMenu *edit_menu;
    wxMenu *parts_menu;
    wxMenu *bender_menu;
    wxMenu *modulation_menu;
    wxMenu *caf_menu;
    wxMenu *paf_menu;
    wxMenu *cc1_menu;
    wxMenu *cc2_menu;
    wxMenu *setting_menu;
    wxMenu *save_settings_menu;
    wxMenu *misc_menu;
    wxMenu *help_menu;
    wxMenu *audio_menu;

    void EnableDisableMenus();

    tNumberModes NumberMode;	// what to show
    tCounterModes CounterMode;	// what to show

    tPianoWin *GetPianoWin()	{ return (tPianoWin *)NextWin; }
    void DrawNumbers(wxDC* dc);
    void DrawSpeed(wxDC* dc, int Value = -1, Bool down = FALSE);
    void DrawCounters(wxDC* dc);
    void DrawEvents(wxDC* dc);
    const char *CounterStr();
    const char *NumberStr();
    long x2xBar(long x);
    long x2wBar(long x);
    tTrack *y2Track(long y);
    void Mark(long x, long y);	// mark a bar
    void UnMark();
    tRect Marked;

    bool OnClose();
    void Setup();

    tMetronomeInfo MetronomeInfo;

    tTrackWin(wxFrame *frame, char *title, tSong *song, int x = -1, int y = -1, int width = -1, int height = -1 );
    virtual ~tTrackWin();

    void CreateMenu();
    //void OnMenuCommand(int Id);
    void OnPaintSub(wxDC* dc,long x, long y);
    void SnapSelStart(wxMouseEvent &e);
    void SnapSelStop(wxMouseEvent &e);

    virtual int tTrackWin::OnKeyEvent(wxKeyEvent &e);

    int OnMouseEvent(wxMouseEvent &e);
    void MouseNumber(wxMouseEvent &);
    void MouseName(wxMouseEvent &);
    void MouseState(wxMouseEvent &);
    void MousePatch(wxMouseEvent &);
    void MouseEvents(wxMouseEvent &);
    enum MousePlayMode {Mouse, SpaceBar, PlayButton, PlayLoopButton, RecordButton };
    void MousePlay(wxMouseEvent *e, MousePlayMode mode = SpaceBar);
    void MouseSpeed(wxMouseEvent &e);

    tRecordInfo RecInfo;

    void MenCopy();
    void MenSongSettings();
    void MenMetronomeSettings();
    void MenCopyright();
    void MenSplitTracks();
    void MenMergeTracks();
    void MenMixer();
    void MenMaster();
    void MenVibrato();
    void MenSound();
    void MenEnvelope();
    void MenBendBasic();
    void MenBendLfo1();
    void MenBendLfo2();
    void MenModBasic();
    void MenModLfo1();
    void MenModLfo2();
    void MenCAfBasic();
    void MenCAfLfo1();
    void MenCAfLfo2();
    void MenPAfBasic();
    void MenPAfLfo1();
    void MenPAfLfo2();
    void MenCC1Basic();
    void MenCC1Lfo1();
    void MenCC1Lfo2();
    void MenCC2Basic();
    void MenCC2Lfo1();
    void MenCC2Lfo2();
    void MenPartRsrv();
    void MenPartMode();
    void MenEffects();
    void MenTiming();
    void MenMidiThru();
    void MenSynthSettings();
    void MenDrumParam();
    void MenClpCopy(Bool);
    void MenClpPaste();

    void SaveMidiDeviceSettings( int dev );
    void SaveThruSettings();
    void SaveTimingSettings();
    void SaveEffectSettings();
    void SaveMetronomeSettings();
    void SaveGeoSettings();
    void SaveSynthSettings();

    void MenSavePattern();
    void MenLoadPattern();

    void OnZoomIn();
    void OnZoomOut();
    void OnArpeggio();
    void OnAbout();
    void OnHelpMouse();
    void OnQuit();
    void OnLoad();
    void OnPlay();
    void OnPianowin();
    void OnEventList();
    void OnMapper();
    void OnSaveAs();
    void OnSave();
    void OnNew();
    void OnUndo();
    void OnRedo();
    void OnHarmony();

    void OnExportMidi();
    void OnExportSelMidi();
    void OnPreferences();
    void MenClpTrim();
    void MenSplit();
    void MenSelectionSub();

    void     OnSaveAll()  ;
    void     OnMetroOn()  ;
    void     OnRecord()   ;
    void     OnPlayLoop() ;
    void     OnReset();

    void MenClpCopyEraseSrc();
    void MenClpCopyLeaveSrc();
    void OnHelpJazz();

    void OnHelpTrackwin ();
    void OnFilter();
    void OnSettingsDialog();
    void OnShift();
    void OnDevice();
    void OnShuffle();
    void OnLoadTemplate();

    DECLARE_EVENT_TABLE()
};

extern tTrackWin *TrackWin;
extern char *lasts;

#endif

