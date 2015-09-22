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
 * \updates       2015-09-22
 * \license       GNU GPLv2 or above
 *
 */

#include <gtkmm/adjustment.h>
#include <gtkmm/window.h>
// #include <gtkmm/widget.h>
#include <gtkmm/scrollbar.h>

// #include <sigc++/bind.h>
// #include "gtk_helpers.h"

#include "gui_window_gtk2.hpp"
#include "perform.hpp"

// using namespace Gtk::Menu_Helpers;

namespace seq64
{

/**
 *  Principal constructor, has a pointer to a perform object.
 *  We've reordered the pointer members and put them in the initializer
 *  list to make the constructor a bit cleaner.
 *
 * \param a_perf
 *      Refers to the main performance object.
 *
 * \todo
 *      Offload most of the work into an initialization function like
 *      options does; make the perform parameter a reference.
 */

gui_window_gtk2::gui_window_gtk2 (perform & a_perf)
 :
    Gtk::Window         (),
    m_vadjust           (manage(new Gtk::Adjustment(0, 0, 1, 1, 1, 1))),
    m_hadjust           (manage(new Gtk::Adjustment(0, 0, 1, 1, 1, 1))),
    m_vscroll           (manage(new Gtk::VScrollbar(*m_vadjust))),
    m_hscroll           (manage(new Gtk::HScrollbar(*m_hadjust))),
    m_mainperf          (a_perf),
    m_modified          (false)
{
#if 0
    set_icon(Gdk::Pixbuf::create_from_xpm_data(perfedit_xpm));
    set_title("Sequencer24 - Song Editor");                   /* main window */
    set_size_request(700, 400);
    m_table->set_border_width(2);
    m_hlbox->set_border_width(2);
    m_button_grow->add
    (
        *manage(new Gtk::Arrow(Gtk::ARROW_RIGHT, Gtk::SHADOW_OUT))
    );
    m_button_grow->signal_clicked().connect(mem_fun(*this, &perfedit::grow));
    add_tooltip(m_button_grow, "Increase size of Grid.");
#endif

    add_events(Gdk::KEY_PRESS_MASK | Gdk::KEY_RELEASE_MASK);
}

/**
 *  This rote constructor does nothing.  We're going to have to run the
 *  application through valgrind to make sure that nothing is left behind.
 */

gui_window_gtk2::~gui_window_gtk2 ()
{
    // Empty body
}

}           // namespace seq64

/*
 * gui_window_gtk2.cpp
 *
 * vim: sw=4 ts=4 wm=8 et ft=cpp
 */
