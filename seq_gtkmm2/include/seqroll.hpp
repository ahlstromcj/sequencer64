#ifndef SEQ64_SEQROLL_HPP
#define SEQ64_SEQROLL_HPP

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
 * \file          seqroll.hpp
 *
 *  This module declares/defines the base class for drawing on the piano
 *  roll of the patterns editor.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2015-09-13
 * \license       GNU GPLv2 or above
 *
 */

#include <gtkmm/drawingarea.h>

#include "fruityseqroll.hpp"
#include "seq24seqroll.hpp"

namespace Gtk
{
    class Adjustment;
}

namespace seq64
{

class sequence;
class perform;
class seqdata;
class seqevent;
class seqkeys;

/**
 *  A small helper class representing a rectangle.
 */

class rect
{
public:

    int x, y, height, width;
};

/**
 *  Implements the piano roll section of the pattern editor.
 */

class seqroll : public Gtk::DrawingArea
{
    friend struct FruitySeqRollInput;
    friend struct Seq24SeqRollInput;

private:

    Glib::RefPtr<Gdk::GC> m_gc;
    Glib::RefPtr<Gdk::Window> m_window;
    Gdk::Color m_black;
    Gdk::Color m_white;
    Gdk::Color m_grey;
    Gdk::Color m_dk_grey;
    Gdk::Color m_red;
    Glib::RefPtr<Gdk::Pixmap> m_pixmap;
    perform * const m_mainperf;
    int m_window_x;
    int m_window_y;
    int m_current_x;
    int m_current_y;
    int m_drop_x;
    int m_drop_y;
    Gtk::Adjustment * const m_vadjust;
    Gtk::Adjustment * const m_hadjust;
    Glib::RefPtr<Gdk::Pixmap> m_background;
    rect m_old;
    rect m_selected;
    sequence * const m_seq;
    sequence * m_clipboard;
    seqdata * const m_seqdata_wid;
    seqevent * const m_seqevent_wid;
    seqkeys * const m_seqkeys_wid;
    FruitySeqRollInput m_fruity_interaction;
    Seq24SeqRollInput m_seq24_interaction;
    int m_pos;

    /**
     * one pixel == m_zoom ticks*
     */

    int m_zoom;

    int m_snap;
    int m_note_length;
    int m_scale;
    int m_key;

    /**
     *  Indicates what is the data window currently editing.
     */

    unsigned char m_status;
    unsigned char m_cc;

    /**
     *  When highlighting a bunch of events.
     */

    bool m_selecting;
    bool m_moving;
    bool m_moving_init;
    bool m_growing;
    bool m_painting;
    bool m_paste;
    bool m_is_drag_pasting;
    bool m_is_drag_pasting_start;
    bool m_justselected_one;

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
    int m_background_sequence;
    bool m_drawing_background_seq;
    bool m_ignore_redraw;

public:

    seqroll
    (
        perform * a_perf,
        sequence * a_seq,
        int a_zoom, int a_snap,
        seqdata * a_seqdata_wid,
        seqevent * a_seqevent_wid,
        seqkeys * a_seqkeys_wid,
        int a_pos,
        Gtk::Adjustment * a_hadjust,
        Gtk::Adjustment * a_vadjust
    );
    ~seqroll ();

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
     * \setter m_note_length
     */

    void set_note_length (int a_note_length)
    {
        m_note_length = a_note_length;
    }

    /**
     * \setter m_ignore_redraw
     */

    void set_ignore_redraw (bool a_ignore)
    {
        m_ignore_redraw = a_ignore;
    }

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

private:

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

private:            // callbacks

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
};

}           // namespace seq64

#endif      // SEQ64_SEQROLL_HPP

/*
 * seqroll.hpp
 *
 * vim: sw=4 ts=4 wm=8 et ft=cpp
 */
