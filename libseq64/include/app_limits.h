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
 * \updates       2016-07-31
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

/**
 *  Determins which implementation of a MIDI byte container is used.
 *  See the midifile module.
 */

#define SEQ64_USE_MIDI_VECTOR           /* as opposed to the MIDI list      */

/**
 *  Let's try using lighter solid lines in the piano rolls and see how it
 *  looks.  It looks a little better.
 */

#define SEQ64_SOLID_PIANOROLL_GRID

/**
 *  This provides a build option for having the pattern editor window scroll
 *  to keep of with the progress bar, for sequences that are longer than the
 *  measure or two that a pattern window will show.
 *
 *  We thought about making this a configure option or a run-time option, but
 *  this kind of scrolling is a universal convention of MIDI sequencers.  If
 *  you really don't like this feature, let me know, and I will make it a
 *  configure option.  We could also disable it it "legacy" mode, which also
 *  disables a lot of other features.
 *
 * \warning
 *      This code might still have issues with interactions between triggers
 *      and gaps in the performance (song) window when JACK transport is
 *      active.  Still investigating.
 */

#define SEQ64_FOLLOW_PROGRESS_BAR

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
 *  Default value for c_max_sets.
 */

#define SEQ64_DEFAULT_SET_MAX             32

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
 *  Default value for c_beats_per_minute (global beats-per-minute, also known
 *  as "BPM").  Do not confuse this "bpm" with the other one, "beats per
 *  measure".
 */

#define SEQ64_DEFAULT_BPM                120

/**
 *  Minimum value for c_beats_per_minute (global beats-per-minute, also known
 *  as "BPM").  Mostly for sanity-checking, with extra low values allowed for
 *  debugging and troubleshooting.
 */

#define SEQ64_MINIMUM_BPM                  1         /* 20   */

/**
 *  Maximum value for c_beats_per_minute (global beats-per-minute, also known
 *  as "BPM").  Mostly for sanity-checking.
 */

#define SEQ64_MAXIMUM_BPM                600         /* 500  */

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
 *  Defines the maximum Note On velocity.
 */

#define SEQ64_MAX_NOTE_ON_VELOCITY       127

/**
 *  An older value, previously used for both Note On and Note Off velocity.
 *  See the "Stazed" note in the sequence::add_note() function.
 */

#define SEQ64_DEFAULT_NOTE_VELOCITY      100

/**
 *  Defines the default Note On velocity, a new "stazed" feature.
 */

#define SEQ64_DEFAULT_NOTE_ON_VELOCITY   100

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

