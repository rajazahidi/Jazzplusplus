#include "WxWidgets.h"

#include "NamedValueChoice.h"
#include "NamedValue.h"

//*****************************************************************************
//*****************************************************************************
tNamedValueChoice::tNamedValueChoice(
  wxWindow* pParent,
  tNamedValue* nvals)
  : wxChoice(pParent, wxID_ANY),
    values(nvals) //JAVE label should also be in the wxChoice constructor call
{
  tNamedValue *v = values;
  while (v->Name)
  {
    Append(v->Name);
    ++v;
  }
}

long tNamedValueChoice::GetValue()
{
  int i = GetSelection();
  if (i >= 0)
  {
    return values[i].Value;
  }
  return 16;
}


void tNamedValueChoice::SetValue(long measure)
{
  int i;
  for (i = 0; values[i].Name; i++)
  {
    if (values[i].Value == measure)
    {
      SetSelection(i);
      break;
    }
  }
}
