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

#ifndef JZ_TRACK_H
#define JZ_TRACK_H

#include "Events.h"
#include "DynamicArray.h"
#include "NamedValue.h"

class JZTrackWindow;

// Track-States

#define tsPlay 0
#define tsMute 1
#define tsSolo 2

// Mixer-defs
enum { MxVol = 0, MxPan, MxRev, MxCho, MxParams };


// Param (Nrpn / Rpn) things
enum { NrpnVibRate = 0, NrpnVibDepth, NrpnVibDelay, NrpnVibParams };
enum { NrpnCutoff = 0, NrpnResonance, NrpnSoundParams };
enum { NrpnEnvAttack = 0, NrpnEnvDecay, NrpnEnvRelease, NrpnEnvParams };

class tParam
{
  public:

    tParam(
      int clk,
      int cha,
      unsigned char id1,
      unsigned char msb,
      unsigned char id2,
      unsigned char lsb,
      unsigned char msbval)
      : Msb( clk, cha, id1, msb ),
        Lsb( clk, cha, id2, lsb ),
        DataMsb( clk, cha, 0x06, msbval ),
        ResetMsb( clk, cha, id1, 0x7f ),
        ResetLsb( clk, cha, id2, 0x7f )
    {
    }

    virtual ~tParam()
    {
    }

    virtual int Write(JZWriteBase& Io);
    virtual void SetCha( unsigned char cha );
    virtual int GetVal()
    {
      return( DataMsb.Value );
    }

    tControl Msb;
    tControl Lsb;
    tControl DataMsb;
    tControl ResetMsb;
    tControl ResetLsb;
};

class tNrpn : public tParam
{
  public:

    tNrpn(
      int clk,
      int cha,
      unsigned char msb,
      unsigned char lsb,
      unsigned char msbval)
      : tParam(clk, cha, 0x63, msb, 0x62, lsb, msbval)
    {
    }
};

class tRpn : public tParam
{
  public:

    tRpn(
      int clk,
      int cha,
      unsigned char msb,
      unsigned char lsb,
      unsigned char msbval)
      : tParam(clk, cha, 0x65, msb, 0x64, lsb, msbval)
    {
    }
};

unsigned char *SysExDT1(
  unsigned char aa,
  unsigned char bb,
  unsigned char cc,
  int length,
  unsigned char* dd);

int sysex_channel(int Channel);

enum ModulationSysexParameter
{
  mspModPitchControl = 0, mspModTvfCut, mspModAmpl, mspModLfo1Rate,
  mspModLfo1Pitch, mspModLfo1Tvf, mspModLfo1Tva, mspModLfo2Rate,
  mspModLfo2Pitch, mspModLfo2Tvf, mspModLfo2Tva, mspModulationSysexParameters
};

enum BenderSysexParameter {
  bspBendPitchControl = 0, bspBendTvfCut, bspBendAmpl, bspBendLfo1Rate,
  bspBendLfo1Pitch, bspBendLfo1Tvf, bspBendLfo1Tva, bspBendLfo2Rate,
  bspBendLfo2Pitch, bspBendLfo2Tvf, bspBendLfo2Tva, bspBenderSysexParameters
};

enum CAfSysexParameter {
  cspCAfPitchControl = 0, cspCAfTvfCut, cspCAfAmpl, cspCAfLfo1Rate,
  cspCAfLfo1Pitch, cspCAfLfo1Tvf, cspCAfLfo1Tva, cspCAfLfo2Rate,
  cspCAfLfo2Pitch, cspCAfLfo2Tvf, cspCAfLfo2Tva, cspCAfSysexParameters
};

enum PAfSysexParameter {
  pspPAfPitchControl = 0, pspPAfTvfCut, pspPAfAmpl, pspPAfLfo1Rate,
  pspPAfLfo1Pitch, pspPAfLfo1Tvf, pspPAfLfo1Tva, pspPAfLfo2Rate,
  pspPAfLfo2Pitch, pspPAfLfo2Tvf, pspPAfLfo2Tva, pspPAfSysexParameters
};

enum CC1SysexParameter {
  cspCC1PitchControl = 0, cspCC1TvfCut, cspCC1Ampl, cspCC1Lfo1Rate,
  cspCC1Lfo1Pitch, cspCC1Lfo1Tvf, cspCC1Lfo1Tva, cspCC1Lfo2Rate,
  cspCC1Lfo2Pitch, cspCC1Lfo2Tvf, cspCC1Lfo2Tva, cspCC1SysexParameters
};

enum CC2SysexParameter {
  cspCC2PitchControl = 0, cspCC2TvfCut, cspCC2Ampl, cspCC2Lfo1Rate,
  cspCC2Lfo1Pitch, cspCC2Lfo1Tvf, cspCC2Lfo1Tva, cspCC2Lfo2Rate,
  cspCC2Lfo2Pitch, cspCC2Lfo2Tvf, cspCC2Lfo2Tva, cspCC2SysexParameters
};

enum ReverbSysexParameter {
  rspRevCharacter = 0, rspRevPreLpf, rspRevLevel, rspRevTime,
  rspRevDelayFeedback, rspRevSendChorus, rspReverbSysexParameters
};

enum ChorusSysexParameter {
  cspChoPreLpf = 0, cspChoLevel, cspChoFeedback, cspChoDelay,
  cspChoRate, cspChoDepth, cspChoSendReverb, cspChorusSysexParameters
};

enum ModeSysexParameter {
  mspRxChannel = 0x02, mspRxCAf = 0x04, mspRxPAf = 0x07, mspUseForRhythm = 0x15
};

enum DrumInstrumentParameter {
  drumPitch = 0x18,
  drumTva = 0x1a,
  drumPan = 0x1c,
  drumReverb = 0x1d,
  drumChorus = 0x1e
};

enum DrumInstrumentParameterIndex {
  drumPitchIndex = 0,
  drumTvaIndex,
  drumPanIndex,
  drumReverbIndex,
  drumChorusIndex,
  numDrumParameters
};

int drumParam2Index( int par );
int drumIndex2Param( int index );

class tDrumInstrumentParameterList;

class tDrumInstrumentParameter
{

    friend class tDrumInstrumentParameterList;

  private:
    int pitch;
    tNrpn *param[numDrumParameters];
    tDrumInstrumentParameter *next;
  public:
    tDrumInstrumentParameter( tNrpn *par );
    tNrpn *Get( int index );
    void Put( tNrpn *par );
    tDrumInstrumentParameter *Next();
    int Pitch();
};

class tDrumInstrumentParameterList
{
  private:
    tDrumInstrumentParameter *list;
  public:
    tDrumInstrumentParameterList() : list(0) {}
    tDrumInstrumentParameter *GetElem( int pit );
    tNrpn *GetParam( int pit, int index );
    void PutParam( tNrpn *par );
    void DelParam( int pit, int index );
    tDrumInstrumentParameter *FirstElem();
    tDrumInstrumentParameter *NextElem( tDrumInstrumentParameter *cur );
    void DelElem( int pit );
    void Clear();
    int IsEmpty() { return( list == 0 ); }
};

enum tMtcType { Mtc24 = 0, Mtc25, Mtc30Df, Mtc30Ndf };

class tMtcTime
{
  public:
    tMtcType type;
    int hour;
    int min;
    int sec;
    int fm;

    tMtcTime( tMtcOffset *s ); // an mtc offset or mtc full message
    tMtcTime( int millisek, tMtcType t );
    tMtcTime( char *str, tMtcType t );
    tMtcTime( unsigned h, unsigned m, unsigned s, unsigned f, unsigned t );
    void ToString( char *str );
    tMtcOffset *ToOffset();
    int ToMillisec();
};


class tSimpleEventArray : public wxObject
{
  public:

    // Actual number of events in **Events.
    int nEvents;

    // Memory allocated in **Events
    int MaxEvents;

    JZEvent** Events;

    // Resize **Events
    void Resize();

    virtual void Clear();

    void Put(JZEvent* pEvent);

    void GrabData(tSimpleEventArray &src);

    void Copy(tSimpleEventArray& src, int frclk, int toclk);

    tSimpleEventArray();

    virtual ~tSimpleEventArray();

    void Sort();

    void RemoveEOT();
};


class tUndoBuffer : public tSimpleEventArray
{
  friend class JZTrack;

  public:

    virtual void Clear();
    void Put(JZEvent *e, int killed)
    {
      bits.set(nEvents, killed);
      tSimpleEventArray::Put(e);
    }

  private:

    JZBitset bits;  // set for killed events
};


class tEventArray : public tSimpleEventArray
{
    friend class tEventIterator;
    friend class tTrackDlg;
    friend class JZTrack;

  public:

    tTrackName* mName;
    tCopyright* Copyright;
    tProgram* mPatch;
    tSetTempo* Speed;
    tControl* Volume;
    tControl* Pan;
    tControl* Reverb;
    tControl* Chorus;
    tControl* mpBank;
    tControl* mpBank2;

  public:

    tSysEx* Reset;

    tSysEx* ModulationSettings[mspModulationSysexParameters];
    tSysEx* BenderSettings[bspBenderSysexParameters];
    tSysEx* CAfSettings[cspCAfSysexParameters];
    tSysEx* PAfSettings[pspPAfSysexParameters];
    tSysEx* CC1Settings[cspCC1SysexParameters];
    tSysEx* CC2Settings[cspCC2SysexParameters];

    tSysEx* CC1ControllerNr;
    tSysEx* CC2ControllerNr;

    tSysEx* ReverbType;
    tSysEx* ChorusType;
    tSysEx* EqualizerType;

    tSysEx* ReverbSettings[rspReverbSysexParameters];
    tSysEx* ChorusSettings[cspChorusSysexParameters];

    tSysEx* PartialReserve;
    tSysEx* MasterVol;
    tSysEx* MasterPan;

    tSysEx* RxChannel;
    tSysEx* UseForRhythm;

    tMtcOffset* MtcOffset;

    tNrpn* VibRate;
    tNrpn* VibDepth;
    tNrpn* VibDelay;
    tNrpn* Cutoff;
    tNrpn* Resonance;
    tNrpn* EnvAttack;
    tNrpn* EnvDecay;
    tNrpn* EnvRelease;
    tRpn* BendPitchSens;

    tDrumInstrumentParameterList DrumParams;

    int Channel;  // 1..16, set from first ChannelEvent, 0 = multichannel/nochannel
    int Device;   // 0 for tSeq2/Mpu401
    int ForceChannel;

    virtual void Clear();
    void Cleanup(bool dont_delete_killed_events = 0);

    void Keyoff2Length();
    void Length2Keyoff();

    tEventArray();
    virtual ~tEventArray();

    void Read(JZReadBase& Io);
    void Write(JZWriteBase& Io);

    int GetLastClock();
    int IsEmpty();
    int GetFirstClock();

    int State;    // tsXXX

  public:

    int GetAudioMode() const
    {
      return audio_mode;
    }

    void SetAudioMode(int x)
    {
      audio_mode = x;
    }

  protected:

    int audio_mode;
};



#define MaxUndo 20

class JZTrack : public tEventArray
{
  public:

    static bool changed;

    JZTrack();
    ~JZTrack() { Clear(); }

    bool IsDrumTrack();

    int iUndo;  // index to actual undo buffer
    int nRedo;  // current number of possible redo's
    int nUndo;  // current number of possible undo's
    tUndoBuffer UndoBuffers[MaxUndo];

    wxDialog *DialogBox;

    void Dialog(JZTrackWindow *parent);

    void Put(JZEvent *e)
    {
      changed = true;
      tEventArray::Put(e);
      UndoBuffers[iUndo].Put(e, 0);
    }

    void Kill(JZEvent *e)
    {
      changed = true;
      e->Kill();
      UndoBuffers[iUndo].Put(e, 1);
    }

    void Merge(tEventArray *other);
    void MergeRange(tEventArray *other, int FromClock, int ToClock, int Replace = 0);
    void Undo();
    void Redo();
    void NewUndoBuffer();
    void Clear();
    void Cleanup();

    char *GetName();
    void SetName(char *Name);

    char *GetCopyright();
    void SetCopyright(char *Copyright);

    char *GetStateChar();
    void SetState(int NewState);
    void ToggleState(int Direction);   // +1 = next, -1 = prev

    int  GetChannel() { return Channel; }
    void SetChannel(int NewChannel);

    int  GetDevice() const { return Device; }
    void SetDevice(int d) { Device = d; }

    int  GetPatch();
    void SetPatch(int PatchNr);

    int  GetVolume();
    void SetVolume(int Volume);

    int  GetPan();
    void SetPan(int Pan);

    int  GetReverb();
    void SetReverb(int Reverb);

    int  GetChorus();
    void SetChorus(int Chorus);

    int  GetVibRate();
    void SetVibRate(int VibRate);

    int  GetVibDepth();
    void SetVibDepth(int VibDepth);

    int  GetVibDelay();
    void SetVibDelay(int VibDelay);

    int  GetCutoff();
    void SetCutoff(int Cutoff);

    int  GetResonance();
    void SetResonance(int Resonance);

    int  GetEnvAttack();
    void SetEnvAttack(int EnvAttack);

    int  GetEnvDecay();
    void SetEnvDecay(int EnvDecay);

    int  GetEnvRelease();
    void SetEnvRelease(int EnvRelease);

    int GetDrumParam( int pitch, int index );
    void SetDrumParam(int pitch, int index, int Value);

    int  GetBendPitchSens();
    void SetBendPitchSens(int BendPitchSens);

    int  GetModulationSysex( int msp );
    void SetModulationSysex( int msp, int value);

    int  GetBenderSysex( int bsp );
    void SetBenderSysex( int bsp, int value);

    int  GetCAfSysex( int csp );
    void SetCAfSysex( int csp, int value);

    int  GetPAfSysex( int psp );
    void SetPAfSysex( int psp, int value);

    int  GetCC1Sysex( int csp );
    void SetCC1Sysex( int csp, int value);

    int  GetCC2Sysex( int csp );
    void SetCC2Sysex( int csp, int value);

    int  GetCC1ControllerNr();
    void SetCC1ControllerNr(int ctrlno);

    int  GetCC2ControllerNr();
    void SetCC2ControllerNr(int ctrlno);

    int  GetReverbType(int lsb = 0);
    void SetReverbType(int ReverbType, int lsb = 0);

    int  GetChorusType(int lsb = 0);
    void SetChorusType(int ChorusType, int lsb = 0);

    int  GetEqualizerType();
    void SetEqualizerType(int EqualizerType);

    int  GetRevSysex( int rsp );
    void SetRevSysex( int rsp, int value);

    int  GetChoSysex( int csp );
    void SetChoSysex( int csp, int value);

    int  GetBank();
    void SetBank(int Bank);

    int  GetDefaultSpeed();  // beats per minute
    void SetDefaultSpeed(int bpm);

    int  GetCurrentSpeed( int clk );  // beats per minute

    tSetTempo *GetCurrentTempo( int clk );

    int  GetMasterVol();
    void SetMasterVol(int MasterVol);

    int  GetMasterPan();
    void SetMasterPan(int MasterPan);

    int GetPartRsrv( int chan );
    void SetPartRsrv( unsigned char *rsrv );

    int  GetModeSysex( int param );
    void SetModeSysex( int param, int value);

    tMtcTime* GetMtcOffset();
    void SetMtcOffset( tMtcTime* mtc );


};


// ***********************************************************************
// tEventIterator
// *********************************************************************

class tEventIterator
{
    tSimpleEventArray *Track;
    int Start, Stop, Actual;

  public:

    tEventIterator(tSimpleEventArray *t)
    {
      Track  = t;
      Start  = 0;
      Stop   = Track->nEvents;
      Actual = Start;
    }


    JZEvent *GreaterEqual(int Clock)
    {
      int lo = Start;
      int hi = Stop;
      int clk = 0;
      while (lo < hi)
      {
        Actual  = (hi + lo) / 2;
        clk = Track->Events[Actual]->GetClock();
        if (clk < Clock)
        {
          lo = Actual + 1;
        }
        else
        {
          hi = Actual;
        }
      }
      if (Actual < Stop-1 && clk < Clock)
      {
        clk = Track->Events[++Actual]->GetClock();
      }
      if (Actual < Stop && clk >= Clock)
      {
        return Track->Events[Actual];
      }
      return 0;
    }


    JZEvent *First(int Clock = 0)
    {
      Actual = Start;
      return GreaterEqual(Clock);
    }


    JZEvent *Range(int frClock, unsigned toClock)
    {
      Start = Actual = 0;
      Stop  = Track->nEvents;

      if (!GreaterEqual(frClock))
        return 0;
      Start = Actual;
      if (GreaterEqual(toClock))
        Stop = Actual;
      Actual = Start;
      return (Actual < Stop ? Track->Events[Actual] : 0);
    }


    JZEvent *Next()
    {
      if (Actual < Stop)
      {
        ++Actual;
      }
      return (Actual < Stop ? Track->Events[Actual] : 0);
    }

    int EventsLeft()
    {
      return Stop - Actual;
    }

};

#endif // !defined(JZ_TRACK_H)
