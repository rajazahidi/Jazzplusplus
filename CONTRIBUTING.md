# Contributing to Jazz++

Thank you for your interest in contributing to **Jazz++ MIDI & Digital Audio Sequencer**! We welcome contributions from developers, audio engineers, sound designers, and documentation writers.

---

## Code of Conduct

We are committed to providing a friendly, safe, and welcoming environment for all contributors regardless of background, identity, or experience level. Please treat everyone with respect and empathy in all project spaces (issues, discussions, and pull requests).

---

## How Can I Contribute?

### 1. Reporting Bugs

Before reporting a bug, please search the [GitHub Issues](https://github.com/rajazahidi/Jazzplusplus/issues) tab to see if the issue has already been reported.

If creating a new bug report, please include:
- **Operating System & Version**: (e.g. Ubuntu 24.04 LTS, macOS 14.5 Sonoma arm64, Windows 11 x64).
- **Jazz++ Version**: (e.g. 5.4.16 or specific git commit hash).
- **Audio & MIDI Hardware**: (e.g. Focusrite Scarlett 2i2, ALSA snd_seq, CoreMIDI, WASAPI).
- **Steps to Reproduce**: Clear step-by-step instructions.
- **Expected vs Actual Behavior**.
- **Log Outputs & Crash Stacks**: Terminal output or debugger backtraces where available.

### 2. Suggesting Enhancements

Feature requests and enhancement ideas are encouraged! When submitting a feature request, please describe:
- The musical or engineering problem you are trying to solve.
- Your proposed solution or workflow improvement.
- Any alternative solutions or industry standards considered (e.g. VST3, CLAP, MIDI 2.0, ARA2).

### 3. Submitting Pull Requests

1. **Fork the Repository** on GitHub.
2. **Create a Feature Branch**:
   ```bash
   git checkout -b feat/your-feature-name
   ```
3. **Follow Coding Standards** (see below).
4. **Ensure All Unit Tests Pass**:
   ```bash
   ctest --test-dir build --output-on-failure
   ```
5. **Commit with Clear Messages**: Follow conventional commits (e.g., `feat(dsp): Add dynamic oversampling filter`, `fix(ui): Correct piano roll note snap`).
6. **Push and Open a Pull Request**: Provide a concise summary of the changes and link to any relevant issues.

---

## Coding Standards & Guidelines

### 1. Modern C++17
- The codebase uses **C++17** (`-std=c++17`). Use modern standard library features where appropriate (`std::optional`, `std::string_view`, structured bindings, `constexpr`).
- Prefer standard containers (`std::vector`, `std::array`, `std::unordered_map`) and smart pointers (`std::unique_ptr`, `std::shared_ptr`).
- Follow RAII (Resource Acquisition Is Initialization) for resource management.

### 2. Real-Time Audio Constraints
- **Never allocate memory on the heap** (e.g. `malloc`, `new`, resizing `std::vector`) inside real-time audio callback threads or DSP `process()` methods.
- **Never acquire non-real-time mutexes or locks** on the audio thread. Use lock-free ringbuffers (`JZLockFreeAudioBufferPoolAllocator`) or atomic variables (`std::atomic<T>`).
- Avoid file I/O, network calls, or logging from within DSP processing loops.

### 3. Cross-Platform Parity
- Ensure your code compiles and functions seamlessly on:
  - **Linux** (GCC / Clang + ALSA)
  - **Windows** (MSVC + WASAPI)
  - **macOS** (Apple Clang + CoreMIDI / CoreAudio)
- Use standard `<cstdint>` fixed-width types (`int32_t`, `uint32_t`, `float`, `double`).
- When working with CoreMIDI on macOS, note that CoreMIDI endpoint/client handles are unsigned integers (`UInt32`), not pointer types.

### 4. Unit Testing
- New DSP algorithms, parsers, and utility functions must include unit tests in [`tests/test_main.cpp`](tests/test_main.cpp).
- Tests must pass deterministically across all operating systems and architectures.

---

## License

By contributing to Jazz++, you agree that your contributions will be licensed under the **GNU General Public License v2 (GPL-2.0)** as published by the Free Software Foundation.
