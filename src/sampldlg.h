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

#ifndef sampldlg_h
#define sampldlg_h

#include "wx/wx.h"


#include "random.h"
#include "slidrwin.h"
#include "samplcmd.h"

class tSampleWin;
class tSigEqualizer;
class tPaintableCommand;

/**
 * controls a tPaintableCommand, that is shows the parameter arrays
 * in samplwin and processes OnAction.
 */

class tCommandPainter
{
  public:
    tCommandPainter(tSampleWin &w, tPaintableCommand &cmd);
    virtual ~tCommandPainter();
    virtual void OnAccept(long fr, long to);
  protected:
    tSampleWin &win;
    tPaintableCommand &cmd;
};


class tEqualizer : public tSliderWin
{
  public:
    tEqualizer(tSampleWin &win, wxFrame **ref);
    virtual ~tEqualizer();
    virtual void AddItems();
    virtual void AddEdits();
#ifndef __PORTING
    virtual void OnItem(wxItem& item, wxCommandEvent& event);
#endif // __PORTING
    double Index2Hertz(int index);
  private:
    void Action();
    tRndArray array;
    tSplEqualizer **equ;
    tSampleWin &win;
    tSample    &spl;
    wxButton *action;
    wxButton *cancel;
    static int geo[4];
    int channels;
};

class tDistortion : public tSliderWin
{
  public:
    tDistortion(tSampleWin &win, wxFrame **ref);
    virtual void AddItems();
    virtual void AddEdits();
#ifndef __PORTING
    virtual void OnItem(wxItem& item, wxCommandEvent& event);
#endif // __PORTING
  private:
    void Action();
    void MakeCurve(int cvtype);
    void MakeExpo(int degree);
    void MakeSine(int degree);
    tRndArray arr;
    tSampleWin &win;
    wxButton *action;
    wxButton *cancel;
    wxChoice *curve;
    int N, ymin, ymax;  // array dimensions
    static int geo[4];
};

// ----------------------- additive synthesis ---------------------

class tAddSynth;
class tRhyArrayEdit;

class tSynthDlg : public tSliderWin
{
  public:
    tSynthDlg(tSampleWin &win, wxFrame **ref);
    virtual ~tSynthDlg();
    virtual void AddItems();
    virtual void AddEdits();
#ifndef __PORTING
    virtual void OnItem(wxItem& item, wxCommandEvent& event);
#endif // __PORTING
    virtual void OnMenuCommand(int id);
    friend ostream & operator << (ostream &os, tSynthDlg const &a);
    friend istream & operator >> (istream &is, tSynthDlg &a);

  private:

    void Action();
    void SetupEdits();

    tSampleWin &win;
    wxButton *action;
    wxButton *cancel;
    wxCheckBox *chk_vol;
    wxCheckBox *chk_pan;
    wxCheckBox *chk_frq;
    wxCheckBox *chk_fft;
    wxCheckBox *chk_noise;
    wxSlider   *num_synths_slider;
    wxSlider   *midi_key_slider;
    wxSlider   *duration_slider;

    enum { MAXSYNTHS = 6 };
    tAddSynth *synths[MAXSYNTHS];
    static int num_synths;
    static int midi_key;
    static int duration;
    static bool vol_enable;
    static bool pan_enable;
    static bool frq_enable;
    static bool fft_enable;
    static bool noise_enable;

    static int geo[4];
    char *default_filename;
};


// --------------------------- reverb ----------------------------


class tReverbForm : public tPropertyListDlg
{
  public:
    tReverbForm(tSampleWin &win, void **ref);
    void EditForm(wxPanel *panel);
    void OnOk();
    void OnCancel();
    void OnHelp();
  private:
    // space params
    static int roomsize;
    static int brightness;
    static int rvbtime;
    static int volume;

    void **ref;
    tSampleWin &win;
};


class tEchoForm : public tPropertyListDlg
{
  public:
    tEchoForm(tSampleWin &win, void **ref);
    void EditForm(wxPanel *panel);
    void OnOk();
    void OnCancel();
    void OnHelp();
  private:
    static int num_echos;
    static int delay;
    static int ampl;
    static Bool rand;
    void **ref;
    tSampleWin &win;
};

class tShifterForm : public tPropertyListDlg
{
  public:
    tShifterForm(tSampleWin &win, void **ref);
    void EditForm(wxPanel *panel);
    void OnOk();
    void OnCancel();
    void OnHelp();
  private:
    static int shift_semis;
    static int shift_frac;
    static Bool keep_length;
    static int winsize;
    void **ref;
    tSampleWin &win;
};


class tStretcherForm : public tPropertyListDlg
{
  public:
    tStretcherForm(tSampleWin &win, void **ref);
    void EditForm(wxPanel *panel);
    void OnOk();
    void OnCancel();
    void OnHelp();
  private:
    static int seconds;
    static int centies;
    static int winsize;
    static int oldspeed;
    static int newspeed;
    static Bool keep_pitch;
    void **ref;
    tSampleWin &win;
    tSample    &spl;
};


class tSplFilterForm : public tPropertyListDlg
{
  public:
    tSplFilterForm(tSampleWin &win, void **ref, Bool painter = FALSE);
    void EditForm(wxPanel *panel);
    void OnOk();
    void OnCancel();
    void OnHelp();
  protected:
    void ScanResults();
    static int order;
    static int type;
    static int freq;
    static int lo_freq;
    static int hi_freq;
    static int band_width;
    void **ref;
    tSampleWin &win;
    wxList  typelist;
    char    *typestring;
    Bool    painter;
};


class tWahWah;
class tWahSettingsForm : public tSplFilterForm
{
  public:
    tWahSettingsForm(tSampleWin &win, void **ref, tWahWah &wah);
    void OnOk();
    void OnHelp();
  private:
    tWahWah &wah;
};

class tSplPitch;
class tSplPitchForm : public tPropertyListDlg
{
  public:
    tSplPitchForm(tSampleWin &win, void **ref, tSplPitch &pitch_painter);
    void EditForm(wxPanel *panel);
    void OnOk();
    void OnCancel();
    void OnHelp();
  protected:
    static int range;
    void **ref;
    tSampleWin &win;
    tSplPitch  &pitch;
};


class tChorusForm : public tPropertyListDlg
{
  public:
    tChorusForm(tSampleWin &win, void **ref);
    void EditForm(wxPanel *panel);
    void OnOk();
    void OnCancel();
    void OnHelp();
  private:
    void **ref;
    tSampleWin &win;

    static int pitch_freq;
    static int pitch_range;
    static int pan_freq;
    static int pan_spread;
    static int volume;
};

#if 0
class tStereoForm : public tPropertyListDlg
{
  public:
    tStereoForm(tSampleWin &win, void **ref);
    void EditForm(wxPanel *panel);
    void OnOk();
    void OnCancel();
    void OnHelp();
  private:
    void **ref;
    tSampleWin &win;

    static int delay;
    static int stereo_spread;
};

#endif

#endif

