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

#include "WxWidgets.h"

//#include "config.h"
#include "SampleDialog.h"
#include "Sample.h"
#include "SampleWindow.h"
//#include "jazz.h"
#include "Mapper.h"
#include "Audio.h"
#include "SignalInterface.h"
#include "Song.h"   // Speed()
#include "ToolBar.h"
#include "FileSelector.h"
#include "DeprecatedStringUtils.h"
#include "Help.h"

#include <fstream>
#include <cmath>

using namespace std;

tCommandPainter::tCommandPainter(tSampleWin &w, tPaintableCommand &c)
  : win(w), cmd(c)
{
  c.Initialize();
  win.ClearSelection();
  int n = cmd.NumArrays();
  for (int i = 0; i < n; ++i)
  {
    win.AddParam(&cmd.GetArray(i), cmd.GetLabel(i));
  }
}

tCommandPainter::~tCommandPainter()
{
  win.ClrParam();
}

void tCommandPainter::OnAccept(int fr, int to)
{
  wxBeginBusyCursor();
  cmd.Execute(fr, to);
  wxEndBusyCursor();
}

// ------------------------------ Equalizer --------------------------------


class tEquArrayEdit : public tRhyArrayEdit
{
  public:

    tEquArrayEdit(tEqualizer *parent, JZRndArray &arr, int style)
      : tRhyArrayEdit(parent, arr, 10, 10, 10, 10, style),
        equ(*parent)
    {
    }

    virtual const char *GetXText(int val);

  private:

    tEqualizer &equ;
};

const char* tEquArrayEdit::GetXText(int val)
{
  if (val == 0)
  {
    return "KHz";
  }

  static char buf[20];
  double f = equ.Index2Hertz(val);
  sprintf(buf, "%3.1f", f/1000.0);
  return buf;
}

int tEqualizer::geo[4] = { 50, 80, 350, 200 };

tEqualizer::tEqualizer(tSampleWin &w)
  : tSliderWin(&w, "Equalizer", geo),
    array(12, -100, 100),
    win(w),
    spl(w.GetSample())
{
  int i;
  channels = spl.GetChannels();
  for (i = 0; i < array.Size(); ++i)
  {
    array[i] = 0;
  }
  equ = new tSplEqualizer * [channels];
  for (i = 0; i < channels; ++i)
  {
    equ[i] = new tSplEqualizer(array, spl.GetSamplingRate());
  }
  Initialize();
}

tEqualizer::~tEqualizer()
{
  for (int i = 0; i < channels; i++)
    delete equ[i];
  delete [] equ;
}

void tEqualizer::AddItems()
{
  action = new wxButton(panel,  -1, "Ok") ;     //(wxFunction)ItemCallback,
  cancel = new wxButton(panel,  -1, "Cancel") ; //(wxFunction)ItemCallback,
}

void tEqualizer::AddEdits()
{
  n_sliders = 1;
  sliders[0] = new tEquArrayEdit(this, array, (ARED_GAP | ARED_XTICKS));
}

#ifdef OBSOLETE
void tEqualizer::OnItem(wxItem& item, wxCommandEvent& event)
{
  if (&item == action)
  {
    wxBeginBusyCursor();
    Action();
    wxEndBusyCursor();
  }
  DELETE_THIS();
}
#endif

double tEqualizer::Index2Hertz(int index)
{
  return equ[0]->Index2Hertz(index);
}

void tEqualizer::Action()
{
  int fr, to;
  if (!win.HaveSelection(fr, to))
    return;


  tFloatSample fs(spl, fr, to);
  float oldpeak = fs.Peak();

  int i = 0;
  int channels = fs.GetChannels();
  int n = to - fr;
  float *data = fs.GetData();

  for (i = 0; i < channels; i++)
    equ[i]->Prepare();

  for (i = 0; i < n; i += channels)
  {
    for (int c = 0; c < channels; c++)
      data[i + c] = equ[c]->operator()(data[i + c]);
  }
  fs.Rescale(oldpeak);
  spl.SetSmooth(fs, fr);
  win.Redraw();
}

// -------------------------------------------------------------------------
//                             Distortion
// -------------------------------------------------------------------------

#define CV_LINEAR 0
#define CV_EXPO1  1
#define CV_EXPO2  2
#define CV_EXPO3  3
#define CV_EXPO4  4
#define CV_SINE1  5
#define CV_SINE2  6

static char *cv_strings[] = { "Linear", "Expo 1", "Expo 2", "Expo 3", "Expo 4", "Sine 1", "Sine 2", 0 };

int tDistortion::geo[4] = { 50, 80, 300, 320 };

tDistortion::tDistortion(tSampleWin &w)
: tSliderWin(&w, "Distortion", geo),
  arr(200, 0, 100),
  win(w)
{
  N = 200;
  ymin = 0;
  ymax = 100;
  MakeCurve(CV_LINEAR);
  Initialize();
}



void tDistortion::AddItems()
{
  action = new wxButton(panel, wxID_ANY, "Ok");      //(wxFunction)ItemCallback,
  cancel = new wxButton(panel, wxID_ANY, "Cancel");  //(wxFunction)ItemCallback,
  curve  = new wxChoice(panel, wxID_ANY);//, "Presets"); //(wxFunction)ItemCallback,
  char **s = cv_strings;
  while (*s)
    curve->Append(*s++);
}

void tDistortion::AddEdits()
{
  n_sliders = 1;
  sliders[0] = new tRhyArrayEdit(this, arr, 10,10,10,10, ARED_LINES);
}

#ifdef OBSOLETE
void tDistortion::OnItem(wxItem& item, wxCommandEvent& event)
{
  if (&item == action)
  {
    Action();
    DELETE_THIS();
  }
  else if (&item == curve)
  {
    MakeCurve(((wxChoice &)item).GetSelection());
    sliders[0]->OnPaint();
  }
  else if (&item == cancel)
    DELETE_THIS();
}
#endif

void tDistortion::MakeCurve(int cvtype)
{
  switch (cvtype)
  {
    case CV_LINEAR:
      {
        JZMapper map(0, N, ymin, ymax);
        for (int i = 0; i < N; i++)
          arr[i] = (int)map.XToY(i);
      }
      break;

    case CV_SINE1: MakeSine(1); break;
    case CV_SINE2: MakeSine(2); break;

    case CV_EXPO1: MakeExpo(1); break;
    case CV_EXPO2: MakeExpo(2); break;
    case CV_EXPO3: MakeExpo(4); break;
    case CV_EXPO4: MakeExpo(6); break;

  }
}

void tDistortion::MakeExpo(int degree)
{
  JZMapper xmap(0, N, 0, -degree);
  JZMapper ymap(
    exp(static_cast<double>(0)),
    exp(static_cast<double>(-degree)),
    ymin,
    ymax);
  for (int i = 0; i < N; i++)
  {
    arr[i] = (int)(ymap.XToY(exp(static_cast<double>(xmap.XToY(i)))));
  }
}

#ifdef FOR_MSW
#define PI M_PI
#endif

void tDistortion::MakeSine(int degree)
{
  double x0 = -PI/2;
  double x1 = x0 + 2 * degree * PI;
  JZMapper xmap(0, N, x0, x1);
  JZMapper ymap(-1, 1, ymin, ymax);
  for (int i = 0; i < N; i++)
    arr[i] = (int)(ymap.XToY(sin(xmap.XToY(i))));
}

void tDistortion::Action()
{
  int fr, to;
  if (!win.HaveSelection(fr, to))
    return;

  wxBeginBusyCursor();
  tSample &spl = win.GetSample();
  short *data = spl.GetData();
  JZMapper xmap(0, 32767, 0, 100);
  JZMapper ymap(0, 100, 0, 32767);
  for (int i = fr; i < to; i++)
  {
    short   x = data[i];
    if (x > 0)
    {
      double fx = xmap.XToY(x);
      double fy = ymap.XToY(arr[(float)fx]);
      data[i] = (short)fy;
    }
    else {
      x = -x;
      double fx = xmap.XToY(x);
      double fy = ymap.XToY(arr[(float)fx]);
      data[i] = -(short)fy;
    }
  }
  wxEndBusyCursor();
  win.Redraw();
}

// -------------------------------------------------------------------------
//                             Additive Synthesis
// -------------------------------------------------------------------------

#define N_HARM 40


class tAddSynthArray {
  public:
    JZRndArray arr;
    tRhyArrayEdit *edit;
    tAddSynthArray(wxFrame *parent, const char *label, int n, int ynul, int ymax, int style)
      : arr(n, 0, ymax)
    {
      int i;
      arr.SetNull(ynul);
      for (i = 0; i < n; i++)
        arr[i] = ynul;
      edit = new tRhyArrayEdit(parent, arr, 10,10,10,10, style);
      edit->SetLabel(label);
    }
    void Show(bool x) { edit->Show(x); }
    void Init()       { edit->Init();  }
};

class tAddSynth
{
  public:
    tAddSynthArray fft;
    tAddSynthArray vol;
    tAddSynthArray frq;
    tAddSynthArray pan;

    tAddSynth(wxFrame *parent) :
      fft(parent, "Harmonics",   N_HARM, 0, 100, ARED_XTICKS | ARED_GAP),
      vol(parent, "Envelope",    100,  0, 100, ARED_XTICKS | ARED_LINES),
      frq(parent, "Pitch",       100, 50, 100, ARED_XTICKS | ARED_LINES),
      pan(parent, "Panpot",      100, 50, 100, ARED_XTICKS | ARED_LINES)
    {
      fft.edit->SetXMinMax(1, N_HARM);
    }

    void SetDuration(int durat) {
      vol.edit->SetXMinMax(0, durat);
      pan.edit->SetXMinMax(0, durat);
      frq.edit->SetXMinMax(0, durat);
    }

    void Refresh() {
      fft.edit->Refresh();
      vol.edit->Refresh();
      frq.edit->Refresh();
      pan.edit->Refresh();
    }
    void Init() {
      fft.edit->Init();
      vol.edit->Init();
      frq.edit->Init();
      pan.edit->Init();
    }
    friend std::ostream& operator << (std::ostream& os, tAddSynth const &a);
    friend std::istream& operator >> (std::istream& is, tAddSynth &a);
};

std::ostream& operator << (std::ostream& os, tAddSynth const &a)
{
  os << a.fft.arr << endl;
  os << a.vol.arr << endl;
  os << a.frq.arr << endl;
  os << a.pan.arr << endl;
  return os;
}

std::istream& operator >> (std::istream& is, tAddSynth &a)
{
  is >> a.fft.arr;
  is >> a.vol.arr;
  is >> a.frq.arr;
  is >> a.pan.arr;
  a.Init();
  return is;
}


int  tSynthDlg::geo[4] = { 50, 80, 800, 400 };
int  tSynthDlg::num_synths = 1;
int  tSynthDlg::midi_key   = 30;
int  tSynthDlg::duration   = 50;

bool tSynthDlg::fft_enable = 1;
bool tSynthDlg::vol_enable = 1;
bool tSynthDlg::pan_enable = 0;
bool tSynthDlg::frq_enable = 0;
bool tSynthDlg::noise_enable = 1;

static const int SYN_LOAD = 1;
static const int SYN_SAVE = 2;
static const int SYN_CLOSE = 3;
static const int SYN_GEN  = 4;
static const int SYN_HELP = 5;
static const int SYN_PLAY = 6;


#include "Bitmaps/open.xpm"
#include "Bitmaps/save.xpm"
#include "Bitmaps/rrggen.xpm"
#include "Bitmaps/help.xpm"
#include "Bitmaps/play.xpm"

static JZToolDef syn_tdefs[] = {
  { SYN_LOAD, FALSE, open_xpm,   "open synth settings" },
  { SYN_SAVE, FALSE, save_xpm,   "save synth settings" },
  { JZToolBar::eToolBarSeparator },
  { SYN_GEN,  FALSE, rrggen_xpm, "generate sound" },
  { SYN_PLAY, FALSE, play_xpm,   "play sound" },
  { JZToolBar::eToolBarSeparator },
  { SYN_HELP, FALSE, help_xpm,   "help" },
  { JZToolBar::eToolBarEnd }
};


tSynthDlg::tSynthDlg(tSampleWin &w)
: tSliderWin(&w, "Additive Synthesis", geo, syn_tdefs),
  win(w)
{
  Initialize();
  default_filename = copystring("noname.syn");
}

tSynthDlg::~tSynthDlg()
{
  int i;
  for (i = 0; i < MAXSYNTHS; i++)
    delete synths[i];
  delete [] default_filename;
}

ostream & operator << (ostream &os, tSynthDlg const &a)
{
  int i;
  os << 1000 << endl;
  os << (int)a.num_synths << " ";
  os << (int)a.midi_key   << " ";
  os << (int)a.duration   << " ";
  os << (int)a.vol_enable << " ";
  os << (int)a.pan_enable << " ";
  os << (int)a.frq_enable << " ";
  os << (int)a.fft_enable << " ";
  os << (int)a.noise_enable << " ";
  os << endl;
  for (i = 0; i < a.num_synths; i++)
    os << *a.synths[i] << endl;
  return os;
}


istream & operator >> (istream &is, tSynthDlg &a)
{
  int i, version;
  is >> version;
  if (version != 1000) {
    wxMessageBox("Wrong file format!", "Error", wxOK);
    return is;
  }
  is >> i; a.num_synths = i;
  is >> i; a.midi_key = i;
  is >> i; a.duration = i;
  is >> i; a.vol_enable = i;
  is >> i; a.pan_enable = i;
  is >> i; a.frq_enable = i;
  is >> i; a.fft_enable = i;
  is >> i; a.noise_enable = i;

  for (i = 0; i < a.num_synths; ++i)
  {
    is >> *a.synths[i];
  }

  a.num_synths_slider->SetValue(a.num_synths);
  a.midi_key_slider->SetValue(a.midi_key);
  a.duration_slider->SetValue(a.duration);
  a.chk_vol->SetValue(a.vol_enable);
  a.chk_pan->SetValue(a.pan_enable);
  a.chk_frq->SetValue(a.frq_enable);
  a.chk_fft->SetValue(a.fft_enable);
  a.chk_noise->SetValue(a.noise_enable);

  return is;
}


void tSynthDlg::OnMenuCommand(int id)
{
  switch(id)
  {
    case SYN_LOAD:
      {
        wxString fname = file_selector(default_filename, "Load Synth", 0, 0, "*.syn");
        if (fname)
        {
          ifstream is(fname);
          is >> *this;
          SetupEdits();
          int cw, ch;
          GetClientSize(&cw, &ch);
          //          OnSize(cw, ch);
          //          OnPaint();
          Refresh();
        }
      }
      break;

    case SYN_SAVE:
      {
        wxString fname = file_selector(default_filename, "Save Synth", 1, 0, "*.syn");
        if (fname) {
          ofstream os(fname);
          os << *this;
        }
      }
      break;

    case SYN_GEN:
      wxBeginBusyCursor();
      Action();
      wxEndBusyCursor();
      win.Redraw();
      break;

    case SYN_PLAY:
      win.PlaySample();
      break;

    case SYN_CLOSE:
//PJS      DELETE_THIS();
      Destroy();
      break;

    case SYN_HELP:
      gpHelpInstance->ShowTopic("{Additive Synthesis");
    break;
  }
}

void tSynthDlg::AddItems()
{
  //action = new wxButton(panel, (wxFunction)ItemCallback, "Synth");
  //cancel = new wxButton(panel, (wxFunction)ItemCallback, "Close");
  //panel->NewLine();
  chk_fft = new wxCheckBox(panel, -1, "Harmonics"); //    (wxFunction)ItemCallback,
  chk_fft->SetValue(fft_enable);                                                    
  chk_vol = new wxCheckBox(panel, -1, "Envelope");  //          (wxFunction)ItemCallback,
  chk_vol->SetValue(vol_enable);                                                    
  chk_pan = new wxCheckBox(panel, -1, "Panpot");    //          (wxFunction)ItemCallback,
  chk_pan->SetValue(pan_enable);                                                    
  chk_frq = new wxCheckBox(panel, -1, "Pitch");     //          (wxFunction)ItemCallback,
  chk_frq->SetValue(frq_enable);                                                    
  chk_noise = new wxCheckBox(panel, -1, "Noise");     //  (wxFunction)ItemCallback,
  chk_noise->SetValue(noise_enable);
#ifdef OBSOLETE
  panel->NewLine();
#endif
  num_synths_slider = new wxSlider(panel, wxID_ANY, num_synths, 1, (int)MAXSYNTHS);
  new wxStaticBox(panel, wxID_ANY, "Number of Synths");
#ifdef OBSOLETE
  panel->NewLine();
#endif
  midi_key_slider   = new wxSlider(panel, wxID_ANY, midi_key, 1, 127);
  new wxStaticBox(panel, wxID_ANY, "Midi Key");
#ifdef OBSOLETE
  panel->NewLine();
#endif
  duration_slider   = new wxSlider(panel, wxID_ANY, duration, 1, 100);
  new wxStaticBox(panel, wxID_ANY, "Duration in 1/10 seconds");
  // und: analyze -> Voreinstellungen der synths
  // und: klar, toolbar
}


void tSynthDlg::AddEdits()
{
  int i, k;
  for (i = 0, k = 0; i < MAXSYNTHS; i++) {
    synths[i] = new tAddSynth(this);
    synths[i]->SetDuration(duration);
  }
  SetupEdits();
}

void tSynthDlg::SetupEdits()
{
  int i, k;

  sliders_per_row = 0;
  if (fft_enable) sliders_per_row ++;
  if (vol_enable) sliders_per_row ++;
  if (pan_enable) sliders_per_row ++;
  if (frq_enable) sliders_per_row ++;
  if (sliders_per_row == 0) {
    sliders_per_row = 1;
    fft_enable = 1;
    chk_fft->SetValue(TRUE);
  }

  n_sliders = num_synths * sliders_per_row;

  for (i = 0, k = 0; i < num_synths; i++) {
    tAddSynth &s = *synths[i];
    if (fft_enable) {
      sliders[k++] = s.fft.edit;
      s.fft.Show(TRUE);
    }
    else
      s.fft.Show(FALSE);

    if (vol_enable) {
      sliders[k++] = s.vol.edit;
      s.vol.Show(TRUE);
    }
    else
      s.vol.Show(FALSE);

    if (pan_enable) {
      sliders[k++] = s.pan.edit;
      s.pan.Show(TRUE);
    }
    else
      s.pan.Show(FALSE);

    if (frq_enable) {
      sliders[k++] = s.frq.edit;
      s.frq.Show(TRUE);
    }
    else
      s.frq.Show(FALSE);

  }

  for (; i < MAXSYNTHS; i++) {
    tAddSynth &s = *synths[i];
    s.fft.Show(FALSE);
    s.vol.Show(FALSE);
    s.pan.Show(FALSE);
    s.frq.Show(FALSE);
  }

  if (noise_enable) {
    synths[0]->fft.edit->SetLabel("noise filter");
    synths[0]->frq.edit->Enable(FALSE);
  }
  else {
    synths[0]->fft.edit->SetLabel("harmonics");
    synths[0]->frq.edit->Enable(TRUE);
  }
}


#ifdef OBSOLETE
void tSynthDlg::OnItem(wxItem& item, wxCommandEvent& event)
{
  int redraw = 0;
  int resize = 0;

#if 0
  if (&item == action) {
    wxBeginBusyCursor();
    Action();
    wxEndBusyCursor();
    win.Redraw();
  }
  else if (&item == cancel)
    DELETE_THIS();
  else
#endif

  if (&item == chk_vol) {
    vol_enable = chk_vol->GetValue();
    resize = 1;
  }
  else if (&item == chk_pan) {
    pan_enable = chk_pan->GetValue();
    resize = 1;
  }
  else if (&item == chk_frq) {
    frq_enable = chk_frq->GetValue();
    resize = 1;
  }
  else if (&item == chk_fft) {
    fft_enable = chk_fft->GetValue();
    resize = 1;
  }
  else if (&item == chk_noise) {
    noise_enable = chk_noise->GetValue();
    resize = 1;
  }
  else if (&item == num_synths_slider) {
    int n = num_synths_slider->GetValue();
    if (n != num_synths) { // avoid flashing
      num_synths = n;
      resize = 1;
    }
  }
  else if (&item == duration_slider) {
    duration = duration_slider->GetValue();
    for (int i = 0, k = 0; i < num_synths; i++) {
      tAddSynth &s = *synths[i];
      s.SetDuration(duration);
    }
    redraw = 1;
  }
  else if (&item == midi_key_slider)
    midi_key = midi_key_slider->GetValue();

  if (resize) {
    SetupEdits();
    int cw, ch;
    GetClientSize(&cw, &ch);
    OnSize(cw, ch);
  }

  else if (redraw) {
    for (int i = 0; i < num_synths; i++)
      synths[i]->Refresh();
  }
}
#endif

void tSynthDlg::Action()
{
  int i;
  tSample &spl = win.GetSample();
  JZRndArray *arr[MAXSYNTHS][4];
  for (i = 0; i < MAXSYNTHS; i++) {
    arr[i][0] = &synths[i]->fft.arr;
    arr[i][1] = &synths[i]->vol.arr;
    arr[i][2] = &synths[i]->frq.arr;
    arr[i][3] = &synths[i]->pan.arr;
  }
  double dur = (double)duration / 10.0;
  sig_wavsynth(spl, dur, midi_key, FSEMI*FSEMI, num_synths, arr, noise_enable);
}

// -------------------------------------------------------------------------
//                             Reverb
// -------------------------------------------------------------------------
#ifdef OBSOLETE

// space params
int tReverbForm::roomsize   = 50;  // echo density
int tReverbForm::brightness = 20;  // lowpass freq
int tReverbForm::volume     = 20;  // effect volume
int tReverbForm::rvbtime    = 30;  // echo absorbtion

tReverbForm::tReverbForm(tSampleWin &w)
  : wxForm( USED_WXFORM_BUTTONS ),
    win(w)
{
}


void tReverbForm::OnHelp()
{
  gpHelpInstance->ShowTopic("Reverb");
}


void tReverbForm::EditForm(wxPanel *panel)
{
  Add(wxMakeFormShort(" ",   &roomsize, wxFORM_DEFAULT, new wxList(wxMakeConstraintRange(1.0, 100.0), 0)));
  Add(wxMakeFormMessage("Room Size"));
  Add(wxMakeFormNewLine());
  Add(wxMakeFormShort(" ",  &brightness, wxFORM_DEFAULT, new wxList(wxMakeConstraintRange(1.0, 100.0), 0)));
  Add(wxMakeFormMessage("Brightness"));
  Add(wxMakeFormNewLine());
  Add(wxMakeFormShort(" ", &rvbtime,   wxFORM_DEFAULT, new wxList(wxMakeConstraintRange(0.1, 100.0), 0)));
  Add(wxMakeFormMessage("Length"));
  Add(wxMakeFormNewLine());
  Add(wxMakeFormShort(" ", &volume,   wxFORM_DEFAULT, new wxList(wxMakeConstraintRange(1.0, 100.0), 0)));
  Add(wxMakeFormMessage("Effect Volume"));
  Add(wxMakeFormNewLine());

  AssociatePanel(panel);
}

void tReverbForm::OnCancel()
{
  wxForm::OnCancel();
}


void tReverbForm::OnOk()
{
  tSample &spl = win.GetSample();
  wxBeginBusyCursor();

  float sr = spl.GetSamplingRate();
  JZMapper room_map(1, 100, 0.8, 2.0);
  float room_val = room_map(roomsize);
  JZMapper bright_map(1, 100, 500, sr/4);
  float bright_val = bright_map(brightness);
  JZMapper rvbtime_map(1, 100, 0.2, 5.0);
  float rvbtime_val = rvbtime_map(rvbtime);
  JZMapper volume_map(1, 100, 0, 1);
  float volume_val = volume_map(volume);
  sig_reverb(spl, rvbtime_val, bright_val, volume_val, room_val);

  wxEndBusyCursor();
  win.Redraw();

  wxForm::OnOk();
}



// -------------------------------------------------------------------------
//                             Echo
// -------------------------------------------------------------------------


int tEchoForm::num_echos   = 3;
int tEchoForm::delay       = 50;  // millisec
int tEchoForm::ampl        = 25;  // percent
bool tEchoForm::rand       = FALSE;


tEchoForm::tEchoForm(tSampleWin &w)
: wxForm( USED_WXFORM_BUTTONS ),
  win(w)
{
  wxForm::OnCancel();
}

void tEchoForm::OnOk()
{
  tSample &spl = win.GetSample();
  wxBeginBusyCursor();
  JZMapper dmap(0, 100, 0, spl.Seconds2Samples(1));
  JZMapper amap(0, 100, 0.05, 1);
  tFloatSample fs(spl);
  float peak = fs.Peak();
  if (rand)
  {
    if (fs.GetChannels() == 2)
      fs.RndEchoStereo(num_echos, dmap(delay), (float)amap(ampl));
    else
      fs.RndEcho(num_echos, dmap(delay), (float)amap(ampl));
  }
  else
    fs.Echo(num_echos, dmap(delay), (float)amap(ampl));
  fs.Rescale(peak);
  fs.RemoveTrailingSilence(10);
  spl.Set(fs);
  win.Redraw();
  wxEndBusyCursor();
  wxForm::OnOk();
}



// -------------------------------------------------------------------------
//                             shifter
// -------------------------------------------------------------------------


int tShifterForm::winsize      = 10;  // 0.1 .. 0.2 ??
int tShifterForm::shift_semis  = 0;
int tShifterForm::shift_frac   = 0;
bool tShifterForm::keep_length = 1;


tShifterForm::tShifterForm(tSampleWin &w)
: wxForm( USED_WXFORM_BUTTONS ),
  win(w)
{
}


void tShifterForm::OnHelp()
{
  gpHelpInstance->ShowTopic("Pitch shifter");
}


void tShifterForm::EditForm(wxPanel *panel)
{
  Add(wxMakeFormMessage("Changes pitch of the sample"));
  Add(wxMakeFormNewLine());


  Add(wxMakeFormShort(" ",  &shift_semis, wxFORM_DEFAULT, new wxList(wxMakeConstraintRange(-12.0, 12.0), 0)));
  Add(wxMakeFormMessage("Semitones"));
  Add(wxMakeFormNewLine());
  Add(wxMakeFormShort(" ",  &shift_frac, wxFORM_DEFAULT, new wxList(wxMakeConstraintRange(-100.0, 100.0), 0)));
  Add(wxMakeFormMessage("Semitones/100"));
  Add(wxMakeFormNewLine());

  Add(wxMakeFormBool("Keep length", &keep_length));
  Add(wxMakeFormNewLine());
  Add(wxMakeFormShort(" ",   &winsize, wxFORM_DEFAULT, new wxList(wxMakeConstraintRange(0.0, 100.0), 0)));
  Add(wxMakeFormMessage("Window Size"));
  Add(wxMakeFormNewLine());

  AssociatePanel(panel);
}

void tShifterForm::OnCancel()
{
  wxForm::OnCancel();
}

void tShifterForm::OnOk()
{
  tSample &spl = win.GetSample();
  wxBeginBusyCursor();
#if 1
  tShifterCmd shifter(spl.GetSamplingRate());
  float semis = (float)shift_semis + (float)shift_frac/100.0;
  shifter.ShiftPitch(spl, semis,  keep_length, winsize);

#else
  if (shift_semis != 0 || shift_frac != 0)
  {
    JZMapper wmap(0, 100, 0.05, 0.3);
    tFloatSample inp(spl);
    tFloatSample out(inp.GetChannels(), inp.GetSamplingRate());
    //float peak = inp.Peak();

    tShifterCmd shifter(inp, out);

    float p[8];
    p[0] = 0;
    p[1] = 0;
    p[2] = inp.Samples2Seconds(inp.GetLength());
    p[3] = 1.0;
    p[4] = (float)shift_semis / 100.0 + (float)shift_frac/10000.0;
    p[5] = (float)wmap(winsize);
    p[6] = 0;
    p[7] = 0;
    shifter.rotate(p, 8);
    if (inp.GetChannels() == 2)
    {
      p[6] = 1;
      p[7] = 1;
      shifter.rotate(p, 8);
    }
    out.ClipToCurrent();
    //out.Rescale(peak);
    spl.Set(out);
  }

  if (trans_semis != 0 || trans_frac != 0)
    spl.TransposeSemis(trans_semis + (float)trans_frac/100.0);
#endif

  win.Redraw();
  wxEndBusyCursor();
  wxForm::OnOk();
}

// -------------------------------------------------------------------------
//                             filter
// -------------------------------------------------------------------------


int tSplFilterForm::type                = 0;
int tSplFilterForm::order                 = 2;
int tSplFilterForm::freq                = 1000;
int tSplFilterForm::lo_freq                = 400;
int tSplFilterForm::hi_freq                = 2000;
int tSplFilterForm::band_width          = 20;  // in % of corner freq
static const char *filter_types[] = { "Low Pass", "High Pass", "Band Pass", "Band Stop", 0 };


tSplFilterForm::tSplFilterForm(tSampleWin &w, bool p)
: wxForm( USED_WXFORM_BUTTONS ),
  win(w)
{
  painter = p;
}


void tSplFilterForm::OnHelp()
{
  gpHelpInstance->ShowTopic("Filter");
}


void tSplFilterForm::EditForm(wxPanel *panel)
{
  double maxfreq = (int)(win.GetSample().GetSamplingRate() / 2);
  // watch order of enum in signali.h
  typestring = 0;
  for (int i = 0; filter_types[i]; i++) {
    typelist.Append((wxObject *)filter_types[i]);  // ???
    if (i == (int)type)
      typestring = copystring(filter_types[i]);
  }
  if (!typestring)
    typestring = copystring(filter_types[0]);

  //Add(wxMakeFormString("Filter Type", (char **)&typestring, wxFORM_CHOICE,
  Add(wxMakeFormString("Filter Type", (char **)&typestring, wxFORM_DEFAULT,
      new wxList(wxMakeConstraintStrings(&typelist), 0), NULL, wxHORIZONTAL));
  Add(wxMakeFormNewLine());
  if (painter)
  {
    Add(wxMakeFormShort("freq lo",  &lo_freq, wxFORM_DEFAULT, 0));
    Add(wxMakeFormNewLine());
    Add(wxMakeFormShort("freq hi",  &hi_freq, wxFORM_DEFAULT, 0));
    Add(wxMakeFormNewLine());
  }
  else
  {
    Add(wxMakeFormShort("Corner Frequency",  &freq, wxFORM_DEFAULT, 0));
    Add(wxMakeFormNewLine());
    //Add(wxMakeFormShort(" ", &order, wxFORM_DEFAULT, new wxList(wxMakeConstraintRange(1.0, 5.0), 0), NULL, wxVERTICAL));
    //Add(wxMakeFormMessage("Feedback"));
    //Add(wxMakeFormNewLine());
  }
  Add(wxMakeFormShort(" ", &band_width, wxFORM_DEFAULT, new wxList(wxMakeConstraintRange(0.0, 100.0), 0), NULL, wxVERTICAL));
  Add(wxMakeFormMessage("Bandwidth"));
  Add(wxMakeFormNewLine());

  AssociatePanel(panel);
}

void tSplFilterForm::OnCancel()
{
  wxForm::OnCancel();
}

void tSplFilterForm::ScanResults()
{
  type = (int)tSplFilter::LOWPASS;
  for (int i = 0; filter_types[i]; i++)
    if (strcmp(typestring, filter_types[i]) == 0)
      type = i;

  // limit frequencies to reasonable values
  int sr2 = win.GetSample().GetSamplingRate()/2;

  if (freq <= 10) freq = 10;
  if (freq > sr2) freq = sr2;

  if (lo_freq <= 10) lo_freq = 10;
  if (lo_freq > sr2) lo_freq = sr2;

  if (hi_freq <= 10) hi_freq = 10;
  if (hi_freq > sr2) hi_freq = sr2;

  if (lo_freq >= hi_freq)
    lo_freq = hi_freq - 1;
}

void tSplFilterForm::OnOk()
{
  int fr, to;

  if (!win.HaveSelection(fr, to))
    return;

  tSample &spl = win.GetSample();
  wxBeginBusyCursor();
  ScanResults();

  tFloatSample fs(spl, fr, to);
  float peak = fs.Peak();

  fs.Filter(-1, -1, (tSplFilter::Type)type, order, (double)freq, (double)band_width / 100.0);
//cout << "old peak: " << peak << ", new peak: " << fs.Peak() << endl;
  fs.Rescale(peak);
  spl.SetSmooth(fs, fr);
  win.Redraw();
  wxEndBusyCursor();
  wxForm::OnOk();
}

// -------------------------------------------------------------------------
//                        filter painter settings
// -------------------------------------------------------------------------

tWahSettingsForm::tWahSettingsForm(tSampleWin &win, tWahWah &w)
  : tSplFilterForm(win, TRUE),
    wah(w)
{
  type = (int)wah.filter_type;
}

void tWahSettingsForm::OnHelp()
{
  gpHelpInstance->ShowTopic("Settings");
}

void tWahSettingsForm::OnOk()
{
  ScanResults();
  wah.filter_type = (tSplFilter::Type)type;
  wah.lo_freq     = lo_freq;
  wah.hi_freq     = hi_freq;
  wah.order       = 2; // order;
  wah.band_width  = (double)band_width / 100.0;
  wxForm::OnOk();
}

// -------------------------------------------------------------------------
//                        pitch painter settings
// -------------------------------------------------------------------------

int tSplPitchForm::range = 1;

tSplPitchForm::tSplPitchForm(tSampleWin &w, tSplPitch &p)
: wxForm( USED_WXFORM_BUTTONS ),
  win(w),
  pitch(p)
{
}

void tSplPitchForm::EditForm(wxPanel *panel)
{
  Add(wxMakeFormShort(" ",  &range, wxFORM_DEFAULT, new wxList(wxMakeConstraintRange(1.0, 12.0), 0)));
  Add(wxMakeFormMessage("Range in Semitones"));
  AssociatePanel(panel);
}

void tSplPitchForm::OnOk()
{
  float frange = pow(FSEMI, range);
  pitch.SetRange(frange);
  wxForm::OnOk();
}

void tSplPitchForm::OnCancel()
{
  wxForm::OnOk();
}

void tSplPitchForm::OnHelp()
{
  gpHelpInstance->ShowTopic("Settings");
}


// -----------------------------------------------------------------------
//                                chorus
// -----------------------------------------------------------------------

int tChorusForm::pitch_freq    = 20;  // Hz/10
int tChorusForm::pitch_range   = 20;
int tChorusForm::pan_freq      = 20;
int tChorusForm::pan_spread    = 50;
int tChorusForm::volume        = 50;

tChorusForm::tChorusForm(tSampleWin &w)
: wxForm( USED_WXFORM_BUTTONS ),
  win(w)
{
}

void tChorusForm::EditForm(wxPanel *panel)
{
  Add(wxMakeFormMessage("This mixes a transposed signal to left and right channels"));
  Add(wxMakeFormNewLine());
  Add(wxMakeFormShort(" ",  &pitch_freq, wxFORM_DEFAULT, new wxList(wxMakeConstraintRange(1.0, 100.0), 0)));
  Add(wxMakeFormMessage("Pitch Modulation Freq"));
  Add(wxMakeFormNewLine());
  Add(wxMakeFormShort(" ", &pitch_range,   wxFORM_DEFAULT, new wxList(wxMakeConstraintRange(0.0, 100.0), 0)));
  Add(wxMakeFormMessage("Pitch Intensity"));
  Add(wxMakeFormNewLine());
  Add(wxMakeFormShort(" ", &pan_freq,   wxFORM_DEFAULT, new wxList(wxMakeConstraintRange(1.0, 100.0), 0)));
  Add(wxMakeFormMessage("Pan Freq"));
  Add(wxMakeFormNewLine());
  Add(wxMakeFormShort(" ", &pan_spread,   wxFORM_DEFAULT, new wxList(wxMakeConstraintRange(0.0, 100.0), 0)));
  Add(wxMakeFormMessage("Pan Intensity"));
  Add(wxMakeFormNewLine());
  Add(wxMakeFormShort(" ", &volume,   wxFORM_DEFAULT, new wxList(wxMakeConstraintRange(0.0, 100.0), 0)));
  Add(wxMakeFormMessage("Effect Volume"));
  Add(wxMakeFormNewLine());

  AssociatePanel(panel);
};


void tChorusForm::OnOk()
{
  tSample &spl           = win.GetSample();
  JZMapper map;

  map.Initialize(0, 100, 0.1, 10.0);
  float xpitch_freq    = map(pitch_freq);
  map.Initialize(0, 100, 0, 0.01);
  float xpitch_range   = map(pitch_range);
  map.Initialize(0, 100, 0.1, 10.0);
  float xpan_freq      = map(pan_freq);
  map.Initialize(0, 100, 0, 1);
  float xpan_spread    = map(pan_spread);
  //float xstereo_spread = stereo_spread / 100.0;
  map.Initialize(0, 100, -1, 1);
  float xvolume        = map(volume);

  wxBeginBusyCursor();
  sig_chorus(
    spl,
    xpitch_freq,        // pitch modification freq in Hz
    xpitch_range,           // variable delay in seconds
    xpan_freq,              // pan freq in Hz
    xpan_spread,            // 0..1
    xvolume                 // -1..1
  );
  win.Redraw();
  wxEndBusyCursor();

  wxForm::OnOk();
}

void tChorusForm::OnCancel()
{
  wxForm::OnCancel();
}

void tChorusForm::OnHelp()
{
  gpHelpInstance->ShowTopic("Chorus");
}

// -----------------------------------------------------------------------
//                               stereo
// -----------------------------------------------------------------------

#if 0

int tStereoForm::delay         = 10;  // millisec
int tStereoForm::stereo_spread = 50;

tStereoForm::tStereoForm(tSampleWin &w)
: wxForm( USED_WXFORM_BUTTONS ),
  win(w)
{
}

void tStereoForm::EditForm(wxPanel *panel)
{
  Add(wxMakeFormShort(" ",   &delay, wxFORM_DEFAULT, new wxList(wxMakeConstraintRange(0.0, 20.0), 0)));
  Add(wxMakeFormMessage("Delay"));
  Add(wxMakeFormNewLine());
  Add(wxMakeFormShort(" ", &stereo_spread,   wxFORM_DEFAULT, new wxList(wxMakeConstraintRange(0.0, 100.0), 0)));
  Add(wxMakeFormMessage("Stereo spread"));
  Add(wxMakeFormNewLine());
  Add(wxMakeFormNewLine());

  AssociatePanel(panel);
};


void tStereoForm::OnOk()
{
  tSample &spl           = win.GetSample();
  int    xdelay         = spl.Seconds2Samples(delay/1000.0);
  double  xstereo_spread = stereo_spread / 100.0;

  wxBeginBusyCursor();
  sig_stereo(
    spl,
    xdelay,                 // in samples
    xstereo_spread        // 0..1
  );
  win.Redraw();
  wxEndBusyCursor();

  wxForm::OnOk();
}

void tStereoForm::OnCancel()
{
  wxForm::OnCancel();
}

void tStereoForm::OnHelp()
{
}

#endif

// -------------------------------------------------------------------------
//                             time stretching
// -------------------------------------------------------------------------

int tStretcherForm::winsize     = 10;  // 0.1 .. 0.2 ??
int tStretcherForm::seconds     = 0;
int tStretcherForm::centies     = 0;
int tStretcherForm::oldspeed    = 120;
int tStretcherForm::newspeed    = 0;
bool tStretcherForm::keep_pitch = 1;


tStretcherForm::tStretcherForm(tSampleWin &w)
: wxForm( USED_WXFORM_BUTTONS ),
  win(w),
  spl(w.GetSample())
{
}


void tStretcherForm::OnHelp()
{
  gpHelpInstance->ShowTopic("Time Stretcher");
}


void tStretcherForm::EditForm(wxPanel *panel)
{
  char buf[500];
  Add(wxMakeFormMessage("Changes length of the sample"));
  Add(wxMakeFormNewLine());
  sprintf(buf, "This sample is %6.2lf seconds long", spl.Samples2Seconds(spl.GetLength()));
  Add(wxMakeFormMessage(buf));
  Add(wxMakeFormNewLine());

  Add(wxMakeFormShort(" ",  &seconds, wxFORM_DEFAULT, new wxList(wxMakeConstraintRange(-10.0, 10.0), 0)));
  Add(wxMakeFormMessage("Seconds"));
  Add(wxMakeFormNewLine());
  Add(wxMakeFormShort(" ",  &centies, wxFORM_DEFAULT, new wxList(wxMakeConstraintRange(-100.0, 100.0), 0)));
  Add(wxMakeFormMessage("Seconds/100"));
  Add(wxMakeFormNewLine());

  oldspeed = newspeed = gpSong->Speed();
  sprintf(buf, "current midi speed is %d", oldspeed);
  Add(wxMakeFormMessage(buf));
  Add(wxMakeFormNewLine());
  Add(wxMakeFormShort(" ",  &oldspeed, wxFORM_DEFAULT, new wxList(wxMakeConstraintRange(50.0, 200.0), 0)));
  Add(wxMakeFormMessage("Old Speed"));
  Add(wxMakeFormNewLine());
  Add(wxMakeFormShort(" ",  &newspeed, wxFORM_DEFAULT, new wxList(wxMakeConstraintRange(50.0, 200.0), 0)));
  Add(wxMakeFormMessage("New Speed"));
  Add(wxMakeFormNewLine());

  Add(wxMakeFormBool("Keep pitch", &keep_pitch));
  Add(wxMakeFormNewLine());
  Add(wxMakeFormShort(" ",   &winsize, wxFORM_DEFAULT, new wxList(wxMakeConstraintRange(0.0, 100.0), 0)));
  Add(wxMakeFormMessage("Window Size"));
  Add(wxMakeFormNewLine());

  AssociatePanel(panel);
}

void tStretcherForm::OnCancel()
{
  wxForm::OnCancel();
}

void tStretcherForm::OnOk()
{
  tSample &spl = win.GetSample();
  wxBeginBusyCursor();

  double length = spl.GetLength();
  double dtime = seconds + (double)centies/100.0;
  length += spl.Seconds2Samples(dtime);

  length = length * (double)oldspeed / (double)newspeed;

  tShifterCmd shifter(spl.GetSamplingRate());
  shifter.StretchLength(spl, length, keep_pitch, winsize);

  win.Redraw();
  wxEndBusyCursor();
  wxForm::OnOk();
}

#endif
