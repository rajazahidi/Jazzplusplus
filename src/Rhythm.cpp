//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Copyright (C) 1994-2000 Andreas Voss and Per Sigmond, all rights reserved.
// Modifications Copyright (C) 2004 Patrick Earl
// Modifications Copyright (C) 2008-2013 Peter J. Stieber
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

#include "Rhythm.h"

#include "Command.h"
#include "EventWindow.h"
#include "FileSelector.h"
#include "Filter.h"
#include "Globals.h"
#include "Harmony.h"
#include "Help.h"
#include "KeyStringConverters.h"
#include "PianoWindow.h"
#include "Resources.h"
#include "SelectControllerDialog.h"
#include "Song.h"
#include "StringReadWrite.h"
#include "ToolBar.h"
#include "TrackFrame.h"

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choicdlg.h>
#include <wx/listbox.h>
#include <wx/menu.h>
#include <wx/msgdlg.h>
#include <wx/slider.h>
#include <wx/toolbar.h>

#include <fstream>
#include <sstream>

using namespace std;

#include "Bitmaps/open.xpm"
#include "Bitmaps/save.xpm"
#include "Bitmaps/rrgadd.xpm"
#include "Bitmaps/rrgdel.xpm"
#include "Bitmaps/rrgup.xpm"
#include "Bitmaps/rrgdown.xpm"
#include "Bitmaps/rrggen.xpm"
#include "Bitmaps/help.xpm"

void tRhyGroup::write(ostream& Os) const
{
  Os << listen << ' ';
  Os << contrib << ' ';
}

void tRhyGroup::read(istream& Is, int version)
{
  Is >> listen;
  Is >> contrib;
}

void JZRhythmGroups::write(ostream& Os) const
{
  for (int i = 0; i < MAX_GROUPS; i++)
  {
    g[i].write(Os);
  }
  Os << endl;
}

void JZRhythmGroups::read(istream& Is, int version)
{
  for (int i = 0; i < MAX_GROUPS; i++)
  {
    g[i].read(Is, version);
  }
}



// pseudo key nr's for harmony browser and sound effects
static const int MODE_ALL_OF    = -1;
static const int MODE_ONE_OF    = -2;
static const int MODE_PIANO     = -3;
static const int MODE_CONTROL   = -4;


JZRhythm::JZRhythm(int k)
  : mLabel("random rhythm"),
    rhythm(64, 0, 100),
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
}

JZRhythm::JZRhythm(const JZRhythm& Other)
  : rhythm(Other.rhythm),
    length(Other.length),
    veloc (Other.veloc),
    groups(Other.groups),
    history(Other.history)
{
  mode = Other.mode;
  n_keys = Other.n_keys;
  for (int i = 0; i < n_keys; i++)
  {
    keys[i] = Other.keys[i];
  }
  parm = Other.parm;
  n_bars = Other.n_bars;
  steps_per_count = Other.steps_per_count;
  count_per_bar = Other.count_per_bar;
  randomize = Other.randomize;
  groups = Other.groups;

  mLabel = Other.mLabel;
}

JZRhythm& JZRhythm::operator = (const JZRhythm& Rhs)
{
  if (this != &Rhs)
  {
    mode            = Rhs.mode;
    n_keys          = Rhs.n_keys;
    for (int i = 0; i < n_keys; i++)
    {
      keys[i] = Rhs.keys[i];
    }
    rhythm          = Rhs.rhythm;
    length          = Rhs.length;
    veloc           = Rhs.veloc;
    parm            = Rhs.parm;
    n_bars          = Rhs.n_bars;
    steps_per_count = Rhs.steps_per_count;
    count_per_bar   = Rhs.count_per_bar;
    randomize       = Rhs.randomize;
    groups          = Rhs.groups;
    history         = Rhs.history;

    mLabel = Rhs.mLabel;
  }

  return *this;
}


JZRhythm::~JZRhythm()
{
}


void JZRhythm::write(ostream& Os) const
{
  Os << rhythm;
  Os << length;
  Os << veloc;

  Os << steps_per_count << ' ';
  Os << count_per_bar << ' ';
  Os << n_bars << ' ';
  Os << mode << ' ';
  Os << n_keys << ' ';
  for (int i = 0; i < n_keys; i++)
  {
    Os << keys[i] << ' ';
  }
  Os << parm << endl;
  WriteString(Os, mLabel.c_str()) << endl;

  Os << randomize << ' ';
  groups.write(Os);
}


void JZRhythm::read(istream& Is, int version)
{
  Is >> rhythm;
  Is >> length;
  Is >> veloc;

  Is >> steps_per_count;
  Is >> count_per_bar;
  Is >> n_bars;
  Is >> mode;
  if (mode >= 0) // old format
  {
    n_keys = 1;
    keys[0] = mode;
    mode = MODE_ALL_OF;
  }
  else
  {
    Is >> n_keys;
    for (int i = 0; i < n_keys; i++)
    {
      Is >> keys[i];
    }
  }
  Is >> parm;

  string Label;
  ReadString(Is, Label);
  SetLabel(Label.c_str());

  if (version > 1)
  {
    Is >> randomize;
    groups.read(Is, version);
  }
}

void JZRhythm::SetLabel(const string& Label)
{
  mLabel = Label;
}


int JZRhythm::Clock2i(long clock, const JZBarInfo& BarInfo) const
{
  int clocks_per_step = BarInfo.GetTicksPerBar() / (steps_per_count * count_per_bar);
  return (int)(((clock - start_clock) / clocks_per_step) % rhythm.Size());
}

int JZRhythm::ClocksPerStep(const JZBarInfo& BarInfo) const
{
  return BarInfo.GetTicksPerBar() / (steps_per_count * count_per_bar);
}


void JZRhythm::GenInit(long frc)
{
  int i;
  start_clock = frc;
  next_clock  = frc;

  int nn = rhythm.Size();
  history.Resize(nn);

  // initialize history with random values
  for (i = 0; i < nn; i++)
  {
    history[i] = history.Min();
  }

  for (i = 0; i < nn; i++)
  {
    if (rhythm.Random(i))
    {
      history[i] = history.Max();
      i += length.Random();
    }
  }
}


void JZRhythm::GenerateEvent(JZTrack* pTrack, long clock, short vel, short len)
{
  int chan = pTrack->mChannel - 1;

  // generate key events
  if (mode == MODE_ALL_OF)
  {
    for (int ii = 0; ii < n_keys; ii++)
    {
      JZKeyOnEvent *k = new JZKeyOnEvent(clock, chan, keys[ii], vel, len);
      pTrack->Put(k);
    }
  }
  else if (mode == MODE_ONE_OF)
  {
    int ii = (int)(rnd.asDouble() * n_keys);
    if (ii < n_keys)
    {
      JZKeyOnEvent *k = new JZKeyOnEvent(clock, chan, keys[ii], vel, len);
      pTrack->Put(k);
    }
  }
  else if (mode == MODE_CONTROL)
  {
    // generate controller
    JZControlEvent* c = new JZControlEvent(clock, chan, parm - 1, vel);
    pTrack->Put(c);
  }
  else
  {
    assert(0);
  }
}


#if 0
void JZRhythm::Generate(JZTrack* pTrack, long fr_clock, long to_clock, long ticks_per_bar)
{
  int chan   = pTrack->Channel - 1;
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
      {
        // keep seed < 1.0
        rndval = veloc.Random((double)rhythm[i] / ((double)rhythm.Max() + 0.001));
      }
      else
      {
        rndval = veloc.Random();
      }
      short vel = rndval * 127 / veloc.Size() + 1;
      short len = (length.Random() + 1) * clocks_per_step;

      // generate keys from harmony browser
      if (key == CHORD_KEY || key == BASS_KEY)
      {
        if (gpHarmonyBrowser)
        {
          long step = (clock - fr_clock) * total_steps / (to_clock - fr_clock);
          int keys[12], n_keys;
          if (key == CHORD_KEY)
          {
            n_keys = gpHarmonyBrowser->GetChordKeys(keys, (int)step, (int)total_steps);
          }
          else
          {
            n_keys = gpHarmonyBrowser->GetBassKeys(keys, (int)step, (int)total_steps);
          }
          for (int j = 0; j < n_keys; j++)
          {
            JZKeyOnEvent *k = new JZKeyOnEvent(clock, chan, keys[j], vel, len - clocks_per_step/2);
            pTrack->Put(k);
          }
        }
      }

      // paste pianowin buffer
      else if (key == PASTE_KEY)
      {
        JZEventArray &src = gpTrackWindow->GetPianoWindow()->PasteBuffer;
        for (int ii = 0; ii < src.mEventCount; ii++)
        {
          JZKeyOnEvent* pKeyOn = src.Events[ii]->IsKeyOn();
          if (pKeyOn)
          {
            JZKeyOnEvent *k = new JZKeyOnEvent(clock, chan, pKeyOn->Key, vel, len - clocks_per_step / 2);
            pTrack->Put(k);
          }
        }
      }

      // generate controller
      else if (key == CONTROL_KEY)
      {
        JZControlEvent* c = new JZControlEvent(clock, chan, parm - 1, vel);
        pTrack->Put(c);
      }
      // generate note on events
      else
      {
        JZKeyOnEvent *k = new JZKeyOnEvent(clock, chan, key, vel, len - clocks_per_step/2);
        pTrack->Put(k);
      }

      clock += len;
    }
    else
      clock += clocks_per_step;
  }
}
#endif



void JZRhythm::GenGroup(
  JZRndArray& out,
  int grp,
  const JZBarInfo& BarInfo,
  JZRhythm *rhy[],
  int n_rhy)
{
  out.Clear();

  int clocks_per_step = ClocksPerStep(BarInfo);

  for (int ri = 0; ri < n_rhy; ri++)
  {
    JZRhythm* pRhythm = rhy[ri];
    int fuzz = pRhythm->groups[grp].contrib;
    if (fuzz && pRhythm != this)
    {
      JZRndArray tmp(rhythm);
      tmp.Clear();
      long clock = BarInfo.GetClock();
      while (clock < BarInfo.GetClock() + BarInfo.GetTicksPerBar())
      {
        int i = Clock2i(clock, BarInfo);
        int j = pRhythm->Clock2i(clock, BarInfo);
        tmp[i] = pRhythm->history[j];
        clock += clocks_per_step;
      }
      out.SetUnion(tmp, fuzz);
    }
  }
}


void JZRhythm::Generate(
  JZTrack* pTrack,
  const JZBarInfo& BarInfo,
  JZRhythm* rhy[],
  int n_rhy)
{
  JZRndArray rrg(rhythm);

  // add groups to the rhythm
  JZRndArray tmp(rhythm);
  for (int gi = 0; gi < MAX_GROUPS; gi++)
  {
    if (groups[gi].listen)
    {
      GenGroup(tmp, gi, BarInfo, rhy, n_rhy);
      if (groups[gi].listen > 0)
      {
        rrg.SetIntersection(tmp, groups[gi].listen);
      }
      else
      {
        rrg.SetDifference(tmp, -groups[gi].listen);
      }
    }
  }

  // clear part of the history
  long clock = BarInfo.GetClock();
  int clocks_per_step = ClocksPerStep(BarInfo);
  while (clock < BarInfo.GetClock() + BarInfo.GetTicksPerBar())
  {
    int i = Clock2i(clock, BarInfo);
    history[i] = 0;
    clock += clocks_per_step;
  }

  //  generate the events
  clock = next_clock;
  while (clock < BarInfo.GetClock() + BarInfo.GetTicksPerBar())
  {
    int i = Clock2i(clock, BarInfo);
    if ((!randomize && rrg[i] > 0) || rrg.Random(i))
    {
      // put event here
      history[i] = rhythm.Max();

      short vel = 0;
      if (randomize)
      {
        vel = veloc.Random() * 127 / veloc.Size() + 1;
      }
      else
      {
        vel = rrg[i] * 126 / rrg.Max() + 1;
      }
      short len = (length.Random() + 1) * clocks_per_step;
      GenerateEvent(pTrack, clock, vel, len - clocks_per_step/2);
      clock += len;
    }
    else
    {
      clock += clocks_per_step;
    }
  }
  next_clock = clock;
}


// ============================ JZRhythmWindow ==============================


#define MEN_HELP  4


//#include "Bitmaps/open.xpm"
//#include "Bitmaps/save.xpm"
//#include "Bitmaps/rrgadd.xpm"
//#include "Bitmaps/rrgdel.xpm"
//#include "Bitmaps/rrgup.xpm"
//#include "Bitmaps/rrgdown.xpm"
//#include "Bitmaps/rrggen.xpm"
//#include "Bitmaps/help.xpm"

JZRhythmWindow::JZRhythmWindow(JZEventWindow* pEventWindow, JZSong* pSong)
  : wxFrame(
      0,
      wxID_ANY,
      "Random Rhythm Generator",
      wxPoint(
        gpConfig->GetValue(C_RhythmXpos),
        gpConfig->GetValue(C_RhythmYpos)),
      wxSize(640, 580)),
    edit(0),
    mpEventWindow(pEventWindow),
    mpSong(pSong),
    mDefaultFileName("noname.rhy")
{
#ifdef OBSOLETE
  in_create        = 1;
  n_instruments    = 0;
  act_instrument   = -1;
  has_changed      = false;

  JZToolDef tdefs[] =
  {
    { wxID_OPEN, FALSE, open_xpm,    "open rhythm file" },
    { wxID_SAVE, FALSE, save_xpm,    "save into rhythm file" },
    { JZToolBar::eToolBarSeparator },
    { ID_INSTRUMENT_ADD,      FALSE, rrgadd_xpm,  "add instrument" },
    { ID_INSTRUMENT_DELETE,   FALSE, rrgdel_xpm,  "remove instrument" },
    { ID_INSTRUMENT_UP,       FALSE, rrgup_xpm,   "move instrument up" },
    { ID_INSTRUMENT_DOWN,     FALSE, rrgdown_xpm, "move instrument down" },
    { ID_INSTRUMENT_GENERATE, FALSE, rrggen_xpm,  "generate events into trackwin selection" },
    { JZToolBar::eToolBarSeparator },
    { wxID_HELP_CONTENTS,     FALSE, help_xpm,    "help" },
    { JZToolBar::eToolBarEnd }
  };


  mpToolBar = new JZToolBar(this, tdefs);
  mpToolBar->GetMaxSize(&tb_width, &tb_height);

  steps_per_count = 0;
  count_per_bar   = 0;
  n_bars          = 0;
  instrument_list = 0;

  wxMenuBar* pMenuBar = new wxMenuBar;
  wxMenu* pMenu = new wxMenu;
  pMenu->Append(wxID_OPEN, "&Load");
  pMenu->Append(wxID_SAVE, "&Save");
  pMenu->Append(wxID_CLOSE, "&Close");
  pMenuBar->Append(pMenu, "&File");

  pMenu = new wxMenu;
  pMenu->Append(ID_INSTRUMENT_ADD, "&Add");
  pMenu->Append(ID_INSTRUMENT_DELETE, "&Delete");
  pMenu->Append(ID_INSTRUMENT_UP, "&Up");
  pMenu->Append(ID_INSTRUMENT_DOWN, "&Down");
  pMenu->Append(ID_INSTRUMENT_GENERATE, "&Generate");
  pMenuBar->Append(pMenu, "&Instrument");

  pMenu = new wxMenu;
  pMenu->Append(wxID_HELP, "&Help");
  pMenuBar->Append(pMenu,  "Help");

  SetMenuBar(pMenuBar);

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

  n_bars = new wxSlider(
    inst_panel,
    (wxFunction)ItemCallback,
    "",
    4,
    1,
    16,
    w / 6,
    10,
    2 * h / 12,
    wxFIXED_LENGTH);

#ifdef wx_motif
  (void) new wxMessage(inst_panel, "# bars", -1, (2*h/12)+MOTIF_Y_OFFSET);
#else
  (void) new wxMessage(inst_panel, "# bars");
#endif
  inst_panel->NewLine();
#endif

  inst_panel->SetLabelPosition(wxVERTICAL);
  instrument_list = new wxListBox(
    inst_panel,
    (wxFunction)SelectInstr,
    "Instrument",
    wxLB_SINGLE /* | wxLB_ALWAYS_SB */,
    -1,
    -1,
    220,
    80);

  inst_panel->NewLine();

#if 0
  (void)new wxButton(inst_panel, (wxFunction)Add, "add") ;
  (void)new wxButton(inst_panel, (wxFunction)Del, "del") ;
  (void)new wxButton(inst_panel, (wxFunction)Generate, "gen") ;
#endif

  // RndArray Edits
                                                   //    x    y      w    h
  length_edit = new JZArrayEdit   (this,  edit.length,    x,   y+h/2, w/2, h/4-4);
  length_edit->SetXMinMax(1, 8);
  length_edit->SetLabel("length/interval");

  veloc_edit = new JZArrayEdit    (this,  edit.veloc,     x+w/2, y+h/2, w/2, h/4-4);
  veloc_edit->SetXMinMax(1, 127);
  veloc_edit->SetLabel("velocity");

  rhythm_edit = new JZRhyArrayEdit(this,  edit.rhythm,     x, y+3*h/4, w, h/4-4);
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

void JZRhythmWindow::OnSize(int w, int h)
{
 // wxFrame::OnSize(w, h);
  if (!in_create && mpToolBar)
  {
    int cw, ch;
    GetClientSize(&cw, &ch);
#ifdef OBSOLETE
    float tw = 0.0;
    float th = 0.0;
    mpToolBar->GetMaxSize(&tw, &th);
    mpToolBar->SetSize(0, 0, (int)cw, (int)th);
#endif
  }
}

void JZRhythmWindow::OnMenuCommand(int id)
{
  switch (id)
  {
    case wxID_CLOSE:
        // motif crashes, when Show(FALSE) is called before destructor!
      // Show(FALSE);
//        DELETE_THIS();
        Destroy();
        break;

    case wxID_OPEN:
      {
        wxString FileName = file_selector(
          mDefaultFileName,
          "Load Rhythm",
          false,
          has_changed,
          "*.rhy");
        if (!FileName.empty())
        {
          ifstream Is(FileName.mb_str());
          Is >> *this;
          OnPaint();
        }
      }
      break;

    case wxID_SAVE:
      {
        Win2Instrument();
        wxString FileName = file_selector(
          mDefaultFileName,
          "Save Rhythm",
          true,
          has_changed,
          "*.rhy");
        if (!FileName.empty())
        {
          ofstream Os(FileName.mb_str());
          Os << *this;
        }
      }
      break;

    case ID_INSTRUMENT_ADD:
      AddInstrumentDlg();
      break;
    case ID_INSTRUMENT_DELETE:
      DelInstrument();
      break;
    case ID_INSTRUMENT_GENERATE:
      wxBeginBusyCursor();
      Win2Instrument();
      GenRhythm();
      wxEndBusyCursor();
      break;
    case ID_INSTRUMENT_UP:
      UpInstrument();
      break;
    case ID_INSTRUMENT_DOWN:
      DownInstrument();
      break;
  }
}

void JZRhythmWindow::SelectInstr(wxListBox& list, wxCommandEvent& event)
{
  JZRhythmWindow *win = (JZRhythmWindow *)list.GetParent()->GetParent();
  win->Win2Instrument();
  win->act_instrument = win->instrument_list->GetSelection();
  win->Instrument2Win();
  win->OnPaint();
}

void JZRhythmWindow::SelectGroup(wxListBox& list, wxCommandEvent& event)
{
  JZRhythmWindow *win = (JZRhythmWindow *)list.GetParent()->GetParent();
  win->Win2Instrument();
  win->act_group = list.GetSelection();
  win->Instrument2Win();
}

void JZRhythmWindow::Add(wxButton &but, wxCommandEvent& event)
{
  JZRhythmWindow *win = (JZRhythmWindow *)but.GetParent()->GetParent();
  win->AddInstrumentDlg();
}

void JZRhythmWindow::AddInstrumentDlg()
{
  if (n_instruments >= MAX_INSTRUMENTS)
  {
    return;
  }

  int i, n = 0;
  wxString names[150];
  int keys[150];

  names[n] = "Controller";
  keys[n++] = MODE_CONTROL;

#if 0
  if (gpHarmonyBrowser && gpHarmonyBrowser->SeqDefined())
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

  const vector<pair<string, int> >& DrumNames = gpConfig->GetDrumNames();
  for (
    vector<pair<string, int> >::const_iterator iDrumName = DrumNames.begin();
    iDrumName != DrumNames.end();
    ++iDrumName)
  {
    const string& Name = iDrumName->first;
    if (!Name.empty())
    {
      keys[n]    = iDrumName->second - 1;
      names[n++] = Name;
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

    JZRhythm* pRhythm = 0;
    if (act_instrument >= 0)
    {
      pRhythm = new JZRhythm(*instruments[act_instrument]);
    }
    else
    {
      pRhythm = new JZRhythm(keys[i]);
    }

    // drum key?
    if (keys[i] >= 0)
    {
      pRhythm->n_keys  = 1;
      pRhythm->keys[0] = keys[i];
      pRhythm->mode    = MODE_ALL_OF;
      pRhythm->SetLabel(names[i]);
    }

    // choose controller?
    else if (keys[i] == MODE_CONTROL)
    {
      pRhythm->parm = SelectControllerDlg();
      if (pRhythm->parm < 0)
      {
        return;
      }
      pRhythm->SetLabel(gpConfig->GetCtrlName(pRhythm->parm).first.c_str());
      pRhythm->mode = MODE_CONTROL;
      pRhythm->n_keys = 0;
    }

    else if (keys[i] == MODE_ONE_OF || keys[i] == MODE_ALL_OF)
    {
      ostringstream Oss;
      if (keys[i] == MODE_ONE_OF)
      {
        Oss << "one: ";
      }
      else
      {
        Oss << "all: ";
      }
      pRhythm->n_keys = 0;
      pRhythm->mode   = keys[i];
      JZEventArray events;
      JZCommandCopyToBuffer cmd(gpTrackFrame->GetPianoWindow()->GetFilter(), &events);
      cmd.Execute(0);   // no UNDO

      for (int ii = 0; ii < events.mEventCount; ii++)
      {
        JZKeyOnEvent* pKeyOn = events.mppEvents[ii]->IsKeyOn();
        if (pKeyOn)
        {
          pRhythm->keys[pRhythm->n_keys++] = pKeyOn->GetKey();
          if (pRhythm->n_keys > 1)
          {
            Oss << ", ";
          }
          string KeyString;
          KeyToString(pKeyOn->GetKey(), KeyString);
          Oss << KeyString;
          if (pRhythm->n_keys >= MAX_KEYS)
          {
            break;
          }
        }
      }
      pRhythm->SetLabel(Oss.str());

      if (pRhythm->n_keys == 0)
      {
        wxMessageBox("select some notes in pianowin first", "Error", wxOK);
        delete pRhythm;
        pRhythm = 0;
      }
    }

    if (pRhythm != 0)
    {
      AddInstrument(pRhythm);
    }
  }
}

void JZRhythmWindow::AddInstrument(JZRhythm* pRhythm)
{
  act_instrument = n_instruments++;
  instruments[act_instrument] = pRhythm;
  instrument_list->Append(pRhythm->GetLabel().c_str());

  instrument_list->SetSelection(act_instrument);
  Instrument2Win();
  OnPaint();
}


void JZRhythmWindow::UpInstrument()
{
  if (act_instrument >= 1)
  {
    JZRhythm *tmp = instruments[act_instrument];
    instruments[act_instrument] = instruments[act_instrument-1];
    instruments[act_instrument-1] = tmp;
    act_instrument--;
    InitInstrumentList();
  }
}

void JZRhythmWindow::DownInstrument()
{
  if (act_instrument >= 0 && act_instrument < n_instruments-1)
  {
    JZRhythm *tmp = instruments[act_instrument];
    instruments[act_instrument] = instruments[act_instrument+1];
    instruments[act_instrument+1] = tmp;
    act_instrument++;
    InitInstrumentList();
  }
}

void JZRhythmWindow::InitInstrumentList()
{
  instrument_list->Clear();
  for (int i = 0; i < n_instruments; i++)
  {
    instrument_list->Append(instruments[i]->GetLabel().c_str());
  }
  if (act_instrument >= 0)
  {
    instrument_list->SetSelection(act_instrument);
  }
}

void JZRhythmWindow::Del(wxButton &but, wxCommandEvent& event)
{
  JZRhythmWindow *win = (JZRhythmWindow *)but.GetParent()->GetParent();
  win->DelInstrument();
}


void JZRhythmWindow::DelInstrument()
{
  int i = act_instrument;
  if (i >= 0)
  {
    int k;
    delete instruments[i];
    for (k = i; k < n_instruments-1; k++)
    {
      instruments[k] = instruments[k+1];
    }
    instruments[k] = 0;
    n_instruments--;
    instrument_list->Delete(i);
    act_instrument = instrument_list->GetSelection();
    Instrument2Win();
    OnPaint();
  }
}


void JZRhythmWindow::Generate(wxButton &but, wxCommandEvent& event)
{
  wxBeginBusyCursor();
  JZRhythmWindow *win = (JZRhythmWindow *)but.GetParent()->GetParent();
  win->Win2Instrument();
  win->GenRhythm();
  wxEndBusyCursor();
}


void JZRhythmWindow::GenRhythm()
{
  if (
    !mpEventWindow->EventsSelected(
      "Please mark the destination track in the track window"))
  {
    return;
  }

  JZFilter* pFilter = mpEventWindow->mpFilter;

  if (pFilter->GetFromTrack() != pFilter->GetToTrack())
  {
    wxMessageBox("you must select exacty 1 track", "Error", wxOK);
    return;
  }

  long fr_clock = pFilter->GetFromClock();
  long to_clock = pFilter->GetToClock();
  JZTrack* pTrack = mpSong->GetTrack(pFilter->GetFromTrack());
  mpSong->NewUndoBuffer();

  // remove selection
//  if (
//    wxMessageBox(
//      "Erase destination before generating?",
//      "Replace",
//      wxYES_NO) == wxYES)
  {
    JZCommandErase erase(pFilter, 1);
    erase.Execute(0);
  }

  for (int i = 0; i < n_instruments; i++)
  {
    instruments[i]->GenInit(fr_clock);
  }

  JZBarInfo BarInfo(*mpSong);
  BarInfo.SetClock(fr_clock);

//  for (int i = 0; i < n_instruments; i++)
//  {
//    instruments[i]->Generate(pTrack, fr_clock, to_clock, BarInfo.GetTicksPerBar());
//  }

  while (BarInfo.GetClock() < to_clock)
  {
    for (int i = 0; i < n_instruments; i++)
    {
      instruments[i]->Generate(pTrack, BarInfo, instruments, n_instruments);
    }
    BarInfo.Next();
  }

  pTrack->Cleanup();

  mpEventWindow->Refresh();
}

#ifdef OBSOLETE

void JZRhythmWindow::ItemCallback(wxItem& item, wxCommandEvent& event)
{
  JZRhythmWindow *win = (JZRhythmWindow *)item.GetParent()->GetParent();
  win->Win2Instrument();
  win->RndEnable();
  win->OnPaint();
}
#endif



void JZRhythmWindow::Win2Instrument(int i)
{
  if (in_create)
  {
    return;
  }

  if (i < 0)
  {
    i = act_instrument;
  }
  if (i < 0)
  {
    return;
  }

  edit.steps_per_count = steps_per_count->GetValue();
  edit.count_per_bar   = count_per_bar->GetValue();
  edit.n_bars          = n_bars->GetValue();
  edit.randomize       = rand_checkbox->GetValue();

  if (act_group >= 0)
  {
    edit.groups[act_group].listen = group_listen->GetValue();
    edit.groups[act_group].contrib = group_contrib->GetValue();
  }

  *instruments[i] = edit;
}


void JZRhythmWindow::Instrument2Win(int i)
{
  if (in_create)
  {
    return;
  }

  if (i < 0)
  {
    i = act_instrument;
  }
  if (i < 0)
  {
    return;
  }

  edit = *instruments[i];
  steps_per_count->SetValue(edit.steps_per_count);
  count_per_bar->SetValue(edit.count_per_bar);
  n_bars->SetValue(edit.n_bars);
  rhythm_edit->SetMeter(edit.steps_per_count, edit.count_per_bar, edit.n_bars);
  rand_checkbox->SetValue((bool)edit.randomize);

  switch (edit.mode)
  {
    case MODE_CONTROL:
      veloc_edit->SetLabel("ctrl value");
      break;
    default:
      veloc_edit->SetLabel("velocity");
      break;
  }

  if (act_group >= 0)
  {
    group_listen->SetValue(edit.groups[act_group].listen);
    group_contrib->SetValue(edit.groups[act_group].contrib);
  }

  RndEnable();
}


void JZRhythmWindow::RndEnable()
{
  length_edit->Enable(edit.randomize);
  veloc_edit->Enable(edit.randomize);
  group_listen->Enable(edit.randomize);
}


JZRhythmWindow::~JZRhythmWindow()
{
  int XPixel, YPixel;
  GetPosition(&XPixel, &YPixel);
  gpConfig->Put(C_RhythmXpos, XPixel);
  gpConfig->Put(C_RhythmYpos, YPixel);

  for (int i = 0; i < n_instruments; i++)
  {
    delete instruments[i];
  }
  delete mpToolBar;
}

bool JZRhythmWindow::OnClose()
{
  return true;
}


void JZRhythmWindow::OnPaint()
{
  if (in_create)
  {
    return;
  }

  rhythm_edit->SetMeter(edit.steps_per_count, edit.count_per_bar, edit.n_bars);

  length_edit->Refresh();
  veloc_edit->Refresh();
  rhythm_edit->Refresh();
}

ostream & operator << (ostream& Os, JZRhythmWindow const &a)
{
  int i;
  Os << 2 << endl;
  Os << a.n_instruments << endl;
  for (i = 0; i < a.n_instruments; i++)
  {
    a.instruments[i]->write(Os);
  }
  return Os;
}

istream & operator >> (istream& Is, JZRhythmWindow& a)
{
  int version;
  Is >> version;
  if (version > 2)
  {
    wxMessageBox("Wrong file format!", "Error", wxOK);
    return Is;
  }

  int i, n = a.n_instruments;
  for (i = 0; i < n; i++)
  {
    a.act_instrument = 0;
    a.DelInstrument();
  }

  Is >> n;
  for (i = 0; i < n; i++)
  {
    JZRhythm* pRhythm = new JZRhythm(0);
    pRhythm->read(Is, version);
    a.AddInstrument(pRhythm);
  }
  return Is;
}

//*****************************************************************************
//*****************************************************************************
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZRhythmGeneratorWindow::JZRhythmGeneratorWindow(
  wxFrame* pParent,
  const wxPoint& Position,
  const wxSize& Size)
  : wxWindow(pParent, wxID_ANY, Position, Size),
    mRhythm(0),
    mpLengthEdit(0),
    mpVelocityEdit(0),
    mpRhythmEdit(0)
{
  int x = 0;
  int y = 0;
  int Width, Height;
  GetClientSize(&Width, &Height);

  mpLengthEdit = new JZArrayEdit(
    pParent,
    mRhythm.length,
    wxPoint(x, y + Height / 2),
    wxSize(Width / 2, Height / 4 - 4));
  mpLengthEdit->SetXMinMax(1, 8);
  mpLengthEdit->SetLabel("length/interval");

  mpVelocityEdit = new JZArrayEdit(
    pParent,
    mRhythm.veloc,
    wxPoint(x + Width / 2, y + Height / 2),
    wxSize(Width / 2, Height / 4 - 4));
  mpVelocityEdit->SetXMinMax(1, 127);
  mpVelocityEdit->SetLabel("velocity");

  mpRhythmEdit = new JZRhyArrayEdit(
    pParent,
    mRhythm.rhythm,
    wxPoint(x, y + 3 * Height / 4),
    wxSize(Width, Height/ 4 - 4));
  mpRhythmEdit->SetMeter(
    mRhythm.steps_per_count,
    mRhythm.count_per_bar,
    mRhythm.n_bars);
  mpRhythmEdit->SetLabel("rhythm");
}

//*****************************************************************************
//*****************************************************************************
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(JZRhythmGeneratorFrame, wxFrame)

  EVT_MENU(wxID_HELP, JZRhythmGeneratorFrame::OnHelp)

  EVT_MENU(wxID_HELP_CONTENTS, JZRhythmGeneratorFrame::OnHelpContents)

END_EVENT_TABLE()

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZRhythmGeneratorFrame::JZRhythmGeneratorFrame()
  : wxFrame(
      0,
      wxID_ANY,
      "Rhythm Generator",
      wxPoint(
        gpConfig->GetValue(C_RhythmXpos),
        gpConfig->GetValue(C_RhythmYpos)),
      wxSize(640, 580)),
    mpToolBar(0),
    mpRhythmGeneratorWindow(0)
{
  CreateToolBar();

  wxMenu* pFileMenu = new wxMenu;
  pFileMenu->Append(wxID_OPEN, "&Load...");
  pFileMenu->Append(wxID_SAVEAS, "Save &As...");
  pFileMenu->Append(wxID_CLOSE, "&Close");

  wxMenu* pInstrumentMenu = new wxMenu;
  pInstrumentMenu->Append(ID_INSTRUMENT_ADD, "&Add");
  pInstrumentMenu->Append(ID_INSTRUMENT_DELETE, "&Delete");
  pInstrumentMenu->Append(ID_INSTRUMENT_UP, "&Up");
  pInstrumentMenu->Append(ID_INSTRUMENT_DOWN, "&Down");
  pInstrumentMenu->Append(ID_INSTRUMENT_GENERATE, "&Generate");

  wxMenu* mpHelpMenu = new wxMenu;
  mpHelpMenu->Append(wxID_HELP_CONTENTS, "&Contents");
  mpHelpMenu->Append(wxID_HELP, "&Help");

  wxMenuBar* pMenuBar = new wxMenuBar;
  pMenuBar->Append(pFileMenu, "&File");
  pMenuBar->Append(pInstrumentMenu, "&Instrument");
  pMenuBar->Append(mpHelpMenu, "&Help");

  SetMenuBar(pMenuBar);

  int Width, Height;
  GetClientSize(&Width, &Height);
  mpRhythmGeneratorWindow =
    new JZRhythmGeneratorWindow(this, wxPoint(0, 0), wxSize(Width, Height));
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZRhythmGeneratorFrame::CreateToolBar()
{
  JZToolDef ToolBarDefinitions[] =
  {
    { wxID_OPEN, false, open_xpm, "open rhythm file" },
    { wxID_SAVE, false, save_xpm, "save into rhythm file" },
    { JZToolBar::eToolBarSeparator },
    { ID_INSTRUMENT_ADD, false, rrgadd_xpm, "add instrument" },
    { ID_INSTRUMENT_DELETE, false, rrgdel_xpm, "remove instrument" },
    { ID_INSTRUMENT_UP, false, rrgup_xpm, "move instrument up" },
    { ID_INSTRUMENT_DOWN, false, rrgdown_xpm, "move instrument down" },
    { ID_INSTRUMENT_GENERATE, false, rrggen_xpm, "generate events into trackwin selection" },
    { JZToolBar::eToolBarSeparator },
    { wxID_HELP_CONTENTS, false, help_xpm, "help" },
    { JZToolBar::eToolBarEnd }
  };

  mpToolBar = new JZToolBar(this, ToolBarDefinitions);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZRhythmGeneratorFrame::~JZRhythmGeneratorFrame()
{
  delete mpRhythmGeneratorWindow;

  gpRhythmGeneratorFrame = 0;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZRhythmGeneratorFrame::OnHelp(wxCommandEvent&)
{
  JZHelp::Instance().ShowTopic("Random rhythm generator");
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZRhythmGeneratorFrame::OnHelpContents(wxCommandEvent&)
{
  JZHelp::Instance().DisplayHelpContents();
}

//*****************************************************************************
//*****************************************************************************
void CreateRhythmGenerator()
{
  if (!gpRhythmGeneratorFrame)
  {
    gpRhythmGeneratorFrame = new JZRhythmGeneratorFrame();
  }
  ((JZRhythmGeneratorFrame*)gpRhythmGeneratorFrame)->Show(true);
}
