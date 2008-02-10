#include "WxWidgets.h"

#include "NamedChoice.h"
#include "NamedValue.h"
#include "DeprecatedStringUtils.h"
#include "DeprecatedWx/proplist.h"

tNamedChoice::tNamedChoice(char* pTitle, tNamedValue* pValues, long* pResult)
  : mpTitle(pTitle),
    mpValues(pValues),
    mpSelection(0),
    mpResult(pResult)
{
}

tNamedChoice::~tNamedChoice()
{
  delete mpSelection;
}

#ifdef OBSOLETE
wxFormItem *tNamedChoice::mkFormItem(int w, int h)
{
  SetValue();

  // following adapted from wxwin/src/base/wb_form.cc
  wxList *list = new wxList;
  for (int i = 0; mpValues[i].Name; i++)
    if (*mpValues[i].Name)	// omit empty entries
      list->Append((wxObject *)copystring(mpValues[i].Name));

  wxFormItemConstraint *constraint = wxMakeConstraintStrings(list);
  return wxMakeFormString(mpTitle, &mpSelection, wxFORM_SINGLE_LIST, new wxList(constraint, 0),
    0, 0, w, h);
}
#endif

//  Return a string list validator to use in the wxproplist dialogs.
wxStringListValidator* tNamedChoice::GetStringListValidator()
{
  wxStringList* StringList = new wxStringList();
  for (int i = 0; mpValues[i].Name; ++i)
  {
    // Omit empty entries.
    if (*mpValues[i].Name)
    {
      StringList->Add(wxString(mpValues[i].Name));
    }
  }
  return new wxStringListValidator(StringList);
}

void tNamedChoice::GetValue()
{
  int i;

  if (mpSelection)
  {
    for (i = 0; mpValues[i].Name; ++i)
    {
      if (!strcmp(mpSelection, mpValues[i].Name))
      {
	*mpResult = mpValues[i].Value;
	break;
      }
    }
  }
}

void tNamedChoice::SetValue()
{
  for (int i = 0; mpValues[i].Name; ++i)
  {
    if (*mpResult == mpValues[i].Value)
    {
      delete mpSelection;
      mpSelection = copystring(mpValues[i].Name);
      break;
    }
  }
}
