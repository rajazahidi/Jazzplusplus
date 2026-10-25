//=============================================================================
// IntelligentVocalHarmonizerEngine.cpp
//
// Master Studio Pro Suite v31.0 - Apex Transcendence Sovereign Pinnacle Release
// Real-Time 4-Voice Intelligent Vocal Harmonizer & Key-Scale Follower Engine.
//=============================================================================

#include "IntelligentVocalHarmonizerEngine.h"

namespace Jazz {

static const float kPi = 3.14159265358979323846f;

static const char* kNoteNames[12] = {
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};

JZIntelligentVocalHarmonizerEngine::JZIntelligentVocalHarmonizerEngine(float sampleRate)
    : mSampleRate(sampleRate > 1000.0f ? sampleRate : 48000.0f)
    , mRootNote(0) // C
    , mScaleMode(HarmonizerScaleMode::MAJOR)
    , mDryWet(50.0f)
    , mHumanizeAmount(25.0f)
    , mDetectedPitchHz(220.0f)
    , mDetectedMidiNote(57) // A3
    , mPitchConfidence(0.85f)
    , mWriteIdx(0)
    , mPitchWriteIdx(0)
{
    mPitchDelayBuf.assign(DELAY_BUF_SIZE, 0.0f);
    mPitchDetectorBuf.assign(PITCH_BUF_SIZE, 0.0f);

    // Default 4-Voice Harmony Arrangement:
    // Voice 0: Diatonic 3rd above (+2 steps), pan -40% (left), 0 dB
    mVoices[0].diatonicIntervalSteps = 2;
    mVoices[0].pan = -0.40f;
    mVoices[0].gainDb = 0.0f;
    mVoices[0].enabled = true;

    // Voice 1: Diatonic 5th above (+4 steps), pan +40% (right), -1 dB
    mVoices[1].diatonicIntervalSteps = 4;
    mVoices[1].pan = 0.40f;
    mVoices[1].gainDb = -1.0f;
    mVoices[1].enabled = true;

    // Voice 2: Diatonic 6th below (-2 steps), pan -65% (far left), -3 dB
    mVoices[2].diatonicIntervalSteps = -2;
    mVoices[2].pan = -0.65f;
    mVoices[2].gainDb = -3.0f;
    mVoices[2].enabled = true;

    // Voice 3: Octave Bass (-7 steps), center (0.0 pan), -4 dB
    mVoices[3].diatonicIntervalSteps = -7;
    mVoices[3].pan = 0.0f;
    mVoices[3].gainDb = -4.0f;
    mVoices[3].enabled = true;

    for (size_t v = 0; v < NUM_VOICES; ++v) {
        mHumanizeLfoPhase[v] = static_cast<float>(v) * 1.57f;
    }
}

void JZIntelligentVocalHarmonizerEngine::setSampleRate(float sampleRate) {
    if (sampleRate > 1000.0f) {
        mSampleRate = sampleRate;
        reset();
    }
}

void JZIntelligentVocalHarmonizerEngine::setKeyAndScale(int rootNote, HarmonizerScaleMode mode) {
    mRootNote = (rootNote % 12 + 12) % 12;
    mScaleMode = mode;
}

std::string JZIntelligentVocalHarmonizerEngine::getScaleName() const {
    std::string rootStr = kNoteNames[mRootNote];
    switch (mScaleMode) {
        case HarmonizerScaleMode::MAJOR:          return rootStr + " Major";
        case HarmonizerScaleMode::NATURAL_MINOR:  return rootStr + " Natural Minor";
        case HarmonizerScaleMode::DORIAN:         return rootStr + " Dorian";
        case HarmonizerScaleMode::MIXOLYDIAN:     return rootStr + " Mixolydian";
        case HarmonizerScaleMode::PHRYGIAN:       return rootStr + " Phrygian";
        case HarmonizerScaleMode::LYDIAN:         return rootStr + " Lydian";
        case HarmonizerScaleMode::HARMONIC_MINOR: return rootStr + " Harmonic Minor";
        default: return rootStr + " Scale";
    }
}

void JZIntelligentVocalHarmonizerEngine::setVoiceConfig(size_t voiceIdx, int intervalSteps, float pan, float gainDb, bool enabled) {
    if (voiceIdx < NUM_VOICES) {
        mVoices[voiceIdx].diatonicIntervalSteps = intervalSteps;
        mVoices[voiceIdx].pan = std::max(-1.0f, std::min(1.0f, pan));
        mVoices[voiceIdx].gainDb = std::max(-36.0f, std::min(12.0f, gainDb));
        mVoices[voiceIdx].enabled = enabled;
    }
}

const HarmonyVoiceConfig& JZIntelligentVocalHarmonizerEngine::getVoiceConfig(size_t voiceIdx) const {
    static HarmonyVoiceConfig dummy;
    if (voiceIdx < NUM_VOICES) return mVoices[voiceIdx];
    return dummy;
}

void JZIntelligentVocalHarmonizerEngine::setDryWet(float wetPercent) {
    mDryWet = std::max(0.0f, std::min(100.0f, wetPercent));
}

void JZIntelligentVocalHarmonizerEngine::setHumanizeAmount(float percent) {
    mHumanizeAmount = std::max(0.0f, std::min(100.0f, percent));
}

void JZIntelligentVocalHarmonizerEngine::reset() {
    std::fill(mPitchDelayBuf.begin(), mPitchDelayBuf.end(), 0.0f);
    std::fill(mPitchDetectorBuf.begin(), mPitchDetectorBuf.end(), 0.0f);
    mWriteIdx = 0;
    mPitchWriteIdx = 0;
    for (size_t v = 0; v < NUM_VOICES; ++v) {
        mVoiceGrains[v].reset();
        mHumanizeLfoPhase[v] = static_cast<float>(v) * 1.57f;
    }
}

bool JZIntelligentVocalHarmonizerEngine::isScaleTone(int pitchClass) const {
    // Relative pitch class offset from root
    int rel = (pitchClass - mRootNote + 12) % 12;

    switch (mScaleMode) {
        case HarmonizerScaleMode::MAJOR:
            return (rel == 0 || rel == 2 || rel == 4 || rel == 5 || rel == 7 || rel == 9 || rel == 11);
        case HarmonizerScaleMode::NATURAL_MINOR:
            return (rel == 0 || rel == 2 || rel == 3 || rel == 5 || rel == 7 || rel == 8 || rel == 10);
        case HarmonizerScaleMode::DORIAN:
            return (rel == 0 || rel == 2 || rel == 3 || rel == 5 || rel == 7 || rel == 9 || rel == 10);
        case HarmonizerScaleMode::MIXOLYDIAN:
            return (rel == 0 || rel == 2 || rel == 4 || rel == 5 || rel == 7 || rel == 9 || rel == 10);
        case HarmonizerScaleMode::PHRYGIAN:
            return (rel == 0 || rel == 1 || rel == 3 || rel == 5 || rel == 7 || rel == 8 || rel == 10);
        case HarmonizerScaleMode::LYDIAN:
            return (rel == 0 || rel == 2 || rel == 4 || rel == 6 || rel == 7 || rel == 9 || rel == 11);
        case HarmonizerScaleMode::HARMONIC_MINOR:
            return (rel == 0 || rel == 2 || rel == 3 || rel == 5 || rel == 7 || rel == 8 || rel == 11);
        default:
            return true;
    }
}

int JZIntelligentVocalHarmonizerEngine::calculateDiatonicSemitones(int inputMidiNote, int diatonicIntervalSteps) {
    if (diatonicIntervalSteps == 0) return 0;

    // Scale step intervals from root
    static const int majorScale[7] = {0, 2, 4, 5, 7, 9, 11};
    static const int minorScale[7] = {0, 2, 3, 5, 7, 8, 10};

    const int* scale = (mScaleMode == HarmonizerScaleMode::MAJOR || mScaleMode == HarmonizerScaleMode::LYDIAN)
                       ? majorScale : minorScale;

    int inputPitchClass = inputMidiNote % 12;
    int relPitch = (inputPitchClass - mRootNote + 12) % 12;

    // Find closest scale degree
    int currentDegree = 0;
    int minDiff = 12;
    for (int d = 0; d < 7; ++d) {
        int diff = std::abs(scale[d] - relPitch);
        if (diff < minDiff) {
            minDiff = diff;
            currentDegree = d;
        }
    }

    // Step diatonically
    int targetDegree = currentDegree + diatonicIntervalSteps;
    int octaveShift = 0;
    while (targetDegree < 0) {
        targetDegree += 7;
        octaveShift -= 1;
    }
    while (targetDegree >= 7) {
        targetDegree -= 7;
        octaveShift += 1;
    }

    int targetRelPitch = scale[targetDegree];
    int targetPitchClass = (mRootNote + targetRelPitch) % 12;

    int targetMidiNote = (inputMidiNote / 12 + octaveShift) * 12 + targetPitchClass;
    return targetMidiNote - inputMidiNote;
}

void JZIntelligentVocalHarmonizerEngine::trackPitch() {
    // Autocorrelation pitch detection on mPitchDetectorBuf
    // Target range: 70 Hz to 1000 Hz
    size_t minLag = static_cast<size_t>(mSampleRate / 1000.0f);
    size_t maxLag = static_cast<size_t>(mSampleRate / 70.0f);
    maxLag = std::min(maxLag, PITCH_BUF_SIZE / 2);

    float maxCorr = -1.0f;
    size_t bestLag = minLag;

    float energyZero = 0.0f;
    for (size_t i = 0; i < maxLag; ++i) {
        energyZero += mPitchDetectorBuf[i] * mPitchDetectorBuf[i];
    }
    if (energyZero < 1e-4f) return; // silence

    for (size_t lag = minLag; lag < maxLag; ++lag) {
        float corr = 0.0f;
        for (size_t i = 0; i < maxLag; ++i) {
            corr += mPitchDetectorBuf[i] * mPitchDetectorBuf[i + lag];
        }
        if (corr > maxCorr) {
            maxCorr = corr;
            bestLag = lag;
        }
    }

    float normCorr = maxCorr / energyZero;
    if (normCorr > 0.60f && bestLag > 0) {
        mDetectedPitchHz = mSampleRate / static_cast<float>(bestLag);
        mPitchConfidence = normCorr;
        // MIDI note = 69 + 12 * log2(f / 440)
        float midiFloat = 69.0f + 12.0f * (std::log(mDetectedPitchHz / 440.0f) / std::log(2.0f));
        mDetectedMidiNote = static_cast<int>(std::round(midiFloat));
    }
}

void JZIntelligentVocalHarmonizerEngine::process(const float* inL, const float* inR,
                                               float* outL, float* outR, size_t numSamples) {
    if (!inL || !inR || !outL || !outR || numSamples == 0) return;

    float wetMix = mDryWet * 0.01f;
    float dryMix = 1.0f - wetMix;

    for (size_t i = 0; i < numSamples; ++i) {
        float monoIn = 0.5f * (inL[i] + inR[i]);

        // Write to granular pitch delay line
        mPitchDelayBuf[mWriteIdx] = monoIn;
        mWriteIdx = (mWriteIdx + 1) % DELAY_BUF_SIZE;

        // Write to pitch detector buffer
        mPitchDetectorBuf[mPitchWriteIdx] = monoIn;
        mPitchWriteIdx = (mPitchWriteIdx + 1) % PITCH_BUF_SIZE;
        if (mPitchWriteIdx == 0) {
            trackPitch();
        }

        float harmTotalL = 0.0f;
        float harmTotalR = 0.0f;

        // Process up to 4 harmony voices
        for (size_t v = 0; v < NUM_VOICES; ++v) {
            if (!mVoices[v].enabled) continue;

            // Diatonic transposition calculation
            int semitoneShift = calculateDiatonicSemitones(mDetectedMidiNote, mVoices[v].diatonicIntervalSteps);

            // Micro-pitch humanize LFO (+/- 10 cents)
            float lfoSpeed = 0.0005f * (static_cast<float>(v) + 1.0f);
            mHumanizeLfoPhase[v] += lfoSpeed;
            if (mHumanizeLfoPhase[v] >= 2.0f * kPi) mHumanizeLfoPhase[v] -= 2.0f * kPi;
            float detuneCents = std::sin(mHumanizeLfoPhase[v]) * (mHumanizeAmount * 0.12f);

            float totalSemitones = static_cast<float>(semitoneShift) + (detuneCents * 0.01f);
            float pitchRatio = std::pow(2.0f, totalSemitones / 12.0f);

            // Granular pitch shifter phase increments
            PitchGrainState& grain = mVoiceGrains[v];
            float grainRate = (1.0f - pitchRatio) / grain.grainSizeSamples;

            grain.phase1 += grainRate;
            while (grain.phase1 >= 1.0f) grain.phase1 -= 1.0f;
            while (grain.phase1 < 0.0f) grain.phase1 += 1.0f;

            grain.phase2 += grainRate;
            while (grain.phase2 >= 1.0f) grain.phase2 -= 1.0f;
            while (grain.phase2 < 0.0f) grain.phase2 += 1.0f;

            // Triangular crossfade windows
            float win1 = 0.5f * (1.0f - std::cos(grain.phase1 * 2.0f * kPi));
            float win2 = 0.5f * (1.0f - std::cos(grain.phase2 * 2.0f * kPi));

            size_t delay1 = static_cast<size_t>(grain.phase1 * grain.grainSizeSamples);
            size_t delay2 = static_cast<size_t>(grain.phase2 * grain.grainSizeSamples);

            size_t rIdx1 = (mWriteIdx + DELAY_BUF_SIZE - delay1 - 10) % DELAY_BUF_SIZE;
            size_t rIdx2 = (mWriteIdx + DELAY_BUF_SIZE - delay2 - 10) % DELAY_BUF_SIZE;

            float grainSample = (mPitchDelayBuf[rIdx1] * win1) + (mPitchDelayBuf[rIdx2] * win2);

            // Constant power stereo panning
            float panNorm = (mVoices[v].pan + 1.0f) * 0.5f;
            float panAngle = panNorm * (0.5f * kPi);
            float panGainL = std::cos(panAngle) * 1.4142f;
            float panGainR = std::sin(panAngle) * 1.4142f;

            float vGain = std::pow(10.0f, mVoices[v].gainDb / 20.0f);

            harmTotalL += grainSample * vGain * panGainL;
            harmTotalR += grainSample * vGain * panGainR;
        }

        outL[i] = inL[i] * dryMix + harmTotalL * wetMix;
        outR[i] = inR[i] * dryMix + harmTotalR * wetMix;
    }
}

} // namespace Jazz
