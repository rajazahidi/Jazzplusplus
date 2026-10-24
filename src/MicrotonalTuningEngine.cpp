#include "MicrotonalTuningEngine.h"
#include <sstream>
#include <algorithm>
#include <cctype>

namespace Jazz {

JZMicrotonalTuningEngine::JZMicrotonalTuningEngine()
    : m_preset(MicrotonalPreset::EQUAL_TEMPERAMENT_12)
    , m_scaleDescription("12-Tone Equal Temperament")
    , m_octaveRatio(2.0)
    , m_refFrequency(440.0)
    , m_refMidiNote(69)
    , m_rootKey(60)
    , m_masterOffsetCents(0.0)
{
    for (int i = 0; i < 128; ++i) {
        m_directOverride[i] = false;
        m_noteFrequencies[i] = standard12TETFrequency(i, 440.0);
    }
    setPreset(MicrotonalPreset::EQUAL_TEMPERAMENT_12);
}

double JZMicrotonalTuningEngine::ratioToCents(double ratio) {
    if (ratio <= 0.000001) return 0.0;
    return 1200.0 * (std::log(ratio) / std::log(2.0));
}

double JZMicrotonalTuningEngine::centsToRatio(double cents) {
    return std::pow(2.0, cents / 1200.0);
}

double JZMicrotonalTuningEngine::standard12TETFrequency(int midiNote, double refA4) {
    return refA4 * std::pow(2.0, (midiNote - 69) / 12.0);
}

void JZMicrotonalTuningEngine::setReferencePitch(double freqHz, int refMidiNote) {
    if (freqHz > 10.0 && freqHz < 20000.0) {
        m_refFrequency = freqHz;
    }
    if (refMidiNote >= 0 && refMidiNote < 128) {
        m_refMidiNote = refMidiNote;
    }
    rebuildFrequencyTable();
}

void JZMicrotonalTuningEngine::setRootKey(int rootMidiNote) {
    if (rootMidiNote >= 0 && rootMidiNote < 128) {
        m_rootKey = rootMidiNote;
        rebuildFrequencyTable();
    }
}

void JZMicrotonalTuningEngine::setMasterTuningOffsetCents(double cents) {
    m_masterOffsetCents = cents;
    rebuildFrequencyTable();
}

void JZMicrotonalTuningEngine::setPreset(MicrotonalPreset preset) {
    m_preset = preset;
    for (int i = 0; i < 128; ++i) {
        m_directOverride[i] = false;
    }
    applyBuiltinPreset(preset);
    rebuildFrequencyTable();
}

void JZMicrotonalTuningEngine::resetToPreset() {
    setPreset(m_preset);
}

void JZMicrotonalTuningEngine::setNoteDirectFrequency(int midiNote, double freqHz) {
    if (midiNote >= 0 && midiNote < 128 && freqHz > 0.0) {
        m_noteFrequencies[midiNote] = freqHz;
        m_directOverride[midiNote] = true;
    }
}

double JZMicrotonalTuningEngine::getNoteFrequency(int midiNote, double pitchBendSemitones) const {
    int note = std::max(0, std::min(127, midiNote));
    double freq = m_noteFrequencies[note];
    if (std::abs(pitchBendSemitones) > 1e-6) {
        freq *= std::pow(2.0, pitchBendSemitones / 12.0);
    }
    return freq;
}

double JZMicrotonalTuningEngine::getCentsDeviationFrom12TET(int midiNote) const {
    int note = std::max(0, std::min(127, midiNote));
    double fActual = m_noteFrequencies[note];
    double fStandard = standard12TETFrequency(note, m_refFrequency);
    if (fActual <= 1e-6 || fStandard <= 1e-6) return 0.0;
    return 1200.0 * (std::log(fActual / fStandard) / std::log(2.0));
}

double JZMicrotonalTuningEngine::quantizeFrequency(double inputFreqHz) const {
    if (inputFreqHz <= 1e-3) return 0.0;
    double bestDiff = 1e9;
    double bestFreq = m_noteFrequencies[60];
    for (int i = 0; i < 128; ++i) {
        double diff = std::abs(m_noteFrequencies[i] - inputFreqHz);
        if (diff < bestDiff) {
            bestDiff = diff;
            bestFreq = m_noteFrequencies[i];
        }
    }
    return bestFreq;
}

void JZMicrotonalTuningEngine::applyBuiltinPreset(MicrotonalPreset preset) {
    m_scalePitches.clear();
    m_octaveRatio = 2.0;

    switch (preset) {
        case MicrotonalPreset::EQUAL_TEMPERAMENT_12:
            m_scaleDescription = "12-Tone Equal Temperament";
            for (int i = 1; i <= 12; ++i) {
                ScaleNotePitch p;
                p.isRatio = false;
                p.cents = i * 100.0;
                p.ratioValue = centsToRatio(p.cents);
                p.numerator = 0;
                p.denominator = 0;
                m_scalePitches.push_back(p);
            }
            break;

        case MicrotonalPreset::JUST_INTONATION: {
            m_scaleDescription = "Ptolemaic Just Intonation (5-limit)";
            // 16/15, 9/8, 6/5, 5/4, 4/3, 45/32, 3/2, 8/5, 5/3, 9/5, 15/8, 2/1
            static const int nums[12] = {16, 9, 6, 5, 4, 45, 3, 8, 5, 9, 15, 2};
            static const int dens[12] = {15, 8, 5, 4, 3, 32, 2, 5, 3, 5,  8, 1};
            for (int i = 0; i < 12; ++i) {
                ScaleNotePitch p;
                p.isRatio = true;
                p.numerator = nums[i];
                p.denominator = dens[i];
                p.ratioValue = static_cast<double>(nums[i]) / static_cast<double>(dens[i]);
                p.cents = ratioToCents(p.ratioValue);
                m_scalePitches.push_back(p);
            }
            break;
        }

        case MicrotonalPreset::PYTHAGOREAN: {
            m_scaleDescription = "Pythagorean Tuning (3-limit)";
            static const int nums[12] = {256, 9, 32, 81, 4, 729, 3, 128, 27, 16, 243, 2};
            static const int dens[12] = {243, 8, 27, 64, 3, 512, 2,  81, 16,  9, 128, 1};
            for (int i = 0; i < 12; ++i) {
                ScaleNotePitch p;
                p.isRatio = true;
                p.numerator = nums[i];
                p.denominator = dens[i];
                p.ratioValue = static_cast<double>(nums[i]) / static_cast<double>(dens[i]);
                p.cents = ratioToCents(p.ratioValue);
                m_scalePitches.push_back(p);
            }
            break;
        }

        case MicrotonalPreset::QUARTER_COMMA_MEANTONE: {
            m_scaleDescription = "Quarter-Comma Meantone (12 notes)";
            // Major third is pure 5/4 (386.31 cents), fifth is 5^(1/4) ~ 696.58 cents
            double fifth = std::pow(5.0, 0.25);
            double fifthCents = ratioToCents(fifth);
            // Circle of fifths from Eb to G#
            // C=0, C# = 4*fifth - 2400? We can define canonical meantone cents:
            static const double mtCents[12] = {
                76.0, 193.2, 310.3, 386.3, 503.4, 579.5, 696.6, 772.6, 889.7, 1006.9, 1082.9, 1200.0
            };
            for (int i = 0; i < 12; ++i) {
                ScaleNotePitch p;
                p.isRatio = false;
                p.cents = mtCents[i];
                p.ratioValue = centsToRatio(p.cents);
                p.numerator = 0;
                p.denominator = 0;
                m_scalePitches.push_back(p);
            }
            break;
        }

        case MicrotonalPreset::EDO_19: {
            m_scaleDescription = "19 Equal Divisions of the Octave (19-EDO)";
            for (int i = 1; i <= 19; ++i) {
                ScaleNotePitch p;
                p.isRatio = false;
                p.cents = i * (1200.0 / 19.0);
                p.ratioValue = centsToRatio(p.cents);
                p.numerator = 0;
                p.denominator = 0;
                m_scalePitches.push_back(p);
            }
            break;
        }

        case MicrotonalPreset::EDO_31: {
            m_scaleDescription = "31 Equal Divisions of the Octave (31-EDO)";
            for (int i = 1; i <= 31; ++i) {
                ScaleNotePitch p;
                p.isRatio = false;
                p.cents = i * (1200.0 / 31.0);
                p.ratioValue = centsToRatio(p.cents);
                p.numerator = 0;
                p.denominator = 0;
                m_scalePitches.push_back(p);
            }
            break;
        }

        case MicrotonalPreset::MAQAM_RAST: {
            m_scaleDescription = "Arabic Maqam Rast (7-tone Quarter Tone scale)";
            // 1: C (0), 2: D (+204), 3: E half-flat (+354), 4: F (+498), 5: G (+702), 6: A (+906), 7: B half-flat (+1050), 8: C (+1200)
            static const double rastCents[7] = {204.0, 354.0, 498.0, 702.0, 906.0, 1050.0, 1200.0};
            for (int i = 0; i < 7; ++i) {
                ScaleNotePitch p;
                p.isRatio = false;
                p.cents = rastCents[i];
                p.ratioValue = centsToRatio(p.cents);
                p.numerator = 0;
                p.denominator = 0;
                m_scalePitches.push_back(p);
            }
            break;
        }

        case MicrotonalPreset::MAQAM_BAYATI: {
            m_scaleDescription = "Arabic Maqam Bayati (7-tone)";
            // D-based usually: D (0), E half-flat (+150), F (+300), G (+500), A (+700), Bb (+800), C (+1000), D (+1200)
            static const double bayatiCents[7] = {150.0, 300.0, 500.0, 700.0, 800.0, 1000.0, 1200.0};
            for (int i = 0; i < 7; ++i) {
                ScaleNotePitch p;
                p.isRatio = false;
                p.cents = bayatiCents[i];
                p.ratioValue = centsToRatio(p.cents);
                p.numerator = 0;
                p.denominator = 0;
                m_scalePitches.push_back(p);
            }
            break;
        }

        case MicrotonalPreset::GAMELAN_SLENDRO: {
            m_scaleDescription = "Javanese Gamelan Slendro (5-tone equidistant)";
            for (int i = 1; i <= 5; ++i) {
                ScaleNotePitch p;
                p.isRatio = false;
                p.cents = i * 240.0;
                p.ratioValue = centsToRatio(p.cents);
                p.numerator = 0;
                p.denominator = 0;
                m_scalePitches.push_back(p);
            }
            break;
        }

        case MicrotonalPreset::GAMELAN_PELOG: {
            m_scaleDescription = "Javanese Gamelan Pelog (7-tone unequal)";
            static const double pelogCents[7] = {120.0, 275.0, 545.0, 680.0, 790.0, 960.0, 1200.0};
            for (int i = 0; i < 7; ++i) {
                ScaleNotePitch p;
                p.isRatio = false;
                p.cents = pelogCents[i];
                p.ratioValue = centsToRatio(p.cents);
                p.numerator = 0;
                p.denominator = 0;
                m_scalePitches.push_back(p);
            }
            break;
        }

        case MicrotonalPreset::CUSTOM_SCALA:
        default:
            break;
    }
}

bool JZMicrotonalTuningEngine::loadScalaString(const std::string& sclContent) {
    std::istringstream stream(sclContent);
    std::string line;
    std::vector<std::string> cleanLines;

    while (std::getline(stream, line)) {
        // Strip leading whitespace
        size_t start = line.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) continue;
        if (line[start] == '!') continue; // comment
        // Trim right
        size_t end = line.find_last_not_of(" \t\r\n");
        cleanLines.push_back(line.substr(start, end - start + 1));
    }

    if (cleanLines.size() < 2) {
        return false;
    }

    m_scaleDescription = cleanLines[0];
    int expectedCount = 0;
    try {
        expectedCount = std::stoi(cleanLines[1]);
    } catch (...) {
        return false;
    }

    if (expectedCount <= 0 || expectedCount > 512) {
        return false;
    }

    std::vector<ScaleNotePitch> newPitches;
    for (size_t i = 2; i < cleanLines.size() && static_cast<int>(newPitches.size()) < expectedCount; ++i) {
        const std::string& pitchStr = cleanLines[i];
        ScaleNotePitch p;
        p.isRatio = false;
        p.numerator = 0;
        p.denominator = 0;

        size_t slashPos = pitchStr.find('/');
        size_t dotPos = pitchStr.find('.');

        if (dotPos != std::string::npos) {
            // Cents value
            try {
                p.cents = std::stod(pitchStr);
                p.ratioValue = centsToRatio(p.cents);
                p.isRatio = false;
            } catch (...) {
                return false;
            }
        } else if (slashPos != std::string::npos) {
            // Fractional ratio
            try {
                p.numerator = std::stoi(pitchStr.substr(0, slashPos));
                p.denominator = std::stoi(pitchStr.substr(slashPos + 1));
                if (p.denominator <= 0) return false;
                p.ratioValue = static_cast<double>(p.numerator) / static_cast<double>(p.denominator);
                p.cents = ratioToCents(p.ratioValue);
                p.isRatio = true;
            } catch (...) {
                return false;
            }
        } else {
            // Integer ratio, e.g. "2" means 2/1
            try {
                int val = std::stoi(pitchStr);
                p.numerator = val;
                p.denominator = 1;
                p.ratioValue = static_cast<double>(val);
                p.cents = ratioToCents(p.ratioValue);
                p.isRatio = true;
            } catch (...) {
                return false;
            }
        }
        newPitches.push_back(p);
    }

    if (static_cast<int>(newPitches.size()) != expectedCount) {
        return false;
    }

    m_scalePitches = newPitches;
    m_preset = MicrotonalPreset::CUSTOM_SCALA;
    m_octaveRatio = m_scalePitches.back().ratioValue;
    if (m_octaveRatio <= 1.0) {
        m_octaveRatio = 2.0;
    }

    for (int i = 0; i < 128; ++i) {
        m_directOverride[i] = false;
    }
    rebuildFrequencyTable();
    return true;
}

void JZMicrotonalTuningEngine::rebuildFrequencyTable() {
    int N = static_cast<int>(m_scalePitches.size());
    if (N <= 0) return;

    if (m_octaveRatio <= 1.0001) {
        m_octaveRatio = 2.0;
    }

    // Helper: compute ratio of scale degree k relative to root key (degree 0)
    auto getRatioForInterval = [&](int interval) -> double {
        int oct = 0;
        int deg = interval;
        if (deg >= 0) {
            oct = deg / N;
            deg = deg % N;
        } else {
            oct = -((-deg + N - 1) / N);
            deg = (deg % N + N) % N;
        }
        double baseRatio = (deg == 0) ? 1.0 : m_scalePitches[deg - 1].ratioValue;
        return baseRatio * std::pow(m_octaveRatio, static_cast<double>(oct));
    };

    // Calculate root frequency so that refMidiNote gives exactly m_refFrequency
    int refInterval = m_refMidiNote - m_rootKey;
    double refRatio = getRatioForInterval(refInterval);
    if (refRatio <= 1e-6) refRatio = 1.0;

    double rootFreq = m_refFrequency / refRatio;
    double masterRatio = centsToRatio(m_masterOffsetCents);

    for (int note = 0; note < 128; ++note) {
        if (!m_directOverride[note]) {
            int interval = note - m_rootKey;
            double noteRatio = getRatioForInterval(interval);
            m_noteFrequencies[note] = rootFreq * noteRatio * masterRatio;
        }
    }
}

} // namespace Jazz
