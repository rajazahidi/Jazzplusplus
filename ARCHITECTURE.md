# Jazz++ Architecture & System Design

This document details the software architecture, modular subsystems, real-time audio pipeline, and concurrency model of the **Jazz++ MIDI & Digital Audio Workstation**.

---

## 1. High-Level Architectural Overview

Jazz++ is architected as a modular, decoupled digital audio workstation with clear separation between the musical sequencing model, real-time digital signal processing (DSP), hardware abstraction layer, and the graphical user interface.

```
+--------------------------------------------------------------------------------+
|                        Graphical User Interface (GUI)                          |
|  Track Timeline  |  Piano Roll  |  Event List  |  Guitar Tab  |  Mixer Console  |
+--------------------------------------------------------------------------------+
                                      |
                                      v
+--------------------------------------------------------------------------------+
|                         Sequencer & Session Core                               |
|       Song Model (JZSong)  |  Tracks & Events  |  Tempo Maps  |  Comping       |
+--------------------------------------------------------------------------------+
                                      |
          +---------------------------+---------------------------+
          |                                                       |
          v                                                       v
+------------------------------------+  +------------------------------------+
|         MIDI Subsystem             |  |      Real-Time Audio & DSP         |
| - MIDI 1.0 & MIDI 2.0 (UMP) Engine |  | - Directed Acyclic Graph (DAG) ADC |
| - Polyphonic Micro-Tuning (Hermode)|  | - 35 Apex Master Studio DSP Suites |
| - Humanizer, Arpeggiator & Groove  |  | - Spatial Audio (7.1.4 / 9.1.6 HOA)|
| - MPE (MIDI Polyphonic Expression) |  | - Neural Profiling & Stem Isolation|
+------------------------------------+  +------------------------------------+
          |                                                       |
          v                                                       v
+--------------------------------------------------------------------------------+
|                          Hardware Abstraction Layer                            |
|    Linux: ALSA (`snd_seq` / PCM)   |   macOS: CoreMIDI / CoreAudio (PortMidi)  |
|                  Windows: WASAPI / DirectSound / MMSystem                      |
+--------------------------------------------------------------------------------+
```

---

## 2. Core Subsystems

### 2.1 Sequencer & Event Engine (`JZSong`, `JZTrack`, `JZEvent`)
- **Song Structure**: `JZSong` encapsulates global project parameters, tempo/time signature maps, marker lists, and an ordered collection of audio and MIDI tracks (`JZTrack`).
- **Event Representation**: Discrete musical events (`JZKeyOnEvent`, `JZKeyOffEvent`, `JZControlEvent`, `JZSysExEvent`, `JZTempoEvent`) are indexed along a sample-accurate clock timeline based on PPQN (Pulses Per Quarter Note).
- **Non-Destructive Editing**: Quantization, groove shuffle, humanization, and transpositions are evaluated with reversible transformations, preserving original musical performances.

### 2.2 Directed Acyclic Graph (DAG) Audio Scheduler & ADC (`JZComplexSidechainAdcGraphResolver`)
- Models multi-track routing, aux sends, submix busses, and cross-track sidechain dependencies as a Directed Acyclic Graph.
- Executes **Kahn's Topological Sort** to determine execution order across audio threads without phase distortion.
- Computes sample-accurate delay compensation buffers across parallel processing branches to eliminate acoustic comb filtering.

### 2.3 Real-Time Audio Driver Abstraction (`JZAudioPlayer`, `JZPlayer`)
- **Linux**: Direct ALSA kernel sequencer (`snd_seq`) and hardware PCM (`snd_pcm`) interfaces providing sub-5ms low latency.
- **macOS**: Native CoreAudio framework for multi-channel audio streaming and CoreMIDI via PortMidi for hardware and virtual IAC bus routing.
- **Windows**: Modern Windows Audio Session API (WASAPI) with low-latency exclusive and shared streaming modes, alongside legacy waveOut compatibility.

---

## 3. Real-Time Concurrency & Memory Model

Audio workstations require strict real-time guarantees to avoid audible clicks, pops, and buffer dropouts:

```
+------------------------+       Lock-Free Ringbuffer       +------------------------+
|   UI / Main Thread     |  =============================>  |  Real-Time DSP Thread  |
|  (wxWidgets Event Loop)|       (SPSC Atomic Queue)        | (SCHED_FIFO / MMCSS)   |
+------------------------+                                  +------------------------+
                                                                        |
                                                            Slab Buffer Allocator
                                                            (Zero OS Malloc / Free)
                                                                        |
                                                                        v
                                                            High-Performance Cores
                                                            (P-Core CPU Pinning)
```

1. **Zero Heap Allocation on DSP Thread**: Audio processing blocks (`process()`) operate entirely within pre-allocated slab memory pools (`JZLockFreeAudioBufferPoolAllocator`). System calls (`malloc`, `free`, `new`, `delete`) are strictly forbidden on the audio callback.
2. **Lock-Free Inter-Thread Communication**: Parameter adjustments, transport commands, and automation curves pass from the UI thread to the DSP thread via Single-Producer Single-Consumer (SPSC) lock-free ringbuffers.
3. **P-Core Thread Affinity (`JZRealTimeThreadAffinityScheduler`)**: Pins critical DSP worker threads to physical CPU Performance Cores (P-Cores), preventing the OS scheduler from migrating threads to power-saving Efficiency Cores (E-Cores).
4. **Out-of-Process Plugin Sandboxing (`JZPluginCrashSandboxEngine`)**: Isolates external VST3 and CLAP binary plugins into dedicated worker processes with watchdog heartbeat monitoring (100ms-1000ms) and automatic dry-bypass fallback upon crash detection.

---

## 4. Spatial Audio & Immersive Bed Architecture

Jazz++ supports cutting-edge 3D spatial mixing formats:

- **Multichannel 7.1.4 & 9.1.6 Immersive Beds**: 12 to 16 discrete audio channels supporting height speakers and LFE management.
- **Higher-Order Ambisonics (HOA)**: Full 3rd-order 16-channel spherical harmonics encoding with ACN channel ordering, SN3D normalization, and 3D Euler rotational steering (Yaw, Pitch, Roll).
- **Personalized HRTF & BRIR Convolution**: Spherical head model Woodworth interaural time difference (ITD) delay lines, pinna height notch filters, and SOFA (Spatially Oriented Format for Acoustics) impulse responses for precision headphone monitoring.
- **ITU-R BS.1770-4 Loudness Metering**: Integrated LUFS, Short-Term LUFS, Momentary LUFS, Loudness Range (LRA), and 4x oversampled Spatial Inter-Sample True-Peak detection across all immersive channels.

---

## 5. Broadcast & Hardware Synchronization

- **SMPTE ST 2110-30 & AES67 AoIP**: Networked audio-over-IP streaming synchronization.
- **IEEE 1588v2 Precision Time Protocol (PTP)**: Software dual-loop phase-locked loop (PLL) tracking PTP Grandmaster clocks with sub-microsecond precision.
- **BWF & iXML Metadata Delivery**: Direct export of Broadcast Wave Format files with embedded scene, take, timecode rate, track name, and stem metadata conforming to EBU Tech 3285 standards.
