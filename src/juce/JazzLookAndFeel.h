//*****************************************************************************
// The JAZZ++ Midi Sequencer — JUCE 8 Edition
//
// Copyright (C) 2026 Raja Zahidi
// Modern Dark Neon Studio LookAndFeel
//*****************************************************************************

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class JazzLookAndFeel : public juce::LookAndFeel_V4
{
public:
    // Palette definition: Modern Deep Studio Dark with Neon Accents
    struct Colors
    {
        static inline const juce::Colour backgroundDark    { 0xff0d1117 }; // Deepest background
        static inline const juce::Colour panelBackground   { 0xff161b22 }; // Track/panel cards
        static inline const juce::Colour panelHeader       { 0xff21262d }; // Headers and dividers
        static inline const juce::Colour controlFill       { 0xff30363d }; // Button/slider trough
        static inline const juce::Colour borderOutline     { 0xff30363d }; // Subtle borders
        
        static inline const juce::Colour neonCyan          { 0xff00d2ff }; // Primary highlight & audio
        static inline const juce::Colour neonPurple        { 0xffa855f7 }; // Secondary highlight & MIDI
        static inline const juce::Colour neonEmerald       { 0xff10b981 }; // Playback, active, OK
        static inline const juce::Colour neonAmber         { 0xfff59e0b }; // Warning, solo, loop
        static inline const juce::Colour neonRed           { 0xffef4444 }; // Record, mute, clip
        
        static inline const juce::Colour textPrimary       { 0xfff0f6fc }; // High contrast text
        static inline const juce::Colour textSecondary     { 0xff8b949e }; // Dimmed labels
        static inline const juce::Colour textDimmed        { 0xff484f58 }; // Subtle grid lines
    };

    JazzLookAndFeel()
    {
        setColour(juce::ResizableWindow::backgroundColourId, Colors::backgroundDark);
        setColour(juce::ScrollBar::backgroundColourId, Colors::panelBackground);
        setColour(juce::ScrollBar::thumbColourId, Colors::controlFill);
        
        // Buttons
        setColour(juce::TextButton::buttonColourId, Colors::controlFill);
        setColour(juce::TextButton::buttonOnColourId, Colors::neonCyan.withAlpha(0.35f));
        setColour(juce::TextButton::textColourOffId, Colors::textPrimary);
        setColour(juce::TextButton::textColourOnId, Colors::neonCyan);

        // Sliders
        setColour(juce::Slider::backgroundColourId, Colors::panelHeader);
        setColour(juce::Slider::thumbColourId, Colors::neonCyan);
        setColour(juce::Slider::trackColourId, Colors::neonCyan.withAlpha(0.6f));
        setColour(juce::Slider::rotarySliderFillColourId, Colors::neonCyan);
        setColour(juce::Slider::rotarySliderOutlineColourId, Colors::controlFill);

        // Labels
        setColour(juce::Label::textColourId, Colors::textPrimary);

        // Tabbed Component
        setColour(juce::TabbedButtonBar::tabOutlineColourId, juce::Colours::transparentBlack);
        setColour(juce::TabbedComponent::outlineColourId, Colors::borderOutline);
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider& slider) override
    {
        auto radius = (float) juce::jmin(width / 2, height / 2) - 4.0f;
        auto centreX = (float) x + (float) width * 0.5f;
        auto centreY = (float) y + (float) height * 0.5f;
        auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

        // Outer background ring
        g.setColour(Colors::controlFill);
        juce::Path backgroundArc;
        backgroundArc.addCentredArc(centreX, centreY, radius, radius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.strokePath(backgroundArc, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Dial body
        auto innerRadius = radius - 6.0f;
        juce::ColourGradient dialGrad(Colors::panelHeader.brighter(0.1f), centreX, centreY - innerRadius,
                                      Colors::panelBackground, centreX, centreY + innerRadius, false);
        g.setGradientFill(dialGrad);
        g.fillEllipse(centreX - innerRadius, centreY - innerRadius, innerRadius * 2.0f, innerRadius * 2.0f);

        g.setColour(Colors::borderOutline);
        g.drawEllipse(centreX - innerRadius, centreY - innerRadius, innerRadius * 2.0f, innerRadius * 2.0f, 1.2f);

        // Active value arc
        if (slider.isEnabled())
        {
            juce::Path valueArc;
            valueArc.addCentredArc(centreX, centreY, radius, radius, 0.0f, rotaryStartAngle, angle, true);
            g.setColour(slider.findColour(juce::Slider::rotarySliderFillColourId));
            g.strokePath(valueArc, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            // Indicator needle / dot
            juce::Path p;
            auto pointerLength = innerRadius * 0.75f;
            auto pointerThickness = 3.0f;
            p.addRoundedRectangle(-pointerThickness * 0.5f, -innerRadius + 3.0f, pointerThickness, pointerLength * 0.5f, 1.5f);
            p.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));
            g.setColour(slider.findColour(juce::Slider::rotarySliderFillColourId));
            g.fillPath(p);
        }
    }

    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                              const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
        auto cornerSize = 6.0f;

        juce::Colour baseColor = backgroundColour;
        if (button.getToggleState())
            baseColor = button.findColour(juce::TextButton::buttonOnColourId);
        else if (shouldDrawButtonAsDown)
            baseColor = baseColor.brighter(0.2f);
        else if (shouldDrawButtonAsHighlighted)
            baseColor = baseColor.brighter(0.08f);

        g.setColour(baseColor);
        g.fillRoundedRectangle(bounds, cornerSize);

        // Outline
        juce::Colour outlineColor = button.getToggleState() ?
            button.findColour(juce::TextButton::textColourOnId).withAlpha(0.7f) :
            Colors::borderOutline;

        g.setColour(outlineColor);
        g.drawRoundedRectangle(bounds, cornerSize, 1.2f);
    }

    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          const juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        if (style == juce::Slider::LinearHorizontal || style == juce::Slider::LinearVertical)
        {
            auto isVertical = style == juce::Slider::LinearVertical;
            auto trackWidth = 4.0f;

            juce::Point<float> startPoint(isVertical ? (float) x + (float) width * 0.5f : (float) x,
                                          isVertical ? (float) (y + height) : (float) y + (float) height * 0.5f);
            juce::Point<float> endPoint(isVertical ? (float) x + (float) width * 0.5f : (float) (x + width),
                                        isVertical ? (float) y : (float) y + (float) height * 0.5f);

            // Track background
            g.setColour(Colors::controlFill);
            g.drawLine(juce::Line<float>(startPoint, endPoint), trackWidth);

            // Active track
            juce::Point<float> currentPoint(isVertical ? startPoint.x : sliderPos,
                                            isVertical ? sliderPos : startPoint.y);
            g.setColour(slider.findColour(juce::Slider::trackColourId));
            g.drawLine(juce::Line<float>(startPoint, currentPoint), trackWidth);

            // Thumb
            auto thumbRadius = 7.0f;
            g.setColour(slider.findColour(juce::Slider::thumbColourId));
            g.fillEllipse(currentPoint.x - thumbRadius, currentPoint.y - thumbRadius, thumbRadius * 2.0f, thumbRadius * 2.0f);
            g.setColour(Colors::textPrimary);
            g.drawEllipse(currentPoint.x - thumbRadius, currentPoint.y - thumbRadius, thumbRadius * 2.0f, thumbRadius * 2.0f, 1.5f);
        }
        else
        {
            juce::LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
        }
    }
};
