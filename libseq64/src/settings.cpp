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
 * \updates       2018-11-05
 * \license       GNU GPLv2 or above
 *
 *  The first part of this file defines a couple of global structure
 *  instances, followed by the global variables that these structures
 *  completely replace.
 */

#include "settings.hpp"

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

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
 *  reduce the reliance on the global PPQN, and have a lot more flexibility in
 *  changing the PPQN.
 *
 *  However, this function works completely only if the "user" configuration
 *  file has already been read.  In some cases we may need to retrofit the
 *  desired PPQN value!  Also, the default constructor for
 *  seqmenu::sm_clipboard is called before we read either the configuration
 *  files or the MIDI file, so we don't report issues when that happens.
 *
 * \param ppqn
 *      Provides the PPQN value to be used.  The legal values are:
 *
 *      -   SEQ64_USE_FILE_PPQN (0).  Return 0.  The caller should then get
 *          the PPQN from the usr().file_ppqn() or usr().midi_ppqn() values,
 *          perhaps by calling this function again with no parameters. See the
 *          next option below.
 *      -   SEQ64_USE_DEFAULT_PPQN (-1).  This is the default value of this
 *          parameter.  Behavior:
 *          -   Return the value of usr().midi_ppqn(), if in the range
 *              SEQ64_MINIMUM_PPQN to SEQ64_MAXIMUM_PPQN.  The default
 *              PPQN value can be changed (from 192 to another value) on the
 *              command-line (options "--ppqn" or "-q"), or in the "usr"
 *              configuration file at the "midi_ppqn" setting.
 *          -   If usr().midi_ppqn() is SEQ64_USE_FILE_PPQN (0), then return
 *              usr().file_ppqn() instead.  If that is invalid, return
 *              SEQ64_DEFAULT_PPQN (192).
 *          -   If usr().midi_ppqn() is invalid, return SEQ64_DEFAULT_PPQN.
 *      -   Legal PPQN.  Return it unchanged.
 *      -   Illegal PPQN.  Return SEQ64_DEFAULT_PPQN.
 *
 * \return
 *      Returns the ppqn parameter, unless that parameter is one of the
 *      special values above, or is illegal, as noted above.
 */

int
choose_ppqn (int ppqn)
{
    int result = ppqn;
    if (ppqn == SEQ64_USE_FILE_PPQN)
    {
        result = SEQ64_USE_FILE_PPQN;
    }
    else if (ppqn == SEQ64_USE_DEFAULT_PPQN)
    {
        if (usr().midi_ppqn() == SEQ64_USE_FILE_PPQN)
            result = usr().file_ppqn();
        else
            result = usr().midi_ppqn();

        if (result < SEQ64_MINIMUM_PPQN || result > SEQ64_MAXIMUM_PPQN)
        {
            if (result > SEQ64_USE_FILE_PPQN)
            {
                warnprint("Default & file PPQN out of range, setting PPQN = 192");
            }
            result = SEQ64_DEFAULT_PPQN;
        }
    }
    else if (result < SEQ64_MINIMUM_PPQN || result > SEQ64_MAXIMUM_PPQN)
    {
        warnprint("Provided PPQN out of range, setting PPQN = 192");
        result = SEQ64_DEFAULT_PPQN;
    }
    return result;
}

}           // namespace seq64

/*
 * settings.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

