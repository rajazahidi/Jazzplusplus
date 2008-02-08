#include "WxWidgets.h"

#include "NamedValue.h"
#include "DeprecatedStringUtils.h"
#include "DeprecatedWx/proplist.h"

tNamedChoice::tNamedChoice(char *t, tNamedValue *v, long *r)
{
  Values = v;
  Selection = 0;
  Title = t;
  Result = r;
}


#ifdef OBSOLETE
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
