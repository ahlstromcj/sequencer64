#ifndef SEQ64_KEYSTROKE_HPP
#define SEQ64_KEYSTROKE_HPP

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
 * \file          keystroke.hpp
 *
 *  This module declares/defines the base class for handling many facets
 *  of using a GUI representation of keystrokes.
 *
 * \library       sequencer64 application
 * \author        Chris Ahlstrom
 * \date          2015-09-21
 * \updates       2017-08-19
 * \license       GNU GPLv2 or above
 *
 *  Most of the GUI modules are publicly derived from Gtk::DrawingArea,
 *  and some from Gtk::Window.  In gtkmm-3, the former will be merged into
 *  the latter, but for now Gtk::DrawingArea will be used.
 *
 */

#include "gdk_basic_keys.h"

/**
 *  Provides readability macros for true and false, to indicate if a
 *  keystroke is pressed, or released.
 */

#define SEQ64_KEYSTROKE_RELEASE         false
#define SEQ64_KEYSTROKE_PRESS           true

/**
 *  Range limits for the various integer parameters.  Used for sanity-checking
 *  and unit-testing.
 */

#define SEQ64_KEYSTROKE_BAD_VALUE       0x0000      /* null   */
#define SEQ64_KEYSTROKE_MIN             0x0001      /* Ctrl-A */
#define SEQ64_KEYSTROKE_MAX             0xffff

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *  Encapsulates any practical keystroke.  Useful in passing more generic
 *  events to non-GUI classes.
 */

class keystroke
{

private:

    /**
     *  Determines if the key was a press or a release.  See the
     *  SEQ64_KEYSTROKE_PRESS and SEQ64_KEYSTROKE_RELEASE readability macros.
     */

    bool m_is_press;                    /* versus a release of the key */

    /**
     *  The key that was pressed or released.  Generally, the extended ASCII
     *  range (0 to 255) is supported.  However, Gtk-2.x/3.x will generally
     *  support the full gamut of characters defined in the gdk_basic_keys.h
     *  module.  We define minimum and maximum range macros for keystrokes that
     *  are a bit generous.
     */

    unsigned m_key;

    /**
     *  The optional modifier value.  Note that SEQ64_NO_MASK is our word
     *  for 0, meaning "no modifier".
     */

    seq_modifier_t m_modifier;

public:

    keystroke ();
    keystroke
    (
        unsigned key,
        bool press = SEQ64_KEYSTROKE_PRESS,         /* true */
        int modkey = int(SEQ64_NO_MASK)
    );
    keystroke (const keystroke & rhs);
    keystroke & operator = (const keystroke & rhs);

    /**
     * \getter m_is_press
     */

    bool is_press () const
    {
        return m_is_press;
    }

    bool is_letter (unsigned ch = SEQ64_KEYSTROKE_BAD_VALUE) const;

    /**
     *  Tests the key value to see if it matches the given character exactly
     *  (no case-insensitivity).
     *
     * \param ch
     *      The character to be tested.
     *
     * \return
     *      Returns true if m_key == ch.
     */

    bool is (unsigned ch) const
    {
        return m_key == ch;
    }

    /**
     * \getter m_key to test for a delete-causing key.
     */

    bool is_delete () const
    {
        return m_key == SEQ64_Delete || m_key == SEQ64_BackSpace;
    }

    /**
     * \getter m_key
     */

    unsigned key () const
    {
        return m_key;
    }

    void shift_lock ();

    /**
     * \getter m_modifier
     */

    seq_modifier_t modifier () const
    {
        return m_modifier;
    }

    /**
     * \getter m_modifier tested for Ctrl key.
     */

    bool mod_control () const
    {
        return bool(m_modifier & SEQ64_CONTROL_MASK);   // GDK_CONTROL_MASK
    }

    /**
     * \getter m_modifier tested for Ctrl and Shift key.
     */

    bool mod_control_shift () const
    {
        return
        (
            bool(m_modifier & SEQ64_CONTROL_MASK) &&    // GDK_CONTROL_MASK
            bool(m_modifier & SEQ64_SHIFT_MASK)         // GDK_SHIFT_MASK
        );
    }

    /**
     * \getter m_modifier tested for Mod4/Super/Windows key.
     */

    bool mod_super () const
    {
        return bool(m_modifier & SEQ64_MOD4_MASK);      // GDK_MOD4_MASK
    }

};          // class keystroke

}           // namespace seq64

#endif      // SEQ64_KEYSTROKE_HPP

/*
 * keystroke.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

