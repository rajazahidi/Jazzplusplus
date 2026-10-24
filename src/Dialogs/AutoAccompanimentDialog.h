//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Auto-Accompaniment & Smart Music Intelligence Dialog (Pro Edition)
//
// Integrated Tabbed Hub for Session Band, Strumming, Plucking, Riffs & Solo
// with Live Audition, Groove Swing, Smart Drum Fills & Expressive Soloing.
//*****************************************************************************

#pragma once

#include <wx/dialog.h>
#include <wx/timer.h>
#include "../AutoAccompaniment.h"

class wxChoice;
class wxCheckBox;
class wxRadioButton;
class wxSpinCtrl;
class wxSlider;
class wxNotebook;
class wxButton;
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
    wxPanel* CreateBandTab(wxWindow* pParent);
    wxPanel* CreateStrumPluckTab(wxWindow* pParent);
    wxPanel* CreateRiffTab(wxWindow* pParent);
    wxPanel* CreateSoloTab(wxWindow* pParent);

    void OnNotebookPageChanged(wxBookCtrlEvent& Event);
    void OnRangeRadio(wxCommandEvent& Event);
    void OnSimultaneousCheck(wxCommandEvent& Event);
    void OnAuditionPreview(wxCommandEvent& Event);
    void OnStopPreview(wxCommandEvent& Event);
    void OnGenerateButton(wxCommandEvent& Event);
    void OnExportWav(wxCommandEvent& Event);
    void OnExportStems(wxCommandEvent& Event);
    void OnExportLeadSheet(wxCommandEvent& Event);
    void OnInjectProgression(wxCommandEvent& Event);
    void OnCancelButton(wxCommandEvent& Event);

    void UpdateGenerateButtonLabel();
    void StopAudition();

  private:

    JZProject* mpProject;
    JZAccompanimentConfig mConfig;
    int mDefaultSourceTrack;
    bool mIsAuditioning;

    // Header Controls
    wxChoice* mpSourceTrackChoice;
    wxStaticText* mpKeyCenterLabel;
    wxRadioButton* mpRangeAllRadio;
    wxRadioButton* mpRangeCustomRadio;
    wxSpinCtrl* mpFromBarSpin;
    wxSpinCtrl* mpToBarSpin;

    // Song Starter Chord Progression
    wxChoice* mpProgressionChoice;
    wxButton* mpInjectProgressionButton;

    // Notebook Hub
    wxNotebook* mpNotebook;

    // Tab 1: Band Accompaniment
    wxChoice* mpBandStyleChoice;
    wxChoice* mpSectionChoice;
    wxChoice* mpBassFeelChoice;
    wxCheckBox* mpBassCheckBox;
    wxCheckBox* mpDrumsCheckBox;
    wxCheckBox* mpPadCheckBox;

    // Tab 2: Strum & Pluck
    wxChoice* mpStrumStyleChoice;
    wxChoice* mpStrumTrackChoice;
    wxChoice* mpStrumPatchChoice;
    wxSlider* mpStrumSpeedSlider;

    // Tab 3: Riffs & Hooks
    wxChoice* mpRiffStyleChoice;
    wxChoice* mpRiffTrackChoice;
    wxChoice* mpRiffPatchChoice;
    wxSpinCtrl* mpRiffOctaveSpin;

    // Tab 4: Auto-Solo Lead
    wxChoice* mpSoloStyleChoice;
    wxChoice* mpSoloDensityChoice;
    wxChoice* mpSoloTrackChoice;
    wxChoice* mpSoloPatchChoice;
    wxSpinCtrl* mpSoloOctaveSpin;

    // Musicality & Harmony Options
    wxSlider* mpSwingSlider;
    wxCheckBox* mpDrumFillsCheckBox;
    wxCheckBox* mpSoloBendsCheckBox;
    wxCheckBox* mpHumanizeVelCheckBox;
    wxCheckBox* mpSmoothVoiceLeadingCheckBox;
    wxCheckBox* mpGhostNotesCheckBox;
    wxCheckBox* mpMicroPocketCheckBox;
    wxCheckBox* mpScaleLockCheckBox;
    wxChoice* mpVelocityCurveChoice;
    wxCheckBox* mpOutroRitardandoCheckBox;
    wxCheckBox* mpMicrotonalCheckBox;
    wxChoice* mpMicrotonalChoice;
    wxCheckBox* mpMixerPresetsCheckBox;

    // Bottom Controls
    wxCheckBox* mpOverwriteCheckBox;
    wxCheckBox* mpSimultaneousCheckBox;
    wxButton* mpAuditionButton;
    wxButton* mpStopAuditionButton;
    wxButton* mpExportLeadSheetButton;
    wxButton* mpExportStemsButton;
    wxButton* mpExportWavButton;
    wxButton* mpGenerateButton;
    wxButton* mpCancelButton;

  DECLARE_EVENT_TABLE();
};
