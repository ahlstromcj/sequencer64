#ifndef SEQ24_PERFROLL_HPP
#define SEQ24_PERFROLL_HPP

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
 * \file          perfroll.hpp
 *
 *  This module declares/defines the base class for the Performance window
 *  piano roll.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2015-09-10
 * \license       GNU GPLv2 or above
 *
 */

#include <gtkmm/drawingarea.h>
#include <gtkmm/adjustment.h>

#include "globals.h"

class AbstractPerfInput;
class perform;

/**
 *  These should be private members.  Used by this module and the
 *  perfroll_input module.
 */

static const int c_perfroll_background_x = (c_ppqn * 4 * 16) / c_perf_scale_x;
static const int c_perfroll_size_box_w = 3;
static const int c_perfroll_size_box_click_w = c_perfroll_size_box_w + 1 ;

/**
 *  This class implements the performance roll user interface.
 */

class perfroll : public Gtk::DrawingArea
{

    friend class FruityPerfInput;
    friend class Seq24PerfInput;

private:

    Glib::RefPtr<Gdk::GC> m_gc;
    Glib::RefPtr<Gdk::Window> m_window;
    Gdk::Color m_black;
    Gdk::Color m_white;
    Gdk::Color m_grey;
    Gdk::Color m_lt_grey;
    Glib::RefPtr<Gdk::Pixmap> m_pixmap;
    Glib::RefPtr<Gdk::Pixmap> m_background;
    perform * m_mainperf;
    int m_window_x;
    int m_window_y;
    int m_drop_x;
    int m_drop_y;
    Gtk::Adjustment * m_vadjust;
    Gtk::Adjustment * m_hadjust;
    int m_snap;
    int m_measure_length;
    int m_beat_length;
    long m_old_progress_ticks;
    int m_4bar_offset;
    int m_sequence_offset;
    int m_roll_length_ticks;
    long m_drop_tick;
    long m_drop_tick_trigger_offset;
    int m_drop_sequence;
    bool m_sequence_active[c_max_sequence];
    AbstractPerfInput * m_interaction;
    bool m_moving;
    bool m_growing;
    bool m_grow_direction;

public:

    perfroll
    (
        perform * a_perf,
        Gtk::Adjustment * a_hadjust,
        Gtk::Adjustment * a_vadjust
    );
    ~perfroll();

    void set_guides (int a_snap, int a_measure, int a_beat);
    void update_sizes ();
    void init_before_show ();
    void fill_background_pixmap ();
    void increment_size ();
    void draw_progress ();
    void redraw_dirty_sequences ();

private:

    void convert_xy (int a_x, int a_y, long * a_ticks, int * a_seq);
    void convert_x (int a_x, long * a_ticks);
    void snap_x (int * a_x);
    void start_playing ();
    void stop_playing ();
    void draw_sequence_on (Glib::RefPtr<Gdk::Drawable> a_draw, int a_sequence);
    void draw_background_on (Glib::RefPtr<Gdk::Drawable> a_draw, int a_sequence);
    void draw_drawable_row
    (
        Glib::RefPtr<Gdk::Drawable> a_dest,
        Glib::RefPtr<Gdk::Drawable> a_src,
        long a_y
    );
    void change_horz ();
    void change_vert ();
    void split_trigger(int a_sequence, long a_tick);

private:        // callbacks

    void on_realize ();
    bool on_expose_event (GdkEventExpose * a_ev);
    bool on_button_press_event (GdkEventButton * a_ev);
    bool on_button_release_event (GdkEventButton * a_ev);
    bool on_motion_notify_event (GdkEventMotion * a_ev);
    bool on_scroll_event (GdkEventScroll * a_ev) ;
    bool on_focus_in_event (GdkEventFocus *);
    bool on_focus_out_event (GdkEventFocus *);
    void on_size_request (GtkRequisition *);
    void on_size_allocate (Gtk::Allocation &);
    bool on_key_press_event (GdkEventKey * a_p0);
};

#endif   // SEQ24_PERFROLL_HPP

/*
 * perfroll.hpp
 *
 * vim: sw=4 ts=4 wm=8 et ft=cpp
 */
