#ifndef SEQ64_GUI_ASSISTANT_GTK2_HPP
#define SEQ64_GUI_ASSISTANT_GTK2_HPP

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
 * \file          gui_assistant_gtk2.hpp
 *
 *  This module declares/defines the base class for handling many facets
 *  of using a GUI, without being tied to it.
 *
 * \library       sequencer64 application
 * \author        Chris Ahlstrom
 * \date          2015-09-19
 * \updates       2016-01-15
 * \license       GNU GPLv2 or above
 *
 *  Note that this module automatically creates a Gtk-specific
 *  keys_perform-derived object, which saves the main routine of the
 *  application the trouble of making one and passing it along.
 */

#include "gui_assistant.hpp"            // seq64::gui_assistant interface
#include "keys_perform_gtk2.hpp"        // seq64::keys_perform_gtk2

namespace seq64
{

class jack_assistant;                   // forward reference
class lash;                             // ditto
class keys_perform;                     // ditto

/**
 *  This class provides an interface for some of the Gtk/Gdk/Glib support
 *  needed in Sequencer64.
 */

class gui_assistant_gtk2 : public gui_assistant
{
private:

    static keys_perform_gtk2 sm_internal_keys;      // see the cpp file

public:

    gui_assistant_gtk2 ();
    virtual ~gui_assistant_gtk2 ()
    {
        // stock base-class implementation
    }

    virtual void quit ();

#ifdef SEQ64_JACK_SESSION
    virtual void jack_idle_connect (jack_assistant & jack);
#endif

    virtual void lash_timeout_connect (lash * lashobject);

};

}           // namespace seq64

#endif      // SEQ64_GUI_ASSISTANT_GTK2_HPP

/*
 * gui_assistant_gtk2.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

