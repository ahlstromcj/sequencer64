#ifndef SEQ64_PORTMIDI_H
#define SEQ64_PORTMIDI_H

/*
 *  This file is part of sequencer64, adapted from the PortMIDI project.
 *
 *  sequencer64 is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  sequencer64 is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with seq24; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * \file        portmidi.h
 *
 *      PortMidi Portable Real-Time MIDI Library, PortMidi API Header File,
 *      Latest version available at: http://sourceforge.net/projects/portmedia.
 *
 * \library     sequencer64 application
 * \author      PortMIDI team; modifications by Chris Ahlstrom
 * \date        2017-08-21
 * \updates     2018-05-11
 * \license     GNU GPLv2 or above
 *
 * Copyright (c) 1999-2000 Ross Bencina and Phil Burk
 * Copyright (c) 2001-2006 Roger B. Dannenberg
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * The text above constitutes the entire PortMidi license; however, the
 * PortMusic community also makes the following non-binding requests:
 *
 * Any person wishing to distribute modifications to the Software is requested
 * to send the modifications to the original developer so that they can be
 * incorporated into the canonical version. It is also requested that these
 * non-binding requests be included along with the license above.
 */

#ifdef __cplusplus
extern "C"
{
#endif

#include "pminternal.h"             // midibyte_t typedef

#ifdef _WINDLL
#define PMEXPORT                    __declspec(dllexport)
#else
#define PMEXPORT
#endif

/**
 *  A single PortMidiStream is a descriptor for an open MIDI device.
 */

typedef void PortMidiStream;

/**
 *  Ugh.
 */

#define PmStream PortMidiStream

PMEXPORT PmError Pm_Initialize (void);
PMEXPORT PmError Pm_Terminate (void);
PMEXPORT int Pm_HasHostError (PortMidiStream * stream);
PMEXPORT const char * Pm_GetErrorText (PmError errnum);
PMEXPORT void Pm_GetHostErrorText (char * msg, unsigned int len);
PMEXPORT int Pm_CountDevices (void);

#ifdef SEQ64_PORTMIDI_DEFAULT_DEVICE_ID

/**
 *  Pm_GetDefaultInputDeviceID(), Pm_GetDefaultOutputDeviceID()
 *
 *  Return the default device ID or pmNoDevice if there are no devices.  The
 *  result (but not pmNoDevice) can be passed to Pm_OpenMidi().
 *
 *  The default device can be specified using a small application named
 *  pmdefaults that is part of the PortMidi distribution. This program in turn
 *  uses the Java Preferences object created by
 *  java.util.prefs.Preferences.userRoot().node("/PortMidi"); the preference
 *  is set by calling prefs.put("PM_RECOMMENDED_OUTPUT_DEVICE", prefName); or
 *  prefs.put("PM_RECOMMENDED_INPUT_DEVICE", prefName);
 *
 *  In the statements above, prefName is a string describing the MIDI device
 *  in the form "interf, name" where interf identifies the underlying software
 *  system or API used by PortMdi to access devices and name is the name of
 *  the device. These correspond to the interf and name fields of a
 *  PmDeviceInfo.  (Currently supported interfaces are "MMSystem" for Win32,
 *  "ALSA" for Linux, and "CoreMIDI" for OS X, so in fact, there is no choice
 *  of interface.) In "interf, name", the strings are actually substrings of
 *  the full interface and name strings. For example, the preference "Core,
 *  Sport" will match a device with interface "CoreMIDI" and name "In USB
 *  MidiSport 1x1". It will also match "CoreMIDI" and "In USB MidiSport 2x2".
 *  The devices are enumerated in device ID order, so the lowest device ID
 *  that matches the pattern becomes the default device. Finally, if the
 *  comma-space (", ") separator between interface and name parts of the
 *  preference is not found, the entire preference string is interpreted as a
 *  name, and the interface part is the empty string, which matches anything.
 *
 *  On the MAC, preferences are stored in
 *  /Users/$NAME/Library/Preferences/com.apple.java.util.prefs.plist which is
 *  a binary file. In addition to the pmdefaults program, there are utilities
 *  that can read and edit this preference file.
 *
 *  On the PC, ...
 *
 *  On Linux, ...
 */

PMEXPORT PmDeviceID Pm_GetDefaultInputDeviceID (void);
PMEXPORT PmDeviceID Pm_GetDefaultOutputDeviceID (void);

#endif  // SEQ64_PORTMIDI_DEFAULT_DEVICE_ID

PMEXPORT const PmDeviceInfo * Pm_GetDeviceInfo (PmDeviceID id);
PMEXPORT PmError Pm_OpenInput
(
    PortMidiStream ** stream,
    PmDeviceID inputDevice,
    void * inputDriverInfo,
    int32_t bufferSize,
    PmTimeProcPtr time_proc,
    void * time_info
);
PMEXPORT PmError Pm_OpenOutput
(
    PortMidiStream ** stream,
    PmDeviceID outputDevice,
    void * outputDriverInfo,
    int32_t bufferSize,
    PmTimeProcPtr time_proc,
    void * time_info,
    int32_t latency
);

/*
 * Filter bit-mask definitions
 */

/**
 *  filter active sensing messages (0xFE):
 */

#define PM_FILT_ACTIVE (1 << 0x0E)

/**
 *  filter system exclusive messages (0xF0):
 */

#define PM_FILT_SYSEX (1 << 0x00)

/**
 *  filter MIDI clock message (0xF8)
 */

#define PM_FILT_CLOCK (1 << 0x08)

/**
 *  filter play messages (start 0xFA, stop 0xFC, continue 0xFB)
 */

#define PM_FILT_PLAY ((1 << 0x0A) | (1 << 0x0C) | (1 << 0x0B))

/**
 *  filter tick messages (0xF9)
 */

#define PM_FILT_TICK (1 << 0x09)

/**
 *  filter undefined FD messages
 */

#define PM_FILT_FD (1 << 0x0D)

/**
 *  filter undefined real-time messages
 */

#define PM_FILT_UNDEFINED PM_FILT_FD

/**
 *  filter reset messages (0xFF)
 */

#define PM_FILT_RESET (1 << 0x0F)

/**
 *  filter all real-time messages
 */

#define PM_FILT_REALTIME (PM_FILT_ACTIVE | PM_FILT_SYSEX | PM_FILT_CLOCK | \
    PM_FILT_PLAY | PM_FILT_UNDEFINED | PM_FILT_RESET | PM_FILT_TICK)

/**
 *  filter note-on and note-off (0x90-0x9F and 0x80-0x8F
 */

#define PM_FILT_NOTE ((1 << 0x19) | (1 << 0x18))

/**
 *  filter channel aftertouch (most midi controllers use this) (0xD0-0xDF)
 */

#define PM_FILT_CHANNEL_AFTERTOUCH (1 << 0x1D)

/**
 *  per-note aftertouch (0xA0-0xAF)
 */

#define PM_FILT_POLY_AFTERTOUCH (1 << 0x1A)

/**
 *  filter both channel and poly aftertouch
 */

#define PM_FILT_AFTERTOUCH (PM_FILT_CHANNEL_AFTERTOUCH | PM_FILT_POLY_AFTERTOUCH)

/**
 *  Program changes (0xC0-0xCF)
 */

#define PM_FILT_PROGRAM (1 << 0x1C)

/**
 *  Control Changes (CC's) (0xB0-0xBF)
 */

#define PM_FILT_CONTROL (1 << 0x1B)

/**
 *  Pitch Bender (0xE0-0xEF
 */

#define PM_FILT_PITCHBEND (1 << 0x1E)

/**
 *  MIDI Time Code (0xF1)
 */

#define PM_FILT_MTC (1 << 0x01)

/**
 *  Song Position (0xF2)
 */

#define PM_FILT_SONG_POSITION (1 << 0x02)

/**
 *  Song Select (0xF3)
 */

#define PM_FILT_SONG_SELECT (1 << 0x03)

/**
 *  Tuning request (0xF6)
 */

#define PM_FILT_TUNE (1 << 0x06)

/**
 *  All System Common messages (mtc, song position, song select, tune request)
 */

#define PM_FILT_SYSTEMCOMMON (PM_FILT_MTC | PM_FILT_SONG_POSITION | PM_FILT_SONG_SELECT | PM_FILT_TUNE)

PMEXPORT PmError Pm_SetFilter (PortMidiStream * stream, int32_t filters);

#define Pm_Channel(channel)         (1 << (channel))

PMEXPORT PmError Pm_SetChannelMask (PortMidiStream * stream, int mask);
PMEXPORT PmError Pm_Abort (PortMidiStream * stream);
PMEXPORT PmError Pm_Close (PortMidiStream * stream);
PMEXPORT int Pm_Read
(
    PortMidiStream * stream, PmEvent * buffer, int32_t length
);
PMEXPORT PmError Pm_Synchronize (PortMidiStream * stream);
PMEXPORT PmError Pm_Poll (PortMidiStream * stream);
PMEXPORT PmError Pm_Write
(
    PortMidiStream * stream, PmEvent * buffer, int32_t length
);
PMEXPORT PmError Pm_WriteShort
(
    PortMidiStream * stream, PmTimestamp when, int32_t msg
);
PMEXPORT PmError Pm_WriteSysEx
(
    PortMidiStream * stream, PmTimestamp when, midibyte_t * msg
);

/*
 * New section for accessing static options in the portmidi module.
 */

PMEXPORT void Pm_set_exit_on_error (int flag);
PMEXPORT int Pm_exit_on_error (void);
PMEXPORT void Pm_set_show_debug (int flag);
PMEXPORT int Pm_show_debug (void);
PMEXPORT void Pm_set_error_present (int flag);
PMEXPORT int Pm_error_present (void);
PMEXPORT void Pm_set_hosterror_message (const char * msg);
PMEXPORT const char * Pm_hosterror_message (void);
PMEXPORT int Pm_device_opened (int deviceid);
PMEXPORT int Pm_device_count (void);
PMEXPORT void Pm_print_devices (void);

#ifdef __cplusplus
}           // extern "C"
#endif

#endif      // SEQ64_PORTMIDI_H

/*
 * pminternal.h
 *
 * vim: sw=4 ts=4 wm=4 et ft=c
 */

