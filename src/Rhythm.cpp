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

#include "Rhythm.h"
#include "EventWindow.h"
#include "Song.h"
#include "Command.h"
#include "Harmony.h"
#include "TrackFrame.h"
#include "PianoFrame.h"
#include "ToolBar.h"
#include "DeprecatedStringUtils.h"
#include "FileSelector.h"
#include "StringReadWrite.h"
#include "KeyStringConverters.h"
#include "SelectControllerDialog.h"
#include "Help.h"

#include <fstream>

using namespace std;

tRhythmWin *rhythm_win = 0;

void tRhyGroup::write(ostream &os)
{
  os << listen << " ";
  os << contrib << " ";
}

void tRhyGroup::read(istream &is, int version)
{
  is >> listen;
  is >> contrib;
}

void tRhyGroups::write(ostream &os)
{
  for (int i = 0; i < MAX_GROUPS; i++)
    g[i].write(os);
  os << endl;
}

void tRhyGroups::read(istream &is, int version)
{
  for (int i = 0; i < MAX_GROUPS; i++)
    g[i].read(is, version);
}



// pseudo key nr's for harmony browser and sound effects
static const int MODE_ALL_OF	= -1;
static const int MODE_ONE_OF	= -2;
static const int MODE_PIANO	= -3;
static const int MODE_CONTROL	= -4;


tRhythm::tRhythm(int k)
  : rhythm(64, 0, 100),
    length( 8, 0, 100),
    veloc (32, 0, 100),
    history(64, 0, 100)
{
  mode            = MODE_ALL_OF;
  n_keys          = 1;
  keys[0]         = k;
  parm            = 0;
  steps_per_count = 4;
  count_per_bar   = 4;
  n_bars          = 1;
  randomize       = true;

  label = copystring("random rhythm");
}

tRhythm::tRhythm(const tRhythm &o)
  : rhythm(o.rhythm),
    length(o.length),
    veloc (o.veloc),
    groups(o.groups),
    history(o.history)
{
  mode            = o.mode;
  n_keys          = o.n_keys;
  for (int i = 0; i < n_keys; i++)
    keys[i] = o.keys[i];
  parm            = o.parm;
  n_bars          = o.n_bars;
  steps_per_count = o.steps_per_count;
  count_per_bar   = o.count_per_bar;
  randomize       = o.randomize;
  groups          = o.groups;

  label           = copystring(o.label);
}

tRhythm & tRhythm::operator=(const tRhythm &o)
{
  mode            = o.mode;
  n_keys          = o.n_keys;
  for (int i = 0; i < n_keys; i++)
    keys[i] = o.keys[i];
  rhythm          = o.rhythm;
  length          = o.length;
  veloc           = o.veloc;
  parm            = o.parm;
  n_bars          = o.n_bars;
  steps_per_count = o.steps_per_count;
  count_per_bar   = o.count_per_bar;
  randomize       = o.randomize;
  groups          = o.groups;
  history         = o.history;

  delete label;
  label           = copystring(o.label);

  return *this;
}


tRhythm::~tRhythm()
{
  delete label;
}


void tRhythm::write(ostream &os)
{
  os << rhythm;
  os << length;
  os << veloc;

  os << steps_per_count << " ";
  os << count_per_bar << " ";
  os << n_bars << " ";
  os << mode << " ";
  os << n_keys << " ";
  for (int i = 0; i < n_keys; i++)
    os << keys[i] << " ";
  os << parm << endl;
  WriteString(os, label) << endl;

  os << randomize << " ";
  groups.write(os);
}


void tRhythm::read(istream &is, int version)
{
  char buf[200];
  is >> rhythm;
  is >> length;
  is >> veloc;

  is >> steps_per_count;
  is >> count_per_bar;
  is >> n_bars;
  is >> mode;
  if (mode >= 0) // old format
  {
    n_keys = 1;
    keys[0] = mode;
    mode = MODE_ALL_OF;
  }
  else {
    is >> n_keys;
    for (int i = 0; i < n_keys; i++)
      is >> keys[i];
  }
  is >> parm;

  ReadString(is, buf, sizeof(buf));
  SetLabel(buf);

  if (version > 1)
  {
    is >> randomize;
    groups.read(is, version);
  }
}

void tRhythm::SetLabel(char const *s)
{
  delete label;
  label = copystring(s);
}


int tRhythm::Clock2i(long clock, JZBarInfo &bi) const
{
  int clocks_per_step = bi.TicksPerBar / (steps_per_count * count_per_bar);
  return (int)(((clock - start_clock) / clocks_per_step) % rhythm.Size());
}

int tRhythm::ClocksPerStep(JZBarInfo &bi) const
{
  return bi.TicksPerBar / (steps_per_count * count_per_bar);
}


void tRhythm::GenInit(long frc)
{
  int i;
  start_clock = frc;
  next_clock  = frc;

  int nn = rhythm.Size();
  history.Resize(nn);

  // initialize history with random values
  for (i = 0; i < nn; i++)
    history[i] = history.Min();

  for (i = 0; i < nn; i++) {
    if (rhythm.Random(i)) {
      history[i] = history.Max();
      i += length.Random();
    }
  }
}


void tRhythm::GenerateEvent(tTrack *track, long clock, short vel, short len)
{
  int chan   = track->Channel - 1;

  // generate key events
  if (mode == MODE_ALL_OF)
  {
    for (int ii = 0; ii < n_keys; ii++)
    {
      tKeyOn *k = new tKeyOn(clock, chan, keys[ii], vel, len);
      track->Put(k);
    }
  }

  else if (mode == MODE_ONE_OF)
  {
    int ii = (int)(rnd.asDouble() * n_keys);
    if (ii < n_keys)
    {
      tKeyOn *k = new tKeyOn(clock, chan, keys[ii], vel, len);
      track->Put(k);
    }
  }

  // generate controller
  else if (mode == MODE_CONTROL) {
    tControl *c = new tControl(clock, chan, parm - 1, vel);
    track->Put(c);
  }

  else
    assert(0);
}


#if 0
void tRhythm::Generate(tTrack *track, long fr_clock, long to_clock, long ticks_per_bar)
{
  int chan   = track->Channel - 1;
  long clock = fr_clock;

  long clocks_per_step = ticks_per_bar / (steps_per_count * count_per_bar);
  long total_steps = (to_clock - fr_clock) / clocks_per_step;

  while (clock < to_clock)
  {
    int i = ((clock - fr_clock) / clocks_per_step) % rhythm.Size();
    if (rhythm.Random(i))
    {
      // put event here
      int rndval;
      if (randomize)
        // keep seed < 1.0
        rndval = veloc.Random((double)rhythm[i] / ((double)rhythm.Max() + 0.001));
      else
        rndval = veloc.Random();
      short vel = rndval * 127 / veloc.Size() + 1;
      short len = (length.Random() + 1) * clocks_per_step;

      // generate keys from harmony browser
      if (key == CHORD_KEY || key == BASS_KEY)
      {
        if (the_harmony_browser)
        {
          long step = (clock - fr_clock) * total_steps / (to_clock - fr_clock);
	  int keys[12], n_keys;
          if (key == CHORD_KEY)
            n_keys = the_harmony_browser->GetChordKeys(keys, (int)step, (int)total_steps);
          else
            n_keys = the_harmony_browser->GetBassKeys(keys, (int)step, (int)total_steps);
          for (int j = 0; j < n_keys; j++)
          {
	    tKeyOn *k = new tKeyOn(clock, chan, keys[j], vel, len - clocks_per_step/2);
	    track->Put(k);
	  }
	}
      }

      // paste pianowin buffer
      else if (key == PASTE_KEY)
      {
        tEventArray &src = TrackWin->GetPianoWindow()->PasteBuffer;
        for (int ii = 0; ii < src.nEvents; ii++)
        {
          tKeyOn *on = src.Events[ii]->IsKeyOn();
          if (on)
          {
	    tKeyOn *k = new tKeyOn(clock, chan, on->Key, vel, len - clocks_per_step/2);
	    track->Put(k);
          }
	}
      }

      // generate controller
      else if (key == CONTROL_KEY) {
	tControl *c = new tControl(clock, chan, parm - 1, vel);
	track->Put(c);
      }

      // generate note on events
      else
      {
	tKeyOn *k = new tKeyOn(clock, chan, key, vel, len - clocks_per_step/2);
	track->Put(k);
      }

      clock += len;
    }
    else
      clock += clocks_per_step;
  }
}
#endif



void tRhythm::GenGroup(JZRndArray &out, int grp, JZBarInfo &bi, tRhythm *rhy[], int n_rhy)
{
  out.Clear();

  int clocks_per_step = ClocksPerStep(bi);

  for (int ri = 0; ri < n_rhy; ri++) {
    tRhythm *r = rhy[ri];
    int fuzz = r->groups[grp].contrib;
    if (fuzz && r != this) {
      JZRndArray tmp(rhythm);
      tmp.Clear();
      long clock = bi.Clock;
      while (clock < bi.Clock + bi.TicksPerBar)
      {
	int i = Clock2i(clock, bi);
	int j = r->Clock2i(clock, bi);
	tmp[i] = r->history[j];
	clock += clocks_per_step;
      }
      out.SetUnion(tmp, fuzz);
    }
  }
}


void tRhythm::Generate(tTrack *track, JZBarInfo &bi, tRhythm *rhy[], int n_rhy)
{
  JZRndArray rrg(rhythm);

  // add groups to the rhythm
  JZRndArray tmp(rhythm);
  for (int gi = 0; gi < MAX_GROUPS; gi++)
  {
    if (groups[gi].listen)
    {
      GenGroup(tmp, gi, bi, rhy, n_rhy);
      if (groups[gi].listen > 0)
	rrg.SetIntersection(tmp, groups[gi].listen);
      else
	rrg.SetDifference(tmp, -groups[gi].listen);
    }
  }

  // clear part of the history
  long clock = bi.Clock;
  int clocks_per_step = ClocksPerStep(bi);
  while (clock < bi.Clock + bi.TicksPerBar)
  {
    int i = Clock2i(clock, bi);
    history[i] = 0;
    clock += clocks_per_step;
  }


  //  generate the events
  clock = next_clock;
  while (clock < bi.Clock + bi.TicksPerBar)
  {
    int i = Clock2i(clock, bi);
    if ((!randomize && rrg[i] > 0) || rrg.Random(i))
    {
      // put event here
      history[i] = rhythm.Max();

      short vel = 0;
      if (randomize)
        vel = veloc.Random() * 127 / veloc.Size() + 1;
      else
        vel = rrg[i] * 126 / rrg.Max() + 1;
      short len = (length.Random() + 1) * clocks_per_step;
      GenerateEvent(track, clock, vel, len - clocks_per_step/2);
      clock += len;
    }
    else
      clock += clocks_per_step;
  }
  next_clock = clock;
}


// ============================ tRhythmWin ==============================


#define MEN_CLOSE 1
#define MEN_LOAD  2
#define MEN_SAVE  3
#define MEN_HELP  4
#define MEN_ADD   5
#define MEN_DEL   6
#define MEN_GEN   7
#define MEN_UP    8
#define MEN_DOWN  9


#include "Bitmaps/open.xpm"
#include "Bitmaps/save.xpm"
#include "Bitmaps/rrgadd.xpm"
#include "Bitmaps/rrgdel.xpm"
#include "Bitmaps/rrgup.xpm"
#include "Bitmaps/rrgdown.xpm"
#include "Bitmaps/rrggen.xpm"
#include "Bitmaps/help.xpm"

static JZToolDef tdefs[] =
{
  { MEN_LOAD, FALSE, open_xpm,    "open rhythm file" },
  { MEN_SAVE, FALSE, save_xpm,    "save into rhythm file" },
  { JZToolBar::eToolBarSeparator },
  { MEN_ADD,  FALSE, rrgadd_xpm,  "add instrument" },
  { MEN_DEL,  FALSE, rrgdel_xpm,  "remove instrument" },
  { MEN_UP,   FALSE, rrgup_xpm,   "move instrument up" },
  { MEN_DOWN, FALSE, rrgdown_xpm, "move instrument down" },
  { MEN_GEN,  FALSE, rrggen_xpm,  "generate events into trackwin selection" },
  { JZToolBar::eToolBarSeparator },
  { MEN_HELP, FALSE, help_xpm,    "help" },
  { JZToolBar::eToolBarEnd }
};


tRhythmWin::tRhythmWin(JZEventFrame *e, JZSong *s)
  : wxFrame(0, -1, "Random Rhythm Generator", wxPoint(Config(C_RhythmXpos), Config(C_RhythmYpos)), wxSize(640, 580)),
    edit(0)
{
#ifdef OBSOLETE
  event_win        = e;
  song             = s;
  in_create        = 1;
  n_instruments    = 0;
  act_instrument   = -1;
  default_filename = copystring("noname.rhy");
  has_changed      = false;

  mpToolBar = new JZToolBar(this, tdefs);
  mpToolBar->GetMaxSize(&tb_width, &tb_height);

  steps_per_count = 0;
  count_per_bar   = 0;
  n_bars          = 0;
  instrument_list = 0;

  wxMenuBar *menu_bar = new wxMenuBar;
  wxMenu    *menu = new wxMenu;
  menu->Append(MEN_LOAD,	"&Load");
  menu->Append(MEN_SAVE,	"&Save");
  menu->Append(MEN_CLOSE,	"&Close");
  menu_bar->Append(menu,	"&File");

  menu = new wxMenu;
  menu->Append(MEN_ADD,	        "&Add");
  menu->Append(MEN_DEL,	        "&Delete");
  menu->Append(MEN_UP,	        "&Up");
  menu->Append(MEN_DOWN,	"&Down");
  menu->Append(MEN_GEN,	        "&Generate");
  menu_bar->Append(menu,	"&Instrument");

  menu = new wxMenu;
  menu->Append(MEN_HELP,	"&Help");
  menu_bar->Append(menu,	"Help");

  SetMenuBar(menu_bar);

  int x = 0;
  int y = (int)tb_height;
  int w, h;
  GetClientSize(&w, &h);
  h -= (int)tb_height;
  inst_panel = new wxPanel(this, x, y, w/2, h/2, 0, "InstPanel");
  //  inst_panel->SetLabelPosition(wxHORIZONTAL);

#ifdef __WXMSW__
  steps_per_count = new wxSlider(inst_panel, (wxFunction)ItemCallback, "", 4, 1, 16, w/3);
  (void) new wxMessage(inst_panel, "steps/count");
  inst_panel->NewLine();

  count_per_bar   = new wxSlider(inst_panel, (wxFunction)ItemCallback, "", 4, 1, 16, w/3);
  (void) new wxMessage(inst_panel, "count/bar");
  inst_panel->NewLine();

  n_bars          = new wxSlider(inst_panel, (wxFunction)ItemCallback, "", 4, 1, 16, w/3);
  (void) new wxMessage(inst_panel, "# bars");
  inst_panel->NewLine();
#else

  steps_per_count = new wxSlider(inst_panel, (wxFunction)ItemCallback, "", 4, 1, 16, w/6, 10,     1, wxFIXED_LENGTH);
#ifdef wx_motif
  (void) new wxMessage(inst_panel, "steps/count", -1, MOTIF_Y_OFFSET);
#else
  (void) new wxMessage(inst_panel, "steps/count");
#endif
  inst_panel->NewLine();

  count_per_bar   = new wxSlider(inst_panel, (wxFunction)ItemCallback, "", 4, 1, 16, w/6, 10, 1*h/12, wxFIXED_LENGTH);
#ifdef wx_motif
  (void) new wxMessage(inst_panel, "count/bar", -1, (1*h/12)+MOTIF_Y_OFFSET);
#else
  (void) new wxMessage(inst_panel, "count/bar");
#endif
  inst_panel->NewLine();

  n_bars          = new wxSlider(inst_panel, (wxFunction)ItemCallback, "", 4, 1, 16, w/6, 10, 2*h/12, wxFIXED_LENGTH);
#ifdef wx_motif
  (void) new wxMessage(inst_panel, "# bars", -1, (2*h/12)+MOTIF_Y_OFFSET);
#else
  (void) new wxMessage(inst_panel, "# bars");
#endif
  inst_panel->NewLine();
#endif


  inst_panel->SetLabelPosition(wxVERTICAL);
  instrument_list = new wxListBox(inst_panel, (wxFunction)SelectInstr, "Instrument", wxLB_SINGLE /* | wxLB_ALWAYS_SB */ , -1, -1, 220, 80);
  inst_panel->NewLine();
#if 0
  (void)new wxButton(inst_panel, (wxFunction)Add, "add") ;
  (void)new wxButton(inst_panel, (wxFunction)Del, "del") ;
  (void)new wxButton(inst_panel, (wxFunction)Generate, "gen") ;
#endif

  // RndArray Edits
                                                   //    x    y      w    h
  length_edit = new tArrayEdit   (this,  edit.length,    x,   y+h/2, w/2, h/4-4);
  length_edit->SetXMinMax(1, 8);
  length_edit->SetLabel("length/interval");

  veloc_edit = new tArrayEdit    (this,  edit.veloc,     x+w/2, y+h/2, w/2, h/4-4);
  veloc_edit->SetXMinMax(1, 127);
  veloc_edit->SetLabel("velocity");

  rhythm_edit = new tRhyArrayEdit(this,  edit.rhythm,     x, y+3*h/4, w, h/4-4);
  rhythm_edit->SetMeter(edit.steps_per_count, edit.count_per_bar, edit.n_bars);
  rhythm_edit->SetLabel("rhythm");

  // group panel

  group_panel = new wxPanel(this, x+w/2, y, w/2, h/2, 0, "GroupPanel");

  group_panel->SetLabelPosition(wxHORIZONTAL);

#ifdef __WXMSW__

  group_contrib   = new wxSlider(group_panel, (wxFunction)ItemCallback, "", 0, 0, 100, w/3);
  (void) new wxMessage(group_panel, "contrib");
  group_panel->NewLine();

  group_listen = new wxSlider(group_panel, (wxFunction)ItemCallback, "", 0, -100, 100, w/3);
  (void) new wxMessage(group_panel, "listen");
  group_panel->NewLine();

#else

  group_contrib   = new wxSlider(group_panel, (wxFunction)ItemCallback, "", 0, 0, 100, w/6, 10,      1, wxFIXED_LENGTH);
#ifdef wx_motif
  (void) new wxMessage(group_panel, "contrib", -1, MOTIF_Y_OFFSET);
#else
  (void) new wxMessage(group_panel, "contrib");
#endif
  group_panel->NewLine();

  group_listen = new wxSlider(group_panel, (wxFunction)ItemCallback, "", 0, -100, 100, w/6, 10, 1*h/12, wxFIXED_LENGTH);
#ifdef wx_motif
  (void) new wxMessage(group_panel, "listen", -1, (1*h/12)+MOTIF_Y_OFFSET);
#else
  (void) new wxMessage(group_panel, "listen");
#endif
  group_panel->NewLine();

#endif

  group_panel->SetLabelPosition(wxVERTICAL);
  group_list = new wxListBox(group_panel, (wxFunction)SelectGroup, "Group", wxLB_SINGLE /* | wxLB_ALWAYS_SB */ , -1, -1, 220, 80);
  group_panel->NewLine();

  {
    char buf[100];
    int i;
    for (i = 0; i < MAX_GROUPS; i++)
    {
      sprintf(buf, "group %d", i+1);
      group_list->Append(buf);
    }
  }
  act_group = group_list->GetSelection();

  rand_checkbox = new wxCheckBox(group_panel, (wxFunction)ItemCallback, "Randomize") ;

  in_create = 0;

  Show(TRUE);
#endif

}

void tRhythmWin::OnSize(int w, int h)
{
 // wxFrame::OnSize(w, h);
  if (!in_create && mpToolBar)
  {
    int cw, ch;
    GetClientSize(&cw, &ch);
    float tw = 0.0;
    float th = 0.0;
#ifdef OBSOLETE
    mpToolBar->GetMaxSize(&tw, &th);
    mpToolBar->SetSize(0, 0, (int)cw, (int)th);
#endif
  }
}

void tRhythmWin::OnMenuCommand(int id) {
  switch (id) {
    case MEN_HELP: Help(); break;
    case MEN_CLOSE:
        // motif crashes, when Show(FALSE) is called before destructor!
  	// Show(FALSE);
//        DELETE_THIS();
        Destroy();
	break;

    case MEN_LOAD:
      {
        wxString fname = file_selector(
          default_filename,
          "Load Rhythm",
          false,
          has_changed,
          "*.rhy");
	if (fname)
        {
	  ifstream is(fname);
	  is >> *this;
          OnPaint();
	}
      }
      break;

    case MEN_SAVE:
      {
        Win2Instrument();
        wxString fname = file_selector(
          default_filename,
          "Save Rhythm",
          true,
          has_changed,
          "*.rhy");
	if (fname)
        {
	  ofstream os(fname);
	  os << *this;
	}
      }
      break;

    case MEN_ADD:
      AddInstrumentDlg();
      break;
    case MEN_DEL:
      DelInstrument();
      break;
    case MEN_GEN:
      wxBeginBusyCursor();
      Win2Instrument();
      GenRhythm();
      wxEndBusyCursor();
      break;
    case MEN_UP:
      UpInstrument();
      break;
    case MEN_DOWN:
      DownInstrument();
      break;

  }
}

void tRhythmWin::SelectInstr(wxListBox& list, wxCommandEvent& event)
{
  tRhythmWin *win = (tRhythmWin *)list.GetParent()->GetParent();
  win->Win2Instrument();
  win->act_instrument = win->instrument_list->GetSelection();
  win->Instrument2Win();
  win->OnPaint();
}

void tRhythmWin::SelectGroup(wxListBox& list, wxCommandEvent& event)
{
  tRhythmWin *win = (tRhythmWin *)list.GetParent()->GetParent();
  win->Win2Instrument();
  win->act_group = list.GetSelection();
  win->Instrument2Win();
}

void tRhythmWin::Add(wxButton &but, wxCommandEvent& event)
{
  tRhythmWin *win = (tRhythmWin *)but.GetParent()->GetParent();
  win->AddInstrumentDlg();
}

void tRhythmWin::AddInstrumentDlg()
{
  if (n_instruments >= MAX_INSTRUMENTS)
    return;

  int i, n = 0;
  wxString names[150];
  int keys[150];

  names[n] = "Controller";
  keys[n++] = MODE_CONTROL;

#if 0
  if (the_harmony_browser && the_harmony_browser->SeqDefined())
  {
    names[n] = "harmony: chords";
    keys[n++] = CHORD_KEY;
    names[n] = "harmony: bass";
    keys[n++] = BASS_KEY;
  }
#endif

  names[n] = "pianowin all";
  keys[n++] = MODE_ALL_OF;
  names[n] = "pianowin one";
  keys[n++] = MODE_ONE_OF;

  for (i = 0; Config.DrumName(i).Name; i++)
  {
    if (Config.DrumName(i).Name[0])
    {
      keys[n]    = Config.DrumName(i).Value - 1;
      names[n++] = Config.DrumName(i).Name;
    }
  }

  i = ::wxGetSingleChoiceIndex(
    "Instrument",
    "Select an instrument",
    n,
    names);

  if (i >= 0)
  {
    Win2Instrument(); // save actual values

    tRhythm *r = 0;
    if (act_instrument >= 0)
      r = new tRhythm(*instruments[act_instrument]);
    else
      r = new tRhythm(keys[i]);

    // drum key?
    if (keys[i] >= 0) {
      r->n_keys  = 1;
      r->keys[0] = keys[i];
      r->mode    = MODE_ALL_OF;
      r->SetLabel(names[i]);
    }

    // choose controller?
    else if (keys[i] == MODE_CONTROL)
    {
      r->parm = SelectControllerDlg();
      if (r->parm < 0)
        return;
      r->SetLabel(Config.CtrlName(r->parm).Name);
      r->mode = MODE_CONTROL;
      r->n_keys = 0;
    }

    else if (keys[i] == MODE_ONE_OF || keys[i] == MODE_ALL_OF)
    {
      char buf[2000];
      buf[0] = 0;
      if (keys[i] == MODE_ONE_OF)
        strcpy(buf, "one: ");
      else
        strcpy(buf, "all: ");
      r->n_keys = 0;
      r->mode   = keys[i];
      tEventArray events;
      tCmdCopyToBuffer cmd(TrackWin->GetPianoWindow()->mpFilter, &events);
      cmd.Execute(0);	// no UNDO

      for (int ii = 0; ii < events.nEvents; ii++)
      {
	tKeyOn *on = events.Events[ii]->IsKeyOn();
	if (on) {
	  r->keys[r->n_keys++] = on->Key;
	  if (r->n_keys > 1)
	    strcat(buf, ", ");
	  Key2Str(on->Key, buf + strlen(buf));
	  if (r->n_keys >= MAX_KEYS)
	    break;
	}
      }
      r->SetLabel(buf);

      if (r->n_keys == 0) {
        wxMessageBox("select some notes in pianowin first", "Error", wxOK);
	delete r;
	r = 0;
      }
    }

    if (r != 0)
      AddInstrument(r);
  }
}

void tRhythmWin::AddInstrument(tRhythm *r)
{
  act_instrument = n_instruments++;
  instruments[act_instrument] = r;
  instrument_list->Append((char *)r->GetLabel());

  instrument_list->SetSelection(act_instrument);
  Instrument2Win();
  OnPaint();
}


void tRhythmWin::UpInstrument()
{
  if (act_instrument >= 1)
  {
    tRhythm *tmp = instruments[act_instrument];
    instruments[act_instrument] = instruments[act_instrument-1];
    instruments[act_instrument-1] = tmp;
    act_instrument--;
    InitInstrumentList();
  }
}

void tRhythmWin::DownInstrument()
{
  if (act_instrument >= 0 && act_instrument < n_instruments-1)
  {
    tRhythm *tmp = instruments[act_instrument];
    instruments[act_instrument] = instruments[act_instrument+1];
    instruments[act_instrument+1] = tmp;
    act_instrument++;
    InitInstrumentList();
  }
}

void tRhythmWin::InitInstrumentList()
{
  instrument_list->Clear();
  for (int i = 0; i < n_instruments; i++)
    instrument_list->Append((char *)instruments[i]->GetLabel());
  if (act_instrument >= 0)
    instrument_list->SetSelection(act_instrument);
}

void tRhythmWin::Del(wxButton &but, wxCommandEvent& event)
{
  tRhythmWin *win = (tRhythmWin *)but.GetParent()->GetParent();
  win->DelInstrument();
}


void tRhythmWin::DelInstrument()
{
  int i = act_instrument;
  if (i >= 0)
  {
    int k;
    delete instruments[i];
    for (k = i; k < n_instruments-1; k++)
      instruments[k] = instruments[k+1];
    instruments[k] = 0;
    n_instruments--;
    instrument_list->Delete(i);
    act_instrument = instrument_list->GetSelection();
    Instrument2Win();
    OnPaint();
  }
}


void tRhythmWin::Generate(wxButton &but, wxCommandEvent& event)
{
  wxBeginBusyCursor();
  tRhythmWin *win = (tRhythmWin *)but.GetParent()->GetParent();
  win->Win2Instrument();
  win->GenRhythm();
  wxEndBusyCursor();
}


void tRhythmWin::GenRhythm()
{
  if (!event_win->EventsSelected("please mark destination track in trackwin"))
    return;

  tFilter* pFilter = event_win->mpFilter;

  if (pFilter->FromTrack != pFilter->ToTrack)
  {
    wxMessageBox("you must select exacty 1 track", "Error", wxOK);
    return;
  }

  long fr_clock = pFilter->FromClock;
  long to_clock = pFilter->ToClock;
  tTrack *track = song->GetTrack(pFilter->FromTrack);
  song->NewUndoBuffer();

  // remove selection
  //if (wxMessageBox("Erase destination before generating?", "Replace", wxYES_NO) == wxYES)
  {
    tCmdErase erase(pFilter, 1);
    erase.Execute(0);
  }

  for (int i = 0; i < n_instruments; i++)
    instruments[i]->GenInit(fr_clock);

  JZBarInfo bar_info(song);
  bar_info.SetClock(fr_clock);

  // for (int i = 0; i < n_instruments; i++)
  //   instruments[i]->Generate(track, fr_clock, to_clock, bar_info.TicksPerBar);

  while (bar_info.Clock < to_clock) {
    for (int i = 0; i < n_instruments; i++)
      instruments[i]->Generate(track, bar_info, instruments, n_instruments);
    bar_info.Next();
  }

  track->Cleanup();

  event_win->Redraw();
}


void tRhythmWin::Help()
{
  gpHelpInstance->ShowTopic("Random rhythm generator");
}

#ifdef OBSOLETE

void tRhythmWin::ItemCallback(wxItem& item, wxCommandEvent& event)
{
  tRhythmWin *win = (tRhythmWin *)item.GetParent()->GetParent();
  win->Win2Instrument();
  win->RndEnable();
  win->OnPaint();
}
#endif



void tRhythmWin::Win2Instrument(int i)
{
  if (in_create)
    return;

  if (i < 0)
    i = act_instrument;
  if (i < 0)
    return;

  edit.steps_per_count = steps_per_count->GetValue();
  edit.count_per_bar   = count_per_bar->GetValue();
  edit.n_bars          = n_bars->GetValue();
  edit.randomize       = rand_checkbox->GetValue();

  if (act_group >= 0) {
    edit.groups[act_group].listen = group_listen->GetValue();
    edit.groups[act_group].contrib = group_contrib->GetValue();
  }

  *instruments[i] = edit;
}


void tRhythmWin::Instrument2Win(int i)
{
  if (in_create)
    return;

  if (i < 0)
    i = act_instrument;
  if (i < 0)
    return;

  edit = *instruments[i];
  steps_per_count->SetValue(edit.steps_per_count);
  count_per_bar->SetValue(edit.count_per_bar);
  n_bars->SetValue(edit.n_bars);
  rhythm_edit->SetMeter(edit.steps_per_count, edit.count_per_bar, edit.n_bars);
  rand_checkbox->SetValue((bool)edit.randomize);

  switch(edit.mode)
  {
    case MODE_CONTROL:
      veloc_edit->SetLabel("ctrl value");
      break;
    default:
      veloc_edit->SetLabel("velocity");
      break;
  }

  if (act_group >= 0) {
    group_listen->SetValue(edit.groups[act_group].listen);
    group_contrib->SetValue(edit.groups[act_group].contrib);
  }

  RndEnable();
}


void tRhythmWin::RndEnable()
{
  length_edit->Enable(edit.randomize);
  veloc_edit->Enable(edit.randomize);
  group_listen->Enable(edit.randomize);
}


tRhythmWin::~tRhythmWin()
{
  GetPosition( &Config(C_RhythmXpos), &Config(C_RhythmYpos) );
  for (int i = 0; i < n_instruments; i++)
    delete instruments[i];
  delete mpToolBar;
  rhythm_win = 0;
}

#ifndef __PORTING
bool tRhythmWin::OnClose()
{
  return TRUE;
}
#endif // __PORTING


void tRhythmWin::OnPaint()
{
  if (in_create)
    return;

  rhythm_edit->SetMeter(edit.steps_per_count, edit.count_per_bar, edit.n_bars);

  length_edit->Refresh();
  veloc_edit->Refresh();
  rhythm_edit->Refresh();
}

ostream & operator << (ostream &os, tRhythmWin const &a)
{
  int i;
  os << 2 << endl;
  os << a.n_instruments << endl;
  for (i = 0; i < a.n_instruments; i++)
    a.instruments[i]->write(os);
  return os;
}

istream & operator >> (istream &is, tRhythmWin &a)
{
  int version;
  is >> version;
  if (version > 2)
  {
    wxMessageBox("Wrong file format!", "Error", wxOK);
    return is;
  }

  int i, n = a.n_instruments;
  for (i = 0; i < n; i++)
  {
    a.act_instrument = 0;
    a.DelInstrument();
  }

  is >> n;
  for (i = 0; i < n; i++)
  {
    tRhythm *r = new tRhythm(0);
    r->read(is, version);
    a.AddInstrument(r);
  }
  return is;
}

