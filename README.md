# Jazz++ MIDI & Audio Sequencer

[![CI Build](https://github.com/rajazahidi/Jazzplusplus/actions/workflows/ci.yml/badge.svg)](https://github.com/rajazahidi/Jazzplusplus/actions/workflows/ci.yml)
[![License: GPL-2.0](https://img.shields.io/badge/License-GPL_v2-blue.svg)](COPYING)

**Jazz++** is a full-featured open-source MIDI sequencer and digital audio recording tool for Linux and Windows. Built using **wxWidgets**, it provides a rich visual environment for composing, editing, arranging, and recording multi-track MIDI music and digital audio.

Originally created by Andreas Voss and Per Sigmond, with subsequent major development by Patrick Earl and Peter J. Stieber.

---

## Features

- **Multi-Track Sequencer**: Track timeline editor with mute, solo, track volume, pan, and instrument assignment.
- **Piano Roll Editor**: Note grid editor with velocity bars, pitch bend, and real-time step editing.
- **Event List Editor**: Raw MIDI event viewer and editor supporting SysEx, control changes, and tempo maps.
- **Guitar Fretboard & Tab**: Visual fretboard display for guitar and tablature-oriented composition.
- **Harmony & Rhythm Generator**: Harmony analyzer, chord progressions, groove quantization, and shuffle algorithms.
- **Digital Audio Mixing**: Audio track support with WAV sample playback and recording.
- **ALSA & PortMidi Integration**: Native Linux ALSA sequencer (`snd_seq`) and PCM audio (`snd_pcm`) backend.
- **Synthesizer Definitions**: Preset configurations (`.jzi`) for General MIDI (GM), Roland GS, Roland TD-20, Yamaha XG, and E-mu Proteus.

---

## Quick Start: Download AppImage

The easiest way to run Jazz++ on any modern Linux distribution (Ubuntu, Debian, Fedora, Arch, Zorin OS, Linux Mint, etc.) without installing compilers or development packages:

1. Download **`Jazz++-x86_64.AppImage`** from the [GitHub Releases](https://github.com/rajazahidi/Jazzplusplus/releases) page or [GitHub Actions Artifacts](https://github.com/rajazahidi/Jazzplusplus/actions).
2. Make it executable and run:
   ```bash
   chmod +x Jazz++-x86_64.AppImage
   ./Jazz++-x86_64.AppImage
   ```

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

---

## License

Jazz++ is released under the **GNU General Public License v2 (GPL-2.0)**. See [`COPYING`](COPYING) for details.
