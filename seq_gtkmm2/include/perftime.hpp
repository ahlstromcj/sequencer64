#ifndef SEQ64_PERFTIME_HPP
#define SEQ64_PERFTIME_HPP

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
 * \file          perftime.hpp
 *
 *  The time bar shows markers and numbers for the measures of the song,
 *  and also depicts the left and right markers.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2015-09-10
 * \license       GNU GPLv2 or above
 *
 */

#include <gtkmm/drawingarea.h>
#include <gtkmm/window.h>

namespace Gtk
{
    class Adjustment;
}

class perform;

/**
 *  This class implements drawing the piano time at the top of the
 *  "performance window", also known as the "song editor".
 */

class perftime: public Gtk::DrawingArea
{

private:

    Glib::RefPtr<Gdk::GC> m_gc;
    Glib::RefPtr<Gdk::Window> m_window;
    Gdk::Color m_black;
    Gdk::Color m_white;
    Gdk::Color m_grey;
    Glib::RefPtr<Gdk::Pixmap> m_pixmap;
    perform * const m_mainperf;
    int m_window_x;
    int m_window_y;
    Gtk::Adjustment * const m_hadjust;
    int m_4bar_offset;
    int m_snap;
    int m_measure_length;

public:

    perftime (perform * a_perf, Gtk::Adjustment * a_hadjust);

    void reset ();
    void set_scale (int a_scale);
    void set_guides (int a_snap, int a_measure);
    void increment_size ();

private:

    void update_sizes ();
    void draw_pixmap_on_window ();
    void draw_progress_on_window ();
    void update_pixmap ();
    int idle_progress ();
    void change_horz ();

private:        // callbacks

    void on_realize ();
    bool on_expose_event (GdkEventExpose * a_ev);
    bool on_button_press_event (GdkEventButton * a_ev);
    bool on_button_release_event (GdkEventButton * a_ev);
    void on_size_allocate (Gtk::Allocation & a_r);
};

#endif   // SEQ64_PERFTIME_HPP

/*
 * perftime.hpp
 *
 * vim: sw=4 ts=4 wm=8 et ft=cpp
 */
