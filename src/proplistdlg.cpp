 
#include "wx/wx.h"

#include "proplistdlg.h"

#include <stdlib.h>
#include <ctype.h>
#include <string.h>




////////////////////////////////////////////////////////////////7
/** just wrap the wxPropertyListView so we can intercept the callbacks*/
class myproplistview : public wxPropertyListView
{
protected:
  tPropertyListDlg* parent;
public:
  myproplistview(tPropertyListDlg* parent);

  //the onok and onclose handlers are never called, only onclose, which is bad.
  // onclose and onok are wrongly documented i think, these are the protos defined in the wxwin src
  //  these methods arent even virtual, so i dont know why even onclose is called. it must be because of event table
  
  //soo i gues the right way is to override wxPropertyListFrame instead, and make evt macros for the buttons like this:

  // BEGIN_EVENT_TABLE(wxPropertyListDialog, wxDialog)
  //     EVT_BUTTON(wxID_CANCEL,                wxPropertyListDialog::OnCancel)
  //     EVT_CLOSE(wxPropertyListDialog::OnCloseWindow)
  // END_EVENT_TABLE()

// BEGIN_EVENT_TABLE(wxPropertyListFrame, wxFrame)
//     EVT_CLOSE(wxPropertyListFrame::OnCloseWindow)
// END_EVENT_TABLE()

  //the onclosewindow thould probably be also in the overriden evt table, sine evt tables dont seem to be inherited

  //this means all onclose methods should be renamed onok. this should be called from the tPropertyListDlg::onok, 
  //so then there wouldnt be need for the myproplist class, for this purpose at least

  //or it might be possible to still override here, with the proper evt tbl 
    virtual bool OnClose();
  virtual void OnOk(wxCommandEvent& event);
  virtual void OnCancel(wxCommandEvent& event);

  virtual void OnPropertyChanged(wxProperty *property);
};


myproplistview::myproplistview(tPropertyListDlg* parent) : wxPropertyListView(NULL,
						      wxPROP_BUTTON_OK | wxPROP_BUTTON_CANCEL |
		 				      wxPROP_BUTTON_CHECK_CROSS|wxPROP_DYNAMIC_VALUE_FIELD|wxPROP_PULLDOWN|wxPROP_SHOWVALUES)
{
  this->parent=parent;
}

void myproplistview::OnPropertyChanged(wxProperty *property)
{
  printf("propchange %s\n",  property->GetValue().StringValue());
  parent->OnPropertyChanged(property);
}

void myproplistview::OnOk(wxCommandEvent& event){
  cout<<"myproplistview::OnOk"<<endl;
  wxPropertyListView::OnOk(event);
}

void myproplistview::OnCancel(wxCommandEvent& event){
  cout<<"myproplistview::OnCancel"<<endl;
  wxPropertyListView::OnCancel(event);
}


/** propagate the close event to the parent*/
bool myproplistview::OnClose(){
  printf("myproplistview::OnClose\n");
  parent->OnClose();
  //i guess we should call the base class onclose also
  return FALSE;
}
////////////////////////////////////////////////////////////////7


void tPropertyListDlg::OnPropertyChanged(wxProperty *property)
{
  printf("tPropertyListDlg::OnPropertyChanged parent propchange\n");
}

// PAT - Started adding CreateModal.  It doesn't end the modal dialog anywhere
// yet.  When the dialog is closed, it appears to call myproplistview::OnClose,
// but it doesn't call tPropertyListDlg::OnClose for some reason.

void tPropertyListDlg::CreateModal() {
  sheet = new wxPropertySheet;
  view =    new myproplistview(this);
  
  view->AddRegistry(myListValidatorRegistry);

  //adproperties is meant to be virtual and cannot be called in constructor
  AddProperties();

  wxPropertyListDialog *propDialog = new wxPropertyListDialog(view, NULL, title, wxPoint(-1, -1), wxSize(400, 500));
  view->ShowView(sheet, (wxPanel*)propDialog);
  
  propDialog->Centre(wxBOTH);
  propDialog->ShowModal();
}

void tPropertyListDlg::Create(){
  sheet = new wxPropertySheet;
  view =    new myproplistview(this);
  
//   wxDialog *propDialog     = new wxPropertyListDialog(view, NULL, title,
// 						wxPoint(-1, -1), wxSize(300, 200), wxDEFAULT_DIALOG_STYLE|wxDIALOG_MODELESS);




  view->AddRegistry(myListValidatorRegistry);

  //adproperties is meant to be virtual and cannot be called in constructor
  AddProperties();

//   view->ShowView(sheet, (wxPanel *)propDialog);
//   propDialog->Centre(wxBOTH);
//   //    propDialog->Show(TRUE);
//   propDialog->Show(TRUE);


  wxPropertyListFrame* propFrame = new wxPropertyListFrame(view, NULL, title, wxPoint(-1, -1), wxSize(400, 500));
  propFrame->Initialize();
  view->ShowView(sheet, propFrame->GetPropertyPanel());
  
  propFrame->Centre(wxBOTH);
  propFrame->Show(TRUE);


}


tPropertyListDlg::tPropertyListDlg(wxString title)
{
  this->title=title;
  //the validators can be registered once for all
//tPropertyListDlg::myListValidatorRegistry=NULL;

  //  if(myListValidatorRegistry==NULL){
      myListValidatorRegistry=new wxPropertyValidatorRegistry();
      myListValidatorRegistry->RegisterValidator((wxString)"string", new wxStringListValidator); 
   myListValidatorRegistry->RegisterValidator((wxString)"real", new wxRealListValidator);
   myListValidatorRegistry->RegisterValidator((wxString)"integer", new wxIntegerListValidator);
   myListValidatorRegistry->RegisterValidator((wxString)"bool", new wxBoolListValidator);
   myListValidatorRegistry->RegisterValidator((wxString)"stringlist", new wxListOfStringsListValidator);
   //}

}


bool tPropertyListDlg::OnClose(){
   printf("tPropertyListDlg::OnClose\n");
   return FALSE;
}

/**add properties in subclasses here*/
void tPropertyListDlg::AddProperties(){
   printf("tPropertyListDlg::AddProperties should never be called, override!\n");
}

/////////////////////////////////////////////////////////////
//in constructor: init the delegate with strings from the nam/value array

//map integer on string and show it

//map string on integer and store it

tNamedValueListValidator::tNamedValueListValidator(tNamedValue *v)
{
  Values = v;
  wxStringList* stringlist= new wxStringList();
  for (int i = 0; Values[i].Name; i++)
    if (*Values[i].Name)	// omit empty entries
      stringlist->Add(wxString(Values[i].Name));

  //copied from base(cant call base constructors)
  m_strings = stringlist;
  // If no constraint, we just allow the string to be edited.
  if (!m_strings && ((m_validatorFlags & wxPROP_ALLOW_TEXT_EDITING) == 0))
    m_validatorFlags |= wxPROP_ALLOW_TEXT_EDITING;
}

tNamedValueListValidator::~tNamedValueListValidator(void){
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
}

//it appears not  to be possible to get the validator to provide the value to be displayed in the property list.
//the wxPropertyValue.GetStringRepresentation() is used for that.   

/** fetch property value and display it*/
 bool tNamedValueListValidator::OnDisplayValue(wxProperty *property, wxPropertyListView *view, wxWindow *parentWindow){
  if (!view->GetValueText())
    return FALSE;

  wxString str=MapValue2Name( property->GetValue().IntegerValue()) ; //fetch the int value of the property, convert it to a name

  view->GetValueText()->SetValue(str);
  cout<<"tNamedValueListValidator::OnDisplayValue "<<str<<endl;
  if (m_strings && view->GetValueList() && view->GetValueList()->IsShown() && view->GetValueList()->GetCount() > 0)
  {
    view->GetValueList()->SetStringSelection(str);
  }
  return TRUE;
  //return wxStringListValidator::OnDisplayValue(property, view, parentWindow);
}

/** update the property from the control*/
 bool tNamedValueListValidator::OnRetrieveValue(wxProperty *property, wxPropertyListView *view, wxWindow *parentWindow){
  if (!view->GetValueText())
    return FALSE;
  wxString value(view->GetValueText()->GetValue());
  property->GetValue() = (long)MapName2Value(value) ;
  cout <<"tNamedValueListValidator::OnRetrieveValue "<<property->GetValue().IntegerValue()<<endl;
  //view->GetValueText()->SetValue("apa!");
  return TRUE;
  //  return wxStringListValidator::OnRetrieveValue(property, view, parentWindow);
}


 bool tNamedValueListValidator::OnSelect(bool select, wxProperty *property, wxPropertyListView *view, wxWindow *parentWindow){
  return wxStringListValidator::OnSelect( select, property,  view, parentWindow);
}
 bool tNamedValueListValidator::OnDoubleClick(wxProperty *property, wxPropertyListView *view, wxWindow *parentWindow ){
  return wxStringListValidator::OnDoubleClick(property, view, parentWindow );
}
 bool tNamedValueListValidator::OnValueListSelect(wxProperty *property, wxPropertyListView *view, wxWindow *parentWindow){
  wxString s(view->GetValueList()->GetStringSelection());
  cout <<"tNamedValueListValidator::OnValueListSelect "<<s<<endl;
  if (s != wxT(""))
  {
    view->GetValueText()->SetValue(s);
    view->RetrieveProperty(property); //i think this displays the prop
  }
  return TRUE;
  //return wxStringListValidator::OnValueListSelect(property, view, parentWindow);
}
 bool tNamedValueListValidator::OnPrepareControls( wxProperty *property, wxPropertyListView *view, wxWindow *parentWindow ){
  return wxStringListValidator::OnPrepareControls( property, view, parentWindow );
}
 bool tNamedValueListValidator::OnClearControls(wxProperty *property, wxPropertyListView *view, wxWindow *parentWindow){
  return wxStringListValidator::OnClearControls(property, view, parentWindow);
}
 bool tNamedValueListValidator::OnPrepareDetailControls( wxProperty *property, wxPropertyListView *view, wxWindow *parentWindow){
  return wxStringListValidator::OnPrepareDetailControls(  property, view, parentWindow );
}
 bool tNamedValueListValidator::OnClearDetailControls( wxProperty *property, wxPropertyListView *view, wxWindow *parentWindow){
  return wxStringListValidator::OnClearDetailControls( property, view, parentWindow );
}
 void tNamedValueListValidator::OnEdit(wxProperty *property, wxPropertyListView *view, wxWindow *parentWindow){
  return wxStringListValidator::OnEdit(property, view, parentWindow);
}
 bool tNamedValueListValidator::OnCheckValue( wxProperty *property, wxPropertyListView *view, wxWindow *parentWindow ){
  return wxStringListValidator::OnCheckValue( property, view, parentWindow );
}
