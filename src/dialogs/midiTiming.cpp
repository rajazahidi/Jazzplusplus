/*
**  Alacrity Midi Sequencer
**
** Some Code Copyright (C) 1994-2000 Andreas Voss and Per Sigmond, all rights reserved.
**    I don't know why it says "All Rights Reserved" and then is licensed GPL
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

#include "midiTiming.h"

#ifndef __PORTING

void tTimingDlg::OkFunc( tMidiButton& button, wxCommandEvent& event )
{
  button.OnOk();
}

void tTimingDlg::CancelFunc( tMidiButton& button, wxCommandEvent& event )
{
  button.OnCancel();
}

void tTimingDlg::HelpFunc( tMidiButton& button, wxCommandEvent& event )
{
  button.OnHelp();
}

void tTimingDlg::MtcRecFunc( tMidiButton& button, wxCommandEvent& event )
{
  if ( !strcmp( button.GetLabel(), "Start" ) )
  {
    button.OnInitRec();
    button.SetLabel( "Freeze" );
  }
  else
  {
    button.OnFreezeRec();
    button.SetLabel( "Start" );
  }
}

void tTimingDlg::MtcInitRec()
{
#ifdef wx_msw
  if (Config(C_ClockSource) != CsMtc)
  {
    delete Midi;
    Config(C_ClockSource) = CsMtc;
    ClkSrcListBox->SetStringSelection( ClkSrcArray[ Config(C_ClockSource) ] );
    Midi = new tWinMtcPlayer(EventWin->Song);
    if (!Midi->Installed())
    {
      wxMessageBox("no midi driver installed", "Error", wxOK);
      Midi = new tNullPlayer(EventWin->Song);
    }
  }
#endif
  Midi->InitMtcRec();
}

void tTimingDlg::MtcFreezeRec()
{
  tMtcTime *offs = Midi->FreezeMtcRec();
  if (offs)
  {
    char str[80];
    offs->ToString( str );
    MtcTypeListBox->SetStringSelection( MtcTypeArray[ offs->type ] );
    delete offs;
    MtcOffsetEntry->SetValue( str );
  }
}

tTimingDlg::tTimingDlg(tEventWin *w)
: wxForm( USED_WXFORM_BUTTONS )
{
  EventWin = w;
  ClkSrcArray[CsInt] = "INTERNAL";
  ClkSrcArray[CsFsk] = "FSK";
  ClkSrcArray[CsMidi] = "SONG PTR";
  ClkSrcArray[CsMtc] = "MTC";
  MtcTypeArray[Mtc24] = "24 fm/sec";
  MtcTypeArray[Mtc25] = "25 fm/sec";
  MtcTypeArray[Mtc30Df] = "30 drop";
  MtcTypeArray[Mtc30Ndf] = "30 non-drop";
  ClkSrcListBox = 0;
  MtcTypeListBox = 0;
  RealTimeCheckBox = 0;
  MtcOffsetEntry = 0;
}

void tTimingDlg::CloseWindow()
{
  EventWin->DialogBox->Show( FALSE );
  delete EventWin->DialogBox;
  EventWin->DialogBox = 0;
  DELETE_THIS();
}


void tTimingDlg::OnOk()
{
  int i;
  char *str = copystring( ClkSrcListBox->GetStringSelection() );
  for (i = 0; i < 4; i++) {
	if (!strcmp(str,ClkSrcArray[i])) {
		break;
	}
  }
  delete str;
  if (i > 3)
    i = CsInt;

  if (i != Config(C_ClockSource))
  {
    Config(C_ClockSource) = (tClockSource) i;
#ifdef wx_msw
    // Re-install the midi device
    delete Midi;

    // create new player
    switch (Config(C_ClockSource))
    {
      case CsMidi:
        Midi = new tWinMidiPlayer(EventWin->Song);
        break;
      case CsMtc:
        Midi = new tWinMtcPlayer(EventWin->Song);
        break;
      case CsFsk:
      case CsInt:
      default:
	Midi = new tWinAudioPlayer(EventWin->Song);
        break;
    }
    if (!Midi->Installed())
    {
      wxMessageBox("no midi driver installed", "Error", wxOK);
      Midi = new tNullPlayer(EventWin->Song);
    }
#endif
  }

  tMtcType MtcType;
  str = copystring( MtcTypeListBox->GetStringSelection() );
  for (i = 0; i < 4; i++) {
	if (!strcmp(str,MtcTypeArray[i])) {
		MtcType = (tMtcType) i;
		break;
	}
  }
  delete str;
  if (i > 3)
    MtcType = Mtc30Ndf;
  tMtcTime *offs = new tMtcTime( MtcOffsetEntry->GetValue(), MtcType );
  EventWin->Song->GetTrack(0)->SetMtcOffset( offs );
  delete offs;

  Config(C_RealTimeOut) = RealTimeCheckBox->GetValue();
  EventWin->Redraw();
  CloseWindow();
}

void tTimingDlg::OnHelp()
{
	HelpInstance->ShowTopic("Timing");
}



void tTimingDlg::EditForm(wxPanel *panel)
{

  (void) new tMidiButton( this, panel, (wxFunction) OkFunc, "Ok" );
  (void) new tMidiButton( this, panel, (wxFunction) CancelFunc, "Cancel" );
  (void) new tMidiButton( this, panel, (wxFunction) HelpFunc, "Help" );
  panel->NewLine();

  panel->SetLabelPosition(wxVERTICAL);
  ClkSrcListBox = new wxListBox( panel,
				 NULL,
				 "Clock Source",
				 wxSINGLE|wxALWAYS_SB,
				 -1, -1, -1, -1 );

#ifdef wx_msw
  ClkSrcListBox->Append( ClkSrcArray[CsInt] );
  ClkSrcListBox->Append( ClkSrcArray[CsMidi] );
  ClkSrcListBox->Append( ClkSrcArray[CsMtc] );
#else
  int driver = Config(C_MidiDriver);
  if (driver == C_DRV_OSS || driver == C_DRV_ALSA)
  {
    ClkSrcListBox->Append( ClkSrcArray[CsInt] );
  }
  else if (driver == C_DRV_JAZZ)
  {
    ClkSrcListBox->Append( ClkSrcArray[CsInt] );
    ClkSrcListBox->Append( ClkSrcArray[CsMidi] );
    ClkSrcListBox->Append( ClkSrcArray[CsFsk] );
  }
#endif
  ClkSrcListBox->SetStringSelection( ClkSrcArray[ Config(C_ClockSource) ] );
  panel->NewLine();

  tTrack *t = EventWin->Song->GetTrack(0);
  char str[80];
  tMtcTime *offs = t->GetMtcOffset();
  offs->ToString( str );
  MtcOffsetEntry = new wxText( panel,
			       NULL,
			       "MTC offset",
			       str,
                -1,
                -1,
                100 );

  panel->NewLine();

#ifdef wx_msw
  (void) new wxMessage( panel, "Record MTC offset: " );
  (void) new tMidiButton( this, panel, (wxFunction) MtcRecFunc, "Start" );
  panel->NewLine();
#endif

  MtcTypeListBox = new wxListBox( panel,
				  NULL,
				  "MTC Type",
				  wxSINGLE|wxALWAYS_SB,
				  -1, -1, -1, -1 );
  MtcTypeListBox->Append( MtcTypeArray[0] );
  MtcTypeListBox->Append( MtcTypeArray[1] );
  MtcTypeListBox->Append( MtcTypeArray[2] );
  MtcTypeListBox->Append( MtcTypeArray[3] );
  MtcTypeListBox->SetStringSelection( MtcTypeArray[ offs->type ] );
  delete offs;
  panel->NewLine();

  RealTimeCheckBox = new wxCheckBox( panel,
				     NULL,
				     "Realtime to MIDI Out" );
  RealTimeCheckBox->SetValue( Config(C_RealTimeOut) );
  panel->NewLine();
}



#endif // Porting

