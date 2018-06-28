#ifndef SEQ64_QSEQBASE_HPP
#define SEQ64_QSEQBASE_HPP

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
 * \file          qseqbase.hpp
 *
 *  This module declares/defines the base class for the various editing panes
 *  of Sequencer64's Qt 5 version.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2018-06-20
 * \updates       2018-06-28
 * \license       GNU GPLv2 or above
 *
 *  This class WILL BE the base class for qseqroll, qseqdata, qtriggereditor,
 *  and qseqtime, the four panes of the qseqeditframe64 class or the legacy
 *  Kepler34 qseqeditframe class.
 *
 *  And maybe we can use it in the qperf* classes as well.
 *
 *  It will be used as a mix-in class
 */

#include "app_limits.h"                 /* SEQ64_DEFAULT_ZOOM, _SNAP    */
#include "rect.hpp"

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{
    class perform;
    class sequence;

/**
 * The MIDI note grid in the sequence editor
 */

class qseqbase
{

private:

    /**
     *  Provides a reference to the performance object.
     */

    perform & m_perform;

    /**
     *  Provides a reference to the sequence represented by piano roll.
     */

    sequence & m_seq;

    /**
     *  The previous selection rectangle, used for undrawing it.  Accessed by
     *  the getter/setting functions old_rect().
     */

    seq64::rect m_old;

    /**
     *  Used in moving and pasting notes.  Accessed by the getter/setting
     *  functions selection().
     */

    seq64::rect m_selected;

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
     *  Set when highlighting a bunch of events.
     */

    bool m_selecting;

    /**
     *  Set when in note-adding mode.  This flag was moved from both
     *  the fruity and the seq24 seqroll classes.
     */

    bool m_adding;

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
     *  The x size of the window.  Would be good to allocate this
     *  to a base class for all grid panels.  In Qt 5, this is the width().
     */

    int m_window_width;         // might remove, as Qt's width() is available

    /**
     *  The y size of the window.  Would be good to allocate this
     *  to a base class for all grid panels.  In Qt 5, this is the height().
     */

    int m_window_height;       // might remove, as Qt's height() is available

    /**
     *  The x location of the mouse when dropped.  Would be good to allocate this
     *  to a base class for all grid panels.
     */

    int m_drop_x;

    /**
     *  The x location of the mouse when dropped.  Would be good to allocate this
     *  to a base class for all grid panels.
     */

    int m_drop_y;

    /**
     *  Tells where the dragging started, the x value.
     */

    int m_move_delta_x;

    /**
     *  Tells where the dragging started, the y value.
     */

    int m_move_delta_y;

    /**
     *  Current x coordinate of pointer. Could move it to a base class.
     */

    int m_current_x;

    /**
     *  Current y coordinate of pointer. Could move it to a base class.
     */

    int m_current_y;

    /**
     *  This item is used in the fruityseqroll module.
     */

    int m_move_snap_offset_x;

    /**
     *  Provides the location of the progress bar.
     */

    int m_progress_x;

    /**
     *  Provides the old location of the progress bar, for "playhead" tracking.
     */

    int m_old_progress_x;

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
     *  See qseqroll::keyY.
     */

    int m_unit_height;

    /**
     *  See qseqroll::keyY * c_num_keys + 1.
     */

    int m_total_height;

    /**
     *
     */

    bool m_is_dirty;

public:

    qseqbase
    (
        perform & perf,
        sequence & seq,
        int zoom            = SEQ64_DEFAULT_ZOOM,
        int snap            = SEQ64_DEFAULT_SNAP,
        int unit_height     =  1,
        int total_height    =  1
    );

    const seq64::rect & old_rect () const
    {
        return m_old;
    }

    seq64::rect & old_rect ()
    {
        return m_old;
    }

    const seq64::rect & selection () const
    {
        return m_selected;
    }

    seq64::rect & selection ()
    {
        return m_selected;
    }

    int zoom () const
    {
        return m_zoom;
    }

    bool is_dirty () const
    {
        return m_is_dirty;
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
     *  Indicates if we're drag-pasting, selecting, moving, growing, or
     *  pasting.
     *
     * \return
     *      Returns true if one of those five flags are set.
     */

    bool normal_action () const
    {
        return m_is_drag_pasting || select_action();
    }

    /**
     *  Indicates if we're moving or pasting.
     *
     * \return
     *      Returns true if one of those two flags are set.
     */

    bool drop_action () const
    {
        return moving() || paste();
    }

    int snap () const
    {
        return m_snap;
    }

    bool selecting () const
    {
        return m_selecting;
    }

    bool adding () const
    {
        return m_adding;
    }

    bool moving () const
    {
        return m_moving;
    }

    bool moving_init () const
    {
        return m_moving_init;
    }

    bool growing () const
    {
        return m_growing;
    }

    bool painting () const
    {
        return m_painting;
    }

    bool paste () const
    {
        return m_paste;
    }

    bool is_drag_pasting () const
    {
        return m_is_drag_pasting;
    }

    bool is_drag_pasting_start () const
    {
        return m_is_drag_pasting_start;
    }

    bool just_selected_one () const
    {
        return m_justselected_one;
    }

    // might remove, as Qt's width() and height() are available

    int window_width () const
    {
        return m_window_width;
    }

    int window_height () const
    {
        return m_window_height;
    }

    int drop_x () const
    {
        return m_drop_x;
    }

    int drop_y () const
    {
        return m_drop_y;
    }

    void snap_drop_x ()
    {
        snap_x(m_drop_x);
    }

    void snap_drop_y ()
    {
        snap_y(m_drop_y);
    }

    int move_delta_x () const
    {
        return m_move_delta_x;
    }

    int move_delta_y () const
    {
        return m_move_delta_y;
    }

    int current_x () const
    {
        return m_current_x;
    }

    int current_y () const
    {
        return m_current_y;
    }

    int move_snap_offset_x () const
    {
        return m_move_snap_offset_x;
    }

    int progress_x () const
    {
        return m_progress_x;
    }

    int old_progress_x () const
    {
        return m_old_progress_x;
    }

#ifdef SEQ64_FOLLOW_PROGRESS_BAR
    int scroll_page () const
    {
        return m_scroll_page;
    }

    bool progress_follow () const
    {
        return m_progress_follow;
    }
#endif

    int scroll_offset_ticks () const
    {
        return m_scroll_offset_ticks;
    }

    int scroll_offset_key () const
    {
        return m_scroll_offset_key;
    }

    int scroll_offset_x () const
    {
        return m_scroll_offset_x;
    }

    int scroll_offset_y () const
    {
        return m_scroll_offset_y;
    }

    int unit_height () const
    {
        return m_unit_height;
    }

    int total_height () const
    {
        return m_total_height;
    }

public:

    void zoom_in ()
    {
        if (m_zoom > 1)         // restricted more by qseqeditframe64
            m_zoom /= 2;
    }

    void zoom_out ()
    {
        if (m_zoom < 32)       // restricted more by qseqeditframe64
            m_zoom *= 2;
    }

    void set_zoom (int z)
    {
        m_zoom = z;             // must be validated by the caller
    }

    /**
     * \setter m_snap
     */

    void set_snap (int snap)
    {
        m_snap = snap;
    }

    bool needs_update () const;

    /**
     *  Used by qseqeditframe64 to force a redraw when the user changes
     *  a sequence parameter in this frame.
     */

    void set_dirty (bool f = true)
    {
        m_is_dirty = f;
        //////// perf().modify();   /////// TODO
    }

protected:

    bool check_dirty ()
    {
        bool result = m_is_dirty;
        m_is_dirty = false;
        return result;
    }

    void old_rect (seq64::rect & r)
    {
        m_old = r;
    }

    void selection (seq64::rect & r)
    {
        m_selected = r;
    }

    /**
     *  Clears all the mouse-action flags.
     */

    void clear_action_flags ()
    {
		m_selecting = m_moving = m_growing = m_paste = m_moving_init =
			 m_painting = false;
    }

    void selecting (bool v)
    {
        m_selecting = v;
    }

    void adding (bool v)
    {
        m_adding = v;
    }

    void moving (bool v)
    {
        m_moving = v;
    }

    void moving_init (bool v)
    {
        m_moving_init = v;
    }

    void growing (bool v)
    {
        m_growing = v;
    }

    void painting (bool v)
    {
        m_painting = v;
    }

    void paste (bool v)
    {
        m_paste = v;
    }

    void is_drag_pasting (bool v)
    {
        m_is_drag_pasting = v;
    }

    void is_drag_pasting_start (bool v)
    {
        m_is_drag_pasting_start = v;
    }

    void justselected_one (bool v)
    {
        m_justselected_one = v;
    }

    // might remove, as Qt's width() and height() are available
    void window_width (int v)
    {
        m_window_width = v;
    }

    void window_height (int v)
    {
        m_window_height = v;
    }

    void drop_x (int v)
    {
        m_drop_x = v;
    }

    void drop_y (int v)
    {
        m_drop_y = v;
    }

    void move_delta_x (int v)
    {
        m_move_delta_x = v;
    }

    void move_delta_y (int v)
    {
        m_move_delta_y = v;
    }

    void current_x (int v)
    {
        m_current_x = v;
    }

    void current_y (int v)
    {
        m_current_y = v;
    }

    void move_snap_offset_x (int v)
    {
        m_move_snap_offset_x = v;
    }

    void progress_x (int v)
    {
        m_progress_x = v;
    }

    void old_progress_x (int v)
    {
        m_old_progress_x = v;
    }

#ifdef SEQ64_FOLLOW_PROGRESS_BAR
    void scroll_page (int v)
    {
        m_scroll_page = v;
    }

    void progress_follow (bool v)
    {
        m_progress_follow = v;
    }

#endif
    void scroll_offset_ticks (int v)
    {
        m_scroll_offset_ticks = v;
    }

    void scroll_offset_key (int v)
    {
        m_scroll_offset_key = v;
    }

    void scroll_offset_x (int v)
    {
        m_scroll_offset_x = v;
    }

    void scroll_offset_y (int v)
    {
        m_scroll_offset_y = v;
    }

    void unit_height (int v)
    {
        m_unit_height = v;
    }

    void total_height (int v)
    {
        m_total_height = v;
    }


protected:

    void set_scroll_x ();
    void set_scroll_y ();

    const perform & perf () const
    {
        return m_perform;
    }

    perform & perf ()
    {
        return m_perform;
    }

    const sequence & seq () const
    {
        return m_seq;
    }

    sequence & seq ()
    {
        return m_seq;
    }

    void snap_x (int & x);

    void snap_current_x ()
    {
        snap_x(m_current_x);
    }

    void snap_y (int & y)
    {
        y -= y % m_unit_height;
    }

    void snap_current_y ()
    {
        snap_y(m_current_y);
    }

    void swap_x ()
    {
        int temp = m_current_x;
        m_current_x = m_drop_x;
        m_drop_x = temp;
    }

    void swap_y ()
    {
        int temp = m_current_y;
        m_current_y = m_drop_y;
        m_drop_y = temp;
    }

    /*
     * Takes screen corrdinates, give us notes/keys (to be generalized to
     * other vertical user-interface quantities) and ticks (always the
     * horizontal user-interface quantity).
     */

    void convert_xy (int x, int y, midipulse & ticks, int & note);
    void convert_tn (midipulse ticks, int note, int & x, int & a_y);
    void convert_tn_box_to_rect
    (
        midipulse tick_s, midipulse tick_f, int note_h, int note_l,
        seq64::rect & r
    );

    /**
     *  Meant to be overridden by derived classes to change a user-interface
     *  item, such as the mouse pointer, when entering an adding mode.
     *
     * \param a
     *      The value of the status of adding (e.g. a note).
     */

    virtual void set_adding (bool a)
    {
        adding(a);
    }

    void start_paste();

};          // class qseqbase

}           // namespace seq64

#endif      // SEQ64_QSEQBASE_HPP

/*
 * qseqbase.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

