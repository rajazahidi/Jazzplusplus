/**
JAVE

the proplistdlg class is meant as a convenience wrapper around the wxwin2 class wxPropertyListForm.

these dialogs arent exactly beautiful but are meant to make it simpler to port simple jazz dialogs to 
wx2. many dialogs in jazz are more advanced than can be handled by this class.

in the future this class might be used to implement wxPropertyForms, who can be better looking.
*/

#ifndef PROPLISTDLG_H
#define PROPLISTDLG_H

#include "util.h"
#include "wx/proplist.h"
#include "wx/propform.h"


class tNamedValueListValue : public wxPropertyValue
{
  DECLARE_DYNAMIC_CLASS(tNamedValueListValue)
  tNamedValue* nval;
public:
  tNamedValueListValue::tNamedValueListValue(void):wxPropertyValue(){}
  tNamedValueListValue::tNamedValueListValue(long* val, tNamedValue* nval):wxPropertyValue(val)
  {
    this->nval=nval;
  }

  //overriding these doesnt really work in the display area
/*   virtual wxString GetStringRepresentation(void){ */
/*     cout <<"GetStringRepresentation"<<endl; */
/*     return wxString("apa"); */
/*   } */
/*   int IntegerValue(){ */
/*     return 42; */
/*   } */

};


class tPropertyListDlg
{
 public:
  tPropertyListDlg(wxString title);
  virtual void AddProperties();
  void Create();
  void CreateModal();
  virtual bool OnClose();
  virtual void OnPropertyChanged(wxProperty *property);
 protected:
  wxPropertySheet *sheet;
  wxPropertyListView *view;
  wxPropertyValidatorRegistry *myListValidatorRegistry;//cannot be a temporary! leads to a crash
  wxString title;

};

/** a list validator class for name/value pair arrays

here is an example:

tNamedValue limitSteps[] =
{
  tNamedValue( "1/96", 96 ),
  tNamedValue( "1/192", 192 ),
  tNamedValue(   0,      1  )
};

*/
class tNamedValueListValidator : public wxStringListValidator
{
    tNamedValue *Values;
 public:
  tNamedValueListValidator(tNamedValue *v);
  int MapName2Value(const char* Selection);
  wxString MapValue2Name(int value);
  
  ~tNamedValueListValidator(void);
  virtual bool OnSelect(bool select, wxProperty *property, wxPropertyListView *view, wxWindow *parentWindow);
  virtual bool OnDoubleClick(wxProperty *WXUNUSED(property), wxPropertyListView *WXUNUSED(view), wxWindow *WXUNUSED(parentWindow) );
  virtual bool OnValueListSelect(wxProperty *property, wxPropertyListView *view, wxWindow *parentWindow);
  virtual bool OnPrepareControls( wxProperty *WXUNUSED(property), wxPropertyListView *WXUNUSED(view), wxWindow *WXUNUSED(parentWindow) );
  virtual bool OnClearControls(wxProperty *property, wxPropertyListView *view, wxWindow *parentWindow);
  virtual bool OnPrepareDetailControls( wxProperty *WXUNUSED(property), wxPropertyListView *WXUNUSED(view), wxWindow *WXUNUSED(parentWindow) );
  virtual bool OnClearDetailControls( wxProperty *WXUNUSED(property), wxPropertyListView *WXUNUSED(view), wxWindow *WXUNUSED(parentWindow) );
  virtual void OnEdit(wxProperty *property, wxPropertyListView *view, wxWindow *parentWindow);
  virtual bool OnCheckValue( wxProperty *WXUNUSED(property), wxPropertyListView *WXUNUSED(view), wxWindow *WXUNUSED(parentWindow) );
  virtual bool OnRetrieveValue(wxProperty *property, wxPropertyListView *view, wxWindow *parentWindow);
  virtual bool OnDisplayValue(wxProperty *property, wxPropertyListView *view, wxWindow *parentWindow);

};
#endif
