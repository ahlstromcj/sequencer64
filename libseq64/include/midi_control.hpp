#ifndef SEQ64_MIDI_CONTROL_HPP
#define SEQ64_MIDI_CONTROL_HPP

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
 * \file          midi_control.hpp
 *
 *  This module declares/defines the class for handling MIDI control of the
 *  application.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-11-21
 * \updates       2016-05-17
 * \license       GNU GPLv2 or above
 *
 */

#include "midibyte.hpp"                 /* seq64::midibyte                  */

namespace seq64
{

/**
 *  Pseudo control values for associating MIDI events (I think)
 *  with automation of some of the controls in seq24.  The lowest value is
 *  c_seqs_in_set * 2 = 64.
 *
 *  I think the reason for that value is to perhaps handle two sets or
 *  something like that.  Will figure it out later.
 *
 *  The controls are read in from the "rc" configuration files, and are
 *  written to the c_midictrl section of the "proprietary" final track in a
 *  Seq24/Sequencer64 MIDI file.
 */

const int c_midi_track_ctrl           = c_seqs_in_set * 2;
const int c_midi_control_bpm_up       = c_midi_track_ctrl;
const int c_midi_control_bpm_dn       = c_midi_track_ctrl + 1;
const int c_midi_control_ss_up        = c_midi_track_ctrl + 2;
const int c_midi_control_ss_dn        = c_midi_track_ctrl + 3;
const int c_midi_control_mod_replace  = c_midi_track_ctrl + 4;
const int c_midi_control_mod_snapshot = c_midi_track_ctrl + 5;
const int c_midi_control_mod_queue    = c_midi_track_ctrl + 6;
const int c_midi_control_mod_gmute    = c_midi_track_ctrl + 7;
const int c_midi_control_mod_glearn   = c_midi_track_ctrl + 8;
const int c_midi_control_play_ss      = c_midi_track_ctrl + 9;
const int c_midi_controls             = c_midi_track_ctrl + 10;

/**
 *  This class (formerly a struct) contains the control information for
 *  sequences that make up a live set.
 *
 *  Note that, although we've converted this to a full-fledged class, the
 *  ordering of variables and the data arrays used to fill them is very
 *  signifcant.  See the midifile and optionsfile modules.
 *
 *  The perform module sets up the three following arrays for each of the MIDI
 *  controls that can be defined in the "rc" file:
 *
\verbatim
    m_midi_cc_toggle[]
    m_midi_cc_on[]
    m_midi_cc_off[]
\endverbatim
 *
 *  These three arrays are specified in the "rc" by a line like the following:
 *
\verbatim
    n [0 0   0   0   0   0] [0 0   0   0   0   0] [0 0   0   0   0   0]
\endverbatim
 *
 *  where n ranges from 0 to 73.  Lines 0 to 31 provide controller values for
 *  the "pattern group", one line for each of the 32 pattern slots.
 *  Lines 32 to 63 provide controller values for
 *  the "mute in group", one line for each of the 32 pattern slots.
 *  The rest of the lines provide entries for control of:
 *  BPM up, BPM down, Screen-set up, Screen-set down, Mod Replaces, Mod
 *  Snapshot, Mod Queue, Mod gmute (group mute), Mod glearn (group learn),
 *  and Screen-set Play.
 *
 *  In each of the bracketed sections, the values correspond to the members in
 *  this order: m_active, m_inverse_active, m_status, m_data, m_min_value, and
 *  m_max_value.
 *
 *  Why are the status, data, and min/max values long?  A character or
 *  midibyte would be enough.  We'll fix that later, once we have tested this
 *  stuff.  We do need to convert them from long to int, though, and do that
 *  in the scanning and output done by optionsfile.
 */

class midi_control
{

private:

    /**
     *  Provides the value for active.
     */

    bool m_active;

    /**
     *  Provides the value for inverse-active.
     */

    bool m_inverse_active;

    /**
     *  Provides the value for the status.
     */

    int m_status;               // midibyte

    /**
     *  Provides the value for the data.
     */

    int m_data;                 // midibyte

    /**
     *  Provides the minimum value for the controller.
     */

    int m_min_value;

    /**
     *  Provides the value for the controller.
     */

    int m_max_value;

public:

    /**
     *  This default constructor creates a "zero" object.  Every member is
     *  either false or zero.
     */

    midi_control () :
        m_active            (false),
        m_inverse_active    (false),
        m_status            (0),
        m_data              (0),
        m_min_value         (0),
        m_max_value         (0)
    {
        // Empty body
    }

    /***
     * \getter m_active
     */

    bool active () const
    {
        return m_active;
    }

    /***
     * \getter m_inverse_active
     */

    bool inverse_active () const
    {
        return m_inverse_active;
    }

    /***
     * \getter m_status
     */

    int status () const
    {
        return m_status;
    }

    /***
     * \getter m_data
     */

    int data () const
    {
        return m_data;
    }

    /***
     * \getter m_min_value
     */

    int min_value () const 
    {
        return m_min_value;
    }

    /***
     * \getter m_max_value
     */

    int max_value () const
    {
        return m_max_value;
    }

    /**
     *  Not so sure if this really saves trouble for the caller.  It fits in
     *  with the big-ass sscanf() call in optionsfile.
     *
     * \param values
     *      Provides the six values, in an integer array, to set into the
     *      members in this order: m_active, m_inverse_active, m_status,
     *      m_data, m_min_value, and m_max_value.
     */

    void set (int values[6])
    {
        m_active = bool(values[0]);
        m_inverse_active = bool(values[1]);
        m_status = values[2];
        m_data = values[3];
        m_min_value = values[4];
        m_max_value = values[5];
    }

    /**
     *  Not so sure if this really saves trouble for the caller.  It fits in
     *  with the usage in midifile.
     *
     * \param values
     *      Provides the six values, in a byte array, to set into the
     *      members in this order: m_active, m_inverse_active, m_status,
     *      m_data, m_min_value, and m_max_value.
     */
    
    void set (midibyte values[6])
    {
        m_active = bool(values[0]);
        m_inverse_active = bool(values[1]);
        m_status = int(values[2]);
        m_data = int(values[3]);
        m_min_value = int(values[4]);
        m_max_value = int(values[5]);
    }

    /**
     *  Handles a common check in the perform module.
     *
     * \param status
     *      Provides the status byte, which is checked against m_status.
     *
     * \param data
     *      Provides the data byte, which is checked against m_data.
     */

    bool match (midibyte status, midibyte data) const
    {
        return
        (
            m_active && (status == m_status) && (data == m_data)
        );
    }

    /**
     *  Handles a common check in the perform module.
     */

    bool in_range (midibyte data) const
    {
        return data >= midibyte(m_min_value) && data <= midibyte(m_max_value);
    }

};          // class midi_control

}           // namespace seq64

#endif      // SEQ64_MIDI_CONTROL_HPP

/*
 * midi_control.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

