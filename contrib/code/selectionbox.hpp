#ifndef SEQ64_SELECTIONBOX_HPP
#define SEQ64_SELECTIONBOX_HPP

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
 * \file          selectionbox.hpp
 *
 *  This module declares/defines a class for encapulating a selection box.
 *
 * \library       sequencer64 application
 * \author        Chris Ahlstrom
 * \date          2017-09-16
 * \updates       2017-09-16
 * \license       GNU GPLv2 or above
 *
 *  This class is intended to hold numeric and status information that
 *  allows a user-interface to maintain a selection box.  It will contain no
 *  GUI library code at all.
 */

#include "globals.h"
#include "rect.hpp"

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{
    class perform;

/**
 *  Stores status information for the piano rolls used in Sequencer64.  This
 *  class is all data, with a few supporting functions.  Meant to be mixed
 *  into some of the user-interface objects.
 */

class selectionbox
{

private:

    /**
     *  The previous selection rectangle, used for undrawing it.
     */

    rect m_old;

    /**
     *  Used in moving and pasting notes.
     */

    rect m_selected;

    /**
     *  Zoom setting, means that one pixel == m_zoom ticks.
     */

    int m_zoom;

    /**
     *  The grid-snap setting for a piano roll grid.  This value is the
     *  denominator of the item size used for the snap.
     */

    int m_snap_x;

    /**
     *  The grid-snap setting for a piano roll grid.  This value is the
     *  denominator of the item size used for the snap.
     */

    int m_snap_x;

    /**
     *  The value of PPQN for the current MIDI song.  Supports values other
     *  than the default of 192.  Needed for scaling in time.
     */

    int m_ppqn;

    /**
     *  Set when in item-adding mode.
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
     *  Indicates that we are in the process of pasting items.
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
     *  This items are in gui_drawingarea_gtk2.
     *
    int m_drop_x;
    int m_drop_y;
    int m_current_x;
     *
     *  We also need to worry about:
     *
     *      m_scroll_offset_x
     *      m_scroll_offset_y
     *      c_rollarea_x (if applicable)
     *      c_rollarea_y
     *      c_key_x (if applicable)
     *      c_key_y
     *      midipulses vs int notes
     *
     */

    int m_current_y;

    /**
     *  Tells where the dragging started, the x value.
     */

    int m_move_delta_x;

    /**
     *  Tells where the dragging started, the y value.
     */

    int m_move_delta_y;

    /**
     *  This item is used in the fruityselectionbox module.
     */

    int m_move_snap_offset_x;

public:

    selectionbox
    (
        perform & perf,
        sequence & seq,
        int zoom,
        int snapx,
        int snapy,
        int ppqn = SEQ64_USE_DEFAULT_PPQN
    );

    /**
     *  Sets the snap to the given value.
     *
     * \param snap
     *      Provides the snap value to set.
     */

    void set_snap (int snap)
    {
        m_snap = snap;
    }

    /**
     *  Need to implement zoom limits.
     */

    void set_zoom (int zoom)
    {
        if (m_zoom != zoom)
            m_zoom = zoom;
    }

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

private:

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
    void convert_sel_box_to_rect
    (
        midipulse tick_s, midipulse tick_f, int note_h, int note_l
    );
    void get_selected_box
    (
        midipulse & tick_s, int & note_h, midipulse & tick_f, int & note_l
    );

    void move_selection_box (int dx, int dy);           // new
    void move_selected_notes (int dx, int dy);          // new
    void grow_selected_notes (int dx);                  // new
    void set_adding (bool adding);                      // from seq24 selectionbox

    void align_selection
    (
        midipulse & tick_s, int & note_h,
        midipulse & tick_f, int & note_l, int snapped_x
    );

private:            // new internal/friend functions

    /**
     * \setter m_old
     */

    void clear_selected ()
    {
        m_selected.x = m_selected.y = m_selected.width = m_selected.height = 0;
    }

    /**
     * \setter m_old
     */

    void clear_old ()
    {
        m_old.x = m_old.y = m_old.width = m_old.height = 0;
    }

    /**
     *  Clears all the mouse-action flags.
     */

    void clear_flags ()
    {
		m_selecting = m_moving = m_growing = m_paste = m_moving_init =
			 m_painting = false;
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
        return m_selecting || m_growing || drop_action();
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

};

}           // namespace seq64

#endif      // SEQ64_SELECTIONBOX_HPP

/*
 * selectionbox.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

