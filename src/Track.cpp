//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Copyright (C) 1994-2000 Andreas Voss and Per Sigmond, all rights reserved.
// Modifications Copyright (C) 2004 Patrick Earl
// Modifications Copyright (C) 2008-2010 Peter J. Stieber
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

#include "Track.h"

#include "Configuration.h"
#include "Dialogs/TrackDialog.h"
#include "Globals.h"
#include "JazzPlusPlusApplication.h"
#include "Player.h"
#include "Song.h"
#include "Synth.h"
#include "TrackWindow.h"

#include <cassert>
#include <cstdlib>
#include <sstream>

using namespace std;

int JZParam::Write(JZWriteBase& Io)
{
  return mMsb.Write(Io) + mLsb.Write(Io) + mDataMsb.Write(Io);
}

void JZParam::SetCha(unsigned char Channel)
{
  mMsb.SetChannel(Channel);
  mLsb.SetChannel(Channel);
  mDataMsb.SetChannel(Channel);

#ifdef OBSOLETE
  mResetMb.SetChannel(Channel); //???? JAVE commented out this while porting
#endif // OBSOLETE

  mResetLsb.SetChannel(Channel);
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

static double gFramesPerSecond[] =
{
  24.0,
  25.0,
  30.0,
  30.0
};

JZMtcTime::JZMtcTime(JZMtcOffsetEvent* pMtcOffset)
{
  const unsigned char* pData = pMtcOffset->GetData();
  type = (tMtcType) ((pData[0] & 0x60) >> 5);
  if (type < Mtc24)
  {
    type = Mtc24;
  }
  if (type > Mtc30Ndf)
  {
    type = Mtc30Ndf;
  }
  hour = pData[0] & 0x1f;
  min = pData[1];
  sec = pData[2];
  fm = pData[3];
}

JZMtcTime::JZMtcTime(int millisec, tMtcType t)
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
  double frametime = 1000.0 / gFramesPerSecond[type];
  fm = (int) ((double) msec / frametime);
}

JZMtcTime::JZMtcTime(char* str, tMtcType t)
  : hour(0),
    min(0),
    sec(0),
    fm(0)
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
  sscanf(str, "%d:%d:%d.%d", &hour, &min, &sec, &fm);
  if (fm >= gFramesPerSecond[type])
  {
    fm = (int) gFramesPerSecond[type] - 1;
  }
}

JZMtcTime::JZMtcTime(unsigned h, unsigned m, unsigned s, unsigned f, unsigned t)
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

void JZMtcTime::ToString(string& String)
{
  ostringstream Oss;
  Oss << hour << ':' << min << ':' << sec << '.' << fm;
  String = Oss.str();
}

JZMtcOffsetEvent *JZMtcTime::ToOffset()
{
  unsigned char *mess = new unsigned char[5];
  mess[0] = (unsigned char) hour | ((unsigned char) type << 5);
  mess[1] = (unsigned char) min;
  mess[2] = (unsigned char) sec;
  mess[3] = (unsigned char) fm;
  mess[4] = 0x00;
  JZMtcOffsetEvent *s = new JZMtcOffsetEvent(0, mess, 5);
  delete mess;
  return s;
}

int JZMtcTime::ToMillisec()
{
  int msec = (((((hour * 60L) + min) * 60L) + sec) * 1000L) +
              ((fm * 1000L) / (int) gFramesPerSecond[type]);
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

JZDrumInstrumentParameter::JZDrumInstrumentParameter(JZNrpn *par)
  : mPitch(par->mLsb.GetControlValue()),
    mpNext(0)
{
  for (int i = drumPitchIndex; i < numDrumParameters; i++)
  {
    param[i] = 0;
  }
  param[drumParam2Index(par->mMsb.GetControlValue())] = par;
}

JZNrpn *JZDrumInstrumentParameter::Get(int index)
{
  assert((index >= drumPitchIndex) && (index < numDrumParameters));
  return(param[index]);
}

void JZDrumInstrumentParameter::Put(JZNrpn *par)
{
  param[par->mLsb.GetControlValue()] = par;
}

JZDrumInstrumentParameter *JZDrumInstrumentParameter::Next()
{
  return mpNext;
}

int JZDrumInstrumentParameter::Pitch()
{
  return mPitch;
}

JZDrumInstrumentParameter
*JZDrumInstrumentParameterList::GetElem(int pit)
{
  JZDrumInstrumentParameter *ptr = list;
  while (ptr)
  {
    if (ptr->mPitch == pit)
    {
      break;
    }
    ptr = ptr->mpNext;
  }
  return ptr;
}

JZNrpn *JZDrumInstrumentParameterList::GetParam(int pit, int index)
{
  JZDrumInstrumentParameter *ptr = GetElem(pit);
  if (ptr)
  {
    return ptr->Get(index);
  }
  return 0;
}

void JZDrumInstrumentParameterList::PutParam(JZNrpn *par)
{
  JZDrumInstrumentParameter* ptr = GetElem(par->mLsb.GetControlValue());
  if (!ptr)
  {
    ptr = new JZDrumInstrumentParameter(par);
    ptr->mpNext = list;
    list = ptr;
  }
  else
  {
    ptr->param[drumParam2Index(par->mMsb.GetControlValue())] = par;
  }
}

void JZDrumInstrumentParameterList::DelParam(int pit, int index)
{
  if (list)
  {
    JZDrumInstrumentParameter *elem = GetElem(pit);
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

void JZDrumInstrumentParameterList::DelElem(int pit)
{
  for (int i = drumPitchIndex; i < numDrumParameters; i++)
  {
    DelParam(pit, i);
  }

  JZDrumInstrumentParameter *ptr = list;
  JZDrumInstrumentParameter *prev = 0;
  while (ptr)
  {
    if (ptr->mPitch == pit)
    {
      if (prev)
      {
        prev->mpNext = ptr->mpNext;
      }
      else
      {
        list = ptr->mpNext;
      }
      delete ptr;
      break;
    }
    prev = ptr;
    ptr = ptr->mpNext;
  }
}

JZDrumInstrumentParameter *JZDrumInstrumentParameterList::FirstElem()
{
  return list;
}

JZDrumInstrumentParameter *JZDrumInstrumentParameterList::NextElem(
  JZDrumInstrumentParameter *cur)
{
  if (cur)
  {
    JZDrumInstrumentParameter *ptr = GetElem(cur->mPitch);
    if (ptr)
    {
      return ptr->mpNext;
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

void JZDrumInstrumentParameterList::Clear()
{
  JZDrumInstrumentParameter *ptr = list;
  while (ptr)
  {
    list = ptr->mpNext;
    delete ptr;
    ptr = list;
  }
}


JZSimpleEventArray::JZSimpleEventArray()
  : wxObject(),
    nEvents(0),
    MaxEvents(0),
    Events(0)
{
}

JZSimpleEventArray::~JZSimpleEventArray()
{
  Clear();
  delete [] Events;
  Events = 0;
}

void JZSimpleEventArray::Clear()
{
  int i;
  for (i = 0; i < nEvents; i++)
  {
    delete Events[i];
  }
  nEvents = 0;
}


void JZUndoBuffer::Clear()
{
  int i;
  for (i = 0; i < nEvents; i++)
  {
    if (bits(i))
    {
      delete Events[i];
    }
  }
  nEvents = 0;
}


void JZSimpleEventArray::Resize()
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
void JZSimpleEventArray::RemoveEOT()
{
  int j = 0;
  int newnEvents = nEvents;
  for (int i = 0; i < nEvents; ++i)
  {
    if (Events[i] != 0 && Events[i]->IsEndOfTrack())
    {
      delete Events[i];
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

void JZSimpleEventArray::Put(JZEvent* pEvent)
{
  if (pEvent->IsEndOfTrack())
  {
    // Remove the old EOT if we are adding a new one.
    RemoveEOT();
  }
  if (nEvents >= MaxEvents)
  {
    Resize();
  }
  Events[nEvents++] = pEvent;
}

// Description:
//   Move the data from passed event array this instance.
void JZSimpleEventArray::GrabData(JZSimpleEventArray& src)
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


void JZSimpleEventArray::Copy(JZSimpleEventArray& src, int frclk, int toclk)
{
  JZEventIterator iter(&src);
  JZEvent* pEvent = iter.Range(frclk, toclk);
  while (pEvent)
  {
    Put(pEvent->Copy());
    pEvent = iter.Next();
  }
}


JZEventArray::JZEventArray()
  : JZSimpleEventArray(),
    mpName(0),
    mpCopyright(0),
    mpPatch(0),
    mpSpeed(0),
    mpVolume(0),
    mpPan(0),
    mpReverb(0),
    mpChorus(0),
    mpBank(0),
    mpBank2(0),
    mpReset(0)
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


void JZEventArray::Clear()
{
  int i;

  JZSimpleEventArray::Clear();

//  delete mpName;
  mpName = 0;

//  delete mpCopyright;
  mpCopyright = 0;

  delete mpPatch;
  mpPatch = 0;

//  delete mpVolume;
  mpVolume = 0;

//  delete mpPan;
  mpPan = 0;

//  delete mpReverb;
  mpReverb = 0;

//  delete mpChorus;
  mpChorus = 0;

  delete mpBank;
  mpBank = 0;

  delete mpBank2;
  mpBank2 = 0;

  delete mpReset;
  mpReset = 0;

//  delete mpSpeed;
  mpSpeed = 0;

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


JZEventArray::~JZEventArray()
{
  Clear();
}





static int compare(const void *p1, const void *p2)
{
  JZEvent *e1 = *(JZEvent **)p1;
  JZEvent *e2 = *(JZEvent **)p2;
  return e1->Compare(*e2);
}


void JZSimpleEventArray::Sort()
{
  qsort(Events, nEvents, sizeof(JZEvent *), compare);
}



void JZEventArray::Cleanup(bool dont_delete_killed_events)
{
  JZEvent* pEvent;
  JZControlEvent* pControl;
  JZSysExEvent* s;
  int i;

  Sort();  // moves all killed events to the end of array

  // clear track defaults
//  delete mpName;
  mpName = 0;

  mpCopyright = 0;

  mpSpeed = 0;

  mpVolume = 0;

  mpPan = 0;

  mpReverb = 0;

  mpChorus = 0;

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
    if ((pEvent = Events[i])->IsKilled())
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
    if (pEvent->GetClock() != 0)
    {
      continue;
    }

    if (!mpName)
    {
      mpName = pEvent->IsTrackName();
    }

    if (!mpCopyright)
    {
      mpCopyright = pEvent->IsCopyright();
    }
    if (!mpSpeed)
    {
      mpSpeed = pEvent->IsSetTempo();
    }
    if (!MtcOffset)
    {
      MtcOffset = pEvent->IsMtcOffset();
    }
    if ((pControl = pEvent->IsControl()) != 0)
    {
      switch (pControl->GetControl())
      {
        case 0x07:
          if (!mpVolume)
          {
            mpVolume = pControl;
          }
          break;
        case 0x0a:
          if (!mpPan)
          {
            mpPan = pControl;
          }
          break;
        case 0x5b:
          if (!mpReverb)
          {
            mpReverb = pControl;
          }
          break;
        case 0x5d:
          if (!mpChorus)
          {
            mpChorus = pControl;
          }
          break;
      }
    }
    if ((s = pEvent->IsSysEx()) != 0)
    {
      int SysExId = gpSynth->GetSysexId(s);

      if (!gpSynth->IsGS())
      {
        switch (SysExId)
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
        switch (SysExId)
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
            BenderSettings[SysExId - SX_GS_BendPitch] = s;
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
            ModulationSettings[SysExId - SX_GS_ModPitch] = s;
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
            CAfSettings[SysExId - SX_GS_CafPitch] = s;
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
            PAfSettings[SysExId - SX_GS_PafPitch] = s;
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
            CC1Settings[SysExId - SX_GS_CC1Pitch] = s;
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
            CC2Settings[SysExId - SX_GS_CC2Pitch] = s;
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
            ReverbSettings[SysExId - SX_GS_RevCharacter] = s;
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
            ChorusSettings[SysExId - SX_GS_ChoPreLpf] = s;
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
        switch (SysExId)
        {
          case SX_XG_BendPitch:
          case SX_XG_BendTvf:
          case SX_XG_BendAmpl:
            BenderSettings[SysExId - SX_XG_BendPitch] = s;
            break;

          case SX_XG_BendLfoPitch:
          case SX_XG_BendLfoTvf:
          case SX_XG_BendLfoTva:
            BenderSettings[SysExId + 1 - SX_XG_BendPitch] = s;
            break;

          case SX_XG_ModPitch:
          case SX_XG_ModTvf:
          case SX_XG_ModAmpl:
            ModulationSettings[SysExId - SX_XG_ModPitch] = s;
            break;

          case SX_XG_ModLfoPitch:
          case SX_XG_ModLfoTvf:
          case SX_XG_ModLfoTva:
            ModulationSettings[SysExId + 1 - SX_XG_ModPitch] = s;
            break;

          case SX_XG_CafPitch:
          case SX_XG_CafTvf:
          case SX_XG_CafAmpl:
            CAfSettings[SysExId - SX_XG_CafPitch] = s;
            break;

          case SX_XG_CafLfoPitch:
          case SX_XG_CafLfoTvf:
          case SX_XG_CafLfoTva:
            CAfSettings[SysExId + 1 - SX_XG_CafPitch] = s;
            break;

          case SX_XG_PafPitch:
          case SX_XG_PafTvf:
          case SX_XG_PafAmpl:
            PAfSettings[SysExId - SX_XG_PafPitch] = s;
            break;

          case SX_XG_PafLfoPitch:
          case SX_XG_PafLfoTvf:
          case SX_XG_PafLfoTva:
            PAfSettings[SysExId + 1 - SX_XG_PafPitch] = s;
            break;

          case SX_XG_CC1Pitch:
          case SX_XG_CC1Tvf:
          case SX_XG_CC1Ampl:
            CC1Settings[SysExId - SX_XG_CC1Pitch] = s;
            break;

          case SX_XG_CC1LfoPitch:
          case SX_XG_CC1LfoTvf:
          case SX_XG_CC1LfoTva:
            CC1Settings[SysExId + 1 - SX_XG_CC1Pitch] = s;
            break;

          case SX_XG_CC2Pitch:
          case SX_XG_CC2Tvf:
          case SX_XG_CC2Ampl:
            CC2Settings[SysExId - SX_XG_CC2Pitch] = s;
            break;

          case SX_XG_CC2LfoPitch:
          case SX_XG_CC2LfoTvf:
          case SX_XG_CC2LfoTva:
            CC2Settings[SysExId + 1 - SX_XG_CC2Pitch] = s;
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


void JZEventArray::Length2Keyoff()
{
  int n = nEvents;
  for (int i = 0; i < n; i++)
  {
    JZKeyOnEvent* pKeyOn;
    if ((pKeyOn = Events[i]->IsKeyOn()) != 0 && pKeyOn->GetEventLength() != 0)
    {
//      JZEvent* pKeyOff = new JZKeyOffEvent(
//        pKeyOn->GetClock() + pKeyOn->GetEventLength(),
//        pKeyOn->Channel,
//        pKeyOn->Key);

      // SN++ added off veloc
      JZEvent* pKeyOff = new JZKeyOffEvent(
        pKeyOn->GetClock() + pKeyOn->GetEventLength(),
        pKeyOn->GetChannel(),
        pKeyOn->GetKey(),
        pKeyOn->GetOffVelocity());

      pKeyOn->SetLength(0);
      pKeyOff->SetDevice(pKeyOn->GetDevice());
      Put(pKeyOff);
    }
  }
  Sort();
}


#if 0

void JZEventArray::Keyoff2Length()
{
  int i;
  for (i = 1; i < nEvents; i++)
  {
    JZKeyOffEvent* pKeyOff;
    if ((pKeyOff = Events[i]->IsKeyOff()) != 0)
    {
      JZEvent** ppEvent = &Events[i - 1];
      while (ppEvent >= Events)
      {
        JZKeyOnEvent* pKeyOn = (*ppEvent)->IsKeyOn();
        if (
          pKeyOn &&
          pKeyOn->Key == pKeyOff->Key &&
          pKeyOn->Channel == pKeyOff->Channel &&
          pKeyOn->Length == 0)
        {
          pKeyOn->Length = pKeyOff->GetClock() - pKeyOn->GetClock();
          if (pKeyOn->Length <= 0L)
          {
            pKeyOn->Length = 1;
          }
          pKeyOff->Kill();
          break;
        }
        --ppEvent;
      }
    }
  }

  // kill all KeyOn's with non matching KeyOff's
  for (i = 0; i < nEvents; i++)
  {
    JZKeyOnEvent *k = Events[i]->IsKeyOn();
    if (k && k->Length <= 0)
    {
      k->Kill();
    }
  }
  Cleanup(0);
}

#else

void JZEventArray::Keyoff2Length()
{
  // Searches forward from a KeyOn to find the matching KeyOff.
  // This is compatible with Cubase.
  int i;
  for (i = 0; i < nEvents; i++)
  {
    JZKeyOnEvent* pKeyOn;
    if ((pKeyOn = Events[i]->IsKeyOn()) != 0 && pKeyOn->GetEventLength() == 0)
    {
      int j;
      for (j = i + 1; j < nEvents; j++)
      {
        JZKeyOffEvent* pKeyOff = Events[j]->IsKeyOff();
        if (
          pKeyOff &&
          !pKeyOff->IsKilled() &&
          pKeyOn->GetKey() == pKeyOff->GetKey() &&
          pKeyOn->GetChannel() == pKeyOff->GetChannel())
        {
          pKeyOn->SetLength(pKeyOff->GetClock() - pKeyOn->GetClock());
          if (pKeyOn->GetEventLength() <= 0)
          {
            pKeyOn->SetLength(1);
          }
          pKeyOff->Kill();
          break;
        }
      }
    }
  }

  // kill all KeyOn's with non matching KeyOff's
  // and kill all remaining KeyOff's
  for (i = 0; i < nEvents; i++)
  {
    JZKeyOnEvent* pKeyOn = Events[i]->IsKeyOn();
    if (pKeyOn && pKeyOn->GetEventLength() <= 0)
    {
      pKeyOn->Kill();
    }
    JZKeyOffEvent* pKeyOff = Events[i]->IsKeyOff();
    if (pKeyOff)
    {
      pKeyOff->Kill();
    }
  }
  Cleanup(0);
}

#endif


void JZEventArray::Write(JZWriteBase& Io)
{
  JZEvent* pEvent;
  int WrittenBefore;

  Length2Keyoff();
  Io.NextTrack();

  // Write copyright notice first (according to spec):
  if (mpCopyright)
  {
    mpCopyright->Write(Io);
  }

  // Write MTC offset before any transmittable events (spec)
  if (MtcOffset)
  {
    MtcOffset->Write(Io);
  }

  // Synth reset
  if (mpReset)
  {
    mpReset->Write(Io);
  }

  // Rpn / Nrpn:
  // All these must be written in order (three JZControlEvent's in a row)
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

  JZDrumInstrumentParameter *dpar = DrumParams.FirstElem();
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

  if (mpPatch)
  {
    mpPatch->Write(Io);
  }

  JZJazzMetaEvent JazzMeta;
  JazzMeta.SetAudioMode(audio_mode);
  JazzMeta.SetTrackState(State);
  JazzMeta.SetTrackDevice(Device);
  JazzMeta.SetIntroLength(gpSong->GetIntroLength());
  JazzMeta.Write(Io);

  for (int i = 0; i < nEvents; i++)
  {
    pEvent = Events[i];
    WrittenBefore = 0;
    if (pEvent->IsControl())
    {
      switch (pEvent->IsControl()->GetControl())
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
          if (pEvent->GetClock() == 0)
          {
            WrittenBefore = 1;
          }
          break;
        default:
          WrittenBefore = 0;
      }
    }
    else if (pEvent->IsProgram())
    {
      // Don't write these again if present as events
      // and clock == 0 (should not happen)
      if (pEvent->GetClock() == 0)
      {
        WrittenBefore = 1;
      }
    }
    else if (pEvent->IsCopyright() || pEvent->IsMtcOffset())
    {
      // Will probably happen
      WrittenBefore = 1;
    }
    if (!WrittenBefore)
    {
      pEvent->Write(Io);
    }
  }
  Keyoff2Length();
}

void JZEventArray::Read(JZReadBase& Io)
{
  JZEvent* pEvent;
  Channel = 0;
  unsigned char Msb, Lsb, Data;
  bool SpecialEvent;

  Msb = Lsb = Data = 0xff;
  int cha;

  bool NeedToDelete;

  Io.NextTrack();
  while ((pEvent = Io.Read()) != 0)
  {
    NeedToDelete = false;
    SpecialEvent = false;
    if (pEvent->IsJazzMeta())
    {
      JZJazzMetaEvent* j = pEvent->IsJazzMeta();
      audio_mode = (int)j->GetAudioMode();
      State      = (int)j->GetTrackState();
      Device     = (int)j->GetTrackDevice();
      gpSong->SetIntroLength((int)j->GetIntroLength());
      delete j;
      continue;
    }
    if (pEvent->IsControl())
    {
      switch (pEvent->IsControl()->GetControl())
      {
        // Grab Rpn/Nrpn/Bank from file and save them, don't put
        // them into event-array
        case 0x63:
        case 0x65:
          Msb = pEvent->IsControl()->GetControlValue(); // Rpn/Nrpn Msb
          SpecialEvent = true;
          break;
        case 0x62:
        case 0x64:
          Lsb = pEvent->IsControl()->GetControlValue(); // Rpn/Nrpn Lsb
          SpecialEvent = true;
          break;
        case 0x06:
          Data = pEvent->IsControl()->GetControlValue(); // Rpn/Nrpn Data
          SpecialEvent = true;
          cha = pEvent->IsControl()->GetChannel();
          switch (Msb)
          {
            case 0x01: // Nrpn
              switch (Lsb)
              {
                case 0x08:
                  if (!VibRate)
                  {
                    VibRate = new JZNrpn(0, cha, Msb, Lsb, Data);
                  }
                  break;
                case 0x09:
                  if (!VibDepth)
                  {
                    VibDepth = new JZNrpn(0, cha, Msb, Lsb, Data);
                  }
                  break;
                case 0x0a:
                  if (!VibDelay)
                  {
                    VibDelay = new JZNrpn(0, cha, Msb, Lsb, Data);
                  }
                  break;
                case 0x20:
                  if (!Cutoff)
                  {
                    Cutoff = new JZNrpn(0, cha, Msb, Lsb, Data);
                  }
                  break;
                case 0x21:
                  if (!Resonance)
                  {
                    Resonance = new JZNrpn(0, cha, Msb, Lsb, Data);
                  }
                  break;
                case 0x63:
                  if (!EnvAttack)
                  {
                    EnvAttack = new JZNrpn(0, cha, Msb, Lsb, Data);
                  }
                  break;
                case 0x64:
                  if (!EnvDecay)
                  {
                    EnvDecay = new JZNrpn(0, cha, Msb, Lsb, Data);
                  }
                  break;
                case 0x66:
                  if (!EnvRelease)
                  {
                    EnvRelease = new JZNrpn(0, cha, Msb, Lsb, Data);
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
              DrumParams.PutParam(new JZNrpn(0, cha, Msb, Lsb, Data));
              break;
            case 0x00: // Rpn
              if (Lsb == 0x00)
              {
                // Pitch Bend Sensivity
                if (!BendPitchSens)
                {
                  BendPitchSens = new JZRpn(0, cha, Msb, Lsb, Data);
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
            SpecialEvent = true;
            mpBank = pEvent->IsControl();
            mpBank->SetClock(0);
          }
          break;
        case 0x20:
          if (!mpBank2)
          {
            SpecialEvent = true;
            mpBank2 = pEvent->IsControl();
            mpBank2->SetClock(0);
          }
          break;
        default:
          SpecialEvent = false; // Other control
          break;
      }
    }
    else if (pEvent->IsProgram())
    {
      if (!mpPatch)
      {
        mpPatch = pEvent->IsProgram();
        mpPatch->SetClock(0);
        SpecialEvent = true;
      }
    }
    else if (pEvent->IsCopyright())
    {
      if (!mpCopyright)
      {
        mpCopyright = pEvent->IsCopyright();

        // Just make sure clock is zero, then put into event array
        mpCopyright->SetClock(0);
      }
    }
    else if (pEvent->IsSysEx())
    {
      NeedToDelete = true;

      // Get hold of the Reset sysex...
      int SysExId = gpSynth->GetSysexId(pEvent->IsSysEx());

      if (SysExId == SX_GM_ON || SysExId == SX_GS_ON || SysExId == SX_XG_ON)
      {
        // Take them all away
        SpecialEvent = true;

        // Save it in the track defaults if it fits with synth
        // type settings
        if (
          (gpSynth->IsGM() && (SysExId == SX_GM_ON)) ||
          (gpSynth->IsGS() && (SysExId == SX_GS_ON)) ||
          (gpSynth->IsXG() && (SysExId == SX_XG_ON)))
        {
          if (!mpReset)
          {
            mpReset = pEvent->IsSysEx();
            NeedToDelete = false;
          }
        }
      }
    }

    if (!SpecialEvent)
    {
      Put(pEvent);
      NeedToDelete = false;
      if (!Channel && pEvent->IsChannelEvent())
      {
        Channel = pEvent->IsChannelEvent()->GetChannel() + 1;
      }
    }
    if (pEvent->IsEndOfTrack())
    {
      // JAVE I want explicit end of track events
      // Break out of loop here because endoftrack is end, and we want it read
      // FIXME we shoulnt break, we should keep on reading, and handle eot in
      // track play instead.
      break;
    }

    if (NeedToDelete)
    {
      delete pEvent;
    }

  } // while read

  if (!Channel)
  {
    Channel = 1;
  }

  Keyoff2Length();
}


int JZEventArray::GetLastClock() const
{
  if (!nEvents)
  {
    return 0;
  }
  return Events[nEvents - 1]->GetClock();
}

bool JZEventArray::IsEmpty() const
{
  return nEvents == 0;
}

int JZEventArray::GetFirstClock()
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

class JZTrackDlg : public wxForm
{
  JZTrackWindow* TrackWin;
  JZTrack *trk;
  char *TrackName;
  JZNamedChoice PatchChoice;
  JZNamedChoice DeviceChoice;
  int PatchNr;
  int Device;
  int BankNr;
  int ClearTrack;
  int AudioMode;

 public:
  JZTrackDlg::JZTrackDlg(JZTrackWindow *w, JZTrack *t);
  void EditForm(wxPanel *panel);
  virtual void OnOk();
  virtual void OnCancel();
  virtual void OnHelp();
};


JZTrackDlg::JZTrackDlg(JZTrackWindow *w, JZTrack *t)
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

void JZTrackDlg::OnCancel()
{
  trk->mpDialog = 0;
  TrackWin->Redraw();
  wxForm::OnCancel();
}

void JZTrackDlg::OnHelp()
{
  gpHelpInstance->ShowTopic("Trackname, midi channel etc");
}

void JZTrackDlg::OnOk()
{
  trk->mpDialog->GetPosition(&Config(C_TrackDlgXpos), &Config(C_TrackDlgYpos));
  trk->mpDialog = 0;
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
    JZChannelEvent *c;
    JZSysExEvent* s;
    JZEventIterator Iterator(trk);
    trk->Sort();
    JZEvent* pEvent = Iterator.Range(0, (unsigned) trk->GetLastClock() + 1);
    while (pEvent)
    {
      if ((c = pEvent->IsChannelEvent()) != 0)
      {
        c = (JZChannelEvent *)pEvent->Copy();
        c->SetChannel(trk->Channel - 1);
        trk->Kill(pEvent);
        trk->Put(c);
      }
      else if ((s = pEvent->IsSysEx()) != 0)
      {
        // Check for sysex that contains channel number
        const unsigned char* pChannel = gpSynth->GetSysexChaPtr(s);
        if (pChannel)
        {
          if (gpSynth->IsXG())
          {
            *pChannel = trk->Channel - 1;
          }
          else
          {
            *pChannel &= 0xf0;
            *pChannel |= sysex_channel(trk->Channel);
          }

          s = (JZSysExEvent *) pEvent->Copy();
          trk->Kill(pEvent);
          trk->Put(s);
        }
      }
      pEvent = Iterator.Next();
    } // while pEvent

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
    if (trk->mpPatch)
    {
      trk->mpPatch->Channel = trk->Channel - 1;
    }
    if (!trk->DrumParams.IsEmpty())
    {
      JZDrumInstrumentParameter *dpar = trk->DrumParams.FirstElem();
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

void JZTrackDlg::EditForm(wxPanel *panel)
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
  mpDialog = new wxDialogBox(
    pParent,
    "Track Settings",
    modal,
    Config(C_TrackDlgXpos),
    Config(C_TrackDlgYpos));
#endif // OBSOLETE
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool JZTrack::IsEditing() const
{
  if (mpDialog)
  {
    return (mpDialog->GetHandle() != 0);
  }
  return false;
}


//*****************************************************************************
// Description:
//   This is the track class definition.
//*****************************************************************************
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool JZTrack::mChanged = false;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZTrack::JZTrack()
  : JZEventArray(),
    mUndoIndex(0),
    mRedoCount(0),
    mUndoCount(0),
    mpDialog(0)
{
  ForceChannel = 1;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZTrack::~JZTrack()
{
  Clear();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool JZTrack::IsDrumTrack()
{
  return Channel == gpConfig->GetValue(C_DrumChannel);
}

void JZTrack::Merge(JZEventArray *t)
{
  for (int i = 0; i < t->nEvents; i++)
  {
    Put(t->Events[i]);
  }
  t->nEvents = 0;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZTrack::MergeRange(
  const JZEventArray& Other,
  int FromClock,
  int ToClock,
  int Replace)
{
  // Erase destin
  if (Replace)
  {
    JZEventIterator Erase(this);
    JZEvent* pEvent = Erase.Range(FromClock, ToClock);
    while (pEvent)
    {
      Kill(pEvent);
      pEvent = Erase.Next();
    }
  }

  // Merge Recorded Events
  JZEventIterator Copy(&Other);
  JZEvent* pEvent = Copy.Range(FromClock, ToClock);
  while (pEvent)
  {
    JZEvent* c = pEvent->Copy();
    if (ForceChannel)
    {
      JZChannelEvent* pChannelEvent = c->IsChannelEvent();
      if (pChannelEvent)
      {
        pChannelEvent->SetChannel(Channel - 1);
      }
    }
    Put(c);
    pEvent = Copy.Next();
  }
  Cleanup();
}


void JZTrack::Cleanup()
{
  // on audio tracks, adjust length of keyon events to
  // actual sample length
  gpMidiPlayer->AdjustAudioLength(this);
  JZEventArray::Cleanup(TRUE);
}




void JZTrack::Undo()
{
  if (mUndoCount > 0)
  {
    JZUndoBuffer *undo = &mUndoBuffers[mUndoIndex];
    for (int i = undo->nEvents - 1; i >= 0; i--)
    {
      JZEvent* pEvent = undo->Events[i];
      if (undo->bits(i))
      {
        undo->bits.set(i, 0);
        pEvent->UnKill();
        JZEventArray::Put(pEvent);
      }
      else
      {
        undo->bits.set(i, 1);
        pEvent->Kill();
      }
    }
    JZEventArray::Cleanup(TRUE);

    mUndoIndex = (mUndoIndex - 1 + MaxUndo) % MaxUndo;
    --mUndoCount;
    ++mRedoCount;
  }
}

void JZTrack::Redo()
{
  if (mRedoCount > 0)
  {
    mUndoIndex = (mUndoIndex + 1) % MaxUndo;

    JZUndoBuffer *undo = &mUndoBuffers[mUndoIndex];
    for (int i = 0; i < undo->nEvents; i++)
    {
      JZEvent* pEvent = undo->Events[i];
      if (undo->bits(i))
      {
        undo->bits.set(i, 0);
        pEvent->UnKill();
        JZEventArray::Put(pEvent);
      }
      else
      {
        undo->bits.set(i, 1);
        pEvent->Kill();
      }
    }
    JZEventArray::Cleanup(TRUE);

    --mRedoCount;
    ++mUndoCount;
  }
}


void JZTrack::NewUndoBuffer()
{
  mRedoCount = 0;
  ++mUndoCount;
  if (mUndoCount > MaxUndo)
  {
    mUndoCount = MaxUndo;
  }

  mUndoIndex = (mUndoIndex + 1) % MaxUndo;
  mUndoBuffers[mUndoIndex].Clear();
};


void JZTrack::Clear()
{
  for (int i = 0; i < MaxUndo; i++)
  {
    mUndoBuffers[i].Clear();
  }
  State  = tsPlay;
  JZEventArray::Clear();
}

// ----------------------- Copyright ------------------------------------

const char* JZTrack::GetCopyright()
{
  if (mpCopyright)
  {
    return (const char *)mpCopyright->GetData();
  }
  return "";
}



void JZTrack::SetCopyright(char *str)
{
  if (mpCopyright)
  {
    Kill(mpCopyright);
  }
  if (str && strlen(str))
  {
    int len = 127;
    if ((int)strlen(str) < len)
    {
      len = strlen(str);
    }
    Put(new JZCopyrightEvent(0, (unsigned char *)str, len));
  }
  Cleanup();
}

// ----------------------- Name ------------------------------------

const char* JZTrack::GetName()
{
  if (mpName)
  {
    return (const char*)mpName->GetData();
  }
  return "";
}

void JZTrack::SetName(const char* pTrackName)
{
  if (mpName)
  {
    Kill(mpName);
  }
  if (strlen(pTrackName))
  {
    Put(new JZTrackNameEvent(
      0,
      (unsigned char *)pTrackName,
      strlen(pTrackName)));
  }
  Cleanup();
}

// ------------------------ Volume ------------------------------

int JZTrack::GetVolume()
{
  if (mpVolume)
  {
    return mpVolume->GetControlValue() + 1;
  }
  return 0;
}

void JZTrack::SetVolume(int Value)
{
  if (mpVolume)
  {
    Kill(mpVolume);
  }
  if (Value > 0)
  {
    JZEvent* pEvent = new JZControlEvent(0, Channel - 1, 0x07, Value - 1);
    Put(pEvent);
    gpMidiPlayer->OutNow(this, pEvent);
  }
  Cleanup();
}

bool JZTrack::DecreaseVolume()
{
  if (mpVolume && mpVolume->GetControlValue() > 0)
  {
    Kill(mpVolume);

    mpVolume->SetControlValue(mpVolume->GetControlValue() - 1);

    JZEvent* pEvent = new JZControlEvent(
      0,
      Channel - 1,
      0x07,
      mpVolume->GetControlValue());
    Put(pEvent);
    gpMidiPlayer->OutNow(this, pEvent);

    Cleanup();

    return true;
  }
  return false;
}

bool JZTrack::IncreaseVolume()
{
  if (mpVolume && mpVolume->GetControlValue() < 127)
  {
    Kill(mpVolume);

    mpVolume->SetControlValue(mpVolume->GetControlValue() + 1);

    JZEvent* pEvent = new JZControlEvent(
      0,
      Channel - 1,
      0x07,
      mpVolume->GetControlValue());

    Put(pEvent);
    gpMidiPlayer->OutNow(this, pEvent);

    Cleanup();

    return true;
  }
  return false;
}

// ------------------------ Pan ------------------------------

int JZTrack::GetPan()
{
  if (mpPan)
  {
    return mpPan->GetControlValue() + 1;
  }
  return 0;
}

void JZTrack::SetPan(int Value)
{
  if (mpPan)
  {
    Kill(mpPan);
  }
  if (Value > 0)
  {
    JZEvent* pEvent = new JZControlEvent(0, Channel - 1, 0x0a, Value - 1);
    Put(pEvent);
    gpMidiPlayer->OutNow(this, pEvent);
  }
  Cleanup();
}

// ------------------------ Reverb ------------------------------

int JZTrack::GetReverb()
{
  if (mpReverb)
  {
    return mpReverb->GetControlValue() + 1;
  }
  return 0;
}

void JZTrack::SetReverb(int Value)
{
  if (mpReverb)
  {
    Kill(mpReverb);
  }
  if (Value > 0)
  {
    JZEvent* pEvent = new JZControlEvent(0, Channel - 1, 0x5B, Value - 1);
    Put(pEvent);
    gpMidiPlayer->OutNow(this, pEvent);
  }
  Cleanup();
}

// ------------------------ Chorus ------------------------------

int JZTrack::GetChorus()
{
  if (mpChorus)
  {
    return mpChorus->GetControlValue() + 1;
  }
  return 0;
}

void JZTrack::SetChorus(int Value)
{
  if (mpChorus)
  {
    Kill(mpChorus);
  }
  if (Value > 0)
  {
    JZEvent* pEvent = new JZControlEvent(0, Channel - 1, 0x5D, Value - 1);
    Put(pEvent);
    gpMidiPlayer->OutNow(this, pEvent);
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
      return mpBank->GetControlValue();
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
        gpConfig->BankEntry(i).Command[0] == mpBank->GetControlValue() &&
        gpConfig->BankEntry(i).Command[1] == mpBank2->GetControlValue())
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
      mpBank = new JZControlEvent(
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
    mpBank  = new JZControlEvent(
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
    mpBank2 = new JZControlEvent(
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
    mChanged = true;
  }
}

// ------------------------  Patch ------------------------------

int JZTrack::GetPatch()
{
  if (mpPatch)
  {
    return mpPatch->GetProgram() + 1;
  }
  return 0;
}

void JZTrack::SetPatch(int PatchNr)
{
  if (mpPatch)
  {
    delete mpPatch;
    mpPatch = 0;
  }
  if (PatchNr > 0)
  {
    mpPatch = new JZProgramEvent(0, Channel - 1, PatchNr - 1);
    gpMidiPlayer->OutNow(this, mpPatch);
    mChanged = true;
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
    VibRate = new JZNrpn(0, Channel - 1, 0x01, 0x08, Value - 1);
    gpMidiPlayer->OutNow(this, VibRate);
    mChanged = true;
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
    VibDepth = new JZNrpn(0, Channel - 1, 0x01, 0x09, Value - 1);
    gpMidiPlayer->OutNow(this,  VibDepth);
    mChanged = true;
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
    VibDelay = new JZNrpn(0, Channel - 1, 0x01, 0x0a, Value - 1);
    gpMidiPlayer->OutNow(this,  VibDelay);
    mChanged = true;
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
    Cutoff = new JZNrpn(0, Channel - 1, 0x01, 0x20, Value - 1);
    gpMidiPlayer->OutNow(this,  Cutoff);
    mChanged = true;
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
    Resonance = new JZNrpn(0, Channel - 1, 0x01, 0x21, Value - 1);
    gpMidiPlayer->OutNow(this,  Resonance);
    mChanged = true;
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
    EnvAttack = new JZNrpn(0, Channel - 1, 0x01, 0x63, Value - 1);
    gpMidiPlayer->OutNow(this,  EnvAttack);
    mChanged = true;
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
    EnvDecay = new JZNrpn(0, Channel - 1, 0x01, 0x64, Value - 1);
    gpMidiPlayer->OutNow(this,  EnvDecay);
    mChanged = true;
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
    EnvRelease = new JZNrpn(0, Channel - 1, 0x01, 0x66, Value - 1);
    gpMidiPlayer->OutNow(this,  EnvRelease);
    mChanged = true;
  }
}

// ------------------------  DrumParam ------------------------------

int JZTrack::GetDrumParam(int pitch, int index)
{
  if (!DrumParams.IsEmpty())
  {
    JZNrpn *par = DrumParams.GetParam(pitch, index);
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
      new JZNrpn(0, Channel - 1, drumIndex2Param(index), pitch, Value - 1));
    gpMidiPlayer->OutNow(this, DrumParams.GetParam(pitch, index));
    mChanged = true;
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
    BendPitchSens = new JZRpn(0, Channel - 1, 0x00, 0x00, Value - 1);
    gpMidiPlayer->OutNow(this, BendPitchSens);
    mChanged = true;
  }
}

// ------------------------  Modulation Sysex ------------------------------

int JZTrack::GetModulationSysex(int msp)
{
  const unsigned char* pValue =
    gpSynth->GetSysexValPtr(ModulationSettings[msp]);

  if (pValue)
  {
    return *pValue + 1;
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
    JZEvent* pEvent = gpSynth->ModSX(msp, 0, Channel, Value - 1);
    if (pEvent)
    {
      Put(pEvent);
      gpMidiPlayer->OutNow(this, pEvent);
    }
  }
  Cleanup();
}

// ------------------------  Bender Sysex ------------------------------

int JZTrack::GetBenderSysex(int bsp)
{
  const unsigned char* pValue = gpSynth->GetSysexValPtr(BenderSettings[bsp]);

  if (pValue)
  {
    return *pValue + 1;
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
    JZEvent* pEvent = gpSynth->BendSX(bsp, 0, Channel, Value - 1);
    if (pEvent)
    {
      Put(pEvent);
      gpMidiPlayer->OutNow(this, pEvent);
    }
  }
  Cleanup();
}

// ------------------------  CAf Sysex ------------------------------

int JZTrack::GetCAfSysex(int csp)
{
  const unsigned char* pValue = gpSynth->GetSysexValPtr(CAfSettings[csp]);

  if (pValue)
  {
    return *pValue + 1;
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
    JZEvent* pEvent = gpSynth->CafSX(csp, 0, Channel, Value - 1);
    if (pEvent)
    {
      Put(pEvent);
      gpMidiPlayer->OutNow(this, pEvent);
    }
  }
  Cleanup();
}

// ------------------------  PAf Sysex ------------------------------

int JZTrack::GetPAfSysex(int psp)
{
  const unsigned char* pValue = gpSynth->GetSysexValPtr(PAfSettings[psp]);

  if (pValue)
  {
    return *pValue + 1;
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
    JZEvent* pEvent = gpSynth->PafSX(psp, 0, Channel, Value - 1);
    if (pEvent)
    {
      Put(pEvent);
      gpMidiPlayer->OutNow(this, pEvent);
    }
  }
  Cleanup();
}

// ------------------------  CC1 Sysex ------------------------------

int JZTrack::GetCC1Sysex(int csp)
{
  const unsigned char* pValue = gpSynth->GetSysexValPtr(CC1Settings[csp]);

  if (pValue)
  {
    return *pValue + 1;
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
    JZEvent* pEvent = gpSynth->CC1SX(csp, 0, Channel, Value - 1);
    if (pEvent)
    {
      Put(pEvent);
      gpMidiPlayer->OutNow(this, pEvent);
    }
  }
  Cleanup();
}

// ------------------------  CC2 Sysex ------------------------------

int JZTrack::GetCC2Sysex(int csp)
{
  const unsigned char* pValue = gpSynth->GetSysexValPtr(CC2Settings[csp]);

  if (pValue)
  {
    return *pValue + 1;
  }

  return 0;
}

void JZTrack::SetCC2Sysex(int csp, int Value)
{
  if (CC2Settings[csp])
    Kill(CC2Settings[csp]);
  if (Value > 0)
  {
    JZEvent* pEvent = gpSynth->CC2SX(csp, 0, Channel, Value - 1);
    if (pEvent)
    {
      Put(pEvent);
      gpMidiPlayer->OutNow(this, pEvent);
    }
  }
  Cleanup();
}

// ------------------------  CC1ControllerNr Sysex ------------------------------

int JZTrack::GetCC1ControllerNr()
{
  const unsigned char* pValue = gpSynth->GetSysexValPtr(CC1ControllerNr);

  if (pValue)
  {
    return *pValue + 1;
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
    JZEvent* pEvent = gpSynth->ControllerNumberSX(1, 0, Channel, Value - 1);
    if (pEvent)
    {
      Put(pEvent);
      gpMidiPlayer->OutNow(this, pEvent);
    }
  }
  Cleanup();
}

// ------------------------  CC2ControllerNr Sysex ------------------------------

int JZTrack::GetCC2ControllerNr()
{
  const unsigned char* pValue = gpSynth->GetSysexValPtr(CC2ControllerNr);

  if (pValue)
  {
    return *pValue + 1;
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
    JZEvent* pEvent = gpSynth->ControllerNumberSX(2, 0, Channel, Value - 1);
    if (pEvent)
    {
      Put(pEvent);
      gpMidiPlayer->OutNow(this, pEvent);
    }
  }
  Cleanup();
}

// ------------------------  Reverb Type ------------------------------

int JZTrack::GetReverbType(int lsb)
{
  const unsigned char* pValue = gpSynth->GetSysexValPtr(ReverbType);

  if (pValue)
  {
    if (lsb)
    {
      ++pValue;
    }
    return *pValue + 1;
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
    JZEvent* pEvent = gpSynth->ReverbMacroSX(0, Value - 1, lsb - 1);
    if (pEvent)
    {
      Put(pEvent);
      if (gpConfig->GetValue(C_UseReverbMacro))
      {
        gpMidiPlayer->OutNow(this, pEvent);
      }
    }
  }
  Cleanup();
}

// ------------------------  Chorus Type ------------------------------

int JZTrack::GetChorusType(int lsb)
{
  const unsigned char *pValue = gpSynth->GetSysexValPtr(ChorusType);

  if (pValue)
  {
    if (lsb)
    {
      ++pValue;
    }

    return *pValue + 1;
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
    JZEvent* pEvent = gpSynth->ChorusMacroSX(0, Value - 1, lsb - 1);
    if (pEvent)
    {
      Put(pEvent);
      if (gpConfig->GetValue(C_UseChorusMacro))
      {
        gpMidiPlayer->OutNow(this, pEvent);
      }
    }
  }
  Cleanup();
}

// -----------------------  Equalizer Type ------------------------------

int JZTrack::GetEqualizerType()
{
  const unsigned char* pValue = gpSynth->GetSysexValPtr(EqualizerType);

  if (pValue)
  {
    return *pValue + 1;
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
    JZEvent* pEvent = gpSynth->EqualizerMacroSX(0, Value - 1);
    if (pEvent)
    {
      Put(pEvent);
      gpMidiPlayer->OutNow(this, pEvent);
    }
  }
  Cleanup();
}

// ------------------------  Reverb Parameters Sysex ------------------------------

int JZTrack::GetRevSysex(int rsp)
{
  const unsigned char* pValue = gpSynth->GetSysexValPtr(ReverbSettings[rsp]);

  if (pValue)
  {
    return *pValue + 1;
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
    JZEvent* pEvent = gpSynth->ReverbParamSX(rsp, 0, Value - 1);
    if (pEvent)
    {
      Put(pEvent);
      if (!gpConfig->GetValue(C_UseReverbMacro))
      {
        gpMidiPlayer->OutNow(this, pEvent);
      }
    }
  }
  Cleanup();
}

// ------------------------  Chorus Parameters Sysex ------------------------------

int JZTrack::GetChoSysex(int csp)
{
  const unsigned char* pValue = gpSynth->GetSysexValPtr(ChorusSettings[csp]);

  if (pValue)
  {
    return *pValue + 1;
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
    JZEvent* pEvent = gpSynth->ChorusParamSX(csp, 0, Value - 1);
    if (pEvent)
    {
      Put(pEvent);
      if (!gpConfig->GetValue(C_UseChorusMacro))
      {
        gpMidiPlayer->OutNow(this, pEvent);
      }
    }
  }
  Cleanup();
}


// ------------------------  Partial Reserve ------------------------------

int JZTrack::GetPartRsrv(int chan)
{
  const unsigned char* pValue = gpSynth->GetSysexValPtr(PartialReserve);

  if (pValue)
  {
    return *(pValue + sysex_channel(chan)) + 1;
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
    JZEvent* pEvent = gpSynth->PartialReserveSX(0, Channel, rsrv);
    if (pEvent)
    {
      Put(pEvent);
      gpMidiPlayer->OutNow(this, pEvent);
    }
  }
  Cleanup();
}

// ------------------------  Master Volume ------------------------------

int JZTrack::GetMasterVol()
{
  const unsigned char* pValue = gpSynth->GetSysexValPtr(MasterVol);

  if (pValue)
  {
    if (gpSynth->GetSysexId(MasterVol) == SX_GM_MasterVol)
    {
      // first data byte is lsb; get msb instead!
      ++pValue;
    }

    return *pValue + 1;
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
    JZEvent* pEvent = gpSynth->MasterVolSX(0, Value - 1);
    if (pEvent)
    {
      Put(pEvent);
      gpMidiPlayer->OutNow(this, pEvent);
    }
  }
  Cleanup();
}


// ------------------------  Master Pan ------------------------------

int JZTrack::GetMasterPan()
{
  const unsigned char* pValue = gpSynth->GetSysexValPtr(MasterPan);

  if (pValue)
  {
    return *pValue + 1;
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
     JZEvent* pEvent = gpSynth->MasterPanSX(0, Value - 1);
    if (pEvent)
    {
       Put(pEvent);
       gpMidiPlayer->OutNow(this, pEvent);
    }
  }
  Cleanup();
}

// ------------------------  Mode Sysex ------------------------------

int JZTrack::GetModeSysex(int param)
{
   const unsigned char* pValue = 0;

   switch (param)
   {
     case mspRxChannel:
       pValue = gpSynth->GetSysexValPtr(RxChannel);
       break;

     case mspUseForRhythm:
       pValue = gpSynth->GetSysexValPtr(UseForRhythm);
       break;
   }

   if (pValue)
   {
     return *pValue + 1;
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

  JZEvent* pEvent = 0;

  if (Value > 0)
  {
    switch (param)
    {
      case mspRxChannel:
        pEvent = gpSynth->RxChannelSX(0, Channel, Value - 1);
        break;
      case mspUseForRhythm:
        pEvent = gpSynth->UseForRhythmSX(0, Channel, Value - 1);
        break;
    }

    if (pEvent)
    {
      Put(pEvent);
      gpMidiPlayer->OutNow(this, pEvent);
    }
  }
  Cleanup();
}

// ------------------------  Mtc offset (Time Code Offset) ------------------------------

JZMtcTime* JZTrack::GetMtcOffset()
{
  if (MtcOffset)
  {
    return new JZMtcTime(MtcOffset);
  }
  return(new JZMtcTime(0, Mtc30Ndf));
}

void JZTrack::SetMtcOffset(JZMtcTime* mtc)
{
  if (MtcOffset)
  {
    Kill(MtcOffset);
  }
  if (mtc)
  {
    JZEvent* pEvent = mtc->ToOffset();
    Put(pEvent);
  }
  Cleanup();
}

// ------------------------ Speed ------------------------------

int JZTrack::GetDefaultSpeed()
{
  if (mpSpeed)
  {
    return mpSpeed->GetBPM();
  }
  return 120;
}


void JZTrack::SetDefaultSpeed(int bpm)
{
  JZEvent* pEvent = new JZSetTempoEvent(0, bpm);
  if (mpSpeed)
  {
    Kill(mpSpeed);
  }
  Put(pEvent);
  gpMidiPlayer->OutNow(this, pEvent);
  Cleanup();
}

JZSetTempoEvent *JZTrack::GetCurrentTempo(int clk)
{
  JZEventIterator Iterator(this);
  Sort();
  JZEvent* pEvent = Iterator.Range(0, clk + 1);
  JZSetTempoEvent* t = mpSpeed;
  while (pEvent)
  {
    if (pEvent->IsSetTempo())
    {
      t = pEvent->IsSetTempo();
    }
    pEvent = Iterator.Next();
  } // while pEvent

  return t;
}

int JZTrack::GetCurrentSpeed(int clk)
{
  JZSetTempoEvent *t = GetCurrentTempo(clk);
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
