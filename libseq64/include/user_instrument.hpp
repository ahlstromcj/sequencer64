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
 * \updates       2015-11-04
 * \license       GNU GPLv2 or above
 *
 */

#include <string>

#include "easy_macros.h"               // with platform_macros.h, too

/**
 *  Default value for c_max_instruments.
 */

#define SEQ64_DEFAULT_INSTRUMENT_MAX     64

namespace seq64
{

/**
 *  Provides the maximum number of instruments that can be defined in the
 *  <code> ~/.seq24usr </code> or
 *  <code> ~/.config/sequencer64/sequencer64.rc </code>
 *  file.  With a value of 64, this is more of a sanity-check than a
 *  realistic number of instruments defined by a user.
 */

const int c_max_instruments = SEQ64_DEFAULT_INSTRUMENT_MAX;

/**
 *  Manifest constant for the maximum value limit of a MIDI byte when used
 *  to limit the size of an array.  Here, it is the upper limit on the
 *  number of MIDI controllers that can be supported.
 */

#define SEQ64_MIDI_CONTROLLER_MAX       128

/**
 *  This structure corresponds to <code> [user-instrument-N] </code>
 *  definitions in the <code> ~/.seq24usr </code> or
 *  <code> ~/.config/sequencer64/sequencer64.usr </code> file.
 */

struct user_instrument_t
{
    /**
     *  Provides the name of the "instrument" being supported.  Do not confuse
     *  "instrument" with "program" here.   An "instrument" is most likely
     *  a hardware MIDI sound-box (though it could be a software synthesizer
     *  as well.
     */

    std::string instrument;

    /**
     *  Provides a list of up to 128 controllers (e.g. "Modulation").
     *  If a controller isn't present, or if General MIDI is in force,
     *  this name might be empty.
     */

    std::string controllers[SEQ64_MIDI_CONTROLLER_MAX];

    /**
     *  Provides a flag that indicates if each of up to 128 controller is
     *  active and supported.  If false, it might be an unsupported controller
     *  or a General MIDI device.
     */

    bool controllers_active[SEQ64_MIDI_CONTROLLER_MAX];
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
     *  Provides the actual number of non-default controllers actually
     *  set.  Often, the "user" configuration file has only a few out of
     *  the 128 assigned explicitly.
     */

    int m_controller_count;

    /**
     *  The instance of the structure that this class wraps.
     */

    user_instrument_t m_instrument_def;

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

    /**
     * \getter m_instrument_def.instrument (name of instrument)
     */

    const std::string & name () const
    {
        return m_instrument_def.instrument;
    }

    /**
     * \getter m_controller_count
     *      This function returns the number of active controllers.
     */

    int controller_count () const
    {
        return m_controller_count;
    }

    /**
     * \getter MIDI_CONTROLLER_MAX
     *      This function returns the maximum number of controllers,
     *      active or inactive.  Remember that the controller numbers for
     *      each MIDI instrument range from 0 to 127
     *      (MIDI_CONTROLLER_MAX-1).
     */

    int controller_max () const
    {
        return SEQ64_MIDI_CONTROLLER_MAX;
    }

    const std::string & controller_name (int c) const;  // getter
    bool controller_active (int c) const;               // getter
    void set_controller                                 // setter
    (
        int c, const std::string & cname, bool isactive
    );

private:

    void set_name (const std::string & instname);
    void copy_definitions (const user_instrument & rhs);

};

}           // namespace seq64

#endif      // SEQ64_USER_INSTRUMENT_HPP

/*
 * user_instrument.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

