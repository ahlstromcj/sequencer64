#ifndef SEQ64_GUI_KEY_TESTS_HPP
#define SEQ64_GUI_KEY_TESTS_HPP

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
 * \file          gui_key_tests.hpp
 *
 *  This module declares/defines free functions for Gtk state-testing
 *  operations.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2016-07-15
 * \updates       2017-10-15
 * \license       GNU GPLv2 or above
 *
 *  This module applies only to GtkMM.
 */

#include <gdk/gdkevents.h>
#include <gtk/gtkwidget.h>

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{
    extern bool is_ctrl_key (GdkEventKey * ev);
    extern bool is_shift_key (GdkEventKey * ev);
    extern bool is_no_modifier (GdkEventScroll * ev);
    extern bool is_ctrl_key (GdkEventScroll * ev);
    extern bool is_shift_key (GdkEventScroll * ev);
    extern bool is_ctrl_key (GdkEventButton * ev);
    extern bool is_shift_key (GdkEventButton * ev);
    extern bool is_ctrl_shift_key (GdkEventButton * ev);
    extern bool is_super_key (GdkEventButton * ev);
    extern void test_widget_click (GtkWidget * w);
    extern bool is_left_drag (GdkEventMotion * ev);
    extern bool is_drag_motion (GdkEventMotion * ev);
}

#endif          // SEQ64_GUI_KEY_TESTS_HPP

/*
 * gui_key_tests.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

