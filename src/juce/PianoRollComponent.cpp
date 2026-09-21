//*****************************************************************************
// The JAZZ++ Midi Sequencer — JUCE 8 Edition
//
// Copyright (C) 2026 Raja Zahidi
// Interactive Piano Roll Editor Implementation
//*****************************************************************************

#include "PianoRollComponent.h"
#include <algorithm>

PianoRollComponent::PianoRollComponent(AudioMidiEngine& eng)
    : engine(eng)
{
    engine.addEngineListener(this);

    // Default scale: C Natural Minor
    setScale("C", "Natural Minor");

    // Add sample notes (C Minor Synth Riff)
    notes.push_back({ 60, 0, 240, 105, false });        // C4
    notes.push_back({ 63, 240, 240, 95, false });      // Eb4
    notes.push_back({ 67, 480, 240, 110, false });     // G4
    notes.push_back({ 70, 720, 240, 100, false });     // Bb4
    notes.push_back({ 72, 960, 480, 115, false });     // C5
    notes.push_back({ 70, 1440, 240, 90, false });     // Bb4
    notes.push_back({ 67, 1680, 240, 100, false });    // G4
}

PianoRollComponent::~PianoRollComponent()
{
    engine.removeEngineListener(this);
}

void PianoRollComponent::setScale(const juce::String& rootNote, const juce::String& scaleName)
{
    scaleRootPitch = JZChordScaleLibrary::NoteNameToSemitone(rootNote.toStdString());
    if (scaleRootPitch < 0) scaleRootPitch = 0;

    scaleIntervals.clear();
    const auto& scales = JZChordScaleLibrary::GetScales();
    for (const auto& s : scales)
    {
        if (s.mName == scaleName.toStdString())
        {
            scaleIntervals = s.mIntervals;
            break;
        }
    }

    if (scaleIntervals.empty())
    {
        scaleIntervals = { 0, 2, 4, 5, 7, 9, 11 }; // Major default
    }

    repaint();
}

bool PianoRollComponent::isPitchRoot(int pitch) const
{
    return (pitch % 12) == scaleRootPitch;
}

bool PianoRollComponent::isPitchInScale(int pitch) const
{
    int semitone = (pitch % 12 - scaleRootPitch + 12) % 12;
    for (int interval : scaleIntervals)
    {
        if (interval == semitone)
            return true;
    }
    return false;
}

int64_t PianoRollComponent::xToTick(float x) const
{
    float relativeX = x - kKeyboardWidth;
    if (relativeX < 0) relativeX = 0;
    float bars = relativeX / pixelsPerBar;
    return static_cast<int64_t>(bars * (kTicksPerQuarter * 4));
}

float PianoRollComponent::tickToX(int64_t tick) const
{
    float bars = static_cast<float>(tick) / static_cast<float>(kTicksPerQuarter * 4);
    return kKeyboardWidth + bars * pixelsPerBar;
}

int PianoRollComponent::yToPitch(float y) const
{
    float gridY = y - kRulerHeight;
    int row = static_cast<int>(gridY / kKeyHeight);
    int pitch = kHighestPitch - row;
    return juce::jlimit(kLowestPitch, kHighestPitch, pitch);
}

float PianoRollComponent::pitchToY(int pitch) const
{
    int row = kHighestPitch - pitch;
    return static_cast<float>(kRulerHeight + row * kKeyHeight);
}

void PianoRollComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    g.fillAll(JazzLookAndFeel::Colors::backgroundDark);

    auto rulerArea = bounds.removeFromTop(kRulerHeight);
    auto velocityArea = bounds.removeFromBottom(kVelocityHeight);
    auto keyboardArea = bounds.removeFromLeft(kKeyboardWidth);
    auto gridArea = bounds;

    drawRuler(g, rulerArea);
    drawKeyboard(g, keyboardArea);
    drawGrid(g, gridArea);
    drawNotes(g, gridArea);
    drawVelocityLane(g, velocityArea);
    drawPlayhead(g, gridArea);
}

void PianoRollComponent::drawRuler(juce::Graphics& g, juce::Rectangle<int> area)
{
    g.setColour(JazzLookAndFeel::Colors::panelHeader);
    g.fillRect(area);

    // Keyboard corner
    g.setColour(JazzLookAndFeel::Colors::panelBackground);
    g.fillRect(area.removeFromLeft(kKeyboardWidth));

    // Ruler bars and beats
    int numBars = static_cast<int>(area.getWidth() / pixelsPerBar) + 2;
    for (int bar = 0; bar < numBars; ++bar)
    {
        float barX = area.getX() + bar * pixelsPerBar;
        g.setColour(JazzLookAndFeel::Colors::borderOutline);
        g.drawVerticalLine(static_cast<int>(barX), static_cast<float>(area.getY()), static_cast<float>(area.getBottom()));

        g.setColour(JazzLookAndFeel::Colors::textPrimary);
        g.setFont(10.0f);
        g.drawText(juce::String(bar + 1), static_cast<int>(barX + 4), area.getY() + 2, 30, 14, juce::Justification::topLeft);

        for (int beat = 1; beat < 4; ++beat)
        {
            float beatX = barX + (beat * pixelsPerBar / 4.0f);
            g.setColour(JazzLookAndFeel::Colors::borderOutline.withAlpha(0.4f));
            g.drawVerticalLine(static_cast<int>(beatX), static_cast<float>(area.getY() + 12), static_cast<float>(area.getBottom()));
        }
    }

    g.setColour(JazzLookAndFeel::Colors::borderOutline);
    g.drawHorizontalLine(area.getBottom() - 1, 0.0f, static_cast<float>(getWidth()));
}

void PianoRollComponent::drawKeyboard(juce::Graphics& g, juce::Rectangle<int> area)
{
    g.setColour(JazzLookAndFeel::Colors::panelBackground);
    g.fillRect(area);

    for (int pitch = kLowestPitch; pitch <= kHighestPitch; ++pitch)
    {
        float y = pitchToY(pitch);
        juce::Rectangle<float> keyRect(static_cast<float>(area.getX()), y,
                                      static_cast<float>(area.getWidth()), static_cast<float>(kKeyHeight));

        int noteInOctave = pitch % 12;
        bool isBlackKey = (noteInOctave == 1 || noteInOctave == 3 || noteInOctave == 6 ||
                           noteInOctave == 8 || noteInOctave == 10);

        if (pitch == auditionPitch)
        {
            g.setColour(JazzLookAndFeel::Colors::neonCyan);
        }
        else if (isBlackKey)
        {
            g.setColour(JazzLookAndFeel::Colors::panelHeader.darker(0.3f));
        }
        else
        {
            g.setColour(JazzLookAndFeel::Colors::panelHeader.brighter(0.15f));
        }

        g.fillRect(keyRect);

        // Key border
        g.setColour(JazzLookAndFeel::Colors::borderOutline.withAlpha(0.6f));
        g.drawHorizontalLine(static_cast<int>(y + kKeyHeight - 1), keyRect.getX(), keyRect.getRight());

        // Octave / Pitch label on C keys
        if (noteInOctave == 0)
        {
            g.setColour(JazzLookAndFeel::Colors::textPrimary);
            g.setFont(juce::FontOptions(9.0f).withStyle("Bold"));
            int octave = (pitch / 12) - 1;
            g.drawText("C" + juce::String(octave), keyRect.toNearestInt().reduced(2), juce::Justification::centredRight);
        }
        else if (isPitchRoot(pitch))
        {
            g.setColour(JazzLookAndFeel::Colors::neonPurple);
            g.fillEllipse(keyRect.getRight() - 10.0f, keyRect.getCentreY() - 3.0f, 6.0f, 6.0f);
        }
    }

    // Vertical separator
    g.setColour(JazzLookAndFeel::Colors::borderOutline);
    g.drawVerticalLine(area.getRight() - 1, static_cast<float>(area.getY()), static_cast<float>(area.getBottom()));
}

void PianoRollComponent::drawGrid(juce::Graphics& g, juce::Rectangle<int> area)
{
    // Draw horizontal pitch rows
    for (int pitch = kLowestPitch; pitch <= kHighestPitch; ++pitch)
    {
        float y = pitchToY(pitch);
        juce::Rectangle<float> rowRect(static_cast<float>(area.getX()), y,
                                       static_cast<float>(area.getWidth()), static_cast<float>(kKeyHeight));

        int noteInOctave = pitch % 12;
        bool isBlackKey = (noteInOctave == 1 || noteInOctave == 3 || noteInOctave == 6 ||
                           noteInOctave == 8 || noteInOctave == 10);

        if (isPitchRoot(pitch))
        {
            g.setColour(JazzLookAndFeel::Colors::neonPurple.withAlpha(0.15f));
        }
        else if (isPitchInScale(pitch))
        {
            g.setColour(JazzLookAndFeel::Colors::neonCyan.withAlpha(0.08f));
        }
        else if (isBlackKey)
        {
            g.setColour(JazzLookAndFeel::Colors::backgroundDark.darker(0.3f));
        }
        else
        {
            g.setColour(JazzLookAndFeel::Colors::panelBackground.withAlpha(0.3f));
        }

        g.fillRect(rowRect);

        g.setColour(JazzLookAndFeel::Colors::borderOutline.withAlpha(0.15f));
        g.drawHorizontalLine(static_cast<int>(y + kKeyHeight - 1), rowRect.getX(), rowRect.getRight());
    }

    // Draw vertical bar and beat grid lines
    int numBars = static_cast<int>(area.getWidth() / pixelsPerBar) + 2;
    for (int bar = 0; bar < numBars; ++bar)
    {
        float barX = area.getX() + bar * pixelsPerBar;
        g.setColour(JazzLookAndFeel::Colors::borderOutline.withAlpha(0.4f));
        g.drawVerticalLine(static_cast<int>(barX), static_cast<float>(area.getY()), static_cast<float>(area.getBottom()));

        for (int beat = 1; beat < 4; ++beat)
        {
            float beatX = barX + (beat * pixelsPerBar / 4.0f);
            g.setColour(JazzLookAndFeel::Colors::borderOutline.withAlpha(0.18f));
            g.drawVerticalLine(static_cast<int>(beatX), static_cast<float>(area.getY()), static_cast<float>(area.getBottom()));

            // 16th subdivisions
            for (int sub = 1; sub < 4; ++sub)
            {
                float subX = beatX - (pixelsPerBar / 16.0f) * sub;
                g.setColour(JazzLookAndFeel::Colors::borderOutline.withAlpha(0.06f));
                g.drawVerticalLine(static_cast<int>(subX), static_cast<float>(area.getY()), static_cast<float>(area.getBottom()));
            }
        }
    }
}

void PianoRollComponent::drawNotes(juce::Graphics& g, juce::Rectangle<int> /*area*/)
{
    for (size_t i = 0; i < notes.size(); ++i)
    {
        const auto& n = notes[i];
        float startX = tickToX(n.startTick);
        float endX = tickToX(n.startTick + n.lengthTicks);
        float noteW = std::max(6.0f, endX - startX);
        float noteY = pitchToY(n.pitch);

        juce::Rectangle<float> noteRect(startX + 1.0f, noteY + 1.0f, noteW - 2.0f, static_cast<float>(kKeyHeight - 2));

        bool isSelected = (selectedNoteIdx == static_cast<int>(i));

        // Note gradient fill
        float velAlpha = 0.5f + (static_cast<float>(n.velocity) / 127.0f) * 0.5f;
        juce::Colour baseColor = isPitchRoot(n.pitch) ?
            JazzLookAndFeel::Colors::neonPurple :
            JazzLookAndFeel::Colors::neonCyan;

        juce::ColourGradient grad(baseColor.withAlpha(velAlpha), noteRect.getX(), noteRect.getY(),
                                  baseColor.darker(0.3f).withAlpha(velAlpha), noteRect.getX(), noteRect.getBottom(), false);
        g.setGradientFill(grad);
        g.fillRoundedRectangle(noteRect, 3.0f);

        // Border
        g.setColour(isSelected ? juce::Colours::white : baseColor.brighter(0.3f));
        g.drawRoundedRectangle(noteRect, 3.0f, isSelected ? 2.0f : 1.0f);

        // Note name label inside if wide enough
        if (noteW > 24.0f)
        {
            g.setColour(JazzLookAndFeel::Colors::textPrimary);
            g.setFont(9.0f);
            g.drawText(JZChordScaleLibrary::PitchToNoteName(n.pitch), noteRect.toNearestInt().reduced(2), juce::Justification::centredLeft, true);
        }
    }
}

void PianoRollComponent::drawVelocityLane(juce::Graphics& g, juce::Rectangle<int> area)
{
    g.setColour(JazzLookAndFeel::Colors::panelBackground);
    g.fillRect(area);

    // Left label
    g.setColour(JazzLookAndFeel::Colors::panelHeader);
    g.fillRect(area.removeFromLeft(kKeyboardWidth));
    g.setColour(JazzLookAndFeel::Colors::textSecondary);
    g.setFont(10.0f);
    g.drawText("VELOCITY", 4, area.getY() + 4, kKeyboardWidth - 8, 16, juce::Justification::centredLeft);

    // Divider line
    g.setColour(JazzLookAndFeel::Colors::borderOutline);
    g.drawHorizontalLine(area.getY(), 0.0f, static_cast<float>(getWidth()));

    // Draw velocity stalks for notes
    for (size_t i = 0; i < notes.size(); ++i)
    {
        const auto& n = notes[i];
        float noteX = tickToX(n.startTick) + 3.0f;
        float velHeight = (static_cast<float>(n.velocity) / 127.0f) * (area.getHeight() - 14);
        float stalkY = area.getBottom() - velHeight;

        bool isSelected = (selectedNoteIdx == static_cast<int>(i));
        juce::Colour stalkColor = isSelected ? juce::Colours::white : JazzLookAndFeel::Colors::neonCyan;

        g.setColour(stalkColor);
        g.drawVerticalLine(static_cast<int>(noteX), stalkY, static_cast<float>(area.getBottom()));
        g.fillEllipse(noteX - 3.0f, stalkY - 3.0f, 6.0f, 6.0f);
    }
}

void PianoRollComponent::drawPlayhead(juce::Graphics& g, juce::Rectangle<int> area)
{
    float playheadX = tickToX(currentTick);
    if (playheadX < area.getX() || playheadX > area.getRight())
        return;

    g.setColour(JazzLookAndFeel::Colors::neonCyan.withAlpha(0.4f));
    g.drawVerticalLine(static_cast<int>(playheadX) - 1, static_cast<float>(area.getY()), static_cast<float>(area.getBottom()));
    g.drawVerticalLine(static_cast<int>(playheadX) + 1, static_cast<float>(area.getY()), static_cast<float>(area.getBottom()));

    g.setColour(JazzLookAndFeel::Colors::neonCyan);
    g.drawVerticalLine(static_cast<int>(playheadX), static_cast<float>(area.getY()), static_cast<float>(area.getBottom()));
}

void PianoRollComponent::mouseDown(const juce::MouseEvent& e)
{
    auto pos = e.getPosition();

    // 1. Keyboard click (Audition note)
    if (pos.x < kKeyboardWidth && pos.y >= kRulerHeight && pos.y < getHeight() - kVelocityHeight)
    {
        auditionPitch = yToPitch(static_cast<float>(pos.y));
        engine.sendNoteOn(1, auditionPitch, 0.8f);
        repaint();
        return;
    }

    // 2. Velocity lane click
    if (pos.y >= getHeight() - kVelocityHeight)
    {
        if (selectedNoteIdx >= 0 && selectedNoteIdx < static_cast<int>(notes.size()))
        {
            isDraggingVelocity = true;
            float normY = 1.0f - static_cast<float>(pos.y - (getHeight() - kVelocityHeight)) / static_cast<float>(kVelocityHeight);
            notes[selectedNoteIdx].velocity = juce::jlimit(1, 127, static_cast<int>(normY * 127.0f));
            repaint();
        }
        return;
    }

    // 3. Ruler click
    if (pos.y < kRulerHeight && pos.x >= kKeyboardWidth)
    {
        isDraggingPlayhead = true;
        int64_t tick = xToTick(static_cast<float>(pos.x));
        engine.setPlayheadClock(tick);
        repaint();
        return;
    }

    // 4. Grid click: check if clicked on existing note
    if (pos.x >= kKeyboardWidth && pos.y >= kRulerHeight && pos.y < getHeight() - kVelocityHeight)
    {
        int clickPitch = yToPitch(static_cast<float>(pos.y));
        int64_t clickTick = xToTick(static_cast<float>(pos.x));

        selectedNoteIdx = -1;
        for (size_t i = 0; i < notes.size(); ++i)
        {
            auto& n = notes[i];
            if (n.pitch == clickPitch && clickTick >= n.startTick && clickTick <= n.startTick + n.lengthTicks)
            {
                selectedNoteIdx = static_cast<int>(i);

                if (e.mods.isRightButtonDown())
                {
                    // Delete note
                    notes.erase(notes.begin() + i);
                    selectedNoteIdx = -1;
                    repaint();
                    return;
                }

                // Check resize handle (right 6 pixels)
                float noteEndX = tickToX(n.startTick + n.lengthTicks);
                if (std::abs(pos.x - noteEndX) <= 6.0f)
                {
                    isDraggingNoteResize = true;
                }
                else
                {
                    isDraggingNoteMove = true;
                    dragStartTick = clickTick - n.startTick;
                    dragPitchOffset = clickPitch - n.pitch;
                }

                // Play preview note
                engine.sendNoteOn(1, n.pitch, static_cast<float>(n.velocity) / 127.0f);
                repaint();
                return;
            }
        }

        // Clicked on empty grid -> Create new note
        if (!e.mods.isRightButtonDown())
        {
            // Snap to 16th note (120 ticks)
            int snap = kTicksPerQuarter / 4;
            int64_t snappedTick = (clickTick / snap) * snap;

            PianoRollNote newNote;
            newNote.pitch = clickPitch;
            newNote.startTick = snappedTick;
            newNote.lengthTicks = 240; // 8th note
            newNote.velocity = 100;
            notes.push_back(newNote);
            selectedNoteIdx = static_cast<int>(notes.size() - 1);

            engine.sendNoteOn(1, newNote.pitch, 0.8f);
            repaint();
        }
    }
}

void PianoRollComponent::mouseDrag(const juce::MouseEvent& e)
{
    auto pos = e.getPosition();

    if (isDraggingPlayhead)
    {
        int64_t tick = xToTick(static_cast<float>(pos.x));
        engine.setPlayheadClock(tick);
        repaint();
    }
    else if (isDraggingVelocity && selectedNoteIdx >= 0 && selectedNoteIdx < static_cast<int>(notes.size()))
    {
        float normY = 1.0f - static_cast<float>(pos.y - (getHeight() - kVelocityHeight)) / static_cast<float>(kVelocityHeight);
        notes[selectedNoteIdx].velocity = juce::jlimit(1, 127, static_cast<int>(normY * 127.0f));
        repaint();
    }
    else if (isDraggingNoteResize && selectedNoteIdx >= 0 && selectedNoteIdx < static_cast<int>(notes.size()))
    {
        int64_t curTick = xToTick(static_cast<float>(pos.x));
        int snap = kTicksPerQuarter / 4;
        curTick = (curTick / snap) * snap;
        int64_t newLen = curTick - notes[selectedNoteIdx].startTick;
        notes[selectedNoteIdx].lengthTicks = std::max<int64_t>(snap, newLen);
        repaint();
    }
    else if (isDraggingNoteMove && selectedNoteIdx >= 0 && selectedNoteIdx < static_cast<int>(notes.size()))
    {
        int64_t curTick = xToTick(static_cast<float>(pos.x)) - dragStartTick;
        int snap = kTicksPerQuarter / 4;
        curTick = (curTick / snap) * snap;
        if (curTick < 0) curTick = 0;

        int newPitch = yToPitch(static_cast<float>(pos.y)) - dragPitchOffset;
        newPitch = juce::jlimit(kLowestPitch, kHighestPitch, newPitch);

        notes[selectedNoteIdx].startTick = curTick;
        notes[selectedNoteIdx].pitch = newPitch;
        repaint();
    }
}

void PianoRollComponent::mouseUp(const juce::MouseEvent& /*e*/)
{
    if (auditionPitch >= 0)
    {
        engine.sendNoteOff(1, auditionPitch);
        auditionPitch = -1;
    }

    if (selectedNoteIdx >= 0 && selectedNoteIdx < static_cast<int>(notes.size()))
    {
        engine.sendNoteOff(1, notes[selectedNoteIdx].pitch);
    }

    isDraggingPlayhead = false;
    isDraggingNoteMove = false;
    isDraggingNoteResize = false;
    isDraggingVelocity = false;
    repaint();
}

void PianoRollComponent::mouseDoubleClick(const juce::MouseEvent& e)
{
    // Double click to delete note
    auto pos = e.getPosition();
    int clickPitch = yToPitch(static_cast<float>(pos.y));
    int64_t clickTick = xToTick(static_cast<float>(pos.x));

    for (size_t i = 0; i < notes.size(); ++i)
    {
        if (notes[i].pitch == clickPitch && clickTick >= notes[i].startTick && clickTick <= notes[i].startTick + notes[i].lengthTicks)
        {
            notes.erase(notes.begin() + i);
            selectedNoteIdx = -1;
            repaint();
            return;
        }
    }
}

void PianoRollComponent::playheadMoved(int /*bar*/, int /*beat*/, int /*tick*/, double /*seconds*/)
{
    currentTick = engine.getPlayheadClock();
    repaint();
}

void PianoRollComponent::playbackStateChanged(bool /*isPlaying*/)
{
    repaint();
}

void PianoRollComponent::resized()
{
}
