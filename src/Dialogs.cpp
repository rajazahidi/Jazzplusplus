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

#include "Dialogs.h"
#include "Song.h"
#include "Synth.h"
#include "Command.h"
#include "EventWindow.h"
#include "ProjectManager.h"
#include "Track.h"
#include "Events.h"
#include "Player.h"
#include "PianoFrame.h"
#include "PianoWindow.h"
#include "ClockDialog.h"
#include "KeyDialog.h"
#include "PropertyListDialog.h"
#include "Globals.h"
#include "NamedChoice.h"
#include "Help.h"
#include "DeprecatedWx/proplist.h"

#include "Dialogs/KeyOnDialog.h"

#include <sstream>
#include <iomanip>

using namespace std;

// **************************************************************************
// Shift
// *************************************************************************
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
tShiftDlg::tShiftDlg(JZEventFrame* pEventWindow, JZFilter* pFilter, long unit)
  : tPropertyListDlg("Shift events left/right"),
    mSteps(0),
    mUnit(unit),
    mpFilter(pFilter),
    mpSong(pFilter->mpSong)
{
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool tShiftDlg::OnClose()
{
  cout << "tShiftDlg::OnClose " << mSteps << endl;
  tCmdShift cmd(mpFilter, mSteps * mUnit);
  cmd.Execute();

  JZProjectManager::Instance()->UpdateAllViews();

  //  wxForm::OnOk();
  return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void tShiftDlg::OnHelp()
{
  gpHelpInstance->ShowTopic("Shift");
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void tShiftDlg::AddProperties()
{
//send wxPropertyValue REFERENCE not POINTER
  sheet->AddProperty(new wxProperty("Snaps",  wxPropertyValue(&mSteps), "integer", new wxIntegerListValidator(-16, 16)));
  sheet->AddProperty(new wxProperty("clocks per snap",  (long)mUnit, "integer"));//informational only
}




// **************************************************************************
// Cleanup
// *************************************************************************

int tCleanupDlg::lowLimit = 48;
bool tCleanupDlg::shortenOverlaps = 1;



tCleanupDlg::tCleanupDlg(JZEventFrame *w, JZFilter *f)
  : tPropertyListDlg( "Clean up events" )
{
  Filter = f;
  Song = f->mpSong;
}



bool tCleanupDlg::OnClose()
{
  int limit = Song->GetTicksPerQuarter() * 4 / lowLimit;
  cout
    << "tCleanupDlg::OnClose " << lowLimit << ' ' << shortenOverlaps
    << endl;
  tCmdCleanup cln(Filter, limit, shortenOverlaps);
  cln.Execute();

  JZProjectManager::Instance()->UpdateAllViews();

  //wxForm::OnOk();
  return false;
}

void tCleanupDlg::OnHelp()
{
  gpHelpInstance->ShowTopic("Cleanup");
}


//IMPLEMENT_DYNAMIC_CLASS(tNamedValueListValue, wxPropertyValue)

void tCleanupDlg::AddProperties()
{
  // JAVE this doesnt work
//  sheet->AddProperty(new wxProperty(
//    "Delete notes shorther than",
//    wxPropertyValue("1/8"),
//    "props",
//    Steps.GetStringListValidator()));

  // how it ought to work
  // gLimitSteps is a name/value pair vector
//  sheet->AddProperty(new wxProperty(
//    "Delete notes shorther than",
//    wxPropertyValue(&lowLimit),
//    "props",
//    new tNamedValueListValidator(gLimitSteps)));

  // How it really ought to work (except the validator might ask the "value"
  // for the list of allowed values).
  // http://sourceforge.net/tracker/?group_id=9863&atid=109863 wx bugracker
  tNamedValueListValue val1 = tNamedValueListValue(
    &lowLimit,
    gLimitSteps);

  wxPropertyValue& val = val1;
  cout << "little test:" << val.GetStringRepresentation() << endl;
  sheet->AddProperty(new wxProperty(
    "Delete shorther than",
    (tNamedValueListValue&)val1,
    "props",
    new tNamedValueListValidator(gLimitSteps)));

  sheet->AddProperty(new wxProperty(
    "Shorten overlapping",
    wxPropertyValue((bool*)&shortenOverlaps), "bool"));

  // There seems to be a padding limit in
  // wxPropertyListView::MakeNameValueString set to 25.
}


// **************************************************************************
// SearchReplace
// *************************************************************************

int tSearchReplaceDlg::frCtrl = 1;
int tSearchReplaceDlg::toCtrl = 1;

tSearchReplaceDlg::tSearchReplaceDlg(JZEventFrame *w, JZFilter *f)
   : tPropertyListDlg("Search and replace controller types" )
{
  Filter = f;
  Song = f->mpSong;
}

bool tSearchReplaceDlg::OnClose()
{
  tCmdSearchReplace sr(Filter, frCtrl - 1, toCtrl-1);
  sr.Execute();

  JZProjectManager::Instance()->UpdateAllViews();

  return false;
}

void tSearchReplaceDlg::OnHelp()
{
  gpHelpInstance->ShowTopic("Search Replace");
}

void tSearchReplaceDlg::AddProperties()
{
  sheet->AddProperty(new wxProperty(
    "Search",
    tNamedValueListValue(&frCtrl, gpConfig->GetControlNames()),
    "props",
    new tNamedValueListValidator(gpConfig->GetControlNames())));

  sheet->AddProperty(new wxProperty(
    "Replace",
    tNamedValueListValue(&toCtrl, gpConfig->GetControlNames()),
    "props",
    new tNamedValueListValidator(gpConfig->GetControlNames())));
}




// **************************************************************************
// Transpose
// *************************************************************************

int tTransposeDlg::Notes = 0;
int tTransposeDlg::Scale = gScaleChromatic;
bool tTransposeDlg::FitIntoScale = 0;

tTransposeDlg::tTransposeDlg(JZEventFrame *w, JZFilter *f)
  : tPropertyListDlg("Transpose")
{
  Filter = f;
  Song   = f->mpSong;
}


bool tTransposeDlg::OnClose()
{
  tCmdTranspose trn(Filter, Notes, Scale, FitIntoScale);
  trn.Execute();

  JZProjectManager::Instance()->UpdateAllViews();

  return false;
}

void tTransposeDlg::OnHelp()
{
  gpHelpInstance->ShowTopic("Transpose");
}


void tTransposeDlg::AddProperties()
{
  int s = tScale::Analyze(Filter);

  sheet->AddProperty(new wxProperty(
    "selection looks like",
    wxPropertyValue(gScaleNames[s + 2].first),
    "string"));
  sheet->AddProperty(new wxProperty(
    "Amount",
    wxPropertyValue(&Notes),
    "integer",
    new wxIntegerListValidator(-12, 12)));
  sheet->AddProperty(new wxProperty(
    "Fit into Scale",
    wxPropertyValue((bool*)&FitIntoScale),
    "bool"));
}

// **************************************************************************
// SetChannel
// *************************************************************************

int tSetChannelDlg::NewChannel = 1;

tSetChannelDlg::tSetChannelDlg(JZFilter *f)
: tPropertyListDlg("Set MIDI Channel")
{
  Filter = f;
  Song = f->mpSong;
}



bool tSetChannelDlg::OnClose()
{
  if (NewChannel)
  {
    tCmdSetChannel exe(Filter, NewChannel - 1);
    exe.Execute();
  }
  //tPropertyListDlg::OnClose();
  return false;
}

void tSetChannelDlg::OnHelp()
{
  gpHelpInstance->ShowTopic("Set MIDI Channel");
}


void tSetChannelDlg::AddProperties()
{
 //  Add(wxMakeFormShort("new Channel", &NewChannel, wxFORM_DEFAULT,
//                        new wxList(wxMakeConstraintRange(1.0, 16.0), 0)));
//   Add(wxMakeFormNewLine());
  // AssociatePanel(panel);

  sheet->AddProperty(new wxProperty(
    "new Channel",
    wxPropertyValue(&NewChannel),
    "integer",
    new wxIntegerListValidator(1, 16)));
}



// **************************************************************************
// Velocity
// *************************************************************************

int tVelocityDlg::FromValue = 64;
int tVelocityDlg::ToValue = 0;
int tVelocityDlg::Mode = 0;


tVelocityDlg::tVelocityDlg(JZFilter *f)
: tPropertyListDlg( "Velocity" )
{
  Filter = f;
  Song = f->mpSong;
}


bool tVelocityDlg::OnClose()
{
  tCmdVelocity cmd(Filter, FromValue, ToValue, Mode);
  cmd.Execute();
  return false;
}

void tVelocityDlg::OnHelp()
{
  gpHelpInstance->ShowTopic("Velocity");
}

void tVelocityDlg::AddProperties()
{
  sheet->AddProperty(new wxProperty(
    "Start",
    wxPropertyValue(&FromValue),
    "integer",
    new wxIntegerListValidator(0, 127)));
  sheet->AddProperty(new wxProperty(
    "Stop",
    wxPropertyValue(&ToValue),
    "integer",
    new wxIntegerListValidator(0, 127)));
  sheet->AddProperty(new wxProperty(
    "Mode",
    tNamedValueListValue(&Mode, gModes),
    "props",
    new tNamedValueListValidator(gModes)));
}




// **************************************************************************
// Length
// *************************************************************************

int tLengthDlg::FromValue = 30;
int tLengthDlg::ToValue = 0;

int tLengthDlg::Mode;

tLengthDlg::tLengthDlg(JZEventFrame *w, JZFilter *f)
: tPropertyListDlg("Length")
{
  Filter = f;
  Song = f->mpSong;
}


bool tLengthDlg::OnClose()
{
  tCmdLength cmd(Filter, FromValue, ToValue, Mode);
  cmd.Execute();

  JZProjectManager::Instance()->UpdateAllViews();

  //tPropertyListDlg::OnClose();
  return false;
}

void tLengthDlg::OnHelp()
{
  gpHelpInstance->ShowTopic("Length");
}


void tLengthDlg::AddProperties()
{
  sheet->AddProperty(new wxProperty(
    "Ticks/Quarter",
    wxPropertyValue((long)Song->GetTicksPerQuarter()),
    "integer",
    new wxIntegerListValidator(-16, 16)));  //r/o

  sheet->AddProperty(new wxProperty(
    "Start",
    wxPropertyValue(&FromValue),
    "integer",
    new wxIntegerListValidator(0, Song->GetTicksPerQuarter() * 4)));
  sheet->AddProperty(new wxProperty(
    "Stop",
    wxPropertyValue(&ToValue),
    "integer",
    new wxIntegerListValidator(0, Song->GetTicksPerQuarter() * 4)));
  sheet->AddProperty(new wxProperty(
    "Mode",
    tNamedValueListValue(&Mode, gModes),
    "props",
    new tNamedValueListValidator(gModes)));
}



// **************************************************************************
// seqLength
// *************************************************************************

double tSeqLengthDlg::scale = 1.0;



tSeqLengthDlg::tSeqLengthDlg(JZEventFrame *w, JZFilter *f)
: tPropertyListDlg("stretch/contract by scale from start of selected sequence" )
{
  Filter = f;
  Song = f->mpSong;
}


bool tSeqLengthDlg::OnClose()
{
  tCmdSeqLength cmd(Filter, scale);
  cmd.Execute();

  JZProjectManager::Instance()->UpdateAllViews();

  //tPropertyListDlg::OnClose();
  return false;
}

void tSeqLengthDlg::OnHelp()
{
  gpHelpInstance->ShowTopic("SeqLength");
}


void tSeqLengthDlg::AddProperties()
{
  sheet->AddProperty(new wxProperty(
    "Scale",
    wxPropertyValue((double*)&scale),
    "double",
    new wxRealListValidator()));
}


// **************************************************************************
// midiDelay
// *************************************************************************

double tMidiDelayDlg::scale = 0.5;
long tMidiDelayDlg::clockDelay = 10;
int tMidiDelayDlg::repeat = 6;

tMidiDelayDlg::tMidiDelayDlg(JZEventFrame *w, JZFilter *f)
: tPropertyListDlg("MIDI delay line" )
{
  Filter = f;
  Song = f->mpSong;
}


bool tMidiDelayDlg::OnClose()
{

  tCmdMidiDelay cmd(Filter, scale,clockDelay,repeat);
  cmd.Execute();

  JZProjectManager::Instance()->UpdateAllViews();

  //tPropertyListDlg::OnClose();
  return false;
}

void tMidiDelayDlg::OnHelp()
{
  gpHelpInstance->ShowTopic("MidiDelay");
}


void tMidiDelayDlg::AddProperties()
{

  // Old system:
  //   Add(wxMakeFormMessage("MIDI delay line"));
  //   Add(wxMakeFormNewLine());
  //   Add(wxMakeFormDouble("Scale", &scale, wxFORM_DEFAULT,
  //                        new wxList(wxMakeConstraintRange(0.0, 4.0), 0)));
  //   Add(wxMakeFormNewLine());
  //   Add(wxMakeFormLong("Delay in clocks ", &clockDelay, wxFORM_DEFAULT,
  //                        new wxList(wxMakeConstraintRange(-100.0, 1000.0), 0)));
  //   Add(wxMakeFormNewLine());
  //   Add(wxMakeFormShort("Repeats ", &repeat, wxFORM_DEFAULT,
  //                        new wxList(wxMakeConstraintRange(0.0, 100.0), 0)));
  //   Add(wxMakeFormNewLine());

  //   AssociatePanel(panel);

  //System 2:
  //   sheet->AddProperty(new wxProperty("Scale",  wxPropertyValue((double*)&scale), "real", new wxRealListValidator(0.0001, 4.0)));
  //   sheet->AddProperty(new wxProperty("Delay in clocks",  wxPropertyValue((long*)&clockDelay), "integer", new wxIntegerListValidator(-1000, 1000)));
  //   sheet->AddProperty(new wxProperty("Repeats",  wxPropertyValue((long*)&repeat), "integer", new wxIntegerListValidator(0, 100)));

  //System 3:
}



// *************************************************************************
// Delete
// *************************************************************************

bool tDeleteDlg::LeaveSpace = 1;

tDeleteDlg::tDeleteDlg(JZEventFrame *w, JZFilter *f)
: tPropertyListDlg("Delete" )
{
  Filter = f;
}


bool tDeleteDlg::OnClose()
{
  tCmdErase cmd(Filter, LeaveSpace);
  cmd.Execute();

  JZProjectManager::Instance()->UpdateAllViews();

//  tPropertyListDlg::OnClose();
  return false;
}

void tDeleteDlg::OnHelp()
{
  gpHelpInstance->ShowTopic("Delete");
}

void tDeleteDlg::AddProperties()
{
//   Add(wxMakeFormBool("Leave Space", &LeaveSpace));
//  AssociatePanel(panel);
  sheet->AddProperty(new wxProperty("Leave Space",  wxPropertyValue((bool*)&LeaveSpace), "bool"));
}

//LAST ADDED EVENT


// **************************************************************************
// Snap
// *************************************************************************

tSnapDlg::tSnapDlg(JZPianoWindow* pPianoWindow, int* snapptr)
  : tPropertyListDlg("Snap:quantize cut/paste events"),
    mpPianoWindow(pPianoWindow)
{
//, Steps("Snap value", limitSteps, snapptr)
  //limitSteps lives in util.cpp
   ptr = snapptr;
}



bool tSnapDlg::OnClose()
{
  //Steps.GetValue();
  // toggle the tool buttons
  mpPianoWindow->SetSnapDenom(*ptr);
  //tPropertyListDlg::OnClose();
  return false;
}

void tSnapDlg::OnHelp()
{
  gpHelpInstance->ShowTopic("Snap");
}

void tSnapDlg::AddProperties()
{
//   panel->SetLabelPosition(wxVERTICAL);
//   Add(wxMakeFormMessage("quantize cut/paste events"));
//   Add(wxMakeFormNewLine());
//   Add(Steps.mkFormItem(100));
//   Add(wxMakeFormNewLine());
//   AssociatePanel(panel);

   sheet->AddProperty(new wxProperty(
     "Steps",
     tNamedValueListValue(ptr, gLimitSteps),
     "props",
     new tNamedValueListValidator(gLimitSteps)));
}


// **************************************************************************
// Quantize
// *************************************************************************

bool tQuantizeDlg::NoteStart = 1;
bool tQuantizeDlg::NoteLength = 0;
int tQuantizeDlg::QntStep = 16;
int tQuantizeDlg::Delay = 0;
int tQuantizeDlg::Groove = 0;

tQuantizeDlg::tQuantizeDlg(JZEventFrame *w, JZFilter *f)
   : tPropertyListDlg("Quantize" )
  //, Steps("steps", gQntSteps, &gQntStep)
{
  Filter = f;
  Song = f->mpSong;
}



bool tQuantizeDlg::OnClose()
{
  //Steps.GetValue();
  int step = Song->GetTicksPerQuarter() * 4 / QntStep;
  tCmdQuantize qnt(Filter, step, Groove * step / 100, Delay * step / 100);
  qnt.NoteStart = NoteStart;
  qnt.NoteLength = NoteLength;
  qnt.Execute();

  JZProjectManager::Instance()->UpdateAllViews();

  //tPropertyListDlg::OnClose();
  return false;
}

void tQuantizeDlg::OnHelp()
{
//  if (mpEventWindow->NextWin)
//  {
//    gpHelpInstance->ShowTopic("Quantize");
//  }
//  else
//  {
//    gpHelpInstance->ShowTopic("Pianowin Quantize");
//  }
}

void tQuantizeDlg::AddProperties()
{
  sheet->AddProperty(new wxProperty(
    "Note start",
    wxPropertyValue((bool*)&NoteStart),
    "bool"));
  sheet->AddProperty(new wxProperty(
    "Note length",
    wxPropertyValue((bool*)&NoteLength),
    "bool"));
  sheet->AddProperty(new wxProperty(
    "Groove",
    wxPropertyValue(&Groove),
    "int",
    new wxRealListValidator(-100, 100)));
  sheet->AddProperty(new wxProperty(
    "Delay",
    wxPropertyValue(&Delay),
    "int",
    new wxRealListValidator(-100, 100)));
}


// ***********************************************************************
// Event-Dialogue
// ***********************************************************************

class tEventDlg : public tPropertyListDlg
{
  public:

    JZTrack    *Track;
    JZClockDialog ClockDlg;
    JZPianoWindow* Win;

    JZEvent    *Event;
    JZEvent    *Copy;

    tEventDlg(JZEvent *e, JZPianoWindow* w, JZTrack *t);
    virtual void AddProperties();
    virtual bool OnClose();
    virtual void OnHelp();
    virtual void OnCancel();
};


tEventDlg::tEventDlg(JZEvent *e, JZPianoWindow* w, JZTrack *t)
  : tPropertyListDlg( "Event" ),
    ClockDlg(w->GetSong(), "Time ", e->GetClock())
{
  Win   = w;
  Track = t;
  Event = e;
  Copy  = e->Copy();
}

void tEventDlg::AddProperties()
{
  sheet->AddProperty(ClockDlg.mkProperty());
}

void tEventDlg::OnCancel()
{
  delete Copy;
  //tPropertyListDlg::OnCancel();
}

bool tEventDlg::OnClose()
{
  Copy->SetClock(ClockDlg.GetClock());
  Track->Kill(Event);
  Track->Put(Copy);
  Track->Cleanup();
  Win->Refresh();
  return tPropertyListDlg::OnClose();
}

void tEventDlg::OnHelp()
{
  gpHelpInstance->ShowTopic("Piano Window");
}

// --------------------------- ChannelEvent ----------------------------


class tChEventDlg : public tEventDlg
{
  public:

    int Channel;

    tChEventDlg(tChannelEvent *e, JZPianoWindow* w, JZTrack *t)
      : tEventDlg(e, w, t)
    {
      Channel = e->Channel + 1;                // 1..16
    }
    void AddProperties();
    bool OnClose();
};

void tChEventDlg::AddProperties()
{
  sheet->AddProperty(new wxProperty(
    "Channel",
    wxPropertyValue(&Channel),
    "integer",
    new wxIntegerListValidator(1, 16)));
  tEventDlg::AddProperties();
}


bool tChEventDlg::OnClose()
{
  ((tChannelEvent *)Copy)->Channel = Channel - 1;
  tEventDlg::OnClose();
  return false;
}

// -------------------------------- Pitch -------------------------------

class tPitchDlg : public tChEventDlg
{
 public:

  int Value;

  tPitchDlg(tPitch* e, JZPianoWindow* w, JZTrack *t);

  void AddProperties();
  bool OnClose();
};


tPitchDlg::tPitchDlg(tPitch *e, JZPianoWindow* w, JZTrack *t)
  : tChEventDlg(e, w, t)
{
  Event = e;
  Value = e->Value;
}


bool tPitchDlg::OnClose()
{
  ((tPitch *)Copy)->Value = Value;
  return tChEventDlg::OnClose();
}

void tPitchDlg::AddProperties()
{
  //Add(wxMakeFormShort("Pitch:", &Value, wxFORM_DEFAULT, new wxList(wxMakeConstraintRange(-8191.0, 8191.0), 0)));
  sheet->AddProperty(new wxProperty(
    "Pitch:",
    wxPropertyValue(&Value),
    "integer",
    new wxIntegerListValidator(-8191, 8191)));

  tChEventDlg::AddProperties();
}

// -------------------------------- Controller ---------------------------

class tControlDlg : public tChEventDlg
{
 public:

  int Value;
  int Control;
  //tNamedChoice Choice;

  tControlDlg(tControl *e, JZPianoWindow* w, JZTrack *t);

  void AddProperties();
  bool OnClose();
};


tControlDlg::tControlDlg(tControl *e, JZPianoWindow* w, JZTrack *t)
  : tChEventDlg(e, w, t)//,
//    Choice("Controller", &gpConfig->GetCtrlName(0), &Control)
{
  Event = e;
  Value = e->mValue;
  Control = e->mControl + 1;
}


bool tControlDlg::OnClose()
{
  ((tControl *)Copy)->mValue = Value;
  //  Choice.GetValue();
  ((tControl *)Copy)->mControl = Control - 1;
  return tChEventDlg::OnClose();
}

void tControlDlg::AddProperties()
{
  //  Add(Choice.mkFormItem(300, 300));
//  Choice("Controller", &gpConfig->GetCtrlName(0), &Control)
  sheet->AddProperty(new wxProperty(
    "Controller",
    tNamedValueListValue(&Control, gpConfig->GetControlNames()),
    "props",
    new tNamedValueListValidator(gpConfig->GetControlNames())));

  sheet->AddProperty(new wxProperty(
    "Value",
    wxPropertyValue(&Value),
    "integer",
    new wxIntegerListValidator(0, 127)));

//  Add(wxMakeFormShort(
//    "Value:",
//    &Value,
//    wxFORM_DEFAULT,
//    new wxList(wxMakeConstraintRange(0.0, 127.0), 0)));

  tChEventDlg::AddProperties();
}

// -------------------------------- Play track ---------------------------

class tPlayTrackDlg : public tEventDlg
{
 public:

  int transpose;
  int track;
  int eventlength;

  tNamedChoice Choice;

  tPlayTrackDlg(tPlayTrack *e, JZPianoWindow* w, JZTrack *t);

  void AddProperties();
  bool OnClose();
};


tPlayTrackDlg::tPlayTrackDlg(tPlayTrack *e, JZPianoWindow* w, JZTrack *t)
  : tEventDlg(e, w, t),
    Choice("playtrack", gpConfig->GetControlNames(), &track)
{
  Event = e;
  track = e->track;
  transpose=e->transpose;
  eventlength=e->eventlength;
}


bool tPlayTrackDlg::OnClose()
{
  tPlayTrack* p=(tPlayTrack*)Copy;

  Choice.GetValue();
  p->track = track;
  p->transpose = transpose;
  p->eventlength=eventlength;
  return tEventDlg::OnClose();
}

void tPlayTrackDlg::AddProperties()
{
  sheet->AddProperty(new wxProperty(
    "Track",
    wxPropertyValue(&track),
    "integer",
    new wxIntegerListValidator(0, 127)));
  sheet->AddProperty(new wxProperty(
    "Transpose",
    wxPropertyValue(&transpose),
    "integer",
    new wxIntegerListValidator(-127, 127)));
  sheet->AddProperty(new wxProperty(
    "Length",
    wxPropertyValue(&eventlength),
    "integer",
    new wxIntegerListValidator(0, 127)));
  tEventDlg::AddProperties();
}

// -------------------------------- text event ---------------------------

class tTextDlg : public tEventDlg
{
 public:

  char* text;
  int track;
  tNamedChoice Choice;

  tTextDlg(tText *e, JZPianoWindow* w, JZTrack *t);

  void AddProperties();
  bool OnClose();
};


tTextDlg::tTextDlg(tText *e, JZPianoWindow* w, JZTrack *t)
  : tEventDlg(e, w, t),
    Choice("text", gpConfig->GetControlNames(), &track)
{
  Event = e;
  text=new char[2048];
  strcpy(text,(const char*)(e->GetText()));
}


bool tTextDlg::OnClose()
{
  tText* p;
  p=new tText( ((tText*)Copy)->GetClock(), (unsigned char*)text, strlen(text));
  delete Copy;
  Copy=p;
  fprintf(stderr,"text:%s",text);
  Choice.GetValue();
  delete text;
  return tEventDlg::OnClose();
}

void tTextDlg::AddProperties()
{
  sheet->AddProperty(new wxProperty("Text", wxPropertyValue((char**)&text), "string"));
  //  Add(wxMakeFormString("Text:", (char**)&text));
  tEventDlg::AddProperties();
}



// -------------------------------- EOT ---------------------------

class tEndOfTrackDlg : public tEventDlg
{
 public:
  int track;

  tNamedChoice Choice;

  tEndOfTrackDlg(tEndOfTrack *e, JZPianoWindow* w, JZTrack *t);

  void AddProperties();
  bool OnClose();
};


tEndOfTrackDlg::tEndOfTrackDlg(tEndOfTrack *e, JZPianoWindow* w, JZTrack *t)
  : tEventDlg(e, w, t),
    Choice("End Of Track", gpConfig->GetControlNames(), &track)
{

}


bool tEndOfTrackDlg::OnClose()
{
//  tEndOfTrack* p=(tEndOfTrack*)Copy;
  Choice.GetValue();
  return tEventDlg::OnClose();
}

void tEndOfTrackDlg::AddProperties()
{
  tEventDlg::AddProperties();
}


// -------------------------------- Program ---------------------------

class tProgramDlg : public tEventDlg
{
 public:

  int Program;
  //  tNamedChoice Choice;

  tProgramDlg(tProgram *e, JZPianoWindow* w, JZTrack *t);

  void AddProperties();
  bool OnClose();
};


tProgramDlg::tProgramDlg(tProgram *e, JZPianoWindow* w, JZTrack *t)
  : tEventDlg(e, w, t),
    Program(e->Program + 1)//,
//    Choice("Program", &gpConfig->GetVoiceName(0), &Program)
{
  Event = e;
}


bool tProgramDlg::OnClose()
{
  //Choice.GetValue();
  if (Program <= 0)
    Program = 1;
  ((tProgram *)Copy)->Program = Program - 1;
  return tEventDlg::OnClose();
}

void tProgramDlg::AddProperties()
{
  //Add(Choice.mkFormItem(300, 300));
  sheet->AddProperty(new wxProperty(
    "Program",
    tNamedValueListValue(&Program, gpConfig->GetVoiceNames()),
    "props",
    new tNamedValueListValidator(gpConfig->GetVoiceNames())));
}



// -------------------------------- Set Tempo -------------------------------

class tSetTempoDlg : public tEventDlg
{
 public:

  int Value;

  tSetTempoDlg(tSetTempo *e, JZPianoWindow* w, JZTrack *t);

  void AddProperties();
  bool OnClose();
};


tSetTempoDlg::tSetTempoDlg(tSetTempo *e, JZPianoWindow* w, JZTrack *t)
  : tEventDlg(e, w, t)
{
  Event = e;
  Value = e->GetBPM();
}


bool tSetTempoDlg::OnClose()
{
  ((tSetTempo *)Copy)->SetBPM( Value );
  return tEventDlg::OnClose();
}

void tSetTempoDlg::AddProperties()
{
  //  Add(wxMakeFormShort("Tempo:", &Value, wxFORM_DEFAULT, new wxList(wxMakeConstraintRange(20.0, 240.0), 0)));
  sheet->AddProperty(new wxProperty(
    "Tempo",
    wxPropertyValue(&Value),
    "integer",
    new wxIntegerListValidator(20, 240)));
  tEventDlg::AddProperties();
}

// -------------------------------- Sysex Edit-------------------------------

class tSysexDlg : public tEventDlg
{
  public:

  char *str;

  tSysexDlg(tSysEx *s, JZPianoWindow* w, JZTrack *t);

  void AddProperties();
  bool OnClose();
};


tSysexDlg::tSysexDlg(tSysEx *s, JZPianoWindow* w, JZTrack *t)
  : tEventDlg(s, w, t)
{
  Event = s;
  char hexbyte[10];

  str = new char[256];
  str[0] = 0;

  if (s->Length)
     strcat( str, "f0 " );

  for (int i = 0; i < s->Length; i++)
  {
    sprintf(hexbyte, "%02x ", s->mpData[i]);
    strcat(str, hexbyte);
  }
}


bool tSysexDlg::OnClose()
{
  int i;
  int j;
  int k;
  int len;

  unsigned char d[256];
  memset( d, 0, 256 );

  int jstop = strlen(str);

  unsigned int TempInteger;
  for (i = 0, j = 0; j <= jstop; j += k, ++i)
  {
    sscanf(str + j, "%02x %n", &TempInteger, &k);

    d[i] = static_cast<unsigned char>(TempInteger);
    if (d[i] == 0xf7)
      break;
  }

  int found = 0;
  for (i = 0; i < 256; i++)
  {
    if (d[i] == 0xf7)
    {
      found = 1;
      break;
    }
  }

  if (found)
    len = i + 1;
  else
    len = 0;

  long clk = ((tSysEx *)Copy)->GetClock();
  delete Copy;
  Copy = new tSysEx( clk, d + 1, len - 1 );

  gpSynth->FixSysexCheckSum( Copy->IsSysEx() );

#if 0
  printf("Sysex:");
  for (i = 0; i < ((tSysEx *)Copy)->Length; i++)
  {
    printf( "%02x ", ((tSysEx *)Copy)->Data[i] );
  }
  printf("\n");

  gpMidiPlayer->OutNow( (tSysEx *)Copy );
#endif

  delete str;

  tEventDlg::OnClose();
  return false;
}

void tSysexDlg::AddProperties()
{
//  char label1[100];
  unsigned char* uptr;

  if (Event->IsSysEx()->Length)
  {
//    sprintf(
//      label1,
//      "Loaded sysex: %s",
//      tSynthSysex::GetSysexName(gpSynth->GetSysexId(Event->IsSysEx())));

//    Add(wxMakeFormMessage(label1));

    sheet->AddProperty(new wxProperty(
      "Loaded sysex",
      wxPropertyValue(tSynthSysex::GetSysexName(gpSynth->GetSysexId(
        Event->IsSysEx()))),
      "string"));//r/o

    uptr = gpSynth->GetSysexValPtr(Event->IsSysEx());

    if (uptr)
    {
      ostringstream Oss;
      Oss
        << "First data byte is at offset "
        << uptr - Event->IsSysEx()->mpData + 1 << ", value "
        << setw(2) << hex << static_cast<int>(*uptr)
        << dec << " (" << static_cast<int>(*uptr) << " decimal)";
      sheet->AddProperty(new wxProperty(
        Oss.str().c_str(),
        wxPropertyValue((char*)""),
        "string"));//r/o
//      Add(wxMakeFormMessage(Oss.str().c_str()));
    }
  }
  else
  {
//    Add(wxMakeFormMessage("Example input: f0 7f 7f 04 01 00 7f f7"));
  }

//  Add(wxMakeFormMessage("(any DT1/RQ1 checksums will be corrected)"));

//  Add(wxMakeFormString("SysEx (hex):", &str, wxFORM_DEFAULT, NULL, NULL, wxVERTICAL, 300 ));
  sheet->AddProperty(new wxProperty("SysEx (hex)", wxPropertyValue((char**)&str), "string"));//r/o

  tEventDlg::AddProperties();
}


// --------------------------------------------------------------------------
// create new event
// --------------------------------------------------------------------------

static JZEvent *CreateEventDialog(long Clock, int Channel, int Pitch)
{
  wxArrayString Names;

   Names.Add("Note On");
   Names.Add( "Controller");
   Names.Add( "Program Change");
   Names.Add( "Set Tempo");
   Names.Add( "SysEx");
   Names.Add( "Play Track(experimental)");
   Names.Add("End Of Track");
   Names.Add( "Text" );

  static long Values[] =
  {
    StatKeyOn,
    StatControl,
    StatProgram,
    StatSetTempo,
    StatSysEx,
    StatPlayTrack,
    StatEndOfTrack,
    StatText,
    -1
  };

  JZEvent *e = 0;

  int i = ::wxGetSingleChoiceIndex(
    "Select event to create",
    "Create Event",
    Names);

  if (i >= 0)
  {
    switch (Values[i])
    {
      case StatKeyOn:
        e = new tKeyOn(Clock, Channel, Pitch, 64, 64);
        break;
      case StatPitch:
        e = new tPitch(Clock, Channel, 0);
        e->SetPitch(Pitch);
        break;
      case StatControl:
        e = new tControl(Clock, Channel, Pitch, 64);
        break;
      case StatProgram:
        e = new tProgram(Clock, Channel, Pitch);
        break;
     case StatSetTempo:
        e = new tSetTempo(Clock, 100);
        break;
     case StatSysEx:
        e = new tSysEx(Clock, (unsigned char*) "", 0);
        break;
     case StatPlayTrack:
        e = new tPlayTrack(Clock, 0, Pitch);
        break;
     case StatEndOfTrack:
        e = new tEndOfTrack(Clock);
        break;
     case StatText:
        e = new tText(Clock, (unsigned char*)"");
        break;
    }
  }
  return e;
}




void EventDialog(
  JZEvent* e,
  JZPianoWindow* pPianoWindow,
  JZTrack* t,
  long Clock,
  int Channel,
  int Pitch)
{
  if (!e)
    e = CreateEventDialog(Clock, Channel, Pitch);
  if (!e)
    return;

  tEventDlg *dlg = 0;
  const char* str = 0;
  switch (e->GetStat())
  {
    case StatKeyOn:
      if (t->GetAudioMode())
      {
        if (!gpMidiPlayer->IsPlaying())
          gpMidiPlayer->EditSample(e->IsKeyOn()->mKey);
        break;
      }
      str = "Key On";
//      dlg = new tKeyOnDlg(e->IsKeyOn(), pPianoWindow, t);
      {
        JZKeyOnDialog KeyOnDialog(e->IsKeyOn(), pPianoWindow);
        KeyOnDialog.ShowModal();
      }
      break;

    case StatPitch:
      str = "Pitch Wheel";
      dlg = new tPitchDlg(e->IsPitch(), pPianoWindow, t);
      break;

    case StatControl:
      str = "Controller";
      dlg = new tControlDlg(e->IsControl(), pPianoWindow, t);
      break;

    case StatProgram:
      str = "Program Change";
      dlg = new tProgramDlg(e->IsProgram(), pPianoWindow, t);
      break;

    case StatSetTempo:
      str = "Set Tempo (for track 0)";
      dlg = new tSetTempoDlg(e->IsSetTempo(), pPianoWindow, pPianoWindow->GetSong()->GetTrack(0) );
      break;

    case StatSysEx:
      str = "System Exclusive";
      dlg = new tSysexDlg(e->IsSysEx(), pPianoWindow, t );
      break;

    case StatPlayTrack:
      str = "Play Track";
      //dlg = new tPlayTrackDlg(e->IsPlayTrack(), pPianoWindow, t );
      dlg = new tPlayTrackDlg(e->IsPlayTrack(), pPianoWindow, t );
      break;
    case StatEndOfTrack:
      str = "End Of Track";
      dlg = new tEndOfTrackDlg(e->IsEndOfTrack(), pPianoWindow, t );
      break;

    case StatText:
      str = "Text";
      dlg = new tTextDlg(e->IsText(), pPianoWindow, t );
      break;


    default:
      break;
  }
  if (dlg)
  {
    dlg->Create();
  }
}

