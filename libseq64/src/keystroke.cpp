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
 *  of using a GUI representation of keystrokes.
 *
 * \library       sequencer64 application
 * \author        Chris Ahlstrom
 * \date          2015-09-30
 * \updates       2016-05-15
 * \license       GNU GPLv2 or above
 *
 */

#include <ctype.h>

#include "keystroke.hpp"            // seq64::keystroke

namespace seq64
{

/**
 *  The default constructor for class keystroke.
 */

keystroke::keystroke ()
 :
    m_is_press  (SEQ64_KEYSTROKE_RELEASE),          /* false */
    m_key       (SEQ64_KEYSTROKE_MIN),
    m_modifier  (SEQ64_NO_MASK)
{
    // Empty body
}

/**
 *  The principal constructor.
 *
 * \param key
 *      The keystroke number of the key that was pressed or released.
 *
 * \param press
 *      If true, the keystroke action was a press, otherwise it was a release.
 *
 * \param modkey
 *      The modifier key combination that was pressed, if any, in the form of
 *      a bit-mask, as defined in the gdk_basic_keys module.  Common mask
 *      values are SEQ64_SHIFT_MASK, SEQ64_CONTROL_MASK, SEQ64_MOD1_MASK, and
 *      SEQ64_MOD4_MASK.  If no modifier, this value is SEQ64_NO_MASK.
 */

keystroke::keystroke (unsigned int key, bool press, int modkey)
 :
    m_is_press  (press),
    m_key       (key),
    m_modifier  (seq_modifier_t(modkey))
{
    // Empty body
}

/**
 *  Provides the rote copy constructor.
 *
 * \param rhs
 *      The object to be copied.
 */

keystroke::keystroke (const keystroke & rhs)
 :
    m_is_press  (rhs.m_is_press),
    m_key       (rhs.m_key),
    m_modifier  (rhs.m_modifier)
{
    // Empty body
}

/**
 *  Provides the rote principal assignment operator.
 *
 * \param rhs
 *      The object to be assigned.
 *
 * \return
 *      Returns the reference to the current object, for use in assignment
 *      chains.
 */

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
 * \getter m_key to test letters, handles ASCII only.
 *
 * \param ch
 *      An optional character to test as an ASCII letter.
 *
 * \return
 *      If a character is not provided, true is returned if it is an upper
 *      or lower-case letter.  Otherwise, true is returned if the m_key
 *      value matches the character case-insensitively.
 *      \tricky
 */

bool
keystroke::is_letter (unsigned int ch) const
{
    if (ch == SEQ64_KEYSTROKE_BAD_VALUE)
        return bool(isalpha(m_key));
    else
        return tolower(m_key) == tolower(ch);
}

}           // namespace seq64

/*
 * keystroke.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

