# Jazz++ MIDI & Digital Audio Workstation

[![GitHub Pages](https://img.shields.io/badge/GitHub%20Pages-Live%20Site-00d2ff?logo=github)](https://rajazahidi.github.io/Jazzplusplus/)
[![Latest Release](https://img.shields.io/github/v/release/rajazahidi/Jazzplusplus?color=10b981)](https://github.com/rajazahidi/Jazzplusplus/releases/tag/v5.4.16)
[![CI Build](https://github.com/rajazahidi/Jazzplusplus/actions/workflows/ci.yml/badge.svg)](https://github.com/rajazahidi/Jazzplusplus/actions/workflows/ci.yml)
[![Tests: 555 Passing](https://img.shields.io/badge/Tests-555%20Passing%20(100%25)-10b981.svg)](tests/test_main.cpp)
[![Platforms](https://img.shields.io/badge/Platforms-Linux%20%7C%20macOS%20%7C%20Windows-blue.svg)](INSTALL.md)
[![C++ Standard](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](CMakeLists.txt)
[![License: GPL-2.0](https://img.shields.io/badge/License-GPL_v2-blue.svg)](COPYING)

**Jazz++** is an enterprise-grade, open-source multi-track MIDI sequencer and digital audio workstation (DAW) for **Linux**, **macOS**, and **Windows**. Built using **modern C++17** and **wxWidgets**, it delivers an ultra-low-latency real-time audio pipeline, an extensive suite of 35 specialized Apex Master Studio DSP and spatial audio engines, out-of-process plugin crash sandboxing, physical acoustic modeling, and broadcast metadata export.

Visit the official website: **[https://rajazahidi.github.io/Jazzplusplus/](https://rajazahidi.github.io/Jazzplusplus/)**

Originally created by Andreas Voss and Per Sigmond, with subsequent development by Patrick Earl and Peter J. Stieber. Maintained, modernized, and expanded into a flagship Master Studio DAW by **Raja Zahidi** ([rajazahidi@aol.com](mailto:rajazahidi@aol.com)).

---

## Documentation Quick Links

| Document | Description |
| :--- | :--- |
| [**`INSTALL.md`**](INSTALL.md) | Comprehensive step-by-step installation & build guide for Linux, macOS, and Windows |
| [**`ARCHITECTURE.md`**](ARCHITECTURE.md) | In-depth system design, DAG ADC scheduling, real-time concurrency, and threading model |
| [**`CONTRIBUTING.md`**](CONTRIBUTING.md) | Contribution guidelines, C++17 coding standards, and pull request workflow |
| [**`SECURITY.md`**](SECURITY.md) | Security policy, vulnerability disclosure, and out-of-process plugin isolation details |
| [**`ChangeLog`**](ChangeLog) | Complete history of changes, version announcements, and maintenance commits |

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
- **Full Menu, Function & Knob Simulation Suite**: Validates automated transport controls (Play, Loop, Record, Metronome, Panic Reset), menu command routing across File, Edit, Tools, Settings, Audio, and Effects, as well as rotary knobs and faders scaling.

---

## Apex Colossus Flagship Master Studio Suites (v1.0 – v35.0)

Jazz++ features an enterprise-grade studio DSP, neural inference, spatial metadata, collaborative AoIP, and hardware control surface architecture comprising 35 specialized production suites:

### 1. Apex Transcendence Sovereign Ultimate Release Suite (Suite v35.0)
- **Out-of-Process VST3/CLAP Plugin Crash Sandboxing & Host Isolation Architecture** (JZPluginCrashSandboxEngine): Protects the DAW host process from buggy third-party plugin crashes (Bitwig Studio / FL Studio style) featuring lock-free IPC shared-memory ringbuffers, watchdog heartbeat monitoring (100ms-1000ms), structured exception handling (SEH) signal isolation trap, failsafe automatic dry-bypass, and seamless zero-downtime worker respawn with state chunk restoration.
- **ITU-R BS.1770-4 Multichannel 7.1.4 Immersive Audio Loudness & Spatial True-Peak Meter** (JZMultichannelAtmosLoudnessMeterEngine): Implements full 12-channel surround and immersive bed loudness metering with 2-stage K-weighting pre-filtering (Stage 1 acoustic head model high-shelf, Stage 2 RLB high-pass), surround/overhead channel weighting (+1.5 dB on surrounds and tops), LFE exclusion, dual-gated Integrated LUFS (-70 LKFS absolute gate and -10 LU relative gate), Momentary LUFS (400ms), Short-Term LUFS (3s), Loudness Range (LRA), and 4x oversampled Spatial Inter-Sample True-Peak detection across all 12 channels.
- **Lossless Broadcast Wave Format (BWF / EBU Tech 3285) & iXML Multi-Stem Metadata Exporter** (JZBroadcastWaveIxmlExporterEngine): Production-grade RIFF chunk packager for film scoring, television post-production, and broadcast delivery with standard ext (Broadcast Audio Extension) chunk supporting SMPTE sample offsets from midnight, EBU R128 integrated loudness and true-peak metrics, and embedded iXML chunk schemas (Project, Scene, Take, Tape, Timecode Rate, Track Names, Polyphonic stem flags).
- **Multi-Resolution Hierarchical Pyramid GPU Waveform Tile Cache & Viewport Rasterizer** (JZGpuWaveformTileCacheEngine): Multi-scale 5-level hierarchical LOD pyramid (1:1 full sample resolution, 1:16, 1:256, 1:4096, 1:65536 macroscopic zoom) with fixed-size LRU vertex tile caching, sub-millisecond viewport tile retrieval, eliminating CPU audio buffer re-reading bottlenecks during rapid horizontal zoom and scrub, and guaranteeing 120 FPS buttery-smooth timeline navigation across 200+ audio tracks.
- **Complex Sidechain Directed Acyclic Graph (DAG) Automatic Delay Compensation (ADC) Solver** (JZComplexSidechainAdcGraphResolver): Resolves multi-hop nested subgroup routing, parallel audio busses, and cross-track sidechain dependencies with Kahn's topological sort, illegal feedback cycle detection, critical-path latency analysis, and sample-accurate delay compensation buffer calculation across all parallel branches, eliminating comb filtering and phase smearing.
- **Real-Time Polyphonic Dynamic Hermode Micro-Tuning & Pure Just Intonation Retuner** (JZDynamicHermodeTuningEngine): Dynamically eliminates harmonic beating in orchestral, choral, and acoustic instruments by analyzing polyphonic chord structures in real time, detecting root/bass foundation, and calculating pure 5-limit / 7-limit Just Intonation cents offsets (Major 3rd -13.7c, Minor 3rd +15.6c, Perfect 5th +2.0c, Harmonic 7th -31.2c) with continuous micro-cents glide smoothing.

### 2. Apex Transcendence Sovereign Eternity Suite (Suite v34.0)
- **Continuous Frequency-Dependent All-Pass Phase Dispersion & Multi-Mic Coherence Maximizer** (JZSpectralPhaseDisperserEngine): Frequency-dependent phase rotation matrix (0 to 360 degrees continuously per band) across 4 bands (Sub, Low, Mid, High) mitigating acoustic comb filtering and maximizing multi-microphone drum/room coherence without transient smearing.
- **Bowed String Acoustic Body Impulse & Chladni Wood Resonator** (JZBowedStringAcousticBodyConvolverEngine): Physically models acoustic soundboxes across the 4 violin family instruments (Violin, Viola, Cello, Double Bass) with spruce top-plate and maple back-plate Chladni eigenmodes (B1- and B1+), bridge rock/bounce transmission filter, F-hole Helmholtz bass air cavity coupling (A0 mode: 280 Hz violin, 98 Hz cello), and soundpost asymmetric transfer.
- **Transient-Gated Modal Ringing & Resonant Overhang Damping Engine** (JZTransientRingingSuppressorEngine): Eliminates stubborn drum shell and metallic percussion ringing overhang (Oeksound Spiff / Eventide SplitEQ style) featuring 100% transparent transient strike preservation (0-15ms attack window), spectral decay rate tracking, adaptive 8-notch modal ring damping (-3 dB to -30 dB), and Ringing Delta Solo mode.
- **Binaural Relativistic Acoustic Doppler & Moving Sound Source Spatializer** (JZBinauralDopplerMotionSpatializerEngine): Calculates 3D vector velocity relativistic acoustic Doppler pitch shift and trajectories with independent Left/Right fractional-delay sinc interpolation lines, 3D kinematics (position x, y, z and velocity vx, vy, vz), interaural parallax Doppler effect during high-speed fly-bys, Woodworth ITD head delay, and distance attenuation.
- **Dynamic Kick-to-Bass Fundamental Frequency Surgical Sidechain Carver** (JZDynamicSubBassSpectralCarverEngine): Dynamically unmasks kick drum punch from bass tracks without destructive wideband pumping (TrackSpacer / Soothe style) with real-time kick fundamental pitch tracking (35 Hz to 120 Hz), surgical dynamic notch carving (0 dB to -24 dB) during kick transient impact, ultra-fast recovery, and Carve Delta Solo mode.
- **Historic 18dB/24dB Diode Ladder Non-Linear Resonant Filter Engine** (JZVintageDiodeLadderFilterEngine): Circuit-accurate physical modeling of the legendary 1971 EMS VCS3 and 1982 Roland TB-303 diode ladder filters featuring 4-stage diode ladder network with non-linear Shockley diode conduction, asymmetrical dynamic clipping, acid resonance squelch with low-end retention, accent overdrive punch, and selectable 18dB/oct vs 24dB/oct slopes.

### 3. Apex Transcendence Sovereign Infinity Suite (Suite v33.0)
- **Dynamic Atmospheric Wind Buffeting & Diaphragm Overload Attenuator** (JZDynamicWindTurbulenceSuppressorEngine): Reduces outdoor wind buffeting, micro-gust turbulence, and microphone capsule overload (Cedar Wind Filter / iZotope RX De-wind style) with sub-band energy correlation (<150 Hz), non-stationary gust acceleration tracking, adaptive high-pass cutoff expansion (20 Hz to 350 Hz), sub-band gust damping (-3 dB to -36 dB), and Wind Delta Solo audition mode.
- **Historic Fasel / Halo Toroidal Inductor Wah-Wah & Resonant Drive Engine** (JZVintageInductorWahPedalEngine): Physical circuit modeling of the legendary 1967 Vox Clyde McCoy and Thomas Organ Cry Baby wah pedals featuring 500mH toroidal inductor magnetic flux saturation with non-linear B-H tanh() curve, 100k ICAR S-taper potentiometer taper, 2N5172 BJT transistor feedback gain stage, selectable Fasel Red / Yellow / Halo inductors, and foot treadle frequency sweep (350 Hz to 2.2 kHz).
- **Ultra-Low-Latency Polyphase Anti-Aliasing Oversampling Wrapper Engine** (JZPolyphaseAntiAliasingOversamplerEngine): Studio-grade multi-rate polyphase FIR oversampling (2x, 4x, 8x, 16x) with symmetric half-band coefficients and >100 dB stopband alias attenuation, dual phase modes (Linear Phase for zero-distortion mastering vs Minimum Phase for zero-latency live tracking), and wrapped processing functors for non-linear distortion algorithms.
- **Electromechanical 91-Tonewheel Generator & 9-Drawbar Organ Engine** (JZTonewheelOrganDrawbarEngine): Physically models the 1935 Hammond Model B3 / C3 tonewheel organ with all 91 electromechanical tonewheels running at precise historical gear ratios (32.7 Hz to 5919 Hz), 9 standard harmonic drawbars (16', 5 1/3', 8', 4', 2 2/3', 2', 1 3/5', 1 1/3', 1') with 0-8 steps, touch-responsive 2nd/3rd harmonic percussion with Fast/Slow decay, mechanical key click noise generator, and magnetic pickup leakage crosstalk.
- **ISO 9613-1 Atmospheric Air Molecular Absorption & Acoustic Distance Attenuator** (JZAtmosphericAirAbsorptionSpatialEngine): Physically simulates sound wave propagation through air according to the ISO 9613-1 standard with molecular relaxation of Oxygen and Nitrogen dependent on temperature (-10C to +40C) and relative humidity (10% to 100%), progressive high-frequency absorption filtering over distance (0.5m to 300m), geometric inverse-square law attenuation, speed-of-sound propagation delay line (343 m/s), and close-mic proximity bass boost.
- **Alan Blumlein Frequency-Dependent Stereo Shuffler & Polar Image Sculptor** (JZStereoWidthSpectralShufflerEngine): Implements the classic Blumlein stereo shuffling matrix for frequency-dependent stereo image manipulation with low-frequency Side bass boost below 700 Hz for acoustic ITD spaciousness, sub-bass elliptical mono-maker (<120 Hz) for vinyl/subwoofer punch, high-frequency Air expansion (>6 kHz), and continuous phase correlation metering with automatic mono-cancellation guard.

### 4. Apex Transcendence Sovereign Millennium Suite (Suite v32.0)
- **Real-Time Vocal Microphone Rustle, Clothing Friction & Mechanical Thud Attenuator** (JZVocalMicRustleAttenuatorEngine): Eliminates non-stationary clothing friction and mechanical mic capsule thuds (iZotope RX De-rustle / Accentize style) featuring 8-band friction variance tracking (80 Hz to 1.2 kHz), fast dynamic suppression (-3 dB to -24 dB) with sub-5ms recovery, vocal formant and fundamental pitch preservation, and Rustle Delta Solo audition mode.
- **Historic Robert Moog 4-Pole 24dB/Octave Non-Linear Transistor Ladder Filter Engine** (JZMoogTransistorLadderFilterEngine): Physical modeling of the legendary 1965 Minimoog Model D ladder filter featuring 4 cascaded differential transistor pairs with non-linear tanh() saturation, negative resonance feedback, pure sinusoidal self-oscillation at resonance >= 1.0, thermal voltage drift, and pre-filter analog drive (+18 dB).
- **Spatial Mid/Side Transient-Tonal Polar Balancer & Stereo Image Sculptor** (JZSpatialTransientTonalBalancerEngine): Independent spatial positioning for transients versus sustained tonal bodies (Eventide SplitEQ / Sound Radix Powair style) with dual-envelope M/S decomposition, independent transient width (0.0x - 2.0x) and tonal width (0.0x - 2.5x), polar angle panning, transient centering lock for mono punch, and broadcast phase correlation guard.
- **Adaptive Mains Hum (50/60Hz) & Electromagnetic Ground-Loop Harmonic De-Hummer** (JZAdaptiveMainsHumRemoverEngine): Removes electrical mains hum and ground loop buzz (iZotope RX De-hum / Waves WNS style) with software Phase-Locked Loop (PLL) tracking electrical grid drift (+/- 2 Hz around 50 Hz and 60 Hz), an array of 16 phase-coherent narrow notch filters tracking all harmonics simultaneously, odd/even harmonic balance, and Hum Delta Solo mode.
- **Concert Grand Piano Sympathetic String & Damper Pedal Acoustic Resonance Engine** (JZGrandPianoSympatheticResonanceEngine): Physically simulates sympathetic soundboard and string coupling (Modartt Pianoteq / Ivory II style) with physical 88-string bridge cross-coupling matrix (A0 to C8), damper pedal (MIDI CC 64) full-string un-damped harmonic wash, modal resonator bank for harmonic nodes, duplex scale inharmonic treble singing overtones, and spruce soundboard radiation.
- **5-Band Linear-Phase Mastering Dynamics Processor & Multiband Compressor** (JZMasterLinearPhaseMultibandDynamicsEngine): Zero-phase-smearing mastering dynamics (FabFilter Pro-MB / TC Electronic Master X HD style) featuring 5-band linear-phase crossover (Sub, Low-Mid, Mid, High-Mid, Air), independent soft-knee logarithmic VCA compressor per band, ultra-fast attack and program-dependent release, individual band solo, and dynamic gain reduction telemetry.

### 5. Apex Transcendence Sovereign Pinnacle Suite (Suite v31.0)
- **Real-Time Neural Spectral Dereverberation & Early Reflection Stripper** (JZNeuralSpectralDereverberationEngine): Eliminates room acoustic boxiness and early wall reflections (< 40ms) with sub-band spectral cross-correlation and diffuse reverberant tail subtraction featuring unvoiced consonant transient protection, room size acoustic modeling (2.0 to 25.0 meters), and Reverb Tail Delta Solo audition mode.
- **Vintage Compact Cassette Lo-Fi Tape & Mechanical Flutter/Dropouts Engine** (JZVintageCassetteLoFiTapeEngine): Physically simulates 1-7/8 IPS compact cassette tape decks (Universal Audio Oxide / RC-20 / SketchCassette style) with 3 tape formulations (Type I Ferric, Type II Chrome, Type IV Metal), chaotic dual-motor mechanical Wow (0.5 - 3.5 Hz) and Flutter (20 - 50 Hz), Bessel head-gap loss high-frequency rolloff, tape hiss profile, and random magnetic oxide signal dropouts.
- **Real-Time 4-Voice Intelligent Vocal Harmonizer & Key-Scale Follower Engine** (JZIntelligentVocalHarmonizerEngine): Generates 4-part backing vocal stacks conforming to musical key and diatonic scale (Eventide H9000 / Antares Harmony Engine style) featuring YIN/autocorrelation fundamental pitch tracking (70 Hz - 1000 Hz), scale follower (Major, Minor, Dorian, Mixolydian, Phrygian, Lydian, Harmonic Minor), independent formant shifting, per-voice 3D stereo panning and gain, and humanized micro-pitch detune drift (+/- 12 cents).
- **Mastering Target Curve Spectrum Matcher & 32-Band Morphing Equalizer** (JZMasteringTargetCurveMatcherEngine): Aligns mix balances with commercial mastering curves (FabFilter Pro-Q 3 Match EQ / iZotope Ozone style) featuring 32-band Long-Term Average Spectrum (LTAS) analysis, standard target curves (Pink Noise -4.5dB/oct, Modern Pop/Urban, Warm Acoustic, Classical Hi-Fi, EDM Sub), Gaussian spectral curve smoothing, and adjustable match depth (0% to 200%).
- **Digital True-Peak De-Clipper & Cubic Hermite Spline Waveform Inpainter** (JZDigitalTruePeakDeClipperEngine): Repairs harsh digital overs and flat-topped waveform clipping (iZotope RX De-clip / Cedar Audio Declip style) featuring adaptive flat-top saturation detection, Cubic Hermite Spline inpainting reconstructing lost peak trajectories above 0 dBFS, harmonic peak resynthesis, and automatic dynamic headroom compensation.
- **Stereo Transaural Acoustic Crosstalk Cancellation (XTC) & 3D Monitor Spatializer** (JZTransauralCrosstalkCancellerEngine): Delivers 180-degree immersive 3D soundfields across physical studio nearfield monitors without headphones (BACCH 3D Sound / Universal Audio Transaural style) featuring Cooper-Bauck recursive 2nd-order cancellation matrix, Woodworth inter-aural time difference (ITD) delay calculation, head shadow acoustic modeling, and adjustable monitor aperture angle (20 to 90 degrees).

### 6. Apex Colossus Sovereign Diamond Jubilee Suite (Suite v30.0)
- **Real-Time Non-Linear Dilated WaveNet Guitar Amp Profiler** (JZNeuralAmpWaveNetProfilerEngine): Deep temporal convolutional neural guitar amplifier profiling (Neural Amp Modeler / Quad Cortex / Neural DSP Archetype style) with 4-stage dilated 1D temporal convolution layers (dilations: 1, 2, 4, 8), non-linear gated activations (tanh * sigmoid), 12AX7 preamp dual-triode asymmetric bias clipping, 6L6/EL34 push-pull power stage with dynamic transformer saturation, real-time power supply rail sag, and sub-0.5ms zero-latency execution.
- **Real-Time Polyphonic Audio Chord Parser & Harmonic Voice-Leading Transcriber** (JZPolyphonicAudioChordParserEngine): Parses raw polyphonic audio into discrete harmonic chords and voice-leading structures (Melodyne Polyphonic / zplane deCoda style) featuring 12-bin Pitch Class Profile (PCP) chromagram extraction across 4 octaves, harmonic template correlation matching (Major, Minor, Dominant 7th, Maj7, Min7, Diminished, Augmented, Sus4, Min9), bass note and inversion detection (slash chords), temporal hysteresis, and multi-note MIDI voicing generation.
- **3-Band Linear-Phase Frequency-Split Mid/Side Dynamic Spectral Panner** (JZSpectralFrequencySplitPannerEngine): Precision frequency-selective stereo imaging and spatial placement (NUGEN Stereoizer / Leapwing StageOne style) featuring 3-band Linkwitz-Riley 4th-order (24 dB/oct) crossover (Low, Mid, High), low-end mono centering lock (< 120 Hz) for mono compatibility, independent pan, width, and gain per band, dynamic auto-pan LFO modulation on high/mid transients, and broadcast phase correlation guard with anti-phase damping.
- **Broadcast Audio Psychoacoustic Watermark & DSSS Steganography Engine** (JZBroadcastAudioWatermarkSteganographyEngine): Embeds and detects robust copyright forensic metadata (Nielsen Watermark / Content ID / Shazam style) featuring Direct Sequence Spread Spectrum (DSSS) modulation with 63-bit Gold Code pseudo-noise sequences, sub-audible psychoacoustic perceptual masking (-24 dB below host energy), 64-bit frame structure (16-bit sync preamble 0xA55A, 32-bit payload, 16-bit CRC-CCITT), and robust decoding surviving lossy compression and volume attenuation.
- **16-Band Sub-Band Dynamic Sidechain Carving Ducker Engine** (JZMultibandDynamicSidechainDuckerEngine): Sculpts acoustic space between colliding tracks (Trackspacer / Cableguys VolumeShaper style) featuring 16-band logarithmic constant-Q filter bank (40 Hz to 16 kHz), external sidechain spectral footprint analysis, dynamic gain reduction applied only to colliding frequency zones, fast envelope tracking (< 2ms attack, program-dependent release), frequency focus range controls, and Delta Solo audition mode.
- **Real-Time Lock-Free Cache-Aligned Slab Buffer Pool Allocator** (JZLockFreeAudioBufferPoolAllocator): Guarantees zero OS heap allocations on real-time audio threads during high-track-count project playback featuring 64-byte CPU cache-line aligned slab buffers, lock-free wait-free O(1) atomic index stack / free-list, thread-safe SPSC/MPMC recycling without mutex locks or system calls, allocation starvation telemetry, and RAII ScopedAudioBuffer automatic memory recovery.

### 7. Apex Transcendence Monumental Suite (Suite v29.0)
- **Real-Time Acoustic Drum Transient Tracker & Phase-Locked Sample Replacer** (JZPhaseLockedDrumReplacerEngine): Professional sample layering and drum replacement (Slate Trigger 2 / Superior Drummer 3 style) featuring sub-sample cross-correlation phase-lock alignment eliminating comb filtering and flanging with acoustic mic bleed, spill rejection bandpass gating, multi-velocity sample crossfading, and polarity inversion.
- **Real-Time Dynamic Psychoacoustic Spectral Resonance Suppressor** (JZSpectralResonanceSuppressorEngine): Automatically tames harshness and resonant modes (Oeksound Soothe 2 / Gullfoss style) with 16-band Bark-scale dynamic spectral smoothing, an array of 8 floating surgical dynamic notch biquads (Q = 5 to 40) tracking moving harshness peaks, user focus bandpass filtering (1.2 kHz to 11 kHz), and Delta Solo listen mode.
- **Historic Teletronix T4 Optical Attenuator Leveling Amplifier Engine** (JZOpticalT4LevelingAmplifierEngine): Physical modeling of classic electro-luminescent photocell leveling amplifiers (Teletronix LA-2A / Tube-Tech CL 1B style) with cadmium-sulfide (CdS) photocell luminescence memory hysteresis, dual-stage release decay (60ms fast initial recovery followed by 1.5 to 5 second program-dependent tail), 2-knob operation, Compress vs Limit modes, and 12AX7/12BH7 tube cathode saturation.
- **16-Channel Binaural Room Impulse Response (BRIR) SOFA Interpolation Engine** (JZBinauralRoomImpulseInterpolationEngine): World-class Dolby Atmos headphone spatialization (Smyth Realiser A16 style) featuring 16-channel discrete BRIR FIR convolution, continuous SOFA spherical coordinate rotation with real-time head-tracking (Yaw, Pitch, Roll), Woodworth ITD and ILD filtering, and multi-room acoustic profiling.
- **Analog Console Component Tolerance Modeling (TMT) & Stereo Crosstalk Matrix** (JZAnalogConsoleChannelToleranceEngine): Recreates the organic three-dimensional depth of legendary analog desks (Neve 8048 / SSL 9000J / Brainworx TMT style) with per-channel pseudo-random component tolerance variance across channels 1 to 64 (1% to 3% capacitor and resistor drift), inter-channel stereo crosstalk bleed (-65 to -90 dB), and Marinair MuMetal transformer core hysteresis.
- **Broadcast Loudness Range (LRA) Dynamics Sculptor & True-Peak Compliance Guard** (JZLoudnessRangeDynamicsSculptorEngine): Conforms audio mixes to strict streaming standards (TC Electronic Master X / NUGEN ISL style) featuring ITU-R BS.1770-4 K-weighting loudness measurement, automatic short-term to integrated LRA compression/expansion targeting an exact Loudness Range (4 to 18 LU), and linear-phase 8x oversampled true-peak inter-sample overshoot limiting (-1.0 dBTP ceiling).

### 8. Apex Transcendence Paramount Suite (Suite v28.0)
- **Vintage Dual-Triode 6386 Variable-Mu Tube Compressor Engine** (JZVariableMuVintageTubeCompressorEngine): Legendary tube master bus compression (Fairchild 670 / Manley Variable Mu style) featuring remote-cutoff dual-triode push-pull gain reduction with smooth continuous variable-slope knee (1.5:1 up to 20:1 under heavy compression), 6 historic time constants, program-dependent dual-decay auto-release, triode harmonic saturation, and Mid/Side matrix processing.
- **Mastering Elliptical Equalizer & Low-End Mono Centering Matrix** (JZEllipticalMonoBassCenteringEngine): Eliminates out-of-phase low-frequency mud and vinyl cutting phase cancellation (Neumann W495 / Brainworx bx_digital style) with Mid/Side frequency-selective Side high-pass filtering (40 Hz to 350 Hz, 12 and 24 dB/oct), sub-bass energy redirection into the Mid channel, and high-frequency stereo width expansion (0.0x to 2.0x).
- **Vocal Plosive & Microphone Wind Blast Infrasonic De-Popper Engine** (JZVocalPlosiveDePopperEngine): Real-time microphone 'P', 'B', and wind blast pop elimination (Cedar Studio / iZotope RX De-plosive style) with asymmetric diaphragm displacement tracking in the 20 Hz – 85 Hz infrasonic band, sub-2ms dynamic suppression, and chest resonance preservation.
- **ITU-R BS.775-3 & Dolby Atmos Intelligent Immersive Folddown Downmix Matrix** (JZImmersiveFolddownDownmixEngine): Precision downmixing of 16-channel 9.1.6 and 12-channel 7.1.4 immersive audio into 5.1 surround and 2.0 stereo with center channel acoustic buildup trim (-3.0 to -6.0 dB), 90-degree Hilbert quadrature phase steering on surround/height channels, and output soft-knee peak protection.
- **Vocal Vibrato, Pitch Drift & Micro-Intonation Humanizer Engine** (JZVocalVibratoHumanizerEngine): Reintroduces organic vocal expression into rigidly pitch-corrected tracks (Celemony Melodyne / Auto-Tune style) with freeform LFO vibrato rate (3.5 to 8.5 Hz) and depth, note-on onset attack delay ramping (0 to 1200 ms), fractal 1/f slow pitch drift, fractional delay interpolation, and coupled amplitude tremolo.
- **Multi-Core CPU Performance Core (P-Core) Affinity & Real-Time Audio Scheduler** (JZRealTimeThreadAffinityScheduler): Locks real-time audio DSP worker threads to high-frequency physical Performance Cores (P-Cores), preventing OS thread migration to power-saving E-Cores with OS thread priority elevation (Windows MMCSS Pro Audio / Linux SCHED_FIFO), lock-free SPSC telemetry ring buffer, and microsecond buffer cycle deadline monitoring.

### 9. Apex Transcendence Sovereign Suite (Suite v27.0)
- **Vocal Sibilance Split-Band Lookahead De-Esser Engine** (`JZVocalSibilanceSplitDeEsserEngine`): Transparent vocal sibilance and harshness suppression (FabFilter Pro-DS / Weiss Deess style) with zero-phase split-band crossover (4 kHz to 12 kHz), psychoacoustic energy detection, -24 dB range reduction, sidechain audition mode, and lookahead dynamic preservation.
- **Master Tape Electro-Magnetic Head-Bump Bass Resonance & Varispeed Transport Engine** (`JZTapeHeadBumpVarispeedTransportEngine`): Physical simulation of master tape machine transport physics and head pole geometry (Studer A800 / Ampex ATR-102 style) featuring speed-dependent head-bump bass resonance (55 Hz at 15 IPS, 110 Hz at 30 IPS), continuous motor varispeed pitch/speed scrubbing (+/- 100%), and mechanical motor brake tape-stop inertia wind-down.
- **Immersive 9.1.6 16-Channel Bass Management & LFE Sub Crossover Matrix** (`JZImmersiveBassManagementCrossoverEngine`): Professional theater-grade bass management for Dolby Atmos and immersive 3D audio environments (Trinnov Altitude / JBL Synthesis style) with 16-channel discrete routing, 4th-order (24 dB/oct) Linkwitz-Riley crossover filtering at 80 Hz, +10 dB cinema LFE subwoofer gain boost, and high-pass acoustic protection on main/height monitors.
- **Master Bus Glue VCA Compressor & Thrust Sidechain Engine** (`JZMasterBusGlueVcaCompressorEngine`): Legendary British analog console stereo master bus VCA compression (SSL 4000 G-Master / API 2500 style) with soft-knee logarithmic VCA compression curve, -3 dB/octave High-Pass Thrust sidechain filter preventing low-frequency pumping, program-dependent dual-decay auto-release, and parallel wet/dry blend.
- **Reactive Load Box Speaker Voice-Coil Impedance Simulation Engine** (`JZReactiveLoadSpeakerImpedanceEngine`): Physical tube amplifier voice-coil reactive interactions (Suhr Reactive Load / Two Notes Torpedo style) featuring dual-resonant LC reactive impedance curve (75 Hz bass resonance with 32 Ohm peak, 2.8 kHz inductive voice-coil rise), power soak attenuation with back-EMF damping, tube amplifier high-voltage power supply rail sag, and multi-cabinet impedance profiling (Vintage 30 4x12, Greenback 4x12, Creamback 2x12, Alnico Blue 1x12).
- **Binaural Acoustic Mastering Room & Boundary Decay Convolver Engine** (`JZBinauralAcousticRoomConvolverEngine`): Transforms spatial Atmos headphone mixes into world-class acoustic mastering control room monitoring (Abbey Road Studio 3 / Sunset Sound Studio style) featuring 3D shoebox room geometry modeling, image-source discrete early reflection taps with Woodworth spherical head model ITD and head shadow ILD filtering, Eyring boundary reverberant decay matching target RT60, and binaural speaker monitor crossfeed.

### 10. Apex Genesis Zenith Suite (Suite v26.0)
- **Acoustic Guitar Body Impulse Convolver Engine** (`JZAcousticGuitarBodyImpulseConvolverEngine`): Transforms harsh undersaddle piezo guitar pickups into rich studio microphone recordings (Fishman Aura / ToneDexter / LR Baggs Voiceprint style) with anti-quack non-linear decompression, 102Hz Helmholtz body cavity resonance, soundhole notch filtering, and studio microphone emulation (Neumann KM184, Royer R-121, AKG C414).
- **Analog Subharmonic Bass & Optical Air Exciter Engine** (`JZAnalogSubharmonicBassExciterEngine`): Dual-band psychoacoustic analog harmonic excitation (Peavey Kosmos / Aphex Aural Exciter style) featuring phase-locked sub-octave fundamental synthesis with low-frequency envelope tracking and program-dependent non-linear optical saturation producing even and odd harmonic air (3 kHz to 12 kHz).
- **Multiband Transient & Sustain Sculptor Engine** (`JZMultibandTransientSculptorEngine`): Frequency-split independent transient punch and sustain shaping (Softube Transient Shaper / Eventide Physion style) across 4 linear-phase bands (Low, Low-Mid, High-Mid, High) with dual fast/slow differential envelope followers and per-band attack and sustain gains.
- **RotaFonic Rotary Speaker Cabinet Simulator (Leslie 122)** (`JZRotaFonicRotarySpeakerSimulator`): Physical aero-acoustic dual-rotor Leslie speaker cabinet simulation featuring rotating treble horn Doppler frequency modulation, wooden bass drum amplitude modulation, mechanical motor inertia acceleration/braking (Chorale, Tremolo, Brake), 12AX7 tube saturation, and dual stereo cabinet mic placement.
- **Real-Time Speech Spectral Subtraction De-Noiser** (`JZSpeechDenoiseSpectralSubtractorEngine`): Continuous background room noise, HVAC hiss, and preamp hum removal (iZotope Voice De-noise / Cedar Studio style) featuring 16-band ambient noise profile learning, adaptive Wiener filter gain suppression (-3 to -30 dB), and unvoiced consonant protection.
- **Spatial Audio Headphone Ear Canal EQ Calibration Engine** (`JZSpatialAudioEarCanalEqCalibrationEngine`): Headphone acoustic calibration and Harman Target Curve auto-equalization (Sonarworks SoundID Reference / AutoEq style) matching Harman 2018 Over-Ear and 2019 In-Ear curves with model compensation presets (Sennheiser HD650, Beyerdynamic DT990, Audio-Technica M50x, AirPods Max) for precision 3D spatial Atmos monitoring.

### 11. Apex Omnipresence Horizon Suite (Suite v25.0)
- **Adaptive Live Acoustic Feedback Suppressor & Dynamic Notch Hunter** (`JZAcousticFeedbackSuppressorEngine`): Real-time live concert acoustic feedback howl suppression (Sabine FBX / dbx AFS style) featuring sub-10ms runaway oscillation peak tracking, 16 ultra-narrow surgical notch biquad filters (Q = 50 to 80), Fixed vs Live decay release modes, and panic limiter ducking.
- **Multi-Track Drum Bleed & Spill Remover** (`JZMultiTrackDrumBleedRemoverEngine`): Cross-microphone drum leakage isolation (Wilkinson De-Bleed / Oxford Drum Gate style) carves out hi-hat and cymbal spill from close-mic snare, kick, and tom tracks using sidechain transient fingerprinting, 220Hz body transient detection, and solo bleed monitoring.
- **Multi-Track Vocal Audio Alignment & Doubler Elastic Time-Warper** (`JZVocalAlignmentTimeWarperEngine`): Automated vocal timing and phrasing alignment (Synchro Arts VocALign / Revoice Pro style) matching dub backing vocal stacks to a lead guide track using Dynamic Time Warping (DTW), phoneme energy contour extraction, and tightness interpolation.
- **Physical Cold-Rolled Steel Plate Reverb Engine** (`JZSteelPlateReverbPhysicalEngine`): Physical simulation of historic mechanical steel plate reverberation (EMT 140 style) solving 2D wave equation modal dispersion across 16 rectangular plate eigenmodes with electromagnetic driver coil saturation, asymmetrical dual-piezo pickups, and mechanical porous damper pad decay (0.5 to 5.5s).
- **Solid-State Analog BBD Chorus & Spatial Dimension Matrix** (`JZAnalogBbdChorusDimensionEngine`): Discrete Bucket Brigade Device physical modeling (Panasonic MN3005 / MN3007) featuring Charge Transfer Inefficiency (CTI) dispersion, clock-rate-dependent Sallen-Key reconstruction filter droop, and 4-mode spatial stereo dimension matrix (Roland Dimension D style).
- **3rd-Order High-Order Ambisonics 16-Channel Spherical Harmonics Engine** (`JZHighOrderAmbisonics3rdOrderEngine`): 16-channel 3rd-order spatial soundfield encoding and decoding adhering to ACN channel ordering and SN3D normalization with real-time 3D Euler coordinate rotation (Yaw, Pitch, Roll) and energy-preserving max-rE binaural headphone decoding.

### 12. Apex Transcendence Nexus Suite (Suite v24.0)
- **Psychoacoustic 24-Band Dynamic Spectral Sidechain Unmasker** (`JZSpectralSidechainUnmaskerEngine`): Real-time Bark-scale dynamic spectral carving (smart:EQ / Soothe style) across 24 critical frequency bands (50 Hz to 13.5 kHz), attenuating conflicting frequencies in the main track using sidechain inputs without audible volume ducking.
- **Anthropometric Personalized Pinna & Ear Canal Modeler** (`JZPersonalizedPinnaAcousticModeler`): Precision personalized HRTF binaural headphone monitoring (Genelec Aural ID / Apple Spatial Audio style) with customizable head circumference (Woodworth ITD), ear canal 1/4-wave acoustic resonance (3.4 kHz boost), and pinna height notch elevation cues.
- **Historic Analog Tape Print-Through & High-Frequency Demagnetization Simulator** (`JZTapePrintThroughDemagSimulator`): Physical electro-magnetic simulation of reel-to-reel magnetic tape revolution print-through pre/post-echo timing (T = 2*pi*R/v at 7.5, 15, and 30 ips), inter-layer magnetic transfer, and cumulative multi-pass high-frequency demagnetization loss (-0.12 dB/pass).
- **Polyphonic Vocal Formant, Throat Length Scaling & Gender Morpher** (`JZPolyphonicVocalFormantMorpherEngine`): Formant resynthesis and vocal transformation (Antares Throat / Celemony Melodyne style) with acoustic vocal tract scaling (0.5x to 1.8x), 4-formant shifting (+/- 18 semitones), continuous gender morphing, and glottal aspiration noise.
- **True Bode Linear Frequency Shifter & Serge Non-Linear Wave Multiplier** (`JZBodeFrequencyShifterWaveMultiplierEngine`): Analog modular experimental sound design (Bode 1630 / Serge Modular style) featuring a 4-stage 90-degree Hilbert quadrature phase network for true linear shifting (+/- 5000 Hz) with Upper/Lower/Stereo-Split sidebands and non-linear diode wave folding.
- **16-Channel 9.1.6 Dolby Atmos Immersive Speaker Room Distance Alignment Engine** (`JZImmersiveSpeakerAlignmentEngine`): Multi-channel studio control room speaker calibration (Trinnov / Dolby DARDT style) across 16 discrete channels with sub-sample time-of-flight acoustic delay compensation (0.5 to 15.0 m), SPL calibration trim (-18 to +18 dB), 180-degree polarity inversion, and measurement chirp generation.

### 13. Apex Infinity Quantum Suite (Suite v23.0)
- **Psychoacoustic Missing Fundamental Bass Generator** (`JZPsychoacousticPhantomBassEngine`): Generates psychoacoustic bass overtones (Waves MaxxBass / Aphex Big Bottom style) enabling smartphone and earbud micro-speakers to perceive deep 30-80 Hz sub-bass through Chebyshev polynomial harmonic synthesis.
- **Quantum Gravitational Particle Granulator & Doppler Field** (`JZQuantumParticleGranularEngine`): 3D spatial particle synthesis (Curtis Roads / Xenakis style) driven by N-body gravitational attraction and Lorentz chaotic attractor orbits with spatial Doppler pitch shifting.
- **Adaptive Room Acoustic Spectral Dereverberation Engine** (`JZAdaptiveRoomDereverberationEngine`): Suppresses room acoustic reflections and late diffuse reverberation tails (Zynaptiq UNVEIL / Cedar De-Reverb style) up to -20 dB based on statistical Sabine/Eyring decay tracking.
- **Polyphonic Direct Note Access (DNA) Audio-to-MIDI Transcriber** (`JZPolyphonicNeuralAudioToMidiEngine`): Transcribes multi-note guitar chords and complex polyphonic piano recordings into discrete MIDI notes (Melodyne DNA style) with per-note MPE pitch bends and MusicXML score export.
- **Historic Optical Film Soundtrack Audio Restoration Suite** (`JZOpticalFilmAudioRestorationEngine`): Restores historic 35mm/16mm optical sound film recordings (1930s-1970s Variable-Area & Variable-Density) featuring scanning slit inverse sinc de-blurring, bilateral grain de-clicking, and 24Hz shutter notch filtering.
- **IEEE 1588v2 PTP Grandmaster Clock & SMPTE ST 2110-30 Synchronizer** (`JZPtpClockPrecisionSyncEngine`): Television broadcast network synchronization featuring a software Dual-Loop PLL locking to PTP Grandmaster Clock with sub-200ns accuracy and SMPTE ST 2110-30 / AES67 media clock alignment.

### 14. Apex Zenith Enterprise Suite (Suite v22.0)
- **Real-Time 3-Way Transient-Tonal-Ambience Spectral Decomposer** (`JZTransientTonalDecomposeEngine`): 3-way spectral decomposition separating fast attack transients, sustained harmonic bodies, and diffuse ambient noise beds (Cedar / SpectraLayers style) with dual-envelope tracking and surgical gain staging.
- **Jiles-Atherton Tape Magnetic Hysteresis & Azimuth Skew Engine** (`JZTapeHysteresisAzimuthEngine`): Physics-based ferromagnetic domain simulation (remanence, saturation, coercivity) with 7.5, 15, and 30 IPS head-bump modeling and automated sub-sample stereo tape head azimuth skew phase alignment.
- **Live Stage Dual-Rig Playback Redundancy & Failover Hub** (`JZLiveStagePlaybackRedundancyEngine`): Concert touring playback rig failover (Radial SW8 / PlayAUDIO12 style) featuring pilot heartbeat tone loss detection, clickless Rig B crossfade, and automated 16-channel MIDI panic flush (CC 120, CC 123, Pitch Bend Reset).
- **Dynamic Xenharmonic Lattice Morpher & Tonality Diamond** (`JZXenharmonicLatticeMorpherEngine`): Multidimensional Just Intonation lattices (Harry Partch 43-tone tonality diamond, Euler-Fokker genus) with continuous real-time morphing across 12-TET, Bohlen-Pierce (3:1 tritave), Arabic Maqam, and Gamelan Slendro.
- **ITU-R BS.1770-4 / EBU R128 S1 Speech-Gated Loudness Normalizer** (`JZDialogueGatedLoudnessEngine`): Cinema and OTT streaming broadcast dialogue loudness normalization (Netflix, Apple TV+, Dolby Vision compliance) with Voice Activity Detection (VAD) speech gating (-10 LU relative gate), K-weighting pre-filtering, and QC certificate generation.
- **Physical Helical Spring Reverb Modeler & Mechanical Chirp Dispersion** (`JZHelicalSpringDispersionEngine`): Accutronics 3-spring tank physical model featuring a 6-stage all-pass dispersive ladder simulating helical wire chirp dispersion, transducer saturation, and mechanical tank kick shock impacts.

### 15. Apex Singularity Flagship Suite (Suite v21.0)
- **Real-Time Neural Timbre Transfer & Resynthesis Engine** (`JZNeuralTimbreTransferEngine`): Low-latency neural instrument resynthesis and timbre transfer (RAVE / DDSP style) featuring zero-latency pitch and envelope tracking, 16-component differentiable harmonic sinusoidal bank, stochastic noise modeling, and instrument profiles (Stradivarius Violin, Tenor Saxophone, Concert Flute, Warm Cello).
- **Acoustic Guitar Sympathetic Coupling & Helmholtz Body Modeler** (`JZSympatheticGuitarResonanceEngine`): Physical modeling of acoustic guitar bridge coupling, sympathetic string vibrations, Helmholtz soundhole cavity resonance (102 Hz), spruce soundboard wood modes, and real-time capo acoustic impedance shifts (frets 1-7).
- **MPEG-H 3D Audio Interactive Broadcast Authoring Engine** (`JZMpegHInteractiveAudioEngine`): Next-Generation broadcast audio authoring adhering to ISO/IEC 23008-3 and ATSC 3.0 standards, featuring interactive audio elements, viewer accessibility dialogue gain limits (-6dB to +6dB), multi-language switching, and MPEG-H 3D XML metadata authoring.
- **Infinite Barberpole Flanger & Shepard-Risset Spectrum Engine** (`JZInfiniteBarberpoleFlangerEngine`): Continuous Shepard-Risset infinite flanging illusion with a 4-phase staggered delay comb filter bank, raised-cosine windowing, clickless bi-directional sweeps, and through-zero delay alignment for deep jet-plane phase notches.
- **Sub-Harmonic Waveguide Bass Synthesizer & Phase-Locked Octave Generator** (`JZSubHarmonicWaveguideBassEngine`): Low-frequency subharmonic synthesizer modeled after classic hardware units (dbx 120XP / Little Labs Voice of God), featuring zero-crossing phase-locked octave divider (-1 and -2 octaves), resonant acoustic waveguide delay line (20-90 Hz), soft saturation drive, and infrasonic protection filtering.
- **MADI (AES10) / Dante Live Stream Health & Jitter Telemetry Hub** (`JZLiveStreamHealthTelemetryEngine`): Broadcast stream telemetry and hitless dual-redundant stream switching (SMPTE ST 2022-7 style) featuring clock jitter tracking in PPM and microseconds, buffer slip detection, AES3/MADI parity and CRC error monitoring, and clickless crossfade failover between Primary A and Secondary B paths.

### 16. Apex Omniverse Jubilee Suite (Suite v20.0)
- **Dynamic Spectral Resonance Suppression & De-Bleeder Engine** (`JZDynamicResonanceSuppressorEngine`): Threshold-free dynamic resonance damping and harsh frequency taming (Oeksound Soothe 2 style) across 8 critical tracking bands (650Hz - 9.2kHz) with sharpness/selectivity Q controls and delta monitoring.
- **Mastering Vinyl Disc Cutting Lathe & Stylus Simulator** (`JZVinylLatheMechanicalSimulator`): Physical electro-mechanical modeling of vinyl cutting lathes (Neumann VMS-80 / SX-74) featuring 33-1/3 vs 45 RPM speeds, high-frequency stylus acceleration limiting, elliptical bass mono filter, vertical excursion lift-off guard, and Pinch Effect tracing distortion.
- **Bio-Mechanical Vocal Tract Aero-Acoustics Engine** (`JZBiomechanicalVocalTractEngine`): Sampleless physical human singing voice synthesis powered by a 2-mass vocal fold Bernoulli flow model, 8-section acoustic waveguide tube (pharynx, velum, oral cavity, lips radiation), and formant vowel articulation (/a/, /e/, /i/, /o/, /u/).
- **Stereo-to-7.1.4 Dolby Atmos & 3D Ambisonics Soundfield Up-Mixer** (`JZSpatialAmbisonicsUpMixerEngine`): Transforms 2-channel stereo audio into immersive 7.1.4 Dolby Atmos and 3D Ambisonics (B-format W, X, Y, Z) via direct-diffuse soundfield decomposition, phantom center dialogue anchoring, LFE subwoofer extraction (80 Hz), and decorrelated ceiling height channels.
- **Autonomous AI Multi-Track Mix Balancing & Masking Resolver** (`JZAutonomousMixAssistantEngine`): Multi-bus spectral monitoring detecting masking clashes (Kick vs Bass at 65Hz, Guitars vs Vocals at 2.4kHz) with automated dynamic sidechain EQ notch carving and fader balance leveling recommendations.
- **3D Wave-Terrain & Optical Graphic Synthesizer Engine** (`JZWaveTerrainGraphicSynthEngine`): 3D wave-terrain mathematical and optical synthesis (Daphne Oram Oramics & ANS Synthesizer style) featuring 2D/3D topographical surfaces (Radial Ripple Torus, Hyperbolic Saddle, Chaotic Attractor), orbital trajectories (Circular, Ellipse, Lissajous a:b, Spiral Galaxy), and direct 2D grayscale optical bitmap-to-audio wave generation.

### 17. Apex Transcendence Flagship Suite (Suite v19.0)
- **Non-Linear Volterra Kernel Dynamic Convolution Engine** (`JZVolterraDynamicConvolutionEngine`): Dynamic analog warmth and harmonic saturation (Acustica Audio style) using 1st-order linear, 2nd-order quadratic (even harmonics), and 3rd-order cubic (odd harmonics) Volterra series expansion with vintage tube, analog tape, and transformer models.
- **Multi-Microphone Phase Auto-Alignment & Comb Filter Recovery Engine** (`JZMultiMicPhaseAlignerEngine`): Eliminates acoustic phase cancellation across multi-microphone drum kits and multi-amp guitar setups via sub-sample cross-correlation, automated polarity inversion, and low-end bass power recovery (+3.5 dB).
- **Polyphonic Spectral Audio Morphing & Formant Cross-Synthesis Engine** (`JZSpectralAudioMorphEngine`): Cinematic spectral transmutation and cross-synthesis (Kyma / Zynaptiq Morph style) featuring optimal transport energy interpolation, formant envelope transplantation, and transient-tonal chimera hybridization.
- **Real-Time Adaptive Hermode Tuning & Just Intonation Engine** (`JZAdaptiveHermodeTuningEngine`): Dynamic polyphonic chord analysis retuning notes on the fly to eliminate acoustical beating (Hermode Tuning), producing pure Just Intonation intervals (Major 3rd -13.7c, 5th +2.0c) and supporting 19/31/53-EDO microtonality.
- **Binaural Virtual Studio Control Room Acoustic Simulator** (`JZVirtualStudioMonitorSimulator`): Reproduces iconic physical mixing control rooms inside headphones (Abbey Road Studio 3, Ocean Way Nashville, Mastering Lab Hollywood, Car Audio) with 60-degree cross-feed ITD/ILD delays, early room reflections, and NS-10M/Genelec/Soffit monitor models.
- **Psychoacoustic Audio Watermarking & Cryptographic Provenance Engine** (`JZAudioWatermarkProvenanceEngine`): Imperceptible Direct Sequence Spread Spectrum (DSSS) psychoacoustic watermark embedding with 32-bit PN-coded payloads, SHA-256 raw PCM audio hashing for tamper detection, and standardized ISRC/ISWC/BWF broadcast metadata packaging.

### 18. Apex Sovereign Ultimate Flagship Suite (Suite v18.0)
- **Ultra-High-Resolution DSD / DXD Audio Engine** (JZDsdDxdEngine): Audiophile SACD Direct Stream Digital format processing (DSD64 at 2.8224 MHz, DSD128 at 5.6448 MHz, DSD256 at 11.2896 MHz) featuring DoP v1.1 24-bit framing with 0x05/0xFA markers, multi-stage FIR decimation to 352.8 kHz 32-bit float DXD, 5th-order Delta-Sigma feedback modulation, and ultrasonic noise floor tracking.
- **Hollywood Film Scoring SMPTE Video Reference & PLL Genlock Sync Engine** (JZSmpteVideoSyncEngine): Frame-accurate cinema video synchronization across 23.976 fps, 24 fps theatrical, 25 fps PAL, 29.97 DF/NDF, 30 fps, and 59.94 DF UHD formats featuring SMPTE drop-frame arithmetic (17,982 frames per 10min), sub-frame PLL chase locking, freewheel drop-out recovery, and EDL scene-cut alignment.
- **Deep Learning Spectral 4-Stem Audio Demixing Engine** (JZNeuralStemSeparatorEngine): Demucs-style deep unmixing of stereo mixes into Vocals, Drums, Bass, and Other using multi-band soft-mask spectral decomposition, transient onset percussive tracking, mid-band vocal formant enhancement, and strict mathematical 1.0 energy conservation with zero residual loss (< 1e-4).
- **Hybrid Studio Hardware Outboard Ping Latency Compensator** (JZHardwareInsertDelayCompensator): Automatic round-trip analog latency calibration for outboard hardware (Neve, SSL, Pultec) using single-sample Dirac ping impulses, 3-point parabolic sub-sample peak interpolation, polarity flip detection, and master multi-track delay alignment buffers.
- **Acoustic Room Correction & SOFA Headphone Profile Engine** (JZRoomAcousticCalibrationEngine): Farina logarithmic sine sweep generation (20 Hz - 20,000 Hz) with room impulse response deconvolution, acoustic target curves (Flat Reference, Harman Studio Curve, B&K 1974, SMPTE 202M Cinema X-Curve), automated standing-wave resonant biquad filters, and SOFA binaural headphone correction.
- **Bjorklund Euclidean & Multilane Polymetric Generative Sequencer** (JZEuclideanPolyrhythmEngine): Algorithmic Euclidean rhythm generator E(k, n) (Tresillo, Cinquillo, etc.) coupled with concurrent multilane polymetric sequencing (independent step lengths per lane), probabilistic triggers, velocity jitter, sub-step ratcheting bursts, and micro-timing swing offset.

### 19. Apex Colossus Flagship Suite (Suite v17.0)
- **Real-Time Audio-over-IP & Timeline Delta-CRDT Collaboration** (`JZNetworkSessionEngine`): AES67 and WebRTC peer-to-peer low-latency streaming audio frames with jitter buffer packet-loss concealment (PLC) and conflict-free replicated data type (delta-CRDT) timeline synchronization.
- **Interactive Game Audio Middleware Bridge** (`JZGameAudioBridge`): Direct game audio integration (Wwise, FMOD Studio, Unreal Engine 5, Unity) featuring Real-Time Parameter Controls (RTPC), game state triggers, ray-traced acoustic obstruction/occlusion LPF (20kHz to 800Hz), and SoundBank XML packaging.
- **Studio Tape Machine Alignment & Fluxivity Engine** (`JZTapeMachineCalibrationEngine`): High-end mastering tape machine modeling (Studer A800, Ampex ATR-102, Otari MTR-90) with NAB and CCIR/IEC1 equalizations, elevated fluxivity levels (185 to 355 nWb/m), 1kHz 0 VU and 10kHz azimuth alignment tones, and magnetic hysteresis saturation.
- **Dolby Atmos ADM BWF ITU-R BS.2076-1 Metadata Engine** (`JZAdmBwfSpatialMetadataEngine`): Generates broadcast-compliant Audio Definition Model Broadcast Wave Format (ADM BWF) multi-channel packages with `<chna>` binary chunks, standardized `<axml>` schemas, 3D object coordinate automation, and binaural render mode metadata for Apple Music, Netflix, and Tidal.
- **AI Assistive Mastering & Spectral Crest Profiler** (`JZIntelligentMasteringAssistant`): Psychoacoustic mix analysis computing Integrated LUFS, Spectral Centroid, and Dynamic Crest Factor with one-click automated mastering chains tailored for Spotify (-14 LUFS), Apple Music (-16 LUFS), Vinyl (-18 LUFS with mono bass guard), and Cinema (-23 LUFS).
- **Studio Hardware Control Surface MCU / HUI Protocol Hub** (`JZControlSurfaceManager`): Bi-directional Mackie Control Universal (MCU) and Mackie HUI protocol integration with 10-bit motorized fader pitch-bend messages, 7-character LCD scribble strip SysEx formatting, V-Pot LED rings, and transport jog-wheel scrub tracking.

### 20. Apex Quantum Enterprise Suite (Suite v16.0)
- **Modern VST3 & CLAP Plugin Host Bridge with Out-of-Process Sandboxing** (`JZPluginHostBridge`): Full 3rd-party plugin support (CLAP and VST3) with isolated crash-resilient subprocess execution, lock-free parameter automation, and watchdog recovery.
- **Multithreaded DAG Audio Processing Graph Scheduler** (`JZAudioGraphScheduler`): Directed Acyclic Graph (DAG) task scheduling allocating audio processing nodes across 8 to 32 CPU cores with lock-free atomic queues, zero-buffer-underrun multi-core parallelization, and cycle detection.
- **Next-Generation MIDI 2.0 Universal MIDI Packet (UMP) Engine** (`JZMidi2UniversalPacketEngine`): 64-bit and 128-bit UMP packet formats, 16-bit note velocities, 32-bit CC resolution (4.29 billion steps) eliminating zipper noise, per-note pitch bend, and bi-directional MIDI-CI Property Exchange.
- **ARA2 (Audio Random Access 2) Non-Destructive Host Bridge** (`JZAra2HostBridge`): Direct, random-access sample and timeline communication with Celemony Melodyne, Synchro Arts VocALign, and Steinberg SpectraLayers without streaming capture.
- **Real-Time Spatial 6-DoF Head-Tracking Telemetry Engine** (`JZSpatialHeadTracker`): 360-degree head-tracking via Open Sound Control (OSC) and IMU Bluetooth sensors with quaternion sensor fusion, dynamically rotating 3D sound fields relative to listener position for binaural monitoring.
- **Embedded Neural Audio & Deep Learning DSP Engine** (`JZNeuralAudioEngine`): Local ONNX Runtime deep learning architecture supporting 4-stem deep neural unmixing (Demucs v4 style), accelerated GELU/ReLU activations, and Differentiable DSP (DDSP) harmonic resynthesis.

### 21. Apex Singularity Master Studio Pro (Suite v15.0)
- **Real-Time Transient / Tonal Spectral Audio Splitter** (`JZTransientTonalSplitter`): Separates complex mixdown signals into independent transient attack and tonal sustain streams with lossless, phase-coherent reconstruction.
- **Aliasing-Free True-Peak Brickwall Clipper & Oversampler** (`JZBrickwallClipperEngine`): Features 4x/8x linear-phase polyphase oversampling, inter-sample true-peak enforcement (-0.1 dBTP), and 4 analog saturation curves (Hard Brickwall, Soft-Knee Parabolic, Tube Diode, and Apogee Soft-Limit).
- **Generative Drum Poly-Fill & Pattern Morpher** (`JZGenerativeDrumMorpher`): Stochastically morphs rhythm patterns and generates automated polyrhythmic drum fills at phrase boundaries (Bars 4/8/16) across 4 variation styles with timing jitter and ghost notes.
- **Analog Tape Scrape Flutter & Mechanical Chaos Modeler** (`JZAnalogTapeFlutterModeler`): Recreates head-tape micro-friction scrape flutter (2.2kHz - 4.2kHz at 7.5/15/30 IPS), pinch-roller eccentricity (5.5Hz), asymmetric Brownian motor drift, and magnetic particle dropouts.
- **Immersive 3D Spatial Multichannel Reverb Convolver** (`JZImmersive3DReverbConvolver`): Computes 3D room boundary early reflections across 7.1.4 Dolby Atmos ceiling geometries (343 m/s) coupled with an 8-line orthogonal Feedback Delay Network (FDN) and Householder unitary mixing matrix.
- **Broadcast Audio Redundancy & Hot-Standby Failover** (`JZBroadcastFailoverEngine`): Ultra-fast fault detection (< 4ms for silence, buffer stall, NaN/Inf) with 2ms equal-power zero-click crossfading for tour concerts and live broadcast networks.

### 22. Aura Omniscience & Quantum Zenith Suites (Suites v13.0 – v14.0)
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

### 23. Flagship Enterprise & Next-Gen Production (Suites v11.0 – v12.0)
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

### 24. Core Production, Arranger & Mastering Architecture (Suites v1.0 – v10.0)
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
# Run all 136 unit tests via CTest
ctest --test-dir build --output-on-failure
```

```
Test project C:/Project/jazz/build
    Start 1: JazzUnitTest
1/1 Test #1: JazzUnitTest .....................   Passed    0.15 sec

100% tests passed, 0 tests failed out of 1
Total Test time (real) =   0.18 sec
```

All **136 test suites** pass with 100% accuracy, verifying sample-accurate DSP processing, numerical stability, and memory safety.

---

## Pre-Built Standalone Binaries

Official pre-built standalone binaries are generated automatically on every release:

- **Linux AppImage**: Download [`Jazz++-x86_64.AppImage`](https://github.com/rajazahidi/Jazzplusplus/releases/tag/v5.4.16), make executable with `chmod +x`, and run directly on Ubuntu 20.04+, Debian 11+, Fedora, Arch, or openSUSE.
- **macOS Native Bundle**: Download [`Jazz++-macOS-arm64.zip`](https://github.com/rajazahidi/Jazzplusplus/releases/tag/v5.4.16), extract `Jazz++.app`, and drag to your `/Applications` directory. Fully native on Apple Silicon (M1/M2/M3/M4) via CoreAudio and CoreMIDI.
- **Windows Portable**: Download [`Jazz++-Windows-x64.zip`](https://github.com/rajazahidi/Jazzplusplus/releases/tag/v5.4.16), extract to any folder, and double-click `jazz.exe`. All required 64-bit runtime libraries are included.

---

## Quick Build Instructions

For complete prerequisite commands and distro-specific instructions, see [**`INSTALL.md`**](INSTALL.md).

### 1. Linux (Ubuntu / Debian / Fedora / Arch)
```bash
# Install dependencies (Ubuntu/Debian)
sudo apt update && sudo apt install -y build-essential cmake libasound2-dev libwxgtk3.2-dev

# Configure and compile using CMake
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel $(nproc)

# Run application & execute test suite
./build/jazz
ctest --test-dir build --output-on-failure
```

### 2. macOS (Apple Silicon arm64 & Intel)
```bash
# Install dependencies via Homebrew
brew install cmake wxwidgets

# Configure and compile native application bundle
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel $(sysctl -n hw.ncpu)

# Run Jazz++.app & execute test suite
open build/jazz.app
ctest --test-dir build --output-on-failure
```

### 3. Windows (MSVC x64)
```powershell
# Configure with CMake (pointing to wxWidgets directory)
cmake -B build -DCMAKE_BUILD_TYPE=Release `
  -DwxWidgets_ROOT_DIR="D:/wxWidgets" `
  -DwxWidgets_LIB_DIR="D:/wxWidgets/lib/vc14x_x64_dll" `
  -DwxWidgets_CONFIGURATION=mswu

# Compile in Release mode
cmake --build build --config Release --parallel

# Run tests and executable
ctest --test-dir build -C Release --output-on-failure
.\build\Release\jazz.exe
```

---

## Automated CI/CD & Test Coverage

Jazz++ enforces enterprise-grade stability with an automated GitHub Actions CI matrix running on every push and pull request:
- **macOS (macos-14 Apple Silicon arm64)**: Native Apple Clang compile, CoreMIDI / CoreAudio link, 100% CTest pass, `.app` bundle zip packaging.
- **Linux (Ubuntu 24.04 LTS & 22.04 LTS)**: GCC 11/13 compile, ALSA kernel sequencing, 100% CTest pass.
- **Universal Linux AppImage**: Self-contained portable AppImage generation on Ubuntu 22.04.
- **Windows (MSVC x64)**: Visual Studio 2022 build, WASAPI driver integration, 100% CTest pass, standalone portable zip packaging.

---

## Directory Overview

- [`src/`](src/): Main C++ application source code, UI frames, dialogs, DSP audio engines, and MIDI drivers.
- [`conf/`](conf/): Instrument maps (`.jzi`), drum sets, and default configuration (`jazz.cfg`).
- [`song/`](song/): Demo songs (`.mid`), rhythm templates (`.rhy`), and sample audio files (`.wav`).
- [`portmidi/`](portmidi/): Embedded PortMidi library source code supporting macOS CoreMIDI and cross-platform MIDI.
- [`tests/`](tests/): Automated CTest unit test suite covering over 550 test assertions across all 35 studio suites.
- [`resources/`](resources/): Vector SVG and multi-resolution PNG application icons.
- [`scripts/`](scripts/): Packaging and launcher helper scripts (`build-appimage.sh`, `jazz-launcher.sh`).

---

## License

Jazz++ is released under the **GNU General Public License v2 (GPL-2.0)**. See [`COPYING`](COPYING) for details.
