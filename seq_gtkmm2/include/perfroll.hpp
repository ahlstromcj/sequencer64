#ifndef SEQ64_PERFROLL_HPP
#define SEQ64_PERFROLL_HPP

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
 * \updates       2015-10-27
 * \license       GNU GPLv2 or above
 *
 */

#include "globals.h"
#include "gui_drawingarea_gtk2.hpp"

namespace Gtk
{
    class Adjustment;
}

namespace seq64
{

class AbstractPerfInput;
class perform;

/**
 *  These should be private members.  Used by this module and the
 *  perfroll_input module.  We need to be able to adjust
 *  c_perfroll_background_x per the selected PPQN value.  This adjustment is
 *  made in the constructor, and assigned to the perfroll::m_background_x
 *  member.
 */

static const int c_perfroll_background_x = (c_ppqn * 4 * 16) / c_perf_scale_x;
static const int c_perfroll_size_box_w = 3;
static const int c_perfroll_size_box_click_w = c_perfroll_size_box_w + 1 ;

/**
 *  This class implements the performance roll user interface.
 */

class perfroll : public gui_drawingarea_gtk2
{

    friend class FruityPerfInput;
    friend class Seq24PerfInput;

private:

    int m_snap;
    int m_ppqn;
    int m_page_factor;              // 4096, provisional name
    int m_divs_per_beat;            // 16, provisional name
    int m_ticks_per_bar;            // m_ppqn * m_divs_per_beat, provisional name
    int m_perf_scale_x;
    int m_names_y;
    int m_background_x;
    int m_size_box_w;
    int m_size_box_click_w;
    int m_measure_length;
    int m_beat_length;
    long m_old_progress_ticks;
    int m_4bar_offset;
    int m_sequence_offset;
    int m_roll_length_ticks;
    long m_drop_tick;
    long m_drop_tick_trigger_offset;
    int m_drop_sequence;
    int m_sequence_max;
    bool m_sequence_active[c_max_sequence];
    AbstractPerfInput * m_interaction;
    bool m_moving;
    bool m_growing;
    bool m_grow_direction;

public:

    perfroll
    (
        perform & a_perf,
        Gtk::Adjustment & a_hadjust,
        Gtk::Adjustment & a_vadjust,
        int ppqn = SEQ64_USE_DEFAULT_PPQN
    );
    ~perfroll();

    void set_guides (int a_snap, int a_measure, int a_beat);
    void update_sizes ();
    void init_before_show ();
    void fill_background_pixmap ();
    void increment_size ();
    void draw_progress ();
    void redraw_dirty_sequences ();
    void draw_all ();                       // used by perfroll_input

private:

    void set_ppqn (int ppqn);
    void convert_xy (int x, int y, long & ticks, int & seq);
    void convert_x (int x, long & ticks);
    void snap_x (int & x);
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
    void on_size_allocate (Gtk::Allocation &);
    bool on_key_press_event (GdkEventKey * a_p0);

    /**
     *  This callback throws away a size request.
     */

    void on_size_request (GtkRequisition * /*a_r*/ )
    {
        // Empty body
    }

};

}           // namespace seq64

#endif      // SEQ64_PERFROLL_HPP

/*
 * perfroll.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
