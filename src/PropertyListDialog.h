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

// JAVE
//
// The proplistdlg class is meant as a convenience wrapper around the wxwin2
// class wxPropertyListForm.
//
// These dialogs aren't exactly beautiful but are meant to make it simpler to
// port simple jazz dialogs to wx2.  Many dialogs in jazz are more advanced
// than can be handled by this class.
//
// In the future this class might be used to implement wxPropertyForms, who
// can be better looking.

#ifndef JZ_PROPERTYLISTDIALOG_H
#define JZ_PROPERTYLISTDIALOG_H

#include "NamedValue.h"
#include "DeprecatedWx/proplist.h"
#include "DeprecatedWx/propform.h"

#include <vector>
#include <string>

class tNamedValueListValue : public wxPropertyValue
{
//  DECLARE_DYNAMIC_CLASS(tNamedValueListValue)

  public:

//    tNamedValueListValue()
//      : wxPropertyValue()
//    {
//    }

    tNamedValueListValue(
      int* pValue,
      const std::vector<std::pair<std::string, int> >& Pairs)
      : wxPropertyValue(pValue),
        mPairs(Pairs)
    {
    }

  //overriding these doesnt really work in the display area
/*   virtual wxString GetStringRepresentation(void){ */
/*     cout <<"GetStringRepresentation"<<endl; */
/*     return wxString("apa"); */
/*   } */
/*   int IntegerValue(){ */
/*     return 42; */
/*   } */

  private:

//    tNamedValue* nval;
    const std::vector<std::pair<std::string, int> >& mPairs;
};


class tPropertyListDlg
{
 public:
  tPropertyListDlg(wxString title);
  virtual ~tPropertyListDlg()
  {
  }
  virtual void AddProperties();
  void Create();
  void CreateModal();
  virtual bool OnClose();
  virtual void OnPropertyChanged(wxProperty* pProperty);
 protected:
  wxPropertySheet* sheet;
  wxPropertyListView* view;
  wxPropertyValidatorRegistry *myListValidatorRegistry;//cannot be a temporary! leads to a crash
  wxString title;

};

class tNamedValueListValidator : public wxStringListValidator
{
  public:

    tNamedValueListValidator(
      const std::vector<std::pair<std::string, int> >& Values);

    ~tNamedValueListValidator();

    int MapName2Value(const char* pSelection);

    wxString MapValue2Name(int Value);
  
    virtual bool OnSelect(
      bool Select,
      wxProperty* pProperty,
      wxPropertyListView* pView,
      wxWindow* pParentWindow);

    virtual bool OnDoubleClick(
      wxProperty* WXUNUSED(pProperty),
      wxPropertyListView* WXUNUSED(pView),
      wxWindow* WXUNUSED(pParentWindow));

    virtual bool OnValueListSelect(
      wxProperty* pProperty,
      wxPropertyListView* pView,
      wxWindow* pParentWindow);

    virtual bool OnPrepareControls(
      wxProperty* WXUNUSED(pProperty),
      wxPropertyListView* WXUNUSED(pView),
      wxWindow* WXUNUSED(pParentWindow));

    virtual bool OnClearControls(
      wxProperty* pProperty,
      wxPropertyListView* pView,
      wxWindow* pParentWindow);

    virtual bool OnPrepareDetailControls(
      wxProperty* WXUNUSED(pProperty),
      wxPropertyListView* WXUNUSED(pView),
      wxWindow* WXUNUSED(pParentWindow));

    virtual bool OnClearDetailControls(
      wxProperty* WXUNUSED(pProperty),
      wxPropertyListView* WXUNUSED(pView),
      wxWindow* WXUNUSED(pParentWindow));

    virtual void OnEdit(
      wxProperty* pProperty,
      wxPropertyListView* pView,
      wxWindow* pParentWindow);

    virtual bool OnCheckValue(
      wxProperty* WXUNUSED(pProperty),
      wxPropertyListView* WXUNUSED(pView),
      wxWindow* WXUNUSED(pParentWindow));

    virtual bool OnRetrieveValue(
      wxProperty* pProperty,
      wxPropertyListView* pView,
      wxWindow* pParentWindow);

    virtual bool OnDisplayValue(
      wxProperty* pProperty,
      wxPropertyListView* pView,
      wxWindow *pParentWindow);

  private:

    const std::vector<std::pair<std::string, int> >& mValues;
};

#endif // !defined(JZ_PROPERTYLISTDIALOG_H)
