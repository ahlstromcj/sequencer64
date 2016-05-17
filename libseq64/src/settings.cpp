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
 * \file          settings.cpp
 *
 *  This module declares/defines just some of the global (gasp!) variables
 *  and functions in this application.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2016-05-17
 * \updates       2016-05-17
 * \license       GNU GPLv2 or above
 *
 *  The first part of this file defines a couple of global structure
 *  instances, followed by the global variables that these structures
 *  completely replace.
 */

#include "settings.hpp"

namespace seq64
{

/**
 *  Provides the replacement for all of the other "global_xxx" variables.
 */

static rc_settings g_rc_settings;

/**
 *  Returns a reference to the global rc_settings object.  Why a function
 *  instead of direct variable access?  Encapsulation.  We are then free to
 *  change the way "global" settings are accessed, without changing client
 *  code.
 *
 * \return
 *      Returns the global object g_rc_settings.
 */

rc_settings &
rc ()
{
    return g_rc_settings;
}

/**
 *  Provides the replacement for all of the other settings in the "user"
 *  configuration file, plus some of the "constants" in the globals module.
 */

static user_settings g_user_settings;

/**
 *  Returns a reference to the global user_settings object, for better
 *  encapsulation.
 *
 * \return
 *      Returns the global object g_user_settings.
 */

user_settings &
usr ()
{
    return g_user_settings;
}

/**
 *  Common code for handling PPQN settings.  Putting it here means we can
 *  reduce the reliance on the global ppqn.
 *
 * \param ppqn
 *      Provides the PPQN value to be used.
 *
 * \return
 *      Returns the ppqn parameter, unless that parameter is
 *      SEQ64_USE_DEFAULT_PPQN (-1), then usr().midi_ppqn is returned.
 */

int
choose_ppqn (int ppqn)
{
    return (ppqn == SEQ64_USE_DEFAULT_PPQN) ? usr().midi_ppqn() : ppqn ;
}

}           // namespace seq64

/*
 * settings.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

