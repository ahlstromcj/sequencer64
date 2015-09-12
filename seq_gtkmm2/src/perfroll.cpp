/*
 *  This file is part of seq24/sequencer64.
 *
 *  seq24 is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General)mm Public License as published by
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
 * \file          perfroll.cpp
 *
 *  This module declares/defines the base class for the Performance window
 *  piano roll.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2015-09-11
 * \license       GNU GPLv2 or above
 *
 */

#include <gtkmm/accelkey.h>

#include "event.hpp"
#include "perform.hpp"
#include "perfroll.hpp"
#include "perfroll_input.hpp"
#include "sequence.hpp"

/**
 *  Principal constructor.
 */

perfroll::perfroll
(
    perform * a_perf,
    Gtk::Adjustment * a_hadjust,
    Gtk::Adjustment * a_vadjust
) :
    Gtk::DrawingArea    (),
    m_gc                (),
    m_window            (),
    m_black             (Gdk::Color("black")),
    m_white             (Gdk::Color("white")),
    m_grey              (Gdk::Color("grey")),
    m_lt_grey           (Gdk::Color("light grey")),
    m_pixmap            (),
    m_background        (),
    m_mainperf          (a_perf),
    m_window_x          (0),
    m_window_y          (0),
    m_drop_x            (0),
    m_drop_y            (0),
    m_vadjust           (a_vadjust),
    m_hadjust           (a_hadjust),
    m_snap              (0),
    m_measure_length    (0),
    m_beat_length       (0),
    m_old_progress_ticks(0),
    m_4bar_offset       (0),
    m_sequence_offset   (0),
    m_roll_length_ticks (0),
    m_drop_tick         (0),
    m_drop_tick_trigger_offset (0),
    m_drop_sequence     (0),
    m_sequence_active   (),             // array
    m_interaction       (nullptr),
    m_moving            (false),
    m_growing           (false),
    m_grow_direction    (false)
{
    Glib::RefPtr<Gdk::Colormap> colormap = get_default_colormap();
    colormap->alloc_color(m_black);
    colormap->alloc_color(m_white);
    colormap->alloc_color(m_grey);
    colormap->alloc_color(m_lt_grey);

    // IDEA: m_text_font_6_12 = Gdk_Font(c_font_6_12);

    add_events
    (
        Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK |
        Gdk::POINTER_MOTION_MASK | Gdk::KEY_PRESS_MASK |
        Gdk::KEY_RELEASE_MASK | Gdk::FOCUS_CHANGE_MASK | Gdk::SCROLL_MASK
    );
    set_size_request(10, 10);           // source of these constants?
    set_double_buffered(false);
    for (int i = 0; i < c_max_sequence; ++i)
        m_sequence_active[i] = false;

    switch (global_interactionmethod)
    {
    case e_fruity_interaction:
        m_interaction = new FruityPerfInput;
        break;

    default:
    case e_seq24_interaction:
        m_interaction = new Seq24PerfInput;
        break;
    }
}

/**
 *  This destructor deletes the interaction object.
 */

perfroll::~perfroll ()
{
    if (not_nullptr(m_interaction))
        delete m_interaction;
}

/**
 *  Changes the 4-bar horizontal offset member and queues up a draw
 *  operation.
 */

void
perfroll::change_horz ()
{
    if (m_4bar_offset != (int) m_hadjust->get_value())
    {
        m_4bar_offset = (int) m_hadjust->get_value();
        queue_draw();
    }
}

/**
 *  Changes the 4-bar vertical offset member and queues up a draw
 *  operation.
 */

void
perfroll::change_vert()
{
    if (m_sequence_offset != (int) m_vadjust->get_value())
    {
        m_sequence_offset = (int) m_vadjust->get_value();
        queue_draw();
    }
}

/**
 *  Sets the roll-lengths ticks member.
 */

void
perfroll::init_before_show()
{
    m_roll_length_ticks = m_mainperf->get_max_trigger();
    m_roll_length_ticks -= (m_roll_length_ticks % (c_ppqn * 16));
    m_roll_length_ticks += c_ppqn * 4096;
}

/**
 *  Updates the sizes of various items.
 */

void
perfroll::update_sizes()
{
    int h_bars = m_roll_length_ticks / (c_ppqn * 16);
    int h_bars_visable = (m_window_x * c_perf_scale_x) / (c_ppqn * 16);
    int h_max_value = h_bars - h_bars_visable;
    m_hadjust->set_lower(0);
    m_hadjust->set_upper(h_bars);
    m_hadjust->set_page_size(h_bars_visable);
    m_hadjust->set_step_increment(1);
    m_hadjust->set_page_increment(1);
    if (m_hadjust->get_value() > h_max_value)
    {
        m_hadjust->set_value(h_max_value);
    }
    m_vadjust->set_lower(0);
    m_vadjust->set_upper(c_max_sequence);
    m_vadjust->set_page_size(m_window_y / c_names_y);
    m_vadjust->set_step_increment(1);
    m_vadjust->set_page_increment(1);

    int v_max_value = c_max_sequence - (m_window_y / c_names_y);
    if (m_vadjust->get_value() > v_max_value)
        m_vadjust->set_value(v_max_value);

    if (is_realized())
        m_pixmap = Gdk::Pixmap::create(m_window, m_window_x, m_window_y, -1);

    queue_draw();
}

/**
 *  Increments the value of m_roll_length_ticks by the PPQN * 512, then
 *  calls update_sizes().
 */

void
perfroll::increment_size()
{
    m_roll_length_ticks += (c_ppqn * 512);
    update_sizes();
}

/**
 *  This function updates the background of the Performance roll.
 */

void
perfroll::fill_background_pixmap()
{
    m_gc->set_foreground(m_white);                  /* clear background */
    m_background->draw_rectangle
    (
        m_gc, true, 0, 0, c_perfroll_background_x, c_names_y
    );
    m_gc->set_foreground(m_grey);                   /* draw horz grey lines */
    gint8 dash = 1;
    m_gc->set_dashes(0, &dash, 1);
    m_gc->set_line_attributes
    (
        1, Gdk::LINE_ON_OFF_DASH, Gdk::CAP_NOT_LAST, Gdk::JOIN_MITER
    );
    m_background->draw_line
    (
        m_gc, 0, 0, c_perfroll_background_x, 0
    );
    int beats = m_measure_length / m_beat_length;
    for (int i = 0; i < beats ;)                    /* draw vertical lines   */
    {
        if (i == 0)
        {
            m_gc->set_line_attributes
            (
                1, Gdk::LINE_SOLID, Gdk::CAP_NOT_LAST, Gdk::JOIN_MITER
            );
        }
        else
        {
            m_gc->set_line_attributes
            (
                1, Gdk::LINE_ON_OFF_DASH, Gdk::CAP_NOT_LAST, Gdk::JOIN_MITER
            );
        }
        m_gc->set_foreground(m_grey);
        m_background->draw_line                     /* solid line, every beat */
        (
            m_gc, i * m_beat_length / c_perf_scale_x,
            0,    i * m_beat_length / c_perf_scale_x, c_names_y
        );
        if (m_beat_length < c_ppqn / 2)             /* jump 2 if 16th notes   */
            i += (c_ppqn / m_beat_length);
        else
            ++i;
    }
    m_gc->set_line_attributes                       /* reset line style       */
    (
        1, Gdk::LINE_SOLID, Gdk::CAP_NOT_LAST, Gdk::JOIN_MITER
    );
}

/**
 *  This function sets the snap, measure, and beats members, fills in
 *  the background, and queues up a draw operation.
 */

void
perfroll::set_guides (int a_snap, int a_measure, int a_beat)
{
    m_snap = a_snap;
    m_measure_length = a_measure;
    m_beat_length = a_beat;
    if (is_realized())
        fill_background_pixmap();

    queue_draw();
}

/**
 *  Draws the progess line that shows where we are in the performance.
 */

void
perfroll::draw_progress ()
{
    long tick = m_mainperf->get_tick();
    long tick_offset = m_4bar_offset * c_ppqn * 16;
    int progress_x = (tick - tick_offset) / c_perf_scale_x;
    int old_progress_x = (m_old_progress_ticks - tick_offset) / c_perf_scale_x;
    m_window->draw_drawable                                 /* draw old */
    (
        m_gc, m_pixmap, old_progress_x, 0, old_progress_x, 0, 1, m_window_y
    );
    m_gc->set_foreground(m_black);
    m_window->draw_line(m_gc, progress_x, 0, progress_x, m_window_y);
    m_old_progress_ticks = tick;
}

/**
 *  Draws the given pattern/sequence on the given drawable area.
 *  Statement nesting from hell!
 */

void
perfroll::draw_sequence_on (Glib::RefPtr<Gdk::Drawable> a_draw, int a_sequence)
{
    if ((a_sequence < c_max_sequence) && m_mainperf->is_active(a_sequence))
    {
        long tick_offset = m_4bar_offset * c_ppqn * 16;
        long x_offset = tick_offset / c_perf_scale_x;
        m_sequence_active[a_sequence] = true;
        sequence * seq =  m_mainperf->get_sequence(a_sequence);
        seq->reset_draw_trigger_marker();
        a_sequence -= m_sequence_offset;

        long sequence_length = seq->get_length();
        int length_w = sequence_length / c_perf_scale_x;
        long tick_on;
        long tick_off;
        long offset;
        bool selected;

        while (seq->get_next_trigger(&tick_on, &tick_off, &selected, &offset))
        {
            if (tick_off > 0)
            {
                long x_on  = tick_on  / c_perf_scale_x;
                long x_off = tick_off / c_perf_scale_x;
                int w = x_off - x_on + 1;
                int x = x_on;
                int y = c_names_y * a_sequence + 1;     // + 2
                int h = c_names_y - 2;                  // - 4
                x -= x_offset;          /* adjust to screen coordinates */
                if (selected)
                    m_gc->set_foreground(m_grey);
                else
                    m_gc->set_foreground(m_white);

                a_draw->draw_rectangle
                (
                    m_gc, true, x, y, w, h
                );
                m_gc->set_foreground(m_black);
                a_draw->draw_rectangle
                (
                    m_gc, false, x, y, w, h
                );

                m_gc->set_foreground(m_black);
                a_draw->draw_rectangle
                (
                    m_gc, false, x, y,
                    c_perfroll_size_box_w, c_perfroll_size_box_w    // ?
                );
                a_draw->draw_rectangle
                (
                    m_gc, false,
                    x + w - c_perfroll_size_box_w,
                    y + h - c_perfroll_size_box_w,
                    c_perfroll_size_box_w, c_perfroll_size_box_w    // ?
                );
                m_gc->set_foreground(m_black);

                long length_marker_first_tick =
                (
                    tick_on - (tick_on % sequence_length) +
                    (offset % sequence_length) - sequence_length
                );
                long tick_marker = length_marker_first_tick;
                while (tick_marker < tick_off)
                {
                    long tick_marker_x =
                        (tick_marker / c_perf_scale_x) - x_offset;

                    if (tick_marker > tick_on)
                    {
                        m_gc->set_foreground(m_lt_grey);
                        a_draw->draw_rectangle
                        (
                            m_gc, true, tick_marker_x, y + 4, 1, h - 8
                        );
                    }

                    int lowest_note = seq->get_lowest_note_event();
                    int highest_note = seq->get_highest_note_event();
                    int height = highest_note - lowest_note + 2;
                    int length = seq->get_length();
                    long tick_s;
                    long tick_f;
                    int note;
                    bool selected;
                    int velocity;
                    draw_type dt;

                    seq->reset_draw_marker();
                    m_gc->set_foreground(m_black);
                    while
                    (
                        (
                            dt = seq->get_next_note_event
                            (
                                &tick_s, &tick_f, &note, &selected, &velocity
                            )
                        ) != DRAW_FIN
                    )
                    {
                        int note_y =
                        (
                            (c_names_y-6) -
                            ((c_names_y-6) * (note-lowest_note)) / height
                        ) + 1;
                        int tick_s_x =
                            ((tick_s * length_w)  / length) + tick_marker_x;

                        int tick_f_x =
                            ((tick_f * length_w)  / length) + tick_marker_x;

                        if (dt == DRAW_NOTE_ON || dt == DRAW_NOTE_OFF)
                            tick_f_x = tick_s_x + 1;

                        if (tick_f_x <= tick_s_x)
                            tick_f_x = tick_s_x + 1;

                        if (tick_s_x < x)
                            tick_s_x = x;

                        if (tick_f_x > x + w)
                            tick_f_x = x + w;

                        if (tick_f_x >= x && tick_s_x <= x + w)
                        {
                            m_pixmap->draw_line
                            (
                                m_gc, tick_s_x, y + note_y,
                                tick_f_x, y + note_y
                            );
                        }
                    }
                    tick_marker += sequence_length;
                }
            }
        }
    }
}

/**
 *  Draws the given pattern/sequence background on the given drawable area.
 */

void perfroll::draw_background_on
(
    Glib::RefPtr<Gdk::Drawable> a_draw,
    int a_sequence
)
{
    long tick_offset = m_4bar_offset * c_ppqn * 16;
    long first_measure = tick_offset / m_measure_length;
    a_sequence -= m_sequence_offset;

    int y = c_names_y * a_sequence;
    int h = c_names_y;
    m_gc->set_foreground(m_white);
    a_draw->draw_rectangle(m_gc, true, 0, y, m_window_x, h);
    m_gc->set_foreground(m_black);
    for
    (
        int i = first_measure;
        i < first_measure + (m_window_x*c_perf_scale_x/m_measure_length) + 1;
        i++
    )
    {
        int x_pos = ((i * m_measure_length) - tick_offset) / c_perf_scale_x;
        a_draw->draw_drawable
        (
            m_gc, m_background, 0, 0, x_pos, y,
            c_perfroll_background_x, c_names_y
        );
    }
}

/**
 *  Redraws patterns/sequences that have been modified.
 */

void
perfroll::redraw_dirty_sequences ()
{
    bool draw = false;
    int y_s = 0;
    int y_f = m_window_y / c_names_y;
    for (int y = y_s; y <= y_f; y++)
    {
        int seq = y + m_sequence_offset;
        if (m_mainperf->is_dirty_perf(seq))
        {
            draw_background_on(m_pixmap, seq);
            draw_sequence_on(m_pixmap, seq);
            draw = true;
        }
    }
    if (draw)
    {
        m_window->draw_drawable
        (
            m_gc, m_pixmap, 0, 0, 0, 0, m_window_x, m_window_y
        );
    }
}

/**
 *  Not quite sure what this draws yet.
 */

void
perfroll::draw_drawable_row
(
    Glib::RefPtr<Gdk::Drawable> a_dest,
    Glib::RefPtr<Gdk::Drawable> a_src,
    long a_y
)
{
    int s = a_y / c_names_y;
    a_dest->draw_drawable
    (
        m_gc, a_src, 0, s*c_names_y, 0, s*c_names_y, m_window_x, c_names_y
    );
}

/**
 *  Start the performance playing.  We need to keep in sync with
 *  perfedit's start_playing()... wish we could call it directly.
 */

void
perfroll::start_playing ()
{
    m_mainperf->position_jack(true);
    m_mainperf->start_jack();
    m_mainperf->start(true);
}

/**
 *  Stop the performance playing.  We need to keep in sync with
 *  perfedit's stop_playing()... wish we could call it directly.
 */

void
perfroll::stop_playing ()
{
    m_mainperf->stop_jack();
    m_mainperf->stop();
}

/**
 *  Provides the on-realization callback.  Calls the base-class version
 *  first.
 *
 *  Then it allocates the additional resources need, that couldn't be
 *  initialized in the constructor, and makes some connections.
 */

void
perfroll::on_realize ()
{
    Gtk::DrawingArea::on_realize();
    set_flags(Gtk::CAN_FOCUS);
    m_window = get_window();
    m_gc = Gdk::GC::create(m_window);
    m_window->clear();
    update_sizes();
    m_hadjust->signal_value_changed().connect
    (
        mem_fun(*this, &perfroll::change_horz)
    );
    m_vadjust->signal_value_changed().connect
    (
        mem_fun(*this, &perfroll::change_vert)
    );
    m_background = Gdk::Pixmap::create
    (
        m_window, c_perfroll_background_x, c_names_y, -1
    );
    fill_background_pixmap(); /* fill the background (dotted lines n' such) */
}

/**
 *  Handles the on-expose event.
 */

bool
perfroll::on_expose_event (GdkEventExpose * e)
{
    int y_s = e->area.y / c_names_y;
    int y_f = (e->area.y  + e->area.height) / c_names_y;
    for (int y = y_s; y <= y_f; y++)
    {
        /*
        for ( int x=x_s; x<=x_f; x++ )
        {
            m_pixmap->draw_drawable
            (
                m_gc, m_background, 0, 0, x * c_perfroll_background_x,
                c_names_y * y, c_perfroll_background_x, c_names_y
            );
        }
         */

        draw_background_on(m_pixmap, y + m_sequence_offset);
        draw_sequence_on(m_pixmap, y + m_sequence_offset);
    }
    m_window->draw_drawable
    (
        m_gc, m_pixmap, e->area.x, e->area.y,
        e->area.x, e->area.y, e->area.width, e->area.height
    );
    return true;
}

/**
 *  This callback function handles a button press by forwarding it to the
 *  interaction object's button-press function.  This gives us Seq24
 *  versus Fruity behavior.
 */

bool
perfroll::on_button_press_event (GdkEventButton * a_ev)
{
    return m_interaction->on_button_press_event(a_ev, *this);
}

/**
 *  This callback function handles a button release by forwarding it to the
 *  interaction object's button-release function.  This gives us Seq24
 *  versus Fruity behavior.
 */

bool
perfroll::on_button_release_event (GdkEventButton * a_ev)
{
    return m_interaction->on_button_release_event(a_ev, *this);
}

/**
 *  Handles horizontal and vertical scrolling.
 */

bool
perfroll::on_scroll_event (GdkEventScroll * a_ev)
{
    guint modifiers;                /* used to filter out caps/num lock etc. */
    modifiers = gtk_accelerator_get_default_mod_mask();
    if ((a_ev->state & modifiers) == GDK_SHIFT_MASK)
    {
        double val = m_hadjust->get_value();
        if (a_ev->direction == GDK_SCROLL_UP)
            val -= m_hadjust->get_step_increment();
        else if (a_ev->direction == GDK_SCROLL_DOWN)
            val += m_hadjust->get_step_increment();

        m_hadjust->clamp_page(val, val + m_hadjust->get_page_size());
    }
    else
    {
        double val = m_vadjust->get_value();
        if (a_ev->direction == GDK_SCROLL_UP)
            val -= m_vadjust->get_step_increment();
        else if (a_ev->direction == GDK_SCROLL_DOWN)
            val += m_vadjust->get_step_increment();

        m_vadjust->clamp_page(val, val + m_vadjust->get_page_size());
    }
    return true;
}

/**
 *  Handles motion notification by forwarding it to the interaction
 *  object's motion-notification callback function.
 */

bool
perfroll::on_motion_notify_event (GdkEventMotion * a_ev)
{
    return m_interaction->on_motion_notify_event(a_ev, *this);
}

/**
 *  This callback function handles a key-press event.
 */

bool
perfroll::on_key_press_event (GdkEventKey * a_p0)
{
    bool result = false;
    if (m_mainperf->is_active(m_drop_sequence))
    {
        if (a_p0->type == GDK_KEY_PRESS)
        {
            if (a_p0->keyval ==  GDK_Delete || a_p0->keyval == GDK_BackSpace)
            {
                m_mainperf->push_trigger_undo();
                m_mainperf->get_sequence(m_drop_sequence)->del_selected_trigger();
                result = true;
            }
            if (a_p0->state & GDK_CONTROL_MASK)
            {
                if (a_p0->keyval == GDK_x || a_p0->keyval == GDK_X) /* cut */
                {
                    m_mainperf->push_trigger_undo();
                    m_mainperf->get_sequence(m_drop_sequence)->
                        cut_selected_trigger();

                    result = true;
                }
                if (a_p0->keyval == GDK_c || a_p0->keyval == GDK_C) /* copy */
                {
                    m_mainperf->get_sequence(m_drop_sequence)->
                        copy_selected_trigger();

                    result = true;
                }
                if (a_p0->keyval == GDK_v || a_p0->keyval == GDK_V) /* paste */
                {
                    m_mainperf->push_trigger_undo();
                    m_mainperf->get_sequence(m_drop_sequence)->paste_trigger();
                    result = true;
                }
            }
        }
    }
    if (result)
    {
        fill_background_pixmap();
        queue_draw();
        return true;
    }
    else
        return false;
}

/**
 *  This function performs a 'snap' action on x.
 *
 *      -   m_snap = number pulses to snap to
 *      -   c_perf_scale_x = number of pulses per pixel
 *
 *  Therefore mod = m_snap/c_perf_scale_x equals the number pixels to snap
 *  to.
 */

void
perfroll::snap_x (int * a_x)
{
    int mod = m_snap / c_perf_scale_x;
    if (mod <= 0)
        mod = 1;

    *a_x = *a_x - (*a_x % mod);
}

/**
 *  Converts a tick-offset on the x coordinate.
 *
 *  The result is returned via the a_tick parameter.
 */

void
perfroll::convert_x (int a_x, long * a_tick)
{
    long tick_offset = m_4bar_offset * c_ppqn * 16;
    *a_tick = a_x * c_perf_scale_x + tick_offset;
}

/**
 *  Converts a tick-offset....
 *
 *  The results are returned via the a_tick and a_seq parameters.
 */

void
perfroll::convert_xy (int a_x, int a_y, long * a_tick, int * a_seq)
{
    long tick_offset = m_4bar_offset * c_ppqn * 16;
    long tick = a_x * c_perf_scale_x + tick_offset;
    int seq = a_y / c_names_y + m_sequence_offset;
    if (seq >= c_max_sequence)
        seq = c_max_sequence - 1;

    if (seq < 0)
        seq = 0;

    *a_tick = tick;
    *a_seq = seq;
}

/**
 *  This callback handles an in-focus event by setting the flag to
 *  HAS_FOCUS.
 */

bool
perfroll::on_focus_in_event (GdkEventFocus *)
{
    set_flags(Gtk::HAS_FOCUS);
    return false;
}

/**
 *  This callback handles an out-of-focus event by resetting the flag
 *  HAS_FOCUS.
 */

bool
perfroll::on_focus_out_event (GdkEventFocus *)
{
    unset_flags(Gtk::HAS_FOCUS);
    return false;
}

/**
 *  Upon a size allocation event, this callback calls the base-class
 *  version of this function, then sets m_window_x and m_window_y, and
 *  calls update_sizes().
 */

void
perfroll::on_size_allocate (Gtk::Allocation & a_r)
{
    Gtk::DrawingArea::on_size_allocate(a_r);
    m_window_x = a_r.get_width();
    m_window_y = a_r.get_height();
    update_sizes();
}

/**
 *  This callback throws away a size request.
 */

void
perfroll::on_size_request (GtkRequisition * a_r)
{
    // Empty body
}

/**
 *  Splits a trigger, whatever than means.
 */

void
perfroll::split_trigger (int a_sequence, long a_tick)
{
    m_mainperf->push_trigger_undo();
    m_mainperf->get_sequence(a_sequence)->split_trigger(a_tick);
    draw_background_on(m_pixmap, a_sequence);
    draw_sequence_on(m_pixmap, a_sequence);
    draw_drawable_row(m_window, m_pixmap, m_drop_y);
}

/*
 * perfroll.cpp
 *
 * vim: sw=4 ts=4 wm=8 et ft=cpp
 */
