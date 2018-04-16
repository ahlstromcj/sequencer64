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
 *  This module declares/defines the base class for drawing patterns/sequences
 *  in the Patterns Panel grid.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2018-02-17
 * \license       GNU GPLv2 or above
 *
 *  Wonder where the name "wid" came from....
 */

#include "globals.h"                    /* c_max_sequence, etc.     */
#include "gui_drawingarea_gtk2.hpp"     /* one base class           */
#include "seqmenu.hpp"                  /* the other base class     */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{
    class perform;                      /* forward reference        */

/**
 *  This class implements the piano roll area of the application.  It inherits
 *  from gui_drawingarea_gtk2 to support the font, color, and other GUI
 *  functionality, and from seqmenu to support the right-click Edit/New/Cut
 *  right-click menu.  The friend class and function are for updating the
 *  current sequence and for control via the mainwnd object.
 */

class mainwid : public gui_drawingarea_gtk2, public seqmenu
{
    friend class mainwnd;
    friend void update_mainwid_sequences ();

private:

    /**
     *  Holds the progress color for armed sequences, which have a black
     *  background.  If the progress color is black(), we want to change it to
     *  white for unmuted patterns.
     */

    Color m_armed_progress_color;

    /**
     *  Holds a partial copy of the sequence we are moving on the patterns
     *  panel.  The assignment is made by sequence::partial_copy(), which
     *  behaves like the legacy seq24 code.
     */

    sequence m_moving_seq;

    /**
     *  Indicates that the mouse button is still down.  Used in the
     *  drag-and-drop functionality.
     */

    bool m_button_down;

    /**
     *  Indicates that we are still in the middle of a drag-and-drop
     *  operation.
     */

    bool m_moving;

    /**
     *  Holds the sequence number of a sequence being drag-and-dropped.
     */

    int m_old_seq;

    /**
     *  Indicates the current screenset that is visible.
     */

    int m_screenset;

    /**
     *  Holds the last active tick for each sequence, used in erasing the
     *  progress bar.
     */

    long m_last_tick_x[c_max_sequence];

    /**
     *  These values are assigned to the values given by the constants of
     *  similar names in globals.h, and we will make them parameters or
     *  user-interface configuration items later.  Some of them already have
     *  counterparts in the user_settings class.
     */

    const int m_mainwnd_rows;   /**< Number of rows, unused part of settings.   */
    const int m_mainwnd_cols;   /**< Number of columns, unused in settings.     */
    const int m_seqarea_x;      /**< Roughly with width of the main window.     */
    const int m_seqarea_y;      /**< Roughly with height of the main window.    */
    const int m_seqarea_seq_x;  /**< To be determined.                          */
    const int m_seqarea_seq_y;  /**< To be determined.                          */
    const int m_mainwid_x;      /**< Horizontal size of the main window grid.   */
    const int m_mainwid_y;      /**< Vertical size of the main window grid.     */
    const int m_mainwid_border_x;   /**< Border offset fudge factor, x.         */
    const int m_mainwid_border_y;   /**< Border offset fudge factor, y.         */
    const int m_mainwid_spacing;    /**< Spacing, untested setting.             */
    const int m_text_size_x;    /**< Text width, varies with font in use.       */
    const int m_text_size_y;    /**< Text height, varies with font in use.      */
    const int m_max_sets;       /**< Maximum number of sets, used all over.     */

    /**
     *  Provides a convenience variable for avoiding multiplications.
     *  It is equal to m_mainwnd_rows * m_mainwnd_cols.
     */

    const int m_screenset_slots;

    /**
     *  Provides a convenience variable for avoiding multiplications.
     *  It is equal to m_screenset_slots * m_screenset.
     */

    int m_screenset_offset;

    /**
     *  Provides the height of the progress bar, to save calculations and for
     *  consistency between drawing and erasing the progress bar.
     */

    const int m_progress_height;

public:

    mainwid (perform & p, int ss = 0);
    virtual ~mainwid ();
    int set_screenset (int ss);

private:

    void log_screenset (int ss);

    /**
     *  This function redraws everything and queues up a redraw operation.
     */

    void reset ()
    {
        draw_sequences_on_pixmap();
        draw_pixmap_on_window();
    }

    /**
     *  Updates the image of multiple sequencer/pattern slots.  Used by the
     *  friend class mainwnd, but also useful for our new feature to
     *  fully highlight the current sequence.  Calls reset() if
     *  SEQ64_EDIT_SEQUENCE_HIGHLIGHT is defined.
     */

    void update_sequences_on_window ()
    {
#ifdef SEQ64_EDIT_SEQUENCE_HIGHLIGHT
        reset();
#endif
    }

    /**
     *  This function queues the blit of pixmap to window.
     */

    void draw_pixmap_on_window ()
    {
        queue_draw();
    }

    /**
     *  This function updates the background window, clearing it.
     */

    void fill_background_window ()
    {
        draw_normal_rectangle_on_pixmap(0, 0, m_window_x, m_window_y);
    }

    /**
     * \getter m_mainwid_x
     */

    int nominal_width () const
    {
        return m_mainwid_x;
    }

    /**
     * \getter m_mainwid_y
     */

    int nominal_height () const
    {
        return m_mainwid_y;
    }

    virtual void redraw (int seq);              /* override seqmenu's       */
    virtual void seq_set_and_edit (int seqnum); /* ditto                    */
    virtual void seq_set_and_eventedit (int seqnum);

    void draw_marker_on_sequence (int seq, int tick);
    void update_markers (int ticks);            /* ditto                    */
    bool valid_sequence (int seq);
    void draw_sequence_on_pixmap (int seq);
    void draw_sequences_on_pixmap ();
    void draw_sequence_pixmap_on_window (int seq);
    int seq_from_xy (int x, int y);
    int timeout ();
    void calculate_base_sizes (int seq, int & basex, int & basey);
    void select_fg_bg_colors (int seqnum);

private:    // Gtkmm 2.4 callbacks

    void on_realize ();
    bool on_expose_event (GdkEventExpose * ev);
    bool on_button_press_event (GdkEventButton * ev);
    bool on_button_release_event (GdkEventButton * ev);
    bool on_motion_notify_event (GdkEventMotion * p0);
    bool on_focus_in_event (GdkEventFocus *);
    bool on_focus_out_event (GdkEventFocus *);

};          // class mainwid

/*
 * Free functions and values in the seq64 namespace.
 */

extern void update_mainwid_sequences ();

}           // namespace seq64

#endif      // SEQ64_MAINWID_HPP

/*
 * mainwid.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

