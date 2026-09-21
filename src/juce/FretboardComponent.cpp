//*****************************************************************************
// The JAZZ++ Midi Sequencer — JUCE 8 Edition
//
// Copyright (C) 2026 Raja Zahidi
// Interactive Fretboard Implementation
//*****************************************************************************

#include "FretboardComponent.h"
#include <cmath>

FretboardComponent::FretboardComponent(AudioMidiEngine& eng)
    : engine(eng)
{
    // Configure Root Selector
    int id = 1;
    for (const auto& name : JZChordScaleLibrary::GetRootNoteNames())
    {
        rootSelector.addItem(name, id++);
    }
    rootSelector.setSelectedId(1); // "C"

    // Configure Scale Selector
    id = 1;
    for (const auto& s : JZChordScaleLibrary::GetScales())
    {
        scaleSelector.addItem(s.mName, id++);
    }
    scaleSelector.setSelectedId(1); // "Major"

    addAndMakeVisible(rootSelector);
    addAndMakeVisible(scaleSelector);

    auto updateScale = [this] {
        setScale(rootSelector.getText(), scaleSelector.getText());
    };

    rootSelector.onChange = updateScale;
    scaleSelector.onChange = updateScale;

    updateScale();
}

void FretboardComponent::setScale(const juce::String& rootNote, const juce::String& scaleName)
{
    scaleRootPitch = JZChordScaleLibrary::NoteNameToSemitone(rootNote.toStdString());
    if (scaleRootPitch < 0) scaleRootPitch = 0;

    scaleIntervals.clear();
    for (const auto& s : JZChordScaleLibrary::GetScales())
    {
        if (s.mName == scaleName.toStdString())
        {
            scaleIntervals = s.mIntervals;
            break;
        }
    }

    if (scaleIntervals.empty())
    {
        scaleIntervals = { 0, 2, 4, 5, 7, 9, 11 };
    }

    repaint();
}

bool FretboardComponent::isPitchRoot(int pitch) const
{
    return (pitch % 12) == scaleRootPitch;
}

bool FretboardComponent::isPitchInScale(int pitch) const
{
    int semitone = (pitch % 12 - scaleRootPitch + 12) % 12;
    for (int interval : scaleIntervals)
    {
        if (interval == semitone)
            return true;
    }
    return false;
}

int FretboardComponent::getPitchAtFret(int stringIdx, int fret) const
{
    if (stringIdx < 0 || stringIdx >= static_cast<int>(stringTunings.size()))
        return 60;
    return stringTunings[stringIdx] + fret;
}

void FretboardComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    g.fillAll(JazzLookAndFeel::Colors::backgroundDark);

    // Top control strip
    auto topArea = bounds.removeFromTop(44);
    g.setColour(JazzLookAndFeel::Colors::panelHeader);
    g.fillRect(topArea);

    g.setColour(JazzLookAndFeel::Colors::neonCyan);
    g.setFont(juce::FontOptions(14.0f).withStyle("Bold"));
    g.drawText("INTERACTIVE GUITAR / BASS FRETBOARD", 16, 0, 320, 44, juce::Justification::centredLeft);

    // Fretboard Neck Area
    auto neckArea = bounds.reduced(24, 20);

    // Neck wood background (Ebony / Dark Rosewood)
    juce::ColourGradient neckGrad(juce::Colour(0xff181c24), neckArea.getX(), neckArea.getY(),
                                 juce::Colour(0xff0e1117), neckArea.getX(), neckArea.getBottom(), false);
    g.setGradientFill(neckGrad);
    g.fillRoundedRectangle(neckArea.toFloat(), 6.0f);

    g.setColour(JazzLookAndFeel::Colors::borderOutline);
    g.drawRoundedRectangle(neckArea.toFloat(), 6.0f, 1.5f);

    int numStrings = static_cast<int>(stringTunings.size());
    float stringSpacing = static_cast<float>(neckArea.getHeight()) / static_cast<float>(numStrings + 1);

    // Nut (fret 0)
    int nutW = 8;
    int nutX = neckArea.getX() + 40;
    g.setColour(juce::Colour(0xffe2e8f0)); // Bone nut
    g.fillRect(nutX, neckArea.getY(), nutW, neckArea.getHeight());

    // Fret positions (exponential scale simulation or evenly spaced for clean display)
    float neckLength = static_cast<float>(neckArea.getRight() - (nutX + nutW) - 10);
    float fretSpacing = neckLength / static_cast<float>(kNumFrets);

    // Draw Inlay markers
    auto drawInlay = [&](int fret, bool doubleDot) {
        float x = (nutX + nutW) + (fret - 0.5f) * fretSpacing;
        float cy = neckArea.getCentreY();
        g.setColour(juce::Colour(0xffcbd5e1).withAlpha(0.6f));
        if (doubleDot)
        {
            g.fillEllipse(x - 4.0f, cy - 22.0f, 8.0f, 8.0f);
            g.fillEllipse(x - 4.0f, cy + 14.0f, 8.0f, 8.0f);
        }
        else
        {
            g.fillEllipse(x - 4.0f, cy - 4.0f, 8.0f, 8.0f);
        }
    };

    drawInlay(3, false);
    drawInlay(5, false);
    drawInlay(7, false);
    drawInlay(9, false);
    drawInlay(12, true);
    drawInlay(15, false);
    drawInlay(17, false);
    drawInlay(19, false);
    drawInlay(21, false);
    drawInlay(24, true);

    // Fret wires & numbers
    for (int f = 1; f <= kNumFrets; ++f)
    {
        float x = (nutX + nutW) + f * fretSpacing;
        g.setColour(juce::Colour(0xff94a3b8)); // Nickel fret wire
        g.drawVerticalLine(static_cast<int>(x), static_cast<float>(neckArea.getY()), static_cast<float>(neckArea.getBottom()));

        // Fret number below neck
        g.setColour(JazzLookAndFeel::Colors::textSecondary);
        g.setFont(10.0f);
        g.drawText(juce::String(f), static_cast<int>(x - fretSpacing), neckArea.getBottom() + 2, static_cast<int>(fretSpacing), 16, juce::Justification::centred);
    }

    // Strings
    for (int s = 0; s < numStrings; ++s)
    {
        float y = neckArea.getY() + (s + 1) * stringSpacing;
        float thickness = 1.0f + (static_cast<float>(s) * 0.5f); // Thicker for lower pitch

        g.setColour(juce::Colour(0xffcbd5e1));
        g.drawLine(static_cast<float>(neckArea.getX()), y, static_cast<float>(neckArea.getRight()), y, thickness);

        // String name on headstock/nut area
        g.setColour(JazzLookAndFeel::Colors::neonCyan);
        g.setFont(juce::FontOptions(11.0f).withStyle("Bold"));
        g.drawText(stringNames[s], neckArea.getX() + 6, static_cast<int>(y - 8), 30, 16, juce::Justification::centredLeft);

        // Note markers across frets (0 to 24)
        for (int f = 0; f <= kNumFrets; ++f)
        {
            int pitch = getPitchAtFret(s, f);
            float noteX = (f == 0) ? (neckArea.getX() + 28.0f) : ((nutX + nutW) + (f - 0.5f) * fretSpacing);

            bool isRoot = isPitchRoot(pitch);
            bool inScale = isPitchInScale(pitch);

            if (isRoot || inScale)
            {
                juce::Colour markerCol = isRoot ?
                    JazzLookAndFeel::Colors::neonPurple :
                    JazzLookAndFeel::Colors::neonCyan;

                float radius = isRoot ? 11.0f : 9.5f;

                // Glowing outer ring for root
                if (isRoot)
                {
                    g.setColour(markerCol.withAlpha(0.3f));
                    g.fillEllipse(noteX - radius - 3.0f, y - radius - 3.0f, (radius + 3.0f) * 2.0f, (radius + 3.0f) * 2.0f);
                }

                g.setColour(markerCol);
                g.fillEllipse(noteX - radius, y - radius, radius * 2.0f, radius * 2.0f);

                g.setColour(juce::Colours::white);
                g.drawEllipse(noteX - radius, y - radius, radius * 2.0f, radius * 2.0f, 1.2f);

                // Note text
                g.setColour(juce::Colours::black);
                g.setFont(juce::FontOptions(isRoot ? 10.0f : 9.0f).withStyle("Bold"));
                g.drawText(JZChordScaleLibrary::PitchToNoteName(pitch),
                           static_cast<int>(noteX - radius), static_cast<int>(y - radius),
                           static_cast<int>(radius * 2.0f), static_cast<int>(radius * 2.0f),
                           juce::Justification::centred);
            }
        }
    }
}

void FretboardComponent::mouseDown(const juce::MouseEvent& e)
{
    auto pos = e.getPosition();
    auto neckArea = getLocalBounds().reduced(24, 20);
    neckArea.removeFromTop(44);

    if (neckArea.contains(pos))
    {
        int numStrings = static_cast<int>(stringTunings.size());
        float stringSpacing = static_cast<float>(neckArea.getHeight()) / static_cast<float>(numStrings + 1);

        int stringIdx = static_cast<int>((pos.y - neckArea.getY() - stringSpacing * 0.5f) / stringSpacing);
        stringIdx = juce::jlimit(0, numStrings - 1, stringIdx);

        int nutW = 8;
        int nutX = neckArea.getX() + 40;
        float neckLength = static_cast<float>(neckArea.getRight() - (nutX + nutW) - 10);
        float fretSpacing = neckLength / static_cast<float>(kNumFrets);

        int fret = 0;
        if (pos.x >= nutX + nutW)
        {
            fret = static_cast<int>((pos.x - (nutX + nutW)) / fretSpacing) + 1;
            fret = juce::jlimit(1, kNumFrets, fret);
        }

        auditionPitch = getPitchAtFret(stringIdx, fret);
        engine.sendNoteOn(1, auditionPitch, 0.85f);
        repaint();
    }
}

void FretboardComponent::mouseUp(const juce::MouseEvent& /*e*/)
{
    if (auditionPitch >= 0)
    {
        engine.sendNoteOff(1, auditionPitch);
        auditionPitch = -1;
        repaint();
    }
}

void FretboardComponent::resized()
{
    auto topArea = getLocalBounds().removeFromTop(44);
    topArea.removeFromLeft(330);

    int comboH = 26;
    int y = (44 - comboH) / 2;

    rootSelector.setBounds(topArea.getX() + 8, y, 70, comboH);
    scaleSelector.setBounds(topArea.getX() + 86, y, 160, comboH);
}
