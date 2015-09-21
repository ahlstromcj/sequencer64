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
 * \updates       2015-09-21
 * \license       GNU GPLv2 or above
 *
 *  Most of the GUI modules are publicly derived from Gtk::DrawingArea,
 *  and some from Gtk::Window.  In gtkmm-3, the former will be merged into
 *  the latter, but for now Gtk::DrawingArea will be used.
 */

namespace seq64
{

class gui_assistant;                    // forward reference

/**
 *  This class provides an interface for basic GUI support.  Much is to be
 *  determined at this point.
 */

class gui_base
{

private:

    /**
     *  We will probably need this one.
     */

    gui_assistant & m_gui_asst;

public:

    gui_base ();
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

