#ifndef SEQ64_MAINTIME_HPP
#define SEQ64_MAINTIME_HPP

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
 * \file          maintime.hpp
 *
 *  This module declares/defines the base class for the "time" progress
 *  window.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2015-11-07
 * \license       GNU GPLv2 or above
 *
 */

#include "gui_drawingarea_gtk2.hpp"

namespace seq64
{

/**
 *  This class provides the drawing of the progress bar at the top of the
 *  main window, along with two "pills" that move in time with the
 *  beat and measure.  We added a lot of members to hold the results of
 *  calculations that involve what are essentially constant.  This saves CPU
 *  time, and maybe a little memory for the code to make those calculations
 *  more than once.
 */

class maintime : public gui_drawingarea_gtk2
{

    friend class mainwnd;               /* it calls idle_progress()     */

private:

    /**
     *  Provides the divisor for ticks to produce a beat value.  Currently,
     *  this value is hardwired to 4, but will eventually be wired up as
     *  usr().midi_beat_width().
     */

    const int m_beat_width;

    /**
     *  Provides the divisor for ticks to produce a bar value.  Currently,
     *  this value is hardwired to 16, but will eventually be wired up as
     *  usr().midi_beat_width() * usr().midi_beats_per_bar().
     */

    const int m_bar_width;

    /**
     *  Provides the width of the pills, little black squares that show the
     *  progress of a beat and a bar (measure).
     */

    const int m_pill_width;

    /**
     *  The width/length  of the rectangle to be drawn inside the maintime
     *  window.  This item absolutely depends on the main window being
     *  non-resizable.
     */

    const int m_box_width;

    /**
     *  The height of the rectangle to be drawn inside the maintime window.
     *  This item absolutely depends on the main window being non-resizable.
     */

    const int m_box_height;

    /**
     *  The width/length of the flashing rectangle to be drawn inside the
     *  maintime window.  Just a bit smaller than m_box_width.
     */

    const int m_flash_width;

    /**
     *  The height of the flashing rectangle to be drawn inside the maintime
     *  window.  Just a bit smaller than m_box_width.
     */

    const int m_flash_height;

    /**
     *  The x value at which a flash should occur.
     */

    const int m_flash_x;

    /**
     *  The width/length of the maintime window minus the width of the pill.
     */

    const int m_box_less_pill;

    /**
     *  Provides the active PPQN value.  While this is effectively a constant
     *  for the duration of a tune, it might change as different tunes are
     *  loaded.
     */

    int m_ppqn;

private:

    maintime (const maintime &);
    maintime & operator = (const maintime &);

public:

    maintime
    (
        perform & p,
        int ppqn = SEQ64_USE_DEFAULT_PPQN
    );

private:

    int idle_progress (long ticks);

private:        // callbacks

    void on_realize ();
    bool on_expose_event (GdkEventExpose * ev);

};

}           // namespace seq64

#endif      // SEQ64_MAINTIME_HPP

/*
 * maintime.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

