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
 * \file          maintime.cpp
 *
 *  This module declares/defines the base class for the "time" progress
 *  window.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2015-10-29
 * \license       GNU GPLv2 or above
 *
 *  The "time" window is the horizontal bar at the upper right of the main
 *  window, just to the left of the "L" button.  This bar shows one or
 *  more black "pills" that show that the tune, and the measures, are
 *  progressing as playback occurs.
 */

#include "globals.h"
#include "maintime.hpp"

namespace seq64
{

/**
 *  This constructor sets up the colors black, white, and grey, and then
 *  allocates them.  In the constructor you can only allocate colors;
 *  get_window() would return 0 because the windows has not yet been
 *  realized.
 */

maintime::maintime
(
    perform & p,
    int ppqn,
    int pillwidth,
    int x,
    int y
) :
    gui_drawingarea_gtk2    (p, x, y),
    m_pill_width            (pillwidth),
    m_ppqn                  (0)
{
    m_ppqn = choose_ppqn(ppqn);
}

/**
 *  Handles realization of the window.  It performs the base class's
 *  on_realize() function.  It then allocates some additional resources: a
 *  window, a GC (?), and it clears the window.  Then it sets the default
 *  size of the window, specified by GUI constructor parameters.
 */

void
maintime::on_realize ()
{
    gui_drawingarea_gtk2::on_realize();

    /*
     * Handled in the base-class constructor.
     *
     * set_size_request(c_maintime_x, c_maintime_y);
     */
}

/**
 *  This function clears the window, sets the foreground to black, draws
 *  the "time" window's rectangle, and then draws a rectangle for noting the
 *  progress of the beat, and the progress for a bar.
 *
 *  Idle hands do the devil's work.  We should eventually support some generic
 *  coloring for "dark themes".  The default coloring is better for "light
 *  themes".
 *
 * \param ticks
 *      Provides the main tick setting.  This setting is provided by
 *      mainwnd(), in its timer callback.
 *
 * \return
 *      Always returns 1 (it used to return "true"!).
 */

int
maintime::idle_progress (long ticks)
{
    m_window->clear();
    draw_rectangle(black(), 0, 0, m_window_x - 1, m_window_y - 1, false);

    int width = m_window_x - m_pill_width - 1;
    int tick_x = ((ticks % m_ppqn) * (m_window_x - 1)) / m_ppqn ;
    int beat_x = (((ticks / 4) % m_ppqn) * width) / m_ppqn ;
    int bar_x = (((ticks / 16) % m_ppqn) * width) / m_ppqn ;
    if (tick_x <= (m_window_x / 4))         /* 4 is number of beats, bw! */
    {
        /*
         * This rectangle gives the maintime bar a flashing effect, four times
         * per measure.
         */

        draw_rectangle
        (
            grey(), 2, /*tick_x + 2,*/ 2, m_window_x - 4, m_window_y - 4
        );
    }
    draw_rectangle(black(), beat_x + 2, 2, m_pill_width, m_window_y - 4);
    draw_rectangle(bar_x + 2, 2, m_pill_width, m_window_y - 4);
    return true;
}

/*
 * ca 2015-07-25
 * Eliminate this annoying warning.  Will do it for Microsoft's bloddy
 * compiler later.
 */

#ifdef PLATFORM_GNU
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

/**
 *  This function merely idles.  We don't need the m_tick member, the function
 *  works as well if 0 is passed in.  We've removed m_tick permanently.
 */

bool
maintime::on_expose_event (GdkEventExpose * a_e)
{
    idle_progress(0);               /* idle_progress(m_tick); */
    return true;
}

}           // namespace seq64

/*
 * maintime.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

