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
 * \file          keystroke.cpp
 *
 *  This module declares/defines the base class for handling many facets
 *  of using a GUI.
 *
 * \library       sequencer64 application
 * \author        Chris Ahlstrom
 * \date          2015-09-30
 * \updates       2015-09-30
 * \license       GNU GPLv2 or above
 *
 */

#include "keystroke.hpp"            // seq64::keystroke

namespace seq64
{

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

keystroke::keystroke (unsigned int key, bool press, seq_modifier_t modkey)
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

}           // namespace seq64

/*
 * keystroke.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

