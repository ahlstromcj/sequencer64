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
 * \updates       2018-08-11
 * \license       GNU GPLv2 or above
 *
 *  This class represents the central piano-roll user-interface area of the
 *  performance/song editor.
 */

#include "globals.h"                    /* seq64::c_max_sequence            */
#include "gui_drawingarea_gtk2.hpp"     /* seq64::gui_drawingarea_gtk2      */
#include "rect.hpp"                     /* seq64::rect class                */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace Gtk
{
    class Adjustment;
}

/**
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{
    class perform;
    class perfedit;

/**
 *  This class implements the performance roll user interface.
 */

class perfroll : public gui_drawingarea_gtk2
{
    /**
     *  These friend implement interaction-specific behavior, although only
     *  the Seq24 interactions support full keyboard processing, except for
     *  some common functionality provided by perform::perfroll_key_event().
     *  The perfedit class needs access to the private enqueue_draw()
     *  function.
     */

    friend class perfedit;

protected:

    /**
     *  Static sizing members for initial zoom of the perfroll.
     */

    static int sm_perfroll_size_box_w;
    static int sm_perfroll_background_x;
    static int sm_perfroll_size_box_click_w;

    /**
     *  Provides a link to the perfedit that created this object.  We want to
     *  support two perfedit windows, but the children of perfedit will have
     *  to communicate changes requiring a redraw through the parent.
     */

    perfedit & m_parent;

    /**
     *  Indicates we are in the middle of adding a sequence segment to the
     *  performance.  Moved from AbstractPerfInput.
     */

    bool m_adding;

    /**
     *  Indicates if the left mouse button is pressed while in adding mode.
     *  Moved from AbstractPerfInput.
     */

    bool m_adding_pressed;

    /**
     *  Provides the horizontal page increment for the horizontal scrollbar.
     *  It was set to 1, the same as the step increment.  That is too little.
     *  This value will be set to 4, for now.  Might be a useful "user"
     *  configuration option.
     */

    int m_h_page_increment;

    /**
     *  Provides the vertical page increment for the vertical scrollbar.  It
     *  was set to 1, the same as the step increment.  That is too little.
     *  This value will be set to 8, for now.  Might be a useful "user"
     *  configuration option.
     */

    int m_v_page_increment;

    int m_snap_x;                   /**< Amount of horizontal snap, pulses. */
    int m_snap_y;                   /**< The amount of vertical snap.       */
    int m_ppqn;                     /**< Parts-per-quarter-note value.      */
    int m_page_factor;              /**< 4096, horizonal page sizing.       */
    int m_divs_per_beat;            /**< Holds current tick scaling value.  */
    midipulse m_ticks_per_bar;      /**< Holds current bar scaling value.   */
    int m_perf_scale_x;             /**< Scaling based on zoom and PPQN.    */
    int m_w_scale_x;                /**< Scaling based on zoom and PPQN.    */

    /**
     *  New value to attempt a rudimentary time-zoom feature.  It seems to
     *  work pretty well now.
     */

    int m_zoom;

    /**
     *  The maximum height of the perfroll names box, in pixels.  This is
     *  currently semantically a constant set to c_names_y = 24.
     */

    int m_names_y;

    /**
     *  The width of the perfroll background.  This is based on the m_ppqn
     *  value and the value of c_perf_scale_x (or is m_perf_scale_x preferable?)
     */

    int m_background_x;

    /**
     *  This is a basically constant value set to s_perfroll_size_box_w = 3.
     *  It is used in drawing the short lines of the small box that sits at
     *  the top-left and bottom-right corners of each segment in the pattern
     *  editor.  These can be used to lengthen and shorten a section in the
     *  song editor.  We will increase this size, perhaps double it, to make
     *  it easier to grab.
     */

    int m_size_box_w;

    /**
     *  The legnth of a measure, in beat units.
     */

    int m_measure_length;

    /**
     *  The length of a beat, in parts-per-quarter note.
     */

    int m_beat_length;

    /**
     *  Saves the position of the progress bar, for erasing it in preparation
     *  for drawing it at the next tick value.  See the draw_progress()
     *  function.  This could almost be static inside that function.
     */

    midipulse m_old_progress_ticks;

    /**
     *  Provides the current scroll page in which the progress bar resides.
     */

    int m_scroll_page;

    /**
     *  Used in the fruity and seq24 perfroll input classes to help with
     *  trigger push/pop management.
     */

    bool m_have_button_press;

#ifdef USE_UNNECESSARY_TRANSPORT_FOLLOW_CALLBACK

    /**
     *  Indicates that the application should follow JACK transport.
     *  The alternative is ...?
     */

    bool m_transport_follow;

    /**
     *  Indicates if the follow-transport button is pressed.
     */

    bool m_trans_button_press;

#endif

    /**
     *  Holds the horizontal offset related to the horizontal scroll-bar
     *  position.  Used in drawing the progress bar and the sequence events.
     *  Also used in convert_x() and convert_xy().  This used to be the offset
     *  in units of bar ticks, but now we use it as a full-fledged ticks
     *  value.  See the change_horz() function.
     */

    midipulse m_4bar_offset;

    /**
     *  This value is the vertical version of m_4bar_offset.  It is obtained
     *  or changed when the vertical scroll-bar moves.  It is used for drawing
     *  the correct vertical window in the piano roll.
     */

    int m_sequence_offset;

    /**
     *  Provides the width of the piano roll in ticks.  Calculated in
     *  init_before_show() based on the maximum trigger found in the perform
     *  object, the ticks/bar, the PPQN, and the page factor.  Also can be
     *  increased in size in the increment_size() function [tied to the Grow
     *  button].  Used in update_sizes().
     */

    int m_roll_length_ticks;

    /**
     *  The horizontal location for section movement.  Used only by the
     *  friend modules perfroll_input and fruityperfroll_input.
     */

    midipulse m_drop_tick;

    /**
     *  The horizontal trigger location for section movement.  Used only by
     *  the modules perfroll_input and fruityperfroll_input.
     */

    midipulse m_drop_tick_offset;

    /**
     *  Holds the currently-selected sequence being moved.  Used for redrawing
     *  the sequence.
     *
     * Extension?
     *
     *  We would like to extend this to a list of sequencens so that we could
     *  move more than one sequence at once.
     */

    int m_drop_sequence;

    /**
     *  Currently, just a class-specific version of c_max_sequence, meant for
     *  the future.
     */

    int m_sequence_max;

    /**
     *  Used when drawing an active sequence.  Not sure yet why we can't just
     *  use the sequence's member function to access this status boolean.
     */

    bool m_sequence_active[c_max_sequence];

#ifdef SEQ64_SONG_BOX_SELECT

    /**
     *  The previous selection rectangle, used for undrawing it.  Not yet
     *  ready, first starting Shift-Select methods.
     */

    rect m_old;

    /**
     *  The previous selection rectangle, used for undrawing it.
     */

    rect m_selected;

    /**
     *  Set to true if the song editor is in box-selection mode.
     */

    bool m_box_select;

    /**
     *  The lower sequence number for the box-select mode.
     */

    int m_box_select_low;

    /**
     *  The upper sequence number for the box-select mode.
     */

    int m_box_select_high;

    /**
     *
     */

    midipulse m_last_tick;

    /**
     *  The horizontal value of the scroll window in units of pixels.
     */

    int m_scroll_offset_x;

    /**
     *  The vertical value of the scroll window in units of pixels.
     */

    int m_scroll_offset_y;

#endif  // SEQ64_SONG_BOX_SELECT

    /**
     *  Used in the Seq24 or Fruity processing when moving a section of
     *  triggers.
     */

    bool m_moving;

    /**
     *  Used in the Seq24 or Fruity processing when growing a section of
     *  triggers.
     */

    bool m_growing;

    /**
     *  Used in the Seq24 or Fruity processing when growing a section of
     *  triggers.  Determines whether the section is growing to the left or to
     *  the right.
     */

    bool m_grow_direction;

public:

    perfroll
    (
        perform & perf,
        perfedit & parent,
        Gtk::Adjustment & hadjust,
        Gtk::Adjustment & vadjust,
        int ppqn = SEQ64_USE_DEFAULT_PPQN
    );
    virtual ~perfroll();

    void set_guides (int snap, int measure, int beat);
    void update_sizes ();
    void init_before_show ();
    void fill_background_pixmap ();
    void increment_size ();
    void draw_all ();                       /* used by perfroll_input   */
    void follow_progress ();

    /**
     *  Helper function to simplify the client call.
     */

    void redraw_progress ()
    {
        redraw_dirty_sequences ();
        draw_progress();
    }

protected:

    void draw_progress ();                  /* called by perfedit       */
    void redraw_dirty_sequences ();         /* called by perfedit       */
    void set_ppqn (int ppqn);
    void convert_xy (int x, int y, midipulse & tick, int & seq);
    void convert_x (int x, midipulse & tick);
    void snap_x (int & x);
    void snap_y (int & y);
    void draw_sequence_on (int seqnum);         /* perform::SeqOperation    */
    void draw_background_on (int seqnum);
    void draw_drawable_row (int y);

#ifdef SEQ64_SONG_BOX_SELECT

    void draw_selection_on_window ();

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
     * \getter m_selecting
     */

    bool selecting () const
    {
        return m_box_select;
    }

    /**
     * \setter m_selecting
     */

    void selecting (bool flag)
    {
        m_box_select = flag;
    }

    /**
     *  Indicates if we're selecting or moving.
     *
     * \return
     *      Returns true if one of those four flags are set.
     */

    bool select_action () const
    {
        return selecting() || drop_action();
    }

#endif  // SEQ64_SONG_BOX_SELECT

    /**
     *  To be used in iterating through a set.
     */

    void draw_sequence (int seqnum)
    {
        draw_background_on(seqnum);
        draw_sequence_on(seqnum);
    }

    void offset_sequence (int seqnum, midipulse offset);
    void change_horz ();

#ifdef USE_STAZED_PERF_AUTO_SCROLL
    void auto_scroll_horz ();
#endif

    void change_vert ();
    void split_trigger(int sequence, midipulse tick);
    void enqueue_draw ();
    void set_zoom (int z);

    /**
     *  A convenience function.
     */

    void convert_drop_xy ()
    {
        convert_xy(m_drop_x, m_drop_y, m_drop_tick, m_drop_sequence);
    }

    /**
     *  This function provides optimization for the on_scroll_event() function.
     *  A duplicate of the one in seqroll.
     *
     * \param step
     *      Provides the step value to use for adjusting the horizontal
     *      scrollbar.  See gui_drawingarea_gtk2::scroll_hadjust() for more
     *      information.
     */

    void horizontal_adjust (double step)
    {
        scroll_hadjust(m_hadjust, step);
    }

    /**
     *  This function provides optimization for the on_scroll_event() function.
     *  A near-duplicate of the one in seqroll.
     *
     * \param step
     *      Provides the step value to use for adjusting the vertical
     *      scrollbar.  See gui_drawingarea_gtk2::scroll_vadjust() for more
     *      information.
     */

    void vertical_adjust (double step)
    {
        scroll_vadjust(m_vadjust, step);
    }

    /**
     *  Sets the exact position of a horizontal scroll-bar.
     *
     * \param value
     *      The desired position.  Mostly this is either 0.0 or 9999999.0 (an
     *      "infinite" value to select the start or end position.
     */

    void horizontal_set (double value)
    {
        scroll_hset(m_hadjust, value);
    }

    /**
     *  Sets the exact position of a vertical scroll-bar.
     *
     * \param value
     *      The desired position.  Mostly this is either 0.0 or 9999999.0 (an
     *      "infinite" value to select the start or end position.
     */

    void vertical_set (double value)
    {
        scroll_vset(m_vadjust, value);
    }

protected:

    virtual void activate_adding (bool adding) = 0;
    virtual bool handle_motion_key (bool is_left) = 0;

    /**
     * \getter m_adding
     */

    bool is_adding () const
    {
        return m_adding;
    }

    /**
     * \setter m_adding
     */

    void set_adding (bool flag)
    {
        m_adding = flag;
    }

    /**
     * \getter m_adding_pressed
     */

    bool is_adding_pressed () const
    {
        return m_adding_pressed;
    }

    /**
     * \setter m_adding_pressed
     */

    void set_adding_pressed (bool flag)
    {
        m_adding_pressed = flag;
    }

    /**
     * \getter m_growing
     */

    bool growing () const
    {
        return m_growing;
    }

    /**
     * \getter m_moving
     */

    bool moving () const
    {
        return m_moving;
    }

    /**
     *  Indicates if we're moving.
     *
     * \return
     *      Returns true if one of those two flags are set.
     */

    bool drop_action () const
    {
        return moving();
    }

protected:        // callbacks

    virtual void on_realize ();
    virtual bool on_expose_event (GdkEventExpose * ev);
    virtual bool on_button_press_event (GdkEventButton * ev);
    virtual bool on_button_release_event (GdkEventButton * ev);
    virtual bool on_motion_notify_event (GdkEventMotion * ev);
    virtual bool on_scroll_event (GdkEventScroll * ev) ;
    virtual bool on_focus_in_event (GdkEventFocus * ev);
    virtual bool on_focus_out_event (GdkEventFocus * ev);
    virtual void on_size_allocate (Gtk::Allocation & al);
    virtual bool on_key_press_event (GdkEventKey * ev);

    /**
     *  This do-nothing callback effectively throws away a size request.
     */

    virtual void on_size_request (GtkRequisition *)
    {
        // Empty body
    }

};          // class perfroll

}           // namespace seq64

#endif      // SEQ64_PERFROLL_HPP

/*
 * perfroll.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

