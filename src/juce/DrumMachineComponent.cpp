//*****************************************************************************
// The JAZZ++ Midi Sequencer — JUCE 8 Edition
//
// Copyright (C) 2026 Raja Zahidi
// 808 Drum Machine & 16-Step Sequencer Implementation
//*****************************************************************************

#include "DrumMachineComponent.h"

DrumMachineComponent::DrumMachineComponent(AudioMidiEngine& eng)
    : engine(eng)
{
    engine.addEngineListener(this);

    // Configure 8 Drum Voices
    voices[0] = { "808 KICK",   eDrum808Kick,      false, eSFXLaser,     JazzLookAndFeel::Colors::neonRed,     0.95f, 0.5f };
    voices[1] = { "808 SNARE",  eDrum808Snare,     false, eSFXLaser,     JazzLookAndFeel::Colors::neonAmber,   0.90f, 0.5f };
    voices[2] = { "HI-HAT CL",  eDrumHiHatClosed,  false, eSFXLaser,     JazzLookAndFeel::Colors::neonCyan,    0.80f, 0.4f };
    voices[3] = { "HI-HAT OP",  eDrumHiHatOpen,    false, eSFXLaser,     JazzLookAndFeel::Colors::neonCyan,    0.80f, 0.6f };
    voices[4] = { "808 CLAP",   eDrum808Clap,      false, eSFXLaser,     JazzLookAndFeel::Colors::neonPurple,  0.85f, 0.5f };
    voices[5] = { "808 TOM",    eDrum808Tom,       false, eSFXLaser,     JazzLookAndFeel::Colors::neonEmerald, 0.85f, 0.45f };
    voices[6] = { "COWBELL",    eDrum808Snare,     true,  eSFXCoin,      JazzLookAndFeel::Colors::neonAmber,   0.75f, 0.55f };
    voices[7] = { "CHIP SFX",   eDrum808Kick,      true,  eSFXLaser,     JazzLookAndFeel::Colors::neonCyan,    0.80f, 0.5f };

    // Preset buttons
    addAndMakeVisible(btnPresetTrap);
    addAndMakeVisible(btnPresetHouse);
    addAndMakeVisible(btnPresetTechno);
    addAndMakeVisible(btnPresetArcade);
    addAndMakeVisible(btnClear);

    btnPresetTrap.onClick   = [this] { loadPreset(0); };
    btnPresetHouse.onClick  = [this] { loadPreset(1); };
    btnPresetTechno.onClick = [this] { loadPreset(2); };
    btnPresetArcade.onClick = [this] { loadPreset(3); };
    btnClear.onClick        = [this] { clearPattern(); };

    // Load initial preset
    loadPreset(0);
}

DrumMachineComponent::~DrumMachineComponent()
{
    engine.removeEngineListener(this);
}

void DrumMachineComponent::clearPattern()
{
    for (auto& row : pattern)
        row.fill(false);
    repaint();
}

void DrumMachineComponent::loadPreset(int presetIdx)
{
    clearPattern();

    if (presetIdx == 0) // 808 Trap
    {
        // Kick
        pattern[0][0] = true; pattern[0][6] = true; pattern[0][10] = true; pattern[0][14] = true;
        // Snare
        pattern[1][4] = true; pattern[1][12] = true;
        // Hi-Hat Closed (all 16ths)
        for (int i = 0; i < 16; ++i) pattern[2][i] = true;
        // Hi-Hat Open
        pattern[3][2] = true; pattern[3][8] = true;
        // Clap
        pattern[4][4] = true; pattern[4][12] = true;
        // Tom
        pattern[5][15] = true;
        // Chip SFX
        pattern[7][8] = true;
    }
    else if (presetIdx == 1) // Classic House
    {
        // Kick: 4-on-the-floor
        pattern[0][0] = true; pattern[0][4] = true; pattern[0][8] = true; pattern[0][12] = true;
        // Snare / Clap
        pattern[1][4] = true; pattern[1][12] = true;
        pattern[4][4] = true; pattern[4][12] = true;
        // Hi-Hat Open on off-beats
        pattern[3][2] = true; pattern[3][6] = true; pattern[3][10] = true; pattern[3][14] = true;
        // Hi-Hat Closed
        for (int i = 0; i < 16; i += 2) pattern[2][i] = true;
    }
    else if (presetIdx == 2) // Techno Pulse
    {
        // Kick
        pattern[0][0] = true; pattern[0][4] = true; pattern[0][8] = true; pattern[0][12] = true;
        // Closed Hat on off-beats
        pattern[2][2] = true; pattern[2][6] = true; pattern[2][10] = true; pattern[2][14] = true;
        // Tom
        pattern[5][7] = true; pattern[5][15] = true;
        // Clap
        pattern[4][4] = true; pattern[4][12] = true;
    }
    else if (presetIdx == 3) // Retro Chiptune
    {
        pattern[0][0] = true; pattern[0][8] = true;
        pattern[1][4] = true; pattern[1][12] = true;
        pattern[6][2] = true; pattern[6][6] = true; pattern[6][10] = true; pattern[6][14] = true;
        pattern[7][0] = true; pattern[7][7] = true; pattern[7][11] = true;
    }

    repaint();
}

void DrumMachineComponent::stepTriggered(int step16th)
{
    currentPlayStep = step16th;

    for (int v = 0; v < kNumVoices; ++v)
    {
        if (pattern[v][step16th])
        {
            if (voices[v].isRetroSFX)
                engine.triggerRetroSFX(voices[v].sfxType, voices[v].volume);
            else
                engine.triggerDrum(voices[v].type, voices[v].volume, voices[v].pan);
        }
    }

    repaint();
}

void DrumMachineComponent::playbackStateChanged(bool isPlaying)
{
    if (!isPlaying)
    {
        currentPlayStep = -1;
    }
    repaint();
}

void DrumMachineComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    g.fillAll(JazzLookAndFeel::Colors::backgroundDark);

    // Top control strip
    auto topArea = bounds.removeFromTop(44);
    g.setColour(JazzLookAndFeel::Colors::panelHeader);
    g.fillRect(topArea);

    g.setColour(JazzLookAndFeel::Colors::neonCyan);
    g.setFont(juce::FontOptions(14.0f).withStyle("Bold"));
    g.drawText("808 VINTAGE DRUM MACHINE & STEP SEQUENCER", 16, 0, 380, 44, juce::Justification::centredLeft);

    // Bottom section: MPC Trigger pads
    auto padsArea = bounds.removeFromBottom(130);
    drawTriggerPads(g, padsArea);

    // Center section: 16-step sequencer grid
    drawSequencerGrid(g, bounds);
}

void DrumMachineComponent::drawSequencerGrid(juce::Graphics& g, juce::Rectangle<int> area)
{
    g.setColour(JazzLookAndFeel::Colors::panelBackground);
    g.fillRect(area);

    int headerW = 120;
    int gridW = area.getWidth() - headerW - 16;
    float stepW = static_cast<float>(gridW) / static_cast<float>(kNumSteps);
    float rowH = static_cast<float>(area.getHeight()) / static_cast<float>(kNumVoices);

    // Step Header (1..16) at top of grid
    for (int s = 0; s < kNumSteps; ++s)
    {
        float x = area.getX() + headerW + s * stepW;
        bool isBeatHead = (s % 4 == 0);
        bool isCurrent = (s == currentPlayStep);

        if (isCurrent)
        {
            g.setColour(JazzLookAndFeel::Colors::neonCyan.withAlpha(0.25f));
            g.fillRect(x, static_cast<float>(area.getY()), stepW, static_cast<float>(area.getHeight()));
        }
        else if (isBeatHead)
        {
            g.setColour(JazzLookAndFeel::Colors::controlFill.withAlpha(0.15f));
            g.fillRect(x, static_cast<float>(area.getY()), stepW, static_cast<float>(area.getHeight()));
        }
    }

    for (int v = 0; v < kNumVoices; ++v)
    {
        float y = area.getY() + v * rowH;

        // Voice Label
        juce::Rectangle<float> lblRect(static_cast<float>(area.getX() + 8), y + 4.0f,
                                      static_cast<float>(headerW - 16), rowH - 8.0f);
        g.setColour(JazzLookAndFeel::Colors::panelHeader);
        g.fillRoundedRectangle(lblRect, 4.0f);

        g.setColour(voices[v].color);
        g.fillRect(lblRect.getX(), lblRect.getY(), 4.0f, lblRect.getHeight());

        g.setColour(JazzLookAndFeel::Colors::textPrimary);
        g.setFont(11.0f);
        g.drawText(voices[v].name, lblRect.toNearestInt().reduced(6, 0), juce::Justification::centredLeft);

        // Step buttons
        for (int s = 0; s < kNumSteps; ++s)
        {
            float x = area.getX() + headerW + s * stepW;
            juce::Rectangle<float> padRect(x + 2.0f, y + 3.0f, stepW - 4.0f, rowH - 6.0f);

            bool active = pattern[v][s];
            bool isCurrent = (s == currentPlayStep);

            if (active)
            {
                g.setColour(isCurrent ? juce::Colours::white : voices[v].color);
                g.fillRoundedRectangle(padRect, 4.0f);
            }
            else
            {
                juce::Colour bg = (s / 4) % 2 == 0 ?
                    JazzLookAndFeel::Colors::controlFill :
                    JazzLookAndFeel::Colors::controlFill.darker(0.2f);

                g.setColour(isCurrent ? bg.brighter(0.3f) : bg);
                g.fillRoundedRectangle(padRect, 4.0f);
            }

            // Outline
            g.setColour(isCurrent ? JazzLookAndFeel::Colors::neonCyan : JazzLookAndFeel::Colors::borderOutline);
            g.drawRoundedRectangle(padRect, 4.0f, isCurrent ? 2.0f : 1.0f);
        }

        // Horizontal row divider
        g.setColour(JazzLookAndFeel::Colors::borderOutline.withAlpha(0.4f));
        g.drawHorizontalLine(static_cast<int>(y + rowH - 1), static_cast<float>(area.getX()), static_cast<float>(area.getRight()));
    }
}

void DrumMachineComponent::drawTriggerPads(juce::Graphics& g, juce::Rectangle<int> area)
{
    g.setColour(JazzLookAndFeel::Colors::panelHeader);
    g.fillRect(area);

    g.setColour(JazzLookAndFeel::Colors::borderOutline);
    g.drawHorizontalLine(area.getY(), 0.0f, static_cast<float>(getWidth()));

    // Draw 8 MPC pads
    int padW = (area.getWidth() - 32) / 8;
    int padH = area.getHeight() - 20;

    for (int i = 0; i < kNumVoices; ++i)
    {
        juce::Rectangle<float> padRect(static_cast<float>(16 + i * padW + 3),
                                      static_cast<float>(area.getY() + 10),
                                      static_cast<float>(padW - 6),
                                      static_cast<float>(padH));

        bool isFlashing = (padFlashIdx == i);

        juce::Colour padColor = isFlashing ?
            voices[i].color.brighter(0.5f) :
            JazzLookAndFeel::Colors::panelBackground;

        g.setColour(padColor);
        g.fillRoundedRectangle(padRect, 8.0f);

        // Neon border
        g.setColour(isFlashing ? juce::Colours::white : voices[i].color.withAlpha(0.8f));
        g.drawRoundedRectangle(padRect, 8.0f, isFlashing ? 2.5f : 1.5f);

        // Pad label
        g.setColour(isFlashing ? juce::Colours::white : JazzLookAndFeel::Colors::textPrimary);
        g.setFont(juce::FontOptions(11.0f).withStyle("Bold"));
        g.drawText(voices[i].name, padRect.toNearestInt(), juce::Justification::centred);
    }
}

void DrumMachineComponent::mouseDown(const juce::MouseEvent& e)
{
    auto pos = e.getPosition();

    // 1. Check MPC Trigger Pads
    int padsTop = getHeight() - 130;
    if (pos.y >= padsTop)
    {
        int padW = (getWidth() - 32) / 8;
        int clickedPad = (pos.x - 16) / padW;

        if (clickedPad >= 0 && clickedPad < kNumVoices)
        {
            padFlashIdx = clickedPad;
            if (voices[clickedPad].isRetroSFX)
                engine.triggerRetroSFX(voices[clickedPad].sfxType, 1.0f);
            else
                engine.triggerDrum(voices[clickedPad].type, 1.0f, voices[clickedPad].pan);

            repaint();

            juce::Timer::callAfterDelay(120, [this] {
                padFlashIdx = -1;
                repaint();
            });
        }
        return;
    }

    // 2. Check Step Sequencer Grid
    int topOffset = 44;
    int headerW = 120;
    int gridH = getHeight() - topOffset - 130;
    int gridW = getWidth() - headerW - 16;

    if (pos.y >= topOffset && pos.y < topOffset + gridH && pos.x >= headerW && pos.x < headerW + gridW)
    {
        float rowH = static_cast<float>(gridH) / static_cast<float>(kNumVoices);
        float stepW = static_cast<float>(gridW) / static_cast<float>(kNumSteps);

        int voiceIdx = static_cast<int>((pos.y - topOffset) / rowH);
        int stepIdx = static_cast<int>((pos.x - headerW) / stepW);

        if (voiceIdx >= 0 && voiceIdx < kNumVoices && stepIdx >= 0 && stepIdx < kNumSteps)
        {
            pattern[voiceIdx][stepIdx] = !pattern[voiceIdx][stepIdx];

            // Audition sound if activated
            if (pattern[voiceIdx][stepIdx])
            {
                if (voices[voiceIdx].isRetroSFX)
                    engine.triggerRetroSFX(voices[voiceIdx].sfxType, voices[voiceIdx].volume);
                else
                    engine.triggerDrum(voices[voiceIdx].type, voices[voiceIdx].volume, voices[voiceIdx].pan);
            }

            repaint();
        }
    }
}

void DrumMachineComponent::resized()
{
    auto topArea = getLocalBounds().removeFromTop(44);
    topArea.removeFromLeft(400); // Leave title room

    int btnW = 95;
    int btnH = 26;
    int y = (44 - btnH) / 2;

    btnPresetTrap.setBounds(topArea.getX() + 8, y, btnW, btnH);
    btnPresetHouse.setBounds(topArea.getX() + 16 + btnW, y, btnW, btnH);
    btnPresetTechno.setBounds(topArea.getX() + 24 + btnW * 2, y, btnW, btnH);
    btnPresetArcade.setBounds(topArea.getX() + 32 + btnW * 3, y, btnW + 10, btnH);
    btnClear.setBounds(topArea.getX() + 50 + btnW * 4, y, 70, btnH);
}
