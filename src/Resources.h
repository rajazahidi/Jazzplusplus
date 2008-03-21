//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Copyright (C) 1994-2000 Andreas Voss and Per Sigmond, all rights reserved.
// Modifications Copyright (C) 2004 Patrick Earl
// Modifications Copyright (C) 2008 Peter J. Stieber
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

#define ID_SETTINGS_SYNTH                    wxID_HIGHEST + 3

#define ID_TRIM                              wxID_HIGHEST + 4
#define ID_QUANTIZE                          wxID_HIGHEST + 5
#define ID_SHIFT_LEFT                        wxID_HIGHEST + 6
#define ID_SHIFT_RIGHT                       wxID_HIGHEST + 7
#define ID_SNAP_8                            wxID_HIGHEST + 8
#define ID_SNAP_8D                           wxID_HIGHEST + 9
#define ID_SNAP_16                           wxID_HIGHEST + 10
#define ID_SNAP_16D                          wxID_HIGHEST + 11
#define ID_MIXER                             wxID_HIGHEST + 12
#define ID_PIANOWIN                          wxID_HIGHEST + 13
#define ID_METRONOME_ON                      wxID_HIGHEST + 14

#define ID_PLAY                              wxID_HIGHEST + 15
#define ID_PLAY_LOOP                         wxID_HIGHEST + 16
#define ID_RECORD                            wxID_HIGHEST + 17
#define ID_METRONOME                         wxID_HIGHEST + 18

#define ID_SELECT                            wxID_HIGHEST + 19

#define ID_CHANGE_LENGTH                     wxID_HIGHEST + 20

#define ID_EVENT_DIALOG                      wxID_HIGHEST + 21
#define ID_CUT_PASTE_EVENTS                  wxID_HIGHEST + 22
#define ID_SHOW_ALL_EVENTS_FROM_ALL_TRACKS   wxID_HIGHEST + 23

#define ID_TOOLS_HARMONY_BROWSER             wxID_HIGHEST + 25

#define ID_HELP_PIANO_WINDOW                 wxID_HIGHEST + 40

#define MEN_CLEAR                            wxID_HIGHEST + 50
#define MEN_SETTINGS                         wxID_HIGHEST + 60

#endif // !defined(JZ_RESOURCES_H)
