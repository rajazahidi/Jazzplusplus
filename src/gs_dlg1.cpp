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

void tGsButton::GsOnDismiss()
{
   dialog->CloseWindow();
}

void tGsButton::GsOnHelp()
{
   if (dialog->helpKeyword)
   {
      HelpInstance->ShowTopic(dialog->helpKeyword);
   }
}

char *helpKeywords[ numSliderDialogs ] =
{
   "Track window", 			// tSliderDlgGeneric
   "Mixer and Master", 			// tSliderDlgMixer
   "Mixer and Master",			// tSliderDlgMaster
   "Sound, Vibrato and Envelope",  	// tSliderDlgSound
   "Sound, Vibrato and Envelope",  	// tSliderDlgVibrato
   "Sound, Vibrato and Envelope",  	// tSliderDlgEnvelope
   "Controller dialogs",			// tSliderDlgBendBasic
   "Controller dialogs",			// tSliderDlgBendLfo1
   "Controller dialogs",			// tSliderDlgBendLfo2
   "Controller dialogs",			// tSliderDlgModBasic
   "Controller dialogs",			// tSliderDlgModLfo1
   "Controller dialogs",			// tSliderDlgModLfo2
   "Controller dialogs",			// tSliderDlgCAfBasic
   "Controller dialogs",			// tSliderDlgCAfLfo1
   "Controller dialogs",			// tSliderDlgCAfLfo2
   "Controller dialogs",			// tSliderDlgPAfBasic
   "Controller dialogs",			// tSliderDlgPAfLfo1
   "Controller dialogs",			// tSliderDlgPAfLfo2
   "Controller dialogs",			// tSliderDlgCC1Basic
   "Controller dialogs",			// tSliderDlgCC1Lfo1
   "Controller dialogs",			// tSliderDlgCC1Lfo2
   "Controller dialogs",			// tSliderDlgCC2Basic
   "Controller dialogs",			// tSliderDlgCC2Lfo1
   "Controller dialogs",			// tSliderDlgCC2Lfo2
   "Partial Reserve",			// tSliderDlgPartRsrv
   "Part Mode",				// tSliderDlgPartMode
   "Effects",				// tSliderDlgEffects
   "Drum Instrument Parameters"		// tSliderDlgDrumParam
};

// ******************************************************************
// SliderWin Dialog Class
// ******************************************************************

tGsSlider *tSliderDlg::SliderArray[SliderWinMaxRow][SliderWinMaxCol];

tSliderDlg::tSliderDlg(tEventWin *w, int id)
{
   EventWin = w;
   classId = id;
   helpKeyword = helpKeywords[ classId ];
}

void tSliderDlg::CloseWindow()
{
   EventWin->MixerForm->GetPosition( &Config(C_PartsDlgXpos), &Config(C_PartsDlgYpos) );
   EventWin->MixerForm->Show(FALSE);
   delete EventWin->MixerForm;
   EventWin->MixerForm = 0;
   DELETE_THIS();
}

void tSliderDlg::Dismiss( tGsButton& button, wxCommandEvent& event )
{
   button.GsOnDismiss();
}

void tSliderDlg::Help( tGsButton& button, wxCommandEvent& event )
{
   button.GsOnHelp();
}

void tSliderDlg::SliderChange( tGsSlider& slider, wxCommandEvent& event ) {
   slider.OnChange();
}


void tSliderDlg::EditForm(wxPanel *panel)
{
#ifdef wx_motif
#define SLIDER_WIDTH_MACRO ((((classId == tSliderDlgCC1Basic) || (classId == tSliderDlgCC2Basic)) && (i == 0)) ? SLIDER_WIDTH + 50 : SLIDER_WIDTH)
#define SLIDER_XPOS_MACRO ((((classId == tSliderDlgCC1Basic) || (classId == tSliderDlgCC2Basic)) && (i > 0)) ? xstart_sliders + 50 + (i*xcol_interval) : xstart_sliders + (i*xcol_interval))
#define TEXT_XPOS_MACRO ((((classId == tSliderDlgCC1Basic) || (classId == tSliderDlgCC2Basic)) && (i > 0)) ? xstart_text + 50 + (i*xcol_interval) : xstart_text + (i*xcol_interval))
#else
#define SLIDER_WIDTH_MACRO slider_width
#define SLIDER_XPOS_MACRO (xstart_sliders + (i*xcol_interval))
#define TEXT_XPOS_MACRO (xstart_text + (i*xcol_interval))
#endif

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

   // Set max number of columns to show
   if (NrColumns > Config(C_PartsColumnsMax))
      NrColumns = Config(C_PartsColumnsMax);

   (void) new tGsButton( this, panel, (wxFunction) Dismiss, "Dismiss" );
   (void) new tGsButton( this, panel, (wxFunction) Help, "Help" );

   int i;

   for ( i = 0; i < NrColumns; i++ ) {
      (void) new wxMessage( panel, ColumnLabel[i], TEXT_XPOS_MACRO, ypos_text);
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

      if (classId == tSliderDlgMaster) {
	 track = EventWin->Song->GetTrack(0);
	 sprintf( labelstr, "Master" );
      }
      else {
	 track = NULL;
	 for (int j = first_track; j <= last_track; j++) {
	    tTrack *t = EventWin->Song->GetTrack(j);
	    if (t->IsEmpty()) continue;
	    if (t && (t->Channel == (chan + 1)) ) {
	       if (t->GetName() && strlen(t->GetName())) {
		  track = t;
		  break;
	       }
	    }
	 }
	 if (track) {
// SN++ Name auf 16 Zeichen kuerzen. (Ueberlage Namen bringen wxmotif zum Absturz)
#ifdef wx_motif
	    sprintf( labelstr, "%d %.16s", chan + 1, track->GetName() );
//
#else
	    sprintf( labelstr, "%d %s", chan + 1, track->GetName() );
#endif
	 }
      }


      if (track) {
	 int val[SliderWinMaxCol];
	 for (i = 0; i < NrColumns; i++) {
	    val[i] = GetParameter( track, i );
	    if (!val[i]) {
	       val[i] = norm[i].valdef;
	       SetParameter( track, i, val[i] );
	    }
	 }


	 for ( i = 0; i < NrColumns; i++ ) {
	    tGsSlider *sl =
	       new tGsSlider( 	this, track, norm[i],
				i,
				panel, (wxFunction) SliderChange, " ",
				val[i] - norm[i].valnorm, norm[i].valmin, norm[i].valmax,
				SLIDER_WIDTH_MACRO, SLIDER_XPOS_MACRO,
				ystart_sliders + chan*yline_interval + slid_vert_offs );
	    if (classId == tSliderDlgMixer)
	       SliderArray[chan][i] = sl;
	 }
      }
      else {
	 sprintf( labelstr, "%d", chan + 1 );
      }

      (void) new wxMessage( panel, labelstr, xpos_label, ystart_sliders + chan*yline_interval);
      // Draw tracknames on the right side too?
      if (Config(C_PartsTracknamesRight))
      {
	 (void) new wxMessage( panel, labelstr, xstart_sliders + (NrColumns*xcol_interval) + 10, ystart_sliders + chan*yline_interval);
      }
   }

}


// ******************************************************************
// Mixer Dialog
// ******************************************************************
// It's declared in gs_dlgs.h:
//class tMixerDlg : public tSliderDlg
//{
// public:
//  tMixerDlg(tEventWin *w);
//  virtual int GetParameter( tTrack *track, int type );
//  virtual void SetParameter( tTrack *track, int type, int value );
//  static void SetSliderVal( int chan, int type, int val );
//};

tMixerDlg::tMixerDlg(tEventWin *w) : tSliderDlg( w, tSliderDlgMixer )
{
   NrColumns = MxParams;
   NrRows = 16;

   ColumnLabel[MxVol] = " Volume";
   ColumnLabel[MxPan] = "   Pan";
   ColumnLabel[MxRev] = " Reverb";
   ColumnLabel[MxCho] = " Chorus";

   // valnorm, valdef, valmin, valmax:
   norm[MxVol].init( 0, 100, 1, 128 ); // Volume
   norm[MxPan].init( 65, 65, -63, 63 ); // Pan
   norm[MxRev].init( 0, 41, 1, 128 ); // Reverb
   norm[MxCho].init( 0, 1, 1, 128 ); // Chorus
}

void tMixerDlg::CloseWindow()
{
   for (int r = 0; r < SliderWinMaxRow; r++)
      for (int c = 0; c < SliderWinMaxCol; c++)
	 SliderArray[r][c] = NULL;
   tSliderDlg::CloseWindow();
}

int tMixerDlg::GetParameter( tTrack *track, int type ) {
   switch (type) {
    case MxVol:
       return( track->GetVolume() );
    case MxPan:
       return( track->GetPan() );
    case MxRev:
       return( track->GetReverb() );
    case MxCho:
       return( track->GetChorus() );
   }
   return 0;
}

void tMixerDlg::SetParameter( tTrack *track, int type, int value ) {
   switch (type) {
    case MxVol:
       track->SetVolume( value );
       break;
    case MxPan:
       track->SetPan( value );
       break;
    case MxRev:
       track->SetReverb( value );
       break;
    case MxCho:
       track->SetChorus( value );
       break;
   }

   ((tTrackWin*) EventWin)->DrawCounters();

   return;
}

void tMixerDlg::SetSliderVal( int chan, int type, int val ) {
   if (SliderArray[chan][type]) {
      SliderArray[chan][type]->SetNormValue( val );
   }
}

void tTrackWin::MenMixer()
{
   tMixerDlg *dlg;
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
   MixerForm = new wxDialogBox(this, "Mixer", FALSE, Config(C_PartsDlgXpos), Config(C_PartsDlgYpos));
   dlg = new tMixerDlg(this);
   dlg->EditForm(MixerForm);
   MixerForm->Fit();
   MixerForm->Show(TRUE);
}


// ******************************************************************
// Master Dialog
// ******************************************************************
class tMasterDlg : public tSliderDlg
{
   public:
      tMasterDlg(tEventWin *w);
      virtual int GetParameter( tTrack *track, int type );
      virtual void SetParameter( tTrack *track, int type, int value );

};

tMasterDlg::tMasterDlg(tEventWin *w) : tSliderDlg( w, tSliderDlgMaster )
{
   if (Synth->IsGS())
   {
      NrColumns = 2;
   }
   else
   {
      NrColumns = 1;
   }

   NrRows = 1;

   ColumnLabel[MxVol] = " Volume";
   ColumnLabel[MxPan] = "   Pan";

   // valnorm, valdef, valmin, valmax:
   norm[MxVol].init( 0, 128, 1, 128 ); // Volume
   norm[MxPan].init( 65, 65, -63, 63 ); // Pan
}

int tMasterDlg::GetParameter( tTrack *track, int type ) {
   switch (type) {
    case MxVol:
       return( track->GetMasterVol() );
    case MxPan:
       return( track->GetMasterPan() );
   }
   return 0;
}

void tMasterDlg::SetParameter( tTrack *track, int type, int value ) {
   switch (type) {
    case MxVol:
       track->SetMasterVol( value );
       return;
    case MxPan:
       track->SetMasterPan( value );
       return;
   }
}

void tTrackWin::MenMaster()
{
   tMasterDlg *dlg;
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
   MixerForm = new wxDialogBox(this, "Mixer", FALSE, Config(C_PartsDlgXpos), Config(C_PartsDlgYpos));
   dlg = new tMasterDlg(this);
   dlg->EditForm(MixerForm);
   MixerForm->Fit();
   MixerForm->Show(TRUE);
}



// ******************************************************************
// Sound Dialog
// ******************************************************************

class tSoundDlg : public tSliderDlg
{
   public:
      tSoundDlg(tEventWin *w);
      virtual int GetParameter( tTrack *track, int type );
      virtual void SetParameter( tTrack *track, int type, int value );

};

tSoundDlg::tSoundDlg(tEventWin *w) : tSliderDlg( w, tSliderDlgSound )
{
   NrColumns = NrpnSoundParams;
   NrRows = 16;

   ColumnLabel[NrpnCutoff] = "Cutoff frequency";
   ColumnLabel[NrpnResonance] = "Resonance";

   // valnorm, valdef, valmin, valmax:
   norm[NrpnCutoff].init( 65, 65, -50, 16 ); // Cutoff
   norm[NrpnResonance].init( 65, 65, -50, 50 ); // Resonance
}

int tSoundDlg::GetParameter( tTrack *track, int type ) {
   switch (type) {
    case NrpnCutoff:
       return( track->GetCutoff() );
    case NrpnResonance:
       return( track->GetResonance() );
   }
   return 0;
}

void tSoundDlg::SetParameter( tTrack *track, int type, int value ) {
   switch (type) {
    case NrpnCutoff:
       track->SetCutoff( value );
       return;
    case NrpnResonance:
       track->SetResonance( value );
       return;
   }
}

void tTrackWin::MenSound()
{
   tSoundDlg *dlg;
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
   MixerForm = new wxDialogBox(this, "Sound", FALSE, Config(C_PartsDlgXpos), Config(C_PartsDlgYpos));
   dlg = new tSoundDlg(this);
   dlg->EditForm(MixerForm);
   MixerForm->Fit();
   MixerForm->Show(TRUE);
}


// ******************************************************************
// Vibrato Dialog
// ******************************************************************

class tVibratoDlg : public tSliderDlg
{
   public:
      tVibratoDlg(tEventWin *w);
      virtual int GetParameter( tTrack *track, int type );
      virtual void SetParameter( tTrack *track, int type, int value );

};

tVibratoDlg::tVibratoDlg(tEventWin *w) : tSliderDlg( w, tSliderDlgVibrato )
{
   NrColumns = NrpnVibParams;
   NrRows = 16;

   ColumnLabel[NrpnVibRate] = "Vib. Rate";
   ColumnLabel[NrpnVibDepth] = "Vib. Depth";
   ColumnLabel[NrpnVibDelay] = "Vib. Delay";

   // valnorm, valdef, valmin, valmax:
   norm[NrpnVibRate].init( 65, 65, -50, 50 ); // Rate
   norm[NrpnVibDepth].init( 65, 65, -50, 50 ); // Depth
   norm[NrpnVibDelay].init( 65, 65, -50, 50 ); // Delay
}

int tVibratoDlg::GetParameter( tTrack *track, int type ) {
   switch (type) {
    case NrpnVibRate:
       return( track->GetVibRate() );
    case NrpnVibDepth:
       return( track->GetVibDepth() );
    case NrpnVibDelay:
       return( track->GetVibDelay() );
   }
   return 0;
}

void tVibratoDlg::SetParameter( tTrack *track, int type, int value ) {
   switch (type) {
    case NrpnVibRate:
       track->SetVibRate( value );
       return;
    case NrpnVibDepth:
       track->SetVibDepth( value );
       return;
    case NrpnVibDelay:
       track->SetVibDelay( value );
       return;
   }
}

void tTrackWin::MenVibrato()
{
   tVibratoDlg *dlg;
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
   MixerForm = new wxDialogBox(this, "Vibrato", FALSE, Config(C_PartsDlgXpos), Config(C_PartsDlgYpos));
   dlg = new tVibratoDlg(this);
   dlg->EditForm(MixerForm);
   MixerForm->Fit();
   MixerForm->Show(TRUE);
}


// ******************************************************************
// Envelope Dialog
// ******************************************************************

class tEnvelopeDlg : public tSliderDlg
{
   public:
      tEnvelopeDlg(tEventWin *w);
      virtual int GetParameter( tTrack *track, int type );
      virtual void SetParameter( tTrack *track, int type, int value );

};

tEnvelopeDlg::tEnvelopeDlg(tEventWin *w) : tSliderDlg( w, tSliderDlgEnvelope )
{
   NrColumns = NrpnEnvParams;
   NrRows = 16;

   ColumnLabel[NrpnEnvAttack] = "Env. Attack";
   ColumnLabel[NrpnEnvDecay] = "Env. Decay";
   ColumnLabel[NrpnEnvRelease] = "Env. Release";

   // valnorm, valdef, valmin, valmax:
   norm[NrpnEnvAttack].init( 65, 65, -50, 50 ); // Attack
   norm[NrpnEnvDecay].init( 65, 65, -50, 50 ); // Decay
   norm[NrpnEnvRelease].init( 65, 65, -50, 50 ); // Release
}

int tEnvelopeDlg::GetParameter( tTrack *track, int type ) {
   switch (type) {
    case NrpnEnvAttack:
       return( track->GetEnvAttack() );
    case NrpnEnvDecay:
       return( track->GetEnvDecay() );
    case NrpnEnvRelease:
       return( track->GetEnvRelease() );
   }
   return 0;
}

void tEnvelopeDlg::SetParameter( tTrack *track, int type, int value ) {
   switch (type) {
    case NrpnEnvAttack:
       track->SetEnvAttack( value );
       return;
    case NrpnEnvDecay:
       track->SetEnvDecay( value );
       return;
    case NrpnEnvRelease:
       track->SetEnvRelease( value );
       return;
   }
}

void tTrackWin::MenEnvelope()
{
   tEnvelopeDlg *dlg;
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
   MixerForm = new wxDialogBox(this, "Envelope", FALSE, Config(C_PartsDlgXpos), Config(C_PartsDlgYpos));
   dlg = new tEnvelopeDlg(this);
   dlg->EditForm(MixerForm);
   MixerForm->Fit();
   MixerForm->Show(TRUE);
}


// ******************************************************************
// Bender Basic Dialog
// ******************************************************************

enum { 	BendPitchSens = 0, BendTvfCut, BendAmpl, BendBasicParams };

class tBendBasicDlg : public tSliderDlg
{
   public:
      tBendBasicDlg(tEventWin *w);
      virtual int GetParameter( tTrack *track, int type );
      virtual void SetParameter( tTrack *track, int type, int value );

};

tBendBasicDlg::tBendBasicDlg(tEventWin *w) : tSliderDlg( w, tSliderDlgBendBasic )
{
   if (Synth->IsGM())
   {
      NrColumns = 1;
   }
   else
   {
      NrColumns = BendBasicParams;
   }

   NrRows = 16;

   ColumnLabel[BendPitchSens] = "Pitch Sens";
   ColumnLabel[BendTvfCut] = "TVF Cutoff";
   ColumnLabel[BendAmpl] = "Amplitude";

   // valnorm, valdef, valmin, valmax:
   norm[BendPitchSens].init( 1, 3, 0, 24 );
   norm[BendTvfCut].init( 65, 65, -63, 63 );
   norm[BendAmpl].init( 65, 65, -63, 63 );
}

int tBendBasicDlg::GetParameter( tTrack *track, int type ) {
   switch (type) {
    case BendPitchSens:
       return( track->GetBendPitchSens() );
    case BendTvfCut:
    case BendAmpl:
       return( track->GetBenderSysex( type ) );
   }
   return 0;
}

void tBendBasicDlg::SetParameter( tTrack *track, int type, int value ) {
   switch (type) {
    case BendPitchSens:
       track->SetBendPitchSens( value );
       return;
    case BendTvfCut:
    case BendAmpl:
       track->SetBenderSysex( type, value );
       return;
   }
}

void tTrackWin::MenBendBasic()
{
   tBendBasicDlg *dlg;
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
   MixerForm = new wxDialogBox(this, "Bender Basic", FALSE, Config(C_PartsDlgXpos), Config(C_PartsDlgYpos));
   dlg = new tBendBasicDlg(this);
   dlg->EditForm(MixerForm);
   MixerForm->Fit();
   MixerForm->Show(TRUE);
}


// ******************************************************************
// Bender LFO1 Dialog
// ******************************************************************

enum { 	BendLfo1Rate = 0, BendLfo1Pitch, BendLfo1Tvf, BendLfo1Tva, BendLfo1Params };

class tBendLfo1Dlg : public tSliderDlg
{
      int xg_offset;
   public:
      tBendLfo1Dlg(tEventWin *w);
      virtual int GetParameter( tTrack *track, int type );
      virtual void SetParameter( tTrack *track, int type, int value );

};

tBendLfo1Dlg::tBendLfo1Dlg(tEventWin *w) : tSliderDlg( w, tSliderDlgBendLfo1 )
{
   if (Synth->IsXG())
   {
      xg_offset = 1;
   }
   else
   {
      xg_offset = 0;
   }

   NrColumns = BendLfo1Params - xg_offset;

   NrRows = 16;

   if (!Synth->IsXG())
   {
      ColumnLabel[BendLfo1Rate] = "LFO1 Rate";
   }

   ColumnLabel[BendLfo1Pitch - xg_offset] = "LFO1 Pitch";
   ColumnLabel[BendLfo1Tvf - xg_offset] = "LFO1 TVF";
   ColumnLabel[BendLfo1Tva - xg_offset] = "LFO1 TVA";

   // valnorm, valdef, valmin, valmax:
   if (!Synth->IsXG())
   {
      norm[BendLfo1Rate].init( 65, 65, -63, 63 );
   }

   norm[BendLfo1Pitch - xg_offset].init( 1, 1, 0, 127 );
   norm[BendLfo1Tvf - xg_offset].init( 1, 1, 0, 127 );
   norm[BendLfo1Tva - xg_offset].init( 1, 1, 0, 127 );
}

int tBendLfo1Dlg::GetParameter( tTrack *track, int type ) {
   return( track->GetBenderSysex( type + xg_offset + 3 ) );
}

void tBendLfo1Dlg::SetParameter( tTrack *track, int type, int value ) {
   track->SetBenderSysex( type + xg_offset + 3, value );
}

void tTrackWin::MenBendLfo1()
{
   tBendLfo1Dlg *dlg;
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
   MixerForm = new wxDialogBox(this, "Bender LFO1", FALSE, Config(C_PartsDlgXpos), Config(C_PartsDlgYpos));
   dlg = new tBendLfo1Dlg(this);
   dlg->EditForm(MixerForm);
   MixerForm->Fit();
   MixerForm->Show(TRUE);
}


// ******************************************************************
// Bender LFO2 Dialog
// ******************************************************************

enum { 	BendLfo2Rate = 0, BendLfo2Pitch, BendLfo2Tvf, BendLfo2Tva, BendLfo2Params };

class tBendLfo2Dlg : public tSliderDlg
{
   public:
      tBendLfo2Dlg(tEventWin *w);
      virtual int GetParameter( tTrack *track, int type );
      virtual void SetParameter( tTrack *track, int type, int value );

};

tBendLfo2Dlg::tBendLfo2Dlg(tEventWin *w) : tSliderDlg( w, tSliderDlgBendLfo2 )
{
   NrColumns = BendLfo2Params;
   NrRows = 16;

   ColumnLabel[BendLfo2Rate] = "LFO2 Rate";
   ColumnLabel[BendLfo2Pitch] = "LFO2 Pitch";
   ColumnLabel[BendLfo2Tvf] = "LFO2 TVF";
   ColumnLabel[BendLfo2Tva] = "LFO2 TVA";

   // valnorm, valdef, valmin, valmax:
   norm[BendLfo2Rate].init( 65, 65, -63, 63 );
   norm[BendLfo2Pitch].init( 1, 1, 0, 127 );
   norm[BendLfo2Tvf].init( 1, 1, 0, 127 );
   norm[BendLfo2Tva].init( 1, 1, 0, 127 );
}

int tBendLfo2Dlg::GetParameter( tTrack *track, int type ) {
   return( track->GetBenderSysex( type + 7 ) );
}

void tBendLfo2Dlg::SetParameter( tTrack *track, int type, int value ) {
   track->SetBenderSysex( type + 7, value );
}

void tTrackWin::MenBendLfo2()
{
   tBendLfo2Dlg *dlg;
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
   MixerForm = new wxDialogBox(this, "Bender LFO2", FALSE, Config(C_PartsDlgXpos), Config(C_PartsDlgYpos));
   dlg = new tBendLfo2Dlg(this);
   dlg->EditForm(MixerForm);
   MixerForm->Fit();
   MixerForm->Show(TRUE);
}


// ******************************************************************
// Modulation Basic Dialog
// ******************************************************************

enum { 	ModPitchControl = 0, ModTvfCut, ModAmpl, ModBasicParams };

class tModBasicDlg : public tSliderDlg
{
   public:
      tModBasicDlg(tEventWin *w);
      virtual int GetParameter( tTrack *track, int type );
      virtual void SetParameter( tTrack *track, int type, int value );

};

tModBasicDlg::tModBasicDlg(tEventWin *w) : tSliderDlg( w, tSliderDlgModBasic )
{
   NrColumns = ModBasicParams;
   NrRows = 16;

   ColumnLabel[ModPitchControl] = "Pitch Ctrl";
   ColumnLabel[ModTvfCut] = "TVF Cutoff";
   ColumnLabel[ModAmpl] = "Amplitude";

   // valnorm, valdef, valmin, valmax:
   norm[ModPitchControl].init( 65, 65, -24, 24 );
   norm[ModTvfCut].init( 65, 65, -63, 63 );
   norm[ModAmpl].init( 65, 65, -63, 63 );
}

int tModBasicDlg::GetParameter( tTrack *track, int type ) {
   return( track->GetModulationSysex( type ) );
}

void tModBasicDlg::SetParameter( tTrack *track, int type, int value ) {
   track->SetModulationSysex( type, value );
   return;
}

void tTrackWin::MenModBasic()
{
   tModBasicDlg *dlg;
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
   MixerForm = new wxDialogBox(this, "Modulation Basic", FALSE, Config(C_PartsDlgXpos), Config(C_PartsDlgYpos));
   dlg = new tModBasicDlg(this);
   dlg->EditForm(MixerForm);
   MixerForm->Fit();
   MixerForm->Show(TRUE);
}

// ******************************************************************
// Modulation LFO1 Dialog
// ******************************************************************

enum { 	ModLfo1Rate = 0, ModLfo1Pitch, ModLfo1Tvf, ModLfo1Tva, ModLfo1Params };

class tModLfo1Dlg : public tSliderDlg
{
      int xg_offset;

   public:
      tModLfo1Dlg(tEventWin *w);
      virtual int GetParameter( tTrack *track, int type );
      virtual void SetParameter( tTrack *track, int type, int value );

};

tModLfo1Dlg::tModLfo1Dlg(tEventWin *w) : tSliderDlg( w, tSliderDlgModLfo1 )
{
   if (Synth->IsXG())
   {
      xg_offset = 1;
   }
   else
   {
      xg_offset = 0;
   }

   NrColumns = ModLfo1Params - xg_offset;
   NrRows = 16;

   if (!Synth->IsXG())
   {
      ColumnLabel[ModLfo1Rate] = "LFO1 Rate";
   }

   ColumnLabel[ModLfo1Pitch - xg_offset] = "LFO1 Pitch";
   ColumnLabel[ModLfo1Tvf - xg_offset] = "LFO1 TVF";
   ColumnLabel[ModLfo1Tva - xg_offset] = "LFO1 TVA";

   // valnorm, valdef, valmin, valmax:
   if (!Synth->IsXG())
   {
      norm[ModLfo1Rate].init( 65, 65, -63, 63 );
   }

   norm[ModLfo1Pitch - xg_offset].init( 1, 11, 0, 127 );
   norm[ModLfo1Tvf - xg_offset].init( 1, 1, 0, 127 );
   norm[ModLfo1Tva - xg_offset].init( 1, 1, 0, 127 );
}

int tModLfo1Dlg::GetParameter( tTrack *track, int type ) {
   return( track->GetModulationSysex( type + xg_offset + 3 ) );
}

void tModLfo1Dlg::SetParameter( tTrack *track, int type, int value ) {
   track->SetModulationSysex( type + xg_offset + 3, value );
}

void tTrackWin::MenModLfo1()
{
   tModLfo1Dlg *dlg;
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
   MixerForm = new wxDialogBox(this, "Modulation LFO1", FALSE, Config(C_PartsDlgXpos), Config(C_PartsDlgYpos));
   dlg = new tModLfo1Dlg(this);
   dlg->EditForm(MixerForm);
   MixerForm->Fit();
   MixerForm->Show(TRUE);
}


// ******************************************************************
// Modulation LFO2 Dialog
// ******************************************************************

enum { 	ModLfo2Rate = 0, ModLfo2Pitch, ModLfo2Tvf, ModLfo2Tva, ModLfo2Params };

class tModLfo2Dlg : public tSliderDlg
{
   public:
      tModLfo2Dlg(tEventWin *w);
      virtual int GetParameter( tTrack *track, int type );
      virtual void SetParameter( tTrack *track, int type, int value );

};

tModLfo2Dlg::tModLfo2Dlg(tEventWin *w) : tSliderDlg( w, tSliderDlgModLfo2 )
{
   NrColumns = ModLfo2Params;
   NrRows = 16;

   ColumnLabel[ModLfo2Rate] = "LFO2 Rate";
   ColumnLabel[ModLfo2Pitch] = "LFO2 Pitch";
   ColumnLabel[ModLfo2Tvf] = "LFO2 TVF";
   ColumnLabel[ModLfo2Tva] = "LFO2 TVA";

   // valnorm, valdef, valmin, valmax:
   norm[ModLfo2Rate].init( 65, 65, -63, 63 );
   norm[ModLfo2Pitch].init( 1, 1, 0, 127 );
   norm[ModLfo2Tvf].init( 1, 1, 0, 127 );
   norm[ModLfo2Tva].init( 1, 1, 0, 127 );
}

int tModLfo2Dlg::GetParameter( tTrack *track, int type ) {
   return( track->GetModulationSysex( type + 7 ) );
}

void tModLfo2Dlg::SetParameter( tTrack *track, int type, int value ) {
   track->SetModulationSysex( type + 7, value );
}

void tTrackWin::MenModLfo2()
{
   tModLfo2Dlg *dlg;
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
   MixerForm = new wxDialogBox(this, "Modulation LFO2", FALSE, Config(C_PartsDlgXpos), Config(C_PartsDlgYpos));
   dlg = new tModLfo2Dlg(this);
   dlg->EditForm(MixerForm);
   MixerForm->Fit();
   MixerForm->Show(TRUE);
}


// ******************************************************************
// CAf Basic Dialog
// ******************************************************************

enum { 	CAfPitchControl = 0, CAfTvfCut, CAfAmpl, CAfBasicParams };

class tCAfBasicDlg : public tSliderDlg
{
   public:
      tCAfBasicDlg(tEventWin *w);
      virtual int GetParameter( tTrack *track, int type );
      virtual void SetParameter( tTrack *track, int type, int value );

};

tCAfBasicDlg::tCAfBasicDlg(tEventWin *w) : tSliderDlg( w, tSliderDlgCAfBasic )
{
   NrColumns = CAfBasicParams;
   NrRows = 16;

   ColumnLabel[CAfPitchControl] = "Pitch Ctrl";
   ColumnLabel[CAfTvfCut] = "TVF Cutoff";
   ColumnLabel[CAfAmpl] = "Amplitude";

   // valnorm, valdef, valmin, valmax:
   norm[CAfPitchControl].init( 65, 65, -24, 24 );
   norm[CAfTvfCut].init( 65, 65, -63, 63 );
   norm[CAfAmpl].init( 65, 65, -63, 63 );
}

int tCAfBasicDlg::GetParameter( tTrack *track, int type ) {
   return( track->GetCAfSysex( type ) );
}

void tCAfBasicDlg::SetParameter( tTrack *track, int type, int value ) {
   track->SetCAfSysex( type, value );
   return;
}

void tTrackWin::MenCAfBasic()
{
   tCAfBasicDlg *dlg;
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
   MixerForm = new wxDialogBox(this, "CAf Basic", FALSE, Config(C_PartsDlgXpos), Config(C_PartsDlgYpos));
   dlg = new tCAfBasicDlg(this);
   dlg->EditForm(MixerForm);
   MixerForm->Fit();
   MixerForm->Show(TRUE);
}


// ******************************************************************
// CAf LFO1 Dialog
// ******************************************************************

enum { 	CAfLfo1Rate = 0, CAfLfo1Pitch, CAfLfo1Tvf, CAfLfo1Tva, CAfLfo1Params };

class tCAfLfo1Dlg : public tSliderDlg
{
      int xg_offset;

   public:
      tCAfLfo1Dlg(tEventWin *w);
      virtual int GetParameter( tTrack *track, int type );
      virtual void SetParameter( tTrack *track, int type, int value );

};

tCAfLfo1Dlg::tCAfLfo1Dlg(tEventWin *w) : tSliderDlg( w, tSliderDlgCAfLfo1 )
{
   if (Synth->IsXG())
   {
      xg_offset = 1;
   }
   else
   {
      xg_offset = 0;
   }

   NrColumns = CAfLfo1Params - xg_offset;
   NrRows = 16;

   if (!Synth->IsXG())
   {
      ColumnLabel[CAfLfo1Rate] = "LFO1 Rate";
   }

   ColumnLabel[CAfLfo1Pitch - xg_offset] = "LFO1 Pitch";
   ColumnLabel[CAfLfo1Tvf - xg_offset] = "LFO1 TVF";
   ColumnLabel[CAfLfo1Tva - xg_offset] = "LFO1 TVA";

   // valnorm, valdef, valmin, valmax:
   if (!Synth->IsXG())
   {
      norm[CAfLfo1Rate].init( 65, 65, -63, 63 );
   }

   norm[CAfLfo1Pitch - xg_offset].init( 1, 1, 0, 127 );
   norm[CAfLfo1Tvf - xg_offset].init( 1, 1, 0, 127 );
   norm[CAfLfo1Tva - xg_offset].init( 1, 1, 0, 127 );
}

int tCAfLfo1Dlg::GetParameter( tTrack *track, int type ) {
   return( track->GetCAfSysex( type + xg_offset + 3 ) );
}

void tCAfLfo1Dlg::SetParameter( tTrack *track, int type, int value ) {
   track->SetCAfSysex( type + xg_offset + 3, value );
}

void tTrackWin::MenCAfLfo1()
{
   tCAfLfo1Dlg *dlg;
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
   MixerForm = new wxDialogBox(this, "CAf LFO1", FALSE, Config(C_PartsDlgXpos), Config(C_PartsDlgYpos));
   dlg = new tCAfLfo1Dlg(this);
   dlg->EditForm(MixerForm);
   MixerForm->Fit();
   MixerForm->Show(TRUE);
}

// ******************************************************************
// CAf LFO2 Dialog
// ******************************************************************

enum { 	CAfLfo2Rate = 0, CAfLfo2Pitch, CAfLfo2Tvf, CAfLfo2Tva, CAfLfo2Params };

class tCAfLfo2Dlg : public tSliderDlg
{
   public:
      tCAfLfo2Dlg(tEventWin *w);
      virtual int GetParameter( tTrack *track, int type );
      virtual void SetParameter( tTrack *track, int type, int value );

};

tCAfLfo2Dlg::tCAfLfo2Dlg(tEventWin *w) : tSliderDlg( w, tSliderDlgCAfLfo2 )
{
   NrColumns = CAfLfo2Params;
   NrRows = 16;

   ColumnLabel[CAfLfo2Rate] = "LFO2 Rate";
   ColumnLabel[CAfLfo2Pitch] = "LFO2 Pitch";
   ColumnLabel[CAfLfo2Tvf] = "LFO2 TVF";
   ColumnLabel[CAfLfo2Tva] = "LFO2 TVA";

   // valnorm, valdef, valmin, valmax:
   norm[CAfLfo2Rate].init( 65, 65, -63, 63 );
   norm[CAfLfo2Pitch].init( 1, 1, 0, 127 );
   norm[CAfLfo2Tvf].init( 1, 1, 0, 127 );
   norm[CAfLfo2Tva].init( 1, 1, 0, 127 );
}

int tCAfLfo2Dlg::GetParameter( tTrack *track, int type ) {
   return( track->GetCAfSysex( type + 7 ) );
}

void tCAfLfo2Dlg::SetParameter( tTrack *track, int type, int value ) {
   track->SetCAfSysex( type + 7, value );
}

void tTrackWin::MenCAfLfo2()
{
   tCAfLfo2Dlg *dlg;
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
   MixerForm = new wxDialogBox(this, "CAf LFO2", FALSE, Config(C_PartsDlgXpos), Config(C_PartsDlgYpos));
   dlg = new tCAfLfo2Dlg(this);
   dlg->EditForm(MixerForm);
   MixerForm->Fit();
   MixerForm->Show(TRUE);
}

// ******************************************************************
// PAf Basic Dialog
// ******************************************************************

enum { 	PAfPitchControl = 0, PAfTvfCut, PAfAmpl, PAfBasicParams };

class tPAfBasicDlg : public tSliderDlg
{
   public:
      tPAfBasicDlg(tEventWin *w);
      virtual int GetParameter( tTrack *track, int type );
      virtual void SetParameter( tTrack *track, int type, int value );

};

tPAfBasicDlg::tPAfBasicDlg(tEventWin *w) : tSliderDlg( w, tSliderDlgPAfBasic )
{
   NrColumns = PAfBasicParams;
   NrRows = 16;

   ColumnLabel[PAfPitchControl] = "Pitch Ctrl";
   ColumnLabel[PAfTvfCut] = "TVF Cutoff";
   ColumnLabel[PAfAmpl] = "Amplitude";

   // valnorm, valdef, valmin, valmax:
   norm[PAfPitchControl].init( 65, 65, -24, 24 );
   norm[PAfTvfCut].init( 65, 65, -63, 63 );
   norm[PAfAmpl].init( 65, 65, -63, 63 );
}

int tPAfBasicDlg::GetParameter( tTrack *track, int type ) {
   return( track->GetPAfSysex( type ) );
}

void tPAfBasicDlg::SetParameter( tTrack *track, int type, int value ) {
   track->SetPAfSysex( type, value );
   return;
}

void tTrackWin::MenPAfBasic()
{
   tPAfBasicDlg *dlg;
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
   MixerForm = new wxDialogBox(this, "PAf Basic", FALSE, Config(C_PartsDlgXpos), Config(C_PartsDlgYpos));
   dlg = new tPAfBasicDlg(this);
   dlg->EditForm(MixerForm);
   MixerForm->Fit();
   MixerForm->Show(TRUE);
}


// ******************************************************************
// PAf LFO1 Dialog
// ******************************************************************

enum { 	PAfLfo1Rate = 0, PAfLfo1Pitch, PAfLfo1Tvf, PAfLfo1Tva, PAfLfo1Params };

class tPAfLfo1Dlg : public tSliderDlg
{
      int xg_offset;

   public:
      tPAfLfo1Dlg(tEventWin *w);
      virtual int GetParameter( tTrack *track, int type );
      virtual void SetParameter( tTrack *track, int type, int value );

};

tPAfLfo1Dlg::tPAfLfo1Dlg(tEventWin *w) : tSliderDlg( w, tSliderDlgPAfLfo1 )
{
   if (Synth->IsXG())
   {
      xg_offset = 1;
   }
   else
   {
      xg_offset = 0;
   }

   NrColumns = PAfLfo1Params - xg_offset;
   NrRows = 16;

   if (!Synth->IsXG())
   {
      ColumnLabel[PAfLfo1Rate] = "LFO1 Rate";
   }

   ColumnLabel[PAfLfo1Pitch - xg_offset] = "LFO1 Pitch";
   ColumnLabel[PAfLfo1Tvf - xg_offset] = "LFO1 TVF";
   ColumnLabel[PAfLfo1Tva - xg_offset] = "LFO1 TVA";

   // valnorm, valdef, valmin, valmax:
   if (!Synth->IsXG())
   {
      norm[PAfLfo1Rate].init( 65, 65, -63, 63 );
   }

   norm[PAfLfo1Pitch - xg_offset].init( 1, 1, 0, 127 );
   norm[PAfLfo1Tvf - xg_offset].init( 1, 1, 0, 127 );
   norm[PAfLfo1Tva - xg_offset].init( 1, 1, 0, 127 );
}

int tPAfLfo1Dlg::GetParameter( tTrack *track, int type ) {
   return( track->GetPAfSysex( type + xg_offset + 3 ) );
}

void tPAfLfo1Dlg::SetParameter( tTrack *track, int type, int value ) {
   track->SetPAfSysex( type + xg_offset + 3, value );
}

void tTrackWin::MenPAfLfo1()
{
   tPAfLfo1Dlg *dlg;
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
   MixerForm = new wxDialogBox(this, "PAf LFO1", FALSE, Config(C_PartsDlgXpos), Config(C_PartsDlgYpos));
   dlg = new tPAfLfo1Dlg(this);
   dlg->EditForm(MixerForm);
   MixerForm->Fit();
   MixerForm->Show(TRUE);
}

