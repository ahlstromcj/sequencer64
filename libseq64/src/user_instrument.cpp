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
 * \file          user_instrument.cpp
 *
 *  This module declares/defines just some of the global (gasp!) variables
 *  in this application.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-09-25
 * \updates       2015-09-27
 * \license       GNU GPLv2 or above
 *
 *  Note that this module also sets the legacy global variables, so that
 *  they can be used by modules that have not yet been cleaned up.
 */

#include "globals.h"                    /* to support legacy variables */

/**
 *  Default constructor.
 */

user_instrument::user_instrument (const std::string & name)
 :
    m_is_valid          (false),
    m_controller_count  (0),
    m_instrument_def    ()
{
    set_defaults();
    set_name(name);
}

/**
 *  Copy constructor.
 */

user_instrument::user_instrument (const user_instrument & rhs)
 :
    m_is_valid          (rhs.m_is_valid),
    m_controller_count  (rhs.m_controller_count),
    m_instrument_def    ()
{
    copy_definitions(rhs);
}

/**
 *  Principal assignment operator.
 */

user_instrument &
user_instrument::operator = (const user_instrument & rhs)
{
    if (this != &rhs)
    {
        m_is_valid = rhs.m_is_valid;
        m_controller_count = rhs.m_controller_count;
        copy_definitions(rhs);
    }
    return *this;
}

/**
 *  Sets the default values.  Also invalidates the object.
 */

void
user_instrument::set_defaults ()
{
    m_is_valid = false;
    m_controller_count = 0;
    m_instrument_def.instrument.clear();
    for (int c = 0; c < MIDI_CONTROLLER_MAX; c++)
    {
        m_instrument_def.controllers_active[c] = false;
        m_instrument_def.controllers[c].clear();
    }
}

/**
 *  Copies the current values of the member variables into the
 *  selected legacy global variable.  Should be called at initialization,
 *  and after settings are read from the "user" configuration file.
 *
 *  This function fills in all of the MIDI_CONTROLLER_MAX (128) values of the
 *  controllers and controllers_active fields.
 *
 *  Note that this is done only if the object is valid.
 *
 * \param instrum
 *      Provides the destination instrument number.  In order to support
 *      the legacy code, this index value must be less than
 *      c_max_instruments (64).
 */

void
user_instrument::set_global (int instrum) const
{
    if (m_is_valid && instrum >= 0 && instrum < c_max_instruments) // CONST GLOBAL
    {
        global_user_instrument_definitions[instrum].instrument =
            m_instrument_def.instrument;

        for (int c = 0; c < MIDI_CONTROLLER_MAX; c++)
        {
            global_user_instrument_definitions[instrum].controllers_active[c] =
                m_instrument_def.controllers_active[c];

            global_user_instrument_definitions[instrum].controllers[c] =
                m_instrument_def.controllers[c];
        }
    }
}

/**
 *  Copies the current values of the selected legacy global variable into
 *  corresponding member variable.  Should be called before settings are
 *  written to the "user" configuration file.
 *
 *  This function fills in all of the MIDI_CONTROLLER_MAX (128) values of
 *  the controllers and controllers_active fields.
 *
 *  This function also sets the validity flag to true if the instrument
 *  name is not empty; the rest of the values are not checked.
 *
 * \param instrum
 *      Provides the source instrument number.  In order to support the
 *      legacy code, this index value must be less than c_max_instruments
 *      (64).
 */

void
user_instrument::get_global (int instrum)
{
    if (instrum >= 0 && instrum < c_max_instruments)  // CONST GLOBAL danger!
    {
        m_controller_count = 0;
        set_name(global_user_instrument_definitions[instrum].instrument);
        for (int c = 0; c < MIDI_CONTROLLER_MAX; c++)
        {
            bool active =
                global_user_instrument_definitions[instrum].controllers_active[c];

            m_instrument_def.controllers_active[c] = active;
            m_instrument_def.controllers[c] =
                global_user_instrument_definitions[instrum].controllers[c];

            if (active)
                ++m_controller_count;
        }
    }
}

/**
 * \setter m_instrument_def.instrument
 *
 *      If the name parameter is not empty, the validity flag is set to
 *      true, otherwise it is set to false.  Too tricky?
 */

void
user_instrument::set_name (const std::string & instname)
{
    m_instrument_def.instrument = instname;
    m_is_valid = ! instname.empty();
}

/**
 * \setter m_instrument_def.controllers[c] and .controllers_active[c]
 *      Only sets the controller values if the object is already valid.
 *
 * \param c
 *      The index of the desired controller.
 *
 * \param cname
 *      The name of the controller to be set as the controller name.
 *
 * \param isactive
 *      A flag that indicates if the desired controller is active.
 */

void
user_instrument::set_controller
(
    int c,
    const std::string & cname,
    bool isactive
)
{
    if (m_is_valid && c >= 0 && c < MIDI_CONTROLLER_MAX)
    {
        m_instrument_def.controllers[c] = cname;
        m_instrument_def.controllers_active[c] = isactive;
        if (isactive)
            ++m_controller_count;
        else
        {
            infoprint("Use this as a breakpoint");
        }
    }
}

/**
 * \getter m_instrument_def.controllers[c]
 *
 * \param c
 *      The index of the desired controller.
 *
 * \return
 *      The name of the desired controller has is returned.  If the
 *      index c is out of range, or the object is not valid, then a
 *      reference to an internal, empty string is returned.
 */

const std::string &
user_instrument::controller_name (int c) const
{
    static std::string s_empty;
    if (m_is_valid && c >= 0 && c < MIDI_CONTROLLER_MAX)
        return m_instrument_def.controllers[c];
    else
        return s_empty;
}

/**
 * \getter m_instrument_def.controllers_active[c]
 *
 * \param c
 *      The index of the desired controller.
 *
 * \return
 *      The status of the desired controller has is returned.  If the
 *      index c is out of range, or the object is not valid, then false is
 *      returned.
 */

bool
user_instrument::controller_active (int c) const
{
    if (m_is_valid && c >= 0 && c < MIDI_CONTROLLER_MAX)
        return m_instrument_def.controllers_active[c];
    else
        return false;
}

/**
 *  Copies the array members from one instance of user_instrument to this
 *  one.  Does not include the validity flag.
 */

void
user_instrument::copy_definitions (const user_instrument & rhs)
{
    m_instrument_def.instrument = rhs.m_instrument_def.instrument;
    for (int c = 0; c < MIDI_CONTROLLER_MAX; c++)
    {
        m_instrument_def.controllers_active[c] =
            rhs.m_instrument_def.controllers_active[c];

        m_instrument_def.controllers[c] = rhs.m_instrument_def.controllers[c];
    }
}

/*
 * user_instrument.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
