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
 * \updates       2016-06-19
 * \license       GNU GPLv2 or above
 *
 */

#include "globals.h"
#include "gui_drawingarea_gtk2.hpp"
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

    /**
     *  The x-coordinate of the origin of the rectangle.
     */

    int x;

    /**
     *  The y-coordinate of the origin of the rectangle.
     */

    int y;

    /**
     *  The height of the rectangle, in units of pixels.
     */

    int height;

    /**
     *  The width of the rectangle, in units of pixels.
     */

    int width;
};

/**
 *  Implements the piano roll section of the pattern editor.
 */

class seqroll : public gui_drawingarea_gtk2
{
    /**
     *  These friend implement interaction-specific behavior, although only
     *  the Seq24 interactions support keyboard processing.  (Actually,
     *  keyboard processing is common to both types of behavior.
     */

    friend class FruitySeqRollInput;
    friend class Seq24SeqRollInput;

private:

    /**
     *  We need direct access to the horizontal scrollbar if we want to be
     *  able to make it follow the progress bar.
     */

    Gtk::Adjustment & m_horizontal_adjust;

    /**
     *  We need direct access to the vertical scrollbar if we want to be
     *  able to make it follow PageUp and PageDown
     */

    Gtk::Adjustment & m_vertical_adjust;

    /**
     *  The previous selection rectangle, used for undrawing it.
     */

    rect m_old;

    /**
     *  Used in moving and pasting notes.
     */

    rect m_selected;

    /**
     *  Provides a reference to the seqeunce represented by piano roll.
     */

    sequence & m_seq;

    /*
     *  Provides a sequence object to hold a copy of the .... sequence.
     *  Not used at all.
     *
     *  sequence * m_clipboard;
     */

    /**
     *  Holds a reference to the seqkeys pane that is associated with the
     *  seqroll piano roll.
     */

    seqkeys & m_seqkeys_wid;

    /**
     *  Provides a fruity input object, whether it is needed or not.
     */

    FruitySeqRollInput m_fruity_interaction;

    /**
     *  Provides a normal seq24 input object, which is always needed to
     *  handle, for example, keystroke input.
     */

    Seq24SeqRollInput m_seq24_interaction;

    /**
     *  A position value.  Need to clarify what exactly this member is used
     *  for.
     */

    int m_pos;

    /**
     *  Zoom setting, means that one pixel == m_zoom ticks.
     */

    int m_zoom;

    /**
     *  The grid-snap setting for the piano roll grid.  Same meaning as for the
     *  event-bar grid.  This value is the denominator of the note size used
     *  for the snap.
     */

    int m_snap;

    /**
     *  The value of PPQN for the current MIDI song.  Supports values other
     *  than the default of 192.
     */

    int m_ppqn;

    /**
     *  Holds the note length in force for this sequence.  Used in the
     *  seq24seqroll module only.
     */

    int m_note_length;

    /**
     *  Indicates the musical scale in force for this sequence.
     */

    int m_scale;

    /**
     *  Indicates the musical key in force for this sequence.
     */

    int m_key;

    /**
     *  Set when highlighting a bunch of events.
     */

    bool m_selecting;

    /**
     *  Set when moving a bunch of events.
     */

    bool m_moving;

    /**
     *  Indicates the beginning of moving some events.  Used in the fruity and
     *  seq24 mouse-handling modules.
     */

    bool m_moving_init;

    /**
     *  Indicates that the notes are to be extended or reduced in length.
     */

    bool m_growing;

    /**
     *  Indicates the painting of events.  Used in the fruity and seq24
     *  mouse-handling modules.
     */

    bool m_painting;

    /**
     *  Indicates that we are in the process of painting notes.
     */

    bool m_paste;

    /**
     *  Indicates the drag-pasting of events.  Used in the fruity
     *  mouse-handling module.
     */

    bool m_is_drag_pasting;

    /**
     *  Indicates the drag-pasting of events.  Used in the fruity
     *  mouse-handling module.
     */

    bool m_is_drag_pasting_start;

    /**
     *  Indicates the selection of one event.  Used in the fruity
     *  mouse-handling module.
     */

    bool m_justselected_one;

    /**
     *  Tells where the dragging started, the x value.
     */

    int m_move_delta_x;

    /**
     *  Tells where the dragging started, the y value.
     */

    int m_move_delta_y;

    /**
     *  This item is used in the fruityseqroll module.
     */

    int m_move_snap_offset_x;

    /**
     *  Provides the location of the progress bar.
     */

    int m_progress_x;

    /**
     *  The horizontal value of the scroll window in units of
     *  ticks/pulses/divisions.
     */

    int m_scroll_offset_ticks;

    /**
     *  The vertical offset of the scroll window in units of MIDI notes/keys.
     */

    int m_scroll_offset_key;

    /**
     *  The horizontal value of the scroll window in units of pixels.
     */

    int m_scroll_offset_x;

    /**
     *  The vertical value of the scroll window in units of pixels.
     */

    int m_scroll_offset_y;

    /**
     *  Provides the current scroll page in which the progress bar resides.
     */

    int m_scroll_page;

    /**
     *  Holds the value of the musical background sequence that is shown in
     *  cyan (formerly grey) on the background of the piano roll.
     */

    int m_background_sequence;

    /**
     *  Set to true if the drawing of the background sequence is to be done.
     */

    bool m_drawing_background_seq;

    /**
     *  Set to true to avoid the call to update_and_draw().  Used in
     *  set_background_sequence(), change_horz(), change_vert(), reset()....
     *  Never set to true, except in seq24, let's just comment it out for now.
     *  It hasn't been used in sequencer64 for awhile now.
     *
     * bool m_ignore_redraw;
     */

    /**
     *  The current status/event selected in the seqedit.  Not used in seqroll
     *  at present.
     */

    midibyte m_status;

    /**
     *  The current MIDI control value selected in the seqedit.  Not used in
     *  seqroll at present.
     */

    midibyte m_cc;

public:

    seqroll
    (
        perform & perf, sequence & seq, int zoom, int snap,
        seqkeys & seqkeys_wid, int pos,
        Gtk::Adjustment & hadjust, Gtk::Adjustment & vadjust,
        int ppqn = SEQ64_USE_DEFAULT_PPQN
    );
    virtual ~seqroll ();

    /**
     *  Sets the snap to the given value, and then resets the view.
     *
     * \param snap
     *      Provides the sname value to set.
     */

    void set_snap (int snap)
    {
        m_snap = snap;
        reset();
    }

    void set_zoom (int zoom);

    /**
     * \setter m_note_length
     */

    void set_note_length (int note_length)
    {
        m_note_length = note_length;
    }

    /*
     * \setter m_ignore_redraw
     *
     *  void set_ignore_redraw (bool ignore)
     *  {
     *      m_ignore_redraw = ignore;
     *  }
     *
     */

    void set_key (int key);
    void set_scale (int scale);

    /**
     *  Sets the status to the given parameter, and the CC value to the given
     *  optional control parameter, which defaults to 0.  Unlike the same
     *  function in seqevent, this version does not redraw.  Used by seqedit.
     */

    void set_data_type (midibyte status, midibyte control)
    {
        m_status = status;
        m_cc = control;
    }

    void set_background_sequence (bool state, int seq);
    void update_pixmap ();
    void update_sizes ();
    void update_background ();
    void draw_background_on_pixmap ();
    void draw_events_on_pixmap ();
    void draw_selection_on_window ();
    void draw_progress_on_window ();
    void reset ();
    void update_and_draw (int force = false);
    void redraw ();
    void redraw_events ();
    void start_paste ();
    void follow_progress ();

private:

    virtual void force_draw ();

    /**
     *  This function provides optimization for the on_scroll_event() function.
     *  A duplicate of the one in seqedit.
     *
     * \param step
     *      Provides the step value to use for adjusting the horizontal
     *      scrollbar.  See gui_drawingarea_gtk2::scroll_hadjust() for more
     *      information.
     */

    void horizontal_adjust (double step)
    {
        scroll_hadjust(m_horizontal_adjust, step);
    }

    /**
     *  This function provides optimization for the on_scroll_event() function.
     *  A duplicate of the one in seqedit.
     *
     * \param step
     *      Provides the step value to use for adjusting the vertical
     *      scrollbar.  See gui_drawingarea_gtk2::scroll_vadjust() for more
     *      information.
     */

    void vertical_adjust (double step)
    {
        scroll_vadjust(m_vertical_adjust, step);
    }

    /**
     *  Snaps the y value to the piano-key "height".
     *
     * \param [out] y
     *      The y-value to be snapped.
     */

    void snap_y (int & y)
    {
        y -= (y % c_key_y);
    }

    void snap_x (int & x);
    void convert_xy (int x, int y, midipulse & ticks, int & note);
    void convert_tn (midipulse ticks, int note, int & x, int & y);
    void xy_to_rect
    (
        int x1, int y1, int x2, int y2,
        int & x, int & y, int & w, int & h
    );
    void convert_tn_box_to_rect
    (
        midipulse tick_s, midipulse tick_f, int note_h, int note_l,
        int & x, int & y, int & w, int & h
    );
    void draw_events_on (Glib::RefPtr<Gdk::Drawable> draw);
    int idle_redraw ();
    int idle_progress ();
    void change_horz ();
    void change_vert ();
    void move_selection_box (int dx, int dy);           // new
    void move_selected_notes (int dx, int dy);          // new
    void grow_selected_notes (int dx);                  // new

private:            // callbacks

    void on_realize ();
    bool on_expose_event (GdkEventExpose * ev);
    bool on_button_press_event (GdkEventButton * ev);
    bool on_button_release_event (GdkEventButton * ev);
    bool on_motion_notify_event (GdkEventMotion * ev);
    bool on_focus_in_event (GdkEventFocus *);
    bool on_focus_out_event (GdkEventFocus *);
    bool on_key_press_event (GdkEventKey * ev);
    bool on_scroll_event (GdkEventScroll * a_ev);
    void on_size_allocate (Gtk::Allocation &);
    bool on_leave_notify_event (GdkEventCrossing * p0);
    bool on_enter_notify_event (GdkEventCrossing * p0);

};

}           // namespace seq64

#endif      // SEQ64_SEQROLL_HPP

/*
 * seqroll.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

