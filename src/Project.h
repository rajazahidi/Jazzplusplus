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

#ifndef JZ_PROJECT_H
#define JZ_PROJECT_H

class tPlayer;
class JZSynth;
class tFilter;

// needed until tRecordInfo gets moved to its own file
//#include "TrackFrame.h"
#include "Song.h"
#include "Metronome.h"

class tRecordInfo
{
public:
  tTrack *Track;	// 0 == not recording

  long   FromClock;     // recording from clock
  long   ToClock;	// recording to clock
  long   TrackNr;	// recording on this track
  int    Muted;		// recording track is muted
};

//*****************************************************************************
// Description:
//   This is the Jazz++ project class declaration.  This class is derived from
// JZSong solely as a shortcut to having a track interface in the project
// object.  As the backend storage stuff gets rewritten, we'll be overloading
// the JZSong members accordingly, until JZSong itself will be gone.  JZSong is
// deprecated right now, and anything that still uses it needs to switch to
// using jppProject instead through its global instance.
//*****************************************************************************
class jppProject : public JZSong
{
  public:

    // These should probably become private members at some point.  I'm not
    // sure about that though.  They are currently used as a holding pen for
    // loose global variables while the globals are brought under control,
    // again.
    tPlayer* Midi;
    JZSynth *Synth;

    jppProject();

    ~jppProject();

    // restart play here if space bar hit
    long mStartTime;

    // Not yet sure what this does
    long mStopTime;

    // Loop flag, loops play if true
    bool mLoop;

    // If true, mutes output
    bool mMuted;

    // If true, records from midi in
    bool mRecord;

    // Not yet sure what this does
    tFilter *Filter;

    // Stores metrome information
    tMetronomeInfo mMetronomeInfo;

    // Returns the internal pointer to the metronome
    tMetronomeInfo GetMetronome();

    // Number of bars
    int mNumBars;

    // Returns whether or not the project has changed since last save
    bool HasChanged();

    // Name of the song file currently loaded
    wxString mSongFileName;

    // Name of the pattern file currently loaded
    wxString mPatternFileName;

    // These provide access to the Project

    // Sets the song name
    void SetSong(wxString newsong);

    // Sets the pattern name
    void SetPattern(wxString newpattern);

    // Open the Song
    void OpenSong(wxString newsong);

    // Save the song
    void Save(wxString newsong);

    // Here is the new play interface.  For now it just acts as a layer
    // between the Project and the GUI.
    // Returns true during playback
    bool IsPlaying();

    // Starts playback
    void Play();

    // Stops playback
    void Stop();

    // Sets the playback cursor to a specific position
    void SetPlayPosition(long newposition);

    // Mutes playback
    void Mute(bool newmute);

    // Set loop, true to loop, false not to loop
    void SetLoop(bool newloop);

    // Set record, true to record, false not to record
    void SetRecord(bool newrecord);

    // Beats me what this does.
    void SetLoopClock(long newclock);

    // Sets selection
//    void SetSelection();

    // Beats me what this does.
    tRecordInfo *GetRecInfo();

    // Sets RecInfo, jppProject takes ownership of this object
    void SetRecInfo(tRecordInfo* newRecInfo);

  private:

    bool mChanged;

    bool mIsPlaying;

    tRecordInfo* mRecInfo;
};

#endif // !defined(JZ_PROJECT_H)
