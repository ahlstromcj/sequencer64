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
 * \updates       2015-11-05
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

/**
 *  This class implements drawing the piano time at the top of the
 *  "performance window" (the "song editor").
 */

class perftime : public gui_drawingarea_gtk2
{

private:

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
     *  everywhere, when PPQN is changed from global_ppqn = 192.
     */

    int m_ppqn;

    /**
     *  Snap value, starts out very small, equal to m_ppqn.
     */

    int m_snap;

    int m_measure_length;
    int m_perf_scale_x;                 /* no y version used, though */
    int m_timearea_y;                   /* no x version used, though */

public:

    perftime
    (
        perform & perf,
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

    void draw_progress_on_window ();
    void change_horz ();
    void set_ppqn (int ppqn);

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

};

}           // namespace seq64

#endif      // SEQ64_PERFTIME_HPP

/*
 * perftime.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
