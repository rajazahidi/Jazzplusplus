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

#ifndef JZ_EVENTS_H
#define JZ_EVENTS_H

class JZEvent;

#include <cstdio>

//*****************************************************************************
// Output device, can be
//   - Midi-Standard-File
//   - Ascii-File
//   - Midi-Port
//*****************************************************************************
class JZReadBase
{
  public:

    JZReadBase();

    virtual ~JZReadBase();

    virtual int Open(const char* pFileName);

    virtual void Close();

    int GetTicksPerQuarter() const;

    virtual JZEvent* Read() = 0;

    virtual int NextTrack() = 0;

  protected:

    // Ths value is known after a call to Open.
    int mTicksPerQuarter;

    int mTrackCount;

    FILE* mpFd;
};

//*****************************************************************************
// Description:
//   These are the read base class inline member functions.
//*****************************************************************************
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline
int JZReadBase::GetTicksPerQuarter() const
{
  return mTicksPerQuarter;
}

//*****************************************************************************
//*****************************************************************************
class JZWriteBase
{
  public:

    JZWriteBase();

    virtual ~JZWriteBase();

    virtual int Open(
      const char* pFileName,
      int TrackCount,
      int TicksPerQuarter);

    virtual void Close();

    virtual int Write(JZEvent* pEvent);

    virtual int Write(JZEvent* pEvent, unsigned char Character);

    virtual int Write(
      JZEvent* pEvent,
      unsigned char Character1,
      unsigned char Character2);

    virtual int Write(
      JZEvent* pEvent,
      unsigned char Character1,
      unsigned char Character2,
      unsigned char Character3);

    virtual int Write(
      JZEvent* pEvent,
      unsigned char Character1,
      unsigned char Character2,
      unsigned char Character3,
      unsigned char Character4);

    virtual int Write(
      JZEvent* pEvent,
      unsigned char* pData,
      int Length) = 0;

    virtual void NextTrack();

  protected:

    FILE* mpFd;
};

//*****************************************************************************
// Description:
//   These are the write base class inline member functions.
//*****************************************************************************
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline
int JZWriteBase::Write(JZEvent* pEvent)
{
  return Write(pEvent, 0, 0);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline
int JZWriteBase::Write(JZEvent* pEvent, unsigned char Character)
{
  return Write(pEvent, &Character, 1);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline
int JZWriteBase::Write(
  JZEvent* pEvent,
  unsigned char Character1,
  unsigned char Character2)
{
  unsigned char Array[2];

  Array[0] = Character1;
  Array[1] = Character2;

  return Write(pEvent, Array, 2);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline
int JZWriteBase::Write(
  JZEvent* pEvent,
  unsigned char Character1,
  unsigned char Character2,
  unsigned char Character3)
{
  unsigned char Array[3];

  Array[0] = Character1;
  Array[1] = Character2;
  Array[2] = Character3;

  return Write(pEvent, Array, 3);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline
int JZWriteBase::Write(
  JZEvent* pEvent,
  unsigned char Character1,
  unsigned char Character2,
  unsigned char Character3,
  unsigned char Character4)
{
  unsigned char Array[4];

  Array[0] = Character1;
  Array[1] = Character2;
  Array[2] = Character3;
  Array[3] = Character4;

  return Write(pEvent, Array, 4);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline
void JZWriteBase::NextTrack()
{
}

//*****************************************************************************
// tGetMidiBytes
//*****************************************************************************
class tGetMidiBytes : public JZWriteBase
{
  public:

    int Open(const char* pFileName, int nTracks, int TicksPerQuarter)
    {
      return 1;
    }

    void Close()
    {
    }

    void NextTrack()
    {
    }

    // Get JZEvent's bytes.
    int Write(JZEvent* pEvent, unsigned char* pString, int Length);

    unsigned char Buffer[10];
    int nBytes;
};

// ********************************************************************
// Midi-Events
// ********************************************************************

/*
 * Normal events (with the channel)
 */

#define StatKeyOff         0x80
#define StatKeyOn          0x90
#define StatKeyPressure    0xA0            // Key pressure
#define StatControl        0xB0
#define StatProgram        0xC0
#define StatChnPressure    0xD0            // SN++ Channel pressure
#define StatPitch          0xE0

/*
 * Meta events (no channel)
 */

#define StatSysEx          0xF0
#define StatSongPtr        0xF2
#define StatSongSelect     0xF3
#define StatTuneRequest    0xF6
#define StatMidiClock      0xf8
#define StatStartPlay      0xfa
#define StatContPlay       0xfb
#define StatStopPlay       0xfc
#define StatText           0x01
#define StatCopyright      0x02
#define StatTrackName      0x03
#define StatMarker         0x06
#define StatEndOfTrack     0x2F
#define StatSetTempo       0x51
#define StatMtcOffset      0x54
#define StatTimeSignat     0x58
#define StatKeySignat      0x59

#define StatUnknown        0x00

// Proprietary event status
#define StatJazzMeta       0x7F
#define StatPlayTrack      0x7E


#define LAST_CLOCK        (0x7fffffff)
#define KILLED_CLOCK      (0x80000000)




// Event-Classes

class JZEvent;
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

class JZEvent
{
  public:

#ifdef E_DBUG
    int Magic;
    void edb() const
    {
      if (Magic != MAGIC)
      {
        fprintf(stderr, "Magic failed\n");
        fflush(stderr);
        *(char *)0 = 0;
      }
    }
#else
    void edb() const
    {
    }
#endif

    unsigned char Stat;
    int mClock;  // should be protected ...

    int GetClock() const
    {
      return mClock & ~KILLED_CLOCK;
    }
    void SetClock(int c)
    {
      mClock = c;
    }

    // the device is dynamically set when events are copied to
    // the playback queue (from the track device)
    enum
    {
      BROADCAST_DEVICE = 0
    };

    JZEvent(int clk, unsigned char sta)
    {
      mClock = clk;
      Stat  = sta;
      Device = BROADCAST_DEVICE;
#ifdef E_DBUG
      Magic = MAGIC;
#endif
    }

    virtual ~JZEvent()
    {
      edb();
#ifdef E_DBUG
      Magic = 0L;
#endif
    }

    void Kill()
    {
      edb();
      mClock |= KILLED_CLOCK;
    }

    void UnKill()
    {
      edb();
      mClock &= ~KILLED_CLOCK;
    }

    int IsKilled()
    {
      edb();
      return (mClock & KILLED_CLOCK) != 0;
    }

    virtual tMetaEvent*    IsMetaEvent()    { edb(); return 0; }
    virtual tChannelEvent* IsChannelEvent() { edb(); return 0; }
    virtual tKeyOn*        IsKeyOn()        { edb(); return 0; }
    virtual tKeyOff*       IsKeyOff()       { edb(); return 0; }
    virtual tPitch*        IsPitch()        { edb(); return 0; }
    virtual tControl*      IsControl()      { edb(); return 0; }
    virtual tProgram*      IsProgram()      { edb(); return 0; }
    virtual tSysEx*        IsSysEx()        { edb(); return 0; }
    virtual tSongPtr*      IsSongPtr()      { edb(); return 0; }
    virtual tMidiClock*    IsMidiClock()    { edb(); return 0; }
    virtual tStartPlay*    IsStartPlay()    { edb(); return 0; }
    virtual tContPlay*     IsContPlay()     { edb(); return 0; }
    virtual tStopPlay*     IsStopPlay()     { edb(); return 0; }
    virtual tText*         IsText()         { edb(); return 0; }
    virtual tCopyright*    IsCopyright()    { edb(); return 0; }
    virtual tTrackName*    IsTrackName()    { edb(); return 0; }
    virtual tMarker*       IsMarker()       { edb(); return 0; }
    virtual tSetTempo*     IsSetTempo()     { edb(); return 0; }
    virtual tMtcOffset*    IsMtcOffset()    { edb(); return 0; }
    virtual tTimeSignat*   IsTimeSignat()   { edb(); return 0; }
    virtual tKeySignat*    IsKeySignat()    { edb(); return 0; }
    virtual tKeyPressure*  IsKeyPressure()  { edb(); return 0; }
    virtual tJazzMeta*     IsJazzMeta()     { edb(); return 0; }
    virtual tPlayTrack*    IsPlayTrack()    { edb(); return 0; }
    virtual tEndOfTrack*   IsEndOfTrack()   { edb(); return 0; }
    virtual tChnPressure*  IsChnPressure()  { edb(); return 0; }

    virtual int Write(JZWriteBase& io)
    {
      edb();
      return io.Write(this);
    }

    int Compare(JZEvent& Event)
    {
      edb();
      if ((unsigned)Event.mClock > (unsigned)mClock)
      {
        return -1;
      }
      if ((unsigned)Event.mClock < (unsigned)mClock)
      {
        return 1;
      }
      return 0;
    }

    virtual void BarInfo(
      int& TicksPerBar,
      int& CountsPerBar,
      int TicksPerQuarter) const
    {
    }

    virtual JZEvent* Copy() const
    {
      edb();
      return new JZEvent(*this);
    }

    // Filter
    virtual int GetValue() const
    {
      edb();
      return 0;
    }

    virtual unsigned short GetEventLength() const
    {
      edb();
      return 16;
    }

    // Painting
    virtual int GetLength() const
    {
      edb();
      return 16;
    }

    // Value normalized to 0 to 127.
    virtual int GetPitch() const
    {
      edb();
      return 64;
    }

    virtual void SetPitch(int p)
    {
      edb();
    }

    virtual const wxPen* GetPen() const
    {
      return wxBLACK_PEN;
    }

    virtual const wxBrush* GetBrush() const
    {
      return wxBLACK_BRUSH;
    }

    int GetDevice() const
    {
      return Device;
    }

    void SetDevice(int d)
    {
      Device = d;
    }

 private:

    int Device;
};


class tChannelEvent : public JZEvent
{
  public:

    unsigned char Channel;

    tChannelEvent(int clk, unsigned char sta, int cha)
      : JZEvent(clk, sta)
    {
      Channel = cha;
    }

    virtual tChannelEvent* IsChannelEvent()
    {
      edb();
      return this;
    }

    virtual JZEvent* Copy() const
    {
      edb();
      return new tChannelEvent(*this);
    }
};



//*****************************************************************************
//*****************************************************************************
class tKeyOn : public tChannelEvent
{
  public:

    unsigned char mKey;
    unsigned char mVelocity;

    // Length is 0 if a corresponding tKeyOff exists.
    unsigned short mLength;

  private:

    unsigned short mOffVelocity;

  public:

    tKeyOn(
      int clk,
      int cha,
      unsigned char Key,
      unsigned char Velocity,
      unsigned short len = 0)
      : tChannelEvent(clk, StatKeyOn, cha),
        mKey(Key),
        mVelocity(Velocity),
        mLength(len),
        mOffVelocity(0)
    {
    }

    virtual int Write(JZWriteBase &io)
    {
      edb();
      return io.Write(this, mKey, mVelocity);
    }

    virtual tKeyOn* IsKeyOn()
    {
      edb();
      return this;
    }

    virtual JZEvent* Copy() const
    {
      edb();
      return new tKeyOn(*this);
    }

    virtual unsigned short GetEventLength() const
    {
      edb();
      return mLength;
    }

    virtual int GetLength() const
    {
      edb();
      return mLength;
    }

    virtual int GetValue() const
    {
      edb();
      return mKey;
    }

    virtual int GetPitch() const
    {
      edb();
      return mKey;
    }

    virtual void SetPitch(int p)
    {
      edb();
      mKey = p;
    }

    unsigned short GetOffVelocity() const
    {
      return mOffVelocity;
    }

    void SetOffVelocity(int OffVelocity)
    {
      mOffVelocity = OffVelocity;
    }

    virtual const wxPen* GetPen() const
    {
      return wxBLACK_PEN;
    }

    virtual const wxBrush* GetBrush() const
    {
      return wxBLACK_BRUSH;
    }
};


class tKeyOff : public tChannelEvent
{
  public:
    unsigned char Key;
    // SN++
    unsigned char OffVeloc;

    tKeyOff(int clk, int cha, unsigned char key, unsigned char veloc = 0)
      : tChannelEvent(clk, StatKeyOff, cha)
    {
      Key = key;
      OffVeloc = veloc;
    }

    virtual int Write(JZWriteBase &io)
    {
      edb(); return io.Write(this, Key, OffVeloc);
    }

    virtual tKeyOff* IsKeyOff()
    {
      edb();
      return this;
    }

    virtual JZEvent* Copy() const
    {
      edb();
      return new tKeyOff(*this);
    }
};


class tPitch : public tChannelEvent
{
  public:
    short Value;

    tPitch(int clk, unsigned short cha, unsigned char lo, unsigned char hi)
      : tChannelEvent(clk, StatPitch, cha)
    {
      Value  = ((hi << 7) | lo) - 8192;
    }

    tPitch(int clk, unsigned short cha, short val)
      : tChannelEvent(clk, StatPitch, cha)
    {
      Value  = val;
    }

    virtual int Write(JZWriteBase &io)
    {
      int v = Value + 8192;
      edb(); return io.Write(this, (unsigned char)(v & 0x7F), (unsigned char)(v >> 7));
    }

    virtual tPitch* IsPitch()
    {
      edb();
      return this;
    }

    virtual JZEvent* Copy() const
    {
      edb();
      return new tPitch(*this);
    }

    virtual int GetValue() const
    {
      edb();
      return Value;
    }

    virtual int GetPitch() const
    {
      edb();
      return (Value + 8192) >> 7;
    }

    virtual void SetPitch(int p)
    {
      edb();
      Value = (p << 7) - 8192;
    }

    virtual const wxPen* GetPen() const
    {
      return wxRED_PEN;
    }

    virtual const wxBrush* GetBrush() const
    {
      return wxRED_BRUSH;
    }
};



class tControl : public tChannelEvent
{
  public:

    unsigned char Control;
    unsigned char Value;

    tControl(int clk, int cha, unsigned char ctl, unsigned char val)
      : tChannelEvent(clk, StatControl, cha)
    {
      Control = ctl;
      Value   = val;
    }

    virtual int Write(JZWriteBase &io)
    {
      edb();
      return io.Write(this, Control, Value);
    }

    virtual tControl* IsControl()
    {
      edb();
      return this;
    }

    virtual JZEvent* Copy() const
    {
      edb();
      return new tControl(*this);
    }

    virtual int GetValue() const
    {
      edb();
      return Control;
    }

    virtual int GetPitch() const
    {
      edb();
      return Control;
    }

    virtual void SetPitch(int p)
    {
      edb();
      Control = p;
    }

    virtual const wxPen* GetPen() const
    {
      return wxCYAN_PEN;
    }

    virtual const wxBrush* GetBrush() const
    {
      return wxCYAN_BRUSH;
    }
};


class tProgram : public tChannelEvent
{
  public:

    unsigned char Program;

    tProgram(int clk, int cha, unsigned char prg)
      : tChannelEvent(clk, StatProgram, cha)
    {
      Program = prg;
    }

    virtual int Write(JZWriteBase &io)
    {
      edb(); return io.Write(this, Program);
    }

    virtual tProgram* IsProgram()
    {
      edb();
      return this;
    }

    virtual JZEvent* Copy() const
    {
      edb();
      return new tProgram(*this);
    }

    virtual int GetValue() const
    {
      edb();
      return Program;
    }

    virtual int GetPitch() const
    {
      edb();
      return Program;
    }

    virtual void SetPitch(int p)
    {
      edb();
      Program = p;
    }

    virtual const wxPen* GetPen() const
    {
      return wxGREEN_PEN;
    }

    virtual const wxBrush* GetBrush() const
    {
      return wxGREEN_BRUSH;
    }
};



class tMetaEvent : public JZEvent
{
  public:

    unsigned char* mpData;
    unsigned short Length;

    tMetaEvent(
      int clk,
      unsigned char sta,
      unsigned char* dat,
      unsigned short len)
      : JZEvent(clk, sta)
    {
      Length = len;
      mpData = new unsigned char [len + 1];
      if (dat)
        memcpy(mpData, dat, len);
      mpData[len] = 0;
    }

    virtual ~tMetaEvent()
    {
      delete [] mpData;
    }

    virtual int Write(JZWriteBase &io)
    {
      edb();
      return io.Write(this, mpData, Length);
    }

    virtual tMetaEvent* IsMetaEvent()
    {
      edb();
      return this;
    }

    virtual JZEvent* Copy() const
    {
      edb();
      return new tMetaEvent(mClock, Stat, mpData, Length);
    }
};



class tJazzMeta : public tMetaEvent
{
  // proprietary information of jazz stored to a track. This event
  // should not go into a track itself but is read/written from/to
  // the file.
  public:
    enum { DATALEN = 20 };
    tJazzMeta(int clk, unsigned char *dat, unsigned short len)
      : tMetaEvent(clk, StatJazzMeta, dat, len)
    {
    }
    tJazzMeta()
      : tMetaEvent(0, StatJazzMeta, 0, DATALEN)
    {
      memset(mpData, 0, DATALEN);
      memcpy(mpData, "JAZ2", 4);
      mpData[4] = 1; // version or so
    }
    char GetAudioMode() const
    {
      return mpData[5];
    }
    void SetAudioMode(char c)
    {
      mpData[5] = c;
    }
    char GetTrackState() const
    {
      return mpData[6];
    }
    void SetTrackState(char c)
    {
      mpData[6] = c;
    }

    // mpData[7] is unused

    unsigned char GetTrackDevice() const
    {
      return mpData[8];
    }

    void SetTrackDevice(unsigned char x)
    {
      mpData[8] = x;
    }

    unsigned char GetIntroLength() const
    {
      return mpData[9];
    }

    void SetIntroLength(unsigned char x)
    {
      mpData[9] = x;
    }

    virtual tJazzMeta* IsJazzMeta()
    {
      edb();
      return this;
    }

    virtual JZEvent* Copy() const
    {
      edb();
      return new tJazzMeta(mClock, mpData, Length);
    }
};


class tSysEx : public tMetaEvent
{
  public:

    tSysEx(int clk, unsigned char *dat, unsigned short len)
      : tMetaEvent(clk, StatSysEx, dat, len)
    {
    }

    virtual tSysEx* IsSysEx()
    {
      edb();
      return this;
    }

    virtual JZEvent* Copy() const
    {
      edb();
      return new tSysEx(mClock, mpData, Length);
    }

    // todo
    virtual int GetPitch() const;
};

class tSongPtr : public tMetaEvent
{
  public:

    tSongPtr(int clk, unsigned char *dat, unsigned short len)
      : tMetaEvent(clk, StatSongPtr, dat, len)
    {
    }

    virtual tSongPtr* IsSongPtr()
    {
      edb();
      return this;
    }

    virtual JZEvent* Copy() const
    {
      edb();
      return new tSongPtr(mClock, mpData, Length);
    }
};

class tMidiClock : public tMetaEvent
{
  public:
    tMidiClock(int clk, unsigned char *dat, unsigned short len)
      : tMetaEvent(clk, StatMidiClock, dat, len)
    {
    }

    tMidiClock(int clk)
      : tMetaEvent(clk, StatMidiClock, 0, 0)
    {
    }

    virtual tMidiClock *IsMidiClock()
    {
      edb();
      return this;
    }

    virtual JZEvent* Copy() const
    {
      edb();
      return new tMidiClock(mClock, mpData, Length);
    }
};

class tStartPlay : public tMetaEvent
{
  public:
    tStartPlay(int clk, unsigned char *dat, unsigned short len)
      : tMetaEvent(clk, StatStartPlay, dat, len)
    {
    }
    tStartPlay(int clk)
      : tMetaEvent(clk, StatStartPlay, 0, 0)
    {
    }
    virtual tStartPlay* IsStartPlay()
    {
      edb();
      return this;
    }

    virtual JZEvent* Copy() const
    {
      edb();
      return new tStartPlay(mClock, mpData, Length);
    }
};

class tContPlay : public tMetaEvent
{
  public:
    tContPlay(int clk, unsigned char *dat, unsigned short len)
      : tMetaEvent(clk, StatContPlay, dat, len)
    {
    }

    tContPlay(int clk)
      : tMetaEvent(clk, StatContPlay, 0, 0)
    {
    }

    virtual tContPlay* IsContPlay()
    {
      edb();
      return this;
    }

    virtual JZEvent* Copy() const
    {
      edb();
      return new tContPlay(mClock, mpData, Length);
    }
};

class tStopPlay : public tMetaEvent
{
  public:

    tStopPlay(int clk, unsigned char *dat, unsigned short len)
      : tMetaEvent(clk, StatStopPlay, dat, len)
    {
    }

    tStopPlay(int clk)
      : tMetaEvent(clk, StatStopPlay, 0, 0)
    {
    }

    virtual tStopPlay* IsStopPlay()
    {
      edb();
      return this;
    }

    virtual JZEvent* Copy() const
    {
      edb();
      return new tStopPlay(mClock, mpData, Length);
    }
};

class tText : public tMetaEvent
{
  public:
    tText(int clk, unsigned char *dat, unsigned short len)
      : tMetaEvent(clk, StatText, dat, len)
    {
    }
    tText(int clk, unsigned char *dat)
      : tMetaEvent(clk, StatText, dat, strlen((const char*)dat))
    {
    }
    virtual tText* IsText()
    {
      edb();
      return this;
    }

    virtual JZEvent* Copy() const
    {
      edb();
      return new tText(mClock, mpData, Length);
    }

    virtual unsigned char* GetText()
    {
      return mpData;
    }
};



class tCopyright : public tMetaEvent
{
  public:

    tCopyright(int clk, unsigned char *dat, unsigned short len)
      : tMetaEvent(clk, StatCopyright, dat, len)
    {
    }

    virtual tCopyright* IsCopyright()
    {
      edb();
      return this;
    }

    virtual JZEvent* Copy() const
    {
      edb();
      return new tCopyright(mClock, mpData, Length);
    }
};

class tTrackName : public tMetaEvent
{
  public:

    tTrackName(int clk, unsigned char *dat, unsigned short len)
      : tMetaEvent(clk, StatTrackName, dat, len)
    {
// SN++ Diese Restriktion ist viel zu hart. Es genuegt, den Namen im Mixerdialog
//                zu begrenzen!!!
/*
#ifdef wx_motif
      // clip to 16 chars
      if (len > 16)
      {
        mpData[16] = 0;
        Length   = 16;
      }
#endif
*/
    }

    virtual tTrackName* IsTrackName()
    {
      edb();
      return this;
    }

    virtual JZEvent* Copy() const
    {
      edb();
      return new tTrackName(mClock, mpData, Length);
    }
};



class tMarker : public tMetaEvent
{
  public:

    tMarker(int clk, unsigned char *dat, unsigned short len)
      : tMetaEvent(clk, StatMarker, dat, len)
    {
    }

    virtual tMarker* IsMarker()
    {
      edb();
      return this;
    }

    virtual JZEvent* Copy() const
    {
      edb();
      return new tMarker(mClock, mpData, Length);
    }
};

// the meaning of this event is to be able to reference a track and have that
// play the instant the event is executed.  You can also transpose the
// referenced track and loop it for the duration of the playtrack event.  This
// makes it possible to compose in a structured fashion.
//
// Execution of the events takes place in song.cpp.
class tPlayTrack : public tMetaEvent
{
  public:

    // The index of the track to play.
    int track;

    // How many steps to transpose the track.
    int transpose;

    // The length of the event, confusing with the Length field of tMetaEvent,
    // that seems to be for serialization.
    int eventlength;

    tPlayTrack(int clk, unsigned char *chardat, unsigned short len)
      : tMetaEvent(clk, StatPlayTrack, chardat, len)
    {
      int *dat = (int *)chardat;
    
      // Fill in the fields from the data.
      track = 0;
      transpose = 0;
      eventlength = 0;
      if (dat!=0)
      {
        track = dat[0];
        transpose = dat[1];
        eventlength = dat[2];
      }
    }

    tPlayTrack(int clk, int track, int transpose, int eventlength)
      : tMetaEvent(clk, StatPlayTrack, 0, 0)
    {
      this->track=track;
      this->transpose=transpose;
      this->eventlength=eventlength;
    }

    virtual int GetLength() const
    {
      edb();
      return eventlength;
    }

    virtual int Write(JZWriteBase &io)
    {
      mpData = new unsigned char [Length + 1];
      int* dat = (int *)mpData;
      dat[0] = track;
      dat[1] = transpose;
      dat[2] = eventlength;
      Length = sizeof(int) * 3;
      edb();

      mpData[Length] = 0;
      return io.Write(this, mpData, Length);
    }

    virtual tPlayTrack* IsPlayTrack()
    {
      edb();
      return this;
    }

    virtual JZEvent* Copy() const
    {
      edb();
      return new tPlayTrack(mClock, track, transpose, eventlength);
    }

    // This event has no real "pitch" but the rest of jazz use the pitch.  In
    // the piano winow editor, pitch is the y coordinate of the event.
    virtual int GetPitch() const
    {
      return track;
    }
};



class tSetTempo : public JZEvent
{
  public:
    int uSec;

    tSetTempo(
      int clk,
      unsigned char Character1,
      unsigned char Character2,
      unsigned char Character3)
      : JZEvent(clk, StatSetTempo)
    {
      uSec =
        ((unsigned)Character1 << 16L) +
        ((unsigned)Character2 << 8L) +
        Character3;
    }

    tSetTempo(int clk, int bpm)
      : JZEvent(clk, StatSetTempo)
    {
      SetBPM(bpm);
    }

    virtual int GetBPM() const
    {
      edb();
      return int(60000000L / uSec);
    }

    virtual void SetBPM(int bpm)
    {
      uSec = 60000000L / bpm;
    }

    virtual int GetPitch() const
    {
      edb();
      return GetBPM() / 2;
    }

    virtual int Write(JZWriteBase &io)
    {
      edb();
      return io.Write(this, (char)(uSec >> 16), (char)(uSec >> 8), (char)uSec);
    }

    virtual tSetTempo* IsSetTempo()
    {
      edb();
      return this;
    }

    virtual JZEvent* Copy() const
    {
      edb();
      return new tSetTempo(*this);
    }
};

class tMtcOffset : public tMetaEvent
{
  public:
    tMtcOffset(int clk, unsigned char *dat, unsigned short len)
      : tMetaEvent(clk, StatMtcOffset, dat, len)
    {
    }
    virtual tMtcOffset* IsMtcOffset()
    {
      edb();
      return this;
    }

    virtual JZEvent* Copy() const
    {
      edb();
      return new tMtcOffset(mClock, mpData, Length);
    }
};


class tTimeSignat : public JZEvent
{
  public:

    unsigned char Numerator, Denomiator, Clocks, Quarter;

    tTimeSignat(
      int clk,
      unsigned char Character1,
      unsigned char Character2,
      unsigned char Character3 = 24,
      unsigned char Character4 = 8)
      : JZEvent(clk, StatTimeSignat)
    {
      Numerator   = Character1;
      Denomiator  = Character2;
      Clocks      = Character3;
      Quarter     = Character4;
    }

    virtual int Write(JZWriteBase &io)
    {
      edb(); return io.Write(this, Numerator, Denomiator, Clocks, Quarter);
    }

    virtual tTimeSignat* IsTimeSignat()
    {
      edb();
      return this;
    }

    virtual void BarInfo(
      int &TicksPerBar,
      int &CountsPerBar,
      int TicksPerQuarter) const
    {
      TicksPerBar = 4 * TicksPerQuarter * Numerator / (1 << Denomiator);
      CountsPerBar = Numerator;
    }

    virtual JZEvent* Copy() const
    {
      edb();
      return new tTimeSignat(*this);
    }
};

//end of track JAVE new event(it is a standard type, i want it to define track loop points, as defined by the midi standard)
class tEndOfTrack : public JZEvent
{
  public:

    tEndOfTrack(int clk)
      : JZEvent(clk, StatEndOfTrack)
    {
    }

    virtual int Write(JZWriteBase &io)
    {
      edb(); return io.Write(this);
    }

    virtual tEndOfTrack* IsEndOfTrack()
    {
      edb();
      return this;
    }

    virtual JZEvent* Copy() const
    {
      edb();
      return new tEndOfTrack(this->GetClock());
    }
};



class tKeySignat : public JZEvent
{
  public:
    int Sharps;
    int Minor;

    tKeySignat(int clk, int Character1, int Character2)
      : JZEvent(clk, StatKeySignat)
    {
      Sharps = Character1;
      Minor  = Character2;
    }

    virtual int Write(JZWriteBase &io)
    {
      edb(); return io.Write(this, Sharps, Minor);
    }

    virtual tKeySignat* IsKeySignat()
    {
      edb();
      return this;
    }

    virtual JZEvent* Copy() const
    {
      edb();
      return new tKeySignat(*this);
    }
};

// SN++ Aftertouch
class tKeyPressure: public tChannelEvent
{
  public:
    short Value;
    short Key;

    tKeyPressure(int clk, unsigned short cha, unsigned char key, unsigned char val)
      : tChannelEvent(clk, StatKeyPressure, cha)
    {
      Value = val;
      Key = key;
    }

    virtual int Write(JZWriteBase &io)
    {
      edb(); return io.Write(this, Key, Value);
    }

    virtual tKeyPressure* IsKeyPressure()
    {
      edb();
      return this;
    }

    virtual JZEvent* Copy() const
    {
      edb();
      return new tKeyPressure(*this);
    }

    virtual int GetValue() const
    {
      edb();
      return Value;
    }

    virtual int GetPitch() const
    {
      edb();
      return Key;
    }

    virtual void SetPitch(int p)
    {
      edb();
      Key = p;
    }
};

// SN++ Channel Pressure
class tChnPressure : public tChannelEvent
{
  public:
    unsigned char Value;

    tChnPressure(int clk, int cha, unsigned char val)
      : tChannelEvent(clk, StatChnPressure, cha)
    {
      Value = val;
    }

    virtual int Write(JZWriteBase &io)
    {
      edb(); return io.Write(this, Value);
    }

    virtual tChnPressure* IsChnPressure()
    {
      edb();
      return this;
    }

    virtual JZEvent* Copy() const
    {
      edb();
      return new tChnPressure(*this);
    }

    virtual int GetValue() const
    {
      edb();
      return Value;
    }
    virtual int GetPitch() const
    {
      edb();
      return 0;
    }
    virtual void  SetPitch(int v)        { edb();  }

    virtual const wxPen* GetPen() const
    {
      return wxGREEN_PEN;
    }

    virtual const wxBrush* GetBrush() const
    {
      return wxGREEN_BRUSH;
    }
};

#endif // !defined(JZ_EVENTS_H)
