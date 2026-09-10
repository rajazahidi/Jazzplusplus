//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Insert Chord and Scale Dialog Header
//*****************************************************************************

#pragma once

#include "../ChordScaleData.h"

#include <wx/dialog.h>

class JZProject;
class JZTrack;
class wxButton;
class wxChoice;
class wxListBox;
class wxRadioBox;
class wxSpinCtrl;
class wxStaticText;

class JZInsertChordScaleDialog : public wxDialog
{
  public:

    JZInsertChordScaleDialog(
      wxWindow* pParent,
      JZProject* pProject,
      int DefaultTrack = 1,
      int DefaultClock = 0);

    virtual ~JZInsertChordScaleDialog();

  private:

    void CreateControls();

    void PopulateCategories();

    void PopulateItems();

    void UpdateInfoLabel();

    void PlayPreview();

    void ExecuteInsert();

    // Event Handlers
    void OnModeChange(wxCommandEvent& Event);
    void OnCategoryChange(wxCommandEvent& Event);
    void OnItemSelect(wxCommandEvent& Event);
    void OnRootOrOctaveChange(wxCommandEvent& Event);
    void OnPreviewButton(wxCommandEvent& Event);
    void OnInsertButton(wxCommandEvent& Event);
    void OnCloseButton(wxCommandEvent& Event);

  private:

    JZProject* mpProject;
    int mTargetTrackIndex;
    int mStartClock;

    wxRadioBox* mpModeRadio;
    wxChoice* mpCategoryChoice;
    wxListBox* mpItemListBox;
    wxChoice* mpRootChoice;
    wxChoice* mpOctaveChoice;
    wxChoice* mpStyleChoice;
    wxChoice* mpDurationChoice;
    wxChoice* mpTrackChoice;
    wxSpinCtrl* mpVelocitySpin;
    wxStaticText* mpInfoText;

    wxButton* mpPreviewButton;
    wxButton* mpInsertButton;
    wxButton* mpCloseButton;

    DECLARE_EVENT_TABLE()
};
