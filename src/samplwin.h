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

#ifndef samplwin_h
#define samplwin_h

#include "wx/wx.h"
#pragma hdrstop
//#include "wx_scrol.h"

class tSample;
class tSampleCnvs;
class wxToolBar;

class tArrayEdit;
class tRndArray;
class tCommandPainter;
class tEqualizer;
class tDistortion;
class tSynthDlg;

#include "samplcmd.h"


class tSampleWin : public wxFrame
{
  friend class tSampleCnvs;
  friend class tCommandPainter;
  friend class tSmplWinSettingsForm;
  public:
    tSampleWin(wxFrame *parent, tSampleWin **ref, tSample &);
    ~tSampleWin();
    virtual void OnSize(int w, int h);
    virtual bool OnClose();
    virtual void OnMenuCommand(int id);
    void Redraw();
    bool HaveInsertionPoint(long &offs, bool warn = TRUE);
    enum HaveSelectionMode { SelWarn, SelNoWarn, SelAll} ;
    bool HaveSelection(long &fr_smpl, long &to_smpl, HaveSelectionMode = SelAll);

    void AddParam(tRndArray *array, const char *label);
    void ClrParam();
    void ClearSelection();
    tSample &GetSample() {
      return spl;
    }
    void PlaySample();

  private:
    long GetPaintLength();
    long GetPaintOffset();
#ifndef __PORTING
    static void ScrollCallback(wxItem &itm, wxCommandEvent& event);
    void OnScroll(wxItem &item);
#endif // __PORTING
    void SetViewPos(long fr, long to);
    void LoadError(tSample &spl);

    tSample     &spl;
    tSampleCnvs *cnvs;
    wxPanel     *scrol_panel;
    wxScrollBar *pos_scrol;
    wxScrollBar *zoom_scrol;
    wxToolBar   *tool_bar;
    int         in_constructor;
    tSampleWin  **ref;
    static int geo[4];

    static tSample *copy_buffer;

    enum { MAXPARAM = 4 };
    tArrayEdit *params[MAXPARAM];
    int        num_params;

    tCommandPainter  *on_accept;
    tSplVolume vol_command;
    tSplPan    pan_command;
    tSplPitch  pitch_command;
    tWahWah    wah_command;

    tEqualizer *equalizer;
    tDistortion *distortion;
    tSynthDlg   *synth;
    wxDialog *reverb;
    wxDialog *echo;
    wxDialog *chorus;
    wxDialog *shifter;
    wxDialog *stretcher;
    wxDialog *filter;
    wxDialog *settings;
    wxDialog *wah_settings;
    wxDialog *pitch_settings;
};

#endif

