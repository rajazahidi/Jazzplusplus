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


#ifndef util_h
#define util_h

#include "wx/wx.h"
#include "wx/proplist.h"


#include <iostream>
using namespace std;

#include <math.h>

#ifdef wx_msw
#define strcasecmp stricmp
#endif

inline int iabs(int a)
{
  return a < 0 ? -a : a;
}

class tDoubleCommand
{
public:
  int Command[2];
};


class wxStringListValidator;
class wxProperty;
class tSong;

// ************************************************************************
// tNamedChoice
// ************************************************************************


/**
tNamedValue goes with tNamedChoice, and is a name/value pair array.
tNamedChoice presents a dialog with the values to choose from


*/
class tNamedValue
{
public:
   tNamedValue() : Name(""), Value(0) {}
   tNamedValue( char *n, long v ) : Name(n), Value(v) {}
   char *Name;
   long  Value;
};


extern tNamedValue limitSteps[];

class tNamedChoice : public wxObject
{
    char *Selection;
    char *Title;
    tNamedValue *Values;	// Values[last].Name == 0
    long *Result;

  public:

    tNamedChoice(char *title, tNamedValue *values, long *result);
    ~tNamedChoice();
  //    wxFormItem *mkFormItem(int w = 100, int h = 100);
    wxStringListValidator* GetStringListValidator();
    void GetValue();
    void SetValue();
};


class tNamedValueChoice : public wxChoice
{
  tNamedValue *values;
public:
   tNamedValueChoice(wxPanel *parent, wxFunction func, char *label, tNamedValue *values);
   long GetValue();
   void SetValue(long measure);
};

class tMeasureChoice : public tNamedValueChoice
{
public:
   tMeasureChoice(wxPanel *parent, wxFunction func, char *label)
     : tNamedValueChoice(parent, func, label, limitSteps)
   {}

   long GetMeasure() { return GetValue(); }
   void SetMeasure(long measure) { SetValue(measure); }
   long GetTicks(tSong *s);
};

// **************************************************************************
// tRect
// **************************************************************************


/**
this class was originally not inherited, but i had a lot of poring problems so i changed it
*/
class tRect:public wxRect
{
public:
  //float x, y, w, h;
  tRect(int x = 0, int y = 0, int w = 0, int h = 0);

  void SetNormal();		// width, height > 0
  void SetUnion(tRect &);
  int  IsInside(int x, int y);
};


// **************************************************************************
// tClockDlg
// **************************************************************************

class tSong;

class tClockDlg
{
  char *String;
  char *Title;
  tSong *Song;

  public:
    tClockDlg(tSong *s, char *title, long clk);
    ~tClockDlg();
  //wxFormItem *mkFormItem(int w);
    wxProperty* mkProperty();
    long GetClock();
};



class tKeyDlg
{
  char *String;
  char *Title;

  public:
    tKeyDlg(char *title, int Key);
    ~tKeyDlg();
  //wxFormItem *mkFormItem(int w);
    wxProperty* mkProperty();
    int GetKey();
};


void Key2Str(int key, char *str);
int Str2Key(const char *str);

int GetArgOpt( char *opt );


int SelectControllerDlg();
istream & ReadString(istream &is, char *buf, int maxlen);
ostream & WriteString(ostream &os, const char *str);


wxString file_selector(wxString deffile, const wxString title, bool save, bool changed, const wxString ext);

// ------------------------------------------------------------
// ---------------------------- mapper ------------------------
// ------------------------------------------------------------

/**
 * maps one range to another. Useful for drawing, e.g. map world coordinates
 * to screen coordinates.
 */

class tMapper {

  public:

    /**
     * construct a mapper, that maps the range [x0,x1] to [y0,y1].
     */

    tMapper(double xx0, double xx1, double yy0, double yy1) {
      x0 = xx0;
      x1 = xx1;
      y0 = yy0;
      y1 = yy1;
    }
    tMapper() {
      x0 = x1 = y0 = y1 = 0;
    }
    void Initialize(double xx0, double xx1, double yy0, double yy1) {
      x0 = xx0;
      x1 = xx1;
      y0 = yy0;
      y1 = yy1;
    }

    /**
     * transform a value
     */

    double map(double x) {
      return y0 + (x - x0) * (y1 - y0) / (x1 - x0);
    }

    /**
     * alias for map()
     */

    double operator()(double x) {
      return y0 + (x - x0) * (y1 - y0) / (x1 - x0);
    }

    /**
     * perform reverse transformation
     */

    double pam(double y) {
      return x0 + (y - y0) * (x1 - x0) / (y1 - y0);
    }

  private:

    double x0, x1, y0, y1;
};


/**
 * maps the range -x .. +x to the range 1/y ... y using the exp()
 * function (i.e. map(0) == 1).
 */

class tExpMapper
{
  public:
    tExpMapper(double x, double y)
    : map(-x, x, -log(y), log(y))
    {
    }
    double operator()(double x) {
      return exp(map(x));
    }
  private:
    tMapper map;
};





#endif

