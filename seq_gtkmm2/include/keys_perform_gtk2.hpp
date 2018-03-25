#ifndef SEQ64_KEYS_PERFORM_GTK2_HPP
#define SEQ64_KEYS_PERFORM_GTK2_HPP

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
 * \file          keys_perform_gtk2.hpp
 *
 *  This module declares/defines the Gtk-2 class for keystrokes that
 *  depend on the GUI framework.
 *
 * \library       sequencer64 application
 * \author        Chris Ahlstrom
 * \date          2015-09-13
 * \updates       2018-03-25
 * \license       GNU GPLv2 or above
 *
 *  This class has way too many members.
 */

#include "keys_perform.hpp"

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 * Free functions for keyboard support.  The implementation of this function
 * will ultimately depend on the GUI environment; currently it is GTK 2.x, so
 * the implementation is in seq_gtkmm2/src/keys_perform_gtk2.cpp.
 */

extern std::string keyval_name (unsigned key);

/**
 *  This class supports the performance mode.
 *
 *  It has way too many data members, many of the public.
 *  Might be ripe for refactoring.
 */

class keys_perform_gtk2 : public keys_perform
{

public:

    keys_perform_gtk2 ();
    virtual ~keys_perform_gtk2 ();

    /**
     *  Converts a key's value to a human-readable string.
     *
     * \param key
     *      The numerical value of the key.
     *
     * \return
     *      Returns the string version of the key.
     */

    virtual std::string key_name (unsigned key) const
    {
        return keyval_name(key);
    }

    virtual void set_all_key_events ();
    virtual void set_all_key_groups ();

};          // keys_perform_gtk2

}           // namespace seq64

#endif      // SEQ64_KEYS_PERFORM_GTK2_HPP

/*
 * keys_perform_gtk2.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

