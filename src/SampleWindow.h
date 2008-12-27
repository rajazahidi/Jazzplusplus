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

#ifndef samplwin_h
#define samplwin_h

#include "SampleCommand.h"

#include <wx/frame.h>

class JZRndArray;
class JZToolBar;
class tArrayEdit;
class tCommandPainter;
class tDistortion;
class tEqualizer;
class tSample;
class tSampleCnvs;
class tSynthDlg;
class wxDialog;
class wxScrollBar;

class tSampleWin : public wxFrame
{
  friend class tSampleCnvs;
  friend class tCommandPainter;
  friend class tSmplWinSettingsForm;
  public:
    tSampleWin(wxWindow* pParent, tSampleWin **ref, tSample &);
    ~tSampleWin();
    virtual void OnSize(int w, int h);
    virtual bool OnClose();
    virtual void OnMenuCommand(int id);
    void Redraw();
    bool HaveInsertionPoint(int &offs, bool warn = TRUE);
    enum HaveSelectionMode { SelWarn, SelNoWarn, SelAll} ;
    bool HaveSelection(int &fr_smpl, int &to_smpl, HaveSelectionMode = SelAll);

    void AddParam(JZRndArray *array, const char *label);
    void ClrParam();
    void ClearSelection();
    tSample &GetSample() {
      return spl;
    }
    void PlaySample();

  private:
    int GetPaintLength();
    int GetPaintOffset();
#ifdef OBSOLETE
    static void ScrollCallback(wxItem &itm, wxCommandEvent& event);
    void OnScroll(wxItem &item);
#endif
    void SetViewPos(int fr, int to);
    void LoadError(tSample &spl);

    tSample     &spl;
    tSampleCnvs *cnvs;
    wxPanel     *scrol_panel;
    wxScrollBar *pos_scrol;
    wxScrollBar *zoom_scrol;
    JZToolBar* mpToolBar;
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

