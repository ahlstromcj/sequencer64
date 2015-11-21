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
 * \file          globals.cpp
 *
 *  This module declares/defines just some of the global (gasp!) variables
 *  and functions in this application.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-08-07
 * \updates       2015-11-21
 * \license       GNU GPLv2 or above
 *
 *  The first part of this file defines a couple of global structure
 *  instances, followed by the global variables that these structures will
 *  eventually completely replace.
 *
 *  The second part of this file defines some functions needed by other
 *  modules, such as MIDI timing calculations.  These items are in the seq64
 *  namespace.
 *
 * \note
 *      The MIDI time signature is specified by "FF 58 nn dd cc bb" where:
 *
 *      -   <i> nn </i> is the numerator, and counts the number of beats in a
 *          measure (bar).
 *      -   <i> dd </i> is the denominator, and specifies the unit of the beat
 *          (e.g. 4 or 8), and is specified as 2 to the <i> dd </i> power.
 *      -   <i> cc </i> is the MIDI ticks per metronome click.  The standard
 *          MIDI clock ticks 24 times per quarter note, so a value of 6 would
 *          mean the metronome clicks every 1/8th note.
 *      -   <i> bb </i> is the number of 32nd notes per MIDI quarter note.
 *          For example, a value of 16 means that the music plays two quarter
 *          notes for each quarter note metered out by the MIDI clock, so that
 *          the music plays at double speed.
 */

#include "app_limits.h"
#include "globals.h"

namespace seq64
{

/**
 *  Provide the eventual replacement for all of the other "global_xxx"
 *  variables.
 */

static rc_settings g_rc_settings;

/**
 *  Returns a reference to the global rc_settings object.  Why a function
 *  instead of direct variable access?  Encapsulation.  We are then free to
 *  change the way "global" settings are accessed, without changing client
 *  code.
 */

rc_settings &
rc ()
{
    return g_rc_settings;
}

/**
 *  Provide the eventual replacement for all of the other settings in the
 *  "user" configuration file, plus some of the "constants" in the globals
 *  module.
 */

static user_settings g_user_settings;

/**
 *  Returns a reference to the global user_settings object, for better
 *  encapsulation.
 */

user_settings &
usr ()
{
    return g_user_settings;
}

}           // namespace seq64

/*
 * globals.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

