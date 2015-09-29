#ifndef SEQ64_MAINWID_HPP
#define SEQ64_MAINWID_HPP

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
 * \file          mainwid.hpp
 *
 *  This module declares/defines the base class for drawing
 *  patterns/sequences in the Patterns Panel grid.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2015-09-29
 * \license       GNU GPLv2 or above
 *
 */

#include <gtkmm/button.h>
#include <gtkmm/drawingarea.h>
#include <gtkmm/window.h>

#include "seqmenu.hpp"

namespace seq64
{

class perform;

/**
 *    This class implement the piano roll area of the application.
 */

class mainwid : public Gtk::DrawingArea, public seqmenu
{

private:

    static const char m_seq_to_char[c_seqs_in_set];

    Glib::RefPtr<Gdk::GC> m_gc;
    Glib::RefPtr<Gdk::Window> m_window;
    Gdk::Color m_black;
    Gdk::Color m_white;
    Gdk::Color m_grey;
    Gdk::Color m_yellow;
    Gdk::Color m_background;
    Gdk::Color m_foreground;
    Glib::RefPtr<Gdk::Pixmap> m_pixmap;
    perform & m_mainperf;
    int m_window_x;
    int m_window_y;

    /**
     *  The x and y value of the current location of the mouse (during
     *  dragging?)
     */

    int m_current_x;
    int m_current_y;

    /**
     *  These values are used when roping and highlighting a bunch of events.
     *
     *  The x and y value of where the dragging started.
     */

    int m_drop_x;
    int m_drop_y;

    sequence m_moving_seq;
    bool m_button_down;
    bool m_moving;

    int m_old_seq;
    int m_screenset;

    long m_last_tick_x[c_max_sequence];
    bool m_last_playing[c_max_sequence];

public:

    mainwid (perform & a_p);
    ~mainwid ();

    void reset ();
    void set_screenset (int a_ss);      // undefined: int get_screenset ();
    void update_sequence_on_window (int a_seq);
    void update_sequences_on_window ();
    void update_markers (int a_ticks);
    void draw_marker_on_sequence (int a_seq, int a_tick);

private:

    bool valid_sequence (int a_seq);
    void draw_sequence_on_pixmap (int a_seq);
    void draw_sequences_on_pixmap ();
    void fill_background_window ();
    void draw_pixmap_on_window ();
    void draw_sequence_pixmap_on_window (int a_seq);
    int seq_from_xy (int a_x, int a_y);
    int timeout ();
    void redraw (int a_seq);

private:        // callbacks

    void on_realize ();
    bool on_expose_event (GdkEventExpose * a_ev);
    bool on_button_press_event (GdkEventButton * a_ev);
    bool on_button_release_event (GdkEventButton * a_ev);
    bool on_motion_notify_event (GdkEventMotion * a_p0);
    bool on_focus_in_event (GdkEventFocus *);
    bool on_focus_out_event (GdkEventFocus *);

};

}           // namespace seq64

#endif      // SEQ64_MAINWID_HPP

/*
 * mainwid.hpp
 *
 * vim: sw=4 ts=4 wm=8 et ft=cpp
 */
