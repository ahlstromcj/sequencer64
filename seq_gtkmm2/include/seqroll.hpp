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
 * \updates       2018-08-10
 * \license       GNU GPLv2 or above
 *
 *  We are currently moving toward making this class a base class.
 *
 *  User jean-emmanual added support for disabling the following of the
 *  progress bar during playback.  See the seqroll::m_progress_follow member.
 */

#include "globals.h"
#include "gui_drawingarea_gtk2.hpp"
#include "rect.hpp"                     /* seq64::rect class        */
#include "sequence.hpp"
#include "scales.h"                     /* STAZED chord support     */

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace Gtk
{
    class Adjustment;
}

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{
    class perform;
    class seqdata;
    class seqevent;
    class seqkeys;

/**
 *  Implements the piano roll section of the pattern editor.
 */

class seqroll : public gui_drawingarea_gtk2
{
    /**
     *  For accessing on_key_press_event().  It would be good to be able to
     *  avoid this access!
     *
     * \change layk 2016-10-17
     *      Issue #46. No need for this declaration now, due to the fix in
     *      seqedit.
     *
     * friend class seqedit;
     */

protected:

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
     *  Provides a reference to the sequence represented by piano roll.
     */

    sequence & m_seq;

    /**
     *  Holds a reference to the seqkeys pane that is associated with the
     *  seqroll piano roll.
     */

    seqkeys & m_seqkeys_wid;

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
     *  Holds the note length in force for this sequence.  Used in the
     *  seq24seqroll module only.
     */

    int m_note_length;

    /**
     *  Indicates the musical scale in force for this sequence.
     */

    int m_scale;

#ifdef SEQ64_STAZED_CHORD_GENERATOR

    /**
     *  Indicates the current chord in force for this sequence for inserting
     *  notes.
     */

    int m_chord;

#endif

    /**
     *  Indicates the musical key in force for this sequence.
     */

    int m_key;

    /**
     *  Set when in note-adding mode.  This flag was moved from both
     *  the fruity and the seq24 seqroll classes.
     */

    bool m_adding;

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
     *  Indicates that we are in the process of pasting notes.
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
     *  Provides the old location of the progress bar.
     */

    int m_old_progress_x;

    /**
     *  Provides the previous scroll value.
     */

    int m_last_scroll;

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

#ifdef SEQ64_FOLLOW_PROGRESS_BAR

    /**
     *  Provides the current scroll page in which the progress bar resides.
     */

    int m_scroll_page;

    /**
     *  Progress bar follow state.
     */

    bool m_progress_follow;

#endif

    /**
     *  Indicates if we are going to follow the transport in the GUI.
     *  Progress follow?
     */

    bool m_transport_follow;

    /**
     *  TBD.
     */

    bool m_trans_button_press;

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
     *  From Seq32 ?
     */

    bool m_ignore_redraw;

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
        perform & perf,
        sequence & seq,
        int zoom,
        int snap,
        seqkeys & seqkeys_wid,
        int pos,
        Gtk::Adjustment & hadjust,
        Gtk::Adjustment & vadjust
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
     * Convenience functions.
     */

    /**
     * \getter m_note_length, adjusted for the note_off_margin.
     */

    int note_off_length () const
    {
        return m_note_length - m_seq.note_off_margin();
    }

    bool add_note (midipulse tick, int note, bool paint = true);

    void set_ignore_redraw (bool ignore)
    {
        m_ignore_redraw = ignore;
    }

    void set_key (int key);
    void set_scale (int scale);

#ifdef SEQ64_STAZED_CHORD_GENERATOR
    void set_chord (int chord);
#endif

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

    /*
     *  Completes a paste operation based on the current coordinates in the
     *  piano roll.
     */

    void complete_paste ()
    {
        complete_paste(current_x(), current_y());
    }

    void complete_paste (int x, int y);
    void follow_progress ();

#ifdef SEQ64_FOLLOW_PROGRESS_BAR

    /**
     * \getter m_progress_follow
     */

    void set_progress_follow (bool follow)
    {
        m_progress_follow = follow;
    }

    /**
     * \setter m_progress_follow
     */

    bool get_progress_follow ()
    {
        return m_progress_follow;
    }

#endif

protected:

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
    void convert_xy (int x, int y, midipulse & tick, int & note);

    /**
     *  Convenience function that calls convert_xy() for the drop and y
     *  values.
     *
     * \param tick
     *      The horizontal location of the drop.
     *
     * \param note
     *      The vertical location of the drop.
     */

    void convert_drop_xy (midipulse & tick, int & note)
    {
        convert_xy(m_drop_x, m_drop_y, tick, note);
    }

    void convert_tn (midipulse tick, int note, int & x, int & y);
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
    void convert_tn_box_to_rect
    (
        midipulse tick_s, midipulse tick_f, int note_h, int note_l, rect & r
    );
    void convert_sel_box_to_rect
    (
        midipulse tick_s, midipulse tick_f, int note_h, int note_l
    );
    void get_selected_box
    (
        midipulse & tick_s, int & note_h, midipulse & tick_f, int & note_l
    );
    void draw_events_on (Glib::RefPtr<Gdk::Drawable> draw);
    int idle_redraw ();
    int idle_progress ();
    void change_horz ();
    void change_vert ();
    void move_selection_box (int dx, int dy);           // new
    void move_selected_notes (int dx, int dy);          // new
    void grow_selected_notes (int dx);                  // new
    void set_adding (bool adding);                      // from seq24 seqroll
    void update_mouse_pointer (bool adding = false);    // from fruity seqroll

    bool button_press_initial
    (
        GdkEventButton * ev, int & norm_x, int & snapped_x, int & snapped_y
    );
    void align_selection
    (
        midipulse & tick_s, int & note_h,
        midipulse & tick_f, int & note_l, int snapped_x
    );

    /* virtual */ bool button_press (GdkEventButton * ev);
    /* virtual */ bool button_release (GdkEventButton * ev);
    /* virtual */ bool motion_notify (GdkEventMotion * ev);

#ifdef USE_UNREADY_NEW_FUNCTIONS
    void set_hint_note ();
    void add_snapped_note ();
#endif

protected:            // new internal/friend functions

    /**
     *  Clears all the mouse-action flags.
     */

    void clear_flags ()
    {
		m_selecting = m_moving = m_growing = m_paste = m_moving_init =
			 m_painting = false;
    }

    void set_scroll_x ();
    void set_scroll_y ();

    /**
     *  Useful x calculation.  Offsets the x value by the x origin of the
     *  current page.
     *
     * \param x
     *      The x value to offset.
     */

    int scroll_offset_x (int x) const
    {
        return x + m_scroll_offset_x;
    }

    /**
     *  Useful y calculation.  Offsets the y value by the y origin of the
     *  current page.
     *
     * \param y
     *      The y value to offset.
     */

    int scroll_offset_y (int y) const
    {
        return y + m_scroll_offset_y;
    }

    /**
     *  Useful x and y calculation.  Offsets the current x and y values by the
     *  x and y origin of the current page.
     *
     * \param x
     *      The y value to offset.
     *
     * \param y
     *      The y value to offset.
     */

    void set_current_offset_x_y (int x, int y)
    {
        m_current_x = x + m_scroll_offset_x;
        m_current_y = y + m_scroll_offset_y;
    }

    /**
     * \getter m_adding
     */

    bool adding () const
    {
        return m_adding;
    }

    /**
     * \getter m_selecting
     */

    bool selecting () const
    {
        return m_selecting;
    }

    /**
     * \getter m_growing
     */

    bool growing () const
    {
        return m_growing;
    }

    /**
     *  Indicates if we're drag-pasting, selecting, moving, growing, or pasting.
     *
     * \return
     *      Returns true if one of those five flags are set.
     */

    bool normal_action () const
    {
        return m_is_drag_pasting || select_action();
    }

    /**
     *  Indicates if we're selecting, moving, growing, or pasting.
     *
     * \return
     *      Returns true if one of those four flags are set.
     */

    bool select_action () const
    {
        return selecting() || growing() || drop_action();
    }

    /**
     *  Indicates if we're moving or pasting.
     *
     * \return
     *      Returns true if one of those two flags are set.
     */

    bool drop_action () const
    {
        return moving() || m_paste;
    }

    /**
     * \getter m_moving
     */

    bool moving () const
    {
        return m_moving;
    }

protected:            // callbacks

    virtual void on_realize ();
    virtual bool on_expose_event (GdkEventExpose * ev);
    virtual bool on_button_press_event (GdkEventButton * ev);
    virtual bool on_button_release_event (GdkEventButton * ev);
    virtual bool on_motion_notify_event (GdkEventMotion * ev);
    virtual bool on_focus_in_event (GdkEventFocus *);
    virtual bool on_focus_out_event (GdkEventFocus *);
    virtual bool on_key_press_event (GdkEventKey * ev);
    virtual bool on_scroll_event (GdkEventScroll * a_ev);
    virtual void on_size_allocate (Gtk::Allocation &);
    virtual bool on_leave_notify_event (GdkEventCrossing * p0);
    virtual bool on_enter_notify_event (GdkEventCrossing * p0);

};          // class seqroll

}           // namespace seq64

#endif      // SEQ64_SEQROLL_HPP

/*
 * seqroll.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

