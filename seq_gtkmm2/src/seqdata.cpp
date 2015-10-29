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
 * \file          seqdata.cpp
 *
 *  This module declares/defines the base class for plastering
 *  pattern/sequence data information in the data area of the pattern
 *  editor.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2015-10-27
 * \license       GNU GPLv2 or above
 *
 */

#include <gtkmm/adjustment.h>

#include "font.hpp"
#include "perform.hpp"
#include "seqdata.hpp"
#include "sequence.hpp"

namespace seq64
{

/**
 *  Principal constructor.  In the constructor you can only allocate
 *  colors, get_window() returns 0 because we have not been realized.
 */

seqdata::seqdata
(
    sequence & seq,
    perform & p,            // used only to satisfy gui_drawingarea_gtk2()
    int zoom,
    Gtk::Adjustment & hadjust
) :
    gui_drawingarea_gtk2    (p, hadjust, adjustment_dummy(), 10, c_dataarea_y),
    m_seq                   (seq),
    m_zoom                  (zoom),
    m_scroll_offset_ticks   (0),
    m_scroll_offset_x       (0),
    m_background_tile_x     (0),
    m_background_tile_y     (0),
    m_number_w              (p_font_renderer->char_width()+1),      // was 6
    m_number_h              (3*(p_font_renderer->char_height()+1)), // was 3*10
    m_number_offset_y       (p_font_renderer->char_height()-1),     // was 8
    m_status                (0),
    m_cc                    (0),
    m_numbers               (),             // an array
    m_old                   (),
    m_dragging              (false)
{
    set_flags(Gtk::CAN_FOCUS);
}

/**
 *  Updates the sizes in the pixmap if the view is realized, and queues up
 *  a draw operation.  It creates a pixmap with window dimensions given by
 *  m_window_x and m_window_y.
 */

void
seqdata::update_sizes ()
{
    if (is_realized())
    {
        m_pixmap = Gdk::Pixmap::create(m_window, m_window_x, m_window_y, -1);
        redraw();           // instead of update_pixmap(); queue_draw();
    }
}

/**
 *  This function calls update_size().  Then, regardless of whether the
 *  view is realized, updates the pixmap and queues up a draw operation.
 *
 * \note
 *      If it weren't for the is_realized() condition, we could just call
 *      update_sizes(), which does all this anyway.
 */

void
seqdata::reset ()
{
    update_sizes();
    redraw();       // use common code instead of update_pixmap(); queue_draw();
}

/**
 *  Sets the zoom to the given value and resets the view via the reset
 *  function.
 *
 *  This begs the question, do we have GUI access to the zoom setting?
 */

void
seqdata::set_zoom (int zoom)
{
    if (m_zoom != zoom)
    {
        m_zoom = zoom;
        reset();
    }
}

/**
 *  Sets the status to the given value, and the control to the optional
 *  given value, which defaults to 0, then calls redraw().
 *
 *  Perhaps we should check that at least one of the parameters causes a
 *  change.
 */

void
seqdata::set_data_type (unsigned char status, unsigned char control)
{
    m_status = status;
    m_cc = control;
    redraw();
}

/**
 *  Simply calls draw_events_on_pixmap().
 */

void
seqdata::update_pixmap ()
{
    draw_events_on_pixmap();
}

/**
 *  Draws events on the given drawable object.
 */

void
seqdata::draw_events_on (Glib::RefPtr<Gdk::Drawable> drawable)
{
    long tick;
    unsigned char d0, d1;
    bool selected;
    int start_tick = m_scroll_offset_ticks;
    int end_tick = (m_window_x * m_zoom) + m_scroll_offset_ticks;
    draw_rectangle(drawable, white(), 0, 0, m_window_x, m_window_y);
    m_gc->set_foreground(black());
    m_seq.reset_draw_marker();
    while (m_seq.get_next_event(m_status, m_cc, &tick, &d0, &d1, &selected))
    {
        if (tick >= start_tick && tick <= end_tick)
        {
            /* Turn into screen coordinates */

            int event_x = tick / m_zoom;            /* event_width=c_data_x */
            int event_height = d1;                  /* generate the value   */
            if (m_status == EVENT_PROGRAM_CHANGE ||
                    m_status == EVENT_CHANNEL_PRESSURE)
            {
                event_height = d0;
            }
            set_line(Gdk::LINE_SOLID, 2);
//          drawable->draw_line                     /* draw vert lines      */
//          (
//              m_gc, event_x - m_scroll_offset_x + 1,
//              c_dataarea_y - event_height, event_x - m_scroll_offset_x + 1,
//              c_dataarea_y
//          );
            draw_line                               /* draw vert lines      */
            (
                drawable, event_x - m_scroll_offset_x + 1,
                c_dataarea_y - event_height, event_x - m_scroll_offset_x + 1,
                c_dataarea_y
            );

            // event_x + 3 - m_scroll_offset_x, c_dataarea_y - 25, 6, 30

            drawable->draw_drawable
            (
                m_gc, m_numbers[event_height], 0, 0,
                event_x + 3 - m_scroll_offset_x,
                c_dataarea_y - m_number_h + 3,
                m_number_w, m_number_h
            );
        }
    }
}

/**
 *  Draws events on this object's built-in window and pixmap.
 *  This drawing is done only if there is no dragging in progress, to
 *  guarantee no flicker.
 */

int
seqdata::idle_redraw ()
{
    if (! m_dragging)
    {
        draw_events_on(m_window);
        draw_events_on(m_pixmap);
    }
    return true;
}

/**
 *  This function takes two points, and returns an Xwin rectangle, returned
 *  via the last four parameters.  It checks the mins/maxes, then fills in x,
 *  y, and width, height.
 */

void
seqdata::xy_to_rect
(
    int a_x1, int a_y1,
    int a_x2, int a_y2,
    int & r_x, int & r_y,
    int & r_w, int & r_h
)
{
    if (a_x1 < a_x2)
    {
        r_x = a_x1;
        r_w = a_x2 - a_x1;
    }
    else
    {
        r_x = a_x2;
        r_w = a_x1 - a_x2;
    }
    if (a_y1 < a_y2)
    {
        r_y = a_y1;
        r_h = a_y2 - a_y1;
    }
    else
    {
        r_y = a_y2;
        r_h = a_y1 - a_y2;
    }
}

/**
 *  Handles a motion-notify event.  It converts the x,y of the mouse to
 *  ticks, then sets the events in the event-data-range, updates the
 *  pixmap, draws events in the window, and draws a line on the window.
 */

bool
seqdata::on_motion_notify_event (GdkEventMotion * a_p0)
{
    if (m_dragging)
    {
        int adj_x_min, adj_x_max, adj_y_min, adj_y_max;
        m_current_x = (int) a_p0->x + m_scroll_offset_x;
        m_current_y = (int) a_p0->y;
        if (m_current_x < m_drop_x)
        {
            adj_x_min = m_current_x;
            adj_y_min = m_current_y;
            adj_x_max = m_drop_x;
            adj_y_max = m_drop_y;
        }
        else
        {
            adj_x_max = m_current_x;
            adj_y_max = m_current_y;
            adj_x_min = m_drop_x;
            adj_y_min = m_drop_y;
        }

        long tick_s, tick_f;
        convert_x(adj_x_min, tick_s);
        convert_x(adj_x_max, tick_f);
        m_seq.change_event_data_range
        (
            tick_s, tick_f, m_status, m_cc,
            c_dataarea_y - adj_y_min - 1, c_dataarea_y - adj_y_max - 1
        );
        update_pixmap();
        draw_events_on(m_window);
        draw_line_on_window();
    }
    return true;
}

/*
 * ca 2015-07-24
 * Eliminate this annoying warning.  Will do it for Microsoft's bloddy
 * compiler later.
 */

#ifdef PLATFORM_GNU
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

/**
 *  Handles an on-leave notification event.
 */

bool
seqdata::on_leave_notify_event (GdkEventCrossing * p0)
{
    redraw();           // instead of update_pixmap(); queue_draw();
    return true;
}

/**
 *  Draws on vertical line on the data window.
 */

void
seqdata::draw_line_on_window ()
{
    int x, y, w, h;
    m_gc->set_foreground(black());
    set_line(Gdk::LINE_SOLID);
    m_window->draw_drawable                         /* replace old */
    (
        m_gc, m_pixmap, m_old.x, m_old.y, m_old.x, m_old.y,
        m_old.width + 1, m_old.height + 1
    );
    xy_to_rect(m_drop_x, m_drop_y, m_current_x, m_current_y, x, y, w, h);
    x -= m_scroll_offset_x;
    m_old.x = x;
    m_old.y = y;
    m_old.width = w;
    m_old.height = h;
    draw_line
    (
        black(),
        m_current_x - m_scroll_offset_x, m_current_y,
        m_drop_x - m_scroll_offset_x, m_drop_y
    );
}

/**
 *  Change the scrolling offset on the x-axis, and redraw.
 */

void
seqdata::change_horz ()
{
    m_scroll_offset_ticks = int(m_hadjust.get_value());
    m_scroll_offset_x = m_scroll_offset_ticks / m_zoom;
    update_pixmap();
    force_draw();
}

/**
 *  Force a redraw.
 */

void
seqdata::force_draw ()
{
    m_window->draw_drawable(m_gc, m_pixmap, 0, 0, 0, 0, m_window_x, m_window_y);
}

/**
 *  Implements the on-realization event, by calling the base-class version
 *  and then allocating the resources that could not be allocated in the
 *  constructor.  It also connects up the change_horz() function.
 *
 *  Note that this function creates a small pixmap for every possible
 *  y-value, where y ranges from 0 to MIDI_COUNT_MAX-1 = 127.  It then fills
 *  each pixmap with a numeric representation of that y value, up to three
 *  digits (left-padded with spaces).
 */

void
seqdata::on_realize ()
{
    gui_drawingarea_gtk2::on_realize();
    m_hadjust.signal_value_changed().connect
    (
        mem_fun(*this, &seqdata::change_horz)
    );
    m_gc->set_foreground(white());              /* works for all drawing    */
    for (int i = 0; i < c_dataarea_y; ++i)      /* MIDI_COUNT_MAX; 128      */
    {
        m_numbers[i] = Gdk::Pixmap::create(m_window, m_number_w, m_number_h, -1);
//      m_gc->set_foreground(white());
//      m_numbers[i]->draw_rectangle(m_gc, true, 0, 0, m_number_w, m_number_h);
        draw_rectangle(m_numbers[i], 0, 0, m_number_w, m_number_h);

        char val[8];
        char num[8];
        snprintf(val, sizeof(val), "%3d\n", i);
        memset(num, 0, sizeof(num));
        num[0] = val[0];                /* converting to unicode? */
        num[2] = val[1];
        num[4] = val[2];
        render_number(m_numbers[i], 0, 0, &num[0]);
        render_number(m_numbers[i], 0, m_number_offset_y,     &num[2]);
        render_number(m_numbers[i], 0, m_number_offset_y * 2, &num[4]);
    }
    update_sizes();
}

/**
 *  Implements the on-expose event.
 */

bool
seqdata::on_expose_event (GdkEventExpose * a_e)
{
    m_window->draw_drawable
    (
        m_gc, m_pixmap, a_e->area.x, a_e->area.y,
        a_e->area.x, a_e->area.y, a_e->area.width, a_e->area.height
    );
    return true;
}

/**
 *  Implements the on-scroll event.  This scroll event only handles basic
 *  scrolling, without any modifier keys such as GDK_CONTROL_MASK or
 *  GDK_SHIFT_MASK.
 */

bool
seqdata::on_scroll_event (GdkEventScroll * a_ev)
{
    guint modifiers;                    // Used to filter out caps/num lock etc.
    modifiers = gtk_accelerator_get_default_mod_mask();
    if ((a_ev->state & modifiers) != 0)
        return false;

    if (a_ev->direction == GDK_SCROLL_UP)
        m_seq.increment_selected(m_status, m_cc);

    if (a_ev->direction == GDK_SCROLL_DOWN)
        m_seq.decrement_selected(m_status, m_cc);

    update_pixmap();
    queue_draw();
    return true;
}

/**
 *  Implement a button-press event.
 */

bool
seqdata::on_button_press_event (GdkEventButton * a_p0)
{
    if (a_p0->type == GDK_BUTTON_PRESS)
    {
        m_seq.push_undo();
        m_drop_x = (int) a_p0->x + m_scroll_offset_x; /* set values for line  */
        m_drop_y = (int) a_p0->y;
        m_old.x = 0;                /* reset box that holds dirty redraw spot */
        m_old.y = 0;
        m_old.width = 0;
        m_old.height = 0;
        m_dragging = true;          /* we are potentially dragging now!       */
    }
    return true;
}

/**
 *  Implement a button-release event.
 */

bool
seqdata::on_button_release_event (GdkEventButton * a_p0)
{
    m_current_x = (int) a_p0->x + m_scroll_offset_x;
    m_current_y = (int) a_p0->y;
    if (m_dragging)
    {
        long tick_s, tick_f;
        if (m_current_x < m_drop_x)
        {
            std::swap(m_current_x, m_drop_x);
            std::swap(m_current_y, m_drop_y);
        }
        convert_x(m_drop_x, tick_s);
        convert_x(m_current_x, tick_f);
        m_seq.change_event_data_range
        (
            tick_s, tick_f, m_status, m_cc,
            c_dataarea_y - m_drop_y - 1, c_dataarea_y - m_current_y - 1
        );
        m_dragging = false;     /* convert x,y to ticks, set events in range */
    }
    update_pixmap();
    queue_draw();
    return true;
}

/**
 *  Handle a size-allocation event.
 */

void
seqdata::on_size_allocate (Gtk::Allocation & a_r)
{
    gui_drawingarea_gtk2::on_size_allocate(a_r);
    m_window_x = a_r.get_width();
    m_window_y = a_r.get_height();
    update_sizes();
}

}           // namespace seq64

/*
 * seqdata.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
