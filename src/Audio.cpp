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

#include "Sample.h"
#include "Audio.h"
#include "Events.h"
#include "RecordingInfo.h"
#include "Track.h"
#include "Globals.h"
#include "Player.h"
#include "TrackFrame.h"
#include "TrackWindow.h"
#include "SampleWindow.h"
#include "Random.h"
#include "FindFile.h"
#include "FileSelector.h"
#include "StringReadWrite.h"
#include "Help.h"

#include <iostream>
#include <fstream>
#include <cmath>
#include <cstdlib>

#include <assert.h>
#include <sys/stat.h>
#include <string.h>

using namespace std;

#define db(a) cout << #a << " = " << a << endl

// -----------------------------------------------------------


class tSampleVoice {
  /*
   * activated on note on. Copies data from a tSample Object to the
   * output buffer as needed by the driver.
   */

  public:

    tSampleVoice(tSampleSet &s) : set(s) {
    }

    void Start(tSample *s, long c) {
      spl    = s;
      clock  = c;
      first  = 1;
      length = spl->length;
      data   = spl->data;
      prev   = 0;
    }

    void AddBuffer(short *b, long buffer_clock, unsigned int bufsize) {

      // everything done?
      if (length <= 0)
        return;

      // not this buffer
      if (buffer_clock + set.clocks_per_buffer <= clock)
        return;

      // compute offset in buffer
      long offset = 0;
      if (first)
      {
        if (clock >= buffer_clock)
        {
          offset = set.Ticks2Samples(clock - buffer_clock);
        }
        else
        {
          // output starts somewhere in the middle of a sample
          long data_offs = set.Ticks2Samples(buffer_clock - clock);
          data += data_offs;
          length -= data_offs;
          if (length <= 0)
            return;
        }
        first = 0;
      }

      // compute number of samples to put into this buffer
      int count = bufsize - offset;
      if (count > length)
        count = length;

      // update length and copy data
      length -= count;
      b += offset;
      while (count--)
        *b++ += *data++;
    }



    void AddListen(short *b, long fr_smpl, long to_smpl, unsigned int bufsize) {

      // everything done?
      if (length <= 0)
        return;

      if (first) {

        if (to_smpl > 0 && to_smpl < length)
          length = to_smpl;

        if (fr_smpl > 0 && fr_smpl < length)
        {
          data += fr_smpl;
          length -= fr_smpl;
        }
        first = FALSE;
        if (length <= 0)
        {
          return;
        }
      }

      int count = bufsize;
      if (count > length)
        count = length;

      // update length and copy data
      length -= count;
      while (count--)
      {
        *b++ += *data++;
      }
    }


    int Finished() {
      return length <= 0;
    }

  private:
    tSampleSet &set;
    long       clock;
    tSample    *spl;
    short      *data;
    int        first;
    long       length;
    short      prev;
};

// ------------------------------------------------------------

tSampleSet::tSampleSet(long tpm)
  : mDefaultFileName("noname.spl"),
    mRecordFileName("noname.wav")
{
  int i;

  ticks_per_minute = tpm;

  for (i = 0; i < BUFCOUNT; i++)
    buffers[i] = new tAudioBuffer(0);

  adjust_audio_length = 1;
  has_changed  = false;
  spl_dialog   = 0;
  glb_dialog   = 0;
  is_playing   = 0;
  softsync     = 1;
  dirty        = 0;

  for (i = 0; i < MAXSMPL; i++) {
    samples[i] = new tSample(*this);
    samplewin[i] = 0;
  }
  speed    = 22050;
  channels = 1;
  bits     = 16;        // dont change!!

  for (i = 0; i < MAXPOLY; i++)
    voices[i] = new tSampleVoice(*this);
  num_voices = 0;

}


tSampleSet::~tSampleSet()
{
  int i;
  for (i = 0; i < MAXSMPL; i++) {
    delete samples[i];
    delete samplewin[i];
  }
  for (i = 0; i < MAXPOLY; i++)
    delete voices[i];
  for (i = 0; i < BUFCOUNT; i++)
    delete buffers[i];
}


void tSampleSet::Edit(int key)
{
  if (samplewin[key] == 0)
  {
    tSample* spl = samples[key];

    samplewin[key] = new tSampleWin(gpTrackWindow, &samplewin[key], *spl);
  }
  samplewin[key]->Show(TRUE);
  samplewin[key]->Redraw();
}


void tSampleSet::LoadDefaultSettings()
{
  wxString FileName = FindFile("jazz.spl");
  if (!FileName.empty())
  {
    Load(FileName);
  }
}

int tSampleSet::Load(const wxString& FileName)
{
  int version;

  // enable audio when loading a sample set
  gpMidiPlayer->SetAudioEnabled(TRUE);

  wxBeginBusyCursor();
  for (int i = 0; i < MAXSMPL; i++)
  {
    samples[i]->Clear();
  }

  // path of the spl file
  wxString SplFilePath = ::wxPathOnly(FileName);

  ifstream is(FileName.c_str());
  is >> version >> speed >> channels >> softsync;
  while (is)
  {
    int key, pan, vol, pitch;
    char fname[128];
    fname[0] = 0;
    char label[500];
    is >> key;
    ReadString(is, fname, sizeof(fname));
    ReadString(is, label, sizeof(label));
    is >> pan >> vol >> pitch;
    if (fname[0])
    {
      if (!wxFileExists(fname))
      {
        // try to prepend the SplFilePath
        char tmp[128];
        strcpy(tmp, SplFilePath.c_str());
        strcat(tmp, "/");
        strcat(tmp, fname);
        strcpy(fname, tmp);
      }
      if (!wxFileExists(fname))
      {
        char buf[500];
        sprintf(buf, "File not found: \"%s\"", fname);
        wxMessageBox(buf, "Error", wxOK);
        continue;
      }
      assert(0 <= key && key < MAXSMPL);
      samples[key]->SetFilename(fname);
      samples[key]->SetLabel(label);
      samples[key]->SetVolume(vol);
      samples[key]->SetPan(pan);
      samples[key]->SetPitch(pitch);
      if (samples[key]->Load())
      {
        char buf[500];
        sprintf(buf, "could not load \"%s\"", samples[key]->GetFilename());
        wxMessageBox(buf, "Error", wxOK);
      }
      if (samplewin[key])
      {
        samplewin[key]->Redraw();
      }
    }
    else
    {
      break;
    }
  }
  wxEndBusyCursor();
  dirty = 0;
  return 0;
}

void tSampleSet::ReloadSamples()
{
  for (int i = 0; i < MAXSMPL; i++)
    samples[i]->Load(dirty);
  dirty = 0;
}


int tSampleSet::Save(const wxString& FileName)
{
  ofstream os(FileName.c_str());
  os << 1 << " " << speed << " " << channels << " " << softsync << endl;
  for (int i = 0; i < MAXSMPL; i++) {
    tSample *spl = samples[i];
    const char *fname = spl->GetFilename();
    const char *label = spl->GetLabel();
    int vol = spl->GetVolume();
    int pan = spl->GetPan();
    int pitch = spl->GetPitch();
    if (fname[0])
    {
      os << i << " ";
      WriteString(os, fname);
      os << " ";
      WriteString(os, label);
      os << " " << pan << " " << vol << " " << pitch << endl;
    }
  }
  return 0;
}


const char *tSampleSet::GetSampleName(int i) {
  if (0 <= i && i < MAXSMPL)
    return samples[i]->GetLabel();
  return "";
}


int tSampleSet::ResetBuffers(tEventArray *evnt_arr, long clock, long tpm)
{
  int i;
  free_buffers.Clear();
  full_buffers.Clear();
  driv_buffers.Clear();
  for (i = 0; i < BUFCOUNT; i++)
    free_buffers.Put(buffers[i]);
  buffers_written   = 0;

  events            = evnt_arr;
  start_clock       = clock;
  ticks_per_minute  = tpm;
  event_index       = 0;
  bufshorts = BUFSHORTS;
  clocks_per_buffer = Samples2Ticks(bufshorts);
  num_voices        = 0;
  return 0;
}


int tSampleSet::ResetBufferSize(unsigned int bufsize)
{
  if (bufsize == 0 || bufsize > BUFBYTES || (bufsize & 1)) {
    cerr << "invalid buffer size " << bufsize << '\n';
    return 1;
  }
  bufshorts = bufsize / 2;
  clocks_per_buffer = Samples2Ticks(bufshorts);
  return 0;
}


int tSampleSet::FillBuffers(long last_clock)
{
  // check if last_clock is bigger than free buffer space
  // and compute the count of buffers that can be filled
  int i;

  int nfree = free_buffers.Count();
  if (nfree <= 0)
  {
    return 0;
  }

  long max_buffer_clock = BufferClock(buffers_written + nfree);

  if (max_buffer_clock <= last_clock)
  {
    last_clock = max_buffer_clock;
  }
  else
  {
    nfree = (int)((last_clock - start_clock) / clocks_per_buffer) - buffers_written;
  }

  if (nfree <= 0)
  {
    return 0;
  }


  // iterate the events and add sounding voices
  while (event_index < events->nEvents)
  {
    JZEvent *e = events->Events[event_index];
    if (e->GetClock() >= last_clock)
    {
      break;
    }
    event_index++;

    tKeyOn *k = e->IsKeyOn();
    if (k && num_voices < MAXPOLY)
    {
      voices[num_voices++]->Start(samples[k->Key], k->GetClock());
    }
  }

  // add remaining sample data to the buffers
  for (i = 0; i < nfree; i++)
  {
    tAudioBuffer *buf = free_buffers.Get();
    buf->Clear();
    long buffer_clock = BufferClock(buffers_written + i);

//    cout
//      << "write " << (buffers_written + i) % buffer_count
//      << ", clock " << buffer_clock
//      << endl;

    for (int k = 0; k < num_voices; k++)
    {
      voices[k]->AddBuffer(buf->data, buffer_clock, bufshorts);
    }
    full_buffers.Put(buf);
  }

  // delete finished voices
  for (i = 0; i < num_voices; i++)
  {
    if (voices[i]->Finished())
    {
      tSampleVoice *v = voices[i];
      voices[i] = voices[num_voices-1];
      voices[num_voices-1] = v;
      num_voices--;
    }
  }

  // all buffer filled up?
  buffers_written += nfree;

  return nfree;
}


// returns the number of buffers containing sound. Fills as many
// buffers as possible, the last buffers may contain silence only.
int tSampleSet::PrepareListen(tSample *spl, long fr_smpl, long to_smpl)
{
  listen_sample = spl;

  assert(ticks_per_minute);
  ResetBuffers(0, 0, ticks_per_minute);
  voices[0]->Start(spl, 0);
  int nfree = free_buffers.Count();
  int sound_buffers = 0;

  for (int i = 0; i < nfree; i++) {
    tAudioBuffer *buf = free_buffers.Get();
    buf->Clear();
    if (!voices[0]->Finished()) {
      voices[0]->AddListen(buf->Data(), fr_smpl, to_smpl, bufshorts);
      sound_buffers++;
    }
    full_buffers.Put(buf);
  }
  buffers_written = nfree;
  return sound_buffers;
}


int tSampleSet::PrepareListen(int key, long fr_smpl, long to_smpl)
{
  tSample *spl = samples[key];
  return PrepareListen(spl, fr_smpl, to_smpl);
}


int tSampleSet::ContinueListen()
{
  int nfree = free_buffers.Count();
  int sound_buffers = 0;

  for (int i = 0; i < nfree; i++)
  {
    tAudioBuffer *buf = free_buffers.Get();
    buf->Clear();
    if (!voices[0]->Finished()) {
      voices[0]->AddListen(buf->Data(), -1, -1, bufshorts);
      sound_buffers++;
    }
    full_buffers.Put(buf);
  }
  buffers_written += nfree;
  return sound_buffers;
}



void tSampleSet::AdjustAudioLength(JZTrack *t, long tpm)
{
  if (!t->GetAudioMode() || !adjust_audio_length)
    return;

  ticks_per_minute = tpm;

  tEventIterator it(t);
  JZEvent *e = it.First();
  while (e) {
    tKeyOn *k = e->IsKeyOn();
    if (k) {
      k->Length = (int)Samples2Ticks(samples[k->Key]->GetLength());
      if (k->Length < 15)  // invisble?
        k->Length = 15;
    }
    e = it.Next();
  }
}



void tSampleSet::StartPlay(long clock)
{
  ReloadSamples();

  // touch all playback sample data, so they may get swapped into memory
  for (int i = 0; i < MAXSMPL; i++) {
    tSample *spl = samples[i];
    spl->GotoRAM();
  }

  is_playing = 1;
}


void tSampleSet::StopPlay()
{
  is_playing = 0;
}

// ******************************************************************
//                                GUI
// ******************************************************************

class tSamplesDlg : public wxDialog
{
  friend class tSampleSet;
  public:
    tSamplesDlg(wxWindow* pParent, tSampleSet &set);
    ~tSamplesDlg();
#ifdef OBSOLETE
    static void CloseButton(wxItem &item, wxCommandEvent& event);
    static void PlayButton(wxItem &item, wxCommandEvent& event);
    static void EditButton(wxItem &item, wxCommandEvent& event);
    static void AddButton(wxItem &item, wxCommandEvent& event);
    static void ClrButton(wxItem &item, wxCommandEvent& event);
    static void HelpButton(wxItem &item, wxCommandEvent& event);
    static void ListClick(wxItem &item, wxCommandEvent& event);
#endif // OBSOLETE
    void OnCloseButton();
    void OnPlayButton();
    void OnEditButton();
    void OnAddButton();
    void OnClrButton();
    void OnHelpButton();
    void OnListClick();

  private:
    tSampleSet &set;
    char **names;

    wxListBox *list;
    wxSlider  *pan;
    wxSlider  *vol;
    wxSlider  *pitch;
#ifdef OBSOLETE
    wxText    *label;
    wxText    *file;
#endif // OBSOLETE

    static char *path;
    static int  current;

    char *ListEntry(int i);
    void Sample2Win(int index);
    void Win2Sample(int index);
    void SetCurrentListEntry(int i);
};

// -----------------------------------------------------------------
// ------------------------ global settings ------------------------
// -----------------------------------------------------------------
#ifdef OBSOLETE

class tAudioGloblForm : public wxForm
{
  public:
    tAudioGloblForm(tSampleSet &s)
    : wxForm( USED_WXFORM_BUTTONS ),
      set(s)
    {

      ossbug1      = gpConfig->GetValue(C_OssBug1);
      ossbug2      = gpConfig->GetValue(C_OssBug2);
      duplex_audio = gpConfig->GetValue(C_DuplexAudio);

      static const char *speedtxt[] = { "8000", "11025", "22050", "44100", 0 };
      speed    = set.GetSpeed();
      speedstr = 0;
      for (int i = 0; speedtxt[i]; i++)
      {
        strlist.Append((wxObject *)speedtxt[i]);  // ???
        if (atol(speedtxt[i]) == speed)
        {
          speedstr = copystring(speedtxt[i]);
        }
      }
      if (!speedstr)
      {
        speedstr = copystring(speedtxt[0]);
      }

      enable = gpMidiPlayer->GetAudioEnabled();
      stereo = (set.GetChannels() == 2);
      softsync = set.GetSoftSync();

      Add(wxMakeFormBool("Enable Audio", &enable));
      Add(wxMakeFormNewLine());
      //Add(wxMakeFormString("Sample Freq", (char **)&speedstr, wxFORM_CHOICE,
      Add(wxMakeFormString("Sample Freq", (char **)&speedstr, wxFORM_DEFAULT,
          new wxList(wxMakeConstraintStrings(&strlist), 0), NULL, wxHORIZONTAL));
      Add(wxMakeFormNewLine());
      Add(wxMakeFormBool("Stereo", &stereo));
      Add(wxMakeFormNewLine());
      Add(wxMakeFormBool("Software Midi/Audio Sync", &softsync));

      #ifdef wx_x
      Add(wxMakeFormNewLine());
      Add(wxMakeFormBool("OSS 'start audio' bug workaround", &ossbug1));
      //Add(wxMakeFormNewLine());
      //Add(wxMakeFormBool("OSS Bug workaround (2)", &ossbug2));
      #endif

      Add(wxMakeFormNewLine());
      Add(wxMakeFormBool("Duplex Play/Record", &duplex_audio));
    }

    void OnHelp()
    {
      gpHelpInstance->ShowTopic("Global Settings");
    }

    void OnOk()
    {
      if (set.is_playing)
        return;
      wxBeginBusyCursor();
      set.glb_dialog = 0;
      speed = atol(speedstr);
      set.SetSpeed(speed);
      set.SetChannels(stereo ? 2 : 1);
      set.SetSoftSync(softsync);
      gpMidiPlayer->SetAudioEnabled(enable);

      if (gpConfig->GetValue(C_EnableAudio) != enable)
      {
//        Config(C_EnableAudio) = enable;
        Config.Put(C_EnableAudio, enable);
      }

      if (gpConfig->GetValue(C_OssBug1) != ossbug1)
      {
//        Config(C_OssBug1) = ossbug1;
        Config.Put(C_OssBug1, ossbug1);
      }

      if (gpConfig->GetValue(C_OssBug2) != ossbug2)
      {
//        Config(C_OssBug2) = ossbug2;
        Config.Put(C_OssBug2, ossbug2);
      }

      if (gpConfig->GetValue(C_DuplexAudio) != duplex_audio)
      {
        Config(C_DuplexAudio) = duplex_audio;
        Config.Put(C_DuplexAudio, duplex_audio);
      }

      if (enable)
        set.ReloadSamples();
      wxEndBusyCursor();
      wxForm::OnOk();
    }
    void OnCancel() {
      set.glb_dialog = 0;
      wxForm::OnCancel();
    }
  private:
    tSampleSet &set;
    wxList  strlist;

    long speed;
    const char *speedstr;
    bool enable;
    bool stereo;
    bool softsync;
    bool ossbug1;
    bool ossbug2;
    bool duplex_audio;
};

#endif // OBSOLETE

void tSampleSet::GlobalSettingsDlg()
{
  if (spl_dialog) {
    spl_dialog->Show(TRUE);
    return;
  }

  if (glb_dialog == 0)
  {
#ifdef OBSOLETE
    glb_dialog = new wxDialogBox(gpTrackWindow, "Audio Settings", FALSE );
    tAudioGloblForm *form  = new tAudioGloblForm(*this);
    form->AssociatePanel(glb_dialog);
    glb_dialog->Fit();
#endif // OBSOLETE
  }
  glb_dialog->Show(TRUE);
}

void tSampleSet::SaveRecordingDlg(long frc, long toc, tAudioRecordBuffer &buf)
{
  if (frc >= toc)
  {
    return;
  }

  if (buf.num_buffers == 0)
  {
    return;
  }

  int Choice = ::wxMessageBox(
    "Save audio recording ?",
    "Save?",
    wxOK | wxCANCEL);

  if (Choice == wxOK)
  {
    wxString FileName = file_selector(
      mRecordFileName,
      "Save Audio Recording",
      true,
      false,
      "*.wav");
    if (!FileName.empty())
    {
      wxBeginBusyCursor();
      SaveWave(FileName, frc, toc, buf);
      AddNote(FileName, frc, toc);
      wxEndBusyCursor();
    }
  }
}



void tSampleSet::AddNote(const char *fname, long frc, long toc)
{
  int i;
  tSample *spl;

  // see if fname is already present in sample list
  for (i = 0; i < MAXSMPL; i++) {
    spl = samples[i];
    if (strcmp(spl->GetFilename(), fname) == 0)
      break;
  }

  // if no entry is there, add an entry
  if (i >= MAXSMPL) {
    // start somewhere near the top of the list
    for (i = 15; i < MAXSMPL; i++) {
      spl = samples[i];
      if (spl->GetFilename()[0] == 0)
        break;
    }
  }

  if (i >= MAXSMPL)
    return;

  int key = i;
  spl->Clear();   // reset everything to defaults
  spl->SetFilename(fname);
  spl->SetLabel(wxFileNameFromPath((char *)fname));
  spl->Load();   // reload data

  // delete selection
#ifdef OBSOLETE
  JZSong       *song = gpProject->Song;
#endif
  const JZRecordingInfo* info = gpProject->GetRecInfo();
  JZTrack* track = info->mpTrack;
#ifdef OBSOLETE
  song->NewUndoBuffer();
#endif
  tEventIterator iter(info->mpTrack);
  JZEvent *e = iter.Range(frc, toc);
  while (e != 0) {
    track->Kill(e);
    e = iter.Next();
  }
  // add a noteon
  tKeyOn *k = new tKeyOn(
    frc,
    track->Channel - 1,
    key,
    64,
    (unsigned short)(toc - frc));
  track->Put(k);
  track->Cleanup();

  // repaint trackwin
//  gpTrackWindow->Redraw();
  gpTrackWindow->Update();
}



void tSampleSet::SaveWave(const char *fname, long frc, long toc, tAudioRecordBuffer &buf)
{
  WaveHeader wh;
  wh.main_chunk = RIFF;
  wh.chunk_type = WAVE;
  wh.sub_chunk  = FMT;
  wh.data_chunk = DATA;
  wh.format     = PCM_CODE;
  wh.modus      = channels;
  wh.sc_len     = 16;
  wh.sample_fq  = speed;
  wh.bit_p_spl  = bits;
  wh.byte_p_spl = channels * (bits > 8 ? 2 : 1);
  wh.byte_p_sec = wh.byte_p_spl * wh.sample_fq;


  long start_index = Ticks2Samples(frc - start_clock);
  long end_index   = Ticks2Samples(toc - start_clock);

  int bufsize = buf.bufbytes / 2;

  // recording aborted?
  if (end_index > buf.num_buffers * bufsize)
  {
    end_index = buf.num_buffers * bufsize;
  }

  wh.data_length   = (end_index - start_index) * sizeof(short);
  wh.length        = wh.data_length + sizeof(WaveHeader);

  ofstream os(fname, ios::out | ios::binary | ios::trunc);

  os.write((char *)&wh, sizeof(wh));

  int start_buffer = start_index / bufsize;
  int start_offs   = start_index % bufsize;
  int start_length = bufsize - start_offs;
  int end_buffer   = end_index / bufsize;
  int end_length   = end_index % bufsize;

  // save part of first buffer
  os.write((char *)&buf.buffers[start_buffer]->data[start_offs], 2 * start_length);
  // write some complete buffers
  for (int i = start_buffer + 1; i < end_buffer; i++)
    os.write((char *)buf.buffers[i]->data, bufsize * 2);
  // save part of last buffer
  if (end_length > 0)
    os.write((char *)buf.buffers[end_buffer]->data, 2 * end_length);
#if 0
  // very slow, but works!
  ofstream slow("t2.wav", ios::out | ios::bin | ios::trunc);
  slow.write((char *)&wh, sizeof(wh));
  for (long i = start_index; i < end_index; i++) {
    int bi = i / bufsize;
    int di = i % bufsize;
    slow.write((char *)&buf.buffers[bi]->data[di], sizeof(short));
  }
#endif
}

// -----------------------------------------------------------------
// ------------------------------- record  ------------------------
// -----------------------------------------------------------------

DEFINE_ARRAY(tAudioBufferArray, tAudioBuffer *)

void tAudioRecordBuffer::Clear()
{
  int n = buffers.GetSize();
  for (int i = 0; i < n; i++) {
    delete buffers[i];
    buffers[i] = 0;
  }
  num_buffers = 0;
}

tAudioBuffer * tAudioRecordBuffer::RequestBuffer() {
  if (buffers[num_buffers] == 0)
    buffers[num_buffers] = new tAudioBuffer(0);
  if (buffers[num_buffers] == 0) {
    Clear();
    fprintf(stderr, "memory exhausted!\n");
  }
  return buffers[num_buffers++];
}

// -----------------------------------------------------------------
// ------------------------ sample settings ------------------------
// -----------------------------------------------------------------


char * tSamplesDlg::path = 0;
int    tSamplesDlg::current = 0;

tSamplesDlg::tSamplesDlg(wxWindow* pParent, tSampleSet &s)
  : wxDialog(pParent, wxID_ANY, "Sample Settings"),
    set(s)
{
  if (path == 0)
    path = copystring("*.wav");

  names = new char * [tSampleSet::MAXSMPL];
  for (int i = 0; i < tSampleSet::MAXSMPL; i++)
    names[i] = ListEntry(i);

  // buttons
#ifdef OBSOLETE
  new wxButton(this, (wxFunction)CloseButton, "Close") ;
  new wxButton(this, (wxFunction)PlayButton,  "Play") ;
  new wxButton(this, (wxFunction)EditButton,  "Edit") ;
  new wxButton(this, (wxFunction)AddButton,   "Add") ;
  new wxButton(this, (wxFunction)ClrButton,   "Clear") ;
  new wxButton(this, (wxFunction)HelpButton,  "Help") ;
  NewLine();
#endif

#ifdef OBSOLETE
  // list box
  int y = 80;
  SetLabelPosition(wxVERTICAL);
  list = new wxListBox(this, (wxFunction)ListClick, "Samples", wxLB_SINGLE, 10, y, 300, 200, tSampleSet::MAXSMPL, names, wxNEEDED_SB);

  NewLine();
  SetLabelPosition(wxHORIZONTAL);

  // Sliders
  vol = new wxSlider(this, (wxFunction)0, " ", 64,  0, 127,    200);
  (void) new wxMessage(this, "Volume");
  NewLine();
  pan = new wxSlider(this, (wxFunction)0, " ",  0, -63, 63,    200);
  (void) new wxMessage(this, "Panpot");
  NewLine();
  pitch = new wxSlider(this, (wxFunction)0, " ",  0, -12, 12,    200);
  (void) new wxMessage(this, "Pitch");
  NewLine();
  label = new wxText(this, (wxFunction)0, "Label", "", -1, -1, 300);
  NewLine();
  file = new wxText(this, (wxFunction)0, "File", "", -1, -1, 300);
  NewLine();
#endif // OBSOLETE
  Fit();
  Sample2Win(current);
  list->SetSelection(current, TRUE);
  Show(TRUE);
}

char *tSamplesDlg::ListEntry(int i)
{
  char buf[500];
  sprintf(buf, "%d ", i+1);
  //Key2Str(i, buf + strlen(buf));
  sprintf(buf + strlen(buf), set.samples[i]->GetLabel());
  return copystring(buf);
}

void tSamplesDlg::Sample2Win(int i)
{
  tSample *spl = set.samples[i];
  vol->SetValue(spl->GetVolume());
  pitch->SetValue(spl->GetPitch());
  pan->SetValue(spl->GetPan());
#ifdef OBSOLETE
  label->SetValue((char *)spl->GetLabel());
  file->SetValue((char *)spl->GetFilename());
#endif
}

void tSamplesDlg::Win2Sample(int i)
{
  tSample *spl = set.samples[i];
  spl->SetPitch(pitch->GetValue());
  spl->SetVolume(vol->GetValue());
  spl->SetPan(pan->GetValue());
#ifdef OBSOLETE
  spl->SetLabel(label->GetValue());
  spl->SetFilename(file->GetValue());
#endif
}

void tSamplesDlg::SetCurrentListEntry(int i)
{
  if (i >= 0)
  {
    current = i;
    list->SetString(current, ListEntry(current));
    list->SetSelection(current, TRUE);
  }
}

tSamplesDlg::~tSamplesDlg()
{
  for (int i = 0; i < tSampleSet::MAXSMPL; i++)
    delete [] names[i];
  delete [] names;
}

void tSamplesDlg::OnCloseButton()
{
  if (set.is_playing)
    return;
  Win2Sample(current);
  wxBeginBusyCursor();
  set.ReloadSamples();
  wxEndBusyCursor();
  set.spl_dialog = 0;
//  DELETE_THIS();
  Destroy();
}

void tSamplesDlg::OnAddButton()
{
  wxString fname = file_selector(path, "Load Sample", false, false, "*.wav");
  if (fname)
  {
#ifdef OBSOLETE
    file->SetValue(fname);
    label->SetValue(wxFileNameFromPath(fname));
#endif
    Win2Sample(current);
    SetCurrentListEntry(current);
  }
}

void tSamplesDlg::OnEditButton()
{
  wxBeginBusyCursor();
  Win2Sample(current);
  SetCurrentListEntry(current);
  tSample *spl = set.samples[current];
  spl->Load();
  wxEndBusyCursor();

  set.Edit(current);
}

void tSamplesDlg::OnPlayButton()
{
  if (set.is_playing)
    return;
  if (gpMidiPlayer->IsListening())
  {
    gpMidiPlayer->ListenAudio(-1);
    return;
  }
  Win2Sample(current);
  SetCurrentListEntry(current);
  tSample *spl = set.samples[current];
  wxBeginBusyCursor();
  spl->Load();
  gpMidiPlayer->ListenAudio(current);
  wxEndBusyCursor();
}

void tSamplesDlg::OnClrButton()
{
  tSample *spl = set.samples[current];
  spl->Clear();
  SetCurrentListEntry(current);
  Sample2Win(current);
}

void tSamplesDlg::OnHelpButton()
{
  gpHelpInstance->ShowTopic("Sample Settings");
}

void tSamplesDlg::OnListClick()
{
  Win2Sample(current);
  int i = list->GetSelection();
  if (i >= 0) {
    current = i;
    SetCurrentListEntry(i);
    Sample2Win(current);
  }
}

#ifdef OBSOLETE

void tSamplesDlg::CloseButton(wxItem &itm, wxCommandEvent& event)
{
  ((tSamplesDlg *)itm.GetParent())->OnCloseButton();
}
void tSamplesDlg::PlayButton(wxItem &itm, wxCommandEvent& event)
{
  ((tSamplesDlg *)itm.GetParent())->OnPlayButton();
}
void tSamplesDlg::EditButton(wxItem &itm, wxCommandEvent& event)
{
  ((tSamplesDlg *)itm.GetParent())->OnEditButton();
}
void tSamplesDlg::AddButton(wxItem &itm, wxCommandEvent& event)
{
  ((tSamplesDlg *)itm.GetParent())->OnAddButton();
}
void tSamplesDlg::ClrButton(wxItem &itm, wxCommandEvent& event)
{
  ((tSamplesDlg *)itm.GetParent())->OnClrButton();
}
void tSamplesDlg::HelpButton(wxItem &itm, wxCommandEvent& event)
{
  ((tSamplesDlg *)itm.GetParent())->OnHelpButton();
}
void tSamplesDlg::ListClick(wxItem &itm, wxCommandEvent& event)
{
  ((tSamplesDlg *)itm.GetParent())->OnListClick();
}
#endif // OBSOLETE

void tSampleSet::SamplesDlg()
{
  if (glb_dialog) {
    glb_dialog->Show(TRUE);
    return;
  }
  if (spl_dialog == 0)
    spl_dialog = new tSamplesDlg(gpTrackWindow, *this);
  spl_dialog->Show(TRUE);
}

void tSampleSet::RefreshDialogs()
{
  if (spl_dialog)
    spl_dialog->Sample2Win(spl_dialog->current);
}

// -----------------------------------------------------------------
// -------------------------------- menu ---------------------------
// -----------------------------------------------------------------


int tSampleSet::OnMenuCommand(int id)
{
  switch (id)
  {
    case MEN_AUDIO_LOAD:
      {
        wxString fname = file_selector(
          mDefaultFileName,
          "Load Sample Set",
          false,
          has_changed,
          "*.spl");
        if (fname)
        {
          Load(fname);
        }
        return 1;
      }

    case MEN_AUDIO_SAVE_AS:
      {
        wxString fname = file_selector(
          mDefaultFileName,
          "Save Sample Set",
          true,
          has_changed,
          "*.spl");
        if (fname)
        {
          Save(fname);
        }
        return 1;
      }

    case MEN_AUDIO_SAVE:
      {
        if (mDefaultFileName == "noname.spl")
        {
          return OnMenuCommand(MEN_AUDIO_SAVE_AS);
        }
        Save(mDefaultFileName);
        return 1;
      }

    case MEN_AUDIO_GLOBAL:
      GlobalSettingsDlg();
      break;

    case MEN_AUDIO_SAMPLES:
      SamplesDlg();
      return 1;

    case MEN_AUDIO_NEW:
      if (spl_dialog == 0 && glb_dialog == 0)
      {
        if (wxMessageBox("Clear Sample Set?", "Confirm", wxYES_NO) == wxNO)
        {
          return 1;
        }
        for (int i = 0; i < MAXSMPL; i++)
        {
          samples[i]->Clear();
        }
      }
      return 1;

  }
  return 0;
}



