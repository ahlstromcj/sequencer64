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
 * \updates       2018-06-02
 * \license       GNU GPLv2 or above
 *
 *  The data area consists of vertical lines, with the height of each line
 *  representing the value of the event data.  Currently, the scaling of the
 *  line height is very easy... one pixel per value, ranging from 0 to 127.
 */

#include <gtkmm/adjustment.h>

#include "font.hpp"
#include "gdk_basic_keys.h"
#include "gui_key_tests.hpp"            /* is_ctrl_key(), etc.          */
#include "perform.hpp"
#include "seqdata.hpp"
#include "sequence.hpp"

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *  Principal constructor.  In the constructor one can only allocate colors,
 *  get_window() returns 0 because this pane has not yet been realized.
 *
 * \param seq
 *      The sequence that is being displayed and edited by this data pane.
 *
 * \param p
 *      The performance object that oversees all of the sequences.  This
 *      object is needed here only to access the perform::modify() function.
 *
 * \param zoom
 *      The starting zoom of this pane.
 *
 * \param hadjust
 *      The horizontal adjustment object provided by the parent class,
 *      seqedit, that created this pane.
 */

seqdata::seqdata
(
    sequence & seq,
    perform & p,
    int zoom,
    Gtk::Adjustment & hadjust
) :
    gui_drawingarea_gtk2    (p, hadjust, adjustment_dummy(), 10, c_dataarea_y),
    m_seq                   (seq),
    m_zoom                  (zoom),
    m_scroll_offset_ticks   (0),
    m_scroll_offset_x       (0),
    m_number_w              (font_render().char_width()+1),      // was 6
    m_number_h              (3*(font_render().char_height()+1)), // was 3*10
    m_number_offset_y       (font_render().char_height()-1),     // was 8
    m_status                (0),
    m_cc                    (0),
    m_old                   (),
#ifdef USE_STAZED_SEQDATA_EXTENSIONS
    m_drag_handle           (false),
#endif
    m_dragging              (false)
{
    set_flags(Gtk::CAN_FOCUS);
}

/**
 *  Updates the sizes in the pixmap if the view is realized, and queues up
 *  a draw operation.  It creates a pixmap with window dimensions given by
 *  m_window_x and m_window_y.
 *
 *  We thought there was a potential memory leak, since m_pixmap is created
 *  every time the window is resized, but valgrind says otherwise... maybe.
 *  An awful lot of Gtk leaks!
 */

void
seqdata::update_sizes ()
{
    if (is_realized())
    {
        m_pixmap = Gdk::Pixmap::create(m_window, m_window_x, m_window_y, -1);
        redraw();
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
    /*
     * Stazed fix; same code found in change_horz().
     */

    m_scroll_offset_ticks = int(m_hadjust.get_value());
    m_scroll_offset_x = m_scroll_offset_ticks / m_zoom;
    update_sizes();

    /*
     * Instead of this, try forcing the redraw, to eliminate the bug of not
     * redrawing on changing zoom.
     *
     * redraw();
     */

    update_pixmap();
    force_draw();
}

/**
 *  Sets the zoom to the given value and resets the view via the reset
 *  function.  Called by seqedit::set_zoom(), which validates the zoom value.
 *
 * \param z
 *      The desired zoom value, assumed to be validated already.  See the
 *      seqedit::set_zoom() function.
 */

void
seqdata::set_zoom (int z)
{
    if (m_zoom != z)
    {
        m_zoom = z;
        reset();
    }
}

/**
 *  Sets the status to the given value, and the control to the optional given
 *  value, which defaults to 0, then calls redraw().  Perhaps we should check
 *  that at least one of the parameters causes a change.
 *
 * \param status
 *      The MIDI event byte (status byte) to set.
 *
 * \param control
 *      The MIDI CC value to set.
 */

void
seqdata::set_data_type (midibyte status, midibyte control)
{
    bool doredraw = (status != m_status) || (control != m_cc);
    m_status = status;
    m_cc = control;
    if (doredraw)
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
 *  Draws events on the given drawable object.  Very similar to seqevent ::
 *  draw_events_on().  And yet it doesn't handle zooming as well, must fix!
 *
 * Stazed:
 *
 *      For Note On there can be multiple events on the same vertical in which
 *      the selected item can be covered.  For Note On the selected item
 *      needs to be drawn last so it can be seen.  So, for other events the
 *      variable num_selected_events will be -1 for ALL_EVENTS. For Note On
 *      only, the variable will be the number of selected events. If 0 then
 *      only one pass is needed. If > 0 then two passes are needed, one for
 *      unselected (first), and one for selected (last).  For the first pass,
 *      if any events are selected, the selection type is EVENTS_UNSELECTED.
 *      For the second pass, it will be set to num_selected_events.
 *
 *  We now draw the data line for selected event in dark orange, instead of
 *  black.  We're not likely to adopt the Stazed convention of drawing in blue.
 *  Also, there seem to be some bugs in how the data selection works.  Needs
 *  more evaluation.
 *
 *  Also, if we decide to draw handle on each vertical data line, it would
 *  look nicer if a circle.
 *
 * \param drawable
 *      The given drawable object.
 */

void
seqdata::draw_events_on (Glib::RefPtr<Gdk::Drawable> drawable)
{
    int starttick = m_scroll_offset_ticks;
    int endtick = (m_window_x * m_zoom) + m_scroll_offset_ticks;

    /*
     * Add a black border.  However, not sure yet why we can't get a black
     * line at the bottom; what else is painting down there?
     *
     * draw_rectangle(drawable, white_paint(), 0, 0, m_window_x, m_window_y);
     */

    draw_rectangle(drawable, black_paint(), 0, 0, m_window_x, m_window_y);
    draw_rectangle(drawable, white_paint(), 1, 1, m_window_x-2, m_window_y-1);
    m_gc->set_foreground(black_paint());

#ifdef USE_STAZED_SEQDATA_EXTENSIONS
    int numselected = EVENTS_ALL;
    int seltype = numselected;
    if (m_status == EVENT_NOTE_ON)                  // iffy
    {
        numselected = m_seq.get_num_selected_events(m_status, m_cc);
        if (numselected > 0)
            seltype = EVENTS_UNSELECTED;
    }
    do
    {
#endif

        event_list::const_iterator ev;
        m_seq.reset_ex_iterator(ev);
        while (m_seq.get_next_event_ex(m_status, m_cc, ev))
        {
            midipulse tick = ev->get_timestamp();
            bool selected = ev->is_selected();
            if (tick >= starttick && tick <= endtick)
            {
                int event_x = tick / m_zoom;        /* screen coordinate    */
                int x = event_x - m_scroll_offset_x + 1;
                int event_height;
                Color paint = black_paint();
                if (ev->is_tempo())
                {
                    event_height = int(tempo_to_note_value(ev->tempo()));
                    paint = tempo_paint();
                }
                else if (ev->is_ex_data())
                {
                    /*
                     * Do nothing for other Meta events at this time.  Don't
                     * forget to increment the iterator now!
                     */

                    ++ev;                           /* now a must-do        */
                    continue;
                }
                else
                {
                    midibyte d0, d1;
                    ev->get_data(d0, d1);
                    event_height = event::is_one_byte_msg(m_status) ? d0 : d1 ;
                }
                set_line(Gdk::LINE_SOLID, 2);       /* vertical event line  */
                draw_line
                (
                    drawable, selected ? dark_orange() : paint,
                    x, c_dataarea_y - event_height, x, c_dataarea_y
                );

                if (ev->is_tempo())
                {
                    draw_rectangle                  /* draw handle          */
                    (
                        drawable, selected ? dark_orange() : tempo_paint(),
                        event_x - m_scroll_offset_x - 3,
                        c_dataarea_y - event_height,
                        c_data_handle_x,
                        c_data_handle_y
                    );
                }

#ifdef USE_STAZED_SEQDATA_EXTENSIONS
                else
                {
                    draw_rectangle                  /* draw handle          */
                    (
                        drawable, selected ? dark_orange() : black_paint(),
                        event_x - m_scroll_offset_x - 3,
                        c_dataarea_y - event_height,
                        c_data_handle_x,
                        c_data_handle_y
                    );
                }
#endif

                if (ev->is_tempo())
                    render_digits(drawable, int(ev->tempo()), x);
                else
                    render_digits(drawable, event_height, x);
            }
            ++ev;                                   /* now a must-do        */
        }
#ifdef USE_STAZED_SEQDATA_EXTENSIONS
        if (seltype == EVENTS_UNSELECTED)
            seltype = numselected;
        else
            break;

    } while (seltype == EVENTS_UNSELECTED);
#endif
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
 *  Draws on vertical line on the data window.
 */

void
seqdata::draw_line_on_window ()
{
    m_gc->set_foreground(black_paint());
    set_line(Gdk::LINE_SOLID);
    draw_drawable                                   /* replace old */
    (
        m_old.x, m_old.y, m_old.x, m_old.y, m_old.width + 1, m_old.height + 1
    );

    int x, y, w, h;
    xy_to_rect(m_drop_x, m_drop_y, m_current_x, m_current_y, x, y, w, h);
    x -= m_scroll_offset_x;
    m_old.x = x;
    m_old.y = y;
    m_old.width = w;
    m_old.height = h;
    draw_line
    (
        black_paint(), m_current_x - m_scroll_offset_x, m_current_y,
        m_drop_x - m_scroll_offset_x, m_drop_y
    );
}

/**
 *  Change the scrolling offset on the x-axis, and redraw.  Basically
 *  identical to seqevent::change_horz().
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
 *  This function takes two points, and returns an XWin rectangle, returned
 *  via the last four parameters.  It checks the mins/maxes, then fills in x,
 *  y, and width, height.
 *
 * \param x1
 *      The input x value for the first data point.
 *
 * \param y1
 *      The input y value for the first data point.
 *
 * \param x2
 *      The input x value for the second data point.
 *
 * \param y2
 *      The input y value for the second data point.
 *
 * \param [out] rx
 *      The output for the x value of the XWin rectangle.
 *
 * \param [out] ry
 *      The output for the y value of the XWin rectangle.
 *
 * \param [out] rw
 *      The output for the width value of the XWin rectangle.
 *
 * \param [out] rh
 *      The output for the height of the XWin rectangle.
 */

void
seqdata::xy_to_rect
(
    int x1, int y1,
    int x2, int y2,
    int & rx, int & ry,
    int & rw, int & rh
)
{
    if (x1 < x2)
    {
        rx = x1;
        rw = x2 - x1;
    }
    else
    {
        rx = x2;
        rw = x1 - x2;
    }

    if (y1 < y2)
    {
        ry = y1;
        rh = y2 - y1;
    }
    else
    {
        ry = y2;
        rh = y1 - y2;
    }
}

/**
 *  Handles a motion-notify event.  It converts the x,y of the mouse to
 *  ticks, then sets the events in the event-data-range, updates the
 *  pixmap, draws events in the window, and draws a line on the window.
 *
 * \param ev
 *      The motion event.
 *
 * \return
 *      Returns true if a change in event data occurred.  If true, then
 *      the perform modification flag is set.
 */

bool
seqdata::on_motion_notify_event (GdkEventMotion * ev)
{
    bool result = false;
#ifdef USE_STAZED_SEQDATA_EXTENSIONS
    if (m_drag_handle)
    {
        m_current_y = int(ev->y = 3);
        m_current_y = c_dataarea_y - m_current_y;
        if (m_current_y < 0)
            m_current_y = 0;

        if (m_current_y > SEQ64_MAX_DATA_VALUE)             /* 127 */
            m_current_y = SEQ64_MAX_DATA_VALUE;

        m_seq.adjust_data_handle(m_status, m_current_y);
        update_pixmap();
        draw_events_on(m_window);
    }
#endif
    if (m_dragging)
    {
        int adj_x_min, adj_x_max, adj_y_min, adj_y_max;
        m_current_x = int(ev->x) + m_scroll_offset_x;
        m_current_y = int(ev->y);
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

        midipulse tick_s, tick_f;
        convert_x(adj_x_min, tick_s);
        convert_x(adj_x_max, tick_f);
        result = m_seq.change_event_data_range
        (
            tick_s, tick_f, m_status, m_cc,
            c_dataarea_y - adj_y_min - 1, c_dataarea_y - adj_y_max - 1
        );
        update_pixmap();                /* calls draw_events_on_pixmap()    */
        draw_events_on(m_window);
        draw_line_on_window();

        /*
         * \change ca 2016-06-19
         *  Why do we modify here?
         *
         *  if (result)
         *      perf().modify();
         */
    }
    return result;
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
 *
 *  Parameter "p0", the crossing point for the event, is unused.
 */

bool
seqdata::on_leave_notify_event (GdkEventCrossing * /*p0*/)
{
#ifdef USE_STAZED_SEQDATA_EXTENSIONS
    if (m_seq.get_hold_undo())
    {
        m_seq.push_undo(true);
        m_seq.set_hold_undo(false);
    }
#endif
    redraw();
    return true;
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
    m_gc->set_foreground(white_paint());        /* works for all drawing    */
    update_sizes();
}

/**
 *  Implements the on-expose event by calling draw_drawable() on the event.
 *
 * \param ev
 *      Provides the expose-event.
 *
 * \return
 *      Always returns true.
 */

bool
seqdata::on_expose_event (GdkEventExpose * ev)
{
    draw_drawable
    (
        ev->area.x, ev->area.y, ev->area.x, ev->area.y,
        ev->area.width, ev->area.height
    );
    return true;
}

/**
 *  Implements the on-scroll event.  This scroll event only handles basic
 *  scrolling, without any modifier keys such as the Ctrl of Shift masks.
 *  If there is a note (seqroll pane) or event (seqevent pane) selected,
 *  and mouse hovers over the data area (seqdata pane), then this scrolling
 *  action will increase or decrease the value of the data item, which
 *  lengthens of shortens the line drawn.
 *
 * \todo
 *      DOCUMENT the seqdata scrolling behavior in the documentation projects.
 *
 * \param ev
 *      Provides the scroll-event.
 *
 * \return
 *      Always returns true.
 */

bool
seqdata::on_scroll_event (GdkEventScroll * ev)
{
    if (! is_no_modifier(ev))
        return false;

    if (CAST_EQUIVALENT(ev->direction, SEQ64_SCROLL_UP))
        m_seq.increment_selected(m_status, m_cc);

    if (CAST_EQUIVALENT(ev->direction, SEQ64_SCROLL_DOWN))
        m_seq.decrement_selected(m_status, m_cc);

    update_pixmap();
    queue_draw();
    return true;
}

/**
 *  Implements a mouse button-press event.  This function pushes the undo
 *  information for the sequence, sets the drop-point, resets the box that
 *  holds dirty redraw spot, and sets m_dragging to true.
 *
 * \param ev
 *      Provides the button-press event.
 *
 * \return
 *      Always returns true.
 */

bool
seqdata::on_button_press_event (GdkEventButton * ev)
{
    if (CAST_EQUIVALENT(ev->type, SEQ64_BUTTON_PRESS))
    {
        m_drop_x = int(ev->x) + m_scroll_offset_x;  /* set values for line  */
        m_drop_y = int(ev->y);

#ifdef USE_STAZED_SEQDATA_EXTENSIONS

        /*
         * If the user selects the "handle"...
         */

        midipulse tick_s, tick_f;
        convert_x(m_drop_x - 3, tick_s);            /* side-effect  */
        convert_x(m_drop_x + 3, tick_f);            /* side-effect  */
        m_drag_handle = m_seq.select_event_handle
        (
            tick_s, tick_f, m_status, m_cc, c_dataarea_y - m_drop_y + 3
        );

        if (m_drag_handle)
        {
            /* if they used line draw but did not leave... */

            if (! m_seq.get_hold_undo())
                m_seq.push_undo();
        }

        /* reset box that holds dirty redraw spot */

        m_old.x = m_old.y = m_old.width = m_old.height = 0;
        m_dragging = ! m_drag_handle;
#else
        m_old.x = m_old.y = m_old.width = m_old.height = 0;
        m_dragging = true;                          /* may be dragging now  */
#endif
    }
    return true;
}

/**
 *  Implement a button-release event.  Sets the current point.  If m_dragging
 *  is true, then the sequence data is changed, the performance modification
 *  flag is set, and m_dragging is reset.
 *
 * \param ev
 *      Provides the button-release event.
 *
 * \return
 *      Returns true if a modification occurred, and in that case also sets
 *      the perform modification flag.
 */

bool
seqdata::on_button_release_event (GdkEventButton * ev)
{
    bool result = false;
    m_current_x = int(ev->x) + m_scroll_offset_x;
    m_current_y = int(ev->y);
    if (m_dragging)
    {
        midipulse tick_s, tick_f;
        if (m_current_x < m_drop_x)
        {
            std::swap(m_current_x, m_drop_x);
            std::swap(m_current_y, m_drop_y);
        }
        convert_x(m_drop_x, tick_s);
        convert_x(m_current_x, tick_f);
        result = m_seq.change_event_data_range
        (
            tick_s, tick_f, m_status, m_cc,
            c_dataarea_y - m_drop_y - 1, c_dataarea_y - m_current_y - 1
        );
        m_dragging = false;     /* convert x,y to ticks, set events in range */

        /*
         * \change ca 2016-06-19
         *  Why do we modify here?
         *
         *  if (result)
         *      perf().modify();
         */
    }

#ifdef USE_STAZED_SEQDATA_EXTENSIONS
    if (m_drag_handle)
    {
        m_drag_handle = false;
        m_seq.unselect();
        m_seq.set_dirty();
    }
#endif

    update_pixmap();
    queue_draw();
    return result;
}

/**
 *  Handles a size-allocation event by updating m_window_x and m_window_y, and
 *  then updating all of the sizes of the data pane in update_sizes().
 *
 * \param r
 *      Provides the allocation event.
 */

void
seqdata::on_size_allocate (Gtk::Allocation & r)
{
    gui_drawingarea_gtk2::on_size_allocate(r);
    m_window_x = r.get_width();
    m_window_y = r.get_height();
    update_sizes();
}

/**
 *  Renders an up to 3-digit string vertically to represent a data value.
 *
 * \param drawable
 *      Where to draw the digits.
 *
 * \param digits
 *      xxxxxx
 *
 * \param x
 *      xxxxxx
 */

void
seqdata::render_digits
(
    Glib::RefPtr<Gdk::Drawable> drawable,
    int digits, int x
)
{
    static Glib::RefPtr<Gdk::Pixmap> s_pixmap = Gdk::Pixmap::create
    (
        m_window, m_number_w, m_number_h, -1
    );
    static char s_num[8];
    static bool s_needs_init = true;
    if (s_needs_init)
    {
        s_needs_init = false;
        memset(s_num, 0, sizeof s_num);
    }

    char val[4];
    snprintf(val, sizeof val, "%3d", digits);
    m_gc->set_foreground(white_paint());
    s_num[0] = val[0];
    s_num[2] = val[1];
    s_num[4] = val[2];
    draw_rectangle(s_pixmap, 0, 0, m_number_w, m_number_h);
    render_number(s_pixmap, 0, 0, &s_num[0]);
    render_number(s_pixmap, 0, m_number_offset_y,     &s_num[2]);
    render_number(s_pixmap, 0, m_number_offset_y * 2, &s_num[4]);
    drawable->draw_drawable                     /* m_window vs m_pixmap */
    (
        m_gc, s_pixmap, 0, 0,
        x + 2, c_dataarea_y - m_number_h + 3, m_number_w, m_number_h
    );
}

}           // namespace seq64

/*
 * seqdata.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

