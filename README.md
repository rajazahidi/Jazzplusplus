# Jazz++ MIDI & Audio Workstation

[![GitHub Pages](https://img.shields.io/badge/GitHub%20Pages-Live%20Site-00d2ff?logo=github)](https://rajazahidi.github.io/Jazzplusplus/)
[![Latest Release](https://img.shields.io/github/v/release/rajazahidi/Jazzplusplus?color=10b981)](https://github.com/rajazahidi/Jazzplusplus/releases/tag/v6.0.0)
[![CI Build](https://github.com/rajazahidi/Jazzplusplus/actions/workflows/ci.yml/badge.svg)](https://github.com/rajazahidi/Jazzplusplus/actions/workflows/ci.yml)
[![License: GPL-2.0](https://img.shields.io/badge/License-GPL_v2-blue.svg)](COPYING)

**Jazz++** is a full-featured open-source MIDI sequencer and digital audio recording workstation for Linux, Windows, and macOS. Modernized from the ground up, Jazz++ is now powered exclusively by **JUCE 8** and modern **C++20**, featuring native **VST3 & CLAP plugin hosting**, high-DPI vector graphics, low-latency audio/MIDI device management, procedural DSP synthesis, and advanced composition tools.

Visit the official website: **[https://rajazahidi.github.io/Jazzplusplus/](https://rajazahidi.github.io/Jazzplusplus/)**

Originally created by Andreas Voss and Per Sigmond, with subsequent development by Patrick Earl and Peter J. Stieber. Modernized and rewritten by **Raja Zahidi** ([rajazahidi@aol.com](mailto:rajazahidi@aol.com)).

---

## Key Features

### 🔌 VST3 & CLAP Audio Plugin Hosting
- **Dual Format Hosting**: Host both 64-bit **VST3** (via JUCE 8 plugin format manager) and **CLAP** (CLever Audio Plugin, via custom wrapper integrating official CLAP C API headers) instruments and effects.
- **Integrated Scanner & Browser**: Auto-scans standard system and user plugin directories (`/usr/lib/vst3`, `/usr/lib/clap`, `~/.vst3`, `~/.clap`) with live search, format filters, and arbitrary file loading.
- **Active Plugin Racks**: Instrument slot with live MIDI note forwarding, master insert effect slot with bypass toggles, floating native/generic UI windows, and state save/load.

### 🎨 Modern Studio Suite
- **Multi-Track Arranger Timeline**: Interactive measure ruler with subdivisions, track lanes with mute/solo/arm, draggable clips with live note previews, and clip-to-editor navigation.
- **High-DPI Piano Roll**: Note manipulation snapped to grid, interactive vertical piano keyboard with auditioning, musical scale degree highlighting, and velocity lane.
- **808 Vintage Drum Machine**: 16-step sequencer matrix for 8 analog drum voices, pattern presets (Trap, House, Techno, Chiptune), and live velocity-style MPC trigger pads.
- **Interactive 24-Fret Fretboard**: Guitar and bass fretboard with customizable tunings, authentic pearl inlay markers, scale visualization, and click-to-play auditioning.
- **5-Module Studio DSP FX Rack**: Hardware-style rack units featuring 3-Band Parametric EQ, Freeverb Reverb, Stereo Ping-Pong Delay, Stereo Chorus, and Tube Limiter / Overdrive.
- **Low-Latency Audio & MIDI Engine**: Native CoreAudio/CoreMIDI on macOS, ALSA on Linux, DirectSound/WASAPI on Windows.

---

## Quick Start: Pre-built Binaries

Pre-compiled standalone packages for Linux, Windows, and macOS are available from the [GitHub Releases](https://github.com/rajazahidi/Jazzplusplus/releases) page or [GitHub Actions Artifacts](https://github.com/rajazahidi/Jazzplusplus/actions).

### Linux (AppImage)
1. Download **`Jazz++-x86_64.AppImage`**.
2. Make it executable and run:
   ```bash
   chmod +x Jazz++-x86_64.AppImage
   ./Jazz++-x86_64.AppImage
   ```

### Windows (64-bit ZIP)
1. Download **`Jazz++-Windows-x64.zip`**.
2. Extract the archive and launch **`jazz.exe`**.

### macOS
1. Download **`Jazz++-macOS-macos-14.zip`** (macOS 14 Sonoma) or **`Jazz++-macOS-macos-15.zip`** (macOS 15 Sequoia).
2. Extract the archive and run **`Jazz++.app`**.

---

## System Requirements & Prerequisites

### Linux (Debian, Ubuntu, Linux Mint, Zorin OS)
```bash
sudo apt update
sudo apt install -y build-essential cmake libasound2-dev libgl1-mesa-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libfreetype-dev libfontconfig1-dev
```

### Windows
- Visual Studio 2022 / Build Tools (C++20 support)
- CMake 3.16+

### macOS
- Xcode / Command Line Tools (Apple Clang with C++20 support)
- CMake 3.16+

---

## Building Jazz++ from Source

```bash
# Configure project
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Compile Jazz++
cmake --build build --parallel $(nproc)

# Run full workstation
./build/jazz

# Or launch directly into a specific studio view
./build/jazz --view=plugin       # VST3 & CLAP Plugin Host
./build/jazz --view=arranger     # Multi-Track Arranger
./build/jazz --view=pianoroll    # High-DPI Piano Roll
./build/jazz --view=drum         # 808 Drum Machine
./build/jazz --view=fretboard    # 24-Fret Fretboard
./build/jazz --view=fx           # Audio FX Rack

# Run unit tests
ctest --test-dir build --output-on-failure
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

- [`src/juce/`](src/juce/): Modern JUCE 8 application code, studio views, audio/MIDI engine, LookAndFeel, and CLAP plugin format.
- [`src/`](src/): Shared DSP synthesizers, audio effects, MIDI effects, and chord/scale libraries.
- [`conf/`](conf/): Instrument maps (`.jzi`), drum sets, and default configuration (`jazz.cfg`).
- [`song/`](song/): Demo songs (`.mid`), rhythm templates (`.rhy`), and sample audio (`.wav`).
- [`tests/`](tests/): Comprehensive unit test suite.
- [`libs/clap/`](libs/clap/): CLAP audio plugin C API headers.
- [`scripts/`](scripts/): Packaging scripts and smart audio/synth launcher.
