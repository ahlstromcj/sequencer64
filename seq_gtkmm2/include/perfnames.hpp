#ifndef SEQ64_PERFNAMES_HPP
#define SEQ64_PERFNAMES_HPP

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
 * \file          perfnames.hpp
 *
 *  This module declares/defines the base class for performance names.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2015-09-10
 * \license       GNU GPLv2 or above
 *
 */

#include <gtkmm/drawingarea.h>

#include "globals.h"
#include "seqmenu.hpp"

namespace Gtk
{
    class Adjustment;
}

class perform;

/**
 *  This class implements the left-side keyboard in the patterns window.
 */

class perfnames : public virtual Gtk::DrawingArea, public virtual seqmenu
{
private:

    Glib::RefPtr<Gdk::GC> m_gc;
    Glib::RefPtr<Gdk::Window> m_window;
    Gdk::Color m_black;
    Gdk::Color m_white;
    Gdk::Color m_grey;
    Gdk::Color m_yellow;
    Glib::RefPtr<Gdk::Pixmap> m_pixmap;
    perform * m_mainperf;
    int m_window_x;
    int m_window_y;
    Gtk::Adjustment * m_vadjust;
    int m_sequence_offset;
    bool m_sequence_active[c_max_sequence];

public:

    perfnames (perform * a_perf, Gtk::Adjustment * a_vadjust);

    void redraw_dirty_sequences ();

private:

    void draw_area ();
    void update_pixmap ();
    void convert_y (int a_y, int * a_note);
    void draw_sequence (int a_sequence);
    void change_vert ();
    void redraw (int a_sequence);

private:    // Gtk callbacks

    void on_realize ();
    bool on_expose_event (GdkEventExpose * a_ev);
    bool on_button_press_event (GdkEventButton * a_ev);
    bool on_button_release_event (GdkEventButton * a_ev);
    void on_size_allocate (Gtk::Allocation &);
    bool on_scroll_event (GdkEventScroll * a_ev) ;

};

#endif   // SEQ64_PERFNAMES_HPP

/*
 * perfnames.hpp
 *
 * vim: sw=4 ts=4 wm=8 et ft=cpp
 */
