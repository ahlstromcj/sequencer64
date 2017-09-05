#ifndef SEQ64_MASTERMIDIBUS_HPP
#define SEQ64_MASTERMIDIBUS_HPP

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
 * \file          mastermidibus.hpp
 *
 *  This module declares the right version of the mastermidibus header for the
 *  current API.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2016-11-28
 * \updates       2017-09-02
 * \license       GNU GPLv2 or above
 *
 */

#include "seq64-config.h"

#if defined SEQ64_ALSAMIDI_SUPPORT
#include "mastermidibus_am.hpp"         /* seq64::mastermidibus for ALSA    */
#elif defined SEQ64_RTMIDI_SUPPORT
#include "mastermidibus_rm.hpp"         /* seq64::mastermidibus for RtMidi  */
#elif defined SEQ64_PORTMIDI_SUPPORT
#include "mastermidibus_pm.hpp"         /* seq64::mastermidibus, PortMidi   */
#elif defined SEQ64_WINDOWS_SUPPORT
#include "mastermidibus_pm.hpp"         /* Windows uses PortMIDI now        */
#else
#include "mastermidibus_am.hpp"         /* seq64::mastermidibus for ALSA    */
#endif

#endif      // SEQ64_MASTERMIDIBUS_HPP

/*
 * mastermidibus.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

