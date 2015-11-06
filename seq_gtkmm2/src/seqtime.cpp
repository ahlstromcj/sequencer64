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
 * \file          seqtime.cpp
 *
 *  This module declares/defines the base class for drawing the
 *  time/measures bar at the top of the patterns/sequence editor.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2015-11-04
 * \license       GNU GPLv2 or above
 *
 *  The patterns/sequence editor is expandable in both directions, but the
 *  time/measures bar does not change in size.
 */

#include <gtkmm/adjustment.h>

#include "event.hpp"
#include "font.hpp"
#include "seqtime.hpp"
#include "sequence.hpp"

namespace seq64
{

/**
 *  Principal constructor.  In the constructor you can only allocate
 *  colors; get_window() returns 0 because the window is not yet realized>
 */

seqtime::seqtime
(
    sequence & seq,
    perform & p,
    int zoom,
    Gtk::Adjustment & hadjust,
    int ppqn
) :
    gui_drawingarea_gtk2    (p, hadjust, adjustment_dummy(), 10, c_timearea_y),
    m_seq                   (seq),
    m_scroll_offset_ticks   (0),
    m_scroll_offset_x       (0),
    m_zoom                  (zoom),
    m_ppqn                  (0)
{
    m_ppqn = choose_ppqn(ppqn);
}

/**
 *  Updates the pixmap to a new size and queues up a draw operation.
 */

void
seqtime::update_sizes ()
{
    if (is_realized())                                  /* set this for later */
    {
        m_pixmap = Gdk::Pixmap::create(m_window, m_window_x, m_window_y, -1);
        update_pixmap();
        queue_draw();
    }
}

/**
 *  Changes the scrolling horizontal offset, updates the pixmap, and
 *  forces a redraw.
 */

void
seqtime::change_horz ()
{
    m_scroll_offset_ticks = int(m_hadjust.get_value());
    m_scroll_offset_x = m_scroll_offset_ticks / m_zoom;
    update_pixmap();
    force_draw();
}

/**
 *  Sets the scroll offset tick and x values, updates the sizes and the
 *  pixmap, and resets the window.
 */

void
seqtime::reset ()
{
    m_scroll_offset_ticks = int(m_hadjust.get_value());
    m_scroll_offset_x = m_scroll_offset_ticks / m_zoom;
    update_sizes();
    update_pixmap();
    draw_pixmap_on_window();
}

/**
 *  Very similar to the reset() function, except it doesn't update the
 *  sizes.
 */

void
seqtime::redraw ()
{
    m_scroll_offset_ticks = int(m_hadjust.get_value());
    m_scroll_offset_x = m_scroll_offset_ticks / m_zoom;
    update_pixmap();
    draw_pixmap_on_window();
}

/**
 *  Updates the pixmap.
 *
 *  When the zoom is at 32, there is a bar for every measure.
 *  At 16, ???
 *
 \verbatim
        zoom   32         16         8        4        1
        ml
        m_ppqn
        *
        1      128
        2      64
        4      32        16         8
        8      16m       8          4          2       1
        16     8m        4          2          1       1
        32     4m        2          1          1       1
        64     2m        1          1          1       1
        128    1m        1          1          1       1
\ endverbatim
 */

void
seqtime::update_pixmap ()
{
    draw_rectangle_on_pixmap(white(), 0, 0, m_window_x, m_window_y);
    draw_line_on_pixmap(black(), 0, m_window_y - 1, m_window_x, m_window_y - 1);

    /*
     * See the description in the banner.
     */

    int measure_length_32nds = m_seq.get_beats_per_bar() * 32 /
        m_seq.get_beat_width();

    int measures_per_line = (128 / measure_length_32nds) / (32 / m_zoom);
    if (measures_per_line <= 0)
        measures_per_line = 1;

    int ticks_per_measure =  m_seq.get_beats_per_bar() * (4 * m_ppqn) /
        m_seq.get_beat_width();

    int ticks_per_step = ticks_per_measure * measures_per_line;
    int start_tick = m_scroll_offset_ticks -
        (m_scroll_offset_ticks % ticks_per_step);

    int end_tick = (m_window_x * m_zoom) + m_scroll_offset_ticks;
    m_gc->set_foreground(black());                      /* draw vert lines  */
    for (int i = start_tick; i < end_tick; i += ticks_per_step)
    {
        int base_line = i / m_zoom;
        draw_line_on_pixmap                             /* draw the beat    */
        (
            base_line - m_scroll_offset_x,
            0, base_line - m_scroll_offset_x, m_window_y
        );

        char bar[8];
        snprintf(bar, sizeof(bar), "%d", (i / ticks_per_measure) + 1);
        render_string_on_pixmap
        (
            base_line + 2 - m_scroll_offset_x, 0, bar, font::BLACK
        );
    }

    long end_x = m_seq.get_length() / m_zoom - m_scroll_offset_x;
    draw_rectangle_on_pixmap(black(), end_x, 9, 19, 8);
    render_string_on_pixmap(end_x + 1, 9, "END", font::WHITE);
}

/**
 *  Draws the pixmap on the window.
 */

void
seqtime::draw_pixmap_on_window ()
{
    draw_drawable(0, 0, 0, 0, m_window_x, m_window_y);
}

/**
 *  Same as draw_pixmap_on_window().
 */

void
seqtime::force_draw ()
{
    draw_drawable(0, 0, 0, 0, m_window_x, m_window_y);
}

/**
 *  Called when the window is drawn.  Call the base-class version of this
 *  function first.  Then addition resources are allocated.
 */

void
seqtime::on_realize()
{
    gui_drawingarea_gtk2::on_realize();
    Glib::signal_timeout().connect(mem_fun(*this, &seqtime::idle_progress), 50);
    m_hadjust.signal_value_changed().connect
    (
        mem_fun(*this, &seqtime::change_horz)
    );
    update_sizes();
}

/**
 *  Implements the on-expose event handler.
 */

bool
seqtime::on_expose_event (GdkEventExpose * a_e)
{
    draw_drawable
    (
        a_e->area.x, a_e->area.y, a_e->area.x, a_e->area.y,
        a_e->area.width, a_e->area.height
    );
    return true;
}

/**
 *  Implements the on-size-allocate event handler.
 */

void
seqtime::on_size_allocate (Gtk::Allocation & a)
{
    gui_drawingarea_gtk2::on_size_allocate(a);
    m_window_x = a.get_width();
    m_window_y = a.get_height();
    update_sizes();
}

}           // namespace seq64

/*
 * seqtime.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

