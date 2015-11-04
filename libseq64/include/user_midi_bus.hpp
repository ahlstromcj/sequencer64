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
 * \updates       2015-11-04
 * \license       GNU GPLv2 or above
 *
 *  This class replaces an global_user_midi_bus_definitions[] array element
 *  with a wrapper class for better safety.
 */

#include <string>

#include "easy_macros.h"               // with platform_macros.h, too

/**
 *  Default value for c_max_busses.
 */

#define DEFAULT_BUSS_MAX                 32

namespace seq64
{

/**
 *  Provides the maximum number of MIDI buss definitions supported in the
 *  "user" file.
 */

const int c_max_busses = DEFAULT_BUSS_MAX;

/**
 *  Manifest constant for the maximum number of "instrument" values in a
 *  user_midi_bus_t structure.
 */

#define MIDI_BUS_CHANNEL_MAX             16

/**
 *  This structure corresponds to <tt> [user-midi-bus-0] </tt>
 *  definitions in the <tt> ~/.seq24usr </tt> ("user") file
 *  (<tt> ~/.config/sequencer64/sequencer64.usr </tt> in the latest version of
 *  the application).
 */

struct user_midi_bus_t
{
    /**
     *  Provides the user's desired name for the MIDI bus.  For example,
     *  "2x2 A" for some kind of MIDI card or USB MIDI cable.  If
     *  manual-alsa-ports is enabled, this could be something like
     *  "[0] seq24 0", and that is what should be shown in that case.
     */

    std::string alias;

    /**
     *  Provides an implicit list of MIDI channels from 0 to 15 (1 to 16) and
     *  the "instrument" number assigned to each channel.  Note that the
     *  "instrument" is not a MIDI program number.  Instead, it is the number
     *  associated with a "user-instrument" section in the "user"
     *  configuration file.
     */

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
     *  Provides a validity flag, useful in returning a reference to a
     *  bogus object for internal error-check.  Callers should check
     *  this flag via the is_valid() accessor before using this object.
     *  This flag is set to true when any valid member assignment occurs
     *  via a public setter call.
     */

    bool m_is_valid;

    /**
     *  Provides the actual number of non-default buss channels actually
     *  set.  Often, the "user" configuration file has only a few out of
     *  the 16 assigned explicitly.
     */

    int m_channel_count;

    /**
     *  The instance of the structure that this class wraps.
     */

    user_midi_bus_t m_midi_bus_def;

public:

    user_midi_bus (const std::string & name = "");
    user_midi_bus (const user_midi_bus & rhs);
    user_midi_bus & operator = (const user_midi_bus & rhs);

    /**
     * \getter m_is_valid
     */

    bool is_valid () const
    {
        return m_is_valid;
    }

    void set_defaults ();

    /**
     * \getter m_midi_bus_def.alias (name of alias)
     */

    const std::string & name () const
    {
        return m_midi_bus_def.alias;
    }

    /**
     * \getter m_channel_count
     * \return
     *      This function returns the number of channels.  Basically this
     *      value is always the same as that returned by channel_max(),
     *      but this pair of functions is consistent with the count
     *      functions in the user_instrument class.
     */

    int channel_count () const
    {
        return m_channel_count;
    }

    /**
     * \getter MIDI_BUS_CHANNEL_MAX
     * \return
     *      Returns the maximum number of MIDI buss channels.
     *      Remember that the instrument channels for each MIDI buss
     *      range from 0 to 15 (MIDI_BUS_CHANNEL_MAX-1).
     */

    int channel_max () const
    {
        return MIDI_BUS_CHANNEL_MAX;
    }

    int instrument (int channel) const;                     // getter
    void set_instrument (int channel, int instrum);         // setter

private:

    /**
     * \setter m_midi_bus_def.alias (name of alias)
     *      Also sets the validity flag according to the emptiness of the
     *      name parameter.
     */

    void set_name (const std::string & name)
    {
        m_midi_bus_def.alias = name;
        m_is_valid = ! name.empty();
    }

    void copy_definitions (const user_midi_bus & rhs);

};

}           // namespace seq64

#endif      // SEQ64_USER_MIDI_BUS_HPP

/*
 * user_midi_bus.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

