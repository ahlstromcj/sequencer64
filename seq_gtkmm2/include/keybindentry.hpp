#ifndef SEQ64_KEYBINDENTRY_HPP
#define SEQ64_KEYBINDENTRY_HPP

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
 * \file          keybindentry.hpp
 *
 *  This module declares/defines the base class for keybinding entries.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2015-09-13
 * \license       GNU GPLv2 or above
 *
 *  This module define a GTK text-edit widget for getting keyboard button
 *  values (for binding keys).  Put the cursor in the text-box, hit a key,
 *  and something like  'a' (42)  appears...  each keypress replaces the
 *  previous text.  It also supports key-event and key-group maps in the
 *  perform class.
 */

#include <gtkmm/entry.h>

#include "easy_macros.h"               // nullptr

namespace seq64
{

class perform;

/**
 *  Class for management of application key-bindings.
 */

class keybindentry : public Gtk::Entry
{

    friend class options;

private:

    /**
     * Provides the type of keybindings that can be made.
     *
     * \var location
     *      Used for handling a keystroke made while a keyboard-options
     *      field is active, for selecting a key via the keyboard, and
     *      binding to pattern/sequence boxes, we think.  It is used in
     *      the options class to associate a key with the binding.
     *
     * \var events
     *      Used for binding to events.
     *
     * \var groups
     *      Used for binding to groups.
     */

    enum type
    {
        location,
        events,
        groups
    };

private:

    /**
     *  Points to the value of the key that is part of this key-binding.
     *  Not yet sure by the address of this key value is needed.
     *  It can be a null pointer, as well.
     */

    unsigned int * m_key;

    /**
     *  Stores the type of key-binding.
     */

    type m_type;

    /**
     *  Stores an optional pointer to a perform object.
     */

    perform * m_perf;

    /**
     *  Provides???
     */

    long m_slot;

public:

    keybindentry
    (
        type t,
        unsigned int * location_to_write = nullptr,
        perform * p = nullptr,
        long s = 0
    );

    void set (unsigned int val);

    virtual bool on_key_press_event (GdkEventKey * event);
};

}           // namespace seq64

#endif      // SEQ64_KEYBINDENTRY_HPP

/*
 * keybindentry.hpp
 *
 * vim: sw=4 ts=4 wm=8 et ft=cpp
 */
