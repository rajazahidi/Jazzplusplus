//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Audio Effects DSP Dialog Header (EQ, Reverb, Delay, Chorus, Limiter)
//*****************************************************************************

#pragma once

#include "../AudioEffects.h"

#include <wx/dialog.h>

class JZProject;
class JZSample;
class wxButton;
class wxChoice;
class wxNotebook;
class wxPanel;
class wxSlider;
class wxStaticText;
class wxCheckBox;

class JZAudioEffectsDialog : public wxDialog
{
  public:
    JZAudioEffectsDialog(
      wxWindow* pParent,
      JZProject* pProject,
      int DefaultSampleIndex = 0);

    virtual ~JZAudioEffectsDialog();

  private:
    void CreateControls();

    // Event Handlers
    void OnApplyButton(wxCommandEvent& Event);
    void OnPreviewButton(wxCommandEvent& Event);
    void OnCloseButton(wxCommandEvent& Event);
    void OnSampleSelect(wxCommandEvent& Event);

    void ProcessAudio(short* pBuffer, int numSamples, int channels, int sampleRate);

  private:
    JZProject* mpProject;
    int mSelectedSampleIndex;

    wxNotebook* mpNotebook;
    wxChoice* mpSampleChoice;

    // Tab 1: Equalizer
    wxSlider* mpEqBassSlider;
    wxSlider* mpEqMidSlider;
    wxSlider* mpEqTrebleSlider;

    // Tab 2: Reverb (Freeverb)
    wxSlider* mpRevRoomSlider;
    wxSlider* mpRevDampSlider;
    wxSlider* mpRevWidthSlider;
    wxSlider* mpRevWetSlider;
    wxSlider* mpRevDrySlider;

    // Tab 3: Stereo Delay
    wxSlider* mpDelayTimeSlider;
    wxSlider* mpDelayFeedbackSlider;
    wxSlider* mpDelayDampSlider;
    wxSlider* mpDelayWetSlider;
    wxSlider* mpDelayDrySlider;

    // Tab 4: Chorus
    wxSlider* mpChoRateSlider;
    wxSlider* mpChoDepthSlider;
    wxSlider* mpChoFeedbackSlider;
    wxSlider* mpChoMixSlider;

    // Tab 5: Limiter / Distortion
    wxSlider* mpLimDriveSlider;
    wxSlider* mpLimCeilingSlider;
    wxCheckBox* mpLimSoftClipCheck;

    wxButton* mpPreviewButton;
    wxButton* mpApplyButton;
    wxButton* mpCloseButton;

    DECLARE_EVENT_TABLE()
};
