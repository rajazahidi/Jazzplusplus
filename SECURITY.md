# Security Policy

The Jazz++ development team takes software security, memory safety, and stability very seriously. This document outlines our vulnerability disclosure process and security architecture.

---

## Supported Versions

Security updates, bug fixes, and stability patches are applied to the active release branch:

| Version | Supported | Status |
| :--- | :---: | :--- |
| **5.4.x** (v5.4.16) | :white_check_mark: | Current Stable Release (Actively Maintained) |
| < 5.4.0 | :x: | End of Life (Upgrade Recommended) |

---

## Reporting a Vulnerability

If you discover a security vulnerability or potential exploit in Jazz++, please report it responsibly:

1. **Email Contact**: Send details to the maintainer at [rajazahidi@aol.com](mailto:rajazahidi@aol.com) with the subject line `[SECURITY] Jazz++ Vulnerability Report`.
2. **Details to Include**:
   - Description of the vulnerability.
   - Operating system and environment details.
   - Proof of Concept (PoC) file or reproduction steps (e.g. malformed MIDI file, crafted RIFF/WAV chunk, or plugin buffer overflow).
   - Any proposed remediation or patch.
3. **Response Window**: You will receive an initial response within **48 hours**. We will coordinate a fix and release a security advisory before public disclosure.

---

## Security Architecture & Defenses

Jazz++ incorporates several defense-in-depth mechanisms to protect user systems:

### 1. Out-of-Process Plugin Sandboxing (`JZPluginCrashSandboxEngine`)
- Third-party VST3 and CLAP binary plugins can run in isolated worker child processes.
- Memory corruption, null pointer exceptions, or crashes within untrusted plugin code cannot bring down the main DAW host process or compromise user projects.
- Shared-memory IPC ringbuffers operate with strict boundary validation.

### 2. Robust Audio & Metadata Parser Hardening
- RIFF, Broadcast Wave Format (BWF), iXML, and Standard MIDI File (SMF) parsers enforce strict length validation to prevent integer overflow and heap buffer overruns from crafted media files.
- SysEx (System Exclusive) buffers are bounded with maximum message limits.

### 3. Lock-Free Audio Memory Safety
- Real-time DSP routines utilize pre-allocated slab buffer pools (`JZLockFreeAudioBufferPoolAllocator`) to eliminate thread contention and memory race conditions.
