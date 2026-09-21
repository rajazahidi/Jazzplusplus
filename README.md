# Jazz++ MIDI & Audio Sequencer

[![GitHub Pages](https://img.shields.io/badge/GitHub%20Pages-Live%20Site-00d2ff?logo=github)](https://rajazahidi.github.io/Jazzplusplus/)
[![Latest Release](https://img.shields.io/github/v/release/rajazahidi/Jazzplusplus?color=10b981)](https://github.com/rajazahidi/Jazzplusplus/releases/tag/v5.4.16)
[![CI Build](https://github.com/rajazahidi/Jazzplusplus/actions/workflows/ci.yml/badge.svg)](https://github.com/rajazahidi/Jazzplusplus/actions/workflows/ci.yml)
[![License: GPL-2.0](https://img.shields.io/badge/License-GPL_v2-blue.svg)](COPYING)

**Jazz++** is a full-featured open-source MIDI sequencer and digital audio recording workstation for Linux and Windows. It offers two application editions designed to coexist seamlessly in the same repository:

1. **Modern JUCE 8 Edition (`jazz-juce`)**: Next-generation workstation powered by **JUCE 8** and modern **C++20**, featuring native **VST3 & CLAP plugin hosting**, high-DPI vector graphics, non-blocking multi-threaded audio/MIDI engine, and modern studio tools.
2. **Classic wxWidgets Edition (`jazz`)**: The battle-tested, lightweight desktop sequencer powered by **wxWidgets 3.2** and native ALSA (`snd_seq`) / WinMM drivers.

Visit the official website: **[https://rajazahidi.github.io/Jazzplusplus/](https://rajazahidi.github.io/Jazzplusplus/)**

Originally created by Andreas Voss and Per Sigmond, with subsequent development by Patrick Earl and Peter J. Stieber. Maintained, modernized, and expanded by **Raja Zahidi** ([rajazahidi@aol.com](mailto:rajazahidi@aol.com)).

---

## Key Features

### 🔌 VST3 & CLAP Audio Plugin Hosting (`jazz-juce`)
- **Dual Format Support**: Host both **VST3** (via JUCE 8 format manager) and **CLAP** (CLever Audio Plugin, via custom wrapper integrating official CLAP C API headers) instruments and effects.
- **Plugin Scanner & Browser**: Auto-scans standard system and user plugin directories (`/usr/lib/vst3`, `/usr/lib/clap`, `~/.vst3`, `~/.clap`) with live search, format filters, and arbitrary file loading.
- **Active Plugin Racks**: Instrument slot with live MIDI note forwarding and Master insert effect slot with bypass toggles, floating native/generic UI windows, and preset management.

### 🎨 Modern Studio Suite (`jazz-juce`)
- **Multi-Track Arranger Timeline**: Interactive measure ruler with subdivisions, track lanes with mute/solo/arm, draggable clips with live note previews, and clip-to-editor navigation.
- **High-DPI Piano Roll**: Note manipulation snapped to grid, interactive vertical piano keyboard with auditioning, musical scale degree highlighting, and velocity lane.
- **808 Vintage Drum Machine**: 16-step sequencer matrix for 8 analog drum voices, pattern presets (Trap, House, Techno, Chiptune), and live velocity-style MPC trigger pads.
- **Interactive 24-Fret Fretboard**: Guitar and bass fretboard with customizable tunings, authentic pearl inlay markers, scale visualization, and click-to-play auditioning.
- **5-Module Studio DSP FX Rack**: Hardware-style rack units featuring 3-Band Parametric EQ, Freeverb Reverb, Stereo Ping-Pong Delay, Stereo Chorus, and Tube Limiter / Overdrive.

### 🎹 Classic Sequencer Core (`jazz`)
- **Multi-Track Sequencer**: Track timeline editor with mute, solo, track volume, pan, and instrument assignment.
- **Piano Roll & Event List**: Note grid and raw MIDI event viewer/editor supporting SysEx, control changes, and tempo maps.
- **ALSA & PortMidi Backend**: Non-blocking ALSA sequencer (`snd_seq`) with kernel queue synchronization and automatic synthesizer discovery (FluidSynth, TiMidity).
- **Procedural Sound FX & 808 Synth**: Built-in retro sound generator and 808 percussion synthesizer with WAV export and project insertion.
- **Synthesizer Definitions**: Preset configurations (`.jzi`) for General MIDI (GM), Roland GS, Roland TD-20, Yamaha XG, and E-mu Proteus.

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
2. Extract the archive and launch **`jazz.exe`**. All required wxWidgets 3.2.6 runtime DLLs are included.

---

## System Requirements & Prerequisites

On Debian, Ubuntu, Linux Mint, or Zorin OS:

```bash
sudo apt update
sudo apt install build-essential cmake libwxgtk3.2-dev libasound2-dev libgl1-mesa-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev
```

*(For older Ubuntu releases, `libwxgtk3.0-gtk3-dev` can be used).*

---

## Building Jazz++

### 1. Build Modern JUCE 8 Edition (`jazz-juce`)

```bash
# Configure with JUCE 8 enabled
cmake -B build -DBUILD_JAZZ_JUCE=ON

# Compile modern JUCE 8 application
cmake --build build --target jazz-juce -j$(nproc)

# Run full workstation
./build/jazz-juce

# Or launch directly into a specific studio view
./build/jazz-juce --view=plugin       # VST3 & CLAP Plugin Host
./build/jazz-juce --view=arranger     # Multi-Track Arranger
./build/jazz-juce --view=pianoroll    # High-DPI Piano Roll
./build/jazz-juce --view=drum         # 808 Drum Machine
./build/jazz-juce --view=fretboard    # 24-Fret Fretboard
./build/jazz-juce --view=fx           # Audio FX Rack
```

### 2. Build Classic wxWidgets Edition (`jazz`)

```bash
# Configure and compile
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target jazz -j$(nproc)

# Run classic application
./build/jazz
```

### 3. Run Core Unit Tests

```bash
cmake --build build --target jazz_tests
ctest --test-dir build --output-on-failure
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

---

## License

Jazz++ is released under the **GNU General Public License v2 (GPL-2.0)**. See [`COPYING`](COPYING) for details.
