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
#pragma hdrstop

#include "gs_dlgs.h"
#include "trackwin.h"
#include "song.h"
#include "mstdfile.h"
#include "filter.h"
#include "player.h"
#include "jazz.h"

// ******************************************************************
// PAf LFO2 Dialog
// ******************************************************************

enum { 	PAfLfo2Rate = 0, PAfLfo2Pitch, PAfLfo2Tvf, PAfLfo2Tva, PAfLfo2Params };

class tPAfLfo2Dlg : public tSliderDlg
{
   public:
      tPAfLfo2Dlg(tEventWin *w);
      virtual int GetParameter( tTrack *track, int type );
      virtual void SetParameter( tTrack *track, int type, int value );

};

tPAfLfo2Dlg::tPAfLfo2Dlg(tEventWin *w) : tSliderDlg( w, tSliderDlgPAfLfo2 )
{
   NrColumns = PAfLfo2Params;
   NrRows = 16;

   ColumnLabel[PAfLfo2Rate] = "LFO2 Rate";
   ColumnLabel[PAfLfo2Pitch] = "LFO2 Pitch";
   ColumnLabel[PAfLfo2Tvf] = "LFO2 TVF";
   ColumnLabel[PAfLfo2Tva] = "LFO2 TVA";

   // valnorm, valdef, valmin, valmax:
   norm[PAfLfo2Rate].init( 65, 65, -63, 63 );
   norm[PAfLfo2Pitch].init( 1, 1, 0, 127 );
   norm[PAfLfo2Tvf].init( 1, 1, 0, 127 );
   norm[PAfLfo2Tva].init( 1, 1, 0, 127 );
}

int tPAfLfo2Dlg::GetParameter( tTrack *track, int type ) {
   return( track->GetPAfSysex( type + 7 ) );
}

void tPAfLfo2Dlg::SetParameter( tTrack *track, int type, int value ) {
   track->SetPAfSysex( type + 7, value );
}

void tTrackWin::MenPAfLfo2()
{
   tPAfLfo2Dlg *dlg;
   if (MixerForm) {
      MixerForm->Show(TRUE);
      return;
   }
   for (int i = 0; i < Song->nTracks; i++) {
      tTrack *t = Song->GetTrack(i);
      if (t && t->DialogBox) {
	 return;
      }
   }
   MixerForm = new wxDialogBox(this, "PAf LFO2", FALSE, Config(C_PartsDlgXpos), Config(C_PartsDlgYpos));
   dlg = new tPAfLfo2Dlg(this);
   dlg->EditForm(MixerForm);
   MixerForm->Fit();
   MixerForm->Show(TRUE);
}

// ******************************************************************
// CC1 Basic Dialog
// ******************************************************************

enum { 	CC1ControllerNr = 0, CC1PitchControl, CC1TvfCut, CC1Ampl, CC1BasicParams };

class tCC1BasicDlg : public tSliderDlg
{
   public:
      tCC1BasicDlg(tEventWin *w);
      virtual int GetParameter( tTrack *track, int type );
      virtual void SetParameter( tTrack *track, int type, int value );

};

tCC1BasicDlg::tCC1BasicDlg(tEventWin *w) : tSliderDlg( w, tSliderDlgCC1Basic )
{
   NrColumns = CC1BasicParams;
   NrRows = 16;

   ColumnLabel[CC1ControllerNr] = "Controller Nr";
   ColumnLabel[CC1PitchControl] = "Pitch Ctrl";
   ColumnLabel[CC1TvfCut] = "TVF Cutoff";
   ColumnLabel[CC1Ampl] = "Amplitude";

   // valnorm, valdef, valmin, valmax:
   norm[CC1ControllerNr].init( 1, 17, 0, 95 );
   norm[CC1PitchControl].init( 65, 65, -24, 24 );
   norm[CC1TvfCut].init( 65, 65, -63, 63 );
   norm[CC1Ampl].init( 65, 65, -63, 63 );
}

int tCC1BasicDlg::GetParameter( tTrack *track, int type ) {
   switch (type) {
    case CC1ControllerNr:
       return( track->GetCC1ControllerNr() );
    case CC1PitchControl:
    case CC1TvfCut:
    case CC1Ampl:
       return( track->GetCC1Sysex( type - 1 ) );
   }
   return 0;
}

void tCC1BasicDlg::SetParameter( tTrack *track, int type, int value ) {
   switch (type) {
    case CC1ControllerNr:
       track->SetCC1ControllerNr( value );
       return;
    case CC1PitchControl:
    case CC1TvfCut:
    case CC1Ampl:
       track->SetCC1Sysex( type - 1, value );
       return;
   }
}

void tTrackWin::MenCC1Basic()
{
   tCC1BasicDlg *dlg;
   if (MixerForm) {
      MixerForm->Show(TRUE);
      return;
   }
   for (int i = 0; i < Song->nTracks; i++) {
      tTrack *t = Song->GetTrack(i);
      if (t && t->DialogBox) {
	 return;
      }
   }
   MixerForm = new wxDialogBox(this, "CC1 Basic", FALSE, Config(C_PartsDlgXpos), Config(C_PartsDlgYpos));
   dlg = new tCC1BasicDlg(this);
   dlg->EditForm(MixerForm);
   MixerForm->Fit();
   MixerForm->Show(TRUE);
}


// ******************************************************************
// CC1 LFO1 Dialog
// ******************************************************************

enum { 	CC1Lfo1Rate = 0, CC1Lfo1Pitch, CC1Lfo1Tvf, CC1Lfo1Tva, CC1Lfo1Params };

class tCC1Lfo1Dlg : public tSliderDlg
{
      int xg_offset;

   public:
      tCC1Lfo1Dlg(tEventWin *w);
      virtual int GetParameter( tTrack *track, int type );
      virtual void SetParameter( tTrack *track, int type, int value );

};

tCC1Lfo1Dlg::tCC1Lfo1Dlg(tEventWin *w) : tSliderDlg( w, tSliderDlgCC1Lfo1 )
{
   if (Synth->IsXG())
   {
      xg_offset = 1;
   }
   else
   {
      xg_offset = 0;
   }

   NrColumns = CC1Lfo1Params - xg_offset;
   NrRows = 16;

   if (!Synth->IsXG())
   {
      ColumnLabel[CC1Lfo1Rate] = "LFO1 Rate";
   }

   ColumnLabel[CC1Lfo1Pitch - xg_offset] = "LFO1 Pitch";
   ColumnLabel[CC1Lfo1Tvf - xg_offset] = "LFO1 TVF";
   ColumnLabel[CC1Lfo1Tva - xg_offset] = "LFO1 TVA";

   // valnorm, valdef, valmin, valmax:
   if (!Synth->IsXG())
   {
      norm[CC1Lfo1Rate].init( 65, 65, -63, 63 );
   }

   norm[CC1Lfo1Pitch - xg_offset].init( 1, 1, 0, 127 );
   norm[CC1Lfo1Tvf - xg_offset].init( 1, 1, 0, 127 );
   norm[CC1Lfo1Tva - xg_offset].init( 1, 1, 0, 127 );
}

int tCC1Lfo1Dlg::GetParameter( tTrack *track, int type ) {
   return( track->GetCC1Sysex( type + xg_offset + 3 ) );
}

void tCC1Lfo1Dlg::SetParameter( tTrack *track, int type, int value ) {
   track->SetCC1Sysex( type + xg_offset + 3, value );
}

void tTrackWin::MenCC1Lfo1()
{
   tCC1Lfo1Dlg *dlg;
   if (MixerForm) {
      MixerForm->Show(TRUE);
      return;
   }
   for (int i = 0; i < Song->nTracks; i++) {
      tTrack *t = Song->GetTrack(i);
      if (t && t->DialogBox) {
	 return;
      }
   }
   MixerForm = new wxDialogBox(this, "CC1 LFO1", FALSE, Config(C_PartsDlgXpos), Config(C_PartsDlgYpos));
   dlg = new tCC1Lfo1Dlg(this);
   dlg->EditForm(MixerForm);
   MixerForm->Fit();
   MixerForm->Show(TRUE);
}


// ******************************************************************
// CC1 LFO2 Dialog
// ******************************************************************

enum { 	CC1Lfo2Rate = 0, CC1Lfo2Pitch, CC1Lfo2Tvf, CC1Lfo2Tva, CC1Lfo2Params };

class tCC1Lfo2Dlg : public tSliderDlg
{
   public:
      tCC1Lfo2Dlg(tEventWin *w);
      virtual int GetParameter( tTrack *track, int type );
      virtual void SetParameter( tTrack *track, int type, int value );

};

tCC1Lfo2Dlg::tCC1Lfo2Dlg(tEventWin *w) : tSliderDlg( w, tSliderDlgCC1Lfo2 )
{
   NrColumns = CC1Lfo2Params;
   NrRows = 16;

   ColumnLabel[CC1Lfo2Rate] = "LFO2 Rate";
   ColumnLabel[CC1Lfo2Pitch] = "LFO2 Pitch";
   ColumnLabel[CC1Lfo2Tvf] = "LFO2 TVF";
   ColumnLabel[CC1Lfo2Tva] = "LFO2 TVA";

   // valnorm, valdef, valmin, valmax:
   norm[CC1Lfo2Rate].init( 65, 65, -63, 63 );
   norm[CC1Lfo2Pitch].init( 1, 1, 0, 127 );
   norm[CC1Lfo2Tvf].init( 1, 1, 0, 127 );
   norm[CC1Lfo2Tva].init( 1, 1, 0, 127 );
}

int tCC1Lfo2Dlg::GetParameter( tTrack *track, int type ) {
   return( track->GetCC1Sysex( type + 7 ) );
}

void tCC1Lfo2Dlg::SetParameter( tTrack *track, int type, int value ) {
   track->SetCC1Sysex( type + 7, value );
}

void tTrackWin::MenCC1Lfo2()
{
   tCC1Lfo2Dlg *dlg;
   if (MixerForm) {
      MixerForm->Show(TRUE);
      return;
   }
   for (int i = 0; i < Song->nTracks; i++) {
      tTrack *t = Song->GetTrack(i);
      if (t && t->DialogBox) {
	 return;
      }
   }
   MixerForm = new wxDialogBox(this, "CC1 LFO2", FALSE, Config(C_PartsDlgXpos), Config(C_PartsDlgYpos));
   dlg = new tCC1Lfo2Dlg(this);
   dlg->EditForm(MixerForm);
   MixerForm->Fit();
   MixerForm->Show(TRUE);
}

// ******************************************************************
// CC2 Basic Dialog
// ******************************************************************

enum { 	CC2ControllerNr = 0, CC2PitchControl, CC2TvfCut, CC2Ampl, CC2BasicParams };

class tCC2BasicDlg : public tSliderDlg
{
   public:
      tCC2BasicDlg(tEventWin *w);
      virtual int GetParameter( tTrack *track, int type );
      virtual void SetParameter( tTrack *track, int type, int value );

};

tCC2BasicDlg::tCC2BasicDlg(tEventWin *w) : tSliderDlg( w, tSliderDlgCC2Basic )
{
   NrColumns = CC2BasicParams;
   NrRows = 16;

   ColumnLabel[CC2ControllerNr] = "Controller Nr";
   ColumnLabel[CC2PitchControl] = "Pitch Ctrl";
   ColumnLabel[CC2TvfCut] = "TVF Cutoff";
   ColumnLabel[CC2Ampl] = "Amplitude";

   // valnorm, valdef, valmin, valmax:
   norm[CC2ControllerNr].init( 1, 18, 0, 95 );
   norm[CC2PitchControl].init( 65, 65, -24, 24 );
   norm[CC2TvfCut].init( 65, 65, -63, 63 );
   norm[CC2Ampl].init( 65, 65, -63, 63 );
}

int tCC2BasicDlg::GetParameter( tTrack *track, int type ) {
   switch (type) {
    case CC2ControllerNr:
       return( track->GetCC2ControllerNr() );
    case CC2PitchControl:
    case CC2TvfCut:
    case CC2Ampl:
       return( track->GetCC2Sysex( type - 1 ) );
   }
   return 0;
}

void tCC2BasicDlg::SetParameter( tTrack *track, int type, int value ) {
   switch (type) {
    case CC2ControllerNr:
       track->SetCC2ControllerNr( value );
       return;
    case CC2PitchControl:
    case CC2TvfCut:
    case CC2Ampl:
       track->SetCC2Sysex( type - 1, value );
       return;
   }
}

void tTrackWin::MenCC2Basic()
{
   tCC2BasicDlg *dlg;
   if (MixerForm) {
      MixerForm->Show(TRUE);
      return;
   }
   for (int i = 0; i < Song->nTracks; i++) {
      tTrack *t = Song->GetTrack(i);
      if (t && t->DialogBox) {
	 return;
      }
   }
   MixerForm = new wxDialogBox(this, "CC2 Basic", FALSE, Config(C_PartsDlgXpos), Config(C_PartsDlgYpos));
   dlg = new tCC2BasicDlg(this);
   dlg->EditForm(MixerForm);
   MixerForm->Fit();
   MixerForm->Show(TRUE);
}



// ******************************************************************
// CC2 LFO1 Dialog
// ******************************************************************

enum { 	CC2Lfo1Rate = 0, CC2Lfo1Pitch, CC2Lfo1Tvf, CC2Lfo1Tva, CC2Lfo1Params };

class tCC2Lfo1Dlg : public tSliderDlg
{
      int xg_offset;

   public:
      tCC2Lfo1Dlg(tEventWin *w);
      virtual int GetParameter( tTrack *track, int type );
      virtual void SetParameter( tTrack *track, int type, int value );

};

tCC2Lfo1Dlg::tCC2Lfo1Dlg(tEventWin *w) : tSliderDlg( w, tSliderDlgCC2Lfo1 )
{
   if (Synth->IsXG())
   {
      xg_offset = 1;
   }
   else
   {
      xg_offset = 0;
   }

   NrColumns = CC2Lfo1Params - xg_offset;
   NrRows = 16;

   if (!Synth->IsXG())
   {
      ColumnLabel[CC2Lfo1Rate] = "LFO1 Rate";
   }

   ColumnLabel[CC2Lfo1Pitch - xg_offset] = "LFO1 Pitch";
   ColumnLabel[CC2Lfo1Tvf - xg_offset] = "LFO1 TVF";
   ColumnLabel[CC2Lfo1Tva - xg_offset] = "LFO1 TVA";

   // valnorm, valdef, valmin, valmax:
   if (!Synth->IsXG())
   {
      norm[CC2Lfo1Rate].init( 65, 65, -63, 63 );
   }

   norm[CC2Lfo1Pitch - xg_offset].init( 1, 1, 0, 127 );
   norm[CC2Lfo1Tvf - xg_offset].init( 1, 1, 0, 127 );
   norm[CC2Lfo1Tva - xg_offset].init( 1, 1, 0, 127 );
}

int tCC2Lfo1Dlg::GetParameter( tTrack *track, int type ) {
   return( track->GetCC2Sysex( type + xg_offset + 3 ) );
}

void tCC2Lfo1Dlg::SetParameter( tTrack *track, int type, int value ) {
   track->SetCC2Sysex( type + xg_offset + 3, value );
}

void tTrackWin::MenCC2Lfo1()
{
   tCC2Lfo1Dlg *dlg;
   if (MixerForm) {
      MixerForm->Show(TRUE);
      return;
   }
   for (int i = 0; i < Song->nTracks; i++) {
      tTrack *t = Song->GetTrack(i);
      if (t && t->DialogBox) {
	 return;
      }
   }
   MixerForm = new wxDialogBox(this, "CC2 LFO1", FALSE, Config(C_PartsDlgXpos), Config(C_PartsDlgYpos));
   dlg = new tCC2Lfo1Dlg(this);
   dlg->EditForm(MixerForm);
   MixerForm->Fit();
   MixerForm->Show(TRUE);
}

// ******************************************************************
// CC2 LFO2 Dialog
// ******************************************************************

enum { 	CC2Lfo2Rate = 0, CC2Lfo2Pitch, CC2Lfo2Tvf, CC2Lfo2Tva, CC2Lfo2Params };

class tCC2Lfo2Dlg : public tSliderDlg
{
   public:
      tCC2Lfo2Dlg(tEventWin *w);
      virtual int GetParameter( tTrack *track, int type );
      virtual void SetParameter( tTrack *track, int type, int value );

};

tCC2Lfo2Dlg::tCC2Lfo2Dlg(tEventWin *w) : tSliderDlg( w, tSliderDlgCC2Lfo2 )
{
   NrColumns = CC2Lfo2Params;
   NrRows = 16;

   ColumnLabel[CC2Lfo2Rate] = "LFO2 Rate";
   ColumnLabel[CC2Lfo2Pitch] = "LFO2 Pitch";
   ColumnLabel[CC2Lfo2Tvf] = "LFO2 TVF";
   ColumnLabel[CC2Lfo2Tva] = "LFO2 TVA";

   // valnorm, valdef, valmin, valmax:
   norm[CC2Lfo2Rate].init( 65, 65, -63, 63 );
   norm[CC2Lfo2Pitch].init( 1, 1, 0, 127 );
   norm[CC2Lfo2Tvf].init( 1, 1, 0, 127 );
   norm[CC2Lfo2Tva].init( 1, 1, 0, 127 );
}

int tCC2Lfo2Dlg::GetParameter( tTrack *track, int type ) {
   return( track->GetCC2Sysex( type + 7 ) );
}

void tCC2Lfo2Dlg::SetParameter( tTrack *track, int type, int value ) {
   track->SetCC2Sysex( type + 7, value );
}

void tTrackWin::MenCC2Lfo2()
{
   tCC2Lfo2Dlg *dlg;
   if (MixerForm) {
      MixerForm->Show(TRUE);
      return;
   }
   for (int i = 0; i < Song->nTracks; i++) {
      tTrack *t = Song->GetTrack(i);
      if (t && t->DialogBox) {
	 return;
      }
   }
   MixerForm = new wxDialogBox(this, "CC2 LFO2", FALSE, Config(C_PartsDlgXpos), Config(C_PartsDlgYpos));
   dlg = new tCC2Lfo2Dlg(this);
   dlg->EditForm(MixerForm);
   MixerForm->Fit();
   MixerForm->Show(TRUE);
}

// ******************************************************************
// Drum Instrument Parameter Dialog
// ******************************************************************

class tDrumListBox;
class tDrumButton;

class tDrumParamDlg : public tSliderDlg
{
   public:
      tDrumParamDlg(tEventWin *w);
      virtual int GetParameter( tTrack *track, int type );
      virtual void SetParameter( tTrack *track, int type, int value );
      virtual void EditForm(wxPanel *);

      int pitch;
      tTrack *drumTrack;
      int drumchan;
      SliderWinNorm drumNorm[numDrumParameters];
      tGsSlider *slider[numDrumParameters];
      tDrumListBox *instrument_list;
      tGsCheckBox *random_pan_checkbox;
      int numInstr;
      enum { maxInstr = 60 };

      char *index2Name[130];
      int index2Pitch[130];
      int pitch2Index[130];
      int numNames;
      int instr2Pitch[maxInstr];
      char *sliderlabel[numDrumParameters];

      void AddInstrument();
      void DelInstrument();
      void SelectInstr();

      static void Select(tDrumListBox& list, wxCommandEvent& event);
      static void AddFunc(tDrumButton &but, wxCommandEvent& event);
      static void DelFunc(tDrumButton &but, wxCommandEvent& event);
      static void CheckChange( tGsCheckBox& box, wxCommandEvent& event );
};

class tDrumListBox : public wxListBox
{
   public:
      tDrumListBox( tDrumParamDlg *dlg,
		    wxPanel *panel, wxFunction func, char *label,
		    Bool Multiple = wxSINGLE,
		    int x = -1, int y = -1, int width = -1, int height = -1,
		    int N = 0, char **Choices = NULL,
		    long style = 0, char *name = "listBox" )
	 : wxListBox( panel, func, label, Multiple, x, y, width, height,
		      N, Choices, style, name )
      {
	 dialog = dlg;
      }

      void OnSelect()
      {
	 dialog->SelectInstr();
      }

   private:
      tDrumParamDlg *dialog;
};

class tDrumButton : public tGsButton {
   public:
      tDrumButton( tDrumParamDlg *dlg,
		   wxPanel *panel, wxFunction func, char *label, int x = -1, int y = -1,
		   int width = -1, int height = -1, long style = 0, char *name = "button")
	 : tGsButton( dlg, panel, func, label, x, y, width, height, style, name )
      {
	 drumdlg = dlg;
      }

      void OnAdd()
      {
	 drumdlg->AddInstrument();
      }

      void OnDel()
      {
	 drumdlg->DelInstrument();
      }

   private:
      tDrumParamDlg *drumdlg;
};

tDrumParamDlg::tDrumParamDlg(tEventWin *w) : tSliderDlg( w, tSliderDlgDrumParam )
{
   // valnorm, valdef, valmin, valmax:
   drumNorm[drumPitchIndex].init( 65, 65, -63, 63 );
   drumNorm[drumTvaIndex].init( 0, 128, 1, 128 );
   drumNorm[drumPanIndex].init( 65, 65, -63, 63 );
   drumNorm[drumReverbIndex].init( 0, 100, 1, 128 );
   drumNorm[drumChorusIndex].init( 0, 100, 1, 128 );

   sliderlabel[drumPitchIndex] =  " Pitch ";
   sliderlabel[drumTvaIndex] =    " TVA   ";
   sliderlabel[drumPanIndex] =    " Pan   ";
   sliderlabel[drumReverbIndex] = " Reverb";
   sliderlabel[drumChorusIndex] = " Chorus";

   drumchan = Config(C_DrumChannel) - 1;
   instrument_list = 0;
   numInstr = 0;

   numNames = 0;
   for (int i = 0; Config.DrumName(i).Name; i++)
   {
      if (Config.DrumName(i).Name[0])
      {
	 index2Pitch[numNames] = Config.DrumName(i).Value - 1;
	 pitch2Index[Config.DrumName(i).Value - 1] = numNames;
	 index2Name[numNames++] = Config.DrumName(i).Name;
      }
   }
}

int tDrumParamDlg::GetParameter( tTrack *track, int type ) {
   if (numInstr > 0) {
      return( track->GetDrumParam( pitch, type ) );
   }
   return 0;
}

void tDrumParamDlg::SetParameter( tTrack *track, int type, int value ) {
   if (numInstr > 0) {
      if (type == drumPanIndex) {
	 if (random_pan_checkbox->GetValue()) {
	    // checked means random pan selected
	    track->SetDrumParam( pitch, type, 1 );
	    slider[type]->SetValue(0);
	    return;
	 }
      }
      track->SetDrumParam( pitch, type, value );
   }
}

void tDrumParamDlg::SelectInstr()
{

   if (numInstr <= 0)
      return;

   int instr = instrument_list->GetSelection();
   if (instr < 0)
      return;

   pitch = instr2Pitch[ instr ];

   int val[numDrumParameters];
   for (int i = 0; i < numDrumParameters; i++) {
      val[i] = GetParameter( drumTrack, i );
      if (!val[i]) {
	 val[i] = drumNorm[i].valdef;
	 SetParameter( drumTrack, i, val[i] );
      }
      if (i == drumPanIndex) {
	 if (val[i] == 1) {
	    // value == 1 means random pan
	    random_pan_checkbox->SetValue( TRUE );
	    slider[i]->SetValue(0);
	 }
	 else {
	    random_pan_checkbox->SetValue( FALSE );
	    slider[i]->SetNormValue( val[i] );
	 }
      }
      else {
	 slider[i]->SetNormValue( val[i] );
      }

   }
}

void tDrumParamDlg::Select(tDrumListBox& list, wxCommandEvent& event)
{
   list.OnSelect();
}

void tDrumParamDlg::AddFunc(tDrumButton &but, wxCommandEvent& event)
{
   but.OnAdd();
}

void tDrumParamDlg::AddInstrument()
{
   if (numInstr >= maxInstr)
      return;

   int index = wxGetSingleChoiceIndex("Instrument", "Select an instrument", numNames, index2Name );
   if (index >= 0)
   {
      int pit = index2Pitch[ index ];
      int found = 0;
      for (int i = 0; i < numInstr; i++)
      {
	 if (instr2Pitch[i] == pit)
	 {
	    // Already have it
	    instrument_list->SetSelection( i );
	    found = 1;
	    break;
	 }
      }
      if (!found)
      {
	 instrument_list->Append( index2Name[ index ] );
	 numInstr++;
	 instr2Pitch[ numInstr - 1 ] = pit;
	 instrument_list->SetSelection( numInstr - 1 );
      }
      instrument_list->OnSelect();
   }
}

void tDrumParamDlg::DelFunc(tDrumButton &but, wxCommandEvent& event)
{
   but.OnDel();
}

void tDrumParamDlg::CheckChange( tGsCheckBox& box, wxCommandEvent& event ) {
   box.OnCheck();
}

void tDrumParamDlg::DelInstrument()
{
   if (numInstr <= 0)
      return;

   int instr = instrument_list->GetSelection();
   pitch = instr2Pitch[ instr ];
   drumTrack->DrumParams.DelElem( pitch );

   instrument_list->Delete( instr );
   numInstr--;
   for (int i = instr; i < numInstr; i++)
   {
      instr2Pitch[ i ] = instr2Pitch[ i + 1 ];
   }
   if (instr >= numInstr)
   {
      instr = numInstr-1;
   }
   if (numInstr > 0)
   {
      instrument_list->SetSelection( instr );
   }
   instrument_list->OnSelect();
}


void tDrumParamDlg::EditForm(wxPanel *panel)
{
#ifdef wx_motif
   const int slider_width = SLIDER_WIDTH * 3 / 2;
#else
   const int slider_width = SLIDER_WIDTH;
#endif
   const int listbox_width = 220;
#ifdef wx_motif
   const int listbox_height = 120;
#else
   const int listbox_height = 100;
#endif

   int i;

   int first_track = 0, last_track = 0;
   for ( i = 0; i < EventWin->Song->nTracks; i++ ) {
      if ( !EventWin->Song->GetTrack(i)->IsEmpty() ) {
	 first_track = i;
	 break;
      }
   }
   for ( i = EventWin->Song->nTracks - 1; i >= first_track; i-- ) {
      if ( !EventWin->Song->GetTrack(i)->IsEmpty() ) {
	 last_track = i;
	 break;
      }
   }


   drumTrack = NULL;
   for (int j = first_track; j <= last_track; j++) {
      tTrack *t = EventWin->Song->GetTrack(j);
      if (t->IsEmpty()) continue;
      if (t && (t->Channel == (drumchan + 1)) ) {
	 if (t->GetName() && strlen(t->GetName())) {
	    drumTrack = t;
	    break;
	 }
      }
   }

   (void)new tDrumButton( 	this, panel, (wxFunction) Dismiss, "Dismiss" );
   (void) new tGsButton( this, panel, (wxFunction) Help, "Help" );
   panel->NewLine();

   char labelstr[60];

   if (drumTrack) {
      sprintf( labelstr, "Drum Track: %s (channel %d)", drumTrack->GetName(), drumchan+1 );
      (void) new wxMessage( panel, labelstr );
      panel->NewLine();
      panel->SetLabelPosition(wxVERTICAL);
      instrument_list = new tDrumListBox( this, panel,
					  (wxFunction)Select,
					  "Drum Instrument",
					  wxSINGLE|wxALWAYS_SB,
					  -1,
					  -1,
					  listbox_width,
					  listbox_height);
      panel->NewLine();

      tDrumInstrumentParameter *dpar = drumTrack->DrumParams.FirstElem();
      while ( dpar ) {
	 pitch = dpar->Pitch();
	 int index = pitch2Index[ pitch ];

	 instrument_list->Append( index2Name[ index ] );
	 numInstr++;
	 instr2Pitch[ numInstr - 1 ] = index2Pitch[ index ];

	 dpar = drumTrack->DrumParams.NextElem( dpar );
      }

      (void) new wxMessage( 	panel, "Instrument list: " );
      (void)new tDrumButton(	this, panel, (wxFunction)AddFunc, "Add");
      (void)new tDrumButton(	this, panel, (wxFunction)DelFunc, "Del");
      panel->NewLine();

      for ( i = 0; i < numDrumParameters; i++ ) {
	 panel->SetLabelPosition(wxHORIZONTAL);
	 slider[i] =
	    new tGsSlider( 	this, drumTrack, drumNorm[i],
				i,
				panel, (wxFunction) SliderChange,
				sliderlabel[i],
				drumNorm[i].valdef-drumNorm[i].valnorm,
				drumNorm[i].valmin, drumNorm[i].valmax,
				slider_width );
	 if (i == drumPanIndex) {
	    panel->NewLine();
	    random_pan_checkbox =
	       new tGsCheckBox(this,
			       drumTrack,
			       drumNorm[i],
			       i,
			       FALSE,
			       panel,
			       (wxFunction) CheckChange,
			       "Random Pan"
		  );
	 }
	 panel->NewLine();
      }


      if (numInstr > 0)
      {
	 instrument_list->SetSelection( 0 );
	 instrument_list->OnSelect();
      }
   }
   else {
      sprintf( labelstr, "No drum track defined (channel %d)", drumchan + 1 );
      wxMessageBox( labelstr, "User error", wxOK);
      CloseWindow();
   }

}





void tTrackWin::MenDrumParam()
{
   tDrumParamDlg *dlg;
   if (MixerForm) {
      MixerForm->Show(TRUE);
      return;
   }
   for (int i = 0; i < Song->nTracks; i++) {
      tTrack *t = Song->GetTrack(i);
      if (t && t->DialogBox) {
	 return;
      }
   }
   MixerForm = new wxDialogBox(this, "Drum Instrument Parameters", FALSE, Config(C_PartsDlgXpos), Config(C_PartsDlgYpos));
   dlg = new tDrumParamDlg(this);
   dlg->EditForm(MixerForm);
   if (MixerForm)
   {
      MixerForm->Fit();
      MixerForm->Show(TRUE);
   }
}

// ******************************************************************
// Partial Reserve Dialog
// ******************************************************************

class tPartRsrvDlg : public tSliderDlg
{
   public:
      tPartRsrvDlg(tEventWin *w);
      virtual int GetParameter( tTrack *track, int type );
      virtual void SetParameter( tTrack *track, int type, int value );
      void EditForm(wxPanel *panel);
      SliderWinNorm rsrvnorm[17];
      uchar val[17];

      static void Ok( tGsButton& button, wxCommandEvent& event );
      static void Cancel( tGsButton& button, wxCommandEvent& event );

};


tPartRsrvDlg::tPartRsrvDlg(tEventWin *w) : tSliderDlg( w, tSliderDlgPartRsrv )
{
   // valnorm, valdef, valmin, valmax:
   for (int i = 0; i < 16; i++) {
      rsrvnorm[i].valnorm = 1;
      rsrvnorm[i].valmin = 0;
      rsrvnorm[i].valmax = 24;
      val[i] = 0;
   }
   rsrvnorm[16].valnorm = 1;
   rsrvnorm[16].valmin = 24;
   rsrvnorm[16].valmax = 128;

   rsrvnorm[0].valdef = 3; // CH. 10
   rsrvnorm[1].valdef = 7; // CH. 1
   rsrvnorm[2].valdef = 3; // CH. 2
   rsrvnorm[3].valdef = 3; // CH. 3
   rsrvnorm[4].valdef = 3; // CH. 4
   rsrvnorm[5].valdef = 3; // CH. 5
   rsrvnorm[6].valdef = 3; // CH. 6
   rsrvnorm[7].valdef = 3; // CH. 7
   rsrvnorm[8].valdef = 3; // CH. 8
   rsrvnorm[9].valdef = 3; // CH. 9
   rsrvnorm[10].valdef = 1; // CH. 11
   rsrvnorm[11].valdef = 1; // CH. 12
   rsrvnorm[12].valdef = 1; // CH. 13
   rsrvnorm[13].valdef = 1; // CH. 14
   rsrvnorm[14].valdef = 1; // CH. 15
   rsrvnorm[15].valdef = 1; // CH. 16
   rsrvnorm[16].valdef = 25; // Sum
}

int tPartRsrvDlg::GetParameter( tTrack *track, int type ) {
   // Here "type" means channel (1-16)
   return( track->GetPartRsrv( type ) );
}

void tPartRsrvDlg::SetParameter( tTrack *track, int type, int value ) {
   // Here "type" means slider number (0-17)
   val[type] = value;
}

void tPartRsrvDlg::Ok( tGsButton& button, wxCommandEvent& event )
{
   uchar sum = 0;
   int i;
   tPartRsrvDlg* dialog = (tPartRsrvDlg*) button.getDialog();
   uchar *val = dialog->val;
   SliderWinNorm *rsrvnorm = dialog->rsrvnorm;

   for (i = 0; i < 16; i++) {
      sum += val[i] - rsrvnorm[i].valnorm;
   }
   if (sum > (val[16] - rsrvnorm[16].valnorm)) {
      wxMessageBox("Too many voices, max. polyphony exceeded", "Error", wxOK);
      return;
   }

   for (i = 0; i < 16; i++) {
      if (val[i] > 0) val[i] = val[i] - rsrvnorm[i].valnorm;
   }

   tTrackWin* tw = (tTrackWin*) dialog->EventWin;
   tTrack *t = tw->Song->GetTrack(0);
   t->SetPartRsrv( val );

   tw->MixerForm->Show(FALSE);
   delete tw->MixerForm;
   tw->MixerForm = 0;
}

void tPartRsrvDlg::Cancel( tGsButton& button, wxCommandEvent& event )
{
   tPartRsrvDlg* dialog = (tPartRsrvDlg*) button.getDialog();
   tTrackWin* tw = (tTrackWin*) dialog->EventWin;

   tw->MixerForm->Show(FALSE);
   delete tw->MixerForm;
   tw->MixerForm = 0;
}

void tPartRsrvDlg::EditForm(wxPanel *panel)
{
   const int slider_width = SLIDER_WIDTH;
   const int ypos_buttons = 5;
   const int ypos_text = ypos_buttons + SLIDER_VERTICAL_SPACE;
   const int xpos_label = 5;
   const int xstart_sliders = XSTART_SLIDERS;
   const int xstart_text = XSTART_TEXT;
   const int xcol_interval = XCOL_INTERVAL;
   const int ystart_sliders = ypos_text + SLIDER_VERTICAL_SPACE;
   const int yline_interval = SLIDER_VERTICAL_SPACE;
   const int slid_vert_offs = SLIDER_VERTICAL_OFFSET;

   (void) new tGsButton( this, panel, (wxFunction) Ok, "Ok" );
   (void) new tGsButton( this, panel, (wxFunction) Cancel, "Cancel" );
   (void) new tGsButton( this, panel, (wxFunction) Help, "Help" );

   (void) new wxMessage( panel, "Partial Reserve", xstart_text, ypos_text);

   tTrack *track;
   tTrack *track_one = EventWin->Song->GetTrack(0);

   int i;
   int first_track = 0, last_track = 0;
   for ( i = 0; i < EventWin->Song->nTracks; i++ ) {
      if ( !EventWin->Song->GetTrack(i)->IsEmpty() ) {
	 first_track = i;
	 break;
      }
   }
   for ( i = EventWin->Song->nTracks - 1; i >= first_track; i-- ) {
      if ( !EventWin->Song->GetTrack(i)->IsEmpty() ) {
	 last_track = i;
	 break;
      }
   }

   for (int line = 0; line < 16; line++) {
      char labelstr[40];

      int chan;

      if (line == 0)
	 chan = 9;
      else if (line < 10)
	 chan = line - 1;
      else chan = line;

      track = NULL;
      for (int j = first_track; j <= last_track; j++) {
	 tTrack *t = EventWin->Song->GetTrack(j);
	 if (t && (t->Channel == (chan + 1)) ) {
	    if (t->GetName() && strlen(t->GetName())) {
	       track = t;
	       break;
	    }
	 }
      }

      if (track) {
	 sprintf( labelstr, "%d %s", chan + 1, track->GetName() );

      }
      else {
	 sprintf( labelstr, "%d", chan + 1 );
      }

      val[line] = GetParameter( track_one, chan + 1 );
      if (!val[line]) {
	 val[line] = rsrvnorm[line].valdef;
      }


      (void) new tGsSlider( 	this, track_one, rsrvnorm[line], line,
				panel, (wxFunction) SliderChange, " ",
				val[line] - rsrvnorm[line].valnorm,
				rsrvnorm[line].valmin, rsrvnorm[line].valmax,
				slider_width, xstart_sliders,
				ystart_sliders + line*yline_interval + slid_vert_offs );


      (void) new wxMessage( panel, labelstr, xpos_label, ystart_sliders + line*yline_interval);
   }

   val[16] = rsrvnorm[16].valdef;

   (void) new wxMessage( panel, "Max. polyphony", xpos_label, ystart_sliders + 17*yline_interval);
   (void) new tGsSlider( 	this, track_one, rsrvnorm[16], 16,
				panel, (wxFunction) SliderChange, " ",
				val[16] - rsrvnorm[16].valnorm,
				rsrvnorm[16].valmin, rsrvnorm[16].valmax,
				slider_width, xstart_sliders,
				ystart_sliders + 17*yline_interval + slid_vert_offs );

}


void tTrackWin::MenPartRsrv()
{
   tPartRsrvDlg *dlg;
   if (MixerForm) {
      MixerForm->Show(TRUE);
      return;
   }
   for (int i = 0; i < Song->nTracks; i++) {
      tTrack *t = Song->GetTrack(i);
      if (t && t->DialogBox) {
	 return;
      }
   }
   MixerForm = new wxDialogBox(this, "Partial Reserve", FALSE, Config(C_PartsDlgXpos), Config(C_PartsDlgYpos));
   dlg = new tPartRsrvDlg(this);
   dlg->EditForm(MixerForm);
   MixerForm->Fit();
   MixerForm->Show(TRUE);
}

// ******************************************************************
// Part Mode Dialog
// ******************************************************************

enum { RxChannel = 0x02, RxCAf = 0x04, RxPAf = 0x07, UseForRhythm = 0x15 };

class tPartModeDlg : public tSliderDlg
{
   public:
      tPartModeDlg(tEventWin *w);
      virtual int GetParameter( tTrack *track, int type );
      virtual void SetParameter( tTrack *track, int type, int value );
      void EditForm(wxPanel *panel);
};

tPartModeDlg::tPartModeDlg(tEventWin *w) : tSliderDlg( w, tSliderDlgPartMode )
{
   NrColumns = 2;
   NrRows = 16;

   ColumnLabel[0] = "Rx Ch (17=off)";
   ColumnLabel[1] = "Use For Rhythm";

   // valnorm, valdef, valmin, valmax:
   norm[0].init( 0, 0, 1, 17 ); // Rx channel (valdef not used)

   if (Synth->IsXG())
   {
      norm[1].init( 1, 1, 0, 5 ); // Use for rythm (valdef not used)
   }
   else
   {
      norm[1].init( 1, 1, 0, 2 ); // Use for rythm (valdef not used)
   }
}

int tPartModeDlg::GetParameter( tTrack *track, int type ) {
   int val;

   switch (type) {
    case 0:
       val = track->GetModeSysex( RxChannel );
       if (val > 17)
       {
	  return 17;
       }
       else
       {
	  return val;
       }

    case 1:
       return( track->GetModeSysex( UseForRhythm ) );
   }
   return(0);
}

void tPartModeDlg::SetParameter( tTrack *track, int type, int value ) {
   switch (type) {
    case 0:
       if (Synth->IsXG() && (value == 17))
       {
	  value = 128; // 127 means "off" in XG
       }
       track->SetModeSysex( RxChannel, value );
       break;
    case 1:
       track->SetModeSysex( UseForRhythm, value );
       break;
   }
   return;
}

void tPartModeDlg::EditForm(wxPanel *panel)
{
   const int slider_width = SLIDER_WIDTH;
   const int ypos_buttons = 5;
   const int ypos_text = ypos_buttons + SLIDER_VERTICAL_SPACE;
   const int xpos_label = 5;
   const int xstart_sliders = XSTART_SLIDERS;
   const int xstart_text = XSTART_TEXT;
   const int xcol_interval = XCOL_INTERVAL;
   const int ystart_sliders = ypos_text + SLIDER_VERTICAL_SPACE;
   const int yline_interval = SLIDER_VERTICAL_SPACE;
   const int slid_vert_offs = SLIDER_VERTICAL_OFFSET;

   (void) new tGsButton( this, panel, (wxFunction) Dismiss, "Dismiss" );
   (void) new tGsButton( this, panel, (wxFunction) Help, "Help" );

   int i;

   for ( i = 0; i < NrColumns; i++ ) {
      (void) new wxMessage( panel, ColumnLabel[i], xstart_text + (i*xcol_interval), ypos_text);
   }

   tTrack *track;

   int first_track = 0, last_track = 0;
   for ( i = 0; i < EventWin->Song->nTracks; i++ ) {
      if ( !EventWin->Song->GetTrack(i)->IsEmpty() ) {
	 first_track = i;
	 break;
      }
   }
   for ( i = EventWin->Song->nTracks - 1; i >= first_track; i-- ) {
      if ( !EventWin->Song->GetTrack(i)->IsEmpty() ) {
	 last_track = i;
	 break;
      }
   }


   int chan;
   for (chan = 0; chan < NrRows; chan++) {
      char labelstr[40];

      track = NULL;
      for (int j = first_track; j <= last_track; j++) {
	 tTrack *t = EventWin->Song->GetTrack(j);
	 if (t && (t->Channel == (chan + 1)) ) {
	    if (t->GetName() && strlen(t->GetName())) {
	       track = t;
	       break;
	    }
	 }
      }


      if (track) {
	 int val[2];
	 sprintf( labelstr, "%d %s", chan + 1, track->GetName() );

	 val[0] = GetParameter( track, 0 );
	 if (!val[0]) {
	    val[0] = chan + 1;
	    SetParameter( track, 0, val[0] );
	 }
	 val[1] = GetParameter( track, 1 );
	 if (!val[1]) {
	    if (chan == 9)
	       val[1] = 2;
	    else
	       val[1] = 1;
				// Don't do SetParameter here; resets the part
				// (set it only if slider changes)
	 }

	 for ( i = 0; i < NrColumns; i++ ) {
	    (void) new tGsSlider( 	this, track, norm[i], i,
					panel, (wxFunction) SliderChange, " ",
					val[i] - norm[i].valnorm,
					norm[i].valmin, norm[i].valmax,
					slider_width, xstart_sliders + (i*xcol_interval),
					ystart_sliders + chan*yline_interval + slid_vert_offs );
	 }
      }
      else {
	 sprintf( labelstr, "%d", chan + 1 );
      }

      (void) new wxMessage( panel, labelstr, xpos_label, ystart_sliders + chan*yline_interval);
      // Draw tracknames on right side too?
      if (Config(C_PartsTracknamesRight)) {
	 (void) new wxMessage( panel, labelstr, xstart_sliders + NrColumns*xcol_interval + 10, ystart_sliders + chan*yline_interval);
      }
   }

}

void tTrackWin::MenPartMode()
{
   tPartModeDlg *dlg;
   if (MixerForm) {
      MixerForm->Show(TRUE);
      return;
   }
   for (int i = 0; i < Song->nTracks; i++) {
      tTrack *t = Song->GetTrack(i);
      if (t && t->DialogBox) {
	 return;
      }
   }
   MixerForm = new wxDialogBox(this, "Part Mode", FALSE, Config(C_PartsDlgXpos), Config(C_PartsDlgYpos));
   dlg = new tPartModeDlg(this);
   dlg->EditForm(MixerForm);
   MixerForm->Fit();
   MixerForm->Show(TRUE);
}

// ******************************************************************
// GS Effects Dialog
// ******************************************************************

enum { 	RevCharacter = 0, RevPreLpf, RevLevel, RevTime, RevDelayFeedback, RevSendChorus,
 	ChoPreLpf, ChoLevel, ChoFeedback, ChoDelay, ChoRate, ChoDepth, ChoSendReverb,
	GSRevMacro, GSChoMacro, UseRevMacro, UseChoMacro, GSEffectParams
};

class tGSEffectsDlg : public tSliderDlg
{
   public:
      tGSEffectsDlg(tEventWin *w);
      virtual int GetParameter( tTrack *track, int type );
      virtual void SetParameter( tTrack *track, int type, int value );
      virtual void EditForm(wxPanel *panel);
      virtual void CloseWindow();

      SliderWinNorm effectnorm[GSEffectParams];
      char *RevSelArray[8];
      char *ChoSelArray[8];

      static void SelectionChange( tGsListBox& box, wxCommandEvent& event );
      static void CheckChange( tGsCheckBox& box, wxCommandEvent& event );

};

tGSEffectsDlg::tGSEffectsDlg(tEventWin *w) : tSliderDlg( w, tSliderDlgEffects )
{
   RevSelArray[0] = "Room 1";
   RevSelArray[1] = "Room 2";
   RevSelArray[2] = "Room 3";
   RevSelArray[3] = "Hall 1";
   RevSelArray[4] = "Hall 2";
   RevSelArray[5] = "Plate";
   RevSelArray[6] = "Delay";
   RevSelArray[7] = "Panning Delay";

   ChoSelArray[0] = "Chorus 1";
   ChoSelArray[1] = "Chorus 2";
   ChoSelArray[2] = "Chorus 3";
   ChoSelArray[3] = "Chorus 4";
   ChoSelArray[4] = "Feedback Chorus";
   ChoSelArray[5] = "Flanger";
   ChoSelArray[6] = "Short Delay";
   ChoSelArray[7] = "Short Delay FB";

   // valnorm, valdef, valmin, valmax:
   effectnorm[RevCharacter].init( 1, 5, 0, 7 );
   effectnorm[RevPreLpf].init( 1, 1, 0, 7 );
   effectnorm[RevLevel].init( 1, 65, 0, 127 );
   effectnorm[RevTime].init( 1, 65, 0, 127 );
   effectnorm[RevDelayFeedback].init( 1, 1, 0, 127 );
   effectnorm[RevSendChorus].init( 1, 1, 0, 127 );
   effectnorm[ChoPreLpf].init( 1, 1, 0, 7 );
   effectnorm[ChoLevel].init( 1, 65, 0, 127 );
   effectnorm[ChoFeedback].init( 1, 9, 0, 127 );
   effectnorm[ChoDelay].init( 1, 81, 0, 127 );
   effectnorm[ChoRate].init( 1, 4, 0, 127 );
   effectnorm[ChoDepth].init( 1, 19, 0, 127 );
   effectnorm[ChoSendReverb].init( 1, 1, 0, 127 );
   effectnorm[GSRevMacro].init( 1, 5, 0, 7 );
   effectnorm[GSChoMacro].init( 1, 3, 0, 7 );
   effectnorm[UseRevMacro].init( 1, 2, 0, 1 );
   effectnorm[UseChoMacro].init( 1, 2, 0, 1 );
}

void tGSEffectsDlg::CloseWindow()
{
   EventWin->DialogBox->GetPosition( &Config(C_PartsDlgXpos), &Config(C_PartsDlgYpos) );
   EventWin->DialogBox->Show(FALSE);
   delete EventWin->DialogBox;
   EventWin->DialogBox = 0;
   DELETE_THIS();
}

int tGSEffectsDlg::GetParameter( tTrack *track, int type ) {
   switch (type) {
    case RevCharacter:
    case RevPreLpf:
    case RevLevel:
    case RevTime:
    case RevDelayFeedback:
    case RevSendChorus:
       return( track->GetRevSysex( type ) );
    case ChoPreLpf:
    case ChoLevel:
    case ChoFeedback:
    case ChoDelay:
    case ChoRate:
    case ChoDepth:
    case ChoSendReverb:
       return( track->GetChoSysex( type - 6 ) );
    case GSRevMacro:
       return( track->GetReverbType() );
    case GSChoMacro:
       return( track->GetChorusType() );
    case UseRevMacro:
       return( Config(C_UseReverbMacro) + 1 );
    case UseChoMacro:
       return( Config(C_UseChorusMacro) + 1 );
   }
   return( 0 );
}

void tGSEffectsDlg::SetParameter( tTrack *track, int type, int value ) {
   switch (type) {
    case RevCharacter:
    case RevPreLpf:
    case RevLevel:
    case RevTime:
    case RevDelayFeedback:
    case RevSendChorus:
       track->SetRevSysex( type, value );
       break;
    case ChoPreLpf:
    case ChoLevel:
    case ChoFeedback:
    case ChoDelay:
    case ChoRate:
    case ChoDepth:
    case ChoSendReverb:
       track->SetChoSysex( type - 6, value );
       break;
    case GSRevMacro:
       track->SetReverbType( value );
       break;
    case GSChoMacro:
       track->SetChorusType( value );
       break;
    case UseRevMacro:
       Config(C_UseReverbMacro) = value - 1;
       break;
    case UseChoMacro:
       Config(C_UseChorusMacro) = value - 1;
       break;
   }
   return;
}

void tGSEffectsDlg::SelectionChange( tGsListBox& box, wxCommandEvent& event ) {
   box.OnSelect();
}

void tGSEffectsDlg::CheckChange( tGsCheckBox& box, wxCommandEvent& event ) {
   box.OnCheck();
}

void tGSEffectsDlg::EditForm(wxPanel *panel)
{
   const int slider_width = SLIDER_WIDTH;
   const int xpos_buttons = 5;
   const int ypos_buttons = 10;
   const int ypos_header = ypos_buttons + SLIDER_VERTICAL_SPACE;
   const int xpos_rev_label = 25;
   const int xpos_rev_sliders = xpos_rev_label + 100;
#ifdef wx_motif
   const int xpos_cho_label = xpos_rev_sliders + (2 * slider_width);
#else
#ifdef wx_xview
   const int xpos_cho_label = xpos_rev_sliders + (4 * slider_width);
#else
   const int xpos_cho_label = xpos_rev_sliders + 230;
#endif
#endif
   const int xpos_cho_sliders = xpos_cho_label + 100;
   const int xcol_interval = XCOL_INTERVAL;

   const int ystart_sliders = ypos_header + SLIDER_VERTICAL_SPACE;
   const int yline_interval = SLIDER_VERTICAL_SPACE;
   const int slid_vert_offs = SLIDER_VERTICAL_OFFSET;

   (void) new tGsButton( this, panel, (wxFunction) Dismiss, "Dismiss" );
   (void) new tGsButton( this, panel, (wxFunction) Help, "Help" );

   (void) new wxMessage( panel, "Reverb Parameters", xpos_rev_sliders, ypos_header);
   (void) new wxMessage( panel, "Chorus Parameters", xpos_cho_sliders, ypos_header);

   (void) new wxMessage( panel, "Character", xpos_rev_label, ystart_sliders);
   (void) new wxMessage( panel, "Pre Lpf", xpos_rev_label, ystart_sliders + yline_interval);
   (void) new wxMessage( panel, "Level", xpos_rev_label, ystart_sliders + 2*yline_interval);
   (void) new wxMessage( panel, "Time", xpos_rev_label, ystart_sliders + 3*yline_interval);
   (void) new wxMessage( panel, "Delay Feedb", xpos_rev_label, ystart_sliders + 4*yline_interval);
   (void) new wxMessage( panel, "Send to Chorus", xpos_rev_label, ystart_sliders + 5*yline_interval);

   (void) new wxMessage( panel, "Pre Lpf", xpos_cho_label, ystart_sliders);
   (void) new wxMessage( panel, "Level", xpos_cho_label, ystart_sliders + yline_interval);
   (void) new wxMessage( panel, "Feedback", xpos_cho_label, ystart_sliders + 2*yline_interval);
   (void) new wxMessage( panel, "Delay", xpos_cho_label, ystart_sliders + 3*yline_interval);
   (void) new wxMessage( panel, "Rate", xpos_cho_label, ystart_sliders + 4*yline_interval);
   (void) new wxMessage( panel, "Depth", xpos_cho_label, ystart_sliders + 5*yline_interval);
   (void) new wxMessage( panel, "Send to Reverb", xpos_cho_label, ystart_sliders + 6*yline_interval);

   tTrack *t = EventWin->Song->GetTrack(0);

   int val[GSEffectParams];
   int i;
   for (i = RevCharacter; i < GSEffectParams; i++) {
      val[i] = GetParameter( t, i );
      if (!val[i]) {
	 val[i] = effectnorm[i].valdef;
	 SetParameter( t, i, val[i] );
      }
   }

   for (i = RevCharacter; i <= RevSendChorus; i++) {
      (void) new tGsSlider( 	this, t, effectnorm[i], i,
				panel, (wxFunction) SliderChange, " ",
				val[i] - effectnorm[i].valnorm,
				effectnorm[i].valmin, effectnorm[i].valmax,
				slider_width, xpos_rev_sliders,
				ystart_sliders + (i*yline_interval) + slid_vert_offs );
   }

   for (i = ChoPreLpf; i <= ChoSendReverb; i++) {
      (void) new tGsSlider( 	this, t, effectnorm[i], i,
				panel, (wxFunction) SliderChange, " ",
				val[i] - effectnorm[i].valnorm,
				effectnorm[i].valmin, effectnorm[i].valmax,
				slider_width, xpos_cho_sliders,
				ystart_sliders + ((i-6)*yline_interval) + slid_vert_offs );
   }

   panel->SetLabelPosition(wxVERTICAL);
   (void) new tGsListBox( this, t, effectnorm[GSRevMacro], GSRevMacro,
			  val[GSRevMacro] - effectnorm[GSRevMacro].valnorm,
			  panel, (wxFunction) SelectionChange, "Reverb Macro",
			  wxSINGLE, xpos_rev_label, ystart_sliders + 8*yline_interval,
			  -1, -1, 8, RevSelArray );

   (void) new tGsListBox( this, t, effectnorm[GSChoMacro], GSChoMacro,
			  val[GSChoMacro] - effectnorm[GSChoMacro].valnorm,
			  panel, (wxFunction) SelectionChange, "Chorus Macro",
			  wxSINGLE, xpos_cho_label, ystart_sliders + 8*yline_interval,
			  -1, -1, 8, ChoSelArray );

   (void) new tGsCheckBox( this, t, effectnorm[UseRevMacro], UseRevMacro,
			   val[UseRevMacro] - effectnorm[UseRevMacro].valnorm,
			   panel, (wxFunction) CheckChange, "Use Reverb Macro",
			   xpos_rev_label, ystart_sliders + 8*yline_interval + 120 );

   (void) new tGsCheckBox( this, t, effectnorm[UseChoMacro], UseChoMacro,
			   val[UseChoMacro] - effectnorm[UseChoMacro].valnorm,
			   panel, (wxFunction) CheckChange, "Use Chorus Macro",
			   xpos_cho_label, ystart_sliders + 8*yline_interval + 120 );

}


// ******************************************************************
// XG Effects Dialog
// ******************************************************************

enum { 	XGRevMacro, XGChoMacro, XGEqMacro, XGEffectParams };

class tXGEffectsDlg : public tSliderDlg
{
   public:
      tXGEffectsDlg(tEventWin *w);
      virtual int GetParameter( tTrack *track, int type );
      virtual void SetParameter( tTrack *track, int type, int value );
      virtual void EditForm(wxPanel *panel);
      virtual void CloseWindow();

      SliderWinNorm effectnorm[XGEffectParams];
      char *RevSelArray[13];
      char *ChoSelArray[11];
      char *EqSelArray[5];

      static void SelectionChange( tGsListBox& box, wxCommandEvent& event );
};

tXGEffectsDlg::tXGEffectsDlg(tEventWin *w) : tSliderDlg( w, tSliderDlgEffects )
{
   RevSelArray[0] = "No effect";
   RevSelArray[1] = "Hall 1";
   RevSelArray[2] = "Hall 2";
   RevSelArray[3] = "Room 1";
   RevSelArray[4] = "Room 2";
   RevSelArray[5] = "Room 3";
   RevSelArray[6] = "Stage 1";
   RevSelArray[7] = "Stage 2";
   RevSelArray[8] = "Plate";
   RevSelArray[9] = "White Room";
   RevSelArray[10] = "Tunnel";
   RevSelArray[11] = "Canyon";
   RevSelArray[12] = "Basement";


   ChoSelArray[0] = "No effect";
   ChoSelArray[1] = "Chorus 1";
   ChoSelArray[2] = "Chorus 2";
   ChoSelArray[3] = "Chorus 3";
   ChoSelArray[4] = "Celeste 1";
   ChoSelArray[5] = "Celeste 2";
   ChoSelArray[6] = "Celeste 3";
   ChoSelArray[7] = "Flanger 1";
   ChoSelArray[8] = "Flanger 2";
   ChoSelArray[9] = "Symphonic";
   ChoSelArray[10] = "Phaser";

   EqSelArray[0] = "Flat";
   EqSelArray[1] = "Jazz";
   EqSelArray[2] = "Pops";
   EqSelArray[3] = "Rock";
   EqSelArray[4] = "Classic";

   // valnorm, valdef, valmin, valmax:
   effectnorm[XGRevMacro].init( 1, 2, 0, 12 );
   effectnorm[XGChoMacro].init( 1, 2, 0, 10 );
   effectnorm[XGEqMacro].init( 1, 1, 0, 4 );

   Config(C_UseReverbMacro) = 1;
   Config(C_UseChorusMacro) = 1;
}

void tXGEffectsDlg::CloseWindow()
{
   EventWin->DialogBox->GetPosition( &Config(C_PartsDlgXpos), &Config(C_PartsDlgYpos) );
   EventWin->DialogBox->Show(FALSE);
   delete EventWin->DialogBox;
   EventWin->DialogBox = 0;
   DELETE_THIS();
}

int tXGEffectsDlg::GetParameter( tTrack *track, int type ) {

   int msb;
   int lsb;

   switch (type) {
    case XGRevMacro:
       msb = track->GetReverbType();
       lsb = track->GetReverbType(1);
       switch (msb)
       {
	case 1:
	   return 1 + lsb - 1; // No effect
	case 2:
	   return 2 + lsb - 1; // Hall 1,2
	case 3:
	   return 4 + lsb - 1; // Room 1,2,3
	case 4:
	   return 7 + lsb - 1; // Stage 1,2
	case 5:
	   return 9 + lsb - 1; // Plate
	case 17:
	   return 10 + lsb - 1; // White Room
	case 18:
	   return 11 + lsb - 1; // Tunnel
	case 19:
	   return 12 + lsb - 1; // Canyon
	case 20:
	   return 13 + lsb - 1; // Basement
	default:
	   return 0;
       }
       break;
    case XGChoMacro:
       msb = track->GetChorusType() - 64; // 64 means "No effect"
       lsb = track->GetChorusType(1);
       switch (msb)
       {
	case 1:
	   return 1 + lsb - 1; // No effect
	case 2:
	   return 2 + lsb - 1; // Chorus 1,2,3
	case 3:
	   return 5 + lsb - 1; // Celeste 1,2,3
	case 4:
	   return 8 + lsb - 1; // Flanger 1,2
	case 5:
	   return 10 + lsb - 1; // Symphonic
	case 9:
	   return 11 + lsb - 1; // Phaser
	default:
	   return 0;
       }
    case XGEqMacro:
       return( track->GetEqualizerType() );
   }
   return( 0 );
}

void tXGEffectsDlg::SetParameter( tTrack *track, int type, int value ) {
   switch (type) {
    case XGRevMacro:
       switch (value)
       {
	case 1:
	   track->SetReverbType( 1, 1 ); // No effect
	   break;
	case 2:
	   track->SetReverbType( 2, 1 ); // Hall 1
	   break;
	case 3:
	   track->SetReverbType( 2, 2 ); // Hall 2
	   break;
	case 4:
	   track->SetReverbType( 3, 1 ); // Room 1
	   break;
	case 5:
	   track->SetReverbType( 3, 2 ); // Room 2
	   break;
	case 6:
	   track->SetReverbType( 3, 3 ); // Room 3
	   break;
	case 7:
	   track->SetReverbType( 4, 1 ); // Stage 1
	   break;
	case 8:
	   track->SetReverbType( 4, 2 ); // Stage 2
	   break;
	case 9:
	   track->SetReverbType( 5, 1 ); // Plate
	   break;
	case 10:
	   track->SetReverbType( 17, 1 ); // White Room
	   break;
	case 11:
	   track->SetReverbType( 18, 1 ); // Tunnel
	   break;
	case 12:
	   track->SetReverbType( 19, 1 ); // Canyon
	   break;
	case 13:
	   track->SetReverbType( 20, 1 ); // Basement
	   break;
	default:
	   break;
       }

       break;
    case XGChoMacro:
       switch (value)
       {
	case 1:
	   track->SetChorusType( 64 + 1, 1 ); // No Effect
	   break;
	case 2:
	   track->SetChorusType( 64 + 2, 1 ); // Chorus 1
	   break;
	case 3:
	   track->SetChorusType( 64 + 2, 2 ); // Chorus 2
	   break;
	case 4:
	   track->SetChorusType( 64 + 2, 3 ); // Chorus 3
	   break;
	case 5:
	   track->SetChorusType( 64 + 3, 1 ); // Celeste 1
	   break;
	case 6:
	   track->SetChorusType( 64 + 3, 2 ); // Celeste 2
	   break;
	case 7:
	   track->SetChorusType( 64 + 3, 3 ); // Celeste 3
	   break;
	case 8:
	   track->SetChorusType( 64 + 4, 1 ); // Flanger 1
	   break;
	case 9:
	   track->SetChorusType( 64 + 4, 2 ); // Flanger 2
	   break;
	case 10:
	   track->SetChorusType( 64 + 5, 1 ); // Symphonic
	   break;
	case 11:
	   track->SetChorusType( 64 + 9, 1 ); // Phaser
	   break;

	 default:
	    break;
       }

       break;
    case XGEqMacro:
       track->SetEqualizerType( value );
       break;
   }
   return;
}

void tXGEffectsDlg::SelectionChange( tGsListBox& box, wxCommandEvent& event ) {
   box.OnSelect();
}

void tXGEffectsDlg::EditForm(wxPanel *panel)
{
   const int slider_width = SLIDER_WIDTH;
   const int xpos_buttons = 5;
   const int ypos_buttons = 10;
   const int ypos_header = ypos_buttons + SLIDER_VERTICAL_SPACE;
   const int xpos_rev_label = 25;
   const int xpos_rev_sliders = xpos_rev_label + 100;
#ifdef wx_motif
   const int xpos_cho_label = xpos_rev_label + slider_width;
   const int xpos_eq_label = xpos_cho_label + slider_width;
#else
#ifdef wx_xview
   const int xpos_cho_label = xpos_rev_sliders + (3 * slider_width);
   const int xpos_eq_label = xpos_rev_sliders + (3 * slider_width);
#else
   const int xpos_cho_label = xpos_rev_sliders + 200;
   const int xpos_eq_label = xpos_rev_sliders + 400;
#endif
#endif
   const int xpos_cho_sliders = xpos_cho_label + 100;
   const int xcol_interval = XCOL_INTERVAL;

   const int ystart_sliders = ypos_header + SLIDER_VERTICAL_SPACE;
   const int yline_interval = SLIDER_VERTICAL_SPACE;
   const int slid_vert_offs = SLIDER_VERTICAL_OFFSET;

   (void) new tGsButton( this, panel, (wxFunction) Dismiss, "Dismiss" );
   (void) new tGsButton( this, panel, (wxFunction) Help, "Help" );

   tTrack *t = EventWin->Song->GetTrack(0);

   int val[XGEffectParams];
   int i;
   for (i = XGRevMacro; i < XGEffectParams; i++) {
      val[i] = GetParameter( t, i );
      if (!val[i]) {
	 val[i] = effectnorm[i].valdef;
	 SetParameter( t, i, val[i] );
      }
   }

   panel->SetLabelPosition(wxVERTICAL);
   (void) new tGsListBox( this, t, effectnorm[XGRevMacro], XGRevMacro,
			  val[XGRevMacro] - effectnorm[XGRevMacro].valnorm,
			  panel, (wxFunction) SelectionChange, "Reverb Macro",
			  wxSINGLE, xpos_rev_label, ystart_sliders,
			  -1, -1, 13, RevSelArray );

   (void) new tGsListBox( this, t, effectnorm[XGChoMacro], XGChoMacro,
			  val[XGChoMacro] - effectnorm[XGChoMacro].valnorm,
			  panel, (wxFunction) SelectionChange, "Chorus Macro",
			  wxSINGLE, -1, ystart_sliders,
			  -1, -1, 11, ChoSelArray );

   (void) new tGsListBox( this, t, effectnorm[XGEqMacro], XGEqMacro,
			  val[XGEqMacro] - effectnorm[XGEqMacro].valnorm,
			  panel, (wxFunction) SelectionChange, "Equalizer Macro",
			  wxSINGLE, -1, ystart_sliders,
			  -1, -1, 5, EqSelArray );

}

void tTrackWin::MenEffects()
{
   tSliderDlg *dlg;

   if (DialogBox)
   {
      DialogBox->Show(TRUE);
      return;
   }
   if (Synth->IsXG())
   {
      DialogBox = new wxDialogBox(this, "XG Effect Settings", FALSE, Config(C_PartsDlgXpos), Config(C_PartsDlgYpos));
      dlg = new tXGEffectsDlg(this);
   }
   else
   {
      DialogBox = new wxDialogBox(this, "GS Effect Settings", FALSE, Config(C_PartsDlgXpos), Config(C_PartsDlgYpos));
      dlg = new tGSEffectsDlg(this);
   }

   dlg->EditForm(DialogBox);
   DialogBox->Fit();
   DialogBox->Show(TRUE);
}

