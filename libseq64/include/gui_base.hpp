#ifndef SEQ64_GUI_BASE_HPP
#define SEQ64_GUI_BASE_HPP

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
 * \file          gui_base.hpp
 *
 *  This module declares/defines the base class for GUI frameworks used in
 *  some of the window-support modules.
 *
 * \library       sequencer64 application
 * \author        Chris Ahlstrom
 * \date          2015-09-21
 * \updates       2015-09-28
 * \license       GNU GPLv2 or above
 *
 *  Most of the GUI modules are publicly derived from Gtk::DrawingArea,
 *  and some from Gtk::Window.  In gtkmm-3, the former will be merged into
 *  the latter, but for now Gtk::DrawingArea will be used.
 */

#include "gdk_basic_keys.h"

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
     *  The optional modifier value.  Note that GDK_NO_MASK is our word
     *  for 0, meaning "no modifier".
     */

    gdk_modifier_t m_modifier;

public:

    click ();
    click
    (
        int x, int y, int button, bool press = true,
        gdk_modifier_t modkey = GDK_NO_MASK
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

    gdk_modifier_t modifier () const
    {
        return m_modifier;
    }

};          // class click

/**
 *  Encapsulates any practical key.  Useful in passing more generic
 *  events to non-GUI classes.
 */

class key
{

private:

    /**
     *  Determines if the key was a press or a release.
     */

    bool m_is_press;                    /* versus a release of the key */

    /**
     *  The x-coordinate of the key.  0 is the left-most coordinate.
     *  Not sure if this is useful yet.
     */

    int m_x;

    /**
     *  The y-coordinate of the key.  0 is the top-most coordinate.
     *  Not sure if this is useful yet.
     */

    int m_y;

    /**
     *  The key that was pressed or released.  Left is 1, mmiddle is 2,
     *  and right is 3.
     */

    int m_key;

    /**
     *  The optional modifier value.  Note that GDK_NO_MASK is our word
     *  for 0, meaning "no modifier".
     */

    gdk_modifier_t m_modifier;

public:

    key ();
    key
    (
        int x, int y, int key, bool press = true,
        gdk_modifier_t modkey = GDK_NO_MASK
    );
    key (const key & rhs);
    key & operator = (const key & rhs);

    /**
     * \getter m_is_press
     */

    bool is_press () const
    {
        return m_is_press;
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
     * \getter m_key
     */

    int key () const
    {
        return m_key;
    }

    /**
     * \getter m_modifier
     */

    gdk_modifier_t modifier () const
    {
        return m_modifier;
    }

};          // class key

/**
 *  This class provides an interface for basic GUI support.  Much is to be
 *  determined at this point.
 */

class gui_base
{

private:

    // Any members useful?

public:

    gui_base ()
    {
    }
    virtual ~gui_base ()
    {
        // stock base-class implementation
    }

    virtual void quit () = 0;

};

}           // namespace seq64

#endif      // SEQ64_GUI_BASE_HPP

/*
 * gui_base.hpp
 *
 * vim: sw=4 ts=4 wm=8 et ft=cpp
 */

