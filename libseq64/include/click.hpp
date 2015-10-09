#ifndef SEQ64_CLICK_HPP
#define SEQ64_CLICK_HPP

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
 * \file          click.hpp
 *
 *  This module declares/defines the base class for GUI frameworks used in
 *  some of the window-support modules.
 *
 * \library       sequencer64 application
 * \author        Chris Ahlstrom
 * \date          2015-09-30
 * \updates       2015-10-09
 * \license       GNU GPLv2 or above
 *
 *  Most of the GUI modules are publicly derived from Gtk::DrawingArea,
 *  and some from Gtk::Window.  In gtkmm-3, the former will be merged into
 *  the latter, but for now Gtk::DrawingArea will be used.
 *
 */

#include "gdk_basic_keys.h"

/**
 *  Range limits for the various integer parameters.  Used for sanity-checking
 *  and unit-testing.
 */

#define CLICK_X_MIN                     0
#define CLICK_X_MAX                  1920       // just one pixel too high

#define CLICK_Y_MIN                     0
#define CLICK_Y_MAX                  1080       // just one pixel too high

#define CLICK_BUTTON_MIN                1
#define CLICK_BUTTON_LEFT               1
#define CLICK_BUTTON_MIDDLE             2
#define CLICK_BUTTON_RIGHT              3
#define CLICK_BUTTON_MAX                3

#define CLICK_BAD_VALUE               (-1)

/**
 *  Readability macros for testing (GDK) button clicks.  Meant for legacy
 *  code; use the corresponding click mod_xxx() member functions for new code.
 *  However, keep these macros, as they are used in the member functions now.
 */

#define CLICK_IS_LEFT(x)            ((x) == 1)
#define CLICK_IS_MIDDLE(x)          ((x) == 2)
#define CLICK_IS_RIGHT(x)           ((x) == 3)

namespace seq64
{

/**
 *  Encapsulates any possible mouse click.  Useful in passing more generic
 *  events to non-GUI classes.
 */

class click
{

private:

    /**
     *  Determines if the click was a press or a release.
     */

    bool m_is_press;                    /* versus a release of the button */

    /**
     *  The x-coordinate of the click.  0 is the left-most coordinate.
     */

    int m_x;

    /**
     *  The y-coordinate of the click.  0 is the top-most coordinate.
     */

    int m_y;

    /**
     *  The button that was pressed or released.  Left is 1, mmiddle is 2,
     *  and right is 3.
     */

    int m_button;

    /**
     *  The optional modifier value.  Note that SEQ64_NO_MASK is our word
     *  for 0, meaning "no modifier".
     */

    seq_modifier_t m_modifier;

public:

    click ();
    click
    (
        int x,
        int y,
        int button = CLICK_BUTTON_LEFT,
        bool press = true,
        seq_modifier_t modkey = SEQ64_NO_MASK
    );
    click (const click & rhs);
    click & operator = (const click & rhs);

    /**
     * \getter m_is_press
     */

    bool is_press () const
    {
        return m_is_press;
    }

    /**
     * \getter m_button to test for left, right, and middle buttons.
     */

    bool is_left () const
    {
        return CLICK_IS_LEFT(m_button);
    }

    bool is_middle () const
    {
        return CLICK_IS_MIDDLE(m_button);
    }

    bool is_right () const
    {
        return CLICK_IS_RIGHT(m_button);
    }

    /**
     * \getter m_x
     */

    int x () const
    {
        return m_x;
    }

    /**
     * \getter m_y
     */

    int y () const
    {
        return m_y;
    }

    /**
     * \getter m_button
     */

    int button () const
    {
        return m_button;
    }

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

};          // class click

}           // namespace seq64

#endif      // SEQ64_CLICK_HPP

/*
 * click.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
