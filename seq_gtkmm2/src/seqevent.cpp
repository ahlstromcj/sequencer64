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
 * \file          seqevent.cpp
 *
 *  This module declares/defines the base class for the event pane.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2015-09-13
 * \license       GNU GPLv2 or above
 *
 */

#include <gtkmm/accelkey.h>
#include <gtkmm/adjustment.h>

#include "event.hpp"
#include "seqevent.hpp"
#include "seqdata.hpp"
#include "sequence.hpp"

namespace seq64
{

/**
 *  Principal constructor.
 */

seqevent::seqevent
(
    sequence * a_seq,
    int a_zoom,
    int a_snap,
    seqdata * a_seqdata_wid,
    Gtk::Adjustment * a_hadjust
) :
    m_fruity_interaction    (),
    m_seq24_interaction     (),
    m_gc                    (),
    m_window                (),
    m_black                 (Gdk::Color("black")),
    m_white                 (Gdk::Color("white")),
    m_grey                  (Gdk::Color("grey")),
    m_red                   (Gdk::Color("orange")),
    m_pixmap                (),
    m_window_x              (0),
    m_window_y              (0),
    m_drop_x                (0),
    m_drop_y                (0),
    m_current_x             (0),
    m_current_y             (0),
    m_hadjust               (a_hadjust),
    m_seq                   (a_seq),
    m_zoom                  (a_zoom),
    m_snap                  (a_snap),
    m_old                   (),
    m_selected              (),
    m_scroll_offset_ticks   (0),
    m_scroll_offset_x       (0),
    m_seqdata_wid           (a_seqdata_wid),
    m_selecting             (false),
    m_moving_init           (false),
    m_moving                (false),
    m_growing               (false),
    m_painting              (false),
    m_paste                 (false),
    m_move_snap_offset_x    (0),
    m_status                (EVENT_NOTE_ON),
    m_cc                    (0)
{
    Glib::RefPtr<Gdk::Colormap> colormap = get_default_colormap();
    colormap->alloc_color(m_black);
    colormap->alloc_color(m_white);
    colormap->alloc_color(m_grey);
    colormap->alloc_color(m_red);
    add_events
    (
        Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK |
        Gdk::POINTER_MOTION_MASK | Gdk::KEY_PRESS_MASK |
        Gdk::KEY_RELEASE_MASK | Gdk::FOCUS_CHANGE_MASK
    );
    set_size_request(10, c_eventarea_y);
    set_double_buffered(false);
}

/**
 *  Changes the horizontal scrolling offset for ticks, then updates the
 *  pixmap and forces a redraw.
 */

void
seqevent::change_horz ()
{
    m_scroll_offset_ticks = (int) m_hadjust->get_value();
    m_scroll_offset_x = m_scroll_offset_ticks / m_zoom;
    update_pixmap();
    force_draw();
}

/**
 *  Implements redraw while idling.
 *
 *  Who calls this routine?
 */

int
seqevent::idle_redraw ()
{
    draw_events_on(m_window);
    draw_events_on(m_pixmap);
    return true;
}

/**
 *  If the window is realized, this function creates a pixmap with window
 *  dimensions, the updates the pixmap, and queues up a redraw.
 *
 *  This ends up filling  the background with dotted lines, etc.
 */

void
seqevent::update_sizes ()
{
    if (is_realized())
    {
        m_pixmap = Gdk::Pixmap::create(m_window, m_window_x, m_window_y, -1);
        update_pixmap();
        queue_draw();
    }
}

/**
 *  This function basically resets the whole widget as if it was realized
 *  again.
 */

void
seqevent::reset ()
{
    m_scroll_offset_ticks = (int) m_hadjust->get_value();
    m_scroll_offset_x = m_scroll_offset_ticks / m_zoom;
    update_sizes();
    update_pixmap();
    draw_pixmap_on_window();
}

/**
 *  Adjusts the scrolling offset for ticks, updates the pixmap, and draws
 *  it on the window.
 */

void
seqevent::redraw ()
{
    m_scroll_offset_ticks = (int) m_hadjust->get_value();
    m_scroll_offset_x = m_scroll_offset_ticks / m_zoom;
    update_pixmap();
    draw_pixmap_on_window();
}

/**
 *  This function updates the background.  It sets the foreground to
 *  white, draws the rectangle.
 */

void
seqevent::draw_background ()
{
    m_gc->set_foreground(m_white);              /* clear background */
    m_pixmap->draw_rectangle
    (
        m_gc, true, 0, 0, m_window_x, m_window_y
    );

    int measures_per_line = 1;
    int ticks_per_measure =  m_seq->get_bpm() * (4 * c_ppqn) / m_seq->get_bw();
    int ticks_per_beat = (4 * c_ppqn) / m_seq->get_bw();
    int ticks_per_step = 6 * m_zoom;
    int ticks_per_m_line =  ticks_per_measure * measures_per_line;
    int start_tick = m_scroll_offset_ticks -
        (m_scroll_offset_ticks % ticks_per_step);

    int end_tick = (m_window_x * m_zoom) + m_scroll_offset_ticks;
    m_gc->set_foreground(m_grey);
    for (int i = start_tick; i < end_tick; i += ticks_per_step)
    {
        int base_line = i / m_zoom;
        if (i % ticks_per_m_line == 0)          /* a solid line on every beat */
        {
            m_gc->set_foreground(m_black);
            m_gc->set_line_attributes
            (
                1, Gdk::LINE_SOLID, Gdk::CAP_NOT_LAST, Gdk::JOIN_MITER
            );
        }
        else if (i % ticks_per_beat == 0)
        {
            m_gc->set_foreground(m_grey);
            m_gc->set_line_attributes
            (
                1, Gdk::LINE_SOLID, Gdk::CAP_NOT_LAST, Gdk::JOIN_MITER
            );
        }
        else
        {
            m_gc->set_foreground(m_grey);
            m_gc->set_line_attributes
            (
                1, Gdk::LINE_ON_OFF_DASH, Gdk::CAP_NOT_LAST, Gdk::JOIN_MITER
            );
            gint8 dash = 1;
            m_gc->set_dashes(0, &dash, 1);
        }
        m_pixmap->draw_line
        (
            m_gc, base_line - m_scroll_offset_x,
            0, base_line - m_scroll_offset_x, m_window_y
        );
    }

    m_gc->set_line_attributes                       /* reset line style */
    (
        1, Gdk::LINE_SOLID, Gdk::CAP_NOT_LAST, Gdk::JOIN_MITER
    );
    m_gc->set_foreground(m_black);
    m_pixmap->draw_rectangle
    (
        m_gc, false, -1, 0, m_window_x + 1, m_window_y - 1
    );
}

/**
 *  Sets zoom to the given value, and resets if the value ended up being
 *  changed.
 */

void
seqevent::set_zoom (int a_zoom)
{
    if (m_zoom != a_zoom)
    {
        m_zoom = a_zoom;
        reset();
    }
}

/**
 *  Sets the status to the given parameter, and the CC value to the given
 *  optional control parameter, which defaults to 0.  Then redraws.
 */

void
seqevent::set_data_type (unsigned char a_status, unsigned char a_control = 0)
{
    m_status = a_status;
    m_cc = a_control;
    redraw();               // this->redraw();
}

/**
 *  Redraws the background pixmap on the main pixmap, then puts the events on.
 */

void
seqevent::update_pixmap ()
{
    draw_background();
    draw_events_on_pixmap();
    m_seqdata_wid->update_pixmap();
    m_seqdata_wid->draw_pixmap_on_window();
}

/**
 *  Draws events on the given drawable object.
 */

void
seqevent::draw_events_on (Glib::RefPtr<Gdk::Drawable> a_draw)
{
    long tick;
    int x;
    unsigned char d0, d1;
    bool selected;
    m_gc->set_foreground(m_black);          /* draw boxes from sequence */

    int start_tick = m_scroll_offset_ticks ;
    int end_tick = (m_window_x * m_zoom) + m_scroll_offset_ticks;
    m_seq->reset_draw_marker();
    while
    (
        m_seq->get_next_event(m_status, m_cc, &tick, &d0, &d1, &selected)
    )
    {
        if ((tick >= start_tick && tick <= end_tick))
        {
            x = tick / m_zoom;              /* turn into screen coordinates */
            m_gc->set_foreground(m_black);
            a_draw->draw_rectangle
            (
                m_gc, true, x -  m_scroll_offset_x,
                (c_eventarea_y - c_eventevent_y) / 2,
                c_eventevent_x, c_eventevent_y
            );

            if (selected)
                m_gc->set_foreground(m_red);
            else
                m_gc->set_foreground(m_white);

            a_draw->draw_rectangle
            (
                m_gc, true, x -  m_scroll_offset_x + 1,
                (c_eventarea_y - c_eventevent_y) / 2 + 1,
                c_eventevent_x - 3, c_eventevent_y - 3
            );
        }
    }
}

/**
 *  This function fills the main pixmap with events.
 */

void
seqevent::draw_events_on_pixmap ()
{
    draw_events_on(m_pixmap);
}

/**
 *  This function currently just queues up a draw operation for the
 *  pixmap.
 *
 *  Old comments:
 *
 *      It then tells event to do the same.
 *
 *      We changed something on this window, and chances are we need to
 *      update the event widget as well and update our velocity window.
 *
 *          m_seqdata_wid->update_pixmap();
 *          m_seqdata_wid->draw_pixmap_on_window();
 *          RCB ??
 */

void
seqevent::draw_pixmap_on_window ()
{
    queue_draw();
}

/**
 *  This function checks the mins / maxes.  Then it fills in x
 *  and the width.
 */

void
seqevent::x_to_w (int a_x1, int a_x2, int *a_x, int *a_w)
{
    if (a_x1 < a_x2)
    {
        *a_x = a_x1;
        *a_w = a_x2 - a_x1;
    }
    else
    {
        *a_x = a_x2;
        *a_w = a_x1 - a_x2;
    }
}

/**
 *  Draw the selected events on the window.
 */

void
seqevent::draw_selection_on_window ()
{
    int x, w;
    int y = (c_eventarea_y - c_eventevent_y) / 2;
    int h =  c_eventevent_y;
    m_gc->set_line_attributes
    (
        1, Gdk::LINE_SOLID, Gdk::CAP_NOT_LAST, Gdk::JOIN_MITER
    );

    m_window->draw_drawable                 /* replace old */
    (
        m_gc, m_pixmap, m_old.x, y, m_old.x, y, m_old.width + 1, h + 1
    );
    if (m_selecting)
    {
        x_to_w(m_drop_x, m_current_x, &x, &w);
        x -= m_scroll_offset_x;
        m_old.x = x;
        m_old.width = w;
        m_gc->set_foreground(m_black);
        m_window->draw_rectangle(m_gc, false, x, y, w, h);
    }
    if (m_moving || m_paste)
    {
        int delta_x = m_current_x - m_drop_x;
        x = m_selected.x + delta_x;
        x -= m_scroll_offset_x;
        m_gc->set_foreground(m_black);
        m_window->draw_rectangle(m_gc, false, x, y, m_selected.width, h);
        m_old.x = x;
        m_old.width = m_selected.width;
    }
}

/**
 *  Forces a draw on the current drawable area of the window.
 */

void
seqevent::force_draw ()
{
    m_window->draw_drawable(m_gc, m_pixmap, 0, 0, 0, 0, m_window_x, m_window_y);
    draw_selection_on_window();
}

/**
 *  Starts a paste operation.
 */

void
seqevent::start_paste ()
{
    long tick_s;
    long tick_f;
    int note_h;
    int note_l;
    int x, w;
    snap_x(&m_current_x);
    snap_y(&m_current_x);
    m_drop_x = m_current_x;
    m_drop_y = m_current_y;
    m_paste = true;

    /* get the box that selected elements are in */

    m_seq->get_clipboard_box(&tick_s, &note_h, &tick_f, &note_l);

    /* convert box to X,Y values */

    convert_t(tick_s, &x);
    convert_t(tick_f, &w);

    /* w is actually coordinates now, so we have to change */

    w = w - x;

    /* set m_selected rectangle to hold the x,y,w,h of our selected events */

    m_selected.x = x;
    m_selected.width = w;
    m_selected.y = (c_eventarea_y - c_eventevent_y) / 2;
    m_selected.height = c_eventevent_y;

    /* adjust for clipboard being shifted to tick 0 */

    m_selected.x  += m_drop_x;
}

/**
 *  Takes the screen x coordinate, multiplies it by the current zoom, and
 *  returns the tick value in the given parameter.
 */

void
seqevent::convert_x (int a_x, long * a_tick)
{
    *a_tick = a_x * m_zoom;
}

/**
 *  Converts the given tick value to an x corrdinate, based on the zoom,
 *  and returns it via the second parameter.
 */

void
seqevent::convert_t (long a_ticks, int * a_x)
{
    *a_x = a_ticks /  m_zoom;
}

/**
 *  This function performs a 'snap' on y.
 */

void
seqevent::snap_y(int * a_y)
{
    *a_y = *a_y - (*a_y % c_key_y);
}

/**
 *  This function performs a 'snap' on x.
 *
 *      -   snap = number pulses to snap to
 *      -   m_zoom = number of pulses per pixel,
 *
 *  Therefore snap / m_zoom  = number pixels to snap to.
 */

void
seqevent::snap_x (int * a_x)
{
    int mod = (m_snap / m_zoom);
    if (mod <= 0)
        mod = 1;

    *a_x -= (*a_x % mod);       // *a_x = *a_x - (*a_x % mod);
}

/**
 *  Implements the on-realize callback.  It calls the base-class version,
 *  and then allocates additional resource not allocated in the
 *  constructor.  Finally, it connects up the change_horz function.
 */

void
seqevent::on_realize ()
{
    Gtk::DrawingArea::on_realize();
    set_flags(Gtk::CAN_FOCUS);
    m_window = get_window();
    m_gc = Gdk::GC::create(m_window);
    m_window->clear();
    m_hadjust->signal_value_changed().connect
    (
        mem_fun(*this, &seqevent::change_horz)
    );
    update_sizes();
}

/**
 *  Implements the on-size-allocate event callback.
 */

void
seqevent::on_size_allocate (Gtk::Allocation & a_r)
{
    Gtk::DrawingArea::on_size_allocate(a_r);
    m_window_x = a_r.get_width();
    m_window_y = a_r.get_height();
    update_sizes();
}

/**
 *  Implements the on-expose event callback.
 */

bool
seqevent::on_expose_event (GdkEventExpose * e)
{
    m_window->draw_drawable
    (
        m_gc, m_pixmap, e->area.x, e->area.y,
        e->area.x, e->area.y, e->area.width, e->area.height
    );
    draw_selection_on_window();
    return true;
}

/**
 *  Implements the on-button-press event callback.  It distinguishes
 *  between the Seq24 and Fruity varieties of mouse interaction.
 *
 *  Odd.  The fruity case fell through to the Seq24 case.  We will assume
 *  for now that this is correct.
 */

bool
seqevent::on_button_press_event (GdkEventButton * a_ev)
{
    bool result;
    switch (global_interactionmethod)
    {
    case e_fruity_interaction:
        result = m_fruity_interaction.on_button_press_event(a_ev, *this);

        /*
         * FALL THROUGH
         */

    case e_seq24_interaction:
        result = m_seq24_interaction.on_button_press_event(a_ev, *this);
        break;

    default:
        result = false;
        break;
    }
    return result;
}

/**
 *  Drops (adds) an event at the given tick. It sets the first byte
 *  properly for after-touch, program-change, channel-pressure, and
 *  pitch-wheel.  The type of event is determined by m_status.
 */

void
seqevent::drop_event (long a_tick)
{
    unsigned char status = m_status;
    unsigned char d0 = m_cc;
    unsigned char d1 = 0x40;
    if (m_status == EVENT_AFTERTOUCH)
        d0 = 0;

    if (m_status == EVENT_PROGRAM_CHANGE)
        d0 = 0;                             /* d0 == new patch */

    if (m_status == EVENT_CHANNEL_PRESSURE)
        d0 = 0x40;                          /* d0 == pressure */

    if (m_status == EVENT_PITCH_WHEEL)
        d0 = 0;

    m_seq->add_event(a_tick, status, d0, d1, true);
}

/**
 *  Implements the on-button-release event callback.  It distinguishes
 *  between the Seq24 and Fruity varieties of mouse interaction.
 *
 *  Odd.  The fruity case fell through to the Seq24 case.  We will assume
 *  for now that this is correct.
 */

bool
seqevent::on_button_release_event (GdkEventButton * a_ev)
{
    bool result;

    switch (global_interactionmethod)
    {
    case e_fruity_interaction:
        result = m_fruity_interaction.on_button_release_event(a_ev, *this);

        /*
         * FALL THROUGH
         */

    case e_seq24_interaction:
        result = m_seq24_interaction.on_button_release_event(a_ev, *this);
        break;

    default:
        result = false;
        break;
    }
    return result;
}

/**
 *  Implements the on-motion-notify event callback.  It distinguishes
 *  between the Seq24 and Fruity varieties of mouse interaction.
 *
 *  Odd.  The fruity case fell through to the Seq24 case.  We will assume
 *  for now that this is correct.
 */

bool
seqevent::on_motion_notify_event (GdkEventMotion * a_ev)
{
    bool result;

    switch (global_interactionmethod)
    {
    case e_fruity_interaction:
        result = m_fruity_interaction.on_motion_notify_event(a_ev, *this);

        /*
         * FALL THROUGH
         */

    case e_seq24_interaction:
        result = m_seq24_interaction.on_motion_notify_event(a_ev, *this);
        break;

    default:
        result = false;
        break;
    }
    return result;
}

/**
 *  Responds to a focus event by setting the HAS_FOCUS flag.
 */

bool
seqevent::on_focus_in_event (GdkEventFocus *)
{
    set_flags(Gtk::HAS_FOCUS);
    return false;
}

/**
 *  Responds to a unfocus event by resetting the HAS_FOCUS flag.
 */

bool
seqevent::on_focus_out_event (GdkEventFocus *)
{
    unset_flags(Gtk::HAS_FOCUS);
    return false;
}

/**
 *  Implements the key-press event callback function.
 *
 *  It handles deleted a selection via the Backspace or Delete keys,
 *  cut via Ctrl-X, copy via Ctrl-C, paste via Ctrl-V, and undo via
 *  Ctrl-Z.
 *
 *  Would be nice to provide redo functionality via Ctrl-Y.  :-)
 */

bool
seqevent::on_key_press_event (GdkEventKey * a_p0)
{
    bool result = false;
    if (a_p0->type == GDK_KEY_PRESS)
    {
        if (a_p0->keyval ==  GDK_Delete || a_p0->keyval == GDK_BackSpace)
        {
            m_seq->push_undo();
            m_seq->mark_selected();
            m_seq->remove_marked();
            result = true;
        }
        if (a_p0->state & GDK_CONTROL_MASK)
        {
            if (a_p0->keyval == GDK_x || a_p0->keyval == GDK_X) /* cut */
            {
                m_seq->copy_selected();
                m_seq->mark_selected();
                m_seq->remove_marked();
                result = true;
            }
            if (a_p0->keyval == GDK_c || a_p0->keyval == GDK_C) /* copy */
            {
                m_seq->copy_selected();
                result = true;
            }
            if (a_p0->keyval == GDK_v || a_p0->keyval == GDK_V) /* paste */
            {
                start_paste();
                result = true;
            }
            if (a_p0->keyval == GDK_z || a_p0->keyval == GDK_Z) /* Undo */
            {
                m_seq->pop_undo();
                result = true;
            }
        }
    }
    if (result)
    {
        redraw();
        m_seq->set_dirty();
    }
    return result;
}

}           // namespace seq64

/*
 * seqevent.cpp
 *
 * vim: sw=4 ts=4 wm=8 et ft=cpp
 */
