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
 * \file          keybindentry.cpp
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

#include "keybindentry.hpp"
#include "perform.hpp"

namespace seq64
{

/**
 *  This constructor initializes the member with values dependent on the
 *  value type provided in the first parameter.
 */

keybindentry::keybindentry
(
    type t,
    unsigned int * location_to_write,
    perform * p,
    long s
) :
    Gtk::Entry  (),
    m_key       (location_to_write),
    m_type      (t),
    m_perf      (p),
    m_slot      (s)
{
    switch (m_type)
    {
    case location:
        if (m_key)
            set(*m_key);
        break;

    case events:
        set(m_perf->lookup_keyevent_key(m_slot));
        break;

    case groups:
        set(m_perf->lookup_keygroup_key(m_slot));
        break;
    }
}

/**
 *  Gets the key name from the integer value; if there is one, then it is
 *  printed into a temporary buffer, otherwise the value is printed into
 *  that buffer as is.  Then we call set_text(buf).  The set_width_char()
 *  function is then called.
 */

void
keybindentry::set (unsigned int val)
{
    char buf[256] = "";
    char * special = gdk_keyval_name(val);
    char * p_buf = &buf[strlen(buf)];           // why not just &buf[0]?
    if (special)
        snprintf(p_buf, sizeof buf - (p_buf-buf), "%s", special);
    else
        snprintf(p_buf, sizeof buf - (p_buf-buf), "'%c'", (char) val);

    set_text(buf);
    int width = strlen(buf) - 1;
    set_width_chars(1 <= width ? width : 1);
}

/**
 *  Handles a key press by calling set() with the event's key value.
 *  This value is used to set the event or key depending on the value of
 *  m_type.
 */

bool
keybindentry::on_key_press_event (GdkEventKey * event)
{
    bool result = Entry::on_key_press_event(event);
    set(event->keyval);
    switch (m_type)
    {
    case location:
        if (m_key)
            *m_key = event->keyval;
        break;

    case events:
        m_perf->set_key_event(event->keyval, m_slot);
        break;

    case groups:
        m_perf->set_key_group(event->keyval, m_slot);
        break;
    }
    return result;
}

}           // namespace seq64

/*
 * keybindentry.cpp
 *
 * vim: sw=4 ts=4 wm=8 et ft=cpp
 */
