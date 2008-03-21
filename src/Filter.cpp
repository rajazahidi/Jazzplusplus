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

#include "PropertyListDialog.h"

#include "Filter.h"
#include "Events.h"
#include "ClockDialog.h"
#include "Song.h"
#include "Help.h"

#include <cstdlib>


const tFltEvent FltEvents[nFltEvents] =
{
  { StatKeyOn,          "Note",               1,     0,  127},
  { StatKeyPressure,    "Poly Aftertouch",    1,     0,  127},
  { StatControl,        "Controller",         1,     0,  127},
  { StatProgram,        "Patch",              1,     0,  127},
  { StatPitch,          "Pitch",              1, -8192, 8192},
  { StatTimeSignat,     "Meter",              1,     0,    0},
  { StatChnPressure,    "Channel Aftertouch", 1,     0,    0},
  { StatSysEx,          "SysEx",              1,     0,    0}
};



tFilter::tFilter(JZSong *s)
{
  mpSong = s;
  FltEvents = new tFltEvent [nFltEvents];
  memcpy(FltEvents, ::FltEvents, sizeof(::FltEvents));

  FromClock = 0;
  ToClock   = 120*4;
  FromTrack = 1;
  ToTrack   = 1;
  OtherSelected = 1;

  for (int i = 0; i < nFltEvents; i++)
  {
    FltEvents[i].FromValue = FltEvents[i].MinValue;
    FltEvents[i].ToValue = FltEvents[i].MaxValue;
  }
}


tFilter::tFilter(tFilter *f)
{
  copy (*f);
}


tFilter::tFilter(tFilter const &o) {
  copy(o);
}


tFilter& tFilter::operator=(tFilter const &o) {
  delete FltEvents;
  copy(o);
  return *this;
}

void tFilter::copy(tFilter const &o)
{
  mpSong        = o.mpSong;
  FromClock     = o.FromClock;
  ToClock       = o.ToClock;
  FromTrack     = o.FromTrack;
  ToTrack       = o.ToTrack;
  OtherSelected = o.OtherSelected;

  FltEvents = new tFltEvent [nFltEvents];
  memcpy(FltEvents, o.FltEvents, sizeof(::FltEvents));
}


tFilter::~tFilter()
{
  delete FltEvents;
}


// *************************************************************************
// Dialog
// *************************************************************************



class tFilterDlg : public tPropertyListDlg
{
  tFilter *Filter;
  tClockDlg FromClockDlg, ToClockDlg;

 public:
  tFilterDlg(tFilter *f, JZSong *s, int ShowEventStats);
  void AddProperties();
  bool OnClose();
  void OnHelp();
  int ShowEventStats;
};


tFilterDlg::tFilterDlg(tFilter *f, JZSong *Song, int ShowEventStats)
  : tPropertyListDlg("Filter"),
    FromClockDlg(Song, "From Time: ", f->FromClock),
    ToClockDlg(Song, "To Time: ", f->ToClock)
{
  this->ShowEventStats=ShowEventStats;
  Filter = f;
}


void tFilterDlg::AddProperties()
{
//   Add(FromClockDlg.mkFormItem(150));
//   Add(ToClockDlg.mkFormItem(150));
//   Add(wxMakeFormNewLine());
// #ifdef __WXMSW__
//   Add(wxMakeFormShort("From Track:", &Filter->FromTrack, wxFORM_DEFAULT, 0,0,0,110));
//   Add(wxMakeFormShort("To Track:", &Filter->ToTrack, wxFORM_DEFAULT, 0,0,0,110));
//   Add(wxMakeFormNewLine());
// #else
//   Add(wxMakeFormShort("From Track:", &Filter->FromTrack, wxFORM_DEFAULT));
//   Add(wxMakeFormShort("To Track:", &Filter->ToTrack, wxFORM_DEFAULT));
//   Add(wxMakeFormNewLine());
// #endif

//   if (ShowEventStats)
//   {
//     for (int i = 0; i < nFltEvents; i++)
//     {
//       if (Filter->FltEvents[i].MinValue != Filter->FltEvents[i].MaxValue)
//       {
//         Add(wxMakeFormShort("Min:", &Filter->FltEvents[i].FromValue, wxFORM_DEFAULT,0,0,0,90));
//         Add(wxMakeFormShort("Max:", &Filter->FltEvents[i].ToValue, wxFORM_DEFAULT,0,0,0,90));
//         Add(wxMakeFormBool(Filter->FltEvents[i].Name, &Filter->FltEvents[i].Selected, wxFORM_DEFAULT));
//         Add(wxMakeFormNewLine());
//       }
//       else
//         Add(wxMakeFormBool(Filter->FltEvents[i].Name, &Filter->FltEvents[i].Selected, wxFORM_DEFAULT));
//     }

//     Add(wxMakeFormBool("Other", &Filter->OtherSelected));
//   }
//   AssociatePanel(panel);
}


bool tFilterDlg::OnClose()
{
  Filter->FromClock = FromClockDlg.GetClock();
  Filter->ToClock = ToClockDlg.GetClock();
  //wxForm::OnOk();
  return FALSE;
}

void tFilterDlg::OnHelp()
{
  gpHelpInstance->ShowTopic("Filter");
}






void tFilter::Dialog(wxFrame *parent, int ShowEventStats)
{
  tFilterDlg *dlg;
  //  DialogBox = new wxDialogBox(parent, "Event Filter", FALSE );
  dlg = new tFilterDlg(this, mpSong, ShowEventStats);
  dlg->Create();
//   dlg->EditForm(DialogBox, ShowEventStats);
//   DialogBox->Fit();
//   DialogBox->Show(TRUE);
}




// --------------------------------------------------------------------------
// tTrackIterator
// --------------------------------------------------------------------------



tTrackIterator::tTrackIterator(tFilter *f, int rev)
{
  Filter = f;
  Song   = Filter->mpSong;
  Reverse = rev;
}


JZTrack *tTrackIterator::First()
{
  if (Reverse)
    TrackNr = Filter->ToTrack;
  else
    TrackNr = Filter->FromTrack;
  return Song->GetTrack(TrackNr);
}


JZTrack *tTrackIterator::Next()
{
  if (Reverse)
  {
    -- TrackNr;
    if (TrackNr < Filter->FromTrack)
      return 0;
  }
  else
  {
    ++ TrackNr;
    if (TrackNr > Filter->ToTrack)
      return 0;
  }
  return Song->GetTrack(TrackNr);
}

int tTrackIterator::Count() const {
  return Filter->ToTrack - Filter->FromTrack + 1;
}

