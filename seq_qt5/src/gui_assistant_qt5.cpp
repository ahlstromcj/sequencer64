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
 * \file          gui_assistant_qt5.cpp
 *
 *  This module declares/defines the base class for handling many facets
 *  of using a GUI, without being tied to it.
 *
 * \library       sequencer64 application
 * \author        Chris Ahlstrom
 * \date          2018-03-16
 * \updates       2018-03-17
 * \license       GNU GPLv2 or above
 *
 *  The Qt 5 version does not (yet) support the jack_assistant and lash
 *  classes, nor does it support the quit notification.
 */

#include "gui_assistant_qt5.hpp"       /* seq64::gui_assistant_qt5      */

/*
 * Provides the namespace for all Sequencer64 libraries.
 */

namespace seq64
{

/**
 *  Provides a pre-made keys_perform object.  This object is set into
 *  the reference provided in the gui_assistant base class.
 */

keys_perform_qt5 gui_assistant_qt5::sm_internal_keys;

/**
 *  This class provides an interface for some of the Gtk/Gdk/Glib support
 *  needed in Sequencer64.
 */

gui_assistant_qt5::gui_assistant_qt5 ()
 :
    gui_assistant   (sm_internal_keys)
{
    // No code yet
}

}           // namespace seq64

/*
 * gui_assistant_qt5.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

