//*****************************************************************************
// The JAZZ++ Midi Sequencer — JUCE 8 Edition
//
// Copyright (C) 2026 Raja Zahidi
// Arranger Multi-Track Timeline Implementation
//*****************************************************************************

#include "ArrangerComponent.h"
#include <algorithm>

ArrangerComponent::ArrangerComponent(AudioMidiEngine& eng)
    : engine(eng)
{
    engine.addEngineListener(this);

    // Populate with initial demo project tracks and clips
    addTrack("808 Drums", JazzLookAndFeel::Colors::neonRed, 10);
    addTrack("Bassline", JazzLookAndFeel::Colors::neonPurple, 1);
    addTrack("Chord Pad", JazzLookAndFeel::Colors::neonCyan, 2);
    addTrack("Synth Lead", JazzLookAndFeel::Colors::neonEmerald, 3);
    addTrack("Retro SFX", JazzLookAndFeel::Colors::neonAmber, 4);

    // Add demo clips
    // 808 Drums: 4 bars
    for (int b = 0; b < 4; ++b)
    {
        ArrangerClip c;
        c.name = "808 Beat " + juce::String(b + 1);
        c.startTick = b * (kTicksPerQuarter * 4);
        c.lengthTicks = kTicksPerQuarter * 4;
        c.color = tracks[0].color;
        // Mock mini notes for visual waveform/piano preview
        c.miniNotes = { {36, 0}, {42, 120}, {38, 240}, {42, 360}, {36, 480}, {38, 720} };
        tracks[0].clips.push_back(c);
    }

    // Bassline: Bars 1 to 4
    for (int b = 0; b < 4; ++b)
    {
        ArrangerClip c;
        c.name = "Sub Bass " + juce::String(b + 1);
        c.startTick = b * (kTicksPerQuarter * 4);
        c.lengthTicks = kTicksPerQuarter * 4;
        c.color = tracks[1].color;
        c.miniNotes = { {36, 0}, {36, 240}, {39, 480}, {41, 720} };
        tracks[1].clips.push_back(c);
    }

    // Chord Pad: Bars 1 to 4
    for (int b = 0; b < 4; ++b)
    {
        ArrangerClip c;
        c.name = "Neon Pad " + juce::String(b + 1);
        c.startTick = b * (kTicksPerQuarter * 4);
        c.lengthTicks = kTicksPerQuarter * 4;
        c.color = tracks[2].color;
        c.miniNotes = { {60, 0}, {63, 0}, {67, 0}, {70, 0} };
        tracks[2].clips.push_back(c);
    }

    // Lead: Bars 2 & 4
    {
        ArrangerClip c1;
        c1.name = "Lead Riff A";
        c1.startTick = 1 * (kTicksPerQuarter * 4);
        c1.lengthTicks = kTicksPerQuarter * 4;
        c1.color = tracks[3].color;
        c1.miniNotes = { {72, 0}, {75, 120}, {74, 240}, {79, 360} };
        tracks[3].clips.push_back(c1);

        ArrangerClip c2;
        c2.name = "Lead Riff B";
        c2.startTick = 3 * (kTicksPerQuarter * 4);
        c2.lengthTicks = kTicksPerQuarter * 4;
        c2.color = tracks[3].color;
        c2.miniNotes = { {72, 0}, {75, 120}, {77, 240}, {82, 360} };
        tracks[3].clips.push_back(c2);
    }
}

ArrangerComponent::~ArrangerComponent()
{
    engine.removeEngineListener(this);
}

void ArrangerComponent::addTrack(const juce::String& name, juce::Colour color, int channel)
{
    ArrangerTrack t;
    t.name = name;
    t.color = color;
    t.midiChannel = channel;
    tracks.push_back(t);
    repaint();
}

int64_t ArrangerComponent::xToTick(float x) const
{
    float relativeX = x - kHeaderWidth;
    if (relativeX < 0) relativeX = 0;
    float bars = relativeX / pixelsPerBar;
    return static_cast<int64_t>(bars * (kTicksPerQuarter * 4));
}

float ArrangerComponent::tickToX(int64_t tick) const
{
    float bars = static_cast<float>(tick) / static_cast<float>(kTicksPerQuarter * 4);
    return kHeaderWidth + bars * pixelsPerBar;
}

void ArrangerComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Background
    g.fillAll(JazzLookAndFeel::Colors::backgroundDark);

    auto rulerArea = bounds.removeFromTop(kRulerHeight);
    auto headersArea = bounds.removeFromLeft(kHeaderWidth);
    auto lanesArea = bounds;

    drawRuler(g, rulerArea);
    drawTrackHeaders(g, headersArea);
    drawLanes(g, lanesArea);
    drawPlayhead(g, lanesArea);
}

void ArrangerComponent::drawRuler(juce::Graphics& g, juce::Rectangle<int> area)
{
    g.setColour(JazzLookAndFeel::Colors::panelHeader);
    g.fillRect(area);

    // Header corner
    g.setColour(JazzLookAndFeel::Colors::panelBackground);
    g.fillRect(area.removeFromLeft(kHeaderWidth));
    g.setColour(JazzLookAndFeel::Colors::textSecondary);
    g.setFont(12.0f);
    g.drawText("TRACKS / TIMELINE", 8, 0, kHeaderWidth - 16, kRulerHeight, juce::Justification::centredLeft);

    // Ruler ticks & bar numbers
    int numBars = static_cast<int>(area.getWidth() / pixelsPerBar) + 2;
    for (int bar = 0; bar < numBars; ++bar)
    {
        float barX = area.getX() + bar * pixelsPerBar;
        g.setColour(JazzLookAndFeel::Colors::borderOutline);
        g.drawVerticalLine(static_cast<int>(barX), static_cast<float>(area.getY()), static_cast<float>(area.getBottom()));

        g.setColour(JazzLookAndFeel::Colors::textPrimary);
        g.setFont(11.0f);
        g.drawText(juce::String(bar + 1), static_cast<int>(barX + 4), area.getY() + 2, 40, 14, juce::Justification::topLeft);

        // Beat markers
        for (int beat = 1; beat < 4; ++beat)
        {
            float beatX = barX + (beat * pixelsPerBar / 4.0f);
            g.setColour(JazzLookAndFeel::Colors::borderOutline.withAlpha(0.4f));
            g.drawVerticalLine(static_cast<int>(beatX), static_cast<float>(area.getY() + 16), static_cast<float>(area.getBottom()));
        }
    }

    // Ruler bottom divider
    g.setColour(JazzLookAndFeel::Colors::borderOutline);
    g.drawHorizontalLine(area.getBottom() - 1, 0.0f, static_cast<float>(getWidth()));
}

void ArrangerComponent::drawTrackHeaders(juce::Graphics& g, juce::Rectangle<int> area)
{
    g.setColour(JazzLookAndFeel::Colors::panelBackground);
    g.fillRect(area);

    int y = area.getY();
    for (size_t i = 0; i < tracks.size(); ++i)
    {
        auto& t = tracks[i];
        juce::Rectangle<int> row(area.getX(), y, area.getWidth(), kTrackHeight);

        bool isSelected = (selectedTrackIdx == static_cast<int>(i));
        if (isSelected)
        {
            g.setColour(JazzLookAndFeel::Colors::controlFill.brighter(0.1f));
            g.fillRect(row);
        }

        // Left color accent strip
        g.setColour(t.color);
        g.fillRect(row.getX(), row.getY(), 4, row.getHeight());

        // Track Name
        g.setColour(JazzLookAndFeel::Colors::textPrimary);
        g.setFont(13.0f);
        g.drawText(t.name, row.getX() + 12, row.getY() + 6, 110, 20, juce::Justification::centredLeft, true);

        // Track controls: M, S, R buttons
        auto btnArea = row.removeFromRight(76).reduced(4);
        int btnW = 20;

        // Mute button
        juce::Rectangle<int> mRect(btnArea.getX(), btnArea.getY() + 4, btnW, 20);
        g.setColour(t.isMuted ? JazzLookAndFeel::Colors::neonRed : JazzLookAndFeel::Colors::controlFill);
        g.fillRoundedRectangle(mRect.toFloat(), 3.0f);
        g.setColour(t.isMuted ? juce::Colours::white : JazzLookAndFeel::Colors::textSecondary);
        g.setFont(10.0f);
        g.drawText("M", mRect, juce::Justification::centred);

        // Solo button
        juce::Rectangle<int> sRect(btnArea.getX() + 24, btnArea.getY() + 4, btnW, 20);
        g.setColour(t.isSolo ? JazzLookAndFeel::Colors::neonAmber : JazzLookAndFeel::Colors::controlFill);
        g.fillRoundedRectangle(sRect.toFloat(), 3.0f);
        g.setColour(t.isSolo ? juce::Colours::black : JazzLookAndFeel::Colors::textSecondary);
        g.drawText("S", sRect, juce::Justification::centred);

        // Arm button
        juce::Rectangle<int> rRect(btnArea.getX() + 48, btnArea.getY() + 4, btnW, 20);
        g.setColour(t.isArmed ? JazzLookAndFeel::Colors::neonRed : JazzLookAndFeel::Colors::controlFill);
        g.fillRoundedRectangle(rRect.toFloat(), 3.0f);
        g.setColour(t.isArmed ? juce::Colours::white : JazzLookAndFeel::Colors::textSecondary);
        g.drawText("R", rRect, juce::Justification::centred);

        // Divider
        g.setColour(JazzLookAndFeel::Colors::borderOutline);
        g.drawHorizontalLine(y + kTrackHeight - 1, static_cast<float>(area.getX()), static_cast<float>(area.getRight()));

        y += kTrackHeight;
    }

    // Right divider between headers and lanes
    g.setColour(JazzLookAndFeel::Colors::borderOutline);
    g.drawVerticalLine(area.getRight() - 1, static_cast<float>(area.getY()), static_cast<float>(area.getBottom()));
}

void ArrangerComponent::drawLanes(juce::Graphics& g, juce::Rectangle<int> area)
{
    // Alternate row background & grid
    int y = area.getY();
    for (size_t i = 0; i < tracks.size(); ++i)
    {
        juce::Rectangle<int> row(area.getX(), y, area.getWidth(), kTrackHeight);

        g.setColour(i % 2 == 0 ? JazzLookAndFeel::Colors::backgroundDark : JazzLookAndFeel::Colors::panelBackground.darker(0.3f));
        g.fillRect(row);

        // Draw bar grid lines
        int numBars = static_cast<int>(area.getWidth() / pixelsPerBar) + 2;
        for (int bar = 0; bar < numBars; ++bar)
        {
            float barX = area.getX() + bar * pixelsPerBar;
            g.setColour(JazzLookAndFeel::Colors::borderOutline.withAlpha(0.2f));
            g.drawVerticalLine(static_cast<int>(barX), static_cast<float>(row.getY()), static_cast<float>(row.getBottom()));
        }

        // Draw clips in this track
        const auto& t = tracks[i];
        for (size_t cIdx = 0; cIdx < t.clips.size(); ++cIdx)
        {
            const auto& clip = t.clips[cIdx];
            float startX = tickToX(clip.startTick);
            float endX = tickToX(clip.startTick + clip.lengthTicks);
            float clipW = std::max(12.0f, endX - startX);

            juce::Rectangle<float> clipRect(startX + 1.0f, static_cast<float>(row.getY() + 4),
                                            clipW - 2.0f, static_cast<float>(kTrackHeight - 8));

            bool isClipSelected = (selectedTrackIdx == static_cast<int>(i) && selectedClipIdx == static_cast<int>(cIdx));

            // Clip background
            juce::ColourGradient grad(clip.color.withAlpha(0.35f), clipRect.getX(), clipRect.getY(),
                                      clip.color.withAlpha(0.15f), clipRect.getX(), clipRect.getBottom(), false);
            g.setGradientFill(grad);
            g.fillRoundedRectangle(clipRect, 5.0f);

            // Border
            g.setColour(isClipSelected ? juce::Colours::white : clip.color.withAlpha(0.8f));
            g.drawRoundedRectangle(clipRect, 5.0f, isClipSelected ? 2.0f : 1.0f);

            // Clip label
            g.setColour(JazzLookAndFeel::Colors::textPrimary);
            g.setFont(11.0f);
            g.drawText(clip.name, clipRect.toNearestInt().reduced(4), juce::Justification::topLeft, true);

            // Mini note previews
            g.setColour(clip.color.brighter(0.4f));
            for (const auto& note : clip.miniNotes)
            {
                float noteX = startX + (static_cast<float>(note.second) / static_cast<float>(clip.lengthTicks)) * clipW;
                float noteY = clipRect.getY() + clipRect.getHeight() * (1.0f - (static_cast<float>(note.first % 24) / 24.0f));
                g.fillRect(noteX, noteY, 4.0f, 2.0f);
            }
        }

        // Horizontal row divider
        g.setColour(JazzLookAndFeel::Colors::borderOutline.withAlpha(0.5f));
        g.drawHorizontalLine(y + kTrackHeight - 1, static_cast<float>(area.getX()), static_cast<float>(area.getRight()));

        y += kTrackHeight;
    }
}

void ArrangerComponent::drawPlayhead(juce::Graphics& g, juce::Rectangle<int> area)
{
    float playheadX = tickToX(currentTick);
    if (playheadX < area.getX() || playheadX > area.getRight())
        return;

    // Glowing vertical line
    g.setColour(JazzLookAndFeel::Colors::neonCyan.withAlpha(0.3f));
    g.drawVerticalLine(static_cast<int>(playheadX) - 1, static_cast<float>(0), static_cast<float>(getHeight()));
    g.drawVerticalLine(static_cast<int>(playheadX) + 1, static_cast<float>(0), static_cast<float>(getHeight()));

    g.setColour(JazzLookAndFeel::Colors::neonCyan);
    g.drawVerticalLine(static_cast<int>(playheadX), static_cast<float>(0), static_cast<float>(getHeight()));

    // Top pointer triangle
    juce::Path p;
    p.addTriangle(playheadX - 6.0f, 0.0f, playheadX + 6.0f, 0.0f, playheadX, 10.0f);
    g.fillPath(p);
}

void ArrangerComponent::mouseDown(const juce::MouseEvent& e)
{
    auto pos = e.getPosition();

    // Check click on Ruler
    if (pos.y < kRulerHeight && pos.x >= kHeaderWidth)
    {
        isDraggingPlayhead = true;
        int64_t tick = xToTick(static_cast<float>(pos.x));
        engine.setPlayheadClock(tick);
        repaint();
        return;
    }

    // Check click on Track Headers
    if (pos.x < kHeaderWidth && pos.y >= kRulerHeight)
    {
        int trackIdx = (pos.y - kRulerHeight) / kTrackHeight;
        if (trackIdx >= 0 && trackIdx < static_cast<int>(tracks.size()))
        {
            selectedTrackIdx = trackIdx;

            // Check M, S, R buttons
            int rightOffset = kHeaderWidth - pos.x;
            if (rightOffset <= 76)
            {
                if (rightOffset > 52) tracks[trackIdx].isMuted = !tracks[trackIdx].isMuted;
                else if (rightOffset > 28) tracks[trackIdx].isSolo = !tracks[trackIdx].isSolo;
                else tracks[trackIdx].isArmed = !tracks[trackIdx].isArmed;
            }

            repaint();
        }
        return;
    }

    // Check click on Clips
    if (pos.x >= kHeaderWidth && pos.y >= kRulerHeight)
    {
        int trackIdx = (pos.y - kRulerHeight) / kTrackHeight;
        if (trackIdx >= 0 && trackIdx < static_cast<int>(tracks.size()))
        {
            selectedTrackIdx = trackIdx;
            selectedClipIdx = -1;

            int64_t clickTick = xToTick(static_cast<float>(pos.x));
            auto& t = tracks[trackIdx];
            for (size_t c = 0; c < t.clips.size(); ++c)
            {
                if (clickTick >= t.clips[c].startTick &&
                    clickTick <= t.clips[c].startTick + t.clips[c].lengthTicks)
                {
                    selectedClipIdx = static_cast<int>(c);
                    isDraggingClip = true;
                    dragClipOffsetTick = clickTick - t.clips[c].startTick;
                    break;
                }
            }
            repaint();
        }
    }
}

void ArrangerComponent::mouseDrag(const juce::MouseEvent& e)
{
    auto pos = e.getPosition();
    if (isDraggingPlayhead)
    {
        int64_t tick = xToTick(static_cast<float>(pos.x));
        engine.setPlayheadClock(tick);
        repaint();
    }
    else if (isDraggingClip && selectedTrackIdx >= 0 && selectedClipIdx >= 0)
    {
        int64_t targetTick = xToTick(static_cast<float>(pos.x)) - dragClipOffsetTick;
        // Snap to 16th note (120 ticks)
        int snapTicks = kTicksPerQuarter / 4;
        targetTick = (targetTick / snapTicks) * snapTicks;
        if (targetTick < 0) targetTick = 0;

        tracks[selectedTrackIdx].clips[selectedClipIdx].startTick = targetTick;
        repaint();
    }
}

void ArrangerComponent::mouseUp(const juce::MouseEvent& /*e*/)
{
    isDraggingPlayhead = false;
    isDraggingClip = false;
}

void ArrangerComponent::mouseDoubleClick(const juce::MouseEvent& e)
{
    if (selectedTrackIdx >= 0 && selectedClipIdx >= 0 && onClipDoubleClicked)
    {
        onClipDoubleClicked(selectedTrackIdx, selectedClipIdx);
    }
}

void ArrangerComponent::playheadMoved(int /*bar*/, int /*beat*/, int /*tick*/, double /*seconds*/)
{
    currentTick = engine.getPlayheadClock();
    repaint();
}

void ArrangerComponent::playbackStateChanged(bool /*isPlaying*/)
{
    repaint();
}

void ArrangerComponent::resized()
{
}
