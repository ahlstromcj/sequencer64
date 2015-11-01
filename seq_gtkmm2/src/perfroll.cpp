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
 * \updates       2015-10-31
 * \license       GNU GPLv2 or above
 *
 */

#include <gtkmm/accelkey.h>
#include <gtkmm/adjustment.h>

#include "event.hpp"
#include "keystroke.hpp"
#include "fruityperfroll_input.hpp"     /* alternate mouse-input class  */
#include "perform.hpp"
#include "perfroll.hpp"
#include "perfroll_input.hpp"
#include "sequence.hpp"

namespace seq64
{

/**
 *  Principal constructor.
 */

perfroll::perfroll
(
    perform & p,
    Gtk::Adjustment & hadjust,
    Gtk::Adjustment & vadjust,
    int ppqn
) :
    gui_drawingarea_gtk2    (p, hadjust, vadjust, 10, 10),
    m_snap                  (0),
    m_ppqn                  (0),                            // set in the body
    m_page_factor           (PERFROLL_PAGE_FACTOR),         // 4096
    m_divs_per_beat         (PERFROLL_DIVS_PER_BEAT),       // 16
    m_ticks_per_bar         (0),                            // set in the body
    m_perf_scale_x          (c_perf_scale_x),               // 32 ticks per pixel
    m_names_y               (c_names_y),
    m_background_x          (c_perfroll_background_x),      // gets adjusted!
    m_size_box_w            (c_perfroll_size_box_w),        // 3
    m_size_box_click_w      (c_perfroll_size_box_click_w),  // 3+1, not yet used
    m_measure_length        (0),
    m_beat_length           (0),
    m_old_progress_ticks    (0),
    m_4bar_offset           (0),
    m_sequence_offset       (0),
    m_roll_length_ticks     (0),
    m_drop_tick             (0),
    m_drop_tick_trigger_offset (0),
    m_drop_sequence         (0),
    m_sequence_max          (c_max_sequence),
    m_sequence_active       (),                 // array, size c_max_sequence
    m_interaction           (nullptr),
    m_moving                (false),
    m_growing               (false),
    m_grow_direction        (false)
{
    set_ppqn(ppqn);
    for (int i = 0; i < m_sequence_max; ++i)
        m_sequence_active[i] = false;

    switch (global_interactionmethod)
    {
    case e_fruity_interaction:
        m_interaction = new FruityPerfInput;
        break;

    case e_seq24_interaction:
    default:
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
 *  Handles changes to the PPQN value in one place.
 *
 *  The m_ticks_per_bar member replaces the construct "c_ppqn * 16".  This
 *  construct is parts-per-quarter-note times 4 quarter notes times 4
 *  sixteenth notes in a bar.  (We think...)
 *
 *  The m_perf_scale_x member starts out at c_perf_scale_x, which is 32 ticks
 *  per pixel at the default tick rate of 192 PPQN.  We adjust this now.
 *  But note that this calculation still involves the c_perf_scale_x constant.
 */

void
perfroll::set_ppqn (int ppqn)
{
    if (ppqn > 0 || ppqn == SEQ64_USE_DEFAULT_PPQN)
    {
        m_ppqn = (ppqn == SEQ64_USE_DEFAULT_PPQN) ? global_ppqn : ppqn ;
        m_ticks_per_bar = m_ppqn * m_divs_per_beat;
        m_background_x = (m_ppqn * 4 * 16) / c_perf_scale_x;
        m_perf_scale_x = c_perf_scale_x * m_ppqn / c_ppqn;
    }
}

/**
 *  Changes the 4-bar horizontal offset member and queues up a draw
 *  operation.
 */

void
perfroll::change_horz ()
{
    if (m_4bar_offset != int(m_hadjust.get_value()))
    {
        m_4bar_offset = int(m_hadjust.get_value());
        queue_draw();
    }
}

/**
 *  Changes the 4-bar vertical offset member and queues up a draw
 *  operation.
 */

void
perfroll::change_vert ()
{
    if (m_sequence_offset != int(m_vadjust.get_value()))
    {
        m_sequence_offset = int(m_vadjust.get_value());
        queue_draw();
    }
}

/**
 *  Sets the roll-lengths ticks member.  First, it gets the largest trigger
 *  value among the active sequences.  Then it truncates this value to the
 *  nearest PPQN * 16 ticks. Then it adds PPQN * 4096 ticks.
 */

void
perfroll::init_before_show ()
{
    m_roll_length_ticks = perf().get_max_trigger();
    m_roll_length_ticks -= (m_roll_length_ticks % m_ticks_per_bar);
    m_roll_length_ticks += m_ppqn * m_page_factor;
}

/**
 *  Updates the sizes of various items.
 *
 * \note
 *      Trying to figure out what the 16 is.  So take the "bars-visible"
 *      calculation, the c_perf_scale_x value, assume that "ticks" is
 *      another name for "pulses", and assume that "beats" is a quarter note.
 *      Ignoring the numbers, the units come out to:
 *
\verbatim
                pixels * ticks / pixel
        bars = --------------------------------
                ticks / beat * beats / bar
\endverbatim
 *
 *      Thus, the 16 is a "beats per bar" or "beats per measure" value.
 *      This doesn't quite make sense, but there are 16 divisions per
 *      beat on the perfroll user-interface.  So for now we'll call it
 *      the latter, and make a variable called "m_divs_per_beat", see its
 *      definition in the class initializer list.
 */

void
perfroll::update_sizes ()
{
    int h_bars = m_roll_length_ticks / m_ticks_per_bar;
    int h_bars_visible = (m_window_x * m_perf_scale_x) / m_ticks_per_bar;
    int h_max_value = h_bars - h_bars_visible;
    m_hadjust.set_lower(0);
    m_hadjust.set_upper(h_bars);
    m_hadjust.set_page_size(h_bars_visible);
    m_hadjust.set_step_increment(1);
    m_hadjust.set_page_increment(1);
    if (m_hadjust.get_value() > h_max_value)
        m_hadjust.set_value(h_max_value);

    m_vadjust.set_lower(0);
    m_vadjust.set_upper(m_sequence_max);
    m_vadjust.set_page_size(m_window_y / m_names_y);
    m_vadjust.set_step_increment(1);
    m_vadjust.set_page_increment(1);

    int v_max_value = m_sequence_max - (m_window_y / m_names_y);
    if (m_vadjust.get_value() > v_max_value)
        m_vadjust.set_value(v_max_value);

    if (is_realized())
        m_pixmap = Gdk::Pixmap::create(m_window, m_window_x, m_window_y, -1);

    queue_draw();
}

/**
 *  Increments the value of m_roll_length_ticks by the PPQN * 512, then
 *  calls update_sizes().
 */

void
perfroll::increment_size ()
{
    m_roll_length_ticks += (m_ppqn * 512);
    update_sizes();
}

/**
 *  This function updates the background of the Performance roll.  This first
 *  thing done is to clear the background by painting it with a filled white
 *  rectangle.
 */

void
perfroll::fill_background_pixmap ()
{
    gint8 dash = 1;
    draw_rectangle(m_background, white(), 0, 0, m_background_x, m_names_y);
    m_gc->set_dashes(0, &dash, 1);
    set_line(Gdk::LINE_ON_OFF_DASH);
    draw_line(m_background, grey(), 0, 0, m_background_x, 0);

    int beats = m_measure_length / m_beat_length;
    m_gc->set_foreground(grey());
    for (int i = 0; i < beats; /* inc'd in body */) /* draw vertical lines   */
    {
        if (i == 0)
            set_line(Gdk::LINE_SOLID);
        else
            set_line(Gdk::LINE_ON_OFF_DASH);

        int beat_x = i * m_beat_length / m_perf_scale_x;
        draw_line                                   /* solid line, every beat */
        (
            m_background, beat_x, 0, beat_x, m_names_y
        );
        if (m_beat_length < m_ppqn / 2)             /* jump 2 if 16th notes   */
            i += (m_ppqn / m_beat_length);
        else
            ++i;
    }
    set_line(Gdk::LINE_SOLID);
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
    long tick = perf().get_tick();
    long tick_offset = m_4bar_offset * m_ticks_per_bar;
    int progress_x = (tick - tick_offset) / m_perf_scale_x;
    int old_progress_x = (m_old_progress_ticks - tick_offset) / m_perf_scale_x;
    draw_drawable                                       /* erase old line */
    (
        old_progress_x, 0, old_progress_x, 0, 1, m_window_y
    );
    draw_line(black(), progress_x, 0, progress_x, m_window_y);
    m_old_progress_ticks = tick;
}

/**
 *  Draws the given pattern/sequence on the given drawable area.
 *  Statement nesting from hell!
 */

void
perfroll::draw_sequence_on (/*Glib::RefPtr<Gdk::Drawable> a_draw,*/ int seqnum)
{
    if ((seqnum < m_sequence_max) && perf().is_active(seqnum))
    {
        long tick_offset = m_4bar_offset * m_ticks_per_bar;
        long x_offset = tick_offset / m_perf_scale_x;
        m_sequence_active[seqnum] = true;
        sequence * seq =  perf().get_sequence(seqnum);
        seq->reset_draw_trigger_marker();
        seqnum -= m_sequence_offset;

        long sequence_length = seq->get_length();
        int length_w = sequence_length / m_perf_scale_x;
        long tick_on;
        long tick_off;
        long offset;
        bool selected;

        while (seq->get_next_trigger(&tick_on, &tick_off, &selected, &offset))
        {
            if (tick_off > 0)
            {
                long x_on  = tick_on  / m_perf_scale_x;
                long x_off = tick_off / m_perf_scale_x;
                int w = x_off - x_on + 1;
                int x = x_on;
                int y = m_names_y * seqnum + 1;     // + 2
                int h = m_names_y - 2;                  // - 4
                x -= x_offset;          /* adjust to screen coordinates */

                /*
                 * Change a_draw to m_pixmap; later use the right function.
                 */

                draw_rectangle_on_pixmap(selected ? grey() : white(), x, y, w, h);
                draw_rectangle_on_pixmap(black(), x, y, w, h, false);
                draw_rectangle_on_pixmap
                (
                    black(), x, y, m_size_box_w, m_size_box_w, false
                );
                draw_rectangle_on_pixmap
                (
                    // m_pixmap, // black(), [done in previous call, tricky]
                    x + w - m_size_box_w, y + h - m_size_box_w,
                    m_size_box_w, m_size_box_w,
                    false
                );

                long tickmarker =           /* length marker first tick */
                (
                    tick_on - (tick_on % sequence_length) +
                    (offset % sequence_length) - sequence_length
                );
                while (tickmarker < tick_off)
                {
                    long tickmarker_x = (tickmarker / m_perf_scale_x) - x_offset;
                    if (tickmarker > tick_on)
                    {
                        draw_rectangle
                        (
                            m_pixmap, light_grey(), tickmarker_x, y + 4, 1, h - 8
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
                    m_gc->set_foreground(black());
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
                        int mny = m_names_y - 6;        // ????
                        int note_y =
                        (
                            mny - (mny * (note - lowest_note)) / height
                        ) + 1;
                        int tick_s_x =
                            ((tick_s * length_w) / length) + tickmarker_x;

                        int tick_f_x =
                            ((tick_f * length_w) / length) + tickmarker_x;

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
                            draw_line_on_pixmap
                            (
                                tick_s_x, y + note_y, tick_f_x, y + note_y
                            );
                        }
                    }
                    tickmarker += sequence_length;
                }
            }
        }
    }
}

/**
 *  Draws the given pattern/sequence background on the given drawable area.
 */

void perfroll::draw_background_on (int seqnum)
{
    long tick_offset = m_4bar_offset * m_ticks_per_bar;
    long first_measure = tick_offset / m_measure_length;
    long last_measure = first_measure +
        (m_window_x * m_perf_scale_x / m_measure_length) + 1;

    seqnum -= m_sequence_offset;

    int h = m_names_y;
    int y = h * seqnum;
    draw_rectangle_on_pixmap(white(), 0, y, m_window_x, h);
    m_gc->set_foreground(black());
    for (long i = first_measure; i < last_measure; ++i)
    {
        int x_pos = ((i * m_measure_length) - tick_offset) / m_perf_scale_x;
        m_pixmap->draw_drawable
        (
            m_gc, m_background, 0, 0, x_pos, y,
            m_background_x, m_names_y
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
    int y_f = m_window_y / m_names_y;
    for (int y = y_s; y <= y_f; y++)
    {
        int seq = y + m_sequence_offset;
        if (perf().is_dirty_perf(seq))
        {
            draw_background_on(seq);
            draw_sequence_on(seq);
            draw = true;
        }
    }
    if (draw)
        draw_drawable(0, 0, 0, 0, m_window_x, m_window_y);
}

/**
 *  Not quite sure what this draws yet.
 */

void
perfroll::draw_drawable_row
(
    long a_y
)
{
    int s = a_y / m_names_y;
    draw_drawable(0, s * m_names_y, 0, s * m_names_y, m_window_x, m_names_y);
}

/**
 *  Provides a very common sequence of calls used in perfroll_input.
 */

void
perfroll::draw_all ()
{
    draw_background_on(m_drop_sequence);
    draw_sequence_on(m_drop_sequence);

    /*
     * We can replace this with stock code.
     */

    draw_drawable_row(m_drop_y);
}

/**
 *  Start the performance playing.  We need to keep in sync with
 *  perfedit's start_playing()... wish we could call it directly.
 *  Well, now we go to the source, calling perform::start_playing().
 */

void
perfroll::start_playing ()
{
    perf().start_playing(true);
}

/**
 *  Stop the performance playing.  We need to keep in sync with
 *  perfedit's stop_playing()... wish we could call it directly.
 *  Well, now we go to the source, calling perform::stop_playing().
 */

void
perfroll::stop_playing ()
{
    perf().stop_playing();
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
    gui_drawingarea_gtk2::on_realize();
    set_flags(Gtk::CAN_FOCUS);
    update_sizes();
    m_hadjust.signal_value_changed().connect
    (
        mem_fun(*this, &perfroll::change_horz)
    );
    m_vadjust.signal_value_changed().connect
    (
        mem_fun(*this, &perfroll::change_vert)
    );
    m_background = Gdk::Pixmap::create
    (
        m_window, m_background_x, m_names_y, -1
    );
    fill_background_pixmap(); /* fill the background (dotted lines n' such) */
}

/**
 *  Handles the on-expose event.
 */

bool
perfroll::on_expose_event (GdkEventExpose * ev)
{
    int y_s = ev->area.y / m_names_y;
    int y_f = (ev->area.y + ev->area.height) / m_names_y;
    for (int y = y_s; y <= y_f; ++y)
    {
        draw_background_on(y + m_sequence_offset);
        draw_sequence_on(y + m_sequence_offset);
    }
    m_window->draw_drawable
    (
        m_gc, m_pixmap, ev->area.x, ev->area.y,
        ev->area.x, ev->area.y, ev->area.width, ev->area.height
    );
    return true;
}

/**
 *  This callback function handles a button press by forwarding it to the
 *  interaction object's button-press function.  This gives us Seq24
 *  versus Fruity behavior.
 */

bool
perfroll::on_button_press_event (GdkEventButton * ev)
{
    return m_interaction->on_button_press_event(ev, *this);
}

/**
 *  This callback function handles a button release by forwarding it to the
 *  interaction object's button-release function.  This gives us Seq24
 *  versus Fruity behavior.
 */

bool
perfroll::on_button_release_event (GdkEventButton * ev)
{
    return m_interaction->on_button_release_event(ev, *this);
}

/**
 *  Handles horizontal and vertical scrolling.
 */

bool
perfroll::on_scroll_event (GdkEventScroll * ev)
{
    guint modifiers;                /* used to filter out caps/num lock etc. */
    modifiers = gtk_accelerator_get_default_mod_mask();
    if ((ev->state & modifiers) == SEQ64_SHIFT_MASK)
    {
        double val = m_hadjust.get_value();
        if (CAST_EQUIVALENT(ev->direction, SEQ64_SCROLL_UP))
            val -= m_hadjust.get_step_increment();
        else if (CAST_EQUIVALENT(ev->direction, SEQ64_SCROLL_DOWN))
            val += m_hadjust.get_step_increment();

        m_hadjust.clamp_page(val, val + m_hadjust.get_page_size());
    }
    else
    {
        double val = m_vadjust.get_value();
        if (CAST_EQUIVALENT(ev->direction, SEQ64_SCROLL_UP))
            val -= m_vadjust.get_step_increment();
        else if (CAST_EQUIVALENT(ev->direction, SEQ64_SCROLL_DOWN))
            val += m_vadjust.get_step_increment();

        m_vadjust.clamp_page(val, val + m_vadjust.get_page_size());
    }
    return true;
}

/**
 *  Handles motion notification by forwarding it to the interaction
 *  object's motion-notification callback function.
 */

bool
perfroll::on_motion_notify_event (GdkEventMotion * ev)
{
    return m_interaction->on_motion_notify_event(ev, *this);
}

/**
 *  This callback function handles a key-press event.  If we don't check the
 *  event type first, then the ev->keyval value is something weird like 65507.
 */

bool
perfroll::on_key_press_event (GdkEventKey * ev)
{
    keystroke k(ev->keyval, SEQ64_KEYSTROKE_PRESS, ev->state);
    bool result = perf().perfroll_key_event(k, m_drop_sequence);
    if (result)
    {
        fill_background_pixmap();
        queue_draw();
    }
    return result;
}

/**
 *  This function performs a 'snap' action on x.
 *
 *      -   m_snap = number pulses to snap to
 *      -   m_perf_scale_x = number of pulses per pixel
 *
 *  Therefore mod = m_snap/m_perf_scale_x equals the number pixels to snap
 *  to.
 */

void
perfroll::snap_x (int & x)
{
    int mod = m_snap / m_perf_scale_x;
    if (mod <= 0)
        mod = 1;

    x -= (x % mod);
}

/**
 *  Converts a tick-offset on the x coordinate.
 *
 *  The result is returned via the a_tick parameter.
 */

void
perfroll::convert_x (int x, long & tick)
{
    long tick_offset = m_4bar_offset * m_ticks_per_bar;
    tick = x * m_perf_scale_x + tick_offset;
}

/**
 *  Converts a tick-offset....
 *
 *  The results are returned via the a_tick and a_seq parameters.
 */

void
perfroll::convert_xy (int x, int y, long & a_tick, int & a_seq)
{
    long tick_offset = m_4bar_offset * m_ticks_per_bar;
    long tick = x * m_perf_scale_x + tick_offset;
    int seq = y / m_names_y + m_sequence_offset;
    if (seq >= m_sequence_max)
        seq = m_sequence_max - 1;
    else if (seq < 0)
        seq = 0;

    a_tick = tick;
    a_seq = seq;
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
perfroll::on_size_allocate (Gtk::Allocation & a)
{
    gui_drawingarea_gtk2::on_size_allocate(a);
    m_window_x = a.get_width();             /* side-effect  */
    m_window_y = a.get_height();            /* side-effect  */
    update_sizes();
}

/**
 *  Splits a trigger, whatever than means.
 */

void
perfroll::split_trigger (int seqnum, long a_tick)
{
    perf().push_trigger_undo();
    perf().get_sequence(seqnum)->split_trigger(a_tick);
    draw_background_on(seqnum);
    draw_sequence_on(seqnum);
    draw_drawable_row(m_drop_y);
}

}           // namespace seq64

/*
 * perfroll.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

