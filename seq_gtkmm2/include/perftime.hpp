#ifndef SEQ64_PERFTIME_HPP
#define SEQ64_PERFTIME_HPP

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
 * \file          perftime.hpp
 *
 *  The time bar shows markers and numbers for the measures of the song,
 *  and also depicts the left and right markers.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2016-04-05
 * \license       GNU GPLv2 or above
 *
 */

#include "gui_drawingarea_gtk2.hpp"

namespace Gtk
{
    class Adjustment;
}

namespace seq64
{

class perform;
class perfedit;

/**
 *  This class implements drawing the piano time at the top of the
 *  "performance window" (the "song editor").
 */

class perftime : public gui_drawingarea_gtk2
{

    friend class perfedit;

private:

    /**
     *  Provides a link to the perfedit that created this object.  We want to
     *  support two perfedit windows, but the children of perfedit will have
     *  to communicate changes requiring a redraw through the parent.
     */

    perfedit & m_parent;

    /**
     *  Not yet sure exactly what this member represents.
     */

    int m_4bar_offset;

    /**
     *  This member is m_4bar_offset times 16 times the current PPQN,
     *  to save some calculations and centralize this value.
     */

    int m_tick_offset;

    /**
     *  The current value of PPQN, which we are trying to get to work
     *  everywhere, when PPQN is changed from the global ppqn = 192.
     */

    int m_ppqn;

    /**
     *  Snap value, starts out very small, equal to m_ppqn.
     */

    int m_snap;

    /**
     *  Provides the length of a measure in pulses or ticks.  This value is
     *  m_ppqn * 4, though eventually we want to employ a more flexible
     *  representation of measure length.  Supports perftime's keystroke
     *  processing.
     */

    int m_measure_length;

    /**
     *  Holds the current location of the left (L) marker when arrow movement
     *  is in force.  Otherwise it is -1.  Supports perftime's keystroke
     *  processing.
     */

    int m_left_marker_tick;

    /**
     *  Holds the current location of the right (R) marker when arrow movement
     *  is in force.  Otherwise it is -1.  Supports perftime's keystroke
     *  processing.
     */

    int m_right_marker_tick;

    /**
     *  A class version of the global c_perf_scale_x factor.
     */

    int m_perf_scale_x;                 /* no y version used, though */

    /**
     *  A class version of the global c_timerarea_y factor.
     */

    int m_timearea_y;                   /* no x version used, though */

public:

    perftime
    (
        perform & perf,
        perfedit & parent,
        Gtk::Adjustment & hadjust,
        int ppqn = SEQ64_USE_DEFAULT_PPQN
    );

    void reset ();
    void set_scale (int scale);
    void set_guides (int snap, int measure);

    /**
     *  This function does nothing.
     */

    void increment_size ()
    {
        // Empty body
    }

private:

    void enqueue_draw ();
    void zoom (int z);
    void draw_background ();
    void draw_progress_on_window ();
    void change_horz ();
    void set_ppqn (int ppqn);

    /**
     *  Common calculation to convert a pulse/tick value to a perftime x value.
     */

    long tick_to_pixel (midipulse tick)
    {
        return (tick - m_tick_offset) / m_perf_scale_x;
    }

    /**
     *  The inverse of tick_to_pixel().
     */

    midipulse pixel_to_tick (long pixel)
    {
        return pixel * m_perf_scale_x + m_tick_offset;
    }

    /**
     *  This function does nothing.
     */

    void update_sizes ()
    {
        // Empty body
    }

    /**
     *  This function just returns true.
     */

    int idle_progress ()
    {
        return true;
    }

    /**
     *  This function does nothing.
     */

    void update_pixmap ()
    {
        // Empty body
    }

    /**
     *  This function does nothing.
     */

    void draw_pixmap_on_window ()
    {
        // Empty body
    }

private:        // callbacks

    void on_realize ();
    bool on_expose_event (GdkEventExpose * ev);
    bool on_button_press_event (GdkEventButton * ev);
    void on_size_allocate (Gtk::Allocation & r);

    /**
     *  This button-release handler does nothing.
     */

    bool on_button_release_event (GdkEventButton * /*ev*/)
    {
        return false;
    }

    bool key_press_event (GdkEventKey * ev);    // perftime keys processing

};

}           // namespace seq64

#endif      // SEQ64_PERFTIME_HPP

/*
 * perftime.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

