//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Sound Generator Dialog Header (Procedural SFX & Synth Drums)
//*****************************************************************************

#pragma once

#include "../SoundGenerator.h"

#include <wx/dialog.h>
#include <vector>
#include <string>

class JZProject;
class wxButton;
class wxChoice;
class wxNotebook;
class wxPanel;
class wxSlider;
class wxSpinCtrl;
class wxStaticText;

class JZSoundGeneratorDialog : public wxDialog
{
  public:
    JZSoundGeneratorDialog(
      wxWindow* pParent,
      JZProject* pProject,
      int DefaultTrack = 1,
      long DefaultClock = 0);

    virtual ~JZSoundGeneratorDialog();

  private:
    void CreateControls();

    // Event Handlers
    void OnSFXPresetChange(wxCommandEvent& Event);
    void OnDrumPresetChange(wxCommandEvent& Event);
    void OnSFXPreview(wxCommandEvent& Event);
    void OnDrumPreview(wxCommandEvent& Event);
    void OnSFXExportWav(wxCommandEvent& Event);
    void OnDrumExportWav(wxCommandEvent& Event);
    void OnSFXInsertTrack(wxCommandEvent& Event);
    void OnDrumInsertTrack(wxCommandEvent& Event);
    void OnCloseButton(wxCommandEvent& Event);

    std::vector<short> GenerateCurrentSFX();
    std::vector<short> GenerateCurrentDrum();

  private:
    JZProject* mpProject;
    int mTargetTrackIndex;
    long mStartClock;

    wxNotebook* mpNotebook;

    // Tab 1: Retro SFX Controls
    wxChoice* mpSFXPresetChoice;
    wxChoice* mpSFXWaveformChoice;
    wxSlider* mpSFXStartFreqSlider;
    wxSlider* mpSFXEndFreqSlider;
    wxSlider* mpSFXDurationSlider;
    wxSlider* mpSFXDutySlider;
    wxSlider* mpSFXVolumeSlider;
    wxButton* mpSFXPreviewBtn;
    wxButton* mpSFXExportBtn;
    wxButton* mpSFXInsertBtn;

    // Tab 2: Analog Synth Drum Controls
    wxChoice* mpDrumPresetChoice;
    wxSlider* mpDrumPitchSlider;
    wxSlider* mpDrumPitchDropSlider;
    wxSlider* mpDrumDurationSlider;
    wxSlider* mpDrumToneMixSlider;
    wxSlider* mpDrumClickSlider;
    wxSlider* mpDrumOverdriveSlider;
    wxChoice* mpDrumKeyChoice;
    wxButton* mpDrumPreviewBtn;
    wxButton* mpDrumExportBtn;
    wxButton* mpDrumInsertBtn;

    wxButton* mpCloseButton;

    DECLARE_EVENT_TABLE()
};
