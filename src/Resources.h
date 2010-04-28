//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Copyright (C) 1994-2000 Andreas Voss and Per Sigmond, all rights reserved.
// Modifications Copyright (C) 2004 Patrick Earl
// Modifications Copyright (C) 2008-2010 Peter J. Stieber
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//*****************************************************************************

#ifndef JZ_RESOURCES_H
#define JZ_RESOURCES_H

// The wxWidgets documentation says it's safe to roll your own identifiers
// below "wxID_LOWEST" or above "wxID_HIGHEST".  In recent versions, the
// identifiers were enumerated but we need macros for resources since the
// resource compilers cannot handle C++ enum values, so we set the value
// of wxID_HIGHEST here.
#define wxID_HIGHEST                         5999

#define ID_EXPORT_MIDI                       wxID_HIGHEST + 1
#define ID_EXPORT_SELECTION_AS_MIDI          wxID_HIGHEST + 2

#define ID_SETTINGS_METRONOME                wxID_HIGHEST + 10
#define ID_SETTINGS_SYNTHESIZER              wxID_HIGHEST + 11
#define ID_SETTINGS_MIDI_DEVICE              wxID_HIGHEST + 12

#define ID_AUDIO_GLOBAL_SETTINGS             wxID_HIGHEST + 20
#define ID_AUDIO_SAMPLE_SETTINGS             wxID_HIGHEST + 21
#define ID_AUDIO_LOAD_SAMPLE_SET             wxID_HIGHEST + 22
#define ID_AUDIO_SAVE_SAMPLE_SET             wxID_HIGHEST + 23
#define ID_AUDIO_SAVE_SAMPLE_SET_AS          wxID_HIGHEST + 24
#define ID_AUDIO_NEW_SAMPLE_SET              wxID_HIGHEST + 25

#define ID_TRIM                              wxID_HIGHEST + 30
#define ID_QUANTIZE                          wxID_HIGHEST + 31
#define ID_SET_CHANNEL                       wxID_HIGHEST + 32
#define ID_SHIFT                             wxID_HIGHEST + 33
#define ID_SHIFT_LEFT                        wxID_HIGHEST + 34
#define ID_SHIFT_RIGHT                       wxID_HIGHEST + 35
#define ID_SNAP                              wxID_HIGHEST + 36
#define ID_SNAP_8                            wxID_HIGHEST + 37
#define ID_SNAP_8D                           wxID_HIGHEST + 38
#define ID_SNAP_16                           wxID_HIGHEST + 39
#define ID_SNAP_16D                          wxID_HIGHEST + 40
#define ID_MIXER                             wxID_HIGHEST + 41
#define ID_PIANOWIN                          wxID_HIGHEST + 42
#define ID_METRONOME_TOGGLE                  wxID_HIGHEST + 43
#define ID_VELOCITY                          wxID_HIGHEST + 44
#define ID_LENGTH                            wxID_HIGHEST + 45
#define ID_MISC_TRACK_MERGE                  wxID_HIGHEST + 46
#define ID_MISC_SPLIT_TRACKS                 wxID_HIGHEST + 47
#define ID_MISC_METER_CHANGE                 wxID_HIGHEST + 48
#define ID_MISC_RESET_MIDI                   wxID_HIGHEST + 49
#define ID_MISC_SET_COPYRIGHT                wxID_HIGHEST + 50
#define ID_TRANSPOSE                         wxID_HIGHEST + 51
#define ID_CLEANUP                           wxID_HIGHEST + 52
#define ID_SEARCH_AND_REPLACE                wxID_HIGHEST + 53

#define ID_PLAY                              wxID_HIGHEST + 60
#define ID_PLAY_LOOP                         wxID_HIGHEST + 61
#define ID_RECORD                            wxID_HIGHEST + 62

#define ID_SELECT                            wxID_HIGHEST + 65

#define ID_CHANGE_LENGTH                     wxID_HIGHEST + 70

#define ID_EVENT_DIALOG                      wxID_HIGHEST + 81
#define ID_CUT_PASTE_EVENTS                  wxID_HIGHEST + 82
#define ID_SHOW_ALL_EVENTS_FROM_ALL_TRACKS   wxID_HIGHEST + 83

#define ID_TOOLS_HARMONY_BROWSER             wxID_HIGHEST + 95

#define ID_HELP_PIANO_WINDOW                 wxID_HIGHEST + 100

#define MEN_CLEAR                            wxID_HIGHEST + 110
#define MEN_SETTINGS                         wxID_HIGHEST + 120

#define IDC_KB_VOLUME                        wxID_HIGHEST + 1000

#define IDC_KB_VELOCITY                      wxID_HIGHEST + 1100
#define IDC_KB_OFF_VELOCITY                  wxID_HIGHEST + 1101
#define IDC_KB_CHANNEL                       wxID_HIGHEST + 1102

// JZVelocityDialog resource IDs.
#define IDC_KB_VELOCITY_START                wxID_HIGHEST + 1200
#define IDC_KB_VELOCITY_STOP                 wxID_HIGHEST + 1201

// JZLengthDialog resource IDs.
#define IDC_KB_LENGTH_START                  wxID_HIGHEST + 1210
#define IDC_KB_LENGTH_STOP                   wxID_HIGHEST + 1211

// JZMidiChannelDialog resource IDs.
#define IDC_KB_MIDI_CHANNEL                  wxID_HIGHEST + 1220

// JZQuantizeDialog resource IDs.
#define IDC_KB_GROOVE                        wxID_HIGHEST + 1230
#define IDC_KB_DELAY                         wxID_HIGHEST + 1231

// JZTransposeDialog resource IDs.
#define IDC_KB_AMOUNT                        wxID_HIGHEST + 1232

#endif // !defined(JZ_RESOURCES_H)
