#include "ControllerDialog.h"

#include "../Configuration.h"
#include "../Globals.h"

#include <wx/button.h>
#include <wx/listbox.h>
#include <wx/sizer.h>

using namespace std;

//*****************************************************************************
//*****************************************************************************
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(JZControllerDialog, wxDialog)
END_EVENT_TABLE()

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZControllerDialog::JZControllerDialog(wxWindow* pParent)
  : wxDialog(pParent, wxID_ANY, wxString("Controller")),
    mpControllerListBox(0)
{
  mpControllerListBox = new wxListBox(this, wxID_ANY);

  const vector<pair<string, int> >& VoiceNames = gpConfig->GetControlNames();
  for (
    vector<pair<string, int> >::const_iterator iName = VoiceNames.begin();
    iName != VoiceNames.end();
    ++iName)
  {
    mpControllerListBox->Append(iName->first.c_str());
  }

  wxButton* pOkButton = new wxButton(this, wxID_OK, "&OK");
  wxButton* pCancelButton = new wxButton(this, wxID_CANCEL, "Cancel");
  wxButton* pHelpButton = new wxButton(this, wxID_HELP, "Help");
  pOkButton->SetDefault();

  wxBoxSizer* pTopSizer = new wxBoxSizer(wxVERTICAL);

  pTopSizer->Add(mpControllerListBox, 0, wxGROW | wxALL, 2);

  wxBoxSizer* pButtonSizer = new wxBoxSizer(wxHORIZONTAL);

  pButtonSizer->Add(pOkButton, 0, wxALL, 5);
  pButtonSizer->Add(pCancelButton, 0, wxALL, 5);
  pButtonSizer->Add(pHelpButton, 0, wxALL, 5);

  pTopSizer->Add(pButtonSizer, 0, wxALIGN_CENTER | wxBOTTOM, 6);

  SetAutoLayout(true);
  SetSizer(pTopSizer);

  pTopSizer->SetSizeHints(this);
  pTopSizer->Fit(this);
}
