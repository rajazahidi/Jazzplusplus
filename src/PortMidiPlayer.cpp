#include "WxWidgets.h"

#include "PortMidiPlayer.h"
#include "TrackWindow.h"
#include "Song.h"
#include "Globals.h"

JZPortMidiPlayer::JZPortMidiPlayer(JZSong* pSong)
  : JZPlayer(pSong),
    mInputDevices(),
    mOutputDevices(),
    mpStream(0),
    mInitialized(false),
    mStartTime(0),
    mStartClock(0),
    mTicksPerMinute(100),
    mInputDevice(),
    mOutputDevice(),
    mInDev(-1),
    mOutDev(-1)
{
  InitPM();
  mInDev = Pm_GetDefaultOutputDeviceID();
  mOutDev = Pm_GetDefaultOutputDeviceID();
  TermPM();

  Pt_Start(1, 0, 0);

//  mOutputQueue = Pm_QueueCreate(1024, sizeof(PmEvent));
}

JZPortMidiPlayer::~JZPortMidiPlayer()
{
//   Pm_QueueDestroy(mOutputQueue);
  TermPM();
}

int
JZPortMidiPlayer::Installed()
{
  return true;
}

wxString
JZPortMidiPlayer::GetInputDeviceName()
{
  return mInputDevice;
}

wxString
JZPortMidiPlayer::GetOutputDeviceName()
{
  return mOutputDevice;
}


void
JZPortMidiPlayer::SetInputDevice(const wxString & name)
{
  bool term = InitPM();
  PmDeviceID id = FindDevice(name, true);

  if (id != pmNoDevice)
  {
    mInputDevice = name;
  }

  if (term)
  {
    TermPM();
  }
}

void
JZPortMidiPlayer::SetOutputDevice(const wxString& name)
{
  bool term = InitPM();
  PmDeviceID id = FindDevice(name, false);

  if (id != pmNoDevice)
  {
    mOutputDevice = name;
  }

  if (term)
  {
    TermPM();
  }
}

int
JZPortMidiPlayer::SupportsMultipleDevices()
{
  return true;
}

tDeviceList& 
JZPortMidiPlayer::GetOutputDevices()
{
  bool term = InitPM();
  int cnt;

  cnt = Pm_CountDevices();

  mOutputDevices.Clear();

  for (int i = 0; i < cnt; i++)
  {
    const PmDeviceInfo *di = Pm_GetDeviceInfo(i);

    if (di && di->output)
    {
      wxString name = 
        wxString(di->interf, wxConvISO8859_1) +
        wxT(", ") +
        wxString(di->name, wxConvISO8859_1);
      mOutputDevices.Add(name);
    }
  }

  if (term)
  {
    TermPM();
  }

  return mOutputDevices;
}

tDeviceList&
JZPortMidiPlayer::GetInputDevices()
{
  bool term = InitPM();
  int cnt;
   
  cnt = Pm_CountDevices();

  mInputDevices.Clear();

  for (int i = 0; i < cnt; i++)
  {
    const PmDeviceInfo *di = Pm_GetDeviceInfo(i);

    if (di && di->input)
    {
      wxString name =
        wxString(di->interf, wxConvISO8859_1) +
        wxT(", ") +
        wxString(di->name, wxConvISO8859_1);
      mInputDevices.Add(name);
    }
  }

  if (term)
  {
    TermPM();
  }

  return mInputDevices;
}

PmDeviceID
JZPortMidiPlayer::FindDevice(const wxString & name, bool input)
{
  int cnt = Pm_CountDevices();

  for (int i = 0; i < cnt; i++)
  {
    const PmDeviceInfo *di = Pm_GetDeviceInfo(i);

    if (di && (input ? di->input : di->output))
    {
      wxString n =
        wxString(di->interf, wxConvISO8859_1) +
        wxT(", ") +
        wxString(di->name, wxConvISO8859_1);

      if (name == n)
      {
        return i;
      }
    }
  }

  return pmNoDevice;
}

int
JZPortMidiPlayer::Clock2Time(int clock)
{
  if (clock < mStartClock)
  {
    return mStartTime;
  }

  return (int)((double)(clock - mStartClock) * 60000.0 / (double) mTicksPerMinute + mStartTime);
}

int
JZPortMidiPlayer::Time2Clock(int time)
{
  if (time < mStartTime)
  {
    return mStartClock;
  }

  return (int)((double)(time - mStartTime) * (double) mTicksPerMinute / 60000.0 + mStartClock);
}

void
JZPortMidiPlayer::SetTempo(int bpm, int clock)
{
  int t1 = Clock2Time(clock);
  mTicksPerMinute = bpm * Song->GetTicksPerQuarter();
  int t2 = Clock2Time(clock);
  mStartTime += (t1 - t2);
}

void
JZPortMidiPlayer::OutBreak()
{
}

int
JZPortMidiPlayer::OutEvent(JZEvent* pEvent, int now)
{
  PmError rc = pmNoError;
  PmTimestamp t = (now ? 0 : pEvent->GetClock());

  t = Clock2Time(t);

#define WSHORT(a, b) \
  rc = Pm_WriteShort(mpStream, t, Pm_Message(pEvent->GetStat() | k->GetChannel(), a, b))

  switch (pEvent->GetStat())
  {
    case StatKeyOn:
    {
      tKeyOn *k = pEvent->IsKeyOn();

      WSHORT(k->GetKey(), k->GetVelocity());
    }
    break;

    case StatKeyOff:
    {
      tKeyOff *k = pEvent->IsKeyOff();

      WSHORT(k->GetKey(), k->GetOffVelocity());
    }
    break;

    case StatProgram:
    {
      tProgram *k = pEvent->IsProgram();

      WSHORT(k->GetProgram(), 0);
    }
    break;

    case StatKeyPressure:
    {
      tKeyPressure *k = pEvent->IsKeyPressure();

      WSHORT(k->GetKey(), k->GetValue());
    }
    break;

    case StatChnPressure:
    {
      tChnPressure *k = pEvent->IsChnPressure();

      WSHORT(k->GetValue(), 0);
    }
    break;

    case StatControl:
    {
      tControl *k = pEvent->IsControl();

      WSHORT(k->GetControl(), k->GetValue());
    }
    break;

    case StatPitch:
    {
      tPitch *k = pEvent->IsPitch();

      WSHORT(k->GetValue(), 0);
    }
    break;

    case StatSetTempo:
    {
      tSetTempo *k = pEvent->IsSetTempo();
      if (k->GetClock() > 0)
      {
        SetTempo(k->GetBPM(), k->GetClock());
      }
    }
    break;

    case StatSysEx:
    {
      tSysEx *s = pEvent->IsSysEx();

      unsigned char *buf = new unsigned char[s->GetLength() + 2];

      buf[0] = 0xF0;
      memcpy(&buf[1], s->GetData(), s->GetLength());
      buf[s->GetLength() + 2] = 0xf7;
      rc = Pm_WriteSysEx(mpStream, t, buf);

      delete[] buf;
    }
    break;

    default:
      break;
  }

  return rc != pmNoError;
}

int
JZPortMidiPlayer::OutEvent(JZEvent* pEvent)
{
  return OutEvent(pEvent, 0);
}

void
JZPortMidiPlayer::OutNow(JZEvent*pEvent)
{
  OutEvent(pEvent, 1);
}

void
JZPortMidiPlayer::StartPlay(int clock, int loopClock, int cont)
{
  bool term = InitPM();
  PmDeviceID id = FindDevice(mOutputDevice, false);

  if (id == pmNoDevice)
  {
    if (term)
    {
      TermPM();
    }

    return;
  }

  printf("rc = %d %d\n", Pm_OpenOutput(&mpStream, id, NULL, 0, NULL, NULL, 100), id);

  mStartTime = Pt_Time() + 500;
  mStartClock = clock;
  mTicksPerMinute  = Song->GetTicksPerQuarter() * Song->Speed();

  JZPlayer::StartPlay(clock, loopClock, cont);
}

void
JZPortMidiPlayer::StopPlay()
{
  JZPlayer::StopPlay();

  if (mpStream)
  {
    Pm_Abort(mpStream);
    Pm_Close(mpStream);
    mpStream = NULL;
  }

  TermPM();
}

long
JZPortMidiPlayer::GetRealTimeClock()
{
  long t = Pt_Time();

  gpTrackWindow->NewPlayPosition(PlayLoop->Ext2IntClock(Time2Clock(t) / 48 * 48));

  return Time2Clock(t);
}

bool
JZPortMidiPlayer::InitPM()
{
  if (mInitialized)
  {
    return false;
  }

  Pm_Initialize();

  mInitialized = true;

  return true;
}

bool
JZPortMidiPlayer::TermPM()
{
  if (!mInitialized)
  {
    return false;
  }

  Pm_Terminate();

  mInitialized = false;

  return true;
}
