/*
**  JazzPlusPlus Midi Sequencer
**
** Some Code Copyright (C) 1994-2000 Andreas Voss and Per Sigmond, all rights reserved.
**    I don't know why it says "All Rights Reserved" and then is licensed GPL
**    But the GPL says I can't change the copyright notice.
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

class tPlayer;
class tSong;
class tSynth;
class tFilter;

// needed until tRecordInfo gets moved to its own file
#include "../trackwin.h"

#ifndef JPPPROJECT
#define JPPPROJECT

class jppProject {
    public:
        // These should probably become private members at some point.  I'm not sure about that
        // though.  They are currently used as a holding pen for loose global variables while
        // the globals are brought under control, again.
        tSong* Song;
        tPlayer* Midi;
        tSynth *Synth;

        jppProject();

        // restart play here if space bar hit
        long mStartTime;
        long mStopTime;
        bool mLoop;
        bool mMuted;
        bool mRecord;

        tFilter *Filter;
        tMetronomeInfo mMetronomeInfo;

        tMetronomeInfo GetMetronome();

        /// Number of bars
        int mNumBars;

        bool HasChanged();

        wxString mSongFileName;
        wxString mPatternFileName;

        // These provide access to the Project

        /// Sets the song name
        void SetSong(wxString newsong);
        void SetPattern(wxString newpattern);

        /// Open the Song
        void OpenSong(wxString newsong);
        void Save();

        // Here is the new play interface.  For now it just acts as a layer between the Project
        // and the GUI.
        bool IsPlaying();
        void Play();
        void Stop();
        void SetPlayPosition(long newposition);
        void Mute(bool newmute);
        void SetLoop(bool newloop);
        void SetRecord(bool newrecord);
        void SetLoopClock(long newclock);

        tRecordInfo GetRecInfo();
    private:
        bool mChanged;
        bool mIsPlaying;
        tRecordInfo mRecInfo;
};

#endif // JPPPROJECT

