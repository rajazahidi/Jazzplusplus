#include "WxWidgets.h"

#include "NamedValue.h"

tNamedValue::tNamedValue()
  : Name(""),
    Value(0)
{
}

tNamedValue::tNamedValue(char* pName, long v)
  : Name(pName),
    Value(v)
{
}
