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
 * \file          gui_window_gtk2.cpp
 *
 *  This module declares/defines the base class for Gtk::Window-derived
 *  objects.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-09-22
 * \updates       2015-09-29
 * \license       GNU GPLv2 or above
 *
 */

#include <gtkmm/adjustment.h>
#include <gtkmm/window.h>
#include <gtkmm/scrollbar.h>

#include "gui_window_gtk2.hpp"
#include "perform.hpp"

namespace seq64
{

/**
 *  Principal constructor, has a reference to the all-important  perform object.
 *
 * \param a_perf
 *      Refers to the main performance object.
 */

gui_window_gtk2::gui_window_gtk2 (perform & a_perf)
 :
    Gtk::Window         (),
    m_mainperf          (a_perf),
    m_modified          (false)
{
    add_events(Gdk::KEY_PRESS_MASK | Gdk::KEY_RELEASE_MASK);
}

/**
 *  This rote constructor does nothing.
 */

gui_window_gtk2::~gui_window_gtk2 ()
{
    // Empty body
}

}           // namespace seq64

/*
 * gui_window_gtk2.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
