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
#include "eventlst.h"
#include "trackwin.h"
#include "command.h"
#include "toolbar.h"
#include "util.h"
#include "events.h"

/* PAT - Removed the ifdef, due to the update of gcc.  Newer versions are more
   standards compliant.  Note that the old ifdef did some funny things, and I'm
   not 100% sure that just removing it is okay, but it did allow it to compile
   okay. */
/*
#ifdef wx_msw
#include <iomanip>
#include <sstream>
#define ENDS ""
using namespace std;
#else
#include <iomanip.h>
#include <strstream.h>
#define stringstream strstream
#include <string>
#include <stdlib.h>
#define ENDS ends
#endif
*/

#include <iomanip>
#include <sstream>
#define ENDS ""
using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////////
  MyTextWindow::MyTextWindow(wxFrame *parent, int x, int y, int w, int h)
    : wxFrame(parent, -1, "test",wxPoint(x, y), wxSize(w, h))
  {

    cout<<"MyTextWindow::constructor"<<endl;
  }
  void OnChar(wxKeyEvent& event);

void MyTextWindow::Create()
{
  wxSize size=GetClientSize();
  textCtrl=new wxTextCtrl(this, -1,"test", wxPoint(0,0),size,wxTE_MULTILINE);
  cout<<"MyTextWindow::Create "<<endl;

}

void MyTextWindow::OnChar(wxKeyEvent& event)
{
#ifndef __PORTING 
  int code = event.KeyCode();
  if (code < 0)
    ;
  else
    wxTextWindow::OnChar(event);
#endif // __PORTING
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////
class tCmdText : public tCommand
{
  stringstream out;
  tTrack *track;
public:
  tCmdText(tFilter *f) : tCommand(f)
  {
  }
  virtual void ExecuteEvent(tTrack *t, tEvent *e);
  string GetText() {
    out << ENDS;
    return out.str();
  }

  string chn(tEvent *e) {
    stringstream tmp;
    tmp << (int)((tChannelEvent *)e)->Channel + 1 << "\t" << ENDS;
    return tmp.str();
  }

  string key(int k) {
    stringstream tmp;
    char buf[100];
    Key2Str(k, buf);
    tmp << buf << "\t" << ENDS;
    return tmp.str();
  }

  string val(int v) {
    stringstream tmp;
    tmp << v << "\t" << ENDS;
    return tmp.str();
  }


  string sysex(tEvent *e) {
    tMetaEvent *m = e->IsMetaEvent();
    stringstream tmp;
    tmp << "f0 ";  // initial 0xf0
    for (int i = 0; i < m->Length; i++)
      tmp << hex << setw(2) << setfill('0') << (int)(m->Data[i] & 0xff) << " ";
    tmp << ENDS;
    return tmp.str();
  }

  string metat(tEvent *e) {
    tMetaEvent *m = e->IsMetaEvent();
    stringstream tmp;
    for (int i = 0; i < m->Length; i++)
      tmp << (char)m->Data[i];
    tmp << ENDS;
    return tmp.str();
  }

  string clk(tEvent *e) {
    char buf[100];
    Song->Clock2String(e->Clock, buf);
    strcat(buf, "\t");
    return buf;
  }

};



void tCmdText::ExecuteEvent(tTrack *t, tEvent *e) {

  switch (e->Stat) {
    case  StatKeyOn: {
      tKeyOn *k = e->IsKeyOn();
      if (k != 0)
        out << clk(e) << "key\t" << chn(e) << key(k->Key) << val(k->Veloc) << val(k->Length) << endl;
    }
    break;

    case  StatKeyPressure: {
      tKeyPressure *k = e->IsKeyPressure();
      if (k != 0)
        out << clk(e) << "kpr\t" << chn(k) << key(k->Key) << val(k->Value) << endl;
    }
    break;

    case  StatControl:{
      tControl *k = e->IsControl();
      if (k != 0)
        out << clk(e) << "ctl\t" << chn(k) << val(k->Control) << val(k->Value) << endl;
    }
    break;

    case  StatProgram:{
      tProgram *k = e->IsProgram();
      if (k != 0)
        out << clk(e) << "prg\t" << chn(k) << val(k->Program) << endl;
    }
    break;

    case  StatChnPressure:{
      tChnPressure *k = e->IsChnPressure();
      if (k != 0)
        out << clk(e) << "cpr\t" << chn(k) << val(k->Value) << endl;
    }
    break;

    case  StatPitch:{
      tPitch *k = e->IsPitch();
      if (k != 0)
        out << clk(e) << "pit\t" << chn(k) << val(k->Value) << endl;
    }
    break;

    case  StatSysEx:{
      out << clk(e) << "sys\t" << sysex(e) << endl;
    }
    break;

    case  StatText:{
      out << clk(e) << "txt\t" << metat(e) << endl;
    }
    break;

    case  StatCopyright:{
      out << clk(e) << "cpy\t" << metat(e) << endl;
    }
    break;

    case  StatTrackName:{
      out << clk(e) << "trn\t" << metat(e) << endl;
    }
    break;

    case  StatMarker:{
      out << clk(e) << "mrk\t" << metat(e) << endl;
    }
    break;

    case  StatSetTempo:{
      out << clk(e) << "spd\t" << e->IsSetTempo()->GetBPM() << endl;
    }
    break;

    case  StatTimeSignat:{
      tTimeSignat *k = e->IsTimeSignat();
      if (k != 0)
        out << clk(e) << "tsi\t" << (int)k->Numerator << "/" << (int)(1<<k->Denomiator) << endl;
    }
    break;

    case  StatKeySignat:{
      tKeySignat *k = e->IsKeySignat();
      if (k != 0)
        out << clk(e) << "ksi\t" << (k->Minor ? "min\t" : "maj\t") << val(k->Sharps) << endl;
    }
    break;

    default:
    break;

  }

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////


class string_tokenizer {
  int beg, end;
  string str;
  static const char *delim;

  void gonext() {
    if (beg != string::npos) {
      end = str.find_first_of(delim, beg);
      if (end == string::npos)
        end = str.length();
    }
  }

public:
  string_tokenizer() {
    beg = string::npos;
  }
  string_tokenizer(const string &s)  {
    set(s);
  }
  void set(const string &s) {
    str = s;
    beg = str.find_first_not_of(delim);
    gonext();
  }
  operator bool() {
    return beg != string::npos;
  }
  const string get() {
    return str.substr(beg, end-beg);
  }
  string_tokenizer& next() {
    beg = str.find_first_not_of(delim, end);
    gonext();
    return *this;
  }
  string getnext() {
    string s = get();
    next();
    return s;
  }
  int pos() const {
    return beg;
  }
  string remainder() {
    return str.substr(beg, str.length() - beg);
  }
};

const char *string_tokenizer::delim = " \t,;/\r\n";



class tString2Event {
public:
  tString2Event(tFilter *f) {
    filter = f;
    trk = filter->Song->GetTrack(filter->FromTrack);
    error = false;
  }
  void begin();
  void add(const string &str);
  void end();
  operator bool() {
    return !error;
  }
private:
  long clk();
  int  val();
  int  chn();
  int  key();
  bool comment();
  int  sysex(char *buf);
  string metat();

  string tok;
  string_tokenizer st;
  tFilter *filter;
  tTrack *trk;
  bool error;
};


int tString2Event::sysex(char *buf) {
  char *p;
  int n = 0;
  st.next(); // skip 0xf0
  while (st) {
    string s = st.getnext();
    buf[n++] = strtoul(s.c_str(), &p, 16);
  }
  return n;
}

string tString2Event::metat() {
  string s = st.remainder();
  int end = s.length();
  while (end > 0 && (s[end] == 0 || isspace(s[end])))
    end--;
  return s.substr(0, end+1);
}


void tString2Event::begin() {
  tCmdErase eraser(filter, 1);
  eraser.Execute();
}

int tString2Event::val() {
  if (st) {
    string s = st.getnext();
    return atol(s.c_str());
  }
  error = 1;
  return 0;
}

int tString2Event::chn() {
  if (st) {
    string s = st.getnext();
    return atol(s.c_str()) - 1;
  }
  error = 1;
  return 0;
}

long tString2Event::clk() {
  if (st) {
    string s = st.getnext();
    return filter->Song->String2Clock(s.c_str());
  }
  error = 1;
  return 0;
}

int tString2Event::key() {
  if (st) {
    string s = st.getnext();
    return Str2Key(s.c_str());
  }
  error = 1;
  return 0;
}

bool tString2Event::comment() {
  if (!st)
    return true;
  string s = st.get();
  if (s.at(0) == '#')
    return true;
  return false;
}

void tString2Event::add(const string &str)
{
  if (error)
    return;

  char buf[16384];
  st.set(str);

  if (comment())
    return;

  long cl = clk();

  string s = "err";
  if (st)
    s = st.getnext();

  if (s.compare("key") == 0) {
    int c = chn();
    int k = key();
    int v = val();
    int l = val();
    trk->Put(new tKeyOn(cl, c, k, v, l));
  }

  else if (s.compare("kpr") == 0) {
    int c = chn();
    int k = key();
    int v = val();
    trk->Put(new tKeyPressure(cl, c, k, v));
  }

  else if (s.compare("ctl") == 0) {
    int c = chn();
    int n = val();
    int v = val();
    trk->Put(new tControl(cl, c, n, v));
  }

  else if (s.compare("prg") == 0) {
    int c = chn();
    int v = val();
    trk->Put(new tProgram(cl, c, v));
  }

  else if (s.compare("cpr") == 0) {
    int c = chn();
    int v = val();
    trk->Put(new tChnPressure(cl, c, v));
  }

  else if (s.compare("pit") == 0) {
    int c = chn();
    int v = val();
    trk->Put(new tPitch(cl, c, v));
  }

  else if (s.compare("sys") == 0) {
    int len = sysex(buf);
    if (len < 2)
      error = 1;
    else
      trk->Put(new tSysEx(cl, (uchar *)buf, len));
  }

  else if (s.compare("txt") == 0) {
    string s = metat();
    trk->Put(new tText(cl, (uchar *)s.data(), s.length()));
  }
  else if (s.compare("cpy") == 0) {
    string s = metat();
    trk->Put(new tCopyright(cl, (uchar *)s.data(), s.length()));
  }
  else if (s.compare("trn") == 0) {
    string s = metat();
    trk->Put(new tTrackName(cl, (uchar *)s.data(), s.length()));
  }
  else if (s.compare("mrk") == 0) {
    string s = metat();
    trk->Put(new tMarker(cl, (uchar *)s.data(), s.length()));
  }

  else if (s.compare("spd") == 0) {
    int bpm = val();
    trk->Put(new tSetTempo(cl, bpm));
  }

  else if (s.compare("tsi") == 0) {
    int num = val();
    int den = val();
    for (int i = 0; i < 16; i++) {
      if ((1 << i) == den) {
        den = i;
        break;
      }
    }

    trk->Put(new tTimeSignat(cl, num, den));
  }

  else if (s.compare("ksi") == 0) {
    int minor = 0;
    if (st) {
      if (st.get().compare("min") == 0)
        minor = 1;
      st.next();
      int sharps = val();
      trk->Put(new tKeySignat(cl, minor, sharps));
    }
    else
      error = 1;
  }

  else
    error = 1;

  if (error)
    wxMessageBox((char *)str.c_str(), "Error", wxOK);
}


void tString2Event::end() {
  trk->Cleanup();
  if (error)
    filter->Song->Undo();
}



////////////////////////////////////////////////////////////////
//tEventList implementation

#define MEN_ACCEPT 1
#define MEN_CANCEL 2
#define MEN_HELP 3

#ifdef wx_x

#include "../bitmaps/accept.xpm"
#include "../bitmaps/cancel.xpm"
#include "../bitmaps/help.xpm"

static tToolDef tdefs[] = {
  { MEN_ACCEPT,	FALSE,  0, accept_xpm },
  { MEN_CANCEL,	FALSE,  1, cancel_xpm },
  { MEN_HELP,  	FALSE,  0, help_xpm }
};

#else

static tToolDef tdefs[] = {
  { MEN_ACCEPT,	FALSE,  0, "accept_xpm", "accept changes" },
  { MEN_CANCEL,	FALSE,  1, "cancel_xpm", "discard changes" },
  { MEN_HELP,  	FALSE,  0, "help_xpm", "help" }
};

#endif

int tEventList::geo[4] = { 50, 80, 400, 400 };


tEventList::tEventList(tTrackWin *w, wxFrame **ref)
: tSliderWin(w, ref, "Event List", geo, tdefs, 3),
  tw(w),
  filter(tw->Song)
{
  default_filename = copystring("noname.txt");

  wxMenuBar *menu_bar = new wxMenuBar;
  wxMenu    *menu;

  menu = new wxMenu();
  menu->Append(MEN_ACCEPT, "&Accept changes");
  menu->Append(MEN_CANCEL, "&Discard changes");
  menu_bar->Append(menu, "&File");

  menu = new wxMenu();
  menu->Append(MEN_HELP, "&Event List");
  menu_bar->Append(menu, "&Help");

  SetMenuBar(menu_bar);

  //  sliders[0] = textwin = new MyTextWindow(this, -1, -1, -1, -1);
  sliders[0] = textwin = new wxTextCtrl(this, -1,"test", wxPoint(0,0),wxSize(1000,1000),wxTE_MULTILINE);
  n_sliders=1;
  //  Initialize();

  //////////////////////////////////////
  //from initialize

  AddItems();
  panel->Fit();
      AddEdits();
  in_constructor = FALSE;
  //////////////////////////////
}


tEventList::~tEventList()
{
}

void tEventList::OnMenuCommand(int id)
{
  switch (id) {
    case MEN_CANCEL:
      DELETE_THIS();
      break;

    case MEN_ACCEPT:
      if (Accept())
        DELETE_THIS();
      break;

    case MEN_HELP:
      HelpInstance->ShowTopic("Event List Editor");
      break;

  }
}


bool tEventList::Accept()
{

  int N = textwin->GetNumberOfLines();

  wxString buf;
  tString2Event se(&filter);
  se.begin();
  for (int i = 0; i < N; i++) {

    buf= textwin->GetLineText(i);

    se.add((string)buf);
  }
  se.end();
  delete[] buf;

  return se;
}

void tEventList::AddItems()
{
}

void tEventList::OnChar(wxKeyEvent& event) {}

void tEventList::AddEdits()
{
  n_sliders = 1;
  sliders_per_row = 1;
}


void tEventList::Scan(tFilter *f)
{
  filter = *f;
  tCmdText ct(&filter);
  ct.Execute(0);

  textwin->Clear();
  textwin->WriteText((char *)ct.GetText().c_str());

}

