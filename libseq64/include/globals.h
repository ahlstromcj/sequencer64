#ifndef SEQ64_GLOBALS_HPP
#define SEQ64_GLOBALS_HPP

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
 *  in this application.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-25
 * \updates       2015-10-02
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
 *    This set of variables would be better off placed in a object that the
 *    mainwnd class and its clients can access a little more safely and with
 *    a lot more clarity for the human reader.
 */

#include <string>

#include "easy_macros.h"                // with platform_macros.h, too
#include "rc_settings.hpp"              // seq64::rc_settings
#include "user_settings.hpp"            // seq64::user_settings

extern rc_settings g_rc_settings;
extern user_settings g_user_settings;

/**
 *  This constant indicates that a configuration file numeric value is
 *  the default value for specifying that an instrument is a GM
 *  instrument.  Used in the "user" configuration-file processing.
 */

#define GM_INSTRUMENT_FLAG              (-1)
/**
 *  A manifest constant for the normal number of semitones in an
 *  equally-tempered octave.  The name is short deliberately.
 */

#define OCTAVE_SIZE                      12

/**
 *  Default value for c_ppqn (global parts-per-quarter-note value).
 */

#define DEFAULT_PPQN                    192

/**
 *  Default value for c_bpm (global beats-per-minute, also known as "BPM").
 *  Do not confuse this "bpm" with the other one, "beats per measure".
 */

#define DEFAULT_BPM                     120

/**
 *  Default value for "beats-per-measure".  This is the "numerator" in a 4/4
 *  time signature.  True?
 */

#define DEFAULT_BEATS_PER_MEASURE         4

/**
 *  Default value for "beat-width".  This is the "denominator" in a 4/4 time
 *  signature.  True?
 */

#define DEFAULT_BEAT_WIDTH                4

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

/**
 *  Number of rows in the Patterns Panel.  The current value is 4, and
 *  probably won't change, since other values depend on it.  Together with
 *  c_mainwnd_cols, this value fixes the patterns grid into a 4 x 8 set of
 *  patterns known as a "screen set".
 */

const int c_mainwnd_rows = 4;

/**
 *  Number of columns in the Patterns Panel.  The current value is 4, and
 *  probably won't change, since other values depend on it.  Together with
 *  c_mainwnd_rows, this value fixes the patterns grid into a 4 x 8 set of
 *  patterns known as a "screen set".
 */

const int c_mainwnd_cols = 8;

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

const int c_max_sets = 32;

/*
 *  The maximum number of patterns supported is given by the number of
 *  patterns supported in the panel (32) times the maximum number of sets
 *  (32), or 1024 patterns.  It is basically the same value as
 *  c_max_sequence, so we're going to make it obsolete.
 *
 *      const int c_total_seqs = c_seqs_in_set * c_max_sets;
 */

/**
 *  The maximum number of patterns supported is given by the number of
 *  patterns supported in the panel (32) times the maximum number of sets
 *  (32), or 1024 patterns.
 */

const int c_max_sequence = c_seqs_in_set * c_max_sets;

/**
 *  Provides the timing resolution of a MIDI sequencer, known as "pulses
 *  per quarter note.  For this application, 192 is the default, and it
 *  doesn't change.
 */

const int c_ppqn = DEFAULT_PPQN;

/**
 *  Provides the default number BPM (beats per minute), which describes
 *  the overall speed at which the sequencer will play a tune.  The
 *  default value is 120.
 */

const int c_bpm = DEFAULT_BPM;

/**
 *  The trigger width in milliseconds.  This value is 4 ms.
 */

const int c_thread_trigger_width_ms = DEFAULT_TRIGWIDTH_MS;

/**
 *  The trigger lookahead in milliseconds.  This value is 2 ms.
 */

const int c_thread_trigger_lookahead_ms = DEFAULT_TRIGLOOK_MS;

/**
 *  Constants for the mainwid class.  The c_text_x and c_text_y constants
 *  help define the "seqarea" size.  It looks like these two values are
 *  the character width (x) and height (y) in pixels.  Thus, these values
 *  would be dependent on the font chosen.  But that, currently, is
 *  hard-wired.  See the c_font_6_12[] array for the default font
 *  specification.
 *
 *  However, please not that font files are not used.  Instead, the fonts
 *  are provided by two pixmaps in the <tt> src/pixmap </tt> directory:
 *  <tt> font_b.xpm </tt> (black lettering on a white background) and
 *  <tt> font_w.xpm </tt> (white lettering on a black background).
 */

const int c_text_x =  6;
const int c_text_y = 12;

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

const int c_mainwid_border = 0;             // try 2 or 3instead of 0
const int c_mainwid_spacing = 2;            // try 4 or 6 instead of 2

/**
 *  This constants seems to be created for a future purpose, perhaps to
 *  reserve space for a new bar on the mainwid pane.  But it is used only
 *  in this header file, to define c_mainwid_y, but doesn't add anything
 *  to that value.
 */

const int c_control_height = 0;

/**
 * The width of the main pattern/sequence grid, in pixels.  Affected by
 * the c_mainwid_border and c_mainwid_spacing values.
 */

const int c_mainwid_x =
(
    (c_seqarea_x + c_mainwid_spacing) * c_mainwnd_cols -
        c_mainwid_spacing + c_mainwid_border * 2
);

/*
 * The height  of the main pattern/sequence grid, in pixels.  Affected by
 * the c_mainwid_border and c_control_height values.
 */

const int c_mainwid_y =
(
    (c_seqarea_y + c_mainwid_spacing) * c_mainwnd_rows +
         c_control_height + c_mainwid_border * 2
);

/**
 *  The height of the data-entry area for velocity, aftertouch, and other
 *  controllers, as well as note on and off velocity.  This value looks to
 *  be in pixels; one pixel per MIDI value.
 */

const int c_dataarea_y = (MIDI_COUNT_MAX * 1);

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
const int c_key_y = 8;

/**
 *  The number of MIDI keys, as well as keys in the virtual keyboard.
 *  Note that only a subset of the virtual keys will be shown; one must
 *  scroll to see them all.
 */

const int c_num_keys = MIDI_COUNT_MAX;      // 128

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
 *  each event.
 */

const int c_eventevent_x = 5;
const int c_eventevent_y = 10;

/**
 *  A new constant that presents the padding above and below and event
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

const int c_midi_notes = 256;

/**
 *  Provides the default string for the name of a pattern or sequence.
 */

const std::string c_dummy = "Untitled";

/**
 *  Provides the maximum size of sequence, and the default size.
 *  It is the maximum number of beats in a sequence.
 */

const int c_maxbeats = 0xFFFF;

/**
 *  Provides tags used by the midifile class to control the reading and
 *  writing of the extra "proprietary" information stored in a Seq24 MIDI
 *  file.
 */

const unsigned long c_midibus =         0x24240001;
const unsigned long c_midich =          0x24240002;
const unsigned long c_midiclocks =      0x24240003;
const unsigned long c_triggers =        0x24240004;
const unsigned long c_notes =           0x24240005;
const unsigned long c_timesig =         0x24240006;
const unsigned long c_bpmtag =          0x24240007;
const unsigned long c_triggers_new =    0x24240008;
const unsigned long c_mutegroups =      0x24240009;
const unsigned long c_midictrl =        0x24240010;

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
 *  Provides constants for the perform object (performance editor).
 */

const int c_names_x = 6 * 24;
const int c_names_y = 22;
const int c_perf_scale_x = 32;  // units are ticks per pixel

/**
 *  These global values seemed to be use mainly in the options,
 *  optionsfile, perform, seq24, and userfile modules.
 */

extern bool global_legacy_format;      /* new 2015-08-16 */
extern bool global_lash_support;       /* new 2015-08-27 */
extern bool global_showmidi;
extern bool global_priority;
extern bool global_stats;
extern bool global_pass_sysex;
extern bool global_with_jack_transport;
extern bool global_with_jack_master;
extern bool global_with_jack_master_cond;
extern bool global_jack_start_mode;
extern bool global_manual_alsa_ports;
extern bool global_is_pattern_playing;
extern bool global_print_keys;
extern bool global_device_ignore;           // seq24 module
extern int global_device_ignore_num;        // seq24 module
extern std::string global_filename;
extern std::string global_jack_session_uuid;
extern std::string global_last_used_dir;
extern std::string global_config_directory;
extern std::string global_config_filename;
extern std::string global_user_filename;
extern std::string global_config_filename_alt;
extern std::string global_user_filename_alt;

/**
 *  Global arrays.  To be moved to user_settings SOON.
 */

extern user_midi_bus_t global_user_midi_bus_definitions[c_max_busses];
extern user_instrument_t global_user_instrument_definitions[c_max_instruments];

/**
 *  Corresponds to the small number of musical scales that the application
 *  can handle.  Scales can be shown in the piano roll as gray bars for
 *  reference purposes.
 *
 *  It would be good to offload this stuff into a new "scale" class.
 */

enum c_music_scales
{
    c_scale_off,
    c_scale_major,
    c_scale_minor,
    c_scale_harmonic_minor,
    c_scale_melodic_minor,
    c_scale_c_whole_tone,
    c_scale_size            // a "maximum" or "size of set" value.
};

/**
 *  Each value in the kind of scale is denoted by a true value in these
 *  arrays.  See the following sites for more information:
 *
 *      http://method-behind-the-music.com/theory/scalesandkeys/
 *
 *      https://en.wikipedia.org/wiki/Heptatonic_scale
 *
 *  Note that melodic minor descends in the same way as the natural minor
 *  scale, so it descends differently than it ascends.  We don't deal with
 *  that trick, at all.
 *
\verbatim
    Chromatic           C  C# D  D# E  F  F# G  G# A  A# B   Notes, chord
    Major               C  .  D  .  E  F  .  G  .  A  .  B
    Minor               C  .  D  Eb .  F  .  G  Ab .  Bb .
    Harmonic Minor      C  .  D  Eb .  F  .  G  Ab .  .  B
    Melodic Minor       C  .  D  Eb .  F  .  G  .  A  .  B   Descending diff.
    C Whole Tone        C  .  D  .  E  .  F# .  G# .  A# .   C+7 chord
    C Lydian Dominant   C  .  D  .  E  .  F# .  G  .  A  Bb  Unimplemented, A7
    A Mixolydian        C# .  D  .  E  .  F# .  G  .  A  .   Unimplemented, A7
    B Whole Tone        .  Db .  Eb .  F  .  G  .  A  .  B   Unimplemented
    G Whole Tone        .  C# .  D# .  F  .  G  .  A  .  B   Unimplemented, same
    G Octatonic         .  C# D  .  E  F  .  G  Ab .  Bb B   Unimplemented
\endverbatim
 */

const bool c_scales_policy[c_scale_size][OCTAVE_SIZE] =
{
    {                                                       /* off = chromatic */
        true, true, true, true, true, true,
        true, true, true, true, true, true
    },
    {                                                       /* major           */
        true, false, true, false, true, true,
        false, true, false, true, false, true
    },
    {                                                       /* minor           */
        true, false, true, true, false, true,
        false, true, true, false, true, false
    },
    {                                                       /* harmonic minor  */
        true, false, true, true, false, true,
        false, true, true, false, false, true
    },
    {                                                       /* melodic minor   */
        true, false, true, true, false, true,
        false, true, false, true, false, true
    },
    {                                                       /* C whole tone    */
        true, false, true, false, true, false,
        true, false, true, false, true, false
    },
};

/**
 *  Increment values needed to transpose each scale up so that it remains
 *  in the same key.  For example, if we simply add 1 semitone to each
 *  note, it remains a minor key, but it is in a different minor key.
 *  Using the transpositions in these arrays, the minor key remains the
 *  same minor key.
 *
\verbatim
    Major               C  .  D  .  E  F  .  G  .  A  .  B
    Transpose up        2  0  2  0  1  2  0  2  0  2  0  1
    Result up           D  .  E  .  F  G  .  A  .  B  .  C

    Minor               C  .  D  D# .  F  .  G  G# .  A# .
    Transpose up        2  0  1  2  0  2  0  1  2  0  2  0
    Result up           D  .  D# F  .  G  .  G# A# .  C  .

    Harmonic minor      C  .  D  Eb .  F  .  G  Ab .  .  B
    Transpose up        2  .  1  2  .  2  .  1  3  .  .  1
    Result up           D  .  Eb F  .  G  .  Ab B  .  .  C

    Melodic minor       C  .  D  Eb .  F  .  G  .  A  .  B
    Transpose up        2  .  1  2  .  2  .  2  .  2  .  1
    Result up           D  .  Eb F  .  G  .  A  .  B  .  C

    C Whole Tone        C  .  D  .  E  .  F# .  G# .  A# .
    Transpose up        2  .  2  .  2  .  2  .  2  .  2  .
    Result up           D  .  E  .  F# .  G# .  A# .  C  .
\endverbatim
 */

const int c_scales_transpose_up[c_scale_size][OCTAVE_SIZE] =
{
    { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},              /* off = chromatic */
    { 2, 0, 2, 0, 1, 2, 0, 2, 0, 2, 0, 1},              /* major           */
    { 2, 0, 1, 2, 0, 2, 0, 1, 2, 0, 2, 0},              /* minor           */
    { 2, 0, 1, 2, 0, 2, 0, 1, 3, 0, 0, 1},              /* harmonic minor  */
    { 2, 0, 1, 2, 0, 2, 0, 2, 0, 2, 0, 1},              /* melodic minor   */
    { 2, 0, 2, 0, 2, 0, 2, 0, 2, 0, 2, 0},              /* C whole tone    */
};

const int c_scales_transpose_dn[c_scale_size][OCTAVE_SIZE] =
{
    { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},  /* off = chromatic */
    { -1, 0, -2, 0, -2, -1, 0, -2, 0, -2, 0, -2},       /* major           */
    { -2, 0, -2, -1, 0, -2, 0, -2, -1, 0, -2, 0},       /* minor           */
    { -1, -0, -2, -1, 0, -2, 0, -2, -1, 0, 0, -3},      /* harmonic minor  */
    { -1, 0, -2, -1, 0, -2, 0, -2, 0, -2, 0, -2},       /* melodic minor   */
    { -2, 0, -2, 0, -2, 0, -2, 0, -2, 0, -2, 0},        /* C whole tone    */
};

/**
 *  Making these positive makes it easier to read.  Just remember to
 *  negate each value before using it.
 *
\verbatim
    Major               C  .  D  .  E  F  .  G  .  A  .  B
    Transpose down      1  0  2  0  2  1  0  2  0  2  0  2
    Result down         B  .  C  .  D  E  .  F  .  G  .  A

    Minor               C  .  D  D# .  F  .  G  G# .  A# .
    Transpose down      2  0  2  1  0  2  0  2  1  0  2  0
    Result down         A# .  C  D  .  D# .  F  G  .  G# .

    Harmonic minor      C  .  D  Eb .  F  .  G  Ab .  .  B
    Transpose down      1  .  2  1  .  2  .  2  1  .  .  3
    Result down         B  .  C  D  .  Eb .  F  G  .  .  Ab

    Melodic minor       C  .  D  Eb .  F  .  G  .  A  .  B
    Transpose down      1  .  2  1  .  2  .  2  .  2  .  2
    Result down         B  .  C  D  .  Eb .  F  .  G  .  A

    C whole tone        C  .  D  .  E  .  F# .  G# .  A# .
    Transpose down      2  .  2  .  2  .  2  .  2  .  2  .
    Result down         A# .  C  .  D  .  E  .  F# .  G# .
\endverbatim
 */

const int c_scales_transpose_dn_neg[c_scale_size][OCTAVE_SIZE] =
{
    { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},                  /* off = chromatic */
    { 1, 0, 2, 0, 2, 1, 0, 2, 0, 2, 0, 2},                  /* major           */
    { 2, 0, 2, 1, 0, 2, 0, 2, 1, 0, 2, 0},                  /* minor           */
    { 1, 0, 2, 1, 0, 2, 0, 2, 1, 0, 0, 3},                  /* harmonic minor  */
    { 1, 0, 2, 1, 0, 2, 0, 2, 0, 2, 0, 2},                  /* melodic minor   */
    { 2, 0, 2, 0, 2, 0, 2, 0, 2, 0, 2, 0},                  /* C whole tone    */
};

/**
 * \internal
 *
 *  This array is currently commented out int seqkeys.cpp in the
 *  update_pixmap() function.
 *
\verbatim
const int c_scales_symbol[c_scale_size][OCTAVE_SIZE] =
{
    { 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32},      // off = chromatic
    { 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32},      // major
    { 32, 32, 32, 32, 32, 32, 32, 32, 129, 128, 129, 128},  // minor
    { 32, 32, 32, 32, 32, 32, 32, 32, 129, 128, 129, 128},  // harmonic minor
    { 32, 32, 32, 32, 32, 32, 32, 32, 129, 128, 129, 128},  // melodic minor
    { 32, 32, 32, 32, 32, 32, 32, 32, 129, 128, 129, 128},  // C whole tone
};
\endverbatim
 */

/**
 *  The names of the supported scales.
 */

const char c_scales_text[c_scale_size][32] =                /* careful!        */
{
    "Off (chromatic)",
    "Major",
    "Minor",
    "Harmonic Minor",
    "Melodic Minor",
    "Whole Tone",
};

/**
 *  Provides the entries for the Key dropdown menu in the Pattern Editor
 *  window.
 */

const char c_key_text[OCTAVE_SIZE][3] =
{
    "C",
    "C#",
    "D",
    "D#",
    "E",
    "F",
    "F#",
    "G",
    "G#",
    "A",
    "A#",
    "B"
};

/**
 *  Provides the entries for the Interval dropdown menu in the Pattern Editor
 *  window.
 */

const char c_interval_text[16][3] =
{
    "P1",
    "m2",
    "M2",
    "m3",
    "M3",
    "P4",
    "TT",
    "P5",
    "m6",
    "M6",
    "m7",
    "M7",
    "P8",
    "m9",
    "M9",
    ""
};

/**
 *  Provides the entries for the Chord dropdown menu in the Pattern Editor
 *  window.  However, I have not seen this menu in the GUI!  Ah, it only
 *  appears if the user has selected a musical scale like Major or Minor.
 */

const char c_chord_text[8][5] =
{
    "I",
    "II",
    "III",
    "IV",
    "V",
    "VI",
    "VII",
    "VIII"
};

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

/**
 *  Provides the value of the interaction method in use, either "seq24" or
 *  "fruity".
 */

extern interaction_method_t global_interactionmethod;

/**
 *  Provides the value of usage of the Mod4 (Super or Windows) key in
 *  disabling the exiting of the note-add mode of the seqroll module.
 *  Currently applies only the the "seq24" interaction method, not the
 *  "fruity" method.  Defaults to true.
 */

extern bool global_allow_mod4_mode;

#endif  // SEQ64_GLOBALS_HPP

/*
 * globals.h
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
