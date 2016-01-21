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
 * \updates       2016-01-21
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
 *  Let's try using lighter solid lines in the piano rolls and see how it
 *  looks.  It looks a little better.
 */

#define SEQ64_SOLID_PIANOROLL_GRID

/**
 *  No global buss override is in force if the global buss override number is
 *  this value (-1).
 */

#define SEQ64_BAD_BUSS                  (char(-1))

/**
 *  An easier macro for testing SEQ64_BAD_BUSS.
 */

#define SEQ64_NO_BUSS_OVERRIDE(b)       (char(b) == SEQ64_BAD_BUSS)

/**
 *  Default value for c_max_sets.
 */

#define SEQ64_DEFAULT_SET_MAX            32

/**
 *  Default value for c_max_busses.
 */

#define SEQ64_DEFAULT_BUSS_MAX           32

/**
 *  Guessing that this has to do with the width of the performance piano roll.
 *  See perfroll::init_before_show().
 */

#define SEQ64_PERFROLL_PAGE_FACTOR      4096

/**
 *  Guessing that this describes the number of subdivisions of the grid in a
 *  beat on the perfroll user-interace.  Changing this doesn't change anything
 *  obvious in the user-interface, though.
 */

#define SEQ64_PERFROLL_DIVS_PER_BEAT      16

/**
 *  Default number of rows in the main-window's grid.
 */

#define SEQ64_DEFAULT_MAINWND_ROWS         4

/**
 *  Default number of columns in the main-window's grid.
 */

#define SEQ64_DEFAULT_MAINWND_COLUMNS      8

/**
 *  Default number of sequences in a set, controlled by the number of rows and
 *  columns in the main window.
 */

#define SQ64_DEFAULT_SEQS_IN_SET \
   (SEQ64_DEFAULT_MAINWND_ROWS * SEQ64_DEFAULT_MAINWND_COLUMNS)

/**
 *  This constant indicates that a configuration file numeric value is
 *  the default value for specifying that an instrument is a GM
 *  instrument.  Used in the "user" configuration-file processing.
 */

#define SEQ64_GM_INSTRUMENT_FLAG        (-1)

/**
 *  This value indicates to use the default value of PPQN and ignore (to some
 *  extent) what value is specified in the MIDI file.  Note that the default
 *  default PPQN is given by the global ppqn (192) or, if the "--ppqn qn"
 *  option is specified on the command-line, by the global ppqn = qn.
 */

#define SEQ64_USE_DEFAULT_PPQN          (-1)

/**
 *  Default value for the global parts-per-quarter-note value.  This is
 *  the unit of time for delta timing.  It represents the units, ticks, or
 *  pulses per beat.  Note that we're migrating this global value into the
 *  user_settings class.
 */

#define SEQ64_DEFAULT_PPQN              192

/**
 *  Minimum value for PPQN.  Mostly for sanity checking.
 */

#define SEQ64_MINIMUM_PPQN               96

/**
 *  Maximum value for PPQN.  Mostly for sanity checking, with higher values
 *  possibly useful for debugging..
 */

#define SEQ64_MAXIMUM_PPQN              19200       /* 960  */

/**
 *  Default value for c_beats_per_minute (global beats-per-minute, also known
 *  as "BPM").  Do not confuse this "bpm" with the other one, "beats per
 *  measure".
 */

#define SEQ64_DEFAULT_BPM               120

/**
 *  Minimum value for c_beats_per_minute (global beats-per-minute, also known
 *  as "BPM").  Mostly for sanity-checking, with extra low values allowed for
 *  debugging and troubleshooting.
 */

#define SEQ64_MINIMUM_BPM                 2         /* 20   */

/**
 *  Maximum value for c_beats_per_minute (global beats-per-minute, also known
 *  as "BPM").  Mostly for sanity-checking.
 */

#define SEQ64_MAXIMUM_BPM               500

/**
 *  Default value for "beats-per-measure".  This is the "numerator" in a 4/4
 *  time signature.  It also seems to be the value used for JACK's
 *  jack_position_t.beats_per_bar field.  For abbreviation, we will call this
 *  value "BPB", or "beats per bar", to distinguish it from "BPM", or "beats
 *  per minute".
 */

#define SEQ64_DEFAULT_BEATS_PER_MEASURE   4

/**
 *  Default value for "beat-width".  This is the "denominator" in a 4/4 time
 *  signature.  It also seems to be the value used for JACK's
 *  jack_position_t.beat_type field. For abbreviation, we will call this value
 *  "BW", or "beat width", not to be confused with "bandwidth".
 */

#define SEQ64_DEFAULT_BEAT_WIDTH          4

/**
 *  Default value for major divisions per bar.  A graphics version of
 *  DEFAULT_BEATS_PER_MEASURE.
 */

#define SEQ64_DEFAULT_LINES_PER_MEASURE   4

/**
 *  Default value for perfedit snap.
 */

#define SEQ64_DEFAULT_PERFEDIT_SNAP       8

/**
 *  Default value for c_thread_trigger_width_ms.
 */

#define SEQ64_DEFAULT_TRIGWIDTH_MS        4

/**
 *  Default value for c_thread_trigger_width_ms.
 */

#define SEQ64_DEFAULT_TRIGLOOK_MS         2

/**
 *  Defines the maximum number of MIDI values, and one more than the
 *  highest MIDI value, which is 127.
 */

#define SEQ64_MIDI_COUNT_MAX            128

/**
 *  Defines the maximum number of notes playing at one time that the
 *  application will support.
 */

#define SEQ64_MIDI_NOTES_MAX            256

#endif      // SEQ64_APP_LIMITS_H

/*
 * app_limits.h
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

