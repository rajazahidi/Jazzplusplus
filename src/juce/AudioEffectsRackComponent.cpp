//*****************************************************************************
// The JAZZ++ Midi Sequencer — JUCE 8 Edition
//
// Copyright (C) 2026 Raja Zahidi
// Real-Time Studio DSP Effects Rack Implementation
//*****************************************************************************

#include "AudioEffectsRackComponent.h"

AudioEffectsRackComponent::AudioEffectsRackComponent(AudioMidiEngine& eng)
    : engine(eng)
{
    // Configure Equalizer
    setupRotary(slEqBass,   lblEqBass,   -18.0, 18.0, 2.0, " dB");
    setupRotary(slEqMid,    lblEqMid,    -18.0, 18.0, 0.0, " dB");
    setupRotary(slEqTreble, lblEqTreble, -18.0, 18.0, 1.5, " dB");
    btnEqEnable.setToggleState(true, juce::dontSendNotification);
    addAndMakeVisible(btnEqEnable);

    auto onEqChanged = [this] {
        auto& s = engine.getEqualizerSettings();
        s.bassGainDB = static_cast<float>(slEqBass.getValue());
        s.midGainDB = static_cast<float>(slEqMid.getValue());
        s.trebleGainDB = static_cast<float>(slEqTreble.getValue());
        engine.updateEqualizer();
    };
    slEqBass.onValueChange = onEqChanged;
    slEqMid.onValueChange = onEqChanged;
    slEqTreble.onValueChange = onEqChanged;

    // Configure Reverb
    setupRotary(slRevRoom, lblRevRoom, 0.0, 1.0, 0.45, "");
    setupRotary(slRevDamp, lblRevDamp, 0.0, 1.0, 0.35, "");
    setupRotary(slRevWet,  lblRevWet,  0.0, 1.0, 0.25, "");
    btnRevEnable.setToggleState(true, juce::dontSendNotification);
    addAndMakeVisible(btnRevEnable);

    auto onRevChanged = [this] {
        auto& s = engine.getReverbSettings();
        s.roomSize = static_cast<float>(slRevRoom.getValue());
        s.damping = static_cast<float>(slRevDamp.getValue());
        s.wet = static_cast<float>(slRevWet.getValue());
        engine.updateReverb();
    };
    slRevRoom.onValueChange = onRevChanged;
    slRevDamp.onValueChange = onRevChanged;
    slRevWet.onValueChange = onRevChanged;

    // Configure Delay
    setupRotary(slDelTime,     lblDelTime,     0.05, 1.0, 0.35, " s");
    setupRotary(slDelFeedback, lblDelFeedback, 0.0, 0.95, 0.35, "");
    setupRotary(slDelWet,      lblDelWet,      0.0, 1.0, 0.3, "");
    btnDelEnable.setToggleState(false, juce::dontSendNotification);
    addAndMakeVisible(btnDelEnable);

    auto onDelChanged = [this] {
        auto& s = engine.getDelaySettings();
        float t = static_cast<float>(slDelTime.getValue() * 1000.0);
        s.delayTimeMsL = t;
        s.delayTimeMsR = t * 1.3f;
        s.feedback = static_cast<float>(slDelFeedback.getValue());
        s.wet = static_cast<float>(slDelWet.getValue());
        engine.updateDelay();
    };
    slDelTime.onValueChange = onDelChanged;
    slDelFeedback.onValueChange = onDelChanged;
    slDelWet.onValueChange = onDelChanged;

    // Configure Chorus
    setupRotary(slChoRate,  lblChoRate,  0.1, 5.0, 1.2, " Hz");
    setupRotary(slChoDepth, lblChoDepth, 1.0, 15.0, 4.0, " ms");
    setupRotary(slChoMix,   lblChoMix,   0.0, 1.0, 0.3, "");
    btnChoEnable.setToggleState(false, juce::dontSendNotification);
    addAndMakeVisible(btnChoEnable);

    auto onChoChanged = [this] {
        auto& s = engine.getChorusSettings();
        s.rateHz = static_cast<float>(slChoRate.getValue());
        s.depthMs = static_cast<float>(slChoDepth.getValue());
        s.mix = static_cast<float>(slChoMix.getValue());
        engine.updateChorus();
    };
    slChoRate.onValueChange = onChoChanged;
    slChoDepth.onValueChange = onChoChanged;
    slChoMix.onValueChange = onChoChanged;

    // Configure Limiter
    lblLimThresh.setText("WARMTH", juce::dontSendNotification);
    setupRotary(slLimDrive,  lblLimDrive,  1.0, 10.0, 1.0, "x");
    setupRotary(slLimThresh, lblLimThresh, 0.0, 1.0, 0.0, "");
    setupRotary(slLimCeil,   lblLimCeil,   -12.0, 0.0, -0.3, " dB");
    btnLimEnable.setToggleState(true, juce::dontSendNotification);
    addAndMakeVisible(btnLimEnable);

    auto onLimChanged = [this] {
        auto& s = engine.getLimiterSettings();
        s.drive = static_cast<float>(slLimDrive.getValue());
        s.ceilingDB = static_cast<float>(slLimCeil.getValue());
        s.enableOverdrive = (slLimThresh.getValue() > 0.5);
        engine.updateLimiter();
    };
    slLimDrive.onValueChange = onLimChanged;
    slLimThresh.onValueChange = onLimChanged;
    slLimCeil.onValueChange = onLimChanged;

    auto onToggle = [this] { updateEngineFx(); };
    btnEqEnable.onClick = onToggle;
    btnRevEnable.onClick = onToggle;
    btnDelEnable.onClick = onToggle;
    btnChoEnable.onClick = onToggle;
    btnLimEnable.onClick = onToggle;

    updateEngineFx();
}

void AudioEffectsRackComponent::setupRotary(juce::Slider& sl, juce::Label& lbl, double min, double max, double def, const juce::String& suffix)
{
    sl.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    sl.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 48, 14);
    sl.setRange(min, max, 0.01);
    sl.setValue(def, juce::dontSendNotification);
    sl.setTextValueSuffix(suffix);
    addAndMakeVisible(sl);

    lbl.setFont(juce::FontOptions(10.0f).withStyle("Bold"));
    lbl.setJustificationType(juce::Justification::centred);
    lbl.setColour(juce::Label::textColourId, JazzLookAndFeel::Colors::textSecondary);
    addAndMakeVisible(lbl);
}

void AudioEffectsRackComponent::updateEngineFx()
{
    engine.setFxEnabled(btnEqEnable.getToggleState(),
                        btnRevEnable.getToggleState(),
                        btnDelEnable.getToggleState(),
                        btnChoEnable.getToggleState(),
                        btnLimEnable.getToggleState());
}

void AudioEffectsRackComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    g.fillAll(JazzLookAndFeel::Colors::backgroundDark);

    // Header
    auto topArea = bounds.removeFromTop(44);
    g.setColour(JazzLookAndFeel::Colors::panelHeader);
    g.fillRect(topArea);

    g.setColour(JazzLookAndFeel::Colors::neonCyan);
    g.setFont(juce::FontOptions(14.0f).withStyle("Bold"));
    g.drawText("STUDIO DSP EFFECTS RACK", 16, 0, 300, 44, juce::Justification::centredLeft);

    // 5 Rack module panels
    auto rackArea = bounds.reduced(16, 12);
    int numModules = 5;
    int spacing = 12;
    int modW = (rackArea.getWidth() - spacing * (numModules - 1)) / numModules;

    const char* modTitles[] = { "PARAMETRIC EQ", "FREEVERB REVERB", "PING-PONG DELAY", "STEREO CHORUS", "TUBE LIMITER" };
    juce::Colour modColors[] = {
        JazzLookAndFeel::Colors::neonCyan,
        JazzLookAndFeel::Colors::neonPurple,
        JazzLookAndFeel::Colors::neonAmber,
        JazzLookAndFeel::Colors::neonEmerald,
        JazzLookAndFeel::Colors::neonRed
    };

    for (int i = 0; i < numModules; ++i)
    {
        juce::Rectangle<int> modRect(rackArea.getX() + i * (modW + spacing), rackArea.getY(), modW, rackArea.getHeight());

        // Chassis background
        g.setColour(JazzLookAndFeel::Colors::panelBackground);
        g.fillRoundedRectangle(modRect.toFloat(), 6.0f);

        // Rack Header banner
        auto hdrRect = modRect.removeFromTop(36);
        g.setColour(JazzLookAndFeel::Colors::panelHeader);
        g.fillRoundedRectangle(hdrRect.toFloat(), 6.0f);

        g.setColour(modColors[i]);
        g.fillRect(hdrRect.getX(), hdrRect.getY(), 4, hdrRect.getHeight());

        g.setColour(JazzLookAndFeel::Colors::textPrimary);
        g.setFont(juce::FontOptions(11.0f).withStyle("Bold"));
        g.drawText(modTitles[i], hdrRect.reduced(10, 0), juce::Justification::centredLeft);

        // Chassis border
        g.setColour(JazzLookAndFeel::Colors::borderOutline);
        g.drawRoundedRectangle(modRect.withTop(hdrRect.getY()).toFloat(), 6.0f, 1.2f);
    }
}

void AudioEffectsRackComponent::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop(44);

    auto rackArea = bounds.reduced(16, 12);
    int numModules = 5;
    int spacing = 12;
    int modW = (rackArea.getWidth() - spacing * (numModules - 1)) / numModules;

    auto layoutModule = [&](int idx, juce::ToggleButton& btn,
                           juce::Slider& s1, juce::Label& l1,
                           juce::Slider& s2, juce::Label& l2,
                           juce::Slider& s3, juce::Label& l3) {
        juce::Rectangle<int> mod(rackArea.getX() + idx * (modW + spacing), rackArea.getY(), modW, rackArea.getHeight());
        mod.removeFromTop(40); // header room

        btn.setBounds(mod.getX() + 12, mod.getY() + 4, mod.getWidth() - 24, 22);

        int knobSize = 64;
        int y1 = mod.getY() + 38;
        int kX = mod.getX() + (modW - knobSize) / 2;

        l1.setBounds(mod.getX() + 4, y1, modW - 8, 14);
        s1.setBounds(kX, y1 + 16, knobSize, knobSize + 14);

        int y2 = y1 + knobSize + 36;
        l2.setBounds(mod.getX() + 4, y2, modW - 8, 14);
        s2.setBounds(kX, y2 + 16, knobSize, knobSize + 14);

        int y3 = y2 + knobSize + 36;
        l3.setBounds(mod.getX() + 4, y3, modW - 8, 14);
        s3.setBounds(kX, y3 + 16, knobSize, knobSize + 14);
    };

    layoutModule(0, btnEqEnable,  slEqBass, lblEqBass, slEqMid, lblEqMid, slEqTreble, lblEqTreble);
    layoutModule(1, btnRevEnable, slRevRoom, lblRevRoom, slRevDamp, lblRevDamp, slRevWet, lblRevWet);
    layoutModule(2, btnDelEnable, slDelTime, lblDelTime, slDelFeedback, lblDelFeedback, slDelWet, lblDelWet);
    layoutModule(3, btnChoEnable, slChoRate, lblChoRate, slChoDepth, lblChoDepth, slChoMix, lblChoMix);
    layoutModule(4, btnLimEnable, slLimDrive, lblLimDrive, slLimThresh, lblLimThresh, slLimCeil, lblLimCeil);
}
