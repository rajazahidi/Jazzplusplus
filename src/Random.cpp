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

#include "Random.h"

#include "Mapper.h"

#include <wx/dcclient.h>
#include <wx/frame.h>
#include <wx/scrolwin.h>

#include <cassert>
#include <cstdlib>

using namespace std;

double JZRandomGenerator::asDouble()
{
  return double(rand()) / double(RAND_MAX);
}

JZRandomGenerator rnd;

// Array of probabilities

JZRndArray::JZRndArray(int nn, int mmin, int mmax)
{
  int i;
  n = nn;
  for (i = 0; i < n; i++)
  {
    mArray[i] = mmin;
  }
  min = mmin;
  max = mmax;
  nul = min > 0 ? min : 0;
}

JZRndArray::JZRndArray(const JZRndArray& Other)
  : mArray(Other.mArray),
    n(Other.n),
    nul(Other.nul),
    min(Other.min),
    max(Other.max)
{
}

void JZRndArray::SetMinMax(int mi, int ma)
{
  min = mi;
  max = ma;
  nul = min > 0 ? min : 0;
  for (int i = 0; i < mArray.GetSize(); i++)
  {
    if (mArray[i] < min)
    {
      mArray[i] = min;
    }
    else if (mArray[i] > max)
    {
      mArray[i] = max;
    }
  }
}

JZRndArray::~JZRndArray()
{
}

/* PAT - The following ifdef was removed due to the changes implemented in gcc
   3.x.  The new gcc is more compatible with Windows. */
/*#ifdef FOR_MSW*/
double JZRndArray::operator[](double f)
  /*#else
double JZRndArray::operator[](double f) const
#endif*/
{
  int i = (int)f;
  if (i < 0)
    i = 0;
  else if (i >= n - 2)
    i = n - 2;
  JZMapper Map(i, i+1, mArray[i], mArray[i+1]);
  return Map.XToY(f);
}

JZRndArray& JZRndArray::operator = (const JZRndArray& Rhs)
{
  if (this != &Rhs)
  {
    mArray = Rhs.mArray;
    n = Rhs.n;
    min = Rhs.min;
    max = Rhs.max;
    nul = Rhs.nul;
  }
  return *this;
}

int JZRndArray::Random()
{
  return Random(rnd.asDouble());
}

int JZRndArray::Random(double rndval)
{
  double sum, dec;
  int i;

  assert(n > 0);

  sum = 0.0;
  for (i = 0; i < n; i++)
  {
    assert(mArray[i] >= 0);
    sum += mArray[i];
  }
  if (sum <= 0)
    return 0;

  dec = sum * rndval * 0.99999;
  assert(dec < sum);

  i = 0;
  while (dec >= 0.0)
  {
    dec -= mArray[i];
    i++;
  }
  i--;

  assert(i >= 0 && i < n);
  return i;
}


int JZRndArray::Interval(int seed)
{
  if (seed < 0)                // initial ?
    seed = int(rnd.asDouble() * n);
  int delta = Random();
  if (rnd.asDouble() < 0.5)
    delta = -delta;
  seed = (seed + n + delta) % n;
  return seed;
}

int JZRndArray::Random(int i)
{
  return rnd.asDouble() * (max - min) < mArray[i];
}


void JZRndArray::SetUnion(JZRndArray &o, int fuzz)
{
  for (int i = 0; i < n; i++)
  {
    int val = mArray[i];
    if (o.mArray[i] > val)
    {
      val = o.mArray[i];
    }
    mArray[i] = Fuzz(fuzz, mArray[i], val);
  }
}


void JZRndArray::SetIntersection(JZRndArray &o, int fuzz)
{
  for (int i = 0; i < n; i++)
  {
    int val = mArray[i];
    if (o.mArray[i] < val)
    {
      val = o.mArray[i];
    }
    mArray[i] = Fuzz(fuzz, mArray[i], val);
  }
}


void JZRndArray::SetDifference(JZRndArray &o, int fuzz)
{
  JZRndArray tmp(o);
  tmp.SetInverse(tmp.Max());
  SetIntersection(tmp, fuzz);
}


void JZRndArray::SetInverse(int fuzz)
{
  for (int i = 0; i < n; i++)
  {
    mArray[i] = Fuzz(fuzz, mArray[i], min + max - mArray[i]);
  }
}


int JZRndArray::Fuzz(int fuz, int v1, int v2) const
{
  // interpolate between v1 and v2
  return (fuz - min) * v2 / (max - min) + (max - fuz) * v1 / (max - min);
}


void JZRndArray::Clear()
{
  for (int i = 0; i < n; i++)
  {
    mArray[i] = min;
  }
}


ostream & operator << (ostream &os, JZRndArray const &a)
{
  int i;

  os << a.n << " " << a.min << " " << a.max << endl;
  for (i = 0; i < a.n; i++)
  {
    os << a.mArray[i] << " ";
  }
  os << endl;
  return os;
}


istream & operator >> (istream &is, JZRndArray &a)
{
  int i;
  is >> a.n >> a.min >> a.max;
  for (i = 0; i < a.n; i++)
    is >> a.mArray[i];
  return is;
}


// --------------------------------- JZArrayEdit -------------------------------------

// length of tickmark line
#define TICK_LINE 0

JZArrayEdit::JZArrayEdit(
  wxFrame *frame,
  JZRndArray &ar,
  const wxPoint& Position,
  const wxSize& Size,
  int StyleBits)
  : wxScrolledWindow(frame, wxID_ANY, Position, Size),
    mArray(ar),
    n(ar.n),
    min(ar.min),
    max(ar.max),
    nul(ar.nul)
{
  draw_bars = 0;
  enabled = 1;
  dragging = 0;
  index = -1;
  mStyleBits = StyleBits;

  xmin = 0;
  xmax = n;

  x = 0;        // draw to topleft corner of canvas
  y = 0;
  w = Size.GetWidth();
  h = Size.GetHeight();

  int tw, th;

  wxClientDC Dc(this);
  Dc.SetFont(*wxSMALL_FONT);
  Dc.GetTextExtent("123", &tw, &th);

  if (mStyleBits & ARED_XTICKS)
  {
    // leave space for bottomline
    h -= (int)th;
  }

  if (mStyleBits & (ARED_MINMAX | ARED_YTICKS))
  {
    // leave space to display min / max
    x = (int)(tw + TICK_LINE);
    w -= (int)(tw + TICK_LINE);
  }

  ynul = y + h - h * (nul - min) / (max - min);
}

BEGIN_EVENT_TABLE(JZArrayEdit, wxScrolledWindow)
  EVT_SIZE(JZArrayEdit::OnSize)
  EVT_MOUSE_EVENTS(JZArrayEdit::OnMouseEvent)
END_EVENT_TABLE()


void JZArrayEdit::OnSize(wxSizeEvent& Event)
{
  w = Event.GetSize().GetWidth();
  h = Event.GetSize().GetHeight();

  Event.Skip();

  int tw, th;

  wxClientDC Dc(this);
  Dc.GetTextExtent("123", &tw, &th);

  if (mStyleBits & ARED_XTICKS)
    h -= (int)th;
  if (mStyleBits & (ARED_MINMAX | ARED_YTICKS))
  {
    x = (int)(tw + TICK_LINE);
    w -= (int)(tw + TICK_LINE);
  }
  ynul = y + h - h * (nul - min) / (max - min);
}

JZArrayEdit::~JZArrayEdit()
{
}

void JZArrayEdit::DrawBar(wxDC& Dc, int i, int black)
{
  if (mStyleBits & ARED_LINES)
  {
    if (!black)
    {
      Dc.SetPen(*wxWHITE_PEN);
    }

    JZMapper XMap(0, n, 0, w);
    JZMapper YMap(min, max, h, 0);

    int x1 = (int)XMap.XToY(i + 0.5);
    int y1 = (int)YMap.XToY(mArray[i]);
    if (i > 0)
    {
      // draw line to prev position
      int x0 = (int)XMap.XToY(i - 0.5);
      int y0 = (int)YMap.XToY(mArray[i-1]);
      Dc.DrawLine(x0, y0, x1, y1);
    }
    if (i < n-1)
    {
      // draw line to next position
      int x2 = (int)XMap.XToY(i + 1.5);
      int y2 = (int)YMap.XToY(mArray[i+1]);
      Dc.DrawLine(x1, y1, x2, y2);
    }

    if (!black)
    {
      Dc.SetPen(*wxBLACK_PEN);
    }
    return;
  }

  int gap = 0;
  if (mStyleBits & ARED_GAP)
  {
    gap = w / n / 6;
    if (!gap && w / n > 3)
      gap = 1;
  }
  int xbar, ybar, wbar, hbar;

  wbar = w / n - 2 * gap;
  xbar = x + i * w / n + gap;
  hbar = h * (mArray[i] - nul) / (max - min);

  if (mStyleBits & ARED_BLOCKS)
  {
    /*
    ybar = ynul - hbar;
    if (hbar < 0)
      hbar = -hbar;
    hbar = (hbar < 2) ? hbar : 2;
    */
    int hblk = 12;

    ybar = ynul - hbar - hblk/2;
    hbar = hblk;
    if (ybar < y)
    {
      int d = y - ybar;
      ybar += d;
      hbar -= d;
    }
    if (ybar + hbar > y + h)
    {
      int d = (ybar + hbar) - (y + h);
      hbar -= d;
    }
    if (hbar < 2)
      hbar = 2;
  }
  else

  if (hbar < 0)
  {
    ybar = ynul;
    hbar = -hbar;
  }
  else
    ybar = ynul - hbar;

  if (ybar == y)
    ++ybar, --hbar;

  if (!black)
  {
    Dc.SetBrush(*wxWHITE_BRUSH);
    Dc.SetPen(*wxWHITE_PEN);
  }
  if (wbar && hbar)
  {
    Dc.DrawRectangle(xbar, ybar, wbar, hbar);
  }
  if (!black)
  {
    Dc.SetBrush(*wxBLACK_BRUSH);
    Dc.SetPen(*wxBLACK_PEN);
  }
}

const char *JZArrayEdit::GetXText(int xval)
{
  static char buf[8];
  sprintf(buf, "%d", xval);
  return buf;
}

const char *JZArrayEdit::GetYText(int yval)
{
  static char buf[8];
  sprintf(buf, "%d", yval);
  return buf;
}

void JZArrayEdit::DrawXTicks(wxDC& Dc)
{
  int tw, th;

  if (!(mStyleBits & ARED_XTICKS))
  {
    return;
  }

  Dc.SetFont(*wxSMALL_FONT);

  // compute tickmark x-distance
  Dc.GetTextExtent("-123", &tw, &th);
  int max_labels = (int)(w / (tw + tw/2));
  if (max_labels > 0)
  {
    int step = (xmax - xmin + 1) / max_labels;
    if (step <= 0)
      step = 1;
    for (int val = xmin; val <= xmax; val += step)
    {
      const char *buf = GetXText(val);
      //sprintf(buf, "%d", val);
      Dc.GetTextExtent((char *)buf, &tw, &th);
      float yy = y + h;
      float xx = x + w * (val - xmin) / (xmax - xmin + 1);
      xx -= tw/2;        // center text
      xx += 0.5 * w / n; // middle of bar
      Dc.DrawText(buf, (int)xx, (int)yy);
      //Dc.DrawLine(x - TICK_LINE, yy, x, yy);
    }
  }

  Dc.SetFont(*wxNORMAL_FONT);
}


void JZArrayEdit::DrawYTicks(wxDC& Dc)
{

  Dc.SetFont(*wxSMALL_FONT);

  if (mStyleBits & ARED_YTICKS)
  {
    // compute tickmark y-distance
    int tw, th;
    Dc.GetTextExtent("-123", &tw, &th);
    int max_labels = (int)(h / (th + th/2));
    if (max_labels > 0)
    {
      int step = (max - min) / max_labels;
      if (step <= 0)
        step = 1;
      for (int val = min; val < max; val += step)
      {
        const char *buf = GetYText(val);
        //sprintf(buf, "%d", val);
        Dc.GetTextExtent((char *)buf, &tw, &th);
        float yy = y + h - h * (val - min) / (max - min) - th/2;
        Dc.DrawText(buf, x - tw - TICK_LINE, (int)yy);
        //Dc.DrawLine(x - TICK_LINE, yy, x, yy);
      }
    }
  }

  else if (mStyleBits & ARED_MINMAX)
  {
    // min/max
    int tw, th;
    char buf[20];
    sprintf(buf, "%d", max);
    Dc.GetTextExtent(buf, &tw, &th);
    Dc.DrawText(buf, x - tw, y);
    sprintf(buf, "%d", min);
    Dc.GetTextExtent(buf, &tw, &th);
    Dc.DrawText(buf, x - tw, y + h - th);

  }

  Dc.SetFont(*wxNORMAL_FONT);

}

void JZArrayEdit::DrawLabel(wxDC& Dc)
{
  Dc.SetFont(*wxSMALL_FONT);
  if (!mLabel.empty())
  {
    Dc.DrawText(mLabel.c_str(), x + 5, y + 2);
  }
  Dc.SetFont(*wxNORMAL_FONT);
}



void JZArrayEdit::OnDraw(wxDC& Dc)
{
  int i;

  // surrounding rectangle
  Dc.Clear();
  if (enabled)
  {
    Dc.SetBrush(*wxWHITE_BRUSH);
  }
  else
  {
    Dc.SetBrush(*wxGREY_BRUSH);
  }

  Dc.SetPen(*wxBLACK_PEN);
  if (w && h)
  {
    Dc.DrawRectangle(x, y, w, h);
  }

  // sliders
  Dc.SetBrush(*wxBLACK_BRUSH);
  for (i = 0; i < n; ++i)
  {
    DrawBar(Dc, i, 1);
  }

  DrawXTicks(Dc);
  DrawLabel(Dc);
  DrawYTicks(Dc);
  DrawNull(Dc);
  if (draw_bars)
  {
    draw_bars->DrawBars(Dc);
  }
}



void JZArrayEdit::DrawNull(wxDC& Dc)
{

  Dc.SetPen(*wxCYAN_PEN);
  // draw y-null line
  if (min < nul && nul < max)
    Dc.DrawLine(x, ynul, x+w, ynul);
  // draw x-null line
  if (xmin < 0 && 0 < xmax)
  {
    int x0 = w * (0 - xmin) / (xmax - xmin);
    Dc.DrawLine(x0, y, x0, y + h);
  }
  Dc.SetPen(*wxBLACK_PEN);
}



void JZArrayEdit::SetXMinMax(int xmi, int xma)
{
  xmin = xmi;
  xmax = xma;
}

int JZArrayEdit::Index(wxMouseEvent& MouseEvent)
{
  int ex, ey;
  MouseEvent.GetPosition(&ex, &ey);
  int i = (int)( ((short)ex - x) * n / w);
  i = i < 0 ? 0 : i;
  i = i >= n ? n-1 : i;
  return i;
}

int JZArrayEdit::Dragging(wxMouseEvent& MouseEvent)
{
  if (!dragging)
  {
    return 0;
  }

  if (index < 0)
  {
    index = Index(MouseEvent);
  }

  wxClientDC Dc(this); // PORTING this is evil and shoud go

  int val = nul;
  if (MouseEvent.LeftIsDown())
  {
    int ex, ey;
    MouseEvent.GetPosition(&ex, &ey);

#if 0
    {
      // in msw ex,ey are 65536 for negative values!
      char buf[500];
      sprintf(buf, "x %4.0f, y %4.0f, sh %d", ex, ey, MouseEvent.ShiftDown());
      Dc.DrawText(buf, 50, 50);
    }
#endif

    // $blk$ val = (int)( (y + h - (short)ey) * (max - min) / h + min);
    val = (int)( (double)(y + h - ey) * (max - min) / h + min + 0.5);
    val = val > max ? max : val;
    val = val < min ? min : val;
  }

  if (MouseEvent.ShiftDown())
  {
    int k;
    for (k = 0; k < n; k++)
    {

      DrawBar(Dc, k, 0);
      mArray[k] = val;
      DrawBar(Dc, k, 1);

    }
  }
  else if (MouseEvent.ControlDown())
  {
    DrawBar(Dc, index, 0);
    mArray[index] = val;
    DrawBar(Dc, index, 1);
  }
  else
  {
    int i = Index(MouseEvent);
    int k = i;
    if (i < index)
      for (; i <= index; i++)
      {
        DrawBar(Dc, i, 0);
        mArray[i] = val;
        DrawBar(Dc, i, 1);
      }
    else
      for (; i >= index; i--)
      {
        DrawBar(Dc, i, 0);
        mArray[i] = val;
        DrawBar(Dc, i, 1);
      }
    index = k;
  }

  return 0;
}

int JZArrayEdit::ButtonDown(wxMouseEvent& MouseEvent)
{
#ifdef __WXMSW__
  CaptureMouse();
#endif
  dragging = 1;
  index = Index(MouseEvent);
  Dragging(MouseEvent);
  return 0;
}

int JZArrayEdit::ButtonUp(wxMouseEvent& MouseEvent)
{
#ifdef __WXMSW__
  ReleaseMouse();
#endif
  dragging = 0;
  index    = -1;
//  wxClientDC Dc(this); // PORTING this is evil and shoud go
//  DrawLabel(Dc);
//  DrawNull(Dc);
  Refresh();
  return 0;
}


void JZArrayEdit::OnMouseEvent(wxMouseEvent& MouseEvent)
{
  if (!enabled)
  {
    return;
  }
  if (MouseEvent.ButtonDown())
  {
    ButtonDown(MouseEvent);
  }
  else if (MouseEvent.Dragging())
  {
    Dragging(MouseEvent);
  }
  else if (MouseEvent.ButtonUp())
  {
    ButtonUp(MouseEvent);
  }
}

void JZArrayEdit::Enable(int e)
{
  enabled = e;
}

void JZArrayEdit::SetLabel(char const* pLabel)
{
  mLabel = pLabel;
}

void JZArrayEdit::SetYMinMax(int mi, int ma)
{
  mArray.SetMinMax(mi, ma);
  ynul = y + h - h * (nul - min) / (max - min);
}

void JZArrayEdit::DrawBarLine(wxDC& Dc, int xx)
{
//  cerr << "x: " << x << " xx: " << xx << endl;
  if (xx > x && xx + 1 < x + w)
  {
    Dc.SetPen(*wxLIGHT_GREY_PEN);
    Dc.DrawLine(xx, y + 1, xx, y + h - 2);
    Dc.SetPen(*wxBLACK_PEN);
  }
}



JZRhyArrayEdit::JZRhyArrayEdit(
  wxFrame *parent,
  JZRndArray& Array,
  const wxPoint& Position,
  const wxSize& Size,
  int StyleBits)
  : JZArrayEdit(parent, Array, Position, Size, StyleBits)
{
  steps_per_count = 4;
  count_per_bar   = 4;
  n_bars          = 4;
}

void JZRhyArrayEdit::SetMeter(int s, int c, int b)
{
  steps_per_count = s;
  count_per_bar   = c;
  n_bars          = b;
  mArray.Resize(s * c * b);
  SetXMinMax(1, s * c * b);
}


void JZRhyArrayEdit::DrawXTicks(wxDC& Dc)
{
  if (!(mStyleBits & ARED_RHYTHM))
  {
    JZArrayEdit::DrawXTicks(Dc);
    return;
  }

  char buf[20];
  int tw, th;

  Dc.SetFont(*wxSMALL_FONT);

  // tick marks
  assert(steps_per_count && count_per_bar && n_bars);
  int i;
  for (i = 0; i < n; i += steps_per_count)
  {
    int mark = (i / steps_per_count) % count_per_bar + 1;
    sprintf(buf, "%d", mark);
    int yy = y + h;
    int xx = (int)(x + (i + 0.5) * w / n);
    Dc.GetTextExtent(buf, &tw, &th);
    xx -= (int)(tw/2.0);
    Dc.DrawText(buf, xx, yy);
  }
  Dc.SetFont(*wxNORMAL_FONT);
}
