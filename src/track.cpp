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

#include "synth.h"
#include "track.h"
#include "util.h"
#include "player.h"
#include "jazz.h"
#include "eventwin.h"
#include "trackwin.h"

#include <stdlib.h>

//for sleep
#include <unistd.h>

#include <assert.h>

int tParam::Write(tWriteBase &io) {
	return( Msb.Write( io ) + Lsb.Write( io ) + DataMsb.Write( io ) );
}

void tParam::SetCha( uchar cha ) {
	Msb.Channel = cha;
	Lsb.Channel = cha;
	DataMsb.Channel = cha;
#ifndef __PORTING

		ResetMb.Channel = cha; //???? JAVE commented out this while porting
#endif // __PORTING


	ResetLsb.Channel = cha;
}

/*
uchar sys_Sysex[7] = { 0x41, 0x10, 0x42, 0x12, 0x40, 0x00, 0x00 };

uchar *SysExDT1( uchar aa, uchar bb, uchar cc, int datalen, uchar *data ) {

	 int length = 9 + datalen;
	 uchar *mess = new uchar[length];
	 mess[0] = 0x41;
	 mess[1] = 0x10;
	 mess[2] = 0x42;
	 mess[3] = 0x12;
	 mess[4] = aa;
   	 mess[5] = bb;
	 mess[6] = cc;
	 int i;
	 for (i = 0; i < datalen; i++)
		mess[i+7] = data[i];
	 uchar sum = 0x00;
	 for (i = 4; i < (length-2); i++)
		sum += mess[i];
   	 mess[length - 2] = (0x80 - (sum & 0x7f)) & 0x7f;
	 mess[length - 1] = 0xf7;
	 return( mess );
}
*/

static double framesPerSecond[] = { 24.0, 25.0, 30.0, 30.0 };

tMtcTime::tMtcTime( tMtcOffset *s )
{
  type = (tMtcType) ((s->Data[0] & 0x60) >> 5);
  if (type < Mtc24) type = Mtc24;
  if (type > Mtc30Ndf) type = Mtc30Ndf;
  hour = s->Data[0] & 0x1f;
  min = s->Data[1];
  sec = s->Data[2];
  fm = s->Data[3];
}

tMtcTime::tMtcTime( long millisec, tMtcType t )
{
  type = t;
  if (type < Mtc24) type = Mtc24;
  if (type > Mtc30Ndf) type = Mtc30Ndf;
  sec = millisec / 1000;
  long msec = millisec % 1000;
  min = sec / 60;
  sec = sec % 60;
  hour = min / 60;
  min = min % 60;
  double frametime = 1000.0 / framesPerSecond[ type ];
  fm = (long) ((double) msec / frametime);
}

tMtcTime::tMtcTime( char *str, tMtcType t )
{
  type = t;
  if (type < Mtc24) type = Mtc24;
  if (type > Mtc30Ndf) type = Mtc30Ndf;
  hour = 0;
  min = 0;
  sec = 0;
  fm = 0;
  sscanf( str, "%ld:%ld:%ld.%ld", &hour, &min, &sec, &fm );
  if (fm >= framesPerSecond[ type ])
    fm = (long) framesPerSecond[ type ] - 1;
}

tMtcTime::tMtcTime( unsigned h, unsigned m, unsigned s, unsigned f, unsigned t )
{
  hour = h;
  min = m;
  sec = s;
  fm = f;
  type = (tMtcType) t;
  if (type < Mtc24) type = Mtc24;
  if (type > Mtc30Ndf) type = Mtc30Ndf;
}

void tMtcTime::ToString( char *str )
{
  sprintf( str, "%ld:%ld:%ld.%ld", hour, min, sec, fm );
}

tMtcOffset *tMtcTime::ToOffset()
{
  uchar *mess = new uchar[5];
  mess[0] = (uchar) hour | ((uchar) type << 5);
  mess[1] = (uchar) min;
  mess[2] = (uchar) sec;
  mess[3] = (uchar) fm;
  mess[4] = 0x00;
  tMtcOffset *s = new tMtcOffset(0, mess, 5);
  delete mess;
  return( s );
}

long tMtcTime::ToMillisec()
{
  long msec = (((((hour * 60L) + min) * 60L) + sec) * 1000L) +
              ((fm * 1000L) / (long) framesPerSecond[type]);
  return( msec );
}

int sysex_channel( int Channel ) {
	if (Channel < 10)
		return( Channel );
	else if (Channel == 10)
		return( 0 );
	else 	return( Channel - 1 );
}

int drumParam2Index( int par )
{
	switch (par)
	{
		case drumPitch:
			return( drumPitchIndex );
		case drumTva:
			return( drumTvaIndex );
		case drumPan:
			return( drumPanIndex );
		case drumReverb:
			return( drumReverbIndex );
		case drumChorus:
			return( drumChorusIndex );
		default:
			assert( 0 );
	}
	return 0;
}

int drumIndex2Param( int index )
{
	switch (index)
	{
		case drumPitchIndex:
			return( drumPitch );
		case drumTvaIndex:
			return( drumTva );
		case drumPanIndex:
			return( drumPan );
		case drumReverbIndex:
			return( drumReverb );
		case drumChorusIndex:
			return( drumChorus );
		default:
			assert(0);
	}
	return 0;
}

tDrumInstrumentParameter::tDrumInstrumentParameter( tNrpn *par )
: pitch( par->Lsb.Value ), next(0)
{
	for (int i = drumPitchIndex; i < numDrumParameters; i++)
	{
		param[i] = 0;
	}
	param[ drumParam2Index( par->Msb.Value ) ] = par;
}

tNrpn *tDrumInstrumentParameter::Get( int index )
{
	assert( (index >= drumPitchIndex) && (index < numDrumParameters) );
	return( param[ index ] );
}

void tDrumInstrumentParameter::Put( tNrpn *par )
{
	param[ par->Lsb.Value ] = par;
}

tDrumInstrumentParameter *tDrumInstrumentParameter::Next()
{
	return( next );
}

int tDrumInstrumentParameter::Pitch()
{
	return( pitch );
}

tDrumInstrumentParameter
*tDrumInstrumentParameterList::GetElem( int pit )
{
	tDrumInstrumentParameter *ptr = list;
	while (ptr)
	{
		if (ptr->pitch == pit)
		{
			break;
		}
		ptr = ptr->next;
	}
	return( ptr );
}

tNrpn *tDrumInstrumentParameterList::GetParam( int pit, int index )
{
	tDrumInstrumentParameter *ptr = GetElem( pit );
	if (ptr)
		 return( ptr->Get( index ) );
	else
		return( 0 );
}

void tDrumInstrumentParameterList::PutParam( tNrpn *par )
{
	tDrumInstrumentParameter *ptr = GetElem( par->Lsb.Value );
	if (!ptr)
	{
		ptr = new tDrumInstrumentParameter( par );
		ptr ->next = list;
		list = ptr;
	}
	else
	{
		ptr->param[ drumParam2Index( par->Msb.Value ) ] = par;
	}
}

void tDrumInstrumentParameterList::DelParam( int pit, int index )
{
	if (list)
	{
		tDrumInstrumentParameter *elem = GetElem( pit );
		if (elem)
		{
			if (elem->Get(index))
				delete elem->param[ index ];
			elem->param[ index ] = 0;
		}
	}
}

void tDrumInstrumentParameterList::DelElem( int pit )
{
	for (int i = drumPitchIndex; i < numDrumParameters; i++)
	{
		DelParam( pit, i );
	}

	tDrumInstrumentParameter *ptr = list;
	tDrumInstrumentParameter *prev = 0;
	while (ptr)
	{
		if (ptr->pitch == pit)
		{
			if (prev)
			{
				prev->next = ptr->next;
			}
			else
			{
				list = ptr->next;
			}
			delete ptr;
			break;
		}
		prev = ptr;
		ptr = ptr->next;
	}
}

tDrumInstrumentParameter *tDrumInstrumentParameterList::FirstElem()
{
	return( list );
}

tDrumInstrumentParameter *tDrumInstrumentParameterList::NextElem( tDrumInstrumentParameter *cur )
{
	if (cur)
	{
		tDrumInstrumentParameter *ptr = GetElem( cur->pitch );
		if (ptr)
			return( ptr->next );
		else
			return( 0 );
	}
	else
	{
		return( 0 );
	}
}

void tDrumInstrumentParameterList::Clear()
{
	tDrumInstrumentParameter *ptr = list;
	while( ptr )
	{
		list = ptr->next;
		delete ptr;
		ptr = list;
	}
}


tSimpleEventArray::tSimpleEventArray()
{
  Events = 0;
  nEvents = 0;
  MaxEvents = 0;
}

tSimpleEventArray::~tSimpleEventArray()
{
  Clear();
  delete [] Events;
  Events = 0;
}

void tSimpleEventArray::Clear()
{
  int i;
#ifdef E_DBUG
  { for (int i = 0; i < nEvents; i++) Events[i]->edb(); }
#endif
  for (i = 0; i < nEvents; i++)
    delete Events[i];
  nEvents = 0;
}


void tUndoBuffer::Clear()
{
  int i;
#ifdef E_DBUG
  { for (int i = 0; i < nEvents; i++) if (bits(i)) Events[i]->edb(); }
#endif
  for (i = 0; i < nEvents; i++)
    if (bits(i))
      delete Events[i];
  nEvents = 0;
}


void tSimpleEventArray::Resize()
{
  long i;
  MaxEvents += 50;
  tEvent **tmp = new tEvent * [MaxEvents];
  for (i = 0; i < nEvents; i++)
    tmp[i] = Events[i];
  for (; i < MaxEvents; i++)
    tmp[i] = 0;
  delete [] Events;
  Events = tmp;
}

/** JAVE remove the(any) end of track event from the track
there can only be one eot in a track, so remove it before inserting a new one
*/
void tSimpleEventArray::RemoveEOT()
{
  int j=0;
  int newnEvents=nEvents;
  for (int i = 0; i < nEvents; i++) 
    {
      if(Events[i]!=0 && Events[i]->IsEndOfTrack())
	{
	  j++;
	  newnEvents--;
	}
 
      tEvent* item;
      if(j<=MaxEvents){
	item=Events[j++];
      }
      else{item=0;}
      Events[i]=item;
    }
  nEvents=newnEvents;
}

void tSimpleEventArray::Put(tEvent *e)
{
  if(e->IsEndOfTrack())
     RemoveEOT(); //JAVE remove old EOT if we are adding a new one
  if (nEvents >= MaxEvents)
    Resize();
  Events[nEvents++] = e;
  
#ifdef E_DBUG
  { for (int i = 0; i < nEvents; i++) Events[i]->edb(); }
#endif
}


/**
 * move the data from source to this. this will be cleaned up
 * first and src will be empty afterwards.
 */

void tSimpleEventArray::GrabData(tSimpleEventArray &src)
{
  Clear();
  delete [] Events;

  Events = src.Events;
  nEvents = src.nEvents;
  MaxEvents = src.MaxEvents;
  src.Events = 0;
  src.nEvents = 0;
  src.MaxEvents = 0;
}


void tSimpleEventArray::Copy(tSimpleEventArray &src, long frclk, long toclk) {
  tEventIterator iter(&src);
  tEvent *e = iter.Range(frclk, toclk);
  while (e) {
    Put(e->Copy());
    e = iter.Next();
  }
}



tEventArray::tEventArray()
{
  nEvents = 0;

  MaxEvents = 0;
  Events = 0;
  Channel = 0;
  Device = 0;
  ForceChannel = 0;
  State = tsPlay;
#ifdef AUDIO
  audio_mode = 0;
#endif

  Clear();
}


void tEventArray::Clear()
{
  int i;

  tSimpleEventArray::Clear();

  Name   = 0;
  Copyright = 0;
  Patch  = 0;
  Volume = 0;
  Pan    = 0;
  Reverb = 0;
  Chorus = 0;
  Bank   = 0;
  Bank2  = 0;
  Reset  = 0;
  Speed  = 0;
  Channel = 1;
  Device  = 0;

  for (i = 0; i < mspModulationSysexParameters; i++)
	ModulationSettings[i] = 0;

  for (i = 0; i < bspBenderSysexParameters; i++)
	BenderSettings[i] = 0;

  for (i = 0; i < cspCAfSysexParameters; i++)
	CAfSettings[i] = 0;

  for (i = 0; i < pspPAfSysexParameters; i++)
	PAfSettings[i] = 0;

  for (i = 0; i < cspCC1SysexParameters; i++)
	CC1Settings[i] = 0;

  for (i = 0; i < cspCC2SysexParameters; i++)
	CC2Settings[i] = 0;

  CC1ControllerNr = 0;
  CC2ControllerNr = 0;

  ReverbType = 0;
  ChorusType = 0;
  EqualizerType = 0;

  for (i = 0; i < rspReverbSysexParameters; i++)
	ReverbSettings[i] = 0;

  for (i = 0; i < cspChorusSysexParameters; i++)
	ChorusSettings[i] = 0;

  PartialReserve = 0;
  MasterVol = 0;
  MasterPan = 0;
  RxChannel = 0;
  UseForRhythm = 0;
  MtcOffset = 0;

  VibRate = 0;
  VibDepth = 0;
  VibDelay = 0;
  Cutoff = 0;
  Resonance = 0;
  EnvAttack = 0;
  EnvDecay = 0;
  EnvRelease = 0;
  BendPitchSens = 0;

  DrumParams.Clear();

  if (Events)
    delete [] Events;
  Events = 0;
  MaxEvents = 0;

  State = tsPlay;
#ifdef AUDIO
  audio_mode = 0;
#endif
}


tEventArray::~tEventArray()
{
  Clear();
}





static int compare(const void *p1, const void *p2)
{
  tEvent *e1 = *(tEvent **)p1;
  tEvent *e2 = *(tEvent **)p2;
  return e1->Compare(*e2);
}


void tSimpleEventArray::Sort()
{
  qsort(Events, nEvents, sizeof(tEvent *), compare);
}



void tEventArray::Cleanup(Bool dont_delete_killed_events)
{
  tEvent *e;
  tControl *c;
  tSysEx *s;
  int i;

  Sort();	// moves all killed events to the end of array

  // clear track defaults
  Name = 0;
  Copyright = 0;
  Speed = 0;
  Volume = 0;
  Pan = 0;
  Reverb = 0;
  Chorus = 0;

  for (i = 0; i < mspModulationSysexParameters; i++)
	ModulationSettings[i] = 0;

  for (i = bspBendPitchControl; i < bspBenderSysexParameters; i++)
	BenderSettings[i] = 0;

  for (i = 0; i < cspCAfSysexParameters; i++)
	CAfSettings[i] = 0;

  for (i = 0; i < pspPAfSysexParameters; i++)
	PAfSettings[i] = 0;

  for (i = 0; i < cspCC1SysexParameters; i++)
	CC1Settings[i] = 0;

  for (i = 0; i < cspCC2SysexParameters; i++)
	CC2Settings[i] = 0;

  CC1ControllerNr = 0;
  CC2ControllerNr = 0;

  ReverbType = 0;
  ChorusType = 0;
  EqualizerType = 0;

  for (i = 0; i < rspReverbSysexParameters; i++)
	ReverbSettings[i] = 0;

  for (i = 0; i < cspChorusSysexParameters; i++)
	ChorusSettings[i] = 0;

  PartialReserve = 0;
  MasterVol = 0;
  MasterPan = 0;
  RxChannel = 0;
  UseForRhythm = 0;
  MtcOffset = 0;

  for (i = 0; i < nEvents; i++)
  {
    if ((e = Events[i])->IsKilled())
    {
      if (!dont_delete_killed_events)
      {
	for (int j = i; j < nEvents; j++)
	  delete Events[j];
      }
      nEvents = i;
      break;
    }

    // accept only events having clock == 0 as track defaults
    if (e->Clock != 0)
      continue;

    if (!Name)
      Name = e->IsTrackName();
    if (!Copyright)
      Copyright = e->IsCopyright();
    if (!Speed)
      Speed = e->IsSetTempo();
    if (!MtcOffset)
      MtcOffset = e->IsMtcOffset();
    if ((c = e->IsControl()) != 0)
    {
      switch (c->Control)
      {
        case 0x07: if (!Volume) Volume = c; break;
        case 0x0a: if (!Pan)    Pan    = c; break;
        case 0x5b: if (!Reverb) Reverb = c; break;
        case 0x5d: if (!Chorus) Chorus = c; break;
      }
    }
    if ((s = e->IsSysEx()) != 0) {

       int sxid = Synth->GetSysexId( s );

       if (!Synth->IsGS())
       {
	  switch (sxid)
	  {
	   case SX_GM_MasterVol:
	      // GS has its own; SC-55 doesn't recognize GM Mastervol
	      MasterVol = s;
	      break;
	   default:
	      break;
	  }
       }

       if (Synth->IsGS())
       {
	  switch (sxid)
	  {
	   case SX_GS_MasterVol:
	      MasterVol = s;
	      break;
	   case SX_GS_MasterPan:
	      MasterPan = s;
	      break;
	   case SX_GS_BendPitch:
	   case SX_GS_BendTvf:
	   case SX_GS_BendAmpl:
	   case SX_GS_BendLfo1Rate:
	   case SX_GS_BendLfo1Pitch:
	   case SX_GS_BendLfo1Tvf:
	   case SX_GS_BendLfo1Tva:
	   case SX_GS_BendLfo2Rate:
	   case SX_GS_BendLfo2Pitch:
	   case SX_GS_BendLfo2Tvf:
	   case SX_GS_BendLfo2Tva:
	      BenderSettings[sxid - SX_GS_BendPitch] = s;
	      break;

	   case SX_GS_ModPitch:
	   case SX_GS_ModTvf:
	   case SX_GS_ModAmpl:
	   case SX_GS_ModLfo1Rate:
	   case SX_GS_ModLfo1Pitch:
	   case SX_GS_ModLfo1Tvf:
	   case SX_GS_ModLfo1Tva:
	   case SX_GS_ModLfo2Rate:
	   case SX_GS_ModLfo2Pitch:
	   case SX_GS_ModLfo2Tvf:
	   case SX_GS_ModLfo2Tva:
	      ModulationSettings[sxid - SX_GS_ModPitch] = s;
	      break;

	   case SX_GS_CafPitch:
	   case SX_GS_CafTvf:
	   case SX_GS_CafAmpl:
	   case SX_GS_CafLfo1Rate:
	   case SX_GS_CafLfo1Pitch:
	   case SX_GS_CafLfo1Tvf:
	   case SX_GS_CafLfo1Tva:
	   case SX_GS_CafLfo2Rate:
	   case SX_GS_CafLfo2Pitch:
	   case SX_GS_CafLfo2Tvf:
	   case SX_GS_CafLfo2Tva:
	      CAfSettings[sxid - SX_GS_CafPitch] = s;
	      break;

	   case SX_GS_PafPitch:
	   case SX_GS_PafTvf:
	   case SX_GS_PafAmpl:
	   case SX_GS_PafLfo1Rate:
	   case SX_GS_PafLfo1Pitch:
	   case SX_GS_PafLfo1Tvf:
	   case SX_GS_PafLfo1Tva:
	   case SX_GS_PafLfo2Rate:
	   case SX_GS_PafLfo2Pitch:
	   case SX_GS_PafLfo2Tvf:
	   case SX_GS_PafLfo2Tva:
	      PAfSettings[sxid - SX_GS_PafPitch] = s;
	      break;

	   case SX_GS_CC1Pitch:
	   case SX_GS_CC1Tvf:
	   case SX_GS_CC1Ampl:
	   case SX_GS_CC1Lfo1Rate:
	   case SX_GS_CC1Lfo1Pitch:
	   case SX_GS_CC1Lfo1Tvf:
	   case SX_GS_CC1Lfo1Tva:
	   case SX_GS_CC1Lfo2Rate:
	   case SX_GS_CC1Lfo2Pitch:
	   case SX_GS_CC1Lfo2Tvf:
	   case SX_GS_CC1Lfo2Tva:
	      CC1Settings[sxid - SX_GS_CC1Pitch] = s;
	      break;

	   case SX_GS_CC2Pitch:
	   case SX_GS_CC2Tvf:
	   case SX_GS_CC2Ampl:
	   case SX_GS_CC2Lfo1Rate:
	   case SX_GS_CC2Lfo1Pitch:
	   case SX_GS_CC2Lfo1Tvf:
	   case SX_GS_CC2Lfo1Tva:
	   case SX_GS_CC2Lfo2Rate:
	   case SX_GS_CC2Lfo2Pitch:
	   case SX_GS_CC2Lfo2Tvf:
	   case SX_GS_CC2Lfo2Tva:
	      CC2Settings[sxid - SX_GS_CC2Pitch] = s;
	      break;

	   case SX_GS_ReverbMacro:
	      ReverbType = s;
	      break;

	   case SX_GS_RevCharacter:
	   case SX_GS_RevPreLpf:
	   case SX_GS_RevLevel:
	   case SX_GS_RevTime:
	   case SX_GS_RevDelayFeedback:
	   case SX_GS_RevSendChorus:
	      ReverbSettings[ sxid - SX_GS_RevCharacter] = s;
	      break;

	   case SX_GS_ChorusMacro:
	      ChorusType = s;
	      break;

	   case SX_GS_ChoPreLpf:
	   case SX_GS_ChoLevel:
	   case SX_GS_ChoFeedback:
	   case SX_GS_ChoDelay:
	   case SX_GS_ChoRate:
	   case SX_GS_ChoDepth:
	   case SX_GS_ChoSendReverb:
	      ChorusSettings[ sxid - SX_GS_ChoPreLpf] = s;
	      break;

	   case SX_GS_CC1CtrlNo:
	      CC1ControllerNr = s;
	      break;

	   case SX_GS_CC2CtrlNo:
	      CC2ControllerNr = s;
	      break;

	   case SX_GS_PartialReserve:
	      PartialReserve = s;
	      break;

	   case SX_GS_RxChannel:
	      RxChannel = s;
	      break;

	   case SX_GS_UseForRhythm:
	      UseForRhythm = s;
	      break;

	   default:
	      break;
	  }
       }
       else if (Synth->IsXG())
       {
	  switch (sxid)
	  {
	   case SX_XG_BendPitch:
	   case SX_XG_BendTvf:
	   case SX_XG_BendAmpl:
	      BenderSettings[ sxid - SX_XG_BendPitch] = s;
	      break;

	   case SX_XG_BendLfoPitch:
	   case SX_XG_BendLfoTvf:
	   case SX_XG_BendLfoTva:
	      BenderSettings[ sxid + 1 - SX_XG_BendPitch] = s;
	      break;

	   case SX_XG_ModPitch:
	   case SX_XG_ModTvf:
	   case SX_XG_ModAmpl:
	      ModulationSettings[ sxid - SX_XG_ModPitch] = s;
	      break;

	   case SX_XG_ModLfoPitch:
	   case SX_XG_ModLfoTvf:
	   case SX_XG_ModLfoTva:
	      ModulationSettings[ sxid + 1 - SX_XG_ModPitch] = s;
	      break;

	   case SX_XG_CafPitch:
	   case SX_XG_CafTvf:
	   case SX_XG_CafAmpl:
	      CAfSettings[ sxid - SX_XG_CafPitch] = s;
	      break;

	   case SX_XG_CafLfoPitch:
	   case SX_XG_CafLfoTvf:
	   case SX_XG_CafLfoTva:
	      CAfSettings[ sxid + 1 - SX_XG_CafPitch] = s;
	      break;

	   case SX_XG_PafPitch:
	   case SX_XG_PafTvf:
	   case SX_XG_PafAmpl:
	      PAfSettings[ sxid - SX_XG_PafPitch] = s;
	      break;

	   case SX_XG_PafLfoPitch:
	   case SX_XG_PafLfoTvf:
	   case SX_XG_PafLfoTva:
	      PAfSettings[ sxid + 1 - SX_XG_PafPitch] = s;
	      break;

	   case SX_XG_CC1Pitch:
	   case SX_XG_CC1Tvf:
	   case SX_XG_CC1Ampl:
	      CC1Settings[ sxid - SX_XG_CC1Pitch] = s;
	      break;

	   case SX_XG_CC1LfoPitch:
	   case SX_XG_CC1LfoTvf:
	   case SX_XG_CC1LfoTva:
	      CC1Settings[ sxid + 1 - SX_XG_CC1Pitch] = s;
	      break;

	   case SX_XG_CC2Pitch:
	   case SX_XG_CC2Tvf:
	   case SX_XG_CC2Ampl:
	      CC2Settings[ sxid - SX_XG_CC2Pitch] = s;
	      break;

	   case SX_XG_CC2LfoPitch:
	   case SX_XG_CC2LfoTvf:
	   case SX_XG_CC2LfoTva:
	      CC2Settings[ sxid + 1 - SX_XG_CC2Pitch] = s;
	      break;

	   case SX_XG_ReverbMacro:
	      ReverbType = s;
	      break;

	   case SX_XG_ChorusMacro:
	      ChorusType = s;
	      break;

	   case SX_XG_EqualizerMacro:
	      EqualizerType = s;
	      break;

	   case SX_XG_CC1CtrlNo:
	      CC1ControllerNr = s;
	      break;

	   case SX_XG_CC2CtrlNo:
	      CC2ControllerNr = s;
	      break;

	   case SX_XG_RxChannel:
	      RxChannel = s;
	      break;

	   case SX_XG_UseForRhythm:
	      UseForRhythm = s;
	      break;

	   default:
	      break;
	  }
       }
    }
  }
}


void tEventArray::Length2Keyoff()
{
  int n = nEvents;
  for (int i = 0; i < n; i++)
  {
    tKeyOn *on;
    if ((on = Events[i]->IsKeyOn()) != 0 && on->Length != 0)
    {
//      tEvent *of = new tKeyOff(on->Clock + on->Length, on->Channel, on->Key);
      // SN++ added off veloc
      tEvent *of = new tKeyOff(on->Clock + on->Length, on->Channel,
                              on->Key,on->OffVeloc);

      on->Length = 0;
      of->SetDevice(on->GetDevice());
      Put(of);
    }
  }
  Sort();
}


#if 0

void tEventArray::Keyoff2Length()
{
  int i;
  for (i = 1; i < nEvents; i++)
  {
    tKeyOff *of;
    if ((of = Events[i]->IsKeyOff()) != 0)
    {
      tEvent **e = &Events[i - 1];
      while (e >= Events)
      {
        tKeyOn *on = (*e)->IsKeyOn();
        if (on && on->Key == of->Key && on->Channel == of->Channel && on->Length == 0)
        {
          on->Length = of->Clock - on->Clock;
          if (on->Length <= 0L)
            on->Length = 1;
          of->Kill();
	  break;
	}
	-- e;
      }
    }
  }

  // kill all KeyOn's with non matching KeyOff's
  for (i = 0; i < nEvents; i++)
  {
    tKeyOn *k = Events[i]->IsKeyOn();
    if (k && k->Length <= 0)
      k->Kill();
  }
  Cleanup(0);
}

#else

void tEventArray::Keyoff2Length()
{
  /* searches forward from a KeyOn to find the matching KeyOff.
     This is compatible with Cubase */
  int i;
  for (i = 0; i < nEvents; i++)
  {
    tKeyOn *on;
    if ((on = Events[i]->IsKeyOn()) != 0 && on->Length == 0)
    {
      int j;
      for (j = i + 1; j < nEvents; j++)
      {
        tKeyOff *of = Events[j]->IsKeyOff();
        if (of && !of->IsKilled() && on->Key == of->Key && on->Channel == of->Channel)
        {
          on->Length = of->Clock - on->Clock;
          if (on->Length <= 0L)
            on->Length = 1;
          of->Kill();
	  break;
	}
      }
    }
  }

  // kill all KeyOn's with non matching KeyOff's
  // and kill all remaining KeyOff's
  for (i = 0; i < nEvents; i++)
  {
    tKeyOn *on = Events[i]->IsKeyOn();
    if (on && on->Length <= 0)
      on->Kill();
    tKeyOff *of = Events[i]->IsKeyOff();
    if (of)
      of->Kill();
  }
  Cleanup(0);
}

#endif


void tEventArray::Write(tWriteBase &io)
{
  tEvent *e;
  int WrittenBefore;

  Length2Keyoff();
  io.NextTrack();

  // Write copyright notice first (according to spec):
  if (Copyright) Copyright->Write(io);

  // Write MTC offset before any transmittable events (spec)
  if (MtcOffset) MtcOffset->Write(io);

  // Synth reset
  if (Reset) Reset->Write(io);

  // Rpn / Nrpn:
  // All these must be written in order (three tControl's in a row)
  if (VibRate) VibRate->Write(io);
  if (VibDepth) VibDepth->Write(io);
  if (VibDelay) VibDelay->Write(io);
  if (Cutoff) Cutoff->Write(io);
  if (Resonance) Resonance->Write(io);
  if (EnvAttack) EnvAttack->Write(io);
  if (EnvDecay) EnvDecay->Write(io);
  if (EnvRelease) EnvRelease->Write(io);
  if (BendPitchSens) BendPitchSens->Write(io);

  tDrumInstrumentParameter *dpar = DrumParams.FirstElem();
  while ( dpar ) {
	int index;
	for (index = drumPitchIndex; index < numDrumParameters; index++)
	{
		if (dpar->Get(index))
			dpar->Get(index)->Write( io );
	}
	dpar = DrumParams.NextElem( dpar );
  }

  // Bank: Must be sure bank is written before program:
  if (Bank) Bank->Write(io);
  if (Bank2) Bank2->Write(io);
  if (Patch) Patch->Write(io);

  // write jazz track info
  tJazzMeta jazz;
#ifdef AUDIO
  jazz.SetAudioMode(audio_mode);
#endif
  jazz.SetTrackState(State);
  jazz.SetTrackDevice(Device);
  jazz.SetIntroLength(TheSong->GetIntroLength());
  jazz.Write(io);

  for (int i = 0; i < nEvents; i++) {
	e = Events[i];
	WrittenBefore = 0;
	if (e->IsControl()) {
		switch (e->IsControl()->Control) {
			// Don't write these again if present as events
			// and clock == 0 (should not happen)
			case 0x65: // Rpn Msb
			case 0x64: // Rpn Lsb
			case 0x63: // Nrpn Msb
			case 0x62: // Nrpn Lsb
			case 0x06: // Rpn/Nrpn Data
			case 0x00: // Bank
			case 0x20: // Bank2
				if (e->Clock == 0)
					WrittenBefore = 1;
				break;
			default:
				WrittenBefore = 0;
		}
	}
	else if (e->IsProgram()) {
		// Don't write these again if present as events
		// and clock == 0 (should not happen)
		if (e->Clock == 0)
			WrittenBefore = 1;
	}
	else if (e->IsCopyright() || e->IsMtcOffset()) {
		// Will probably happen
		WrittenBefore = 1;
	}
	if (!WrittenBefore) {
	    	e->Write(io);
	}
  }
  Keyoff2Length();
}



void tEventArray::Read(tReadBase &io)
{
  tEvent *e;
  Channel = 0;
  uchar Msb, Lsb, Data;
  int SpecialEvent;

  Msb = Lsb = Data = 0xff;
  int cha;

  io.NextTrack();
  while ( ((e = io.Read()) != 0) )
  {

    SpecialEvent = 0;
    if (e->IsJazzMeta()) {
      tJazzMeta *j = e->IsJazzMeta();
      audio_mode = (int)j->GetAudioMode();
      State      = (int)j->GetTrackState();
      Device     = (int)j->GetTrackDevice();
      TheSong->SetIntroLength((int)j->GetIntroLength());
      delete j;
      continue;
    }
    if (e->IsControl()) {
	switch (e->IsControl()->Control) {
		// Grab Rpn/Nrpn/Bank from file and save them, don't put
		// them into event-array
		case 0x63:
		case 0x65:
			Msb = e->IsControl()->Value; // Rpn/Nrpn Msb
			SpecialEvent = 1;
			break;
		case 0x62:
		case 0x64:
			Lsb = e->IsControl()->Value; // Rpn/Nrpn Lsb
			SpecialEvent = 1;
			break;
		case 0x06:
			Data = e->IsControl()->Value; // Rpn/Nrpn Data
			SpecialEvent = 1;
			cha = e->IsControl()->Channel;
			switch (Msb) {
				case 0x01: // Nrpn
					switch (Lsb) {
						case 0x08:
							if (!VibRate)
							  VibRate = new tNrpn( 0, cha, Msb, Lsb, Data );
							break;
						case 0x09:
							if (!VibDepth)
							  VibDepth = new tNrpn( 0, cha, Msb, Lsb, Data );
							break;
						case 0x0a:
							if (!VibDelay)
							  VibDelay = new tNrpn( 0, cha, Msb, Lsb, Data );
							break;
						case 0x20:
							if (!Cutoff)
							  Cutoff = new tNrpn( 0, cha, Msb, Lsb, Data );
							break;
						case 0x21:
							if (!Resonance)
							  Resonance = new tNrpn( 0, cha, Msb, Lsb, Data );
							break;
						case 0x63:
							if (!EnvAttack)
							  EnvAttack = new tNrpn( 0, cha, Msb, Lsb, Data );
							break;
						case 0x64:
							if (!EnvDecay)
							  EnvDecay = new tNrpn( 0, cha, Msb, Lsb, Data );
							break;
						case 0x66:
							if (!EnvRelease)
							  EnvRelease = new tNrpn( 0, cha, Msb, Lsb, Data );
							break;
						default:
							break;
					}
					break;
				case drumPitch:
				case drumTva:
				case drumPan:
				case drumReverb:
				case drumChorus:
					DrumParams.PutParam( new tNrpn( 0, cha, Msb, Lsb, Data ) );
					break;
				case 0x00: // Rpn
					if (Lsb == 0x00) { // Pitch Bend Sensivity
						if (!BendPitchSens)
						  BendPitchSens = new tRpn( 0, cha, Msb, Lsb, Data );
					}
					break;
				default:
					break;
			}
  			Msb = Lsb = Data = 0xff;
			break;
		case 0x00:
                        if (!Bank)
                        {
			  SpecialEvent = 1;
			  Bank = e->IsControl(); // Bank
                          Bank->Clock = 0;
                        }
			break;
		case 0x20:
                        if (!Bank2)
                        {
			  SpecialEvent = 1;
			  Bank2 = e->IsControl(); // Bank
                          Bank2->Clock = 0;
                        }
			break;
		default:
			SpecialEvent = 0; // Other control
			break;
	}
    }
    else if (e->IsProgram()) {
                if (!Patch)
                {
		  Patch = e->IsProgram();
                  Patch->Clock = 0;
		  SpecialEvent = 1;
                }
    }
    else if (e->IsCopyright()) {
		if (!Copyright)
		{
		  Copyright = e->IsCopyright();
		  // Just make sure clock is zero, then put into event array
		  Copyright->Clock = 0;
		}
    }
    else if (e->IsSysEx())
    {
       // Get hold of the Reset sysex...
       int sxid = Synth->GetSysexId( e->IsSysEx() );

       if ((sxid == SX_GM_ON) ||
           (sxid == SX_GS_ON) ||
           (sxid == SX_XG_ON))
       {
          // Take them all away
	  SpecialEvent = 1;

          // Save it in the track defaults if it fits with synth
          // type settings
          if ((Synth->IsGM() && (sxid == SX_GM_ON)) ||
	      (Synth->IsGS() && (sxid == SX_GS_ON)) ||
	      (Synth->IsXG() && (sxid == SX_XG_ON)))
          {
	     if (!Reset)
	     {
	        Reset = e->IsSysEx();
	     }
          }
       }
    }

    if (!SpecialEvent) {
	    Put(e);
	    if (!Channel && e->IsChannelEvent())
	      Channel = e->IsChannelEvent()->Channel + 1;
    }
    if (e->IsEndOfTrack()){  //JAVE I want explicit end of track events
      break; //break out of loop here because endoftrack is end, and we want it read FIXME we shoulnt break, we should keep on reading, and handle eot in track play instead
    }
  } // while read

  if (!Channel)
    Channel = 1;

  Keyoff2Length();
}


long tEventArray::GetLastClock()
{
  if (!nEvents)
    return 0;
  return Events[nEvents-1]->Clock;
}

int tEventArray::IsEmpty()
{
  return nEvents == 0;
}

long tEventArray::GetFirstClock()
{
  if (nEvents)
    return Events[0]->Clock;
  return LastClock;
}

// ***********************************************************************
// Dialog
// ***********************************************************************
#ifndef __PORTING

class tTrackDlg : public wxForm
{
  tTrackWin *TrackWin;
  tTrack *trk;
  char *TrackName;
  tNamedChoice PatchChoice;
  tNamedChoice DeviceChoice;
  long PatchNr;
  long Device;
  long BankNr;
  int ClearTrack;
#ifdef AUDIO
  int AudioMode;
#endif

 public:
  tTrackDlg::tTrackDlg(tTrackWin *w, tTrack *t);
  void EditForm(wxPanel *panel);
  virtual void OnOk();
  virtual void OnCancel();
  virtual void OnHelp();
};


tTrackDlg::tTrackDlg(tTrackWin *w, tTrack *t)
  : wxForm( USED_WXFORM_BUTTONS ),
    PatchChoice("Patch", t->IsDrumTrack() ? &Config.DrumSet(0) : &Config.VoiceName(0), &PatchNr),
    DeviceChoice("Device", Midi->GetOutputDevices().AsNamedValue(), &Device)
{
  TrackWin = w;
  trk = t;
}


void tTrackDlg::OnCancel()
{
  trk->DialogBox = 0;
  TrackWin->Redraw();
  wxForm::OnCancel();
}

void tTrackDlg::OnHelp()
{
  HelpInstance->ShowTopic("Trackname, midi channel etc");
}


void tTrackDlg::OnOk()
{
  trk->DialogBox->GetPosition( &Config(C_TrackDlgXpos), &Config(C_TrackDlgYpos) );
  trk->DialogBox = 0;
#ifdef AUDIO
  trk->SetAudioMode(AudioMode);
#endif
  if (ClearTrack) {
	trk->Clear();
  	delete TrackName;
        TrackWin->Redraw();
  	wxForm::OnOk();
	return;
  }
  trk->SetName(TrackName);
  delete TrackName;
  PatchChoice.GetValue();
  DeviceChoice.GetValue();
  BankNr = (PatchNr & 0x0000ff00) >> 8;
  PatchNr = PatchNr & 0x000000ff;
  trk->SetBank(BankNr);
  trk->SetPatch(PatchNr);
  trk->SetDevice(Device);
  if (trk->ForceChannel) {
     tChannelEvent *c;
     tSysEx *s;
     tEventIterator Iterator(trk);
     trk->Sort();
     tEvent *e = Iterator.Range(0, (long unsigned) trk->GetLastClock() + 1);
     while (e) {
	if ((c = e->IsChannelEvent()) != 0) {
	   c = (tChannelEvent *)e->Copy();
	   c->Channel = trk->Channel - 1;
	   trk->Kill(e);
	   trk->Put(c);
	}
	else if ((s = e->IsSysEx()) != 0) {
	   // Check for sysex that contains channel number
	   uchar *chaptr = Synth->GetSysexChaPtr( s );
	   if (chaptr)
	   {
	      if (Synth->IsXG())
	      {
		 *chaptr = trk->Channel - 1;
	      }
	      else
	      {
		 *chaptr &= 0xf0;
		 *chaptr |= sysex_channel(trk->Channel);
	      }

	      s = (tSysEx *) e->Copy();
	      trk->Kill(e);
	      trk->Put(s);
	   }
	}
	e = Iterator.Next();
     } // while e
     if (trk->VibRate) trk->VibRate->SetCha( trk->Channel - 1 );
     if (trk->VibDepth) trk->VibDepth->SetCha( trk->Channel - 1 );
     if (trk->VibDelay) trk->VibDelay->SetCha( trk->Channel - 1 );
     if (trk->Cutoff) trk->Cutoff->SetCha( trk->Channel - 1 );
     if (trk->Resonance) trk->Resonance->SetCha( trk->Channel - 1 );
     if (trk->EnvAttack) trk->EnvAttack->SetCha( trk->Channel - 1 );
     if (trk->EnvDecay) trk->EnvDecay->SetCha( trk->Channel - 1 );
     if (trk->EnvRelease) trk->EnvRelease->SetCha( trk->Channel - 1 );
     if (trk->BendPitchSens) trk->BendPitchSens->SetCha( trk->Channel - 1 );
     if (trk->Bank) trk->Bank->Channel = trk->Channel - 1;
     if (trk->Patch) trk->Patch->Channel = trk->Channel - 1;
     if (!trk->DrumParams.IsEmpty())
     {
	tDrumInstrumentParameter *dpar = trk->DrumParams.FirstElem();
	while ( dpar ) {
	   int index;
	   for (index = drumPitchIndex; index < numDrumParameters; index++)
	   {
	      if (dpar->Get(index))
		 dpar->Get(index)->SetCha( trk->Channel - 1 );
	   }
	   dpar = trk->DrumParams.NextElem( dpar );
	}
     }
     trk->Cleanup();
  }
  TrackWin->Canvas->Refresh();
  wxForm::OnOk();
}



void tTrackDlg::EditForm(wxPanel *panel)
{
  PatchNr   = trk->GetPatch() + (trk->GetBank() << 8);
  Device    = trk->GetDevice();
  TrackName = copystring(trk->GetName());
  Add(wxMakeFormString("Trackname:", &TrackName, wxFORM_DEFAULT, NULL, NULL, wxVERTICAL, 300 ));

  Add(wxMakeFormNewLine());
  Add(PatchChoice.mkFormItem(300, 200));
  Add(wxMakeFormNewLine());
  {
    char buf[500];
    sprintf(buf, "Set Channel to %d to make a drum track", Config(C_DrumChannel));
    Add(wxMakeFormMessage(buf));
    Add(wxMakeFormNewLine());
  }
  Add(wxMakeFormShort("Channel", &trk->Channel, wxFORM_DEFAULT,
                       new wxList(wxMakeConstraintRange(1.0, 16.0), 0)));
#ifdef AUDIO
  AudioMode = trk->GetAudioMode();
  Add(wxMakeFormNewLine());
  Add(wxMakeFormBool("Audio Track", &AudioMode));
#endif
  Add(wxMakeFormNewLine());
  Add(wxMakeFormBool("Force channel number onto all events on track", &trk->ForceChannel));
  ClearTrack = 0;
  Add(wxMakeFormNewLine());
  Add(wxMakeFormBool("Clear track (NB! erase all events, name etc...)", &ClearTrack));

  if (Midi->SupportsMultipleDevices()) {
    Add(wxMakeFormNewLine());
    Add(DeviceChoice.mkFormItem(300, 50));
  }
  AssociatePanel(panel);
}
#endif // __PORTING

void tTrack::Dialog(tTrackWin *parent)
{
#ifndef __PORTING 

  tTrackDlg *dlg;
  if (DialogBox)
  {
    DialogBox->Show(TRUE);
    return;
  }
  #ifdef wx_msw
  Bool modal = TRUE;  // keep button down
  #else
  Bool modal = FALSE;
  #endif
  DialogBox = new wxDialogBox(parent, "Track Settings", modal, Config(C_TrackDlgXpos), Config(C_TrackDlgYpos) );
  dlg = new tTrackDlg( (tTrackWin*) parent, this);
  dlg->EditForm(DialogBox);
  DialogBox->Fit();
  DialogBox->Show(TRUE);
#endif // __PORTING
}



// ***********************************************************************
// tTrack
// ***********************************************************************


int tTrack::changed = 0;

tTrack::tTrack()
  : tEventArray()
{
  iUndo = 0;
  nRedo = 0;
  nUndo = 0;
  DialogBox = 0;
  PatchNames = 0;
  ForceChannel = 1;
}


Bool tTrack::IsDrumTrack()
{
  return Channel == Config(C_DrumChannel);
}

void tTrack::Merge(tEventArray *t)
{
  for (int i = 0; i < t->nEvents; i++)
    Put(t->Events[i]);
  t->nEvents = 0;
}


void tTrack::MergeRange(tEventArray *other, long FromClock, long ToClock, int Replace)
{
  // Erase destin
  if (Replace)
  {
    tEventIterator Erase(this);
    tEvent *e = Erase.Range(FromClock, ToClock);
    while (e)
    {
      Kill(e);
      e = Erase.Next();
    }
  }

  // Merge Recorded Events
  tEventIterator Copy(other);
  tEvent *e = Copy.Range(FromClock, ToClock);
  while (e)
  {
    tEvent *c = e->Copy();
    if (ForceChannel)
    {
      tChannelEvent *k = c->IsChannelEvent();
      if (k)
        k->Channel = Channel - 1;
    }
    Put(c);
    e = Copy.Next();
  }
  Cleanup();
}


void tTrack::Cleanup()
{
#ifdef AUDIO
  // on audio tracks, adjust length of keyon events to
  // actual sample length
  Midi->AdjustAudioLength(this);
#endif
  tEventArray::Cleanup(TRUE);
}




void tTrack::Undo()
{
  if (nUndo > 0)
  {
    tUndoBuffer *undo = &UndoBuffers[iUndo];
    for (int i = undo->nEvents - 1; i >= 0; i--)
    {
      tEvent *e = undo->Events[i];
      if (undo->bits(i))
      {
	undo->bits.set(i, 0);
        e->UnKill();
        tEventArray::Put(e);
      }
      else
      {
	undo->bits.set(i, 1);
        e->Kill();
      }
    }
    tEventArray::Cleanup(TRUE);

    iUndo = (iUndo - 1 + MaxUndo) % MaxUndo;
    nUndo--;
    nRedo++;
  }
}

void tTrack::Redo()
{
  if (nRedo > 0)
  {
    iUndo = (iUndo + 1) % MaxUndo;

    tUndoBuffer *undo = &UndoBuffers[iUndo];
    for (int i = 0; i < undo->nEvents; i++)
    {
      tEvent *e = undo->Events[i];
      if (undo->bits(i))
      {
	undo->bits.set(i, 0);
        e->UnKill();
        tEventArray::Put(e);
      }
      else
      {
	undo->bits.set(i, 1);
        e->Kill();
      }
    }
    tEventArray::Cleanup(TRUE);

    nRedo--;
    nUndo++;
  }
}


void tTrack::NewUndoBuffer()
{
  nRedo = 0;
  nUndo++;
  if (nUndo > MaxUndo)
    nUndo = MaxUndo;

  iUndo = (iUndo + 1) % MaxUndo;
  UndoBuffers[iUndo].Clear();
};


void tTrack::Clear()
{
  for (int i = 0; i < MaxUndo; i++)
    UndoBuffers[i].Clear();
  State  = tsPlay;
  tEventArray::Clear();
}

// ----------------------- Copyright ------------------------------------

char *tTrack::GetCopyright()
{
  if (Copyright)
    return (char *)Copyright->Data;
  return "";
}



void tTrack::SetCopyright(char *str)
{
  if (Copyright)
    Kill(Copyright);
  if (str && strlen(str))
    {
      int len = 127;
      if ((int)strlen(str) < len)
        len = strlen( str );
      Put(new tCopyright(0, (uchar *)str, len));
    }
  Cleanup();
}

// ----------------------- Name ------------------------------------

char *tTrack::GetName()
{
  if (Name)
    return (char *)Name->Data;
  return "";
}



void tTrack::SetName(char *str)
{
  if (Name)
    Kill(Name);
  if (strlen(str))
    Put(new tTrackName(0, (uchar *)str, strlen(str)));
  Cleanup();
}

// ------------------------  Volume ------------------------------

int tTrack::GetVolume()
{
  if (Volume)
    return Volume->Value + 1;
  return 0;
}

void tTrack::SetVolume(int Value)
{
  if (Volume)
    Kill(Volume);
  if (Value > 0)
  {
    tEvent *e = new tControl(0, Channel - 1, 0x07, Value - 1);
    Put(e);
    Midi->OutNow(this, e);
  }
  Cleanup();
}

// ------------------------  Pan ------------------------------

int tTrack::GetPan()
{
  if (Pan)
    return Pan->Value + 1;
  return 0;
}

void tTrack::SetPan(int Value)
{
  if (Pan)
    Kill(Pan);
  if (Value > 0)
  {
    tEvent *e = new tControl(0, Channel - 1, 0x0a, Value - 1);
    Put(e);
    Midi->OutNow(this, e);
  }
  Cleanup();
}

// ------------------------  Reverb ------------------------------

int tTrack::GetReverb()
{
  if (Reverb)
    return Reverb->Value + 1;
  return 0;
}

void tTrack::SetReverb(int Value)
{
  if (Reverb)
    Kill(Reverb);
  if (Value > 0)
  {
    tEvent *e = new tControl(0, Channel - 1, 0x5B, Value - 1);
    Put(e);
    Midi->OutNow(this, e);
  }
  Cleanup();
}

// ------------------------  Chorus ------------------------------

int tTrack::GetChorus()
{
  if (Chorus)
    return Chorus->Value + 1;
  return 0;
}

void tTrack::SetChorus(int Value)
{
  if (Chorus)
    Kill(Chorus);
  if (Value > 0)
  {
    tEvent *e = new tControl(0, Channel - 1, 0x5D, Value - 1);
    Put(e);
    Midi->OutNow(this, e);
  }
  Cleanup();
}

// ------------------------  Bank ------------------------------

int tTrack::GetBank()
{
  if (!Config(C_UseTwoCommandBankSelect))
  {
    DEBUG(fprintf(stderr, "Get single bank select command\n");)
    if (Bank)
    {
      DEBUG(fprintf(stderr,"Bank %d selected.\n\n",Bank->Value);)
      return Bank->Value;
    }
    else
      return 0;
  }
  DEBUG(fprintf(stderr, "Get double bank select command.\n");)
  if (Bank && Bank2)
  {
    for (int i=0; Config.BankEntry(i).Command[0]>=0; i++)
      if (Config.BankEntry(i).Command[0]==Bank->Value &&
          Config.BankEntry(i).Command[1]==Bank2->Value)
      {
          DEBUG(fprintf(stderr,"Bank %d selected.\n\n",i);)
          return i;
      }
  }
  return 0;
}

void tTrack::SetBank(int Value)
{
  if (Bank) {
	delete Bank;
	Bank = 0;
  }
  if (Bank2) {
      delete Bank2;
      Bank2 = 0;
  }
  if (Value >= 0)
  {
    if (!Config(C_UseTwoCommandBankSelect))
    {
      DEBUG(fprintf (stderr, "Single command bank select (Bank %d).\n",
            Value);)
      Bank = new tControl(0, Channel - 1, Config(C_BankControlNumber), Value);
      Midi->OutNow(this, Bank);
      return;
    }
    while (Config.BankEntry(Value).Command[0]<0 && Value>0)
      Value--;
    assert(Config.BankEntry(Value).Command[0]>=0);
    DEBUG(fprintf (stderr, "Double command bank select (Bank %d).\n",Value);)
    Bank  = new tControl(0, Channel - 1, Config(C_BankControlNumber),
                        Config.BankEntry(Value).Command[0]);
    Midi->OutNow(this, Bank);
    DEBUG(fprintf (stderr, "First bank select command: %d %d\n",
      Bank->Control, Bank->Value);
    )
    Bank2 = new tControl(0, Channel - 1, Config(C_BankControlNumber2),
                        Config.BankEntry(Value).Command[1]);
    Midi->OutNow(this, Bank2);
    DEBUG(fprintf (stderr, "Second bank select command: %d %d\n\n",
      Bank2->Control, Bank2->Value);
    )
    changed = 1;
  }
}

// ------------------------  Patch ------------------------------

int tTrack::GetPatch()
{
  if (Patch)
    return Patch->Program + 1;
  return 0;
}

void tTrack::SetPatch(int PatchNr)
{
  if (Patch) {
    delete Patch;
    Patch = 0;
  }
  if (PatchNr > 0)
  {
    Patch = new tProgram(0, Channel - 1, PatchNr - 1);
    Midi->OutNow(this, Patch);
    changed = 1;
  }
}

// ------------------------  VibRate ------------------------------

int tTrack::GetVibRate()
{
  if (VibRate)
    return VibRate->GetVal() + 1;
  return 0;
}

void tTrack::SetVibRate(int Value)
{
  if (VibRate) {
    delete VibRate;
    VibRate = 0;
  }
  if (Value > 0)
  {
    VibRate = new tNrpn( 0, Channel - 1, 0x01, 0x08, Value - 1 );
    Midi->OutNow(this, VibRate);
    changed = 1;
  }
}

// ------------------------  VibDepth ------------------------------

int tTrack::GetVibDepth()
{
  if (VibDepth)
    return VibDepth->GetVal() + 1;
  return 0;
}

void tTrack::SetVibDepth(int Value)
{
  if (VibDepth) {
    delete VibDepth;
    VibDepth = 0;
  }
  if (Value > 0)
  {
	VibDepth = new tNrpn( 0, Channel - 1, 0x01, 0x09, Value - 1 );
	Midi->OutNow(this,  VibDepth );
        changed = 1;
  }
}

// ------------------------  VibDelay ------------------------------

int tTrack::GetVibDelay()
{
  if (VibDelay)
    return VibDelay->GetVal() + 1;
  return 0;
}

void tTrack::SetVibDelay(int Value)
{
  if (VibDelay) {
    delete VibDelay;
    VibDelay = 0;
  }
  if (Value > 0)
  {
	VibDelay = new tNrpn( 0, Channel - 1, 0x01, 0x0a, Value - 1 );
	Midi->OutNow(this,  VibDelay );
        changed = 1;
  }
}

// ------------------------  Cutoff ------------------------------

int tTrack::GetCutoff()
{
  if (Cutoff)
    return Cutoff->GetVal() + 1;
  return 0;
}

void tTrack::SetCutoff(int Value)
{
  if (Cutoff) {
    delete Cutoff;
    Cutoff = 0;
  }
  if (Value > 0)
  {
	Cutoff = new tNrpn( 0, Channel - 1, 0x01, 0x20, Value - 1 );
	Midi->OutNow(this,  Cutoff );
        changed = 1;
  }
}

// ------------------------  Resonance ------------------------------

int tTrack::GetResonance()
{
  if (Resonance)
    return Resonance->GetVal() + 1;
  return 0;
}

void tTrack::SetResonance(int Value)
{
  if (Resonance) {
	delete Resonance;
	Resonance = 0;
  }
  if (Value > 0)
  {
	Resonance = new tNrpn( 0, Channel - 1, 0x01, 0x21, Value - 1 );
	Midi->OutNow(this,  Resonance );
        changed = 1;
  }
}

// ------------------------  EnvAttack ------------------------------

int tTrack::GetEnvAttack()
{
  if (EnvAttack)
    return EnvAttack->GetVal() + 1;
  return 0;
}

void tTrack::SetEnvAttack(int Value)
{
  if (EnvAttack) {
	delete EnvAttack;
	EnvAttack = 0;
  }
  if (Value > 0)
  {
	EnvAttack = new tNrpn( 0, Channel - 1, 0x01, 0x63, Value - 1 );
	Midi->OutNow(this,  EnvAttack );
        changed = 1;
  }
}

// ------------------------  EnvDecay ------------------------------

int tTrack::GetEnvDecay()
{
  if (EnvDecay)
    return EnvDecay->GetVal() + 1;
  return 0;
}

void tTrack::SetEnvDecay(int Value)
{
  if (EnvDecay) {
	delete EnvDecay;
	EnvDecay = 0;
  }
  if (Value > 0)
  {
	EnvDecay = new tNrpn( 0, Channel - 1, 0x01, 0x64, Value - 1 );
	Midi->OutNow(this,  EnvDecay );
        changed = 1;
  }
}

// ------------------------  EnvRelease ------------------------------

int tTrack::GetEnvRelease()
{
  if (EnvRelease)
    return EnvRelease->GetVal() + 1;
  return 0;
}

void tTrack::SetEnvRelease(int Value)
{
  if (EnvRelease) {
	delete EnvRelease;
	EnvRelease = 0;
  }
  if (Value > 0)
  {
	EnvRelease = new tNrpn( 0, Channel - 1, 0x01, 0x66, Value - 1 );
	Midi->OutNow(this,  EnvRelease );
        changed = 1;
  }
}

// ------------------------  DrumParam ------------------------------

int tTrack::GetDrumParam( int pitch, int index )
{
  if (!DrumParams.IsEmpty())
  {
    tNrpn *par = DrumParams.GetParam( pitch, index );
    if (par)
      return( par->GetVal() + 1 );
  }
  return 0;
}

void tTrack::SetDrumParam(int pitch, int index, int Value)
{
  DrumParams.DelParam( pitch, index );
  if (Value > 0)
  {
	DrumParams.PutParam( new tNrpn( 0, Channel - 1, drumIndex2Param( index ), pitch, Value - 1 ) );
	Midi->OutNow(this,  DrumParams.GetParam( pitch, index ) );
        changed = 1;
  }
}

// ------------------------  BendPitchSens ------------------------------

int tTrack::GetBendPitchSens()
{
  if (BendPitchSens)
    return BendPitchSens->GetVal() + 1;
  return 0;
}

void tTrack::SetBendPitchSens(int Value)
{
  if (BendPitchSens) {
	delete BendPitchSens;
	BendPitchSens = 0;
  }
  if (Value > 0)
  {
	BendPitchSens = new tRpn( 0, Channel - 1, 0x00, 0x00, Value - 1 );
	Midi->OutNow(this,  BendPitchSens );
        changed = 1;
  }
}

// ------------------------  Modulation Sysex ------------------------------

int tTrack::GetModulationSysex( int msp )
{
   uchar *valp = Synth->GetSysexValPtr(ModulationSettings[ msp ]);

   if (valp)
      return *valp + 1;

   return 0;
}

void tTrack::SetModulationSysex(int msp, int Value)
{
  if (ModulationSettings[ msp ])
    Kill(ModulationSettings[ msp ]);
  if (Value > 0)
  {
    tEvent *e = Synth->ModSX( msp, 0, Channel, Value - 1 );
    if (e)
    {
       Put(e);
       Midi->OutNow(this, e);
    }
  }
  Cleanup();
}

// ------------------------  Bender Sysex ------------------------------

int tTrack::GetBenderSysex( int bsp )
{
   uchar *valp = Synth->GetSysexValPtr(BenderSettings[ bsp ]);

   if (valp)
      return *valp + 1;

   return 0;
}

void tTrack::SetBenderSysex(int bsp, int Value)
{
  if (BenderSettings[ bsp ])
    Kill(BenderSettings[ bsp ]);
  if (Value > 0)
  {
    tEvent *e = Synth->BendSX( bsp, 0, Channel, Value - 1 );
    if (e)
    {
       Put(e);
       Midi->OutNow(this, e);
    }
  }
  Cleanup();
}

// ------------------------  CAf Sysex ------------------------------

int tTrack::GetCAfSysex( int csp )
{
   uchar *valp = Synth->GetSysexValPtr(CAfSettings[ csp ]);

   if (valp)
      return *valp + 1;

   return 0;
}

void tTrack::SetCAfSysex(int csp, int Value)
{
  if (CAfSettings[ csp ])
    Kill(CAfSettings[ csp ]);
  if (Value > 0)
  {
    tEvent *e = Synth->CafSX( csp, 0, Channel, Value - 1 );
    if (e)
    {
       Put(e);
       Midi->OutNow(this, e);
    }
  }
  Cleanup();
}

// ------------------------  PAf Sysex ------------------------------

int tTrack::GetPAfSysex( int psp )
{
   uchar *valp = Synth->GetSysexValPtr(PAfSettings[ psp ]);

   if (valp)
      return *valp + 1;

   return 0;
}

void tTrack::SetPAfSysex(int psp, int Value)
{
  if (PAfSettings[ psp ])
    Kill(PAfSettings[ psp ]);
  if (Value > 0)
  {
    tEvent *e = Synth->PafSX( psp, 0, Channel, Value - 1 );
    if (e)
    {
       Put(e);
       Midi->OutNow(this, e);
    }
  }
  Cleanup();
}

// ------------------------  CC1 Sysex ------------------------------

int tTrack::GetCC1Sysex( int csp )
{
   uchar *valp = Synth->GetSysexValPtr(CC1Settings[ csp ]);

   if (valp)
      return *valp + 1;

   return 0;
}

void tTrack::SetCC1Sysex(int csp, int Value)
{
  if (CC1Settings[ csp ])
    Kill(CC1Settings[ csp ]);
  if (Value > 0)
  {
    tEvent *e = Synth->CC1SX( csp, 0, Channel, Value - 1 );
    if (e)
    {
       Put(e);
       Midi->OutNow(this, e);
    }
  }
  Cleanup();
}

// ------------------------  CC2 Sysex ------------------------------

int tTrack::GetCC2Sysex( int csp )
{
   uchar *valp = Synth->GetSysexValPtr(CC2Settings[ csp ]);

   if (valp)
      return *valp + 1;

   return 0;
}

void tTrack::SetCC2Sysex(int csp, int Value)
{
  if (CC2Settings[ csp ])
    Kill(CC2Settings[ csp ]);
  if (Value > 0)
  {
    tEvent *e = Synth->CC2SX( csp, 0, Channel, Value - 1 );
    if (e)
    {
       Put(e);
       Midi->OutNow(this, e);
    }
  }
  Cleanup();
}

// ------------------------  CC1ControllerNr Sysex ------------------------------

int tTrack::GetCC1ControllerNr()
{
   uchar *valp = Synth->GetSysexValPtr(CC1ControllerNr);

   if (valp)
      return *valp + 1;

   return 0;
}

void tTrack::SetCC1ControllerNr(int Value)
{
  if (CC1ControllerNr)
    Kill(CC1ControllerNr);
  if (Value > 0)
  {
    tEvent *e = Synth->ControllerNumberSX( 1, 0, Channel, Value - 1 );
    if (e)
    {
       Put(e);
       Midi->OutNow(this, e);
    }
  }
  Cleanup();
}

// ------------------------  CC2ControllerNr Sysex ------------------------------

int tTrack::GetCC2ControllerNr()
{
   uchar *valp = Synth->GetSysexValPtr(CC2ControllerNr);

   if (valp)
      return *valp + 1;

   return 0;
}

void tTrack::SetCC2ControllerNr(int Value)
{
  if (CC2ControllerNr)
    Kill(CC2ControllerNr);
  if (Value > 0)
  {
    tEvent *e = Synth->ControllerNumberSX( 2, 0, Channel, Value - 1 );
    if (e)
    {
       Put(e);
       Midi->OutNow(this, e);
    }
  }
  Cleanup();
}

// ------------------------  Reverb Type ------------------------------

int tTrack::GetReverbType(int lsb)
{
   uchar *valp = Synth->GetSysexValPtr(ReverbType);

   if (valp)
   {
      if (lsb)
      {
	 valp++;
      }

      return *valp + 1;
   }

   return 0;
}

void tTrack::SetReverbType(int Value, int lsb)
{
  if (ReverbType)
    Kill(ReverbType);
  if (Value > 0)
  {
    tEvent *e = Synth->ReverbMacroSX( 0, Value - 1, lsb - 1 );
    if (e)
    {
       Put(e);
       if (Config(C_UseReverbMacro))
	  Midi->OutNow(this, e);
    }
  }
  Cleanup();
}

// ------------------------  Chorus Type ------------------------------

int tTrack::GetChorusType(int lsb)
{
   uchar *valp = Synth->GetSysexValPtr(ChorusType);

   if (valp)
   {
      if (lsb)
      {
	 valp++;
      }

      return *valp + 1;
   }

   return 0;
}

void tTrack::SetChorusType(int Value, int lsb)
{
  if (ChorusType)
    Kill(ChorusType);

  if (Value > 0)
  {
    tEvent *e = Synth->ChorusMacroSX( 0, Value - 1, lsb - 1 );
    if (e)
    {
       Put(e);
       if (Config(C_UseChorusMacro))
	  Midi->OutNow(this, e);
    }
  }
  Cleanup();
}

// -----------------------  Equalizer Type ------------------------------

int tTrack::GetEqualizerType()
{
   uchar *valp = Synth->GetSysexValPtr(EqualizerType);

   if (valp)
      return *valp + 1;

   return 0;
}

void tTrack::SetEqualizerType(int Value)
{
  if (EqualizerType)
    Kill(EqualizerType);

  if (Value > 0)
  {
    tEvent *e = Synth->EqualizerMacroSX( 0, Value - 1 );
    if (e)
    {
       Put(e);
       Midi->OutNow(this, e);
    }
  }
  Cleanup();
}

// ------------------------  Reverb Parameters Sysex ------------------------------

int tTrack::GetRevSysex( int rsp )
{
   uchar *valp = Synth->GetSysexValPtr(ReverbSettings[ rsp ]);

   if (valp)
      return *valp + 1;

   return 0;
}

void tTrack::SetRevSysex(int rsp, int Value)
{
  if (ReverbSettings[ rsp ])
    Kill(ReverbSettings[ rsp ]);

  if (Value > 0)
  {
    tEvent *e = Synth->ReverbParamSX( rsp, 0, Value - 1 );
    if (e)
    {
       Put(e);
       if (!Config(C_UseReverbMacro))
	  Midi->OutNow(this, e);
    }
  }
  Cleanup();
}

// ------------------------  Chorus Parameters Sysex ------------------------------

int tTrack::GetChoSysex( int csp )
{
   uchar *valp = Synth->GetSysexValPtr(ChorusSettings[ csp ]);

   if (valp)
      return *valp + 1;

   return 0;
}

void tTrack::SetChoSysex(int csp, int Value)
{
  if (ChorusSettings[ csp ])
    Kill(ChorusSettings[ csp ]);

  if (Value > 0)
  {
    tEvent *e = Synth->ChorusParamSX( csp, 0, Value - 1 );
    if (e)
    {
       Put(e);
       if (!Config(C_UseChorusMacro))
	  Midi->OutNow(this, e);
    }
  }
  Cleanup();
}


// ------------------------  Partial Reserve ------------------------------

int tTrack::GetPartRsrv( int chan )
{
   uchar *valp = Synth->GetSysexValPtr(PartialReserve);

   if (valp)
      return *(valp + sysex_channel(chan)) + 1;

   return 0;
}

void tTrack::SetPartRsrv(uchar *rsrv)
{
  if (PartialReserve)
     Kill(PartialReserve);

  if (rsrv) {
     tEvent *e = Synth->PartialReserveSX( 0, Channel, rsrv );
    if (e)
    {
       Put(e);
       Midi->OutNow(this, e);
    }
  }
  Cleanup();
}

// ------------------------  Master Volume ------------------------------

int tTrack::GetMasterVol()
{
   uchar *valp = Synth->GetSysexValPtr(MasterVol);

   if (valp)
   {
      if (Synth->GetSysexId(MasterVol) == SX_GM_MasterVol)
      {
	 // first data byte is lsb; get msb instead!
	 valp++;
      }

      return *valp + 1;
   }

   return 0;
}

void tTrack::SetMasterVol(int Value)
{
  if (MasterVol)
    Kill(MasterVol);
  if (Value > 0)
  {
     tEvent *e = Synth->MasterVolSX( 0, Value - 1 );
     if (e)
     {
	Put(e);
	Midi->OutNow(this, e);
     }
  }
  Cleanup();
}


// ------------------------  Master Pan ------------------------------

int tTrack::GetMasterPan()
{
   uchar *valp = Synth->GetSysexValPtr(MasterPan);

   if (valp)
      return *valp + 1;

   return 0;
}

void tTrack::SetMasterPan(int Value)
{
  if (MasterPan)
    Kill(MasterPan);
  if (Value > 0)
  {
     tEvent *e = Synth->MasterPanSX( 0, Value - 1 );
    if (e)
    {
       Put(e);
       Midi->OutNow(this, e);
    }
  }
  Cleanup();
}

// ------------------------  Mode Sysex ------------------------------

int tTrack::GetModeSysex( int param )
{
   uchar *valp = 0;

   switch (param) {
    case mspRxChannel:
       valp = Synth->GetSysexValPtr(RxChannel);
       break;

    case mspUseForRhythm:
       valp = Synth->GetSysexValPtr(UseForRhythm);
       break;
   }

   if (valp)
      return *valp + 1;

   return 0;
}

void tTrack::SetModeSysex(int param, int Value)
{
  switch (param) {
   case mspRxChannel:
      if (RxChannel)
	 Kill(RxChannel);
      break;
   case mspUseForRhythm:
      if (UseForRhythm)
	 Kill(UseForRhythm);
      break;
  }

  tEvent *e = 0;

  if (Value > 0) {
     switch (param) {
      case mspRxChannel:
	 e = Synth->RxChannelSX( 0, Channel, Value - 1 );
	 break;
      case mspUseForRhythm:
	 e = Synth->UseForRhythmSX( 0, Channel, Value - 1 );
	 break;
     }

    if (e)
    {
       Put(e);
       Midi->OutNow(this, e);
    }
  }
  Cleanup();
}

// ------------------------  Mtc offset (Time Code Offset) ------------------------------

tMtcTime* tTrack::GetMtcOffset()
{
  if (MtcOffset)
    return( new tMtcTime( MtcOffset ) );
  return( new tMtcTime( 0L, Mtc30Ndf ) );
}

void tTrack::SetMtcOffset(tMtcTime* mtc)
{
  if (MtcOffset)
    Kill(MtcOffset);
  if (mtc)
  {
    tEvent *e = mtc->ToOffset();
    Put(e);
  }
  Cleanup();
}

// ------------------------  Speed ------------------------------

int tTrack::GetDefaultSpeed()
{
  if (Speed)
    return Speed->GetBPM();
  return 120;
}


void tTrack::SetDefaultSpeed(int bpm)
{
  tEvent *e = new tSetTempo(0, bpm);
  if (Speed)
    Kill(Speed);
  Put(e);
  Midi->OutNow(this, e);
  Cleanup();
}

tSetTempo *tTrack::GetCurrentTempo( long clk )
{
  tEventIterator Iterator(this);
  Sort();
  tEvent *e = Iterator.Range(0, clk + 1);
  tSetTempo *t = Speed;
  while (e) {
    if (e->IsSetTempo()) {
      t = e->IsSetTempo();
    }
    e = Iterator.Next();
  } // while e

  return t;
}

int tTrack::GetCurrentSpeed( long clk )
{
  tSetTempo *t = GetCurrentTempo( clk );
  if (t)
    return t->GetBPM();
  else
    return 120;
}



// ------------------------- State ----------------------------------


char *tTrack::GetStateChar()
{
  switch (State)
  {
    case tsPlay: return "P";
    case tsMute: return "M";
    case tsSolo: return "S";
  }
  return "?";
}

void tTrack::SetState(int NewState)
{
  State = NewState % 3;
}

void tTrack::ToggleState(int Direction)
{
  State = (State + Direction + 3) % 3;
}

// ------------------------- Channel ---------------------------

void tTrack::SetChannel(int NewChannel)
{
  Channel = NewChannel;
}


