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
#include "config.h"
#include "arpeggio.h"
#include "trackwin.h"
#include "toolbar.h"
#include "random.h"
#include "command.h"


class tSoundingNotes {
public:
  tSoundingNotes(tSimpleEventArray &ea, long frclk, long toclk, long step)
    : frc(frclk), toc(toclk), stp(step), clock(frclk), iter(&ea)
  {
    evnt = iter.Range(frc, toc);
    for (int i = 0; i < 128; i++)
      kons[i] = 0;
  }

  int next();                 // returns number of notes
  tKeyOn* operator[](int i);  // get the ith note
private:
  long frc, toc, stp, clock;
  tEventIterator iter;
  tEvent *evnt;
  tKeyOn *kons[128];
};

// return the number of sounding notes at this step
int tSoundingNotes::next() {
  clock += stp;
  while (evnt && evnt->Clock <= clock) {
    if (evnt->IsKeyOn()) {
      tKeyOn *k = evnt->IsKeyOn();
      kons[k->Key] = k;
    }
    evnt = iter.Next();
  }

  int nkeys = 0;
  for (int i = 0; i < 128; i++) {
    if (kons[i] && kons[i]->Clock + kons[i]->Length < clock)
      kons[i] = 0;
    if (kons[i])
      ++ nkeys;
  }
  return nkeys;
}

// return the ith sounding note
tKeyOn *tSoundingNotes::operator[](int nth) {
  int i, k = -1;
  for (i = 0; i < nth + 1; i++) {
    ++k;
    while (k < 128 && kons[k] == 0)
      ++k;
  }
  return kons[k];
}



class tNoteArpeggio : public tCommand
{
public:
  tNoteArpeggio(tFilter *f, tRndArray &pit, int xticks)
    : tCommand(f), parr(pit), ticks(xticks)
  {
  }

  void ExecuteTrack(tTrack *t);
private:
  tRndArray &parr;
  int ticks;
};


void tNoteArpeggio::ExecuteTrack(tTrack *t)
{

  tEventIterator iter(t);
  long frc = Filter->FromClock;
  long toc = Filter->ToClock;

  tSimpleEventArray sea;
  sea.Copy(*t, frc, toc);
  tSoundingNotes sn(sea, frc, toc, ticks);
  tCmdErase eraser(Filter);
  eraser.Execute(1);

  long clock = frc;
  int  ith   = 0;
  while (clock < toc)
  {
    int n = sn.next();   // number of sounding notes
    int pitch  = parr[ith % parr.Size()] - 1;
    if (n > 2 && pitch >= 0) {
      //int length = step * larr[ith % larr.Size()] / larr.Max();
      int length = ticks - 2;
      if (length < 2)
        length = 2;

      int oct    = 0;
      while (pitch >= n) {
        pitch -= n;
        ++ oct;
      }

      tKeyOn *k = (tKeyOn *)sn[pitch]->Copy();
      k->Clock  = clock;
      k->Key   += oct * 12;
      k->Length = length;
      t->Put(k);
    }
    clock += ticks;
    ++ ith;
  }
  t->Cleanup();
}








class tCtrlArpeggio : public tCommand
{
public:
  tCtrlArpeggio(tFilter *f, int xctrl, tRndArray &xval, int xticks)
    : tCommand(f), ctrl(xctrl), values(xval), ticks(xticks)
  {
  }

  void ExecuteTrack(tTrack *t);
private:
  int ctrl;
  tRndArray &values;
  int ticks;
};


void tCtrlArpeggio::ExecuteTrack(tTrack *t)
{
  tEventIterator iter(t);
  long frc = Filter->FromClock;
  long toc = Filter->ToClock;

  tCmdErase eraser(Filter);
  eraser.Execute(1);

  long clock = frc;
  int  ith   = 0;
  while (clock < toc)
  {
    int value = values[ith % values.Size()];
    int chn = t->Channel - 1;
    tControl *c = new tControl(clock, chn, ctrl, value);
    t->Put(c);
    clock += ticks;
    ++ ith;
  }
  t->Cleanup();
}






#define MEN_LOAD 1
#define MEN_SAVE 2
#define MEN_PLAY 3
#define MEN_GEN  4
#define MEN_HELP 5
#define MEN_CLOSE 6
#define MEN_TRANSP 7
#define MEN_RANDOM 8

#ifdef wx_x

#include "../bitmaps/open.xpm"
#include "../bitmaps/save.xpm"
#include "../bitmaps/rrggen.xpm"
#include "../bitmaps/alea.xpm"
#include "../bitmaps/play.xpm"
#include "../bitmaps/help.xpm"

static tToolDef tdefs[] = {
  { MEN_LOAD,  	FALSE,  0, open_xpm },
  { MEN_SAVE,  	FALSE,  1, save_xpm },
  { MEN_GEN,  	FALSE,  0, rrggen_xpm },
  { MEN_RANDOM,	FALSE,  0, alea_xpm },
  { MEN_PLAY,	FALSE,  1, play_xpm },
  { MEN_HELP,  	FALSE,  0, help_xpm }
};

#else

static tToolDef tdefs[] = {
  { MEN_LOAD,  	FALSE,  0, "tb_open", "load settings" },
  { MEN_SAVE,  	FALSE,  1, "tb_save", "save settings" },
  { MEN_GEN,  	FALSE,  0, "tb_rrggen", "generate" },
  { MEN_RANDOM,	FALSE,  0, "tb_alea", "random arpeggio" },
  { MEN_PLAY,	FALSE,  1, "tb_play",   "play" },
  { MEN_HELP,  	FALSE,  0, "tb_help", "help" }
};

#endif

int tArpeggioWin::geo[4] = { 20, 20, 400, 250 };

int tArpeggioWin::step_count   =  8;
int tArpeggioWin::note_count   = 4;
bool tArpeggioWin::is_controller  = 0;
int tArpeggioWin::controller   = 10;
int tArpeggioWin::measure = 16;


tArpeggioWin::tArpeggioWin(tTrackWin *w, wxFrame **ref)
: tSliderWin(w, ref, "Arpeggiator", geo, tdefs, 6),
  tw(w)
{

  edits = (tRhyArrayEdit **)sliders;

  default_filename = copystring("noname.apg");

  wxMenuBar *menu_bar = new wxMenuBar;
  wxMenu    *menu;

  menu = new wxMenu();
  menu->Append(MEN_LOAD, "&Load");
  menu->Append(MEN_SAVE, "&Save");
  menu->AppendSeparator();
  menu->Append(MEN_CLOSE, "&Close");
  menu_bar->Append(menu, "&File");

  menu = new wxMenu();
  menu->Append(MEN_GEN,  "&Generate");
  menu->Append(MEN_RANDOM, "&Randomize");
  menu->Append(MEN_PLAY, "&Play");
  menu_bar->Append(menu, "&Generator");

  menu = new wxMenu();
  menu->Append(MEN_HELP, "&Random Arpeggio");
  menu_bar->Append(menu, "&Help");

  SetMenuBar(menu_bar);

  arrays[PITCH] = new tRndArray(step_count, 0, note_count);
  edits[PITCH] = new tRhyArrayEdit(this, *arrays[PITCH], 10, 10, 10, 10, (ARED_GAP | ARED_XTICKS));
  edits[PITCH]->SetLabel("Note");
  edits[PITCH]->SetStyle(ARED_GAP | ARED_BLOCKS | ARED_XTICKS | ARED_YTICKS);
  edits[PITCH]->SetXMinMax(1, step_count);
  edits[PITCH]->SetYMinMax(0, note_count);

#if 0
  arrays[LENGTH] = new tRndArray(step_count, 1, 8);
  edits[LENGTH] = new tRhyArrayEdit(this, *arrays[LENGTH], 10, 10, 10, 10, (ARED_GAP | ARED_XTICKS));
  edits[LENGTH]->SetLabel("length");
  edits[LENGTH]->SetStyle(ARED_GAP | ARED_XTICKS | ARED_YTICKS);
  edits[LENGTH]->SetXMinMax(1, step_count);
#endif

  Initialize();

  Random();
}


void tArpeggioWin::OnMenuCommand(int id)
{
  switch (id) {
    case MEN_CLOSE:
      DELETE_THIS();
      break;

    case MEN_LOAD:
      {
        wxString fname= file_selector(default_filename, "Load Arpeggio", 0, 0, "*.apg");
        if (fname) {
          int x;
	  ifstream is(fname);
          is >> x; // version
          is >> step_count;
          is >> note_count;
          is >> x; is_controller = x;
          is >> controller;
          is >> measure;
          is >> *arrays[PITCH];
          if (is.bad())
            wxMessageBox("IO Error", "Error", wxOK);
          step_count_slider->SetValue(step_count);
          note_count_slider->SetValue(note_count);
          is_controller_cb->SetValue(is_controller);
          measure_cb->SetMeasure(measure);
          edits[PITCH]->SetXMinMax(1, arrays[PITCH]->Size());
          edits[PITCH]->SetYMinMax(arrays[PITCH]->Min(), arrays[PITCH]->Max());
          if (is_controller)
            edits[PITCH]->SetLabel(Config.CtrlName(controller).Name);
          else
            edits[PITCH]->SetLabel("Notes");
          ForceRepaint();
        }
      }
      break;

    case MEN_SAVE:
      {
        wxString fname = file_selector(default_filename, "Save Arpeggio", 1, 0, "*.apg");
        if (fname) {
	  ofstream os(fname);
          os << 1 << endl; // version
          os << step_count << " ";
          os << note_count << " ";
          os << (int)is_controller << " ";
          os << controller << " ";
          os << measure << endl;
          os << *arrays[PITCH] << endl;
          if (os.bad())
            wxMessageBox("IO Error", "Error", wxOK);
        }
      }
      break;

    case MEN_GEN:
      Generate();
      break;

    case MEN_PLAY:
      tw->MousePlay(0, tTrackWin::PlayLoopButton);
      break;

    case MEN_RANDOM:
      Random();
      ForceRepaint();
      break;

    case MEN_HELP:
      HelpInstance->ShowTopic("Arpeggio Generator");
      break;


  }
}


void tArpeggioWin::Random()
{
  int i;
  tRndArray &a = *arrays[PITCH];
  for (i = 0; i < a.Size(); i++)
    a[i] = (int) (rnd.asDouble() * (a.Max() - 1)) + 1;

#if 0
  tRndArray &b = *arrays[LENGTH];
  for (i = 0; i < b.Size(); i++)
    if (rnd.asDouble() < 0.1)
      b[i] = b.Max();
    else
      b[i] = b.Min();
#endif
}

void tArpeggioWin::AddItems()
{
  int w = 150;

  //  step_count_slider   = new wxSlider(panel,  (wxFunction)OnItem, "",   step_count,  4,  32, w, -1, -1, wxFIXED_LENGTH);
  step_count_slider   = new wxSlider(panel, -1,    step_count,  4,  32);//, w, -1, -1, wxFIXED_LENGTH);
  step_count_slider->SetValue(step_count);
  (void) new wxStaticText(panel, -1,"# steps");

  //  is_controller_cb = new wxCheckBox(panel, (wxFunction)OnItem, "Controller");
  is_controller_cb = new wxCheckBox(panel, -1, "Controller");
  is_controller_cb->SetValue(is_controller);
  //  panel->NewLine();


  note_count_slider    = new wxSlider(panel, -1, note_count,  4,  32);//, w, -1, -1, wxFIXED_LENGTH);
  note_count_slider->SetValue(note_count);
  (void) new wxStaticText(panel, -1,"# notes");

#ifndef __PORTING 
  measure_cb = new tMeasureChoice(panel, (wxFunction)OnItem, "");
  measure_cb->SetMeasure(measure);
  //  panel->NewLine();
#endif // __PORTING

  panel->Fit();

}

void tArpeggioWin::AddEdits()
{
  n_sliders = N_EDITS;
  sliders_per_row = 1;
}

#ifndef __PORTING 
void tArpeggioWin::OnItem(wxItem& item, wxCommandEvent& event)
{
  wxWindow *win = item.GetParent();  // sub panel
  win = win->GetParent();           // the window

  if (&item == step_count_slider) {
    step_count  = step_count_slider->GetValue();
    arrays[PITCH]->Resize(step_count);
    edits[PITCH]->SetXMinMax(1, step_count);
#if 0
    arrays[LENGTH]->Resize(step_count);
    edits[LENGTH]->SetXMinMax(1, step_count);
#endif
  }

  if (&item == note_count_slider) {
    note_count   = note_count_slider->GetValue();
    arrays[PITCH]->SetMinMax(0, note_count);
    edits[PITCH]->SetYMinMax(0, note_count);
  }

  if (&item == is_controller_cb) {
    is_controller = is_controller_cb->GetValue();
    int ctrl = -1;
    if (is_controller)
      ctrl = SelectControllerDlg();

    if (ctrl >= 0) {
      controller = ctrl;
      edits[PITCH]->SetLabel(Config.CtrlName(controller).Name);
      arrays[PITCH]->SetMinMax(0, 127);
      edits[PITCH]->SetYMinMax(0, 127);
      note_count_slider->Enable(FALSE);
    }
    else {
      edits[PITCH]->SetLabel("Note");
      arrays[PITCH]->SetMinMax(0, note_count);
      edits[PITCH]->SetYMinMax(0, note_count);
      note_count_slider->Enable(TRUE);
      is_controller_cb->SetValue(false);
      is_controller = false;

    }
  }

  if (&item == measure_cb)
    measure = measure_cb->GetMeasure();

  ForceRepaint();
}

 void tArpeggioWin::Callback(wxItem& item, wxCommandEvent& event)
 {

   ((tArpeggioWin *)win)->OnItem(item, event);
 }

#endif // __PORTING

void tArpeggioWin::Generate()
{
  if (!tw->EventsSelected())
    return;

#ifndef __PORTING 
  if (is_controller) {
    tCtrlArpeggio arp(tw->Filter, controller - 1, *arrays[PITCH], measure_cb->GetTicks(tw->Song));
    arp.Execute(1);
  }
  else {
    tNoteArpeggio arp(tw->Filter, *arrays[PITCH], measure_cb->GetTicks(tw->Song));
    arp.Execute(1);
  }
#endif // __PORTING
  tw->Redraw();
}
