#ifndef JZ_MEASURECHOICE_H
#define JZ_MEASURECHOICE_H

#include "NamedValueChoice.h"

class JZSong;

class tMeasureChoice : public tNamedValueChoice
{
  public:

    tMeasureChoice(wxWindow* pParent);

   long GetMeasure();

   void SetMeasure(long Measure);

   long GetTicks(JZSong* pSong);
};

inline
long tMeasureChoice::GetMeasure()
{
  return GetValue();
}

inline
void tMeasureChoice::SetMeasure(long Measure)
{
  SetValue(Measure);
}

#endif // !defined(JZ_MEASURECHOICE_H)
