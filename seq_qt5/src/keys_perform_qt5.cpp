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
 * \file          keys_perform_qt5.cpp
 *
 *  This module defines Gtk-2 interface items.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2018-03-16
 * \updates       2018-03-25
 * \license       GNU GPLv2 or above
 *
 */

#include "keys_perform_qt5.hpp"
#include "qskeymaps.hpp"                /* mapping between Gtkmm and Qt     */

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *  This construction initializes a vast number of member variables, some
 *  of them public!
 */

keys_perform_qt5::keys_perform_qt5 ()
 :
    keys_perform    ()
{
    set_all_key_events();
    set_all_key_groups();
}

/**
 *  A rote virtual destructor.  No action.
 */

keys_perform_qt5::~keys_perform_qt5 ()
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
keys_perform_qt5::set_all_key_events ()
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
keys_perform_qt5::set_all_key_groups ()
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
 *  There is a new function in the qskeymaps module, called qt_key_name(),
 *  that can provide a better name.  However, it requires both the
 *  QKeyEvent::key() and QKeyEvent::text() values in order to do a workable
 *  lookup.  But we can use the gdk_map_to_qt() function to work around this
 *  limitation.
 *
 * \param key
 *      Provides the key-number to be converted to a key name.  Under the
 *      current regime, this value is a Gdk keystroke value, which is either
 *      in the ASCII character range or in the special character range of
 *      0xFF00 and above.
 *
 * \return
 *      Returns the key name as looked up by the GDK infrastructure.  If the
 *      key is not found, then an empty string is returned.
 */

std::string
keyval_name (unsigned key)
{
#if USE_OLD_CODE
    std::string result;
    char temp[16];
    (void) snprintf(temp, sizeof temp, "Key %ux", key);
        result = std::string(temp);

    return result;
#else
    unsigned qtkey = gdk_map_to_qt(key);
    unsigned qttext;
    if (qtkey >= 0x1000000)
    {
        qttext = 0;
    }
    else
    {
        qttext = key;
        qtkey = toupper(key);
    }
    return qt_key_name(qtkey, qttext);
#endif
}

}           // namespace seq64

/*
 * keys_perform_qt5.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
