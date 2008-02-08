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

#include "PropertyListDialog.h"

#include <cstdlib>
#include <ctype.h>
#include <string.h>

using namespace std;

////////////////////////////////////////////////////////////////
// just wrap the wxPropertyListView so we can intercept the callbacks
class myproplistview : public wxPropertyListView
{
  public:

    myproplistview(tPropertyListDlg* parent);

    // The OnOk and OnClose handlers are never called, only OnClose, which is
    // bad.  OnClose and OnOk are wrongly documented i think, these are the
    // protos defined in the wxwin src these methods arent even virtual, so i
    // dont know why even OnClose is called. it must be because of event table

    // so i guess the right way is to override wxPropertyListFrame instead,
    // and make evt macros for the buttons like this:

    // BEGIN_EVENT_TABLE(wxPropertyListDialog, wxDialog)
    //   EVT_BUTTON(wxID_CANCEL, wxPropertyListDialog::OnCancel)
    //   EVT_CLOSE(wxPropertyListDialog::OnCloseWindow)
    // END_EVENT_TABLE()

    // BEGIN_EVENT_TABLE(wxPropertyListFrame, wxFrame)
    //   EVT_CLOSE(wxPropertyListFrame::OnCloseWindow)
    // END_EVENT_TABLE()

    // the OnCloseWindow should probably be also in the overriden event table,
    // since event tables don't seem to be inherited.

    // This means all OnClose methods should be renamed onok. this should be
    // called from the tPropertyListDlg::onok, so then there wouldnt be need
    // for the myproplist class, for this purpose at least

    // or it might be possible to still override here, with the proper evt tbl
    virtual bool OnClose();
    virtual void OnOk(wxCommandEvent& event);
    virtual void OnCancel(wxCommandEvent& event);

    virtual void OnPropertyChanged(wxProperty* pProperty);

  protected:

    tPropertyListDlg* parent;

};

myproplistview::myproplistview(tPropertyListDlg* pParent)
  : wxPropertyListView(
      NULL,
      wxPROP_BUTTON_OK | wxPROP_BUTTON_CANCEL | wxPROP_BUTTON_CHECK_CROSS |
        wxPROP_DYNAMIC_VALUE_FIELD | wxPROP_PULLDOWN | wxPROP_SHOWVALUES)
{
  parent = pParent;
}

void myproplistview::OnPropertyChanged(wxProperty* pProperty)
{
  cout << "propchange " << pProperty->GetValue().StringValue() << endl;
  parent->OnPropertyChanged(pProperty);
}

void myproplistview::OnOk(wxCommandEvent& event)
{
  cout << "myproplistview::OnOk" << endl;
  wxPropertyListView::OnOk(event);
}

void myproplistview::OnCancel(wxCommandEvent& event)
{
  cout << "myproplistview::OnCancel" << endl;
  wxPropertyListView::OnCancel(event);
}

// propagate the close event to the parent
bool myproplistview::OnClose()
{
  cout << "myproplistview::OnClose" << endl;

  parent->OnClose();

  // i guess we should call the base class OnClose also

  return false;
}

void tPropertyListDlg::OnPropertyChanged(wxProperty* pProperty)
{
  cout << "tPropertyListDlg::OnPropertyChanged parent propchange" << endl;
}

// PAT - Started adding CreateModal.  It doesn't end the modal dialog anywhere
// yet.  When the dialog is closed, it appears to call
// myproplistview::OnClose, but it doesn't call tPropertyListDlg::OnClose for
// some reason.

void tPropertyListDlg::CreateModal()
{
  sheet = new wxPropertySheet;
  view = new myproplistview(this);

  view->AddRegistry(myListValidatorRegistry);

  // addproperties is meant to be virtual and cannot be called in constructor
  AddProperties();

  wxPropertyListDialog *propDialog = new wxPropertyListDialog(
    view,
    NULL,
    title,
    wxDefaultPosition,
    wxSize(400, 500));
  view->ShowView(sheet, (wxPanel*)propDialog);

  propDialog->Centre(wxBOTH);
  propDialog->ShowModal();
}

void tPropertyListDlg::Create()
{
  sheet = new wxPropertySheet;
  view = new myproplistview(this);

//  wxDialog *propDialog = new wxPropertyListDialog(
//    view,
//    NULL,
//    title,
//    wxDefaultPosition,
//    wxSize(300, 200),
//    wxDEFAULT_DIALOG_STYLE | wxDIALOG_MODELESS);

  view->AddRegistry(myListValidatorRegistry);

  // addproperties is meant to be virtual and cannot be called in constructor
  AddProperties();

//  pView->ShowView(sheet, (wxPanel *)propDialog);
//  propDialog->Centre(wxBOTH);
//  //  propDialog->Show(true);
//  propDialog->Show(true);

  wxPropertyListFrame* propFrame = new wxPropertyListFrame(
    view,
    NULL,
    title,
    wxDefaultPosition,
    wxSize(400, 500));

  propFrame->Initialize();

  view->ShowView(sheet, propFrame->GetPropertyPanel());

  propFrame->Centre(wxBOTH);
  propFrame->Show(true);
}

tPropertyListDlg::tPropertyListDlg(wxString title)
{
  this->title = title;

  // The validators can be registered once for all.
//  tPropertyListDlg::myListValidatorRegistry = 0;

//  if (myListValidatorRegistry == 0)
//  {
    myListValidatorRegistry=new wxPropertyValidatorRegistry();
    myListValidatorRegistry->RegisterValidator(
      (wxString)"string",
      new wxStringListValidator);
    myListValidatorRegistry->RegisterValidator(
      (wxString)"real",
      new wxRealListValidator);
    myListValidatorRegistry->RegisterValidator(
      (wxString)"integer",
      new wxIntegerListValidator);
    myListValidatorRegistry->RegisterValidator(
      (wxString)"bool",
      new wxBoolListValidator);
    myListValidatorRegistry->RegisterValidator(
      (wxString)"stringlist",
      new wxListOfStringsListValidator);
//  }
}

bool tPropertyListDlg::OnClose()
{
  cout << "tPropertyListDlg::OnClose" << endl;
  return false;
}

// add properties in subclasses here
void tPropertyListDlg::AddProperties()
{
  cout
    << "tPropertyListDlg::AddProperties should never be called, override!"
    << endl;
}

/////////////////////////////////////////////////////////////
// in constructor: init the delegate with strings from the nam/value array

// map integer on string and show it

// map string on integer and store it

tNamedValueListValidator::tNamedValueListValidator(tNamedValue *v)
{
  Values = v;
  wxStringList* stringlist= new wxStringList();
  for (int i = 0; Values[i].Name; i++)
  {
    // omit empty entries
    if (*Values[i].Name)
    {
      stringlist->Add(wxString(Values[i].Name));
    }
  }

  // copied from base(cant call base constructors)
  m_strings = stringlist;

  // If no constraint, we just allow the string to be edited.
  if (!m_strings && ((m_validatorFlags & wxPROP_ALLOW_TEXT_EDITING) == 0))
  {
    m_validatorFlags |= wxPROP_ALLOW_TEXT_EDITING;
  }
}

tNamedValueListValidator::~tNamedValueListValidator()
{
  //wxStringListValidator::~wxStringListValidator(); hmm...
}

int tNamedValueListValidator::MapName2Value(const char* Selection)
{
  int i;
  int Result;

  if (Selection)
  {
    for (i = 0; Values[i].Name; i++)
    {
      if (!strcmp(Selection, Values[i].Name))
      {
	Result = Values[i].Value;
	break;
      }
    }
  }
  return Result;
}


wxString tNamedValueListValidator::MapValue2Name(int value)
{
  int i;

  for (i = 0; Values[i].Name; i++)
  {
    if (value == Values[i].Value)
    {
      return wxString(Values[i].Name);
    }
  }
  return ::wxEmptyString;
}

// it appears not to be possible to get the validator to provide the value to
// be displayed in the property list.
// wxPropertyValue.GetStringRepresentation() is used for that.

// fetch property value and display it
bool tNamedValueListValidator::OnDisplayValue(
  wxProperty* pProperty,
  wxPropertyListView* pView,
  wxWindow* pParent)
{
  if (!pView->GetValueText())
  {
    return false;
  }

  // fetch the int value of the property, convert it to a name
  wxString str=MapValue2Name(pProperty->GetValue().IntegerValue());

  pView->GetValueText()->SetValue(str);
  cout << "tNamedValueListValidator::OnDisplayValue " << str <<endl;
  if (
    m_strings &&
    pView->GetValueList() &&
    pView->GetValueList()->IsShown() &&
    pView->GetValueList()->GetCount() > 0)
  {
    pView->GetValueList()->SetStringSelection(str);
  }
  return true;

//  return wxStringListValidator::OnDisplayValue(pProperty, pView, pParent);
}

// update the property from the control
bool tNamedValueListValidator::OnRetrieveValue(
  wxProperty* pProperty,
  wxPropertyListView* pView,
  wxWindow* pParent)
{
  if (!pView->GetValueText())
  {
    return false;
  }
  wxString value(pView->GetValueText()->GetValue());
  pProperty->GetValue() = (long)MapName2Value(value);
  cout
    << "tNamedValueListValidator::OnRetrieveValue "
    << pProperty->GetValue().IntegerValue()
    << endl;
//  pView->GetValueText()->SetValue("apa!");

  return true;

//  return wxStringListValidator::OnRetrieveValue(pProperty, pView, pParent);
}


bool tNamedValueListValidator::OnSelect(
  bool select,
  wxProperty* pProperty,
  wxPropertyListView* pView,
  wxWindow* pParent)
{
  return wxStringListValidator::OnSelect(select, pProperty, pView, pParent);
}

bool tNamedValueListValidator::OnDoubleClick(
  wxProperty* pProperty,
  wxPropertyListView* pView,
  wxWindow* pParent)
{
  return wxStringListValidator::OnDoubleClick(pProperty, pView, pParent);
}

bool tNamedValueListValidator::OnValueListSelect(
  wxProperty* pProperty,
  wxPropertyListView* pView,
  wxWindow* pParent)
{
  wxString s(pView->GetValueList()->GetStringSelection());
  cout <<"tNamedValueListValidator::OnValueListSelect "<<s<<endl;
  if (s != wxT(""))
  {
    pView->GetValueText()->SetValue(s);
    pView->RetrieveProperty(pProperty); //i think this displays the prop
  }
  return true;

//  return wxStringListValidator::OnValueListSelect(pProperty, pView, pParent);
}

bool tNamedValueListValidator::OnPrepareControls(
  wxProperty* pProperty,
  wxPropertyListView* pView,
  wxWindow* pParent)
{
  return wxStringListValidator::OnPrepareControls(pProperty, pView, pParent);
}

bool tNamedValueListValidator::OnClearControls(
  wxProperty* pProperty,
  wxPropertyListView* pView,
  wxWindow* pParent)
{
  return wxStringListValidator::OnClearControls(pProperty, pView, pParent);
}

bool tNamedValueListValidator::OnPrepareDetailControls(
  wxProperty* pProperty,
  wxPropertyListView* pView,
  wxWindow* pParent)
{
  return wxStringListValidator::OnPrepareDetailControls(
    pProperty,
    pView,
    pParent);
}

bool tNamedValueListValidator::OnClearDetailControls(
  wxProperty* pProperty,
  wxPropertyListView* pView,
  wxWindow* pParent)
{
  return wxStringListValidator::OnClearDetailControls(
    pProperty,
    pView,
    pParent);
}

void tNamedValueListValidator::OnEdit(
  wxProperty* pProperty,
  wxPropertyListView* pView,
  wxWindow* pParent)
{
  return wxStringListValidator::OnEdit(pProperty, pView, pParent);
}

bool tNamedValueListValidator::OnCheckValue(
  wxProperty* pProperty,
  wxPropertyListView* pView,
  wxWindow* pParent)
{
  return wxStringListValidator::OnCheckValue(pProperty, pView, pParent);
}
