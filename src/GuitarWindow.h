//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Copyright (C) 1994-2000 Andreas Voss and Per Sigmond, all rights reserved.
// Modifications Copyright (C) 2004 Patrick Earl
// Modifications Copyright (C) 2008-2013 Peter J. Stieber
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

#pragma once

#include <wx/scrolwin.h>

class JZGuitarFrame;

//*****************************************************************************
// Description:
//   This is the guitar window class declaration.
//*****************************************************************************
class JZGuitarWindow : public wxScrolledWindow
{
  public:

    JZGuitarWindow(
      JZGuitarFrame* pParent,
      const wxPoint& Position,
      const wxSize& Size);

    virtual ~JZGuitarWindow();

    virtual void OnDraw(wxDC& Dc);

    void ClearBuffer();

    virtual void SetWindowSize(int Width, int Height)
    {
      mWidth = Width;
      mHeight = Height;
      Refresh();
    }

    void ShowPitch(int Pitch);
    void UpdateSettings();

    static bool GetChordMode() { return mChordMode; }
    static void SetChordMode(bool c) { mChordMode = c; }
    static bool GetBassGuitar() { return mBassGuitar; }
    static void SetBassGuitar(bool b);
    static bool GetShowOctaves() { return mShowOctaves; }
    static void SetShowOctaves(bool s) { mShowOctaves = s; }
    static int  GetFretCount() { return mFretCount; }
    static void SetFretCount(int count);

  private:

    void DrawBoard(wxDC& Dc);

    void DrawPitch(wxDC& Dc, int Pitch, int String, bool Show);

    void DrawPitch(wxDC& Dc, int Pitch, bool Show);

    void ShowPitch(wxDC& Dc, int Pitch);

    int y2String(int y);

    int x2Grid(int x);

    int Xy2Pitch(int x, int y);

    void OnSize(wxSizeEvent& Event);

    void OnPaint(wxPaintEvent& Event);

    void OnMouseMove(wxMouseEvent& MouseEvent);

    void OnMouseDown(wxMouseEvent& MouseEvent);

    void OnMouseUp(wxMouseEvent& MouseEvent);

    void OnMouseLeave(wxMouseEvent& MouseEvent);

    void OnMouseCaptureLost(wxMouseCaptureLostEvent& Event);

    void PlayNote(int pitch);

    void StopNote();

  private:

    static bool mChordMode;
    static bool mBassGuitar;
    static bool mShowOctaves;
    static int  mFretCount;

    int mStringCount;
    const int* mpPitches;
    static const int mGuitarPitches[6];
    static const int mBassPitches[4];

    int mWidth, mHeight;
    int mStringHeight, mFretWidth;
    int mNutX;
    int mMargin;
    int mActivePitch;     // mouse hover
    int mPlayPitch;       // active sound playing

    wxFont* mpFont;
    wxFont* mpBoldFont;
    wxFont* mpSmallFont;

    DECLARE_EVENT_TABLE()
};
