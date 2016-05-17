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
 * \updates       2016-05-17
 * \license       GNU GPLv2 or above
 *
 */

#include "gui_drawingarea_gtk2.hpp"
#include "fruityperfroll_input.hpp"     /* FruityPerfInput      */
#include "perfroll_input.hpp"           /* Seq24PerfInput       */

namespace Gtk
{
    class Adjustment;
}

namespace seq64
{

class AbstractPerfInput;
class perform;
class perfedit;

/**
 *  This class implements the performance roll user interface.
 */

class perfroll : public gui_drawingarea_gtk2
{

    /**
     *  These friend implement interaction-specific behavior, although only
     *  the Seq24 interactions support keyboard processing, except for some
     *  common functionality provided by perform::perfroll_key_event().
     *  The perfedit class needs access to the private enqueue_draw()
     *  function.
     */

    friend class FruityPerfInput;
    friend class Seq24PerfInput;
    friend class perfedit;

private:

    /**
     *  Provides a link to the perfedit that created this object.  We want to
     *  support two perfedit windows, but the children of perfedit will have
     *  to communicate changes requiring a redraw through the parent.
     */

    perfedit & m_parent;

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

    int m_snap;
    int m_ppqn;
    int m_page_factor;              // 4096, provisional name
    int m_divs_per_beat;            // 16, provisional name
    int m_ticks_per_bar;            // m_ppqn * m_divs_per_beat, provisional name
    int m_perf_scale_x;

    /**
     *  New value to attempt a rudimentary time-zoom feature.
     */

    int m_zoom;

    int m_names_y;
    int m_background_x;
    int m_size_box_w;
    int m_size_box_click_w;
    int m_measure_length;
    int m_beat_length;
    midipulse m_old_progress_ticks;
    int m_4bar_offset;
    int m_sequence_offset;
    int m_roll_length_ticks;
    midipulse m_drop_tick;
    midipulse m_drop_tick_trigger_offset;
    int m_drop_sequence;
    int m_sequence_max;
    bool m_sequence_active[c_max_sequence];

    /**
     *  We need both styles of interaction object present.  Even if the user
     *  specifies the fruity interaction, the Seq24 interaction is still
     *  needed to handle our new keystroke support for the perfroll.  We need
     *  both objects to exist all the time, similar to the Fruity/Seq24 roles
     *  in the seqroll object.
     *
     * \obsolete
     *      AbstractPerfInput * m_interaction
     */

    FruityPerfInput m_fruity_interaction;

    /**
     *  Provides support for standard Seq24 mouse handling, plus the keystroke
     *  handlers.
     */

    Seq24PerfInput m_seq24_interaction;

    bool m_moving;
    bool m_growing;
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
    ~perfroll();

    void set_guides (int snap, int measure, int beat);
    void update_sizes ();
    void init_before_show ();
    void fill_background_pixmap ();
    void increment_size ();
    void draw_all ();                       /* used by perfroll_input   */

    /**
     *  Helper function to simplify the client call.
     */

    void redraw_progress ()
    {
        redraw_dirty_sequences ();
        draw_progress();
    }

private:

    void draw_progress ();                  /* called by perfedit       */
    void redraw_dirty_sequences ();         /* called by perfedit       */
    void set_ppqn (int ppqn);
    void convert_xy (int x, int y, midipulse & ticks, int & seq);
    void convert_x (int x, midipulse & ticks);
    void snap_x (int & x);
    void draw_sequence_on (int seqnum);
    void draw_background_on (int seqnum);
    void draw_drawable_row (long y);
    void change_horz ();
    void change_vert ();
    void split_trigger(int sequence, midipulse tick);
    void enqueue_draw ();
    void set_zoom (int z);

private:        // callbacks

    void on_realize ();
    bool on_expose_event (GdkEventExpose * ev);
    bool on_button_press_event (GdkEventButton * ev);
    bool on_button_release_event (GdkEventButton * ev);
    bool on_motion_notify_event (GdkEventMotion * ev);
    bool on_scroll_event (GdkEventScroll * ev) ;
    bool on_focus_in_event (GdkEventFocus * ev);
    bool on_focus_out_event (GdkEventFocus * ev);
    void on_size_allocate (Gtk::Allocation & al);
    bool on_key_press_event (GdkEventKey * ev);

    /**
     *  This do-nothing callback effectively throws away a size request.
     */

    void on_size_request (GtkRequisition *)
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

