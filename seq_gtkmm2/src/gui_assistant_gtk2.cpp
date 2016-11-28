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
 * \file          gui_assistant_gtk2.cpp
 *
 *  This module declares/defines the base class for handling many facets
 *  of using a GUI, without being tied to it.
 *
 * \library       sequencer64 application
 * \author        Chris Ahlstrom
 * \date          2015-09-19
 * \updates       2016-09-20
 * \license       GNU GPLv2 or above
 *
 */

#include <gtkmm/main.h>                 /* Glib::Main                   */
#include <sigc++/slot.h>

#include "gui_assistant_gtk2.hpp"       /* seq64::gui_assistant_gtk2    */
#include "jack_assistant.hpp"           /* seq64::jack_assistant        */
#include "lash.hpp"                     /* seq64::lash                  */

#if defined SEQ64_JACK_SESSION || defined SEQ64_LASH_SUPPORT
#include <glibmm.h>                     /* Glib::signal_idle() etc.     */
#endif

/**
 *  A manifest constant for the LASH time-out signal value.  This value is in
 *  milliseconds.
 */

#define SEQ64_LASH_TIMEOUT      250

/*
 * Provides the namespace for all Sequencer64 libraries.
 */

namespace seq64
{

/**
 *  Provides a pre-made keys_perform object.  This object is set into
 *  the reference provided in the gui_assistant base class.
 */

keys_perform_gtk2 gui_assistant_gtk2::sm_internal_keys;

/**
 *  This class provides an interface for some of the Gtk/Gdk/Glib support
 *  needed in Sequencer64.
 */

gui_assistant_gtk2::gui_assistant_gtk2 ()
 :
    gui_assistant   (sm_internal_keys)
{
    // No code yet
}

/**
 *  Calls the Glib Main object's quit() function.
 */

void
gui_assistant_gtk2::quit ()
{
    Gtk::Main::quit();
}

/**
 *  Connects the JACK session-event callback to the Glib idle object.
 *  If JACK session support is not enabled, we might emit a message.
 *  This mainly prevents a compiler warning about an unused parameter.
 */

#ifdef SEQ64_JACK_SESSION

void
gui_assistant_gtk2::jack_idle_connect (jack_assistant & jack)
{
    Glib::signal_idle().connect
    (
        sigc::mem_fun(jack, &jack_assistant::session_event)
    );
}

#endif  // SEQ64_JACK_SESSION

/**
 *  Connects the LASH timeout-event callback to the Glib timeout object.
 *  The time-out value is set to 250 ms.
 */

void
gui_assistant_gtk2::lash_timeout_connect (lash * lashobject)
{
    if (not_nullptr(lashobject))
    {
#ifdef SEQ64_LASH_SUPPORT
        Glib::signal_timeout().connect
        (
            sigc::mem_fun(lashobject, &lash::process_events),
            SEQ64_LASH_TIMEOUT
        );
#else
        infoprint("[No LASH support]");
#endif
    }
    else
    {
        infoprint("[No LASH support]");
    }
}

}           // namespace seq64

/*
 * gui_assistant_gtk2.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

