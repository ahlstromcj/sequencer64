#ifndef SEQ64_GUI_PLAY_BASE_HPP
#define SEQ64_GUI_PLAY_BASE_HPP

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
 * \file          gui_play_base.hpp
 *
 *  This module declares/defines the base class for GUI callback functions
 *  used in some of the window-support modules.
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
 *  Many of those function can be implemented by offloading functionality to
 *  existing objects (such as perform), and so the utility of this base class
 *  is doubtful.
 *
 */

#include "click.hpp"
#include "keystroke.hpp"

namespace seq64
{

/**
 *  This class provides an interface for basic GUI support.  Much is to be
 *  determined at this point.
 */

class gui_play_base
{

private:

    // Any members useful?

public:

    gui_play_base ()
    {
        // Empty body
    }

    /**
     *  Stock base-class implementation of a virtual destructor.
     */

    virtual ~gui_play_base ()
    {
        // Empty body
    }

    virtual void quit () = 0;

protected:

    virtual bool do_button_event (const click & ev) = 0;
    virtual bool do_key_event (const keystroke & k) = 0;

    // Other possible events:
    //
    //  do_scroll_event()
    //  do_size_allocate()
    //  do_size_request()
    //  do_left_button_event()
    //  do_right_button_event()

    /**
     *  Do-nothing interface function that might not need to be overridden
     *  in many classes.
     */

    virtual bool do_realize_event ()
    {
        return true;
    }

    /**
     *  Do-nothing interface function that might not need to be overridden
     *  in many classes.
     */

    virtual bool do_expose_event ()
    {
        return true;
    }

    /**
     *  Do-nothing interface function that might not need to be overridden
     *  in many classes.
     */

    virtual bool do_focus_in_event ()
    {
        return true;
    }

    /**
     *  Do-nothing interface function that might not need to be overridden
     *  in many classes.
     */

    virtual bool do_focus_out_event ()
    {
        return true;
    }

    /**
     *  Do-nothing interface function that might not need to be overridden
     *  in many classes.
     */

    virtual bool do_motion_notify_event (click & /* ev */)
    {
        return true;
    }

    /**
     *  Do-nothing interface function that might not need to be overridden
     *  in many classes.
     */

    virtual bool do_delete_event ()
    {
        return true;
    }

};

}           // namespace seq64

#endif      // SEQ64_GUI_PLAY_BASE_HPP

/*
 * gui_play_base.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

