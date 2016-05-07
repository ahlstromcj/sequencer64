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
 *  This module declares/defines the base class for handling many facets
 *  of using a GUI representation of mouse clicks.
 *
 * \library       sequencer64 application
 * \author        Chris Ahlstrom
 * \date          2015-09-30
 * \updates       2016-05-05
 * \license       GNU GPLv2 or above
 *
 *  Most of the GUI modules are publicly derived from Gtk::DrawingArea,
 *  and some from Gtk::Window.  In gtkmm-3, the former will be merged into
 *  the latter, but for now Gtk::DrawingArea will be used.
 *
 *  Doxygen weirdness:  Adding a Doxygen comment in front of the namespace
 *  declaration causes the creation of the PDF file to fail!
 */

#include "gdk_basic_keys.h"

/**
 *  Provides readability macros for true and false, to indicate if a
 *  mouse button is pressed (true), or released (false).
 */

#define SEQ64_CLICK_RELEASE             false
#define SEQ64_CLICK_PRESS               true

/**
 *  Range limits for the various integer parameters of a button-click.  Used
 *  for sanity-checking and unit-testing.
 */

#define SEQ64_CLICK_X_MIN                   0
#define SEQ64_CLICK_X_MAX                1920       // just one pixel too high

#define SEQ64_CLICK_Y_MIN                   0
#define SEQ64_CLICK_Y_MAX                1080       // just one pixel too high

/**
 *  Defines the integer values associated with the left mouse button (1), the
 *  middle mouse button (2), and the right mouse button (3).
 */

#define SEQ64_CLICK_BUTTON_MIN              1
#define SEQ64_CLICK_BUTTON_LEFT             1
#define SEQ64_CLICK_BUTTON_MIDDLE           2
#define SEQ64_CLICK_BUTTON_RIGHT            3
#define SEQ64_CLICK_BUTTON_MAX              3

/**
 *  Provides a "bad" value (-1) for values related to clicks.
 */

#define SEQ64_CLICK_BAD_VALUE             (-1)

/**
 *  Readability macros for testing (GDK) button clicks.  Meant for legacy
 *  code; use the corresponding click mod_xxx() member functions for new code.
 *  However, keep these macros, as they are used in the member functions now.
 *  Note that the "b" parameter is the ev->button field.
 */

#define SEQ64_CLICK_LEFT(b)             ((b) == SEQ64_CLICK_BUTTON_LEFT)
#define SEQ64_CLICK_MIDDLE(b)           ((b) == SEQ64_CLICK_BUTTON_MIDDLE)
#define SEQ64_CLICK_RIGHT(b)            ((b) == SEQ64_CLICK_BUTTON_RIGHT)

/**
 *  Combination-test macros.  Note the "b" parameter is the ev->button field,
 *  and the "s" parameter is the ev->state field.
 *
 *  SEQ64_CLICK_LEFT_MIDDLE() is used where either the left or middle button
 *  is acceptable.
 *
 *  SEQ64_CLICK_LEFT_RIGHT() is used where either the left or middle button
 *  is acceptable.
 *
 *  SEQ64_CLICK_CTRL_LEFT_MIDDLE() is used where the middle button or
 *  Ctrl-left button can be used.
 */

#define SEQ64_CLICK_LEFT_MIDDLE(b) \
 (SEQ64_CLICK_LEFT(b) || SEQ64_CLICK_MIDDLE(b))

#define SEQ64_CLICK_LEFT_RIGHT(b) \
 (SEQ64_CLICK_LEFT(b) || SEQ64_CLICK_RIGHT(b))

#define SEQ64_CLICK_CTRL_LEFT_MIDDLE(b, s) \
 (SEQ64_CLICK_MIDDLE(b) || (SEQ64_CLICK_LEFT(b) && ((s) & SEQ64_CONTROL_MASK)) )

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
     *  Determines if the click was a press or a release event.
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
     *  and right is 3.  These numbers are defined via macros, and are
     *  Linux-specific and Gtk-specific.
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
        int button = SEQ64_CLICK_BUTTON_LEFT,
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
     * \getter m_button to test for the left button.
     */

    bool is_left () const
    {
        return SEQ64_CLICK_LEFT(m_button);
    }

    /**
     * \getter m_button to test for the middle button.
     */

    bool is_middle () const
    {
        return SEQ64_CLICK_MIDDLE(m_button);
    }

    /**
     * \getter m_button to test for the right button.
     */

    bool is_right () const
    {
        return SEQ64_CLICK_RIGHT(m_button);
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

