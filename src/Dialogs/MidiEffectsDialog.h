//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// MIDI Effects Dialog Header (Arpeggiator, Humanizer, Harmonizer, Echo)
//*****************************************************************************

#pragma once

#include "../MidiEffects.h"

#include <wx/dialog.h>

class JZProject;
class JZTrack;
class wxButton;
class wxChoice;
class wxNotebook;
class wxPanel;
class wxRadioBox;
class wxSlider;
class wxSpinCtrl;
class wxStaticText;
class wxCheckBox;

class JZMidiEffectsDialog : public wxDialog
{
  public:
    JZMidiEffectsDialog(
      wxWindow* pParent,
      JZProject* pProject,
      int DefaultTrack = 1,
      long SelectionFromClock = 0,
      long SelectionToClock = 0);

    virtual ~JZMidiEffectsDialog();

  private:
    void CreateControls();

    // Event Handlers
    void OnApplyButton(wxCommandEvent& Event);
    void OnCloseButton(wxCommandEvent& Event);

    void ApplyArpeggiator(JZTrack* pTrack, long fromClock, long toClock);
    void ApplyHumanizer(JZTrack* pTrack, long fromClock, long toClock);
    void ApplyHarmonizer(JZTrack* pTrack, long fromClock, long toClock);
    void ApplyEcho(JZTrack* pTrack, long fromClock, long toClock);

  private:
    JZProject* mpProject;
    int mTargetTrackIndex;
    long mFromClock;
    long mToClock;

    wxNotebook* mpNotebook;
    wxChoice* mpTrackChoice;
    wxRadioBox* mpScopeRadio; // 0: Selected Range, 1: Entire Track

    // Tab 1: Arpeggiator
    wxChoice* mpArpPatternChoice;
    wxChoice* mpArpRateChoice;
    wxSpinCtrl* mpArpOctavesSpin;
    wxSlider* mpArpGateSlider;
    wxCheckBox* mpArpScaleLockCheck;
    wxChoice* mpArpRootChoice;
    wxChoice* mpArpScaleChoice;

    // Tab 2: Humanizer
    wxSlider* mpHumTimingSlider;
    wxSlider* mpHumVelocitySlider;
    wxSlider* mpHumSwingSlider;

    // Tab 3: Harmonizer
    wxChoice* mpHarmIntervalChoice;
    wxChoice* mpHarmRootChoice;
    wxChoice* mpHarmScaleChoice;
    wxSlider* mpHarmVelScaleSlider;

    // Tab 4: Echo / Delay
    wxChoice* mpEchoDelayChoice;
    wxSpinCtrl* mpEchoRepeatsSpin;
    wxSlider* mpEchoFeedbackSlider;
    wxSpinCtrl* mpEchoPitchShiftSpin;

    wxButton* mpApplyButton;
    wxButton* mpCloseButton;

    DECLARE_EVENT_TABLE()
};
