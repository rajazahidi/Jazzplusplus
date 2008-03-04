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

#ifndef sampldlg_h
#define sampldlg_h

#include "Random.h"
#include "SliderWindow.h"
#include "SampleCommand.h"
#include "PropertyListDialog.h"

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
    virtual void OnAccept(int fr, int to);
  protected:
    tSampleWin &win;
    tPaintableCommand &cmd;
};


class tEqualizer : public tSliderWin
{
  public:
    tEqualizer(tSampleWin &win);
    virtual ~tEqualizer();
    virtual void AddItems();
    virtual void AddEdits();
#ifdef OBSOLETE
    virtual void OnItem(wxItem& item, wxCommandEvent& event);
#endif
    double Index2Hertz(int index);
  private:
    void Action();
    JZRndArray array;
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
    tDistortion(tSampleWin &win);
    virtual void AddItems();
    virtual void AddEdits();
#ifdef OBSOLETE
    virtual void OnItem(wxItem& item, wxCommandEvent& event);
#endif
  private:
    void Action();
    void MakeCurve(int cvtype);
    void MakeExpo(int degree);
    void MakeSine(int degree);
    JZRndArray arr;
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
    tSynthDlg(tSampleWin &win);
    virtual ~tSynthDlg();
    virtual void AddItems();
    virtual void AddEdits();
#ifdef OBSOLETE
    virtual void OnItem(wxItem& item, wxCommandEvent& event);
#endif
    virtual void OnMenuCommand(int id);
    friend std::ostream& operator << (std::ostream& os, tSynthDlg const &a);
    friend std::istream& operator >> (std::istream& is, tSynthDlg &a);

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
    tReverbForm(tSampleWin &win);
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

    tSampleWin &win;
};


class tEchoForm : public tPropertyListDlg
{
  public:
    tEchoForm(tSampleWin &win);
    void EditForm(wxPanel *panel);
    void OnOk();
    void OnCancel();
    void OnHelp();
  private:
    static int num_echos;
    static int delay;
    static int ampl;
    static bool rand;
    tSampleWin &win;
};

class tShifterForm : public tPropertyListDlg
{
  public:
    tShifterForm(tSampleWin &win);
    void EditForm(wxPanel *panel);
    void OnOk();
    void OnCancel();
    void OnHelp();
  private:
    static int shift_semis;
    static int shift_frac;
    static bool keep_length;
    static int winsize;
    tSampleWin &win;
};


class tStretcherForm : public tPropertyListDlg
{
  public:
    tStretcherForm(tSampleWin &win);
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
    static bool keep_pitch;
    tSampleWin &win;
    tSample    &spl;
};


class tSplFilterForm : public tPropertyListDlg
{
  public:
    tSplFilterForm(tSampleWin &win, bool painter = FALSE);
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
    tSampleWin &win;
    wxList  typelist;
    char    *typestring;
    bool    painter;
};


class tWahWah;
class tWahSettingsForm : public tSplFilterForm
{
  public:
    tWahSettingsForm(tSampleWin &win, tWahWah &wah);
    void OnOk();
    void OnHelp();
  private:
    tWahWah &wah;
};

class tSplPitch;
class tSplPitchForm : public tPropertyListDlg
{
  public:
    tSplPitchForm(tSampleWin &win, tSplPitch &pitch_painter);
    void EditForm(wxPanel *panel);
    void OnOk();
    void OnCancel();
    void OnHelp();
  protected:
    static int range;
    tSampleWin &win;
    tSplPitch  &pitch;
};


class tChorusForm : public tPropertyListDlg
{
  public:
    tChorusForm(tSampleWin &win);
    void EditForm(wxPanel *panel);
    void OnOk();
    void OnCancel();
    void OnHelp();
  private:
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
    tStereoForm(tSampleWin &win);
    void EditForm(wxPanel *panel);
    void OnOk();
    void OnCancel();
    void OnHelp();
  private:
    tSampleWin &win;

    static int delay;
    static int stereo_spread;
};

#endif

#endif

