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

#ifndef JZ_HARMONY_H
#define JZ_HARMONY_H

#include "ToolBar.h"

class wxObject;
class HBAnalyzer;
class HBCanvas;
class tGenMelody;


class tHBInterface
{
  public:
    virtual ~tHBInterface()        {}
    virtual int SeqDefined() = 0;        // true = yes

    // return number of keys in out
    virtual int GetChordKeys(int *out, int step, int n_steps) = 0;
    virtual int GetBassKeys(int *out, int step, int n_steps) = 0;
    virtual int GetSelectedChord(int *out) = 0; // returns # keys
    virtual int GetSelectedScale(int *out) = 0; // returns # keys
    virtual HBAnalyzer * getAnalyzer() = 0;
    virtual void TransposeSelection() = 0;
};

class HBFrame : public wxFrame, public tHBInterface
{
  public:
    HBFrame(wxFrame *parent);
    ~HBFrame();
    int SeqDefined();
    int GetChordKeys(int *out, int step, int n_steps);
    int GetSelectedChord(int *out);
    int GetSelectedScale(int *out);
    int GetBassKeys(int *out, int step, int n_steps);
    virtual void OnSize(wxSizeEvent& Event);
    HBAnalyzer * getAnalyzer();
    virtual bool OnClose();
    void TransposeSelection();
  protected:
    virtual void OnMenuCommand(int id);
  private:
    HBCanvas* cnvs;
    int SeqSelected();
    JZToolBar* mpToolBar;
    tGenMelody* genmeldy;
    DECLARE_EVENT_TABLE()
};

extern void CreateHarmonyBrowser(wxFrame* pParent);

#endif // !defined(JZ_HARMONY_H)
