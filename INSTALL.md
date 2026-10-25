# Installation & Build Guide

Welcome to the installation and build guide for **Jazz++ MIDI & Digital Audio Sequencer** (Version 6.0.0).

---

## Table of Contents

1. [Supported Platforms](#supported-platforms)
2. [Quick Start (Pre-Built Binaries)](#quick-start-pre-built-binaries)
3. [Building from Source](#building-from-source)
   - [Linux (Ubuntu / Debian / Fedora / Arch)](#1-linux)
   - [macOS (Apple Silicon & Intel)](#2-macos)
   - [Windows (10 / 11 64-bit)](#3-windows)
4. [Running Unit Tests](#running-unit-tests)
5. [System-Wide Installation](#system-wide-installation)
6. [Audio & MIDI Device Setup](#audio--midi-device-setup)
7. [Troubleshooting & FAQ](#troubleshooting--faq)

---

## Supported Platforms

| Platform | Supported Architectures | Minimum OS Version | Recommended Audio/MIDI Backend |
| :--- | :--- | :--- | :--- |
| **Linux** | `x86_64` | Ubuntu 22.04 LTS / Debian 11+ | ALSA Sequencer (`snd_seq`) & ALSA PCM |
| **macOS** | `arm64` (Apple Silicon), `x86_64` | macOS 14 Sonoma (macOS 13+) | Apple CoreAudio & CoreMIDI |
| **Windows** | `x64` | Windows 10 (Build 19041+) / 11 | Windows WASAPI & MMSystem |

---

## Quick Start (Pre-Built Binaries)

Official pre-built standalone releases are generated on every release by our continuous integration pipeline:

- **Linux Universal AppImage**:
  ```bash
  # Download and make executable
  chmod +x Jazz++-x86_64.AppImage
  ./Jazz++-x86_64.AppImage
  ```

- **macOS Bundle**:
  1. Download `Jazz++-macOS-arm64.zip`.
  2. Double-click to extract the `Jazz++.app` bundle.
  3. Drag `Jazz++.app` into your `/Applications` directory.
  4. Launch Jazz++ from Finder, Spotlight, or Launchpad.

- **Windows Portable ZIP**:
  1. Download `Jazz++-Windows-x64.zip`.
  2. Extract to your preferred location (e.g. `C:\Program Files\Jazz++` or desktop).
  3. Run `jazz.exe`. All required 64-bit DLLs and configuration files are self-contained.

---

## Building from Source

### Prerequisites

- **CMake**: Version 3.16 or higher.
- **C++ Compiler**: A modern C++17 compliant compiler:
  - GCC 9+ or Clang 10+ (Linux)
  - Apple Clang from Xcode 15+ (macOS)
  - Microsoft Visual Studio 2019 / 2022 with MSVC v142/v143 (Windows)
- **wxWidgets**: Version 3.0, 3.2, or 3.3.

---

### 1. Linux

#### Install Dependencies

**Debian / Ubuntu 22.04 & 24.04:**
```bash
sudo apt update
sudo apt install -y build-essential cmake libasound2-dev libwxgtk3.2-dev
# If libwxgtk3.2-dev is not available (Ubuntu 20.04/22.04), install:
# sudo apt install -y libwxgtk3.0-gtk3-dev
```

**Fedora / RHEL / CentOS Stream:**
```bash
sudo dnf install -y gcc-c++ cmake alsa-lib-devel wxGTK3-devel
```

**Arch Linux / Manjaro:**
```bash
sudo pacman -S --needed base-devel cmake alsa-lib wxwidgets-gtk3
```

#### Build Instructions

```bash
# 1. Clone the repository
git clone https://github.com/rajazahidi/Jazzplusplus.git
cd Jazzplusplus

# 2. Configure with CMake
cmake -B build -DCMAKE_BUILD_TYPE=Release

# 3. Compile using all CPU cores
cmake --build build --parallel $(nproc)

# 4. Launch Jazz++
./build/jazz
```

---

### 2. macOS

Jazz++ features native macOS integration supporting **Apple Silicon (M1/M2/M3/M4)** and **Intel** architectures using Apple CoreAudio and CoreMIDI via PortMidi.

#### Install Dependencies via Homebrew

Ensure you have [Homebrew](https://brew.sh/) installed:
```bash
# Install Xcode Command Line Tools
xcode-select --install

# Install CMake and wxWidgets
brew install cmake wxwidgets
```

#### Build Instructions

```bash
# 1. Clone the repository
git clone https://github.com/rajazahidi/Jazzplusplus.git
cd Jazzplusplus

# 2. Configure with CMake
cmake -B build -DCMAKE_BUILD_TYPE=Release

# 3. Build native Jazz++.app bundle and test suite
cmake --build build --parallel $(sysctl -n hw.ncpu)

# 4. Run the macOS application bundle
open build/jazz.app
```

---

### 3. Windows

#### Option A: Pre-Compiled wxWidgets (Recommended)

1. Download **wxWidgets 3.2+** Development and Header 7z archives from [wxWidgets Releases](https://github.com/wxWidgets/wxWidgets/releases).
2. Extract to a directory such as `D:\wxWidgets`.
3. Open **Developer Command Prompt for VS 2022** or **PowerShell**:

```powershell
# 1. Clone the repository
git clone https://github.com/rajazahidi/Jazzplusplus.git
cd Jazzplusplus

# 2. Configure with CMake
cmake -B build `
  -DCMAKE_BUILD_TYPE=Release `
  -DwxWidgets_ROOT_DIR="D:/wxWidgets" `
  -DwxWidgets_LIB_DIR="D:/wxWidgets/lib/vc14x_x64_dll" `
  -DwxWidgets_CONFIGURATION=mswu

# 3. Build the Release target
cmake --build build --config Release --parallel

# 4. Run Jazz++
.\build\Release\jazz.exe
```

---

## Running Unit Tests

Jazz++ includes a comprehensive automated DSP and core sequencing test suite (over 550 test assertions covering all 35 studio suites and audio engines).

To execute the test suite:

**Linux / macOS:**
```bash
ctest --test-dir build --output-on-failure
```

**Windows:**
```powershell
ctest --test-dir build -C Release --output-on-failure
```

---

## System-Wide Installation

To install Jazz++ globally on Linux systems:

```bash
sudo cmake --install build
```

This installs:
- Executable: `/usr/local/bin/jazz`
- Desktop Entry: `/usr/local/share/applications/jazz.desktop`
- Application Icons: `/usr/local/share/icons/hicolor/.../jazz.png`
- Configuration & Synthesizer Maps: `/usr/local/share/jazz/conf/`
- Song Templates & Audio Assets: `/usr/local/share/jazz/song/`
- Help Files: `/usr/local/share/jazz/HelpFiles/`

---

## Audio & MIDI Device Setup

### Linux (ALSA & SoundFonts)
- Ensure your user belongs to the `audio` group:
  ```bash
  sudo usermod -aG audio $USER
  ```
- For software sound synthesis, install a SoundFont player such as FluidSynth:
  ```bash
  sudo apt install fluidsynth fluid-soundfont-gm
  fluidsynth -a alsa -m alsa_seq -l -i /usr/share/sounds/sf2/FluidR3_GM.sf2 &
  ```

### macOS (CoreMIDI & IAC Bus)
- Open **Audio MIDI Setup** (`/Applications/Utilities/Audio MIDI Setup.app`).
- Choose **Window > Show MIDI Studio**.
- Double-click the **IAC Driver** and check **Device is online** to create internal MIDI routing ports for DAWs and soft-synths.

### Windows (WASAPI & Microsoft GS Synth)
- Jazz++ automatically connects to the default Windows **Microsoft GS Wavetable Synth**.
- For low-latency virtual MIDI routing between applications, install [loopMIDI](https://www.tobias-erichsen.de/software/loopmidi.html).

---

## Troubleshooting & FAQ

**Q: CMake reports `Could NOT find wxWidgets` on Linux.**  
A: Ensure you have installed the GTK3 development package (`libwxgtk3.2-dev` or `libwxgtk3.0-gtk3-dev`).

**Q: Jazz++ starts but no sound is heard on Linux.**  
A: Check `Settings > MIDI Device...` and ensure an active ALSA sequencer port (such as FluidSynth, TiMidity++, or a hardware MIDI interface) is selected.

**Q: macOS displays "App cannot be opened because it is from an unidentified developer".**  
A: Go to **System Settings > Privacy & Security**, scroll down, and click **Open Anyway**.

---

For additional questions and community discussions, visit the [Jazz++ Project Repository](https://github.com/rajazahidi/Jazzplusplus).
