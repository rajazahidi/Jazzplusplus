//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// RealTracksDialog.cpp - Studio RealBand: RealTracks & RealDrums Session Pro Dialog
//*****************************************************************************

#include "RealTracksDialog.h"

using namespace Jazz;

#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/radiobox.h>
#include <wx/choice.h>
#include <wx/checkbox.h>
#include <wx/slider.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/notebook.h>
#include <wx/panel.h>
#include <wx/stattext.h>
#include <wx/filedlg.h>
#include <wx/dirdlg.h>
#include <wx/msgdlg.h>

enum {
    ID_RT_MODE = 11001,
    ID_RT_KIT,
    ID_RT_STYLE,
    ID_RT_CHORD_ROOT,
    ID_RT_CHORD_QUAL,
    ID_RT_DRUM_KICK,
    ID_RT_DRUM_SNARE,
    ID_RT_DRUM_HH_CLOSED,
    ID_RT_DRUM_HH_OPEN,
    ID_RT_DRUM_TOM,
    ID_RT_DRUM_CRASH,
    ID_RT_DRUM_RIDE,
    ID_RT_SF2_BROWSE,
    ID_RT_SMP_BROWSE,
    ID_RT_AUDITION_BTN,
    ID_RT_APPLY_BTN,
    ID_RT_TIMER
};

BEGIN_EVENT_TABLE(JZRealTracksDialog, wxDialog)
    EVT_RADIOBOX(ID_RT_MODE, JZRealTracksDialog::OnModeChanged)
    EVT_CHOICE(ID_RT_KIT, JZRealTracksDialog::OnKitChanged)
    EVT_CHOICE(ID_RT_STYLE, JZRealTracksDialog::OnStyleChanged)
    EVT_CHOICE(ID_RT_CHORD_ROOT, JZRealTracksDialog::OnChordChanged)
    EVT_CHOICE(ID_RT_CHORD_QUAL, JZRealTracksDialog::OnChordChanged)
    EVT_BUTTON(ID_RT_DRUM_KICK, JZRealTracksDialog::OnDrumPadClicked)
    EVT_BUTTON(ID_RT_DRUM_SNARE, JZRealTracksDialog::OnDrumPadClicked)
    EVT_BUTTON(ID_RT_DRUM_HH_CLOSED, JZRealTracksDialog::OnDrumPadClicked)
    EVT_BUTTON(ID_RT_DRUM_HH_OPEN, JZRealTracksDialog::OnDrumPadClicked)
    EVT_BUTTON(ID_RT_DRUM_TOM, JZRealTracksDialog::OnDrumPadClicked)
    EVT_BUTTON(ID_RT_DRUM_CRASH, JZRealTracksDialog::OnDrumPadClicked)
    EVT_BUTTON(ID_RT_DRUM_RIDE, JZRealTracksDialog::OnDrumPadClicked)
    EVT_BUTTON(ID_RT_SF2_BROWSE, JZRealTracksDialog::OnBrowseSoundFont)
    EVT_BUTTON(ID_RT_SMP_BROWSE, JZRealTracksDialog::OnBrowseSamplePack)
    EVT_BUTTON(ID_RT_AUDITION_BTN, JZRealTracksDialog::OnAuditionToggle)
    EVT_TIMER(ID_RT_TIMER, JZRealTracksDialog::OnTimer)
    EVT_BUTTON(ID_RT_APPLY_BTN, JZRealTracksDialog::OnApply)
    EVT_BUTTON(wxID_OK, JZRealTracksDialog::OnOK)
END_EVENT_TABLE()

JZRealTracksDialog::JZRealTracksDialog(wxWindow* pParent)
    : wxDialog(pParent, wxID_ANY, "Studio RealBand: RealTracks & RealDrums Session Pro",
               wxDefaultPosition, wxSize(660, 680),
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      m_mgr(JZRealInstrumentManager::getInstance()),
      m_timer(this, ID_RT_TIMER)
{
    CreateControls();
    TransferDataToControls();
    Centre();
}

JZRealTracksDialog::~JZRealTracksDialog()
{
    if (m_timer.IsRunning()) {
        m_timer.Stop();
    }
}

void JZRealTracksDialog::CreateControls()
{
    wxBoxSizer* pMainSizer = new wxBoxSizer(wxVERTICAL);

    // Header info banner
    wxStaticText* pTitle = new wxStaticText(
        this, wxID_ANY,
        "Studio Acoustic RealAudio & Multi-Sample Enhancement Suite");
    wxFont font = pTitle->GetFont();
    font.SetWeight(wxFONTWEIGHT_BOLD);
    font.SetPointSize(font.GetPointSize() + 1);
    pTitle->SetFont(font);
    pMainSizer->Add(pTitle, 0, wxALL | wxALIGN_CENTER, 8);

    // Mode Selection RadioBox
    wxString modeChoices[] = {
        "Standard MIDI Synth (Raw GM synthesis)",
        "RealDrums Enhanced (Acoustic Studio Drum Replacement on Ch. 10)",
        "Full RealTracks Band (Acoustic Rhythm Section + SoundFont & Physical Modeling)"
    };
    mpModeRadio = new wxRadioBox(this, ID_RT_MODE, "Studio RealAudio Engine Mode",
                                 wxDefaultPosition, wxDefaultSize,
                                 3, modeChoices, 1, wxRA_SPECIFY_COLS);
    pMainSizer->Add(mpModeRadio, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    // Tabbed Notebook Hub
    mpNotebook = new wxNotebook(this, wxID_ANY);
    mpNotebook->AddPage(CreateRealDrumsPanel(mpNotebook), "RealDrums Studio", true);
    mpNotebook->AddPage(CreateRealTracksPanel(mpNotebook), "RealTracks Accompaniment", false);
    mpNotebook->AddPage(CreateSoundFontPanel(mpNotebook), "SoundFont & Sample Packs", false);
    pMainSizer->Add(mpNotebook, 1, wxEXPAND | wxLEFT | wxRIGHT, 8);

    // Bottom Controls Sizer
    wxStaticBoxSizer* pBottomBox = new wxStaticBoxSizer(wxVERTICAL, this, "Master Output & Real Audio Blend");
    wxBoxSizer* pBlendSizer = new wxBoxSizer(wxHORIZONTAL);
    pBlendSizer->Add(new wxStaticText(this, wxID_ANY, "Synth MIDI (0%)"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    mpBlendSlider = new wxSlider(this, wxID_ANY, 100, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
    pBlendSizer->Add(mpBlendSlider, 1, wxEXPAND | wxLEFT | wxRIGHT, 4);
    pBlendSizer->Add(new wxStaticText(this, wxID_ANY, "Real Audio (100%)"), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 4);
    pBottomBox->Add(pBlendSizer, 0, wxEXPAND | wxALL, 4);

    wxBoxSizer* pActionRow = new wxBoxSizer(wxHORIZONTAL);
    mpAuditionBtn = new wxButton(this, ID_RT_AUDITION_BTN, "Live Audition Preview");
    pActionRow->Add(mpAuditionBtn, 0, wxRIGHT, 8);

    mpStatusText = new wxStaticText(this, wxID_ANY, "Ready. RealAudio pipeline active.");
    pActionRow->Add(mpStatusText, 1, wxALIGN_CENTER_VERTICAL);

    wxButton* pApplyBtn = new wxButton(this, ID_RT_APPLY_BTN, "&Apply");
    wxButton* pOkBtn = new wxButton(this, wxID_OK, "&OK");
    wxButton* pCancelBtn = new wxButton(this, wxID_CANCEL, "&Cancel");
    pActionRow->Add(pApplyBtn, 0, wxRIGHT, 4);
    pActionRow->Add(pOkBtn, 0, wxRIGHT, 4);
    pActionRow->Add(pCancelBtn, 0);

    pBottomBox->Add(pActionRow, 0, wxEXPAND | wxTOP, 6);
    pMainSizer->Add(pBottomBox, 0, wxEXPAND | wxALL, 8);

    SetSizer(pMainSizer);
}

wxPanel* JZRealTracksDialog::CreateRealDrumsPanel(wxWindow* pParent)
{
    wxPanel* pPanel = new wxPanel(pParent);
    wxBoxSizer* pSizer = new wxBoxSizer(wxVERTICAL);

    // Kit Selector Row
    wxBoxSizer* pKitRow = new wxBoxSizer(wxHORIZONTAL);
    pKitRow->Add(new wxStaticText(pPanel, wxID_ANY, "Acoustic Drum Kit Profile:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    wxString kits[] = {
        "Vintage Maple Studio (Warm 1970s maple shells, intimate punch)",
        "Modern Birch Recording (Scooped mids, sharp transient snap)",
        "Jazz Club Custom (Open dynamics, brush/stick response, singing ride)",
        "Rock Stadium Punch (Heavy shells, gated room reverb impulse)"
    };
    mpKitChoice = new wxChoice(pPanel, ID_RT_KIT, wxDefaultPosition, wxDefaultSize, 4, kits);
    pKitRow->Add(mpKitChoice, 1, wxEXPAND);
    pSizer->Add(pKitRow, 0, wxEXPAND | wxALL, 6);

    // Realistic behavior toggles
    wxBoxSizer* pToggleRow = new wxBoxSizer(wxHORIZONTAL);
    mpChokeGroupCheck = new wxCheckBox(pPanel, wxID_ANY, "Hi-Hat Choke Group (Closed/Pedal HH cuts Open HH)");
    mpRoundRobinCheck = new wxCheckBox(pPanel, wxID_ANY, "Round-Robin Humanization (Anti-Machine Gun)");
    pToggleRow->Add(mpChokeGroupCheck, 1, wxRIGHT, 8);
    pToggleRow->Add(mpRoundRobinCheck, 1);
    pSizer->Add(pToggleRow, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);

    // Acoustic parameters grid
    wxFlexGridSizer* pGrid = new wxFlexGridSizer(4, 2, 6, 12);
    pGrid->AddGrowableCol(1, 1);

    pGrid->Add(new wxStaticText(pPanel, wxID_ANY, "Velocity Dynamic Sensitivity:"), 0, wxALIGN_CENTER_VERTICAL);
    mpSensitivitySlider = new wxSlider(pPanel, wxID_ANY, 85, 10, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
    pGrid->Add(mpSensitivitySlider, 1, wxEXPAND);

    pGrid->Add(new wxStaticText(pPanel, wxID_ANY, "Sympathetic Snare Wire Buzz:"), 0, wxALIGN_CENTER_VERTICAL);
    mpSnareBuzzSlider = new wxSlider(pPanel, wxID_ANY, 25, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
    pGrid->Add(mpSnareBuzzSlider, 1, wxEXPAND);

    pGrid->Add(new wxStaticText(pPanel, wxID_ANY, "Studio Room Ambiance Send:"), 0, wxALIGN_CENTER_VERTICAL);
    mpRoomAmbianceSlider = new wxSlider(pPanel, wxID_ANY, 30, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
    pGrid->Add(mpRoomAmbianceSlider, 1, wxEXPAND);

    pGrid->Add(new wxStaticText(pPanel, wxID_ANY, "RealDrums Master Level:"), 0, wxALIGN_CENTER_VERTICAL);
    mpDrumsMasterSlider = new wxSlider(pPanel, wxID_ANY, 100, 0, 200, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
    pGrid->Add(mpDrumsMasterSlider, 1, wxEXPAND);

    pSizer->Add(pGrid, 0, wxEXPAND | wxALL, 6);

    // Audition Drum Pads Row
    wxStaticBoxSizer* pPadBox = new wxStaticBoxSizer(wxHORIZONTAL, pPanel, "Audition Acoustic Drum Hits");
    pPadBox->Add(new wxButton(pPanel, ID_RT_DRUM_KICK, "Kick"), 1, wxALL, 2);
    pPadBox->Add(new wxButton(pPanel, ID_RT_DRUM_SNARE, "Snare"), 1, wxALL, 2);
    pPadBox->Add(new wxButton(pPanel, ID_RT_DRUM_HH_CLOSED, "Closed HH"), 1, wxALL, 2);
    pPadBox->Add(new wxButton(pPanel, ID_RT_DRUM_HH_OPEN, "Open HH"), 1, wxALL, 2);
    pPadBox->Add(new wxButton(pPanel, ID_RT_DRUM_TOM, "Tom"), 1, wxALL, 2);
    pPadBox->Add(new wxButton(pPanel, ID_RT_DRUM_CRASH, "Crash"), 1, wxALL, 2);
    pPadBox->Add(new wxButton(pPanel, ID_RT_DRUM_RIDE, "Ride"), 1, wxALL, 2);
    pSizer->Add(pPadBox, 0, wxEXPAND | wxALL, 6);

    pPanel->SetSizer(pSizer);
    return pPanel;
}

wxPanel* JZRealTracksDialog::CreateRealTracksPanel(wxWindow* pParent)
{
    wxPanel* pPanel = new wxPanel(pParent);
    wxBoxSizer* pSizer = new wxBoxSizer(wxVERTICAL);

    // Style Selector Row
    wxBoxSizer* pStyleRow = new wxBoxSizer(wxHORIZONTAL);
    pStyleRow->Add(new wxStaticText(pPanel, wxID_ANY, "RealTracks Arrangement Style:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    wxString styles[] = {
        "Jazz Swing Trio (Upright Bass, Archtop Guitar, Piano comping)",
        "Acoustic Folk Ballad (Dreadnought strum & fingerpicking, Upright Bass)",
        "Bossa Nova Club (Nylon Bossa Guitar, Mellow Upright, Soft Piano)",
        "Pop Rock Studio (Acoustic 12-String, Driving Electric Bass, Piano)",
        "Blues Shuffle (Walking Bass, Shuffle Rhythm Guitar, Honky-tonk)",
        "Funk Groove (Syncopated Slap Bass, 16th Chops, Vintage Rhodes)"
    };
    mpStyleChoice = new wxChoice(pPanel, ID_RT_STYLE, wxDefaultPosition, wxDefaultSize, 6, styles);
    pStyleRow->Add(mpStyleChoice, 1, wxEXPAND);
    pSizer->Add(pStyleRow, 0, wxEXPAND | wxALL, 6);

    // Track Enables & Levels Grid
    wxStaticBoxSizer* pTracksBox = new wxStaticBoxSizer(wxVERTICAL, pPanel, "Instrument Layers & Volume Faders");
    wxFlexGridSizer* pTGrid = new wxFlexGridSizer(4, 3, 4, 8);
    pTGrid->AddGrowableCol(2, 1);

    mpBassCheck = new wxCheckBox(pPanel, wxID_ANY, "Acoustic / Electric Bass");
    pTGrid->Add(mpBassCheck, 0, wxALIGN_CENTER_VERTICAL);
    pTGrid->Add(new wxStaticText(pPanel, wxID_ANY, "Level:"), 0, wxALIGN_CENTER_VERTICAL);
    mpBassVolSlider = new wxSlider(pPanel, wxID_ANY, 85, 0, 150, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
    pTGrid->Add(mpBassVolSlider, 1, wxEXPAND);

    mpGuitarCheck = new wxCheckBox(pPanel, wxID_ANY, "Acoustic / Electric Guitar");
    pTGrid->Add(mpGuitarCheck, 0, wxALIGN_CENTER_VERTICAL);
    pTGrid->Add(new wxStaticText(pPanel, wxID_ANY, "Level:"), 0, wxALIGN_CENTER_VERTICAL);
    mpGuitarVolSlider = new wxSlider(pPanel, wxID_ANY, 80, 0, 150, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
    pTGrid->Add(mpGuitarVolSlider, 1, wxEXPAND);

    mpPianoCheck = new wxCheckBox(pPanel, wxID_ANY, "Concert Grand Piano");
    pTGrid->Add(mpPianoCheck, 0, wxALIGN_CENTER_VERTICAL);
    pTGrid->Add(new wxStaticText(pPanel, wxID_ANY, "Level:"), 0, wxALIGN_CENTER_VERTICAL);
    mpPianoVolSlider = new wxSlider(pPanel, wxID_ANY, 85, 0, 150, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
    pTGrid->Add(mpPianoVolSlider, 1, wxEXPAND);

    mpHornsCheck = new wxCheckBox(pPanel, wxID_ANY, "Orchestral Strings / Horns");
    pTGrid->Add(mpHornsCheck, 0, wxALIGN_CENTER_VERTICAL);
    pTGrid->Add(new wxStaticText(pPanel, wxID_ANY, "Level:"), 0, wxALIGN_CENTER_VERTICAL);
    mpTracksMasterSlider = new wxSlider(pPanel, wxID_ANY, 70, 0, 150, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
    pTGrid->Add(mpTracksMasterSlider, 1, wxEXPAND);

    pTracksBox->Add(pTGrid, 1, wxEXPAND | wxALL, 4);
    pSizer->Add(pTracksBox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);

    // Harmonic Chord Follower preview controls
    wxBoxSizer* pChordRow = new wxBoxSizer(wxHORIZONTAL);
    pChordRow->Add(new wxStaticText(pPanel, wxID_ANY, "Live Harmonic Chord:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);

    wxString roots[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    mpChordRootChoice = new wxChoice(pPanel, ID_RT_CHORD_ROOT, wxDefaultPosition, wxDefaultSize, 12, roots);
    pChordRow->Add(mpChordRootChoice, 0, wxRIGHT, 6);

    wxString quals[] = { "Major", "Minor", "Dominant 7", "Major 7", "Minor 7", "Diminished", "Sus4" };
    mpChordQualityChoice = new wxChoice(pPanel, ID_RT_CHORD_QUAL, wxDefaultPosition, wxDefaultSize, 7, quals);
    pChordRow->Add(mpChordQualityChoice, 1);
    pSizer->Add(pChordRow, 0, wxEXPAND | wxALL, 6);

    // Physical modeling convolution suite
    wxStaticBoxSizer* pPhysBox = new wxStaticBoxSizer(wxVERTICAL, pPanel, "Physical Acoustic Resonance Modeling Suite");
    mpGuitarBodyCheck = new wxCheckBox(pPanel, wxID_ANY, "102 Hz Helmholtz Acoustic Guitar Body Impulse Convolver (Fishman Aura)");
    mpPianoResonanceCheck = new wxCheckBox(pPanel, wxID_ANY, "88-String Concert Grand Sympathetic Resonance & Damper Wash");
    mpBinauralRoomCheck = new wxCheckBox(pPanel, wxID_ANY, "Binaural 3D Virtual Studio Room Acoustic Convolver");

    pPhysBox->Add(mpGuitarBodyCheck, 0, wxALL, 3);
    pPhysBox->Add(mpPianoResonanceCheck, 0, wxALL, 3);
    pPhysBox->Add(mpBinauralRoomCheck, 0, wxALL, 3);
    pSizer->Add(pPhysBox, 0, wxEXPAND | wxALL, 6);

    pPanel->SetSizer(pSizer);
    return pPanel;
}

wxPanel* JZRealTracksDialog::CreateSoundFontPanel(wxWindow* pParent)
{
    wxPanel* pPanel = new wxPanel(pParent);
    wxBoxSizer* pSizer = new wxBoxSizer(wxVERTICAL);

    // SoundFont Row
    wxStaticBoxSizer* pSfBox = new wxStaticBoxSizer(wxVERTICAL, pPanel, "General MIDI SoundFont (.sf2 / .sf3)");
    wxBoxSizer* pSfRow = new wxBoxSizer(wxHORIZONTAL);
    mpSoundFontText = new wxTextCtrl(pPanel, wxID_ANY, "");
    wxButton* pSfBrowseBtn = new wxButton(pPanel, ID_RT_SF2_BROWSE, "Browse...");
    pSfRow->Add(mpSoundFontText, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    pSfRow->Add(pSfBrowseBtn, 0);
    pSfBox->Add(pSfRow, 0, wxEXPAND | wxALL, 4);

    wxStaticText* pSfInfo = new wxStaticText(
        pPanel, wxID_ANY,
        "Load high-definition multi-sampled studio instruments (e.g. FluidR3_GM, Yamaha C7 Grand, Roland SC-55)."
    );
    pSfBox->Add(pSfInfo, 0, wxLEFT | wxRIGHT | wxBOTTOM, 4);
    pSizer->Add(pSfBox, 0, wxEXPAND | wxALL, 6);

    // Sample Packs Row
    wxStaticBoxSizer* pSmpBox = new wxStaticBoxSizer(wxVERTICAL, pPanel, "Custom RealDrums / RealAudio Sample Packs Directory");
    wxBoxSizer* pSmpRow = new wxBoxSizer(wxHORIZONTAL);
    mpSamplePackText = new wxTextCtrl(pPanel, wxID_ANY, "");
    wxButton* pSmpBrowseBtn = new wxButton(pPanel, ID_RT_SMP_BROWSE, "Browse...");
    pSmpRow->Add(mpSamplePackText, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    pSmpRow->Add(pSmpBrowseBtn, 0);
    pSmpBox->Add(pSmpRow, 0, wxEXPAND | wxALL, 4);

    wxStaticText* pSmpInfo = new wxStaticText(
        pPanel, wxID_ANY,
        "Directory containing 24-bit studio WAV drum loops, acoustic strums, and multi-velocity hits."
    );
    pSmpBox->Add(pSmpInfo, 0, wxLEFT | wxRIGHT | wxBOTTOM, 4);
    pSizer->Add(pSmpBox, 0, wxEXPAND | wxALL, 6);

    // Built-in Acoustic Synthesis note
    wxStaticBoxSizer* pNoteBox = new wxStaticBoxSizer(wxVERTICAL, pPanel, "Procedural Physical Acoustic Fallback Engine");
    wxStaticText* pNoteText = new wxStaticText(
        pPanel, wxID_ANY,
        "When no external SoundFont or sample pack is provided, Jazz++ utilizes its built-in physical acoustic modeling\n"
        "synthesis engine (sub-bass kick envelopes, snappy rimshot noise, inharmonic metallic cymbals, 102Hz Helmholtz cavity,\n"
        "and 88-string cross-coupling) to produce authentic studio acoustic instruments in real-time with zero latency."
    );
    pNoteBox->Add(pNoteText, 0, wxALL, 4);
    pSizer->Add(pNoteBox, 1, wxEXPAND | wxALL, 6);

    pPanel->SetSizer(pSizer);
    return pPanel;
}

void JZRealTracksDialog::TransferDataToControls()
{
    // Mode
    mpModeRadio->SetSelection(static_cast<int>(m_mgr.getMode()));

    // RealDrums
    auto& drums = m_mgr.getRealDrums();
    const auto& dParams = drums.getParameters();
    mpKitChoice->SetSelection(static_cast<int>(dParams.kitType));
    mpChokeGroupCheck->SetValue(dParams.enableChokeGroup);
    mpRoundRobinCheck->SetValue(dParams.enableRoundRobin);
    mpSensitivitySlider->SetValue(static_cast<int>(dParams.velocitySensitivity * 100.0f));
    mpSnareBuzzSlider->SetValue(static_cast<int>(dParams.snareBuzzAmount * 100.0f));
    mpRoomAmbianceSlider->SetValue(static_cast<int>(dParams.roomAmbianceSend * 100.0f));
    mpDrumsMasterSlider->SetValue(static_cast<int>(dParams.masterLevel * 100.0f));

    // RealTracks
    auto& tracks = m_mgr.getRealTracks();
    const auto& tParams = tracks.getParameters();
    mpStyleChoice->SetSelection(static_cast<int>(tParams.style));
    mpBassCheck->SetValue(tParams.trackEnabled[static_cast<size_t>(JZRealTrackType::BASS)]);
    mpGuitarCheck->SetValue(tParams.trackEnabled[static_cast<size_t>(JZRealTrackType::GUITAR)]);
    mpPianoCheck->SetValue(tParams.trackEnabled[static_cast<size_t>(JZRealTrackType::PIANO)]);
    mpHornsCheck->SetValue(tParams.trackEnabled[static_cast<size_t>(JZRealTrackType::HORNS_STRINGS)]);

    mpBassVolSlider->SetValue(static_cast<int>(tParams.trackVolume[static_cast<size_t>(JZRealTrackType::BASS)] * 100.0f));
    mpGuitarVolSlider->SetValue(static_cast<int>(tParams.trackVolume[static_cast<size_t>(JZRealTrackType::GUITAR)] * 100.0f));
    mpPianoVolSlider->SetValue(static_cast<int>(tParams.trackVolume[static_cast<size_t>(JZRealTrackType::PIANO)] * 100.0f));
    mpTracksMasterSlider->SetValue(static_cast<int>(tParams.masterLevel * 100.0f));

    mpGuitarBodyCheck->SetValue(m_mgr.isGuitarBodyResonanceEnabled());
    mpPianoResonanceCheck->SetValue(m_mgr.isPianoSympatheticResonanceEnabled());
    mpBinauralRoomCheck->SetValue(m_mgr.isBinauralRoomEnabled());

    mpChordRootChoice->SetSelection(tParams.currentChordRoot % 12);
    mpChordQualityChoice->SetSelection(static_cast<int>(tParams.currentChordQuality));

    // SoundFont & Sample packs
    mpSoundFontText->SetValue(m_mgr.getSoundFontPath());
    mpSamplePackText->SetValue(m_mgr.getSamplePackPath());

    // Blend
    mpBlendSlider->SetValue(static_cast<int>(m_mgr.getRealAudioBlend() * 100.0f));
}

void JZRealTracksDialog::TransferDataFromControls()
{
    // Mode
    m_mgr.setMode(static_cast<JZRealAudioMode>(mpModeRadio->GetSelection()));

    // RealDrums
    auto& drums = m_mgr.getRealDrums();
    drums.setKitType(static_cast<JZDrumKitType>(mpKitChoice->GetSelection()));
    drums.setChokeGroupEnabled(mpChokeGroupCheck->GetValue());
    drums.setRoundRobinEnabled(mpRoundRobinCheck->GetValue());
    drums.setVelocitySensitivity(mpSensitivitySlider->GetValue() / 100.0f);
    drums.setSnareBuzzAmount(mpSnareBuzzSlider->GetValue() / 100.0f);
    drums.setRoomAmbianceSend(mpRoomAmbianceSlider->GetValue() / 100.0f);
    drums.setMasterLevel(mpDrumsMasterSlider->GetValue() / 100.0f);

    // RealTracks
    auto& tracks = m_mgr.getRealTracks();
    tracks.setStyle(static_cast<JZRealTracksStyle>(mpStyleChoice->GetSelection()));
    tracks.setTrackEnabled(JZRealTrackType::BASS, mpBassCheck->GetValue());
    tracks.setTrackEnabled(JZRealTrackType::GUITAR, mpGuitarCheck->GetValue());
    tracks.setTrackEnabled(JZRealTrackType::PIANO, mpPianoCheck->GetValue());
    tracks.setTrackEnabled(JZRealTrackType::HORNS_STRINGS, mpHornsCheck->GetValue());

    tracks.setTrackVolume(JZRealTrackType::BASS, mpBassVolSlider->GetValue() / 100.0f);
    tracks.setTrackVolume(JZRealTrackType::GUITAR, mpGuitarVolSlider->GetValue() / 100.0f);
    tracks.setTrackVolume(JZRealTrackType::PIANO, mpPianoVolSlider->GetValue() / 100.0f);
    tracks.setMasterLevel(mpTracksMasterSlider->GetValue() / 100.0f);

    m_mgr.setGuitarBodyResonanceEnabled(mpGuitarBodyCheck->GetValue());
    m_mgr.setPianoSympatheticResonanceEnabled(mpPianoResonanceCheck->GetValue());
    m_mgr.setBinauralRoomEnabled(mpBinauralRoomCheck->GetValue());

    int root = mpChordRootChoice->GetSelection();
    auto qual = static_cast<JZRealChordQuality>(mpChordQualityChoice->GetSelection());
    tracks.setChord(root, qual);

    // SoundFont & Sample packs
    m_mgr.setSoundFontPath(mpSoundFontText->GetValue().ToStdString());
    m_mgr.setSamplePackPath(mpSamplePackText->GetValue().ToStdString());

    // Blend
    m_mgr.setRealAudioBlend(mpBlendSlider->GetValue() / 100.0f);
}

void JZRealTracksDialog::OnModeChanged(wxCommandEvent& WXUNUSED(Event))
{
    TransferDataFromControls();
    int mode = mpModeRadio->GetSelection();
    if (mode == 0) {
        mpStatusText->SetLabel("Standard MIDI Synth active (no RealAudio replacement).");
    } else if (mode == 1) {
        mpStatusText->SetLabel("RealDrums Enhanced: Channel 10 drums replaced by acoustic samples.");
    } else {
        mpStatusText->SetLabel("Full RealTracks Band: Drums, Bass, Guitar, Piano & Physical Modeling active.");
    }
}

void JZRealTracksDialog::OnKitChanged(wxCommandEvent& WXUNUSED(Event))
{
    m_mgr.getRealDrums().setKitType(static_cast<JZDrumKitType>(mpKitChoice->GetSelection()));
    mpStatusText->SetLabel(wxString::Format("Kit changed to: %s", m_mgr.getRealDrums().getKitName().c_str()));
}

void JZRealTracksDialog::OnStyleChanged(wxCommandEvent& WXUNUSED(Event))
{
    m_mgr.getRealTracks().setStyle(static_cast<JZRealTracksStyle>(mpStyleChoice->GetSelection()));
    mpStatusText->SetLabel(wxString::Format("Style changed to: %s", m_mgr.getRealTracks().getStyleName().c_str()));
}

void JZRealTracksDialog::OnChordChanged(wxCommandEvent& WXUNUSED(Event))
{
    int root = mpChordRootChoice->GetSelection();
    auto qual = static_cast<JZRealChordQuality>(mpChordQualityChoice->GetSelection());
    m_mgr.getRealTracks().setChord(root, qual);
    mpStatusText->SetLabel(wxString::Format("Chord changed to: %s", m_mgr.getRealTracks().getChordString().c_str()));
}

void JZRealTracksDialog::OnDrumPadClicked(wxCommandEvent& Event)
{
    int id = Event.GetId();
    int note = 36;
    switch (id) {
        case ID_RT_DRUM_KICK:      note = 36; break;
        case ID_RT_DRUM_SNARE:     note = 38; break;
        case ID_RT_DRUM_HH_CLOSED: note = 42; break;
        case ID_RT_DRUM_HH_OPEN:   note = 46; break;
        case ID_RT_DRUM_TOM:       note = 45; break;
        case ID_RT_DRUM_CRASH:     note = 49; break;
        case ID_RT_DRUM_RIDE:      note = 51; break;
    }
    m_mgr.getRealDrums().noteOn(note, 0.9f);
    mpStatusText->SetLabel(wxString::Format("Triggered drum audition note: %d", note));
}

void JZRealTracksDialog::OnBrowseSoundFont(wxCommandEvent& WXUNUSED(Event))
{
    wxFileDialog openDialog(
        this, "Select SoundFont File", "", "",
        "SoundFont Files (*.sf2;*.sf3)|*.sf2;*.sf3|All Files (*.*)|*.*",
        wxFD_OPEN | wxFD_FILE_MUST_EXIST
    );

    if (openDialog.ShowModal() == wxID_OK) {
        mpSoundFontText->SetValue(openDialog.GetPath());
        m_mgr.setSoundFontPath(openDialog.GetPath().ToStdString());
        mpStatusText->SetLabel("Loaded SoundFont: " + openDialog.GetFilename());
    }
}

void JZRealTracksDialog::OnBrowseSamplePack(wxCommandEvent& WXUNUSED(Event))
{
    wxDirDialog dirDialog(this, "Select Sample Pack Directory", "", wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);
    if (dirDialog.ShowModal() == wxID_OK) {
        mpSamplePackText->SetValue(dirDialog.GetPath());
        m_mgr.setSamplePackPath(dirDialog.GetPath().ToStdString());
        mpStatusText->SetLabel("Sample pack folder set.");
    }
}

void JZRealTracksDialog::OnAuditionToggle(wxCommandEvent& WXUNUSED(Event))
{
    TransferDataFromControls();
    m_isAuditioning = !m_isAuditioning;

    if (m_isAuditioning) {
        mpAuditionBtn->SetLabel("Stop Audition");
        m_timer.Start(250); // Fire pulse every 250ms
        mpStatusText->SetLabel(wxString::Format("Auditioning RealBand groove in %s", m_mgr.getRealTracks().getChordString().c_str()));
    } else {
        mpAuditionBtn->SetLabel("Live Audition Preview");
        m_timer.Stop();
        mpStatusText->SetLabel("Audition stopped.");
    }
}

void JZRealTracksDialog::OnTimer(wxTimerEvent& WXUNUSED(Event))
{
    static int beat = 0;
    // Trigger rhythmic pattern: Kick on 1 and 3, Snare on 2 and 4, Closed HH on all 8ths
    if (beat % 2 == 0) {
        m_mgr.getRealDrums().noteOn(42, 0.6f); // Closed HH
    } else {
        m_mgr.getRealDrums().noteOn(42, 0.45f);
    }

    if (beat == 0 || beat == 4) {
        m_mgr.getRealDrums().noteOn(36, 0.85f); // Kick
    } else if (beat == 2 || beat == 6) {
        m_mgr.getRealDrums().noteOn(38, 0.90f); // Snare
    }

    beat = (beat + 1) % 8;
}

void JZRealTracksDialog::OnApply(wxCommandEvent& WXUNUSED(Event))
{
    TransferDataFromControls();
    mpStatusText->SetLabel("Settings applied successfully.");
}

void JZRealTracksDialog::OnOK(wxCommandEvent& Event)
{
    if (m_timer.IsRunning()) {
        m_timer.Stop();
    }
    TransferDataFromControls();
    EndModal(wxID_OK);
}
