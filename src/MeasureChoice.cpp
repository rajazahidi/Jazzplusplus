#include "WxWidgets.h"

#include "MeasureChoice.h"
#include "Song.h"
#include "Globals.h"

tMeasureChoice::tMeasureChoice(wxWindow* pParent)
  : tNamedValueChoice(pParent, limitSteps)
{
}

long tMeasureChoice::GetTicks(JZSong* pSong)
{
  long m = GetMeasure();
  if (m < 0)
  {
    m = 16;
  }
  m = pSong->TicksPerQuarter * 4 / m;
  return (m > 0) ? m : 1;
}
