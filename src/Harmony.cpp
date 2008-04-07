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

#define MEN_CLOSE       1
#define MEN_MIDI        2
#define MEN_TRANSPOSE   4
#define MEN_CLEARSEQ    6
#define MEN_EDIT        7
#define MEN_MOUSE       8
#define MEN_HELP        9

#define MEN_MAJSCALE    10
#define MEN_HARSCALE    11
#define MEN_MELSCALE    12

#define MEN_EQ4         13
#define MEN_EQ3         14
#define MEN_EQ2         15
#define MEN_EQ1         16
#define MEN_EQH         17
#define MEN_EQ0         18
#define MEN_251         19
#define MEN_TRITONE     20
#define MEN_PIANO       21
#define MEN_EQB         22
#define MEN_HAUNSCH     23
#define MEN_ANALYZE     24
#define MEN_IONSCALE    25
#define MEN_SETTINGS    26
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
  { MEN_LOAD,      false, open_xpm,     "open harmony file" },
  { MEN_SAVE,      false, save_xpm,     "save harmony file" },
  { JZToolBar::eToolBarSeparator },
  { MEN_MAJSCALE,  true,  majscale_xpm, "major scale" },
  { MEN_HARSCALE,  true,  harscale_xpm, "harmonic scale" },
  { MEN_MELSCALE,  true,  melscale_xpm, "melodic scale" },
  { MEN_IONSCALE,  true,  ionscale_xpm, "ionic b13 scale" },
  { JZToolBar::eToolBarSeparator },
  { MEN_EQ4,       true,  same4_xpm,    "4 common notes" },
  { MEN_EQ3,       true,  same3_xpm,    "3 common notes" },
  { MEN_EQ2,       true,  same2_xpm,    "2 common notes" },
  { MEN_EQ1,       true,  same1_xpm,    "1 common note" },
  { MEN_EQ0,       true,  same0_xpm,    "0 common notes" },
  { MEN_EQH,       true,  sameh_xpm,    "one half note difference" },
  { MEN_251,       true,  std251_xpm,   "next in 2-5-1 move" },
  { MEN_EQB,       true,  sameb_xpm,    "same base note" },
  { MEN_TRITONE,   true,  tritone_xpm,  "tritone substitute" },
  { MEN_PIANO,     true,  piano_xpm,    "pianowin copy buffer" },
  { JZToolBar::eToolBarSeparator },
  { MEN_HAUNSCH,   true,  haunsch_xpm,  "haunschild layout" },
  { JZToolBar::eToolBarSeparator },
  { MEN_TRANSPOSE, false, transpos_xpm, "transpose trackwin selection" },
  { MEN_ANALYZE,   false, analyze_xpm,  "analyze trackwin selection" },
  { MEN_CLEARSEQ,  false, delchord_xpm, "clear harmonies" },
  { JZToolBar::eToolBarEnd }
};


//*****************************************************************************
// Description:
//   This class handles the playing of the harmony.
//*****************************************************************************
class HBPlayer : public wxTimer
{
    friend class HBCanvas;

  public:

    HBPlayer();

    void StartPlay(const HBContext &);

    void Paste(tEventArray &);

    void StopPlay();

    void SettingsDialog(wxFrame *parent);

    int IsPlaying() const
    {
      return playing;
    }

    const HBContext& Context()
    {
      return context;
    }

    virtual void Notify();

    int GetChordKeys(int *out, const HBContext &);

    int GetMeldyKeys(int *out, const HBContext &);

    int GetBassKey(const HBContext &);

  private:

    HBContext context;

    static int bass_channel, bass_veloc;

    static int chord_channel, chord_veloc;

    static int meldy_channel, meldy_veloc;

    static bool mBassEnabled, mChordEnabled, mMeldyEnabled;

    static int bass_pitch, chord_pitch, meldy_pitch;

    static int meldy_speed;

    int bass_key, chord_keys[12], n_chord_keys;

    int meldy_keys[12], n_meldy_keys, meldy_index;

    int note_length;

    int playing;

    int device;

};

bool HBPlayer::mBassEnabled = true;
int HBPlayer::bass_channel  = 1;
int HBPlayer::bass_veloc    = 90;
int HBPlayer::bass_pitch    = 40;

bool HBPlayer::mChordEnabled = true;
int HBPlayer::chord_channel  = 2;
int HBPlayer::chord_veloc    = 90;
int HBPlayer::chord_pitch    = 60;

bool HBPlayer::mMeldyEnabled = false;
int HBPlayer::meldy_channel  = 3;
int HBPlayer::meldy_veloc    = 90;
int HBPlayer::meldy_pitch    = 70;
int HBPlayer::meldy_speed    = 100;


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
  {
    key += 12;
  }
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
  if (mBassEnabled)
  {
    tKeyOn e(0, bass_channel - 1, bass_key, bass_veloc, note_length);
    arr.Put(e.Copy());
  }

  if (mChordEnabled)
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
  {
    StopPlay();
  }
  playing = 1;
  context = ct;

  bass_key = GetBassKey(context);
  n_chord_keys = GetChordKeys(chord_keys, context);
  n_meldy_keys = GetMeldyKeys(meldy_keys, context);

  // Generate KeyOn's
  if (mBassEnabled)
  {
    tKeyOn e(0, bass_channel - 1, bass_key, bass_veloc);
    gpMidiPlayer->OutNow(device, &e);
  }

  if (mChordEnabled)
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
  if (mMeldyEnabled)
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
  {
    return;
  }
  Stop();
  playing = 0;

  int i;

  // Generate KeyOff's
  if (mBassEnabled)
  {
    tKeyOff e(0, bass_channel - 1, bass_key);
    gpMidiPlayer->OutNow(device, &e);
  }

  if (mChordEnabled)
  {
    for (i = 0; i < n_chord_keys; i++)
    {
      tKeyOff e(0, chord_channel - 1, chord_keys[i]);
      gpMidiPlayer->OutNow(device, &e);
    }
  }

  if (mMeldyEnabled)
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
  wxDialogBox *panel = new wxDialogBox(pParent, "MIDI settings", false );
  tHBPlayerForm      *form  = new tHBPlayerForm;

  form->Add(wxMakeFormMessage("Note Length for paste into piano window"));
  form->Add(wxMakeFormNewLine());
  form->Add(wxMakeFormShort("Length", &note_length, wxFORM_DEFAULT, new wxList(wxMakeConstraintRange(10.0, 120.0), 0)));
  form->Add(wxMakeFormNewLine());

  panel->SetLabelPosition(wxHORIZONTAL);

  form->Add(wxMakeFormBool("Bass enable", &mBassEnabled));
  form->Add(wxMakeFormNewLine());
  form->Add(wxMakeFormShort("Channel", &bass_channel, wxFORM_DEFAULT, new wxList(wxMakeConstraintRange(1.0, 16.0), 0)));
  form->Add(wxMakeFormNewLine());
  form->Add(wxMakeFormShort("Velocity", &bass_veloc, wxFORM_DEFAULT, new wxList(wxMakeConstraintRange(1.0, 127.0), 0)));
  form->Add(wxMakeFormNewLine());
  form->Add(wxMakeFormShort("Pitch", &bass_pitch, wxFORM_DEFAULT, new wxList(wxMakeConstraintRange(30.0, 99.0), 0)));
  form->Add(wxMakeFormNewLine());

  form->Add(wxMakeFormBool("Chord enable", &mChordEnabled));
  form->Add(wxMakeFormNewLine());
  form->Add(wxMakeFormShort("Channel", &chord_channel, wxFORM_DEFAULT, new wxList(wxMakeConstraintRange(1.0, 16.0), 0)));
  form->Add(wxMakeFormNewLine());
  form->Add(wxMakeFormShort("Velocity", &chord_veloc, wxFORM_DEFAULT, new wxList(wxMakeConstraintRange(1.0, 127.0), 0)));
  form->Add(wxMakeFormNewLine());
  form->Add(wxMakeFormShort("Pitch", &chord_pitch, wxFORM_DEFAULT, new wxList(wxMakeConstraintRange(30.0, 99.0), 0)));
  form->Add(wxMakeFormNewLine());

#if 0
  form->Add(wxMakeFormBool("Scale enable", &mMeldyEnabled));
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

  panel->Show(true);
#endif
}

//*****************************************************************************
// Description:
//   This is the harmony browser window.
//*****************************************************************************
class HBCanvas : public wxScrolledWindow
{
    friend class HBSettingsDlg;
    friend class HBFrame;
    friend class HBMatchMarkers;
    friend ostream & operator << (ostream &os, HBCanvas const &a);
    friend istream & operator >> (istream &is, HBCanvas &a);

  public:

    HBCanvas(wxFrame* pParent, int x, int y, int w, int h);

    virtual ~HBCanvas();

    virtual void OnDraw(wxDC& Dc);

    void DrawMarkers(wxDC& Dc, const HBContext &c);

    void ClearSeq();

    int SeqDefined()
    {
      return mSequenceCount > 0;
    }

    int GetChordKeys(int *out, int step, int n_steps);

    int GetSelectedChord(int *out);

    int GetSelectedScale(int *out);

    int GetBassKeys(int *out, int step, int n_steps);

    void SettingsDialog();

    void ToggleHaunschildLayout();

    void FileLoad();

    void OnMenuCommand(int id, wxToolBar *mpToolBar);

    void TransposeSelection();

    HBPlayer player;

    enum
    {
      SEQMAX = 256
    };

    HBAnalyzer* GetAnalyzer();

  protected:

    static const int ScFa;

    void ChordRect(JZRectangle& Rectangle, const HBContext &ct);

    void DrawChord(wxDC& Dc, const HBContext &ct);

    void UnDrawChord(wxDC& Dc, const HBContext &ct);

    bool Find(float x, float y, HBContext &out);

  private:

    virtual void OnMouseEvent(wxMouseEvent& MouseEvent);

    void SetMarker(int id, wxToolBar *mpToolBar);

    void SetScaleType(int menu_id, tScaleType st, wxToolBar *tb);

  private:

    static tScaleType scale_type;

    static int transpose_res;

    static int analyze_res;

    float mChordX, mChordY, mChordWidth, mChordHeight;

    float ofs;

//    wxFrame* parent;

    HBContext* seq[SEQMAX];

    int mSequenceCount;

    std::string mDefaultFileName;

    bool has_changed;

    HBContext mouse_context;

    bool mHaunschildLayout;

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

    int active_marker;

  DECLARE_EVENT_TABLE()
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
tScaleType HBCanvas::scale_type = Major;
const int HBCanvas::ScFa = 50;
int HBCanvas::transpose_res = 8;
int HBCanvas::analyze_res = 8;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(HBCanvas, wxScrolledWindow)

  EVT_MOUSE_EVENTS(HBCanvas::OnMouseEvent)

END_EVENT_TABLE()

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
HBCanvas::HBCanvas(wxFrame* pParent, int x, int y, int w, int h)
  : wxScrolledWindow(pParent, wxID_ANY, wxPoint(x, y), wxSize(w, h))
{
//  parent = pParent;
  mSequenceCount  = 0;

  active_marker = 0;
  mHaunschildLayout = false;
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
  {
    seq[i] = new HBContext();
  }

  wxClientDC Dc(this);

  Dc.SetFont(*wxSMALL_FONT);
  int TextWidth, TextHeight;
  Dc.GetTextExtent("xD#j75+9-x", &TextWidth, &TextHeight);

  mChordWidth = 1.2 * TextWidth;
  mChordHeight = 2.5 * TextHeight;
  mChordX = 50;
  mChordY = 4 * mChordHeight;
  ofs    = TextHeight / 4;

  mDefaultFileName = "noname.har";
  has_changed      = false;

  SetScrollbars(0, (int)(mChordHeight + 0.5), 0, 12 + SEQMAX / 8 + 2, 0, 0);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
HBCanvas::~HBCanvas()
{
  if (player.IsPlaying())
  {
    player.StopPlay();
  }
  for (int i = 0; i < SEQMAX; i++)
  {
    delete seq[i];
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
ostream & operator << (ostream& Os, HBCanvas const &a)
{
  int i;
  Os << 1 << endl;
  Os << a.mSequenceCount << endl;
  for (i = 0; i < a.mSequenceCount; i++)
  {
    Os << *a.seq[i];
  }
  return Os;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
istream& operator >> (istream& Is, HBCanvas &a)
{
  int i, version;
  Is >> version;
  if (version != 1)
  {
    wxMessageBox("Wrong file format!", "Error", wxOK);
    return Is;
  }
  Is >> a.mSequenceCount;
  for (i = 0; i < a.mSequenceCount; i++)
  {
    Is >> *a.seq[i];
  }
  a.Refresh();
  return Is;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void HBCanvas::SetMarker(int id, wxToolBar *mpToolBar)
{
  if (id != active_marker)
  {
    if (active_marker && mpToolBar->GetToolState(active_marker))
    {
      mpToolBar->ToggleTool(active_marker, false);
    }
    mpToolBar->ToggleTool(id, true);
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

  if (id > 0 && !mpToolBar->GetToolState(id))
  {
    mpToolBar->ToggleTool(id, true);
  }

  Refresh();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int HBCanvas::GetChordKeys(int *out, int step, int n_steps)
{
  if (mSequenceCount == 0)
  {
    return 0;
  }
  int i = step * mSequenceCount / n_steps;
  return player.GetChordKeys(out, *seq[i]);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int HBCanvas::GetSelectedChord(int *out)
{
  return player.GetChordKeys(out, mouse_context);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int HBCanvas::GetSelectedScale(int *out)
{
  return player.GetMeldyKeys(out, mouse_context);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int HBCanvas::GetBassKeys(int *out, int step, int n_steps)
{
  if (mSequenceCount == 0)
  {
    return 0;
  }
  int i = step * mSequenceCount / n_steps;
  out[0] = player.GetBassKey(*seq[i]);
  return 1;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void HBCanvas::ChordRect(JZRectangle& Rectangle, const HBContext &ct)
{
  if (ct.SeqNr())
  {
    Rectangle.x = (int)(mChordX + (ct.SeqNr() - 1) % 8 * mChordWidth);
    Rectangle.y = (int)(mChordHeight * ((ct.SeqNr() -1) / 8 + 0.5));
  }
  else if (!mHaunschildLayout)
  {
    Rectangle.x = (int)(mChordX + ct.ChordNr() * mChordWidth);
    Rectangle.y = (int)(mChordY + ct.ScaleNr() * mChordHeight);
  }
  else
  {
    Rectangle.x = (int)(
      mChordX + (5 * ct.ChordNr() % 7 + 5 * ct.ScaleNr() % 12) % 7 * mChordWidth);
    Rectangle.y = (int)(mChordY + (5 * ct.ScaleNr() % 12) * mChordHeight);
  }

  Rectangle.x += (int)ofs;
  Rectangle.y -= (int)ofs;
  Rectangle.width =  (int)(mChordWidth - 2 * ofs);
  Rectangle.height =  (int)(mChordHeight - 2 * ofs);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void HBCanvas::DrawChord(wxDC& Dc, const HBContext &ct)
{
  // Draw the surrounding box.
  JZRectangle Rectangle;
  ChordRect(Rectangle, ct);

  Dc.DrawRectangle(
    Rectangle.x,
    Rectangle.y,
    Rectangle.width,
    Rectangle.height);

  int w, h;
  const char* pName = ct.ChordName();
  Dc.GetTextExtent(pName, &w, &h);
  Dc.DrawText(
    pName,
    Rectangle.x + (Rectangle.width - w) / 2,
    Rectangle.y + (Rectangle.height - h) / 2);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void HBCanvas::UnDrawChord(wxDC& Dc, const HBContext& ct)
{
  // draw surrounding box
  JZRectangle Rectangle;
  ChordRect(Rectangle, ct);

  Dc.SetPen(*wxWHITE_PEN);
  Dc.DrawRectangle(
    Rectangle.x,
    Rectangle.y,
    Rectangle.width,
    Rectangle.height);
  Dc.SetPen(*wxBLACK_PEN);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void HBCanvas::OnDraw(wxDC& Dc)
{
  Dc.Clear();

  Dc.SetFont(*wxSMALL_FONT);

  Dc.DrawText("Seq", 5, 5);

  mChordY =
    (mSequenceCount / 8 + 1) * mChordHeight + (mSequenceCount % 8 ? mChordHeight : 0) +
    mChordHeight;

  HBContextIterator iter;
  iter.SetSequence(seq, mSequenceCount);
  iter.SetScaleType(scale_type);
  while (iter())
  {
    const HBContext &ct = iter.Context();
    DrawChord(Dc, ct);
    if (ct.ChordNr() == 0 && ct.SeqNr() == 0)
    {
      JZRectangle Rectangle;
      ChordRect(Rectangle, ct);
      Dc.DrawText(ct.ScaleName(), 5, Rectangle.y);
    }
  }
  DrawMarkers(Dc, mouse_context);

  if (!mHaunschildLayout)
  {
    for (int j = 0; j < 7; j++)
    {
      HBContext ct(0, j, scale_type);
      JZRectangle Rectangle;
      ChordRect(Rectangle, ct);
      Rectangle.y -= (int)mChordHeight;
      int w, h;

      const char *name = ct.ChordNrName();
      Dc.GetTextExtent(name, &w, &h);
      Dc.DrawText((char *)name, Rectangle.x + (Rectangle.width - w)/2, Rectangle.y + (Rectangle.height - h)/2);

      const char *type = ct.ContextName();
      Dc.GetTextExtent(type, &w, &h);
      Dc.DrawText((char *)type, Rectangle.x + (Rectangle.width - w)/2, Rectangle.y + (Rectangle.height - h)/2 - h);
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
    { mpHbWindow = c; }
    virtual void OnOk() { mpHbWindow->OnPaint(); wxForm::OnOk(); }
    virtual void OnHelp();
  private:
    HBCanvas *mpHbWindow;
};
#endif

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void HBCanvas::SettingsDialog()
{
#ifdef OBSOLETE
  wxDialogBox *panel = new wxDialogBox(this, "settings", false );
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
  panel->Show(true);
#endif
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void HBCanvas::ToggleHaunschildLayout()
{
  mHaunschildLayout = !mHaunschildLayout;
  Refresh();
}







//*****************************************************************************
// HBMatchMarkers
//*****************************************************************************
class HBMatchMarkers : public HBMatch
{
  public:

    HBMatchMarkers(const HBContext &ct, HBCanvas *cv);

    virtual bool operator()(const HBContext &);

    const char * GetText()
    {
      // + 2 for ", "
      return msg + 2;
    }

  private:

    HBCanvas* mpHbWindow;
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
  mpHbWindow      = cv;
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

  if (mpHbWindow->mark_piano)
  {
    tEventArray &buf = gpTrackFrame->GetPianoWindow()->mPasteBuffer;
    for (int i = 0; i < buf.nEvents; i++)
    {
      tKeyOn *on = buf.Events[i]->IsKeyOn();
      if (on)
      {
        piano += on->Key;
      }
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

  if (mpHbWindow->mark_piano && o_chord.Contains(piano))
  {
    strcat(msg, ", P");
  }

  if (mpHbWindow->mark_4_common && o_chord == chord)
  {
    strcat(msg, ", =4");
  }

  if (mpHbWindow->mark_3_common && n_common == 3)
  {
    strcat(msg, ", =3");
  }

  if (mpHbWindow->mark_2_common && n_common == 2)
  {
    strcat(msg, ", =2");
  }

  if (mpHbWindow->mark_1_common && n_common == 1)
  {
    strcat(msg, ", =1");
  }

  if (mpHbWindow->mark_0_common && n_common == 0)
  {
    strcat(msg, ", =0");
  }

  if (mpHbWindow->mark_1_semi && n_common == n_chord - 1)
  {
    HBChord delta = chord ^ o_chord;
    int key = delta.Iter(0);
    if (delta.Contains(key + 1) || delta.Contains(key - 1))
    {
      strcat(msg, ", 1/2");
    }
  }

  if (mpHbWindow->mark_251 && key251 == o_chord_key)
  {
    strcat(msg, ", 251");
  }

  if (mpHbWindow->mark_b_common && chord_key == o_chord_key)
  {
    strcat(msg, ", =B");
  }

  if (mpHbWindow->mark_tritone && o_chord_key == tritone)
  {
    strcat(msg, ", =T");
  }

  return msg[2] ? 1 : 0;
}

void HBCanvas::DrawMarkers(wxDC& Dc, const HBContext& ct)
{
  JZRectangle Rectangle;
  Dc.SetLogicalFunction(wxXOR);
  Dc.SetBrush(*wxTRANSPARENT_BRUSH);
  HBMatchMarkers match(ct, this);
  HBContextIterator iter(match);
  iter.SetSequence(seq, mSequenceCount);
  iter.SetScaleType(scale_type);
  while (iter())
  {
    ChordRect(Rectangle, iter.Context());
    Rectangle.x += 3;
    Rectangle.y += 3;
    Rectangle.width -= 6;
    Rectangle.height -= 6;
    Dc.DrawRectangle(
      Rectangle.x,
      Rectangle.y,
      Rectangle.width,
      Rectangle.height);
  }

  // invert actual chord
  if (ct.ScaleType() == scale_type)
  {
    Dc.SetBrush(*wxBLACK_BRUSH);
    ChordRect(Rectangle, ct);
    Dc.DrawRectangle(
      Rectangle.x,
      Rectangle.y,
      Rectangle.width,
      Rectangle.height);
    if (ct.SeqNr() > 0)
    {
      HBContext c(ct);
      c.SetSeqNr(0);
      ChordRect(Rectangle, c);
      Dc.DrawRectangle(
        Rectangle.x,
        Rectangle.y,
        Rectangle.width,
        Rectangle.height);
    }
  }
  Dc.SetLogicalFunction(wxCOPY);
  Dc.SetBrush(*wxWHITE_BRUSH);
}

bool HBCanvas::Find(float x, float y, HBContext &out)
{
  HBContextIterator iter;
  iter.SetSequence(seq, mSequenceCount);
  iter.SetScaleType(scale_type);
  while (iter())
  {
    JZRectangle Rectangle;
    ChordRect(Rectangle, iter.Context());
    if (Rectangle.IsInside((int)x, (int)y))
    {
      out = iter.Context();
      return true;
    }
  }
  return false;
}


void HBCanvas::ClearSeq()
{
  mSequenceCount = 0;
  mouse_context.SetSeqNr(0);
  Refresh();
}


void HBCanvas::OnMouseEvent(wxMouseEvent& MouseEvent)
{
  wxClientDC Dc(this);

  DoPrepareDC(Dc);

  Dc.SetFont(*wxSMALL_FONT);

  HBContext context;
  int x, y;
  MouseEvent.GetPosition(&x, &y);
  if (Find(x, y, context))
  {
    if (MouseEvent.ButtonDown())
    {
      player.StartPlay(context);
      if (
        MouseEvent.LeftDown() &&
        MouseEvent.ShiftDown() ||
        MouseEvent.MiddleDown())
      {
        if (context.SeqNr())
        {
          // remove a chord
          if (context.SeqNr() == mSequenceCount)
          {
            // remove markers first
            if (mouse_context.SeqNr() == mSequenceCount)
            {
              DrawMarkers(Dc, mouse_context);
              mouse_context.SetSeqNr(0);
              DrawMarkers(Dc, mouse_context);
            }
            --mSequenceCount;
            UnDrawChord(Dc, context);
            context.SetSeqNr(0);
            Refresh();
          }
        }
        else if (mSequenceCount < SEQMAX)
        {
          // add a chord
          context.SetSeqNr(mSequenceCount + 1);
          *seq[mSequenceCount++] = context;
          DrawMarkers(Dc, mouse_context);
          DrawChord(Dc, context);
          DrawMarkers(Dc, mouse_context);
          Refresh();
        }
      }
    }
    else if (
      MouseEvent.Dragging() && player.IsPlaying() && context != player.Context())
    {
      player.StopPlay();
      player.StartPlay(context);
    }

    if (MouseEvent.LeftDown() || MouseEvent.MiddleDown()) // && context != mouse_context)
    {
      DrawMarkers(Dc, mouse_context);
      mouse_context = context;
      //mouse_context.SetSeqNr(0);
      DrawMarkers(Dc, mouse_context);

      // paste to PianoWin buffer
      if (!mark_piano && gpTrackFrame->GetPianoWindow())
      {
        tEventArray &buf = gpTrackFrame->GetPianoWindow()->mPasteBuffer;
        buf.Clear();
        player.Paste(buf);
        gpTrackFrame->GetPianoWindow()->Refresh();
      }

      if (gpTrackFrame->GetPianoWindow())
      {
        // Show in GuitarWin
        JZGuitarFrame* guitar = gpTrackFrame->GetPianoWindow()->GetGuitarFrame();
        if (guitar)
        {
          // Remove actual pianowin/mouse position
          guitar->ShowPitch(0);
//          guitar->Redraw();
          guitar->Update();
        }
      }
    }
  }
  if (MouseEvent.ButtonUp() && player.IsPlaying())
  {
    player.StopPlay();
  }
}


void HBCanvas::SetScaleType(int menu_id, tScaleType st, wxToolBar *tb)
{
  scale_type = st;
  tb->ToggleTool(MEN_MAJSCALE, false);
  tb->ToggleTool(MEN_HARSCALE, false);
  tb->ToggleTool(MEN_MELSCALE, false);
  tb->ToggleTool(MEN_IONSCALE, false);
  tb->ToggleTool(menu_id, true);
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
    HBAnalyzer analyzer(seq, mSequenceCount);
    analyzer.Transpose(gpTrackWindow->mpFilter, transpose_res);
    wxEndBusyCursor();
  }
}

void HBCanvas::FileLoad()
{
  wxString FileName = file_selector(
    mDefaultFileName.c_str(),
    "Load Harmonies",
    false,
    has_changed,
    "*.har");

  ifstream Is(FileName.c_str());
  Is >> *this;
}

void HBCanvas::OnMenuCommand(int id, wxToolBar *mpToolBar)
{
  switch (id)
  {
    case MEN_SAVE:
      {
        wxString fname = file_selector(
          mDefaultFileName.c_str(),
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

    case MEN_ANALYZE:
      if (gpTrackWindow->EventsSelected("please select source range in track window"))
      {
        wxBeginBusyCursor();
        HBAnalyzer analyzer(seq, (int)SEQMAX);
        mSequenceCount = analyzer.Analyze(gpTrackWindow->mpFilter, analyze_res);
        Refresh();
        wxEndBusyCursor();
      }
      break;

    case MEN_TRANSPOSE:
      TransposeSelection();
      break;

    case MEN_SETTINGS:
      SettingsDialog();
      break;

    default:
      SetMarker(id, mpToolBar);
      break;

  }
}


HBAnalyzer * HBCanvas::GetAnalyzer()
{
  if (mSequenceCount > 0 && gpTrackWindow->mpSnapSel->Selected)
  {
    HBAnalyzer *analyzer = new HBAnalyzer(seq, mSequenceCount);
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

    HBCanvas*   mpHbWindow;
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

  mpHbWindow = c;
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

  const char* notename[12] =
  {
    "1",
    0,
    "9",
    0,
    "3",
    "11",
    0,
    "5",
    0,
    "13",
    0,
    "7"
  };

  for (i = 0; i < 12; i++)
  {
    int x = w * i + 10;
    chord_chk[i] = new wxCheckBox(this, wxID_ANY,  " ", wxPoint(x, y+1*h));//(wxFunction)ChordCheck,
    scale_chk[i] = new wxCheckBox(this, wxID_ANY,  " ", wxPoint(x, y+2*h));//(wxFunction)ScaleCheck,
    if (notename[i])
    {
      (void) new wxStaticText(this, wxID_ANY, (char *)notename[i], wxPoint(x, y+3*h));
    }
    (void) new wxStaticText(this, wxID_ANY, (char *)HBChord::ScaleName(i + chord_key), wxPoint(x, y+0*h));
  }
  y += 4*h;

  // list boxes                                                               x  y    w    h
#ifdef OBSOLETE
  SetLabelPosition(wxVERTICAL);
#endif

  wxString* cnames = new wxString[n_chord_names];

  for (i = 0; i < n_chord_names; i++)
  {
    cnames[i] = (char *)chord_names[i].name;
  }

  chord_lst = new wxListBox(this, -1,   wxPoint(10, y), wxSize(100, 200), n_chord_names, cnames, wxLB_SINGLE| wxLB_NEEDED_SB);//"Chords"

  delete [] cnames;

  wxString* snames = new wxString[n_scale_names];
  for (i = 0; i < n_scale_names; i++)
  {
    snames[i] = (char *)scale_names[i].name;
  }
  scale_lst = new wxListBox(
    this,
    wxID_ANY,
    wxPoint(200, y),
    wxSize(300, 200),
    n_scale_names,
    snames,
    wxLB_SINGLE | wxLB_NEEDED_SB);//"Scales",

  delete [] snames;

  // thats it
  Fit();
  Show(true);
  ShowValues();
}

HBContextDlg::~HBContextDlg()
{
  if (player.IsPlaying())
  {
    player.StopPlay();
  }
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
    {
      chord += ChordKey(i);
    }
    if (scale_chk[i]->GetValue())
    {
      scale += ScaleKey(i);
    }
  }
  *pcontext->PChord() = chord;
  *pcontext->PScale() = scale;
  mpHbWindow->Refresh();
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
    {
      chord += ChordKey(i);
    }
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
    {
      scale += ScaleKey(i);
    }
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

//*****************************************************************************
//*****************************************************************************
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(HBFrame, wxFrame)

//  EVT_SIZE(HBFrame::OnSize)

  EVT_MENU(MEN_LOAD, HBFrame::OnFileLoad)

  EVT_MENU(MEN_MIDI, HBFrame::OnSettingsMidi)

  EVT_MENU(MEN_HAUNSCH, HBFrame::OnSettingsHaunschild)

  EVT_MENU(MEN_CLEARSEQ, HBFrame::OnActionClearSequence)

  EVT_MENU(MEN_MOUSE, HBFrame::OnMouseHelp)

  EVT_MENU(MEN_HELP, HBFrame::OnHelp)

END_EVENT_TABLE()

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
HBFrame::HBFrame()
  : wxFrame(
      0,
      wxID_ANY,
      "Harmony Browser",
      wxPoint(
        gpConfig->GetValue(C_HarmonyXpos),
        gpConfig->GetValue(C_HarmonyYpos)),
      wxSize(660, 530))
{
  mpHbWindow = 0;
  genmeldy = 0;

  mpToolBar = new JZToolBar(this, tdefs);

  wxMenu* pFileMenu = new wxMenu;
  pFileMenu->Append(MEN_LOAD, "&Load...");
  pFileMenu->Append(MEN_SAVE, "&Save");
  pFileMenu->Append(MEN_CLOSE, "&Close");

  wxMenu* pSettingsMenu = new wxMenu;
  pSettingsMenu->Append(MEN_EDIT, "&Chord");
  pSettingsMenu->Append(MEN_SETTINGS, "&Global");
  pSettingsMenu->Append(MEN_MIDI, "&Midi");
  pSettingsMenu->Append(MEN_HAUNSCH, "&Haunschild Layout");

  wxMenu* pScaleMenu = new wxMenu;
  pScaleMenu->Append(MEN_MAJSCALE, "&Major");
  pScaleMenu->Append(MEN_HARSCALE, "&Harmonic Minor");
  pScaleMenu->Append(MEN_MELSCALE, "&Melodic Minor");
  pScaleMenu->Append(MEN_IONSCALE, "&Ionic");

  wxMenu* pShowMenu = new wxMenu;
  pShowMenu->Append(MEN_EQ4, "&4 equal notes");
  pShowMenu->Append(MEN_EQ3, "&3 equal notes");
  pShowMenu->Append(MEN_EQ2, "&2 equal notes");
  pShowMenu->Append(MEN_EQ1, "&1 equal note");
  pShowMenu->Append(MEN_EQ0, "&0 equal notes");
  pShowMenu->Append(MEN_EQH, "1/2 note &difference");
  pShowMenu->Append(MEN_251, "2-5-1 &move");
  pShowMenu->Append(MEN_EQB, "&Same bass note");
  pShowMenu->Append(MEN_TRITONE, "&Tritone substitute");
  pShowMenu->Append(MEN_PIANO, "Contains &Pianowin Buffer");

  wxMenu* pActionMenu = new wxMenu;
  pActionMenu->Append(MEN_TRANSPOSE, "&Transpose");
  pActionMenu->Append(MEN_ANALYZE, "&Analyze");
  pActionMenu->AppendSeparator();
  pActionMenu->Append(MEN_CLEARSEQ, "&Clear Sequence");

  wxMenu* pHelpMenu = new wxMenu;
  pHelpMenu->Append(MEN_HELP, "&Harmony Browser");
  pHelpMenu->Append(MEN_MOUSE, "&Mouse");

  wxMenuBar* pMenuBar = new wxMenuBar;

  pMenuBar->Append(pFileMenu, "&File");
  pMenuBar->Append(pSettingsMenu, "&Settings");
  pMenuBar->Append(pScaleMenu, "&Scale");
  pMenuBar->Append(pShowMenu, "&Show");
  pMenuBar->Append(pActionMenu, "&Action");
  pMenuBar->Append(pHelpMenu, "&Help");

  SetMenuBar(pMenuBar);

  int w, h;
  GetClientSize(&w, &h);
  mpHbWindow = new HBCanvas(this, 0, 0, w, h);

  mpToolBar->ToggleTool(MEN_MAJSCALE, true);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
HBFrame::~HBFrame()
{
  int XPixel, YPixel;
  GetPosition(&XPixel, &YPixel);
  gpConfig->Put(C_HarmonyXpos, XPixel);
  gpConfig->Put(C_HarmonyYpos, YPixel);
  delete mpToolBar;
  delete mpHbWindow;
  gpHarmonyBrowser = 0;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool HBFrame::OnClose()
{
  return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int HBFrame::SeqDefined()
{
  return mpHbWindow->SeqDefined();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int HBFrame::SeqSelected()
{
  if (mpHbWindow->mSequenceCount == 0 || mpHbWindow->mouse_context.SeqNr() == 0)
  {
    wxMessageBox("Select a chord from sequence first", "Error", wxOK);
    return 0;
  }
  return 1;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int HBFrame::GetChordKeys(int *out, int step, int n_steps)
{
  return mpHbWindow->GetChordKeys(out, step, n_steps);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int HBFrame::GetSelectedChord(int *out)
{
  return mpHbWindow->GetSelectedChord(out);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int HBFrame::GetSelectedScale(int *out)
{
  return mpHbWindow->GetSelectedScale(out);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int HBFrame::GetBassKeys(int *out, int step, int n_steps)
{
  return mpHbWindow->GetBassKeys(out, step, n_steps);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void HBFrame::OnMenuCommand(int id)
{
  switch(id)
  {

    case MEN_EDIT:
      {
        if (!SeqSelected())
        {
          return;
        }
      }
      (void) new HBContextDlg(
        mpHbWindow,
        this,
        mpHbWindow->seq[mpHbWindow->mouse_context.SeqNr() - 1]);
      break;

    case MEN_CLOSE:
//      DELETE_THIS();
      Destroy();
      break;

    default:
      mpHbWindow->OnMenuCommand(id, mpToolBar->GetDelegateToolBar());
      break;
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void HBFrame::OnSettingsMidi(wxCommandEvent& Event)
{
  mpHbWindow->player.SettingsDialog(this);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void HBFrame::OnSettingsHaunschild(wxCommandEvent& Event)
{
  mpHbWindow->ToggleHaunschildLayout();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void HBFrame::OnFileLoad(wxCommandEvent& Event)
{
  mpHbWindow->FileLoad();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void HBFrame::OnActionClearSequence(wxCommandEvent& Event)
{
  mpHbWindow->ClearSeq();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void HBFrame::OnMouseHelp(wxCommandEvent& Event)
{
  wxMessageBox(
    "left: select chord\n"
    "  +shift: put chord into sequence\n"
    "middle: same as left+shift\n"
    "right: play chord\n", "Mousebuttons", wxOK);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void HBFrame::OnHelp(wxCommandEvent& Event)
{
//  gpHelpInstance->ShowTopic("Harmony browser");
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
HBAnalyzer* HBFrame::GetAnalyzer()
{
  return mpHbWindow->GetAnalyzer();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void HBFrame::TransposeSelection()
{
  mpHbWindow->TransposeSelection();
}

void CreateHarmonyBrowser()
{
  if (!gpHarmonyBrowser)
  {
    gpHarmonyBrowser = new HBFrame();
  }
  ((HBFrame *)gpHarmonyBrowser)->Show(true);
}
