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
 *  This module declares/defines the base class for key-binding entries.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2018-03-25
 * \license       GNU GPLv2 or above
 *
 *  This module defines a GTK text-edit widget for getting keyboard button
 *  values (for binding keys).  Put the cursor in the text-box, hit a key,
 *  and something like  'a' (42)  appears...  each keypress replaces the
 *  previous text.  It also supports key-event and key-group maps in the
 *  perform class.
 */

#include "keybindentry.hpp"
#include "keys_perform_gtk2.hpp"
#include "perform.hpp"

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *  This constructor initializes the member with values dependent on the
 *  value type provided in the first parameter.
 *
 * \usage
 *      In options, a pointer to a new key-binding entry is managed by
 *      calling <tt>keybindentry(keybindentry::location, &perf->keyname)</tt>.
 *
 * \param t
 *      Provides the type of key-binding:  location, events, or groups.
 *
 * \param location_to_write
 *      The location that holds the value of the key associated with
 *      the key-binding.
 *
 * \param p
 *      Points to the performance object used with this key-binding.  The
 *      default value of this parameter is the null pointer, but it is needed
 *      for the pattern hot-keys frame and the mute-group frame of the Options
 *      dialog.
 *
 * \param s
 *      Provides the slot value for this key-binding.  The default value
 *      of this parameter is zero, but it is needed to provide numeric labels
 *      for the hot-keys and mute-group frames of the Options dialog.
 */

keybindentry::keybindentry
(
    type t,
    unsigned * location_to_write,
    perform * p,
    int s
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
        if (not_nullptr(m_perf))
            set(m_perf->lookup_keyevent_key(m_slot));
        break;

    case groups:
        if (not_nullptr(m_perf))
            set(m_perf->lookup_keygroup_key(m_slot));
        break;
    }
}

/**
 *  Gets the key name from the integer value; if there is one, then it is
 *  printed into a temporary buffer, otherwise the value is printed into
 *  that buffer as is.
 *
 *  Then we call set_text(buf).  The set_width_char() function is then
 *  called.
 */

void
keybindentry::set (unsigned val)
{
    char buf[64] = "";
    std::string special = keyval_name(val);
    if (special.empty())
        snprintf(buf, sizeof(buf), "'%c'", char(val));
    else
        snprintf(buf, sizeof(buf), "%s", special.c_str());

    set_text(buf);
    int width = strlen(buf) - 1;
    set_width_chars(width >= 1 ? width : 1);
}

/**
 *  Handles a key press by calling set() with the event's key value.
 *  This value is used to set the event or key depending on the value of
 *  m_type.
 *
 * \param event
 *      Provides the key-press event.
 *
 * \return
 *      Returns the result of the call to Entry::on_key_press_event().
 */

bool
keybindentry::on_key_press_event (GdkEventKey * event)
{
    bool result = Entry::on_key_press_event(event);
    set(event->keyval);
    switch (m_type)
    {
    case location:          /* copy the pressed key into this binding   */

        if (not_nullptr(m_key))
        {
            *m_key = event->keyval;
        }
        else
        {
            warnprint("keybindentry(): null key pointer");
        }
        break;

    case events:            /* set the event key in the perform object  */

        if (not_nullptr(m_perf))
        {
            m_perf->set_key_event(event->keyval, m_slot);
        }
        else
        {
            warnprint("keybindentry(): null perform pointer for events");
        }
        break;

    case groups:            /* set the group key in the perform object  */

        if (not_nullptr(m_perf))
        {
            m_perf->set_key_group(event->keyval, m_slot);
        }
        else
        {
            warnprint("keybindentry(): null perform pointer for groups");
        }
        break;
    }
    return result;
}

}           // namespace seq64

/*
 * keybindentry.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

