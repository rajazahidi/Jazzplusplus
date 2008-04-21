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

#include "Track.h"
#include "Synth.h"
#include "Configuration.h"
#include "Player.h"
#include "JazzPlusPlusApplication.h"
#include "TrackWindow.h"
#include "Globals.h"
#include "Song.h"
#include "Dialogs/TrackDialog.h"

#include <cstdlib>
#include <assert.h>

int tParam::Write(JZWriteBase& Io)
{
  return Msb.Write(Io) + Lsb.Write(Io) + DataMsb.Write(Io);
}

void tParam::SetCha(unsigned char cha)
{
  Msb.Channel = cha;
  Lsb.Channel = cha;
  DataMsb.Channel = cha;

#ifdef OBSOLETE
  ResetMb.Channel = cha; //???? JAVE commented out this while porting
#endif // OBSOLETE

  ResetLsb.Channel = cha;
}

/*
unsigned char sys_Sysex[7] = { 0x41, 0x10, 0x42, 0x12, 0x40, 0x00, 0x00 };

unsigned char *SysExDT1(
  unsigned char aa,
  unsigned char bb,
  unsigned char cc,
  int datalen,
  unsigned char *data)
{
  int length = 9 + datalen;
  unsigned char *mess = new unsigned char[length];
  mess[0] = 0x41;
  mess[1] = 0x10;
  mess[2] = 0x42;
  mess[3] = 0x12;
  mess[4] = aa;
  mess[5] = bb;
  mess[6] = cc;
  int i;
  for (i = 0; i < datalen; i++)
  {
    mess[i+7] = data[i];
  }
  unsigned char sum = 0x00;
  for (i = 4; i < (length-2); i++)
  {
    sum += mess[i];
  }
  mess[length - 2] = (0x80 - (sum & 0x7f)) & 0x7f;
  mess[length - 1] = 0xf7;
  return mess;
}
*/

static double framesPerSecond[] = { 24.0, 25.0, 30.0, 30.0 };

tMtcTime::tMtcTime(tMtcOffset *s)
{
  type = (tMtcType) ((s->Data[0] & 0x60) >> 5);
  if (type < Mtc24)
  {
    type = Mtc24;
  }
  if (type > Mtc30Ndf)
  {
    type = Mtc30Ndf;
  }
  hour = s->Data[0] & 0x1f;
  min = s->Data[1];
  sec = s->Data[2];
  fm = s->Data[3];
}

tMtcTime::tMtcTime(int millisec, tMtcType t)
{
  type = t;
  if (type < Mtc24)
  {
    type = Mtc24;
  }
  if (type > Mtc30Ndf)
  {
    type = Mtc30Ndf;
  }
  sec = millisec / 1000;
  int msec = millisec % 1000;
  min = sec / 60;
  sec = sec % 60;
  hour = min / 60;
  min = min % 60;
  double frametime = 1000.0 / framesPerSecond[type];
  fm = (int) ((double) msec / frametime);
}

tMtcTime::tMtcTime(char *str, tMtcType t)
{
  type = t;
  if (type < Mtc24)
  {
    type = Mtc24;
  }
  if (type > Mtc30Ndf)
  {
    type = Mtc30Ndf;
  }
  hour = 0;
  min = 0;
  sec = 0;
  fm = 0;
  sscanf(str, "%d:%d:%d.%d", &hour, &min, &sec, &fm);
  if (fm >= framesPerSecond[type])
  {
    fm = (int) framesPerSecond[type] - 1;
  }
}

tMtcTime::tMtcTime(unsigned h, unsigned m, unsigned s, unsigned f, unsigned t)
{
  hour = h;
  min = m;
  sec = s;
  fm = f;
  type = (tMtcType) t;
  if (type < Mtc24)
  {
    type = Mtc24;
  }
  if (type > Mtc30Ndf)
  {
    type = Mtc30Ndf;
  }
}

void tMtcTime::ToString(char *str)
{
  sprintf(str, "%d:%d:%d.%d", hour, min, sec, fm);
}

tMtcOffset *tMtcTime::ToOffset()
{
  unsigned char *mess = new unsigned char[5];
  mess[0] = (unsigned char) hour | ((unsigned char) type << 5);
  mess[1] = (unsigned char) min;
  mess[2] = (unsigned char) sec;
  mess[3] = (unsigned char) fm;
  mess[4] = 0x00;
  tMtcOffset *s = new tMtcOffset(0, mess, 5);
  delete mess;
  return s;
}

int tMtcTime::ToMillisec()
{
  int msec = (((((hour * 60L) + min) * 60L) + sec) * 1000L) +
              ((fm * 1000L) / (int) framesPerSecond[type]);
  return msec;
}

int sysex_channel(int Channel)
{
  if (Channel < 10)
  {
    return(Channel);
  }
  else if (Channel == 10)
  {
    return 0;
  }
  return Channel - 1;
}

int drumParam2Index(int par)
{
  switch (par)
  {
    case drumPitch:
      return(drumPitchIndex);
    case drumTva:
      return(drumTvaIndex);
    case drumPan:
      return(drumPanIndex);
    case drumReverb:
      return(drumReverbIndex);
    case drumChorus:
      return(drumChorusIndex);
    default:
      assert(0);
  }
  return 0;
}

int drumIndex2Param(int index)
{
  switch (index)
  {
    case drumPitchIndex:
      return drumPitch;
    case drumTvaIndex:
      return drumTva;
    case drumPanIndex:
      return drumPan;
    case drumReverbIndex:
      return drumReverb;
    case drumChorusIndex:
      return drumChorus;
    default:
      assert(0);
  }
  return 0;
}

tDrumInstrumentParameter::tDrumInstrumentParameter(tNrpn *par)
  : pitch(par->Lsb.Value),
    next(0)
{
  for (int i = drumPitchIndex; i < numDrumParameters; i++)
  {
    param[i] = 0;
  }
  param[drumParam2Index(par->Msb.Value)] = par;
}

tNrpn *tDrumInstrumentParameter::Get(int index)
{
  assert((index >= drumPitchIndex) && (index < numDrumParameters));
  return(param[index]);
}

void tDrumInstrumentParameter::Put(tNrpn *par)
{
  param[par->Lsb.Value] = par;
}

tDrumInstrumentParameter *tDrumInstrumentParameter::Next()
{
  return next;
}

int tDrumInstrumentParameter::Pitch()
{
  return pitch;
}

tDrumInstrumentParameter
*tDrumInstrumentParameterList::GetElem(int pit)
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
  return ptr;
}

tNrpn *tDrumInstrumentParameterList::GetParam(int pit, int index)
{
  tDrumInstrumentParameter *ptr = GetElem(pit);
  if (ptr)
  {
    return ptr->Get(index);
  }
  return 0;
}

void tDrumInstrumentParameterList::PutParam(tNrpn *par)
{
  tDrumInstrumentParameter *ptr = GetElem(par->Lsb.Value);
  if (!ptr)
  {
    ptr = new tDrumInstrumentParameter(par);
    ptr ->next = list;
    list = ptr;
  }
  else
  {
    ptr->param[drumParam2Index(par->Msb.Value)] = par;
  }
}

void tDrumInstrumentParameterList::DelParam(int pit, int index)
{
  if (list)
  {
    tDrumInstrumentParameter *elem = GetElem(pit);
    if (elem)
    {
      if (elem->Get(index))
      {
        delete elem->param[index];
      }
      elem->param[index] = 0;
    }
  }
}

void tDrumInstrumentParameterList::DelElem(int pit)
{
  for (int i = drumPitchIndex; i < numDrumParameters; i++)
  {
    DelParam(pit, i);
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
  return list;
}

tDrumInstrumentParameter *tDrumInstrumentParameterList::NextElem(
  tDrumInstrumentParameter *cur)
{
  if (cur)
  {
    tDrumInstrumentParameter *ptr = GetElem(cur->pitch);
    if (ptr)
    {
      return ptr->next;
    }
    else
    {
      return 0;
    }
  }
  else
  {
    return 0;
  }
}

void tDrumInstrumentParameterList::Clear()
{
  tDrumInstrumentParameter *ptr = list;
  while (ptr)
  {
    list = ptr->next;
    delete ptr;
    ptr = list;
  }
}


tSimpleEventArray::tSimpleEventArray()
  : wxObject(),
    nEvents(0),
    MaxEvents(0),
    Events(0)
{
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
  {
    for (int i = 0; i < nEvents; i++)
    {
      Events[i]->edb();
    }
  }
#endif
  for (i = 0; i < nEvents; i++)
  {
    delete Events[i];
  }
  nEvents = 0;
}


void tUndoBuffer::Clear()
{
  int i;
#ifdef E_DBUG
  {
    for (int i = 0; i < nEvents; i++)
    {
      if (bits(i))
      {
        Events[i]->edb();
      }
    }
  }
#endif
  for (i = 0; i < nEvents; i++)
  {
    if (bits(i))
    {
      delete Events[i];
    }
  }
  nEvents = 0;
}


void tSimpleEventArray::Resize()
{
  int i;
  MaxEvents += 50;
  JZEvent** ppEvents = new JZEvent* [MaxEvents];

  // Copy the previuosly existing event pointers.
  for (i = 0; i < nEvents; ++i)
  {
    ppEvents[i] = Events[i];
  }

  // Initialize the new event pointers to 0.
  for (; i < MaxEvents; ++i)
  {
    ppEvents[i] = 0;
  }

  // Delete the old event pointers
  delete [] Events;

  // Set the data member to the new storage location.
  Events = ppEvents;
}

//   Remove any end of track (EOT) events from the track.  There can only be
// one EOT event in a track, so remove it before inserting a new one.
void tSimpleEventArray::RemoveEOT()
{
  int j = 0;
  int newnEvents = nEvents;
  for (int i = 0; i < nEvents; ++i)
  {
    if (Events[i] != 0 && Events[i]->IsEndOfTrack())
    {
      ++j;
      --newnEvents;
    }

    JZEvent* item;
    if (j <= MaxEvents)
    {
      item = Events[j++];
    }
    else
    {
      item = 0;
    }
    Events[i] = item;
  }
  nEvents = newnEvents;
}

void tSimpleEventArray::Put(JZEvent* e)
{
  if (e->IsEndOfTrack())
  {
    // Remove the old EOT if we are adding a new one.
    RemoveEOT();
  }
  if (nEvents >= MaxEvents)
  {
    Resize();
  }
  Events[nEvents++] = e;

#ifdef E_DBUG
  {
    for (int i = 0; i < nEvents; i++)
    {
      Events[i]->edb();
    }
  }
#endif
}

// Description:
//   Move the data from passed event array this instance.
void tSimpleEventArray::GrabData(tSimpleEventArray& src)
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


void tSimpleEventArray::Copy(tSimpleEventArray& src, int frclk, int toclk)
{
  tEventIterator iter(&src);
  JZEvent* e = iter.Range(frclk, toclk);
  while (e)
  {
    Put(e->Copy());
    e = iter.Next();
  }
}


tEventArray::tEventArray()
  : tSimpleEventArray(),
    mName(0),
    Copyright(0),
    mPatch(0),
    Speed(0),
    Volume(0),
    Pan(0),
    Reverb(0),
    Chorus(0),
    mpBank(0),
    mpBank2(0),
    Reset(0)
{
  nEvents = 0;

  MaxEvents = 0;
  Events = 0;
  Channel = 0;
  Device = 0;
  ForceChannel = 0;
  State = tsPlay;
  audio_mode = 0;

  Clear();
}


void tEventArray::Clear()
{
  int i;

  tSimpleEventArray::Clear();

//  delete mName;
  mName = 0;

  Copyright = 0;

  delete mPatch;
  mPatch = 0;

  Volume = 0;

  Pan = 0;

  Reverb = 0;

  Chorus = 0;

  delete mpBank;
  mpBank = 0;

  delete mpBank2;
  mpBank2 = 0;

  Reset = 0;
  Speed = 0;
  Channel = 1;
  Device = 0;

  for (i = 0; i < mspModulationSysexParameters; i++)
  {
    ModulationSettings[i] = 0;
  }

  for (i = 0; i < bspBenderSysexParameters; i++)
  {
    BenderSettings[i] = 0;
  }

  for (i = 0; i < cspCAfSysexParameters; i++)
  {
    CAfSettings[i] = 0;
  }

  for (i = 0; i < pspPAfSysexParameters; i++)
  {
    PAfSettings[i] = 0;
  }

  for (i = 0; i < cspCC1SysexParameters; i++)
  {
    CC1Settings[i] = 0;
  }

  for (i = 0; i < cspCC2SysexParameters; i++)
  {
    CC2Settings[i] = 0;
  }

  CC1ControllerNr = 0;
  CC2ControllerNr = 0;

  ReverbType = 0;
  ChorusType = 0;
  EqualizerType = 0;

  for (i = 0; i < rspReverbSysexParameters; i++)
  {
    ReverbSettings[i] = 0;
  }

  for (i = 0; i < cspChorusSysexParameters; i++)
  {
    ChorusSettings[i] = 0;
  }

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
  {
    delete [] Events;
  }
  Events = 0;
  MaxEvents = 0;

  State = tsPlay;
  audio_mode = 0;
}


tEventArray::~tEventArray()
{
  Clear();
}





static int compare(const void *p1, const void *p2)
{
  JZEvent *e1 = *(JZEvent **)p1;
  JZEvent *e2 = *(JZEvent **)p2;
  return e1->Compare(*e2);
}


void tSimpleEventArray::Sort()
{
  qsort(Events, nEvents, sizeof(JZEvent *), compare);
}



void tEventArray::Cleanup(bool dont_delete_killed_events)
{
  JZEvent *e;
  tControl *c;
  tSysEx *s;
  int i;

  Sort();  // moves all killed events to the end of array

  // clear track defaults
//  delete mName;
  mName = 0;

  Copyright = 0;

  Speed = 0;

  Volume = 0;

  Pan = 0;

  Reverb = 0;

  Chorus = 0;

  for (i = 0; i < mspModulationSysexParameters; i++)
  {
    ModulationSettings[i] = 0;
  }

  for (i = bspBendPitchControl; i < bspBenderSysexParameters; i++)
  {
    BenderSettings[i] = 0;
  }

  for (i = 0; i < cspCAfSysexParameters; i++)
  {
    CAfSettings[i] = 0;
  }

  for (i = 0; i < pspPAfSysexParameters; i++)
  {
    PAfSettings[i] = 0;
  }

  for (i = 0; i < cspCC1SysexParameters; i++)
  {
    CC1Settings[i] = 0;
  }

  for (i = 0; i < cspCC2SysexParameters; i++)
  {
    CC2Settings[i] = 0;
  }

  CC1ControllerNr = 0;
  CC2ControllerNr = 0;

  ReverbType = 0;
  ChorusType = 0;
  EqualizerType = 0;

  for (i = 0; i < rspReverbSysexParameters; i++)
  {
    ReverbSettings[i] = 0;
  }

  for (i = 0; i < cspChorusSysexParameters; i++)
  {
    ChorusSettings[i] = 0;
  }

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
        {
          delete Events[j];
        }
      }
      nEvents = i;
      break;
    }

    // accept only events having clock == 0 as track defaults
    if (e->GetClock() != 0)
    {
      continue;
    }

    if (!mName)
    {
      mName = e->IsTrackName();
    }

    if (!Copyright)
    {
      Copyright = e->IsCopyright();
    }
    if (!Speed)
    {
      Speed = e->IsSetTempo();
    }
    if (!MtcOffset)
    {
      MtcOffset = e->IsMtcOffset();
    }
    if ((c = e->IsControl()) != 0)
    {
      switch (c->Control)
      {
        case 0x07:
          if (!Volume)
          {
            Volume = c;
          }
          break;
        case 0x0a:
          if (!Pan)
          {
            Pan = c;
          }
          break;
        case 0x5b:
          if (!Reverb)
          {
            Reverb = c;
          }
          break;
        case 0x5d:
          if (!Chorus)
          {
            Chorus = c;
          }
          break;
      }
    }
    if ((s = e->IsSysEx()) != 0)
    {
      int sxid = gpSynth->GetSysexId(s);

      if (!gpSynth->IsGS())
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

      if (gpSynth->IsGS())
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
            ReverbSettings[sxid - SX_GS_RevCharacter] = s;
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
            ChorusSettings[sxid - SX_GS_ChoPreLpf] = s;
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
      else if (gpSynth->IsXG())
      {
        switch (sxid)
        {
          case SX_XG_BendPitch:
          case SX_XG_BendTvf:
          case SX_XG_BendAmpl:
            BenderSettings[sxid - SX_XG_BendPitch] = s;
            break;

          case SX_XG_BendLfoPitch:
          case SX_XG_BendLfoTvf:
          case SX_XG_BendLfoTva:
            BenderSettings[sxid + 1 - SX_XG_BendPitch] = s;
            break;

          case SX_XG_ModPitch:
          case SX_XG_ModTvf:
          case SX_XG_ModAmpl:
            ModulationSettings[sxid - SX_XG_ModPitch] = s;
            break;

          case SX_XG_ModLfoPitch:
          case SX_XG_ModLfoTvf:
          case SX_XG_ModLfoTva:
            ModulationSettings[sxid + 1 - SX_XG_ModPitch] = s;
            break;

          case SX_XG_CafPitch:
          case SX_XG_CafTvf:
          case SX_XG_CafAmpl:
            CAfSettings[sxid - SX_XG_CafPitch] = s;
            break;

          case SX_XG_CafLfoPitch:
          case SX_XG_CafLfoTvf:
          case SX_XG_CafLfoTva:
            CAfSettings[sxid + 1 - SX_XG_CafPitch] = s;
            break;

          case SX_XG_PafPitch:
          case SX_XG_PafTvf:
          case SX_XG_PafAmpl:
            PAfSettings[sxid - SX_XG_PafPitch] = s;
            break;

          case SX_XG_PafLfoPitch:
          case SX_XG_PafLfoTvf:
          case SX_XG_PafLfoTva:
            PAfSettings[sxid + 1 - SX_XG_PafPitch] = s;
            break;

          case SX_XG_CC1Pitch:
          case SX_XG_CC1Tvf:
          case SX_XG_CC1Ampl:
            CC1Settings[sxid - SX_XG_CC1Pitch] = s;
            break;

          case SX_XG_CC1LfoPitch:
          case SX_XG_CC1LfoTvf:
          case SX_XG_CC1LfoTva:
            CC1Settings[sxid + 1 - SX_XG_CC1Pitch] = s;
            break;

          case SX_XG_CC2Pitch:
          case SX_XG_CC2Tvf:
          case SX_XG_CC2Ampl:
            CC2Settings[sxid - SX_XG_CC2Pitch] = s;
            break;

          case SX_XG_CC2LfoPitch:
          case SX_XG_CC2LfoTvf:
          case SX_XG_CC2LfoTva:
            CC2Settings[sxid + 1 - SX_XG_CC2Pitch] = s;
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
    if ((on = Events[i]->IsKeyOn()) != 0 && on->mLength != 0)
    {
//      JZEvent *of = new tKeyOff(on->GetClock() + on->mLength, on->Channel, on->Key);
      // SN++ added off veloc
      JZEvent *of = new tKeyOff(
        on->GetClock() + on->mLength,
        on->Channel,
        on->mKey,
        on->GetOffVelocity());

      on->mLength = 0;
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
      JZEvent **e = &Events[i - 1];
      while (e >= Events)
      {
        tKeyOn *on = (*e)->IsKeyOn();
        if (on && on->Key == of->Key && on->Channel == of->Channel && on->Length == 0)
        {
          on->Length = of->GetClock() - on->GetClock();
          if (on->Length <= 0L)
          {
            on->Length = 1;
          }
          of->Kill();
          break;
        }
        --e;
      }
    }
  }

  // kill all KeyOn's with non matching KeyOff's
  for (i = 0; i < nEvents; i++)
  {
    tKeyOn *k = Events[i]->IsKeyOn();
    if (k && k->Length <= 0)
    {
      k->Kill();
    }
  }
  Cleanup(0);
}

#else

void tEventArray::Keyoff2Length()
{
  // Searches forward from a KeyOn to find the matching KeyOff.
  // This is compatible with Cubase.
  int i;
  for (i = 0; i < nEvents; i++)
  {
    tKeyOn* on;
    if ((on = Events[i]->IsKeyOn()) != 0 && on->mLength == 0)
    {
      int j;
      for (j = i + 1; j < nEvents; j++)
      {
        tKeyOff *of = Events[j]->IsKeyOff();
        if (
          of &&
          !of->IsKilled() &&
          on->mKey == of->Key &&
          on->Channel == of->Channel)
        {
          on->mLength = of->GetClock() - on->GetClock();
          if (on->mLength <= 0L)
          {
            on->mLength = 1;
          }
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
    tKeyOn* on = Events[i]->IsKeyOn();
    if (on && on->mLength <= 0)
    {
      on->Kill();
    }
    tKeyOff *of = Events[i]->IsKeyOff();
    if (of)
    {
      of->Kill();
    }
  }
  Cleanup(0);
}

#endif


void tEventArray::Write(JZWriteBase& Io)
{
  JZEvent *e;
  int WrittenBefore;

  Length2Keyoff();
  Io.NextTrack();

  // Write copyright notice first (according to spec):
  if (Copyright)
  {
    Copyright->Write(Io);
  }

  // Write MTC offset before any transmittable events (spec)
  if (MtcOffset)
  {
    MtcOffset->Write(Io);
  }

  // Synth reset
  if (Reset)
  {
    Reset->Write(Io);
  }

  // Rpn / Nrpn:
  // All these must be written in order (three tControl's in a row)
  if (VibRate)
  {
    VibRate->Write(Io);
  }
  if (VibDepth)
  {
    VibDepth->Write(Io);
  }
  if (VibDelay)
  {
    VibDelay->Write(Io);
  }
  if (Cutoff)
  {
    Cutoff->Write(Io);
  }
  if (Resonance)
  {
    Resonance->Write(Io);
  }
  if (EnvAttack)
  {
    EnvAttack->Write(Io);
  }
  if (EnvDecay)
  {
    EnvDecay->Write(Io);
  }
  if (EnvRelease)
  {
    EnvRelease->Write(Io);
  }
  if (BendPitchSens)
  {
    BendPitchSens->Write(Io);
  }

  tDrumInstrumentParameter *dpar = DrumParams.FirstElem();
  while (dpar)
  {
    int index;
    for (index = drumPitchIndex; index < numDrumParameters; index++)
    {
      if (dpar->Get(index))
      {
        dpar->Get(index)->Write(Io);
      }
    }
    dpar = DrumParams.NextElem(dpar);
  }

  // mpBank: Must be sure bank is written before program:
  if (mpBank)
  {
    mpBank->Write(Io);
  }

  if (mpBank2)
  {
    mpBank2->Write(Io);
  }

  if (mPatch)
  {
    mPatch->Write(Io);
  }

  // write jazz track info
  tJazzMeta *jazz = new tJazzMeta;
  jazz->SetAudioMode(audio_mode);
  jazz->SetTrackState(State);
  jazz->SetTrackDevice(Device);
  jazz->SetIntroLength(gpSong->GetIntroLength());
  jazz->Write(Io);

  for (int i = 0; i < nEvents; i++)
  {
    e = Events[i];
    WrittenBefore = 0;
    if (e->IsControl())
    {
      switch (e->IsControl()->Control)
      {
        // Don't write these again if present as events
        // and clock == 0 (should not happen)
        case 0x65: // Rpn Msb
        case 0x64: // Rpn Lsb
        case 0x63: // Nrpn Msb
        case 0x62: // Nrpn Lsb
        case 0x06: // Rpn/Nrpn Data
        case 0x00: // mpBank
        case 0x20: // Bank2
          if (e->GetClock() == 0)
          {
            WrittenBefore = 1;
          }
          break;
        default:
          WrittenBefore = 0;
      }
    }
    else if (e->IsProgram())
    {
      // Don't write these again if present as events
      // and clock == 0 (should not happen)
      if (e->GetClock() == 0)
      {
        WrittenBefore = 1;
      }
    }
    else if (e->IsCopyright() || e->IsMtcOffset())
    {
      // Will probably happen
      WrittenBefore = 1;
    }
    if (!WrittenBefore)
    {
      e->Write(Io);
    }
  }
  Keyoff2Length();
}

void tEventArray::Read(JZReadBase& Io)
{
  JZEvent *e;
  Channel = 0;
  unsigned char Msb, Lsb, Data;
  int SpecialEvent;

  Msb = Lsb = Data = 0xff;
  int cha;

  bool NeedToDelete;

  Io.NextTrack();
  while ((e = Io.Read()) != 0)
  {
    NeedToDelete = false;
    SpecialEvent = 0;
    if (e->IsJazzMeta())
    {
      tJazzMeta *j = e->IsJazzMeta();
      audio_mode = (int)j->GetAudioMode();
      State      = (int)j->GetTrackState();
      Device     = (int)j->GetTrackDevice();
      gpSong->SetIntroLength((int)j->GetIntroLength());
      delete j;
      continue;
    }
    if (e->IsControl())
    {
      switch (e->IsControl()->Control)
      {
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
          switch (Msb)
          {
            case 0x01: // Nrpn
              switch (Lsb)
              {
                case 0x08:
                  if (!VibRate)
                  {
                    VibRate = new tNrpn(0, cha, Msb, Lsb, Data);
                  }
                  break;
                case 0x09:
                  if (!VibDepth)
                  {
                    VibDepth = new tNrpn(0, cha, Msb, Lsb, Data);
                  }
                  break;
                case 0x0a:
                  if (!VibDelay)
                  {
                    VibDelay = new tNrpn(0, cha, Msb, Lsb, Data);
                  }
                  break;
                case 0x20:
                  if (!Cutoff)
                  {
                    Cutoff = new tNrpn(0, cha, Msb, Lsb, Data);
                  }
                  break;
                case 0x21:
                  if (!Resonance)
                  {
                    Resonance = new tNrpn(0, cha, Msb, Lsb, Data);
                  }
                  break;
                case 0x63:
                  if (!EnvAttack)
                  {
                    EnvAttack = new tNrpn(0, cha, Msb, Lsb, Data);
                  }
                  break;
                case 0x64:
                  if (!EnvDecay)
                  {
                    EnvDecay = new tNrpn(0, cha, Msb, Lsb, Data);
                  }
                  break;
                case 0x66:
                  if (!EnvRelease)
                  {
                    EnvRelease = new tNrpn(0, cha, Msb, Lsb, Data);
                  }
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
              DrumParams.PutParam(new tNrpn(0, cha, Msb, Lsb, Data));
              break;
            case 0x00: // Rpn
              if (Lsb == 0x00)
              {
                // Pitch Bend Sensivity
                if (!BendPitchSens)
                {
                  BendPitchSens = new tRpn(0, cha, Msb, Lsb, Data);
                }
              }
              break;
            default:
              break;
          }
          Msb = Lsb = Data = 0xff;
          break;
        case 0x00:
          if (!mpBank)
          {
            SpecialEvent = 1;
            mpBank = e->IsControl();
            mpBank->SetClock(0);
          }
          break;
        case 0x20:
          if (!mpBank2)
          {
            SpecialEvent = 1;
            mpBank2 = e->IsControl();
            mpBank2->SetClock(0);
          }
          break;
        default:
          SpecialEvent = 0; // Other control
          break;
      }
    }
    else if (e->IsProgram())
    {
      if (!mPatch)
      {
        mPatch = e->IsProgram();
        mPatch->SetClock(0);
        SpecialEvent = 1;
      }
    }
    else if (e->IsCopyright())
    {
      if (!Copyright)
      {
        Copyright = e->IsCopyright();

        // Just make sure clock is zero, then put into event array
        Copyright->SetClock(0);
      }
    }
    else if (e->IsSysEx())
    {
      NeedToDelete = true;

      // Get hold of the Reset sysex...
      int sxid = gpSynth->GetSysexId(e->IsSysEx());

      if ((sxid == SX_GM_ON) || (sxid == SX_GS_ON) || (sxid == SX_XG_ON))
      {
        // Take them all away
        SpecialEvent = 1;

        // Save it in the track defaults if it fits with synth
        // type settings
        if (
          (gpSynth->IsGM() && (sxid == SX_GM_ON)) ||
          (gpSynth->IsGS() && (sxid == SX_GS_ON)) ||
          (gpSynth->IsXG() && (sxid == SX_XG_ON)))
        {
          if (!Reset)
          {
            Reset = e->IsSysEx();
            NeedToDelete = false;
          }
        }
      }
    }

    if (!SpecialEvent)
    {
      Put(e);
      NeedToDelete = false;
      if (!Channel && e->IsChannelEvent())
      {
        Channel = e->IsChannelEvent()->Channel + 1;
      }
    }
    if (e->IsEndOfTrack())
    {
      // JAVE I want explicit end of track events
      // Break out of loop here because endoftrack is end, and we want it read
      // FIXME we shoulnt break, we should keep on reading, and handle eot in
      // track play instead.
      break;
    }

    if (NeedToDelete)
    {
      delete e;
    }

  } // while read

  if (!Channel)
  {
    Channel = 1;
  }

  Keyoff2Length();
}


int tEventArray::GetLastClock() const
{
  if (!nEvents)
  {
    return 0;
  }
  return Events[nEvents - 1]->GetClock();
}

int tEventArray::IsEmpty()
{
  return nEvents == 0;
}

int tEventArray::GetFirstClock()
{
  if (nEvents)
  {
    return Events[0]->GetClock();
  }
  return LAST_CLOCK;
}

// ***********************************************************************
// Dialog
// ***********************************************************************
#ifdef OBSOLETE

class tTrackDlg : public wxForm
{
  JZTrackWindow* TrackWin;
  JZTrack *trk;
  char *TrackName;
  tNamedChoice PatchChoice;
  tNamedChoice DeviceChoice;
  int PatchNr;
  int Device;
  int BankNr;
  int ClearTrack;
  int AudioMode;

 public:
  tTrackDlg::tTrackDlg(JZTrackWindow *w, JZTrack *t);
  void EditForm(wxPanel *panel);
  virtual void OnOk();
  virtual void OnCancel();
  virtual void OnHelp();
};


tTrackDlg::tTrackDlg(JZTrackWindow *w, JZTrack *t)
  : wxForm(USED_WXFORM_BUTTONS),
    PatchChoice(
      "Patch",
      t->IsDrumTrack() ? &gpConfig->GetDrumSet(0) : &gpConfig->GetVoiceName(0),
      &PatchNr),
    DeviceChoice("Device", gpMidiPlayer->GetOutputDevices().AsNamedValue(), &Device)
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
  gpHelpInstance->ShowTopic("Trackname, midi channel etc");
}

void tTrackDlg::OnOk()
{
  trk->DialogBox->GetPosition(&Config(C_TrackDlgXpos), &Config(C_TrackDlgYpos));
  trk->DialogBox = 0;
  trk->SetAudioMode(AudioMode);

  if (ClearTrack)
  {
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
  if (trk->ForceChannel)
  {
    tChannelEvent *c;
    tSysEx *s;
    tEventIterator Iterator(trk);
    trk->Sort();
    JZEvent *e = Iterator.Range(0, (unsigned) trk->GetLastClock() + 1);
    while (e)
    {
      if ((c = e->IsChannelEvent()) != 0)
      {
        c = (tChannelEvent *)e->Copy();
        c->Channel = trk->Channel - 1;
        trk->Kill(e);
        trk->Put(c);
      }
      else if ((s = e->IsSysEx()) != 0)
      {
        // Check for sysex that contains channel number
        unsigned char *chaptr = gpSynth->GetSysexChaPtr(s);
        if (chaptr)
        {
          if (gpSynth->IsXG())
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

    if (trk->VibRate)
    {
      trk->VibRate->SetCha(trk->Channel - 1);
    }
    if (trk->VibDepth)
    {
      trk->VibDepth->SetCha(trk->Channel - 1);
    }
    if (trk->VibDelay)
    {
      trk->VibDelay->SetCha(trk->Channel - 1);
    }
    if (trk->Cutoff)
    {
      trk->Cutoff->SetCha(trk->Channel - 1);
    }
    if (trk->Resonance)
    {
      trk->Resonance->SetCha(trk->Channel - 1);
    }
    if (trk->EnvAttack)
    {
      trk->EnvAttack->SetCha(trk->Channel - 1);
    }
    if (trk->EnvDecay)
    {
      trk->EnvDecay->SetCha(trk->Channel - 1);
    }
    if (trk->EnvRelease)
    {
      trk->EnvRelease->SetCha(trk->Channel - 1);
    }
    if (trk->BendPitchSens)
    {
      trk->BendPitchSens->SetCha(trk->Channel - 1);
    }
    if (trk->mpBank)
    {
      trk->mpBank->Channel = trk->Channel - 1;
    }
    if (trk->mPatch)
    {
      trk->mPatch->Channel = trk->Channel - 1;
    }
    if (!trk->DrumParams.IsEmpty())
    {
      tDrumInstrumentParameter *dpar = trk->DrumParams.FirstElem();
      while (dpar)
      {
        for (int index = drumPitchIndex; index < numDrumParameters; ++index)
        {
          if (dpar->Get(index))
          {
            dpar->Get(index)->SetCha(trk->Channel - 1);
          }
        }
        dpar = trk->DrumParams.NextElem(dpar);
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
  Add(wxMakeFormString(
    "Trackname:",
    &TrackName,
    wxFORM_DEFAULT,
    NULL,
    NULL,
    wxVERTICAL,
    300));

  Add(wxMakeFormNewLine());
  Add(PatchChoice.mkFormItem(300, 200));
  Add(wxMakeFormNewLine());
  {
    char buf[500];
    sprintf(
      buf,
      "Set Channel to %d to make a drum track",
      Config(C_DrumChannel));
    Add(wxMakeFormMessage(buf));
    Add(wxMakeFormNewLine());
  }
  Add(wxMakeFormShort(
    "Channel",
    &trk->Channel,
    wxFORM_DEFAULT,
    new wxList(wxMakeConstraintRange(1.0, 16.0), 0)));
  AudioMode = trk->GetAudioMode();
  Add(wxMakeFormNewLine());
  Add(wxMakeFormBool("Audio Track", &AudioMode));
  Add(wxMakeFormNewLine());
  Add(wxMakeFormBool("Force channel number onto all events on track", &trk->ForceChannel));
  ClearTrack = 0;
  Add(wxMakeFormNewLine());
  Add(wxMakeFormBool("Clear track (NB! erase all events, name etc...)", &ClearTrack));

  if (gpMidiPlayer->SupportsMultipleDevices())
  {
    Add(wxMakeFormNewLine());
    Add(DeviceChoice.mkFormItem(300, 50));
  }
  AssociatePanel(panel);
}

#endif // OBSOLETE

void JZTrack::Dialog(JZTrackWindow* pParent)
{
  JZTrackDialog TrackDialog(*this, pParent);
  TrackDialog.ShowModal();
#ifdef OBSOLETE
  DialogBox = new wxDialogBox(
    pParent,
    "Track Settings",
    modal,
    Config(C_TrackDlgXpos),
    Config(C_TrackDlgYpos));
#endif // OBSOLETE
}



// ***********************************************************************
// JZTrack
// ***********************************************************************


bool JZTrack::changed = false;

JZTrack::JZTrack()
  : tEventArray()
{
  iUndo = 0;
  nRedo = 0;
  nUndo = 0;
  DialogBox = 0;
  ForceChannel = 1;
}


bool JZTrack::IsDrumTrack()
{
  return Channel == gpConfig->GetValue(C_DrumChannel);
}

void JZTrack::Merge(tEventArray *t)
{
  for (int i = 0; i < t->nEvents; i++)
  {
    Put(t->Events[i]);
  }
  t->nEvents = 0;
}


void JZTrack::MergeRange(tEventArray *other, int FromClock, int ToClock, int Replace)
{
  // Erase destin
  if (Replace)
  {
    tEventIterator Erase(this);
    JZEvent *e = Erase.Range(FromClock, ToClock);
    while (e)
    {
      Kill(e);
      e = Erase.Next();
    }
  }

  // Merge Recorded Events
  tEventIterator Copy(other);
  JZEvent *e = Copy.Range(FromClock, ToClock);
  while (e)
  {
    JZEvent *c = e->Copy();
    if (ForceChannel)
    {
      tChannelEvent *k = c->IsChannelEvent();
      if (k)
      {
        k->Channel = Channel - 1;
      }
    }
    Put(c);
    e = Copy.Next();
  }
  Cleanup();
}


void JZTrack::Cleanup()
{
  // on audio tracks, adjust length of keyon events to
  // actual sample length
  gpMidiPlayer->AdjustAudioLength(this);
  tEventArray::Cleanup(TRUE);
}




void JZTrack::Undo()
{
  if (nUndo > 0)
  {
    tUndoBuffer *undo = &UndoBuffers[iUndo];
    for (int i = undo->nEvents - 1; i >= 0; i--)
    {
      JZEvent *e = undo->Events[i];
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

void JZTrack::Redo()
{
  if (nRedo > 0)
  {
    iUndo = (iUndo + 1) % MaxUndo;

    tUndoBuffer *undo = &UndoBuffers[iUndo];
    for (int i = 0; i < undo->nEvents; i++)
    {
      JZEvent *e = undo->Events[i];
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


void JZTrack::NewUndoBuffer()
{
  nRedo = 0;
  nUndo++;
  if (nUndo > MaxUndo)
  {
    nUndo = MaxUndo;
  }

  iUndo = (iUndo + 1) % MaxUndo;
  UndoBuffers[iUndo].Clear();
};


void JZTrack::Clear()
{
  for (int i = 0; i < MaxUndo; i++)
  {
    UndoBuffers[i].Clear();
  }
  State  = tsPlay;
  tEventArray::Clear();
}

// ----------------------- Copyright ------------------------------------

const char* JZTrack::GetCopyright()
{
  if (Copyright)
  {
    return (const char *)Copyright->Data;
  }
  return "";
}



void JZTrack::SetCopyright(char *str)
{
  if (Copyright)
  {
    Kill(Copyright);
  }
  if (str && strlen(str))
  {
    int len = 127;
    if ((int)strlen(str) < len)
    {
      len = strlen(str);
    }
    Put(new tCopyright(0, (unsigned char *)str, len));
  }
  Cleanup();
}

// ----------------------- Name ------------------------------------

const char* JZTrack::GetName()
{
  if (mName)
  {
    return (const char*)mName->Data;
  }
  return "";
}



void JZTrack::SetName(char *str)
{
  if (mName)
  {
    Kill(mName);
  }
  if (strlen(str))
  {
    Put(new tTrackName(0, (unsigned char *)str, strlen(str)));
  }
  Cleanup();
}

// ------------------------  Volume ------------------------------

int JZTrack::GetVolume()
{
  if (Volume)
  {
    return Volume->Value + 1;
  }
  return 0;
}

void JZTrack::SetVolume(int Value)
{
  if (Volume)
  {
    Kill(Volume);
  }
  if (Value > 0)
  {
    JZEvent *e = new tControl(0, Channel - 1, 0x07, Value - 1);
    Put(e);
    gpMidiPlayer->OutNow(this, e);
  }
  Cleanup();
}

// ------------------------  Pan ------------------------------

int JZTrack::GetPan()
{
  if (Pan)
  {
    return Pan->Value + 1;
  }
  return 0;
}

void JZTrack::SetPan(int Value)
{
  if (Pan)
  {
    Kill(Pan);
  }
  if (Value > 0)
  {
    JZEvent *e = new tControl(0, Channel - 1, 0x0a, Value - 1);
    Put(e);
    gpMidiPlayer->OutNow(this, e);
  }
  Cleanup();
}

// ------------------------  Reverb ------------------------------

int JZTrack::GetReverb()
{
  if (Reverb)
  {
    return Reverb->Value + 1;
  }
  return 0;
}

void JZTrack::SetReverb(int Value)
{
  if (Reverb)
  {
    Kill(Reverb);
  }
  if (Value > 0)
  {
    JZEvent *e = new tControl(0, Channel - 1, 0x5B, Value - 1);
    Put(e);
    gpMidiPlayer->OutNow(this, e);
  }
  Cleanup();
}

// ------------------------  Chorus ------------------------------

int JZTrack::GetChorus()
{
  if (Chorus)
  {
    return Chorus->Value + 1;
  }
  return 0;
}

void JZTrack::SetChorus(int Value)
{
  if (Chorus)
  {
    Kill(Chorus);
  }
  if (Value > 0)
  {
    JZEvent *e = new tControl(0, Channel - 1, 0x5D, Value - 1);
    Put(e);
    gpMidiPlayer->OutNow(this, e);
  }
  Cleanup();
}

// ------------------------  Bank ------------------------------

int JZTrack::GetBank()
{
  if (!gpConfig->GetValue(C_UseTwoCommandBankSelect))
  {
    DEBUG(fprintf(stderr, "Get single bank select command\n");)
    if (mpBank)
    {
      DEBUG(fprintf(stderr,"Bank %d selected.\n\n",mpBank->Value);)
      return mpBank->Value;
    }
    else
    {
      return 0;
    }
  }
  DEBUG(fprintf(stderr, "Get double bank select command.\n");)
  if (mpBank && mpBank2)
  {
    for (int i=0; gpConfig->BankEntry(i).Command[0]>=0; i++)
    {
      if (
        gpConfig->BankEntry(i).Command[0] == mpBank->Value &&
        gpConfig->BankEntry(i).Command[1] == mpBank2->Value)
      {
        DEBUG(fprintf(stderr,"Bank %d selected.\n\n",i);)
        return i;
      }
    }
  }
  return 0;
}

void JZTrack::SetBank(int Value)
{
  if (mpBank)
  {
    delete mpBank;
    mpBank = 0;
  }

  if (mpBank2)
  {
    delete mpBank2;
    mpBank2 = 0;
  }

  if (Value >= 0)
  {
    if (!gpConfig->GetValue(C_UseTwoCommandBankSelect))
    {
      DEBUG(fprintf (stderr, "Single command bank select (Bank %d).\n",
            Value);)
      mpBank = new tControl(
        0,
        Channel - 1,
        gpConfig->GetValue(C_BankControlNumber),
        Value);
      gpMidiPlayer->OutNow(this, mpBank);
      return;
    }
    while (gpConfig->BankEntry(Value).Command[0]<0 && Value>0)
    {
      Value--;
    }
    assert(gpConfig->BankEntry(Value).Command[0] >= 0);
    DEBUG(fprintf(stderr, "Double command bank select (Bank %d).\n",Value);)
    mpBank  = new tControl(
      0,
      Channel - 1,
      gpConfig->GetValue(C_BankControlNumber),
      gpConfig->BankEntry(Value).Command[0]);
    gpMidiPlayer->OutNow(this, mpBank);
    DEBUG(
      fprintf(
        stderr,
        "First bank select command: %d %d\n",
        mpBank->Control,
        mpBank->Value);)
    mpBank2 = new tControl(
      0,
      Channel - 1,
      gpConfig->GetValue(C_BankControlNumber2),
      gpConfig->BankEntry(Value).Command[1]);

    gpMidiPlayer->OutNow(this, mpBank2);

    DEBUG(fprintf(
      stderr,
      "Second bank select command: %d %d\n\n",
      mpBank2->Control,
      mpBank2->Value);
    )
    changed = true;
  }
}

// ------------------------  Patch ------------------------------

int JZTrack::GetPatch()
{
  if (mPatch)
  {
    return mPatch->Program + 1;
  }
  return 0;
}

void JZTrack::SetPatch(int PatchNr)
{
  if (mPatch)
  {
    delete mPatch;
    mPatch = 0;
  }
  if (PatchNr > 0)
  {
    mPatch = new tProgram(0, Channel - 1, PatchNr - 1);
    gpMidiPlayer->OutNow(this, mPatch);
    changed = true;
  }
}

// ------------------------  VibRate ------------------------------

int JZTrack::GetVibRate()
{
  if (VibRate)
  {
    return VibRate->GetVal() + 1;
  }
  return 0;
}

void JZTrack::SetVibRate(int Value)
{
  if (VibRate)
  {
    delete VibRate;
    VibRate = 0;
  }

  if (Value > 0)
  {
    VibRate = new tNrpn(0, Channel - 1, 0x01, 0x08, Value - 1);
    gpMidiPlayer->OutNow(this, VibRate);
    changed = true;
  }
}

// ------------------------  VibDepth ------------------------------

int JZTrack::GetVibDepth()
{
  if (VibDepth)
  {
    return VibDepth->GetVal() + 1;
  }
  return 0;
}

void JZTrack::SetVibDepth(int Value)
{
  if (VibDepth)
  {
    delete VibDepth;
    VibDepth = 0;
  }
  if (Value > 0)
  {
    VibDepth = new tNrpn(0, Channel - 1, 0x01, 0x09, Value - 1);
    gpMidiPlayer->OutNow(this,  VibDepth);
    changed = true;
  }
}

// ------------------------  VibDelay ------------------------------

int JZTrack::GetVibDelay()
{
  if (VibDelay)
  {
    return VibDelay->GetVal() + 1;
  }
  return 0;
}

void JZTrack::SetVibDelay(int Value)
{
  if (VibDelay)
  {
    delete VibDelay;
    VibDelay = 0;
  }

  if (Value > 0)
  {
    VibDelay = new tNrpn(0, Channel - 1, 0x01, 0x0a, Value - 1);
    gpMidiPlayer->OutNow(this,  VibDelay);
    changed = true;
  }
}

// ------------------------  Cutoff ------------------------------

int JZTrack::GetCutoff()
{
  if (Cutoff)
  {
    return Cutoff->GetVal() + 1;
  }
  return 0;
}

void JZTrack::SetCutoff(int Value)
{
  if (Cutoff)
  {
    delete Cutoff;
    Cutoff = 0;
  }

  if (Value > 0)
  {
    Cutoff = new tNrpn(0, Channel - 1, 0x01, 0x20, Value - 1);
    gpMidiPlayer->OutNow(this,  Cutoff);
    changed = true;
  }
}

// ------------------------  Resonance ------------------------------

int JZTrack::GetResonance()
{
  if (Resonance)
  {
    return Resonance->GetVal() + 1;
  }
  return 0;
}

void JZTrack::SetResonance(int Value)
{
  if (Resonance)
  {
    delete Resonance;
    Resonance = 0;
  }

  if (Value > 0)
  {
    Resonance = new tNrpn(0, Channel - 1, 0x01, 0x21, Value - 1);
    gpMidiPlayer->OutNow(this,  Resonance);
    changed = true;
  }
}

// ------------------------  EnvAttack ------------------------------

int JZTrack::GetEnvAttack()
{
  if (EnvAttack)
  {
    return EnvAttack->GetVal() + 1;
  }
  return 0;
}

void JZTrack::SetEnvAttack(int Value)
{
  if (EnvAttack)
  {
    delete EnvAttack;
    EnvAttack = 0;
  }

  if (Value > 0)
  {
    EnvAttack = new tNrpn(0, Channel - 1, 0x01, 0x63, Value - 1);
    gpMidiPlayer->OutNow(this,  EnvAttack);
    changed = true;
  }
}

// ------------------------  EnvDecay ------------------------------

int JZTrack::GetEnvDecay()
{
  if (EnvDecay)
  {
    return EnvDecay->GetVal() + 1;
  }
  return 0;
}

void JZTrack::SetEnvDecay(int Value)
{
  if (EnvDecay)
  {
    delete EnvDecay;
    EnvDecay = 0;
  }

  if (Value > 0)
  {
    EnvDecay = new tNrpn(0, Channel - 1, 0x01, 0x64, Value - 1);
    gpMidiPlayer->OutNow(this,  EnvDecay);
    changed = true;
  }
}

// ------------------------  EnvRelease ------------------------------

int JZTrack::GetEnvRelease()
{
  if (EnvRelease)
  {
    return EnvRelease->GetVal() + 1;
  }
  return 0;
}

void JZTrack::SetEnvRelease(int Value)
{
  if (EnvRelease)
  {
    delete EnvRelease;
    EnvRelease = 0;
  }

  if (Value > 0)
  {
    EnvRelease = new tNrpn(0, Channel - 1, 0x01, 0x66, Value - 1);
    gpMidiPlayer->OutNow(this,  EnvRelease);
    changed = true;
  }
}

// ------------------------  DrumParam ------------------------------

int JZTrack::GetDrumParam(int pitch, int index)
{
  if (!DrumParams.IsEmpty())
  {
    tNrpn *par = DrumParams.GetParam(pitch, index);
    if (par)
    {
      return(par->GetVal() + 1);
    }
  }
  return 0;
}

void JZTrack::SetDrumParam(int pitch, int index, int Value)
{
  DrumParams.DelParam(pitch, index);
  if (Value > 0)
  {
    DrumParams.PutParam(
      new tNrpn(0, Channel - 1, drumIndex2Param(index), pitch, Value - 1));
    gpMidiPlayer->OutNow(this, DrumParams.GetParam(pitch, index));
    changed = true;
  }
}

// ------------------------  BendPitchSens ------------------------------

int JZTrack::GetBendPitchSens()
{
  if (BendPitchSens)
  {
    return BendPitchSens->GetVal() + 1;
  }
  return 0;
}

void JZTrack::SetBendPitchSens(int Value)
{
  if (BendPitchSens)
  {
    delete BendPitchSens;
    BendPitchSens = 0;
  }

  if (Value > 0)
  {
    BendPitchSens = new tRpn(0, Channel - 1, 0x00, 0x00, Value - 1);
    gpMidiPlayer->OutNow(this,  BendPitchSens);
    changed = true;
  }
}

// ------------------------  Modulation Sysex ------------------------------

int JZTrack::GetModulationSysex(int msp)
{
  unsigned char *valp = gpSynth->GetSysexValPtr(ModulationSettings[msp]);

  if (valp)
  {
    return *valp + 1;
  }

  return 0;
}

void JZTrack::SetModulationSysex(int msp, int Value)
{
  if (ModulationSettings[msp])
  {
    Kill(ModulationSettings[msp]);
  }
  if (Value > 0)
  {
    JZEvent *e = gpSynth->ModSX(msp, 0, Channel, Value - 1);
    if (e)
    {
      Put(e);
      gpMidiPlayer->OutNow(this, e);
    }
  }
  Cleanup();
}

// ------------------------  Bender Sysex ------------------------------

int JZTrack::GetBenderSysex(int bsp)
{
  unsigned char *valp = gpSynth->GetSysexValPtr(BenderSettings[bsp]);

  if (valp)
  {
    return *valp + 1;
  }

  return 0;
}

void JZTrack::SetBenderSysex(int bsp, int Value)
{
  if (BenderSettings[bsp])
  {
    Kill(BenderSettings[bsp]);
  }
  if (Value > 0)
  {
    JZEvent *e = gpSynth->BendSX(bsp, 0, Channel, Value - 1);
    if (e)
    {
      Put(e);
      gpMidiPlayer->OutNow(this, e);
    }
  }
  Cleanup();
}

// ------------------------  CAf Sysex ------------------------------

int JZTrack::GetCAfSysex(int csp)
{
  unsigned char *valp = gpSynth->GetSysexValPtr(CAfSettings[csp]);

  if (valp)
  {
    return *valp + 1;
  }

  return 0;
}

void JZTrack::SetCAfSysex(int csp, int Value)
{
  if (CAfSettings[csp])
  {
    Kill(CAfSettings[csp]);
  }
  if (Value > 0)
  {
    JZEvent *e = gpSynth->CafSX(csp, 0, Channel, Value - 1);
    if (e)
    {
      Put(e);
      gpMidiPlayer->OutNow(this, e);
    }
  }
  Cleanup();
}

// ------------------------  PAf Sysex ------------------------------

int JZTrack::GetPAfSysex(int psp)
{
  unsigned char *valp = gpSynth->GetSysexValPtr(PAfSettings[psp]);

  if (valp)
  {
    return *valp + 1;
  }

  return 0;
}

void JZTrack::SetPAfSysex(int psp, int Value)
{
  if (PAfSettings[psp])
  {
    Kill(PAfSettings[psp]);
  }
  if (Value > 0)
  {
    JZEvent *e = gpSynth->PafSX(psp, 0, Channel, Value - 1);
    if (e)
    {
      Put(e);
      gpMidiPlayer->OutNow(this, e);
    }
  }
  Cleanup();
}

// ------------------------  CC1 Sysex ------------------------------

int JZTrack::GetCC1Sysex(int csp)
{
  unsigned char *valp = gpSynth->GetSysexValPtr(CC1Settings[csp]);

  if (valp)
  {
    return *valp + 1;
  }

  return 0;
}

void JZTrack::SetCC1Sysex(int csp, int Value)
{
  if (CC1Settings[csp])
  {
    Kill(CC1Settings[csp]);
  }
  if (Value > 0)
  {
    JZEvent *e = gpSynth->CC1SX(csp, 0, Channel, Value - 1);
    if (e)
    {
      Put(e);
      gpMidiPlayer->OutNow(this, e);
    }
  }
  Cleanup();
}

// ------------------------  CC2 Sysex ------------------------------

int JZTrack::GetCC2Sysex(int csp)
{
  unsigned char *valp = gpSynth->GetSysexValPtr(CC2Settings[csp]);

  if (valp)
  {
    return *valp + 1;
  }

  return 0;
}

void JZTrack::SetCC2Sysex(int csp, int Value)
{
  if (CC2Settings[csp])
    Kill(CC2Settings[csp]);
  if (Value > 0)
  {
    JZEvent *e = gpSynth->CC2SX(csp, 0, Channel, Value - 1);
    if (e)
    {
      Put(e);
      gpMidiPlayer->OutNow(this, e);
    }
  }
  Cleanup();
}

// ------------------------  CC1ControllerNr Sysex ------------------------------

int JZTrack::GetCC1ControllerNr()
{
  unsigned char *valp = gpSynth->GetSysexValPtr(CC1ControllerNr);

  if (valp)
  {
    return *valp + 1;
  }

  return 0;
}

void JZTrack::SetCC1ControllerNr(int Value)
{
  if (CC1ControllerNr)
  {
    Kill(CC1ControllerNr);
  }
  if (Value > 0)
  {
    JZEvent *e = gpSynth->ControllerNumberSX(1, 0, Channel, Value - 1);
    if (e)
    {
      Put(e);
      gpMidiPlayer->OutNow(this, e);
    }
  }
  Cleanup();
}

// ------------------------  CC2ControllerNr Sysex ------------------------------

int JZTrack::GetCC2ControllerNr()
{
  unsigned char *valp = gpSynth->GetSysexValPtr(CC2ControllerNr);

  if (valp)
  {
    return *valp + 1;
  }

  return 0;
}

void JZTrack::SetCC2ControllerNr(int Value)
{
  if (CC2ControllerNr)
  {
    Kill(CC2ControllerNr);
  }
  if (Value > 0)
  {
    JZEvent *e = gpSynth->ControllerNumberSX(2, 0, Channel, Value - 1);
    if (e)
    {
      Put(e);
      gpMidiPlayer->OutNow(this, e);
    }
  }
  Cleanup();
}

// ------------------------  Reverb Type ------------------------------

int JZTrack::GetReverbType(int lsb)
{
  unsigned char *valp = gpSynth->GetSysexValPtr(ReverbType);

  if (valp)
  {
    if (lsb)
    {
      ++valp;
    }
    return *valp + 1;
  }

 return 0;
}

void JZTrack::SetReverbType(int Value, int lsb)
{
  if (ReverbType)
  {
    Kill(ReverbType);
  }

  if (Value > 0)
  {
    JZEvent *e = gpSynth->ReverbMacroSX(0, Value - 1, lsb - 1);
    if (e)
    {
      Put(e);
      if (gpConfig->GetValue(C_UseReverbMacro))
      {
        gpMidiPlayer->OutNow(this, e);
      }
    }
  }
  Cleanup();
}

// ------------------------  Chorus Type ------------------------------

int JZTrack::GetChorusType(int lsb)
{
  unsigned char *valp = gpSynth->GetSysexValPtr(ChorusType);

  if (valp)
  {
    if (lsb)
    {
      ++valp;
    }

    return *valp + 1;
  }

  return 0;
}

void JZTrack::SetChorusType(int Value, int lsb)
{
  if (ChorusType)
  {
    Kill(ChorusType);
  }

  if (Value > 0)
  {
    JZEvent *e = gpSynth->ChorusMacroSX(0, Value - 1, lsb - 1);
    if (e)
    {
      Put(e);
      if (gpConfig->GetValue(C_UseChorusMacro))
      {
        gpMidiPlayer->OutNow(this, e);
      }
    }
  }
  Cleanup();
}

// -----------------------  Equalizer Type ------------------------------

int JZTrack::GetEqualizerType()
{
  unsigned char *valp = gpSynth->GetSysexValPtr(EqualizerType);

  if (valp)
  {
    return *valp + 1;
  }

  return 0;
}

void JZTrack::SetEqualizerType(int Value)
{
  if (EqualizerType)
  {
    Kill(EqualizerType);
  }

  if (Value > 0)
  {
    JZEvent *e = gpSynth->EqualizerMacroSX(0, Value - 1);
    if (e)
    {
      Put(e);
      gpMidiPlayer->OutNow(this, e);
    }
  }
  Cleanup();
}

// ------------------------  Reverb Parameters Sysex ------------------------------

int JZTrack::GetRevSysex(int rsp)
{
  unsigned char *valp = gpSynth->GetSysexValPtr(ReverbSettings[rsp]);

  if (valp)
  {
    return *valp + 1;
  }

  return 0;
}

void JZTrack::SetRevSysex(int rsp, int Value)
{
  if (ReverbSettings[rsp])
  {
    Kill(ReverbSettings[rsp]);
  }

  if (Value > 0)
  {
    JZEvent *e = gpSynth->ReverbParamSX(rsp, 0, Value - 1);
    if (e)
    {
      Put(e);
      if (!gpConfig->GetValue(C_UseReverbMacro))
      {
        gpMidiPlayer->OutNow(this, e);
      }
    }
  }
  Cleanup();
}

// ------------------------  Chorus Parameters Sysex ------------------------------

int JZTrack::GetChoSysex(int csp)
{
  unsigned char *valp = gpSynth->GetSysexValPtr(ChorusSettings[csp]);

  if (valp)
  {
    return *valp + 1;
  }

  return 0;
}

void JZTrack::SetChoSysex(int csp, int Value)
{
  if (ChorusSettings[csp])
  {
    Kill(ChorusSettings[csp]);
  }

  if (Value > 0)
  {
    JZEvent *e = gpSynth->ChorusParamSX(csp, 0, Value - 1);
    if (e)
    {
      Put(e);
      if (!gpConfig->GetValue(C_UseChorusMacro))
      {
        gpMidiPlayer->OutNow(this, e);
      }
    }
  }
  Cleanup();
}


// ------------------------  Partial Reserve ------------------------------

int JZTrack::GetPartRsrv(int chan)
{
  unsigned char *valp = gpSynth->GetSysexValPtr(PartialReserve);

  if (valp)
  {
    return *(valp + sysex_channel(chan)) + 1;
  }

  return 0;
}

void JZTrack::SetPartRsrv(unsigned char *rsrv)
{
  if (PartialReserve)
  {
    Kill(PartialReserve);
  }

  if (rsrv)
  {
    JZEvent *e = gpSynth->PartialReserveSX(0, Channel, rsrv);
    if (e)
    {
      Put(e);
      gpMidiPlayer->OutNow(this, e);
    }
  }
  Cleanup();
}

// ------------------------  Master Volume ------------------------------

int JZTrack::GetMasterVol()
{
  unsigned char *valp = gpSynth->GetSysexValPtr(MasterVol);

  if (valp)
  {
    if (gpSynth->GetSysexId(MasterVol) == SX_GM_MasterVol)
    {
      // first data byte is lsb; get msb instead!
      ++valp;
    }

    return *valp + 1;
  }

  return 0;
}

void JZTrack::SetMasterVol(int Value)
{
  if (MasterVol)
  {
    Kill(MasterVol);
  }
  if (Value > 0)
  {
    JZEvent *e = gpSynth->MasterVolSX(0, Value - 1);
    if (e)
    {
      Put(e);
      gpMidiPlayer->OutNow(this, e);
    }
  }
  Cleanup();
}


// ------------------------  Master Pan ------------------------------

int JZTrack::GetMasterPan()
{
  unsigned char *valp = gpSynth->GetSysexValPtr(MasterPan);

  if (valp)
  {
    return *valp + 1;
  }

  return 0;
}

void JZTrack::SetMasterPan(int Value)
{
  if (MasterPan)
  {
    Kill(MasterPan);
  }
  if (Value > 0)
  {
     JZEvent *e = gpSynth->MasterPanSX(0, Value - 1);
    if (e)
    {
       Put(e);
       gpMidiPlayer->OutNow(this, e);
    }
  }
  Cleanup();
}

// ------------------------  Mode Sysex ------------------------------

int JZTrack::GetModeSysex(int param)
{
   unsigned char *valp = 0;

   switch (param)
   {
     case mspRxChannel:
       valp = gpSynth->GetSysexValPtr(RxChannel);
       break;

     case mspUseForRhythm:
       valp = gpSynth->GetSysexValPtr(UseForRhythm);
       break;
   }

   if (valp)
   {
     return *valp + 1;
   }

   return 0;
}

void JZTrack::SetModeSysex(int param, int Value)
{
  switch (param)
  {
    case mspRxChannel:
      if (RxChannel)
      {
        Kill(RxChannel);
      }
      break;

    case mspUseForRhythm:
      if (UseForRhythm)
      {
        Kill(UseForRhythm);
      }
      break;
  }

  JZEvent *e = 0;

  if (Value > 0)
  {
    switch (param)
    {
      case mspRxChannel:
        e = gpSynth->RxChannelSX(0, Channel, Value - 1);
        break;
      case mspUseForRhythm:
        e = gpSynth->UseForRhythmSX(0, Channel, Value - 1);
        break;
    }

    if (e)
    {
      Put(e);
      gpMidiPlayer->OutNow(this, e);
    }
  }
  Cleanup();
}

// ------------------------  Mtc offset (Time Code Offset) ------------------------------

tMtcTime* JZTrack::GetMtcOffset()
{
  if (MtcOffset)
  {
    return new tMtcTime(MtcOffset);
  }
  return(new tMtcTime(0, Mtc30Ndf));
}

void JZTrack::SetMtcOffset(tMtcTime* mtc)
{
  if (MtcOffset)
  {
    Kill(MtcOffset);
  }
  if (mtc)
  {
    JZEvent *e = mtc->ToOffset();
    Put(e);
  }
  Cleanup();
}

// ------------------------  Speed ------------------------------

int JZTrack::GetDefaultSpeed()
{
  if (Speed)
  {
    return Speed->GetBPM();
  }
  return 120;
}


void JZTrack::SetDefaultSpeed(int bpm)
{
  JZEvent *e = new tSetTempo(0, bpm);
  if (Speed)
  {
    Kill(Speed);
  }
  Put(e);
  gpMidiPlayer->OutNow(this, e);
  Cleanup();
}

tSetTempo *JZTrack::GetCurrentTempo(int clk)
{
  tEventIterator Iterator(this);
  Sort();
  JZEvent *e = Iterator.Range(0, clk + 1);
  tSetTempo *t = Speed;
  while (e)
  {
    if (e->IsSetTempo())
    {
      t = e->IsSetTempo();
    }
    e = Iterator.Next();
  } // while e

  return t;
}

int JZTrack::GetCurrentSpeed(int clk)
{
  tSetTempo *t = GetCurrentTempo(clk);
  if (t)
  {
    return t->GetBPM();
  }
  return 120;
}



// ------------------------- State ----------------------------------


const char* JZTrack::GetStateChar()
{
  switch (State)
  {
    case tsPlay:
      return "P";
    case tsMute:
      return "M";
    case tsSolo:
      return "S";
  }
  return "?";
}

void JZTrack::SetState(int NewState)
{
  State = NewState % 3;
}

void JZTrack::ToggleState(int Direction)
{
  State = (State + Direction + 3) % 3;
}

// ------------------------- Channel ---------------------------

void JZTrack::SetChannel(int NewChannel)
{
  Channel = NewChannel;
}
