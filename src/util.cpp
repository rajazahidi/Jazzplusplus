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

#include "wxwin2port.h"

#include "wx/wx.h"

#include "util.h"
#include "song.h"
#include "jazz.h"

#include <ctype.h>


// *************************************************************************
// tNamedChoice
// *************************************************************************

tNamedValue limitSteps[] =
{
  tNamedValue( "1/8",   8 ),
  tNamedValue( "1/12", 12 ),
  tNamedValue( "1/16", 16 ),
  tNamedValue( "1/24", 24 ),
  tNamedValue( "1/32", 32 ),
  tNamedValue( "1/48", 48 ),
  tNamedValue( "1/96", 96 ),
  tNamedValue( "1/192", 192 ),
  tNamedValue(   0,      1  )
};


tNamedChoice::tNamedChoice(char *t, tNamedValue *v, long *r)
{
  Values = v;
  Selection = 0;
  Title = t;
  Result = r;
}


#ifndef __PORTING
wxFormItem *tNamedChoice::mkFormItem(int w, int h)
{
  SetValue();

  // following adapted from wxwin/src/base/wb_form.cc
  wxList *list = new wxList;
  for (int i = 0; Values[i].Name; i++)
    if (*Values[i].Name)	// omit empty entries
      list->Append((wxObject *)copystring(Values[i].Name));

  wxFormItemConstraint *constraint = wxMakeConstraintStrings(list);
  return wxMakeFormString(Title, &Selection, wxFORM_SINGLE_LIST, new wxList(constraint, 0),
    0, 0, w, h);
}
#endif

/**return a string list validator to use in wxproplist dialogs*/
wxStringListValidator* tNamedChoice::GetStringListValidator()
{
  
  wxStringList* stringlist= new wxStringList();
  for (int i = 0; Values[i].Name; i++)
    if (*Values[i].Name)	// omit empty entries
      stringlist->Add(wxString(Values[i].Name));
  return new wxStringListValidator(stringlist);
}


void tNamedChoice::GetValue()
{
  int i;

  if (Selection)
  {
    for (i = 0; Values[i].Name; i++)
    {
      if (!strcmp(Selection, Values[i].Name))
      {
	*Result = Values[i].Value;
	break;
      }
    }
  }
}


void tNamedChoice::SetValue()
{
  int i;

  for (i = 0; Values[i].Name; i++)
  {
    if (*Result == Values[i].Value)
    {
      delete Selection;
      Selection = copystring(Values[i].Name);
      break;
    }
  }
}

tNamedChoice::~tNamedChoice()
{
  delete Selection;
}


// **********************************************************************
// tNamedValuesChoice
// **********************************************************************

tNamedValueChoice::tNamedValueChoice(wxPanel *parent, wxFunction func, char *label, tNamedValue* nvals)
  : wxChoice(parent,-1), values(nvals) //JAVE label should also be in the wxChoice constructor call
{
  tNamedValue *v = values;
  while (v->Name) {
    Append(v->Name);
    ++v;
  }
}

long tNamedValueChoice::GetValue()
{
  int i = GetSelection();
  if (i >= 0)
    return values[i].Value;
  return 16;
}


void tNamedValueChoice::SetValue(long measure)
{
  int i;
  for (i = 0; values[i].Name; i++) {
    if (values[i].Value == measure) {
      SetSelection(i);
      break;
    }
  }
}

#ifndef __PORTING


long tMeasureChoice::GetTicks(tSong *s) {
  long m = GetMeasure();
  if (m < 0)
    m = 16;
  m = s->TicksPerQuarter * 4 / m;
  return (m > 0) ? m : 1;
}

#endif // __PORTING

// **********************************************************************
// tRect
// **********************************************************************


tRect::tRect(int xx, int yy, int ww, int hh)
  :wxRect(xx,yy,ww,hh)
{

}

/**
add 2 rects together by making a new rect that overlap both(not strictly true, since we only take max of width and height and do not care about start x y)
 */
void tRect::SetUnion(tRect &r)
{
  if (r.width > width)
    width = r.width;
  if (r.height > height)
    height = r.height;
}


/**
"normalize" the rectangle, by making width and height always positive, and giving minimum width/height=1
*/
void tRect::SetNormal()
{
  if (width < 0)
  {
    width = -width;
    x -= width;
  }
  if (height < 0)
  {
    height = -height;
    y -= height;
  }
  if (width == 0)
    width = 1;
  if (height == 0)
    height = 1;
}

/**
check if a point is inside a rectangle
 */
int tRect::IsInside(int xx, int yy)
{
  if (xx < x || xx > x + width)
    return 0;
  if (yy < y || yy > y + height)
    return 0;
  return 1;
}

// **************************************************************************
// tClockDlg
// **************************************************************************



tClockDlg::tClockDlg(tSong *s, char *t, long c)
{
  char buf[500];
  s->Clock2String(c, buf);
  String = copystring(buf);
  Title = t;
  Song = s;
}




tClockDlg::~tClockDlg()
{
  delete String;
}


wxProperty* tClockDlg::mkProperty()
{
  return new wxProperty(Title, wxPropertyValue((char**)&String), "string"); 
}





long tClockDlg::GetClock()
{
  return Song->String2Clock(String);
}



// ***********************************************************************
// KeyStr
// ***********************************************************************


void Key2Str(int key, char *str)
{
  static char *Names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
  strcpy(str, Names[key % 12]);
  sprintf(str + strlen(str), "%d", key / 12);
}


int Str2Key(const char *str)
{
  static char sKey[] = "cCdDeEfFgGaAbB";
  static int  nKey[]={0,0,2,2,4,4,5,5,7,7,9,9,11,11};
  int i, key = 0;

  while (*str)
  {
    if (*str == '#')
    {
      key += 1;
      str++;
    }
    else if (isdigit(*str))
    {
      int n = 0;
      while (isdigit(*str))
        n = 10 * n + *str++ - '0';
      key += 12 * n;
    }
    else
    {
      for (i = 0; sKey[i]; i++)
      {
	if (str[0] == sKey[i])
	{
	  key += nKey[i];
	  str++;
	  break;
	}
      }
      if (!sKey[i])	// error
        ++ str;
    }
  }
  return key;
}

// **************************************************************************
// tNoteDlg
// **************************************************************************

tKeyDlg::tKeyDlg(char *t, int Key)
{
  char buf[50];
  Key2Str(Key, buf);
  String = copystring(buf);
  Title = t;
}


tKeyDlg::~tKeyDlg()
{
  delete String;
}

#ifndef __PORTING
wxFormItem *tKeyDlg::mkFormItem(int w)
{
  return wxMakeFormString(Title, &String, wxFORM_DEFAULT,0,0,0,w);
}
#endif

wxProperty* tKeyDlg::mkProperty(){
  return new wxProperty(Title, wxPropertyValue((char**)&String), "string"); 
}

int tKeyDlg::GetKey()
{
  return Str2Key(String);
}

int GetArgOpt( char *opt ) {

	int arg = 0;

	for (int i = 1; i < wxTheApp->argc; i++) {
		if (!strcmp( wxTheApp->argv[i], opt )) {
				arg = i;
		}
	}
	return( arg );
}


int SelectControllerDlg()
{
  int i, n = 0;
  //  char *names[130];
  // PAT - The following line used to be this:
  //  wxArrayString names=new wxArrayString();
  wxArrayString names;
  int   ctrls[130];
  for (i = 0; Config.CtrlName(i).Name; i++)
  {
    if (Config.CtrlName(i).Name[0])
    {
      ctrls[n]   = Config.CtrlName(i).Value;
      //      names.Add(*(new wxString(Config.CtrlName(i).Name))); //JAVE leaking?
      names.Add(Config.CtrlName(i).Name); //JAVE leaking?
    }
  }
  i = wxGetSingleChoiceIndex("Controller", "Select a controller", names);
  if (i >= 0)
    return ctrls[i];
  return -1;
}

#if 0
// select multiple drum instruments
// returns:
//  -1 : user pressed escape
//   0 : user pressed ok, but no instrument was selected
//   n : user pressed ok, n = number of selected instruments. The keys
//       are returned in keys parameter

int SelectMultipleDrumsDlg(int *keys)
{
  int i, n = 0;
  char *names[150];
  int  selection[150];
  int  n_sel;

  for (i = 0; DrumNames[i].Name; i++)
    if (DrumNames[i].Name[0])
      names[n++] = DrumNames[i].Name;
  n_sel = wxGetMultipleChoice("Select some instruments", "Drums", n, names, 0, selection);
  for (i = 0; i < n_sel; i++)
    keys[i] = DrumNames[selection[i]].Value - 1;
  return n_sel;
}
#endif

////////////////////////////////////////////////////////////////////////////////////
// read/write strings from stream
////////////////////////////////////////////////////////////////////////////////////

istream & ReadString(istream &is, char *buf, int maxlen)
{
  // read a string (I hate C++)
  maxlen--;	// space for \0
  int c, i;
  do
    c = is.get();	// ignore \"
  while (c != '"' && c != EOF);
  for (i = 0; i < maxlen; i++) {
    c = is.get();
    if (c == '"' || c == EOF)
      break;
    buf[i] = c;
  }
  buf[i] = 0;
  return is;
}


ostream & WriteString(ostream &os, const char *str)
{
  os << '"' << str << '"';
  return os;
}


////////////////////////////////////////////////////////////////////////////////////
// file selection utils
////////////////////////////////////////////////////////////////////////////////////


/**
   add a supplied extension to a filename, if the filename doesnt already have an extension.
   ext is assumed to have a leading dot. (.mid for example)
*/
wxString add_default_ext(const wxString fn, const wxString ext)

{
  // is any extension already there?
  {
    const wxString x = wxFileNameFromPath(fn);
    if (x.find( '.', TRUE) != 0) //if there is a dot, assume its an extension and return
      return fn;
  }

  //otherwise append the supplied extension and return
  wxString rv=fn;
  rv+=ext;
  return rv;

  //this was the old c version

//   const int elen = strlen(ext);
//   const int flen = strlen(fn);
//   char *fn2 = new char[strlen(fn) + elen + 1];
//   strcpy(fn2, fn);
//   strcat(fn2, ext);
//   delete [] fn;
//   return fn2;


}

/**
presents a file selector, with some defaults given in the default.
can present a load or save selector.
if save, handles situations like showing a "really save over..." question box.

returns a  wxString filename. (was char*)

the caller owns the string and must deallocate

when converting to wxwin2 i had to change many char* to wxString. 
i havent figured out the best way to handle ownership of strings yet, so there are probably some leaks and bugs here

later on: it seems that wxstrings and char* does not mix well, at least in wx232, so im trying to move all the way to wxStrings.
wxStrings have refernce counting, so should be less error prone.

*/
wxString file_selector(wxString deffile, 
		    const wxString title, 
		    Bool save, 
		    Bool changed, 
		    const wxString ext)
{
   wxString s;
   wxString file;
   wxString path;

   if (save)
     file = wxFileNameFromPath(deffile);
   
   path = wxPathOnly(deffile);
   
   int flags = save ? wxSAVE : wxOPEN;
   s = wxFileSelector(title, path, file, 0, ext, flags);

  // add extension if missing
  if (s && ext) {
    s = add_default_ext(s, ext);
  }

  // warn if overwriting existent file
  if (s && save)
  {
    if (wxFileExists(s))
    {
      wxString buf;
      //sprintf(buf, "overwrite %s?", (char*)s);
      buf << "overwrite "<<s<<"?";
      if (wxMessageBox(buf, "Save ?", wxYES_NO) == wxNO) {

        s = wxEmptyString;
      }
    }
  }

  if (s && !save && changed)
  {
    wxString buf;
    buf<<deffile;
    buf <<" has changed. Load anyway?";
    if (wxMessageBox(buf, "Load ?", wxYES_NO) == wxNO) {
      s = wxEmptyString;
    }
  }

  if (s!=wxEmptyString && !save)
  {
    if (!wxFileExists(s))
    {
      wxString buf;
      buf<< "Cannot find file ";
      buf<< s;
      wxMessageBox( buf, "Error", wxOK );
      s = wxEmptyString;
    }
  }

  if (s!=wxEmptyString)
  {
    //delete [] deffile;
    //deffile = s;
    //return deffile;
    //i dont understand the point of the above original construct
//     wxString rv=*(new wxString(s));
//     return rv;//copy the local string and return it
    return s; //should be safe to return locally allocated string
  }

  return wxEmptyString;
}


