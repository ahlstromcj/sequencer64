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
 * \updates       2015-11-15
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
     Zseqmax    pulses/pixel    32          Seq editor max zoom out
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

#include "globals.h"                    /* to support legacy variables */
#include "user_settings.hpp"

namespace seq64
{

/**
 *  Default constructor.
 */

user_settings::user_settings ()
 :
    /*
     * [user-midi-bus-definitions]
     */

    m_midi_buses                (),         // vector

    /*
     * [user-instrument-definitions]
     */

    m_instruments               (),         // vector

    /*
     * [user-interface-settings]
     */

    m_grid_style                (grid_style_normal),
    m_grid_brackets             (1),
    m_mainwnd_rows              (0),
    m_mainwnd_cols              (0),
    m_max_sets                  (0),
    m_mainwid_border            (0),
    m_mainwid_spacing           (0),
    m_control_height            (0),
    m_current_zoom              (0),
    m_global_seq_feature_save   (false),  // will be true once supported
    m_seqedit_scale             (0),
    m_seqedit_key               (0),
    m_seqedit_bgsequence        (0),
    m_use_new_font              (false),

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
    m_midi_beats_per_minute     (0),
    m_midi_beat_width           (0),
    m_midi_buss_override        (0),

    /*
     * Calculated from other member values in the normalize() function.
     */

    m_total_seqs                (0),
    m_seqs_in_set               (0),
    m_gmute_tracks              (0),
    m_max_sequence              (0),
    m_seqarea_x                 (0),
    m_seqarea_y                 (0),
    m_seqarea_seq_x             (0),
    m_seqarea_seq_y             (0),
    m_mainwid_x                 (0),
    m_mainwid_y                 (0),

    /*
     * Constant values.
     */

    mc_min_zoom                 (1),
    mc_max_zoom                 (32),
    mc_baseline_ppqn            (SEQ64_DEFAULT_PPQN)
{
    // Empty body; it's no use to call normalize() here, see set_defaults().
}

/**
 *  Copy constructor.
 */

user_settings::user_settings (const user_settings & rhs)
 :
    /*
     * [user-midi-bus-definitions]
     */

    m_midi_buses                (),                     // vector

    /*
     * [user-instrument-definitions]
     */

    m_instruments               (),                     // vector

    /*
     * [user-interface-settings]
     */

    m_grid_style                (rhs.m_grid_style),
    m_grid_brackets             (rhs.m_grid_brackets),
    m_mainwnd_rows              (rhs.m_mainwnd_rows),
    m_mainwnd_cols              (rhs.m_mainwnd_cols),
    m_max_sets                  (rhs.m_max_sets),
    m_mainwid_border            (rhs.m_mainwid_border),
    m_mainwid_spacing           (rhs.m_mainwid_spacing),
    m_control_height            (rhs.m_control_height),
    m_current_zoom              (rhs.m_current_zoom),
    m_global_seq_feature_save   (rhs.m_global_seq_feature_save),
    m_seqedit_scale             (rhs.m_seqedit_scale),
    m_seqedit_key               (rhs.m_seqedit_key),
    m_seqedit_bgsequence        (rhs.m_seqedit_bgsequence),
    m_use_new_font              (rhs.m_use_new_font),

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
    m_midi_beats_per_minute     (rhs.m_midi_beats_per_minute),
    m_midi_beat_width           (rhs.m_midi_beat_width),
    m_midi_buss_override        (rhs.m_midi_buss_override),

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

    /*
     * Constant values.
     */

    mc_min_zoom                 (rhs.mc_min_zoom),
    mc_max_zoom                 (rhs.mc_max_zoom),
    mc_baseline_ppqn            (SEQ64_DEFAULT_PPQN)
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
        /*
         * [user-midi-bus-definitions]
         */

        m_midi_buses                = rhs.m_midi_buses;

        /*
         * [user-instrument-definitions]
         */

        m_instruments               = rhs.m_instruments;

        /*
         * [user-interface-settings]
         */

        m_grid_style                = rhs.m_grid_style;
        m_grid_brackets             = rhs.m_grid_brackets;
        m_mainwnd_rows              = rhs.m_mainwnd_rows;
        m_mainwnd_cols              = rhs.m_mainwnd_cols;
        m_max_sets                  = rhs.m_max_sets;
        m_mainwid_border            = rhs.m_mainwid_border;
        m_mainwid_spacing           = rhs.m_mainwid_spacing;
        m_control_height            = rhs.m_control_height;
        m_current_zoom              = rhs.m_current_zoom;
        m_global_seq_feature_save   = rhs.m_global_seq_feature_save;
        m_seqedit_scale             = rhs.m_seqedit_scale;
        m_seqedit_key               = rhs.m_seqedit_key;
        m_seqedit_bgsequence        = rhs.m_seqedit_bgsequence;
        m_use_new_font              = rhs.m_use_new_font;

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
        m_midi_beats_per_minute     = rhs.m_midi_beats_per_minute;
        m_midi_beat_width           = rhs.m_midi_beat_width;
        m_midi_buss_override        = rhs.m_midi_buss_override;

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
         */

        normalize();

        /*
         * Constant values.  These values cannot be modified.
         *
         * mc_min_zoom              = rhs.mc_min_zoom;
         * mc_max_zoom              = rhs.mc_max_zoom;
         * mc_baseline_ppqn         = rhs.mc_baseline_ppqn;
         */
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
    m_midi_buses.clear();
    m_instruments.clear();

    m_grid_style = grid_style_normal;   // range: 0-2
    m_grid_brackets = 1;                // range: -30 to 0 to 30
    m_mainwnd_rows = 4;                 // range: 4-8
    m_mainwnd_cols = 8;                 // range: 8-10
    m_max_sets = 32;                    // range: 32-64
    m_mainwid_border = 0;               // range: 0-3, try 2 or 3
    m_mainwid_spacing = 2;              // range: 2-6, try 4 or 6
    m_control_height = 0;               // range: 0-4?
    m_current_zoom = 2;                 // range: 1-32
    m_global_seq_feature_save = true;
    m_seqedit_scale = int(c_scale_off); // range: c_scale_off to < c_scale_size
    m_seqedit_key = SEQ64_KEY_OF_C;     // range: 0-11
    m_seqedit_bgsequence = SEQ64_NULL_SEQUENCE; // range -1, 0, 1, 2, ...
    m_use_new_font = ! rc().legacy_format();

    m_text_x =  6;                      // range: 6-6
    m_text_y = 12;                      // range: 12-12
    m_seqchars_x = 15;                  // range: 15-15
    m_seqchars_y =  5;                  // range: 5-5

    m_midi_ppqn = SEQ64_DEFAULT_PPQN;   // range: 96 to 960, default 192
    m_midi_beats_per_measure = SEQ64_DEFAULT_BEATS_PER_MEASURE; // range: 1-16
    m_midi_beats_per_minute = SEQ64_DEFAULT_BPM;                // range: 20-500
    m_midi_beat_width = SEQ64_DEFAULT_BEAT_WIDTH;     // range: 1-16, powers of 2
    m_midi_buss_override = SEQ64_BAD_BUSS;            // range: 1 to 32

    /*
     * mc_min_zoom
     * mc_max_zoom
     * mc_baseline_ppqn
     */

    normalize();                        // recalculate derived values
}

/**
 *  Calculate the derived values from the already-set values.
 */

void
user_settings::normalize ()
{
    m_seqs_in_set = m_mainwnd_rows * m_mainwnd_cols;
    m_gmute_tracks = m_seqs_in_set * m_seqs_in_set;
    m_total_seqs = m_seqs_in_set * m_max_sets;
    m_max_sequence = m_seqs_in_set * m_max_sets;
    m_seqarea_x = m_text_x * m_seqchars_x;
    m_seqarea_y = m_text_y * m_seqchars_y;
    m_seqarea_seq_x = m_text_x * 13;
    m_seqarea_seq_y = m_text_y * 2;
    m_mainwid_x =
    (
        (m_seqarea_x + m_mainwid_spacing) * m_mainwnd_cols -
            m_mainwid_spacing + m_mainwid_border * 2
    );
    m_mainwid_y =
    (
        (m_seqarea_y + m_mainwid_spacing) * m_mainwnd_rows +
             m_control_height + m_mainwid_border * 2
    );
}

/**
 *  Copies the current values of the member variables into their
 *  corresponding global variables.  Should be called at initialization,
 *  and after settings are read from the "user" configuration file.
 *
 *  DO NOT PUT ANY GLOBALS HERE UNTIL THEIR EFFECTS HAVE BEEN TESTED!!!!
 */

void
user_settings::set_globals () const
{
    /*
     * Done with the full conversion to [user-midi-bus] and
     * [user-instrument] values, they don't need to be here anymore.
     */

    /*
     *  [user-interface-settings]
     *
     *  We're ignoring these for now, don't want to mess up the GUI.
     */

    /*
     *  [user-midi-settings]:  No more globals to set, hurrah!
     */
}

/**
 *  Copies the current values of the global variables into their
 *  corresponding member variables.  Should be called before settings are
 *  written to the "user" configuration file.
 */

void
user_settings::get_globals ()
{
    /*
     * Done with the full conversion to [user-midi-bus] and
     * [user-instrument] values, they don't need to be here anymore.
     */

    /*
     *  [user-interface-settings]
     *
     *  We're ignoring these for now, don't want to mess up the GUI.
     */

    /*
     *  [user-midi-settings]: No more globals to set, hurrah!
     */
}

/**
 *  Adds a user bus to the container, but only does so if the name
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
    if (value >= 4 && value <= 8)
    {
        m_mainwnd_rows = value;
        normalize();
    }
}

/**
 * \setter m_mainwnd_cols
 *      This value is not modified unless the value parameter is
 *      between 8 and 10, inclusive.  The default value is 8.
 *      Dependent values are recalculated after the assignment.
 */

void
user_settings::mainwnd_cols (int value)
{
    if (value >= 8 && value <= 10)
    {
        m_mainwnd_cols = value;
        normalize();
    }
}

/**
 * \setter m_max_sets
 *      This value is not modified unless the value parameter is
 *      between 32 and 64, inclusive.  The default value is 32.
 *      Dependent values are recalculated after the assignment.
 */

void
user_settings::max_sets (int value)
{
    if (value >= 32 && value <= 64)
    {
        m_max_sets = value;
        normalize();
    }
}

/**
 * \setter m_text_x
 *      This value is not modified unless the value parameter is
 *      between 6 and 6, inclusive.  The default value is 6.
 *      Dependent values are recalculated after the assignment.
 *      This value is currently restricted, until we can code up a bigger
 *      font.
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
 *      This value is not modified unless the value parameter is
 *      between 12 and 12, inclusive.  The default value is 12.
 *      Dependent values are recalculated after the assignment.
 *      This value is currently restricted, until we can code up a bigger
 *      font.
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
 *      between 1 and 32, inclusive.  The default value is 2.
 */

void
user_settings::zoom (int value)
{
    if (value >= mc_min_zoom && value <= mc_max_zoom)
    {
        m_current_zoom = value;
    }
}

/**
 * \setter m_midi_ppqn
 *      This value can be set from 96 to 960 (this upper limit will be
 *      determined by what Sequencer64 can actually handle).
 *      The default value is 192.
 *      Dependent values may be recalculated after the assignment.
 */

void
user_settings::midi_ppqn (int value)
{
    if (value >= 96 && value <= 960)
    {
        m_midi_ppqn = value;

        /*
         * Any need to normalize()?
         */
    }
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
 * \setter m_midi_beats_minute
 *      This value can be set from 20 to 500.  The default value is 120.
 */

void
user_settings::midi_beats_per_minute (int value)
{
    if (value >= SEQ64_MINIMUM_BPM && value <= SEQ64_MAXIMUM_BPM)
        m_midi_beats_per_minute = value;
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
        "   midi_beats_per_minute() = %d\n"
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

