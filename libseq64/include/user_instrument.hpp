#ifndef SEQ64_USER_INSTRUMENT_HPP
#define SEQ64_USER_INSTRUMENT_HPP

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
 * \file          user_instrument.hpp
 *
 *  This module declares/defines the user instrument section of the "user"
 *  configuration file.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-09-25
 * \updates       2015-09-26
 * \license       GNU GPLv2 or above
 *
 */

#include <string>

#include "easy_macros.h"               // with platform_macros.h, too

/**
 *  Default value for c_max_instruments.
 */

#define DEFAULT_INSTRUMENT_MAX           64

/**
 *  Provides the maximum number of instruments that can be defined in the
 *  <tt> ~/.seq24usr </tt> or <tt> ~/.config/sequencer64/sequencer64.rc </tt>
 *  file.  With a value of 64, this is more of a sanity-check than a
 *  realistic number of instruments defined by a user.
 */

const int c_max_instruments = DEFAULT_INSTRUMENT_MAX;

/**
 *  Manifest constant for the maximum value limit of a MIDI byte when used
 *  to limit the size of an array.  Here, it is the upper limit on the
 *  number of MIDI controllers that can be supported.
 */

#define MIDI_CONTROLLER_MAX             128

/**
 *  This structure corresponds to <tt> [user-instrument-N] </tt>
 *  definitions in the <tt> ~/.seq24usr </tt> or
 *  <tt> ~/.config/sequencer64/sequencer64.rc </tt> file.
 */

struct user_instrument_t
{
    std::string instrument;
    std::string controllers[MIDI_CONTROLLER_MAX];
    bool controllers_active[MIDI_CONTROLLER_MAX];
};

/**
 *  Provides data about the MIDI instruments, readable from the "user"
 *  configuration file.  Will later make the size adjustable, if it
 *  makes sense to do so.
 */

class user_instrument
{

    /**
     *  Provides a validity flag, useful in returning a reference to a
     *  bogus object for internal error-check.  Callers should check
     *  this flag via the is_valid() accessor before using this object.
     *  This flag is set to true when any valid member assignment occurs
     *  via a public setter call.  However, setting an empty name for the
     *  instrument member will render the object invalid.
     */

    bool m_is_valid;

    /**
     *  The instance of the structure that this class wraps.
     */

    user_instrument_t m_instrument_def;    // [c_max_instruments];

public:

    user_instrument (const std::string & name = "");
    user_instrument (const user_instrument & rhs);
    user_instrument & operator = (const user_instrument & rhs);

    /**
     * \getter m_is_valid
     */

    bool is_valid () const
    {
        return m_is_valid;
    }

    void set_defaults ();
    void set_global (int instrum) const;
    void get_global (int instrum);

    /**
     * \getter m_instrument_def.instrument (name of instrument)
     */

    const std::string & name () const
    {
        return m_instrument_def.instrument;
    }


    /**
     * \getter MIDI_CONTROLLER_MAX
     *      Remember that the controller numbers for each MIDI instrument
     *      range from 0 to 127 (MIDI_CONTROLLER_MAX-1).
     */

    int controller_count () const
    {
        return MIDI_CONTROLLER_MAX;
    }

    const std::string & controller_name (int c) const;
    bool controller_active (int c) const;

    void set_controller
    (
        int c, const std::string & cname, bool isactive
    );

private:

    void set_name (const std::string & instname);
    void copy_definitions (const user_instrument & rhs);

};

#endif  // SEQ64_USER_INSTRUMENT_HPP

/*
 * user_instrument.hpp
 *
 * vim: sw=4 ts=4 wm=8 et ft=cpp
 */
