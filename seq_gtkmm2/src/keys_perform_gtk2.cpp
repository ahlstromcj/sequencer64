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
 *  This module defines Gtk-2 interface items, VERY TENTATIVE
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-09-13
 * \updates       2015-10-10
 * \license       GNU GPLv2 or above
 *
 */

#include <gtkmm/accelkey.h>             // For keys
#include "keys_perform_gtk2.hpp"

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
 *  The destructor sets some running flags to false, signals this
 *  condition, then joins the input and output threads if the were
 *  launched. Finally, any active patterns/sequences are deleted.
 */

keys_perform_gtk2::~keys_perform_gtk2 ()
{
    // what to do?
}

/**
 *  Obtains the name of the key.  In gtkmm, this is done via the
 *  gdk_keyval_name() function.  Here, in the base class, we just provide an
 *  easy-to-create string.
 */

std::string
keys_perform_gtk2::key_name (unsigned int key) const
{
    return std::string(gdk_keyval_name(key));
}

/**
 *  Sets up the keys for arming/unmuting events in the Gtk-2 environment.
 *  The base-class function call makes sure the the related lists are
 *  cleared before rebuilding them here.
 */

void
keys_perform_gtk2::set_all_key_events ()
{
    keys_perform::set_all_key_events();
    set_key_event(GDK_KEY_1, 0);
    set_key_event(GDK_KEY_q, 1);
    set_key_event(GDK_KEY_a, 2);
    set_key_event(GDK_KEY_z, 3);
    set_key_event(GDK_KEY_2, 4);
    set_key_event(GDK_KEY_w, 5);
    set_key_event(GDK_KEY_s, 6);
    set_key_event(GDK_KEY_x, 7);
    set_key_event(GDK_KEY_3, 8);
    set_key_event(GDK_KEY_e, 9);
    set_key_event(GDK_KEY_d, 10);
    set_key_event(GDK_KEY_c, 11);
    set_key_event(GDK_KEY_4, 12);
    set_key_event(GDK_KEY_r, 13);
    set_key_event(GDK_KEY_f, 14);
    set_key_event(GDK_KEY_v, 15);
    set_key_event(GDK_KEY_5, 16);
    set_key_event(GDK_KEY_t, 17);
    set_key_event(GDK_KEY_g, 18);
    set_key_event(GDK_KEY_b, 19);
    set_key_event(GDK_KEY_6, 20);
    set_key_event(GDK_KEY_y, 21);
    set_key_event(GDK_KEY_h, 22);
    set_key_event(GDK_KEY_n, 23);
    set_key_event(GDK_KEY_7, 24);
    set_key_event(GDK_KEY_u, 25);
    set_key_event(GDK_KEY_j, 26);
    set_key_event(GDK_KEY_m, 27);
    set_key_event(GDK_KEY_8, 28);
    set_key_event(GDK_KEY_i, 29);
    set_key_event(GDK_KEY_k, 30);
    set_key_event(GDK_KEY_comma, 31);
}

/**
 *  Sets up the keys for group events in the Gtk-2 environment.
 *  The base-class function call makes sure the the related lists are
 *  cleared before rebuilding them here.
 */

void
keys_perform_gtk2::set_all_key_groups ()
{
    keys_perform::set_all_key_groups();
    set_key_group(GDK_KEY_exclam, 0);
    set_key_group(GDK_KEY_quotedbl, 1);
    set_key_group(GDK_KEY_numbersign, 2);
    set_key_group(GDK_KEY_dollar, 3);
    set_key_group(GDK_KEY_percent, 4);
    set_key_group(GDK_KEY_ampersand, 5);
    set_key_group(GDK_KEY_parenleft, 7);
    set_key_group(GDK_KEY_slash, 6);
    set_key_group(GDK_KEY_semicolon, 31);
    set_key_group(GDK_KEY_A, 16);
    set_key_group(GDK_KEY_B, 28);
    set_key_group(GDK_KEY_C, 26);
    set_key_group(GDK_KEY_D, 18);
    set_key_group(GDK_KEY_E, 10);
    set_key_group(GDK_KEY_F, 19);
    set_key_group(GDK_KEY_G, 20);
    set_key_group(GDK_KEY_H, 21);
    set_key_group(GDK_KEY_I, 15);
    set_key_group(GDK_KEY_J, 22);
    set_key_group(GDK_KEY_K, 23);
    set_key_group(GDK_KEY_M, 30);
    set_key_group(GDK_KEY_N, 29);
    set_key_group(GDK_KEY_Q, 8);
    set_key_group(GDK_KEY_R, 11);
    set_key_group(GDK_KEY_S, 17);
    set_key_group(GDK_KEY_T, 12);
    set_key_group(GDK_KEY_U, 14);
    set_key_group(GDK_KEY_V, 27);
    set_key_group(GDK_KEY_W, 9);
    set_key_group(GDK_KEY_X, 25);
    set_key_group(GDK_KEY_Y, 13);
    set_key_group(GDK_KEY_Z, 24);
}

}           // namespace seq64

/*
 * keys_perform_gtk2.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
