//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Auto-Accompaniment (Smart Band) Dialog
//*****************************************************************************

#pragma once

#include <wx/dialog.h>
#include "../AutoAccompaniment.h"

class wxChoice;
class wxCheckBox;
class wxRadioButton;
class wxSpinCtrl;
class JZProject;

//*****************************************************************************
//*****************************************************************************
class JZAutoAccompanimentDialog : public wxDialog
{
  public:

    JZAutoAccompanimentDialog(
      wxWindow* pParent,
      JZProject* pProject,
      int defaultSourceTrack = 0);

    virtual ~JZAutoAccompanimentDialog();

    const JZAccompanimentConfig& GetConfig() const
    {
      return mConfig;
    }

  private:

    void CreateControls();
    void OnRangeRadio(wxCommandEvent& Event);
    void OnGenerateButton(wxCommandEvent& Event);
    void OnCancelButton(wxCommandEvent& Event);

  private:

    JZProject* mpProject;
    JZAccompanimentConfig mConfig;
    int mDefaultSourceTrack;

    wxChoice* mpSourceTrackChoice;
    wxChoice* mpStyleChoice;

    wxCheckBox* mpBassCheckBox;
    wxCheckBox* mpDrumsCheckBox;
    wxCheckBox* mpPadCheckBox;

    wxRadioButton* mpRangeAllRadio;
    wxRadioButton* mpRangeCustomRadio;
    wxSpinCtrl* mpFromBarSpin;
    wxSpinCtrl* mpToBarSpin;

    wxCheckBox* mpOverwriteCheckBox;

  DECLARE_EVENT_TABLE();
};
