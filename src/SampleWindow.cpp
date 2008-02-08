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

#include "SampleWindow.h"
#include "SampleCommand.h"
#include "SampleDialog.h"
#include "ToolBar.h"
#include "Sample.h"
#include "Audio.h"
#include "MouseAction.h"
#include "Song.h"
#include "Player.h"
#include "Rhythm.h"
#include "SliderWindow.h"
#include "Mapper.h"
#include "FileSelector.h"
#include "Globals.h"
#include "Help.h"

#include <iostream>

#define MEN_LOAD	1
#define MEN_SAVE	2
#define MEN_CLOSE	3
#define MEN_PLAY	4
#define MEN_HELP	5
#define MEN_SAVEAS	6
#define MEN_REVERT      7

#define MEN_CUT         10
#define MEN_COPY        11

#define MEN_PASTE       12
#define MEN_PASTE_MIX   15


#define MEN_VOLUME_MAX  19
#define MEN_VOLUME_PNT  20

#define MEN_SILENCE     21
#define MEN_SILENCE_INS 22
#define MEN_SILENCE_APP 23
#define MEN_SILENCE_OVR 24
#define MEN_WAHWAH      25
#define MEN_WAHSETTINGS 26

#define MEN_PAN_PNT     27
#define MEN_TRANSP_PNT  28
#define MEN_TRANSP_SET  29

#define MEN_ZOOM_IN     40
#define MEN_ZOOM_OUT    41
#define MEN_ACCEPT      42
#define MEN_CANCEL      43
#define MEN_ECHO        44
#define MEN_DISTORTION  45
#define MEN_EQUALIZER   46
#define MEN_REVERB      47
#define MEN_SHIFTER     48
#define MEN_REVERSE     49
#define MEN_SETTINGS    50
#define MEN_FILTER      51
#define MEN_CHORUS      52
#define MEN_STEREO      53
#define MEN_STRETCHER   54
#define MEN_SYNTH       55
#define MEN_FLIP        56
#define MEN_FLIP_LEFT   57
#define MEN_FLIP_RIGHT  58

class tSamplePlayPosition;

class tInsertionPoint {
  public:
    tInsertionPoint(wxScrolledWindow *c) : cnvs(c) {
      last_x = 0;
      visible = 0;
    };
    void Draw(int x) {
      last_x = x;
      visible ^= 1;
      wxDC *dc = new wxClientDC(cnvs);
      int cw, ch;
      cnvs->GetClientSize(&cw, &ch);
      dc->SetPen(*wxRED_PEN);
      dc->SetLogicalFunction(wxXOR);
      dc->DrawLine(x, 0, x, ch);
      dc->SetPen(*wxBLACK_PEN);
      dc->SetLogicalFunction(wxCOPY);
    }
    void Draw() {
      Draw(last_x);
    }
    int IsVisible() const {
      return visible;
    }
    float GetX() const {
      return last_x;
    }
  private:
    int last_x;
    int visible;
    wxScrolledWindow *cnvs;
};




class tSampleCnvs : public wxScrolledWindow
{
  friend class tSampleWin;
  friend class tSmplWinSettingsForm;
  public:
    tSampleCnvs(tSampleWin *win, tSample &sample);
    virtual ~tSampleCnvs();
    void Redraw() {
      OnPaint();
    }
    virtual void OnPaint();
    virtual void OnSize(int w, int h);
    virtual void OnEvent(wxMouseEvent &evt);
    void ClearSelection();
    void SetInsertionPoint(long offs);
    void SetSelection(long fr, long to);

    int Sample2Pixel(long sample);
    long  Pixel2Sample(float pixel);
    void  Play();


  private:
    void DrawSample(int channel, int x, int y, int w, int h);
    tSampleWin *win;
    tSample &spl;
    void DrawTicks(int x, int y, int w);

    long paint_offset;
    long paint_length;

    tSnapSelection snapsel;
    // sel_fr == 0: no selection and no insertion point
    // sel_fr >  0 && sel_fr == sel_to: insertion point
    // sel_fr >  0 && sel_fr <  sel_to: selected range
    long sel_fr, sel_to;
    tInsertionPoint inspt;
    int mouse_up_sets_insertion_point;
    tSamplePlayPosition *playpos;

    // for tickmark display
    bool midi_time;
    long midi_offs;

    bool mouse_down;
};

#ifdef OBSOLETE

class tSmplWinSettingsForm : public wxForm
{
  public:
    tSmplWinSettingsForm(tSampleWin &w)
    : wxForm( USED_WXFORM_BUTTONS ), win(w) {}
    void EditForm(wxPanel *panel) {
      Add(wxMakeFormBool("Show Midi Time", &win.cnvs->midi_time));
      Add(wxMakeFormNewLine());
      AssociatePanel(panel);
    }
    void OnOk() {
      win.settings = 0;
      win.Redraw();
      wxForm::OnOk();
    }
    void OnCancel() {
      win.settings = 0;
      wxForm::OnCancel();
    }
    void OnHelp() {
      gpHelpInstance->ShowTopic("Settings");
    }
  private:
    tSampleWin &win;
};

#endif

class tSamplePlayPosition : public wxTimer
{
  public:
    tSamplePlayPosition(tSampleCnvs &c, tPlayer *p, tSample &s)
      : cnvs(c), player(p), spl(s)
    {
      visible = FALSE;
      x = 0;
    }

    ~tSamplePlayPosition() {
      Stop();
      if (visible)
        Draw();
    }

    void StopListen() {
      Stop();
      if (Midi->IsListening())
	Midi->ListenAudio(-1);
      if (visible)
	Draw();
    }

    void StartListen(long fr, long to) {
      fr_smpl = fr;
      to_smpl = to;
      Midi->ListenAudio(spl, fr_smpl, to_smpl);
      Start(100);
    }

    bool IsListening() const {
      return Midi->IsListening();
    }

    void Draw()
    {
      visible ^= 1;
      wxDC *dc = new wxClientDC(&cnvs);
      int cw, ch;
      cnvs.GetClientSize(&cw, &ch);
      dc->SetPen(*wxGREEN_PEN);
      dc->SetLogicalFunction(wxXOR);
      dc->DrawLine(x, 0, x, ch);
      dc->SetPen(*wxBLACK_PEN);
      dc->SetLogicalFunction(wxCOPY);
    }

    virtual void Notify() {
      long pos = player->GetListenerPlayPosition();
      if (pos < 0) {
        StopListen();
        return;
      }
      if (visible)
        Draw();
      x = cnvs.Sample2Pixel(fr_smpl + pos);
      Draw();
    }

  private:
    tSampleCnvs &cnvs;
    tPlayer *player;
    tSample &spl;
    bool visible;
  int x;
    long fr_smpl;
    long to_smpl;
};



tSampleCnvs::tSampleCnvs(tSampleWin *win, tSample &sample)
  : wxScrolledWindow(win),
    spl(sample),
    snapsel(this),
    inspt(this)
{
  this->win = win;
  sel_fr = sel_to = -1;
  mouse_up_sets_insertion_point = 0;
  playpos = new tSamplePlayPosition(*this, Midi, spl);
  midi_time = TRUE;
  midi_offs = 0;
  mouse_down = 0;
}


tSampleCnvs::~tSampleCnvs()
{
  delete playpos;
}


void tSampleCnvs::OnSize(int w, int h)
{
  int cw, ch;
  GetClientSize(&cw, &ch);
  //snapsel.SetYSnap(0, ch, ch / spl.GetChannels());
  snapsel.SetYSnap(0, ch, ch);

  AdjustScrollbars();
}



void tSampleCnvs::OnEvent(wxMouseEvent &e)
{
  // dont accept mouse events as long as the
  // array edit is up
  if (win->on_accept)
  {
    return;
  }

  wxDC* pDc = new wxClientDC(this);

  // tSnapSel is strange ...
  if (e.LeftDown())
  {
    mouse_up_sets_insertion_point = 0;
    mouse_down = TRUE;
    if (snapsel.Selected)
    {
      snapsel.Draw(pDc);
      snapsel.Selected = 0;
    }
    else if (inspt.IsVisible())
      inspt.Draw();
    else
      mouse_up_sets_insertion_point = 1;
    snapsel.Event(e);
  }
  else if (e.LeftUp())
  {
    mouse_down = FALSE;
    snapsel.Event(e);
    if (snapsel.Selected)
    {
      snapsel.Draw(pDc);
      sel_fr = Pixel2Sample(snapsel.r.x);
      sel_to = Pixel2Sample(snapsel.r.x + snapsel.r.width);
    }
    else if (mouse_up_sets_insertion_point) {
      int x, y;
      e.GetPosition(&x, &y);
      sel_fr = sel_to = Pixel2Sample(x);
      inspt.Draw(x);
    }
    else
      sel_fr = sel_to = -1;
  }
  else if (e.Dragging() && mouse_down)
    snapsel.Event(e);
}


void tSampleCnvs::ClearSelection()
{
  if (snapsel.Selected)
  {
    wxDC* pDc = new wxClientDC(this);
    snapsel.Draw(pDc);
    snapsel.Selected = 0;
  }
  else if (inspt.IsVisible())
  {
    inspt.Draw();
  }
  sel_fr = sel_to = -1;
}


void tSampleCnvs::SetInsertionPoint(long offs)
{
  ClearSelection();
  sel_fr = sel_to = offs;
  int x = Sample2Pixel(offs);
  inspt.Draw(x);
}


void tSampleCnvs::SetSelection(long fr, long to)
{
  ClearSelection();
  sel_fr = fr;
  sel_to = to;
  JZRectangle r;
  r.SetX(Sample2Pixel(fr));
  r.SetWidth(Sample2Pixel(to) - r.x);
  int cw, ch;
  GetClientSize(&cw, &ch);
  r.SetY(0);
  r.SetHeight(ch);
  snapsel.r = r;
  snapsel.Selected = TRUE;
  wxDC* pDc = new wxClientDC(this);
  snapsel.Draw(pDc);
}


int tSampleCnvs::Sample2Pixel(long sample)
{
  long offs   = win->GetPaintOffset();
  long length = win->GetPaintLength();
  int cw, ch;
  GetClientSize(&cw, &ch);
  JZMapper Map(offs, offs + length, 0, cw);
  return Map.XToY(sample);
}


long tSampleCnvs::Pixel2Sample(float pixel)
{
  long offs   = win->GetPaintOffset();
  long length = win->GetPaintLength();
  int cw, ch;
  GetClientSize(&cw, &ch);
  JZMapper Map(0, cw, offs, offs + length);
  long ofs = (long)Map.XToY(pixel);
  return spl.Align(ofs);
}


void tSampleCnvs::OnPaint()
{
  paint_offset = win->GetPaintOffset();
  paint_length = win->GetPaintLength();

  wxDC *dc = new wxPaintDC(win);
//OBSOLETE  dc->BeginDrawing();
  if (inspt.IsVisible())
    inspt.Draw();  // clear insertion point if there
  dc->Clear();

  int cw, ch;
  GetClientSize(&cw, &ch);
  int n = spl.GetChannels();
  for (int i = 0; i < n; ++i)
  {
    int x = 0;
    int y = ch * i / n;
    int w = cw;
    int h = ch / n;
    DrawSample(i, x, y, w, h);
    // separate the channels
    dc->DrawLine(x, y, x+w, y);
    if (i > 0)  // not the first one
      DrawTicks(x, y, w);
  }

  if (snapsel.Selected)
  {
    JZRectangle r;
    r.SetX(Sample2Pixel(sel_fr));
    r.SetWidth(Sample2Pixel(sel_to) - r.x);
    r.SetY(0);
    r.SetHeight(ch);
    snapsel.r = r;
    wxDC* pDc = new wxClientDC(this);
    snapsel.Draw(pDc);
  }
  else if (sel_fr > 0)
  {
    int x = Sample2Pixel(sel_fr);
    inspt.Draw(x);
  }

//OBSOLETE  dc->EndDrawing();
}


void tSampleCnvs::DrawTicks(int x, int y, int w)
{
  wxDC *dc = new wxClientDC(this);
  wxFont f = dc->GetFont();
  dc->SetFont(*wxSMALL_FONT);

  long sfr = win->GetPaintOffset();
  long sto = sfr + win->GetPaintLength();

  if (!midi_time) {
    // display time
    JZMapper Map(sfr, sto, x, x+w);
    int tfr = spl->Samples2Time(sfr) / 1000;
    int tto = spl->Samples2Time(sto) / 1000 + 1;
    for (int sec = tfr; sec < tto; sec++) {
      for (long mil = 0; mil < 1000; mil += 100) {
        long t = spl->Time2Samples(sec * 1000 + mil);
	int xx = Map.XToY(t);
	// draw a tickmark line
	dc->DrawLine(xx, y - 5, xx, y);
	// draw a text
        char buf[50];
	sprintf(buf, "%ld.%ld", sec, mil/100);
	int fw, fh;
	dc->GetTextExtent(buf, &fw, &fh);
        dc->DrawText(buf, xx - fw/2, y + 2);
      }
    }
  }
  else {
    // display midi counts
    long cfr = (long)spl->Samples2Ticks(sfr);
    long cto = (long)spl->Samples2Ticks(sto);
    JZMapper Map(cfr, cto, x, x+w);
    JZBarInfo bi(TheSong);
    bi.SetClock(cfr);
    bi.SetBar(bi.BarNr);
    while (bi.Clock < cto) {
      long ticks_per_count = bi.TicksPerBar / bi.CountsPerBar;
      long ticks_per_step = ticks_per_count / 4;
      for (int i = 0; i < bi.CountsPerBar; i++) {
        for (int j = 0; j < 4; j++) {
	  long clock = bi.Clock + i * ticks_per_count + j * ticks_per_step;
	  int xx = Map.XToY(clock);
	  // draw a tickmark line
	  dc->DrawLine(xx, y - 5, xx, y);
	  // draw a text
	  if (j == 0) {
	    char buf[50];
	    sprintf(buf, "%d", i + 1);
	    int fw, fh;
	    dc->GetTextExtent(buf, &fw, &fh);
	    dc->DrawText(buf, xx - fw/2, y + 2);
	  }
	}
      }

      bi.Next();
    }
  }

  dc->SetFont(f);
}


void tSampleCnvs::DrawSample(int channel, int x, int y, int w, int h)
{
  const short *data = spl.GetData();
  long length       = spl.GetLength();
  long step         = spl.GetChannels();

  // compute display range from position scrollbar
  long xfr = paint_offset + channel;
  long xto = paint_offset + paint_length;
  if (xto > length)
    xto = length;

  if (xfr >= xto)
    return;

  JZMapper XMap(xfr, xto, x, x + w);
  JZMapper YMap(-32767.0, 32767.0, y+h, y);

  wxDC *dc = new wxClientDC(this);

  short prev_ymin = 0;
  short prev_ymax = 0;
  short ymin = 0;
  short ymax = 0;
  long  x1   = x;
  for (long n = xfr; n < xto; n += step)
  {
    long  x2 = (long)XMap.XToY(n);
    short sy = data[n];
    if (x1 != x2) {
      // new x-coordinate

      short y1min, y1max;
      if (prev_ymin > ymax)
        y1max = prev_ymin;
      else
        y1max = ymax;

      if (prev_ymax < ymin)
        y1min = prev_ymax;
      else
        y1min = ymin;

      float y1 = (float)YMap.XToY(y1min);
      float y2 = (float)YMap.XToY(y1max);
      dc->DrawLine((float)x1, y1, (float)x1, y2);
      prev_ymin = ymin;
      prev_ymax = ymax;
      ymin = sy;
      ymax = sy;
      x1   = x2;
    }
    else {
      if (sy > ymax)
        ymax = sy;
      else if (sy < ymin)
        ymin = sy;
    }
  }
}


void tSampleCnvs::Play()
{
  if (playpos->IsListening())
    playpos->StopListen();
  else
  {
    long fr_smpl = sel_fr > 0L ? sel_fr : -1L;
    long to_smpl = sel_to > sel_fr ? sel_to : -1L;
    playpos->StartListen(fr_smpl, to_smpl);
  }
}


// ----------------------------------------------------------------
// -------------------------- tSampleWin --------------------------
// ----------------------------------------------------------------


#include "Bitmaps/open.xpm"
#include "Bitmaps/save.xpm"
#include "Bitmaps/play.xpm"
#include "Bitmaps/help.xpm"
#include "Bitmaps/accept.xpm"
#include "Bitmaps/cancel.xpm"
#include "Bitmaps/zoomin.xpm"
#include "Bitmaps/zoomout.xpm"

static JZToolDef tdefs[] = {
  { MEN_LOAD,     FALSE, open_xpm,    "open wave file" },
  { MEN_SAVE,     FALSE, save_xpm,    "save wave file" },
  { JZToolBar::eToolBarSeparator },
  { MEN_ZOOM_IN,  FALSE, zoomin_xpm,  "zoom to selection" },
  { MEN_ZOOM_OUT, FALSE, zoomout_xpm, "zoom out" },
  { MEN_ACCEPT,   FALSE, accept_xpm,  "accept painting" },
  { MEN_CANCEL,   FALSE, cancel_xpm,  "cancel painting" },
  { JZToolBar::eToolBarSeparator },
  { MEN_PLAY,     FALSE, play_xpm,    "play sample" },
  { MEN_HELP,     FALSE, help_xpm,    "help" },
  { JZToolBar::eToolBarEnd }
};


int tSampleWin::geo[4] = { 30, 30, 600, 300 };

tSample *tSampleWin::copy_buffer;

tSampleWin::tSampleWin(wxFrame *parent, tSampleWin **ref, tSample &sample)
  : wxFrame(
      0,
      wxID_ANY,
      (char *)sample.GetFilename(),
      wxPoint(geo[0], geo[1]),
      wxSize(geo[2], geo[3])),
    spl(sample),
    vol_command(sample),
    pan_command(sample),
    pitch_command(sample),
    wah_command(sample)
{
  this->ref = ref;

  in_constructor = TRUE;

  cnvs         = 0;
  mpToolBar     = 0;
  scrol_panel  = 0;
  pos_scrol    = 0;
  zoom_scrol   = 0;
  num_params   = 0;
  on_accept    = 0;
  equalizer    = 0;
  distortion   = 0;
  reverb       = 0;
  echo         = 0;
  shifter      = 0;
  stretcher    = 0;
  filter       = 0;
  settings     = 0;
  wah_settings = 0;
  pitch_settings = 0;
  chorus       = 0;
  synth        = 0;

  if (copy_buffer == 0)
    copy_buffer = new tSample(spl.SampleSet());

  mpToolBar = new JZToolBar(this, tdefs);

  wxMenuBar *menu_bar = new wxMenuBar;
  wxMenu    *menu = new wxMenu;
  menu->Append(MEN_REVERT,	"&Revert to Saved");
  menu->Append(MEN_LOAD,	"&Load ...");
  menu->Append(MEN_SAVE,	"&Save");
  menu->Append(MEN_SAVEAS,	"&Save as ...");
  menu->Append(MEN_CLOSE,	"&Close");
  menu_bar->Append(menu,	"&File");

  menu = new wxMenu;
  menu->Append(MEN_CUT,		"&Cut");
  menu->Append(MEN_COPY,	"Co&py");

  menu->Append(MEN_PASTE,       "&Paste");
  menu->Append(MEN_PASTE_MIX,   "Paste &Merge");
  wxMenu *sub = new wxMenu;
  sub->Append(MEN_SILENCE_OVR,  "&Replace");
  sub->Append(MEN_SILENCE_INS,  "&Insert");
  sub->Append(MEN_SILENCE_APP,  "&Append");
  menu->Append(MEN_SILENCE,	"&Silence", sub);
  sub = new wxMenu;
  sub->Append(MEN_FLIP_LEFT,  "Left");
  sub->Append(MEN_FLIP_RIGHT, "Right");
  menu->Append(MEN_FLIP, "In&vert Phase", sub);
  menu->Append(MEN_VOLUME_MAX,   "&Maximize Volume");
  menu_bar->Append(menu,	 "&Edit");

  menu = new wxMenu;
  menu->Append(MEN_VOLUME_PNT,   "&Volume ...");
  menu->Append(MEN_PAN_PNT,      "&Panpot ...");
  menu->Append(MEN_TRANSP_PNT,   "&Pitch ...");
  menu->Append(MEN_WAHWAH,       "&Filter ...");
  menu->Append(MEN_CANCEL,       "&None ...");
  menu_bar->Append(menu,	 "&Painters");

  menu = new wxMenu;
  menu->Append(MEN_EQUALIZER,    "&Equalizer...");
  menu->Append(MEN_FILTER,       "&Filter...");
  menu->Append(MEN_DISTORTION,   "&Distortion...");
  menu->Append(MEN_REVERB,       "&Reverb...");
  menu->Append(MEN_ECHO,         "&Echo...");
  menu->Append(MEN_CHORUS,       "&Chorus...");
  menu->Append(MEN_SHIFTER,      "&Pitch shifter...");
  menu->Append(MEN_STRETCHER,    "&Time stretcher...");
  menu->Append(MEN_REVERSE,      "Re&verse");
  menu->Append(MEN_SYNTH,        "&Synth...");
  menu_bar->Append(menu,	 "&Effects");

  menu = new wxMenu;
  menu->Append(MEN_TRANSP_SET,   "&Pitch Painter ...");
  menu->Append(MEN_WAHSETTINGS,  "&Filter Painter ...");
  //menu->Append(MEN_ZOOM_IN,	"Zoom &In");
  //menu->Append(MEN_ZOOM_OUT,	"Zoom &Out");
  menu->Append(MEN_SETTINGS,	"&View Settings...");
  menu_bar->Append(menu,	"&Settings");

  SetMenuBar(menu_bar);

  // construct a panel containing the scrollbars
  cnvs = new tSampleCnvs(this, spl);
  scrol_panel = new wxPanel(this);

//OBSOLETE  pos_scrol   = new wxScrollBar(scrol_panel, (wxFunction)ScrollCallback);
  pos_scrol = new wxScrollBar(scrol_panel, wxID_ANY);
//  pos_scrol->SetObjectLength(1000);
//  pos_scrol->SetViewLength(1000);
//  pos_scrol->SetValue(0);
  pos_scrol->SetScrollbar(0, 1000, 1000, 1000);

//OBSOLETE  zoom_scrol  = new wxScrollBar(scrol_panel, (wxFunction)ScrollCallback);
  zoom_scrol  = new wxScrollBar(scrol_panel, wxID_ANY);
//  zoom_scrol->SetObjectLength(1000);
//  zoom_scrol->SetViewLength(10);
//  zoom_scrol->SetPageLength(100);
//  zoom_scrol->SetValue(0);
  zoom_scrol->SetScrollbar(0, 10, 1000, 100);

  in_constructor = FALSE;

  // now force a resize for motif
  int cw, ch;
  GetClientSize(&cw, &ch);
  OnSize(cw, ch);
}


tSampleWin::~tSampleWin()
{
  *ref = 0;
  GetPosition(&geo[0], &geo[1]);
  GetSize(&geo[2], &geo[3]);
  delete mpToolBar;
  delete cnvs;
  delete zoom_scrol;
  delete pos_scrol;
  delete scrol_panel;
  for (int i = 0; i < num_params; i++)
    delete params[i];
  delete equalizer;
  delete distortion;
  delete reverb;
  delete echo;
  delete chorus;
  delete synth;
  delete shifter;
  delete stretcher;
  delete filter;
  delete settings;
  delete wah_settings;
  delete pitch_settings;
}


bool tSampleWin::OnClose()
{
  return TRUE;
}


void tSampleWin::OnSize(int w, int h)
{
  // constructor finished?
  if (in_constructor)
    return;

  int cw, ch;
  GetClientSize(&cw, &ch);

  wxSize ToolBarSize = mpToolBar->GetSize();
  int pw, ph;
  pos_scrol->GetSize(&pw, &ph);
  int zw, zh;
  zoom_scrol->GetSize(&zw, &zh);

//OBSOLETE  mpToolBar->SetSize(0, 0, (int)cw, ToolBarSize.GetHeight());
  scrol_panel->SetSize(0, ch-zh-ph, cw, zh+ph);
  zoom_scrol->SetSize(0, 0, cw, zh);
  pos_scrol->SetSize(0, zh, cw, ph);

  // divide the remaining space on cnvs and params
  int xx = 0;
  int yy = ToolBarSize.GetHeight();
  int ww = cw;
  int hh = ch - ToolBarSize.GetHeight() - zh - ph;
  int nn = spl.GetChannels() + num_params;

  int hi = hh * spl.GetChannels() / nn;
  cnvs->SetSize(xx, yy, ww, hi);

  hi = hh / nn;
  for (int i = 0; i < num_params; i++) {
    int yi = yy + (i + spl.GetChannels()) * hh / nn;
    params[i]->SetSize(xx, yi, ww, hi);
  }
}


void tSampleWin::Redraw()
{
  cnvs->Redraw();
}

bool tSampleWin::HaveInsertionPoint(long &offs, bool warn)
{
  if (cnvs->sel_fr == cnvs->sel_to && cnvs->sel_fr >= 0) {
    offs = cnvs->sel_fr;
    return TRUE;
  }
  else {
    offs = -1;
    if (warn)
      wxMessageBox("please set insertion point first", "Error", wxOK);
    return FALSE;
  }
}

bool tSampleWin::HaveSelection(long &fr_smpl, long &to_smpl, HaveSelectionMode mode)
{
  if (cnvs->sel_fr < cnvs->sel_to && cnvs->sel_fr >= 0) {
    fr_smpl = cnvs->sel_fr;
    to_smpl = cnvs->sel_to;
    return TRUE;
  }
  else if (mode == SelAll)
  {
    fr_smpl = 0;
    to_smpl = spl.GetLength();
    return TRUE;
  }
  fr_smpl = to_smpl = -1;
  if (mode == SelWarn)
    wxMessageBox("please select samples first", "Error", wxOK);
  return FALSE;
}


void tSampleWin::AddParam(JZRndArray *array, const char *label)
{
  params[num_params] = new tArrayEdit(this, *array, 0, 0, 10, 10, 0); // ARED_LINES);
  params[num_params]->SetLabel(label);
  num_params++;
  int cw, ch;
  GetClientSize(&cw, &ch);
  OnSize(cw, ch);
}


void tSampleWin::ClrParam()
{
  if (num_params > 0) {
    int n = num_params;
    num_params = 0;
    for (int i = 0; i < n; ++i)
    {
      delete params[i];
    }
    int cw, ch;
    GetClientSize(&cw, &ch);
    OnSize(cw, ch);
  }
}

void tSampleWin::ClearSelection()
{
  cnvs->ClearSelection();
}


void tSampleWin::LoadError(tSample &spl)
{
  char buf[500];
  sprintf(buf, "could not load \"%s\"", spl.GetFilename());
  wxMessageBox(buf, "Error", wxOK);
}

extern int effect(tSample &spl);

void tSampleWin::OnMenuCommand(int id)
{
  if (Midi->IsPlaying())
  {
    return;
  }

  // player crashes if data disappear ...
  if (id != MEN_PLAY)
  {
    cnvs->playpos->StopListen();
  }

  switch(id)
  {
    case MEN_EQUALIZER:
      if (equalizer == 0)
        equalizer = new tEqualizer(*this, (wxFrame **)&equalizer);
      equalizer->Show(TRUE);
      break;

    case MEN_FLIP_LEFT:
      spl.Flip(0);
      break;
    case MEN_FLIP_RIGHT:
      spl.Flip(1);
      break;

    case MEN_DISTORTION:
      if (distortion == 0)
        distortion = new tDistortion(*this, (wxFrame **)&distortion);
      distortion->Show(TRUE);
      break;

    case MEN_REVERB:
#ifdef OBSOLETE
      if (reverb == 0)
      {
        // Old version was not modal.
        reverb = new wxDialog(this, wxID_ANY, "Reverb");
        tReverbForm *form = new tReverbForm(*this, (void **)&reverb);
        form->EditForm(reverb);
        reverb->Fit();
      }
      ClearSelection();
      SetViewPos(0, spl.GetLength());
      reverb->Show(TRUE);
#endif
      break;

    case MEN_REVERSE:
      {
        long fr, to;
	if (HaveSelection(fr, to))
	{
	  spl.Reverse(fr, to);
	  Redraw();
	}
      }
      break;

    case MEN_SHIFTER:
#ifdef OBSOLETE
      if (shifter == 0)
      {
        shifter = new wxDialogBox(this, "Shifter", FALSE );
        tShifterForm *form = new tShifterForm(*this, (void **)&shifter);
        form->EditForm(shifter);
        shifter->Fit();
      }
      ClearSelection();
      SetViewPos(0, spl.GetLength());
      shifter->Show(TRUE);
#endif
      break;

    case MEN_STRETCHER:
#ifdef OBSOLETE
      if (stretcher == 0)
      {
        stretcher = new wxDialogBox(this, "Stretcher", FALSE );
        tStretcherForm *form = new tStretcherForm(*this, (void **)&stretcher);
        form->EditForm(stretcher);
        stretcher->Fit();
      }
      ClearSelection();
      SetViewPos(0, spl.GetLength());
      stretcher->Show(TRUE);
#endif
      break;

    case MEN_FILTER:
#ifdef OBSOLETE
      if (filter == 0)
      {
        filter = new wxDialogBox(this, "Filter", FALSE );
        tSplFilterForm *form = new tSplFilterForm(*this, (void **)&filter);
        form->EditForm(filter);
        filter->Fit();
      }
      filter->Show(TRUE);
#endif
      break;

    case MEN_SETTINGS:
#ifdef OBSOLETE
      if (settings == 0)
      {
        settings = new wxDialogBox(this, "Settings", FALSE );
        tSmplWinSettingsForm *form = new tSmplWinSettingsForm(*this);
        form->EditForm(settings);
        settings->Fit();
      }
      settings->Show(TRUE);
#endif
      break;

    case MEN_ECHO:
#ifdef OBSOLETE
      if (echo == 0)
      {
        echo = new wxDialogBox(this, "Echo", FALSE );
        tEchoForm *form = new tEchoForm(*this, (void **)&echo);
        form->EditForm(echo);
        echo->Fit();
      }
      ClearSelection();
      SetViewPos(0, spl.GetLength());
      echo->Show(TRUE);
#endif
      break;

    case MEN_CHORUS:
#ifdef OBSOLETE
      if (chorus == 0)
      {
        chorus = new wxDialogBox(this, "Chorus", FALSE );
        tChorusForm *form = new tChorusForm(*this, (void **)&chorus);
        form->EditForm(chorus);
        chorus->Fit();
      }
      ClearSelection();
      SetViewPos(0, spl.GetLength());
      chorus->Show(TRUE);
#endif
      break;

    case MEN_SYNTH:
      if (synth == 0)
        synth = new tSynthDlg(*this, (wxFrame **)&synth);
      synth->Show(TRUE);
      break;

    case MEN_ACCEPT:
      if (on_accept) {
        long fr = GetPaintOffset();
	long to = fr + GetPaintLength();
        on_accept->OnAccept(fr, to);
	delete on_accept;
	on_accept = 0;
      }
      break;

    case MEN_CANCEL:
      if (on_accept) {
	delete on_accept;
	on_accept = 0;
      }
      break;

    case MEN_CUT:
      {
	long fr, to;
	if (HaveSelection(fr, to, SelWarn))
	{
	  spl.Cut(*copy_buffer, fr, to);
	  cnvs->ClearSelection();
	  cnvs->SetInsertionPoint(fr);
	  Redraw();
	}
      }
      break;

    case MEN_COPY:
      {
	long fr, to;
	if (HaveSelection(fr, to, SelAll))
	  spl.Copy(*copy_buffer, fr, to);
      }
      break;

    case MEN_ZOOM_IN:
      {
	long fr, to;
	if (HaveSelection(fr, to, SelWarn))
	  SetViewPos(fr, to);
      }
      break;

    case MEN_ZOOM_OUT:
      SetViewPos(0, spl.GetLength());
      break;

    case MEN_VOLUME_MAX:
      spl.Rescale();
      Redraw();
      break;

    case MEN_VOLUME_PNT:
      delete on_accept;
      on_accept = new tCommandPainter(*this, vol_command);
      break;

    case MEN_WAHWAH:
      delete on_accept;
      on_accept = new tCommandPainter(*this, wah_command);
      break;

    case MEN_WAHSETTINGS:
#ifdef OBSOLETE
      if (wah_settings == 0)
      {
        wah_settings = new wxDialogBox(this, "Filter Painter", FALSE);
        tWahSettingsForm *form = new tWahSettingsForm(*this, (void **)&wah_settings, wah_command);
        form->EditForm(wah_settings);
        wah_settings->Fit();
      }
      wah_settings->Show(TRUE);
#endif
      break;

    case MEN_TRANSP_SET:
#ifdef OBSOLETE
      if (pitch_settings == 0)
      {
        pitch_settings = new wxDialogBox(this, "Pitch Painter");
        tSplPitchForm *form = new tSplPitchForm(*this, (void **)&pitch_settings, pitch_command);
        form->EditForm(pitch_settings);
        pitch_settings->Fit();
      }
      pitch_settings->Show(TRUE);
#endif
      break;


    case MEN_PAN_PNT:
      delete on_accept;
      on_accept = new tCommandPainter(*this, pan_command);
      break;

    case MEN_PASTE_MIX:
      {
	long offs;
	if (HaveInsertionPoint(offs))
	{
          spl.PasteMix(*copy_buffer, offs);
	  cnvs->SetSelection(offs, offs + copy_buffer->GetLength());
	  Redraw();
	}
      }
      break;

    case MEN_PASTE:
      {
        long offs, fr, to;
        if (HaveInsertionPoint(offs, FALSE))
        {
          spl.PasteIns(*copy_buffer, offs);
	  cnvs->SetSelection(offs, offs + copy_buffer->GetLength());
	  Redraw();
        }
	else if (HaveSelection(fr, to, SelWarn))
	{
	  spl.PasteOvr(*copy_buffer, fr, to);
          cnvs->SetInsertionPoint(fr);
	  Redraw();
	}
      }
      break;

    case MEN_SILENCE_INS:
      {
        long fr, to;
	if (HaveSelection(fr, to, SelWarn))
	{
	  spl.InsertSilence(fr, to - fr);
	  Redraw();
	}
      }
      break;

    case MEN_SILENCE_APP:
      {
        long fr, to;
	if (HaveSelection(fr, to, SelWarn))
	{
	  spl.InsertSilence(to, to - fr);
	  Redraw();
	}
      }
      break;

    case MEN_SILENCE_OVR:
      {
        long fr, to;
	if (HaveSelection(fr, to, SelWarn))
	{
	  spl.ReplaceSilence(fr, to - fr);
	  Redraw();
	}
      }
      break;

    case MEN_TRANSP_PNT:
      delete on_accept;
      SetViewPos(0, spl.GetLength());
      on_accept = new tCommandPainter(*this, pitch_command);
      break;

    case MEN_REVERT:
      cnvs->ClearSelection();
      if (spl.Load(TRUE))
        LoadError(spl);
      Redraw();
      break;

    case MEN_CLOSE:
//      DELETE_THIS();
      Destroy();
      break;

    case MEN_PLAY:
      cnvs->Play();
      break;

    case MEN_LOAD:
      {
        char *defname = copystring(spl.GetFilename());
        wxString fname = file_selector(defname, "Load Sample", FALSE, FALSE, "*.wav");
        if (!fname.empty())
        {
	  wxBeginBusyCursor();
	  cnvs->ClearSelection();
	  spl.SetFilename(fname);
	  if (spl.Load(TRUE))
	    LoadError(spl);
	  spl->RefreshDialogs();
	  SetTitle(fname);
	  Redraw();
	  wxEndBusyCursor();
	}
	delete [] defname;
      }
      break;

    case MEN_SAVEAS:
      {
        char *defname = copystring(spl.GetFilename());
        wxString fname = file_selector(defname, "Save Sample", TRUE, FALSE, "*.wav");
        if (!fname.empty())
        {
	  spl.SetFilename(fname);
	  OnMenuCommand(MEN_SAVE);
	  spl->RefreshDialogs();
	  SetTitle(fname);
	}
	delete [] defname;
      }
      break;

    case MEN_SAVE:
      {
        if (spl.GetFilename()[0] == 0)
	  OnMenuCommand(MEN_SAVEAS);
	else
	{
	  wxBeginBusyCursor();
	  cnvs->ClearSelection();
	  int err = spl.Save();
	  Redraw();
	  wxEndBusyCursor();
	  if (err)
	    wxMessageBox("writing failed!!", "Error", wxOK);
	}
      }
      break;

  case MEN_HELP:
    gpHelpInstance->ShowTopic("Sample Editor");
    break;

    default:
      break;
  }
}

void tSampleWin::PlaySample() {
  cnvs->Play();
}

long tSampleWin::GetPaintLength()
{
  // return the visible amount of sample data
  double sb = zoom_scrol->GetThumbPosition();
  JZMapper Map(0, 1000, spl.GetLength(), 0);
  long len = (long)Map.XToY(sb);
  return spl.Align(len);
}


long tSampleWin::GetPaintOffset()
{
  // return the visible Offset in sample data
  double sb = pos_scrol->GetThumbPosition();
  JZMapper Map(0, 1000, 0, spl.GetLength());
  long ofs = (long)Map.XToY(sb);
  return spl.Align(ofs);
}

void tSampleWin::SetViewPos(long fr, long to)
{
  JZMapper Map(0, spl.GetLength(), 0, 1000);
  int zval = 1000 - (int)Map.XToY(to - fr);
  zoom_scrol->SetThumbPosition(zval);

  int  pval = (int)Map.XToY(fr);
  if (pval > zval)
    pval = zval;

  // avoid motif warnings: by setting a very small length,
  // every position is valid.
//  pos_scrol->SetViewLength(1);
//  pos_scrol->SetValue(pval);
//  pos_scrol->SetViewLength(1000 - zval);
//  pos_scrol->SetPageLength((1000 - zval) * 2 / 3);
  pos_scrol->SetScrollbar(pval, 1, 1000 - zval, (1000 - zval) * 2 / 3);

  Redraw();
}

#ifdef OBSOLETE
void tSampleWin::OnScroll(wxItem &item)
{
  int  zval   = zoom_scrol->GetValue();
  int  pval   = pos_scrol->GetValue();

  if (pval > zval)
    pval = zval;

  pos_scrol->SetValue(pval);
  pos_scrol->SetViewLength(1000 - zval);
  pos_scrol->SetPageLength((1000 - zval) * 2 / 3);

  Redraw();
}

void tSampleWin::ScrollCallback(wxItem &itm, wxCommandEvent& Event)
{
  ((tSampleWin *)(itm.GetParent()->GetParent()))->OnScroll(itm);
}
#endif
