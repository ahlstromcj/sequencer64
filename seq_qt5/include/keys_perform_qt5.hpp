#ifndef SEQ64_KEYS_PERFORM_QT5_HPP
#define SEQ64_KEYS_PERFORM_QT5_HPP

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
 * \file          keys_perform_qt5.hpp
 *
 *  This module declares/defines the Gtk-2 class for keystrokes that
 *  depend on the GUI framework.
 *
 * \library       sequencer64 application
 * \author        Chris Ahlstrom
 * \date          2018-01-01
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

/*
 *  Free functions.
 */

extern std::string keyval_name (unsigned key);      /* Qt 5 version     */

/**
 *  This class supports the performance mode.
 *
 *  It has way too many data members, many of the public.
 *  Might be ripe for refactoring.
 */

class keys_perform_qt5 : public keys_perform
{

public:

    keys_perform_qt5 ();
    virtual ~keys_perform_qt5 ();

    /**
     * \getter keyval_name(), Qt 5 version
     */

    virtual std::string key_name (unsigned key) const
    {
        return keyval_name(key);
    }

    virtual void set_all_key_events ();
    virtual void set_all_key_groups ();

};          // class keys_perform_qt5

}           // namespace seq64

#endif      // SEQ64_KEYS_PERFORM_QT5_HPP

/*
 * keys_perform_qt5.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

