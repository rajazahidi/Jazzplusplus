//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Copyright (C) 1994-2000 Andreas Voss and Per Sigmond, all rights reserved.
// Modifications Copyright (C) 2004 Patrick Earl
// Modifications Copyright (C) 2008-2010 Peter J. Stieber
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

#include "SampleWindow.h"

#include "Audio.h"
#include "FileSelector.h"
#include "Globals.h"
#include "Help.h"
#include "Mapper.h"
#include "MouseAction.h"
#include "Player.h"
#include "Resources.h"
#include "Rhythm.h"
#include "Sample.h"
#include "SampleCommand.h"
#include "SampleDialog.h"
#include "Song.h"
#include "SliderWindow.h"
#include "ToolBar.h"

#include <wx/dcclient.h>
#include <wx/menu.h>
#include <wx/msgdlg.h>
#include <wx/scrolbar.h>

//DEBUG#include <iostream>

#define MEN_HELP        5

#define MEN_SILENCE     21
#define MEN_SILENCE_INS 22
#define MEN_SILENCE_APP 23
#define MEN_SILENCE_OVR 24

#define MEN_ACCEPT      42

#define MEN_FLIP        56
#define MEN_FLIP_LEFT   57
#define MEN_FLIP_RIGHT  58

class tSamplePlayPosition;

class tInsertionPoint
{
  public:

    tInsertionPoint(wxScrolledWindow *c)
      : cnvs(c)
    {
      last_x = 0;
      visible = 0;
    }

    void Draw(int x)
    {
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

    void Draw()
    {
      Draw(last_x);
    }

    int IsVisible() const
    {
      return visible;
    }

    float GetX() const
    {
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

    void Redraw()
    {
      OnPaint();
    }

    virtual void OnPaint();

    virtual void OnSize(int w, int h);

    virtual void OnEvent(wxMouseEvent& MouseEvent);

    void ClearSelection();

    void SetInsertionPoint(int offs);

    void SetSelection(int fr, int to);

    int Sample2Pixel(int sample);

    int Pixel2Sample(float pixel);

    void Play();

  private:

    void DrawSample(int channel, int x, int y, int w, int h);

    void DrawTicks(int x, int y, int w);

  private:

    tSampleWin *win;

    tSample &spl;

    int paint_offset;
    int paint_length;

    JZSnapSelection snapsel;

    // sel_fr == 0: no selection and no insertion point
    // sel_fr >  0 && sel_fr == sel_to: insertion point
    // sel_fr >  0 && sel_fr <  sel_to: selected range
    int sel_fr, sel_to;
    tInsertionPoint inspt;
    int mouse_up_sets_insertion_point;
    tSamplePlayPosition *playpos;

    // for tickmark display
    bool midi_time;
    int midi_offs;

    bool mouse_down;
};

#ifdef OBSOLETE

class tSmplWinSettingsForm : public wxForm
{
  public:
    tSmplWinSettingsForm(tSampleWin &w)
      : wxForm( USED_WXFORM_BUTTONS ),
        win(w)
    {}
    void EditForm(wxPanel *panel)
    {
      Add(wxMakeFormBool("Show Midi Time", &win.cnvs->midi_time));
      Add(wxMakeFormNewLine());
      AssociatePanel(panel);
    }
    void OnOk()
    {
      win.settings = 0;
      win.Redraw();
      wxForm::OnOk();
    }
    void OnCancel()
    {
      win.settings = 0;
      wxForm::OnCancel();
    }
    void OnHelp()
    {
      gpHelpInstance->ShowTopic("Settings");
    }
  private:
    tSampleWin &win;
};

#endif

class tSamplePlayPosition : public wxTimer
{
  public:

    tSamplePlayPosition(tSampleCnvs &c, JZPlayer *p, tSample &s)
      : cnvs(c),
        player(p),
        spl(s)
    {
      visible = false;
      x = 0;
    }

    ~tSamplePlayPosition()
    {
      Stop();
      if (visible)
        Draw();
    }

    void StopListen()
    {
      Stop();
      if (gpMidiPlayer->IsListening())
        gpMidiPlayer->ListenAudio(-1);
      if (visible)
        Draw();
    }

    void StartListen(int fr, int to)
    {
      fr_smpl = fr;
      to_smpl = to;
      gpMidiPlayer->ListenAudio(spl, fr_smpl, to_smpl);
      Start(100);
    }

    bool IsListening() const
    {
      return gpMidiPlayer->IsListening();
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

    virtual void Notify()
    {
      int pos = player->GetListenerPlayPosition();
      if (pos < 0)
      {
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
    JZPlayer *player;
    tSample &spl;
    bool visible;
  int x;
    int fr_smpl;
    int to_smpl;
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
  playpos = new tSamplePlayPosition(*this, gpMidiPlayer, spl);
  midi_time = true;
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
// snapsel.SetYSnap(0, ch, ch / spl.GetChannels());
  snapsel.SetYSnap(0, ch, ch);

  AdjustScrollbars();
}



void tSampleCnvs::OnEvent(wxMouseEvent& MouseEvent)
{
  // dont accept mouse events as long as the
  // array edit is up
  if (win->on_accept)
  {
    return;
  }

  wxDC* pDc = new wxClientDC(this);

  // tSnapSel is strange.
  if (MouseEvent.LeftDown())
  {
    mouse_up_sets_insertion_point = 0;
    mouse_down = true;
    if (snapsel.IsSelected())
    {
      snapsel.Draw(*pDc, 0, 0);
      snapsel.SetSelected(false);
    }
    else if (inspt.IsVisible())
    {
      inspt.Draw();
    }
    else
    {
      mouse_up_sets_insertion_point = 1;
    }
    snapsel.ProcessMouseEvent(MouseEvent, 0, 0);
  }
  else if (MouseEvent.LeftUp())
  {
    mouse_down = false;
    snapsel.ProcessMouseEvent(MouseEvent, 0, 0);
    if (snapsel.IsSelected())
    {
      snapsel.Draw(*pDc, 0, 0);
      sel_fr = Pixel2Sample(
        snapsel.GetRectangle().x);
      sel_to = Pixel2Sample(
        snapsel.GetRectangle().x + snapsel.GetRectangle().width);
    }
    else if (mouse_up_sets_insertion_point)
    {
      int x, y;
      MouseEvent.GetPosition(&x, &y);
      sel_fr = sel_to = Pixel2Sample(x);
      inspt.Draw(x);
    }
    else
    {
      sel_fr = sel_to = -1;
    }
  }
  else if (MouseEvent.Dragging() && mouse_down)
  {
    snapsel.ProcessMouseEvent(MouseEvent, 0, 0);
  }
}


void tSampleCnvs::ClearSelection()
{
  if (snapsel.IsSelected())
  {
    wxDC* pDc = new wxClientDC(this);
    snapsel.Draw(*pDc, 0, 0);
    snapsel.SetSelected(false);
  }
  else if (inspt.IsVisible())
  {
    inspt.Draw();
  }
  sel_fr = sel_to = -1;
}


void tSampleCnvs::SetInsertionPoint(int offs)
{
  ClearSelection();
  sel_fr = sel_to = offs;
  int x = Sample2Pixel(offs);
  inspt.Draw(x);
}


void tSampleCnvs::SetSelection(int fr, int to)
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
  snapsel.SetRectangle(r);
  snapsel.SetSelected(true);
  wxDC* pDc = new wxClientDC(this);
  snapsel.Draw(*pDc, 0, 0);
}


int tSampleCnvs::Sample2Pixel(int sample)
{
  int offs   = win->GetPaintOffset();
  int length = win->GetPaintLength();
  int cw, ch;
  GetClientSize(&cw, &ch);
  JZMapper Map(offs, offs + length, 0, cw);
  return static_cast<int>(Map.XToY(sample));
}


int tSampleCnvs::Pixel2Sample(float pixel)
{
  int offs   = win->GetPaintOffset();
  int length = win->GetPaintLength();
  int cw, ch;
  GetClientSize(&cw, &ch);
  JZMapper Map(0, cw, offs, offs + length);
  int ofs = static_cast<int>(Map.XToY(pixel));
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

  if (snapsel.IsSelected())
  {
    JZRectangle r;
    r.SetX(Sample2Pixel(sel_fr));
    r.SetWidth(Sample2Pixel(sel_to) - r.x);
    r.SetY(0);
    r.SetHeight(ch);
    snapsel.SetRectangle(r);
    wxDC* pDc = new wxClientDC(this);
    snapsel.Draw(*pDc, 0, 0);
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

  int sfr = win->GetPaintOffset();
  int sto = sfr + win->GetPaintLength();

  if (!midi_time)
  {
    // display time
    JZMapper Map(sfr, sto, x, x+w);
    int tfr = spl->Samples2Time(sfr) / 1000;
    int tto = spl->Samples2Time(sto) / 1000 + 1;
    for (int sec = tfr; sec < tto; sec++)
    {
      for (int mil = 0; mil < 1000; mil += 100)
      {
        int t = spl->Time2Samples(sec * 1000 + mil);
        int xx = static_cast<int>(Map.XToY(t));
        // draw a tickmark line
        dc->DrawLine(xx, y - 5, xx, y);
        // draw a text
        char buf[50];
        sprintf(buf, "%d.%d", sec, mil/100);
        int fw, fh;
        dc->GetTextExtent(buf, &fw, &fh);
        dc->DrawText(buf, xx - fw/2, y + 2);
      }
    }
  }
  else
  {
    // Display midi counts.
    int cfr = static_cast<int>(spl->Samples2Ticks(sfr));
    int cto = static_cast<int>(spl->Samples2Ticks(sto));
    JZMapper Map(cfr, cto, x, x+w);
    JZBarInfo BarInfo(*gpSong);
    BarInfo.SetClock(cfr);
    BarInfo.SetBar(BarInfo.GetBarIndex());
    while (BarInfo.GetClock() < cto)
    {
      int ticks_per_count = BarInfo.GetTicksPerBar() / BarInfo.GetCountsPerBar();
      int ticks_per_step = ticks_per_count / 4;
      for (int i = 0; i < BarInfo.GetCountsPerBar(); ++i)
      {
        for (int j = 0; j < 4; j++)
        {
          int clock = BarInfo.GetClock() + i * ticks_per_count + j * ticks_per_step;
          int xx = static_cast<int>(Map.XToY(clock));
          // draw a tickmark line
          dc->DrawLine(xx, y - 5, xx, y);
          // draw a text
          if (j == 0)
          {
            char buf[50];
            sprintf(buf, "%d", i + 1);
            int fw, fh;
            dc->GetTextExtent(buf, &fw, &fh);
            dc->DrawText(buf, xx - fw/2, y + 2);
          }
        }
      }

      BarInfo.Next();
    }
  }

  dc->SetFont(f);
}


void tSampleCnvs::DrawSample(int channel, int x, int y, int w, int h)
{
  const short* data = spl.GetData();
  int length = spl.GetLength();
  int step = spl.GetChannels();

  // compute display range from position scrollbar
  int xfr = paint_offset + channel;
  int xto = paint_offset + paint_length;
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
  int x1 = x;
  for (int n = xfr; n < xto; n += step)
  {
    int x2 = static_cast<int>(XMap.XToY(n));
    short sy = data[n];
    if (x1 != x2)
    {
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

      int y1 = static_cast<int>(YMap.XToY(y1min));
      int y2 = static_cast<int>(YMap.XToY(y1max));
      dc->DrawLine(x1, y1, x1, y2);
      prev_ymin = ymin;
      prev_ymax = ymax;
      ymin = sy;
      ymax = sy;
      x1   = x2;
    }
    else
    {
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
    int fr_smpl = sel_fr > 0L ? sel_fr : -1L;
    int to_smpl = sel_to > sel_fr ? sel_to : -1L;
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

static JZToolDef tdefs[] =
{
  { wxID_OPEN,     false, open_xpm,    "open wave file" },
  { wxID_SAVE,     false, save_xpm,    "save wave file" },
  { JZToolBar::eToolBarSeparator },
  { wxID_ZOOM_IN,  false, zoomin_xpm,  "zoom to selection" },
  { wxID_ZOOM_OUT, false, zoomout_xpm, "zoom out" },
  { MEN_ACCEPT,   false, accept_xpm,  "accept painting" },
  { ID_PAINTER_NONE,   false, cancel_xpm,  "cancel painting" },
  { JZToolBar::eToolBarSeparator },
  { ID_PLAY,     false, play_xpm,    "play sample" },
  { MEN_HELP,     false, help_xpm,    "help" },
  { JZToolBar::eToolBarEnd }
};


int tSampleWin::geo[4] =
{
  30,
  30,
  600,
  300
};

tSample *tSampleWin::copy_buffer;

tSampleWin::tSampleWin(wxWindow* pParent, tSampleWin **ref, tSample& sample)
  : wxFrame(
      pParent,
      wxID_ANY,
      sample.GetFileName(),
      wxPoint(geo[0], geo[1]),
      wxSize(geo[2], geo[3])),
    spl(sample),
    vol_command(sample),
    pan_command(sample),
    pitch_command(sample),
    wah_command(sample)
{
  this->ref = ref;

  in_constructor = true;

  cnvs         = 0;
  mpToolBar    = 0;
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

  // Create a menu bar, the various menus with entries, and attach them to the
  // menu bar.

  wxMenu* pMenu = 0;
  wxMenu* pSubMenu = 0;
  wxMenuBar* pMenuBar = new wxMenuBar;

  // Create and populate the File menu.
  pMenu = new wxMenu;

  pMenu->Append(ID_FILE_REVERT_TO_SAVED, "&Revert to Saved");
  pMenu->Append(wxID_OPEN, "&Load...");
  pMenu->Append(wxID_SAVE, "&Save");
  pMenu->Append(wxID_SAVEAS, "&Save As...");
  pMenu->Append(wxID_CLOSE, "&Close");

  pMenuBar->Append(pMenu, "&File");

  // Create and populate the Edit menu.
  pMenu = new wxMenu;
  pMenu->Append(wxID_CUT, "&Cut");
  pMenu->Append(wxID_COPY, "Co&py");
  pMenu->Append(wxID_PASTE, "&Paste");
  pMenu->Append(ID_EDIT_PASTE_MERGE, "Paste &Merge");
  pSubMenu = new wxMenu;
  pSubMenu->Append(MEN_SILENCE_OVR, "&Replace");
  pSubMenu->Append(MEN_SILENCE_INS, "&Insert");
  pSubMenu->Append(MEN_SILENCE_APP, "&Append");
  pMenu->Append(MEN_SILENCE, "&Silence", pSubMenu);
  pSubMenu = new wxMenu;
  pSubMenu->Append(MEN_FLIP_LEFT, "Left");
  pSubMenu->Append(MEN_FLIP_RIGHT, "Right");
  pMenu->Append(MEN_FLIP, "In&vert Phase", pSubMenu);
  pMenu->Append(ID_EDIT_MAXIMIZE_VOLUME, "&Maximize Volume");
  pMenuBar->Append(pMenu, "&Edit");

  pMenu = new wxMenu;
  pMenu->Append(ID_PAINTERS_VOLUME, "&Volume...");
  pMenu->Append(ID_PAINTER_PAN, "&Panpot...");
  pMenu->Append(ID_PAINTER_PITCH, "&Pitch...");
  pMenu->Append(ID_PAINTER_WAHWAH, "&Filter...");
  pMenu->Append(ID_PAINTER_NONE, "&None...");
  pMenuBar->Append(pMenu, "&Painters");

  pMenu = new wxMenu;
  pMenu->Append(ID_EFFECTS_EQUALIZER, "&Equalizer...");
  pMenu->Append(ID_EFFECTS_FILTER, "&Filter...");
  pMenu->Append(ID_EFFECTS_DISTORTION, "&Distortion...");
  pMenu->Append(ID_EFFECTS_REVERB, "&Reverb...");
  pMenu->Append(ID_EFFECTS_ECHO, "&Echo...");
  pMenu->Append(ID_EFFECTS_CHORUS, "&Chorus...");
  pMenu->Append(ID_EFFECTS_PITCH_SHIFTER, "&Pitch shifter...");
  pMenu->Append(ID_EFFECTS_STRETCHER, "&Time stretcher...");
  pMenu->Append(ID_EFFECTS_REVERSE, "Re&verse");
  pMenu->Append(ID_EFFECTS_SYNTH, "&Synth...");
  pMenuBar->Append(pMenu,         "&Effects");

  pMenu = new wxMenu;
  pMenu->Append(ID_SETTINGS_PITCH_PAINTER, "&Pitch Painter...");
  pMenu->Append(ID_SETTINGS_WAHWAH, "&Filter Painter...");
//  pMenu->Append(wxID_ZOOM_IN,     "Zoom &In");
//  pMenu->Append(wxID_ZOOM_OUT,     "Zoom &Out");
  pMenu->Append(ID_VIEW_SETTINGS, "&View Settings...");
  pMenuBar->Append(pMenu, "&Settings");

  SetMenuBar(pMenuBar);

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

  in_constructor = false;

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
  return true;
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
  for (int i = 0; i < num_params; i++)
  {
    int yi = yy + (i + spl.GetChannels()) * hh / nn;
    params[i]->SetSize(xx, yi, ww, hi);
  }
}


void tSampleWin::Redraw()
{
  cnvs->Redraw();
}

bool tSampleWin::HaveInsertionPoint(int &offs, bool warn)
{
  if (cnvs->sel_fr == cnvs->sel_to && cnvs->sel_fr >= 0)
  {
    offs = cnvs->sel_fr;
    return true;
  }
  else
  {
    offs = -1;
    if (warn)
      wxMessageBox("please set insertion point first", "Error", wxOK);
    return false;
  }
}

bool tSampleWin::HaveSelection(int &fr_smpl, int &to_smpl, HaveSelectionMode mode)
{
  if (cnvs->sel_fr < cnvs->sel_to && cnvs->sel_fr >= 0)
  {
    fr_smpl = cnvs->sel_fr;
    to_smpl = cnvs->sel_to;
    return true;
  }
  else if (mode == SelAll)
  {
    fr_smpl = 0;
    to_smpl = spl.GetLength();
    return true;
  }
  fr_smpl = to_smpl = -1;
  if (mode == SelWarn)
    wxMessageBox("please select samples first", "Error", wxOK);
  return false;
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
  if (num_params > 0)
  {
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
  sprintf(buf, "could not load \"%s\"", spl.GetFileName());
  wxMessageBox(buf, "Error", wxOK);
}

extern int effect(tSample &spl);

void tSampleWin::OnMenuCommand(int id)
{
  if (gpMidiPlayer->IsPlaying())
  {
    return;
  }

  // Player crashes if data disappear.
  if (id != ID_PLAY)
  {
    cnvs->playpos->StopListen();
  }

  switch (id)
  {
    case ID_EFFECTS_EQUALIZER:
      if (equalizer == 0)
        equalizer = new tEqualizer(*this);
      equalizer->Show(true);
      break;

    case MEN_FLIP_LEFT:
      spl.Flip(0);
      break;
    case MEN_FLIP_RIGHT:
      spl.Flip(1);
      break;

    case ID_EFFECTS_DISTORTION:
      if (distortion == 0)
        distortion = new tDistortion(*this);
      distortion->Show(true);
      break;

    case ID_EFFECTS_REVERB:
#ifdef OBSOLETE
      if (reverb == 0)
      {
        // Old version was not modal.
        reverb = new wxDialog(this, wxID_ANY, "Reverb");
        tReverbForm *form = new tReverbForm(*this);
        form->EditForm(reverb);
        reverb->Fit();
      }
      ClearSelection();
      SetViewPos(0, spl.GetLength());
      reverb->Show(true);
#endif
      break;

    case ID_EFFECTS_REVERSE:
      {
        int fr, to;
        if (HaveSelection(fr, to))
        {
          spl.Reverse(fr, to);
          Redraw();
        }
      }
      break;

    case ID_EFFECTS_PITCH_SHIFTER:
#ifdef OBSOLETE
      if (shifter == 0)
      {
        shifter = new wxDialogBox(this, "Shifter", false );
        tShifterForm *form = new tShifterForm(*this);
        form->EditForm(shifter);
        shifter->Fit();
      }
      ClearSelection();
      SetViewPos(0, spl.GetLength());
      shifter->Show(true);
#endif
      break;

    case ID_EFFECTS_STRETCHER:
#ifdef OBSOLETE
      if (stretcher == 0)
      {
        stretcher = new wxDialogBox(this, "Stretcher", false );
        tStretcherForm *form = new tStretcherForm(*this);
        form->EditForm(stretcher);
        stretcher->Fit();
      }
      ClearSelection();
      SetViewPos(0, spl.GetLength());
      stretcher->Show(true);
#endif
      break;

    case ID_EFFECTS_FILTER:
#ifdef OBSOLETE
      if (filter == 0)
      {
        filter = new wxDialogBox(this, "Filter", false );
        tSplFilterForm *form = new tSplFilterForm(*this);
        form->EditForm(filter);
        filter->Fit();
      }
      filter->Show(true);
#endif
      break;

    case ID_VIEW_SETTINGS:
#ifdef OBSOLETE
      if (settings == 0)
      {
        settings = new wxDialogBox(this, "Settings", false );
        tSmplWinSettingsForm *form = new tSmplWinSettingsForm(*this);
        form->EditForm(settings);
        settings->Fit();
      }
      settings->Show(true);
#endif
      break;

    case ID_EFFECTS_ECHO:
#ifdef OBSOLETE
      if (echo == 0)
      {
        echo = new wxDialogBox(this, "Echo", false );
        tEchoForm *form = new tEchoForm(*this);
        form->EditForm(echo);
        echo->Fit();
      }
      ClearSelection();
      SetViewPos(0, spl.GetLength());
      echo->Show(true);
#endif
      break;

    case ID_EFFECTS_CHORUS:
#ifdef OBSOLETE
      if (chorus == 0)
      {
        chorus = new wxDialogBox(this, "Chorus", false );
        tChorusForm *form = new tChorusForm(*this);
        form->EditForm(chorus);
        chorus->Fit();
      }
      ClearSelection();
      SetViewPos(0, spl.GetLength());
      chorus->Show(true);
#endif
      break;

    case ID_EFFECTS_SYNTH:
      if (synth == 0)
        synth = new tSynthDlg(*this);
      synth->Show(true);
      break;

    case MEN_ACCEPT:
      if (on_accept)
      {
        int fr = GetPaintOffset();
        int to = fr + GetPaintLength();
        on_accept->OnAccept(fr, to);
        delete on_accept;
        on_accept = 0;
      }
      break;

    case ID_PAINTER_NONE:
      if (on_accept)
      {
        delete on_accept;
        on_accept = 0;
      }
      break;

    case wxID_CUT:
      {
        int fr, to;
        if (HaveSelection(fr, to, SelWarn))
        {
          spl.Cut(*copy_buffer, fr, to);
          cnvs->ClearSelection();
          cnvs->SetInsertionPoint(fr);
          Redraw();
        }
      }
      break;

    case wxID_COPY:
      {
        int fr, to;
        if (HaveSelection(fr, to, SelAll))
          spl.Copy(*copy_buffer, fr, to);
      }
      break;

    case wxID_ZOOM_IN:
      {
        int fr, to;
        if (HaveSelection(fr, to, SelWarn))
          SetViewPos(fr, to);
      }
      break;

    case wxID_ZOOM_OUT:
      SetViewPos(0, spl.GetLength());
      break;

    case ID_EDIT_MAXIMIZE_VOLUME:
      spl.Rescale();
      Redraw();
      break;

    case ID_PAINTERS_VOLUME:
      delete on_accept;
      on_accept = new tCommandPainter(*this, vol_command);
      break;

    case ID_PAINTER_WAHWAH:
      delete on_accept;
      on_accept = new tCommandPainter(*this, wah_command);
      break;

    case ID_SETTINGS_WAHWAH:
#ifdef OBSOLETE
      if (wah_settings == 0)
      {
        wah_settings = new wxDialogBox(this, "Filter Painter", false);
        tWahSettingsForm *form = new tWahSettingsForm(*this);
        form->EditForm(wah_settings);
        wah_settings->Fit();
      }
      wah_settings->Show(true);
#endif
      break;

    case ID_SETTINGS_PITCH_PAINTER:
#ifdef OBSOLETE
      if (pitch_settings == 0)
      {
        pitch_settings = new wxDialogBox(this, "Pitch Painter");
        tSplPitchForm *form = new tSplPitchForm(*this);
        form->EditForm(pitch_settings);
        pitch_settings->Fit();
      }
      pitch_settings->Show(true);
#endif
      break;


    case ID_PAINTER_PAN:
      delete on_accept;
      on_accept = new tCommandPainter(*this, pan_command);
      break;

    case ID_EDIT_PASTE_MERGE:
      {
        int offs;
        if (HaveInsertionPoint(offs))
        {
          spl.PasteMix(*copy_buffer, offs);
          cnvs->SetSelection(offs, offs + copy_buffer->GetLength());
          Redraw();
        }
      }
      break;

    case wxID_PASTE:
      {
        int offs, fr, to;
        if (HaveInsertionPoint(offs, false))
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
        int fr, to;
        if (HaveSelection(fr, to, SelWarn))
        {
          spl.InsertSilence(fr, to - fr);
          Redraw();
        }
      }
      break;

    case MEN_SILENCE_APP:
      {
        int fr, to;
        if (HaveSelection(fr, to, SelWarn))
        {
          spl.InsertSilence(to, to - fr);
          Redraw();
        }
      }
      break;

    case MEN_SILENCE_OVR:
      {
        int fr, to;
        if (HaveSelection(fr, to, SelWarn))
        {
          spl.ReplaceSilence(fr, to - fr);
          Redraw();
        }
      }
      break;

    case ID_PAINTER_PITCH:
      delete on_accept;
      SetViewPos(0, spl.GetLength());
      on_accept = new tCommandPainter(*this, pitch_command);
      break;

    case ID_FILE_REVERT_TO_SAVED:
      cnvs->ClearSelection();
      if (spl.Load(true))
        LoadError(spl);
      Redraw();
      break;

    case wxID_CLOSE:
//      DELETE_THIS();
      Destroy();
      break;

    case ID_PLAY:
      cnvs->Play();
      break;

    case wxID_OPEN:
      {
        wxString FileName = file_selector(
          spl.GetFileName(),
          "Load Sample",
          false,
          false,
          "*.wav");
        if (!FileName.empty())
        {
          wxBeginBusyCursor();
          cnvs->ClearSelection();
          spl.SetFileName(FileName);
          if (spl.Load(true))
          {
            LoadError(spl);
          }
          spl->RefreshDialogs();
          SetTitle(FileName);
          Redraw();
          wxEndBusyCursor();
        }
      }
      break;

    case wxID_SAVEAS:
      {
        wxString FileName = file_selector(
          spl.GetFileName(),
          "Save Sample",
          true,
          false,
          "*.wav");
        if (!FileName.empty())
        {
          spl.SetFileName(FileName);
          OnMenuCommand(wxID_SAVE);
          spl->RefreshDialogs();
          SetTitle(FileName);
        }
      }
      break;

    case wxID_SAVE:
      {
        if (spl.GetFileName().empty())
        {
          OnMenuCommand(wxID_SAVEAS);
        }
        else
        {
          wxBeginBusyCursor();
          cnvs->ClearSelection();
          int err = spl.Save();
          Redraw();
          wxEndBusyCursor();
          if (err)
          {
            wxMessageBox("writing failed!!", "Error", wxOK);
          }
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

void tSampleWin::PlaySample()
{
  cnvs->Play();
}

int tSampleWin::GetPaintLength()
{
  // return the visible amount of sample data
  double sb = zoom_scrol->GetThumbPosition();
  JZMapper Map(0, 1000, spl.GetLength(), 0);
  int len = static_cast<int>(Map.XToY(sb));
  return spl.Align(len);
}


int tSampleWin::GetPaintOffset()
{
  // return the visible Offset in sample data
  double sb = pos_scrol->GetThumbPosition();
  JZMapper Map(0, 1000, 0, spl.GetLength());
  int ofs = static_cast<int>(Map.XToY(sb));
  return spl.Align(ofs);
}

void tSampleWin::SetViewPos(int fr, int to)
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
