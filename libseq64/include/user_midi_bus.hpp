#ifndef SEQ64_USER_MIDI_BUS_HPP
#define SEQ64_USER_MIDI_BUS_HPP

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
 * \file          user_midi_bus.hpp
 *
 *  This module declares/defines the user MIDI-buss section of the "user"
 *  configuration file.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-09-25
 * \updates       2015-09-25
 * \license       GNU GPLv2 or above
 *
 */

#include <string>

#include "easy_macros.h"               // with platform_macros.h, too

/**
 *  Default value for c_max_busses.
 */

#define DEFAULT_BUSS_MAX                 32

/**
 *  Provides the maximum number of MIDI buss definitions supported in the
 *  ~/.seq24usr file.
 */

const int c_max_busses = DEFAULT_BUSS_MAX;

/**
 *  Manifest constant for the maximum number of "instrument" values in a
 *  user_midi_bus_t structure.
 */

#define MIDI_BUS_CHANNEL_MAX             16

/**
 *  This structure corresponds to <tt> [user-midi-bus-0] </tt>
 *  definitions in the <tt> ~/.seq24usr </tt> ("user") file.
 */

struct user_midi_bus_t
{
    std::string alias;
    int instrument[MIDI_BUS_CHANNEL_MAX];
};

/**
 *  Provides data about the MIDI busses, readable from the "user"
 *  configuration file.  Will later make the size adjustable, if it
 *  makes sense to do so.
 *
 */

class user_midi_bus
{

    /**
     *  The instance of the structure that this class wraps.
     */

    user_midi_bus_t m_midi_bus_def;         // [c_max_busses];

public:

    user_midi_bus ();
    user_midi_bus (const user_midi_bus & rhs);
    user_midi_bus & operator = (const user_midi_bus & rhs);

    void set_defaults ();
    void set_global (int buss) const;
    void get_global (int buss);

    /**
     * \getter m_midi_bus_def.alias (name of alias)
     */

    const std::string & name () const
    {
        return m_midi_bus_def.alias;
    }

    int instrument (int channel) const;

    /**
     * \setter m_midi_bus_def.alias (name of alias)
     */

    void set_name (const std::string & name)
    {
        m_midi_bus_def.alias = name;
    }

    void set_instrument (int channel, int instrum);

private:

    void copy_definitions (const user_midi_bus & rhs);

};

#endif  // SEQ64_USER_MIDI_BUS_HPP

/*
 * user_midi_bus.hpp
 *
 * vim: sw=4 ts=4 wm=8 et ft=cpp
 */
