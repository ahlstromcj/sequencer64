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
 * \file          keys_perform_gtk2.cpp
 *
 *  This module defines Gtk-2 interface items.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-09-13
 * \updates       2018-03-24
 * \license       GNU GPLv2 or above
 *
 */

#include <gtkmm/accelkey.h>             // For keys
#include "keys_perform_gtk2.hpp"

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *  This construction initializes a vast number of member variables, some
 *  of them public!
 */

keys_perform_gtk2::keys_perform_gtk2 ()
 :
    keys_perform    ()
{
    set_all_key_events();
    set_all_key_groups();
}

/**
 *  A rote virtual destructor.  No action.
 */

keys_perform_gtk2::~keys_perform_gtk2 ()
{
    // what to do?
}

/**
 *  Sets up the keys for arming/unmuting events in the Gtk-2 environment.
 *  The base-class function call makes sure the the related lists are
 *  cleared before rebuilding them here.
 *
 *  The main keys are offloaded to the base class now.  Qt and Gdk keys
 *  overlap a lot.
 */

void
keys_perform_gtk2::set_all_key_events ()
{
    keys_perform::set_all_key_events();
    keys_perform::set_basic_key_events();
}

/**
 *  Sets up the keys for group events in the Gtk-2 environment.
 *  The base-class function call makes sure the the related lists are
 *  cleared before rebuilding them here.
 *
 *  The main keys are offloaded to the base class now.  Qt and Gdk keys
 *  overlap a lot.
 */

void
keys_perform_gtk2::set_all_key_groups ()
{
    keys_perform::set_all_key_groups();
    keys_perform::set_basic_key_groups();
}

/**
 *  Obtains the name of the key.  In gtkmm, this is done via the
 *  gdk_keyval_name() function.  Here, in the base class, we just provide an
 *  easy-to-create string.  Note that this is a free function, not a class
 *  member.
 *
 * \param key
 *      Provides the key-number to be converted to a key name.
 *
 * \return
 *      Returns the key name as looked up by the GDK infrastructure.  If the
 *      key is not found, then an empty string is returned.
 */

std::string
keyval_name (unsigned key)
{
    std::string result;
    gchar * kname = gdk_keyval_name(key);
    if (not_nullptr(kname))
        result = std::string((char *) kname);

    return result;
}

}           // namespace seq64

/*
 * keys_perform_gtk2.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
