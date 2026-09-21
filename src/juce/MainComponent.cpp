//*****************************************************************************
// The JAZZ++ Midi Sequencer — JUCE 8 Edition
//
// Copyright (C) 2026 Raja Zahidi
// Main Application Component Implementation
//*****************************************************************************

#include "MainComponent.h"

MainComponent::MainComponent()
    : arrangerComp(engine),
      pianoRollComp(engine),
      drumMachineComp(engine),
      fretboardComp(engine),
      fxRackComp(engine),
      pluginHostComp(engine)
{
    // 1. Initialize Audio and MIDI Engine
    engine.initialize(0, 2);
    engine.addEngineListener(this);

    // 2. View switcher navigation from arranger clips
    arrangerComp.onClipDoubleClicked = [this](int /*trackIdx*/, int /*clipIdx*/) {
        setActiveView(eViewPianoRoll);
    };

    // 3. Setup Transport Buttons
    btnRewind.onClick = [this] { engine.setPlayheadClock(0); };
    btnPlay.onClick   = [this] { engine.startPlayback(); };
    btnPause.onClick  = [this] { engine.pausePlayback(); };
    btnStop.onClick   = [this] { engine.stopPlayback(); };
    btnLoop.onClick   = [this] {
        engine.setLooping(!engine.isLooping());
        btnLoop.setToggleState(engine.isLooping(), juce::dontSendNotification);
    };

    btnPlay.setColour(juce::TextButton::buttonOnColourId, JazzLookAndFeel::Colors::neonEmerald.withAlpha(0.4f));
    btnPlay.setColour(juce::TextButton::textColourOnId, JazzLookAndFeel::Colors::neonEmerald);
    btnStop.setColour(juce::TextButton::buttonOnColourId, JazzLookAndFeel::Colors::neonAmber.withAlpha(0.4f));
    btnLoop.setColour(juce::TextButton::buttonOnColourId, JazzLookAndFeel::Colors::neonAmber.withAlpha(0.4f));
    btnRecord.setColour(juce::TextButton::buttonOnColourId, JazzLookAndFeel::Colors::neonRed.withAlpha(0.4f));

    addAndMakeVisible(btnRewind);
    addAndMakeVisible(btnPlay);
    addAndMakeVisible(btnPause);
    addAndMakeVisible(btnStop);
    addAndMakeVisible(btnRecord);
    addAndMakeVisible(btnLoop);

    // 4. Tempo (BPM) Slider
    slTempo.setRange(20.0, 300.0, 1.0);
    slTempo.setValue(120.0, juce::dontSendNotification);
    slTempo.setSliderStyle(juce::Slider::LinearBar);
    slTempo.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 48, 20);
    slTempo.onValueChange = [this] { engine.setTempo(slTempo.getValue()); };
    addAndMakeVisible(slTempo);

    lblTempo.setFont(juce::FontOptions(10.0f).withStyle("Bold"));
    lblTempo.setColour(juce::Label::textColourId, JazzLookAndFeel::Colors::textSecondary);
    addAndMakeVisible(lblTempo);

    // 5. Time Signature Combo
    cmbTimeSig.addItem("4 / 4", 1);
    cmbTimeSig.addItem("3 / 4", 2);
    cmbTimeSig.addItem("6 / 8", 3);
    cmbTimeSig.addItem("7 / 8", 4);
    cmbTimeSig.setSelectedId(1, juce::dontSendNotification);
    cmbTimeSig.onChange = [this] {
        int id = cmbTimeSig.getSelectedId();
        if (id == 1) engine.setTimeSignature(4, 4);
        else if (id == 2) engine.setTimeSignature(3, 4);
        else if (id == 3) engine.setTimeSignature(6, 8);
        else if (id == 4) engine.setTimeSignature(7, 8);
    };
    addAndMakeVisible(cmbTimeSig);

    // 6. Time & Bar:Beat:Tick Display
    lblTimeDisplay.setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 13.0f, juce::Font::bold));
    lblTimeDisplay.setColour(juce::Label::textColourId, JazzLookAndFeel::Colors::neonCyan);
    lblTimeDisplay.setColour(juce::Label::backgroundColourId, JazzLookAndFeel::Colors::backgroundDark);
    lblTimeDisplay.setColour(juce::Label::outlineColourId, JazzLookAndFeel::Colors::borderOutline);
    lblTimeDisplay.setText("001:01:000 | 00:00.000", juce::dontSendNotification);
    lblTimeDisplay.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(lblTimeDisplay);

    // 7. Audio Settings Button
    btnAudioSettings.onClick = [this] { showAudioSettingsModal(); };
    addAndMakeVisible(btnAudioSettings);

    // 8. View Switcher Tabs
    btnTabArranger.onClick  = [this] { setActiveView(eViewArranger); };
    btnTabPianoRoll.onClick = [this] { setActiveView(eViewPianoRoll); };
    btnTabDrums.onClick     = [this] { setActiveView(eViewDrumMachine); };
    btnTabFretboard.onClick = [this] { setActiveView(eViewFretboard); };
    btnTabFx.onClick        = [this] { setActiveView(eViewAudioFx); };
    btnTabPlugins.onClick   = [this] { setActiveView(eViewPluginHost); };

    addAndMakeVisible(btnTabArranger);
    addAndMakeVisible(btnTabPianoRoll);
    addAndMakeVisible(btnTabDrums);
    addAndMakeVisible(btnTabFretboard);
    addAndMakeVisible(btnTabFx);
    addAndMakeVisible(btnTabPlugins);

    // 9. Master Volume & Status Bar
    slMasterVolume.setRange(0.0, 1.0, 0.01);
    slMasterVolume.setValue(0.85, juce::dontSendNotification);
    slMasterVolume.setSliderStyle(juce::Slider::LinearHorizontal);
    slMasterVolume.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    slMasterVolume.onValueChange = [this] {
        engine.setMasterVolume(static_cast<float>(slMasterVolume.getValue()));
    };
    addAndMakeVisible(slMasterVolume);

    lblMasterVolume.setFont(juce::FontOptions(10.0f).withStyle("Bold"));
    lblMasterVolume.setColour(juce::Label::textColourId, JazzLookAndFeel::Colors::textSecondary);
    addAndMakeVisible(lblMasterVolume);

    lblStatusBar.setFont(11.0f);
    lblStatusBar.setColour(juce::Label::textColourId, JazzLookAndFeel::Colors::textSecondary);
    lblStatusBar.setText("Ready | Audio: " + juce::String(engine.getDeviceManager().getCurrentAudioDevice() ?
                         juce::String(engine.getDeviceManager().getCurrentAudioDevice()->getCurrentSampleRate()) + "Hz" : "44100Hz") +
                         " | MIDI: " + engine.getMidiOutputDeviceName(), juce::dontSendNotification);
    addAndMakeVisible(lblStatusBar);

    // 10. Add Sub-components
    addChildComponent(arrangerComp);
    addChildComponent(pianoRollComp);
    addChildComponent(drumMachineComp);
    addChildComponent(fretboardComp);
    addChildComponent(fxRackComp);
    addChildComponent(pluginHostComp);

    setActiveView(eViewArranger);

    // Start 30Hz timer for smooth level meters & transport sync
    startTimerHz(30);

    setSize(1280, 800);
}

MainComponent::~MainComponent()
{
    stopTimer();
    engine.removeEngineListener(this);
}

void MainComponent::setActiveView(ViewIndex view)
{
    currentView = view;
    arrangerComp.setVisible(view == eViewArranger);
    pianoRollComp.setVisible(view == eViewPianoRoll);
    drumMachineComp.setVisible(view == eViewDrumMachine);
    fretboardComp.setVisible(view == eViewFretboard);
    fxRackComp.setVisible(view == eViewAudioFx);
    pluginHostComp.setVisible(view == eViewPluginHost);

    updateTabButtons();
    resized();
}

void MainComponent::updateTabButtons()
{
    btnTabArranger.setToggleState(currentView == eViewArranger, juce::dontSendNotification);
    btnTabPianoRoll.setToggleState(currentView == eViewPianoRoll, juce::dontSendNotification);
    btnTabDrums.setToggleState(currentView == eViewDrumMachine, juce::dontSendNotification);
    btnTabFretboard.setToggleState(currentView == eViewFretboard, juce::dontSendNotification);
    btnTabFx.setToggleState(currentView == eViewAudioFx, juce::dontSendNotification);
    btnTabPlugins.setToggleState(currentView == eViewPluginHost, juce::dontSendNotification);
}

void MainComponent::showAudioSettingsModal()
{
    auto* selector = new juce::AudioDeviceSelectorComponent(
        engine.getDeviceManager(),
        0, 2, // Audio inputs
        0, 2, // Audio outputs
        true, // Show MIDI inputs
        true, // Show MIDI outputs
        false, // Channels as stereo pairs
        false  // Hide advanced options
    );
    selector->setSize(520, 420);

    juce::DialogWindow::LaunchOptions opt;
    opt.content.setOwned(selector);
    opt.dialogTitle = "Audio & MIDI Settings";
    opt.dialogBackgroundColour = JazzLookAndFeel::Colors::backgroundDark;
    opt.escapeKeyTriggersCloseButton = true;
    opt.useNativeTitleBar = true;
    opt.resizable = false;
    opt.launchAsync();
}

void MainComponent::playbackStateChanged(bool isPlaying)
{
    btnPlay.setToggleState(isPlaying, juce::dontSendNotification);
}

void MainComponent::tempoChanged(double newBpm)
{
    slTempo.setValue(newBpm, juce::dontSendNotification);
}

void MainComponent::playheadMoved(int bar, int beat, int tick, double seconds)
{
    int mins = static_cast<int>(seconds / 60.0);
    double remSecs = seconds - (mins * 60);

    char buf[64];
    std::snprintf(buf, sizeof(buf), "%03d:%02d:%03d | %02d:%06.3f", bar, beat, tick, mins, remSecs);
    lblTimeDisplay.setText(buf, juce::dontSendNotification);
}

void MainComponent::timerCallback()
{
    // Meter decay
    float targetL = engine.getLeftPeak();
    float targetR = engine.getRightPeak();

    meterLeftDecay = std::max(targetL, meterLeftDecay * 0.82f);
    meterRightDecay = std::max(targetR, meterRightDecay * 0.82f);

    repaint(getLocalBounds().removeFromBottom(34)); // Repaint bottom meter bar
}

void MainComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    g.fillAll(JazzLookAndFeel::Colors::backgroundDark);

    // Top Header & Transport Bar
    auto topArea = bounds.removeFromTop(48);
    g.setColour(JazzLookAndFeel::Colors::panelHeader);
    g.fillRect(topArea);

    // App Logo Banner
    g.setColour(JazzLookAndFeel::Colors::neonCyan);
    g.setFont(juce::FontOptions(15.0f).withStyle("Bold"));
    g.drawText("JAZZ++", 14, 0, 70, 48, juce::Justification::centredLeft);

    g.setColour(JazzLookAndFeel::Colors::neonEmerald);
    g.setFont(juce::FontOptions(11.0f).withStyle("Bold"));
    g.drawText("JUCE 8", 78, 0, 50, 48, juce::Justification::centredLeft);

    // Tab Bar Divider
    auto tabArea = bounds.removeFromTop(34);
    g.setColour(JazzLookAndFeel::Colors::panelBackground);
    g.fillRect(tabArea);
    g.setColour(JazzLookAndFeel::Colors::borderOutline);
    g.drawHorizontalLine(tabArea.getBottom() - 1, 0.0f, static_cast<float>(getWidth()));

    // Bottom Bar
    auto bottomArea = bounds.removeFromBottom(34);
    g.setColour(JazzLookAndFeel::Colors::panelHeader);
    g.fillRect(bottomArea);
    g.setColour(JazzLookAndFeel::Colors::borderOutline);
    g.drawHorizontalLine(bottomArea.getY(), 0.0f, static_cast<float>(getWidth()));

    // Stereo Peak Level Meters
    int meterX = bottomArea.getRight() - 150;
    int meterY = bottomArea.getY() + 9;
    int meterW = 60;
    int meterH = 6;

    auto drawMeter = [&](int y, float val) {
        g.setColour(JazzLookAndFeel::Colors::controlFill);
        g.fillRoundedRectangle(static_cast<float>(meterX), static_cast<float>(y), static_cast<float>(meterW), static_cast<float>(meterH), 2.0f);

        float fillW = juce::jlimit(0.0f, 1.0f, val) * meterW;
        if (fillW > 0.0f)
        {
            juce::Colour c = (val > 0.9f) ? JazzLookAndFeel::Colors::neonRed :
                            ((val > 0.7f) ? JazzLookAndFeel::Colors::neonAmber : JazzLookAndFeel::Colors::neonEmerald);
            g.setColour(c);
            g.fillRoundedRectangle(static_cast<float>(meterX), static_cast<float>(y), fillW, static_cast<float>(meterH), 2.0f);
        }
    };

    drawMeter(meterY, meterLeftDecay);
    drawMeter(meterY + 8, meterRightDecay);

    g.setColour(JazzLookAndFeel::Colors::textSecondary);
    g.setFont(9.0f);
    g.drawText("L", meterX - 12, meterY - 2, 10, 10, juce::Justification::centred);
    g.drawText("R", meterX - 12, meterY + 6, 10, 10, juce::Justification::centred);
}

void MainComponent::resized()
{
    auto bounds = getLocalBounds();

    // 1. Top Transport Bar
    auto topArea = bounds.removeFromTop(48);
    topArea.removeFromLeft(140); // App logo room

    int btnW = 34;
    int btnH = 28;
    int y = (48 - btnH) / 2;

    btnRewind.setBounds(topArea.getX(), y, btnW, btnH);
    btnPlay.setBounds(topArea.getX() + 38, y, 64, btnH);
    btnPause.setBounds(topArea.getX() + 106, y, btnW, btnH);
    btnStop.setBounds(topArea.getX() + 144, y, btnW, btnH);
    btnRecord.setBounds(topArea.getX() + 182, y, 46, btnH);
    btnLoop.setBounds(topArea.getX() + 232, y, 48, btnH);

    // Tempo
    lblTempo.setBounds(topArea.getX() + 290, y + 6, 30, 18);
    slTempo.setBounds(topArea.getX() + 322, y, 65, btnH);

    // Time signature
    cmbTimeSig.setBounds(topArea.getX() + 395, y, 64, btnH);

    // Time display
    lblTimeDisplay.setBounds(topArea.getX() + 468, y, 200, btnH);

    // Audio/Midi Settings
    btnAudioSettings.setBounds(getWidth() - 120, y, 106, btnH);

    // 2. View Switcher Tabs
    auto tabArea = bounds.removeFromTop(34);
    int tabW = 125;
    int tabH = 26;
    int tabY = tabArea.getY() + 4;

    btnTabArranger.setBounds(tabArea.getX() + 8, tabY, tabW, tabH);
    btnTabPianoRoll.setBounds(tabArea.getX() + 16 + tabW, tabY, tabW, tabH);
    btnTabDrums.setBounds(tabArea.getX() + 24 + tabW * 2, tabY, tabW, tabH);
    btnTabFretboard.setBounds(tabArea.getX() + 32 + tabW * 3, tabY, tabW, tabH);
    btnTabFx.setBounds(tabArea.getX() + 40 + tabW * 4, tabY, tabW, tabH);
    btnTabPlugins.setBounds(tabArea.getX() + 48 + tabW * 5, tabY, 175, tabH);

    // 3. Bottom Bar
    auto bottomArea = bounds.removeFromBottom(34);
    lblStatusBar.setBounds(bottomArea.getX() + 12, bottomArea.getY() + 6, 450, 22);

    lblMasterVolume.setBounds(bottomArea.getRight() - 320, bottomArea.getY() + 8, 55, 18);
    slMasterVolume.setBounds(bottomArea.getRight() - 260, bottomArea.getY() + 8, 95, 18);

    // 4. Center Component
    arrangerComp.setBounds(bounds);
    pianoRollComp.setBounds(bounds);
    drumMachineComp.setBounds(bounds);
    fretboardComp.setBounds(bounds);
    fxRackComp.setBounds(bounds);
    pluginHostComp.setBounds(bounds);
}
