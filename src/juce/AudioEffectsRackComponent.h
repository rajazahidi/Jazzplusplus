//*****************************************************************************
// The JAZZ++ Midi Sequencer — JUCE 8 Edition
//
// Copyright (C) 2026 Raja Zahidi
// Real-Time Studio DSP Effects Rack
//*****************************************************************************

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "AudioMidiEngine.h"
#include "JazzLookAndFeel.h"

class AudioEffectsRackComponent : public juce::Component
{
public:
    AudioEffectsRackComponent(AudioMidiEngine& engine);
    ~AudioEffectsRackComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    AudioMidiEngine& engine;

    // 1. Equalizer Controls
    juce::ToggleButton btnEqEnable { "EQ ACTIVE" };
    juce::Slider slEqBass;
    juce::Slider slEqMid;
    juce::Slider slEqTreble;
    juce::Label lblEqBass { {}, "BASS" };
    juce::Label lblEqMid { {}, "MID" };
    juce::Label lblEqTreble { {}, "TREBLE" };

    // 2. Reverb Controls
    juce::ToggleButton btnRevEnable { "REVERB ACTIVE" };
    juce::Slider slRevRoom;
    juce::Slider slRevDamp;
    juce::Slider slRevWet;
    juce::Label lblRevRoom { {}, "ROOM" };
    juce::Label lblRevDamp { {}, "DAMP" };
    juce::Label lblRevWet { {}, "WET" };

    // 3. Delay Controls
    juce::ToggleButton btnDelEnable { "DELAY ACTIVE" };
    juce::Slider slDelTime;
    juce::Slider slDelFeedback;
    juce::Slider slDelWet;
    juce::Label lblDelTime { {}, "TIME" };
    juce::Label lblDelFeedback { {}, "F-BACK" };
    juce::Label lblDelWet { {}, "WET" };

    // 4. Chorus Controls
    juce::ToggleButton btnChoEnable { "CHORUS ACTIVE" };
    juce::Slider slChoRate;
    juce::Slider slChoDepth;
    juce::Slider slChoMix;
    juce::Label lblChoRate { {}, "RATE" };
    juce::Label lblChoDepth { {}, "DEPTH" };
    juce::Label lblChoMix { {}, "MIX" };

    // 5. Limiter / Overdrive Controls
    juce::ToggleButton btnLimEnable { "LIMITER ACTIVE" };
    juce::Slider slLimDrive;
    juce::Slider slLimThresh;
    juce::Slider slLimCeil;
    juce::Label lblLimDrive { {}, "DRIVE" };
    juce::Label lblLimThresh { {}, "THRESH" };
    juce::Label lblLimCeil { {}, "CEILING" };

    void setupRotary(juce::Slider& sl, juce::Label& lbl, double min, double max, double def, const juce::String& suffix);
    void updateEngineFx();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioEffectsRackComponent)
};
