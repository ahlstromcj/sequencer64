#ifndef SEQ64_APP_LIMITS_H
#define SEQ64_APP_LIMITS_H

/*
 *  This file is part of seq24/sequencer64.
 *
 *  seq24 is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  seq24 is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with seq24; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * \file          app_limits.h
 *
 *  This module holds macro constants related to the application limits of
 *  Sequencer64.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-11-08
 * \updates       2015-11-08
 * \license       GNU GPLv2 or above
 *
 *  This collection of global variables describes some facets of the
 *  "Patterns Panel" or "Sequences Window", which is visually presented by
 *  the Gtk::Window-derived class called mainwnd.
 *
 *  The Patterns Panel contains an 8-by-4 grid of "pattern boxes" or
 *  "sequence boxes".  All of the patterns in this grid comprise what is
 *  called a "set" (in the musical sense) or a "screen set".
 *
 */

/**
 *  No global buss override is in force if the global buss override number is
 *  this value (-1).
 */

#define SEQ64_BAD_BUSS                  (char(-1))
#define NO_BUSS_OVERRIDE(b)             (char(b) == SEQ64_BAD_BUSS)

/**
 *  Default value for c_max_busses.
 */

#define DEFAULT_BUSS_MAX                 32

/**
 *  Guessing that this has to do with the width of the performance piano roll.
 *  See perfroll::init_before_show().
 */

#define PERFROLL_PAGE_FACTOR            4096

/**
 *  Guessing that this describes the number of subdivisions of the grid in a
 *  beat on the perfroll user-interace.  Changing this doesn't change anything
 *  obvious in the user-interface, though.
 */

#define PERFROLL_DIVS_PER_BEAT            16

/**
 *  This constant indicates that a configuration file numeric value is
 *  the default value for specifying that an instrument is a GM
 *  instrument.  Used in the "user" configuration-file processing.
 */

#define GM_INSTRUMENT_FLAG              (-1)

/**
 *  Default value for the global parts-per-quarter-note value.  This is
 *  the unit of time for delta timing.  It represents the units, ticks, or
 *  pulses per beat.  Note that we're migrating this global value into the
 *  user_settings class.
 */

#define DEFAULT_PPQN                    192

/**
 *  Minimum value for PPQN.  Mostly for sanity checking.
 */

#define MINIMUM_PPQN                     96

/**
 *  Maximum value for PPQN.  Mostly for sanity checking.
 */

#define MAXIMUM_PPQN                    960

/**
 *  Default value for c_beats_per_minute (global beats-per-minute, also known
 *  as "BPM").  Do not confuse this "bpm" with the other one, "beats per
 *  measure".
 */

#define DEFAULT_BPM                     120

/**
 *  Minimum value for c_beats_per_minute (global beats-per-minute, also known
 *  as "BPM").  Mostly for sanity-checking.
 */

#define MINIMUM_BPM                      20

/**
 *  Maximum value for c_beats_per_minute (global beats-per-minute, also known
 *  as "BPM").  Mostly for sanity-checking.
 */

#define MAXIMUM_BPM                     500

/**
 *  Default value for "beats-per-measure".  This is the "numerator" in a 4/4
 *  time signature.  It also seems to be the value used for JACK's
 *  jack_position_t.beats_per_bar field.  For abbreviation, we will call this
 *  value "BPB", or "beats per bar", to distinguish it from "BPM", or "beats
 *  per minute".
 */

#define DEFAULT_BEATS_PER_MEASURE         4

/**
 *  Default value for "beat-width".  This is the "denominator" in a 4/4 time
 *  signature.  It also seems to be the value used for JACK's
 *  jack_position_t.beat_type field. For abbreviation, we will call this value
 *  "BW", or "beat width", not to be confused with "bandwidth".
 */

#define DEFAULT_BEAT_WIDTH                4

/**
 *  Default value for major divisions per bar.  A graphics version of
 *  DEFAULT_BEATS_PER_MEASURE.
 */

#define DEFAULT_LINES_PER_MEASURE         4

/**
 *  Default value for perfedit snap.
 */

#define DEFAULT_PERFEDIT_SNAP             8

/**
 *  Default value for c_thread_trigger_width_ms.
 */

#define DEFAULT_TRIGWIDTH_MS              4

/**
 *  Default value for c_thread_trigger_width_ms.
 */

#define DEFAULT_TRIGLOOK_MS               2

/**
 *  Defines the maximum number of MIDI values, and one more than the
 *  highest MIDI value, which is 127.
 */

#define MIDI_COUNT_MAX                  128

#endif      // SEQ64_APP_LIMITS_H

/*
 * app_limits.h
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

