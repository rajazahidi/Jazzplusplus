# Jazz++ MIDI & Audio Sequencer

[![GitHub Pages](https://img.shields.io/badge/GitHub%20Pages-Live%20Site-00d2ff?logo=github)](https://rajazahidi.github.io/Jazzplusplus/)
[![Latest Release](https://img.shields.io/github/v/release/rajazahidi/Jazzplusplus?color=10b981)](https://github.com/rajazahidi/Jazzplusplus/releases/tag/v5.4.16)
[![CI Build](https://github.com/rajazahidi/Jazzplusplus/actions/workflows/ci.yml/badge.svg)](https://github.com/rajazahidi/Jazzplusplus/actions/workflows/ci.yml)
[![Tests: 130 Passing](https://img.shields.io/badge/Tests-130%20Passing%20(100%25)-10b981.svg)](tests/test_main.cpp)
[![License: GPL-2.0](https://img.shields.io/badge/License-GPL_v2-blue.svg)](COPYING)

**Jazz++** is a full-featured open-source MIDI sequencer and digital audio workstation for Linux and Windows. Built using **wxWidgets**, it provides a rich visual environment for composing, editing, arranging, and recording multi-track MIDI music and digital audio.

Visit the official website: **[https://rajazahidi.github.io/Jazzplusplus/](https://rajazahidi.github.io/Jazzplusplus/)**

Originally created by Andreas Voss and Per Sigmond, with subsequent development by Patrick Earl and Peter J. Stieber. Maintained, modernized, and expanded into a flagship Master Studio DAW by **Raja Zahidi** ([rajazahidi@aol.com](mailto:rajazahidi@aol.com)).

---

## Features

- **Multi-Track Sequencer**: Track timeline editor with mute, solo, track volume, pan, and instrument assignment.
- **Piano Roll Editor**: Note grid editor with velocity bars, pitch bend, and real-time step editing.
- **Event List Editor**: Raw MIDI event viewer and editor supporting SysEx, control changes, and tempo maps.
- **Guitar Fretboard & Tab**: Overhauled responsive fretboard display (1040x280) with tuning labels ('e B G D A E' / 'G D A E'), fret numbers (0-24), pearl inlays, and real-time sound preview on click & drag.
- **MIDI Effects Suite**: Integrated Arpeggiator, Humanizer, Harmonizer, and MIDI Echo accessible via the Effects menu.
- **Procedural Sound FX & 808 Drums**: Built-in 8-bit retro sound generator (8 presets) and classic 808 analog drum synthesizer (6 voices) with WAV export and project insertion.
- **Audio DSP Engine**: High-fidelity real-time 3-Band Parametric EQ, Freeverb Reverb, Stereo Delay, Chorus, and Master Limiter/Overdrive.
- **Harmony & Rhythm Generator**: Harmony analyzer, chord progressions, groove quantization, and shuffle algorithms.
- **Digital Audio Mixing**: Audio track support with WAV sample playback and recording.
- **ALSA & PortMidi Integration**: Native Linux ALSA sequencer (`snd_seq`) and PCM audio (`snd_pcm`) backend.
- **Synthesizer Definitions**: Preset configurations (`.jzi`) for General MIDI (GM), Roland GS, Roland TD-20, Yamaha XG, and E-mu Proteus.

---

## Apex Quantum Enterprise Master Studio Suites (v1.0 – v16.0)

Jazz++ features an enterprise-grade studio DSP, neural inference, synthesis, and music intelligence architecture comprising 16 specialized production suites:

### 1. Apex Quantum Enterprise Suite (Suite v16.0)
- **Modern VST3 & CLAP Plugin Host Bridge with Out-of-Process Sandboxing** (`JZPluginHostBridge`): Full 3rd-party plugin support (CLAP and VST3) with isolated crash-resilient subprocess execution, lock-free parameter automation, and watchdog recovery.
- **Multithreaded DAG Audio Processing Graph Scheduler** (`JZAudioGraphScheduler`): Directed Acyclic Graph (DAG) task scheduling allocating audio processing nodes across 8 to 32 CPU cores with lock-free atomic queues, zero-buffer-underrun multi-core parallelization, and cycle detection.
- **Next-Generation MIDI 2.0 Universal MIDI Packet (UMP) Engine** (`JZMidi2UniversalPacketEngine`): 64-bit and 128-bit UMP packet formats, 16-bit note velocities, 32-bit CC resolution (4.29 billion steps) eliminating zipper noise, per-note pitch bend, and bi-directional MIDI-CI Property Exchange.
- **ARA2 (Audio Random Access 2) Non-Destructive Host Bridge** (`JZAra2HostBridge`): Direct, random-access sample and timeline communication with Celemony Melodyne, Synchro Arts VocALign, and Steinberg SpectraLayers without streaming capture.
- **Real-Time Spatial 6-DoF Head-Tracking Telemetry Engine** (`JZSpatialHeadTracker`): 360-degree head-tracking via Open Sound Control (OSC) and IMU Bluetooth sensors with quaternion sensor fusion, dynamically rotating 3D sound fields relative to listener position for binaural monitoring.
- **Embedded Neural Audio & Deep Learning DSP Engine** (`JZNeuralAudioEngine`): Local ONNX Runtime deep learning architecture supporting 4-stem deep neural unmixing (Demucs v4 style), accelerated GELU/ReLU activations, and Differentiable DSP (DDSP) harmonic resynthesis.

### 2. Apex Singularity Master Studio Pro (Suite v15.0)
- **Real-Time Transient / Tonal Spectral Audio Splitter** (`JZTransientTonalSplitter`): Separates complex mixdown signals into independent transient attack and tonal sustain streams with lossless, phase-coherent reconstruction.
- **Aliasing-Free True-Peak Brickwall Clipper & Oversampler** (`JZBrickwallClipperEngine`): Features 4x/8x linear-phase polyphase oversampling, inter-sample true-peak enforcement (-0.1 dBTP), and 4 analog saturation curves (Hard Brickwall, Soft-Knee Parabolic, Tube Diode, and Apogee Soft-Limit).
- **Generative Drum Poly-Fill & Pattern Morpher** (`JZGenerativeDrumMorpher`): Stochastically morphs rhythm patterns and generates automated polyrhythmic drum fills at phrase boundaries (Bars 4/8/16) across 4 variation styles with timing jitter and ghost notes.
- **Analog Tape Scrape Flutter & Mechanical Chaos Modeler** (`JZAnalogTapeFlutterModeler`): Recreates head-tape micro-friction scrape flutter (2.2kHz - 4.2kHz at 7.5/15/30 IPS), pinch-roller eccentricity (5.5Hz), asymmetric Brownian motor drift, and magnetic particle dropouts.
- **Immersive 3D Spatial Multichannel Reverb Convolver** (`JZImmersive3DReverbConvolver`): Computes 3D room boundary early reflections across 7.1.4 Dolby Atmos ceiling geometries (343 m/s) coupled with an 8-line orthogonal Feedback Delay Network (FDN) and Householder unitary mixing matrix.
- **Broadcast Audio Redundancy & Hot-Standby Failover** (`JZBroadcastFailoverEngine`): Ultra-fast fault detection (< 4ms for silence, buffer stall, NaN/Inf) with 2ms equal-power zero-click crossfading for tour concerts and live broadcast networks.

### 3. Aura Omniscience & Quantum Zenith Suites (Suites v13.0 – v14.0)
- **Real-Time Spectral Stem Rebalancing** (`JZSpectralRebalanceEngine`): Multiband decomposition isolating Vocals, Drums, Bass, and Other with independent gain, pan, solo, and mute controls.
- **3D Binaural HRTF Headphone Spatializer** (`JZBinauralHrtfSpatializer`): Woodworth ITD calculation (~0.656ms), spherical head-shadow ILD filtering, pinna vertical elevation cues, and yaw/pitch/roll head tracking.
- **True Through-Zero Tape Flanger** (`JZThroughZeroFlanger`): Negative lookahead delay matrix sweeping across true $t = 0$ zero delay, yielding infinite comb-filter cancellations and jet-flanger sweeps.
- **Guitar Valve Amp Profiler & Dual-IR Convolver** (`JZGuitarAmpProfiler`): Asymmetric 12AX7 preamp tubes, Class A/AB power stages, dynamic power supply sag, and 64-tap dual-cabinet impulse response convolution with microphone placement controls.
- **Intelligent Dynamic Sidechain EQ & Carve** (`JZDynamicSidechainEq`): 5-band spectral sidechain analysis carving surgical unmasking notches (Kick vs Bass 60Hz, Vocals vs Guitars 2.5kHz) without broadband pumping.
- **Broadcast SMPTE LTC / MTC Timecode Engine** (`JZSmpteTimecodeEngine`): 80-bit biphase mark LTC audio generator/decoder and MIDI Timecode converter supporting 24, 25, 29.97 DF, and 30 fps.
- **Phase-Locked Drum Replacer** (`JZDrumReplacerEngine`): Zero-phase-drift transient replacement with cymbal bleed rejection and 4 velocity layers.
- **Stereo Granular Cloud Synth** (`JZGranularCloudSynth`): Hanning-windowed micro-grain texture generator (5-120ms) with stereo spray, pitch jitter, reverse probability, and time freeze.
- **Sub-Harmonic Synthesizer** (`JZSubHarmonicSynthesizer`): Dual-pole wave-divider generating pure $f_0 / 2$ sub-bass energy (24Hz - 56Hz) with analog Sub Drive saturation.
- **Room Acoustic Calibrator** (`JZRoomAcousticCalibrator`): Calculates axial room standing waves ($f = c / 2L$) and synthesizes minimum-phase inverse correction filters via chirp sine sweeps.
- **Universal Microtonal Tuning Engine** (`JZMicrotonalTuningEngine`): Scala (.scl) and MTS-ESP tuning support with built-in presets (Gamelan Slendro, Pelog, Maqam Rast, Bayati, Pythagorean, 19-EDO, 31-EDO).
- **3D Lissajous Goniometer & Vectorscope** (`JZGoniometerVectorscope`): Real-time polar and Lissajous stereo field imaging, continuous phase correlation metering, and mono-cancellation warning alerts.

### 4. Flagship Enterprise & Next-Gen Production (Suites v11.0 – v12.0)
- **Pitch-Tracking Dynamic Musical EQ** (`JZPitchTrackingEq`): Real-time fundamental pitch ($f_0$) tracking automatically moving biquad center frequencies to musical harmonics with glissando smoothing.
- **Multi-Layer Fractal Generative Arpeggiator** (`JZFractalGenerativeArp`): 4 concurrent arpeggiator layers driven by Fibonacci and Mandelbrot mathematical sequences with polyrhythmic step lengths.
- **Capstan Speed Stabilizer & Wow/Flutter Corrector** (`JZCapstanSpeedStabilizer`): Inverse dynamic varispeed time-warping correcting motor speed fluctuations on degraded analog tapes and vinyl recordings.
- **Audio Groove Extractor & Swing Quantize** (`JZAudioGrooveExtractor`): Extracts micro-timing swing offsets and velocity envelopes from audio drum breaks and converts them into reusable MIDI groove templates.
- **Physical Modeling Wind & Brass Tube Resonator** (`JZWindTubePhysicalModel`): Bidirectional digital waveguide synthesis simulating woodwinds and brass with non-linear reed tables and breath pressure turbulence.
- **Master Linear-Phase FIR Crossover & Psychoacoustic Imager** (`JZMasterLinearPhaseImager`): 4-band zero-phase 31-tap symmetric FIR crossover with per-band stereo widening and elliptical mono bass guard.
- **Dual-Mic Dynamic Phase Aligner** (`JZPhaseAlignerEngine`): Auto-correlation delay compensation (0.01ms - 25ms), continuous 0-360° phase rotation, and polarity inversion for multi-mic drum and cab setups.
- **AI Melody & Counterpoint Generator** (`JZAiMelodyCounterpointGenerator`): Fuxian counterpoint voice leading adhering to contrary motion and modal tension resolutions.
- **Vocal Stereo Doubler & Micro-Detune Ensemble** (`JZVocalDoublerEngine`): 4-voice dimensional stereo doubler with independent cent detuning, timing jitter, and mono-compatibility guard.
- **Upward / Downward OTT Multiband Compressor** (`JZMultibandOttProcessor`): 3-band simultaneous downward transient taming and upward nuance expansion (+18dB).
- **Vintage Studio Microphone Modeler** (`JZMicrophoneModeler`): Accurate acoustic modeling of German U87, Vintage 251, Broadcast 7B, and Ribbon R121 capsules with proximity effect controls.
- **Streaming Loudness Target & True-Peak Inspector** (`JZStreamingLoudnessInspector`): Integrated LUFS, Short-Term LUFS, LRA, and True-Peak dBTP metering with official Spotify, Apple Music, YouTube, and EBU R128 targets.

### 5. Core Production, Arranger & Mastering Architecture (Suites v1.0 – v10.0)
- **Audio Restoration**: Cubic polynomial De-Clipper and linear-predictive residual De-Clicker (`JZAudioRestorationEngine`).
- **Realistic Guitar Strumming & Fingerpicking**: Strum timing delays, Down/Up alternating strokes, and P-I-M-A classical picking patterns (`JZGuitarStrumEngine`).
- **Immersive 7.1.4 Bed Panner**: 12-channel surround and ceiling elevation object panning (`JZImmersiveAudioPanner`).
- **Vintage Lo-Fi Cassette Modeler**: Wow/flutter pitch wobble, azimuth roll-off, and tape hiss emulation (`JZVintageLofiProcessor`).
- **Automatic Song Key & Chord Timeline Detector**: Krumhansl-Schmuckler chromagram profile matching (`JZKeyChordDetector`).
- **Spectral Resonance Tamer**: Dynamic narrow-band peak suppression across 1kHz - 11kHz (`JZSpectralResonanceTamer`).
- **Vocal Note-Blob Pitch Editor**: Graphical note-level pitch segmentation and correction (`JZVocalPitchEditor`).
- **AI Spectral Stem Separator**: HPSS harmonic-percussive separation and magnitude unmixing (`JZStemSeparatorEngine`).
- **Universal Parameter Modulation Matrix**: 8-slot multi-shape LFO and envelope modulation router (`JZModulationMatrix`).
- **Polymetric Drum Sequencer**: Independent track step lengths, polyrhythms, and ratchets (`JZPolymetricDrumSequencer`).
- **Master Match EQ Profiler**: 64-band reference spectrum learning and curve matching (`JZMatchEqProfiler`).
- **4-Operator FM Synthesizer**: 8 algorithms, feedback modulation, and 16-voice polyphony (`JZFmSynthesizer`).
- **MIDI Humanizer & Note Probability**: Gaussian timing deviations and ghost note injection (`JZMidiHumanizerProbability`).
- **Dynamic Auto-Wah & Vowel Formant Filter**: Dual-resonance vocal tract envelope sweeps (`JZDynamicAutoWah`).
- **Stereo Modulation Multi-FX**: Dimension Chorus, 12-Stage Optical Phaser, and Through-Zero Flanger (`JZModulationMultiFX`).
- **Automatic Vocal Timing Aligner**: Dynamic Time Warping (DTW) dub vocal alignment (`JZVocalAligner`).
- **Master True-Peak Limiter**: 8x polyphase oversampling, lookahead buffer, and -0.1 dBTP ceiling (`JZMasterTruePeakLimiter`).
- **Modern Wavetable Synthesizer**: 64-frame 2D wavetable morphing and Supersaw unison (`JZWavetableSynth`).
- **Audio-to-MIDI Transcription**: Live audio pitch and transient extraction to Piano Roll MIDI notes (`JZAudioToMidiTranscriber`).
- **Dynamic 24-Band Vocoder**: Filter bank robotic speech synthesis with formant shifting (`JZVocoderEngine`).
- **Karplus-Strong Physical Resonator**: Plucked string and acoustic chime synthesis (`JZPhysicalResonator`).
- **Mix Masking Collision Analyzer**: Multi-track 32-band spectral masking detection (`JZMixMaskingAnalyzer`).
- **Master TPDF Dithering & Multi-Format Audio Encoder**: 16/24/32-bit WAV export with psychoacoustic noise shaping (`JZDitherFormatEncoder`).
- **Studio Spectral Denoiser**: Automated spectral fingerprint subtraction noise repair (`JZSpectralDenoise`).
- **Vocal Polyphonic Harmonizer**: 4-part diatonic vocal harmonies with formant lock (`JZVocalHarmonizer`).
- **Stereo Mid-Side Matrix & Mono Bass Maker**: M/S encoding with elliptical sub-bass centering (`JZMidSideProcessor`).
- **Performance Chord Pads**: 16-pad live performance chord trigger matrix with Drop-2 voicing (`JZChordPerformancePads`).
- **Analog Tape Machine & Console Warmer**: Magnetic tape hysteresis and tube transformer saturation (`JZTapeConsoleWarmer`).
- **Dynamic Vocal De-Esser**: Split-band and wideband sibilance compression (`JZDynamicDeEsser`).
- **Audio Flex-Time Warping & Transient Quantizer**: Anchor-pin time manipulation and auto-quantization (`JZAudioFlexTimeWarp`).
- **3-Band Multiband Compressor**: 4th-order Linkwitz-Riley linear-phase dynamics processor (`JZMultibandCompressor`).
- **Non-Linear Session Clip Matrix**: 8x8 Ableton-style live scene launcher grid (`JZClipMatrixEngine`).
- **Transient Shaper & Punch Designer**: Independent attack and sustain envelope sculpting (`JZTransientShaper`).
- **ITU-R BS.1770-4 / EBU R128 Loudness Meter**: Integrated LUFS, Short-Term, Momentary, and True Peak (`JZLoudnessMeterEBU`).
- **Studio Sidechain Compressor & Dynamic Ducking**: External sidechain bus routing with filter listen (`JZSidechainCompressor`).
- **Multi-Track Stems Exporter**: Batch rendering of stems with auto-silence trimming (`JZStemsExporter`).
- **Real-Time Vocal Pitch Correction**: Sub-sample pitch quantizer with retune speed control (`JZPitchCorrectionEngine`).
- **Convolution Reverb & IR Cabinet Simulator**: High-resolution room and speaker impulse response convolver (`JZConvolutionEngine`).
- **Multi-Velocity Zone Sampler**: Polyphonic sampler with key-ranges and dynamic velocity layers (`JZMultiVelocitySampler`).
- **Master Spectrum Analyzer & Visualizer**: 31-band 1/3-octave ISO display and Lissajous goniometer (`JZMasterVisualizer`).
- **WSOLA Time-Stretching & Pitch-Shifting**: Independent audio duration and pitch scaling (`JZAudioTimeStretch`).
- **Virtual Mixing Console Desk**: Multi-channel vertical fader strips, LED meters, and master bus limiter (`JZVirtualMixer`).
- **Transient Slicer & Beat Detective**: Audio loop slicing to chromatic MIDI trigger tracks (`JZTransientSlicer`).
- **Euclidean Rhythm & Algorithmic Arpeggiator**: Bresenham polyrhythmic generation and arpeggio patterns (`JZEuclideanArp`).
- **In-Key Piano Roll & Chord Stamp**: Diatonic snapping and 1-click chord stamp palettes (`JZPianoScaleChordHelper`).
- **Loop Takes & Comping System**: Multi-lane take recording and seamless swipe comping (`JZTakesComping`).
- **Dynamic Automation Curves**: Linear, Bezier, Exponential, and Sigmoid automation lanes (`JZAutomationEngine`).
- **Low-Latency WASAPI Audio Driver**: Sub-3ms real-time audio latency on modern Windows systems (`JZWasapiAudioDriver`).
- **Waveform Display & Clip Editing**: Multi-resolution pyramid peak display and non-destructive trimming (`JZAudioWaveform`).
- **Groove Pool Manager**: MPC-60, MPC-3000, and J Dilla swing and timing pocket humanization (`JZGroovePool`).
- **MusicXML & MPE Engine**: MusicXML 3.1 score exchange and MIDI Polyphonic Expression multi-channel routing (`JZMpeEngine`).
- **Self-Contained Project Bundles**: Consolidated `.jazzbundle` archive format packaging MIDI, audio samples, and presets (`JZProjectBundle`).
- **Intelligent Jazz Reharmonizer**: Tritone substitutions, secondary dominants, and Coltrane changes (`JZAutoAccompaniment`).
- **Vocal & Horn Section Harmonizer**: 3-piece horn and 4-part SATB choir harmonic voicings (`JZAutoAccompaniment`).
- **Timeline Section Banners & SMF Markers**: Color-coded arrangement markers and Standard MIDI File marker export (`JZTrackWindow`).
- **Studio Woodblock Click Track**: Dual-tone metronome count-in audio generator (`JZAutoAccompaniment`).
- **Smooth Voice Leading Engine**: Inversion selection minimizing semitone jumping across chord transitions (`JZAutoAccompaniment`).
- **Global Key Center Detection**: Krumhansl-Schmuckler 24-key tonal correlation algorithm (`JZAutoAccompaniment`).

---

## Automated Unit Testing & Quality Assurance

Jazz++ includes a comprehensive, automated test suite covering all DSP engines, musical generators, and file formats:

```bash
# Run all 130 unit tests via CTest
ctest --test-dir build --output-on-failure
```

```
Test project C:/Project/jazz/build
    Start 1: JazzUnitTest
1/1 Test #1: JazzUnitTest .....................   Passed    0.15 sec

100% tests passed, 0 tests failed out of 1
Total Test time (real) =   0.18 sec
```

All **130 test suites** pass with 100% accuracy, verifying sample-accurate DSP processing, numerical stability, and memory safety.

---

## Quick Start: Pre-built Binaries

Pre-compiled standalone packages for Linux and Windows are available from the [GitHub Releases](https://github.com/rajazahidi/Jazzplusplus/releases) page or [GitHub Actions Artifacts](https://github.com/rajazahidi/Jazzplusplus/actions).

### Linux (AppImage)
1. Download **`Jazz++-x86_64.AppImage`**.
2. Make it executable and run:
   ```bash
   chmod +x Jazz++-x86_64.AppImage
   ./Jazz++-x86_64.AppImage
   ```

### Windows (64-bit ZIP)
1. Download **`Jazz++-Windows-x64.zip`**.
2. Extract the archive and launch **`jazz.exe`**. All required wxWidgets 3.3.4 runtime DLLs are included.

---

## Smart Launcher & SoundFont Playback

Jazz++ includes a smart launcher (`scripts/jazz-launcher.sh`) that automatically detects if an ALSA software synthesizer (such as FluidSynth or TiMidity) is already active. If none is running, it finds installed SoundFonts (e.g., `default-GM.sf2` or `FluidR3_GM.sf2`) and manages a lightweight background synthesizer so you can immediately hear MIDI playback without manual setup!

To check your MIDI audio environment:
```bash
./scripts/jazz-launcher.sh --check
```

---

## System Requirements & Prerequisites

On Debian, Ubuntu, Linux Mint, or Zorin OS:

```bash
sudo apt update
sudo apt install build-essential cmake libwxgtk3.2-dev libasound2-dev
```

*(For older Ubuntu releases, `libwxgtk3.0-gtk3-dev` can be used).*

---

## Building Jazz++

### Recommended: Modern CMake Build

```bash
# 1. Configure the build
cmake -B build -DCMAKE_BUILD_TYPE=Release

# 2. Compile using all available CPU cores
cmake --build build -j$(nproc)

# 3. Run the application
./build/jazz
```

### Alternative: GNU Autotools Build

```bash
# 1. Generate configure script
./bootstrap

# 2. Configure with ALSA and audio driver support
./configure --enable-alsa --enable-sequencer2

# 3. Compile
make -j$(nproc)

# 4. Run
./src/jazz
```

---

## Installation

To install Jazz++ globally to `/usr/local/`:

```bash
sudo cmake --install build
```

This installs:
- Executable: `/usr/local/bin/jazz`
- Desktop Launcher: `/usr/local/share/applications/jazz.desktop`
- Synthesizer Configurations: `/usr/local/share/jazz/conf/`
- Demo Songs & Grooves: `/usr/local/share/jazz/song/`

---

## Directory Overview

- [`src/`](src/): Main C++ application source code, UI frames, dialogs, and audio/MIDI drivers.
- [`conf/`](conf/): Instrument maps (`.jzi`), drum sets, and default configuration (`jazz.cfg`).
- [`song/`](song/): Demo songs (`.mid`), rhythm templates (`.rhy`), and sample audio (`.wav`).
- [`portmidi/`](portmidi/): Embedded PortMidi library source code.
- [`midinetd/`](midinetd/): Standalone remote MIDI daemon (ONC/Sun RPC).
- [`codeblocks/`](codeblocks/): Code::Blocks project files.
- [`vc12/`](vc12/): Visual Studio 2013 project files.
- [`tests/`](tests/): Automated unit test suite covering 130 unit test suites.

---

## License

Jazz++ is released under the **GNU General Public License v2 (GPL-2.0)**. See [`COPYING`](COPYING) for details.
