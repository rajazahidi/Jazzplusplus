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

#include "Dialogs.h"

#include "ClockDialog.h"
#include "Command.h"
#include "DeprecatedWx/proplist.h"
#include "Dialogs/ControllerDialog.h"
#include "Dialogs/EndOfTrackDialog.h"
#include "Dialogs/KeyOnDialog.h"
#include "Dialogs/PitchWheelDialog.h"
#include "Dialogs/ProgramChangeDialog.h"
#include "Dialogs/SetTempoDialog.h"
#include "Dialogs/SysexDialog.h"
#include "Dialogs/TextDialog.h"
#include "Events.h"
#include "EventWindow.h"
#include "Filter.h"
#include "Globals.h"
#include "Help.h"
#include "NamedChoice.h"
#include "PianoFrame.h"
#include "PianoWindow.h"
#include "Player.h"
#include "ProjectManager.h"
#include "Song.h"
#include "Synth.h"
#include "Track.h"

#include <wx/choicdlg.h>

#include <iomanip>
#include <sstream>

using namespace std;

//*****************************************************************************
// Shift
//*****************************************************************************
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//tShiftDlg::tShiftDlg(JZEventFrame* pEventWindow, JZFilter* pFilter, long unit)
//  : tPropertyListDlg("Shift events left/right"),
//    mSteps(0),
//    mUnit(unit),
//    mpFilter(pFilter),
//    mpSong(pFilter->GetSong())
//{
//}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//bool tShiftDlg::OnClose()
//{
//  cout << "tShiftDlg::OnClose " << mSteps << endl;
//  tCmdShift cmd(mpFilter, mSteps * mUnit);
//  cmd.Execute();
//
//  JZProjectManager::Instance()->UpdateAllViews();
//
//  //  wxForm::OnOk();
//  return false;
//}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//void tShiftDlg::OnHelp()
//{
//  gpHelpInstance->ShowTopic("Shift");
//}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//void tShiftDlg::AddProperties()
//{
////send wxPropertyValue REFERENCE not POINTER
//  sheet->AddProperty(new wxProperty(
//    "Snaps",
//    wxPropertyValue(&mSteps),
//    "integer",
//    new wxIntegerListValidator(-16, 16)));
//
//  sheet->AddProperty(new wxProperty(
//    "clocks per snap",
//    (long)mUnit,
//    "integer")); // informational only
//}



//*****************************************************************************
// SearchReplace
//*****************************************************************************

//int tSearchReplaceDlg::frCtrl = 1;
//int tSearchReplaceDlg::toCtrl = 1;
//
//tSearchReplaceDlg::tSearchReplaceDlg(JZEventWindow* w, JZFilter *f)
//   : tPropertyListDlg("Search and replace controller types")
//{
//  Filter = f;
//  Song = f->GetSong();
//}

//bool tSearchReplaceDlg::OnClose()
//{
//  tCmdSearchReplace sr(Filter, frCtrl - 1, toCtrl-1);
//  sr.Execute();
//
//  JZProjectManager::Instance()->UpdateAllViews();
//
//  return false;
//}

//void tSearchReplaceDlg::AddProperties()
//{
//  sheet->AddProperty(new wxProperty(
//    "Search",
//    tNamedValueListValue(&frCtrl, gpConfig->GetControlNames()),
//    "props",
//    new tNamedValueListValidator(gpConfig->GetControlNames())));
//
//  sheet->AddProperty(new wxProperty(
//    "Replace",
//    tNamedValueListValue(&toCtrl, gpConfig->GetControlNames()),
//    "props",
//    new tNamedValueListValidator(gpConfig->GetControlNames())));
//}



//*****************************************************************************
// seqLength
//*****************************************************************************

double tSeqLengthDlg::scale = 1.0;



tSeqLengthDlg::tSeqLengthDlg(JZEventFrame *w, JZFilter *f)
: tPropertyListDlg("stretch/contract by scale from start of selected sequence" )
{
  Filter = f;
  Song = f->GetSong();
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


//*****************************************************************************
// midiDelay
//*****************************************************************************

double tMidiDelayDlg::scale = 0.5;
long tMidiDelayDlg::clockDelay = 10;
int tMidiDelayDlg::repeat = 6;

tMidiDelayDlg::tMidiDelayDlg(JZEventFrame *w, JZFilter *f)
: tPropertyListDlg("MIDI delay line" )
{
  Filter = f;
  Song = f->GetSong();
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



//*****************************************************************************
// Event-Dialogue
//*****************************************************************************

class tEventDlg : public tPropertyListDlg
{
  public:

    JZTrack    *Track;
    JZClockDialog ClockDlg;
    JZPianoWindow* Win;

    JZEvent    *Event;
    JZEvent    *Copy;

    tEventDlg(JZEvent *e, JZPianoWindow* w, JZTrack *pTrack);
    virtual void AddProperties();
    virtual bool OnClose();
    virtual void OnHelp();
    virtual void OnCancel();
};


tEventDlg::tEventDlg(JZEvent *e, JZPianoWindow* w, JZTrack *pTrack)
  : tPropertyListDlg( "Event" ),
    ClockDlg(w->GetProject(), "Time ", e->GetClock())
{
  Win   = w;
  Track = pTrack;
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

    tChEventDlg(JZChannelEvent* pChannelEvent, JZPianoWindow* w, JZTrack *pTrack)
      : tEventDlg(pChannelEvent, w, pTrack)
    {
      Channel = pChannelEvent->GetChannel() + 1;                // 1..16
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
  ((JZChannelEvent *)Copy)->SetChannel(Channel - 1);
  tEventDlg::OnClose();
  return false;
}

// -------------------------------- Play track ---------------------------

class tPlayTrackDlg : public tEventDlg
{
 public:

  int transpose;
  int track;
  int eventlength;

  tNamedChoice Choice;

  tPlayTrackDlg(JZPlayTrackEvent *e, JZPianoWindow* w, JZTrack *pTrack);

  void AddProperties();
  bool OnClose();
};


tPlayTrackDlg::tPlayTrackDlg(JZPlayTrackEvent *e, JZPianoWindow* w, JZTrack *pTrack)
  : tEventDlg(e, w, pTrack),
    Choice("playtrack", gpConfig->GetControlNames(), &track)
{
  Event = e;
  track = e->track;
  transpose=e->transpose;
  eventlength=e->eventlength;
}


bool tPlayTrackDlg::OnClose()
{
  JZPlayTrackEvent* p=(JZPlayTrackEvent*)Copy;

  Choice.GetValue();
  p->track = track;
  p->transpose = transpose;
  p->eventlength = eventlength;
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


//*****************************************************************************
// Description:
//   Display a dialog box to select an event to be created.
//*****************************************************************************
static JZEvent* CreateEventDialog(long Clock, int Channel, int Pitch)
{
  wxArrayString Names;

   Names.Add("Note On");
   Names.Add("Controller");
   Names.Add("Program Change");
   Names.Add("Set Tempo");
   Names.Add("SysEx");
   Names.Add("Play Track(experimental)");
   Names.Add("End Of Track");
   Names.Add("Text");

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

  JZEvent* pEvent = 0;

  int i = ::wxGetSingleChoiceIndex(
    "Select event to create",
    "Create Event",
    Names);

  if (i >= 0)
  {
    switch (Values[i])
    {
      case StatKeyOn:
        pEvent = new JZKeyOnEvent(Clock, Channel, Pitch, 64, 64);
        break;
      case StatPitch:
        pEvent = new JZPitchEvent(Clock, Channel, 0);
        pEvent->SetPitch(Pitch);
        break;
      case StatControl:
        pEvent = new JZControlEvent(Clock, Channel, Pitch, 64);
        break;
      case StatProgram:
        pEvent = new JZProgramEvent(Clock, Channel, Pitch);
        break;
     case StatSetTempo:
        pEvent = new JZSetTempoEvent(Clock, 100);
        break;
     case StatSysEx:
        pEvent = new JZSysExEvent(Clock, (unsigned char*) "", 0);
        break;
     case StatPlayTrack:
        pEvent = new JZPlayTrackEvent(Clock, 0, Pitch);
        break;
     case StatEndOfTrack:
        pEvent = new JZEndOfTrackEvent(Clock);
        break;
     case StatText:
        pEvent = new JZTextEvent(Clock, (unsigned char*)"");
        break;
    }
  }
  return pEvent;
}

//*****************************************************************************
// Description:
//   Display a dialog box to select an event to be created.
//*****************************************************************************
void EventDialog(
  JZEvent* pEvent,
  JZPianoWindow* pPianoWindow,
  JZTrack* pTrack,
  long Clock,
  int Channel,
  int Pitch)
{
  if (!pEvent)
  {
    pEvent = CreateEventDialog(Clock, Channel, Pitch);
  }

  if (!pEvent)
  {
    return;
  }

  tEventDlg* pDialog = 0;
  const char* str = 0;
  switch (pEvent->GetStat())
  {
    case StatKeyOn:
      if (pTrack->GetAudioMode())
      {
        if (!gpMidiPlayer->IsPlaying())
        {
          gpMidiPlayer->EditSample(pEvent->IsKeyOn()->GetKey());
        }
        break;
      }

      {
        JZKeyOnDialog KeyOnDialog(pEvent->IsKeyOn(), pTrack, pPianoWindow);
        KeyOnDialog.ShowModal();
      }
      break;

    case StatPitch:
      {
        JZPitchWheelDialog PitchWheelDialog(
          pEvent->IsPitch(),
          pTrack,
          pPianoWindow);
        PitchWheelDialog.ShowModal();
      }
      break;

    case StatControl:
      {
        JZControllerDialog ControllerDialog(
          pEvent->IsControl(),
          pTrack,
          pPianoWindow);
        ControllerDialog.ShowModal();
      }
      break;

    case StatProgram:
      {
        JZProgramChangeDialog ProgramChangeDialog(
          pEvent->IsProgram(),
          pTrack,
          pPianoWindow);
        ProgramChangeDialog.ShowModal();
      }
      break;

    case StatSetTempo:
      {
        JZSetTempoDialog SetTempoDialog(
          pEvent->IsSetTempo(),
          pPianoWindow->GetProject()->GetTrack(0),
          pPianoWindow);
        SetTempoDialog.ShowModal();
      }
      break;

    case StatSysEx:
      {
        JZSysexDialog SysexDialog(pEvent->IsSysEx(), pTrack, pPianoWindow);
        SysexDialog.ShowModal();
      }
      break;

    case StatPlayTrack:
      str = "Play Track";
      pDialog = new tPlayTrackDlg(pEvent->IsPlayTrack(), pPianoWindow, pTrack);
      break;

    case StatEndOfTrack:
      {
        JZEndOfTrackDialog EndOfTrackDialog(
          pEvent->IsEndOfTrack(),
          pTrack,
          pPianoWindow);
        EndOfTrackDialog.ShowModal();
      }
      break;

    case StatText:
      {
        JZTextDialog SysexDialog(pEvent->IsText(), pTrack, pPianoWindow);
        SysexDialog.ShowModal();
      }
      break;

    default:
      break;
  }

  if (pDialog)
  {
    pDialog->Create();
  }
}
