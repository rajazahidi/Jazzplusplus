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
#include "genmeldy.h"
#include "genmeldp.h"
#include "harmony.h"
#include "jazz.h"
#include "mapper.h"
#include "trackwin.h"
#include "command.h"
#include "harmonyp.h"

#define MEN_LOAD 1
#define MEN_SAVE 2
#define MEN_PLAY 3
#define MEN_GEN  4
#define MEN_HELP 5
#define MEN_CLOSE 6
#define MEN_TRANSP 7


#include "../bitmaps/open.xpm"
#include "../bitmaps/save.xpm"
#include "../bitmaps/rrggen.xpm"
#include "../bitmaps/transpos.xpm"
#include "../bitmaps/play.xpm"
#include "../bitmaps/help.xpm"

static tToolDef tdefs[] = {
  { MEN_LOAD,   FALSE, open_xpm,     "load settings" },
  { MEN_SAVE,   FALSE, save_xpm,     "save settings" },
  TOOLDEF_SEPARATOR,
  { MEN_GEN,    FALSE, rrggen_xpm,   "generate" },
  { MEN_TRANSP, FALSE, transpos_xpm, "transpose trackwin selection" },
  { MEN_PLAY,   FALSE, play_xpm,     "play" },
  TOOLDEF_SEPARATOR,
  { MEN_HELP,   FALSE, help_xpm,     "help" },
  TOOLDEF_END
};


int tGenMelody::geo[4] = { 50, 80, 800, 600 };

int tGenMelody::pitch_center = 64;
int tGenMelody::pitch_range  = 18;
int tGenMelody::note_count   = 1;
int tGenMelody::rand_patlen = 0;
int tGenMelody::rhythm_intv = 0;


tGenMelody::tGenMelody(tTrackWin *w, wxFrame **ref)
: tSliderWin(w, ref, "Random Melody", geo, tdefs),
  tw(w)
{

  int i;

  edits = (tRhyArrayEdit **)sliders;


  default_filename = copystring("noname.mel");

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
  menu->Append(MEN_PLAY, "&Play");
  menu_bar->Append(menu, "&Generator");

  menu = new wxMenu();
  menu->Append(MEN_HELP, "&Melody Generator");
  menu_bar->Append(menu, "&Help");

  SetMenuBar(menu_bar);

  for (i = 0; i < N_EDITS; i++) {
    arrays[i] = new tRndArray(16, 0, 100);
    edits[i] = new tRhyArrayEdit(this, *arrays[i], 10, 10, 10, 10, (ARED_GAP | ARED_XTICKS));
  }


  edits[R_VELOC]->SetLabel("velocity");
  arrays[R_VELOC]->Resize(32);
  edits[R_VELOC]->SetXMinMax(1, 127);

  edits[R_LENGTH]->SetLabel("note length");
  arrays[R_LENGTH]->Resize(16);
  edits[R_LENGTH]->SetXMinMax(1, 16);

  edits[R_PATLEN]->SetLabel("seed length");
  edits[R_PATLEN]->SetXMinMax(1, 16);
  //edits[R_PATLEN]->SetStyle(ARED_XTICKS | ARED_RHYTHM | ARED_GAP);

  edits[R_REPEAT]->SetLabel("seed repeat");
  arrays[R_REPEAT]->Resize(16);
  edits[R_REPEAT]->SetXMinMax(1, 16);

  edits[R_INTERV]->SetLabel("pitch interval");
  arrays[R_INTERV]->Resize(8);
  edits[R_INTERV]->SetXMinMax(0, 7);

  edits[R_VARIAT]->SetLabel("seed variations");
  arrays[R_VARIAT]->Resize(RMGVariations::N_VARIAT);
  edits[R_VARIAT]->SetXMinMax(1, RMGVariations::N_VARIAT);

  edits[R_RHYTHM]->SetLabel("rhythm");
  edits[R_RHYTHM]->SetStyle(ARED_XTICKS | ARED_RHYTHM | ARED_GAP);

  Initialize();
}


void tGenMelody::OnMenuCommand(int id)
{
  switch (id) {
    case MEN_CLOSE:
      DELETE_THIS();
      break;

    case MEN_LOAD:
      {
        wxString fname = file_selector(default_filename, "Load Generator", 0, 0, "*.mel");
        if (fname) {
	  ifstream is(fname);
          int x;
          is >> x; // version
          is >> pitch_center;
          is >> pitch_range;
          is >> note_count;
          is >> rand_patlen;
          for (int i = 0; i < N_EDITS; i++)
            is >> *arrays[i];
          is >> rhythm_intv;

          arrays[R_VARIAT]->Resize(RMGVariations::N_VARIAT);

          pitch_center_slider->SetValue(pitch_center);
          pitch_range_slider->SetValue(pitch_range);
          note_count_slider->SetValue(note_count);
          rand_patlen_cbox->SetValue(rand_patlen);
          rhythm_intv_cbox->SetValue(rhythm_intv);
          edits[R_PATLEN]->Enable(rand_patlen);
          ForceRepaint();
        }
      }
      break;

    case MEN_SAVE:
      {
        wxString fname = file_selector(default_filename, "Save Generator", 1, 0, "*.mel");
        if (fname) {
	  ofstream os(fname);
          os << 1 << endl;
          os << pitch_center << " ";
          os << pitch_range << " ";
          os << note_count << " ";
          os << rand_patlen << endl;
          for (int i = 0; i < N_EDITS; i++)
            os << *arrays[i] << endl;
          os << rhythm_intv << endl;
        }
      }
      break;

    case MEN_GEN:
      Generate();
      break;

    case MEN_PLAY:
      tw->MousePlay(0, tTrackWin::PlayLoopButton);
      break;

#ifndef __PORTING 
    case MEN_TRANSP:
      if (!the_harmony_browser)
        harmony_browser(TrackWin);
      else
        the_harmony_browser->TransposeSelection();
      break;
#endif // __PORTING

    case MEN_HELP:
        HelpInstance->ShowTopic("Random Melody Generator");
	break;

  }
}



void tGenMelody::AddItems()
{
  int x, y, w, h;

  rhythm_panel = new tRhythmSliderPanel(panel, edits[R_RHYTHM], 0,0, 200, 200);
  rhythm_panel->AddEdit(edits[R_PATLEN]);
  rhythm_panel->Fit();
  rhythm_panel->GetPosition(&x, &y);
  rhythm_panel->GetSize(&w, &h);

  wxPanel *p2 = new wxPanel(panel, x+w, y, w, h);
#ifndef __PORTING 
  p2->SetLabelPosition(wxHORIZONTAL);
  p2->NewLine();
#endif // __PORTING
  //  pitch_center_slider = new wxSlider(p2, (wxFunction)Callback,  "", 64,  1,  99, w, -1, -1, wxFIXED_LENGTH);
  pitch_center_slider = new wxSlider(p2, -1,   64,  1,  99);//, w, -1, -1, wxFIXED_LENGTH);
  pitch_center_slider->SetValue(pitch_center);
  (void) new wxStaticText(p2, -1,"pitch center");
#ifndef __PORTING 
  p2->NewLine();
#endif // __PORTING


  //  pitch_range_slider   = new wxSlider(p2, (wxFunction)Callback, "", 18,  1,  36, w, -1, -1, wxFIXED_LENGTH);
  pitch_range_slider   = new wxSlider(p2, -1,18,  1,  36);//, w, -1, -1, wxFIXED_LENGTH);
  pitch_range_slider->SetValue(pitch_range);
  (void) new wxStaticText(p2, -1,"pitch range");
#ifndef __PORTING 
  p2->NewLine();
#endif // __PORTING


  //  note_count_slider    = new wxSlider(p2, (wxFunction)Callback, "",   1, 1,  12, w, -1, -1, wxFIXED_LENGTH);
  note_count_slider    = new wxSlider(p2, -1,   1, 1,  12);//, w, -1, -1, wxFIXED_LENGTH);
  note_count_slider->SetValue(note_count);
  (void) new wxStaticText(p2, -1,"# notes");
#ifndef __PORTING 
  p2->NewLine();
#endif // __PORTING
  p2->Fit();

  p2->GetPosition(&x, &y);
  p2->GetSize(&w, &h);
  wxPanel *p3 = new wxPanel(panel, x+w, y, w, h);
#ifndef __PORTING 
  p3->NewLine();
#endif // __PORTING
  //  rand_patlen_cbox = new wxCheckBox(p3, (wxFunction)Callback, "Random Seed Length");
  rand_patlen_cbox = new wxCheckBox(p3, -1,  "Random Seed Length");
  rand_patlen_cbox->SetValue(rand_patlen);
  edits[R_PATLEN]->Enable(rand_patlen);
#ifndef __PORTING 
  p3->NewLine();
#endif // __PORTING

  //  rhythm_intv_cbox = new wxCheckBox(p3, (wxFunction)Callback, "Rhythm as intervals");
  rhythm_intv_cbox = new wxCheckBox(p3, -1,  "Rhythm as intervals");
  rhythm_intv_cbox->SetValue(rhythm_intv);
#ifndef __PORTING 
  p3->NewLine();
#endif // __PORTING

  p3->Fit();

}

void tGenMelody::AddEdits()
{
  n_sliders = N_EDITS;
  sliders_per_row = 3;
}

#ifndef __PORTING 
void tGenMelody::OnItem(wxItem& item, wxCommandEvent& event)
{

  rand_patlen = rand_patlen_cbox->GetValue();
  edits[R_PATLEN]->Enable(rand_patlen);
  edits[R_PATLEN]->OnPaint();
  rhythm_intv = rhythm_intv_cbox->GetValue();
  pitch_center = pitch_center_slider->GetValue();
  pitch_range  = pitch_range_slider->GetValue();
  note_count   = note_count_slider->GetValue();

}

void tGenMelody::Callback(wxItem& item, wxCommandEvent& event)
{
  wxWindow *win = item.GetParent();  // sub panel
  win = win->GetParent();           // panel
  win = win->GetParent();           // the window
  ((tGenMelody *)win)->OnItem(item, event);
}

#endif // __PORTING


class tGenMelodyOutput : public RMGOutput {
public:
  tGenMelodyOutput(tTrack *t, long frc, long s) : track(t), fr_clock(frc), steps(s) {}
  void Note(long clock, int chn, int key, int vel, int len) {
    tKeyOn *k = new tKeyOn(fr_clock + clock * steps, chn, key, vel, len * steps - steps/2);
    track->Put(k);
  }
private:
  tTrack *track;
  long   fr_clock;
  long   steps;
};


class tGenMelodyScale : public RMGScale {
public:
  tGenMelodyScale() : chord(major_scale) {}
  virtual int next_up(int key, long clock, int signif) {
    while(!chord.Contains(key))
      ++key;
    return key;
  }

  virtual int next_dn(int key, long clock, int signif) {
    while(!chord.Contains(key))
      --key;
    return key;
  }

  HBChord chord;
};


void tGenMelody::Generate()
{

  if (!tw->EventsSelected("please mark destination track in trackwin"))
    return;

  tFilter *filter = tw->Filter;

  if (filter->FromTrack != filter->ToTrack)
  {
    wxMessageBox("you must select exacty 1 track", "Error", wxOK);
    return;
  }

  long fr_clock = filter->FromClock;
  long to_clock = filter->ToClock;
  tSong *song = tw->Song;
  tTrack *track = song->GetTrack(filter->FromTrack);
  song->NewUndoBuffer();

  {
    tCmdErase erase(filter, 1);
    erase.Execute(0);
  }

  long steps_per_bar = rhythm_panel->StepsPerCount() * rhythm_panel->CountPerBar();
  long ticks = (song->TicksPerQuarter * 4) / steps_per_bar;

  tGenMelodyOutput out(track, fr_clock, ticks);
  tGenMelodyScale scale;

  RMGGenerator gen(out, scale);
  gen.param[RMGGenerator::LENGTH]  = new RMGRandom(*arrays[R_LENGTH], 1);
  gen.param[RMGGenerator::VELOC]   = new RMGRandom(*arrays[R_VELOC],  1, 127);
  gen.param[RMGGenerator::INTERV]  = new RMGRandom(*arrays[R_INTERV]);
  gen.param[RMGGenerator::REPEAT]  = new RMGRandom(*arrays[R_REPEAT], 1);
  gen.param[RMGGenerator::VARIAT]  = new RMGRandom(*arrays[R_VARIAT]);

  if (rhythm_intv)
    gen.param[RMGGenerator::RHYTHM]  = new RMGRandom(*arrays[R_RHYTHM], 1);
  else
    gen.param[RMGGenerator::RHYTHM]  = new RMGRhythm(*arrays[R_RHYTHM]);

  if (rand_patlen)
    gen.param[RMGGenerator::SEEDLEN] = new RMGRandom(*arrays[R_PATLEN], 1);
  else
    gen.param[RMGGenerator::SEEDLEN] = new RMGConst(arrays[R_RHYTHM]->Size());

  gen.param[RMGGenerator::CENTER]  = new RMGConst(pitch_center);
  gen.param[RMGGenerator::RANGE]   = new RMGConst(pitch_range);
  gen.param[RMGGenerator::CHORD]   = new RMGConst(note_count);
  gen.param[RMGGenerator::CHANNEL] = new RMGConst(track->Channel - 1);

  long size = (to_clock - fr_clock) / ticks;
  gen.run(size);

  track->Cleanup();
  tw->Redraw();
}
