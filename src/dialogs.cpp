
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

#include "wx/wx.h"

#include "config.h"
#include "dialogs.h"
#include "song.h"
#include "command.h"
#include "eventwin.h"
#include "track.h"
#include "events.h"
#include "util.h"
#include "jazz.h"
#include "player.h"
#include "pianowin.h"


// **************************************************************************
// Shift
// *************************************************************************


//long tShiftDlg::Steps = 0;

tShiftDlg::tShiftDlg(tEventWin *w, tFilter *f, long unit)
: tPropertyListDlg( "Shift events left/right" )
{
  Filter = f;
  Song = f->Song;
  Unit  = unit;
  EventWin = w;
  Steps=0;
}



bool tShiftDlg::OnClose()
{
  cout << "tShiftDlg::OnClose "<<Steps<<endl;
  tCmdShift cmd(Filter, Steps * Unit);
  cmd.Execute();
  EventWin->Redraw();
  if (EventWin->NextWin)
  	EventWin->NextWin->Redraw();
  //  wxForm::OnOk();
  return false;
}


void tShiftDlg::OnHelp()
{
#ifndef __PORTING
	HelpInstance->ShowTopic("Shift");
#endif // __PORTING
}


void tShiftDlg::AddProperties()
{

//send wxPropertyValue REFERENCE not POINTER
  sheet->AddProperty(new wxProperty("Snaps",  wxPropertyValue(&Steps), "integer", new wxIntegerListValidator(-16, 16)));
  sheet->AddProperty(new wxProperty("clocks per snap",  (long)Unit, "integer"));//informational only

}





// **************************************************************************
// Cleanup
// *************************************************************************

long tCleanupDlg::lowLimit = 48;
Bool tCleanupDlg::shortenOverlaps = 1;



tCleanupDlg::tCleanupDlg(tEventWin *w, tFilter *f)
  : tPropertyListDlg( "Clean up events" )
{
  //limitSteps lives in util.cpp

  Filter = f;
  Song = f->Song;
  EventWin = w;
}



bool tCleanupDlg::OnClose()
{

  int limit = Song->TicksPerQuarter * 4 / lowLimit;
  cout<<"tCleanupDlg::OnClose "<<lowLimit<<" "<<shortenOverlaps<<endl;
  tCmdCleanup cln(Filter, limit, shortenOverlaps);
  cln.Execute();
  EventWin->Redraw();
  if (EventWin->NextWin)
 	 EventWin->NextWin->Redraw();
  //wxForm::OnOk();
  return FALSE;
}

void tCleanupDlg::OnHelp()
{
#ifndef __PORTING
  HelpInstance->ShowTopic("Cleanup");
#endif // __PORTING
}


IMPLEMENT_DYNAMIC_CLASS(tNamedValueListValue, wxPropertyValue)

void tCleanupDlg::AddProperties()
{
  //JAVE this doesnt work
  //  sheet->AddProperty(new wxProperty("Delete notes shorther than", wxPropertyValue("1/8") , "props", Steps.GetStringListValidator()))  ;

  //how it ought to work
  //sheet->AddProperty(new wxProperty("Delete notes shorther than", wxPropertyValue((long*)&lowLimit) , 
  //			    "props", new tNamedValueListValidator(limitSteps)))  ; //limitSteps is a name/value pair array in util.cpp

  //how it really ought to work(except the validator might ask the "value" for the list of allowed values)
  //http://sourceforge.net/tracker/?group_id=9863&atid=109863 wx bugracker
  tNamedValueListValue val1 = tNamedValueListValue((long*)&lowLimit, limitSteps);
  wxPropertyValue& val=val1;
  cout << "little test:"<<val.GetStringRepresentation()<<endl;
  sheet->AddProperty(new wxProperty("Delete shorther than", (tNamedValueListValue&)val1 , 
  				    "props", new tNamedValueListValidator(limitSteps)))  ; //limitSteps is a name/value pair array in util.cpp

    
  sheet->AddProperty(new wxProperty("Shorten overlapping", wxPropertyValue((bool*)&shortenOverlaps),"bool"));

  //there seems to be a padding limit in wxPropertyListView::MakeNameValueString set to 25

}


// **************************************************************************
// SearchReplace
// *************************************************************************

long tSearchReplaceDlg::frCtrl = 1;
long tSearchReplaceDlg::toCtrl = 1;

tSearchReplaceDlg::tSearchReplaceDlg(tEventWin *w, tFilter *f)
   : tPropertyListDlg("Search and replace controller types" )
{
  Filter = f;
  Song = f->Song;
  EventWin = w;
}

bool tSearchReplaceDlg::OnClose()
{
  tCmdSearchReplace sr(Filter, frCtrl-1, toCtrl-1);
  sr.Execute();
  EventWin->Redraw();
  if (EventWin->NextWin)
    EventWin->NextWin->Redraw();

  return FALSE;
}

void tSearchReplaceDlg::OnHelp()
{

  HelpInstance->ShowTopic("Search Replace");

}

void tSearchReplaceDlg::AddProperties()
{
   sheet->AddProperty(new wxProperty("Search", tNamedValueListValue((long*)&frCtrl, &Config.CtrlName(0)) , 
  				    "props", new tNamedValueListValidator(&Config.CtrlName(0))))  ;

   sheet->AddProperty(new wxProperty("Replace", tNamedValueListValue((long*)&toCtrl, &Config.CtrlName(0)) , 
  				    "props", new tNamedValueListValidator(&Config.CtrlName(0))))  ;


}




// **************************************************************************
// Transpose
// *************************************************************************

static tNamedValue ScaleNames[] =
{
  tNamedValue("None",     ScaleChromatic),
  tNamedValue("Selected", ScaleSelected),
  tNamedValue("C",   0),
  tNamedValue("C#",  1),
  tNamedValue("D",   2),
  tNamedValue("D#",  3),
  tNamedValue("E",   4),
  tNamedValue("F",   5),
  tNamedValue("F#",  6),
  tNamedValue("G",   7),
  tNamedValue("G#",  8),
  tNamedValue("A",   9),
  tNamedValue("A#", 10),
  tNamedValue("B",  11),
  tNamedValue( 0,   ScaleChromatic)
};


int tTransposeDlg::Notes  = 0;
long tTransposeDlg::Scale  = ScaleChromatic;
bool tTransposeDlg::FitIntoScale = 0;

tTransposeDlg::tTransposeDlg(tEventWin *w, tFilter *f)
  : tPropertyListDlg("Transpose")
{
  EventWin = w;
  Filter = f;
  Song   = f->Song;
}


bool tTransposeDlg::OnClose()
{
  tCmdTranspose trn(Filter, Notes, Scale, FitIntoScale);
  trn.Execute();
  if (EventWin->NextWin)
  	EventWin->NextWin->Redraw();
  else
  	EventWin->Redraw();
  return FALSE;
}

void tTransposeDlg::OnHelp()
{
	HelpInstance->ShowTopic("Transpose");
}


void tTransposeDlg::AddProperties()
{
   int s = tScale::Analyze(Filter);

  sheet->AddProperty(new wxProperty("selection looks like", wxPropertyValue(ScaleNames[s+2].Name),"string"));
  sheet->AddProperty(new wxProperty("Amount",wxPropertyValue((long*) &Notes), "integer",
		     new wxIntegerListValidator(-12, 12)));
  sheet->AddProperty(new wxProperty("Fit into Scale", wxPropertyValue((bool*)&FitIntoScale), "bool"));
		     }

// **************************************************************************
// SetChannel
// *************************************************************************

int tSetChannelDlg::NewChannel = 1;

tSetChannelDlg::tSetChannelDlg(tFilter *f)
: tPropertyListDlg("Set MIDI Channel")
{
  Filter = f;
  Song = f->Song;
}



bool tSetChannelDlg::OnClose()
{
  if (NewChannel)
  {
    tCmdSetChannel exe(Filter, NewChannel - 1);
    exe.Execute();
  }
  //tPropertyListDlg::OnClose();
  return FALSE;
}

void tSetChannelDlg::OnHelp()
{
        HelpInstance->ShowTopic("Set MIDI Channel");
}


void tSetChannelDlg::AddProperties()
{
 //  Add(wxMakeFormShort("new Channel", &NewChannel, wxFORM_DEFAULT,
//                        new wxList(wxMakeConstraintRange(1.0, 16.0), 0)));
//   Add(wxMakeFormNewLine());
  // AssociatePanel(panel);
  
  sheet->AddProperty(new wxProperty("new Channel", wxPropertyValue((long*)&NewChannel), "integer", new wxIntegerListValidator(1, 16)));
}



// **************************************************************************
// Velocity
// *************************************************************************

int tVelocityDlg::FromValue = 64;
int tVelocityDlg::ToValue = 0;
int tVelocityDlg::Mode = 0;


tVelocityDlg::tVelocityDlg(tFilter *f)
: tPropertyListDlg( "Velocity" )
{
  Filter = f;
  Song = f->Song;
}


bool tVelocityDlg::OnClose()
{
  tCmdVelocity cmd(Filter, FromValue, ToValue, Mode);
  cmd.Execute();
  return FALSE;
}

void tVelocityDlg::OnHelp()
{
        HelpInstance->ShowTopic("Velocity");
}

static tNamedValue modes[] =
    {
      tNamedValue( "Set",   8 ),
      tNamedValue( "Add", 12 ),
      tNamedValue( "Sub", 16 ),
      tNamedValue(   0,      1  )
};

void tVelocityDlg::AddProperties()
{
  
    sheet->AddProperty(new wxProperty("Start",  wxPropertyValue((long*)&FromValue), "integer", new wxIntegerListValidator(0, 127)));
    sheet->AddProperty(new wxProperty("Stop",  wxPropertyValue((long*)&ToValue), "integer", new wxIntegerListValidator(0, 127)));
    sheet->AddProperty(new wxProperty("Mode",  tNamedValueListValue((long*)&Mode, modes), "props", new tNamedValueListValidator(modes)));

}




// **************************************************************************
// Length
// *************************************************************************

int tLengthDlg::FromValue = 30;
int tLengthDlg::ToValue = 0;

 int  tLengthDlg::Mode;

tLengthDlg::tLengthDlg(tEventWin *w, tFilter *f)
: tPropertyListDlg("Length")
{
  Filter = f;
  Song = f->Song;
  EventWin = w;
}


bool tLengthDlg::OnClose()
{
  tCmdLength cmd(Filter, FromValue, ToValue, Mode);
  cmd.Execute();

  EventWin->Redraw();
  if (EventWin->NextWin)
  	EventWin->NextWin->Redraw();
  //tPropertyListDlg::OnClose();
  return FALSE;
}

void tLengthDlg::OnHelp()
{
        HelpInstance->ShowTopic("Length");
}


void tLengthDlg::AddProperties()
{


  sheet->AddProperty(new wxProperty("Ticks/Quarter",  wxPropertyValue((long)Song->TicksPerQuarter), "integer", new wxIntegerListValidator(-16, 16)));  //r/o

  sheet->AddProperty(new wxProperty("Start",  wxPropertyValue(&FromValue), "integer", new wxIntegerListValidator(0, Song->TicksPerQuarter * 4)));
  sheet->AddProperty(new wxProperty("Stop",  wxPropertyValue(&ToValue), "integer", new wxIntegerListValidator(0, Song->TicksPerQuarter * 4)));
  sheet->AddProperty(new wxProperty("Mode",  tNamedValueListValue((long*)&Mode, modes), "props", new tNamedValueListValidator(modes)));

}



// **************************************************************************
// seqLength
// *************************************************************************

double tSeqLengthDlg::scale = 1.0;



tSeqLengthDlg::tSeqLengthDlg(tEventWin *w, tFilter *f)
: tPropertyListDlg("stretch/contract by scale from start of selected sequence" )
{
  Filter = f;
  Song = f->Song;
  EventWin = w;
}


bool tSeqLengthDlg::OnClose()
{

  tCmdSeqLength cmd(Filter, scale);
  cmd.Execute();
  EventWin->Redraw();
  if (EventWin->NextWin)
  	EventWin->NextWin->Redraw();
  //tPropertyListDlg::OnClose();
  return FALSE;
}

void tSeqLengthDlg::OnHelp()
{
        HelpInstance->ShowTopic("SeqLength");
}


void tSeqLengthDlg::AddProperties()
{

  sheet->AddProperty(new wxProperty("Scale", wxPropertyValue((double*)&scale), "double", new wxRealListValidator()));
}


// **************************************************************************
// midiDelay
// *************************************************************************

double tMidiDelayDlg::scale = 0.5;
long tMidiDelayDlg::clockDelay = 10;
int tMidiDelayDlg::repeat = 6;

tMidiDelayDlg::tMidiDelayDlg(tEventWin *w, tFilter *f)
: tPropertyListDlg("MIDI delay line" )
{
  Filter = f;
  Song = f->Song;
  EventWin = w;
}


bool tMidiDelayDlg::OnClose()
{

  tCmdMidiDelay cmd(Filter, scale,clockDelay,repeat);
  cmd.Execute();
  EventWin->Redraw();
  if (EventWin->NextWin)
  	EventWin->NextWin->Redraw();
  //tPropertyListDlg::OnClose();
  return FALSE;
}

void tMidiDelayDlg::OnHelp()
{
        HelpInstance->ShowTopic("MidiDelay");
}


void tMidiDelayDlg::AddProperties()
{

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
  sheet->AddProperty(new wxProperty("Scale",  wxPropertyValue((double*)&scale), "real", new wxRealListValidator(0.0001, 4.0)));
  sheet->AddProperty(new wxProperty("Delay in clocks",  wxPropertyValue((long*)&clockDelay), "integer", new wxIntegerListValidator(-1000, 1000)));
  sheet->AddProperty(new wxProperty("Repeats",  wxPropertyValue((long*)&repeat), "integer", new wxIntegerListValidator(0, 100)));

}



// *************************************************************************
// Delete
// *************************************************************************

Bool tDeleteDlg::LeaveSpace = 1;

tDeleteDlg::tDeleteDlg(tEventWin *w, tFilter *f)
: tPropertyListDlg("Delete" )
{
  Filter = f;
  EventWin = w;
}


bool tDeleteDlg::OnClose()
{
  tCmdErase cmd(Filter, LeaveSpace);
  cmd.Execute();
  EventWin->Redraw();
  if (EventWin->NextWin)
  	EventWin->NextWin->Redraw();
  //  tPropertyListDlg::OnClose();
  return FALSE;
  
}

void tDeleteDlg::OnHelp()
{
        HelpInstance->ShowTopic("Delete");
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

tSnapDlg::tSnapDlg(tPianoWin *w, long *snapptr)
  : tPropertyListDlg("Snap:quantize cut/paste events")
{
//, Steps("Snap value", limitSteps, snapptr)
  //limitSteps lives in util.cpp
   win = w;
   ptr = snapptr;
}



bool tSnapDlg::OnClose()
{
  //Steps.GetValue();
  // toggle the tool buttons
  win->SetSnapDenom(*ptr);
  //tPropertyListDlg::OnClose();
  return FALSE;
}

void tSnapDlg::OnHelp()
{
  HelpInstance->ShowTopic("Snap");
}

void tSnapDlg::AddProperties()
{
//   panel->SetLabelPosition(wxVERTICAL);
//   Add(wxMakeFormMessage("quantize cut/paste events"));
//   Add(wxMakeFormNewLine());
//   Add(Steps.mkFormItem(100));
//   Add(wxMakeFormNewLine());
//   AssociatePanel(panel);
   sheet->AddProperty(new wxProperty("Steps", tNamedValueListValue((long*)ptr, limitSteps) , 
  				    "props", new tNamedValueListValidator(limitSteps)))  ; //limitSteps is a name/value pair array in util.cpp

}


// **************************************************************************
// Quantize
// *************************************************************************

bool tQuantizeDlg::NoteStart = 1;
bool tQuantizeDlg::NoteLength = 0;
long tQuantizeDlg::QntStep = 16;
int  tQuantizeDlg::Delay = 0;
int  tQuantizeDlg::Groove = 0;

static tNamedValue QntSteps[] =
{
  tNamedValue( "1/8",   8 ),
  tNamedValue( "1/12", 12 ),
  tNamedValue( "1/16", 16 ),
  tNamedValue( "1/24", 24 ),
  tNamedValue( "1/32", 32 ),
  tNamedValue( "1/48", 48 ),
  tNamedValue( "1/96", 96 ),
  tNamedValue(  0,      1  )
};


tQuantizeDlg::tQuantizeDlg(tEventWin *w, tFilter *f)
   : tPropertyListDlg("Quantize" )
  //, Steps("steps", QntSteps, &QntStep)
{
  Filter = f;
  Song = f->Song;
  EventWin = w;
}



bool tQuantizeDlg::OnClose()
{
  //Steps.GetValue();
  int step = Song->TicksPerQuarter * 4 / QntStep;
  tCmdQuantize qnt(Filter, step, Groove * step / 100, Delay * step / 100);
  qnt.NoteStart = NoteStart;
  qnt.NoteLength = NoteLength;
  qnt.Execute();
  EventWin->Redraw();
  if (EventWin->NextWin)
 	 EventWin->NextWin->Redraw();
  //tPropertyListDlg::OnClose();
  return FALSE;
}

void tQuantizeDlg::OnHelp()
{
  	if (EventWin->NextWin)
		HelpInstance->ShowTopic("Quantize");
	else
		HelpInstance->ShowTopic("Pianowin Quantize");
}

void tQuantizeDlg::AddProperties()
{

    sheet->AddProperty(new wxProperty("Note start", wxPropertyValue((bool*)&NoteStart),"bool"));
    sheet->AddProperty(new wxProperty("Note length", wxPropertyValue((bool*)&NoteLength),"bool"));
    sheet->AddProperty(new wxProperty("Groove", wxPropertyValue((double*)&Groove), "double", new wxRealListValidator(-100,100)));
    sheet->AddProperty(new wxProperty("Delay", wxPropertyValue((double*)&Delay), "double", new wxRealListValidator(-100,100)));
}


// ***********************************************************************
// Event-Dialogue
// ***********************************************************************

class tEventDlg : public tPropertyListDlg
{
  public:

    tTrack    *Track;
    tClockDlg ClockDlg;
    tEventWin *Win;

    tEvent    *Event;
    tEvent    *Copy;

    tEventDlg(tEvent *e, tEventWin *w, tTrack *t);
    virtual void AddProperties();
    virtual bool OnClose();
    virtual void OnHelp();
    virtual void OnCancel();
};


tEventDlg::tEventDlg(tEvent *e, tEventWin *w, tTrack *t)
  : tPropertyListDlg( "Event" ), ClockDlg(w->Song, "Time ", e->Clock)
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
  Copy->Clock = ClockDlg.GetClock();
  Track->Kill(Event);
  Track->Put(Copy);
  Track->Cleanup();
  Win->Redraw();
  if (Win->NextWin)
  	Win->NextWin->Redraw();
  tPropertyListDlg::OnClose();
}

void tEventDlg::OnHelp()
{
        HelpInstance->ShowTopic("Piano Window");
}

// --------------------------- ChannelEvent ----------------------------


class tChEventDlg : public tEventDlg
{
  public:

    int Channel;

    tChEventDlg(tChannelEvent *e, tEventWin *w, tTrack *t)
      : tEventDlg(e, w, t)
    {
      Channel = e->Channel + 1;		// 1..16
    }
    void AddProperties();
    bool OnClose();
};

void tChEventDlg::AddProperties()
{
  sheet->AddProperty(new wxProperty("Channel", wxPropertyValue((long*)&Channel), "integer", new wxIntegerListValidator(1,16)));
  tEventDlg::AddProperties();
}


bool tChEventDlg::OnClose()
{
  ((tChannelEvent *)Copy)->Channel = Channel - 1;
  tEventDlg::OnClose();
  return FALSE;
}

// -------------------------------- Note On -------------------------------

class tKeyOnDlg : public tChEventDlg
{
 public:

  tKeyDlg PitchDlg;
  int Pitch;
  int Veloc;
  int Length;
  // SN++
  int OffVeloc;

  tKeyOnDlg(tKeyOn *e, tEventWin *w, tTrack *t);

  void AddProperties();
  bool OnClose();
};


tKeyOnDlg::tKeyOnDlg(tKeyOn *e, tEventWin *w, tTrack *t)
  : tChEventDlg(e, w, t),
    PitchDlg("Pitch", e->Key)
{
  Event = e;
  Veloc = e->Veloc;
  Pitch = e->Key;
  Length = e->Length;
  // SN++ Off veloc support
  OffVeloc = e->OffVeloc;
}


bool tKeyOnDlg::OnClose()
{
  tKeyOn *k = (tKeyOn *)Copy;
  k->Key = PitchDlg.GetKey();
  k->Veloc = Veloc;
  k->Length = Length;
  // SN++ off veloc support
  k->OffVeloc = OffVeloc;
  tChEventDlg::OnClose();
  return FALSE;
}

void tKeyOnDlg::AddProperties()
{
//   char* tst = copystring("test");
//   sheet->AddProperty(new wxProperty("test", wxPropertyValue((char**)&tst), "string"));

  sheet->AddProperty(PitchDlg.mkProperty());
  sheet->AddProperty(new wxProperty("Velocity", wxPropertyValue((long*)&Veloc), "integer", new wxIntegerListValidator(1,127)));
  // SN++ off veloc support
  //  Add(wxMakeFormShort("OffVel:", &OffVeloc, wxFORM_DEFAULT, new wxList(wxMakeConstraintRange(0.0, 127.0), 0)));
  sheet->AddProperty(new wxProperty("Off Velocity", wxPropertyValue((long*)&OffVeloc), "integer", new wxIntegerListValidator(0,127)));
  sheet->AddProperty(new wxProperty("Length", wxPropertyValue((long*)&Length), "integer"));
  //  Add(wxMakeFormShort("Length:", &Length, wxFORM_DEFAULT,0,0,0,120));
  tChEventDlg::AddProperties();
}


// -------------------------------- Pitch -------------------------------

class tPitchDlg : public tChEventDlg
{
 public:

  int Value;

  tPitchDlg(tPitch *e, tEventWin *w, tTrack *t);

  void AddProperties();
  bool OnClose();
};


tPitchDlg::tPitchDlg(tPitch *e, tEventWin *w, tTrack *t)
  : tChEventDlg(e, w, t)
{
  Event = e;
  Value = e->Value;
}


bool tPitchDlg::OnClose()
{
  ((tPitch *)Copy)->Value = Value;
  tChEventDlg::OnClose();
}

void tPitchDlg::AddProperties()
{
  //Add(wxMakeFormShort("Pitch:", &Value, wxFORM_DEFAULT, new wxList(wxMakeConstraintRange(-8191.0, 8191.0), 0)));
  sheet->AddProperty(new wxProperty("Pitch:", wxPropertyValue((long*)&Value), "integer", new wxIntegerListValidator(-8191,8191)));

  tChEventDlg::AddProperties();
}

// -------------------------------- Controller ---------------------------

class tControlDlg : public tChEventDlg
{
 public:

  int Value;
  long Control;
  //tNamedChoice Choice;

  tControlDlg(tControl *e, tEventWin *w, tTrack *t);

  void AddProperties();
  bool OnClose();
};


tControlDlg::tControlDlg(tControl *e, tEventWin *w, tTrack *t)
  : tChEventDlg(e, w, t)
  //,    Choice("Controller", &Config.CtrlName(0), &Control)
{
  Event = e;
  Value = e->Value;
  Control = e->Control + 1;
}


bool tControlDlg::OnClose()
{
  ((tControl *)Copy)->Value = Value;
  //  Choice.GetValue();
  ((tControl *)Copy)->Control = Control - 1;
  tChEventDlg::OnClose();
}

void tControlDlg::AddProperties()
{
  //  Add(Choice.mkFormItem(300, 300));
 //,    Choice("Controller", &Config.CtrlName(0), &Control)
     sheet->AddProperty(new wxProperty("Controller", tNamedValueListValue((long*)Control, &Config.CtrlName(0)) , 
				       "props", new tNamedValueListValidator(&Config.CtrlName(0))))  ;

  sheet->AddProperty(new wxProperty("Value", wxPropertyValue((long*)&Value), "integer", new wxIntegerListValidator(0,127)));
  //  Add(wxMakeFormShort("Value:", &Value, wxFORM_DEFAULT, new wxList(wxMakeConstraintRange(0.0, 127.0), 0)));
  tChEventDlg::AddProperties();
}
// -------------------------------- Play track ---------------------------

class tPlayTrackDlg : public tEventDlg
{
 public:

  long transpose;
  long track;
  int eventlength;

  tNamedChoice Choice;

  tPlayTrackDlg(tPlayTrack *e, tEventWin *w, tTrack *t);

  void AddProperties();
  bool OnClose();
};


tPlayTrackDlg::tPlayTrackDlg(tPlayTrack *e, tEventWin *w, tTrack *t)
  : tEventDlg(e, w, t),
    Choice("playtrack", &Config.CtrlName(0), &track)
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
  tEventDlg::OnClose();
}

void tPlayTrackDlg::AddProperties()
{
  sheet->AddProperty(new wxProperty("Track", wxPropertyValue((long*)&track), "integer", new wxIntegerListValidator(0,127)));
  sheet->AddProperty(new wxProperty("Transpose", wxPropertyValue((long*)&transpose), "integer", new wxIntegerListValidator(-127,127)));
  sheet->AddProperty(new wxProperty("Length", wxPropertyValue((long*)&eventlength), "integer", new wxIntegerListValidator(0,127)));
  tEventDlg::AddProperties();
}

// -------------------------------- text event ---------------------------

class tTextDlg : public tEventDlg
{
 public:

  char* text;
  long track;
  tNamedChoice Choice;

  tTextDlg(tText *e, tEventWin *w, tTrack *t);

  void AddProperties();
  bool OnClose();
};


tTextDlg::tTextDlg(tText *e, tEventWin *w, tTrack *t)
  : tEventDlg(e, w, t),
    Choice("text", &Config.CtrlName(0), &track)
{
  Event = e;
  text=new char[2048];
  strcpy(text,(const char*)(e->GetText()));
}


bool tTextDlg::OnClose()
{
  tText* p;
  p=new tText( ((tText*)Copy)->GetClock(), (uchar*)text, strlen(text));
  delete Copy;
  Copy=p;
  fprintf(stderr,"text:%s",text);
  Choice.GetValue();
  delete text;
  tEventDlg::OnClose();
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
  long track;

  tNamedChoice Choice;

  tEndOfTrackDlg(tEndOfTrack *e, tEventWin *w, tTrack *t);

  void AddProperties();
  bool OnClose();
};


tEndOfTrackDlg::tEndOfTrackDlg(tEndOfTrack *e, tEventWin *w, tTrack *t)
  : tEventDlg(e, w, t),
    Choice("End Of Track", &Config.CtrlName(0), &track)
{

}


bool tEndOfTrackDlg::OnClose()
{
  tEndOfTrack* p=(tEndOfTrack*)Copy;
  Choice.GetValue();
  tEventDlg::OnClose();
}

void tEndOfTrackDlg::AddProperties()
{
  tEventDlg::AddProperties();
}


// -------------------------------- Program ---------------------------

class tProgramDlg : public tEventDlg
{
 public:

  long Program;
  //  tNamedChoice Choice;

  tProgramDlg(tProgram *e, tEventWin *w, tTrack *t);

  void AddProperties();
  bool OnClose();
};


tProgramDlg::tProgramDlg(tProgram *e, tEventWin *w, tTrack *t)
  : tEventDlg(e, w, t),
    Program(e->Program + 1)
  //,    Choice("Program", &Config.VoiceName(0), &Program)
{
  Event = e;
}


bool tProgramDlg::OnClose()
{
  //Choice.GetValue();
  if (Program <= 0)
    Program = 1;
  ((tProgram *)Copy)->Program = Program - 1;
  tEventDlg::OnClose();
}

void tProgramDlg::AddProperties()
{
  //Add(Choice.mkFormItem(300, 300));
  sheet->AddProperty(new wxProperty("Program", tNamedValueListValue((long*)Program, &Config.VoiceName(0)) , 
				       "props", new tNamedValueListValidator(&Config.VoiceName(0))))  ;
}



// -------------------------------- Set Tempo -------------------------------

class tSetTempoDlg : public tEventDlg
{
 public:

  int Value;

  tSetTempoDlg(tSetTempo *e, tEventWin *w, tTrack *t);

  void AddProperties();
  bool OnClose();
};


tSetTempoDlg::tSetTempoDlg(tSetTempo *e, tEventWin *w, tTrack *t)
  : tEventDlg(e, w, t)
{
  Event = e;
  Value = e->GetBPM();
}


bool tSetTempoDlg::OnClose()
{
  ((tSetTempo *)Copy)->SetBPM( Value );
  tEventDlg::OnClose();
}

void tSetTempoDlg::AddProperties()
{
  //  Add(wxMakeFormShort("Tempo:", &Value, wxFORM_DEFAULT, new wxList(wxMakeConstraintRange(20.0, 240.0), 0)));
  sheet->AddProperty(new wxProperty("Tempo", wxPropertyValue((long*)&Value), "integer", new wxIntegerListValidator(20,240)));
  tEventDlg::AddProperties();
}

// -------------------------------- Sysex Edit-------------------------------

class tSysexDlg : public tEventDlg
{
  public:

  char *str;

  tSysexDlg(tSysEx *s, tEventWin *w, tTrack *t);

  void AddProperties();
  bool OnClose();
};


tSysexDlg::tSysexDlg(tSysEx *s, tEventWin *w, tTrack *t)
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
     sprintf( hexbyte, "%02x ", s->Data[i] );
     strcat( str, hexbyte );
  }
}


bool tSysexDlg::OnClose()
{
   int i;
   int j;
   int k;
   int len;

   uchar d[256];
   memset( d, 0, 256 );

   int jstop = strlen(str);

   for (i = 0, j = 0; j <= jstop; j += k, i++)
   {
      sscanf( str + j, "%02x %n", &d[i], &k );

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

   long clk = ((tSysEx *)Copy)->Clock;
   delete Copy;
   Copy = new tSysEx( clk, d + 1, len - 1 );

   Synth->FixSysexCheckSum( Copy->IsSysEx() );

#if 0
   printf("Sysex:");
   for (i = 0; i < ((tSysEx *)Copy)->Length; i++)
   {
      printf( "%02x ", ((tSysEx *)Copy)->Data[i] );
   }
   printf("\n");

   Midi->OutNow( (tSysEx *)Copy );
#endif

   delete str;

   tEventDlg::OnClose();
   return FALSE;
}

void tSysexDlg::AddProperties()
{
   char label1[100];
   char label2[100];
   uchar *uptr;

   if (Event->IsSysEx()->Length)
   {
     //      sprintf( label1, "Loaded sysex: %s", SysexNames[Synth->GetSysexId(Event->IsSysEx())].Name );

     //Add(wxMakeFormMessage(label1));

     sheet->AddProperty(new wxProperty("Loaded sysex", wxPropertyValue((char*)SysexNames[Synth->GetSysexId(Event->IsSysEx())].Name), "string"));//r/o

      uptr = Synth->GetSysexValPtr(Event->IsSysEx());

      if (uptr)
      {
	 sprintf( label2, "First data byte is at offset %d, value %02x (%d decimal)", uptr - Event->IsSysEx()->Data + 1, *uptr, *uptr );
	 sheet->AddProperty(new wxProperty(label2, wxPropertyValue((char*)""), "string"));//r/o
     //	 Add(wxMakeFormMessage(label2));
      }
   }
   else
   {
     //Add(wxMakeFormMessage("Example input: f0 7f 7f 04 01 00 7f f7"));
   }

      //Add(wxMakeFormMessage("(any DT1/RQ1 checksums will be corrected)"));

      //Add(wxMakeFormString("SysEx (hex):", &str, wxFORM_DEFAULT, NULL, NULL, wxVERTICAL, 300 ));
      sheet->AddProperty(new wxProperty("SysEx (hex)", wxPropertyValue((char**)&str), "string"));//r/o
   tEventDlg::AddProperties();
}


// --------------------------------------------------------------------------
// create new event
// --------------------------------------------------------------------------

static tEvent *CreateEventDialog(long Clock, int Channel, int Pitch)
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
  { StatKeyOn, StatControl, StatProgram, StatSetTempo, StatSysEx, StatPlayTrack, StatEndOfTrack, StatText, -1 };
  tEvent *e = 0;
  int i = wxGetSingleChoiceIndex("Select event to create", "Create Event", Names);
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
	e = new tSysEx(Clock, (uchar*) "", 0);
	break;
     case StatPlayTrack:
	e = new tPlayTrack(Clock, 0, Pitch);
	break;
     case StatEndOfTrack:
	e = new tEndOfTrack(Clock);
	break;
     case StatText:
	e = new tText(Clock, (uchar*)"");
	break;
    }
  }
  return e;
}




void EventDialog(tEvent *e, tEventWin *w, tTrack *t, long Clock, int Channel, int Pitch)
{
  if (!e)
    e = CreateEventDialog(Clock, Channel, Pitch);
  if (!e)
    return;

  tEventDlg *dlg = 0;
  char *str = 0;
  switch (e->Stat)
  {
    case StatKeyOn:
#ifdef AUDIO
      if (t->GetAudioMode()) {
        if (!Midi->IsPlaying())
	  Midi->EditSample(e->IsKeyOn()->Key);
	break;
      }
#endif
      str = "Key On";
      dlg = new tKeyOnDlg(e->IsKeyOn(), w, t);
      break;

    case StatPitch:
      str = "Pitch Wheel";
      dlg = new tPitchDlg(e->IsPitch(), w, t);
      break;

    case StatControl:
      str = "Controller";
      dlg = new tControlDlg(e->IsControl(), w, t);
      break;

    case StatProgram:
      str = "Program Change";
      dlg = new tProgramDlg(e->IsProgram(), w, t);
      break;

    case StatSetTempo:
      str = "Set Tempo (for track 0)";
      dlg = new tSetTempoDlg(e->IsSetTempo(), w, w->Song->GetTrack(0) );
      break;

    case StatSysEx:
      str = "System Exclusive";
      dlg = new tSysexDlg(e->IsSysEx(), w, t );
      break;

    case StatPlayTrack:
      str = "Play Track";
      //dlg = new tPlayTrackDlg(e->IsPlayTrack(), w, t );
      dlg = new tPlayTrackDlg(e->IsPlayTrack(), w, t );
      break;
    case StatEndOfTrack:
      str = "End Of Track";
      dlg = new tEndOfTrackDlg(e->IsEndOfTrack(), w, t );
      break;

    case StatText:
      str = "Text";
      dlg = new tTextDlg(e->IsText(), w, t );
      break;


    default:
      break;
  }
  if (dlg)
  {
    dlg->Create();
  }
}

