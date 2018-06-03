#ifndef SEQ64_GLOBALS_H
#define SEQ64_GLOBALS_H

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
 * \file          globals.h
 *
 *  This module declares/defines just some of the global (gasp!) variables
 *  and functions used in this application.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-25
 * \updates       2018-05-27
 * \license       GNU GPLv2 or above
 *
 *  We're going to try to collect all the globals here in one module, and
 *  try to group them into functional units.
 *
 *  This collection of global variables describes some facets of the
 *  "Patterns Panel" or "Sequences Window", which is visually presented by
 *  the Gtk::Window-derived class called mainwnd.
 *
 *  The Patterns Panel contains an 8-by-4 grid of "pattern boxes" or
 *  "sequence boxes".  All of the patterns in this grid comprise what is
 *  called a "set" (in the musical sense) or a "screen set".
 *
 *  Other elements are also set by some of these variables.  See the
 *  spreadsheet in <code>contrib/sequence24-classes.ods</code>.
 *
 * \note
 *      This set of variables would be better off placed in a object that the
 *      mainwnd class and its clients can access a little more safely and with
 *      a lot more clarity for the human reader.  We are in the process of
 *      adding such support to the user_settings module.
 *
 *  A couple of universal helper functions remain as inline functions in the
 *  module.  The rest have been moved to the calculations module.
 *
 *  Also note that this file really is a C++ header file, and should have
 *  the "hpp" file extension.  We will fix that Real Soon Now.
 *
 * Stazed:
 *
 *      Some additional variables have been added for supporting stazed/seq32
 *      features.  However, the following will note be added:
 *
 *      -   c_seq32_midi and c_song_midi:  Both the Save and Export functions
 *          write files that are standard MIDI files (with SeqSpec sections)
 *          that any sequencer should be able to read.
 *      -   global_is_running and global_is_modified: These statuses are
 *          now maintained in the perform object, instead of globally.
 */

#include <string>

#include "app_limits.h"                 /* basic hardwired app limits   */
#include "easy_macros.h"                /* with platform_macros.h, too  */

/**
 *  Define this macro in order to enable some verbose console output from
 *  various modules.  Undefine it (using "#undef") to disable this extra
 *  output. Note that this macro can be enabled only while a debug build is in
 *  force.  Also, enabling it hear might cause an explosion of output.
 */

#undef  SEQ64_USE_DEBUG_OUTPUT          /* off by default... TMI        */

/*
 *  Default number of rows in the Patterns Panel.  The current default value
 *  is 4, and probably won't change, until no other values depend on it, as we
 *  support a varying number of rows.  Together with c_mainwnd_cols, this
 *  value fixes the patterns grid into a 4 x 8 set of patterns known as a
 *  "screen set", but now, only by default.
 *
 *      const int c_mainwnd_rows = SEQ64_DEFAULT_MAINWND_ROWS;
 */

/*
 *  Default umber of columns in the Patterns Panel.  The current default value
 *  is 8, and probably won't change, until no other values depend on it.
 *  Together with c_mainwnd_rows, this value fixes the patterns grid into a 4
 *  x 8 set of patterns known as a "screen set", but now, only by default.
 *
 *      const int c_mainwnd_cols = SEQ64_DEFAULT_MAINWND_COLUMNS;
 */

/**
 *  Number of patterns/sequences in the Patterns Panel, also known as a "set"
 *  or "screen set".  This value is 4 x 8 = 32 by default.  We have a few
 *  arrays that are allocated to this size, at present. Was c_mainwnd_rows *
 *  c_mainwnd_cols.  This value is now a variable in most contexts.  However,
 *  it is still important in saving and retrieving the [mute-group] section,
 *  which still relies on the old value of 32 patterns/set.
 */

const int c_seqs_in_set = SEQ64_DEFAULT_SEQS_IN_SET;

/**
 *  Maximum number of screen sets that can be supported.  Basically, the
 *  number of times the Patterns Panel can be filled.  32 sets can be created.
 */

const int c_max_sets = SEQ64_DEFAULT_SET_MAX;

/**
 *  Maximum number of set keys that can be supported.  32 keys can be assigned
 *  in the Options / Keyboard tab and "rc" file.  This value applies to the
 *  "[keyboard-group]" and "[keyboard-control]" sections.
 */

const int c_max_keys = SEQ64_SET_KEYS_MAX;

/**
 *  Maximum number of groups that can be supported.  Basically, the number of
 *  groups set in the "rc" file.  32 groups can be filled.
 */

const int c_max_groups = SEQ64_DEFAULT_GROUP_MAX;

/**
 *  The maximum number of patterns supported is given by the number of
 *  patterns supported in the panel (32) times the maximum number of sets
 *  (32), or 1024 patterns.  However, this value is now independent of the
 *  maximum number of sets and the number of sequences in a set.  Instead,
 *  we limit them to a constant value, which seems to be well above the
 *  number of simultaneous playing sequences the application can support.
 *  See SEQ64_SEQUENCE_MAXIMUM.
 */

const int c_max_sequence = SEQ64_SEQUENCE_MAXIMUM;

/*
 *  Number of group-mute tracks that can be supported, which is
 *  c_seqs_in_set squared, or 1024.  This value is the same size as
 *  c_max_sequence, and actually conceptually the same value (it covers all
 *  sequences), and so we're going to optimize this value out.
 *
 * const int c_gmute_tracks = c_max_sets * c_seqs_in_set;
 */

/**
 *  Provides the default number BPM (beats per minute), which describes
 *  the overall speed at which the sequencer will play a tune.  The
 *  default value is 120.
 */

const int c_beats_per_minute = SEQ64_DEFAULT_BPM;

/**
 *  The trigger width in milliseconds.  This value is 4 ms.
 */

const int c_thread_trigger_width_ms = SEQ64_DEFAULT_TRIGWIDTH_MS;

/**
 *  The trigger width in microseconds.  This value is 4000 us.  Makes the code
 *  just a teensy bit tighter.
 */

const int c_thread_trigger_width_us = SEQ64_DEFAULT_TRIGWIDTH_MS * 1000;

/*
 *  The trigger lookahead in milliseconds.  This value is 2 ms.  Not used
 *  anywhere, so commented out.
 *
 *  const int c_thread_trigger_lookahead_ms = SEQ64_DEFAULT_TRIGLOOK_MS;
 */

/**
 *  Constants for the font class.  The c_text_x and c_text_y constants
 *  help define the "seqarea" size.  It looks like these two values are
 *  the character width (x) and height (y) in pixels.  Thus, these values
 *  would be dependent on the font chosen.  But that, currently, is
 *  hard-wired.  See the c_font_6_12[] array for the default font
 *  specification.
 *
 *  However, please note that font files are not used.  Instead, the fonts
 *  are provided by pixmaps in the <code> src/pixmap </code> directory.
 *  These pixmaps lay out all the characters of the font in a grid.
 *  See the font module for a full description of this grid.
 */

const int c_text_x =  6;            /* does not include the inner padding   */
const int c_text_y = 12;            /* does include the inner padding       */

/**
 *  Constants for the mainwid class.  The c_seqchars_x and c_seqchars_y
 *  constants help define the "seqarea" size.  These look like the number
 *  of characters per line and the number of lines of characters, in a
 *  pattern/sequence box.
 */

const int c_seqchars_x = 15;
const int c_seqchars_y =  5;

/**
 *  The c_seqarea_x and c_seqarea_y constants are derived from the width
 *  and heights of the default character set, and the number of characters
 *  in width, and the number of lines, in a pattern/sequence box.
 *
 *  Compare these two constants to c_seqarea_seq_x(y), which was in
 *  mainwid.h, but is now in this file.
 */

const int c_seqarea_x = c_text_x * c_seqchars_x;
const int c_seqarea_y = c_text_y * c_seqchars_y;

/**
 * Area of what?  Doesn't look at all like it is based on the size of
 * characters.  These are used only in the mainwid module.
 */

const int c_seqarea_seq_x = c_text_x * 13;
const int c_seqarea_seq_y = c_text_y * 2;

/**
 *  These control sizes.  We'll try changing them and see what happens.
 *  Increasing these value spreads out the pattern grids a little bit and
 *  makes the Patterns panel slightly bigger.  Seems like it would be
 *  useful to make these values user-configurable.
 */

const int c_mainwid_border = 0;             // try 2 or 3 instead of 0
const int c_mainwid_spacing = 2;            // try 4 or 6 instead of 2

/**
 *  This constants seems to be created for a future purpose, perhaps to
 *  reserve space for a new bar on the mainwid pane.  But it is used only
 *  in this header file, to define c_mainwid_y, but doesn't add anything
 *  to that value.
 */

const int c_control_height = 0;

/**
 *  The height of the data-entry area for velocity, aftertouch, and other
 *  controllers, as well as note on and off velocity.  This value looks to
 *  be in pixels; one pixel per MIDI value, which ranges from 0 to 127.
 *  We're trying to avoid header clutter, and are using a hardwired constant
 *  for this variable, which will eventually go away.
 */

const int c_dataarea_y = 128;

/**
 *  The width of the 'bar', presumably the line that ends a measure, in
 *  pixels.
 */

const int c_data_x = 2;

/**
 *  The width of the seqdata data-handle, a stazed feature.
 */

const int c_data_handle_x = 8;

/**
 *  The height of the seqdata data-handle, a stazed feature.
 */

const int c_data_handle_y = 4;

/**
 *  The dimensions of each key of the virtual keyboard at the left of the
 *  piano roll.
 */

const int c_key_x = 16;
const int c_key_y =  8;

/**
 *  The number of MIDI keys, as well as keys in the virtual keyboard.
 *  Note that only a subset of the virtual keys will be shown; one must
 *  scroll to see them all.
 */

const int c_num_keys = SEQ64_MIDI_COUNT_MAX;      // 128

/**
 *  The dimensions and offset of the virtual keyboard at the left of the
 *  piano roll.
 */

/*
 * Replaced by usr().key_height():  const int c_key_height = 12
 */

const int c_keyarea_x = 36;
const int c_keyoffset_x = c_keyarea_x - c_key_x;
const int c_keyarea_y = c_key_y * c_num_keys + 1;

/**
 *  The height of the piano roll is the same as the height of the virtual
 *  keyboard area.  Note that only a subset of the piano roll will be
 *  shown; one must scroll to see it all.
 */

const int c_rollarea_y = c_keyarea_y;

/**
 *  The dimensions of the little rectangles, in pixels, that represent the
 *  position of each event.
 */

const int c_eventevent_x =  5;
const int c_eventevent_y = 10;

/**
 *  A new constant that presents the padding above and below an event
 *  rectangle, in pixels.
 */

const int c_eventpadding_y = 3;

/**
 *  The height of the events bar, which shows the little squares that
 *  represent the position of each event.  This value was 16, but we've
 *  broken out its components.
 */

const int c_eventarea_y = c_eventevent_y + 2 * c_eventpadding_y;

/**
 *  The height of the time scale window on top of the piano roll, in pixels.
 *  It is slightly thicker than the event-area.
 */

const int c_timearea_y = 18;

/**
 *  The number of MIDI notes in what?  This value is used in the sequence
 *  module.  It looks like it is the maximum number of notes that
 *  seq24/sequencer64 can have playing at one time.  In other words,
 *  "only" 256 simultaneously-playing notes can be managed.
 */

const int c_midi_notes = SEQ64_MIDI_NOTES_MAX;

/**
 *  Provides the default string for the name of a pattern or sequence.
 *  Now a static member of sequence, under the name m_default_name.
 *
 *      const std::string c_dummy = "Untitled";
 */

/**
 *  Provides the maximum size of sequence, and the default size.
 *  It is the maximum number of beats in a sequence.  It's about 8190
 *  measures or bars.  A short song (4 minutes) might be about a 100 bars.
 */

const int c_maxbeats = 0xFFFF;

/**
 *  Values used in the menu to tell setState() what to do.
 */

const int c_adding = 0;
const int c_normal = 1;
const int c_paste  = 2;

/**
 *  Provides the redraw time when recording, in ms.  Can Windows actually
 *  draw faster? :-D
 */

#ifdef PLATFORM_WINDOWS
const int c_redraw_ms = 20;
#else
const int c_redraw_ms = 40;
#endif

/**
 *  Provides constants for the perfroll object (performance editor).
 *  Note the current dependence on the width of a font pixmap's character!
 *  So we will use the font's numeric accessors soon.
 */

const int c_names_x = 6 * 24;           /* width of name box, 24 characters */
const int c_names_y = 24;               /* max height of name box, pixels   */
const int c_perf_scale_x = 32;          /* units are ticks per pixel        */
const int c_perf_max_zoom = 8;          /* limit the amount of perf zoom    */

#endif      // SEQ64_GLOBALS_H

/*
 * globals.h
 *
 * vim: sw=4 ts=4 wm=4 et ft=c
 */

