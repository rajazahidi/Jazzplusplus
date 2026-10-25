#!/usr/bin/env python3
"""
Master Song Generator for Jazz++ DAW
Generates two complete 4-minute instrumental masterworks:
1. "Eternity" - Rock Ballad Instrumental (70 BPM, 70 bars = exactly 240.0s)
2. "Sovereign Symphony" - Orchestral Instrumental (80 BPM, 80 bars = exactly 240.0s)
"""

import os
import math

PPQ = 480  # Pulses per quarter note
BAR_TICKS = 1920  # 4 beats * 480 ticks in 4/4 time

def encode_vlq(val):
    val = int(val)
    bytes_out = [val & 0x7F]
    val >>= 7
    while val > 0:
        bytes_out.append((val & 0x7F) | 0x80)
        val >>= 7
    bytes_out.reverse()
    return bytes(bytes_out)

class MidiTrack:
    def __init__(self, name=""):
        self.name = name
        self.events = []  # List of tuples: (abs_tick, priority, event_bytes)
        # Priority ensures Note Off (0) precedes Note On (1) if simultaneous
        if name:
            self.add_meta_event(0, 0x03, name.encode('utf-8'))

    def add_meta_event(self, abs_tick, meta_type, data_bytes):
        payload = bytes([0xFF, meta_type]) + encode_vlq(len(data_bytes)) + data_bytes
        self.events.append((abs_tick, 0, payload))

    def add_note(self, channel, pitch, velocity, start_tick, duration_ticks):
        # Note On
        self.events.append((start_tick, 2, bytes([0x90 | channel, pitch, velocity])))
        # Note Off
        self.events.append((start_tick + duration_ticks, 1, bytes([0x80 | channel, pitch, 0])))

    def add_cc(self, channel, cc, value, abs_tick):
        self.events.append((abs_tick, 0, bytes([0xB0 | channel, cc, value])))

    def add_program(self, channel, program, abs_tick):
        self.events.append((abs_tick, 0, bytes([0xC0 | channel, program])))

    def add_pitch_bend(self, channel, value_14bit, abs_tick):
        # 8192 is center (0x2000)
        lsb = value_14bit & 0x7F
        msb = (value_14bit >> 7) & 0x7F
        self.events.append((abs_tick, 0, bytes([0xE0 | channel, lsb, msb])))

    def to_bytes(self):
        # Sort by abs_tick, then priority
        self.events.sort(key=lambda x: (x[0], x[1]))
        track_data = bytearray()
        prev_tick = 0
        for abs_tick, _, ev_bytes in self.events:
            delta = abs_tick - prev_tick
            track_data.extend(encode_vlq(delta))
            track_data.extend(ev_bytes)
            prev_tick = abs_tick

        # End of track event
        track_data.extend(encode_vlq(0))
        track_data.extend(b'\xFF\x2F\x00')

        header = b'MTrk' + len(track_data).to_bytes(4, 'big')
        return header + track_data

class MidiFile:
    def __init__(self, ppq=480):
        self.ppq = ppq
        self.tracks = []

    def add_track(self, track):
        self.tracks.append(track)

    def write(self, filename):
        os.makedirs(os.path.dirname(os.path.abspath(filename)), exist_ok=True)
        num_tracks = len(self.tracks)
        header = b'MThd' + (6).to_bytes(4, 'big') + (1).to_bytes(2, 'big') + num_tracks.to_bytes(2, 'big') + self.ppq.to_bytes(2, 'big')
        with open(filename, 'wb') as f:
            f.write(header)
            for t in self.tracks:
                f.write(t.to_bytes())
        print(f"Successfully generated: {filename} ({num_tracks} tracks)")


# =============================================================================
# 1. GENERATE "Eternity" - Rock Ballad Instrumental (70 BPM, 70 bars = 240.0s)
# =============================================================================
def generate_rock_ballad(filepath):
    midi = MidiFile(ppq=PPQ)

    # Tempo & Conductor Track
    cond = MidiTrack("Conductor / Tempo")
    # Time Sig 4/4
    cond.add_meta_event(0, 0x58, bytes([4, 2, 24, 8]))
    # Tempo 70 BPM -> 60,000,000 / 70 = 857,143 us/beat
    tempo_us = int(round(60000000.0 / 70.0))
    cond.add_meta_event(0, 0x51, tempo_us.to_bytes(3, 'big'))
    cond.add_meta_event(0, 0x01, b"Eternity - Rock Ballad Instrumental (4:00)")
    cond.add_meta_event(0, 0x02, b"Copyright (C) 2026 Raja Zahidi / Jazz++ DAW")
    midi.add_track(cond)

    # Tracks definition:
    # Trk 1: Clean Electric Guitar (Ch 0, Patch 27 "Electric Guitar clean")
    # Trk 2: Overdriven Rhythm Guitar (Ch 1, Patch 29 "Overdriven Guitar")
    # Trk 3: Lead Guitar Solo (Ch 2, Patch 30 "Distortion Guitar")
    # Trk 4: Electric Bass Guitar (Ch 3, Patch 33 "Electric Bass finger")
    # Trk 5: Drums (Ch 9, Drum Kit)
    # Trk 6: Grand Piano (Ch 4, Patch 0 "Acoustic Grand Piano")
    # Trk 7: Warm Pad / Organ (Ch 5, Patch 16 "Drawbar Organ")

    trk_clean_gtr = MidiTrack("Clean Electric Guitar")
    trk_clean_gtr.add_program(0, 27, 0) # Clean Guitar
    trk_clean_gtr.add_cc(0, 7, 95, 0)   # Volume
    trk_clean_gtr.add_cc(0, 10, 35, 0)  # Pan Left
    trk_clean_gtr.add_cc(0, 91, 55, 0)  # Reverb
    trk_clean_gtr.add_cc(0, 93, 40, 0)  # Chorus

    trk_dist_gtr = MidiTrack("Distortion Rhythm Guitar")
    trk_dist_gtr.add_program(1, 29, 0)  # Overdrive
    trk_dist_gtr.add_cc(1, 7, 100, 0)
    trk_dist_gtr.add_cc(1, 10, 95, 0)   # Pan Right
    trk_dist_gtr.add_cc(1, 91, 35, 0)

    trk_lead_gtr = MidiTrack("Lead Guitar (Anthem Solo)")
    trk_lead_gtr.add_program(2, 30, 0)  # Distortion Guitar
    trk_lead_gtr.add_cc(2, 7, 105, 0)
    trk_lead_gtr.add_cc(2, 10, 64, 0)   # Center
    trk_lead_gtr.add_cc(2, 91, 65, 0)   # Lush Reverb
    trk_lead_gtr.add_cc(2, 93, 30, 0)

    trk_bass = MidiTrack("Rock Bass Guitar")
    trk_bass.add_program(3, 33, 0)      # Finger Bass
    trk_bass.add_cc(3, 7, 100, 0)
    trk_bass.add_cc(3, 10, 64, 0)
    trk_bass.add_cc(3, 91, 20, 0)

    trk_drums = MidiTrack("Rock Drum Kit")
    # Ch 9 is MIDI channel 10
    trk_drums.add_cc(9, 7, 105, 0)
    trk_drums.add_cc(9, 10, 64, 0)
    trk_drums.add_cc(9, 91, 45, 0)

    trk_piano = MidiTrack("Acoustic Grand Piano")
    trk_piano.add_program(4, 0, 0)      # Grand Piano
    trk_piano.add_cc(4, 7, 90, 0)
    trk_piano.add_cc(4, 10, 50, 0)
    trk_piano.add_cc(4, 91, 50, 0)

    trk_pad = MidiTrack("Hammond Organ & Warm Strings")
    trk_pad.add_program(5, 16, 0)       # Drawbar Organ
    trk_pad.add_cc(5, 7, 80, 0)
    trk_pad.add_cc(5, 10, 75, 0)
    trk_pad.add_cc(5, 91, 70, 0)

    # Chords Progression map (70 bars total):
    # Progression: Em (E G B), C (C E G), G (G B D), D (D F# A)
    chord_roots = [40, 36, 43, 38] # Bass notes: E1, C1, G1, D1
    chord_triads = [
        [52, 55, 59, 64], # Em (E3, G3, B3, E4)
        [48, 52, 55, 60], # C (C3, E3, G3, C4)
        [43, 47, 50, 55], # G (G2, B2, D3, G3)
        [50, 54, 57, 62], # D (D3, F#3, A3, D4)
    ]
    power_chords = [
        [40, 47, 52], # E5
        [36, 43, 48], # C5
        [43, 50, 55], # G5
        [38, 45, 50], # D5
    ]

    for bar in range(70):
        bar_tick = bar * BAR_TICKS
        chord_idx = bar % 4
        root = chord_roots[chord_idx]
        triad = chord_triads[chord_idx]
        pwr = power_chords[chord_idx]

        # Section logic:
        # Bars 0-7: Intro (Clean guitar + soft piano + pad)
        # Bars 8-23: Verse 1 (Clean guitar + bass + drums cross-stick + lead melody)
        # Bars 24-35: Chorus 1 (Power chords + heavy drums + soaring harmony lead)
        # Bars 36-47: Verse 2 (Clean guitar + bass fills + ride cymbal + counter melody)
        # Bars 48-59: Guitar Solo Climax (Virtuoso lead + driving heavy rhythm)
        # Bars 60-67: Final Chorus Tutti (Full heavy band + organ + soaring anthem)
        # Bars 68-69: Outro (Clean arpeggio fadeout to bar 70 beat 4)

        is_intro = (bar < 8)
        is_verse1 = (8 <= bar < 24)
        is_chorus1 = (24 <= bar < 36)
        is_verse2 = (36 <= bar < 48)
        is_solo = (48 <= bar < 60)
        is_chorus2 = (60 <= bar < 68)
        is_outro = (bar >= 68)

        # 1. CLEAN ELECTRIC GUITAR ARPEGGIO (Active in Intro, Verses, and Outro)
        if is_intro or is_verse1 or is_verse2 or is_outro:
            # 8 eighth-note arpeggio pattern: 0, 1, 2, 3, 2, 1, 0, 2
            arp_pattern = [0, 1, 2, 3, 2, 1, 0, 2]
            for step in range(8):
                note_pitch = triad[arp_pattern[step]]
                vel = 70 + (step % 2) * 15
                if is_outro and bar == 69 and step >= 4:
                    continue # Let final chord ring out in outro
                trk_clean_gtr.add_note(0, note_pitch, vel, bar_tick + step * 240, 220)
            if is_outro and bar == 69:
                # Ringing Em9 chord at ending
                for p in [40, 52, 55, 59, 62, 66]:
                    trk_clean_gtr.add_note(0, p, 80, bar_tick + 960, 960)

        # 2. OVERDRIVEN RHYTHM GUITAR (Power chords in Chorus 1, Solo, and Chorus 2)
        if is_chorus1 or is_solo or is_chorus2:
            # Driving rock rhythm: beat 1 (long), beat 2 (& eighths), beat 3, beat 4
            rhythm_offsets = [0, 480, 720, 960, 1200, 1440, 1680]
            for ro in rhythm_offsets:
                for note_pitch in pwr:
                    trk_dist_gtr.add_note(1, note_pitch, 98, bar_tick + ro, 210)

        # 3. ROCK BASS GUITAR (Enters at Verse 1, stays through Chorus 2)
        if not is_intro and not is_outro:
            if is_verse1 or is_verse2:
                # Root-fifth groove on quarter notes
                trk_bass.add_note(3, root, 88, bar_tick + 0, 440)
                trk_bass.add_note(3, root, 80, bar_tick + 480, 440)
                trk_bass.add_note(3, root + 7, 85, bar_tick + 960, 440) # Fifth
                trk_bass.add_note(3, root, 82, bar_tick + 1440, 440)
            elif is_chorus1 or is_solo or is_chorus2:
                # Driving eighth notes
                for step in range(8):
                    p = root if step < 6 else (root + 7 if step == 6 else root + 2)
                    trk_bass.add_note(3, p, 96, bar_tick + step * 240, 220)
        elif is_outro and bar == 68:
            trk_bass.add_note(3, root, 80, bar_tick + 0, 1800)

        # 4. ROCK DRUMS (Channel 10)
        # Note 36: Kick, 38: Snare, 37: Side Stick, 42: Closed HH, 46: Open HH, 49: Crash, 51: Ride
        if is_verse1:
            # Gentle verse groove: Kick on 1 & 3, Side Stick on 2 & 4, Closed HH on eighths
            for step in range(8):
                trk_drums.add_note(9, 42, 65, bar_tick + step * 240, 100) # HH
            trk_drums.add_note(9, 36, 85, bar_tick + 0, 150) # Kick
            trk_drums.add_note(9, 36, 75, bar_tick + 960, 150) # Kick
            trk_drums.add_note(9, 37, 80, bar_tick + 480, 120) # Side Stick
            trk_drums.add_note(9, 37, 85, bar_tick + 1440, 120)
        elif is_verse2:
            # Ride cymbal groove
            for step in range(8):
                trk_drums.add_note(9, 51, 75, bar_tick + step * 240, 120) # Ride
            trk_drums.add_note(9, 36, 90, bar_tick + 0, 150)
            trk_drums.add_note(9, 36, 85, bar_tick + 720, 150)
            trk_drums.add_note(9, 36, 88, bar_tick + 960, 150)
            trk_drums.add_note(9, 38, 92, bar_tick + 480, 150) # Snare
            trk_drums.add_note(9, 38, 95, bar_tick + 1440, 150)
        elif is_chorus1 or is_solo or is_chorus2:
            # Heavy rock drum groove: Crash on downbeat of phrases, Driving Kick, Cracking Snare
            if bar % 4 == 0:
                trk_drums.add_note(9, 49, 110, bar_tick + 0, 400) # Crash
            for step in range(8):
                hh_note = 46 if (step == 2 or step == 6) else 42
                trk_drums.add_note(9, hh_note, 85, bar_tick + step * 240, 120)
            # Driving kick: 1, 1&, 3
            trk_drums.add_note(9, 36, 105, bar_tick + 0, 180)
            trk_drums.add_note(9, 36, 95, bar_tick + 480, 180)
            trk_drums.add_note(9, 36, 102, bar_tick + 960, 180)
            trk_drums.add_note(9, 36, 92, bar_tick + 1200, 180)
            # Snare on 2 and 4
            trk_drums.add_note(9, 38, 110, bar_tick + 480, 200)
            trk_drums.add_note(9, 38, 115, bar_tick + 1440, 200)
            # Tom fill at end of section (bar 35, 47, 59)
            if bar in [35, 47, 59]:
                trk_drums.add_note(9, 50, 105, bar_tick + 960, 180) # High Tom
                trk_drums.add_note(9, 47, 105, bar_tick + 1200, 180) # Mid Tom
                trk_drums.add_note(9, 45, 110, bar_tick + 1440, 180) # Low Tom
                trk_drums.add_note(9, 49, 115, bar_tick + 1680, 240) # Crash
        elif is_outro and bar == 69:
            # Final cymbal swell & kick
            trk_drums.add_note(9, 49, 90, bar_tick + 960, 960) # Crash
            trk_drums.add_note(9, 36, 85, bar_tick + 960, 400)

        # 5. PIANO & PAD
        if is_intro or is_verse1 or is_verse2 or is_chorus1 or is_chorus2:
            # Sustained warm organ / string chords on whole notes
            for note_pitch in triad:
                trk_pad.add_note(5, note_pitch + 12, 72, bar_tick + 0, 1850)
            # Piano plays delicate chord touches on beat 1 and 3
            for note_pitch in triad:
                trk_piano.add_note(4, note_pitch, 78, bar_tick + 0, 900)
                trk_piano.add_note(4, note_pitch + 7, 72, bar_tick + 960, 900)

        # 6. LEAD GUITAR THEMES & VIRTUOSO SOLO
        if is_verse1 and bar >= 12:
            # Gentle singing melodic phrase
            melody_notes = [64, 67, 71, 69, 67, 64] # E4, G4, B4, A4, G4, E4
            note_p = melody_notes[(bar - 12) % len(melody_notes)]
            trk_lead_gtr.add_note(2, note_p, 85, bar_tick + 480, 900)
        elif is_chorus1 or is_chorus2:
            # Soaring Anthem Chorus Theme in high register
            chorus_motifs = [
                (76, 0, 720), (79, 720, 480), (76, 1200, 720), # E5, G5, E5
                (74, 0, 720), (76, 720, 480), (74, 1200, 720), # D5, E5, D5
                (71, 0, 960), (74, 960, 960),                   # B4, D5
                (69, 0, 720), (71, 720, 480), (72, 1200, 720)  # A4, B4, C5
            ]
            m_pitch, m_start, m_dur = chorus_motifs[bar % 4]
            trk_lead_gtr.add_note(2, m_pitch, 105, bar_tick + m_start, m_dur)
            # Add harmony in thirds for epic twin-lead in Chorus 2
            if is_chorus2:
                trk_lead_gtr.add_note(2, m_pitch + 4, 92, bar_tick + m_start, m_dur)
        elif is_solo:
            # Epic Virtuoso Guitar Solo (Bars 48 to 59)
            # Fast expressive pentatonic blues licks, soaring bends, fast triplets
            solo_bar = bar - 48
            if solo_bar % 2 == 0:
                # Fast rising run: E4, G4, A4, B4, D5, E5, G5, A5
                run = [64, 67, 69, 71, 74, 76, 79, 81]
                for idx, note_pitch in enumerate(run):
                    trk_lead_gtr.add_note(2, note_pitch, 102 + idx * 2, bar_tick + idx * 240, 210)
            else:
                # Screaming high bend on E5 / G5 with vibrato
                trk_lead_gtr.add_note(2, 81, 115, bar_tick + 0, 960) # High A5 bend
                trk_lead_gtr.add_note(2, 79, 110, bar_tick + 960, 480) # G5
                trk_lead_gtr.add_note(2, 76, 112, bar_tick + 1440, 480) # E5

    midi.add_track(trk_clean_gtr)
    midi.add_track(trk_dist_gtr)
    midi.add_track(trk_lead_gtr)
    midi.add_track(trk_bass)
    midi.add_track(trk_drums)
    midi.add_track(trk_piano)
    midi.add_track(trk_pad)
    midi.write(filepath)


# =============================================================================
# 2. GENERATE "Sovereign Symphony" - Orchestral Instrumental (80 BPM, 80 bars = 240.0s)
# =============================================================================
def generate_orchestral_symphony(filepath):
    midi = MidiFile(ppq=PPQ)

    # Conductor & Tempo Track
    cond = MidiTrack("Conductor / Tempo")
    cond.add_meta_event(0, 0x58, bytes([4, 2, 24, 8])) # 4/4
    # Tempo 80 BPM -> 60,000,000 / 80 = 750,000 us/beat
    tempo_us = int(round(60000000.0 / 80.0))
    cond.add_meta_event(0, 0x51, tempo_us.to_bytes(3, 'big'))
    cond.add_meta_event(0, 0x01, b"Sovereign Symphony - Orchestral Instrumental (4:00)")
    cond.add_meta_event(0, 0x02, b"Copyright (C) 2026 Raja Zahidi / Jazz++ DAW")
    midi.add_track(cond)

    # Tracks definition:
    # Trk 1: Violins I (Ch 0, Patch 40 "Violin")
    # Trk 2: Violins II (Ch 1, Patch 48 "String Ensemble 1")
    # Trk 3: Violas (Ch 2, Patch 41 "Viola")
    # Trk 4: Cellos (Ch 3, Patch 42 "Cello")
    # Trk 5: Double Basses (Ch 4, Patch 43 "Contrabass")
    # Trk 6: Flutes & Oboes (Ch 5, Patch 73 "Flute")
    # Trk 7: French Horns & Brass (Ch 6, Patch 60 "French Horn")
    # Trk 8: Orchestral Timpani & Percussion (Ch 7, Patch 47 "Timpani")
    # Trk 9: Concert Harp & Piano (Ch 8, Patch 46 "Orchestral Harp")

    trkVln1 = MidiTrack("First Violins (Concert Master)")
    trkVln1.add_program(0, 40, 0)
    trkVln1.add_cc(0, 7, 100, 0)
    trkVln1.add_cc(0, 10, 35, 0) # Pan Left
    trkVln1.add_cc(0, 91, 60, 0)

    trkVln2 = MidiTrack("Second Violins")
    trkVln2.add_program(1, 48, 0)
    trkVln2.add_cc(1, 7, 85, 0)
    trkVln2.add_cc(1, 10, 45, 0)
    trkVln2.add_cc(1, 91, 60, 0)

    trkVla = MidiTrack("Violas")
    trkVla.add_program(2, 41, 0)
    trkVla.add_cc(2, 7, 88, 0)
    trkVla.add_cc(2, 10, 60, 0) # Center-Right
    trkVla.add_cc(2, 91, 60, 0)

    trkVc = MidiTrack("Violoncellos")
    trkVc.add_program(3, 42, 0)
    trkVc.add_cc(3, 7, 95, 0)
    trkVc.add_cc(3, 10, 80, 0) # Right
    trkVc.add_cc(3, 91, 55, 0)

    trkCb = MidiTrack("Double Basses")
    trkCb.add_program(4, 43, 0)
    trkCb.add_cc(4, 7, 95, 0)
    trkCb.add_cc(4, 10, 64, 0)
    trkCb.add_cc(4, 91, 45, 0)

    trkWood = MidiTrack("Flutes & Woodwinds")
    trkWood.add_program(5, 73, 0)
    trkWood.add_cc(5, 7, 90, 0)
    trkWood.add_cc(5, 10, 50, 0)
    trkWood.add_cc(5, 91, 65, 0)

    trkBrass = MidiTrack("French Horns & Brass Choir")
    trkBrass.add_program(6, 60, 0)
    trkBrass.add_cc(6, 7, 98, 0)
    trkBrass.add_cc(6, 10, 70, 0)
    trkBrass.add_cc(6, 91, 55, 0)

    trkPerc = MidiTrack("Orchestral Timpani & Cymbals")
    trkPerc.add_program(7, 47, 0) # Timpani
    trkPerc.add_cc(7, 7, 105, 0)
    trkPerc.add_cc(7, 10, 64, 0)
    trkPerc.add_cc(7, 91, 70, 0)

    trkHarp = MidiTrack("Concert Harp & Celeste")
    trkHarp.add_program(8, 46, 0) # Harp
    trkHarp.add_cc(8, 7, 90, 0)
    trkHarp.add_cc(8, 10, 30, 0)
    trkHarp.add_cc(8, 91, 75, 0)

    # Harmonic Progression in D Major / B Minor across 80 bars:
    # Primary progression: D Maj (D F# A), G Maj (G B D), A Maj (A C# E), B min (B D F#)
    d_major_harmony = [
        # (root, [vln1_notes], [vln2_notes], [vla_notes], [vc_notes])
        (38, [62, 66, 69, 74], [57, 62, 66], [54, 57, 62], [38, 45, 50]), # D
        (43, [67, 71, 74, 79], [59, 62, 67], [55, 59, 62], [43, 47, 50]), # G
        (45, [69, 73, 76, 81], [61, 64, 69], [57, 61, 64], [45, 49, 52]), # A
        (47, [71, 74, 78, 83], [62, 66, 71], [59, 62, 66], [47, 50, 54]), # Bm
    ]

    for bar in range(80):
        bar_tick = bar * BAR_TICKS
        chord_idx = bar % 4
        root_pitch, vln1_chord, vln2_chord, vla_chord, vc_chord = d_major_harmony[chord_idx]

        # Section Breakdown:
        # Bars 0-7: Maestoso Intro (Harp + Horn calls + Woodwinds)
        # Bars 8-23: Exposition Theme A (Strings leading in D Major, sweeping legato)
        # Bars 24-39: Theme B Adagio (Modulation to B minor, solo cello, rich woodwinds)
        # Bars 40-55: Scherzando Development (Brass fanfares, timpani, staccato strings)
        # Bars 56-71: Grand Tutti Climax (Full symphonic majesty, all sections fff)
        # Bars 72-79: Coda & Serene Resolution (Solo violin, flute, harp resolving to D)

        is_intro = (bar < 8)
        is_theme_a = (8 <= bar < 24)
        is_theme_b = (24 <= bar < 40)
        is_dev = (40 <= bar < 56)
        is_climax = (56 <= bar < 72)
        is_coda = (bar >= 72)

        # 1. HARP ARPEGGIOS (Intro, Theme B, and Coda)
        if is_intro or is_theme_b or is_coda:
            # Ascending 16th-note harp sweep
            notes = [vln1_chord[0] - 12, vln1_chord[1] - 12, vln1_chord[2] - 12,
                     vln1_chord[0], vln1_chord[1], vln1_chord[2], vln1_chord[3], vln1_chord[3] + 4]
            for step, p in enumerate(notes):
                if is_coda and bar == 79 and step >= 4:
                    continue # Let final chord linger
                trkHarp.add_note(8, p, 75 + (step % 2) * 10, bar_tick + step * 240, 400)
            if is_coda and bar == 79:
                for p in [62, 66, 69, 74, 81]:
                    trkHarp.add_note(8, p, 85, bar_tick + 960, 960)

        # 2. FRENCH HORNS & BRASS CHOIR
        if is_intro:
            # Regal solo French Horn calls
            if bar % 2 == 0:
                trkBrass.add_note(6, 57, 85, bar_tick + 0, 960) # A3
                trkBrass.add_note(6, 62, 90, bar_tick + 960, 960) # D4
        elif is_dev or is_climax:
            # Triumphant brass fanfare: dotted eighth & sixteenth figures
            fanfare_notes = [54, 57, 62, 66] # D major triad in brass
            for fn in fanfare_notes:
                trkBrass.add_note(6, fn, 105, bar_tick + 0, 720)
                trkBrass.add_note(6, fn, 100, bar_tick + 720, 240)
                trkBrass.add_note(6, fn, 110, bar_tick + 960, 960)

        # 3. FIRST VIOLINS (Lead Theme A, Scherzando, and Grand Climax)
        if is_theme_a or is_climax:
            # Soaring lyrical melody
            theme_notes = [
                (74, 0, 720), (76, 720, 240), (78, 960, 960), # D5, E5, F#5
                (79, 0, 480), (78, 480, 480), (76, 960, 960), # G5, F#5, E5
                (78, 0, 720), (79, 720, 240), (81, 960, 960), # F#5, G5, A5
                (86, 0, 960), (81, 960, 960)                  # D6, A5
            ]
            p, st, dur = theme_notes[bar % 4]
            trkVln1.add_note(0, p, 105 if is_climax else 88, bar_tick + st, dur)
        elif is_dev:
            # Fast sixteenth-note staccato ostinato
            for step in range(8):
                p = vln1_chord[step % len(vln1_chord)]
                trkVln1.add_note(0, p, 95, bar_tick + step * 240, 180)
        elif is_coda and bar < 78:
            # Tender solo violin reminiscence
            trkVln1.add_note(0, vln1_chord[0], 75, bar_tick + 0, 1800)

        # 4. SECOND VIOLINS & VIOLAS (Harmonic movement)
        if not is_intro:
            for p in vln2_chord:
                trkVln2.add_note(1, p, 80 if not is_climax else 98, bar_tick + 0, 1850)
            for p in vla_chord:
                trkVla.add_note(2, p, 82 if not is_climax else 100, bar_tick + 0, 1850)

        # 5. CELLOS (Lyrical in Theme B, Bass in Climax)
        if is_theme_b:
            # Passionate Solo Cello Melody in high tenor register
            cello_melody = [62, 66, 69, 66, 64, 62, 59, 62] # D4, F#4, A4, F#4, E4, D4, B3, D4
            p1 = cello_melody[(bar * 2) % len(cello_melody)]
            p2 = cello_melody[(bar * 2 + 1) % len(cello_melody)]
            trkVc.add_note(3, p1, 92, bar_tick + 0, 900)
            trkVc.add_note(3, p2, 88, bar_tick + 960, 900)
        elif not is_intro:
            for p in vc_chord:
                trkVc.add_note(3, p, 85 if not is_climax else 102, bar_tick + 0, 1850)

        # 6. DOUBLE BASSES (Pedal notes & foundations)
        if not is_intro:
            if is_dev or is_climax:
                # Driving octave staccato rhythm
                trkCb.add_note(4, root_pitch - 12, 100, bar_tick + 0, 440)
                trkCb.add_note(4, root_pitch - 12, 92, bar_tick + 480, 440)
                trkCb.add_note(4, root_pitch - 5, 96, bar_tick + 960, 440)
                trkCb.add_note(4, root_pitch - 12, 95, bar_tick + 1440, 440)
            else:
                trkCb.add_note(4, root_pitch - 12, 85, bar_tick + 0, 1800)

        # 7. FLUTES & WOODWINDS
        if is_intro or is_theme_a or is_climax or is_coda:
            # High delicate fluttering runs & harmonies
            for step in range(4):
                p = vln1_chord[step] + 12
                trkWood.add_note(5, p, 82, bar_tick + step * 480, 400)

        # 8. ORCHESTRAL PERCUSSION (Timpani & Cymbals)
        if is_dev or is_climax:
            # Timpani punch on root notes (38 is D2)
            trkPerc.add_note(7, root_pitch, 108, bar_tick + 0, 440)
            if bar % 4 == 0:
                # Timpani roll into crash cymbal
                trkPerc.add_note(7, root_pitch, 95, bar_tick + 1200, 200)
                trkPerc.add_note(7, root_pitch, 105, bar_tick + 1440, 200)
                trkPerc.add_note(7, root_pitch, 115, bar_tick + 1680, 240)

    midi.add_track(trkVln1)
    midi.add_track(trkVln2)
    midi.add_track(trkVla)
    midi.add_track(trkVc)
    midi.add_track(trkCb)
    midi.add_track(trkWood)
    midi.add_track(trkBrass)
    midi.add_track(trkPerc)
    midi.add_track(trkHarp)
    midi.write(filepath)

if __name__ == "__main__":
    rock_path = "song/Eternity_Rock_Ballad_4min.mid"
    orch_path = "song/Sovereign_Orchestral_Symphony_4min.mid"
    generate_rock_ballad(rock_path)
    generate_orchestral_symphony(orch_path)

    # Mirror to Windows distribution folder and AppDir
    import shutil
    dest_dirs = [
        "bin-wxwidgets-v5.4.16/Jazz++-Windows-x64/song",
        "AppDir/usr/share/jazz/song"
    ]
    for d in dest_dirs:
        if os.path.exists(d):
            shutil.copy(rock_path, d)
            shutil.copy(orch_path, d)
            print(f"Mirrored files to: {d}")
