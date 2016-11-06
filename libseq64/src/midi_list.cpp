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
 * \file          midi_list.cpp
 *
 *  This module declares/defines the concrete class for a container of MIDI
 *  data.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-10-10
 * \updates       2015-10-11
 * \license       GNU GPLv2 or above
 *
 */

#include "midi_list.hpp"                /* seq64::midi_container ABC    */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *    This constructor fills in the members.
 *
 * \param seq
 *      The sequence/track object that is using this container.
 */

midi_list::midi_list (sequence & seq)
 :
    midi_container  (seq),
    m_char_list     ()
{
    // Empty body
}

}           // namespace seq64

/*
 * midi_list.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
