#include "MeterChangeDialog.h"

#include "../Globals.h"
#include "../Help.h"

#include <wx/button.h>
#include <wx/combobox.h>
#include <wx/sizer.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>

//*****************************************************************************
//*****************************************************************************
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(JZMeterChangeDialog, wxDialog)

  EVT_BUTTON(wxID_HELP, JZMeterChangeDialog::OnHelp)

END_EVENT_TABLE()

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZMeterChangeDialog::JZMeterChangeDialog(
  wxWindow* pParent,
  int DefaultBar,
  int DefaultNum,
  int DefaultDenom)
  : wxDialog(pParent, wxID_ANY, wxString("Meter / Time Signature Change")),
    mBar(DefaultBar > 0 ? DefaultBar : 1),
    mNumerator(DefaultNum > 0 ? DefaultNum : 4),
    mDenominator(DefaultDenom > 0 ? DefaultDenom : 4),
    mpBarSpinCtrl(0),
    mpNumeratorComboBox(0),
    mpDenominatorComboBox(0)
{
  wxBoxSizer* pTopSizer = new wxBoxSizer(wxVERTICAL);

  wxFlexGridSizer* pGridSizer = new wxFlexGridSizer(3, 2, 8, 12);
  pGridSizer->AddGrowableCol(1);

  pGridSizer->Add(new wxStaticText(this, wxID_ANY, "Bar Number:"), 0, wxALIGN_CENTER_VERTICAL);
  mpBarSpinCtrl = new wxSpinCtrl(this, wxID_ANY, wxString::Format("%d", mBar), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 9999, mBar);
  pGridSizer->Add(mpBarSpinCtrl, 1, wxEXPAND);

  pGridSizer->Add(new wxStaticText(this, wxID_ANY, "Numerator (Beats):"), 0, wxALIGN_CENTER_VERTICAL);
  mpNumeratorComboBox = new wxComboBox(this, wxID_ANY, wxString::Format("%d", mNumerator), wxDefaultPosition, wxDefaultSize, 0, 0, wxCB_READONLY);
  for (int i = 1; i <= 32; ++i)
  {
    mpNumeratorComboBox->Append(wxString::Format("%d", i));
  }
  mpNumeratorComboBox->SetValue(wxString::Format("%d", mNumerator));
  pGridSizer->Add(mpNumeratorComboBox, 1, wxEXPAND);

  pGridSizer->Add(new wxStaticText(this, wxID_ANY, "Denominator (Note Value):"), 0, wxALIGN_CENTER_VERTICAL);
  mpDenominatorComboBox = new wxComboBox(this, wxID_ANY, wxString::Format("%d", mDenominator), wxDefaultPosition, wxDefaultSize, 0, 0, wxCB_READONLY);
  mpDenominatorComboBox->Append("1");
  mpDenominatorComboBox->Append("2");
  mpDenominatorComboBox->Append("4");
  mpDenominatorComboBox->Append("8");
  mpDenominatorComboBox->Append("16");
  mpDenominatorComboBox->Append("32");
  mpDenominatorComboBox->SetValue(wxString::Format("%d", mDenominator));
  pGridSizer->Add(mpDenominatorComboBox, 1, wxEXPAND);

  pTopSizer->Add(pGridSizer, 0, wxEXPAND | wxALL, 15);

  wxBoxSizer* pButtonSizer = new wxBoxSizer(wxHORIZONTAL);
  wxButton* pOkButton = new wxButton(this, wxID_OK, "&OK");
  pOkButton->SetDefault();
  pButtonSizer->Add(pOkButton, 0, wxRIGHT, 8);
  pButtonSizer->Add(new wxButton(this, wxID_CANCEL, "Cancel"), 0, wxRIGHT, 8);
  pButtonSizer->Add(new wxButton(this, wxID_HELP, "Help"), 0);

  pTopSizer->Add(pButtonSizer, 0, wxALIGN_RIGHT | wxLEFT | wxRIGHT | wxBOTTOM, 12);

  SetSizer(pTopSizer);
  pTopSizer->Fit(this);
  CentreOnParent();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool JZMeterChangeDialog::TransferDataFromWindow()
{
  if (mpBarSpinCtrl)
  {
    mBar = mpBarSpinCtrl->GetValue();
    if (mBar < 1) mBar = 1;
  }
  if (mpNumeratorComboBox)
  {
    long num = 4;
    mpNumeratorComboBox->GetValue().ToLong(&num);
    mNumerator = num > 0 ? static_cast<int>(num) : 4;
  }
  if (mpDenominatorComboBox)
  {
    long den = 4;
    mpDenominatorComboBox->GetValue().ToLong(&den);
    mDenominator = den > 0 ? static_cast<int>(den) : 4;
  }
  return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZMeterChangeDialog::OnHelp(wxCommandEvent& Event)
{
  JZHelp::Instance().ShowTopic("Meterchange");
}
