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

//#include "config.h"

#include "Random.h"

#include "Mapper.h"

#include <assert.h>
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
    min(Other.min),
    max(Other.max),
    nul(Other.nul)
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
  if (seed < 0)		// initial ?
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


// --------------------------------- tArrayEdit -------------------------------------

// length of tickmark line
#define TICK_LINE 0

tArrayEdit::tArrayEdit(wxFrame *frame, JZRndArray &ar, long xx, long yy, long ww, long hh, int sty)
  : wxScrolledWindow(frame,-1, wxPoint(xx, yy), wxSize(ww, hh)),
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
  style_bits = sty;

  xmin = 0;
  xmax = n;

  x = 0;	// draw to topleft corner of canvas
  y = 0;
  w = ww;
  h = hh;

  long tw, th;

  wxDC *dc = new wxClientDC(this);
  dc->SetFont(*wxSMALL_FONT);
  dc->GetTextExtent("123", &tw, &th);

  if (style_bits & ARED_XTICKS)
  {
    // leave space for bottomline
    h -= (int)th;
  }

  if (style_bits & (ARED_MINMAX | ARED_YTICKS))
  {
    // leave space to display min / max
    x = (int)(tw + TICK_LINE);
    w -= (int)(tw + TICK_LINE);
  }

  ynul = y + h - h * (nul - min) / (max - min);
}

BEGIN_EVENT_TABLE(tArrayEdit, wxScrolledWindow)
  EVT_SIZE    (           tArrayEdit::OnSize)
  EVT_MOUSE_EVENTS(tArrayEdit::OnMouseEvent)
END_EVENT_TABLE()


void tArrayEdit::OnSize(wxSizeEvent& event)
{
  w = event.GetSize().GetWidth();
  h = event.GetSize().GetHeight();

  wxScrolledWindow::OnSize(event);

  int tw, th;

  wxClientDC* dc=new wxClientDC(this);
  dc->GetTextExtent("123", &tw, &th);
  delete dc;

  if (style_bits & ARED_XTICKS)
    h -= (int)th;
  if (style_bits & (ARED_MINMAX | ARED_YTICKS))
  {
    x = (int)(tw + TICK_LINE);
    w -= (int)(tw + TICK_LINE);
  }
  ynul = y + h - h * (nul - min) / (max - min);
}

tArrayEdit::~tArrayEdit()
{
}

void tArrayEdit::DrawBar(wxDC *dc, int i, int black)
{
  if (style_bits & ARED_LINES)
  {
    if (!black)
    {
      dc->SetPen(*wxWHITE_PEN);
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
      dc->DrawLine(x0, y0, x1, y1);
    }
    if (i < n-1)
    {
      // draw line to next position
      int x2 = (int)XMap.XToY(i + 1.5);
      int y2 = (int)YMap.XToY(mArray[i+1]);
      dc->DrawLine(x1, y1, x2, y2);
    }

    if (!black)
      dc->SetPen(*wxBLACK_PEN);
    return;
  }

  int gap = 0;
  if (style_bits & ARED_GAP)
  {
    gap = w / n / 6;
    if (!gap && w / n > 3)
      gap = 1;
  }
  long xbar, ybar, wbar, hbar;

  wbar = w / n - 2 * gap;
  xbar = x + i * w / n + gap;
  hbar = h * (mArray[i] - nul) / (max - min);

  if (style_bits & ARED_BLOCKS)
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
    if (ybar < y) {
      int d = y - ybar;
      ybar += d;
      hbar -= d;
    }
    if (ybar + hbar > y + h) {
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
    dc->SetBrush(*wxWHITE_BRUSH);
    dc->SetPen(*wxWHITE_PEN);
  }
  if (wbar && hbar)
    dc->DrawRectangle(xbar, ybar, wbar, hbar);
  if (!black)
  {
    dc->SetBrush(*wxBLACK_BRUSH);
    dc->SetPen(*wxBLACK_PEN);
  }
}

const char *tArrayEdit::GetXText(int xval)
{
  static char buf[8];
  sprintf(buf, "%d", xval);
  return buf;
}

const char *tArrayEdit::GetYText(int yval)
{
  static char buf[8];
  sprintf(buf, "%d", yval);
  return buf;
}

void tArrayEdit::DrawXTicks(wxDC* dc)
{
  int tw, th;

  if (!(style_bits & ARED_XTICKS))
    return;


  dc->SetFont(*wxSMALL_FONT);

  // compute tickmark x-distance
  dc->GetTextExtent("-123", &tw, &th);
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
      dc->GetTextExtent((char *)buf, &tw, &th);
      float yy = y + h;
      float xx = x + w * (val - xmin) / (xmax - xmin + 1);
      xx -= tw/2;        // center text
      xx += 0.5 * w / n; // middle of bar
      dc->DrawText(buf, (int)xx, (int)yy);
      //dc->DrawLine(x - TICK_LINE, yy, x, yy);
    }
  }

  dc->SetFont(*wxNORMAL_FONT);
}


void tArrayEdit::DrawYTicks(wxDC* dc)
{

  dc->SetFont(*wxSMALL_FONT);

  if (style_bits & ARED_YTICKS)
  {
    // compute tickmark y-distance
    int tw, th;
    dc->GetTextExtent("-123", &tw, &th);
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
	dc->GetTextExtent((char *)buf, &tw, &th);
	float yy = y + h - h * (val - min) / (max - min) - th/2;
	dc->DrawText(buf, x - tw - TICK_LINE, (int)yy);
	//dc->DrawLine(x - TICK_LINE, yy, x, yy);
      }
    }
  }

  else if (style_bits & ARED_MINMAX)
  {
    // min/max
    int tw, th;
    char buf[20];
    sprintf(buf, "%d", max);
    dc->GetTextExtent(buf, &tw, &th);
    dc->DrawText(buf, x - tw, y);
    sprintf(buf, "%d", min);
    dc->GetTextExtent(buf, &tw, &th);
    dc->DrawText(buf, x - tw, y + h - th);

  }

  dc->SetFont(*wxNORMAL_FONT);

}

void tArrayEdit::DrawLabel(wxDC* dc)
{
  dc->SetFont(*wxSMALL_FONT);
  if (!mLabel.empty())
  {
    dc->DrawText(mLabel.c_str(), x + 5, y + 2);
  }
  dc->SetFont(*wxNORMAL_FONT);
}



void tArrayEdit::OnDraw(wxDC& indc)
{
  int i;
  wxDC *dc = &indc; //just lazy...

  // surrounding rectangle
  dc->Clear();
  if (enabled)
    dc->SetBrush(*wxWHITE_BRUSH);
  else
    dc->SetBrush(*wxGREY_BRUSH);
  dc->SetPen(*wxBLACK_PEN);
  if (w && h)
    dc->DrawRectangle(x, y, w, h);

  // sliders
  dc->SetBrush(*wxBLACK_BRUSH);
  for (i = 0; i < n; i++)
    DrawBar(dc, i, 1);

  DrawXTicks(dc);
  DrawLabel(dc);
  DrawYTicks(dc);
  DrawNull(dc);
  if (draw_bars)
    draw_bars->DrawBars(dc);
}



void tArrayEdit::DrawNull(wxDC* dc)
{

  dc->SetPen(*wxCYAN_PEN);
  // draw y-null line
  if (min < nul && nul < max)
    dc->DrawLine(x, ynul, x+w, ynul);
  // draw x-null line
  if (xmin < 0 && 0 < xmax)
  {
    int x0 = w * (0 - xmin) / (xmax - xmin);
    dc->DrawLine(x0, y, x0, y + h);
  }
  dc->SetPen(*wxBLACK_PEN);
}



void tArrayEdit::SetXMinMax(int xmi, int xma)
{
  xmin = xmi;
  xmax = xma;
}

int tArrayEdit::Index(wxMouseEvent &e)
{
  int ex, ey;
  e.GetPosition(&ex, &ey);
  int i = (int)( ((short)ex - x) * n / w);
  i = i < 0 ? 0 : i;
  i = i >= n ? n-1 : i;
  return i;
}

int tArrayEdit::Dragging(wxMouseEvent &e)
{
  if (!dragging)
    return 0;

  if (index < 0)
    index = Index(e);

  int val = nul;
  if (e.LeftIsDown())
  {
    int ex, ey;
    e.GetPosition(&ex, &ey);
    // $blk$ val = (int)( (y + h - (short)ey) * (max - min) / h + min);
    val = (int)( (double)(y + h - ey) * (max - min) / h + min + 0.5);
    val = val > max ? max : val;
    val = val < min ? min : val;
  }

#if 0
  {
    // in msw ex,ey are 65536 for negative values!
    wxDC *dc = new wxClientDC(this);//GetDC();
    char buf[500];
    sprintf(buf, "x %4.0f, y %4.0f, sh %d", ex, ey, e.ShiftDown());
    dc->DrawText(buf, 50, 50);
  }
#endif
      wxDC *dc = new wxClientDC(this);//__PORTING this is evil and shoud go
  if (e.ShiftDown())
  {
    int k;
    for (k = 0; k < n; k++)
    {

      DrawBar(dc, k, 0);
      mArray[k] = val;
      DrawBar(dc, k, 1);

    }
  }
  else if (e.ControlDown())
  {
    DrawBar(dc, index, 0);
    mArray[index] = val;
    DrawBar(dc, index, 1);
  }
  else
  {
    int i = Index(e);
    int k = i;
    if (i < index)
      for (; i <= index; i++)
      {
	DrawBar(dc, i, 0);
	mArray[i] = val;
	DrawBar(dc, i, 1);
      }
    else
      for (; i >= index; i--)
      {
	DrawBar(dc, i, 0);
	mArray[i] = val;
	DrawBar(dc, i, 1);
      }
    index = k;
  }
      delete dc;
  return 0;
}

int tArrayEdit::ButtonDown(wxMouseEvent &e)
{
#ifdef __WXMSW__
  CaptureMouse();
#endif
  dragging = 1;
  index = Index(e);
  Dragging(e);
  return 0;
}

int tArrayEdit::ButtonUp(wxMouseEvent &e)
{
#ifdef __WXMSW__
  ReleaseMouse();
#endif
  dragging = 0;
  index    = -1;
//       wxDC *dc = new wxClientDC(this);//__PORTING this is evil and shoud go
//   DrawLabel(dc);
//   DrawNull(dc);
//   delete dc;
  Refresh();
  return 0;
}


void tArrayEdit::OnMouseEvent(wxMouseEvent &e)
{
  if (!enabled)
    return;
  if (e.ButtonDown())
    ButtonDown(e);
  else if (e.Dragging())
    Dragging(e);
  else if (e.ButtonUp())
    ButtonUp(e);
}

void tArrayEdit::Enable(int e)
{
  enabled = e;
}

void tArrayEdit::SetLabel(char const* pLabel)
{
  mLabel = pLabel;
}

void tArrayEdit::SetYMinMax(int mi, int ma)
{
  mArray.SetMinMax(mi, ma);
  ynul = y + h - h * (nul - min) / (max - min);
}

void tArrayEdit::DrawBarLine (wxDC *dc, long xx)
{
  //  wxDC *dc = new wxClientDC(this);//GetDC();
  //  fprintf(stderr,"x: %ld, xx: %ld\n",x,xx);
  if (xx > x && xx + 1 < x + w)
    {
      dc->SetPen (*wxLIGHT_GREY_PEN);
      dc->DrawLine (xx, y + 1, xx, y + h - 2);
      dc->SetPen (*wxBLACK_PEN);
    }
}



tRhyArrayEdit::tRhyArrayEdit(
  wxFrame *parent,
  JZRndArray& Array,
  long xx,
  long yy,
  long ww,
  long hh,
  int sty)
  : tArrayEdit(parent, Array, xx, yy, ww, hh, sty)
{
  steps_per_count = 4;
  count_per_bar   = 4;
  n_bars          = 4;
}

void tRhyArrayEdit::SetMeter(int s, int c, int b)
{
  steps_per_count = s;
  count_per_bar   = c;
  n_bars          = b;
  mArray.Resize(s * c * b);
  SetXMinMax(1, s * c * b);
}


void tRhyArrayEdit::DrawXTicks(wxDC* dc)
{
  if (!(style_bits & ARED_RHYTHM))
  {
    tArrayEdit::DrawXTicks(dc);
    return;
  }

  char buf[20];
  int tw, th;


  dc->SetFont(*wxSMALL_FONT);

  // tick marks
  assert(steps_per_count && count_per_bar && n_bars);
  int i;
  for (i = 0; i < n; i += steps_per_count)
  {
    int mark = (i / steps_per_count) % count_per_bar + 1;
    sprintf(buf, "%d", mark);
    int yy = y + h;
    int xx = (int)(x + (i + 0.5) * w / n);
    dc->GetTextExtent(buf, &tw, &th);
    xx -= (int)(tw/2.0);
    dc->DrawText(buf, xx, yy);
  }
  dc->SetFont(*wxNORMAL_FONT);
}

