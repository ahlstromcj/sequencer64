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
 * \file          seqroll.cpp
 *
 *  This module declares/defines the base class for drawing on the piano
 *  roll of the sequence/pattern editor.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2016-06-19
 * \license       GNU GPLv2 or above
 *
 *  There are a large number of existing items to discuss.  But for now let's
 *  talk about how to have the scrollbar follow the progress bar.
 *
 *  The full extent of the sequence ranges from 0 to its length in ticks (Ts).
 *  The horizontal scrollbar is set to match that:
 *
\verbatim
      lower                 Ps = page-size (ticks)                     upper
  Seq:  0-------------------|-------------------------------------------Ts
 Page:  0-------------------| Pp = page-size (pixels) = window_width * zoom
        |--|--|--|--|         page-increment, one bar
        |--|                  step-increment, one semi-quaver (16th note)
\endverbatim
 *
 *  The window width multiplied by the zoom factor is the "page" size, P.
 *  The page increment is smaller (usually), and is one bar.
 *
 *  When the progress bar is nearly at the end of the current page, we want
 *  to move to the next page and continue the progress bar.
 *
 *  The page-size in ticks is the page-size in pixels divided by the zoom:
 *
\verbatim
        Ps = Pp / Z
\endverbatim
 *
 *  Just before the progress bar reaches a little less than a multiple of Ps,
 *  we want to add to the scrollbar value and adjust it.
 *
 *      seqroll scroll progress=696; value=0; step=96; page=1388; upper=66816
 *
 *  Note that the default zoom is 2, so that the maximum tick value for
 *  the window is one/half the windows size, by default.
 *
 *  With --ppqn 384 option:
 *
 *	    seqroll scroll progress=648; value=0; step=192; page=1388; upper=133632
 */

#include <gtkmm/accelkey.h>
#include <gtkmm/adjustment.h>
#include <gtkmm/menu.h>

#include "app_limits.h"                 /* SEQ64_SOLID_PIANOROLL_GRID   */
#include "event.hpp"
#include "gdk_basic_keys.h"
#include "keystroke.hpp"
#include "scales.h"
#include "seqroll.hpp"
#include "seqdata.hpp"
#include "seqevent.hpp"
#include "sequence.hpp"
#include "seqkeys.hpp"
#include "perform.hpp"
#include "settings.hpp"                 /* seq64::usr() and seq64::rc() */

/**
 *  This provides a build option for having the pattern editor window scroll
 *  to keep of with the progress bar, for sequences that are longer than the
 *  measure or two that a pattern window will show.
 *
 *  We thought about making this a configure option or a run-time option, but
 *  this kind of scrolling is a universal convention of MIDI sequencers.  If
 *  you really don't like this feature, let me know, and I will make it a
 *  configure option.  We could also disable it it "legacy" mode, which also
 *  disables a lot of other features.
 *
 * \warning
 *      This code might still have issues with interactions between triggers
 *      and gaps in the performance (song) window when JACK transport is
 *      active.  Still investigating.
 */

#define SEQ64_FOLLOW_PROGRESS_BAR

/**
 *  This macro defines the amount of overlap between horizontal "pages" that
 *  get scrolled to follow the progress bar.  We think it should be greater
 *  than 0, maybe set to 10. But feel free to experiment.
 */

#define SEQ64_PROGRESS_PAGE_OVERLAP     10

namespace seq64
{

/**
 *  Principal constructor.
 *
 * \param p
 *      The performance object that helps control this piano roll.  Note that
 *      we can get the perform object from the sequence, and save a parameter.
 *      Low priority change.
 *
 * \param seq
 *      The sequence object represented by this piano roll.
 *
 * \param zoom
 *      The initial zoom of this piano roll.
 *
 * \param snap
 *      The initial grid snap of this piano roll.
 *
 * \param seqkeys_wid
 *      A reference to the piano keys window that is shown to the left of this
 *      piano roll.
 *
 * \param pos
 *      A position parameter.  See the description of seqroll::m_pos.
 *      This is actually the sequence number, and is currently unused.
 *      However, we're sure we can find a use for it sometime.
 *
 * \param hadjust
 *      Represents the horizontal scrollbar of this window.  It is actually
 *      created by the "parent" seqedit object.
 *
 * \param vadjust
 *      Represents the vertical scrollbar of this window.  It is actually
 *      created by the "parent" seqedit object.
 *
 * \param ppqn
 *      The initial value of the PPQN for this sequence.  Useful in scale
 *      calculations.
 */

seqroll::seqroll
(
    perform & p,
    sequence & seq,
    int zoom,
    int snap,
    seqkeys & seqkeys_wid,
    int pos,
    Gtk::Adjustment & hadjust,
    Gtk::Adjustment & vadjust,
    int ppqn
) :
    gui_drawingarea_gtk2    (p, hadjust, vadjust, 10, 10),
    m_horizontal_adjust     (hadjust),
    m_vertical_adjust       (vadjust),
    m_old                   (),
    m_selected              (),
    m_seq                   (seq),
    m_seqkeys_wid           (seqkeys_wid),
    m_fruity_interaction    (),
    m_seq24_interaction     (),
    m_pos                   (pos),
    m_zoom                  (zoom),
    m_snap                  (snap),
    m_ppqn                  (0),
    m_note_length           (0),
    m_scale                 (0),
    m_key                   (0),
    m_selecting             (false),
    m_moving                (false),
    m_moving_init           (false),
    m_growing               (false),
    m_painting              (false),
    m_paste                 (false),
    m_is_drag_pasting       (false),
    m_is_drag_pasting_start (false),
    m_justselected_one      (false),
    m_move_delta_x          (0),
    m_move_delta_y          (0),
    m_move_snap_offset_x    (0),        /* used in fruityseqroll */
    m_progress_x            (0),
    m_scroll_offset_ticks   (0),
    m_scroll_offset_key     (0),
    m_scroll_offset_x       (0),
    m_scroll_offset_y       (0),
    m_scroll_page           (0),
    m_background_sequence   (0),
    m_drawing_background_seq(false),
    m_status                (0),
    m_cc                    (0)
{
    m_ppqn = choose_ppqn(ppqn);

    /*
     * These calls don't seem to work in the constructor.  They do in
     * the parent's constructor [seqedit()].
     *
     *      set_can_focus();
     *      grab_focus();
     */
}

/**
 *  Provides a destructor to delete allocated objects.  The only thing to
 *  delete here is the clipboard.  Except it is never used, so is commented
 *  out.
 */

seqroll::~seqroll ()
{
    /*
     * if (not_nullptr(m_clipboard))
     *      delete m_clipboard;
     */
}

/**
 *  This function sets the given sequence onto the piano roll of the pattern
 *  editor, so that the musician can have another pattern to play against.
 *  The state parameter sets the boolean m_drawing_background_seq.
 *
 * \param state
 *      If true, the background sequence will be drawn.
 *
 * \param seq
 *      Provides the sequence number, which is checked against the
 *      SEQ64_IS_LEGAL_SEQUENCE() macro before being used.  This macro allows
 *      the value SEQ64_SEQUENCE_LIMIT, which disables the background
 *      sequence.
 */

void
seqroll::set_background_sequence (bool state, int seq)
{
    m_drawing_background_seq = state;
    if (SEQ64_IS_LEGAL_SEQUENCE(seq))
        m_background_sequence = seq;

    update_and_draw();
}

/**
 *  Update the sizes of items based on zoom, PPQN, BPM, BW (beat width) and
 *  more.  It brings the scrollbar back to the beginning, resets the upper
 *  limit to the number of ticks in the sequence, sets the page-size based on
 *  the window size and the zoom factor.
 *
 *  The horizontal step increment is 1 semiquaver (1/16) note per zoom level.
 *  The horizontal page increment is currently always one bar.  We may want to
 *  make that larger for scrolling after the progress bar.
 *
 *  Tha maximum value set for the scrollbar brings it to the last "page" of the
 *  piano roll.
 *
 *  The vertical size are also adjusted.  More on the story later.
 */

void
seqroll::update_sizes ()
{
    int zoom_x = m_window_x * m_zoom;
    int h_max_value = m_seq.get_length() - zoom_x;
    int page_increment =
        4 * m_ppqn * m_seq.get_beats_per_bar() / m_seq.get_beat_width();

    m_hadjust.set_lower(0);                             /* set default size */
    m_hadjust.set_upper(m_seq.get_length());
    m_hadjust.set_page_size(zoom_x);
    m_hadjust.set_step_increment(m_zoom * m_ppqn / 4);
    m_hadjust.set_page_increment(page_increment);
    if (m_hadjust.get_value() > h_max_value)
        m_hadjust.set_value(h_max_value);

    m_vadjust.set_lower(0);
    m_vadjust.set_upper(c_num_keys);
    m_vadjust.set_page_size(m_window_y / c_key_y);
    m_vadjust.set_step_increment(12);
    m_vadjust.set_page_increment(12);

    int v_max_value = c_num_keys - (m_window_y / c_key_y);
    if (m_vadjust.get_value() > v_max_value)
        m_vadjust.set_value(v_max_value);

    if (is_realized())              /* create pixmaps with window dimensions */
    {
        m_pixmap = Gdk::Pixmap::create(m_window, m_window_x, m_window_y, -1);
        m_background = Gdk::Pixmap::create(m_window, m_window_x, m_window_y, -1);
        change_vert();
    }
}

/**
 *  Change the horizontal scrolling offset and redraw.  Roughly similar to
 *  seqevent::change_horz().
 */

void
seqroll::change_horz ()
{
    m_scroll_offset_ticks = int(m_hadjust.get_value());
    m_scroll_offset_x = m_scroll_offset_ticks / m_zoom;
    update_and_draw(true);
}

/**
 *  Change the vertical scrolling offset and redraw.
 */

void
seqroll::change_vert ()
{
    m_scroll_offset_key = int(m_vadjust.get_value());
    m_scroll_offset_y = m_scroll_offset_key * c_key_y;
    update_and_draw(true);
}

/**
 *  This function basically resets the whole widget as if it were realized
 *  again.  It's almost identical to the change_horz() function, just calling
 *  update_sizes() before update_and_draw().
 */

void
seqroll::reset ()
{
    m_scroll_offset_ticks = int(m_hadjust.get_value());
    m_scroll_offset_x = m_scroll_offset_ticks / m_zoom;
    update_sizes();
    update_and_draw();
}

/**
 *  Redraws unless m_ignore_redraw is true.  Somewhat similar to
 *  seqevent::redraw().  Actually, we don't seem to need to ignore redraw when
 *  making settings in the seqedit constructor, so this member no longer
 *  exists.
 */

void
seqroll::redraw ()
{
    m_scroll_offset_ticks = int(m_hadjust.get_value());
    m_scroll_offset_x = m_scroll_offset_ticks / m_zoom;
    update_and_draw(true);
}

/**
 *  Wraps up some common code.
 *
 * \param force
 *      If true, force an immediate draw, otherwise just queue up a draw.
 */

void
seqroll::update_and_draw (int force)
{
    update_background();
    update_pixmap();
    if (force)
        force_draw();
    else
        queue_draw();
}

/**
 *  Redraws events unless m_ignore_redraw is true.  Actually, that member is
 *  not needed and no longer exists.
 */

void
seqroll::redraw_events ()
{
    update_pixmap();
    force_draw();
}

/**
 *  Draws the main pixmap.
 */

void
seqroll::draw_background_on_pixmap ()
{
    m_pixmap->draw_drawable
    (
        m_gc, m_background, 0, 0, 0, 0, m_window_x, m_window_y
    );
}

/**
 *  Updates the background of this window.  The first thing done is to clear
 *  the background, painting it white.
 */

void
seqroll::update_background ()
{
    draw_rectangle(m_background, white(), 0, 0, m_window_x, m_window_y);

#ifdef SEQ64_SOLID_PIANOROLL_GRID
    bool fruity_lines = true;
    m_gc->set_foreground(light_grey());             /* draw horz grey lines */
    set_line(Gdk::LINE_SOLID);
#else
    bool fruity_lines = rc().interaction_method() == e_fruity_interaction;
    m_gc->set_foreground(grey());                   /* draw horz grey lines */
    set_line(Gdk::LINE_ON_OFF_DASH);
    gint8 dash = 1;
    m_gc->set_dashes(0, &dash, 1);
#endif

    int octkey = SEQ64_OCTAVE_SIZE - m_key;         /* used three times     */
    for (int key = 0; key < (m_window_y / c_key_y) + 1; ++key)
    {
        int remkeys = c_num_keys - key;             /* remaining keys?      */
        int modkey = remkeys - m_scroll_offset_key + octkey;
        if (fruity_lines)
        {
            if ((modkey % SEQ64_OCTAVE_SIZE) == 0)
            {
                m_gc->set_foreground(dark_grey());  /* draw horz lines at C */
                set_line(Gdk::LINE_SOLID);
            }
            else if ((modkey % SEQ64_OCTAVE_SIZE) == (SEQ64_OCTAVE_SIZE-1))
            {
#ifdef SEQ64_SOLID_PIANOROLL_GRID
                m_gc->set_foreground(light_grey()); /* lighter solid line   */
                set_line(Gdk::LINE_SOLID);
#else
                m_gc->set_foreground(grey());       /* lighter dashed lines */
                set_line(Gdk::LINE_ON_OFF_DASH);
#endif
            }
        }
        int y = key * c_key_y;
        draw_line(m_background, 0, y, m_window_x, y);
        if (m_scale != c_scale_off)
        {
            if (! c_scales_policy[m_scale][(modkey - 1) % SEQ64_OCTAVE_SIZE])
            {
                draw_rectangle
                (
                    m_background, light_grey(), 0, y + 1, m_window_x, c_key_y - 1
                );
            }
        }
    }

    /*
     * The ticks_per_step value needs to be figured out.  Why 6 * m_zoom?  6
     * is the number of pixels in the smallest divisions in the default
     * seqroll background.
     */

    int bpbar = m_seq.get_beats_per_bar();
    int bwidth = m_seq.get_beat_width();
    int ticks_per_step = m_zoom * 6;
    int ticks_per_beat = 4 * m_ppqn / bwidth;
    int ticks_per_major = bpbar * ticks_per_beat;
    int endtick = (m_window_x * m_zoom) + m_scroll_offset_ticks;
    int starttick = m_scroll_offset_ticks -
        (m_scroll_offset_ticks % ticks_per_major);

    m_gc->set_foreground(grey());

    /*
     * Incrementing by ticks_per_step only works for PPQN of certain
     * multiples.
     */

    for (int tick = starttick; tick < endtick; tick += ticks_per_step)
    {
        if (tick % ticks_per_major == 0)
        {
#ifdef SEQ64_SOLID_PIANOROLL_GRID
            set_line(Gdk::LINE_SOLID, 2);
#else
            set_line(Gdk::LINE_SOLID);
#endif
            m_gc->set_foreground(black());      /* solid line on every beat */
        }
        else if (tick % ticks_per_beat == 0)
        {
            set_line(Gdk::LINE_SOLID);
            m_gc->set_foreground(dark_grey());
        }
        else
        {
            int tick_snap = tick - (tick % m_snap);

#ifdef SEQ64_SOLID_PIANOROLL_GRID
            if (tick == tick_snap)
            {
                set_line(Gdk::LINE_SOLID);
                m_gc->set_foreground(light_grey());           // dark_grey());
            }
            else
            {
                set_line(Gdk::LINE_ON_OFF_DASH);
                m_gc->set_foreground(light_grey());
                gint8 dash = 1;
                m_gc->set_dashes(0, &dash, 1);
            }
#else
            set_line(Gdk::LINE_ON_OFF_DASH);
            if (tick == tick_snap)
                m_gc->set_foreground(dark_grey());
            else
                m_gc->set_foreground(grey());

            gint8 dash = 1;
            m_gc->set_dashes(0, &dash, 1);
#endif
        }
        int x = (tick / m_zoom) - m_scroll_offset_x;
        draw_line(m_background, x, 0, x, m_window_y);
    }
#ifndef SEQ64_SOLID_PIANOROLL_GRID
    set_line(Gdk::LINE_SOLID);                  /* reset the line style     */
#endif
}

/**
 *  Sets the zoom to the given value, and then resets the view.
 *
 * \param zoom
 *      The desired zoom value.
 */

void
seqroll::set_zoom (int zoom)
{
    if (m_zoom != zoom)
    {
        m_zoom = zoom;
        reset();
    }
}

/**
 *  Sets the music scale to the given value, and then resets the view.
 *
 * \param scale
 *      The desired scale value.
 */

void
seqroll::set_scale (int scale)
{
    if (m_scale != scale)
    {
        m_scale = scale;
        reset();
    }
}

/**
 *  Sets the music key to the given value, and then resets the view.
 *
 * \param key
 *      The desired key value.
 */

void
seqroll::set_key (int key)
{
    if (m_key != key)
    {
        m_key = key;
        reset();
    }
}

/**
 *  This function draws the background pixmap on the main pixmap, and
 *  then draws the events on it.
 */

void
seqroll::update_pixmap ()
{
    draw_background_on_pixmap();
    draw_events_on_pixmap();
}

/**
 *  Draw a progress line on the window.  This is done by first blanking out
 *  the line with the background, which contains white space and grey lines,
 *  using the the draw_drawable function.  Remember that we wrap the
 *  draw_drawable() function so it's parameters are xsrc, ysrc, xdest, ydest,
 *  width, and height.
 *
 *  Note that the progress-bar position is based on the
 *  sequence::get_last_tick() value, the current zoom, and the current
 *  scroll-offset x value.
 */

void
seqroll::draw_progress_on_window ()
{
    if (usr().progress_bar_thick())
    {
        draw_drawable(m_progress_x-1, 0, m_progress_x-1, 0, 2, m_window_y);
        set_line(Gdk::LINE_SOLID, 2);
    }
    else
        draw_drawable(m_progress_x, 0, m_progress_x, 0, 1, m_window_y);

    m_progress_x = (m_seq.get_last_tick() / m_zoom) - m_scroll_offset_x;

    /*
     * TRIAL CODE to ensure the occasional slightly negative value still
     * allows the progress bar to be drawn.
     */

    if (m_progress_x > -16)             // if (m_progress_x >= 0)
    {
        draw_line(progress_color(), m_progress_x, 0, m_progress_x, m_window_y);
        if (usr().progress_bar_thick())
            set_line(Gdk::LINE_SOLID, 1);
    }
}

#ifdef SEQ64_FOLLOW_PROGRESS_BAR

/**
 *  Checks the position of the tick, and, if it is in a different piano-roll
 *  "page" than the last page, moves the page to the next page.
 *
 *  We don't want to do any of this if the length of the sequence fits in the
 *  window, but for now it doesn't hurt; the progress bar just never meets the
 *  criterion for moving to the next page.
 *
 * \todo
 *      -   If playback is disabled (such as by a trigger), then do not update
 *          the page;
 *      -   When it comes back, make sure we're on the correct page;
 *      -   When it stops, put the window back to the beginning, even if the
 *          beginning is not defined as "0".
 */

void
seqroll::follow_progress ()
{
    midipulse progress_tick = m_seq.get_last_tick();
    if (progress_tick > 0)
    {
        int progress_x = progress_tick / m_zoom + SEQ64_PROGRESS_PAGE_OVERLAP;
        int page = progress_x / m_window_x;
        if (page != m_scroll_page)
        {
            midipulse left_tick = page * m_window_x * m_zoom;
            m_scroll_page = page;
            m_hadjust.set_value(double(left_tick));
        }
    }
}

#else

void
seqroll::follow_progress ()
{
    // No code, do not follow the progress bar.
}

#endif      // SEQ64_FOLLOW_PROGRESS_BAR

/**
 *  Draws events on the given drawable area.  "Method 0" draws the background
 *  sequence, if active.  "Method 1" draws the sequence itself.
 *
 * \param draw
 *      The "drawable" area to draw on.
 */

void
seqroll::draw_events_on (Glib::RefPtr<Gdk::Drawable> draw)
{
    midipulse tick_s;
    midipulse tick_f;
    int note;
    bool selected;
    int velocity;
    draw_type dt;
    int starttick = m_scroll_offset_ticks;
    int endtick = (m_window_x * m_zoom) + m_scroll_offset_ticks;
    sequence * seq = nullptr;
    for (int method = 0; method < 2; ++method)  /* weird way to do it       */
    {
        if (method == 0 && m_drawing_background_seq)
        {
            if (perf().is_active(m_background_sequence))
                seq = perf().get_sequence(m_background_sequence);
            else
                ++method;
        }
        else if (method == 0)                   /* done with background?    */
            ++method;                           /* now draw the sequence    */

        if (method == 1)
            seq = &m_seq;

        m_gc->set_foreground(black());          /* draw boxes from sequence */
        seq->reset_draw_marker();
        while
        (
            (
                dt = seq->get_next_note_event(&tick_s, &tick_f,
                   &note, &selected, &velocity)
            ) != DRAW_FIN
        )
        {
            if
            (
                (tick_s >= starttick && tick_s <= endtick) ||
                ((dt == DRAW_NORMAL_LINKED) &&
                    (tick_f >= starttick && tick_f <= endtick))
            )
            {
                int note_width;
                int note_x = tick_s / m_zoom;       /* turn into screen coords */
                int note_y = c_rollarea_y - (note * c_key_y) - c_key_y + 1;
                int note_height = c_key_y - 3;
                int in_shift = 0;
                int length_add = 0;
                if (dt == DRAW_NORMAL_LINKED)
                {
                    if (tick_f >= tick_s)
                    {
                        note_width = (tick_f - tick_s) / m_zoom;
                        if (note_width < 1)
                            note_width = 1;
                    }
                    else
                        note_width = (m_seq.get_length() - tick_s) / m_zoom;
                }
                else
                    note_width = 16 / m_zoom;

                if (dt == DRAW_NOTE_ON)
                {
                    in_shift = 0;
                    length_add = 2;
                }
                else if (dt == DRAW_NOTE_OFF)
                {
                    in_shift = -1;
                    length_add = 1;
                }
                note_x -= m_scroll_offset_x;
                note_y -= m_scroll_offset_y;

                /*
                 * Draw note boxes for main or background sequence.
                 * Use a color more distinguishable from the "scale" color for
                 * the background sequence.
                 */

                if (method == 0)
                     m_gc->set_foreground(dark_cyan());     /* vs dark_grey() */
                else
                    m_gc->set_foreground(black());

                draw_rectangle(draw, note_x, note_y, note_width, note_height);
                if (tick_f < tick_s)
                    draw_rectangle(draw, 0, note_y, tick_f / m_zoom, note_height);

                if (note_width > 3)     /* draw inside box if there is room */
                {
                    if (selected)
                        m_gc->set_foreground(orange());
                    else
                        m_gc->set_foreground(white());

                    if (method == 1)
                    {
                        int xinc = note_x + 1;
                        int yinc = note_y + 1;
                        int hdec = note_height - 3;
                        if (tick_f >= tick_s)
                        {
#if 0
                            draw->draw_rectangle
                            (
                                m_gc, true, xinc + in_shift, yinc,
                                note_width - 3 + length_add, hdec
                            );
#endif
                            draw_rectangle
                            (
                                draw, xinc + in_shift, yinc,
                                note_width - 3 + length_add, hdec
                            );
                        }
                        else
                        {
                            draw_rectangle
                            (
                                draw, xinc + in_shift, yinc, note_width, hdec
                            );

                            /*
                             * TRIAL CODE - ca 2016-06-16.  It helps prevent
                             * two selected begin/end events in a loop from
                             * drawing very long orange selection lines.
                             * However, it still draws a fragment that is
                             * outside the loop boundaries.
                             */

                            int width = (tick_f / m_zoom) - 3 + length_add;
                            if (width < 0)
                                width = note_width;     // 0;

                            draw_rectangle(draw, 0, yinc, width, hdec);
                        }
                    }
                }
            }
        }
    }
}

/**
 *  Fills the main pixmap with events.  Just calls draw_events_on().
 */

void
seqroll::draw_events_on_pixmap ()
{
    draw_events_on(m_pixmap);
}

/**
 *  Draw the events on the main window and on the pixmap.
 */

int
seqroll::idle_redraw ()
{
    draw_events_on(m_window);
    draw_events_on(m_pixmap);
    return true;
}

/**
 *  Draws the current selecton on the main window.
 */

void
seqroll::draw_selection_on_window ()
{
    if (m_selecting || m_moving || m_paste || m_growing)
    {
        set_line(Gdk::LINE_SOLID);
        draw_drawable                                   /* replace old */
        (
            m_old.x, m_old.y, m_old.x, m_old.y, m_old.width + 1, m_old.height + 1
        );
    }

    int x, y, w, h;
    if (m_selecting)
    {
        xy_to_rect(m_drop_x, m_drop_y, m_current_x, m_current_y, x, y, w, h);
        x -= m_scroll_offset_x;
        y -= m_scroll_offset_y;
        m_old.x = x;
        m_old.y = y;
        m_old.width = w;
        m_old.height = h + c_key_y;
        draw_rectangle(black(), x, y, w, h + c_key_y, false);
    }
    if (m_moving || m_paste)
    {
        x = m_selected.x + m_current_x - m_drop_x - m_scroll_offset_x;
        y = m_selected.y + m_current_y - m_drop_y - m_scroll_offset_y;

        /*
         * Orange makes it a little more clear that we're pasting, I think.
         */

#ifdef USE_OLD_COLOR
        draw_rectangle(black(), x, y, m_selected.width, m_selected.height, false);
#else
        draw_rectangle
        (
            dark_orange(), x, y, m_selected.width, m_selected.height, false
        );
#endif
        m_old.x = x;
        m_old.y = y;
        m_old.width = m_selected.width;
        m_old.height = m_selected.height;
    }
    if (m_growing)
    {
        int delta_x = m_current_x - m_drop_x;
        int width = delta_x + m_selected.width;
        if (width < 1)
            width = 1;

        x = m_selected.x - m_scroll_offset_x;
        y = m_selected.y - m_scroll_offset_y;
        draw_rectangle(black(), x, y, width, m_selected.height, false);
        m_old.x = x;
        m_old.y = y;
        m_old.width = width;
        m_old.height = m_selected.height;
    }
}

/**
 *  Set the pixmap into the window and then draws the selection on it.
 */

void
seqroll::force_draw ()
{
    gui_drawingarea_gtk2::force_draw();
    draw_selection_on_window();
}

/*
 *  This function takes the given x and y screen coordinates, and returns
 *  the note and the tick via the pointer parameters.  This function is
 *  the "inverse" of convert_tn().
 *
 * \param x
 *      Provides the x value of the coordinate.
 *
 * \param y
 *      Provides the y value of the coordinate.
 *
 * \param [out] tick
 *      Provides the destination for the horizontal value in MIDI pulses.
 *
 * \param [out] note
 *      Provides the destination for the vertical value, a note value.
 */

void
seqroll::convert_xy (int x, int y, midipulse & tick, int & note)
{
    tick = x * m_zoom;
    note = (c_rollarea_y - y - 2) / c_key_y;
}

/**
 * This function takes the given note and tick, and returns the screen
 * coordinates via the pointer parameters.  This function is the "inverse"
 * of convert_xy().
 *
 * \param tick
 *      Provides the horizontal value in MIDI pulses.
 *
 * \param note
 *      Provides the vertical value, a note value.
 *
 * \param [out] x
 *      Provides the destination x value of the coordinate.
 *
 * \param [out] y
 *      Provides the destination y value of the coordinate.
 */

void
seqroll::convert_tn (midipulse tick, int note, int & x, int & y)
{
    x = tick / m_zoom;
    y = c_rollarea_y - ((note + 1) * c_key_y) - 1;
}

/**
 *  Converts rectangle corner coordinates to a starting coordinate, plus a
 *  width and height.  This function checks the mins / maxes, and then fills
 *  in the x, y, width, and height values.
 *
 *  We should refactor this function to use the utility class seqroll::rect as
 *  the destination for the conversion.
 *
 * \param x1
 *      The x value of the first corner.
 *
 * \param y1
 *      The y value of the first corner.
 *
 * \param x2
 *      The x value of the second corner.
 *
 * \param y2
 *      The y value of the second corner.
 *
 * \param [out] x
 *      The destination for the x value in pixels.
 *
 * \param [out] y
 *      The destination for the y value in pixels.
 *
 * \param [out] w
 *      The destination for the rectangle width in pixels.
 *
 * \param [out] h
 *      The destination for the rectangle height value in pixels.
 */

void
seqroll::xy_to_rect
(
    int x1, int y1, int x2, int y2,
    int & x, int & y, int & w, int & h
)
{
    if (x1 < x2)
    {
        x = x1;
        w = x2 - x1;
    }
    else
    {
        x = x2;
        w = x1 - x2;
    }
    if (y1 < y2)
    {
        y = y1;
        h = y2 - y1;
    }
    else
    {
        y = y2;
        h = y1 - y2;
    }
}

/**
 *  Converts a tick/note box to an x/y rectangle.
 *
 *  We should refactor this function to use the utility class seqroll::rect as
 *  the destination for the conversion.
 *
 * \param tick_s
 *      The starting tick of the rectangle.
 *
 * \param tick_f
 *      The finishing tick of the rectangle.
 *
 * \param note_h
 *      The high note of the rectangle.
 *
 * \param note_l
 *      The low note of the rectangle.
 *
 * \param [out] x
 *      The destination for the x value in pixels.
 *
 * \param [out] y
 *      The destination for the y value in pixels.
 *
 * \param [out] w
 *      The destination for the rectangle width in pixels.
 *
 * \param [out] h
 *      The destination for the rectangle height value in pixels.
 */

void
seqroll::convert_tn_box_to_rect
(
    midipulse tick_s, midipulse tick_f, int note_h, int note_l,
    int & x, int & y, int & w, int & h
)
{
    int x1, y1, x2, y2;
    convert_tn(tick_s, note_h, x1, y1);     /* convert box to X,Y values */
    convert_tn(tick_f, note_l, x2, y2);
    xy_to_rect(x1, y1, x2, y2, x, y, w, h);
    h += c_key_y;
}

/**
 *  Starts a paste operation.
 */

void
seqroll::start_paste ()
{
    snap_x(m_current_x);
    snap_y(m_current_y);                        // was snap_y(m_current_x) !!
    m_drop_x = m_current_x;
    m_drop_y = m_current_y;
    m_paste = true;

    /*
     * Get the box that selected elements are in.
     */

    midipulse tick_s, tick_f;
    int note_h, note_l;
    m_seq.get_clipboard_box(tick_s, note_h, tick_f, note_l);
    convert_tn_box_to_rect
    (
        tick_s, tick_f, note_h, note_l,
        m_selected.x, m_selected.y, m_selected.width, m_selected.height
    );
    m_selected.x += m_drop_x;
    m_selected.y = m_drop_y;
}

/**
 *  Performs a 'snap' operation on the x coordinate.  This function is
 *  similar to snap_y(), but it calculates a modulo value from the snap
 *  and zoom settings.
 *
 *      -   m_snap = number pulses to snap to
 *      -   m_zoom = number of pulses per pixel
 *
 *  Therefore, m_snap / m_zoom = number pixels to snap to.
 *
 * \param [out] x
 *      Provides the x-value to be snapped and returned.  A return value would
 *      be better.
 */

void
seqroll::snap_x (int & x)
{
    int mod = m_snap / m_zoom;
    if (mod <= 0)
        mod = 1;

    x -= x % mod;
}

/**
 *  Function to allow motion of the selection box via the arrow
 *  keys.
 *
 *  Could let the Enter key to deselect the moved notes.  Currently, with them
 *  mouse, selecting all notes, copying them, and moving the selection box,
 *  pasting can be completed with either a left-click or the Enter key.
 *
 * \param dx
 *      The amount to move the selection box.  Values are -1, 0, or 1.  -1 is
 *      left one snap, 0 is no movement horizontally, and 1 is right one snap.
 *
 * \param dy
 *      The amount to move the selection box.  Values are -1, 0, or 1.  -1 is
 *      up one snap, 0 is no movement vertically, and 1 is down one snap.
 */

void
seqroll::move_selection_box (int dx, int dy)
{
    int x = m_old.x + dx * m_snap / m_zoom;
    int y = m_old.y + dy * c_key_y;
    m_current_x = x + m_scroll_offset_x;
    m_current_y = y + m_scroll_offset_y;
    snap_y(m_current_y);

    midipulse tick;
    int note;
    convert_xy(0, m_current_y, tick, note);
    m_seqkeys_wid.set_hint_key(note);
    snap_x(m_current_x);
    draw_selection_on_window();
    m_old.x = x;
    m_old.y = y;
}

/**
 *  Proposed new function to encapsulate the movement of selections even
 *  more fully.
 *
 *  Note that the movement vertically is different for the selection box versus
 *  the notes.  While the movement values are -1, 0, or 1, the differences are
 *  as follows:
 *
 *      -   Selection box vertical movement:
 *          -   -1 is up one note snap.
 *          -   0 is no vertical movement.
 *          -   +1 is down one note snap.
 *      -   Note vertical movement:
 *          -   -1 is down one note.
 *          -   0 is no note vertical movement.
 *          -   +1 is up one note.
 *
 * \param dx
 *      The amount to move the selection box or the selection horizontally.
 *      Values are -1 (left one time snap), 0 (no movement), and +1 (right one
 *      snap).  Obviously values other than +-1 can be used for larger
 *      movement, but the GUI doesn't yet support that ... we could implement
 *      movement by "pages" some day.
 *
 * \param dy
 *      The amount to move the selection box or the selection vertically.  See
 *      the notes above.
 */

void
seqroll::move_selected_notes (int dx, int dy)
{
    if (m_paste)
    {
        move_selection_box(dx, dy);
    }
    else
    {
        int snap_x = dx * m_snap;                   /* time-stamp snap  */
        int snap_y = -dy;                           /* note pitch snap  */

        /*
         * Moved to sequence::move_selected_notes(): perf().modify();
         */

        if (m_seq.any_selected_notes())
        {
            m_seq.move_selected_notes(snap_x, snap_y);
        }
        else
        {
            if (snap_x != 0)
                m_seq.set_last_tick(m_seq.get_last_tick() + snap_x);
        }
    }
}

/**
 *  Proposed new function to encapsulate the movement of selections even
 *  more fully.
 *
 * \param dx
 *      The amount to grow the selection horizontally.  Values are -1 (left one
 *      time snap), 0 (no stretching), and +1 (right one snap).  Obviously
 *      values other than +-1 can be used for larger stretching, but the GUI
 *      doesn't yet support that.
 */

void
seqroll::grow_selected_notes (int dx)
{
    if (! m_paste)
    {
        int snap_x = dx * m_snap;                   /* time-stamp snap  */
        m_growing = true;
        m_seq.push_undo();
        m_seq.grow_selected(snap_x);

        /*
         * Moved to sequence::grow_selected(): perf().modify();
         */
    }
}

/**
 *  Implements the on-realize event handling.
 */

void
seqroll::on_realize ()
{
    gui_drawingarea_gtk2::on_realize();
    set_flags(Gtk::CAN_FOCUS);
    m_hadjust.signal_value_changed().connect
    (
        mem_fun(*this, &seqroll::change_horz)
    );
    m_vadjust.signal_value_changed().connect
    (
        mem_fun(*this, &seqroll::change_vert)
    );
    update_sizes();
}

/**
 *  Implements the on-expose event handling.
 *
 * \param ev
 *      The expose event to process.
 *
 * \return
 *      Always returns true.
 */

bool
seqroll::on_expose_event (GdkEventExpose * ev)
{
    GdkRectangle & area = ev->area;
    draw_drawable(area.x, area.y, area.x, area.y, area.width, area.height);
    draw_selection_on_window();
    return true;
}

/**
 *  Implements the on-button-press event handling.
 *
 * \param ev
 *      The expose event to process.
 *
 * \return
 *      Returns the result of the Seq24 interaction or the Fruity interaction,
 *      that is, the return value of
 *      Seq24SeqRollInput::on_button_press_event() or
 *      FruitySeqRollInput::on_button_press_event().
 */

bool
seqroll::on_button_press_event (GdkEventButton * ev)
{
    bool result;
    if (rc().interaction_method() == e_seq24_interaction)
        result = m_seq24_interaction.on_button_press_event(ev, *this);
    else
        result = m_fruity_interaction.on_button_press_event(ev, *this);

    if (result)
        perf().modify();

    return result;
}

/**
 *  Implements the on-button-release event handling.  This function checks the
 *  "rc" interaction-method option, and calls the forwarding function for the
 *  seq24 or the fruity interaction method.  Might be a good case to prefer
 *  inheritance and not try to support changing the interaction method without
 *  a restart of Sequencer64.
 *
 * \param ev
 *      The button release event to process.
 *
 * \return
 *      Returns the return value of
 *      Seq24SeqRollInput::on_button_release_event() or
 *      FruitySeqRollInput::on_button_release_event().
 */

bool
seqroll::on_button_release_event (GdkEventButton * ev)
{
    bool result;
    if (rc().interaction_method() == e_seq24_interaction)
        result = m_seq24_interaction.on_button_release_event(ev, *this);
    else
        result = m_fruity_interaction.on_button_release_event(ev, *this);

    if (result)
        perf().modify();

    return result;
}

/**
 *  Implements the on-motion-notify event handling.
 *
 * \param ev
 *      The motion-notification event to process.
 *
 * \return
 *      Returns the return value of
 *      Seq24SeqRollInput::on_motion_notify_event() or
 *      FruitySeqRollInput::on_motion_notify_event().
 */

bool
seqroll::on_motion_notify_event (GdkEventMotion * ev)
{
    bool result;
    if (rc().interaction_method() == e_seq24_interaction)
        result = m_seq24_interaction.on_motion_notify_event(ev, *this);
    else
        result = m_fruity_interaction.on_motion_notify_event(ev, *this);

    if (result)
        perf().modify();

    return result;
}

/**
 *  Implements the on-enter-notify event handling.
 *  Calls m_seqkeys_wid.set_hint_state(true).
 *  Parameter "ev" is the event-crossing event, not used.
 *
 * \return
 *      Always returns false.
 */

bool
seqroll::on_enter_notify_event (GdkEventCrossing *)
{
    m_seqkeys_wid.set_hint_state(true);
    return false;
}

/**
 *  Implements the on-leave-notify event handling.
 *  Calls m_seqkeys_wid.set_hint_state(false).
 *  Parameter "ev" is the event-crossing event, not used.
 *
 * \return
 *      Always returns false.
 */

bool
seqroll::on_leave_notify_event (GdkEventCrossing *)
{
    m_seqkeys_wid.set_hint_state(false);
    return false;
}

/**
 *  Implements the on-focus event handling.
 *  Sets the GDK HAS_FOCUS flag.
 *  Parameter "ev" is the event-focus event, not used.
 *
 * \return
 *      Always returns false.
 */

bool
seqroll::on_focus_in_event (GdkEventFocus *)
{
    set_flags(Gtk::HAS_FOCUS);
    return false;
}

/**
 *  Implements the on-unfocus event handling.
 *  Resets the GDK HAS_FOCUS flag.
 *  Parameter "ev" is the event-focus event, not used.
 *
 * \return
 *      Always returns false.
 */

bool
seqroll::on_focus_out_event (GdkEventFocus *)
{
    unset_flags(Gtk::HAS_FOCUS);
    return false;
}

/**
 *  Implements the on-key-press event handling.  The start/end key may be
 *  the same key (i.e. SPACEBAR).  Allow toggling when the same key is
 *  mapped to both triggers (i.e. SPACEBAR).
 *
 *  Concerning the usage of the arrow keys in this function: This code is
 *  reached, but has no visible effect.  Why?  I think they were meant to move
 *  the point for playback.  We may have a bug with our new handling of
 *  triggers (unlikely), or maybe these depend upon the proper playback mode.
 *  In any case, the old functionality is preserved.  However, if there are
 *  notes selected, then these keys support selection movement.
 *
 *  Since the Up and Down arrow keys are used for movement, we'd have to
 *  check selection status before trying to use them to move up and down in
 *  the piano roll, in smaller steps than the new Page-Up and Page-Down key
 *  support.
 *
 * \param ev
 *      The key-press event to process.
 *
 * \return
 *      Returns true if the key-press was handled.
 */

bool
seqroll::on_key_press_event (GdkEventKey * ev)
{
    bool result = false;
    keystroke k(ev->keyval, SEQ64_KEYSTROKE_PRESS, ev->state);
    bool startstop = perf().playback_key_event(k);
    if (startstop)
    {
        /*
         * No code, handled in perform::playback_key_event() now.
         */
    }
    else
    {
        if (ev->state & SEQ64_CONTROL_MASK)
        {
            if (OR_EQUIVALENT(ev->keyval, SEQ64_x, SEQ64_X))        /* cut */
            {
                m_seq.cut_selected();

                /*
                 * Moved to sequence::cut_selected(): perf().modify();
                 */

                result = true;
            }
            else if (OR_EQUIVALENT(ev->keyval, SEQ64_c, SEQ64_C))   /* copy */
            {
                m_seq.copy_selected();
                result = true;
            }
            else if (OR_EQUIVALENT(ev->keyval, SEQ64_v, SEQ64_V))   /* paste */
            {
                start_paste();

                /*
                 * Moved to sequence::paste_selected(): perf().modify();
                 */

                result = true;
            }
            else if (OR_EQUIVALENT(ev->keyval, SEQ64_z, SEQ64_Z))   /* Undo */
            {
                m_seq.pop_undo();
                result = true;
            }
            else if (OR_EQUIVALENT(ev->keyval, SEQ64_a, SEQ64_A))   /* sel all */
            {
                m_seq.select_all();
                result = true;
            }
            else if (ev->keyval == SEQ64_Left)
            {
                grow_selected_notes(-1);
                result = true;
            }
            else if (ev->keyval == SEQ64_Right)
            {
                grow_selected_notes(1);
                result = true;
            }
        }
        else
        {
            /**
             * I think we should be able to move and remove notes while
             * playing, which is already supported using the mouse.
             *
             * if (! perf().is_playing)
             */

            if (OR_EQUIVALENT(ev->keyval, SEQ64_Delete, SEQ64_BackSpace))
            {
                m_seq.cut_selected(false);      /* does not copy the events */

                /*
                 * Moved to sequence::cut_selected(): perf().modify();
                 */

                result = true;
            }
            else if (ev->keyval == SEQ64_Home)
            {
                m_seq.set_last_tick(0);
                result = true;
            }
            else if (ev->keyval == SEQ64_Left)
            {
                move_selected_notes(-1, 0);
                result = true;
            }
            else if (ev->keyval == SEQ64_Right)
            {
                move_selected_notes(1, 0);
                result = true;
            }
            else if (ev->keyval == SEQ64_Down)
            {
                move_selected_notes(0, 1);
                result = true;
            }
            else if (ev->keyval == SEQ64_Up)
            {
                move_selected_notes(0, -1);
                result = true;
            }
            else if (CAST_OR_EQUIVALENT(ev->keyval, SEQ64_Return, SEQ64_KP_Enter))
            {
                if (m_paste)
                {
                    /*
                     * This code is similar to that in Seq24SeqRollInput ::
                     * on_button_press_event().
                     */

                    midipulse tick;
                    int note;
                    m_seq.push_undo();
                    convert_xy(m_current_x, m_current_y, tick, note);
                    m_seq.paste_selected(tick, note);
                    m_paste = false;
                }
                if (m_growing)
                    m_growing = false;

                if (m_moving)
                    m_moving = false;

                m_selecting = false;
                m_selected.x = m_selected.y = m_selected.width = 
                    m_selected.height = 0;

                m_seq.unselect();
                result = true;
            }
            else if (ev->keyval == SEQ64_p)
            {
                m_seq24_interaction.set_adding(true, *this);
                result = true;
            }
            else if (ev->keyval == SEQ64_x)             /* "x-scape" the mode   */
            {
                m_seq24_interaction.set_adding(false, *this);
                result = true;
            }
        }
    }
    if (result)
        m_seq.set_dirty();

    return result;
}

/**
 *  Implements the on-size-allocate event handling.  Calls the base-class
 *  version of this function and sets m_window_x and m_window_y to the width
 *  and height of the allocation parameter.  Then calls update_sizes().
 *
 * \param a
 *      The GDK allocation event object.
 */

void
seqroll::on_size_allocate (Gtk::Allocation & a)
{
    Gtk::DrawingArea::on_size_allocate(a);
    m_window_x = a.get_width();
    m_window_y = a.get_height();
    update_sizes();
}

/**
 *  Implements the on-scroll event handling.  This scroll event only
 *  handles basic scrolling without any modifier keys such as
 *  SEQ64_CONTROL_MASK or SEQ64_SHIFT_MASK.  The seqedit class handles that
 *  fun stuff.
 *
 *  Note that this function seems to duplicate the functionality of
 *  seqkeys::on_scroll_event().  Do we really need both?  Which one do we
 *  need?
 *
 * \param ev
 *      The scroll event to process.
 *
 * \return
 *      Returns true if the scroll event was handled.
 */

bool
seqroll::on_scroll_event (GdkEventScroll * ev)
{
    guint modifiers;                /* used to filter out caps/num lock etc. */
    modifiers = gtk_accelerator_get_default_mod_mask();
    if ((ev->state & modifiers) != 0)
        return false;

    double val = m_vadjust.get_value();
    if (CAST_EQUIVALENT(ev->direction, SEQ64_SCROLL_UP))
        val -= m_vadjust.get_step_increment() / 6;
    else if (CAST_EQUIVALENT(ev->direction, SEQ64_SCROLL_DOWN))
        val += m_vadjust.get_step_increment() / 6;
    else
        return true;

    m_vadjust.clamp_page(val, val + m_vadjust.get_page_size());
    return true;
}

}           // namespace seq64

/*
 * seqroll.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

