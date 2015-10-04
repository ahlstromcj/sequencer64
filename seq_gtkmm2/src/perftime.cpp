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
 * \file          perftime.cpp
 *
 *  This module declares/defines the base class for the time or measures
 *  area at the top of the performance window.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2015-10-04
 * \license       GNU GPLv2 or above
 *
 *  The time bar shows markers and numbers for the measures of the song,
 *  and also depicts the left and right markers.
 */

#include <gtkmm/adjustment.h>

#include "click.hpp"                    /* CLICK_IS_LEFT() etc. */
#include "font.hpp"
#include "perform.hpp"
#include "perftime.hpp"

namespace seq64
{

/**
 *  Principal constructor.
 *  In the constructor you can only allocate colors;
 *  get_window() returns 0 because we have not been realized.
 *
 * \note
 *      Note that we still have to use a global constant in the base-class
 *      constructor; we cannot assign it to the corresponding member
 *      beforehand.
 */

perftime::perftime (perform & p, Gtk::Adjustment & hadjust)
 :
    gui_drawingarea_gtk2    (p, hadjust, adjustment_dummy(), 10, c_timearea_y),
    m_4bar_offset           (0),
    m_ppqn                  (c_ppqn),
    m_snap                  (c_ppqn),
    m_measure_length        (c_ppqn * 4),
    m_perf_scale_x          (c_perf_scale_x),   // 32 ticks per pixel
    m_timearea_y            (c_timearea_y)      // pixel-height of time scale
{
    /*
     * This adds many fewer events than the base class.  Any bad effects?
     *
     * add_events(Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK);
     * set_double_buffered(false);
     */

    m_hadjust.signal_value_changed().connect
    (
        mem_fun(*this, &perftime::change_horz)
    );
}

/**
 *  Change the m_4bar_offset and queue a draw operation.
 */

void
perftime::change_horz ()
{
    if (m_4bar_offset != int(m_hadjust.get_value()))
    {
        m_4bar_offset = int(m_hadjust.get_value());
        queue_draw();
    }
}

/**
 *  Sets the snap value and the measure-length members.
 */

void
perftime::set_guides (int snap, int measure)
{
    m_snap = snap;
    m_measure_length = measure;
    queue_draw();
}

/**
 *  Implements the on-realization event, then allocates some resources the
 *  could not be allocated in the constructor.  It is important to call the
 *  base-class version of this function.
 */

void
perftime::on_realize ()
{
    gui_drawingarea_gtk2::on_realize();     // base-class version

    /*
     * Done in base-class's on_realize() and in its constructor now.
     *
     *  m_window = get_window();
     *  m_gc = Gdk::GC::create(m_window);
     *  m_window->clear();
     *  set_size_request(10, m_timearea_y);
     */
}

/**
 *  Implements the on-expose event.
 *
 * \note
 *      The perfedit object is created early on.  When brought on-screen from
 *      mainwnd (the main window), first, perftime::on_realize() is called,
 *      then this event is called.
 *
 *      It crashes trying to set the foreground color.
 */

bool
perftime::on_expose_event (GdkEventExpose * /* ev */ )
{
    m_gc->set_foreground(white());                  /* clear the background */
    m_window->draw_rectangle(m_gc, true, 0, 0, m_window_x, m_window_y);
    m_gc->set_foreground(black());
    m_window->draw_line(m_gc, 0, m_window_y - 1, m_window_x, m_window_y - 1);
    m_gc->set_foreground(grey());                   /* draw vertical lines  */
    long tick_offset = (m_4bar_offset * 16 * m_ppqn);
    long first_measure = tick_offset / m_measure_length;

#if 0
    0   1   2   3   4   5   6
    |   |   |   |   |   |   |
    |    |    |    |    |    |
    0    1    2    3    4    5
#endif

    long last_measure = first_measure +
        (m_window_x * m_perf_scale_x / m_measure_length) + 1;

    for (long i = first_measure; i < last_measure; ++i)
    {
        int x_pos = ((i * m_measure_length) - tick_offset) / m_perf_scale_x;
        m_window->draw_line(m_gc, x_pos, 0, x_pos, m_window_y);     /* beat */

        char bar[8];
        snprintf(bar, sizeof(bar), "%ld", i + 1);
        m_gc->set_foreground(black());
        p_font_renderer->render_string_on_drawable
        (
            m_gc, x_pos + 2, 0, m_window, bar, font::BLACK
        );
    }

    long left = perf().get_left_tick();
    long right = perf().get_right_tick();
    left -= (m_4bar_offset * 16 * m_ppqn);          /* why 16?          */
    left /= m_perf_scale_x;
    right -= (m_4bar_offset * 16 * m_ppqn);
    right /= m_perf_scale_x;
    if (left >= 0 && left <= m_window_x)            /* draw L marker    */
    {
        m_gc->set_foreground(black());
        m_window->draw_rectangle(m_gc, true, left, m_window_y - 9, 7, 10);
        m_gc->set_foreground(white());
        p_font_renderer->render_string_on_drawable
        (
            m_gc, left + 1, 9, m_window, "L", font::WHITE
        );
    }
    if (right >= 0 && right <= m_window_x)          /* draw R marker    */
    {
        m_gc->set_foreground(black());
        m_window->draw_rectangle(m_gc, true, right - 6, m_window_y - 9, 7, 10);
        m_gc->set_foreground(white());
        p_font_renderer->render_string_on_drawable
        (
            m_gc, right - 6 + 1, 9, m_window, "R", font::WHITE
        );
    }
    return true;
}

/**
 *  Implement the button-press event.
 */

bool
perftime::on_button_press_event (GdkEventButton * p0)
{
    long tick = long(p0->x);
    tick *= m_perf_scale_x;         // tick = tick - (tick % (m_ppqn * 4));
    tick += (m_4bar_offset * 16 * m_ppqn);
    tick -= (tick % m_snap);

    // Why is this disabled?
    //
    // if (CLICK_IS_MIDDLE(p0->button))
    //      perf().set_start_tick(tick);

    if (CLICK_IS_LEFT(p0->button))
        perf().set_left_tick(tick);
    else if (CLICK_IS_RIGHT(p0->button))
        perf().set_right_tick(tick + m_snap);

    queue_draw();
    return true;
}

/**
 *  Implements a size-allocation event.
 */

void
perftime::on_size_allocate (Gtk::Allocation & a_r)
{
    Gtk::DrawingArea::on_size_allocate(a_r);
    m_window_x = a_r.get_width();
    m_window_y = a_r.get_height();
}

}           // namespace seq64

/*
 * perftime.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
