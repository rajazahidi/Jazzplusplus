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

#include "guitar.h"
#include "pianowin.h"
#include "player.h"
#include "util.h"
#include "jazz.h"

// ====================================================================
// tGuitarCanvas
// ====================================================================

class tGuitarCanvas : public wxScrolledWindow
{
    friend class tGuitarSettingsDlg;
  public:
    tGuitarCanvas(tGuitarWin *parent, tPianoWin *piano, int x, int y, int w, int h);
    ~tGuitarCanvas();

    virtual void OnDraw(wxDC& dc);
    virtual void OnEvent(wxMouseEvent &e);

    void ClearBuffer();
    virtual void SetSize(int x, int y, int w, int h)
    {
      ww = w;
      hh = h;
      wxScrolledWindow::SetSize(x, y, w, h);
      Refresh();
    }

    void SettingsDialog();
    int  xy2Pitch(int x, int y);
    void DrawPitch(wxDC *dc,int pitch, int string, Bool show);
    void DrawPitch(wxDC *dc,int pitch, Bool show);

    int y2String(int y)
    {
      return (y + hstring/2) * (n_strings + 1) / hh - 1;
    }

    int x2Grid(int x)
    {
      return x * n_frets / ww;
    }

    void ShowPitch(wxDC* dc, int pitch);

  private:
    void DrawBoard(wxDC *dc);
    void DrawBuffer(wxDC* dc, tEventArray &arr, Bool show);

    tGuitarWin *guitar;
    tPianoWin  *piano;


    static Bool chord_mode;
    static Bool bass_guitar;
    static Bool show_octaves;
    static int  n_frets;

    int    n_strings;
    const int *pitches;
    static const int guitar_pitches[6];
    static const int bass_pitches[4];

    int ww, hh;
    int hstring, wfret;	// rounded values
    int wtext, htext;
    int act_pitch;	// mouse move
    int play_pitch;	// sound
    long put_clock;	// left up

    wxFont *font;
};

Bool tGuitarCanvas::chord_mode = FALSE;
Bool tGuitarCanvas::bass_guitar = FALSE;
Bool tGuitarCanvas::show_octaves = TRUE;
int  tGuitarCanvas::n_frets = 17;

const int tGuitarCanvas::bass_pitches[4] = {28+15, 28+10, 28+5, 28};
const int tGuitarCanvas::guitar_pitches[6] = {40+24, 40+19, 40+15, 40+10, 40+5, 40};

// ====================================================================
// tGuitarSettingsDlg
// ====================================================================

#ifndef __PORTING
class tGuitarSettingsDlg : public wxForm
{
    tGuitarCanvas *cnvs;
  public:

    tGuitarSettingsDlg(tGuitarCanvas *c)
 	: wxForm( USED_WXFORM_BUTTONS ), cnvs(c) {}
    void EditForm(wxPanel *panel);
    virtual void OnOk();
    virtual void OnHelp();
};

void tGuitarSettingsDlg::OnOk()
{
  if (cnvs->bass_guitar)
  {
    cnvs->pitches = cnvs->bass_pitches;
    cnvs->n_strings = 4;
  }
  else
  {
    cnvs->pitches = cnvs->guitar_pitches;
    cnvs->n_strings = 6;
  }
  cnvs->OnPaint();
  wxForm::OnOk();
}

void tGuitarSettingsDlg::OnHelp()
{
        HelpInstance->ShowTopic("Guitar board");
}


void tGuitarSettingsDlg::EditForm(wxPanel *panel)
{
  Add(wxMakeFormBool("Chord mode", &cnvs->chord_mode));
  Add(wxMakeFormBool("Bass guitar", &cnvs->bass_guitar));
  Add(wxMakeFormNewLine());
  Add(wxMakeFormBool("Show octaves", &cnvs->show_octaves));
  Add(wxMakeFormNewLine());
  Add(wxMakeFormShort("# frets", &cnvs->n_frets, wxFORM_DEFAULT));
  AssociatePanel(panel);
}
#endif // __PORTING
// ====================================================================
// tGuitarCanvas
// ====================================================================

tGuitarCanvas::tGuitarCanvas(tGuitarWin *parent, tPianoWin *ppiano, int x, int y, int w, int h)
  : wxScrolledWindow(parent, -1, wxPoint(x, y), wxSize(w, h))
{
  guitar = parent;
  piano = ppiano;
  ww = w;
  hh = h;
  put_clock = 0;

  if (bass_guitar)
  {
    pitches = bass_pitches;
    n_strings = 4;
  }
  else
  {
    pitches = guitar_pitches;
    n_strings = 6;
  }
  hstring = hh / (n_strings + 1);
  wfret   = ww / n_frets;
  act_pitch = 0;
  play_pitch = 0;


}


tGuitarCanvas::~tGuitarCanvas()
{
  //delete font;
}

void tGuitarCanvas::OnDraw(wxDC& indc)
{
  wxDC* dc=&indc;
  dc->SetFont(*wxSMALL_FONT);
  //dc->SetFont(wxNORMAL_FONT);
  //font = new wxFont(12, wxSWISS, wxNORMAL, wxNORMAL);
  //dc->SetFont(font);
  font = 0;
  dc->GetTextExtent("xG#", &wtext, &htext);

  GetSize(&ww, &hh);

  dc->Clear();

  DrawBoard(dc);
  DrawPitch(dc, act_pitch, TRUE);
  DrawBuffer(dc, piano->PasteBuffer, TRUE);
}

void tGuitarCanvas::DrawBuffer(wxDC* dc, tEventArray &arr, Bool show)
{
  int i;
  for (i = 0; i < arr.nEvents; i++)
  {
    tEvent *e = arr.Events[i];
    tKeyOn *on = e->IsKeyOn();
    if (on)
      DrawPitch(dc, on->Key, show);
  }
}


void tGuitarCanvas::DrawBoard(wxDC *dc)
{
  int i;

  hstring = hh / (n_strings + 1);
  wfret   = ww / n_frets;

  // paint strings
  for (i = 0; i < n_strings; i++)
  {
    int y = hh * (i + 1) / (n_strings + 1);
    dc->DrawLine(0, y, ww, y);
  }

  // paint frets
  int y1 = hh / (n_strings + 1);
  int y2 = hh * (n_strings) / (n_strings + 1);
  int d1 = hh / 5;
  for (i = 0; i < n_frets; i++)
  {
    int x = ww * i / n_frets;
    dc->DrawLine(x, y1, x, y2);
    switch (i)
    {
      case 0:
      case 11:
      case 23:
	dc->DrawLine(x+2, y1, x+2, y2);
	break;

      case 4:
      case 6:
      case 16:
      case 18:
	dc->DrawLine(x+2, y1+d1, x+2, y2-d1);
	break;

      default:
      case 2:
      case 8:
      case 14:
      case 20:
	break;

    }
  }
}


int tGuitarCanvas::xy2Pitch(int x, int y)
{
  int string = y2String(y);
  if (string >= 0 && string < n_strings)
  {
    int fret = x2Grid(x);
    if (fret >= 0 && fret < n_frets)
      return pitches[string] + fret + 1;
  }
  return 0;
}


void tGuitarCanvas::DrawPitch(  wxDC *dc, int pitch, int string, Bool show)
{
  static const char *key_names[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
  int x = (pitch - pitches[string] - 1) * ww / n_frets + wfret/3;
  if (x < 0 || x > ww)
    return;

  int y = hh * (string + 1) / (n_strings + 1) - int(htext/2);
  int w = int(wtext);
  int h = int(htext+1);

;
  dc->SetPen(*wxTRANSPARENT_PEN);
  dc->SetBrush(*wxWHITE_BRUSH);
  dc->DrawRectangle(x, y, w, h);
  dc->SetPen(*wxBLACK_PEN);
  dc->SetBrush(*wxBLACK_BRUSH);
  if (show)
    dc->DrawText((char *)key_names[pitch % 12], x, y);
  else
    dc->DrawLine(x, y + int(htext/2), x + w, y + int(htext/2));
}


void tGuitarCanvas::DrawPitch(wxDC* dc,int pitch, Bool show)
{
  if (pitch <= 0)
    return;
  if (show_octaves)
  {
    for (int i = 0; i < n_strings; i++)
      for (int p = pitch % 12; p < 127; p += 12)
	DrawPitch(dc, p, i, show);
  }
  else
    for (int i = 0; i < n_strings; i++)
      DrawPitch(dc, pitch, i, show);
}

void tGuitarCanvas::ShowPitch(wxDC* dc, int pitch)
{
  if (pitch != act_pitch)
  {
    DrawPitch(dc, act_pitch, FALSE);
    act_pitch = pitch;
    DrawPitch(dc, act_pitch, TRUE);
    DrawBuffer(dc, piano->PasteBuffer, TRUE);
  }
}



void tGuitarCanvas::OnEvent(wxMouseEvent &e)
{
  int x, y;
  e.GetPosition(&x, &y);
  int pitch = xy2Pitch(int(x), int(y));

  wxDC* dc = new wxClientDC(this); //Evil, __PORTING

  ShowPitch(dc, pitch);
  piano->ShowPitch( pitch);

  if (e.ButtonDown())
  {
    if (pitch)
    {
      tKeyOn on(0, piano->Channel(), pitch, 80);
      Midi->OutNow(piano->Track, &on);
      play_pitch = pitch;
    }
  }
  else if (e.Dragging())
  {
    if (play_pitch != pitch)
    {
      if (play_pitch)
      {
        tKeyOff of(0, piano->Channel(), play_pitch);
        Midi->OutNow(piano->Track, &of);
      }
      if (pitch)
      {
	tKeyOn on(0, piano->Channel(), pitch, 80);
	Midi->OutNow(piano->Track, &on);
      }
      play_pitch = pitch;
    }
  }
  else if (e.ButtonUp())
  {
    if (play_pitch)
    {
      tKeyOff of(0, piano->Channel(), play_pitch);
      Midi->OutNow(piano->Track, &of);
      play_pitch = 0;
    }
  }

  if (e.LeftUp() && pitch)
  {
    tKeyOn *on = new tKeyOn(put_clock, piano->Channel(), pitch, 80, piano->SnapClocks());
    if (!chord_mode)
      put_clock += piano->SnapClocks();

    //some evil painting code here, should invalidate instead __PORTING

    DrawBuffer(dc, piano->PasteBuffer, FALSE);
    piano->PasteBuffer.Put(on);
    DrawBuffer(dc, piano->PasteBuffer, TRUE);
  }
}


void tGuitarCanvas::ClearBuffer()
{
  piano->PasteBuffer.Clear();
  put_clock = 0;
  Refresh();
}

void tGuitarCanvas::SettingsDialog()
{
#ifndef __PORTING
  wxDialogBox *panel = new wxDialogBox(this, "Guitar board settings", FALSE );
  tGuitarSettingsDlg * dlg = new tGuitarSettingsDlg(this);
  dlg->EditForm(panel);
  panel->Fit();
  panel->Show(TRUE);
#endif // __PORTING
}

// ====================================================================
// tGuitarWin
// ====================================================================

#define MEN_CLEAR	1
#define MEN_SETTINGS	2
#define MEN_CLOSE 	3
#define MEN_HELP 	4

tGuitarWin::tGuitarWin(tPianoWin *parent)
  : wxFrame(0, -1, "Guitar board", wxPoint(20, 20), wxSize(600, 120))
{
  canvas = 0;
  int w = 600, h = 120;
  piano  = parent;
  wxMenu *menu = new wxMenu;
  menu->Append(MEN_CLEAR,	"C&lear");
  menu->Append(MEN_SETTINGS,	"&Settings");
  menu->Append(MEN_HELP,	"&Help");
  menu->Append(MEN_CLOSE,	"&Close");
  wxMenuBar *menu_bar = new wxMenuBar;
  menu_bar->Append(menu,	"&Menu");
  SetMenuBar(menu_bar);
  canvas = new tGuitarCanvas(this, piano, 0, 0, w, h);
}

tGuitarWin::~tGuitarWin()
{
  piano->GuitarWin = 0;
  delete canvas;
}


void tGuitarWin::OnHelp()
{
        HelpInstance->ShowTopic("Guitar board");
}

void tGuitarWin::OnMenuCommand(int id)
{
  switch (id)
  {
    case MEN_CLOSE:
      DELETE_THIS();
      break;
    case MEN_CLEAR:
      canvas->ClearBuffer();
      break;
    case MEN_SETTINGS:
      canvas->SettingsDialog();
      break;
    case MEN_HELP:
      OnHelp();
      break;
  }
}

void tGuitarWin::ShowPitch(int pitch)
{
  wxDC* dc=new wxClientDC();//evil __PORTING
  canvas->ShowPitch(dc, pitch);
}


void tGuitarWin::Redraw()
{
  canvas->Refresh();
}

void tGuitarWin::OnSize(int w, int h)
{
  GetClientSize(&w, &h);
  if (canvas)
    canvas->SetSize(0, 0, w, h);
}

#ifndef __PORTING
Bool tGuitarWin::OnClose()
{
  return TRUE;
}
#endif // __PORTING
