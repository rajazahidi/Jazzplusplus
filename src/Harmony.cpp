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
#include "Harmony.h"
#include "HarmonyP.h"
#include "Player.h"
#include "TrackFrame.h"
#include "TrackWindow.h"
#include "PianoFrame.h"
#include "PianoWindow.h"
#include "GuitarFrame.h"
#include "Song.h"
#include "Filter.h"
#include "ToolBar.h"
#include "HarmonyBrowserAnalyzer.h"
#include "FileSelector.h"
#include "Rectangle.h"
#include "Globals.h"
#include "Help.h"

#include <iostream>
#include <fstream>

using namespace std;

#define MEN_CLOSE        1
#define MEN_MIDI        2
#define MEN_TRANSPOSE        4
#define MEN_CLEARSEQ        6
#define MEN_EDIT        7
#define MEN_MOUSE        8
#define MEN_HELP        9

#define MEN_MAJSCALE        10
#define MEN_HARSCALE        11
#define MEN_MELSCALE        12

#define MEN_EQ4                13
#define MEN_EQ3                14
#define MEN_EQ2                15
#define MEN_EQ1                16
#define MEN_EQH                17
#define MEN_EQ0                18
#define MEN_251                19
#define MEN_TRITONE        20
#define MEN_PIANO        21
#define MEN_EQB                22
#define MEN_HAUNSCH        23
#define MEN_ANALYZE        24
#define MEN_IONSCALE        25
#define MEN_SETTINGS        26
#define MEN_LOAD        27
#define MEN_SAVE        28


#include "Bitmaps/open.xpm"
#include "Bitmaps/save.xpm"
#include "Bitmaps/majscale.xpm"
#include "Bitmaps/harscale.xpm"
#include "Bitmaps/melscale.xpm"
#include "Bitmaps/ionscale.xpm"
#include "Bitmaps/same4.xpm"
#include "Bitmaps/same3.xpm"
#include "Bitmaps/same2.xpm"
#include "Bitmaps/same1.xpm"
#include "Bitmaps/sameh.xpm"
#include "Bitmaps/sameb.xpm"
#include "Bitmaps/same0.xpm"
#include "Bitmaps/std251.xpm"
#include "Bitmaps/tritone.xpm"
#include "Bitmaps/haunsch.xpm"
#include "Bitmaps/piano.xpm"
#include "Bitmaps/transpos.xpm"
#include "Bitmaps/analyze.xpm"
//#include "Bitmaps/rrggen.xpm"
#include "Bitmaps/delchord.xpm"

static JZToolDef tdefs[] =
{
  { MEN_LOAD,      FALSE, open_xpm,     "open harmony file" },
  { MEN_SAVE,      FALSE, save_xpm,     "save harmony file" },
  { JZToolBar::eToolBarSeparator },
  { MEN_MAJSCALE,  TRUE,  majscale_xpm, "major scale" },
  { MEN_HARSCALE,  TRUE,  harscale_xpm, "harmonic scale" },
  { MEN_MELSCALE,  TRUE,  melscale_xpm, "melodic scale" },
  { MEN_IONSCALE,  TRUE,  ionscale_xpm, "ionic b13 scale" },
  { JZToolBar::eToolBarSeparator },
  { MEN_EQ4,       TRUE,  same4_xpm,    "4 common notes" },
  { MEN_EQ3,       TRUE,  same3_xpm,    "3 common notes" },
  { MEN_EQ2,       TRUE,  same2_xpm,    "2 common notes" },
  { MEN_EQ1,       TRUE,  same1_xpm,    "1 common note" },
  { MEN_EQ0,       TRUE,  same0_xpm,    "0 common notes" },
  { MEN_EQH,       TRUE,  sameh_xpm,    "one half note difference" },
  { MEN_251,       TRUE,  std251_xpm,   "next in 2-5-1 move" },
  { MEN_EQB,       TRUE,  sameb_xpm,    "same base note" },
  { MEN_TRITONE,   TRUE,  tritone_xpm,  "tritone substitute" },
  { MEN_PIANO,     TRUE,  piano_xpm,    "pianowin copy buffer" },
  { JZToolBar::eToolBarSeparator },
  { MEN_HAUNSCH,   TRUE,  haunsch_xpm,  "haunschild layout" },
  { JZToolBar::eToolBarSeparator },
  { MEN_TRANSPOSE, FALSE, transpos_xpm, "transpose trackwin selection" },
  { MEN_ANALYZE,   FALSE, analyze_xpm,  "analyze trackwin selection" },
  { MEN_CLEARSEQ,  FALSE, delchord_xpm, "clear harmonies" },
  { JZToolBar::eToolBarEnd }
};


// ------------------------ HBPlayer ------------------------

/** handle playing of the harmony*/
class HBPlayer : public wxTimer
{
    friend class HBCanvas;
  public:
    HBPlayer();
    void StartPlay(const HBContext &);
    void Paste(tEventArray &);
    void StopPlay();
    void SettingsDialog(wxFrame *parent);
    int  IsPlaying() const { return playing; }
    const HBContext &Context()        { return context; }

    virtual void Notify();

    int  GetChordKeys(int *out, const HBContext &);
    int  GetMeldyKeys(int *out, const HBContext &);
    int  GetBassKey(const HBContext &);

  private:
    HBContext context;
    static int bass_channel, bass_veloc;
    static int chord_channel, chord_veloc;
    static int meldy_channel, meldy_veloc;
    static bool bass_enabled, chord_enabled, meldy_enabled;
    static int bass_pitch, chord_pitch, meldy_pitch;
    static int meldy_speed;

    int bass_key, chord_keys[12], n_chord_keys;
    int meldy_keys[12], n_meldy_keys, meldy_index;
    int note_length;
    int playing;
    int device;
};

bool HBPlayer::bass_enabled  = 1;
int HBPlayer::bass_channel  = 1;
int HBPlayer::bass_veloc    = 90;
int HBPlayer::bass_pitch    = 40;

bool HBPlayer::chord_enabled = 1;
int HBPlayer::chord_channel = 2;
int HBPlayer::chord_veloc   = 90;
int HBPlayer::chord_pitch   = 60;

bool HBPlayer::meldy_enabled = 0;
int HBPlayer::meldy_channel = 3;
int HBPlayer::meldy_veloc   = 90;
int HBPlayer::meldy_pitch   = 70;
int HBPlayer::meldy_speed   = 100;


HBPlayer::HBPlayer()
{
  playing = 0;
  bass_key = n_chord_keys = n_meldy_keys = 0;
  note_length = 60;
  meldy_index = 0;
  device = gpSong->GetTrack(0)->GetDevice();
}


int HBPlayer::GetChordKeys(int *out, const HBContext &context)
{
  // build chord keys
  HBChord chord = context.Chord();
  int key = chord.Iter(chord_pitch - 1);
  int n   = chord.Count();
  for (int i = 0; i < n; i++)
  {
    out[i] = key;
    key = chord.Iter(key);
  }
  return n;
}


int HBPlayer::GetBassKey(const HBContext &context)
{
  // build bass note
  int key = context.ChordKey() + bass_pitch - bass_pitch % 12;
  if (key < bass_pitch)
    key += 12;
  return key;
}


int HBPlayer::GetMeldyKeys(int *out, const HBContext &context)
{
  // build melody keys
  HBChord scale = context.Scale();
  int n = scale.Count();

  int key = scale.Iter(meldy_pitch);
  int j = 0;
  for (int i = 0; i < n; i++)
  {
    out[j++] = key;
    key = scale.Iter(key);
  }
  return n;
}

void HBPlayer::Paste(tEventArray &arr)
{
  if (bass_enabled)
  {
    tKeyOn e(0, bass_channel - 1, bass_key, bass_veloc, note_length);
    arr.Put(e.Copy());
  }

  if (chord_enabled)
  {
    for (int i = 0; i < n_chord_keys; i++)
    {
      tKeyOn e(0, chord_channel - 1, chord_keys[i], chord_veloc, note_length);
      arr.Put(e.Copy());
    }
  }
}


void HBPlayer::StartPlay(const HBContext &ct)
{
  int i;

  device = gpSong->GetTrack(0)->GetDevice();

  if (playing)
    StopPlay();
  playing = 1;
  context = ct;

  bass_key = GetBassKey(context);
  n_chord_keys = GetChordKeys(chord_keys, context);
  n_meldy_keys = GetMeldyKeys(meldy_keys, context);

  // Generate KeyOn's
  if (bass_enabled)
  {
    tKeyOn e(0, bass_channel - 1, bass_key, bass_veloc);
    gpMidiPlayer->OutNow(device, &e);
  }

  if (chord_enabled)
  {
    for (i = 0; i < n_chord_keys; i++)
    {
      tKeyOn e(0, chord_channel - 1, chord_keys[i], chord_veloc);
      gpMidiPlayer->OutNow(device, &e);
    }
  }

  Notify();
  Start(60000L / 4 / meldy_speed);
}


void HBPlayer::Notify()
{
  if (meldy_enabled)
  {
    tKeyOff of(0, meldy_channel - 1, meldy_keys[meldy_index]);
    gpMidiPlayer->OutNow(device, &of);
    meldy_index = (meldy_index + 1) % n_meldy_keys;
    tKeyOn on(0, meldy_channel - 1, meldy_keys[meldy_index], meldy_veloc);
    gpMidiPlayer->OutNow(device, &on);
  }
}


void HBPlayer::StopPlay()
{
  if (!playing)
    return;
  Stop();
  playing = 0;

  int i;
  // Generate KeyOff's
  if (bass_enabled)
  {
    tKeyOff e(0, bass_channel - 1, bass_key);
    gpMidiPlayer->OutNow(device, &e);
  }

  if (chord_enabled)
  {
    for (i = 0; i < n_chord_keys; i++)
    {
      tKeyOff e(0, chord_channel - 1, chord_keys[i]);
      gpMidiPlayer->OutNow(device, &e);
    }
  }

  if (meldy_enabled)
  {
    for (i = 0; i < n_meldy_keys; i++)
    {
      tKeyOff of(0, meldy_channel - 1, meldy_keys[i]);
      gpMidiPlayer->OutNow(device, &of);
    }
  }
}

#ifdef OBSOLETE

/** harmony browser playing form*/
class tHBPlayerForm : public wxForm
{
  public:
    tHBPlayerForm() : wxForm( USED_WXFORM_BUTTONS ) {}
    void OnHelp()
    {
      gpHelpInstance->ShowTopic("Harmony browser");
    }
};
#endif


/** show settings dialog for harmony browser*/
void HBPlayer::SettingsDialog(wxFrame *parent)
{
#ifdef OBSOLETE
  wxDialogBox *panel = new wxDialogBox(parent, "MIDI settings", FALSE );
  tHBPlayerForm      *form  = new tHBPlayerForm;

  form->Add(wxMakeFormMessage("Note Length for paste into piano window"));
  form->Add(wxMakeFormNewLine());
  form->Add(wxMakeFormShort("Length", &note_length, wxFORM_DEFAULT, new wxList(wxMakeConstraintRange(10.0, 120.0), 0)));
  form->Add(wxMakeFormNewLine());

  panel->SetLabelPosition(wxHORIZONTAL);

  form->Add(wxMakeFormBool("Bass enable", &bass_enabled));
  form->Add(wxMakeFormNewLine());
  form->Add(wxMakeFormShort("Channel", &bass_channel, wxFORM_DEFAULT, new wxList(wxMakeConstraintRange(1.0, 16.0), 0)));
  form->Add(wxMakeFormNewLine());
  form->Add(wxMakeFormShort("Velocity", &bass_veloc, wxFORM_DEFAULT, new wxList(wxMakeConstraintRange(1.0, 127.0), 0)));
  form->Add(wxMakeFormNewLine());
  form->Add(wxMakeFormShort("Pitch", &bass_pitch, wxFORM_DEFAULT, new wxList(wxMakeConstraintRange(30.0, 99.0), 0)));
  form->Add(wxMakeFormNewLine());

  form->Add(wxMakeFormBool("Chord enable", &chord_enabled));
  form->Add(wxMakeFormNewLine());
  form->Add(wxMakeFormShort("Channel", &chord_channel, wxFORM_DEFAULT, new wxList(wxMakeConstraintRange(1.0, 16.0), 0)));
  form->Add(wxMakeFormNewLine());
  form->Add(wxMakeFormShort("Velocity", &chord_veloc, wxFORM_DEFAULT, new wxList(wxMakeConstraintRange(1.0, 127.0), 0)));
  form->Add(wxMakeFormNewLine());
  form->Add(wxMakeFormShort("Pitch", &chord_pitch, wxFORM_DEFAULT, new wxList(wxMakeConstraintRange(30.0, 99.0), 0)));
  form->Add(wxMakeFormNewLine());

#if 0
  form->Add(wxMakeFormBool("Scale enable", &meldy_enabled));
  form->Add(wxMakeFormNewLine());
  form->Add(wxMakeFormShort("Channel", &meldy_channel, wxFORM_DEFAULT, new wxList(wxMakeConstraintRange(1.0, 16.0), 0)));
  form->Add(wxMakeFormNewLine());
  form->Add(wxMakeFormShort("Velocity", &meldy_veloc, wxFORM_DEFAULT, new wxList(wxMakeConstraintRange(1.0, 127.0), 0)));
  form->Add(wxMakeFormNewLine());
  form->Add(wxMakeFormShort("Pitch", &meldy_pitch, wxFORM_DEFAULT, new wxList(wxMakeConstraintRange(30.0, 99.0), 0)));
  form->Add(wxMakeFormNewLine());
  form->Add(wxMakeFormShort("Speed", &meldy_speed, wxFORM_DEFAULT, new wxList(wxMakeConstraintRange(60.0, 180.0), 0)));
  form->Add(wxMakeFormNewLine());
#endif

  form->AssociatePanel(panel);
  panel->Fit();

  panel->Show(TRUE);
#endif
}

// =============================================================
// HBCanvas
// =============================================================

/** painting component for the harmony browser*/
class HBCanvas : public wxScrolledWindow
{
    friend class HBSettingsDlg;
    friend class HBFrame;
    friend class HBMatchMarkers;
    friend ostream & operator << (ostream &os, HBCanvas const &a);
    friend istream & operator >> (istream &is, HBCanvas &a);
  public:
    HBCanvas(wxFrame *parent, int x, int y, int w, int h);
    virtual ~HBCanvas();
    virtual void OnDraw(wxDC& dc);
    void DrawMarkers(const HBContext &c, wxDC* dc);
    void ClearSeq();
    virtual void OnEvent(wxMouseEvent &e);

    int SeqDefined()        { return n_seq > 0; }
    int GetChordKeys(int *out, int step, int n_steps);
    int GetSelectedChord(int *out);
    int GetSelectedScale(int *out);
    int GetBassKeys(int *out, int step, int n_steps);
    void SettingsDialog(wxFrame *parent);

    void OnMenuCommand(int id, wxToolBar *mpToolBar);
    void TransposeSelection();

    HBPlayer player;

    enum
    {
      SEQMAX = 256
    };

    HBAnalyzer * getAnalyzer();

  protected:
    static const int ScFa;
    void ChordRect(JZRectangle &r, const HBContext &ct);
    void DrawChord(const HBContext &ct);
    void UnDrawChord(const HBContext &ct);
    bool Find(float x, float y, HBContext &out);

  private:

    float xchord, ychord, wchord, hchord;
    float ofs;
    wxFrame *parent;

    HBContext *seq[SEQMAX];
    int n_seq;

    char *default_filename;
    bool has_changed;

    HBContext mouse_context;

    bool haunschild_layout;
    bool mark_4_common;
    bool mark_3_common;
    bool mark_2_common;
    bool mark_1_common;
    bool mark_b_common;
    bool mark_0_common;
    bool mark_1_semi;
    bool mark_251;
    bool mark_tritone;
    bool mark_piano;
    void SetMarker(int id, wxToolBar *mpToolBar);
    int  active_marker;

    static int  transpose_res;
    static int  analyze_res;

    static tScaleType scale_type;
    void SetScaleType(int menu_id, tScaleType st, wxToolBar *tb);
};

tScaleType HBCanvas::scale_type = Major;
const int HBCanvas::ScFa = 50;
int HBCanvas::transpose_res = 8;
int HBCanvas::analyze_res = 8;


HBCanvas::HBCanvas(wxFrame *p, int x, int y, int w, int h)
  : wxScrolledWindow(p, -1, wxPoint(x, y), wxSize(w, h))
{
  parent = p;
  n_seq  = 0;

  active_marker = 0;
  haunschild_layout = 0;
  mark_4_common = 0;
  mark_3_common = 0;
  mark_2_common = 0;
  mark_1_common = 0;
  mark_b_common = 0;
  mark_0_common = 0;
  mark_1_semi = 0;
  mark_251 = 0;
  mark_tritone = 0;
  mark_piano = 0;

  for (int i = 0; i < SEQMAX; i++)
    seq[i] = new HBContext();

  wxDC *dc = new wxClientDC(this);//GetDC();

  dc->SetFont(*wxSMALL_FONT);
  int tw, th;
  dc->GetTextExtent("xD#j75+9-x", &tw, &th);

  delete dc;

  wchord = 1.2 * tw;
  hchord = 2.5 * th;
  xchord = 50;
  ychord = 4 * hchord;
  ofs    = th/4;

  default_filename = copystring("noname.har");
  has_changed      = false;

  SetScrollbars(0, (int)(hchord + 0.5), 0, 12 + SEQMAX/8 + 2, 0, 4);
}


HBCanvas::~HBCanvas()
{
  if (player.IsPlaying())
    player.StopPlay();
  for (int i = 0; i < SEQMAX; i++)
    delete seq[i];
}


inline ostream & operator << (ostream &os, HBCanvas const &a)
{
  int i;
  os << 1 << endl;
  os << a.n_seq << endl;
  for (i = 0; i < a.n_seq; i++)
    os << *a.seq[i];
  return os;
}


inline istream & operator >> (istream &is, HBCanvas &a)
{
  int i, version;
  is >> version;
  if (version != 1) {
    wxMessageBox("Wrong file format!", "Error", wxOK);
    return is;
  }
  is >> a.n_seq;
  for (i = 0; i < a.n_seq; i++)
    is >> *a.seq[i];
  a.Refresh();
  return is;
}


void HBCanvas::SetMarker(int id, wxToolBar *mpToolBar)
{
  if (id != active_marker)
  {
    if (active_marker && mpToolBar->GetToolState(active_marker))
      mpToolBar->ToggleTool(active_marker, FALSE);
    mpToolBar->ToggleTool(id, TRUE);
    active_marker = id;
  }

  mark_4_common = 0;
  mark_3_common = 0;
  mark_2_common = 0;
  mark_1_common = 0;
  mark_b_common = 0;
  mark_0_common = 0;
  mark_1_semi = 0;
  mark_251 = 0;
  mark_tritone = 0;
  mark_piano = 0;

  switch (id)
  {
    case MEN_EQ4:        mark_4_common = 1; break;
    case MEN_EQ3:        mark_3_common = 1; break;
    case MEN_EQ2:        mark_2_common = 1; break;
    case MEN_EQ1:        mark_1_common = 1; break;
    case MEN_EQB:        mark_b_common = 1; break;
    case MEN_EQH:        mark_1_semi = 1; break;
    case MEN_EQ0:        mark_0_common = 1; break;
    case MEN_251:         mark_251 = 1; break;
    case MEN_TRITONE:        mark_tritone = 1; break;
    case MEN_PIANO:        mark_piano = 1; break;
  }

#ifndef wx_xt
  if (id > 0 && !mpToolBar->GetToolState(id))
    mpToolBar->ToggleTool(id, TRUE);
#endif

  Refresh();

}


int HBCanvas::GetChordKeys(int *out, int step, int n_steps)
{
  if (n_seq == 0)
    return 0;
  int i = step * n_seq / n_steps;
  return player.GetChordKeys(out, *seq[i]);
}


int HBCanvas::GetSelectedChord(int *out)
{
  return player.GetChordKeys(out, mouse_context);
}

int HBCanvas::GetSelectedScale(int *out)
{
  return player.GetMeldyKeys(out, mouse_context);
}


int HBCanvas::GetBassKeys(int *out, int step, int n_steps)
{
  if (n_seq == 0)
    return 0;
  int i = step * n_seq / n_steps;
  out[0] = player.GetBassKey(*seq[i]);
  return 1;
}


void HBCanvas::ChordRect(JZRectangle &r, const HBContext &ct)
{
  if (ct.SeqNr())
  {
    r.x = (int)(xchord + (ct.SeqNr() - 1) % 8 * wchord);
    r.y = (int)(hchord * ((ct.SeqNr() -1) / 8 + 0.5));
  }
  else if (!haunschild_layout)
  {
    r.x = (int)(xchord + ct.ChordNr() * wchord);
    r.y = (int)(ychord + ct.ScaleNr() * hchord);
  }
  else
  {
    r.x = (int)(xchord + (5 * ct.ChordNr() % 7 + 5 * ct.ScaleNr() % 12) % 7 * wchord);
    r.y = (int)(ychord + (5 * ct.ScaleNr() % 12) * hchord);
  }

  r.x += (int)ofs;
  r.y -= (int)ofs;
  r.width =  (int)(wchord - 2 * ofs);
  r.height =  (int)(hchord - 2 * ofs);
}


void HBCanvas::DrawChord(const HBContext &ct)
{
  // draw surrounding box
  JZRectangle r;
  ChordRect(r, ct);
  wxDC *dc = new wxClientDC(this);//GetDC();
  dc->DrawRectangle(r.x, r.y, r.width, r.height);

  int w, h;
  const char *name = ct.ChordName();
  dc->GetTextExtent(name, &w, &h);
  dc->DrawText((char *)name, r.x + (r.width - w)/2, r.y + (r.height - h)/2);
  delete dc;
}


void HBCanvas::UnDrawChord(const HBContext &ct)
{
  // draw surrounding box
  JZRectangle r;
  ChordRect(r, ct);

  wxDC *dc = new wxClientDC(this);//GetDC();
  dc->SetPen(*wxWHITE_PEN);
  dc->DrawRectangle(r.x, r.y, r.width, r.height);
  dc->SetPen(*wxBLACK_PEN);
  delete dc;
}


void HBCanvas::OnDraw(wxDC& dcref)
{
  wxDC *dc = &dcref; //im just lazy, didnt want to change old code
  dc->Clear();
  dc->DrawText("Seq", 5, 5);

  ychord = (n_seq/8 + 1) * hchord + (n_seq % 8 ? hchord : 0) + hchord;

  HBContextIterator iter;
  iter.SetSequence(seq, n_seq);
  iter.SetScaleType(scale_type);
  while (iter())
  {
    const HBContext &ct = iter.Context();
    DrawChord(ct);
    if (ct.ChordNr() == 0 && ct.SeqNr() == 0)
    {
      JZRectangle r;
      ChordRect(r, ct);
      dc->DrawText((char *)ct.ScaleName(), 5, r.y);
    }
  }
  DrawMarkers(mouse_context, dc);

  if (!haunschild_layout)
  {
    for (int j = 0; j < 7; j++)
    {
      HBContext ct(0, j, scale_type);
      JZRectangle r;
      ChordRect(r, ct);
      r.y -= (int)hchord;
      int w, h;

      const char *name = ct.ChordNrName();
      dc->GetTextExtent(name, &w, &h);
      dc->DrawText((char *)name, r.x + (r.width - w)/2, r.y + (r.height - h)/2);

      const char *type = ct.ContextName();
      dc->GetTextExtent(type, &w, &h);
      dc->DrawText((char *)type, r.x + (r.width - w)/2, r.y + (r.height - h)/2 - h);
    }
  }

}

// -----------------------------------------------------------------------------
// HBSettingsForm
// -----------------------------------------------------------------------------

#ifdef OBSOLETE

class HBSettingsForm : public wxForm
{
  public:
    HBSettingsForm(HBCanvas *c)
        : wxForm( USED_WXFORM_BUTTONS )
    { cnvs = c; }
    virtual void OnOk() { cnvs->OnPaint(); wxForm::OnOk(); }
    virtual void OnHelp();
  private:
    HBCanvas *cnvs;
};

void HBSettingsForm::OnHelp()
{
  gpHelpInstance->ShowTopic("Harmony browser");
}

#endif

void HBCanvas::SettingsDialog(wxFrame *parent)
{
#ifdef OBSOLETE
  wxDialogBox *panel = new wxDialogBox(parent, "settings", FALSE );
  wxForm      *form  = new HBSettingsForm(this);

  panel->SetLabelPosition(wxHORIZONTAL);

  form->Add(wxMakeFormMessage("Transpose 1/8 notes per chord (0 = map sequence to selection)"));
  form->Add(wxMakeFormNewLine());
  form->Add(wxMakeFormShort("1/8 notes", &transpose_res, wxFORM_DEFAULT, new wxList(wxMakeConstraintRange(0.0, 32.0), 0)));
  form->Add(wxMakeFormNewLine());
  form->Add(wxMakeFormMessage("Analyze 1/8 per chord"));
  form->Add(wxMakeFormNewLine());
  form->Add(wxMakeFormShort("1/8 notes", &analyze_res, wxFORM_DEFAULT, new wxList(wxMakeConstraintRange(1.0, 32.0), 0)));
  form->Add(wxMakeFormNewLine());

  form->AssociatePanel(panel);
  panel->Fit();
  panel->Show(TRUE);
#endif
}

// -----------------------------------------------------------------------------
// HBMatchMarkers
// -----------------------------------------------------------------------------


class HBMatchMarkers : public HBMatch
{
  public:
    HBMatchMarkers(const HBContext &ct, HBCanvas *cv);
    virtual bool operator()(const HBContext &);
    const char * GetText() { return msg + 2; }        // + 2 for ", "
  private:
    HBCanvas  *cnvs;
    HBContext context;
    HBChord   chord;
    HBChord   scale;
    int       n_chord;
    int       chord_key;

    int       tritone;
    HBChord   piano;
    int       key251;

    char      msg[100];
};


HBMatchMarkers::HBMatchMarkers(const HBContext &ct, HBCanvas *cv)
  : context(ct)
{
  cnvs      = cv;
  chord     = context.Chord();
  n_chord   = chord.Count();
  chord_key = context.ChordKey();
  scale     = context.Scale();

  msg[0] = 0;
  msg[2] = 0;

  {
    // 251-move
    HBContext tmp(ct.ScaleNr(), ct.ChordNr() + 3, ct.ScaleType());
    key251 = tmp.ChordKey();
  }

  tritone = (chord_key + 6) % 12;

  if (cnvs->mark_piano)
  {
    tEventArray &buf = gpTrackFrame->GetPianoWindow()->mPasteBuffer;
    for (int i = 0; i < buf.nEvents; i++)
    {
      tKeyOn *on = buf.Events[i]->IsKeyOn();
      if (on)
        piano += on->Key;
    }
  }
}


bool HBMatchMarkers::operator()(const HBContext &o_context)
{
  HBChord o_chord = o_context.Chord();
  int     o_chord_key = o_context.ChordKey();

  HBChord common = (chord & o_chord);
  int n_common = common.Count();

  msg[0] = 0;
  msg[2] = 0;

  if (cnvs->mark_piano && o_chord.Contains(piano))
    strcat(msg, ", P");

  if (cnvs->mark_4_common && o_chord == chord)
    strcat(msg, ", =4");

  if (cnvs->mark_3_common && n_common == 3)
    strcat(msg, ", =3");

  if (cnvs->mark_2_common && n_common == 2)
    strcat(msg, ", =2");

  if (cnvs->mark_1_common && n_common == 1)
    strcat(msg, ", =1");

  if (cnvs->mark_0_common && n_common == 0)
    strcat(msg, ", =0");

  if (cnvs->mark_1_semi && n_common == n_chord - 1)
  {
    HBChord delta = chord ^ o_chord;
    int key = delta.Iter(0);
    if (delta.Contains(key + 1) || delta.Contains(key - 1))
      strcat(msg, ", 1/2");
  }

  if (cnvs->mark_251 && key251 == o_chord_key)
    strcat(msg, ", 251");

  if (cnvs->mark_b_common && chord_key == o_chord_key)
    strcat(msg, ", =B");

  if (cnvs->mark_tritone && o_chord_key == tritone)
    strcat(msg, ", =T");


  return msg[2] ? 1 : 0;
}


void HBCanvas::DrawMarkers(const HBContext &ct, wxDC* dc)
{
  JZRectangle r;
  //wxDC *dc = GetDC();
  dc->SetLogicalFunction(wxXOR);
  dc->SetBrush(*wxTRANSPARENT_BRUSH);
  HBMatchMarkers match(ct, this);
  HBContextIterator iter(match);
  iter.SetSequence(seq, n_seq);
  iter.SetScaleType(scale_type);
  while (iter())
  {
    ChordRect(r, iter.Context());
    r.x += 3;
    r.y += 3;
    r.width -= 6;
    r.height -= 6;
    dc->DrawRectangle(r.x, r.y, r.width, r.height);
  }

  // invert actual chord
  if (ct.ScaleType() == scale_type)
  {
    dc->SetBrush(*wxBLACK_BRUSH);
    ChordRect(r, ct);
    dc->DrawRectangle(r.x, r.y, r.width, r.height);
    if (ct.SeqNr() > 0)
    {
      HBContext c(ct);
      c.SetSeqNr(0);
      ChordRect(r, c);
      dc->DrawRectangle(r.x, r.y, r.width, r.height);
    }
  }
  dc->SetLogicalFunction(wxCOPY);
  dc->SetBrush(*wxWHITE_BRUSH);
}

bool HBCanvas::Find(float x, float y, HBContext &out)
{
  HBContextIterator iter;
  iter.SetSequence(seq, n_seq);
  iter.SetScaleType(scale_type);
  while (iter())
  {
    JZRectangle r;
    ChordRect(r, iter.Context());
    if (r.IsInside((int)x, (int)y))
    {
      out = iter.Context();
      return TRUE;
    }
  }
  return FALSE;
}


void HBCanvas::ClearSeq()
{
  n_seq = 0;
  mouse_context.SetSeqNr(0);
  Refresh();
}


void HBCanvas::OnEvent(wxMouseEvent &e)
{
  wxDC* dc= new wxClientDC(this);
  HBContext context;
  int x, y;
  e.GetPosition(&x, &y);
  if (Find(x, y, context))
  {
    if (e.ButtonDown())
    {
      player.StartPlay(context);
      if (e.LeftDown() && e.ShiftDown() || e.MiddleDown())
      {
        if (context.SeqNr())
        {
          // remove a chord
          if (context.SeqNr() == n_seq)
          {
            // remove markers first
            if (mouse_context.SeqNr() == n_seq)
            {
              DrawMarkers(mouse_context, dc);
              mouse_context.SetSeqNr(0);
              DrawMarkers(mouse_context, dc);
            }
            -- n_seq;
            UnDrawChord(context);
            context.SetSeqNr(0);
            Refresh();
          }
        }
        else if (n_seq < SEQMAX)
        {
          // add a chord
          context.SetSeqNr(n_seq + 1);
          *seq[n_seq ++] = context;
          DrawMarkers(mouse_context, dc);
          DrawChord(context);
          DrawMarkers(mouse_context, dc);
          Refresh();
        }
      }
    }
    else if (e.Dragging() && player.IsPlaying() && context != player.Context())
    {
      player.StopPlay();
      player.StartPlay(context);
    }

    if (e.LeftDown() || e.MiddleDown()) // && context != mouse_context)
    {
      DrawMarkers(mouse_context, dc);
      mouse_context = context;
      //mouse_context.SetSeqNr(0);
      DrawMarkers(mouse_context, dc);

      // paste to PianoWin buffer
      if (!mark_piano)
      {
        tEventArray &buf = gpTrackFrame->GetPianoWindow()->mPasteBuffer;
        buf.Clear();
        player.Paste(buf);
        gpTrackFrame->GetPianoWindow()->Refresh();
      }

      // Show in GuitarWin
      JZGuitarFrame* guitar = gpTrackFrame->GetPianoWindow()->GetGuitarFrame();
      if (guitar)
      {
        guitar->ShowPitch(0);        // remove actual pianowin/mouse position
//        guitar->Redraw();
        guitar->Update();
      }
    }
  }
  if (e.ButtonUp() && player.IsPlaying())
    player.StopPlay();
  delete dc;
}


void HBCanvas::SetScaleType(int menu_id, tScaleType st, wxToolBar *tb)
{
  scale_type = st;
  tb->ToggleTool(MEN_MAJSCALE, FALSE);
  tb->ToggleTool(MEN_HARSCALE, FALSE);
  tb->ToggleTool(MEN_MELSCALE, FALSE);
  tb->ToggleTool(MEN_IONSCALE, FALSE);
  tb->ToggleTool(menu_id, TRUE);
  Refresh();
}


void HBCanvas::TransposeSelection()
{
  if (!SeqDefined())
  {
    wxMessageBox("define a chord sequence first", "error", wxOK);
    return;
  }
  if (gpTrackWindow->EventsSelected("please select destination range in track window"))
  {
    wxBeginBusyCursor();
    HBAnalyzer analyzer(seq, n_seq);
    analyzer.Transpose(gpTrackWindow->mpFilter, transpose_res);
    wxEndBusyCursor();
  }
}

void HBCanvas::OnMenuCommand(int id, wxToolBar *mpToolBar)
{
  switch (id)
  {
    case MEN_LOAD:
      {
        wxString fname = file_selector(
          default_filename,
          "Load Harmonies",
          false,
          has_changed,
          "*.har");
        if (fname)
        {
          ifstream is(fname);
          is >> *this;
        }
      }
      break;

    case MEN_SAVE:
      {
        wxString fname = file_selector(
          default_filename,
          "Save Harmonies",
          true,
          has_changed,
          "*.har");
        if (fname)
        {
          ofstream os(fname);
          os << *this;
        }
      }
      break;

    case MEN_MAJSCALE:
      SetScaleType(id, Major, mpToolBar);
      break;

    case MEN_HARSCALE:
      SetScaleType(id, Harmon, mpToolBar);
      break;

    case MEN_MELSCALE:
      SetScaleType(id, Melod, mpToolBar);
      break;

    case MEN_IONSCALE:
      SetScaleType(id, Ionb13, mpToolBar);
      break;

    case MEN_HAUNSCH:
      haunschild_layout = !haunschild_layout;
      Refresh();
      break;

    case MEN_ANALYZE:
      if (gpTrackWindow->EventsSelected("please select source range in track window"))
      {
        wxBeginBusyCursor();
        HBAnalyzer analyzer(seq, (int)SEQMAX);
        n_seq = analyzer.Analyze(gpTrackWindow->mpFilter, analyze_res);
        Refresh();
        wxEndBusyCursor();
      }
      break;

    case MEN_TRANSPOSE:
      TransposeSelection();
      break;

    case MEN_SETTINGS:
      SettingsDialog(parent);
      break;

    default:
      SetMarker(id, mpToolBar);
      break;

  }
}


HBAnalyzer * HBCanvas::getAnalyzer()
{
  if (n_seq > 0 && gpTrackWindow->mpSnapSel->Selected)
  {
    HBAnalyzer *analyzer = new HBAnalyzer(seq, n_seq);
    analyzer->Init(gpTrackWindow->mpFilter, transpose_res);
    return analyzer;
  }
  return 0;
}

// ---------------------------------------------------------
// HBContextDlg
// ---------------------------------------------------------

struct tNamedChord
{
  const char *name;
  int bits;
};

const int n_chord_names = 12;
const int n_scale_names = 45;


tNamedChord chord_names[n_chord_names] = {
  { " j7",        0x891},
  { " m7",        0x489},
  { " 7",        0x491},
  { " m75-",        0x449},
  { " mj7",        0x889},
  { " j75+",        0x911},
  { " dim",        0x249},
  { " sus4",        0xa1},
  { " 7sus4",        0x4a1},
  { " j7sus4",        0x8a1},
  { " alt (79+13-)",        0x519},
  { " 75-",        0x451},
};

tNamedChord scale_names[n_scale_names] = {
  { "***** major scales *****",                0x0},
  { "maj I   (ionic)",                        0xab5},
  { "maj IV  (lydic)",                        0xad5},
  { "har III (ion #5)",                        0xb35},
  { "har VI  (lyd #9)",                        0xad9},
  { "mel III (lyd #5)",                        0xb55},
  { "augmented",                        0x333},
  { "hj I    (ionic b13)",                0x9b5},
  { "***** minor scales *****",                0x0},
  { "minor penta",                        0x4a9},
  { "maj VI   (aeolic)",                0x5ad},
  { "maj II   (doric)",                        0x6ad},
  { "mel II   (doric b9)",                0x6ab},
  { "maj III  (phrygic)",                0x5ab},
  { "japan penta",                        0x4a3},
  { "har IV   (dor #11)",                0x6cd},
  { "har I    (harmonic minor)",        0x9ad},
  { "mel I    (melodic minor)",                0xaad},
  { "gipsy",                                0x9cd},
  { "hj IV    (melodic #11)",                0xacd},
  { "***** dominant scales *****",        0x0},
  { "major penta",                        0x295},
  { "ind. penta",                        0x4b1},
  { "maj V (mixolyd)",                        0x6b5},
  { "har V (har dominant)",                0x5b3},
  { "mel IV (mixo #11)",                0x6d5},
  { "mixo #11b9",                        0x6d3},
  { "mel V (mixo b13)",                        0x5b5},
  { "hj V  (mixo b9)",                        0x6b3},
  { "full",                                0x555},
  { "hj III (har alt)",                        0x59b},
  { "mel VII (alt)",                        0x55b},
  { "half/full",                        0x6db},
  { "***** semi dimin *****",                0x0},
  { "maj VII (locr)",                        0x56b},
  { "mel VI  (locr 9)",                        0x56d},
  { "har II  (locr 13)",                0x66b},
  { "hj II   (doric b5)",                0x66d},
  { "***** dimin *****",                0x0},
  { "har VII (har dim)",                0x35b},
  { "full/half",                        0xa6d},
  { "hj VII  (locr dim)",                0x36b},
  { "***** blues scales *****",                0x0},
  { "minor penta b5",                        0x4e9},
  { "blues scale",                        0x4f9},
};


class HBContextDlg : public wxDialog
{
  public:
    HBContextDlg(HBCanvas *c, wxFrame *parent, HBContext *pcontext);
    ~HBContextDlg();
  /*    static void OkButton(wxButton &but, wxCommandEvent& event);
    static void CancelButton(wxButton &but, wxCommandEvent& event);
    static void PlayButton(wxButton &but, wxCommandEvent& event);
    static void HelpButton(wxButton &but, wxCommandEvent& event);
    static void ChordCheck(wxControl &item, wxCommandEvent& event);
    static void ScaleCheck(wxControl &item, wxCommandEvent& event);
    static void ChordList(wxControl &item, wxCommandEvent& event);
    static void ScaleList(wxControl &item, wxCommandEvent& event);
  */
    void OnOkButton();
    void OnCancelButton();
    void OnPlayButton();
    void OnChordCheck();
    void OnScaleCheck();
    void OnChordList();
    void OnScaleList();

    void OnHelp();

    void ShowValues();

  private:

    HBCanvas   *cnvs;
    wxCheckBox *chord_chk[12];
    wxCheckBox *scale_chk[12];
    wxListBox  *chord_lst;
    wxListBox  *scale_lst;
    wxStaticText  *chord_msg;

    wxButton   *ok_but;
    wxButton   *cancel_but;
    wxButton   *play_but;
    wxButton   *help_but;

    HBChord    chord;
    HBChord    scale;
    int        chord_key;
    int        scale_key;
    int               ChordKey(int i = 0) const { return (chord_key + i) % 12; }
    int               ScaleKey(int i = 0) const { return (chord_key + i) % 12; } // yes, its chord_key!
    HBContext  *pcontext;

    HBPlayer   player;
    void       RestartPlayer();
};


HBContextDlg::HBContextDlg(HBCanvas *c, wxFrame *parent, HBContext *pct)
  : wxDialog(parent, -1, "Edit chord/scale" )
{
  int i;

  cnvs = c;
  pcontext = pct;
  chord = pcontext->Chord();
  scale = pcontext->Scale();
  chord_key = pcontext->ChordKey();
  scale_key = pcontext->ScaleKey();

  // buttons
  ok_but     = new wxButton(this,  -1, "Ok") ;
  cancel_but = new wxButton(this,  -1, "Cancel") ;
  play_but   = new wxButton(this,  -1, "Play") ;
  help_but   = new wxButton(this,  -1, "Help" );
  //NewLine();

  // top messages
  (void)new wxStaticText(this, -1, "Chord: ");
  chord_msg = new wxStaticText(this, -1, "Am75-13-sus4");
  //NewLine();

  // chord/scale keys

  //have some defaults here for various wx versions
  int w = 45;
  int h = 40;
  int y = 80;

  const char *notename[12] = { "1", 0, "9", 0, "3", "11", 0, "5", 0, "13", 0, "7" };
  for (i = 0; i < 12; i++)
  {
    int x = w * i + 10;
    chord_chk[i] = new wxCheckBox(this, -1,  " ", wxPoint(x, y+1*h));//(wxFunction)ChordCheck,
    scale_chk[i] = new wxCheckBox(this, -1,  " ", wxPoint(x, y+2*h));//(wxFunction)ScaleCheck,
    if (notename[i])
      (void) new wxStaticText(this, -1, (char *)notename[i], wxPoint(x, y+3*h));
    (void) new wxStaticText(this, -1, (char *)HBChord::ScaleName(i + chord_key), wxPoint(x, y+0*h));
  }
  y += 4*h;

  // list boxes                                                               x  y    w    h
#ifdef OBSOLETE
  SetLabelPosition(wxVERTICAL);
#endif

  wxString* cnames = new wxString[n_chord_names];

  for (i = 0; i < n_chord_names; i++)
    cnames[i] = (char *)chord_names[i].name;

  chord_lst = new wxListBox(this, -1,   wxPoint(10, y), wxSize(100, 200), n_chord_names, cnames, wxLB_SINGLE| wxLB_NEEDED_SB);//"Chords"

  delete [] cnames;

  wxString* snames = new wxString[n_scale_names];
  for (i = 0; i < n_scale_names; i++)
    snames[i] = (char *)scale_names[i].name;
  scale_lst = new wxListBox(this, -1, wxPoint(200, y), wxSize(300, 200), n_scale_names, snames, wxLB_SINGLE| wxLB_NEEDED_SB);//"Scales",
  delete [] snames;

  // thats it
  Fit();
  Show(TRUE);
  ShowValues();
}

HBContextDlg::~HBContextDlg()
{
  if (player.IsPlaying())
    player.StopPlay();
}


void HBContextDlg::ShowValues()
{
  // show single notes
  int i;
  char buf[30];
  chord.Name(buf, ChordKey(0));
  chord_msg->SetLabel(buf);
  for (i = 0; i < 12; i++)
  {
    chord_chk[i]->SetValue(0 != chord.Contains(ChordKey(i)));
    scale_chk[i]->SetValue(0 != scale.Contains(ScaleKey(i)));
  }

  // update chord list if necessary
  HBChord c = chord;
  c.Rotate(-ChordKey());
  i = chord_lst->GetSelection();
  if (i < 0 || c.Keys() != chord_names[i].bits)
  {
    for (i = 0; i < n_chord_names; i++)
    {
      if (chord_names[i].bits == c.Keys())
      {
        chord_lst->SetSelection(i);
        break;
      }
    }
  }

  // update scale list
  HBChord s = scale;
  s.Rotate(-ScaleKey());
  i = chord_lst->GetSelection();
  if (i < 0 || s.Keys() != scale_names[i].bits)
  {
    for (i = 0; i < n_scale_names; i++)
    {
      if (scale_names[i].bits == s.Keys())
      {
        scale_lst->SetSelection(i);
        break;
      }
    }
  }

}

void HBContextDlg::OnOkButton()
{
  chord.Clear();
  scale.Clear();
  for (int i = 0; i < 12; i++)
  {
    if (chord_chk[i]->GetValue())
      chord += ChordKey(i);
    if (scale_chk[i]->GetValue())
      scale += ScaleKey(i);
  }
  *pcontext->PChord() = chord;
  *pcontext->PScale() = scale;
  cnvs->Refresh();
//  DELETE_THIS();
  Destroy();
}

void HBContextDlg::OnCancelButton()
{
//  DELETE_THIS();
  Destroy();
}

void HBContextDlg::OnPlayButton()
{
  if (player.IsPlaying())
  {
    play_but->SetLabel("play");
    player.StopPlay();
  }
  else
  {
    HBContext ct(*pcontext);
    *ct.PChord() = chord;
    *ct.PScale() = scale;
    player.StartPlay(ct);
    play_but->SetLabel("stop");
  }
}

void HBContextDlg::OnHelp()
{
  gpHelpInstance->ShowTopic("Edit chord");
}


void HBContextDlg::RestartPlayer()
{
  if (player.IsPlaying())
  {
    player.StopPlay();
    HBContext ct(*pcontext);
    *ct.PChord() = chord;
    *ct.PScale() = scale;
    player.StartPlay(ct);
  }
}

void HBContextDlg::OnChordCheck()
{
  char buf[30];
  chord.Clear();
  for (int i = 0; i < 12; i++)
  {
    if (chord_chk[i]->GetValue())
      chord += ChordKey(i);
  }
  chord.Name(buf, ChordKey());
  chord_msg->SetLabel(buf);
  RestartPlayer();
}


void HBContextDlg::OnScaleCheck()
{
  scale.Clear();
  for (int i = 0; i < 12; i++)
  {
    if (scale_chk[i]->GetValue())
      scale += ScaleKey(i);
  }
  RestartPlayer();
}

void HBContextDlg::OnChordList()
{
  int i = chord_lst->GetSelection();
  if (i >= 0)
  {
    HBChord c(chord_names[i].bits);
    c.Rotate(ChordKey());
    chord = c;
    ShowValues();
    RestartPlayer();
  }
}


void HBContextDlg::OnScaleList()
{
  int i = scale_lst->GetSelection();
  if (i >= 0)
  {
    HBChord s(scale_names[i].bits);
    s.Rotate(ScaleKey());
    scale = s;
    ShowValues();
    RestartPlayer();
  }
}

// ------------------- HBFrame ---------------------------

HBFrame::~HBFrame()
{
  int XPixel, YPixel;
  GetPosition(&XPixel, &YPixel);
  gpConfig->Put(C_HarmonyXpos, XPixel);
  gpConfig->Put(C_HarmonyYpos, YPixel);
  delete mpToolBar;
  delete cnvs;
  gpHarmonyBrowser = 0;
}

bool HBFrame::OnClose()
{
  return true;
}

int HBFrame::SeqDefined()
{
  return cnvs->SeqDefined();
}


int HBFrame::SeqSelected()
{
  if (cnvs->n_seq == 0 || cnvs->mouse_context.SeqNr() == 0)
  {
    wxMessageBox("select a chord from sequence first", "error", wxOK);
    return 0;
  }
  return 1;
}


int HBFrame::GetChordKeys(int *out, int step, int n_steps)
{
  return cnvs->GetChordKeys(out, step, n_steps);
}

int HBFrame::GetSelectedChord(int *out)
{
  return cnvs->GetSelectedChord(out);
}

int HBFrame::GetSelectedScale(int *out)
{
  return cnvs->GetSelectedScale(out);
}

int HBFrame::GetBassKeys(int *out, int step, int n_steps)
{
  return cnvs->GetBassKeys(out, step, n_steps);
}


void HBFrame::OnMenuCommand(int id)
{
  switch(id)
  {
    case MEN_HELP:
        gpHelpInstance->ShowTopic("Harmony browser");
        break;

    case MEN_MOUSE:
        wxMessageBox("left: select chord\n"
                      "  +shift: put chord into sequence\n"
                      "middle: same as left+shift\n"
                      "right: play chord\n", "Mousebuttons", wxOK);
        break;

    case MEN_MIDI:
      cnvs->player.SettingsDialog(this);
      break;

    case MEN_CLEARSEQ:
      cnvs->ClearSeq();
      break;

    case MEN_EDIT:
      {
        if (!SeqSelected())
          return;
      }
      (void) new HBContextDlg(cnvs, this, cnvs->seq[cnvs->mouse_context.SeqNr() - 1]);
      break;

    case MEN_CLOSE:
//      DELETE_THIS();
      Destroy();
      break;

    default:
      cnvs->OnMenuCommand(id, mpToolBar->GetDelegateToolBar());
      break;
  }
}


HBFrame::HBFrame(wxFrame *parent)
  : wxFrame(
      0,
      wxID_ANY,
      "Harmony Browser",
      wxPoint(
        gpConfig->GetValue(C_HarmonyXpos),
        gpConfig->GetValue(C_HarmonyYpos)),
      wxSize(660, 530))
{
  int w, h;
  cnvs = 0;
  genmeldy = 0;

  mpToolBar = new JZToolBar(this, tdefs);

  wxMenuBar *menu_bar = new wxMenuBar;
  wxMenu    *menu;
  /*
  wxMenu    *menu = new wxMenu;
  menu->Append(MEN_EDIT,        "&Edit chord");
  menu->Append(MEN_SETTINGS,        "Se&ttings");
  menu->Append(MEN_MIDI,        "&Midi");
  menu->Append(MEN_HELP,        "&Help");
  menu->Append(MEN_MOUSE,        "Help &mouse");
  menu->Append(MEN_LOAD,        "&Load");
  menu->Append(MEN_SAVE,        "&Save");
  menu->Append(MEN_CLOSE,        "Cl&ose");
  menu_bar->Append(menu,        "&Menu");
  */

  menu = new wxMenu("",wxMENU_TEAROFF);
  menu->Append(MEN_LOAD, "&Load");
  menu->Append(MEN_SAVE, "&Save");
  menu->Append(MEN_CLOSE, "&Close");
  menu_bar->Append(menu, "&File");

  menu = new wxMenu("",wxMENU_TEAROFF);
  menu->Append(MEN_EDIT, "&Chord");
  menu->Append(MEN_SETTINGS, "&Global");
  menu->Append(MEN_MIDI, "&Midi");
  menu->Append(MEN_HAUNSCH, "&Haunschild Layout");
  menu_bar->Append(menu, "&Settings");

  menu = new wxMenu("",wxMENU_TEAROFF);
  menu->Append(MEN_MAJSCALE, "&Major");
  menu->Append(MEN_HARSCALE, "&Harmonic Minor");
  menu->Append(MEN_MELSCALE, "&Melodic Minor");
  menu->Append(MEN_IONSCALE, "&Ionic");
  menu_bar->Append(menu, "&Scale");

  menu = new wxMenu("",wxMENU_TEAROFF);
  menu->Append(MEN_EQ4, "&4 equal notes");
  menu->Append(MEN_EQ3, "&3 equal notes");
  menu->Append(MEN_EQ2, "&2 equal notes");
  menu->Append(MEN_EQ1, "&1 equal note");
  menu->Append(MEN_EQ0, "&0 equal notes");
  menu->Append(MEN_EQH, "1/2 note &difference");
  menu->Append(MEN_251, "2-5-1 &move");
  menu->Append(MEN_EQB, "&Same bass note");
  menu->Append(MEN_TRITONE, "&Tritone substitute");
  menu->Append(MEN_PIANO, "Contains &Pianowin Buffer");
  menu_bar->Append(menu, "&Show");

  menu = new wxMenu("",wxMENU_TEAROFF);
  menu->Append(MEN_TRANSPOSE, "&Transpose");
  menu->Append(MEN_ANALYZE,   "&Analyze");
  menu->AppendSeparator();
  menu->Append(MEN_CLEARSEQ, "&Clear Sequence");
  menu_bar->Append(menu, "&Action");

  menu = new wxMenu("",wxMENU_TEAROFF);
  menu->Append(MEN_HELP, "&Harmony Browser");
  menu->Append(MEN_MOUSE, "&Mouse");
  menu_bar->Append(menu, "&Help");


  SetMenuBar(menu_bar);

  GetClientSize(&w, &h);
  cnvs =  new HBCanvas(this, 0, 0, w, h);
  mpToolBar->ToggleTool(MEN_MAJSCALE, TRUE);
}

HBAnalyzer * HBFrame::getAnalyzer()
{
  return cnvs->getAnalyzer();
}


BEGIN_EVENT_TABLE(HBFrame, wxFrame)
  EVT_SIZE(HBFrame::OnSize)
END_EVENT_TABLE()

void HBFrame::OnSize(wxSizeEvent& Event)
{
  int frameWidth, frameHeight;
  GetClientSize(&frameWidth, &frameHeight);

  cout << "HBFrame::OnSize"<<endl;

#ifdef OBSOLETE
  float th = 0.0;
#endif
  if (cnvs)
       cnvs->SetSize(0, (int)0, (int)frameWidth, (int)(frameHeight));
  //    cnvs->SetSize(0, (int)th, cw, ch - (int)th);

#ifdef OBSOLETE
  float tw = 0.0;
  if (mpToolBar)
    mpToolBar->GetMaxSize(&tw, &th);
  if (mpToolBar)
    mpToolBar->SetSize(0, 0, (int)cw, (int)th);
#endif
}

void HBFrame::TransposeSelection()
{
  cnvs->TransposeSelection();
}

void CreateHarmonyBrowser(wxFrame* pParent)
{
  if (!gpHarmonyBrowser)
  {
    gpHarmonyBrowser = new HBFrame(pParent);
  }
  ((HBFrame *)gpHarmonyBrowser)->Show(true);
}

