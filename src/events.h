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


#ifndef events_h
#define events_h


typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;


#ifndef stdio_h
#include <stdio.h>
#define stdio_h
#endif

#ifndef assert_h
#include <assert.h>
#define assert_h
#endif

#ifndef wx_msw
#ifndef values_h
//#include <values.h>
#define values_h
#endif
#endif

#ifndef wx_wxh
#ifdef wx_xview
#undef BITSPERBYTE
#endif
#include "wx/wx.h"
#endif


#ifndef config_h
#include "config.h"
#endif

void Error(char *fmt, ...);

#define noE_DBUG

#ifdef E_DBUG
#define MAGIC 621476895
#else
#define edb()
#endif


class tEvent;

// **********************************************************************************************
// Ausgabe-Device, kann sein
//   - Midi-Standard-File
//   - Ascii-File
//   - Midi-Port

class tReadBase
{
  protected:
    FILE *fd;
  public:

    int TicksPerQuarter;	// nach Open bekannt
    int nTracks;

    virtual int  Open(const char *fname);
    virtual void Close();

    virtual tEvent *Read() = 0;
    virtual int NextTrack() = 0;
};


class tWriteBase
{
  protected:
    FILE *fd;
  public:
    virtual int  Open(const char *fname, int nTracks, int TicksPerQuarter);
    virtual void Close();

    virtual int Write(tEvent *e)
    {
      return Write(e, (uchar *)0, 0);
    }

    virtual int Write(tEvent *e, uchar c)
    {
      return Write(e, &c, 1);
    }

    virtual int Write(tEvent *e, uchar c1, uchar c2)
    {
      uchar arr[2];
      arr[0] = c1;
      arr[1] = c2;
      return Write(e, arr, 2);
    }

    virtual int Write(tEvent *e, uchar c1, uchar c2, uchar c3)
    {
      uchar arr[3];
      arr[0] = c1;
      arr[1] = c2;
      arr[2] = c3;
      return Write(e, arr, 3);
    }

    virtual int Write(tEvent *e, uchar c1, uchar c2, uchar c3, uchar c4)
    {
      uchar arr[4];
      arr[0] = c1;
      arr[1] = c2;
      arr[2] = c3;
      arr[3] = c4;
      return Write(e, arr, 4);
    }

    virtual int Write(tEvent *e, uchar *s, int len) = 0;
    virtual void NextTrack() {}
};


// --------------------------------------------------------------------------
// tGetMidiBytes
// --------------------------------------------------------------------------

class tGetMidiBytes : public tWriteBase
{
  public:
    int  Open(char *fname, int nTracks, int TicksPerQuarter) { return 1; }
    void Close() {}
    void NextTrack() {}

    // Get tEvent's bytes
    int Write(tEvent *e, uchar *s, int len);

    unsigned char Buffer[10];
    int  nBytes;
};

// ********************************************************************
// Midi-Events
// ********************************************************************

/*
 * normale Events (mit Kanal)
 */

#define StatKeyOff	0x80
#define StatKeyOn	0x90
#define StatKeyPressure	0xA0		// Key pressure
#define StatControl	0xB0
#define StatProgram	0xC0
#define StatChnPressure	0xD0            // SN++ Channel pressure
#define StatPitch	0xE0

/*
 * Meta-Events (no channel)
 */

#define StatSysEx	0xF0
#define StatSongPtr	0xF2
#define StatSongSelect	0xF3
#define StatTuneRequest	0xF6
#define StatMidiClock   0xf8
#define StatStartPlay   0xfa
#define StatContPlay    0xfb
#define StatStopPlay    0xfc
#define StatText	0x01
#define StatCopyright	0x02
#define StatTrackName	0x03
#define StatMarker	0x06
#define StatEndOfTrack	0x2F
#define StatSetTempo	0x51
#define StatMtcOffset	0x54
#define StatTimeSignat	0x58
#define StatKeySignat	0x59

#define StatUnknown	0x00

// proprietary event status
#define StatJazzMeta    0x7F
#define StatPlayTrack    0x7E


#define LastClock 	(0x7fffffffL)
#define KilledClock 	(0x80000000L)




// Event-Classes

class tEvent;
class tChannelEvent;
class tMetaEvent;

class tKeyOn;
class tKeyOff;
class tPitch;
class tControl;
class tProgram;
class tSysEx;
class tSongPtr;
class tMidiClock;
class tStartPlay;
class tContPlay;
class tStopPlay;
class tText;
class tCopyright;
class tTrackName;
class tMarker;
class tSetTempo;
class tMtcOffset;
class tTimeSignat;
class tKeySignat;
class tKeyPressure;
class tJazzMeta;
class tChnPressure;
class tPlayTrack;
class tEndOfTrack;

class tEvent
{
  public:


#ifdef E_DBUG
  long Magic;
  void edb() {
    if (Magic != MAGIC) {
      fprintf(stderr, "Magic failed\n");
      fflush(stderr);
      *(char *)0 = 0;
    }
  }
#endif

    unsigned char Stat;
    long  Clock;  // should be protected ...

    long GetClock() const {
      return Clock & ~KilledClock;
    }
    void SetClock(long c) {
      Clock = c;
    }

    // the device is dynamically set when events are copied to
    // the playback queue (from the track device)
    enum { BROADCAST_DEVICE = 0 };
    tEvent(long clk, unsigned char sta)
    {
      Clock = clk;
      Stat  = sta;
      Device = BROADCAST_DEVICE;
#ifdef E_DBUG
      Magic = MAGIC;
#endif
    }
    virtual ~tEvent()
    {
      edb();
#ifdef E_DBUG
      Magic = 0L;
#endif
    }

    void Kill()    { edb(); Clock |= KilledClock; }
    void UnKill()  { edb(); Clock &= ~KilledClock; }
    int IsKilled() { edb(); return (Clock & KilledClock) != 0; }

    virtual tMetaEvent 	*IsMetaEvent()	{ edb(); return 0; }
    virtual tChannelEvent *IsChannelEvent() { edb(); return 0; }

    virtual tKeyOn 	*IsKeyOn()	{ edb(); return 0; }
    virtual tKeyOff 	*IsKeyOff()	{ edb(); return 0; }
    virtual tPitch 	*IsPitch()	{ edb(); return 0; }
    virtual tControl 	*IsControl()	{ edb(); return 0; }
    virtual tProgram 	*IsProgram()	{ edb(); return 0; }
    virtual tSysEx 	*IsSysEx()	{ edb(); return 0; }
    virtual tSongPtr 	*IsSongPtr()	{ edb(); return 0; }
    virtual tMidiClock 	*IsMidiClock()	{ edb(); return 0; }
    virtual tStartPlay 	*IsStartPlay()	{ edb(); return 0; }
    virtual tContPlay 	*IsContPlay()	{ edb(); return 0; }
    virtual tStopPlay 	*IsStopPlay()	{ edb(); return 0; }
    virtual tText 	*IsText()	{ edb(); return 0; }
    virtual tCopyright 	*IsCopyright()	{ edb(); return 0; }
    virtual tTrackName 	*IsTrackName()	{ edb(); return 0; }
    virtual tMarker 	*IsMarker()	{ edb(); return 0; }
    virtual tSetTempo 	*IsSetTempo()	{ edb(); return 0; }
    virtual tMtcOffset 	*IsMtcOffset()	{ edb(); return 0; }
    virtual tTimeSignat	*IsTimeSignat()	{ edb(); return 0; }
    virtual tKeySignat	*IsKeySignat()	{ edb(); return 0; }
    virtual tKeyPressure *IsKeyPressure() { edb(); return 0; }
    virtual tJazzMeta	*IsJazzMeta()	{ edb(); return 0; }
    virtual tPlayTrack	*IsPlayTrack()	{ edb(); return 0; }
    virtual tEndOfTrack	*IsEndOfTrack()	{ edb(); return 0; }
    virtual tChnPressure       *IsChnPressure()       { edb(); return 0; }

    virtual int Write(tWriteBase &io) { edb(); return io.Write(this); }

    int Compare(tEvent &e)
    {
      edb();
      if ((unsigned long)e.Clock > (unsigned long)Clock)
	return -1;
      if ((unsigned long)e.Clock < (unsigned long)Clock)
	return 1;
      return 0;
    }

    virtual void BarInfo(int &TicksPerBar, int &CountsPerBar, int TicksPerQuarter) {}

    virtual tEvent *Copy() { edb(); return new tEvent(*this); }

    // Filter
    virtual int   GetValue()    	{ edb(); return 0; }

    // Painting
    virtual int   GetLength()		{ edb(); return 16; }
    virtual int   GetPitch()    	{ edb(); return 64; }  // Value normiert auf 0..127
    virtual void  SetPitch(int p) 	{ edb(); }
    virtual wxPen *GetPen()     	{ return wxBLACK_PEN; }
    virtual wxBrush *GetBrush()     	{ return wxBLACK_BRUSH; }

    int GetDevice() const {
      return Device;
    }
    void SetDevice(int d) {
      Device = d;
    }
 private:
    int Device;
};


class tChannelEvent : public tEvent
{
  public:
    uchar  Channel;
    tChannelEvent(long clk, uchar sta, int cha)
      : tEvent(clk, sta)
    {
      Channel = cha;
    }

    virtual tChannelEvent *IsChannelEvent() { edb(); return this; }
    virtual tEvent *Copy() { edb(); return new tChannelEvent(*this); }
};




class tKeyOn : public tChannelEvent
{
  public:
    uchar  Key;
    uchar  Veloc;
    ushort Length;	// 0 if corresponding tKeyOff exists
    // SN++
    ushort OffVeloc;

    tKeyOn(long clk, int cha, uchar key, uchar vel, ushort len = 0)
      : tChannelEvent(clk, StatKeyOn, cha)
    {
      Key 	= key;
      Veloc 	= vel;
      Length 	= len;
      OffVeloc  = 0;
    }

    virtual int Write(tWriteBase &io)
    {
      edb(); return io.Write(this, Key, Veloc);
    }

    virtual tKeyOn *IsKeyOn() 		{ edb(); return this; }
    virtual tEvent *Copy() 		{ edb(); return new tKeyOn(*this); }
    virtual int   GetLength()		{ edb(); return Length; }
    virtual int   GetValue()     	{ edb(); return Key; }
    virtual int   GetPitch()     	{ edb(); return Key; }
    virtual void  SetPitch(int p) 	{ edb(); Key = p; }
    virtual wxPen *GetPen()     	{ return wxBLACK_PEN; }
    virtual wxBrush *GetBrush()     	{ return wxBLACK_BRUSH; }
};


class tKeyOff : public tChannelEvent
{
  public:
    uchar Key;
    // SN++
    uchar OffVeloc;

    tKeyOff(long clk, int cha, uchar key, uchar veloc = 0)
      : tChannelEvent(clk, StatKeyOff, cha)
    {
      Key = key;
      OffVeloc = veloc;
    }

    virtual int Write(tWriteBase &io)
    {
      edb(); return io.Write(this, Key, OffVeloc);
    }

    virtual tKeyOff *IsKeyOff() { edb(); return this; }
    virtual tEvent *Copy() { edb(); return new tKeyOff(*this); }

};


class tPitch : public tChannelEvent
{
  public:
    short Value;

    tPitch(long clk, ushort cha, uchar lo, uchar hi)
      : tChannelEvent(clk, StatPitch, cha)
    {
      Value  = ((hi << 7) | lo) - 8192;
    }

    tPitch(long clk, ushort cha, short val)
      : tChannelEvent(clk, StatPitch, cha)
    {
      Value  = val;
    }

    virtual int Write(tWriteBase &io)
    {
      int v = Value + 8192;
      edb(); return io.Write(this, (uchar)(v & 0x7F), (uchar)(v >> 7));
    }

    virtual tPitch *IsPitch()		{ edb(); return this; }
    virtual tEvent *Copy()      	{ edb(); return new tPitch(*this); }
    virtual int   GetValue()    	{ edb(); return Value; }
    virtual int   GetPitch()    	{ edb(); return (Value + 8192) >> 7; }
    virtual void  SetPitch(int p)    	{ edb(); Value = (p << 7) - 8192; }
    virtual wxPen *GetPen()     	{ return wxRED_PEN; }
    virtual wxBrush *GetBrush()     	{ return wxRED_BRUSH; }
};



class tControl : public tChannelEvent
{
  public:

    uchar Control;
    uchar Value;

    tControl(long clk, int cha, uchar ctl, uchar val)
      : tChannelEvent(clk, StatControl, cha)
    {
      Control = ctl;
      Value   = val;
    }

    virtual int Write(tWriteBase &io)
    {
      edb(); return io.Write(this, Control, Value);
    }
    virtual tControl *IsControl()	{ edb(); return this; }
    virtual tEvent *  Copy() 		{ edb(); return new tControl(*this); }
    virtual int   GetValue()     	{ edb(); return Control; }
    virtual int   GetPitch()     	{ edb(); return Control; }
    virtual void  SetPitch(int p)     	{ edb(); Control = p; }
    virtual wxPen *GetPen()     	{ return wxCYAN_PEN; }
    virtual wxBrush *GetBrush()     	{ return wxCYAN_BRUSH; }
};


class tProgram : public tChannelEvent
{
  public:
    uchar Program;

    tProgram(long clk, int cha, uchar prg)
      : tChannelEvent(clk, StatProgram, cha)
    {
      Program = prg;
    }

    virtual int Write(tWriteBase &io)
    {
      edb(); return io.Write(this, Program);
    }
    virtual tProgram *IsProgram() 	{ edb(); return this; }
    virtual tEvent *Copy() 		{ edb(); return new tProgram(*this); }
    virtual int   GetValue()     	{ edb(); return Program; }
    virtual int   GetPitch()     	{ edb(); return Program; }
    virtual void  SetPitch(int p)	{ edb(); Program = p; }
    virtual wxPen *GetPen()     	{ return wxGREEN_PEN; }
    virtual wxBrush *GetBrush()     	{ return wxGREEN_BRUSH; }
};



class tMetaEvent : public tEvent
{
  public:

    uchar *Data;
    ushort Length;

    tMetaEvent(long clk, uchar sta, uchar *dat, ushort len)
      : tEvent(clk, sta)
    {
      Length = len;
      Data = new uchar [len + 1];
      if (dat)
	memcpy(Data, dat, len);
      Data[len] = 0;
    }

    virtual ~tMetaEvent()
    {
      delete Data;
    }

    virtual int Write(tWriteBase &io)
    {
      edb(); return io.Write(this, Data, Length);
    }
    virtual tMetaEvent *IsMetaEvent() { edb(); return this; }
    virtual tEvent *Copy() { edb(); return new tMetaEvent(Clock, Stat, Data, Length); }
};



class tJazzMeta : public tMetaEvent
{
  // proprietary information of jazz stored to a track. This event
  // should not go into a track itself but is read/written from/to
  // the file.
  public:
    enum { DATALEN = 20 };
    tJazzMeta(long clk, uchar *dat, ushort len)
      : tMetaEvent(clk, StatJazzMeta, dat, len)
    {
    }
    tJazzMeta() : tMetaEvent(0, StatJazzMeta, 0, DATALEN) {
      memset(Data, 0, DATALEN);
      memcpy(Data, "JAZ2", 4);
      Data[4] = 1; // version or so
    }
#ifdef AUDIO
    char GetAudioMode() const {
      return Data[5];
    }
    void SetAudioMode(char c) {
      Data[5] = c;
    }
#endif
    char GetTrackState() const {
      return Data[6];
    }
    void SetTrackState(char c) {
      Data[6] = c;
    }
    
    // Data[7] is unused

    uchar GetTrackDevice() const {
      return Data[8];
    }
    void SetTrackDevice(uchar x) {
      Data[8] = x;
    }
    uchar GetIntroLength() const {
      return Data[9];
    }
    void SetIntroLength(uchar x) {
      Data[9] = x;
    }
    virtual tJazzMeta *IsJazzMeta() { edb(); return this; }
    virtual tEvent *Copy() { edb(); return new tJazzMeta(Clock, Data, Length); }
};


class tSysEx : public tMetaEvent
{
  public:
    tSysEx(long clk, uchar *dat, ushort len)
      : tMetaEvent(clk, StatSysEx, dat, len)
    {
    }
    virtual tSysEx *IsSysEx() { edb(); return this; }
    virtual tEvent *Copy() { edb(); return new tSysEx(Clock, Data, Length); }

    // todo
    virtual int   GetPitch();
};

class tSongPtr : public tMetaEvent
{
  public:
    tSongPtr(long clk, uchar *dat, ushort len)
      : tMetaEvent(clk, StatSongPtr, dat, len)
    {
    }
    virtual tSongPtr *IsSongPtr() { edb(); return this; }
    virtual tEvent *Copy() { edb(); return new tSongPtr(Clock, Data, Length); }
};

class tMidiClock : public tMetaEvent
{
  public:
    tMidiClock(long clk, uchar *dat, ushort len)
      : tMetaEvent(clk, StatMidiClock, dat, len)
    {
    }
    tMidiClock(long clk)
      : tMetaEvent(clk, StatMidiClock, 0, 0)
    {
    }
    virtual tMidiClock *IsMidiClock() { edb(); return this; }
    virtual tEvent *Copy() { edb(); return new tMidiClock(Clock, Data, Length); }
};

class tStartPlay : public tMetaEvent
{
  public:
    tStartPlay(long clk, uchar *dat, ushort len)
      : tMetaEvent(clk, StatStartPlay, dat, len)
    {
    }
    tStartPlay(long clk)
      : tMetaEvent(clk, StatStartPlay, 0, 0)
    {
    }
    virtual tStartPlay *IsStartPlay() { edb(); return this; }
    virtual tEvent *Copy() { edb(); return new tStartPlay(Clock, Data, Length); }
};

class tContPlay : public tMetaEvent
{
  public:
    tContPlay(long clk, uchar *dat, ushort len)
      : tMetaEvent(clk, StatContPlay, dat, len)
    {
    }
    tContPlay(long clk)
      : tMetaEvent(clk, StatContPlay, 0, 0)
    {
    }
    virtual tContPlay *IsContPlay() { edb(); return this; }
    virtual tEvent *Copy() { edb(); return new tContPlay(Clock, Data, Length); }
};

class tStopPlay : public tMetaEvent
{
  public:
    tStopPlay(long clk, uchar *dat, ushort len)
      : tMetaEvent(clk, StatStopPlay, dat, len)
    {
    }
    tStopPlay(long clk)
      : tMetaEvent(clk, StatStopPlay, 0, 0)
    {
    }
    virtual tStopPlay *IsStopPlay() { edb(); return this; }
    virtual tEvent *Copy() { edb(); return new tStopPlay(Clock, Data, Length); }
};

class tText : public tMetaEvent
{
  public:
    tText(long clk, uchar *dat, ushort len)
      : tMetaEvent(clk, StatText, dat, len)
    {
    }
    tText(long clk, uchar *dat)
      : tMetaEvent(clk, StatText, dat, strlen((const char*)dat))
    {
    }
    virtual tText *IsText() { edb(); return this; }
    virtual tEvent *Copy() { edb(); return new tText(Clock, Data, Length); }
    virtual uchar* GetText(){return Data;}
};



class tCopyright : public tMetaEvent
{
  public:
    tCopyright(long clk, uchar *dat, ushort len)
      : tMetaEvent(clk, StatCopyright, dat, len)
    {
    }
    virtual tCopyright *IsCopyright() { edb(); return this; }
    virtual tEvent *Copy() { edb(); return new tCopyright(Clock, Data, Length); }
};

class tTrackName : public tMetaEvent
{
  public:
    tTrackName(long clk, uchar *dat, ushort len)
      : tMetaEvent(clk, StatTrackName, dat, len)
    {
// SN++ Diese Restriktion ist viel zu hart. Es genuegt, den Namen im Mixerdialog
//		zu begrenzen!!!
/*
#ifdef wx_motif
      // clip to 16 chars
      if (len > 16)
      {
	Data[16] = 0;
	Length   = 16;
      }
#endif
*/
    }
    virtual tTrackName *IsTrackName() { edb(); return this; }
    virtual tEvent *Copy() { edb(); return new tTrackName(Clock, Data, Length); }
};



class tMarker : public tMetaEvent
{
  public:
    tMarker(long clk, uchar *dat, ushort len)
      : tMetaEvent(clk, StatMarker, dat, len)
    {
    }
    virtual tMarker *IsMarker() { edb(); return this; }
    virtual tEvent *Copy() { edb(); return new tMarker(Clock, Data, Length); }
};

/* the meaning of this event is to be able to reference a track and have that play the instant
the event is executed. you can also transpose the referenced track and loop it for the duration
of the playtrack event. this makes it possible to compose in a structured fashion.

ececution of the events takes place in song.cpp. 
*/
class tPlayTrack : public tMetaEvent
{
  public:
  int track;  //the track to play
  int transpose; //how many steps to transpose the track
  int eventlength; //the length of the event, confusing with the Length field of tMetaEvent, that seems to be for serialization

  virtual int   GetLength()		{ edb(); return eventlength; }
  
  tPlayTrack(long clk, uchar *dat, ushort len)
    : tMetaEvent(clk, StatPlayTrack, dat, len)
    {
      //fill in the fields from the data 
      track=0;
      transpose=0;
      eventlength=0;
      if(dat!=0){
	track=*(((int*)dat)++);
	transpose=*(((int*)dat)++);
	eventlength=*(((int*)dat)++);
      }
    }
  
    tPlayTrack(long clk, int track, int transpose, int eventlength) 
      : tMetaEvent(clk, StatPlayTrack, 0, 0) 
      { 
        this->track=track; 
        this->transpose=transpose;  
        this->eventlength=eventlength;  
      } 

    virtual int Write(tWriteBase &io)

    {
      Data = new uchar [Length + 1];
      uchar* dat=Data;
      *(((int*)dat)++)=track;
      *(((int*)dat)++)=transpose;
      *(((int*)dat)++)=eventlength;
      Length=sizeof(int)*3;
      edb(); 

      Data[Length] = 0;
      return io.Write(this, Data, Length);
    }
  virtual tPlayTrack *IsPlayTrack() { edb(); return this; }
  virtual tEvent *Copy() { edb(); return new tPlayTrack(Clock, track, transpose, eventlength); }
  
  //this event has no real "pitch" but the rest of jazz use the pitch, in the pianowin editor pitch is the y coord of the event
  virtual int   GetPitch(){
    return track;
  }
};



class tSetTempo : public tEvent
{
  public:
    long uSec;

    tSetTempo(long clk, uchar c1, uchar c2, uchar c3)
      : tEvent(clk, StatSetTempo)
    {
      uSec = ((ulong)c1 << 16L) + ((ulong)c2 << 8L) + c3;
    }

    tSetTempo(long clk, long bpm)
      : tEvent(clk, StatSetTempo)
    {
      SetBPM(bpm);
    }

    virtual int GetBPM() { edb(); return int(60000000L / uSec); }
    virtual void SetBPM(long bpm) { uSec = 60000000L / bpm; }

    virtual int   GetPitch()          { edb(); return GetBPM() / 2; }

    virtual int Write(tWriteBase &io)
    {
      edb(); return io.Write(this, (char)(uSec >> 16), (char)(uSec >> 8), (char)uSec);
    }
    virtual tSetTempo *IsSetTempo() { edb(); return this; }
    virtual tEvent *Copy() { edb(); return new tSetTempo(*this); }
};

class tMtcOffset : public tMetaEvent
{
  public:
    tMtcOffset(long clk, uchar *dat, ushort len)
      : tMetaEvent(clk, StatMtcOffset, dat, len)
    {
    }
    virtual tMtcOffset *IsMtcOffset() { edb(); return this; }
    virtual tEvent *Copy() { edb(); return new tMtcOffset(Clock, Data, Length); }
};


class tTimeSignat : public tEvent
{
  public:

    uchar Numerator, Denomiator, Clocks, Quarter;

    tTimeSignat(long clk, uchar c1, uchar c2, uchar c3 = 24, uchar c4 = 8)
      : tEvent(clk, StatTimeSignat)
    {
      Numerator   = c1;
      Denomiator  = c2;
      Clocks      = c3;
      Quarter     = c4;
    }

    virtual int Write(tWriteBase &io)
    {
      edb(); return io.Write(this, Numerator, Denomiator, Clocks, Quarter);
    }

    virtual tTimeSignat *IsTimeSignat() { edb(); return this; }

    virtual void BarInfo(int &TicksPerBar, int &CountsPerBar, int TicksPerQuarter)
    {
      TicksPerBar = int(4L * TicksPerQuarter * Numerator / (1 << Denomiator));
      CountsPerBar = Numerator;
    }

    virtual tEvent *Copy() { edb(); return new tTimeSignat(*this); }
};

//end of track JAVE new event(it is a standard type, i want it to define track loop points, as defined by the midi standard)
class tEndOfTrack : public tEvent
{
  public:

  tEndOfTrack(long clk)
      : tEvent(clk, StatEndOfTrack)
    {
    }

    virtual int Write(tWriteBase &io)
    {
      edb(); return io.Write(this);
    }

    virtual tEndOfTrack *IsEndOfTrack() { edb(); return this; }
    virtual tEvent *Copy() { edb(); return new tEndOfTrack(this->GetClock()); }
};



class tKeySignat : public tEvent
{
  public:
    int Sharps;
    int Minor;

    tKeySignat(long clk, int c1, int c2)
      : tEvent(clk, StatKeySignat)
    {
      Sharps = c1;
      Minor  = c2;
    }

    virtual int Write(tWriteBase &io)
    {
      edb(); return io.Write(this, Sharps, Minor);
    }
    virtual tKeySignat *IsKeySignat() { edb(); return this; }
    virtual tEvent *Copy() { edb(); return new tKeySignat(*this); }
};

// SN++ Aftertouch
class tKeyPressure: public tChannelEvent
{
  public:
    short Value;
    short Key;

    tKeyPressure(long clk, ushort cha, uchar key, uchar val)
      : tChannelEvent(clk, StatKeyPressure, cha)
    {
      Value = val;
      Key = key;
    }

    virtual int Write(tWriteBase &io)
    {
      edb(); return io.Write(this, Key, Value);
    }

    virtual tKeyPressure *IsKeyPressure() { edb(); return this; }
    virtual tEvent *Copy()          { edb(); return new tKeyPressure(*this); }
    virtual int   GetValue()        { edb(); return Value; }
	virtual int   GetPitch()        { edb(); return Key; }
    virtual void  SetPitch(int p)   { edb(); Key = p; }

};

// SN++ Channel Pressure
class tChnPressure : public tChannelEvent
{
  public:
    uchar Value;

    tChnPressure(long clk, int cha, uchar val)
      : tChannelEvent(clk, StatChnPressure, cha)
    {
      Value = val;
    }

    virtual int Write(tWriteBase &io)
    {
      edb(); return io.Write(this, Value);
    }
    virtual tChnPressure  *IsChnPressure() 	        { edb(); return this; }
    virtual tEvent *Copy() 		{ edb(); return new tChnPressure(*this); }
    virtual int   GetValue()     	{ edb(); return Value; }
    virtual int   GetPitch()     	{ edb(); return 0; }
    virtual void  SetPitch(int v)	{ edb();  }
    virtual wxPen *GetPen()            { return wxGREEN_PEN; }
    virtual wxBrush *GetBrush()        { return wxGREEN_BRUSH; }
};

#endif
