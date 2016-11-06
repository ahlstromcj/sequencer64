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
 * \file          gui_key_tests.cpp
 *
 *  This module declares/defines free functions for Gtk state-testing
 *  operations.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2016-07-15
 * \updates       2016-07-16
 * \license       GNU GPLv2 or above
 *
 *  A little encapsulation never hurt anyone.  Too bad that the GdkEventAny
 *  struct doen't support the state field.
 */

#include <gdk/gdk.h>
#include <gtk/gtk.h>

#include "gdk_basic_keys.h"
#include "gui_key_tests.hpp"

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *  Encapsulates the safe test for the control key, as described here:
 *  https://developer.gnome.org/gtk3/stable/checklist-modifiers.html.
 *  It's a shame that GdkEventAny doesn't also encapsulate the keyboard
 *  state, since that is also available for other events, such as scroll
 *  events.
 *
 * \param ev
 *      The keystroke event to be tested.
 */

bool
is_ctrl_key (GdkEventKey * ev)
{
    guint modifiers = gtk_accelerator_get_default_mod_mask();
    return (ev->state & modifiers) == SEQ64_CONTROL_MASK;
}

/**
 *  Encapsulates the safe test for the shift key.
 *
 * \param ev
 *      The keystroke event to be tested.
 */

bool
is_shift_key (GdkEventKey * ev)
{
    guint modifiers = gtk_accelerator_get_default_mod_mask();
    return (ev->state & modifiers) == SEQ64_SHIFT_MASK;
}

/**
 *  Encapsulates the safe test for no modifier keys, for a scroll event.
 *
 * \param ev
 *      The scroll event to be tested.
 */

bool
is_no_modifier (GdkEventScroll * ev)
{
    guint modifiers = gtk_accelerator_get_default_mod_mask();
    return (ev->state & modifiers) == 0;
}

/**
 *  Encapsulates the safe test for the control key for scrolling.
 *
 * \param ev
 *      The keystroke event to be tested.
 */

bool
is_ctrl_key (GdkEventScroll * ev)
{
    guint modifiers = gtk_accelerator_get_default_mod_mask();
    return (ev->state & modifiers) == SEQ64_CONTROL_MASK;
}

/**
 *  Encapsulates the safe test for the shift key.
 *
 * \param ev
 *      The keystroke event to be tested.
 */

bool
is_shift_key (GdkEventScroll * ev)
{
    guint modifiers = gtk_accelerator_get_default_mod_mask();
    return (ev->state & modifiers) == SEQ64_SHIFT_MASK;
}

/**
 *  Encapsulates the safe test for the control key for buttons.
 *
 * \param ev
 *      The keystroke event to be tested.
 */

bool
is_ctrl_key (GdkEventButton * ev)
{
    guint modifiers = gtk_accelerator_get_default_mod_mask();
    return (ev->state & modifiers) == SEQ64_CONTROL_MASK;
}

/**
 *  Encapsulates the safe test for the shift key.
 *
 * \param ev
 *      The keystroke event to be tested.
 */

bool
is_shift_key (GdkEventButton * ev)
{
    guint modifiers = gtk_accelerator_get_default_mod_mask();
    return (ev->state & modifiers) == SEQ64_SHIFT_MASK;
}

/**
 *  Encapsulates the safe test for the ctrl-shift key combination.
 *
 * \param ev
 *      The keystroke event to be tested.
 */

bool
is_ctrl_shift_key (GdkEventButton * ev)
{
    guint modifiers = gtk_accelerator_get_default_mod_mask();
    return (ev->state & modifiers) == (SEQ64_SHIFT_MASK & SEQ64_CONTROL_MASK);
}

/**
 *  Encapsulates the test for the super (mod4, windows) key for buttons.
 *  Basically just masks off the MOD4 bit; the "safe" method does not work for
 *  this key.
 *
 * \param ev
 *      The keystroke event to be tested.
 */

bool
is_super_key (GdkEventButton * ev)
{
    return (ev->state & SEQ64_MOD4_MASK) != 0;
}

}           // namespace seq64

/*
 * gui_key_tests.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

