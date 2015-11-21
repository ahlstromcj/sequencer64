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
 * \updates       2015-11-21
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
 *  spreadsheet in <tt>contrib/sequence24-classes.ods</tt>.
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
 */

#include <string>

#include "app_limits.h"                 /* basic hardwired app limits   */
#include "easy_macros.h"                /* with platform_macros.h, too  */
#include "rc_settings.hpp"              /* seq64::rc_settings           */
#include "user_settings.hpp"            /* seq64::user_settings         */

namespace seq64
{

/**
 *  Returns a reference to the global rc_settings and user_settings objects.
 *  Why a function instead of direct variable access?  Encapsulation.  We are
 *  then free to change the way "global" settings are accessed, without
 *  changing client code.
 */

extern rc_settings & rc ();
extern user_settings & usr ();

}           // namespace seq64

/**
 *  Define this macro in order to enable some verbose console output from
 *  various modules.  Undefine it (using "#undef") to disable this extra
 *  output Note that this macro can be enabled only while a debug build is in
 *  force.
 */

#ifdef PLATFORM_DEBUG
#undef  SEQ64_USE_DEBUG_OUTPUT          /* off by default... TMI        */
#endif

/**
 *  Number of rows in the Patterns Panel.  The current value is 4, and
 *  probably won't change, since other values depend on it.  Together with
 *  c_mainwnd_cols, this value fixes the patterns grid into a 4 x 8 set of
 *  patterns known as a "screen set".
 */

const int c_mainwnd_rows = SEQ64_DEFAULT_MAINWND_ROWS;

/**
 *  Number of columns in the Patterns Panel.  The current value is 4, and
 *  probably won't change, since other values depend on it.  Together with
 *  c_mainwnd_rows, this value fixes the patterns grid into a 4 x 8 set of
 *  patterns known as a "screen set".
 */

const int c_mainwnd_cols = SEQ64_DEFAULT_MAINWND_COLUMNS;

/**
 *  Number of patterns/sequences in the Patterns Panel, also known as a
 *  "set" or "screen set".  This value is 4 x 8 = 32 by default.
 */

const int c_seqs_in_set = c_mainwnd_rows * c_mainwnd_cols;

/**
 *  Number of group-mute tracks that can be support, which is
 *  c_seqs_in_set squared, or 1024.
 */

const int c_gmute_tracks = c_seqs_in_set * c_seqs_in_set;

/**
 *  Maximum number of screen sets that can be supported.  Basically, that
 *  the number of times the Patterns Panel can be filled.  32 sets can be
 *  created.
 */

const int c_max_sets = SEQ64_DEFAULT_SET_MAX;

/**
 *  The maximum number of patterns supported is given by the number of
 *  patterns supported in the panel (32) times the maximum number of sets
 *  (32), or 1024 patterns.
 */

const int c_max_sequence = c_seqs_in_set * c_max_sets;

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
 *  The trigger lookahead in milliseconds.  This value is 2 ms.
 */

const int c_thread_trigger_lookahead_ms = SEQ64_DEFAULT_TRIGLOOK_MS;

/**
 *  Constants for the font class.  The c_text_x and c_text_y constants
 *  help define the "seqarea" size.  It looks like these two values are
 *  the character width (x) and height (y) in pixels.  Thus, these values
 *  would be dependent on the font chosen.  But that, currently, is
 *  hard-wired.  See the c_font_6_12[] array for the default font
 *  specification.
 *
 *  However, please note that font files are not used.  Instead, the fonts
 *  are provided by pixmaps in the <tt> src/pixmap </tt> directory.
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
 *  for this variable, which will eventually go away..
 */

const int c_dataarea_y = 128;

/**
 *  The width of the 'bar', presumably the line that ends a measure, in
 *  pixels.
 */

const int c_data_x = 2;

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
 *  The dimensions of the little squares that represent the position of
 *  each event, in pixels.
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
 */

const std::string c_dummy = "Untitled";

/**
 *  Provides the maximum size of sequence, and the default size.
 *  It is the maximum number of beats in a sequence.  It's about 8190
 *  measures or bars.  A short song (4 minutes) might be about a 100 bars.
 */

const int c_maxbeats = 0xFFFF;

#if USE_TRADITIONAL_FONT_HANDLING

/**
 *  Provides the various font sizes for the default font.  Not yet sure if
 *  there's a mechanism for selecting the font.  No, there is not.
 *  The font is actually fixed, and is embedded in a couple of XPM
 *  pixmaps.
 */

const char c_font_6_12[] = "-*-fixed-medium-r-*--12-*-*-*-*-*-*";
const char c_font_8_13[] = "-*-fixed-medium-r-*--13-*-*-*-*-*-*";
const char c_font_5_7[]  = "-*-fixed-medium-r-*--7-*-*-*-*-*-*";

#endif  // USE_TRADITIONAL_FONT_HANDLING

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

/**
 *  New variables starting from 2015-08-16 onward.  These variables
 *  replace or augment the "c_" (constant global variables).  Using new names
 *  makes it easier to find old usage, and some of these values will no
 *  longer be hardwired constant values.  They will be replaced/augmented by
 *  new members and accessors in the user_settings class.
 *
 *  There are a lot of places in the code where obscure manifest constants,
 *  such as "4", are used, without comment.  Usage of named values is a lot
 *  more informative, but takes some time to reverse-engineer.
 */

extern bool global_priority;
extern bool global_stats;
extern bool global_manual_alsa_ports;
extern bool global_print_keys;
extern bool global_device_ignore;            /* seq24 module    */
extern int global_device_ignore_num;         /* seq24 module    */

/**
 *  Mouse actions, for the Pattern Editor.  Be sure to update seq24-doc
 *  to use this nomenclature.
 */

enum mouse_action_e
{
    e_action_select,
    e_action_draw,
    e_action_grow
};

/**
 *  Provides names for the mouse-handling used by the application.
 */

const char * const c_interaction_method_names[3] =
{
    "seq24",
    "fruity",
    NULL
};

/**
 *  Provides descriptions for the mouse-handling used by the application.
 */

const char * const c_interaction_method_descs[3] =
{
    "original seq24 method",
    "similar to a certain fruity sequencer we like",
    NULL
};

/*
 * Global functions in the seq64 namespace for MIDI timing calculations.
 */

namespace seq64
{

extern std::string shorten_file_spec (const std::string & fpath, int leng);

/**
 *  Common code for handling PPQN settings.  Putting it here means we can
 *  reduce the reliance on the global ppqn.
 *
 * \param ppqn
 *      Provides the PPQN value to be used.
 *
 * \return
 *      Returns the ppqn parameter, unless that parameter is
 *      SEQ64_USE_DEFAULT_PPQN (-1), then usr().midi_ppqn is returned.
 */

inline int choose_ppqn (int ppqn)
{
    return (ppqn == SEQ64_USE_DEFAULT_PPQN) ? usr().midi_ppqn() : ppqn ;
}

/**
 *  Common code for handling PPQN settings.  Validates a PPQN value.
 *
 * \param ppqn
 *      Provides the PPQN value to be used.
 *
 * \return
 *      Returns true if the ppqn parameter is between MINIMUM_PPQN and
 *      MAXIMUM_PPQN, or is set to SEQ64_USE_DEFAULT_PPQN (-1).
 */

inline bool ppqn_is_valid (int ppqn)
{
    return
    (
        ppqn == SEQ64_USE_DEFAULT_PPQN ||
        (ppqn >= SEQ64_MINIMUM_PPQN && ppqn <= SEQ64_MAXIMUM_PPQN)
    );
}

}           // namespace seq64

#endif      // SEQ64_GLOBALS_H

/*
 * globals.h
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

