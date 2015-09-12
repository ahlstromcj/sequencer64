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
 * \updates       2015-09-11
 * \license       GNU GPLv2 or above
 *
 *  The "time" window is the horizontal bar at the upper right of the main
 *  window, just to the left of the "L" button.  This bar shows one or
 *  more black "pills" that show that the tune, and the measures, are
 *  progressing as playback occurs.
 */

#include "globals.h"
#include "maintime.hpp"

/**
 *  Static internal constants.
 */

static const int c_maintime_x = 300;
static const int c_maintime_y = 10;
static const int c_pill_width = 8;

/**
 *  This constructor sets up the colors black, white, and grey, and then
 *  allocates them.  In the constructor you can only allocate colors;
 *  get_window() would return 0 because the windows has not yet been
 *  realized.
 */

maintime::maintime()
 :
    Gtk::DrawingArea    (),
    m_gc                (),
    m_window            (),
    m_black             (Gdk::Color("black")),
    m_white             (Gdk::Color("white")),
    m_grey              (Gdk::Color("grey")),
    m_tick              (0)
{
    Glib::RefPtr<Gdk::Colormap> colormap = get_default_colormap();
    colormap->alloc_color(m_black);
    colormap->alloc_color(m_white);
    colormap->alloc_color(m_grey);
}

/**
 *  Handles realization of the window.  It performs the base class's
 *  on_realize() function.  It then allocates some additional resources: a
 *  window, a GC (?), and it clears the window.  Then it sets the default
 *  size of the window, specified by c_maintime_x and c_maintime_y.
 */

void
maintime::on_realize ()
{
    Gtk::DrawingArea::on_realize();     // we need to do the default realize
    m_window = get_window();
    m_gc = Gdk::GC::create(m_window);
    m_window->clear();
    set_size_request(c_maintime_x, c_maintime_y);
}

/**
 *  This function clears the window, sets the foreground to black, draws
 *  the "time" window's rectangle, and more.
 *
 *  Idle hands do the devil's work.  We need to figure at a high level
 *  what this routine draws, what a maintime is, and where it is located.
 */

int
maintime::idle_progress (long a_ticks)
{
    m_tick = a_ticks;
    m_window->clear();
    m_gc->set_foreground(m_black);
    m_window->draw_rectangle
    (
        m_gc, false, 0, 0, c_maintime_x - 1, c_maintime_y - 1
    );

    int width = c_maintime_x - 1 - c_pill_width;
    int tick_x = ((m_tick % c_ppqn) * (c_maintime_x - 1)) / c_ppqn ;
    int beat_x = (((m_tick / 4) % c_ppqn) * width) / c_ppqn ;
    int bar_x = (((m_tick / 16) % c_ppqn) * width) / c_ppqn ;
    if (tick_x <= (c_maintime_x / 4))
    {
        m_gc->set_foreground(m_grey);
        m_window->draw_rectangle
        (
            m_gc, true, 2, /*tick_x + 2,*/ 2, c_maintime_x - 4, c_maintime_y - 4
        );
    }
    m_gc->set_foreground(m_black);
    m_window->draw_rectangle
    (
        m_gc, true, beat_x + 2, 2, c_pill_width, c_maintime_y - 4
    );
    m_window->draw_rectangle
    (
        m_gc, true, bar_x + 2, 2, c_pill_width, c_maintime_y - 4
    );
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
 *  This function merely idles.
 */

bool
maintime::on_expose_event (GdkEventExpose * a_e)
{
    idle_progress(m_tick);
    return true;
}

/*
 * maintime.cpp
 *
 * vim: sw=4 ts=4 wm=8 et ft=cpp
 */
