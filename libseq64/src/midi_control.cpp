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
 * \file          midi_control.cpp
 *
 *  This module declares/defines just some of the global (gasp!) variables
 *  and functions for the extended MIDI control feature.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2017-03-14
 * \updates       2017-03-14
 * \license       GNU GPLv2 or above
 *
 */

#include "midi_control.hpp"

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{

int g_midi_control_limit = c_midi_controls_extended;    /* c_midi_controls */

}           // namespace seq64

/*
 * midi_control.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

