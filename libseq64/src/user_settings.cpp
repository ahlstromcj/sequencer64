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

#include "globals.h"                    /* to support legacy variables */

/**
 * \file          user_settings.cpp
 *
 *  This module declares/defines just some of the global (gasp!) variables
 *  in this application.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-09-23
 * \updates       2015-09-24
 * \license       GNU GPLv2 or above
 *
 *  Note that this module also sets the legacy global variables, so that
 *  they can be used by modules that have not yet been cleaned up.
 */

#include "user_settings.hpp"

/**
 *  Default constructor.
 */

user_settings::user_settings ()
 :
    m_midi_bus_defs     (),     // currently a constant-size array
    m_instrument_defs   (),     // currently a constant-size array
    m_mainwnd_rows      (0),
    m_mainwnd_cols      (0),
    m_seqs_in_set       (0),
    m_gmute_tracks      (0),
    m_max_sets          (0),
    m_total_seqs        (0),
    m_max_sequence      (0),
    m_text_x            (0),
    m_text_y            (0),
    m_seqchars_x        (0),
    m_seqchars_y        (0),
    m_seqarea_x         (0),
    m_seqarea_y         (0),
    m_seqarea_seq_x     (0),
    m_seqarea_seq_y     (0),
    m_mainwid_border    (0),
    m_mainwid_spacing   (0),
    m_control_height    (0),
    m_mainwid_x         (0),
    m_mainwid_y         (0)
{
    // Empty body
}

/**
 *  Copy constructor.
 */

user_settings::user_settings (const user_settings & rhs)
 :
    m_midi_bus_defs     (),                     // a constant-size array
    m_instrument_defs   (),                     // a constant-size array
    m_mainwnd_rows      (rhs.m_mainwnd_rows),
    m_mainwnd_cols      (rhs.m_mainwnd_cols),
    m_seqs_in_set       (rhs.m_seqs_in_set),
    m_gmute_tracks      (rhs.m_gmute_tracks),
    m_max_sets          (rhs.m_max_sets),
    m_total_seqs        (rhs.m_total_seqs),
    m_max_sequence      (rhs.m_max_sequence),
    m_text_x            (rhs.m_text_x),
    m_text_y            (rhs.m_text_y),
    m_seqchars_x        (rhs.m_seqchars_x),
    m_seqchars_y        (rhs.m_seqchars_y),
    m_seqarea_x         (rhs.m_seqarea_x),
    m_seqarea_y         (rhs.m_seqarea_y),
    m_seqarea_seq_x     (rhs.m_seqarea_seq_x),
    m_seqarea_seq_y     (rhs.m_seqarea_seq_y),
    m_mainwid_border    (rhs.m_mainwid_border),
    m_mainwid_spacing   (rhs.m_mainwid_spacing),
    m_control_height    (rhs.m_control_height),
    m_mainwid_x         (rhs.m_mainwid_x),
    m_mainwid_y         (rhs.m_mainwid_y)
{
    copy_definitions(rhs);
}

/**
 *  Principal assignment operator.
 */

user_settings &
user_settings::operator = (const user_settings & rhs)
{
    if (this != &rhs)
    {
        // m_midi_bus_defs     = rhs.m_midi_bus_defs;   // a constant-size array
        // m_instrument_defs   = rhs.m_instrument_defs; // a constant-size array

        copy_definitions(rhs);
        m_mainwnd_rows      = rhs.m_mainwnd_rows;
        m_mainwnd_cols      = rhs.m_mainwnd_cols;
        m_seqs_in_set       = rhs.m_seqs_in_set;
        m_gmute_tracks      = rhs.m_gmute_tracks;
        m_max_sets          = rhs.m_max_sets;
        m_total_seqs        = rhs.m_total_seqs;
        m_max_sequence      = rhs.m_max_sequence;
        m_text_x            = rhs.m_text_x;
        m_text_y            = rhs.m_text_y;
        m_seqchars_x        = rhs.m_seqchars_x;
        m_seqchars_y        = rhs.m_seqchars_y;
        m_seqarea_x         = rhs.m_seqarea_x;
        m_seqarea_y         = rhs.m_seqarea_y;
        m_seqarea_seq_x     = rhs.m_seqarea_seq_x;
        m_seqarea_seq_y     = rhs.m_seqarea_seq_y;
        m_mainwid_border    = rhs.m_mainwid_border;
        m_mainwid_spacing   = rhs.m_mainwid_spacing;
        m_control_height    = rhs.m_control_height;
        m_mainwid_x         = rhs.m_mainwid_x;
        m_mainwid_y         = rhs.m_mainwid_y;
    }
    return *this;
}

/**
 *  Sets the default values.
 */

void
user_settings::set_defaults ()
{
    m_mainwnd_rows = 4;                 // range: 4 to 8
    m_mainwnd_cols = 8;                 // range: 8 to 10
    m_max_sets = 32;                    // range: 32 to 64
    m_text_x =  6;                      // range: 6 to 6
    m_text_y = 12;                      // range: 12 to 12
    m_seqchars_x = 15;                  // range: 15 to 15
    m_seqchars_y =  5;                  // range: 5 to 5
    m_mainwid_border = 0;               // range: 0 to 3, try 2 or 3
    m_mainwid_spacing = 2;              // range: 2 to 6, try 4 or 6
    m_control_height = 0;               // range: 0 to 4
    normalize();                        // recalculate derived values
    prepare_midi_defs();                // clears out the arrays
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
 */

void
user_settings::globalize ()
{
    for (int i = 0; i < c_max_busses; i++)
    {
        for (int j = 0; j < 16; j++)
            global_user_midi_bus_definitions[i].instrument[j] =
                m_midi_bus_defs[i].instrument[j];
    }
    for (int i = 0; i < c_max_instruments; i++)
    {
        for (int j = 0; j < MIDI_COUNT_MAX; j++)
            global_user_instrument_definitions[i].controllers_active[j] =
                m_instrument_defs[i].controllers_active[j];
    }
}

/**
 *  Copies the current values of the global variables into their
 *  corresponding member variables.  Should be called before settings are
 *  written to the "user" configuration file.
 */

void
user_settings::get_globals ()
{
    for (int i = 0; i < c_max_busses; i++)
    {
        for (int j = 0; j < 16; j++)
            m_midi_bus_defs[i].instrument[j] =
                global_user_midi_bus_definitions[i].instrument[j];
    }
    for (int i = 0; i < c_max_instruments; i++)
    {
        for (int j = 0; j < MIDI_COUNT_MAX; j++)
            m_instrument_defs[i].controllers_active[j] =
                global_user_instrument_definitions[i].controllers_active[j];
    }
}

/**
 *  Copies the array members from one instance of user_settings to this
 *  one.
 */

void
user_settings::copy_definitions (const user_settings & rhs)
{
    for (int i = 0; i < c_max_busses; i++)
    {
        for (int j = 0; j < 16; j++)
            m_midi_bus_defs[i].instrument[j] =
                rhs.m_midi_bus_defs[i].instrument[j];
    }
    for (int i = 0; i < c_max_instruments; i++)
    {
        for (int j = 0; j < MIDI_COUNT_MAX; j++)
            m_instrument_defs[i].controllers_active[j] =
                rhs.m_instrument_defs[i].controllers_active[j];
    }
}

/**
 * \getter m_midi_bus_defs[index]
 *      If the index is out of range, then a bogus value is returned.
 *      This bogus value has an alias of "bogus", and all the instrument
 *      numbers are -1.
 */

user_midi_bus_t &
user_settings::bus (int index)
{
    static user_midi_bus_t s_bogus;
    static bool s_bogus_is_set = false;
    if (! s_bogus_is_set)
    {
        s_bogus_is_set = true;
        s_bogus.alias = "bogus";
        for (int i = 0; i < MIDI_BUS_CHANNEL_MAX; ++i)       // 16
        {
            s_bogus.instrument[i] = -1;     /* disable channel i */
        }
    }
    if (index >= 0 && index < c_max_busses)
        return m_midi_bus_defs[index];
    else
        return s_bogus;
}

/**
 * \getter m_midi_bus_defs[index].alias
 */

void
user_settings::bus_alias (int index, const std::string & ale)
{
    user_midi_bus_t & mb = bus(index);
    if (mb.alias != "bogus")
        mb.alias = ale;
}

/**
 * \getter m_midi_bus_defs[index].instrument[channel]
 */

void
user_settings::bus_instrument (int index, int channel, int instrum)
{
    user_midi_bus_t & mb = bus(index);
    if (mb.alias != "bogus")
    {
        if (channel >= 0 && channel < MIDI_BUS_CHANNEL_MAX)     // 16
            mb.instrument[channel] = instrum;
    }
}

/**
 * \getter m_instrument_defs[index]
 *      If the index is out of range, then a bogus value is returned.
 *      This bogus value has a "bogus" instrument name, false for all
 *      controllers_active[] values, and empty controllers[] string values.
 */

user_instrument_t &
user_settings::instrument (int index)
{
    static user_instrument_t s_bogus;
    static bool s_bogus_is_set = false;
    if (! s_bogus_is_set)
    {
        s_bogus_is_set = true;
        s_bogus.instrument = "bogus";
        for (int i = 0; i < MIDI_COUNT_MAX; ++i)       // 16
        {
            s_bogus.controllers_active[i] = false;
            s_bogus.controllers[i].clear();
        }
    }
    if (index >= 0 && index < c_max_instruments)
        return m_instrument_defs[index];
    else
        return s_bogus;
}

/**
 * \getter m_midi_instrument_defs[index].instrument
 */

void
user_settings::instrument_name (int index, const std::string & instname)
{
    user_instrument_t & mi = instrument(index);
    if (mi.instrument != "bogus")
        mi.instrument = instname;
}

/**
 * \getter m_midi_instrument_defs[index].controllers, controllers_active
 */

void
user_settings::instrument_controllers
(
    int index, int cc, const std::string & ccname, bool isactive
)
{
    user_instrument_t & mi = instrument(index);
    if (mi.instrument != "bogus")
    {
        if (cc >= 0 && cc < MIDI_COUNT_MAX)
        {
            mi.controllers[cc] = ccname;
            mi.controllers_active[cc] = isactive;
        }
    }
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
 * \setter m_seqs_in_set
 *
 * \warning
 *      This is a dependent value at present, and changing it is
 *      experimental.
 *
void
user_settings::seqs_in_set (int value)
{
    m_seqs_in_set = value;
}
 *
 */

/**
 * \setter m_gmute_tracks
 *
 * \warning
 *      This is a dependent value at present, and changing it is
 *      experimental.
 *
void
user_settings::gmute_tracks (int value)
{
    m_gmute_tracks = value;
}
 *
 */

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
 * \setter m_max_sequence
 *
 * \warning
 *      This is a dependent value at present, and changing it is
 *      experimental.
 *
void
user_settings::max_sequence (int value)
{
    m_max_sequence = value;
}
 *
 */

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

/*
 * \setter m_mainwid_x
 *
 * \warning
 *      This is a dependent value at present, and changing it is
 *      experimental.
 *
void
user_settings::mainwid_x (int value)
{
    m_mainwid_x = value;
}
 *
 */

/**
 * \setter m_mainwid_y
 *
 * \warning
 *      This is a dependent value at present, and changing it is
 *      experimental.
 *
void
user_settings::mainwid_y (int value)
{
    m_mainwid_y = value;
}
 *
 */

/**
 *  Prepare global MIDI definitions.  Why are only 16 instruments
 *  supported in the first for-loop, but 64 (see globals.h) in the
 *  second for-loop?  Well, actually the 16 applies to the number of MIDI
 *  busses, not instruments.
 */

void
user_settings::prepare_midi_defs ()
{
    for (int i = 0; i < c_max_busses; i++)
    {
        for (int j = 0; j < 16; j++)
            m_midi_bus_defs[i].instrument[j] = -1;
    }
    for (int i = 0; i < c_max_instruments; i++)
    {
        for (int j = 0; j < MIDI_COUNT_MAX; j++)
            m_instrument_defs[i].controllers_active[j] = false;
    }
}

/*
 * user_settings.cpp
 *
 * vim: sw=4 ts=4 wm=8 et ft=cpp
 */
