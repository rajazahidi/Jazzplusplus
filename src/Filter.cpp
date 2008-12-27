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

#include "Filter.h"

#include "ClockDialog.h"
#include "Events.h"
#include "Help.h"
#include "PropertyListDialog.h"
#include "Song.h"

#include <cstdlib>


const JZFilterEvent FltEvents[nFltEvents] =
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



JZFilter::JZFilter(JZSong *s)
{
  mpSong = s;
  FltEvents = new JZFilterEvent [nFltEvents];
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


JZFilter::JZFilter(JZFilter *f)
{
  copy (*f);
}


JZFilter::JZFilter(JZFilter const &o) {
  copy(o);
}


JZFilter& JZFilter::operator=(JZFilter const &o) {
  delete FltEvents;
  copy(o);
  return *this;
}

void JZFilter::copy(JZFilter const &o)
{
  mpSong        = o.mpSong;
  FromClock     = o.FromClock;
  ToClock       = o.ToClock;
  FromTrack     = o.FromTrack;
  ToTrack       = o.ToTrack;
  OtherSelected = o.OtherSelected;

  FltEvents = new JZFilterEvent [nFltEvents];
  memcpy(FltEvents, o.FltEvents, sizeof(::FltEvents));
}


JZFilter::~JZFilter()
{
  delete FltEvents;
}


// *************************************************************************
// Dialog
// *************************************************************************



class tFilterDlg : public tPropertyListDlg
{
  JZFilter *Filter;
  JZClockDialog FromClockDlg, ToClockDlg;

 public:
  tFilterDlg(JZFilter *f, JZSong *s, int ShowEventStats);
  void AddProperties();
  bool OnClose();
  void OnHelp();
  int ShowEventStats;
};


tFilterDlg::tFilterDlg(JZFilter *f, JZSong *Song, int ShowEventStats)
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






void JZFilter::Dialog(wxFrame *parent, int ShowEventStats)
{
  tFilterDlg *dlg;
  //  mpDialogBox = new wxDialogBox(parent, "Event Filter", FALSE );
  dlg = new tFilterDlg(this, mpSong, ShowEventStats);
  dlg->Create();
//   dlg->EditForm(mpDialogBox, ShowEventStats);
//   mpDialogBox->Fit();
//   mpDialogBox->Show(TRUE);
}




//*****************************************************************************
// Description:
//   This is the track iterator class definition.
//*****************************************************************************
//-----------------------------------------------------------------------------
JZTrackIterator::JZTrackIterator(JZFilter* pFilter, bool Reverse)
  : mpFilter(pFilter),
    mpSong(mpFilter->mpSong),
    mTrackIndex(0),
    mReverse(Reverse)
{
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZTrack* JZTrackIterator::First()
{
  if (mReverse)
  {
    mTrackIndex = mpFilter->ToTrack;
  }
  else
  {
    mTrackIndex = mpFilter->FromTrack;
  }
  return mpSong->GetTrack(mTrackIndex);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZTrack* JZTrackIterator::Next()
{
  if (mReverse)
  {
    --mTrackIndex;
    if (mTrackIndex < mpFilter->FromTrack)
    {
      return 0;
    }
  }
  else
  {
    ++mTrackIndex;
    if (mTrackIndex > mpFilter->ToTrack)
    {
      return 0;
    }
  }
  return mpSong->GetTrack(mTrackIndex);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int JZTrackIterator::Count() const
{
  return mpFilter->ToTrack - mpFilter->FromTrack + 1;
}
