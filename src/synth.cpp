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

#include "track.h"
#include "util.h"
#include "player.h"
#include "jazz.h"
#include "eventwin.h"
#include "trackwin.h"

#include "synth.h"

#include <stdlib.h>

#include <assert.h>

#define SXDECL(id,len,arr) do {\
  sxlen[id] = len; \
  sxdata[id] = new uchar[len]; \
  memcpy(sxdata[id], arr, len); \
} while (0)


#define GS_DT1 0x41,0x10,0x42,0x12
#define GS_DT1_LEN 10

#define XG_NAT 0x43,0x10,0x4c
#define XG_NAT_LEN 8 // no command ID or checksum for XG native!

tSynth* NewSynth( char *type )
{
   int i;
   for (i = 0; i < NumSynthTypes; i++)
   {
      if (!strcmp( SynthTypes[i].Name, type ))
	 break;
   }
   switch (i)
   {
    case SynthTypeGM:
       return new tGM();
    case SynthTypeGS:
       return new tGS();
    case SynthTypeXG:
       return new tXG();
    case SynthTypeOther:
    default:
       return new tGM();
   }
}


tNamedValue *SysexNames = 0;
tNamedValue *SysexGroupNames = 0;

tSynthSysex::tSynthSysex()
{
   int i;

   for (i = 0; i < NumSysexIds; i++) {
     sxlen[i] = 0;
     sxdata [i] = 0;
   }

   if (!SysexNames)
   {
      SysexNames = new tNamedValue[NumSysexIds + 1];

      for (i = 0; i < (NumSysexIds+1); i++)
      {
	 SysexNames[i].Value = i;
	 SysexNames[i].Name  = "";
      }

      SysexNames[SX_NONE].Name = "Unknown";
      SysexNames[NumSysexIds].Name = 0;

      // Misc
      SysexNames[SX_UNIV_NON_REALTIME].Name = "UNIV_NON_REALTIME";
      SysexNames[SX_UNIV_REALTIME].Name = "UNIV_REALTIME";
      SysexNames[SX_ROLAND_DT1].Name = "ROLAND_DT1";
      SysexNames[SX_ROLAND_RQ1].Name = "ROLAND_RQ1";
      SysexNames[SX_ROLAND_UNKNOWN].Name = "ROLAND_UNKNOWN";
      SysexNames[SX_XG_NATIVE].Name = "XG_NATIVE";
      SysexNames[SX_MU80_NATIVE].Name = "MU80_NATIVE";
      SysexNames[SX_YAMAHA_UNKNOWN].Name = "YAMAHA_UNKNOWN";


      // GM
      SysexNames[SX_GM_ON].Name = "GM_ON";
      SysexNames[SX_GM_MasterVol].Name = "GM_MasterVol";

      // GS DT1
      // 0x40 0x00 0x??:
      SysexNames[SX_GS_ON].Name = "GS_ON";
      SysexNames[SX_GS_MasterVol].Name = "GS_MasterVol";
      SysexNames[SX_GS_MasterPan].Name = "GS_MasterPan";

      // 0x40 0x2n 0x??:
      // Must be in sequence:
      SysexNames[SX_GS_ModPitch].Name = "GS_ModPitch";
      SysexNames[SX_GS_ModTvf].Name = "GS_ModTvf";
      SysexNames[SX_GS_ModAmpl].Name = "GS_ModAmpl";
      SysexNames[SX_GS_ModLfo1Rate].Name = "GS_ModLfo1Rate";
      SysexNames[SX_GS_ModLfo1Pitch].Name = "GS_ModLfo1Pitch";
      SysexNames[SX_GS_ModLfo1Tvf].Name = "GS_ModLfo1Tvf";
      SysexNames[SX_GS_ModLfo1Tva].Name = "GS_ModLfo1Tva";
      SysexNames[SX_GS_ModLfo2Rate].Name = "GS_ModLfo2Rate";
      SysexNames[SX_GS_ModLfo2Pitch].Name = "GS_ModLfo2Pitch";
      SysexNames[SX_GS_ModLfo2Tvf].Name = "GS_ModLfo2Tvf";
      SysexNames[SX_GS_ModLfo2Tva].Name = "GS_ModLfo2Tva";

      // Must be in sequence:
      SysexNames[SX_GS_BendPitch].Name = "GS_BendPitch";
      SysexNames[SX_GS_BendTvf].Name = "GS_BendTvf";
      SysexNames[SX_GS_BendAmpl].Name = "GS_BendAmpl";
      SysexNames[SX_GS_BendLfo1Rate].Name = "GS_BendLfo1Rate";
      SysexNames[SX_GS_BendLfo1Pitch].Name = "GS_BendLfo1Pitch";
      SysexNames[SX_GS_BendLfo1Tvf].Name = "GS_BendLfo1Tvf";
      SysexNames[SX_GS_BendLfo1Tva].Name = "GS_BendLfo1Tva";
      SysexNames[SX_GS_BendLfo2Rate].Name = "GS_BendLfo2Rate";
      SysexNames[SX_GS_BendLfo2Pitch].Name = "GS_BendLfo2Pitch";
      SysexNames[SX_GS_BendLfo2Tvf].Name = "GS_BendLfo2Tvf";
      SysexNames[SX_GS_BendLfo2Tva].Name = "GS_BendLfo2Tva";

      // Must be in sequence:
      SysexNames[SX_GS_CafPitch].Name = "GS_CafPitch";
      SysexNames[SX_GS_CafTvf].Name = "GS_CafTvf";
      SysexNames[SX_GS_CafAmpl].Name = "GS_CafAmpl";
      SysexNames[SX_GS_CafLfo1Rate].Name = "GS_CafLfo1Rate";
      SysexNames[SX_GS_CafLfo1Pitch].Name = "GS_CafLfo1Pitch";
      SysexNames[SX_GS_CafLfo1Tvf].Name = "GS_CafLfo1Tvf";
      SysexNames[SX_GS_CafLfo1Tva].Name = "GS_CafLfo1Tva";
      SysexNames[SX_GS_CafLfo2Rate].Name = "GS_CafLfo2Rate";
      SysexNames[SX_GS_CafLfo2Pitch].Name = "GS_CafLfo2Pitch";
      SysexNames[SX_GS_CafLfo2Tvf].Name = "GS_CafLfo2Tvf";
      SysexNames[SX_GS_CafLfo2Tva].Name = "GS_CafLfo2Tva";

      // Must be in sequence:
      SysexNames[SX_GS_PafPitch].Name = "GS_PafPitch";
      SysexNames[SX_GS_PafTvf].Name = "GS_PafTvf";
      SysexNames[SX_GS_PafAmpl].Name = "GS_PafAmpl";
      SysexNames[SX_GS_PafLfo1Rate].Name = "GS_PafLfo1Rate";
      SysexNames[SX_GS_PafLfo1Pitch].Name = "GS_PafLfo1Pitch";
      SysexNames[SX_GS_PafLfo1Tvf].Name = "GS_PafLfo1Tvf";
      SysexNames[SX_GS_PafLfo1Tva].Name = "GS_PafLfo1Tva";
      SysexNames[SX_GS_PafLfo2Rate].Name = "GS_PafLfo2Rate";
      SysexNames[SX_GS_PafLfo2Pitch].Name = "GS_PafLfo2Pitch";
      SysexNames[SX_GS_PafLfo2Tvf].Name = "GS_PafLfo2Tvf";
      SysexNames[SX_GS_PafLfo2Tva].Name = "GS_PafLfo2Tva";

      // Must be in sequence:
      SysexNames[SX_GS_CC1Pitch].Name = "GS_CC1Pitch";
      SysexNames[SX_GS_CC1Tvf].Name = "GS_CC1Tvf";
      SysexNames[SX_GS_CC1Ampl].Name = "GS_CC1Ampl";
      SysexNames[SX_GS_CC1Lfo1Rate].Name = "GS_CC1Lfo1Rate";
      SysexNames[SX_GS_CC1Lfo1Pitch].Name = "GS_CC1Lfo1Pitch";
      SysexNames[SX_GS_CC1Lfo1Tvf].Name = "GS_CC1Lfo1Tvf";
      SysexNames[SX_GS_CC1Lfo1Tva].Name = "GS_CC1Lfo1Tva";
      SysexNames[SX_GS_CC1Lfo2Rate].Name = "GS_CC1Lfo2Rate";
      SysexNames[SX_GS_CC1Lfo2Pitch].Name = "GS_CC1Lfo2Pitch";
      SysexNames[SX_GS_CC1Lfo2Tvf].Name = "GS_CC1Lfo2Tvf";
      SysexNames[SX_GS_CC1Lfo2Tva].Name = "GS_CC1Lfo2Tva";

      // Must be in sequence:
      SysexNames[SX_GS_CC2Pitch].Name = "GS_CC2Pitch";
      SysexNames[SX_GS_CC2Tvf].Name = "GS_CC2Tvf";
      SysexNames[SX_GS_CC2Ampl].Name = "GS_CC2Ampl]";
      SysexNames[SX_GS_CC2Lfo1Rate].Name = "GS_CC2Lfo1Rate";
      SysexNames[SX_GS_CC2Lfo1Pitch].Name = "GS_CC2Lfo1Pitch";
      SysexNames[SX_GS_CC2Lfo1Tvf].Name = "GS_CC2Lfo1Tvf";
      SysexNames[SX_GS_CC2Lfo1Tva].Name = "GS_CC2Lfo1Tva";
      SysexNames[SX_GS_CC2Lfo2Rate].Name = "GS_CC2Lfo2Rate";
      SysexNames[SX_GS_CC2Lfo2Pitch].Name = "GS_CC2Lfo2Pitch";
      SysexNames[SX_GS_CC2Lfo2Tvf].Name = "GS_CC2Lfo2Tvf";
      SysexNames[SX_GS_CC2Lfo2Tva].Name = "GS_CC2Lfo2Tva";

      // 0x40 0x01 0x??:
      // Must be in sequence:
      SysexNames[SX_GS_ReverbMacro].Name = "GS_ReverbMacro";
      SysexNames[SX_GS_RevCharacter].Name = "GS_RevCharacter";
      SysexNames[SX_GS_RevPreLpf].Name = "GS_RevPreLpf";
      SysexNames[SX_GS_RevLevel].Name = "GS_RevLevel";
      SysexNames[SX_GS_RevTime].Name = "GS_RevTime";
      SysexNames[SX_GS_RevDelayFeedback].Name = "GS_RevDelayFeedback";
      SysexNames[SX_GS_RevSendChorus].Name = "GS_RevSendChorus";

      // Must be in sequence:
      SysexNames[SX_GS_ChorusMacro].Name = "GS_ChorusMacro";
      SysexNames[SX_GS_ChoPreLpf].Name = "GS_ChoPreLpf";
      SysexNames[SX_GS_ChoLevel].Name = "GS_ChoLevel";
      SysexNames[SX_GS_ChoFeedback].Name = "GS_ChoFeedback";
      SysexNames[SX_GS_ChoDelay].Name = "GS_ChoDelay";
      SysexNames[SX_GS_ChoRate].Name = "GS_ChoRate";
      SysexNames[SX_GS_ChoDepth].Name = "GS_ChoDepth";
      SysexNames[SX_GS_ChoSendReverb].Name = "GS_ChoSendReverb]";

      SysexNames[SX_GS_PartialReserve].Name = "GS_PartialReserve";

      // 0x40 0x1n 0x??:
      SysexNames[SX_GS_RxChannel].Name = "GS_RxChannel";
      SysexNames[SX_GS_UseForRhythm].Name = "GS_UseForRhythm";
      SysexNames[SX_GS_CC1CtrlNo].Name = "GS_CC1CtrlNo";
      SysexNames[SX_GS_CC2CtrlNo].Name = "GS_CC2CtrlNo";


      // XG
      SysexNames[SX_XG_ON].Name = "XG_ON";

      // Native Multipart:
      // Must be in sequence
      SysexNames[SX_XG_ModPitch].Name = "XG_ModPitch";
      SysexNames[SX_XG_ModTvf].Name = "XG_ModTvf";
      SysexNames[SX_XG_ModAmpl].Name = "XG_ModAmpl";
      SysexNames[SX_XG_ModLfoPitch].Name = "XG_ModLfoPitch";
      SysexNames[SX_XG_ModLfoTvf].Name = "XG_ModLfoTvf";
      SysexNames[SX_XG_ModLfoTva].Name = "XG_ModLfoTva";
      SysexNames[SX_XG_BendPitch].Name = "XG_BendPitch";
      SysexNames[SX_XG_BendTvf].Name = "XG_BendTvf";
      SysexNames[SX_XG_BendAmpl].Name = "XG_BendAmpl";
      SysexNames[SX_XG_BendLfoPitch].Name = "XG_BendLfoPitch";
      SysexNames[SX_XG_BendLfoTvf].Name = "XG_BendLfoTvf";
      SysexNames[SX_XG_BendLfoTva].Name = "XG_BendLfoTva";

      // Must be in sequence:
      SysexNames[SX_XG_CafPitch].Name = "XG_CafPitch";
      SysexNames[SX_XG_CafTvf].Name = "XG_CafTvf";
      SysexNames[SX_XG_CafAmpl].Name = "XG_CafAmpl]";
      SysexNames[SX_XG_CafLfoPitch].Name = "XG_CafLfoPitch";
      SysexNames[SX_XG_CafLfoTvf].Name = "XG_CafLfoTvf";
      SysexNames[SX_XG_CafLfoTva].Name = "XG_CafLfoTva";
      SysexNames[SX_XG_PafPitch].Name = "XG_PafPitch";
      SysexNames[SX_XG_PafTvf].Name = "XG_PafTvf";
      SysexNames[SX_XG_PafAmpl].Name = "XG_PafAmpl";
      SysexNames[SX_XG_PafLfoPitch].Name = "XG_PafLfoPitch";
      SysexNames[SX_XG_PafLfoTvf].Name = "XG_PafLfoTvf";
      SysexNames[SX_XG_PafLfoTva].Name = "XG_PafLfoTva";
      SysexNames[SX_XG_CC1CtrlNo].Name = "XG_CC1CtrlNo";
      SysexNames[SX_XG_CC1Pitch].Name = "XG_CC1Pitch";
      SysexNames[SX_XG_CC1Tvf].Name = "XG_CC1Tvf";
      SysexNames[SX_XG_CC1Ampl].Name = "XG_CC1Ampl";
      SysexNames[SX_XG_CC1LfoPitch].Name = "XG_CC1LfoPitch";
      SysexNames[SX_XG_CC1LfoTvf].Name = "XG_CC1LfoTvf";
      SysexNames[SX_XG_CC1LfoTva].Name = "XG_CC1LfoTva";
      SysexNames[SX_XG_CC2CtrlNo].Name = "XG_CC2CtrlNo";
      SysexNames[SX_XG_CC2Pitch].Name = "XG_CC2Pitch";
      SysexNames[SX_XG_CC2Tvf].Name = "XG_CC2Tvf";
      SysexNames[SX_XG_CC2Ampl].Name = "XG_CC2Ampl";
      SysexNames[SX_XG_CC2LfoPitch].Name = "XG_CC2LfoPitch";
      SysexNames[SX_XG_CC2LfoTvf].Name = "XG_CC2LfoTvf";
      SysexNames[SX_XG_CC2LfoTva].Name = "XG_CC2LfoTva";



      SysexNames[SX_XG_ReverbMacro].Name = "XG_ReverbMacro";
      SysexNames[SX_XG_ChorusMacro].Name = "XG_ChorusMacro";
      SysexNames[SX_XG_EqualizerMacro].Name = "XG_EqualizerMacro";

      SysexNames[SX_XG_RxChannel].Name = "XG_RxChannel";
      SysexNames[SX_XG_UseForRhythm].Name = "XG_UseForRhythm";
   }

   if (!SysexGroupNames)
   {
      SysexGroupNames = new tNamedValue [130];

      for (i = 0; i < 130; i++)
      {
	 SysexGroupNames[i].Value = i;
	 SysexGroupNames[i].Name = "";
      }

      SysexGroupNames[129].Name = 0;

      SysexGroupNames[SX_GROUP_UNKNOWN + 1].Name = "Other Sysex";
      SysexGroupNames[SX_GROUP_GM + 1].Name = "GM Sysex";
      SysexGroupNames[SX_GROUP_GS + 1].Name = "GS Sysex";
      SysexGroupNames[SX_GROUP_XG + 1].Name = "XG Sysex";

   }

   // GM
   uchar gm_on[] = {0x7e,0x7f,0x09,0x01,0xf7};
   SXDECL(SX_GM_ON, 5, gm_on);

   uchar gm_master_vol[] = {0x7f,0x7f,0x04,0x01,0x00,0x00,0xf7};
   SXDECL(SX_GM_MasterVol, 7, gm_master_vol);


   // GS DT1
   uchar gs_on[] = {GS_DT1,0x40,0x00,0x7f,0x00,0x41,0xf7};
   SXDECL(SX_GS_ON,GS_DT1_LEN, gs_on);

   uchar gs_dt1[] = {GS_DT1,0x40,0x00,0x04,0x00,0x00,0xf7};

   gs_dt1[6] = 0x04;
   SXDECL(SX_GS_MasterVol, GS_DT1_LEN, gs_dt1);

   gs_dt1[6] = 0x06;
   SXDECL(SX_GS_MasterPan, GS_DT1_LEN, gs_dt1);

   gs_dt1[4] = 0x40;
   gs_dt1[5] = 0x20;
   for ( i = 0; i < 11; i++ )
   {
      gs_dt1[6] = 0x00 + i;
      SXDECL(SX_GS_ModPitch + i, GS_DT1_LEN, gs_dt1);
   }

   for ( i = 0; i < 11; i++ )
   {
      gs_dt1[6] = 0x10 + i;
      SXDECL(SX_GS_BendPitch + i, GS_DT1_LEN, gs_dt1);
   }

   for ( i = 0; i < 11; i++ )
   {
      gs_dt1[6] = 0x20 + i;
      SXDECL(SX_GS_CafPitch + i, GS_DT1_LEN, gs_dt1);
   }

   for ( i = 0; i < 11; i++ )
   {
      gs_dt1[6] = 0x30 + i;
      SXDECL(SX_GS_PafPitch + i, GS_DT1_LEN, gs_dt1);
   }

   for ( i = 0; i < 11; i++ )
   {
      gs_dt1[6] = 0x40 + i;
      SXDECL(SX_GS_CC1Pitch + i, GS_DT1_LEN, gs_dt1);
   }

   for ( i = 0; i < 11; i++ )
   {
      gs_dt1[6] = 0x50 + i;
      SXDECL(SX_GS_CC2Pitch + i, GS_DT1_LEN, gs_dt1);
   }

   gs_dt1[4] = 0x40;
   gs_dt1[5] = 0x01;
   for ( i = 0; i < 7; i++ )
   {
      gs_dt1[6] = 0x30 + i;
      SXDECL(SX_GS_ReverbMacro + i, GS_DT1_LEN, gs_dt1);
   }

   for ( i = 0; i < 8; i++ )
   {
      gs_dt1[6] = 0x38 + i;
      SXDECL(SX_GS_ChorusMacro + i, GS_DT1_LEN, gs_dt1);
   }

   const uchar gs_partial_reserve[] = {GS_DT1,0x40,0x01,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xf7};
   SXDECL(SX_GS_PartialReserve, GS_DT1_LEN, gs_partial_reserve);

   gs_dt1[4] = 0x40;
   gs_dt1[5] = 0x10;
   gs_dt1[6] = 0x02;
   SXDECL(SX_GS_RxChannel, GS_DT1_LEN, gs_dt1);

   gs_dt1[4] = 0x40;
   gs_dt1[5] = 0x10;
   gs_dt1[6] = 0x15;
   SXDECL(SX_GS_UseForRhythm, GS_DT1_LEN, gs_dt1);

   gs_dt1[4] = 0x40;
   gs_dt1[5] = 0x10;
   gs_dt1[6] = 0x1f;
   SXDECL(SX_GS_CC1CtrlNo, GS_DT1_LEN, gs_dt1);

   gs_dt1[4] = 0x40;
   gs_dt1[5] = 0x10;
   gs_dt1[6] = 0x20;
   SXDECL(SX_GS_CC2CtrlNo, GS_DT1_LEN, gs_dt1);



   // XG native
   const uchar xg_on[] = {XG_NAT,0x00,0x00,0x7e,0x00,0xf7};
   SXDECL(SX_XG_ON, XG_NAT_LEN, xg_on);

   // XG native multipart
   uchar xg_multi[] = {XG_NAT,0x08,0x00,0x00,0x00,0x7f};

   for ( i = 0; i < 12; i++ )
   {
      xg_multi[5] = 0x1d + i;
      SXDECL(SX_XG_ModPitch + i, XG_NAT_LEN, xg_multi);
   }

   for ( i = 0; i < 26; i++ )
   {
      xg_multi[5] = 0x4d + i;
      SXDECL(SX_XG_CafPitch + i, XG_NAT_LEN, xg_multi);
   }

   xg_multi[5] = 0x04;
   SXDECL(SX_XG_RxChannel, XG_NAT_LEN, xg_multi);

   xg_multi[5] = 0x07;
   SXDECL(SX_XG_UseForRhythm, XG_NAT_LEN, xg_multi);


   // XG native effect 1
   uchar xg_effect1_2[] = {XG_NAT,0x02,0x01,0x00,0x00,0x00,0x7f};

   xg_effect1_2[5] = 0x00;
   SXDECL(SX_XG_ReverbMacro, XG_NAT_LEN, xg_effect1_2);

   xg_effect1_2[5] = 0x20;
   SXDECL(SX_XG_ChorusMacro, XG_NAT_LEN, xg_effect1_2);

   // XG multi EQ
   uchar xg_multiEQ[] = {XG_NAT,0x02,0x40,0x00,0x00,0x00,0x7f};

   xg_multiEQ[5] = 0x00;
   SXDECL(SX_XG_EqualizerMacro, XG_NAT_LEN, xg_multiEQ);
}

tSynthSysex::~tSynthSysex()
{
   for (int i = 0; i < NumSysexIds; i++)
   {
      delete sxdata[i];
   }
}

int tSynthSysex::GetId( tSysEx *s )
{
   if (!s)
      return SX_NONE;

   switch (s->Data[0])
   {
    case 0x7e:
       // GM ON ?
       if (!memcmp(sxdata[SX_GM_ON],s->Data,s->Length))
       {
	  return SX_GM_ON;
       }
       else
       {
	  return SX_UNIV_NON_REALTIME;
       }
       break;

    case 0x7f:
       // GM MasterVol ?
       if (!memcmp(sxdata[SX_GM_MasterVol],s->Data,4))
       {
	  return SX_GM_MasterVol;
       }
       else
       {
	  return SX_UNIV_REALTIME;
       }
       break;

    case 0x41:
       // Roland!

       // GS DT1?
       if ((s->Data[2] == 0x42) && (s->Data[3] == 0x12))
       {
	  register uchar a1 = s->Data[4];
	  register uchar a2 = s->Data[5];
	  register uchar a3 = s->Data[6];

	  if (a1 == 0x40)
	  {
	     // MSB address 0x40:
	     if (a2 == 0x00)
	     {
		// 0x40 0x00 0x??
		switch (a3)
		{
		 case 0x7f:
		    return SX_GS_ON;
		 case 0x04:
		    return SX_GS_MasterVol;
		 case 0x06:
		    return SX_GS_MasterPan;
		 default:
		    break;
		}
	     }
	     else if (a2 == 0x01)
	     {
		// 0x40 0x01 0x??
		if ((a3 >= 0x30) && (a3 <= 0x36))
		   // reverb settings
		   return SX_GS_ReverbMacro + (a3 - 0x30);
		else if ((a3 >= 0x38) && (a3 <= 0x3f))
		   // chorus settings
		   return SX_GS_ChorusMacro + (a3 - 0x38);
		else if (a3 == 0x10)
		   return SX_GS_PartialReserve;
	     }
	     else if ( (a2 & 0xf0) == 0x10 )
	     {
		// 0x40 0x1n 0x??
		switch (a3)
		{
		 case 0x02:
		    return SX_GS_RxChannel;

		 case 0x15:
		    return SX_GS_UseForRhythm;

		 case 0x1f:
		    return SX_GS_CC1CtrlNo;

		 case 0x20:
		    return SX_GS_CC2CtrlNo;
		 default:
		    break;
		}
	     }
	     else if ( (a2 & 0xf0) == 0x20 )
	     {
		// 0x40 0x2n 0x??
		if (a3 <= 0x0a)
		   return SX_GS_ModPitch + (a3 - 0x00);
		else if ((a3 >= 0x10) && (a3 <= 0x1a))
		   return SX_GS_BendPitch + (a3 - 0x10);
		else if ((a3 >= 0x20) && (a3 <= 0x2a))
		   return SX_GS_CafPitch + (a3 - 0x20);
		else if ((a3 >= 0x30) && (a3 <= 0x3a))
		   return SX_GS_PafPitch + (a3 - 0x30);
		else if ((a3 >= 0x40) && (a3 <= 0x4a))
		   return SX_GS_CC1Pitch + (a3 - 0x40);
		else if ((a3 >= 0x50) && (a3 <= 0x5a))
		   return SX_GS_CC2Pitch + (a3 - 0x50);
	     }
	  } // end a1 == 0x40
       } // end GS DT1

       if ((s->Data[3] == 0x12) && (s->Length >= 10))
       {
	  return SX_ROLAND_DT1;
       }
       else if ((s->Data[3] == 0x11) && (s->Length >= 12))
       {
	  return SX_ROLAND_RQ1;
       }
       else
       {
	  return SX_ROLAND_UNKNOWN;
       }

       break; // end Roland

    case 0x43:
       // Yamaha!
       // XG Native?
       if (((s->Data[1] & 0xf0) == 0x10) && (s->Data[2] == 0x4c))
       {
	  register uchar a1 = s->Data[3];
	  register uchar a2 = s->Data[4];
	  register uchar a3 = s->Data[5];

	  // Multipart?
	  if (a1 == 0x08)
	  {
	     if ((a3 >= 0x1d) && (a3 <= 0x28))
	     {
		return SX_XG_ModPitch + (a3 - 0x1d);
	     }
	     else if ((a3 >= 0x4d) && (a3 <= 0x66))
	     {
		return SX_XG_CafPitch + (a3 - 0x4d);
	     }
	     else if (a3 == 0x04)
	     {
		return SX_XG_RxChannel;
	     }
	     else if (a3 == 0x07)
	     {
		return SX_XG_UseForRhythm;
	     }
	  }

	  // Effect 1?
	  else if ((a1 == 0x02) && (a2 == 0x01))
	  {
	     if (a3 == 0x00)
	     {
		return SX_XG_ReverbMacro;
	     }
	     else if (a3 == 0x20)
	     {
		return SX_XG_ChorusMacro;
	     }
	  }
	  // Multi EQ?
	  else if ((a1 == 0x02) && (a2 == 0x40))
	  {
	     if (a3 == 0x00)
	     {
		return SX_XG_EqualizerMacro;
	     }
	  }

	  // XG system on?
	  else if ((a1 == 0x00) && (a2 == 0x00) && (a3 == 0x7e))
	  {
	     return SX_XG_ON;
	  }
       }

       if (s->Data[2] == 0x4c)
       {
	  return SX_XG_NATIVE;
       }
       else if (s->Data[2] == 0x49)
       {
	  return SX_MU80_NATIVE;
       }
       else
       {
	  return SX_YAMAHA_UNKNOWN;
       }

       break; // end Yamaha

    default:
       break;
   }

   // Not recognized
   return SX_NONE;
}


uchar * tSynthSysex::GetValPtr( tSysEx *s )
{
   if (!s)
      return 0;

   switch (s->Data[0])
   {
    case 0x7f:
       // GM MasterVol?
       if (!memcmp(sxdata[SX_GM_MasterVol],s->Data,4))
       {
	  return &s->Data[4];
       }
       break;

    case 0x41:
       // Roland!
       // GS DT1?
       if ((s->Data[2] == 0x42) && (s->Data[3] == 0x12) && (s->Length >= 10))
       {
	  return &s->Data[7];
       }
       // other DT1 or RQ1 ?
       else if (((s->Data[3] == 0x12) && (s->Length >= 10)) ||
		((s->Data[3] == 0x11) && (s->Length >= 12)))
       {
	  return &s->Data[7];
       }
       break;

    case 0x43:
       // Yamaha!
       // XG Native?
       if (((s->Data[1] & 0xf0) == 0x10) && (s->Data[2] == 0x4c))
       {
	  return &s->Data[6];
       }
       break;
    default:
       break;
   }

   // Not recognized
   return 0;
}

uchar * tSynthSysex::GetChaPtr( tSysEx *s )
{
   // Get the byte where the channel number is (if any)

   if (!s)
      return 0;

   switch (s->Data[0])
   {
    case 0x41:
       // Roland!
       // GS DT1 + address 0x40?
       if ((s->Data[2] == 0x42) && (s->Data[3] == 0x12) &&
	   (s->Data[4] == 0x40))
       {
	  if ( ((s->Data[5] & 0xf0) == 0x10 ) || ((s->Data[5] & 0xf0) == 0x20 ) )
	  {
	     return &s->Data[5];
	  }
       }
       break;

    case 0x43:
       // Yamaha!
       // XG Native multipart?
       if (((s->Data[1] & 0xf0) == 0x10) && (s->Data[2] == 0x4c) && (s->Data[3] == 0x08))
       {
	  return &s->Data[4];
       }
       break;
    default:
       break;
   }

   // Not recognized
   return 0;
}


void tSynthSysex::FixCheckSum( tSysEx *s )
{
   if ((s->Data[0] == 0x41) &&
       (((s->Data[3] == 0x12) && (s->Length >= 10)) ||
	((s->Data[3] == 0x11) && (s->Length >= 12))))
   {
      // Roland RQ1 or DT1
      int len = s->Length;
      uchar *sx = s->Data;
      uchar sum = 0x00;

      for (int i = 4; i < (len-2); i++)
	 sum += sx[i];
      sx[len - 2] = (0x80 - (sum & 0x7f)) & 0x7f;
      sx[len-1] = 0xf7;
   }

   return;
}

tSysEx* tSynthSysex::operator()(long clk, int id, uchar val)
{
   return (*this)(clk, id, -1, 1, &val);
}

tSysEx* tSynthSysex::operator()(long clk, int id, int datalen, uchar val[])
{
   return (*this)(clk, id, -1, datalen, val);
}

tSysEx* tSynthSysex::operator()(long clk, int id, int channel, int datalen, uchar val[])
{
   assert( (id > SX_NONE) && (id < NumSysexIds) );
   assert( datalen > 0 );

   int i;
   for (i = 0; i < datalen; i++)
      assert( val[i] < 128 );

   int len = sxlen[id] + datalen - 1;
   uchar* sx = new uchar[len];
   memcpy( sx, sxdata[id], sxlen[id] );
   tSysEx* s;

   if (id == SX_GM_MasterVol)
   {
      // GM
      if (datalen == 2)
	 sx[4] = val[1]; // LSB is second byte!
      else
	 sx[4] = 0;
      sx[5] = val[0]; // MSB
      s = new tSysEx( clk, sx, len );
   }
   else if ((id > SX_GS_ON) && (id < SX_XG_ON))
   {
      // GS DT1
      for (i = 0; i < datalen; i++)
	 sx[i+7] = val[i];

      if (channel >= 0)
      {
	 sx[5] = sx[5] | sysex_channel( channel );
      }

      uchar sum = 0x00;
      for (i = 4; i < (len-2); i++)
	 sum += sx[i];
      sx[len - 2] = (0x80 - (sum & 0x7f)) & 0x7f;
      sx[len-1] = 0xf7;
      s = new tSysEx( clk, sx, len );
   }
   else if (id > SX_XG_ON)
   {
      // XG Native
      for (i = 0; i < datalen; i++)
	 sx[i+6] = val[i];

      if (channel >= 0)
      {
	 sx[4] = channel - 1;
      }
      sx[len-1] = 0xf7;
      s = new tSysEx( clk, sx, len );
   }

   delete sx;
   return s;
}


tEvent* tGS::MasterVolSX( long clk, uchar vol )
{
   return Sysex( clk, SX_GS_MasterVol, vol );
}

tEvent* tGS::MasterPanSX( long clk, uchar pan )
{
   return Sysex( clk, SX_GS_MasterPan, pan );
}

tEvent* tGS::ModSX( int index, long clk, int cha, uchar val )
{
   return Sysex( clk, SX_GS_ModPitch + index, cha, 1, &val );
}

tEvent* tGS::BendSX( int index, long clk, int cha, uchar val )
{
   return Sysex( clk, SX_GS_BendPitch + index, cha, 1, &val );
}

tEvent* tGS::CafSX( int index, long clk, int cha, uchar val )
{
   return Sysex( clk, SX_GS_CafPitch + index, cha, 1, &val );
}

tEvent* tGS::PafSX( int index, long clk, int cha, uchar val )
{
   return Sysex( clk, SX_GS_PafPitch + index, cha, 1, &val );
}

tEvent* tGS::CC1SX( int index, long clk, int cha, uchar val )
{
   return Sysex( clk, SX_GS_CC1Pitch + index, cha, 1, &val );
}

tEvent* tGS::CC2SX( int index, long clk, int cha, uchar val )
{
   return Sysex( clk, SX_GS_CC2Pitch + index, cha, 1, &val );
}

tEvent* tGS::PartialReserveSX( long clk, int cha, uchar *valptr )
{
   return Sysex( clk, SX_GS_PartialReserve, 16, valptr );
}

tEvent* tGS::RxChannelSX( long clk, int cha, uchar val )
{
   return Sysex( clk, SX_GS_RxChannel, cha, 1, &val );
}

tEvent* tGS::UseForRhythmSX( long clk, int cha, uchar val )
{
   return Sysex( clk, SX_GS_UseForRhythm, cha, 1, &val );
}

tEvent* tGS::ControllerNumberSX( int ctrlno, long clk, int cha, uchar val )
{
   if (ctrlno == 1)
      return Sysex( clk, SX_GS_CC1CtrlNo, cha, 1, &val );
   else if (ctrlno == 2)
      return Sysex( clk, SX_GS_CC2CtrlNo, cha, 1, &val );
   else
      return 0;
}

tEvent* tGS::ReverbMacroSX( long clk, uchar val, uchar lsb )
{
   return Sysex( clk, SX_GS_ReverbMacro, val );
}

tEvent* tGS::ReverbParamSX( int index, long clk, uchar val )
{
   return Sysex( clk, SX_GS_RevCharacter + index, val );
}

tEvent* tGS::ChorusMacroSX( long clk, uchar val, uchar lsb )
{
   return Sysex( clk, SX_GS_ChorusMacro, val );
}

tEvent* tGS::ChorusParamSX( int index, long clk, uchar val )
{
   return Sysex( clk, SX_GS_ChoPreLpf + index, val );
}



// XG:

tEvent* tXG::ModSX( int index, long clk, int cha, uchar val )
{
   if ((index >= 0) && (index <= 2))
      return Sysex( clk, SX_XG_ModPitch + index, cha, 1, &val );
   else if ((index >= 4) && (index <=6))
      return Sysex( clk, SX_XG_ModPitch + index - 1, cha, 1, &val );
   else
      return 0;
}

tEvent* tXG::BendSX( int index, long clk, int cha, uchar val )
{
   if ((index >= 0) && (index <= 2))
      return Sysex( clk, SX_XG_BendPitch + index, cha, 1, &val );
   else if ((index >= 4) && (index <=6))
       return Sysex( clk, SX_XG_BendPitch + index - 1, cha, 1, &val );
   else
      return 0;
}

tEvent* tXG::CafSX( int index, long clk, int cha, uchar val )
{
   if ((index >= 0) && (index <= 2))
      return Sysex( clk, SX_XG_CafPitch + index, cha, 1, &val );
   else if ((index >= 4) && (index <= 6))
       return Sysex( clk, SX_XG_CafPitch + index - 1, cha, 1, &val );
   else
      return 0;
}

tEvent* tXG::PafSX( int index, long clk, int cha, uchar val )
{
   if ((index >= 0) && (index <= 2))
      return Sysex( clk, SX_XG_PafPitch + index, cha, 1, &val );
   else if ((index >= 4) && (index <= 6))
       return Sysex( clk, SX_XG_PafPitch + index - 1, cha, 1, &val );
   else
      return 0;
}

tEvent* tXG::CC1SX( int index, long clk, int cha, uchar val )
{
   if ((index >= 0) && (index <= 2))
      return Sysex( clk, SX_XG_CC1Pitch + index, cha, 1, &val );
   else if ((index >= 4) && (index <= 6))
       return Sysex( clk, SX_XG_CC1Pitch + index - 1, cha, 1, &val );
   else
      return 0;
}

tEvent* tXG::CC2SX( int index, long clk, int cha, uchar val )
{
   if ((index >= 0) && (index <= 2))
      return Sysex( clk, SX_XG_CC2Pitch + index, cha, 1, &val );
   else if ((index >= 4) && (index <= 6))
       return Sysex( clk, SX_XG_CC2Pitch + index - 1, cha, 1, &val );
   else
      return 0;
}

tEvent* tXG::RxChannelSX( long clk, int cha, uchar val )
{
   return Sysex( clk, SX_XG_RxChannel, cha, 1, &val );
}

tEvent* tXG::UseForRhythmSX( long clk, int cha, uchar val )
{
   return Sysex( clk, SX_XG_UseForRhythm, cha, 1, &val );
}

tEvent* tXG::ControllerNumberSX( int ctrlno, long clk, int cha, uchar val )
{
   if (ctrlno == 1)
      return Sysex( clk, SX_XG_CC1CtrlNo, cha, 1, &val );
   else if (ctrlno == 2)
      return Sysex( clk, SX_XG_CC2CtrlNo, cha, 1, &val );
   else
      return 0;
}

tEvent* tXG::ReverbMacroSX( long clk, uchar val, uchar lsb )
{
   uchar valp[2];
   valp[0] = val;
   valp[1] = lsb;

   return Sysex( clk, SX_XG_ReverbMacro, 2, valp );
}

tEvent* tXG::ChorusMacroSX( long clk, uchar val, uchar lsb )
{
   uchar valp[2];
   valp[0] = val;
   valp[1] = lsb;

   return Sysex( clk, SX_XG_ChorusMacro, 2, valp );
}

tEvent* tXG::EqualizerMacroSX( long clk, uchar val )
{
   return Sysex( clk, SX_XG_EqualizerMacro, val );
}
