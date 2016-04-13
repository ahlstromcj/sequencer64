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
 * \updates       2016-04-12
 * \license       GNU GPLv2 or above
 *
 *  The patterns/sequence editor is expandable in both directions, but the
 *  time/measures bar does not change in size.
 */

#include <gtkmm/adjustment.h>

#include "app_limits.h"                 /* SEQ64_SOLID_PIANOROLL_GRID   */
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
 *  Sets the zoom to the given value and resets the window.
 */

void
seqtime::set_zoom (int zoom)
{
    if (m_zoom != zoom)
    {
        m_zoom = zoom;
        reset();
    }
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
 *  pixmap, and resets the window.  Basically identical to seqevent::reset().
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
 *  Updates the pixmap.  When the zoom is at 32 ticks per pixel, there is a
 *  thick bar for every measure, and a measure number and major time division
 *  every 4 measures.at the default PPQN of 192.
 *
 *  A major line is a line that has a measure number in the timeline.  The
 *  number of measures in a major line is 1 for zooms from 1:1 to 1:8; 2 for
 *  zoom 1:16; 4 for zoom 1:32; 8 for zoom 1:64 (new); and 16 for zoom 1:128.
 *  Zooms 1:64 and above look good only for high PPQN values.
 *
 *  We calculate the measure length in 32nd notes.  This value is, of course,
 *  32, when the time signature is 4/4.  Then calculate measures/line.
 *  "measures_per_major" is more like "measures per major line".  With a higher
 *  zoom than 32, this calculation yields a floating-point exception if m_zoom
 *  > 32, so we rearrange the calculation and hope that it still works out the
 *  same for smaller values.
*/

void
seqtime::update_pixmap ()
{
    draw_rectangle_on_pixmap(white(), 0, 0, m_window_x, m_window_y);
#ifdef SEQ64_SOLID_PIANOROLL_GRID
    set_line(Gdk::LINE_SOLID, 2);
#else
    set_line(Gdk::LINE_SOLID);
#endif
    draw_line_on_pixmap(black(), 0, m_window_y - 1, m_window_x, m_window_y - 1);

    int bpbar = m_seq.get_beats_per_bar();
    int bwidth = m_seq.get_beat_width();
    int ticks_per_major = 4 * m_ppqn * bpbar / bwidth;
    int endtick = (m_window_x * m_zoom) + m_scroll_offset_ticks;
    int starttick = m_scroll_offset_ticks -
        (m_scroll_offset_ticks % ticks_per_major);

    m_gc->set_foreground(black());                          /* vert. line   */
    for (int tick = starttick; tick < endtick; tick += ticks_per_major)
    {
        char bar[8];
        int x_offset = (tick / m_zoom) - m_scroll_offset_x; /* for the beat */
        draw_line_on_pixmap(x_offset, 0, x_offset, m_window_y);
        snprintf(bar, sizeof(bar), "%d", (tick / ticks_per_major) + 1);
        render_string_on_pixmap(x_offset + 2, 1, bar, font::BLACK);
    }

    /**
     * \todo
     *      Sizing needs to be controlled by font parameters. Instead of 19 or
     *      20, estimate the width of 3 letters. Instead of 9 pixels down, use
     *      the height of the seqtime and the height of a character.
     */

#ifdef SEQ64_SOLID_PIANOROLL_GRID
    /*
     *  Puts number after the number of the next measure:
     *
     *  long end_x = m_seq.get_length() / m_zoom - m_scroll_offset_x + 15;
     *  draw_rectangle_on_pixmap(black(), end_x, 2, 20, 10);
     *  render_string_on_pixmap(end_x + 1, 2, "END", font::WHITE);
     *
     *  The new code puts the number just before the end of the next measure,
     *  and is less cramped.  Some might not like it.
     */

    long end_x = m_seq.get_length() / m_zoom - m_scroll_offset_x - 21;
    draw_rectangle_on_pixmap(black(), end_x, 7, 20, 10);
    render_string_on_pixmap(end_x + 1, 6, "END", font::WHITE);
#else
    long end_x = m_seq.get_length() / m_zoom - m_scroll_offset_x;
    draw_rectangle_on_pixmap(black(), end_x, 9, 19, 8);
    render_string_on_pixmap(end_x + 1, 7, "END", font::WHITE);
#endif
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

