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
 * \author        Chris Ahlstrom
 * \date          2015-11-08
 * \updates       2017-06-04
 * \license       GNU GPLv2 or above
 *
 *  This collection of macros describes some facets of the
 *  "Patterns Panel" or "Sequences Window", which is visually presented by
 *  the Gtk::Window-derived class called mainwnd.
 *
 *  The Patterns Panel contains an 8-by-4 grid of "pattern boxes" or
 *  "sequence boxes".  All of the patterns in this grid comprise what is
 *  called a "set" (in the musical sense) or a "screen set".
 *
 *  These macros also specify other parameters, as well.
 *
 *  Why do we use macros instead of const values?  First, it really doesn't
 *  matter for simple values.  Second, we want to follow the convention that
 *  important values are all upper-case, as is convention with most
 *  macros.  They just stand out more in code. Call us old school or old
 *  fools, you decide.  Hell, we still like snprintf() for some uses!
 */

#include "seq64_features.h"             /* make sure of the feature macros  */

/**
 *  This macro defines the amount of overlap between horizontal "pages" that
 *  get scrolled to follow the progress bar.  We think it should be greater
 *  than 0, maybe set to 10. But feel free to experiment.
 */

#define SEQ64_PROGRESS_PAGE_OVERLAP       10

/**
 *  Indicates the maximum number of MIDI channels, counted internally from 0
 *  to 15, and by humans (sequencer user-interfaces) from 1 to 16.
 */

#define SEQ64_MIDI_CHANNEL_MAX            16

/**
 *  Minimum value for c_max_sets.  The actual maximum number of sets will be
 *  reduced if we add rows (or columns) to each mainwid grid.  This is
 *  actually a derived value, but we still support a macro for it.
 */

#define SEQ64_MIN_SET_MAX                 16

/**
 *  Default (and maximum) value for c_max_sets.  The actual maximum number of
 *  sets will be reduced if we add rows (or columns) to each mainwid grid.
 *  This is actually a derived value, but we still support a macro for it.
 */

#define SEQ64_DEFAULT_SET_MAX             32

/**
 *  Default value for c_seqs_in_set.
 */

#define SEQ64_DEFAULT_SEQS_IN_SET \
    (SEQ64_DEFAULT_MAINWND_ROWS * SEQ64_DEFAULT_MAINWND_COLUMNS)

/**
 *  Default value for c_max_groups.  This value replaces c_seqs_in_set for
 *  usage in obtaining mute-group information from the "rc" file.  Its value
 *  is only "coincidentally" equal to 32.
 */

#define SEQ64_DEFAULT_GROUP_MAX           32

/**
 *  Defines the constant number of sequences/patterns.  This value has
 *  historically been 1024, which is 32 patterns per set times 32 sets.  But
 *  we don't want to support any more than this value, based on trials with
 *  the b4uacuse-stress.midi file, which has only about 4 sets (128 patterns)
 *  and pretty much loads up a CPU.
 */

#define SEQ64_SEQUENCE_MAXIMUM          1024

/**
 *  Default value of number of slot toggle keys (shortcut keys) that
 *  can be defined.  Even if we end up adding more slots to a set, this
 *  would be about the maximum number of keys we could really support.
 */

#define SEQ64_SET_KEYS_MAX                32

/**
 *  Default value of the width (number of columns) of the slot toggle keys.
 *  Again, this matches with number of columns in a set in the main window of
 *  the application.
 */

#define SEQ64_SET_KEYS_COLUMNS             8

/**
 *  Defines a sequence that is invalid and cannot be used.
 */

#define SEQ64_NULL_SEQUENCE              (-1)

/**
 *  No global buss override is in force if the global buss override number is
 *  this value (-1).
 */

#ifdef __cplusplus
#define SEQ64_BAD_BUSS                    (char(-1))
#else
#define SEQ64_BAD_BUSS                    ((char)(-1))
#endif

/**
 *  An easier macro for testing SEQ64_BAD_BUSS.
 */

#define SEQ64_NO_BUSS_OVERRIDE(b)         (char(b) == SEQ64_BAD_BUSS)

/**
 *  Default value for c_max_busses.
 */

#define SEQ64_DEFAULT_BUSS_MAX            32

/**
 *  The number of ALSA busses supported.  See mastermidibus::init().
 */

#define SEQ64_ALSA_OUTPUT_BUSS_MAX        16

/**
 *  Flags an unspecified buss number.  Two spellings are provided, one for
 *  youngsters and one for old men.  :-D
 */

#define SEQ64_NO_BUS                    (-1)
#define SEQ64_NO_BUSS                   (-1)
#define SEQ64_BAD_BUS_ID                (unsigned(-1))

/**
 *  Flags an unspecified port number, or indicates a bad client ID or port
 *  number.
 */

#define SEQ64_NO_PORT                   (-1)
#define SEQ64_BAD_PORT_ID               (unsigned(-1))

/**
 *  Flags an unspecified queue number.
 */

#define SEQ64_NO_QUEUE                  (-1)
#define SEQ64_BAD_QUEUE_ID              (unsigned(-1))

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
 *  Minimum number of rows in the main-window's grid.  This will remain
 *  the same as the default number of rows; we will not reduce the number of
 *  sequences per set, at least at this time.
 */

#define SEQ64_MIN_MAINWND_ROWS             4

/**
 *  Maximum number of rows in the main-window's grid.  With the default number
 *  of columns, this will double the number of sequences per set from 32 to
 *  64, hence the name "seq64".
 */

#define SEQ64_MAX_MAINWND_ROWS             8

/**
 *  Default number of columns in the main-window's grid.
 */

#define SEQ64_DEFAULT_MAINWND_COLUMNS      8

/**
 *  Minimum number of columns in the main-window's grid.  Currently the same
 *  as the default number.  We currently cannot support more sets than 32,
 *  which would happen if we let rows or columns go below the default 4 x 8
 *  settings.
 */

#define SEQ64_MIN_MAINWND_COLUMNS          8

/**
 *  Maximum number of columns in the main-window's grid.  Currently the same
 *  as the default number.
 */

#define SEQ64_MAX_MAINWND_COLUMNS          12   // 8

#if defined SEQ64_MULTI_MAINWID

/**
 *  The maximum number of rows of mainwids we will support, regardless of
 *  screen resolution.
 */

#define SEQ64_MAINWID_BLOCK_ROWS_MAX       3

/**
 *  The maximum number of columns of mainwids we will support, regardless of
 *  screen resolution.
 */

#define SEQ64_MAINWID_BLOCK_COLS_MAX       2

#endif  // SEQ64_MULTI_MAINWID

/**
 *  This constant indicates that a configuration file numeric value is
 *  the default value for specifying that an instrument is a GM
 *  instrument.  Used in the "user" configuration-file processing.
 */

#define SEQ64_GM_INSTRUMENT_FLAG          (-1)

/**
 *  This value indicates to use the default value of PPQN and ignore (to some
 *  extent) what value is specified in the MIDI file.  Note that the default
 *  default PPQN is given by the global ppqn (192) or, if the "--ppqn qn"
 *  option is specified on the command-line, by the global ppqn = qn.
 */

#define SEQ64_USE_DEFAULT_PPQN            (-1)

/**
 *  Default value for the global parts-per-quarter-note value.  This is
 *  the unit of time for delta timing.  It represents the units, ticks, or
 *  pulses per beat.  Note that we're migrating this global value into the
 *  user_settings class.
 */

#define SEQ64_DEFAULT_PPQN               192

/**
 *  Minimum value for PPQN.  Mostly for sanity checking.  This was set to 96,
 *  but there have been tunes set to 32 PPQN, I think.
 */

#define SEQ64_MINIMUM_PPQN                32

/**
 *  Maximum value for PPQN.  Mostly for sanity checking, with higher values
 *  possibly useful for debugging.
 */

#define SEQ64_MAXIMUM_PPQN             19200       /* 960  */

/**
 *  Minimum possible value for zoom, indicating that one pixel represents one
 *  tick.
 */

#define SEQ64_MINIMUM_ZOOM                 1

/**
 *  The default value of the zoom, indicating that one pixel represents two
 *  ticks.  However, it turns out we're going to have to support adapting the
 *  default zoom to the PPQN, in addition to allowing some extra zoom values.
 */

#define SEQ64_DEFAULT_ZOOM                 2

/**
 *  The maximum value of the zoom, indicating that one pixel represents 512
 *  ticks.  The old maximum was 32, but now that we support PPQN up to 19200,
 *  we need a couple of extra entries.
 */

#define SEQ64_MAXIMUM_ZOOM               512

/**
 *  Minimum possible value for the global redraw rate.
 */

#define SEQ64_MINIMUM_REDRAW              10

/**
 *  The default value global redraw rate.
 */

#define SEQ64_DEFAULT_REDRAW              40     /* or 25 for Windows */

/**
 *  The maximum value for the global redraw rate.
 */

#define SEQ64_MAXIMUM_REDRAW             100

/**
 *  Defines the callback rate for gtk_timeout_add() as used by perfedit.
 *  As usual, this value is in milliseconds.
 */

#define SEQ64_FF_RW_TIMEOUT              120

/**
 *  Defines a scale value for BPM so that we can store a higher-precision
 *  version of it in the proprietary "bpm" section.  See the midifile class
 *  for more information.
 */

#define SEQ64_BPM_SCALE_FACTOR          1000.0

/**
 *  Default value for c_beats_per_minute (global beats-per-minute, also known
 *  as "BPM").  Do not confuse this "bpm" with the other one, "beats per
 *  measure".
 */

#define SEQ64_DEFAULT_BPM                120.0

/**
 *  Minimum value for c_beats_per_minute (global beats-per-minute, also known
 *  as "BPM").  Mostly for sanity-checking, with extra low values allowed for
 *  debugging and troubleshooting.
 */

#define SEQ64_MINIMUM_BPM                  1.0       /* 20   */

/**
 *  Maximum value for c_beats_per_minute (global beats-per-minute, also known
 *  as "BPM").  Mostly for sanity-checking.
 */

#define SEQ64_MAXIMUM_BPM                600.0       /* 500  */

/**
 *  Provides a fallback value for the BPM precision.  This is the "legacy"
 *  value.
 */

#define SEQ64_DEFAULT_BPM_PRECISION        0

/**
 *  Provides a minimum value for the BPM precision.  That is, no decimal
 *  point.
 */

#define SEQ64_MINIMUM_BPM_PRECISION        0

/**
 *  Provides a maximum value for the BPM precision, two decimal points.
 */

#define SEQ64_MAXIMUM_BPM_PRECISION        2

/**
 *  Provides a fallback value for the BPM increment.  This is the "legacy"
 *  value.
 */

#define SEQ64_DEFAULT_BPM_INCREMENT        1.0

/**
 *  Provides a minimum value for the BPM increment.
 */

#define SEQ64_MINIMUM_BPM_INCREMENT        0.01

/**
 *  Provides a maximum value for the BPM increment.
 */

#define SEQ64_MAXIMUM_BPM_INCREMENT       25.0

/**
 *  Provides a fallback value for the BPM step increment.  This is the "legacy"
 *  value.
 */

#define SEQ64_DEFAULT_BPM_STEP_INCREMENT   1.0

/**
 *  Provides a fallback value for the BPM page increment.
 */

#define SEQ64_DEFAULT_BPM_PAGE_INCREMENT  10.0

/**
 *  Default value for "beats-per-measure".  This is the "numerator" in a 4/4
 *  time signature.  It also seems to be the value used for JACK's
 *  jack_position_t.beats_per_bar field.  For abbreviation, we will call this
 *  value "BPB", or "beats per bar", to distinguish it from "BPM", or "beats
 *  per minute".
 */

#define SEQ64_DEFAULT_BEATS_PER_MEASURE    4

/**
 *  Default value for "beat-width".  This is the "denominator" in a 4/4 time
 *  signature.  It also seems to be the value used for JACK's
 *  jack_position_t.beat_type field. For abbreviation, we will call this value
 *  "BW", or "beat width", not to be confused with "bandwidth".
 */

#define SEQ64_DEFAULT_BEAT_WIDTH           4

/**
 *  Default value for major divisions per bar.  A graphics version of
 *  SEQ64_DEFAULT_BEATS_PER_MEASURE.
 */

#define SEQ64_DEFAULT_LINES_PER_MEASURE    4

/**
 *  Default value for perfedit snap.
 */

#define SEQ64_DEFAULT_PERFEDIT_SNAP        8

/**
 *  Default value for c_thread_trigger_width_ms.
 */

#define SEQ64_DEFAULT_TRIGWIDTH_MS         4

/*
 *  Default value for c_thread_trigger_width_ms.  Not in use at present.
 *
 *  #define SEQ64_DEFAULT_TRIGLOOK_MS         2
 */

/**
 *  Defines the maximum number of MIDI values, and one more than the
 *  highest MIDI value, which is 17.
 */

#define SEQ64_MIDI_COUNT_MAX             128

/**
 *  Defines the minimum Note On velocity.
 */

#define SEQ64_MIN_NOTE_ON_VELOCITY         0

/**
 *  Defines the default Note On velocity, a new "stazed" feature.
 */

#define SEQ64_DEFAULT_NOTE_ON_VELOCITY   100

/**
 *  Defines the maximum Note On velocity.
 */

#define SEQ64_MAX_NOTE_ON_VELOCITY       127

/**
 *  Indicates to preserve the velocity of incoming MIDI Note events, for both
 *  on or off events.  This value represents the "Free" popup-menu entry for
 *  the "Vol" button in the seqedit window.
 */

#define SEQ64_PRESERVE_VELOCITY         (-1)

/**
 *  Defines the maximum MIDI data value.  This applies to note values as well.
 */

#define SEQ64_MAX_DATA_VALUE             127

/**
 *  An older value, previously used for both Note On and Note Off velocity.
 *  See the "Stazed" note in the sequence::add_note() function.
 */

#define SEQ64_DEFAULT_NOTE_VELOCITY      100

/**
 *  Defines the default Note Off velocity, a new "stazed" feature.
 */

#define SEQ64_DEFAULT_NOTE_OFF_VELOCITY   64

/**
 *  Defines the maximum number of notes playing at one time that the
 *  application will support.
 */

#define SEQ64_MIDI_NOTES_MAX             256

/**
 *  Provides a sanity check for transposition values.
 */

#define SEQ64_TRANSPOSE_UP_LIMIT        (SEQ64_MIDI_COUNT_MAX / 2)
#define SEQ64_TRANSPOSE_DOWN_LIMIT      (-SEQ64_MIDI_COUNT_MAX / 2)

#endif      // SEQ64_APP_LIMITS_H

/*
 * app_limits.h
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

