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
 * \updates       2017-08-19
 * \license       GNU GPLv2 or above
 *
 *  This class makes access to keystroke features simpler.
 */

#include <ctype.h>

#include "keystroke.hpp"            // seq64::keystroke

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

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

keystroke::keystroke (unsigned key, bool press, int modkey)
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
keystroke::is_letter (unsigned ch) const
{
    if (ch == SEQ64_KEYSTROKE_BAD_VALUE)
        return bool(isalpha(m_key));
    else
        return tolower(m_key) == tolower(ch);
}

/**
 *  Holds a pair of characters.  These don't yet apply to A-Z, for speed.
 */

struct charpair_t
{
    unsigned m_character;       /**< The input character.               */
    unsigned m_shift;           /**< The shift of the input character.  */
};

/**
 *  The array of mappings of the non-alphabetic characters.
 */

struct charpair_t s_character_mapping [] =
{
    {   '0',    ')'     },      // no mapping
    {   '1',    '!'     },
    {   '2',    '@'     },      // "
    {   '3',    '#'     },
    {   '4',    '$'     },
    {   '5',    '%'     },
    {   '6',    '&'     },
    {   '7',    '^'     },      // Super-L
    {   '8',    '*'     },      // (
    {   '9',    '('     },      // no mapping
    {   '-',    '_'     },
    {   '=',    '+'     },
    {   '[',    '{'     },
    {   ']',    '}'     },
    {   ';',    ':'     },
    {   '\'',    '"'    },
    {   ',',    '<'     },
    {   '.',    '>'     },
    {   '/',    '?'     },
    {     0,      0     },
};

/**
 *  If a lower-case letter, a number, or another character on the "main" part
 *  of the keyboard, shift the m_key value to upper-case or the character
 *  shifted on a standard American keyboard.  Currently also assumes the ASCII
 *  character set.
 *
 *  There's an oddity here:  the shift of '2' is the '@' character, but seq24
 *  seems to have treated it like the '"' character. Some others were treated
 *  the same:
 *
\verbatim
    Key:        1 2 3 4 5 6 7 8 9 0
    Shift:      ! @ # $ % ^ & * ( )
    Seq24:      ! " # $ % & ' ( ) space
\endverbatim
 *
 *  This function is meant to avoid using the Caps-Lock when picking a
 *  group-learn character in the group-learn mode.
 */

void
keystroke::shift_lock ()
{
    if (m_key >= 'a' && m_key <= 'z')
        m_key -= 32;
    else
    {
        charpair_t * cp_ptr = &s_character_mapping[0];
        while (cp_ptr->m_character != 0)
        {
            if (cp_ptr->m_character == m_key)
            {
                m_key = cp_ptr->m_shift;
                break;
            }
            ++cp_ptr;
        }
    }
}

}           // namespace seq64

/*
 * keystroke.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

