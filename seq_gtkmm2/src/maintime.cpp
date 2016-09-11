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
 * \updates       2016-09-10
 * \license       GNU GPLv2 or above
 *
 *  The "time" window is the horizontal bar at the upper right of the main
 *  window, just to the left of the "L" button.  This bar shows one or
 *  more black "pills" that show that the tune is progressing, both for beats
 *  and for measures. The beats are also shown as a flashing that occurs on
 *  each beat as playback occurs.
 *
 *  We've also added some member variables that represent calculations
 *  involving constants, which need be performed only once.  Save CPU time at
 *  the expense of a little memory.
 */

#include "globals.h"
#include "maintime.hpp"

/**
 *  Static internal constants.  These will eventually be replaced by variables
 *  in the user_settings module.
 */

static const int c_maintime_x = 300;
static const int c_maintime_y =  12;        /* was 10 */
static const int c_pill_width =  10;        /* was  8 */

namespace seq64
{

/**
 *  This constructor sets up the colors black, white, and grey, and then
 *  allocates them.  In the constructor you can only allocate colors;
 *  get_window() would return 0 because the windows has not yet been
 *  realized.
 */

maintime::maintime (perform & p, int ppqn)
 :
    gui_drawingarea_gtk2    (p, c_maintime_x, c_maintime_y),
    m_beat_width            (4),                                // TODO
    m_bar_width             (16),                               // TODO
    m_pill_width            (c_pill_width),
    m_box_width             (m_window_x - 1),
    m_box_height            (m_window_y - 1),
    m_flash_width           (m_window_x - 4),
    m_flash_height          (m_window_y - 4),
    m_flash_x               (m_window_x / m_beat_width),
    m_box_less_pill         (m_window_x - m_pill_width - 1),
    m_tick                  (0),
    m_ppqn                  (choose_ppqn(ppqn))
{
    // No other code
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
     * Odd, we now notice that the pill bar isn't showing until the tune
     * starts to play.  Let's make sure it appears.  Doesn't work.  Something
     * else fixed it, and we forgot to document it.
     *
     * (void) idle_progress(0);
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
maintime::idle_progress (midipulse ticks)
{
    if (ticks >= 0)                     /* ca 2016-03-17 to make bar appear */
    {
        const int yoff = 4;
        int tick_x = (ticks % m_ppqn) * m_box_width / m_ppqn;
        int beat_x = ((ticks / m_beat_width) % m_ppqn) * m_box_less_pill / m_ppqn;
        int bar_x  = ((ticks / m_bar_width)  % m_ppqn) * m_box_less_pill / m_ppqn;
        m_tick = ticks;
        clear_window();
        draw_rectangle(black(), 0, yoff, m_box_width, m_box_height, false);
        if (tick_x <= m_flash_x)       /* for flashing the maintime bar     */
            draw_rectangle(grey(), 2, yoff+2, m_flash_width, m_flash_height);

        draw_rectangle(black(), beat_x + 2, yoff+2, m_pill_width, m_flash_height);
        draw_rectangle(bar_x + 2, yoff+2, m_pill_width, m_flash_height);
    }
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
 *
 *  Actually, it might be useful after all, to avoid flickering under JACK
 *  transport.  Let's put it back for now.  (It doesn't help, but we will
 *  leave it in, the overhead is small.)
 */

bool
maintime::on_expose_event (GdkEventExpose * a_e)
{
    idle_progress(m_tick);               /* idle_progress(0); */
    return true;
}

}           // namespace seq64

/*
 * maintime.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

