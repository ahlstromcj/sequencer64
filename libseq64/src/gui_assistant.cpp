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
 * \file          gui_assistant.cpp
 *
 *  This module declares/defines the base class for handling many facets
 *  of using a GUI, without being tied to it.
 *
 * \library       sequencer64 application
 * \author        Chris Ahlstrom
 * \date          2015-09-19
 * \updates       2015-09-19
 * \license       GNU GPLv2 or above
 *
 */

#include "gui_assistant.hpp"            // seq64::gui_assistant

namespace seq64
{

/**
 *  This constructor wires in some externally (for now) created objects.
 */

gui_assistant::gui_assistant (keys_perform & kp)
 :
    m_keys_perform  (kp)
{
    // no other code
}

}           // namespace seq64

/*
 * gui_assistant.cpp
 *
 * vim: sw=4 ts=4 wm=8 et ft=cpp
 */

