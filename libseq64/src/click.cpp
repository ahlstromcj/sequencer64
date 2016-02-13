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
 *  of using a GUI representation of mouse clicks.
 *
 * \library       sequencer64 application
 * \author        Chris Ahlstrom
 * \date          2015-09-30
 * \updates       2015-10-10
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
 *  The constructor for class click.  Sets all members to false, zero, or the
 *  lowest good value.
 */

click::click ()
 :
    m_is_press  (SEQ64_CLICK_RELEASE),          /* false */
    m_x         (SEQ64_CLICK_X_MIN),
    m_y         (SEQ64_CLICK_Y_MIN),
    m_button    (SEQ64_CLICK_BUTTON_LEFT),
    m_modifier  (SEQ64_NO_MASK)
{
    // Empty body
}

/**
 *  Principal constructor for class click.  This function is the only way to
 *  set value for the click members (other than the copy constructor and
 *  principal assignment operator.
 *
 * \param x
 *      The putative x value of the button click.
 *
 * \param y
 *      The putative y value of the button click.
 *
 * \param button
 *      The value of the button that was clicked, set to 1, 2, or 3.
 *
 * \param press
 *      Set to true if the event was a button press, false if it was a button
 *      release.
 *
 * \param modkey
 *      Indicates which modifier key (such as Ctrl or Alt), if any, was
 *      pressed at the same time as the click action.
 */

click::click (int x, int y, int button, bool press, seq_modifier_t modkey)
 :
    m_is_press  (press),
    m_x         (x),
    m_y         (y),
    m_button    (button),
    m_modifier  (modkey)
{
    if (x < SEQ64_CLICK_X_MIN || x >= SEQ64_CLICK_X_MAX)
        m_x = SEQ64_CLICK_BAD_VALUE;

    if (y < SEQ64_CLICK_Y_MIN || y >= SEQ64_CLICK_Y_MAX)
        m_y = SEQ64_CLICK_BAD_VALUE;

    if (button < SEQ64_CLICK_BUTTON_MIN || button > SEQ64_CLICK_BUTTON_MAX)
        m_button = SEQ64_CLICK_BAD_VALUE;

    unsigned int um = static_cast<unsigned int>(modkey);
    if (um > static_cast<unsigned int>(SEQ64_MASK_MAX))
        m_modifier = SEQ64_MASK_MAX;
}

/**
 *  Provides a stock copy constructor.  It is nice to be explicit about these
 *  kinds of functions, even if it gets tedious.
 *
 * \param rhs
 *      Provies the source object to be copied.
 */

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

/**
 *  Provides a stock principal assignment operator.  It is nice to be explicit
 *  about these kinds of functions, even if it gets tedious.
 *
 * \param rhs
 *      Provies the source object to be assigned from.  The assignment is not
 *      made if "this" has the same address as this parameter.
 *
 * \return
 *      Returns a reference to self for usage in a string of assignments.
 */

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

