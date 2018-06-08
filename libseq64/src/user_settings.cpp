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
 * \file          user_settings.cpp
 *
 *  This module declares/defines just some of the global (gasp!) variables
 *  in this application.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-09-23
 * \updates       2018-05-27
 * \license       GNU GPLv2 or above
 *
 *  Note that this module also sets the remaining legacy global variables, so
 *  that they can be used by modules that have not yet been cleaned up.
 *
 *  Now, we finally sat down and did some measurements of the user interface,
 *  to try to figure out the relationships between the screen resolution and
 *  MIDI time resolution, so that we can understand some of the magic numbers
 *  in Seq24.
 *
 *  We start with one clue, a comment in perftime (IIRC) about units being 32
 *  ticks per pixels.  Note that "ticks" is equivalent to MIDI "pulses", and
 *  sometimes the word "division" is used for "pulses".  So let's solidy the
 *  nomenclature and notation here:
 *
\verbatim
    Symbol      Units           Value       Description

     qn         quarter note    -----       The default unit for a MIDI beat
     P0         pulses/qn       192         Seq24's PPQN value, a constant
     P          pulses/qn       -----       Any other selected PPQN value
     R          -----           -----       P / P0
     Wscreen    pixels          1920        Width of the screen, pixels
     Wperfqn    pixels          6           Song editor q-note width, constant
     Zperf      pulses/pixel    32          Song editor default zoom, constant
     Dperf      minor/major     4           Song editor beats shown per measure
     ?          pulses/pixel    -----       GUI-MIDI resolution from selected P
     S          -----           16          seqroll-to-perfroll width ratio
     Zseqmin    pulses/pixel    1           Seq editor max zoom in
     Zseq0      pulses/pixel    2           Seq editor default zoom
     Zseqmax    pulses/pixel    128 (32)    Seq editor max zoom out
\endverbatim
 *
 * Sequence Editor (seqroll):
 *
 *  Careful measuring on my laptop screen shows that the perfroll covers 80
 *  measures over 1920 pixels.
 *
\verbatim
    1920 pixels
    ----------- = 24 pixels/measure = 6 pixels/qn = Wperfqn
    80 measures
\endverbatim
 *
 * Song Editor (perfroll) Zoom:
 *
 *  The value of S = 16 reflects that the sequence editor piano roll,
 *  at its default zoom (2 pulses/pixel), has 16 times the width resolution
 *  of the performance/song editor piano roll (32 pulses/pixel).  This ratio
 *  (at the default zoom) will be preserved no matter what P (PPQN) is
 *  selected for the song.
 *
 *  The sequence editor supports zooms of 1 pulse/pixel, 2 pulses/pixel (it's
 *  default), and 4, 8, 16, and 32 pulses/pixel (the song editor's only zoom).
 *
 * Song Editor (perfedit, perfroll, pertime) Guides:
 *
 *                    pulses        major
 *  measureticks = P0 ------  Dperf -----
 *                     qn           minor
 *
 *     perfedit:  m_ppqn    m_standard_bpm
 *
 * Time Signature:
 *
 *  Changing the beats-per-measure of the seqroll to from the default 4 to 8
 *  makes the measure have 8 major divisions, each with the standard 16 minor
 *  divisions. An added note still covers only 4 minor divisions.
 *
 *  Changing the beat-width of the seqroll from the default 4 to 8 halves the
 *  pixel-width of reach measure.
 */

#include "settings.hpp"                 /* seq64::rc()                  */
#include "user_settings.hpp"            /* seq64::user_settings         */

#define SEQ64_WINDOW_SCALE_MIN          0.5f
#define SEQ64_WINDOW_SCALE_DEFAULT      1.0f
#define SEQ64_WINDOW_SCALE_MAX          3.0f

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *  Default constructor.
 */

user_settings::user_settings ()
 :
    m_midi_buses                (),     /* [user-midi-bus-definitions]  */
    m_instruments               (),     /* [user-instrument-definitions] */
    m_comments_block                    /* [comments                    */
    (
        "(Comments added to this section are preserved.  Lines starting with\n"
        " a '#' or '[', or that are blank, are ignored.  Start lines that must\n"
        " be blank with a space.)\n"
    ),

    /*
     * [user-interface-settings]
     */

    m_grid_style                (grid_style_normal),
    m_grid_brackets             (1),
    m_mainwnd_rows              (SEQ64_DEFAULT_MAINWND_ROWS),
    m_mainwnd_cols              (SEQ64_DEFAULT_MAINWND_COLUMNS),
    m_max_sets                  (SEQ64_DEFAULT_SET_MAX),
    m_window_scale              (SEQ64_WINDOW_SCALE_DEFAULT),
    m_mainwid_border            (0),
    m_mainwid_spacing           (0),
    m_control_height            (0),
    m_current_zoom              (0),            // 0 is unsafe, but a feature
    m_global_seq_feature_save   (true),
    m_seqedit_scale             (0),
    m_seqedit_key               (0),
    m_seqedit_bgsequence        (0),
    m_use_new_font              (false),
    m_allow_two_perfedits       (false),
    m_h_perf_page_increment     (1),
    m_v_perf_page_increment     (1),
    m_progress_bar_colored      (0),
    m_progress_bar_thick        (false),
    m_inverse_colors            (false),
    m_window_redraw_rate_ms     (c_redraw_ms),  // 40 ms or 20 ms; 25 ms
    m_use_more_icons            (false),
    m_mainwid_block_rows        (1),
    m_mainwid_block_cols        (1),
    m_mainwid_block_independent (false),

    /*
     * The members that follow are not yet part of the .usr file.
     */

    m_text_x                    (0),
    m_text_y                    (0),
    m_seqchars_x                (0),
    m_seqchars_y                (0),

    /*
     * [user-midi-settings]
     */

    m_midi_ppqn                 (0),
    m_midi_beats_per_measure    (0),
    m_midi_bpm_minimum          (0),
    m_midi_beats_per_minute     (0),
    m_midi_bpm_maximum          (SEQ64_MAX_DATA_VALUE),
    m_midi_beat_width           (0),
    m_midi_buss_override        (0),
    m_velocity_override         (SEQ64_PRESERVE_VELOCITY),
    m_bpm_precision             (SEQ64_DEFAULT_BPM_PRECISION),
    m_bpm_step_increment        (SEQ64_DEFAULT_BPM_STEP_INCREMENT),
    m_bpm_page_increment        (SEQ64_DEFAULT_BPM_PAGE_INCREMENT),

    /*
     * Calculated from other member values in the normalize() function.
     */

    m_total_seqs                (0),
    m_seqs_in_set               (0),            // set correctly in normalize()
    m_gmute_tracks              (0),            // same as max-tracks
    m_max_sequence              (0),
    m_seqarea_x                 (0),
    m_seqarea_y                 (0),
    m_seqarea_seq_x             (0),
    m_seqarea_seq_y             (0),
    m_mainwid_x                 (0),
    m_mainwid_y                 (0),
    m_mainwnd_x                 (780),          // constant
    m_mainwnd_y                 (412),          // constant
    m_save_user_config          (false),

    /*
     * Constant values.
     */

    mc_min_zoom                 (SEQ64_MINIMUM_ZOOM),
    mc_max_zoom                 (SEQ64_MAXIMUM_ZOOM),
    mc_baseline_ppqn            (SEQ64_DEFAULT_PPQN),

    /*
     * Back to non-constant values.
     */

    m_user_option_daemonize     (false),
    m_user_use_logfile          (false),
    m_user_option_logfile       (),
    m_work_around_play_image    (false),
    m_work_around_transpose_image (false),

    /*
     * [user-ui-tweaks]
     */

    m_user_ui_key_height        (12)

{
    // Empty body; it's no use to call normalize() here, see set_defaults().
}

/**
 *  Copy constructor.
 */

user_settings::user_settings (const user_settings & rhs)
 :
    m_midi_buses                (rhs.m_midi_buses),     // vector
    m_instruments               (rhs.m_instruments),    // vector
    m_comments_block            (rhs.m_comments_block),
    m_grid_style                (rhs.m_grid_style),
    m_grid_brackets             (rhs.m_grid_brackets),
    m_mainwnd_rows              (rhs.m_mainwnd_rows),
    m_mainwnd_cols              (rhs.m_mainwnd_cols),
    m_max_sets                  (rhs.m_max_sets),
    m_window_scale              (rhs.m_window_scale),
    m_mainwid_border            (rhs.m_mainwid_border),
    m_mainwid_spacing           (rhs.m_mainwid_spacing),
    m_control_height            (rhs.m_control_height),
    m_current_zoom              (rhs.m_current_zoom),
    m_global_seq_feature_save   (rhs.m_global_seq_feature_save),
    m_seqedit_scale             (rhs.m_seqedit_scale),
    m_seqedit_key               (rhs.m_seqedit_key),
    m_seqedit_bgsequence        (rhs.m_seqedit_bgsequence),
    m_use_new_font              (rhs.m_use_new_font),
    m_allow_two_perfedits       (rhs.m_allow_two_perfedits),
    m_h_perf_page_increment     (rhs.m_h_perf_page_increment),
    m_v_perf_page_increment     (rhs.m_v_perf_page_increment),
    m_progress_bar_colored      (rhs.m_progress_bar_colored),
    m_progress_bar_thick        (rhs.m_progress_bar_thick),
    m_inverse_colors            (rhs.m_inverse_colors),
    m_window_redraw_rate_ms     (rhs.m_window_redraw_rate_ms),
    m_use_more_icons            (rhs.m_use_more_icons),
    m_mainwid_block_rows        (rhs.m_mainwid_block_rows),
    m_mainwid_block_cols        (rhs.m_mainwid_block_cols),
    m_mainwid_block_independent (rhs.m_mainwid_block_independent),

    /*
     * The members that follow are not yet part of the .usr file.
     */

    m_text_x                    (rhs.m_text_x),
    m_text_y                    (rhs.m_text_y),
    m_seqchars_x                (rhs.m_seqchars_x),
    m_seqchars_y                (rhs.m_seqchars_y),

    /*
     * [user-midi-settings]
     */

    m_midi_ppqn                 (rhs.m_midi_ppqn),
    m_midi_beats_per_measure    (rhs.m_midi_beats_per_measure),
    m_midi_bpm_minimum          (rhs.m_midi_bpm_minimum),
    m_midi_beats_per_minute     (rhs.m_midi_beats_per_minute),
    m_midi_bpm_maximum          (rhs.m_midi_bpm_maximum),
    m_midi_beat_width           (rhs.m_midi_beat_width),
    m_midi_buss_override        (rhs.m_midi_buss_override),
    m_velocity_override         (rhs.m_velocity_override),
    m_bpm_precision             (rhs.m_bpm_precision),
    m_bpm_step_increment        (rhs.m_bpm_step_increment),
    m_bpm_page_increment        (rhs.m_bpm_page_increment),

    /*
     * Calculated from other member values in the normalize() function.
     */

    m_total_seqs                (rhs.m_total_seqs),
    m_seqs_in_set               (rhs.m_seqs_in_set),
    m_gmute_tracks              (rhs.m_gmute_tracks),
    m_max_sequence              (rhs.m_max_sequence),
    m_seqarea_x                 (rhs.m_seqarea_x),
    m_seqarea_y                 (rhs.m_seqarea_y),
    m_seqarea_seq_x             (rhs.m_seqarea_seq_x),
    m_seqarea_seq_y             (rhs.m_seqarea_seq_y),
    m_mainwid_x                 (rhs.m_mainwid_x),
    m_mainwid_y                 (rhs.m_mainwid_y),
    m_mainwnd_x                 (rhs.m_mainwnd_x),
    m_mainwnd_y                 (rhs.m_mainwnd_y),
    m_save_user_config          (rhs.m_save_user_config),

    /*
     * Constant values.
     */

    mc_min_zoom                 (rhs.mc_min_zoom),
    mc_max_zoom                 (rhs.mc_max_zoom),
    mc_baseline_ppqn            (SEQ64_DEFAULT_PPQN),

    /*
     * Back to non-constant values.

    m_user_option_daemonize     (false),
    m_user_use_logfile          (false),
    m_user_option_logfile       (),
    m_work_around_play_image    (false),
    m_work_around_transpose_image (false)
    m_user_ui_key_height        (12)
     */

    m_user_option_daemonize     (rhs.m_user_option_daemonize),
    m_user_use_logfile          (rhs.m_user_use_logfile),
    m_user_option_logfile       (rhs.m_user_option_logfile),
    m_work_around_play_image    (rhs.m_work_around_play_image),
    m_work_around_transpose_image (rhs.m_work_around_transpose_image),

    /*
     * [user-ui-tweaks]
     */

    m_user_ui_key_height        (rhs.m_user_ui_key_height)
{
    // Empty body; no need to call normalize() here.
}

/**
 *  Principal assignment operator.
 */

user_settings &
user_settings::operator = (const user_settings & rhs)
{
    if (this != &rhs)
    {
        m_midi_buses                = rhs.m_midi_buses;
        m_instruments               = rhs.m_instruments;
        m_comments_block            = rhs.m_comments_block;
        m_grid_style                = rhs.m_grid_style;
        m_grid_brackets             = rhs.m_grid_brackets;
        m_mainwnd_rows              = rhs.m_mainwnd_rows;
        m_mainwnd_cols              = rhs.m_mainwnd_cols;
        m_max_sets                  = rhs.m_max_sets;
        m_window_scale              = rhs.m_window_scale;
        m_mainwid_border            = rhs.m_mainwid_border;
        m_mainwid_spacing           = rhs.m_mainwid_spacing;
        m_control_height            = rhs.m_control_height;
        m_current_zoom              = rhs.m_current_zoom;
        m_global_seq_feature_save   = rhs.m_global_seq_feature_save;
        m_seqedit_scale             = rhs.m_seqedit_scale;
        m_seqedit_key               = rhs.m_seqedit_key;
        m_seqedit_bgsequence        = rhs.m_seqedit_bgsequence;
        m_use_new_font              = rhs.m_use_new_font;
        m_allow_two_perfedits       = rhs.m_allow_two_perfedits;
        m_h_perf_page_increment     = rhs.m_h_perf_page_increment;
        m_v_perf_page_increment     = rhs.m_v_perf_page_increment;
        m_progress_bar_colored      = rhs.m_progress_bar_colored;
        m_progress_bar_thick        = rhs.m_progress_bar_thick;
        m_inverse_colors            = rhs.m_inverse_colors;
        m_window_redraw_rate_ms     = rhs.m_window_redraw_rate_ms;
        m_use_more_icons            = rhs.m_use_more_icons;
        m_mainwid_block_rows        = rhs.m_mainwid_block_rows;
        m_mainwid_block_cols        = rhs.m_mainwid_block_cols;
        m_mainwid_block_independent = rhs.m_mainwid_block_independent;

        /*
         * The members that follow are not yet part of the .usr file.
         */

        m_text_x                    = rhs.m_text_x;
        m_text_y                    = rhs.m_text_y;
        m_seqchars_x                = rhs.m_seqchars_x;
        m_seqchars_y                = rhs.m_seqchars_y;

        /*
         * [user-midi-settings]
         */

        m_midi_ppqn                 = rhs.m_midi_ppqn;
        m_midi_beats_per_measure    = rhs.m_midi_beats_per_measure;
        m_midi_bpm_minimum          = rhs.m_midi_bpm_minimum;
        m_midi_beats_per_minute     = rhs.m_midi_beats_per_minute;
        m_midi_bpm_maximum          = rhs.m_midi_bpm_maximum;
        m_midi_beat_width           = rhs.m_midi_beat_width;
        m_midi_buss_override        = rhs.m_midi_buss_override;
        m_velocity_override         = rhs.m_velocity_override;
        m_bpm_precision             = rhs.m_bpm_precision;
        m_bpm_step_increment        = rhs.m_bpm_step_increment;
        m_bpm_page_increment        = rhs.m_bpm_page_increment;

        /*
         * Calculated from other member values in the normalize() function.
         *
         *  m_total_seqs                = rhs.m_total_seqs;
         *  m_seqs_in_set               = rhs.m_seqs_in_set;
         *  m_gmute_tracks              = rhs.m_gmute_tracks;
         *  m_max_sequence              = rhs.m_max_sequence;
         *  m_seqarea_x                 = rhs.m_seqarea_x;
         *  m_seqarea_y                 = rhs.m_seqarea_y;
         *  m_seqarea_seq_x             = rhs.m_seqarea_seq_x;
         *  m_seqarea_seq_y             = rhs.m_seqarea_seq_y;
         *  m_mainwid_x                 = rhs.m_mainwid_x;
         *  m_mainwid_y                 = rhs.m_mainwid_y;
         *  m_mainwnd_x                 = rhs.m_mainwnd_x;
         *  m_mainwnd_y                 = rhs.m_mainwnd_y;
         */

        m_save_user_config = rhs.m_save_user_config;
        normalize();

        /*
         * Constant values.  These values cannot be modified.
         *
         * mc_min_zoom              = rhs.mc_min_zoom;
         * mc_max_zoom              = rhs.mc_max_zoom;
         * mc_baseline_ppqn         = rhs.mc_baseline_ppqn;
         */

        m_user_option_daemonize = rhs.m_user_option_daemonize;
        m_user_use_logfile = rhs.m_user_use_logfile;
        m_user_option_logfile = rhs.m_user_option_logfile;
        m_work_around_play_image = rhs.m_work_around_play_image;
        m_work_around_transpose_image = rhs.m_work_around_transpose_image;

        /*
         * [user-ui-tweaks]
         */

        m_user_ui_key_height = rhs.m_user_ui_key_height;
    }
    return *this;
}

/**
 *  Sets the default values.  For the m_midi_buses and
 *  m_instruments members, this function can only iterate over the
 *  current size of the vectors.  But the default size is zero!
 */

void
user_settings::set_defaults ()
{
    /*
     * m_comments_block.clear();
     */

    m_midi_buses.clear();
    m_instruments.clear();

    m_grid_style = grid_style_normal;       // range: 0-2
    m_grid_brackets = 1;                    // range: -30 to 0 to 30
    m_mainwnd_rows = SEQ64_DEFAULT_MAINWND_ROWS;    // range: 4-8
    m_mainwnd_cols = SEQ64_DEFAULT_MAINWND_COLUMNS; // range: 8-8
    m_max_sets = SEQ64_DEFAULT_SET_MAX;     // range: 32-64
    m_window_scale = SEQ64_WINDOW_SCALE_DEFAULT; // range: 0.5 to 3.0
    m_mainwid_border = 0;                   // range: 0-3, try 2 or 3
    m_mainwid_spacing = 2;                  // range: 2-6, try 4 or 6
    m_control_height = 0;                   // range: 0-4?
    m_current_zoom = SEQ64_DEFAULT_ZOOM;    // range: 1-128
    m_global_seq_feature_save = true;
    m_seqedit_scale = int(c_scale_off);     // c_scale_off to < c_scale_size
    m_seqedit_key = SEQ64_KEY_OF_C;         // range: 0-11
    m_seqedit_bgsequence = SEQ64_SEQUENCE_LIMIT; // range -1, 0, 1, 2, ...
    m_use_new_font = ! rc().legacy_format();
    m_allow_two_perfedits = true;
    m_h_perf_page_increment = 4;
    m_v_perf_page_increment = 8;
    m_progress_bar_colored = 0;
    m_progress_bar_thick = false;
    m_inverse_colors = false;
    m_window_redraw_rate_ms = c_redraw_ms;
    m_use_more_icons = false;
    m_mainwid_block_rows = 1;
    m_mainwid_block_cols = 1;
    m_mainwid_block_independent = false;
    m_text_x =  6;                          // range: 6-6
    m_text_y = 12;                          // range: 12-12
    m_seqchars_x = 15;                      // range: 15-15
    m_seqchars_y =  5;                      // range: 5-5
    m_midi_ppqn = SEQ64_DEFAULT_PPQN;       // range: 96 to 960, default 192
    m_midi_beats_per_measure = SEQ64_DEFAULT_BEATS_PER_MEASURE; // range: 1-16
    m_midi_bpm_minimum = 0;                 // range: 0 to ???
    m_midi_beats_per_minute = SEQ64_DEFAULT_BPM;    // range: 20-500
    m_midi_bpm_maximum = SEQ64_MAX_DATA_VALUE;      // range: ? to ???
    m_midi_beat_width = SEQ64_DEFAULT_BEAT_WIDTH;   // range: 1-16, powers of 2
    m_midi_buss_override = SEQ64_BAD_BUSS;          // range: 1 to 32
    m_velocity_override = SEQ64_PRESERVE_VELOCITY;  // -1, range: 0 to 127
    m_bpm_precision = SEQ64_DEFAULT_BPM_PRECISION;
    m_bpm_step_increment = SEQ64_DEFAULT_BPM_STEP_INCREMENT;
    m_bpm_page_increment = SEQ64_DEFAULT_BPM_PAGE_INCREMENT;

    /*
     * Constants:
     *
     *  mc_min_zoom
     *  mc_max_zoom
     *  mc_baseline_ppqn
     */

    m_user_option_daemonize = false;
    m_user_use_logfile = false;
    m_user_option_logfile.clear();
    m_work_around_play_image = false;
    m_work_around_transpose_image = false;
    m_user_ui_key_height = 12;
    normalize();                            // recalculate derived values
}

/**
 *  Calculate the derived values from the already-set values.
 *  Should we normalize the BPM increment values here, in case they
 *  are irregular?
 *
 *  gmute_tracks() is viable with variable set sizes only if we stick with the
 *  32 sets by 32 patterns, at this time. It's semantic meaning is... TODO!!
 *
 *  m_max_sequence is now actually a constant (1024), so we enforce that here
 *  now.
 */

void
user_settings::normalize ()
{
    m_seqs_in_set = m_mainwnd_rows * m_mainwnd_cols;
    m_max_sets = c_max_sequence / m_seqs_in_set;            /* 16 to 32...  */
    m_gmute_tracks = m_seqs_in_set * m_seqs_in_set;         /* TODO!        */
    m_total_seqs = m_seqs_in_set * m_max_sets;

    /*
     * This is probably a good idea to do this; still under investigation.
     *
     * m_max_sequence = m_seqs_in_set * m_max_sets;
     */

    /******
     * EXPERIMENTAL!!!

    m_text_x = scale_a_size(m_text_x);
    m_text_y = scale_a_size(m_text_x);
     */

    m_max_sequence = c_max_sequence;
    m_seqarea_x = m_text_x * m_seqchars_x;
    m_seqarea_y = m_text_y * m_seqchars_y;
    m_seqarea_seq_x = m_text_x * 13;
    m_seqarea_seq_y = m_text_y * 2;
    m_mainwid_x =
    (
        (m_seqarea_x + m_mainwid_spacing) * m_mainwnd_cols - m_mainwid_spacing +
            m_mainwid_border * 2
    );
    m_mainwid_y =
    (
        (m_seqarea_y + m_mainwid_spacing) * m_mainwnd_rows +
             m_control_height + m_mainwid_border * 2
    );
}

/**
 * \getter m_mainwnd_x
 */

int
user_settings::mainwnd_x () const
{
    if (block_rows() != 1 || block_columns() != 1)
        return 0;
    else
        return scale_size(m_mainwnd_x);
}

/**
 * \getter m_mainwnd_y
 *      Scaled only if window scaling is less than 1.0.
 */

int
user_settings::mainwnd_y () const
{
    if (block_rows() != 1 || block_columns() != 1)
        return 0;
    else
    {
        return m_window_scale > 1.0f ?
            m_mainwnd_y : int(scale_size(m_mainwnd_y)) ;
    }
}

/**
 *  Adds a user buss to the container, but only does so if the name
 *  parameter is not empty.
 */

bool
user_settings::add_bus (const std::string & alias)
{
    bool result = ! alias.empty();
    if (result)
    {
        size_t currentsize = m_midi_buses.size();
        user_midi_bus temp(alias);
        result = temp.is_valid();
        if (result)
        {
            m_midi_buses.push_back(temp);
            result = m_midi_buses.size() == currentsize + 1;
        }
    }
    return result;
}

/**
 *  Adds a user instrument to the container, but only does so if the name
 *  parameter is not empty.
 */

bool
user_settings::add_instrument (const std::string & name)
{
    bool result = ! name.empty();
    if (result)
    {
        size_t currentsize = m_instruments.size();
        user_instrument temp(name);
        result = temp.is_valid();
        if (result)
        {
            m_instruments.push_back(temp);
            result = m_instruments.size() == currentsize + 1;
        }
    }
    return result;
}

/**
 * \getter m_midi_buses[index] (internal function)
 *      If the index is out of range, then an invalid object is returned.
 *      This invalid object has an empty alias, and all the instrument
 *      numbers are -1.
 */

user_midi_bus &
user_settings::private_bus (int index)
{
    static user_midi_bus s_invalid;                     /* invalid by default */
    if (index >= 0 && index < int(m_midi_buses.size()))
        return m_midi_buses[index];
    else
        return s_invalid;
}

/**
 * \getter m_midi_buses[index].instrument[channel]
 *      Currently this function is used, in the userfile::parse()
 *      function.
 */

void
user_settings::set_bus_instrument (int index, int channel, int instrum)
{
    user_midi_bus & mb = private_bus(index);
    mb.set_instrument(channel, instrum);
}

/**
 * \getter m_instruments[index]
 *      If the index is out of range, then a invalid object is returned.
 *      This invalid object has an empty(), instrument name, false for all
 *      controllers_active[] values, and empty controllers[] string values.
 */

user_instrument &
user_settings::private_instrument (int index)
{
    static user_instrument s_invalid;
    if (index >= 0 && index < int(m_instruments.size()))
        return m_instruments[index];
    else
        return s_invalid;
}

/**
 * \setter m_midi_instrument_defs[index].controllers, controllers_active
 */

void
user_settings::set_instrument_controllers
(
    int index,
    int cc,
    const std::string & ccname,
    bool isactive
)
{
    user_instrument & mi = private_instrument(index);
    mi.set_controller(cc, ccname, isactive);
}

/**
 * \setter m_window_scale
 */

void
user_settings::window_scale (float winscale)
{
    if (winscale >= SEQ64_WINDOW_SCALE_MIN && winscale <= SEQ64_WINDOW_SCALE_MAX)
    {
        m_window_scale = winscale;
        normalize();
    }
}

/**
 * \setter m_grid_style
 */

void
user_settings::grid_style (int gridstyle)
{
    if (gridstyle >= int(grid_style_normal) && gridstyle < int(grid_style_max))
        m_grid_style = mainwid_grid_style_t(gridstyle);
}

/**
 * \setter m_mainwnd_rows
 *      This value is not modified unless the value parameter is
 *      between 4 and 8, inclusive.  The default value is 4.
 *      Dependent values are recalculated after the assignment.
 */

void
user_settings::mainwnd_rows (int value)
{
    if (value >= SEQ64_MIN_MAINWND_ROWS && value <= SEQ64_MAX_MAINWND_ROWS)
    {
        m_mainwnd_rows = value;
        normalize();
    }
}

/**
 * \setter m_mainwnd_cols
 *      This value is not modified unless the value parameter is
 *      between 8 and 8, inclusive.  The default value is 8.
 *      Dependent values are recalculated after the assignment.
 */

void
user_settings::mainwnd_cols (int value)
{
    if (value >= SEQ64_MIN_MAINWND_COLUMNS && value <= SEQ64_MAX_MAINWND_COLUMNS)
    {
        m_mainwnd_cols = value;
        normalize();
    }
}

/**
 * \setter m_max_sets
 *      This value is not modified unless the value parameter is between 16
 *      and 32.  The default value is 32.  Dependent values are recalculated
 *      after the assignment.
 *
 * \param value
 *      Provides the desired setting.  It might be modified by the call to
 *      normalize().  Not sure we really need this function.
 */

void
user_settings::max_sets (int value)
{
    if (value >= SEQ64_MIN_SET_MAX && value <= SEQ64_DEFAULT_SET_MAX)
        m_max_sets = value;

    normalize();
}

/**
 * \setter m_text_x
 *      This value is not modified unless the value parameter is between 6 and
 *      6, inclusive.  The default value is 6.  Dependent values are
 *      recalculated after the assignment.  This value is currently
 *      restricted, until we can code up a bigger font.
 */

void
user_settings::text_x (int value)
{
    if (value == 6)
    {
        m_text_x = value;
        normalize();
    }
}

/**
 * \setter m_text_y
 *      This value is not modified unless the value parameter is between 12
 *      and 12, inclusive.  The default value is 12.  Dependent values are
 *      recalculated after the assignment.  This value is currently
 *      restricted, until we can code up a bigger font.
 */

void
user_settings::text_y (int value)
{
    if (value == 12)
    {
        m_text_y = value;
        normalize();
    }
}

/**
 * \setter m_seqchars_x
 *      This affects the size or crampiness of a pattern slot, and for now
 *      we will hardwire it to 15.
 */

void
user_settings::seqchars_x (int value)
{
    if (value == 15)
    {
        m_seqchars_x = value;
        normalize();
    }
}

/**
 * \setter m_seqchars_y
 *      This affects the size or crampiness of a pattern slot, and for now
 *      we will hardwire it to 5.
 */

void
user_settings::seqchars_y (int value)
{
    if (value == 5)
    {
        m_seqchars_y = value;
        normalize();
    }
}

/**
 * \setter m_seqarea_x
 */

void
user_settings::seqarea_x (int value)
{
    m_seqarea_x = value;
}

/**
 * \setter m_seqarea_y
 */

void
user_settings::seqarea_y (int value)
{
    m_seqarea_y = value;
}

/**
 * \setter m_seqarea_seq_x
 */

void
user_settings::seqarea_seq_x (int value)
{
    m_seqarea_seq_x = value;
}

/**
 * \setter m_seqarea_seq_y
 */

void
user_settings::seqarea_seq_y (int value)
{
    m_seqarea_seq_y = value;
}

/**
 * \setter m_mainwid_border
 *      This value is not modified unless the value parameter is
 *      between 0 and 3, inclusive.  The default value is 0.
 *      Dependent values are recalculated after the assignment.
 */

void
user_settings::mainwid_border (int value)
{
    if (value >= 0 && value <= 3)
    {
        m_mainwid_border = value;
        normalize();
    }
}

/**
 * \setter m_mainwid_spacing
 *      This value is not modified unless the value parameter is
 *      between 2 and 6, inclusive.  The default value is 2.
 *      Dependent values are recalculated after the assignment.
 */

void
user_settings::mainwid_spacing (int value)
{
    if (value >= 2 && value <= 6)
    {
        m_mainwid_spacing = value;
        normalize();
    }
}

/**
 * \setter m_control_height
 *      This value is not modified unless the value parameter is
 *      between 0 and 4, inclusive.  The default value is 0.
 *      Dependent values are recalculated after the assignment.
 */

void
user_settings::control_height (int value)
{
    if (value >= 0 && value <= 4)
    {
        m_control_height = value;
        normalize();
    }
}

/**
 * \setter m_current_zoom
 *      This value is not modified unless the value parameter is
 *      between 1 and 512, inclusive.  The default value is 2.  Note that 0 is
 *      allowed as a special case, which allows the default zoom to be
 *      adjusted when the PPQN value is different from the default.
 */

void
user_settings::zoom (int value)
{
    bool ok = value >= mc_min_zoom && value <= mc_max_zoom;
    if (ok || value == SEQ64_USE_ZOOM_POWER_OF_2)
        m_current_zoom = value;
}

/**
 * \setter m_midi_ppqn
 *      This value can be set from 96 to 19200 (this upper limit will be
 *      determined by what Sequencer64 can actually handle).  The default
 *      value is 192.
 */

void
user_settings::midi_ppqn (int value)
{
    if (value >= SEQ64_MINIMUM_PPQN && value <= SEQ64_MAXIMUM_PPQN)
        m_midi_ppqn = value;
}

/**
 * \setter m_midi_beats_per_measure
 *      This value can be set from 1 to 16.  The default value is 4.
 */

void
user_settings::midi_beats_per_bar (int value)
{
    if (value >= 1 && value <= 16)
        m_midi_beats_per_measure = value;
}

/**
 * \setter m_midi_bpm_minimum
 *      This value can be set from 20 to 500.  The default value is 120.
 */

void
user_settings::midi_bpm_minimum (midibpm value)
{
    if (value >= SEQ64_MINIMUM_BPM && value <= SEQ64_MAXIMUM_BPM)
        m_midi_bpm_minimum = value;
}

/**
 * \setter m_midi_beats_minute
 *      This value can be set from 20 to 500.  The default value is 120.
 */

void
user_settings::midi_beats_per_minute (midibpm value)
{
    if (value >= SEQ64_MINIMUM_BPM && value <= SEQ64_MAXIMUM_BPM)
        m_midi_beats_per_minute = value;
}

/**
 * \setter m_midi_bpm_maximum
 *      This value can be set from 20 to 500.  The default value is 120.
 */

void
user_settings::midi_bpm_maximum (midibpm value)
{
    if (value >= SEQ64_MINIMUM_BPM && value <= SEQ64_MAXIMUM_BPM)
        m_midi_bpm_maximum = value;
}

/**
 * \setter m_midi_beatwidth
 *      This value can be set to any power of 2 in the range from 1 to 16.
 *      The default value is 4.
 */

void
user_settings::midi_beat_width (int bw)
{
    if (bw == 1 || bw == 2 || bw == 4 || bw == 8 ||bw == 16)
        m_midi_beat_width = bw;
}

/**
 * \setter m_midi_buss_override
 *      This value can be set from 0 to 31.  The default value is -1, which
 *      means that there is no buss override.  It provides a way to override
 *      the buss number for smallish MIDI files.  It replaces the buss-number
 *      read from the file.  This option is turned on by the --bus option, and
 *      is merely a convenience feature for the quick previewing of a tune.
 *      (It's called "developer laziness".)
 */

void
user_settings::midi_buss_override (char buss)
{
    if
    (
        (buss >= 0 && buss < SEQ64_DEFAULT_BUSS_MAX) ||
        SEQ64_NO_BUSS_OVERRIDE(buss)
    )
    {
        m_midi_buss_override = buss;
    }
}

/**
 * \setter m_velocity_override
 */

void
user_settings::velocity_override (int vel)
{
    if (vel > SEQ64_MAX_NOTE_ON_VELOCITY)
        vel = SEQ64_MAX_NOTE_ON_VELOCITY;
    else if (vel < 0)
        vel = SEQ64_PRESERVE_VELOCITY;

    m_velocity_override = vel;
}

/**
 * \setter m_bpm_precision
 */

void
user_settings::bpm_precision (int precision)
{
    if (precision > SEQ64_MAXIMUM_BPM_PRECISION)
        precision = SEQ64_MAXIMUM_BPM_PRECISION;
    else if (precision < SEQ64_MINIMUM_BPM_PRECISION)
        precision = SEQ64_MINIMUM_BPM_PRECISION;

    m_bpm_precision = precision;
}

/**
 * \setter m_bpm_step_increment
 */

void
user_settings::bpm_step_increment (midibpm increment)
{
    if (increment > SEQ64_MAXIMUM_BPM_INCREMENT)
        increment = SEQ64_MAXIMUM_BPM_INCREMENT;
    else if (increment < SEQ64_MINIMUM_BPM_INCREMENT)
        increment = SEQ64_MINIMUM_BPM_INCREMENT;

    m_bpm_step_increment = increment;
}

/**
 * \setter m_bpm_page_increment
 */

void
user_settings::bpm_page_increment (midibpm increment)
{
    if (increment > SEQ64_MAXIMUM_BPM_INCREMENT)
        increment = SEQ64_MAXIMUM_BPM_INCREMENT;
    else if (increment < SEQ64_MINIMUM_BPM_INCREMENT)
        increment = SEQ64_MINIMUM_BPM_INCREMENT;

    m_bpm_page_increment = increment;
}

/**
 *  Sets the horizontal page increment size for the horizontal scrollbar of a
 *  perfedit window.  This value ranges from 1 (the original value, really too
 *  small for a "page" operation) to 6 (which is 24 measures, the same as
 *  the typical width of the perfroll)
 */

void
user_settings::perf_h_page_increment (int inc)
{
    if (inc >= 1 && inc <= 6)
        m_h_perf_page_increment = inc;
}

/**
 *  Sets the vertical page increment size for the vertical scrollbar of a
 *  perfedit window.  This value ranges from 1 (the original value, really too
 *  small for a "page" operation) to 18 (which is 18 tracks, slightly more
 *  than the typical height of the perfroll)
 */

void
user_settings::perf_v_page_increment (int inc)
{
    if (inc >= 1 && inc <= 18)
        m_v_perf_page_increment = inc;
}

/**
 * \getter m_user_option_logfile
 *
 * \return
 *      This function returns rc().config_directory() + m_user_option_logfile
 *      if the latter does not contain a path marker ("/").  Otherwise, it
 *      returns m_user_option_logfile, which must be a full path specification
 *      to the desired log-file.
 */

std::string
user_settings::option_logfile () const
{
    std::string result;
    if (! m_user_option_logfile.empty())
    {
        std::size_t slashpos = m_user_option_logfile.find_first_of("/");
        if (slashpos == std::string::npos)
        {
            result = rc().home_config_directory();
            char lastchar = result[result.length() - 1];
            if (lastchar != '/')
                result += '/';
        }
        result += m_user_option_logfile;
    }
    return result;
}

/**
 * \setter m_mainwid_block_rows
 */

void
user_settings::block_rows (int count)
{
#if defined SEQ64_MAINWID_BLOCK_ROWS_MAX
    if (count > 0 && count <= SEQ64_MAINWID_BLOCK_ROWS_MAX)
        m_mainwid_block_rows = count;
#else
    if (count == 1)
        m_mainwid_block_rows = count;
#endif
}

/**
 * \setter m_mainwid_block_cols
 */

void
user_settings::block_columns (int count)
{
#if defined SEQ64_MAINWID_BLOCK_ROWS_MAX
    if (count > 0 && count <= SEQ64_MAINWID_BLOCK_COLS_MAX)
        m_mainwid_block_cols = count;
#else
    if (count == 1)
        m_mainwid_block_cols = count;
#endif
}

/*
 *  Derived calculations
 */

/**
 *  Replaces the hard-wired calculation in the mainwid module.
 *  Affected by the c_mainwid_border and c_mainwid_spacing values.
 *
 * \return
 *      Returns the width, in pixels, of a mainwid grid.
 */

int
user_settings::mainwid_width () const
{
    int result = (m_seqarea_x + m_mainwid_spacing) * m_mainwnd_cols -
        m_mainwid_spacing + m_mainwid_border * 2 + mainwid_width_fudge() * 2;

    return scale_size(result);
}

/**
 *  Replaces the hard-wired calculation in the mainwid module.
 *  Affected by the c_mainwid_border and c_control_height values.
 *
 * \change ca 2017-08-13 Issue #104.
 *      Add 8 to the height calculation until we can figure out how to adjust
 *      for button height increases due to adding the main-window time-stamp
 *      field.
 *
 * \return
 *      Returns the height, in pixels, of a mainwid grid.
 */

int
user_settings::mainwid_height () const
{
    int result = (c_seqarea_y + c_mainwid_spacing) * m_mainwnd_rows +
         (c_control_height + c_mainwid_border * 2) + // MAINWID_HEIGHT_FUDGE
         mainwid_height_fudge() * 2;

    return scale_size(result);
}

/**
 *  Provides a debug dump of basic information to help debug a
 *  surprisingly intractable problem with all busses having the name and
 *  values of the last buss in the configuration.  Does its work only if
 *  PLATFORM_DEBUG and SEQ64_USE_DEBUG_OUTPUT are defined.  Only enabled in
 *  emergencies :-D.
 */

void
user_settings::dump_summary ()
{
#if defined SEQ64_USE_DEBUG_OUTPUT
    int buscount = bus_count();
    printf("[user-midi-bus-definitions] %d busses\n", buscount);
    for (int b = 0; b < buscount; ++b)
    {
        const user_midi_bus & umb = bus(b);
        printf("   [user-midi-bus-%d] '%s'\n", b, umb.name().c_str());
    }

    int instcount = instrument_count();
    printf("[user-instrument-definitions] %d instruments\n", instcount);
    for (int i = 0; i < instcount; ++i)
    {
        const user_instrument & umi = instrument(i);
        printf("   [user-instrument-%d] '%s'\n", i, umi.name().c_str());
    }
    printf("\n");
    printf
    (
        "   mainwnd_rows() = %d\n"
        "   mainwnd_cols() = %d\n"
        "   seqs_in_set() = %d\n"
        "   gmute_tracks() = %d\n"
        "   max_sets() = %d\n"
        "   max_sequence() = %d\n"
        "   text_x(), _y() = %d, %d\n"
        ,
        mainwnd_rows(),
        mainwnd_cols(),
        seqs_in_set(),
        gmute_tracks(),
        max_sets(),
        max_sequence(),
        text_x(), text_y()
    );
    printf
    (
        "   seqchars_x(), _y() = %d, %d\n"
        "   seqarea_x(), _y() = %d, %d\n"
        "   seqarea_seq_x(), _y() = %d, %d\n"
        "   mainwid_border() = %d\n"
        "   mainwid_spacing() = %d\n"
        "   mainwid_x(), _y() = %d, %d\n"
        "   control_height() = %d\n"
        ,
        seqchars_x(), seqchars_y(),
        seqarea_x(), seqarea_y(),
        seqarea_seq_x(), seqarea_seq_y(),
        mainwid_border(),
        mainwid_spacing(),
        mainwid_x(), mainwid_y(),
        control_height()
    );
    printf("\n");
    printf
    (
        "   midi_ppqn() = %d\n"
        "   midi_beats_per_bar() = %d\n"
        "   midi_beats_per_minute() = %g\n"
        "   midi_beat_width() = %d\n"
        "   midi_buss_override() = %d\n"
        ,
        midi_ppqn(),
        midi_beats_per_bar(),
        midi_beats_per_minute(),
        midi_beat_width(),
        midi_buss_override()
    );
#endif      // PLATFORM_DEBUG
}

}           // namespace seq64

/*
 * user_settings.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

