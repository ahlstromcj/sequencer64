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
 * \updates       2015-11-26
 * \license       GNU GPLv2 or above
 *
 */

#include <gtkmm/accelkey.h>
#include <gtkmm/adjustment.h>

#include "app_limits.h"
#include "event.hpp"
#include "keystroke.hpp"
#include "fruityperfroll_input.hpp"     /* alternate mouse-input class  */
#include "perfedit.hpp"
#include "perform.hpp"
#include "perfroll.hpp"
#include "perfroll_input.hpp"
#include "sequence.hpp"

/**
 *  Static (private) convenience values.
 *  We need to be able to adjust s_perfroll_background_x per the selected PPQN
 *  value.  This adjustment is made in the constructor, and assigned to the
 *  perfroll::m_background_x member.  We need named values for 4 and for 16
 *  here.
 */

static int s_perfroll_background_x =
(
    (SEQ64_DEFAULT_PPQN * 4 * 16) / c_perf_scale_x
);
static int s_perfroll_size_box_w = 3;
static int s_perfroll_size_box_click_w = 4; /* s_perfroll_size_box_w + 1 */

namespace seq64
{

/**
 *  Principal constructor.
 */

perfroll::perfroll
(
    perform & p,
    perfedit & parent,
    Gtk::Adjustment & hadjust,
    Gtk::Adjustment & vadjust,
    int ppqn
) :
    gui_drawingarea_gtk2    (p, hadjust, vadjust, 10, 10),
    m_parent                (parent),
    m_h_page_increment      (usr().perf_h_page_increment()),
    m_v_page_increment      (usr().perf_v_page_increment()),
    m_snap                  (0),
    m_ppqn                  (0),                            // set in the body
    m_page_factor           (SEQ64_PERFROLL_PAGE_FACTOR),   // 4096
    m_divs_per_beat         (SEQ64_PERFROLL_DIVS_PER_BEAT), // 16
    m_ticks_per_bar         (0),                            // set in the body
    m_perf_scale_x          (c_perf_scale_x),               // 32 ticks per pixel
    m_names_y               (c_names_y),
    m_background_x          (s_perfroll_background_x),      // gets adjusted!
    m_size_box_w            (s_perfroll_size_box_w),        // 3
    m_size_box_click_w      (s_perfroll_size_box_click_w),  // 3+1, not yet used
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
    m_fruity_interaction    (),
    m_seq24_interaction     (),
    m_moving                (false),
    m_growing               (false),
    m_grow_direction        (false)
{
    set_ppqn(ppqn);
    for (int i = 0; i < m_sequence_max; ++i)
        m_sequence_active[i] = false;
}

/**
 *  This destructor deletes the interaction object.  Well, now there are two
 *  objects, so no explicit deletion necessary.
 */

perfroll::~perfroll ()
{
    // Empty body
}

/**
 *  Handles changes to the PPQN value in one place.
 *
 *  The m_ticks_per_bar member replaces the global ppqn times 16.  This
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
    if (ppqn_is_valid(ppqn))
    {
        m_ppqn = choose_ppqn(ppqn);
        m_ticks_per_bar = m_ppqn * m_divs_per_beat;
        m_background_x = (m_ppqn * 4 * 16) / c_perf_scale_x;
        m_perf_scale_x = c_perf_scale_x * m_ppqn / SEQ64_DEFAULT_PPQN;
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
        enqueue_draw();
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
        enqueue_draw();
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
    m_hadjust.set_page_increment(m_h_page_increment);
    if (m_hadjust.get_value() > h_max_value)
        m_hadjust.set_value(h_max_value);

    m_vadjust.set_lower(0);
    m_vadjust.set_upper(m_sequence_max);
    m_vadjust.set_page_size(m_window_y / m_names_y);
    m_vadjust.set_step_increment(1);
    m_vadjust.set_page_increment(m_v_page_increment);

    int v_max_value = m_sequence_max - (m_window_y / m_names_y);
    if (m_vadjust.get_value() > v_max_value)
        m_vadjust.set_value(v_max_value);

    if (is_realized())
        m_pixmap = Gdk::Pixmap::create(m_window, m_window_x, m_window_y, -1);

    enqueue_draw();
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
    draw_rectangle(m_background, white(), 0, 0, m_background_x, m_names_y);

#ifdef SEQ64_SOLID_PIANOROLL_GRID
    set_line(Gdk::LINE_SOLID);
    draw_line(m_background, light_grey(), 0, 0, m_background_x, 0);
#else
    gint8 dash = 1;
    m_gc->set_dashes(0, &dash, 1);
    set_line(Gdk::LINE_ON_OFF_DASH);
    draw_line(m_background, grey(), 0, 0, m_background_x, 0);
#endif

    int beats = m_measure_length / m_beat_length;
    m_gc->set_foreground(grey());
    for (int i = 0; i < beats; /* inc'd in body */) /* draw vertical lines  */
    {
#ifdef SEQ64_SOLID_PIANOROLL_GRID
        if (i == 0)
            m_gc->set_foreground(dark_grey());      /* was black()          */
        else
            m_gc->set_foreground(light_grey());
#else
        if (i == 0)
            set_line(Gdk::LINE_SOLID);
        else
            set_line(Gdk::LINE_ON_OFF_DASH);
#endif

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
 *  This function sets the m_snap, m_measure_length, and m_beat_length
 *  members directly from the function parameters, which are in units of
 *  pulses (sometimes misleadingly called "ticks".)
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
 *
 * \param beat
 *      Provides the number of beat-pulses (pulses per beat) as calculated in
 *      perfedit::set_guides().
 */

void
perfroll::set_guides (int snap, int measure, int beat)
{
    m_snap = snap;
    m_measure_length = measure;
    m_beat_length = beat;
    if (is_realized())
        fill_background_pixmap();

    enqueue_draw();
}

/**
 *  Wraps queue_draw() and forwards the call to the parent perfedit, so
 *  that it can forward it to any other perfedit that exists.
 *
 *  The parent perfedit will call perfroll::queue_draw() on behalf of this
 *  object, and it will pass a perfroll::enqueue_draw() to the peer perfedit's
 *  perfroll, if the peer exists.
 */

void
perfroll::enqueue_draw ()
{
    m_parent.enqueue_draw();
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
perfroll::draw_sequence_on (int seqnum)
{
    if ((seqnum < m_sequence_max) && perf().is_active(seqnum))
    {
        long tick_offset = m_4bar_offset * m_ticks_per_bar;
        long x_offset = tick_offset / m_perf_scale_x;
        m_sequence_active[seqnum] = true;
        sequence * seq = perf().get_sequence(seqnum);
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
                int y = m_names_y * seqnum + 1;         // + 2
                int h = m_names_y - 2;                  // - 4
                x -= x_offset;          /* adjust to screen coordinates */
                draw_rectangle_on_pixmap(selected ? grey() : white(), x, y, w, h);
                draw_rectangle_on_pixmap(black(), x, y, w, h, false);
                draw_rectangle_on_pixmap
                (
                    black(), x, y, m_size_box_w, m_size_box_w, false
                );
                draw_rectangle_on_pixmap
                (
                    // black(),             [done in previous call, tricky]
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
 *  Not quite sure what this draws yet.  It is involved in the drawing of a
 *  greyed (selected) row.
 *
 *  What's weird is that we divide y by m_names_y, then multiply it by
 *  m_names_y, before passing the result to draw_drawable().  However, if we
 *  just as y casted to an int, then the drawing of the row is only partial,
 *  vertically.
 */

void
perfroll::draw_drawable_row (long y)
{
    int s = y / m_names_y;
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
 *  Splits a trigger, whatever that means.
 */

void
perfroll::split_trigger (int seqnum, long tick)
{
    perf().split_trigger(seqnum, tick);     /* consolidates perform actions */
    draw_background_on(seqnum);
    draw_sequence_on(seqnum);
    draw_drawable_row(m_drop_y);
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
 *  The result is returned via the tick parameter.
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
 *  The results are returned via the d_tick and d_seq parameters.
 */

void
perfroll::convert_xy (int x, int y, long & d_tick, int & d_seq)
{
    long tick_offset = m_4bar_offset * m_ticks_per_bar;
    long tick = x * m_perf_scale_x + tick_offset;
    int seq = y / m_names_y + m_sequence_offset;
    if (seq >= m_sequence_max)
        seq = m_sequence_max - 1;
    else if (seq < 0)
        seq = 0;

    d_tick = tick;
    d_seq = seq;
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
 *
 * \return
 *      Always returns true.
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
 *
 *  One minor issue:  Fruity behavior doesn't yet provide the keystroke
 *  behavior we now handle for the Seq24 mode of operation.
 */

bool
perfroll::on_button_press_event (GdkEventButton * ev)
{
    bool result;
    if (rc().interaction_method() == e_seq24_interaction)
        result = m_seq24_interaction.on_button_press_event(ev, *this);
    else
        result = m_fruity_interaction.on_button_press_event(ev, *this);

    if (result)
        perf().modify();

    enqueue_draw();
    return result;
}

/**
 *  This callback function handles a button release by forwarding it to the
 *  interaction object's button-release function.  This gives us Seq24
 *  versus Fruity behavior.
 */

bool
perfroll::on_button_release_event (GdkEventButton * ev)
{
    bool result;
    if (rc().interaction_method() == e_seq24_interaction)
        result = m_seq24_interaction.on_button_release_event(ev, *this);
    else
        result = m_fruity_interaction.on_button_release_event(ev, *this);

    if (result)
        perf().modify();

    enqueue_draw();
    return result;
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
    bool result;
    if (rc().interaction_method() == e_seq24_interaction)
        result = m_seq24_interaction.on_motion_notify_event(ev, *this);
    else
        result = m_fruity_interaction.on_motion_notify_event(ev, *this);

    if (result)
    {
        perf().modify();
        enqueue_draw();     /* put in if() to reduce flickering */
    }
    return result;
}

/**
 *  This callback function handles a key-press event.  If we don't check the
 *  event type first, then the ev->keyval value is something weird like 65507.
 *  Note that we pass the functionality on to the
 *  perform::perfroll_key_event() function for the handling of delete, cut,
 *  copy, paste, and undo operations.  If the keystroke is not handled by that
 *  function, then we handle it here.
 *
 *  Note that only the Seq24 input interaction object handles additional
 *  keystrokes not handled by the perfroll_key_event() function.
 */

bool
perfroll::on_key_press_event (GdkEventKey * ev)
{
    keystroke k(ev->keyval, SEQ64_KEYSTROKE_PRESS, ev->state);
    bool result = perf().perfroll_key_event(k, m_drop_sequence);
    if (result)
    {
        // Nothing to do here
    }
    else
    {
        if (! perf().is_playing())
        {
            if (ev->keyval == SEQ64_p)
            {
                m_seq24_interaction.set_adding(true, *this);
                result = true;
            }
            else if (ev->keyval == SEQ64_x)         /* "x-scape" the mode   */
            {
                m_seq24_interaction.set_adding(false, *this);
                result = true;
            }
            else if (ev->keyval == SEQ64_Left)
            {
                if (m_seq24_interaction.is_adding())
                    result = m_seq24_interaction.handle_motion_key(true, *this);
            }
            else if (ev->keyval == SEQ64_Right)
            {
                if (m_seq24_interaction.is_adding())
                    result = m_seq24_interaction.handle_motion_key(false, *this);
            }
        }
    }
    if (result)
    {
        fill_background_pixmap();
        perf().modify();
    }
    else
        return Gtk::DrawingArea::on_key_press_event(ev);

    enqueue_draw();
    return result;
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

}           // namespace seq64

/*
 * perfroll.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

