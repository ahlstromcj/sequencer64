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
 * \updates       2018-03-01
 * \license       GNU GPLv2 or above
 *
 *  The performance window allows automatic control of when each
 *  sequence/pattern starts and stops, and thus offers a fixed-composition
 *  option, as opposed to live control of the sequences/patterns.
 *
 * Stazed:
 *
 *      Most of the undo/redo changes were done to eliminate pushes to undo
 *      when nothing actually changed, or to optimize the push in perfedit.
 *      When I [stazed] added the sensitive/insensitive stuff it became very
 *      obvious that undo was doing a lot of unnecessary pushes which
 *      previously seemed like undo was broken - from a user point of view.
 *      You would often have to hit the undo button many times before the undo
 *      occurred because useless undos were pushed from simply clicking on an
 *      open space in the perfedit.
 */

#include <gtkmm/accelkey.h>
#include <gtkmm/adjustment.h>

#include "app_limits.h"
#include "event.hpp"
#include "keystroke.hpp"
#include "fruityperfroll_input.hpp"     /* alternate mouse-input class  */
#include "gui_key_tests.hpp"            /* is_ctrl_key(), etc.          */
#include "perfedit.hpp"
#include "perform.hpp"
#include "perfroll.hpp"
#include "sequence.hpp"
#include "settings.hpp"                 /* seq64::rc() or seq64::usr()  */

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *  Static (private) convenience values.  We need to be able to adjust
 *  sm_perfroll_background_x per the selected PPQN value.  This adjustment is
 *  made in the constructor, and assigned to the perfroll::m_background_x
 *  member.
 *
 *  sm_perfroll_size_box_w is copied into the m_size_box_w member, and is the
 *  width of the small square handle in the corner of each trigger segment.
 *  It gets adjusted in perfroll::set_ppqn().
 */

int perfroll::sm_perfroll_size_box_w = 6;

/**
 *
 *  sm_perfroll_background_x is copied into m_background_x; it gets adjusted
 *  in perfroll::set_ppqn().
 */

int perfroll::sm_perfroll_background_x =
    (SEQ64_DEFAULT_PPQN * 4 * 16) / c_perf_max_zoom;        /* stazed */

/**
 *  Provides sizing information for the perfroll-derived classes, using in the
 *  Seq24PerfInput and FruityPerfInput classes.
 */

int perfroll::sm_perfroll_size_box_click_w = 4;

/**
 *  Principal constructor.
 *
 * \param p
 *      The perform object that this class will affect via user interaction.
 *
 * \param parent
 *      The perfedit object that holds this user-interface class.
 *
 * \param hadjust
 *      A horizontal adjustment object, used here to control scrolling.
 *
 * \param vadjust
 *      A vertical adjustment object, used here to control scrolling.
 *
 * \param ppqn
 *      The "resolution" of the MIDI file, used in zooming and scaling.
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
    m_adding                (false),
    m_adding_pressed        (false),
    m_h_page_increment      (usr().perf_h_page_increment()),
    m_v_page_increment      (usr().perf_v_page_increment()),
    m_snap_x                (0),
    m_snap_y                (0),
    m_ppqn                  (0),                            // set in the body
    m_page_factor           (SEQ64_PERFROLL_PAGE_FACTOR),   // 4096
    m_divs_per_beat         (SEQ64_PERFROLL_DIVS_PER_BEAT), // 16
    m_ticks_per_bar         (0),                            // set in the body
    m_perf_scale_x          (c_perf_scale_x),               // 32 ticks per pixel
    m_w_scale_x             (sm_perfroll_size_box_click_w * m_perf_scale_x),
    m_zoom                  (c_perf_scale_x),               // 32 ticks per pixel
    m_names_y               (c_names_y),
    m_background_x          (sm_perfroll_background_x),     // gets adjusted!
    m_size_box_w            (sm_perfroll_size_box_w),       // 6
    m_measure_length        (0),
    m_beat_length           (0),
    m_old_progress_ticks    (0),
#ifdef SEQ64_FOLLOW_PROGRESS_BAR
    m_scroll_page           (0),
#endif
    m_have_button_press     (false),                        // stazed
#ifdef USE_UNNECESSARY_TRANSPORT_FOLLOW_CALLBACK
    m_transport_follow      (true),
    m_trans_button_press    (false),
#endif
    m_4bar_offset           (0),                            // now a full offset
    m_sequence_offset       (0),
    m_roll_length_ticks     (0),
    m_drop_tick             (0),
    m_drop_tick_offset      (0),
    m_drop_sequence         (0),
    m_sequence_max          (c_max_sequence),
    m_sequence_active       (),                             // [c_max_sequence]
#ifdef SEQ64_SONG_BOX_SELECT
    m_old                   (),                             // seq64::rect
    m_selected              (),                             // seq64::rect
    m_box_select            (false),
    m_box_select_low        (SEQ64_NULL_SEQUENCE),
    m_box_select_high       (SEQ64_NULL_SEQUENCE),
    m_last_tick             (0),
    m_scroll_offset_x       (0),        // m_4bar_offset already in place!
    m_scroll_offset_y       (0),
#endif
    m_moving                (false),
    m_growing               (false),
    m_grow_direction        (false)
{
    set_ppqn(ppqn);                                         // choose_ppqn(ppqn)
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
 *
 * \todo
 *      Resolve the issue of c_perf_scale_x versus m_perf_scale_x in perfroll.
 */

void
perfroll::set_ppqn (int ppqn)
{
    if (ppqn_is_valid(ppqn))
    {
        m_ppqn = choose_ppqn(ppqn);
        m_ticks_per_bar = m_ppqn * m_divs_per_beat;             /* 16 */
        m_background_x = (m_ppqn * 4 * 16) / c_perf_scale_x;
        m_perf_scale_x = m_zoom * m_ppqn / SEQ64_DEFAULT_PPQN;
        m_w_scale_x = sm_perfroll_size_box_click_w * m_perf_scale_x;
        if (m_perf_scale_x == 0)
            m_perf_scale_x = 1;
    }
}

/**
 *  Changes the 4-bar horizontal offset member and queues up a draw
 *  operation.  Since the m_4bar_offset value is always multiplied by
 *  m_ticks_per_bar before usage, let's just do it here and not have to
 *  multiply it later.
 */

void
perfroll::change_horz ()
{
    long current_offset = long(m_hadjust.get_value()) * m_ticks_per_bar;
    if (m_4bar_offset != current_offset)
    {
#ifdef SEQ64_SONG_BOX_SELECT
        m_scroll_offset_x = int(m_hadjust.get_value()) / m_zoom;
#endif
        m_4bar_offset = current_offset;
        enqueue_draw();
    }
}

/**
 *  Changes the vertical offset member and queues up a draw operation.
 *
 * Stazed:
 *
 *      Must adjust m_drop_y or perfroll_input's unselect_triggers() will not
 *      work if scrolled up or down to a new location.  See the note in
 *      on_button_press_event() in the perfroll_input module.  Also see the
 *      note in the draw_all() function.
 */

void
perfroll::change_vert ()
{
    int vvalue = int(m_vadjust.get_value());
    if (m_sequence_offset != vvalue)
    {
        m_drop_y += (m_sequence_offset - vvalue) * m_names_y;
        m_sequence_offset = vvalue;
#ifdef SEQ64_SONG_BOX_SELECT
        m_scroll_offset_y = int(m_vadjust.get_value()) * m_names_y;
#endif
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
        bars = ----------------------------
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

    int vpagesize = m_window_y / m_names_y;
    m_vadjust.set_lower(0);
    m_vadjust.set_upper(m_sequence_max);
    m_vadjust.set_page_size(vpagesize);
    m_vadjust.set_step_increment(1);
    m_vadjust.set_page_increment(m_v_page_increment);

    int v_max_value = m_sequence_max - vpagesize;
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
 *  This function updates the background of the piano roll.  The first thing
 *  done is to clear the background by painting it with a filled white
 *  rectangle.
 *
 *  This function is called whenever something occurs (e.g. zoom) that can
 *  affect how the piano roll is drawn.
 */

void
perfroll::fill_background_pixmap ()
{
    draw_rectangle(m_background, white_paint(), 0, 0, m_background_x, m_names_y);

#ifdef SEQ64_SOLID_PIANOROLL_GRID
    set_line(Gdk::LINE_SOLID);
    draw_line(m_background, light_grey_paint(), 0, 0, m_background_x, 0);
#else
    gint8 dash = 1;
    m_gc->set_dashes(0, &dash, 1);
    set_line(Gdk::LINE_ON_OFF_DASH);
    draw_line(m_background, grey_paint(), 0, 0, m_background_x, 0);
#endif

    int beats = m_measure_length / m_beat_length;
    m_gc->set_foreground(grey_paint());
    for (int i = 0; i < beats; /* inc'd in body */) /* draw vertical lines  */
    {
#ifdef SEQ64_SOLID_PIANOROLL_GRID
        if (i == 0)
            m_gc->set_foreground(dark_grey_paint());      /* was black()          */
        else
            m_gc->set_foreground(light_grey_paint());
#else
        if (i == 0)
            set_line(Gdk::LINE_SOLID);
        else
            set_line(Gdk::LINE_ON_OFF_DASH);
#endif
        /*
         * Draw a solid vertical line, at every beat.
         */

        int beat_x = i * m_beat_length / m_perf_scale_x;
        draw_line(m_background, beat_x, 0, beat_x, m_names_y);
        if (m_beat_length < m_ppqn / 2)             /* jump 2 if 16th notes */
            i += (m_ppqn / m_beat_length);
        else
            ++i;
    }
    set_line(Gdk::LINE_SOLID);
}

/**
 *  This function sets the m_snap_x, m_measure_length, and m_beat_length
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
    m_snap_x = snap;
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
#ifdef SEQ64_SONG_BOX_SELECT
    if (m_box_select)
        draw_selection_on_window();
#endif
    m_parent.enqueue_draw();
}

/**
 *  Draws the progress line that shows where we are in the performance.
 *
 *  We would like to be able to leave the line there when the progress is
 *  paused while running off of JACK transport.  How?  The perf().get_tick()
 *  call always returns 0 when stop is in force.
 *
 *  If we comment out the erasure of the old line, we see that the progress
 *  bar is also erased when a pattern boundary is hit (triggers), and when the
 *  sequence is stopped by the user.
 *
 *  In order to support true pause in the song editor, we tried to replace
 *  perform::get_tick() with perform::get_start_tick() and
 *  perform::get_last_tick() [a new experimental function].  But those
 *  replacements here always return 0, even as perform::get_tick() increases.
 *  (Note that the draw_progress function is called at every timeout, that is,
 *  constantly.)
 */

void
perfroll::draw_progress ()
{
    midipulse tick = perf().get_tick();
    midipulse tick_offset = m_4bar_offset;
    int progress_x = (tick - tick_offset) / m_perf_scale_x;
    int old_progress_x = (m_old_progress_ticks - tick_offset) / m_perf_scale_x;
    if (usr().progress_bar_thick())
    {
        draw_drawable(old_progress_x-1, 0, old_progress_x-1, 0, 3, m_window_y);
        set_line(Gdk::LINE_SOLID, 2);
    }
    else
        draw_drawable(old_progress_x, 0, old_progress_x, 0, 1, m_window_y);

    /*
     * Stazed:
     *      draw_drawable(old_progress_x, 0, old_progress_x, 0, 2, m_window_y);
     *      set_line_attributes(2, Gdk::LINE_SOLID, Gdk::CAP_NOT_LAST,
     *          Gdk::JOIN_MITER);
     */

    draw_line(progress_color(), progress_x, 0, progress_x, m_window_y);
    if (usr().progress_bar_thick())
        set_line(Gdk::LINE_SOLID, 1);

    /*
     * Stazed:
     *      set_line_attributes(1, Gdk::LINE_SOLID, Gdk::CAP_NOT_LAST,
     *          Gdk::JOIN_MITER);
     */

    m_old_progress_ticks = tick;

#ifdef USE_STAZED_PERF_AUTO_SCROLL  // no longer needed, left here just in case
    auto_scroll_horz();             // but sequencer64 now does this anyway
#endif

}

#ifdef SEQ64_FOLLOW_PROGRESS_BAR

/**
 *  Checks the position of the tick, and, if it is in a different piano-roll
 *  "page" than the last page, moves the page to the next page.
 */

void
perfroll::follow_progress ()
{
    midipulse progress_tick = m_old_progress_ticks;
    if (progress_tick > 0)
    {
        int progress_x = progress_tick / m_zoom + SEQ64_PROGRESS_PAGE_OVERLAP;
        int page = progress_x / m_window_x;
        if (page != m_scroll_page)
        {
            midipulse left_tick = page * m_window_x * m_zoom;
            m_scroll_page = page;
            m_hadjust.set_value(double(left_tick / m_ticks_per_bar));
        }
    }
}

#else

void
perfroll::follow_progress ()
{
    // No code, do not follow the progress bar.
}

#endif      // SEQ64_FOLLOW_PROGRESS_BAR

#ifdef USE_STAZED_PERF_AUTO_SCROLL

/**
 *  Supports auto-scrolling.  However, the follow_progress() function seems to
 *  work fine in both ALSA and JACK mode, and it is simpler, so it may be that
 *  the auto_scroll_horz() function will never be necessary.  Let's wait until
 *  the Stazed/Seq32 JACK code is fully tested, including how well it works
 *  under various zooms.
 */

void
perfroll::auto_scroll_horz ()
{
    if (! perf().get_follow_transport())
        return;

    if (m_zoom >= c_perf_scale_x)
    {
        double progress = double(2 * perf().get_tick() / m_zoom / m_ppqn);
        int zoom_ratio = m_zoom / c_perf_scale_x;
        progress *= zoom_ratio;

        int offset = zoom_ratio;
        if (zoom_ratio != 1)
            offset *= -2;

        double page_size_adjust = m_hadjust.get_page_size() / zoom_ratio / 2;
        double get_value_adjust = m_hadjust.get_value() * zoom_ratio;
        if (progress > page_size_adjust || get_value_adjust > progress)
            m_hadjust.set_value(progress - page_size_adjust + offset);

        return;
    }

    midipulse progress_tick = perf().get_tick();
    midipulse tick_offset = m_4bar_offset * m_ppqn * 16;
    int progress_x = (progress_tick - tick_offset) / m_zoom + 100;
    int page = progress_x / m_window_x;
    if (page != 0 || progress_x < 0)
    {
        double left_tick = double(2 * progress_tick / m_zoom / m_ppqn);
        switch (m_zoom)
        {
        case 8:
            m_hadjust.set_value(left_tick / 4);
            break;

        case 16:
            m_hadjust.set_value(left_tick / 2 );
            break;

#if USE_THESE_EXTRA_VALUES
        case 32:
            m_hadjust.set_value(left_tick);
            break;

        case 64:
            m_hadjust.set_value(left_tick * 2);
            break;

        case 128:
            m_hadjust.set_value(left_tick * 4);
            break;
#endif

        default:
            break;
        }
    }
}

#endif  // USE_STAZED_PERF_AUTO_SCROLL

/**
 *  Draws the given pattern/sequence on the given drawable area.
 *  Statement nesting from hell!
 */

void
perfroll::draw_sequence_on (int seqnum)
{
    sequence * seq = perf().get_sequence(seqnum);
    if (not_nullptr(seq))
    {
        midipulse tick_offset = m_4bar_offset;      //  * m_ticks_per_bar;
        midipulse x_offset = tick_offset / m_perf_scale_x;
        m_sequence_active[seqnum] = true;
        seq->reset_draw_trigger_marker();
        seqnum -= m_sequence_offset;

        midipulse sequence_length = seq->get_length();
        int length_w = sequence_length / m_perf_scale_x;
        midipulse tick_on;
        midipulse tick_off;
        midipulse offset;
        bool selected;
        while (seq->get_next_trigger(tick_on, tick_off, selected, offset))
        {
            if (tick_off > 0)
            {
                midipulse x_on  = tick_on  / m_perf_scale_x;
                midipulse x_off = tick_off / m_perf_scale_x;
                int w = x_off - x_on + 1;
                int x = x_on;
                int y = m_names_y * seqnum + 1;         // + 2
                int h = m_names_y - 2;                  // - 4
                x -= x_offset;                  /* adjust to screen coords  */

                /**
                 * Items drawn on the Song editor piano roll:
                 *
                 *  -# Main trigger box (also called a "segment") background.
                 *  -# Trigger outline (the rectangle around a "segment").
                 *  -# The left hand side little sequence grab handle,
                 *     or segment handle.
                 *  -# The right-side segment handle.
                 *
                 *  printf
                 *  (
                 *      "seq %d trigger: %s\n",
                 *      seqnum, selected ? "selected" : "unselected"
                 *  );
                 */

                Color evbkground;
                if (selected)
                    evbkground = grey_paint();
#ifdef SEQ64_SHOW_COLOR_PALETTE
                else
                {
                    int c = seq->color();
                    // if (c != SEQ64_COLOR_NONE)
                        evbkground = get_color(PaletteColor(c));
                }
#else
                else
                    evbkground = white_paint();
#endif
                /*
                 * Fill performance segment background.  Then draw a rectangle
                 * around it, and add the segment handle.
                 */

                draw_rectangle_on_pixmap(evbkground, x, y, w, h);
                draw_rectangle_on_pixmap(black_paint(), x, y, w, h, false);
                draw_rectangle_on_pixmap        /* draw the segment handle  */
                (
                    dark_cyan(),                /* instead of black()       */
                    x, y, m_size_box_w, m_size_box_w, false
                );
                draw_rectangle_on_pixmap        /* color set previous call  */
                (
                    x + w - m_size_box_w, y + h - m_size_box_w,
                    m_size_box_w, m_size_box_w, false
                );

                midipulse tickmarker =          /* length marker first tick */
                (
                    tick_on - (tick_on % sequence_length) +
                    (offset % sequence_length) - sequence_length
                );
                while (tickmarker < tick_off)
                {
                    midipulse tickmarker_x =
                        (tickmarker / m_perf_scale_x) - x_offset;

                    if (tickmarker > tick_on)
                    {
                        draw_rectangle
                        (
                            m_pixmap, light_grey_paint(),
                            tickmarker_x, y + 4, 1, h - 8
                        );
                    }

                    int low_note, high_note;            // for side-effects
                    bool have_notes = seq->get_minmax_note_events
                    (
                        low_note, high_note             // side-effects
                    );
                    if (have_notes)
                    {
                        int height = high_note - low_note + 2;
                        int length = seq->get_length();
                        midipulse tick_s;
                        midipulse tick_f;
                        int note;
                        bool selected;
                        int velocity;
                        draw_type_t dt;

#ifdef SEQ64_STAZED_TRANSPOSE

                        /*
                         * If a pattern is not transposable, draw it in red
                         * instead of black.
                         */

                        bool transposable = seq->get_transposable();
                        if (transposable)
                            m_gc->set_foreground(black_paint());
                        else
                            m_gc->set_foreground(red());
#else
                        bool transposable = false;
                        m_gc->set_foreground(black_paint());
#endif

                        seq->reset_draw_marker();       /* container iterator */
                        do
                        {
                            dt = seq->get_next_note_event   /* side-effects */
                            (
                                tick_s, tick_f, note, selected, velocity
                            );
                            if (dt == DRAW_FIN)
                                break;

                            int mny = m_names_y - 6;
                            int note_y;
                            if (dt == DRAW_TEMPO)
                            {
                                /*
                                 * Do not to scale by the note range here.
                                 */

                                note_y =
                                (
                                    mny - (mny * note) / SEQ64_MAX_DATA_VALUE
                                ) + 1;
                            }
                            else
                            {
                                note_y =
                                (
                                    mny - (mny * (note - low_note)) / height
                                ) + 1;
                            }
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
                                int ny = y + note_y;
                                Color paint = transposable ? black_paint() : red();
                                if (dt == DRAW_TEMPO)
                                {
                                    set_line(Gdk::LINE_SOLID, 2);
                                    paint = tempo_paint();
                                }
                                draw_line_on_pixmap
                                (
                                    paint, tick_s_x, ny, tick_f_x, ny
                                );
                                if (dt == DRAW_TEMPO)
                                {
                                    /*
                                     * We would like to also draw a line from
                                     * the end of the current tempo to the
                                     * start of the next one.  But we
                                     * currently have only the x value of the
                                     * next tempo.
                                     */

                                    set_line(Gdk::LINE_SOLID, 1);
                                }
                            }
                        } while (dt != DRAW_FIN);
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
    midipulse tick_offset = m_4bar_offset;              // * m_ticks_per_bar;
    long first_measure = tick_offset / m_measure_length;
    long last_measure = first_measure +
        (m_window_x * m_perf_scale_x / m_measure_length) + 1;

    seqnum -= m_sequence_offset;

    int h = m_names_y;
    int y = h * seqnum;
    draw_rectangle_on_pixmap(white_paint(), 0, y, m_window_x, h);
    m_gc->set_foreground(black_paint());
    for (long i = first_measure; i < last_measure; ++i)
    {
        int x_pos = ((i * m_measure_length) - tick_offset) / m_perf_scale_x;
        m_pixmap->draw_drawable
        (
            m_gc, m_background, 0, 0, x_pos, y, m_background_x, m_names_y
        );
    }
}

/**
 *  Redraws patterns/sequences that have been modified.
 *
 * \change ca 2016-05-30
 *      Lets try not drawing sequences greater than the maximum, at all.
 */

void
perfroll::redraw_dirty_sequences ()
{
    bool draw = false;
    int yf = m_window_y / m_names_y;
    for (int y = 0; y <= yf; ++y)
    {
        int seq = y + m_sequence_offset;
        if (seq < m_sequence_max && perf().is_dirty_perf(seq))  /* see note */
        {
            draw_sequence(seq);
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
 *  just use y casted to an int, then the drawing of the row is only partial,
 *  vertically.
 */

void
perfroll::draw_drawable_row (int y)
{
    if (y >= 0)             /* make sure user didn't scroll up off window */
    {
        int s = y / m_names_y;
        draw_drawable(0, s * m_names_y, 0, s * m_names_y, m_window_x, m_names_y);
    }
}

#ifdef SEQ64_SONG_BOX_SELECT

/**
 *  Draws the current mouse-selection box on the main perfroll window.  Note
 *  the parameters of draw_drawable(), which we need to be sure of to draw
 *  thicker boxes.
 *
 *      -   x and y position of rectangle to draw
 *      -   x and y position in drawable where rectangle should be drawn
 *      -   width and height of rectangle to draw
 *
 *  A final parameter of false draws an unfilled rectangle.  Orange makes it a
 *  little more clear that we're selecting, I think.  We also want to try to
 *  thicken the lines somehow.
 *
 *  Compare this function to seqroll's more sophisticated version.
 */

void
perfroll::draw_selection_on_window ()
{
    const int thickness = 1;                    /* normally 1               */
    int x = 0, y = 0, w = 0, h = 0;             /* used throughout          */
    set_line(Gdk::LINE_SOLID, thickness);       /* set_line_attributes()    */
    if (selecting())
    {
        m_old.get(x, y, w, h);                      /* get rectangle        */
        draw_drawable(x, y, x, y, w + 1, h + 1);    /* erase old rectangle  */
        m_selected.get(x, y, w, h);
    }

#ifdef SEQ64_USE_BLACK_SELECTION_BOX
    draw_rectangle(black_paint(), x, y, w, h, false);
#else
    draw_rectangle(dark_orange(), x, y, w, h, false);
#endif
}

#endif  // SEQ64_SONG_BOX_SELECT

/**
 *  Provides a very common sequence of calls used in perfroll_input.
 *
 *  m_drop_y is adjusted by perfroll::change_vert() for any scroll after it
 *  was originally selected. The call below to draw_drawable_row() will have
 *  the wrong y location and un-select will not occur if the user scrolls the
 *  track up or down to a new y location, if not adjusted.
 *
 *  For the box set/selections, we create a perform::SeqOperation functor by
 *  binding &perfroll::draw_sequence() to this object and passing it to
 *  perform::selection_operation().  The whole set of operations replaces the
 *  single operation of the "drop" sequence.
 *
 * Note:
 *
 *  We could bind additional parameters as needed, either as place-holders or
 *  constant parameter values.
 */

void
perfroll::draw_all ()
{

#ifdef SEQ64_SONG_BOX_SELECT

#if __cplusplus >= 201103L                  /* C++11                        */
    perform::SeqOperation f = std::bind
    (
        &perfroll::draw_sequence, std::ref(*this), std::placeholders::_1
    );
    perf().selection_operation(f);          /* works with sets of sequences */
#endif

#else
    (void) draw_sequence(m_drop_sequence);  /* draw seq background & events */
#endif

    draw_drawable_row(m_drop_y);
}

/**
 *  Splits a trigger, whatever that means.
 */

void
perfroll::split_trigger (int seqnum, midipulse tick)
{
    perf().split_trigger(seqnum, tick);     /* consolidates perform actions */
    (void) draw_sequence(seqnum);           /* draw seq background & events */
    draw_drawable_row(m_drop_y);
}

/**
 *  This function performs a 'snap' action on x.
 *
 *      -   m_snap_x = number pulses to snap to
 *      -   m_perf_scale_x = number of pulses per pixel
 *
 *  Therefore mod = m_snap_x/m_perf_scale_x equals the number pixels to snap
 *  to.
 *
 * \param [out] x
 *      The destination for the x-snap calculation.
 */

void
perfroll::snap_x (int & x)
{
    int mod = m_snap_x / m_perf_scale_x;
    if (mod <= 0)
        mod = 1;

    x -= (x % mod);
}

/**
 *  This function performs a 'snap' action on y.  We don't do vertical
 *  zoom, so this function is simpler than snap_x().
 *
 * \param [out] y
 *      The destination for the y-snap calculation.
 */

void
perfroll::snap_y (int & y)
{
    y -= (y % m_names_y);
}

/**
 *  Converts an x-coordinate to a tick-offset on the x axis.
 *  The result is returned via the tick parameter.  Note that m_4bar_offset
 *  already includes the m_ticks_per_bar = ppqn * 16 factor, for speed.
 *
 * \param x
 *      The input x (pixel) value.
 *
 * \param [out] tick
 *      Holds the result of the calculation.
 */

void
perfroll::convert_x (int x, midipulse & tick)
{
    tick = m_4bar_offset + midipulse(x * m_perf_scale_x);
}

/**
 *  Converts (x, y) coordinates on the piano roll to tick (pulse) and sequence
 *  numbers.
 *
 *  The results are returned via the d_tick and d_seq parameters.  The
 *  sequence number is clipped to a legal value (0 to m_sequence_max).
 *
 * \param x
 *      The x coordinate of the mouse pointer.
 *
 * \param y
 *      The y coordinate of the mouse pointer.
 *
 * \param [out] d_tick
 *      Holds the calculated tick value.
 *
 * \param [out] d_seq
 *      Holds the calculated sequence-number value.
 */

void
perfroll::convert_xy (int x, int y, midipulse & d_tick, int & d_seq)
{
    midipulse tick = m_4bar_offset + midipulse(x * m_perf_scale_x);
    int seq = m_sequence_offset + (y / m_names_y);
    if (seq >= m_sequence_max)
        seq = m_sequence_max - 1;
    else if (seq < 0)
        seq = 0;

    d_tick = tick;
    d_seq = seq;
}

/**
 *  Implements the horizontal zoom feature.
 *
 * \change ca 2016-04-05
 *      The initial zoom value is c_perf_scale_x (32).  We allow it to range
 *      from 1 to 128, for now.  Smaller values zoom in.
 */

void
perfroll::set_zoom (int z)
{
    if (perfedit::zoom_check(z))
    {
        m_zoom = z;
        set_ppqn(m_ppqn);               /* recalculates other "x" values    */
        update_sizes();
    }
}

/**
 *  Provides the on-realization callback.  Calls the base-class version
 *  first.
 *
 *  Then it allocates the additional resources need, that couldn't be
 *  initialized in the constructor, and makes some connections.
 *
 * Stazed:
 *
 *      This creation of m_background needs to be set to the max width for
 *      proper drawing of zoomed measures or they will get truncated with high
 *      beats per measure and low beat width. Since this is a constant size,
 *      it cannot be adjusted later for zoom. The constant
 *      c_perfroll_background_x is set to the max amount by default for use
 *      here. The drawing functions fill_background_pixmap() and
 *      draw_background_on() which use c_perfroll_background_x also, could be
 *      adjusted by zoom with a substituted variable. Not sure if there is any
 *      benefit to doing the adjustment...  Perhaps a small benefit in speed?
 *      Maybe FIXME if really, really bored...
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
 *  Handles the on-expose event.  Draws a vertical page of the performance
 *  editor.  The part drawn starts at m_sequence_offset and continues until
 *  the last sequence that can be at least partially seen given the height of
 *  the window.
 *
 *  If we're at the bottom of the sequences (1024, a non-existent sequence)
 *  would be the last sequence shown, we don't bother drawing it.  This
 *  prevents debug messages about an illegal sequence, and can show a black
 *  bottom row that is a clear sign we're at the end of the legal sequences.
 *
 * \param ev
 *      Provides the expose event.
 *
 * \return
 *      Always returns true.
 */

bool
perfroll::on_expose_event (GdkEventExpose * ev)
{
    int ys = ev->area.y / m_names_y;
    int yf = (ev->area.y + ev->area.height) / m_names_y;
    for (int y = ys; y <= yf; ++y)
    {
        int seq = y + m_sequence_offset;
        if (seq < m_sequence_max)           /* see note in banner */
            (void) draw_sequence(seq);
    }
    m_window->draw_drawable
    (
        m_gc, m_pixmap, ev->area.x, ev->area.y,
        ev->area.x, ev->area.y, ev->area.width, ev->area.height
    );
    return true;
}

/**
 *  This callback function handles the follow-on work of a button press, and
 *  is called by overridden versions such as Seq24PerfInput ::
 *  on_button_press_event()
 *
 *  One minor issue:  Fruity behavior doesn't yet provide the keystroke
 *  behavior we now handle for the Seq24 mode of operation.
 */

bool
perfroll::on_button_press_event (GdkEventButton * ev)
{

#ifdef USE_UNNECESSARY_TRANSPORT_FOLLOW_CALLBACK

    /*
     *  To avoid double button press on normal seq42 method...  Shouldn't we
     *  only do this if the transport button was the one that was pressed?
     *  And shouldn't perform have all the flags we need?
     */

    if (! m_trans_button_press)
    {
        m_transport_follow = perf().get_follow_transport();
        perf().set_follow_transport(false);
        m_trans_button_press = true;
    }

#endif

    bool result = gui_drawingarea_gtk2::on_button_press_event(ev);
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
    bool result = gui_drawingarea_gtk2::on_button_release_event(ev);

#ifdef USE_UNNECESSARY_TRANSPORT_FOLLOW_CALLBACK
    if (m_trans_button_press)
    {
        perf().set_follow_transport(m_transport_follow);
        m_trans_button_press = false;
    }
#endif

    enqueue_draw();
    return result;
}

/**
 *  Handles horizontal and vertical scrolling.  If the Shift key is held while
 *  scrolling, then the scrolling is horizontal, otherwise it is vertical.
 *  This matches the convention of the seqedit class.
 *
 *  Note that, unlike the seqedit class, Ctrl-Scroll is not used to modify
 *  the zoom value.  Rather than mess up legacy behavior, we will rely on
 *  keystrokes (z, 0, Z, and Ctrl-Page-Up and Ctrl-Page-Down) to implement this
 *  zoom.
 *
 * \param ev
 *      Provides the scroll event.
 *
 * \return
 *      Currently always returns true.
 */

bool
perfroll::on_scroll_event (GdkEventScroll * ev)
{
    if (is_shift_key(ev))
    {
        double val = m_hadjust.get_value();
        double increment = m_hadjust.get_step_increment();
        if (CAST_EQUIVALENT(ev->direction, SEQ64_SCROLL_UP))
            val -= increment;
        else if (CAST_EQUIVALENT(ev->direction, SEQ64_SCROLL_DOWN))
            val += increment;

        m_hadjust.clamp_page(val, val + m_hadjust.get_page_size());
    }
    else if (is_ctrl_key(ev))
    {
        /*
         * Stazed addition, use Ctrl key to effect zoom changes.
         */

        if (CAST_EQUIVALENT(ev->direction, SEQ64_SCROLL_UP))
            m_parent.set_zoom(m_zoom / 2);
        else if (CAST_EQUIVALENT(ev->direction, SEQ64_SCROLL_DOWN))
            m_parent.set_zoom(m_zoom * 2);
    }
    else
    {
        double val = m_vadjust.get_value();
        double increment = m_vadjust.get_step_increment();
        if (CAST_EQUIVALENT(ev->direction, SEQ64_SCROLL_UP))
            val -= increment;
        else if (CAST_EQUIVALENT(ev->direction, SEQ64_SCROLL_DOWN))
            val += increment;

        m_vadjust.clamp_page(val, val + m_vadjust.get_page_size());
    }
    return gui_drawingarea_gtk2::on_scroll_event(ev); /* instead of true */
}

/**
 *  Handles motion notification by forwarding it to the interaction
 *  object's motion-notification callback function.  Actually, this is
 *  backwards; this function is called within the derived object's override.
 */

bool
perfroll::on_motion_notify_event (GdkEventMotion * ev)
{
    enqueue_draw();                 /* put in if() to reduce flickering */

    bool result = gui_drawingarea_gtk2::on_motion_notify_event(ev);
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
 *
 *  The perfroll_key_event() call handles Del, Ctrl-X, Ctrl-C, Ctrl-V, and
 *  Ctrl-Z (which does nothing at present).
 *
 *  We've also added support for moving up and down in the piano roll (Up and
 *  Down arrows), paging up and down (Page-Up and Page-Down keys), paging left
 *  and right (Shift Page-Up and Page-Down), paging to top and bottom (Home
 *  and End), and paging to start and end (Shift Home and End).
 *
 *  The Keypad-End key is an issue on our ASUS "gaming" laptop.  Whether it is
 *  seen as a "1" or an "End" key depends on an interaction between the Shift
 *  and the Num Lock key.  Annoying, takes some time to get used to.
 *
 * Stazed:  there are many changes from seq32 that need to be studied before
 *          including them here.
 */

bool
perfroll::on_key_press_event (GdkEventKey * ev)
{
    keystroke k(ev->keyval, SEQ64_KEYSTROKE_PRESS, ev->state);

    /*
     * If this keystroke is clicked, move the song position to the location of
     * the mouse pointer.  Stazed used p, but we need an alternative.  The
     * default is F7, but it is configurable in the File / Options / Ext Keys
     * tab page.
     */

    if (k.is(perf().keys().pointer_position()))
    {
        midipulse tick = 0;
        int x = 0, y = 0;
        get_pointer(x, y);
        if (x < 0)
            x = 0;

        snap_x(x);                      /* side-effect */
        convert_x(x, tick);             /* side-effect */
        perf().reposition(tick);
        return true;
    }

    /*
     *  Add this call to try to make seqroll and perfroll act the same for
     *  start/stop/play.  Doesn't work, but doesn't break anything.  Turns out
     *  perfedit handles this event.  Added the "true" parameter to force song
     *  mode when starting from perfedit.
     */

    bool result = perf().playback_key_event(k, true);
    if (! result)
        result = perf().perfroll_key_event(k, m_drop_sequence);

    if (result)
    {
        /*
         * enqueue_draw();
         */
    }
    else
    {
        /**
         *  Note that, even though we filter out the Ctrl key here, it still
         *  works for Ctrl-X (cut) and Ctrl-V (paste).  For undo, the Undo
         *  button can be used, Ctrl-Z never worked in this view anyway.
         *
         * \warning
         *      We see that 'x' and 'z' are already handled in
         *      perfroll_key_event() if the Ctrl key was pressed.  Be careful.
         */

        bool is_ctrl = is_ctrl_key(ev);
        bool is_shift = is_shift_key(ev);
        if (! perf().is_running())
        {
            if (is_ctrl)
            {
                /*
                 * We won't bother handling zoom with Ctrl Page keys, yet.
                 */
            }
            else if (is_shift)
            {
                if (k.is(SEQ64_Z))              /* zoom in, "z" no go   */
                {
                    m_parent.set_zoom(m_zoom / 2);
                    result = true;
                }
                else if (k.is(SEQ64_Up))        /* horizontal movement  */
                {
                    double step = m_hadjust.get_step_increment();
                    horizontal_adjust(-step);
                    result = true;
                }
                else if (k.is(SEQ64_Down))
                {
                    double step = m_hadjust.get_step_increment();
                    horizontal_adjust(step);
                    result = true;
                }
                else if (k.is(SEQ64_Page_Up))
                {
                    double step = m_hadjust.get_page_increment();
                    horizontal_adjust(-step);
                    result = true;
                }
                else if (k.is(SEQ64_Home, SEQ64_KP_Home))
                {
                    horizontal_set(0);              /* scroll to beginning  */
                    result = true;
                }
                else if (k.is(SEQ64_Page_Down))
                {
                    double step = m_hadjust.get_page_increment();
                    horizontal_adjust(step);
                    result = true;
                }
                else if (k.is(SEQ64_End, SEQ64_KP_End))
                {
                    horizontal_set(9999999.0);      /* scroll to the end    */
                    result = true;
                }
            }
            else
            {
                if (k.is(SEQ64_p))
                {
                    activate_adding(true);
                    result = true;
                }
                else if (k.is(SEQ64_x))             /* "x-scape" the mode   */
                {
                    activate_adding(false);
                    result = true;
                }
                else if (k.is(SEQ64_0))             /* reset to normal zoom */
                {
                    m_parent.set_zoom(c_perf_scale_x);
                    result = true;
                }
                else if (k.is(SEQ64_z))             /* zoom out             */
                {
                    m_parent.set_zoom(m_zoom * 2);
                    result = true;
                }
                else if (k.is(SEQ64_Left))
                {
                    result = handle_motion_key(true);
                    if (result)
                        perf().modify();
                }
                else if (k.is(SEQ64_Right))
                {
                    result = handle_motion_key(false);
                    if (result)
                        perf().modify();
                }
                else if (k.is(SEQ64_Up))            /* vertical movement    */
                {
                    double step = m_vadjust.get_step_increment();
                    vertical_adjust(-step);
                    result = true;
                }
                else if (k.is(SEQ64_Down))
                {
                    double step = m_vadjust.get_step_increment();
                    vertical_adjust(step);
                    result = true;
                }
                else if (k.is(SEQ64_Page_Up))
                {
                    double step = m_hadjust.get_page_increment();
                    vertical_adjust(-step);
                    result = true;
                }
                else if (k.is(SEQ64_Home, SEQ64_KP_Home))
                {
                    vertical_set(0);                /* scroll to beginning  */
                    result = true;
                }
                else if (k.is(SEQ64_Page_Down))
                {
                    double step = m_hadjust.get_page_increment();
                    vertical_adjust(step);
                    result = true;
                }
                else if (k.is(SEQ64_End, SEQ64_KP_End))
                {
                    vertical_set(9999999.0);        /* scroll to the end    */
                    result = true;
                }
            }
        }
    }
    if (result)
        fill_background_pixmap();
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

