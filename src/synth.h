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


#ifndef synth_h
#define synth_h

#ifndef events_h
#include "events.h"
#endif

#include "dynarray.h"

#ifndef wx_wxh
#include "wx/wx.h"
#endif

#ifndef util_h
#include "util.h"
#endif

#ifndef track_h
#include "track.h"
#endif

#ifndef config_h
#include "config.h"
#endif

enum SysexId {
   SX_NONE = 0,

   // Misc:
   SX_UNIV_NON_REALTIME,
   SX_UNIV_REALTIME,
   SX_ROLAND_DT1,
   SX_ROLAND_RQ1,
   SX_ROLAND_UNKNOWN,
   SX_XG_NATIVE,
   SX_MU80_NATIVE,
   SX_YAMAHA_UNKNOWN,

   // GM
   SX_GM_ON,
   SX_GM_MasterVol,

   // GS DT1
   // 0x40 0x00 0x??:
   SX_GS_ON,
   SX_GS_MasterVol,
   SX_GS_MasterPan,

   // 0x40 0x2n 0x??:
   // Must be in sequence:
   SX_GS_ModPitch,
   SX_GS_ModTvf,
   SX_GS_ModAmpl,
   SX_GS_ModLfo1Rate,
   SX_GS_ModLfo1Pitch,
   SX_GS_ModLfo1Tvf,
   SX_GS_ModLfo1Tva,
   SX_GS_ModLfo2Rate,
   SX_GS_ModLfo2Pitch,
   SX_GS_ModLfo2Tvf,
   SX_GS_ModLfo2Tva,

   // Must be in sequence:
   SX_GS_BendPitch,
   SX_GS_BendTvf,
   SX_GS_BendAmpl,
   SX_GS_BendLfo1Rate,
   SX_GS_BendLfo1Pitch,
   SX_GS_BendLfo1Tvf,
   SX_GS_BendLfo1Tva,
   SX_GS_BendLfo2Rate,
   SX_GS_BendLfo2Pitch,
   SX_GS_BendLfo2Tvf,
   SX_GS_BendLfo2Tva,

   // Must be in sequence:
   SX_GS_CafPitch,
   SX_GS_CafTvf,
   SX_GS_CafAmpl,
   SX_GS_CafLfo1Rate,
   SX_GS_CafLfo1Pitch,
   SX_GS_CafLfo1Tvf,
   SX_GS_CafLfo1Tva,
   SX_GS_CafLfo2Rate,
   SX_GS_CafLfo2Pitch,
   SX_GS_CafLfo2Tvf,
   SX_GS_CafLfo2Tva,

   // Must be in sequence:
   SX_GS_PafPitch,
   SX_GS_PafTvf,
   SX_GS_PafAmpl,
   SX_GS_PafLfo1Rate,
   SX_GS_PafLfo1Pitch,
   SX_GS_PafLfo1Tvf,
   SX_GS_PafLfo1Tva,
   SX_GS_PafLfo2Rate,
   SX_GS_PafLfo2Pitch,
   SX_GS_PafLfo2Tvf,
   SX_GS_PafLfo2Tva,

   // Must be in sequence:
   SX_GS_CC1Pitch,
   SX_GS_CC1Tvf,
   SX_GS_CC1Ampl,
   SX_GS_CC1Lfo1Rate,
   SX_GS_CC1Lfo1Pitch,
   SX_GS_CC1Lfo1Tvf,
   SX_GS_CC1Lfo1Tva,
   SX_GS_CC1Lfo2Rate,
   SX_GS_CC1Lfo2Pitch,
   SX_GS_CC1Lfo2Tvf,
   SX_GS_CC1Lfo2Tva,

   // Must be in sequence:
   SX_GS_CC2Pitch,
   SX_GS_CC2Tvf,
   SX_GS_CC2Ampl,
   SX_GS_CC2Lfo1Rate,
   SX_GS_CC2Lfo1Pitch,
   SX_GS_CC2Lfo1Tvf,
   SX_GS_CC2Lfo1Tva,
   SX_GS_CC2Lfo2Rate,
   SX_GS_CC2Lfo2Pitch,
   SX_GS_CC2Lfo2Tvf,
   SX_GS_CC2Lfo2Tva,

   // 0x40 0x01 0x??:
   // Must be in sequence:
   SX_GS_ReverbMacro,
   SX_GS_RevCharacter,
   SX_GS_RevPreLpf,
   SX_GS_RevLevel,
   SX_GS_RevTime,
   SX_GS_RevDelayFeedback,
   SX_GS_RevSendChorus,

   // Must be in sequence:
   SX_GS_ChorusMacro,
   SX_GS_ChoPreLpf,
   SX_GS_ChoLevel,
   SX_GS_ChoFeedback,
   SX_GS_ChoDelay,
   SX_GS_ChoRate,
   SX_GS_ChoDepth,
   SX_GS_ChoSendReverb,

   SX_GS_PartialReserve,

   // 0x40 0x1n 0x??:
   SX_GS_RxChannel,
   SX_GS_UseForRhythm,
   SX_GS_CC1CtrlNo,
   SX_GS_CC2CtrlNo,


   // XG
   SX_XG_ON,

   // Native Multipart:
   // Must be in sequence
   SX_XG_ModPitch,
   SX_XG_ModTvf,
   SX_XG_ModAmpl,
   SX_XG_ModLfoPitch,
   SX_XG_ModLfoTvf,
   SX_XG_ModLfoTva,
   SX_XG_BendPitch,
   SX_XG_BendTvf,
   SX_XG_BendAmpl,
   SX_XG_BendLfoPitch,
   SX_XG_BendLfoTvf,
   SX_XG_BendLfoTva,

   // Must be in sequence:
   SX_XG_CafPitch,
   SX_XG_CafTvf,
   SX_XG_CafAmpl,
   SX_XG_CafLfoPitch,
   SX_XG_CafLfoTvf,
   SX_XG_CafLfoTva,
   SX_XG_PafPitch,
   SX_XG_PafTvf,
   SX_XG_PafAmpl,
   SX_XG_PafLfoPitch,
   SX_XG_PafLfoTvf,
   SX_XG_PafLfoTva,
   SX_XG_CC1CtrlNo,
   SX_XG_CC1Pitch,
   SX_XG_CC1Tvf,
   SX_XG_CC1Ampl,
   SX_XG_CC1LfoPitch,
   SX_XG_CC1LfoTvf,
   SX_XG_CC1LfoTva,
   SX_XG_CC2CtrlNo,
   SX_XG_CC2Pitch,
   SX_XG_CC2Tvf,
   SX_XG_CC2Ampl,
   SX_XG_CC2LfoPitch,
   SX_XG_CC2LfoTvf,
   SX_XG_CC2LfoTva,



   SX_XG_ReverbMacro,
   SX_XG_ChorusMacro,
   SX_XG_EqualizerMacro,

   SX_XG_RxChannel,
   SX_XG_UseForRhythm,

   NumSysexIds
};

#define SX_GROUP_UNKNOWN 64
#define SX_GROUP_GM 65
#define SX_GROUP_GS 66
#define SX_GROUP_XG 67

extern tNamedValue *SysexNames;
extern tNamedValue *SysexGroupNames;

class tSynthSysex
{
      uchar sxlen[NumSysexIds];
      uchar * sxdata[NumSysexIds];

   public:
      tSynthSysex();
      ~tSynthSysex();

      // Find out what kind of sysex this is
      int GetId( tSysEx *s );

      // Get pointer to the data value (if any)
      uchar * GetValPtr( tSysEx *s );

      // Get pointer to the byte with the channel (if any)
      uchar * GetChaPtr( tSysEx *s );

      // Fix checksum byte (if any)
      void FixCheckSum( tSysEx *s );

      // Constant sysexes like e.g. GM Midi On
      tSysEx* operator()(long clk, int id)
      {
	 assert( (id >= 0) && (id < NumSysexIds) );
	 return( new tSysEx( clk, sxdata[id], sxlen[id] ) );
      }

      // Variable sysexes like e.g. MasterVol or DT1
      tSysEx* operator()(long clk, int id, uchar val);
      tSysEx* operator()(long clk, int id, int datalen, uchar val[]);
      tSysEx* operator()(long clk, int id, int channel, int datalen, uchar val[]);
};

class tGM;
class tGS;
class tXG;

class tSynth
{
   protected:
      tSynthSysex Sysex;

   public:

      virtual tGM* IsGM() { return 0; }
      virtual tGS* IsGS() { return 0; }
      virtual tXG* IsXG() { return 0; }

      virtual int GetSysexId( tSysEx *s )
      {
	 return Sysex.GetId( s );
      }

      virtual uchar * GetSysexValPtr( tSysEx *s )
      {
	 return Sysex.GetValPtr( s );
      }

      virtual uchar * GetSysexChaPtr( tSysEx *s )
      {
	 return Sysex.GetChaPtr( s );
      }

      virtual void FixSysexCheckSum( tSysEx *s )
      {
	 Sysex.FixCheckSum( s );
      }

      virtual tEvent* Reset() = 0;

      virtual tEvent* MasterVolSX( long clk, uchar vol )
      {
	 return Sysex(clk, SX_GM_MasterVol, vol );
      }

      virtual tEvent* MasterPanSX( long clk, uchar pan ) { return 0; }

      virtual tEvent* ModSX( int index, long clk, int cha, uchar val ) { return 0; }

      virtual tEvent* BendSX( int index, long clk, int cha, uchar val) { return 0; }

      virtual tEvent* CafSX( int index, long clk, int cha, uchar val ) { return 0; }

      virtual tEvent* PafSX( int index, long clk, int cha, uchar val ) { return 0; }

      virtual tEvent* CC1SX( int index, long clk, int cha, uchar val ) { return 0; }

      virtual tEvent* CC2SX( int index, long clk, int cha, uchar val ) { return 0; }

      virtual tEvent* PartialReserveSX( long clk, int cha, uchar *valptr ) { return 0; }
      virtual tEvent* RxChannelSX( long clk, int cha, uchar val ) { return 0; }
      virtual tEvent* UseForRhythmSX( long clk, int cha, uchar val ) { return 0; }

      virtual tEvent* ControllerNumberSX( int ctrlno, long clk, int cha, uchar val ) { return 0; }

      virtual tEvent* ReverbMacroSX( long clk, uchar val, uchar lsb = 0 ) { return 0; }
      virtual tEvent* ReverbParamSX( int index, long clk, uchar val ) { return 0; }
      virtual tEvent* ChorusMacroSX( long clk, uchar val, uchar lsb = 0 ) { return 0; }
      virtual tEvent* ChorusParamSX( int index, long clk, uchar val ) { return 0; }

      virtual tEvent* EqualizerMacroSX( long clk, uchar val ) { return 0; }
};

tSynth* NewSynth( char* type );

class tGM : public tSynth
{
   public:
      virtual tGM* IsGM()
      {
	 return this;
      }

      virtual tEvent* Reset()
      {
	 return Sysex(0, SX_GM_ON);
      }
};

class tGS : public tSynth
{
   public:
      virtual tGS* IsGS()
      {
	 return this;
      }

      tEvent* Reset()
      {
	 return Sysex(0, SX_GS_ON);
      }

      virtual tEvent* MasterVolSX( long clk, uchar vol );
      virtual tEvent* MasterPanSX( long clk, uchar pan );
      virtual tEvent* ModSX( int index, long clk, int cha, uchar val );
      virtual tEvent* BendSX( int index, long clk, int cha, uchar val );
      virtual tEvent* CafSX( int index, long clk, int cha, uchar val );
      virtual tEvent* PafSX( int index, long clk, int cha, uchar val );
      virtual tEvent* CC1SX( int index, long clk, int cha, uchar val );
      virtual tEvent* CC2SX( int index, long clk, int cha, uchar val );
      virtual tEvent* PartialReserveSX( long clk, int cha, uchar *valptr );
      virtual tEvent* RxChannelSX( long clk, int cha, uchar val );
      virtual tEvent* UseForRhythmSX( long clk, int cha, uchar val );
      virtual tEvent* ControllerNumberSX( int ctrlno, long clk, int cha, uchar val );
      virtual tEvent* ReverbMacroSX( long clk, uchar val, uchar lsb = 0 );
      virtual tEvent* ReverbParamSX( int index, long clk, uchar val );
      virtual tEvent* ChorusMacroSX( long clk, uchar val, uchar lsb = 0 );
      virtual tEvent* ChorusParamSX( int index, long clk, uchar val );

};

class tXG : public tSynth
{
   public:
      virtual tXG* IsXG()
      {
	 return this;
      }

      tEvent* Reset()
      {
	 return Sysex(0, SX_XG_ON);
      }

      virtual tEvent* ModSX( int index, long clk, int cha, uchar val );
      virtual tEvent* BendSX( int index, long clk, int cha, uchar val );
      virtual tEvent* CafSX( int index, long clk, int cha, uchar val );
      virtual tEvent* PafSX( int index, long clk, int cha, uchar val );
      virtual tEvent* CC1SX( int index, long clk, int cha, uchar val );
      virtual tEvent* CC2SX( int index, long clk, int cha, uchar val );
      virtual tEvent* RxChannelSX( long clk, int cha, uchar val );
      virtual tEvent* UseForRhythmSX( long clk, int cha, uchar val );
      virtual tEvent* ControllerNumberSX( int ctrlno, long clk, int cha, uchar val );
      virtual tEvent* ReverbMacroSX( long clk, uchar val, uchar lsb = 0 );
      virtual tEvent* ChorusMacroSX( long clk, uchar val, uchar lsb = 0 );
      virtual tEvent* EqualizerMacroSX( long clk, uchar val );

};

#endif
