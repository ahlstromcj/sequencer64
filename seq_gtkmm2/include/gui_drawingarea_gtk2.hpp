#ifndef SEQ64_GUI_DRAWINGAREA_GTK2_HPP
#define SEQ64_GUI_DRAWINGAREA_GTK2_HPP

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
 * \file          gui_drawingarea.hpp
 *
 *  This module declares/defines the base class for drawing on the piano
 *  roll of the patterns editor.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-09-21
 * \updates       2015-09-22
 * \license       GNU GPLv2 or above
 *
 */

#include "gui_palette_gtk2.hpp"         // #include <gtkmm/drawingarea.h>

namespace Gtk
{
    class Adjustment;
}

namespace seq64
{

class perform;                          // forward reference

/**
 *  Implements the basic drawing areas of the application.  Note that this
 *  class really "isn't a" gui_pallete_gtk2; it should simply have one.
 *  But that base class must be derived from Gtk::DrawingArea.  We don't
 *  want to waste some space by using a "has-a" relationship, and also put
 *  up with having to access the palette indirectly.  So, in this case, we
 *  tolerate the less strict implementation.
 */

class gui_drawingarea_gtk2 : public gui_palette_gtk2
{

public:

    /**
     *  A small helper structure representing a rectangle.
     */

    struct rect
    {
        public: int x, y, height, width;
    };

private:

    Glib::RefPtr<Gdk::GC> m_gc;
    Glib::RefPtr<Gdk::Window> m_window;
    Gtk::Adjustment & m_vadjust;
    Gtk::Adjustment & m_hadjust;
    Glib::RefPtr<Gdk::Pixmap> m_pixmap;
    Glib::RefPtr<Gdk::Pixmap> m_background;
    perform & m_mainperf;

#if 0
    int m_window_x;
    int m_window_y;
    int m_current_x;
    int m_current_y;
    int m_drop_x;
    int m_drop_y;
    rect m_old;
    int m_pos;
    int m_zoom;
    int m_snap;
#endif

#if 0
    /**
     *  When highlighting a bunch of events.
     */

    rect m_selected;
    bool m_selecting;
    bool m_moving;
    bool m_moving_init;
    bool m_growing;
    bool m_painting;
    bool m_paste;
    bool m_is_drag_pasting;
    bool m_is_drag_pasting_start;
#endif

#if 0
    /**
     *  Tells where the dragging started.
     */

    int m_move_delta_x;
    int m_move_delta_y;
    int m_move_snap_offset_x;
    int m_old_progress_x;
    int m_scroll_offset_ticks;
    int m_scroll_offset_key;
    int m_scroll_offset_x;
    int m_scroll_offset_y;
    bool m_ignore_redraw;
#endif

public:

    gui_drawingarea_gtk2
    (
        perform & a_perf,
        Gtk::Adjustment & a_hadjust,
        Gtk::Adjustment & a_vadjust
    );
    ~gui_drawingarea_gtk2 ();

#if 0
    void reset ();
    void redraw ();
    void redraw_events ();
    void set_key (int a_key);
    void set_scale (int a_scale);

    /**
     *  Sets the snap to the given value, and then resets the view.
     */

    void set_snap (int a_snap)
    {
        m_snap = a_snap;
        reset();
    }

    void set_zoom (int a_zoom);

    /**
     * \setter m_ignore_redraw
     */

    void set_ignore_redraw (bool a_ignore)
    {
        m_ignore_redraw = a_ignore;
    }
#endif  // 0

#if 0
    void set_data_type (unsigned char a_status, unsigned char a_control);
    void set_background_sequence (bool a_state, int a_seq);
    void update_pixmap ();
    void update_sizes ();
    void update_background ();
    void draw_background_on_pixmap ();
    void draw_events_on_pixmap ();
    void draw_selection_on_window ();
    void draw_progress_on_window ();
    int idle_redraw ();
    void start_paste ();
#endif  // 0

private:

#if 0
    void convert_xy (int a_x, int a_y, long * a_ticks, int * a_note);
    void convert_tn (long a_ticks, int a_note, int * a_x, int * a_y);
    void snap_y (int * a_y);
    void snap_x (int * a_x);
    void xy_to_rect
    (
        int a_x1, int a_y1, int a_x2, int a_y2,
        int * a_x, int * a_y, int * a_w, int * a_h
    );
    void convert_tn_box_to_rect
    (
        long a_tick_s, long a_tick_f, int a_note_h, int a_note_l,
        int * a_x, int * a_y, int * a_w, int * a_h
    );
    void draw_events_on (Glib::RefPtr<Gdk::Drawable> a_draw);
    int idle_progress ();
    void change_horz ();
    void change_vert ();
    void force_draw ();
#endif  // 0

private:            // callbacks

#if 0
    void on_realize ();
    bool on_expose_event (GdkEventExpose * a_ev);
    bool on_button_press_event (GdkEventButton * a_ev);
    bool on_button_release_event (GdkEventButton * a_ev);
    bool on_motion_notify_event (GdkEventMotion * a_ev);
    bool on_focus_in_event (GdkEventFocus *);
    bool on_focus_out_event (GdkEventFocus *);
    bool on_key_press_event (GdkEventKey * a_p0);
    bool on_scroll_event (GdkEventScroll * a_ev);
    void on_size_allocate (Gtk::Allocation &);
    bool on_leave_notify_event (GdkEventCrossing * a_p0);
    bool on_enter_notify_event (GdkEventCrossing * a_p0);
#endif  // 0

};

}           // namespace seq64

#endif      // SEQ64_GUI_DRAWINGAREA_GTK2_HPP

/*
 * gui_drawingarea.hpp
 *
 * vim: sw=4 ts=4 wm=8 et ft=cpp
 */
