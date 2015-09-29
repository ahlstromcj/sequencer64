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
 * \file          gui_play_base.cpp
 *
 *  This module declares/defines the base class for handling many facets
 *  of using a GUI.
 *
 * \library       sequencer64 application
 * \author        Chris Ahlstrom
 * \date          2015-09-28
 * \updates       2015-09-29
 * \license       GNU GPLv2 or above
 *
 *  This module defines 3 classes:
 *
 *      -   seq64::click
 *      -   seq64::keystroke
 *      -   seq64::gui_play_base
 */

#include "gui_play_base.hpp"            // seq64::gui_play_base

namespace seq64
{

/**
 *  The constructor for class click.
 */

click::click ()
 :
    m_is_press  (false),
    m_x         (0),
    m_y         (0),
    m_button    (0),
    m_modifier  (SEQ64_NO_MASK)
{
    // Empty body
}

click::click (int x, int y, int button, bool press, seq_modifier_t modkey)
 :
    m_is_press  (press),
    m_x         (x),
    m_y         (y),
    m_button    (button),
    m_modifier  (modkey)
{
    // Empty body
}

click::click (const click & rhs)
 :
    m_is_press  (rhs.m_is_press),
    m_x         (rhs.m_x),
    m_y         (rhs.m_y),
    m_button    (rhs.m_button),
    m_modifier  (rhs.m_modifier)
{
    // Empty body
}

click &
click::operator = (const click & rhs)
{
    if (this != &rhs)
    {
        m_is_press  = rhs.m_is_press;
        m_x         = rhs.m_x;
        m_y         = rhs.m_y;
        m_button    = rhs.m_button;
        m_modifier  = rhs.m_modifier;
    }
    return *this;
}

/**
 *  The constructor for class key.
 */

keystroke::keystroke ()
 :
    m_is_press  (false),
    m_key       (0),
    m_modifier  (SEQ64_NO_MASK)
{
    // Empty body
}

keystroke::keystroke (int key, bool press, seq_modifier_t modkey)
 :
    m_is_press  (press),
    m_key       (key),
    m_modifier  (modkey)
{
    // Empty body
}

keystroke::keystroke (const keystroke & rhs)
 :
    m_is_press  (rhs.m_is_press),
    m_key       (rhs.m_key),
    m_modifier  (rhs.m_modifier)
{
    // Empty body
}

keystroke &
keystroke::operator = (const keystroke & rhs)
{
    if (this != &rhs)
    {
        m_is_press  = rhs.m_is_press;
        m_key       = rhs.m_key;
        m_modifier  = rhs.m_modifier;
    }
    return *this;
}

/**
 *  This constructor wires in some externally (for now) created objects.

gui_play_base::gui_play_base ()
 :
{
    // no other code
}
 */

}           // namespace seq64

/*
 * gui_play_base.cpp
 *
 * vim: sw=4 ts=4 wm=8 et ft=cpp
 */

