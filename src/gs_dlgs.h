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

#ifndef gs_dlgs_h
#define gs_dlgs_h

#include "trackwin.h"
#include "song.h"


#ifdef wx_motif
#define SLIDER_WIDTH 100
#define SLIDER_VERTICAL_SPACE 30
#define SLIDER_VERTICAL_OFFSET (-15)
#define XCOL_INTERVAL (slider_width)
#define XSTART_TEXT (xstart_sliders + 10)
#define XSTART_SLIDERS (xpos_label + 120)
#endif

#ifdef wx_xt
#define SLIDER_WIDTH 100
#define SLIDER_VERTICAL_SPACE 30
#define SLIDER_VERTICAL_OFFSET (-8)
#define XCOL_INTERVAL (slider_width)
#define XSTART_TEXT (xstart_sliders + 10)
#define XSTART_SLIDERS (xpos_label + 120)
#endif

#ifdef wx_msw
#define SLIDER_WIDTH 20 // ignored in mswin
#define SLIDER_VERTICAL_SPACE 25
#define SLIDER_VERTICAL_OFFSET (0)
#define XCOL_INTERVAL (200)
#define XSTART_TEXT (xstart_sliders + 100)
#define XSTART_SLIDERS (xpos_label + 100)
#endif

#ifdef wx_xview
#define SLIDER_WIDTH 50
#define SLIDER_VERTICAL_SPACE 25
#define SLIDER_VERTICAL_OFFSET (0)
#define XCOL_INTERVAL (110 + slider_width)
#define XSTART_TEXT (xstart_sliders + 10 + slider_width)
#define XSTART_SLIDERS (xpos_label + 120)
#endif

enum { 	tSliderDlgGeneric = 0,
	tSliderDlgMixer,
	tSliderDlgMaster,
	tSliderDlgSound,
	tSliderDlgVibrato,
	tSliderDlgEnvelope,
	tSliderDlgBendBasic,
	tSliderDlgBendLfo1,
	tSliderDlgBendLfo2,
	tSliderDlgModBasic,
	tSliderDlgModLfo1,
	tSliderDlgModLfo2,
	tSliderDlgCAfBasic,
	tSliderDlgCAfLfo1,
	tSliderDlgCAfLfo2,
	tSliderDlgPAfBasic,
	tSliderDlgPAfLfo1,
	tSliderDlgPAfLfo2,
	tSliderDlgCC1Basic,
	tSliderDlgCC1Lfo1,
	tSliderDlgCC1Lfo2,
	tSliderDlgCC2Basic,
	tSliderDlgCC2Lfo1,
	tSliderDlgCC2Lfo2,
	tSliderDlgPartRsrv,
	tSliderDlgPartMode,
	tSliderDlgEffects,
	tSliderDlgDrumParam,
	numSliderDialogs
};

struct tSliderArray {
        tTrackWin *tw;
        wxSlider *slider;
        tTrack *track;
};


// ******************************************************************
// SliderWin Dialog Class
// ******************************************************************

#define SliderWinMaxCol	4
#define SliderWinMaxRow 16

struct SliderWinNorm {
	int valnorm;
	int valdef;
	int valmin, valmax;
	void init( int vn, int vd, int vmin, int vmax ) {
		valnorm = vn; valdef = vd;
		valmin = vmin; valmax = vmax;
	}
};



class tGsSlider;
class tGsButton;

class tSliderDlg : public wxForm
{
 public:
  int NrColumns, NrRows;
  char *ColumnLabel[SliderWinMaxCol];
  SliderWinNorm norm[SliderWinMaxCol];
  virtual void EditForm(wxPanel *panel);
  virtual int GetParameter( tTrack *track, int type ) = 0;
  virtual void SetParameter( tTrack *track, int type, int value ) = 0;
  virtual void CloseWindow();

  tEventWin *EventWin;
  tSliderDlg(tEventWin *w, int id);
  int classId;
  char *helpKeyword;

  // Currently only used by tMixerDlg
  static tGsSlider *SliderArray[SliderWinMaxRow][SliderWinMaxCol];

  static void SliderChange( tGsSlider& slider, wxCommandEvent& event );
  static void Dismiss( tGsButton& button, wxCommandEvent& event );
  static void Help( tGsButton& button, wxCommandEvent& event );
};

class tMixerDlg : public tSliderDlg
{
 public:
  tMixerDlg(tEventWin *w);
  virtual int GetParameter( tTrack *track, int type );
  virtual void SetParameter( tTrack *track, int type, int value );
  virtual void CloseWindow();
  static void SetSliderVal( int chan, int type, int val );
};

class tGsSlider : public wxSlider
{
  public:
    tGsSlider(	tSliderDlg *dlg, tTrack *t, SliderWinNorm &n, int tp,
		wxPanel *panel, wxFunction func, char *label, int value,
		int min_value, int max_value, int width, int x = -1, int y = -1,
		long style = wxHORIZONTAL, char *name = "slider" )
       : wxSlider(panel, func, label, value, min_value, max_value, width, x, y,
		style, name )
    {
      dialog = dlg;
      track = t;
      norm  = n;
      type = tp;
    }

    void OnChange()
    {
      dialog->SetParameter( track, type, GetValue() + norm.valnorm );
    }

    void SetNormValue( int val ) {
	SetValue( val - norm.valnorm );
    }

  private:
    tSliderDlg *dialog;
    tTrack *track;
    SliderWinNorm norm;
    int type;
};


class tGsButton : public wxButton {
  public:
    tGsButton( 	tSliderDlg *dlg,
		wxPanel *panel, wxFunction func, char *label, int x = -1, int y = -1,
		int width = -1, int height = -1, long style = 0, char *name = "button")
	: wxButton( panel, func, label, x, y, width, height, style, name )
    {
	dialog = dlg;
    }

    void GsOnDismiss();
    void GsOnHelp();
    tSliderDlg *getDialog() { return dialog; }

  private:
    tSliderDlg *dialog;

};

class tGsListBox : public wxListBox
{
  public:
    tGsListBox(	tSliderDlg *dlg, tTrack *t, SliderWinNorm &n, int tp, int sel,
		wxPanel *panel, wxFunction func, char *label,
		Bool Multiple = wxSINGLE,
		int x = -1, int y = -1, int width = -1, int height = -1,
		int N = 0, char **Choices = NULL,
		long style = 0, char *name = "listBox" )
       : wxListBox( panel, func, label, Multiple, x, y, width, height,
		N, Choices, style, name )
    {
      dialog = dlg;
      track = t;
      norm  = n;
      type = tp;
      SetSelection( sel );
    }

    void OnSelect()
    {
      dialog->SetParameter( track, type, GetSelection() + norm.valnorm );
    }

  private:
    tSliderDlg *dialog;
    tTrack *track;
    SliderWinNorm norm;
    int type;
};

class tGsCheckBox : public wxCheckBox
{
  public:
    tGsCheckBox(tSliderDlg *dlg, tTrack *t, SliderWinNorm &n, int tp, int var,
		wxPanel *panel, wxFunction func, char *label,
		int x = -1, int y = -1, int width = -1, int height = -1,
		long style = 0, char *name = "listBox" )
       : wxCheckBox( panel, func, label, x, y, width, height,
		style, name )
    {
      dialog = dlg;
      track = t;
      norm  = n;
      type = tp;
      SetValue( var );
    }

    void OnCheck()
    {
      dialog->SetParameter( track, type, GetValue() + norm.valnorm );
    }

  private:
    tSliderDlg *dialog;
    tTrack *track;
    SliderWinNorm norm;
    int type;
};

#endif
