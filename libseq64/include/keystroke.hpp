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
 *  This module declares/defines the base class for GUI frameworks used in
 *  some of the window-support modules.
 *
 * \library       sequencer64 application
 * \author        Chris Ahlstrom
 * \date          2015-09-21
 * \updates       2015-09-30
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

#define KEYSTROKE_RELEASE       false
#define KEYSTROKE_PRESS         true

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
     *  KEYSTROKE_PRESS and KEYSTROKE_RELEASE readability macros.
     */

    bool m_is_press;                    /* versus a release of the key */

    /**
     *  The key that was pressed or released.  Left is 1, mmiddle is 2,
     *  and right is 3.
     */

    unsigned int m_key;

    /**
     *  The optional modifier value.  Note that SEQ64_NO_MASK is our word
     *  for 0, meaning "no modifier".
     */

    seq_modifier_t m_modifier;

public:

    keystroke ();
    keystroke
    (
        unsigned int key,
        bool press = true,
        seq_modifier_t modkey = SEQ64_NO_MASK
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

    /**
     * \getter m_key
     */

    unsigned int key () const
    {
        return m_key;
    }

    /**
     * \getter m_modifier
     */

    seq_modifier_t modifier () const
    {
        return m_modifier;
    }

};          // class keystroke

}           // namespace seq64

#endif      // SEQ64_KEYSTROKE_HPP

/*
 * keystroke.hpp
 *
 * vim: sw=4 ts=4 wm=8 et ft=cpp
 */

