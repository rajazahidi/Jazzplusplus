/*
**  The JAZZ++ Midi Sequencer
**
** Copyright (C) 1994-2000 Andreas Voss and Per Sigmond, all rights reserved.
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

#include "config.h"
#include "slidrwin.h"
#include "random.h"
#include "toolbar.h"

/* These two lines are just here to handle the cout usage. */
#include <iostream>
using namespace std;


tSliderWin::tSliderWin(wxFrame *parent, wxFrame **ref, const char *title, int geo[4], tToolDef *tdefs)
  : wxFrame(0, -1, (char *)title, wxPoint(geo[0], geo[1]), wxSize(geo[2], geo[3]))
{
  this->geo = geo;
  in_constructor = TRUE;
  this->ref = ref;
  n_sliders   = 0;
  sliders_per_row = 1;
  panel = new wxPanel(this, 0, 0, 1000, 1000);

  if (tdefs != NULL)
    tool_bar=new tToolBar(this, tdefs);
  else
    tool_bar = 0;
}

void tSliderWin::Initialize()
{
  AddItems();
  panel->Fit();
  AddEdits();
  in_constructor = FALSE;

  int cw, ch;
  GetClientSize(&cw, &ch);
#ifndef __PORTING

  OnSize(cw, ch);
#endif// __PORTING

}


tSliderWin::~tSliderWin()
{
  *ref = 0;
  GetPosition(&geo[0], &geo[1]);
  GetSize(&geo[2], &geo[3]);
  for (int i = 0; i < n_sliders; i++)
    delete sliders[i];
}


BEGIN_EVENT_TABLE(tSliderWin, wxFrame)
  EVT_SIZE    (tSliderWin::OnSize) 
END_EVENT_TABLE()

  /**called from the event table whenever the window is resized

  the resizing algorithm here no longer works for some reason


*/
  void tSliderWin::OnSize(wxSizeEvent& event)//int w, int h)
{
  cout <<"tSliderWin::OnSize "<<in_constructor<<endl;
  wxSize sz=event.GetSize();
  int w=sz.GetWidth();
  int h=sz.GetHeight();

if (in_constructor)
    return;

  int cw, ch;
  GetClientSize(&cw, &ch);
  int pw = 0;
  int ph = 0;
  panel->GetSize(&pw, &ph);
//   if (tool_bar) {
//     int tw = 0;
//     int th = 0;
//     tool_bar->GetMaxSize(&tw, &th);
//     tool_bar->SetSize(0, 0, (int)cw, (int)th);
//     panel->SetSize(0, (int)th, (int)cw, (int)ph);
//     ph += (int)th; // add toolbar height to panel height
//   }
//   else
    panel->SetSize(0, 0, (int)cw, (int)ph);

  int n_rows = (n_sliders - 1) /  sliders_per_row + 1;
  int ay = (int)ph;
  for (int row = 0; row < n_rows; row++)
  {
    float y0 = ph + row * (ch - ph) / n_rows;
    float y1 = y0 + (ch - ph) / n_rows;
    int n_cols = sliders_per_row;
    if (row == n_rows - 1)
      n_cols = (n_sliders - 1) % sliders_per_row + 1;
    for (int col = 0; col < n_cols; col++)
    {
      int k = row * sliders_per_row + col;
      float x0 = col * (float)cw / n_cols;
      float x1 = x0 + (float)cw / n_cols;
#ifndef __PORTING
      sliders[k]->SetSize((int)x0, (int)y0, (int)(x1 - x0), (int)(y1 - y0));
#else
      sliders[k]->SetSize((int)x0, 0, (int)(x1 - x0), (int)(y0-y1));
#endif// __PORTING
      cout<<"slider "<<k<<" size:"<<(int)x0<<" "<< (int)y0<<" "<< (int)(x1 - x0)<<" "<< (int)(y1 - y0)<<endl;
    }
  }

}



bool tSliderWin::OnClose()
{
  return TRUE;
}


void tSliderWin::AddItems()
{
#ifndef __PORTING 
 (void)new wxButton(panel, (wxFunction)ItemCallback, "MyButton") ;
#endif // __PORTING
}


void tSliderWin::AddEdits()
{
  n_sliders = 2;
  sliders_per_row = 2;
  for (int i = 0; i < n_sliders; i++)
    sliders[i] = new tRhyArrayEdit(this, *new tRndArray(20, 0, 100), 10, 10, 10, 10, (ARED_GAP | ARED_XTICKS));
}

#ifndef __PORTING 
void tSliderWin::OnItem(wxItem& item, wxCommandEvent& event)
{
}


void tSliderWin::ItemCallback(wxItem& item, wxCommandEvent& event)
{
  ((tSliderWin *)(item.GetParent()->GetParent()))->OnItem(item, event);
}

#endif // __PORTING

void tSliderWin::ForceRepaint()
{
#ifndef __PORTING 
  int cw, ch;
  GetClientSize(&cw, &ch);
  OnSize(cw, ch);

  OnPaint();

  for (int i = 0; i < n_sliders; i++)
    sliders[i]->OnPaint();
#endif // __PORTING
}
