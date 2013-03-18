#pragma once

#include <wx/control.h>

class JZRndArray;

//*****************************************************************************
//*****************************************************************************
class JZArrayControl : public wxControl
{
  public:

    JZArrayControl(
      wxWindow* pParent,
      wxWindowID Id,
      const JZRndArray& RandomArray,
      const wxPoint& Position = wxDefaultPosition,
      const wxSize& Size = wxSize(40, 40),
      long WindowStyle = wxNO_BORDER,
      const wxValidator& Validator = wxDefaultValidator,
      const wxString& Name = wxT("arraycontrol"));

    virtual ~JZArrayControl();

    void Create(
      wxWindow* pParent,
      wxWindowID Id,
      const JZRndArray& RandomArray,
      const wxPoint& Position = wxDefaultPosition,
      const wxSize& Size = wxSize(40, 40),
      long WindowStyle = wxNO_BORDER,
      const wxValidator& Validator = wxDefaultValidator,
      const wxString& Name = wxT("arraycontrol"));

  private:

    void OnSize(wxSizeEvent& Event);

    void OnPaint(wxPaintEvent& Event);

  private:

    JZRndArray* mpRandomArray;

  DECLARE_EVENT_TABLE()
};
