//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// RealTracksDialog.h - Band-in-a-Box RealTracks & RealDrums Studio Pro Dialog
//
// Interactive control center for RealDrums acoustic kit triggering,
// RealTracks harmonic phrase accompaniment, SoundFont sound bank loading,
// and physical acoustic modeling convolution.
//*****************************************************************************

#pragma once

#include <wx/dialog.h>
#include <wx/timer.h>
#include "../RealInstrumentManager.h"

class wxRadioBox;
class wxChoice;
class wxCheckBox;
class wxSlider;
class wxTextCtrl;
class wxButton;
class wxNotebook;
class wxPanel;
class wxStaticText;

class JZRealTracksDialog : public wxDialog
{
public:
    JZRealTracksDialog(wxWindow* pParent);
    virtual ~JZRealTracksDialog();

private:
    void CreateControls();
    wxPanel* CreateRealDrumsPanel(wxWindow* pParent);
    wxPanel* CreateRealTracksPanel(wxWindow* pParent);
    wxPanel* CreateSoundFontPanel(wxWindow* pParent);

    // Event Handlers
    void OnModeChanged(wxCommandEvent& Event);
    void OnKitChanged(wxCommandEvent& Event);
    void OnStyleChanged(wxCommandEvent& Event);
    void OnChordChanged(wxCommandEvent& Event);
    void OnDrumPadClicked(wxCommandEvent& Event);
    void OnBrowseSoundFont(wxCommandEvent& Event);
    void OnBrowseSamplePack(wxCommandEvent& Event);
    void OnAuditionToggle(wxCommandEvent& Event);
    void OnTimer(wxTimerEvent& Event);
    void OnApply(wxCommandEvent& Event);
    void OnOK(wxCommandEvent& Event);

    void TransferDataToControls();
    void TransferDataFromControls();

    JZRealInstrumentManager& m_mgr;
    wxTimer m_timer;
    bool m_isAuditioning = false;

    // Controls
    wxRadioBox* mpModeRadio = nullptr;
    wxNotebook* mpNotebook = nullptr;

    // RealDrums tab
    wxChoice* mpKitChoice = nullptr;
    wxCheckBox* mpChokeGroupCheck = nullptr;
    wxCheckBox* mpRoundRobinCheck = nullptr;
    wxSlider* mpSensitivitySlider = nullptr;
    wxSlider* mpSnareBuzzSlider = nullptr;
    wxSlider* mpRoomAmbianceSlider = nullptr;
    wxSlider* mpDrumsMasterSlider = nullptr;

    // RealTracks tab
    wxChoice* mpStyleChoice = nullptr;
    wxCheckBox* mpBassCheck = nullptr;
    wxCheckBox* mpGuitarCheck = nullptr;
    wxCheckBox* mpPianoCheck = nullptr;
    wxCheckBox* mpHornsCheck = nullptr;
    wxSlider* mpBassVolSlider = nullptr;
    wxSlider* mpGuitarVolSlider = nullptr;
    wxSlider* mpPianoVolSlider = nullptr;
    wxSlider* mpTracksMasterSlider = nullptr;

    wxCheckBox* mpGuitarBodyCheck = nullptr;
    wxCheckBox* mpPianoResonanceCheck = nullptr;
    wxCheckBox* mpBinauralRoomCheck = nullptr;

    wxChoice* mpChordRootChoice = nullptr;
    wxChoice* mpChordQualityChoice = nullptr;

    // SoundFont tab
    wxTextCtrl* mpSoundFontText = nullptr;
    wxTextCtrl* mpSamplePackText = nullptr;

    // Bottom controls
    wxSlider* mpBlendSlider = nullptr;
    wxButton* mpAuditionBtn = nullptr;
    wxStaticText* mpStatusText = nullptr;

    DECLARE_EVENT_TABLE()
};
