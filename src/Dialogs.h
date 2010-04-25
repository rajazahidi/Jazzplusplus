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

#ifndef JZ_DIALOGS_H
#define JZ_DIALOGS_H

#include "CommandUtilities.h"
#include "PropertyListDialog.h"

class JZPianoWindow;
class JZFilter;
class JZSong;
class JZEventFrame;
class JZEventWindow;
class JZTrack;
class JZEvent;

//class tShiftDlg : public tPropertyListDlg
//{
//  public:
//
//    long mSteps;        // 0 was static
//    long mUnit;
//
//    JZFilter* mpFilter;
//    JZSong* mpSong;
//
//    tShiftDlg(JZEventFrame* pEventWindow, JZFilter* pFilter, long Unit);
//    void AddProperties();
//    bool OnClose();
//    void OnHelp();
//};

//class tSearchReplaceDlg : public tPropertyListDlg
//{
//  public:
//
//    static int frCtrl;
//    static int toCtrl;
///*     tNamedChoice frList; */
///*     tNamedChoice toList; */
//
//    JZFilter *Filter;
//    JZSong   *Song;
//
//    tSearchReplaceDlg(JZEventWindow* w, JZFilter *f);
//    void AddProperties();
//    bool OnClose();
//    void OnHelp();
//};

// seqLength
class tSeqLengthDlg : public tPropertyListDlg
{
  public:


    static double scale;

    JZFilter   *Filter;
    JZSong     *Song;

    tSeqLengthDlg(JZEventFrame *win, JZFilter *f);
    void AddProperties();
    bool OnClose();
    void OnHelp();
};

// midiDelay
class tMidiDelayDlg : public tPropertyListDlg
{
  public:

    static double scale;
    static long clockDelay;
    static int repeat;

    JZFilter   *Filter;
    JZSong     *Song;

    tMidiDelayDlg(JZEventFrame *win, JZFilter *f);
    void AddProperties();
    bool OnClose();
    void OnHelp();
};

void EventDialog(
  JZEvent*,
  JZPianoWindow*,
  JZTrack*,
  long Clock,
  int Channel,
  int Pitch);

//*****************************************************************************
// MeterChange Dialog
//*****************************************************************************
class tMeterChangeDlg : public tPropertyListDlg
{
  public:

    tMeterChangeDlg(JZEventWindow* pEventWindow);

    void AddProperties();

    virtual bool OnClose();
    virtual void OnCancel();
    virtual void OnHelp();

    JZEventWindow* mpEventWindow;
    static int Numerator;
    static int Denomiator;
    static int BarNr;
};

#endif // !defined(JZ_DIALOGS_H)
