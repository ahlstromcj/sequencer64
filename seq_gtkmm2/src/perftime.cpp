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
 * \updates       2015-11-26
 * \license       GNU GPLv2 or above
 *
 *  The time bar shows markers and numbers for the measures of the song,
 *  and also depicts the left and right markers.
 */

#include <gtkmm/adjustment.h>

#include "click.hpp"                    /* SEQ64_CLICK_LEFT() etc.      */
#include "font.hpp"
#include "keystroke.hpp"                /* for new keystroke actions    */
#include "perfedit.hpp"
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

perftime::perftime
(
    perform & p,
    perfedit & parent,
    Gtk::Adjustment & hadjust,
    int ppqn
) :
    gui_drawingarea_gtk2    (p, hadjust, adjustment_dummy(), 10, c_timearea_y),
    m_parent                (parent),
    m_4bar_offset           (0),
    m_tick_offset           (0),
    m_ppqn                  (0),                // set in the body
    m_snap                  (0),                // ditto
    m_measure_length        (0),                // tritto
#ifdef USE_PERFTIME_KEYSTROKE_PROCESSING
    m_left_marker_tick      (-1),
    m_right_marker_tick     (-1),
#endif
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
    set_ppqn(ppqn);
}

/**
 *  Handles changes to the PPQN value in one place.
 */

void
perftime::set_ppqn (int ppqn)
{
    if (ppqn_is_valid(ppqn))
    {
        m_ppqn = choose_ppqn(ppqn);
        m_snap = m_ppqn;
        m_measure_length = m_ppqn * 4;
        m_tick_offset = m_4bar_offset * 16 * m_ppqn;
    }
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
        m_tick_offset = m_4bar_offset * 16 * m_ppqn;
        enqueue_draw();
    }
}

/**
 *  Sets the m_snap value and the m_measure_length members directly from the
 *  function parameters, which are in units of pulses (sometimes misleadingly
 *  called "ticks".)
 *
 *  This function then fills in the background, and queues up a draw operation.
 *
 * \param snap
 *      Provides the number of snap-pulses (pulses per snap interval) as
 *      calculated in perfedit::set_guides().  This is actually equal to the
 *      measure-pulses divided by the snap value in perfedit; the snap value
 *      defaults to 8.
 *
 * \param measure
 *      Provides the number of measure-pulses (pulses per measure) as
 *      calculated in perfedit::set_guides().
 */

void
perftime::set_guides (int snap, int measure)
{
    m_snap = snap;
    m_measure_length = measure;
    enqueue_draw();
}

/**
 *  Wraps queue_draw() and forwards the call to the parent perfedit, so
 *  that it can forward it to any other perfedit that exists.
 *
 *  The parent perfedit will call perftime::queue_draw() on behalf of this
 *  object, and it will pass a perftime::enqueue_draw() to the peer perfedit's
 *  perftime, if the peer exists.
 */

void
perftime::enqueue_draw ()
{
    m_parent.enqueue_draw();
}

/**
 *  Implements the on-realization event, then allocates some resources the
 *  could not be allocated in the constructor.  It is important to call the
 *  base-class version of this function.
 *
 *  Done in base-class's on_realize() and in its constructor now.
 *
\verbatim
        m_window = get_window();
        m_gc = Gdk::GC::create(m_window);
        m_window->clear();
        set_size_request(10, m_timearea_y);
\endverbatim
 */

void
perftime::on_realize ()
{
    gui_drawingarea_gtk2::on_realize();     // base-class version
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
    draw_rectangle(white(), 0, 0, m_window_x, m_window_y);
    draw_line(black(), 0, m_window_y - 1, m_window_x, m_window_y - 1);
    long first_measure = m_tick_offset / m_measure_length;
    long last_measure = first_measure +
        (m_window_x * m_perf_scale_x / m_measure_length) + 1;

    m_gc->set_foreground(grey());                   /* draw vertical lines  */
    for (long i = first_measure; i < last_measure; ++i)
    {
        int x_pos = tick_to_pixel(i * m_measure_length);
        char bar[8];
        snprintf(bar, sizeof(bar), "%ld", i + 1);
        draw_line(x_pos, 0, x_pos, m_window_y);                     /* beat */
        render_string(x_pos + 2, 0, bar, font::BLACK);
    }

    long left = tick_to_pixel(perf().get_left_tick());
    long right = tick_to_pixel(perf().get_right_tick());
    if (left >= 0 && left <= m_window_x)            /* draw L marker    */
    {
        draw_rectangle(black(), left, m_window_y - 9, 7, 10);
        render_string(left + 1, 9, "L", font::WHITE);
    }
    if (right >= 0 && right <= m_window_x)          /* draw R marker    */
    {
        draw_rectangle(black(), right - 6, m_window_y - 9, 7, 10);
        render_string(right - 6 + 1, 9, "R", font::WHITE);
    }
    return true;
}

/**
 *  Implement the button-press event.
 */

bool
perftime::on_button_press_event (GdkEventButton * p0)
{
    midipulse tick = pixel_to_tick(long(p0->x));
    tick -= (tick % m_snap);

    // Why is this disabled?  We should re-enable and see if it works.
    //
    // if (SEQ64_CLICK_MIDDLE(p0->button))
    //      perf().set_start_tick(tick);

    if (SEQ64_CLICK_LEFT(p0->button))
        perf().set_left_tick(tick);
    else if (SEQ64_CLICK_RIGHT(p0->button))
        perf().set_right_tick(tick + m_snap);

    enqueue_draw();
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

#ifdef USE_PERFTIME_KEYSTROKE_PROCESSING

/*
 * Can't get the keystroke events to be seen by perfroll or perftime here
 * using the normal callback function for keystrokes, and not sure why.  This
 * code is currently enabled because the perfedit object can call this
 * function, and that call works.
 */

/**
 *  This callback function handles a key-press event.  However, we cannot get
 *  it to work directly, so the perfedit class, which does get keystrokes,
 *  calls this function to do the work
 *
 *  This function uses the "l" key to activate the movement of the "L" marker
 *  with the arrow keys, by the interval of on snap value for each press.  It
 *  also uses the "r" key to activate the movement of the "R" marker, and the
 *  "x" to deactivate either movement move.
 *
 *  Be aware that there is no visual feedback, as yet, that one is in the
 *  movement mode.
 *
 *  Also be aware the changing the name of this function from
 *  "key_press_event()" to "on_key_press_event()" will disrupt the process,
 *  causing keystrokes not get here.  Too tricky.
 */

bool
perftime::key_press_event (GdkEventKey * ev)
{
    bool result = false;
    {
        if (ev->keyval == SEQ64_l)
        {
            if (m_left_marker_tick == (-1))
            {
                m_right_marker_tick = (-1);
                m_left_marker_tick = perf().get_left_tick();
            }
        }
        else if (ev->keyval == SEQ64_r)
        {
            if (m_right_marker_tick == (-1))
            {
                m_left_marker_tick = (-1);
                m_right_marker_tick = perf().get_right_tick();
            }
        }
        else if (ev->keyval == SEQ64_x)             /* "x-scape" the modes  */
        {
            m_left_marker_tick = m_right_marker_tick = (-1);
        }
        else if (ev->keyval == SEQ64_Left)
        {
            if (m_left_marker_tick != (-1))
            {
                m_left_marker_tick -= m_snap;
                perf().set_left_tick(m_left_marker_tick);
                result = true;
            }
            else if (m_right_marker_tick != (-1))
            {
                m_right_marker_tick -= m_snap;
                perf().set_right_tick(m_right_marker_tick);
                result = true;
            }
        }
        else if (ev->keyval == SEQ64_Right)
        {
            if (m_left_marker_tick != (-1))
            {
                m_left_marker_tick += m_snap;
                perf().set_left_tick(m_left_marker_tick);
                result = true;
            }
            else if (m_right_marker_tick != (-1))
            {
                m_right_marker_tick += m_snap;
                perf().set_right_tick(m_right_marker_tick);
                result = true;
            }
        }
    }
    if (result)
        perf().modify();                                    /* flag it */

    enqueue_draw();
    return result;
}

#endif      // USE_PERFTIME_KEYSTROKE_PROCESSING

}           // namespace seq64

/*
 * perftime.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

