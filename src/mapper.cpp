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
#include "wx/wx.h"

#include "mapper.h"
#include "jazz.h"
#include "trackwin.h"
#include "command.h"

class tSong;
class tTrack;
class tEventWin;


// ==================== tRhythmSliderPanel ==================

/**
a panel contaning 3 sliders, "steps/count", ""count/bar" "#bars"

*/
tRhythmSliderPanel::tRhythmSliderPanel(wxWindow *parent, tRhyArrayEdit *edit, int x, int y, int w, int h)
  : wxPanel(parent, x, y, w, h, 0, "name")
{
  int i;

  in_create = 1;
  enabled = TRUE;
  n_edits = 0;

  edits[n_edits++] = edit;
  steps_per_count = 4;
  count_per_bar   = 4;
  n_bars          = 1;


  wxGridSizer* grid=new wxGridSizer(0,2,5,5);

  for (i = 0; i < n_edits; i++)
    edits[i]->SetMeter(steps_per_count, count_per_bar, n_bars);

  //  steps_per_count_slider = new wxSlider(this, (wxFunction)ItemCallback, "", 4, 1, 8, w/6, 10,     1, wxFIXED_LENGTH);
  steps_per_count_slider = new wxSlider(this, -1,  4, 1, 8,wxDefaultPosition, wxSize(150,8));//, w/6, 10,     1, wxFIXED_LENGTH);
  
  grid->Add(new wxStaticText(this, -1,"steps/count"));
  grid->Add(steps_per_count_slider);

  //  count_per_bar_slider   = new wxSlider(this, (wxFunction)ItemCallback, "", 4, 1, 8, w/6, 10, 1*h/12, wxFIXED_LENGTH);
  count_per_bar_slider   = new wxSlider(this, -1,  4, 1, 8,wxDefaultPosition, wxSize(150,8));//, w/6, 10, 1*h/12, wxFIXED_LENGTH);

  grid->Add(new wxStaticText(this, -1,"count/bar"));
  grid->Add(count_per_bar_slider);

  //  n_bars_slider          = new wxSlider(this, (wxFunction)ItemCallback, "", 1, 1, 8, w/6, 10, 2*h/12, wxFIXED_LENGTH);
  n_bars_slider          = new wxSlider(this, -1,  1, 1, 8,wxDefaultPosition, wxSize(150,8));//, w/6, 10, 2*h/12, wxFIXED_LENGTH);

  grid->Add( new wxStaticText(this, -1, "# bars"));
  grid->Add(n_bars_slider);

  SetAutoLayout( TRUE );
  SetSizer( grid );
  // grid->Fit(this );
//   grid->SetSizeHints(this );


  in_create = 0;

}

#ifndef __PORTING 
void tRhythmSliderPanel::ItemCallback(wxItem& item, wxCommandEvent& event)
{
  ((tRhythmSliderPanel *)item.GetParent())->ItemChanged();
}
#endif // __PORTING


void tRhythmSliderPanel::ItemChanged()
{
  if (in_create)
    return;

  steps_per_count = steps_per_count_slider->GetValue();
  count_per_bar = count_per_bar_slider->GetValue();
  n_bars = n_bars_slider->GetValue();

  for (int i = 0; i < n_edits; i++) {
    edits[i]->SetMeter(steps_per_count, count_per_bar, n_bars);
    edits[i]->Refresh(); //OnPaint();
  }
}

void tRhythmSliderPanel::OnPaint()
{
  for (int i = 0; i < n_edits; i++)
    edits[i]->Refresh();
}


bool tRhythmSliderPanel::Enable(bool enable)
{
  if (in_create)
    return true;

  if (!enable != !enabled)	// avoid flickering
  {
    steps_per_count_slider->Enable(enable);
    count_per_bar_slider->Enable(enable);
    n_bars_slider->Enable(enable);
    enabled = enable;
  }
  return true;
}

// ============================ tMapperWin ==============================

#define MEN_CLOSE 1
#define MEN_LOAD  2
#define MEN_SAVE  3
#define MEN_HELP  4
#define MEN_EQUAL 5
#define MEN_ANALYZE 6


tMapperWin *mapper_win = 0;


ostream & operator << (ostream &os, tMapperWin const &a)
{
  return os;
}

istream & operator >> (istream &is, tMapperWin &a)
{
  return is;
}


tMapperWin::tMapperWin(tEventWin *e, tSong *s)
  : wxFrame(0, -1, "Mapper", wxPoint(10, 10), wxSize(600, 500)),
    array(64, 0, 127)
{
  event_win       = e;
  song            = s;
  in_create       = 1;

  // panel items

  wxMenuBar *menu_bar = new wxMenuBar;
  wxMenu    *menu = new wxMenu;
  menu->Append(MEN_HELP,	"&Help");
  //menu->Append(MEN_LOAD,	"&Load");
  //menu->Append(MEN_SAVE,	"&Save");
  menu->Append(MEN_CLOSE,	"&Close");
  //menu->Append(MEN_EQUAL,	"&Equal");
  //menu->Append(MEN_ANALYZE,	"&Analyze");
  menu_bar->Append(menu,	"&Menu");
  SetMenuBar(menu_bar);

  int w, h;
  GetClientSize(&w, &h);

  edit = new tRhyArrayEdit(this, array, 0, h/3, w, 2*h/3, ARED_XTICKS | ARED_YTICKS | ARED_GAP);


  panel = new wxPanel(this, 0, 0, w/2, h/3, 0, "panel");

  wxGridSizer* grid=new wxGridSizer(0,2,5,5);

  sliders = new tRhythmSliderPanel(this, edit, w/2, 0, w/2, h/3);
  grid->Add(sliders);

  cout<<"mapper news a grid(and other things) that im to lazy to delete"<<endl;


  //  source = new wxListBox(panel, (wxFunction)ItemCallback, "Source", wxSINGLE|wxALWAYS_SB, -1, 10, 120); //, 120);
  source = new wxListBox(panel, -1, wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_SINGLE|wxLB_ALWAYS_SB);//, -1, 10, 120); //, 120);
  source->Append("velocity");
  source->Append("length");
  source->Append("note pitch");
  source->Append("rhythm");
  source->Append("random");
  #define SVEL 0
  #define SLEN 1
  #define SKEY 2
  #define SRHY 3
  #define SRND 4

  grid->Add(new wxStaticText(panel,-1,"Source"));
  grid->Add(source);


  //destin = new wxListBox(panel, (wxFunction)ItemCallback, "Destin", wxSINGLE|wxALWAYS_SB, -1, 10, 120); //, 120);
  destin = new wxListBox(panel, -1,  wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_SINGLE|wxLB_ALWAYS_SB);//, -1, 10, 120); //, 120);

  destin->Append("velocity");
  destin->Append("length");
  destin->Append("note pitch");
  destin->Append("note start");
  //destin->Append("pan");
  //destin->Append("modulation");
  //destin->Append("CC1");
  //destin->Append("CC2");
  //destin->Append("pitch bend");

  grid->Add(new wxStaticText(panel,-1,"Destination"));
  grid->Add(destin);

  #define DVEL 0
  #define DLEN 1
  #define DKEY 2
  #define DCLK 3

  #define DPAN 4
  #define DMOD 5
  #define DCC1 6
  #define DCC2 7
  #define DPIT 8

//   mode = new wxCheckBox(panel, (wxFunction)ItemCallback, "add Value");
//   apply = new wxButton(panel, (wxFunction)ItemCallback, "Apply");
  mode = new wxCheckBox(panel, -1,  "Add Value");
  apply = new wxButton(panel, -1, "Apply");

  grid->Add(mode);
  grid->Add(apply);

  panel->SetAutoLayout( TRUE );
  panel->SetSizer( grid );
  grid->Fit( panel );
  grid->SetSizeHints(panel );
      
  in_create = 0;
  Show(TRUE);
}

void tMapperWin::OnMenuCommand(int id) {
  switch (id) {
    case MEN_EQUAL: {
      for (int i = 0; i < array.Size(); i++)
        array[i] = i;
      OnPaint();
    }
    break;

    case MEN_HELP: Help(); break;
    case MEN_CLOSE:
      // motif crashes if Show(FALSE) is called before destructor
      // Show(FALSE);
      DELETE_THIS();
      break;

    case MEN_LOAD:
      {
	wxString  fname = wxFileSelector("Load Rhythm", NULL, NULL, NULL, "*.rhy");
	if (fname) {
	  ifstream is(fname);
	  is >> *this;
	}
      }
      break;

    case MEN_SAVE:
      {
	wxString fname = wxFileSelector("Save Rhythm", NULL, NULL, NULL, "*.rhy");
	if (fname) {
	  ofstream os(fname);
	  os << *this;
	}
      }
      break;

  }
}


void tMapperWin::Help()
{
  HelpInstance->ShowTopic("Mapper");
}


#ifndef __PORTING 
void tMapperWin::ItemCallback(wxItem& item, wxCommandEvent& event)
{
  tMapperWin *win = (tMapperWin *)item.GetParent()->GetParent();
  if (&item == win->apply)
    win->ApplyPressed();
  else
    win->ItemChanged();
}
#endif // __PORTING



void tMapperWin::ApplyPressed()
{
  if (TrackWin->EventsSelected("please select destination tracks first"))
  {
    tCmdMapper::prop src, dst;

    switch (source->GetSelection())
    {
      case SVEL : src = tCmdMapper::veloc; break;
      case SLEN : src = tCmdMapper::length; break;
      case SKEY : src = tCmdMapper::key; break;
      case SRHY : src = tCmdMapper::rhythm; break;
      case SRND : src = tCmdMapper::random; break;
      default: return;
    }

    switch (destin->GetSelection())
    {
      case DVEL : dst = tCmdMapper::veloc; break;
      case DLEN : dst = tCmdMapper::length; break;
      case DKEY : dst = tCmdMapper::key; break;
      case DPAN : dst = tCmdMapper::pan; break;
      case DMOD : dst = tCmdMapper::modul; break;
      case DCC1 : dst = tCmdMapper::cc1; break;
      case DCC2 : dst = tCmdMapper::cc2; break;
      case DPIT : dst = tCmdMapper::pitch; break;
      case DCLK : dst = tCmdMapper::clock; break;
      default: return;
    }
    int nb  = sliders->NBars();
    int add = mode->GetValue();
    wxBeginBusyCursor();
    tCmdMapper cmd(TrackWin->Filter, src, dst, array, nb, add);
    cmd.Execute();
    wxEndBusyCursor();
    TrackWin->Redraw();
    TrackWin->NextWin->Redraw();
  }
}


void tMapperWin::ItemChanged()
{
  if (in_create)
    return;

  if (destin->GetSelection() == DCLK)
  {
    mode->SetValue(TRUE);
    mode->Enable(FALSE);
  }
  else
    mode->Enable(TRUE);

  bool add = mode->GetValue();
  bool rnd = source->GetSelection() ==  SRND;
  bool rhy = source->GetSelection() ==  SRHY;

  int ymin = 1;
  int ymax = 127;
  int xmin = 1;
  int xmax = 127;

  switch (destin->GetSelection())
  {
    case DCLK: ymax = 24; break;
    case DLEN: ymax = 96; break;
    case DPIT: ymax = 100; break;
    case DKEY: if (add) ymax = 12; break;
  }

  if (add)
    ymin = -ymax;

  if (rnd)
  {
    // swap x,y
    xmin = ymin;
    xmax = ymax;
    ymin = 0;
    ymax = 100;
  }

  if (rhy)
  {
    edit->SetStyle(ARED_XTICKS | ARED_YTICKS | ARED_RHYTHM | ARED_GAP);
    edit->SetMeter(sliders->StepsPerCount(), sliders->CountPerBar(), sliders->NBars());
    sliders->Enable(TRUE);
  }
  else
  {
    edit->SetStyle(ARED_XTICKS | ARED_YTICKS | ARED_GAP);
    array.Resize(xmax - xmin + 1);
    // av-- edit->array_val.Resize(xmax - xmin + 1);
    //    edit->SetXMinMax(xmin, xmax);
    sliders->Enable(FALSE);
  }
  // SN++ BUG FIX for vertical zero line
  edit->SetXMinMax(xmin, xmax);

  edit->SetYMinMax(ymin, ymax);
  OnPaint();
}


tMapperWin::~tMapperWin()
{
  GetPosition( &Config(C_RhythmXpos), &Config(C_RhythmYpos) );
  mapper_win = 0;
}


void tMapperWin::OnPaint()
{
  if (in_create)
    return;

  edit->Refresh();
}

