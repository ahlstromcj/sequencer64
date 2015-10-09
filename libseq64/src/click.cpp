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
 * \file          click.cpp
 *
 *  This module declares/defines the base class for handling many facets
 *  of using a GUI.
 *
 * \library       sequencer64 application
 * \author        Chris Ahlstrom
 * \date          2015-09-30
 * \updates       2015-10-09
 * \license       GNU GPLv2 or above
 *
 *  Provides the implementation of the seq64::click class.
 */

#include "click.hpp"                    // seq64::click
#include "gdk_basic_keys.h"             // seq64::SEQ64_MASK_MAX, etc.
#include "easy_macros.h"                // errprint() macro

namespace seq64
{

/**
 *  The constructor for class click.  Sets all members to false or zero.
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

/**
 *  Principal constructor for class click.  This function is the only way to
 *  set value for the click members (other than the copy constructor and
 *  principal assignment operator.
 */

click::click (int x, int y, int button, bool press, seq_modifier_t modkey)
 :
    m_is_press  (press),
    m_x         (x),
    m_y         (y),
    m_button    (button),
    m_modifier  (modkey)
{
    if (x < CLICK_X_MIN || x >= CLICK_X_MAX)
        m_x = CLICK_BAD_VALUE;

    if (y < CLICK_Y_MIN || y >= CLICK_Y_MAX)
        m_y = CLICK_BAD_VALUE;

    if (button < CLICK_BUTTON_MIN || button > CLICK_BUTTON_MAX)
        m_button = CLICK_BAD_VALUE;

    unsigned int um = (unsigned int)(modkey);
    if (um < (unsigned int)(SEQ64_NO_MASK) || um > (unsigned int)(SEQ64_MASK_MAX))
        m_modifier = SEQ64_MASK_MAX;
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

}           // namespace seq64

/*
 * click.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

