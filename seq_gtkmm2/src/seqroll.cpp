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
 * \updates       2015-11-23
 * \license       GNU GPLv2 or above
 *
 */

#include <gtkmm/accelkey.h>
#include <gtkmm/adjustment.h>
#include <gtkmm/menu.h>

#include "app_limits.h"                 /* SEQ64_SOLID_PIANOROLL_GRID   */
#include "event.hpp"
#include "gdk_basic_keys.h"
#include "scales.h"
#include "seqroll.hpp"
#include "seqdata.hpp"
#include "seqevent.hpp"
#include "sequence.hpp"
#include "seqkeys.hpp"
#include "perform.hpp"

namespace seq64
{

/**
 *  Principal constructor.
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
    m_old                   (),
    m_selected              (),
    m_seq                   (seq),
    m_clipboard             (new sequence()),
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
    m_move_snap_offset_x    (0),
    m_old_progress_x        (0),
    m_scroll_offset_ticks   (0),
    m_scroll_offset_key     (0),
    m_scroll_offset_x       (0),
    m_scroll_offset_y       (0),
    m_background_sequence   (0),
    m_drawing_background_seq(false),
    m_ignore_redraw         (false)
{
    m_ppqn = choose_ppqn(ppqn);
    grab_focus();
}

/**
 *  Provides a destructor to delete allocated objects.
 */

seqroll::~seqroll ()
{
    if (not_nullptr(m_clipboard))
        delete m_clipboard;
}

/**
 *  This function sets the given sequence onto the piano roll of the pattern
 *  editor, so that the musician can have another pattern to play against.
 *  The state parameter sets the boolean m_drawing_background_seq.
 *
 * \param state
 *      If true, the background sequence will be drawn.
 *
 * \pararm seq
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

    if (m_ignore_redraw)
        return;

    update_and_draw();  // update_background(); update_pixmap(); queue_draw();
}

/**
 *  Update the sizes of items based on zoom, PPQN, BPM, BW (beat width) and
 *  more.
 *
 *  Old comments:
 *
 *      Use m_zoom and i % m_seq->get_bpm() == 0,
 *      int numberLines = 128 / m_seq->get_bw() / m_zoom;
 *      int distance = c_ppqn / 32;
 */

void
seqroll::update_sizes ()
{
    m_hadjust.set_lower(0);                            /* set default size */
    m_hadjust.set_upper(m_seq.get_length());
    m_hadjust.set_page_size(m_window_x * m_zoom);

    /*
     * The horizontal step increment is 1 semiquaver (1/16) note per zoom
     * level.
     */

    m_hadjust.set_step_increment((m_ppqn / 4) * m_zoom);

    /*
     * The page increment is always one bar.
     */

    int page_increment = int
    (
        double(m_ppqn) * double(m_seq.get_beats_per_bar()) *
            (4.0 / double(m_seq.get_beat_width()))
    );
    m_hadjust.set_page_increment(page_increment);

    int h_max_value = (m_seq.get_length() - (m_window_x * m_zoom));
    if (m_hadjust.get_value() > h_max_value)
        m_hadjust.set_value(h_max_value);

    m_vadjust.set_lower(0);
    m_vadjust.set_upper(c_num_keys);
    m_vadjust.set_page_size(m_window_y / c_key_y);
    m_vadjust.set_step_increment(12);
    m_vadjust.set_page_increment(12);

    int v_max_value = c_num_keys - (m_window_y / c_key_y);
    if (m_vadjust.get_value() > v_max_value)
    {
        m_vadjust.set_value(v_max_value);
    }
    if (is_realized())              /* create pixmaps with window dimentions */
    {
        m_pixmap = Gdk::Pixmap::create(m_window, m_window_x, m_window_y, -1);
        m_background = Gdk::Pixmap::create(m_window, m_window_x, m_window_y, -1);
        change_vert();
    }
}

/**
 *  Change the horizontal scrolling offset and redraw.
 */

void
seqroll::change_horz ()
{
    m_scroll_offset_ticks = int(m_hadjust.get_value());
    m_scroll_offset_x = m_scroll_offset_ticks / m_zoom;
    if (m_ignore_redraw)
        return;

    update_and_draw(true);  // update_background(); update_pixmap(); force_draw();
}

/**
 *  Change the vertical scrolling offset and redraw.
 */

void
seqroll::change_vert ()
{
    m_scroll_offset_key = int(m_vadjust.get_value());
    m_scroll_offset_y = m_scroll_offset_key * c_key_y;
    if (m_ignore_redraw)
        return;

    update_and_draw(true);  // update_background(); update_pixmap(); force_draw();
}

/**
 *  This function basically resets the whole widget as if it was realized
 *  again.  It's almost identical to the change_horz() function!
 */

void
seqroll::reset ()
{
    m_scroll_offset_ticks = int(m_hadjust.get_value());
    m_scroll_offset_x = m_scroll_offset_ticks / m_zoom;
    if (m_ignore_redraw)
        return;

    update_sizes();
    update_and_draw();  // update_background(); update_pixmap(); queue_draw();
}

/**
 *  Redraws unless m_ignore_redraw is true.
 */

void
seqroll::redraw ()
{
    if (m_ignore_redraw)
        return;

    m_scroll_offset_ticks = int(m_hadjust.get_value());
    m_scroll_offset_x = m_scroll_offset_ticks / m_zoom;
    update_and_draw(true);  // update_background(); update_pixmap(); force_draw();
}

/**
 *  Redraws events unless m_ignore_redraw is true.
 *
 *  Almost: update_and_draw(true) are almost replaceable by
 *  update_background(); update_pixmap(); force_draw();
 */

void
seqroll::redraw_events ()
{
    if (m_ignore_redraw)
        return;

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
     * This could be applied here, and also to seqevent::draw_background().
     *
     * int measure_length_64ths = m_seq.get_beats_per_bar() *
     *      64 / m_seq.get_beat_width();
     * int measures_per_line = (256 / measure_length_64ths) / (32 / m_zoom);
     * if (measures_per_line <= 0)
     *      int measures_per_line = 1;
     */

    int measures_per_line = 1;
    int ticks_per_beat = (4 * m_ppqn) / m_seq.get_beat_width();
    int ticks_per_measure = m_seq.get_beats_per_bar() * ticks_per_beat;
    int ticks_per_step = 6 * m_zoom;
    int ticks_per_m_line = ticks_per_measure * measures_per_line;
    int endtick = (m_window_x * m_zoom) + m_scroll_offset_ticks;
    int starttick = m_scroll_offset_ticks -
         (m_scroll_offset_ticks % ticks_per_step);

    m_gc->set_foreground(grey());
    for (int tick = starttick; tick < endtick; tick += ticks_per_step)
    {
        int base_line = tick / m_zoom;
        if (tick % ticks_per_m_line == 0)
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
        int x = base_line - m_scroll_offset_x;
        draw_line(m_background, x, 0, x, m_window_y);
    }
#ifndef SEQ64_SOLID_PIANOROLL_GRID
    set_line(Gdk::LINE_SOLID);                  /* reset the line style     */
#endif
}

/**
 *  Sets the zoom to the given value, and then resets the view.
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
 *  Draw a progress line on the window.
 */

void
seqroll::draw_progress_on_window ()
{
    draw_drawable
    (
        m_old_progress_x, 0, m_old_progress_x, 0, 1, m_window_y
    );
    m_old_progress_x = (m_seq.get_last_tick() / m_zoom) - m_scroll_offset_x;
    if (m_old_progress_x != 0)
        draw_line(black(), m_old_progress_x, 0, m_old_progress_x, m_window_y);
}

/**
 *  Draws events on the given drawable area.
 *
 *  "Method 0" seems be the one that draws the background sequence, if active.
 *  "Method 1" draws the sequence itself.
 */

void
seqroll::draw_events_on (Glib::RefPtr<Gdk::Drawable> draw)
{
    midipulse tick_s;
    midipulse tick_f;
    int note;
    int note_x, note_y, note_width, note_height;
    bool selected;
    int velocity;
    draw_type dt;
    int starttick = m_scroll_offset_ticks ;
    int endtick = (m_window_x * m_zoom) + m_scroll_offset_ticks;
    sequence * seq = nullptr;
    for (int method = 0; method < 2; ++method)  /* weird way to do it       */
    {
        if (method == 0 && m_drawing_background_seq)
        {
            if (perf().is_active(m_background_sequence))
                seq = perf().get_sequence(m_background_sequence);
            else
                method++;
        }
        else if (method == 0)                   /* done with background?    */
            method++;                           /* now draw the sequence    */

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
                note_x = tick_s / m_zoom;       /* turn into screen coords */
                note_y = c_rollarea_y - (note * c_key_y) - c_key_y - 1 + 2;
                note_height = c_key_y - 3;
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

                /* Draw note boxes for main or background sequence */

                if (method == 0)
                {
                    /*
                     * Let's try a color more distinguishable from the
                     * "scale" color.
                     *
                     * m_gc->set_foreground(dark_grey());
                     */

                     m_gc->set_foreground(dark_cyan());
                }
                else
                    m_gc->set_foreground(black());

                draw_rectangle
                (
                    draw, note_x, note_y, note_width, note_height
                );
                if (tick_f < tick_s)
                {
                    draw_rectangle
                    (
                        draw, 0, note_y, tick_f / m_zoom, note_height
                    );
                }
                if (note_width > 3)     /* draw inside box if there is room */
                {
                    if (selected)
                        m_gc->set_foreground(orange());
                    else
                        m_gc->set_foreground(white());

                    if (method == 1)
                    {
                        if (tick_f >= tick_s)
                        {
                            draw->draw_rectangle
                            (
                                m_gc, true, note_x + 1 + in_shift,
                                note_y + 1, note_width - 3 + length_add,
                                note_height - 3
                            );
                        }
                        else
                        {
                            draw_rectangle
                            (
                                draw, note_x + 1 + in_shift, note_y + 1,
                                note_width, note_height - 3
                            );
                            draw_rectangle
                            (
                                draw,  0, note_y + 1,
                                (tick_f / m_zoom) - 3 + length_add,
                                note_height - 3
                            );
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
    int x, y, w, h;
    if (m_selecting  ||  m_moving || m_paste ||  m_growing)
    {
        set_line(Gdk::LINE_SOLID);
        draw_drawable                                   /* replace old */
        (
            m_old.x, m_old.y, m_old.x, m_old.y, m_old.width + 1, m_old.height + 1
        );
    }
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
        int delta_x = m_current_x - m_drop_x;
        int delta_y = m_current_y - m_drop_y;
        x = m_selected.x + delta_x;
        y = m_selected.y + delta_y;
        x -= m_scroll_offset_x;
        y -= m_scroll_offset_y;
        draw_rectangle(black(), x, y, m_selected.width, m_selected.height, false);
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

        x = m_selected.x;
        y = m_selected.y;
        x -= m_scroll_offset_x;
        y -= m_scroll_offset_y;
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
    draw_drawable(0, 0, 0, 0, m_window_x, m_window_y);
    draw_selection_on_window();
}

/*
 *  This function takes the given x and y screen coordinates, and returns
 *  the note and the tick via the pointer parameters.  This function is
 *  the "inverse" of convert_tn().
 */

void
seqroll::convert_xy (int a_x, int a_y, midipulse & a_tick, int & a_note)
{
    a_tick = a_x * m_zoom;
    a_note = (c_rollarea_y - a_y - 2) / c_key_y;
}

/**
 * This function takes the given note and tick, and returns the screen
 * coordinates via the pointer parameters.  This function is the "inverse"
 * of convert_xy().
 */

void
seqroll::convert_tn (midipulse a_ticks, int a_note, int & a_x, int & a_y)
{
    a_x = a_ticks /  m_zoom;
    a_y = c_rollarea_y - ((a_note + 1) * c_key_y) - 1;
}

/**
 *  This function checks the mins / maxes, and then fills in the x, y,
 *  width, and height values.
 */

void
seqroll::xy_to_rect
(
    int a_x1, int a_y1, int a_x2, int a_y2,
    int & a_x, int & a_y, int & a_w, int & a_h)
{
    if (a_x1 < a_x2)
    {
        a_x = a_x1;
        a_w = a_x2 - a_x1;
    }
    else
    {
        a_x = a_x2;
        a_w = a_x1 - a_x2;
    }
    if (a_y1 < a_y2)
    {
        a_y = a_y1;
        a_h = a_y2 - a_y1;
    }
    else
    {
        a_y = a_y2;
        a_h = a_y1 - a_y2;
    }
}

/**
 *  Converts a tick/note box to an x/y rectangle.
 */

void
seqroll::convert_tn_box_to_rect
(
    midipulse a_tick_s, midipulse a_tick_f, int a_note_h, int a_note_l,
    int & a_x, int & a_y, int & a_w, int & a_h
)
{
    int x1, y1, x2, y2;
    convert_tn(a_tick_s, a_note_h, x1, y1);     /* convert box to X,Y values */
    convert_tn(a_tick_f, a_note_l, x2, y2);
    xy_to_rect(x1, y1, x2, y2, a_x, a_y, a_w, a_h);
    a_h += c_key_y;
}

/**
 *  Starts a paste operation.
 */

void
seqroll::start_paste()
{
    midipulse tick_s;
    midipulse tick_f;
    int note_h;
    int note_l;
    snap_x(m_current_x);
    snap_y(m_current_x);
    m_drop_x = m_current_x;
    m_drop_y = m_current_y;
    m_paste = true;

    /* get the box that selected elements are in */

    m_seq.get_clipboard_box(tick_s, note_h, tick_f, note_l);
    convert_tn_box_to_rect
    (
        tick_s, tick_f, note_h, note_l,
        m_selected.x, m_selected.y, m_selected.width, m_selected.height
    );

    /* adjust for clipboard being shifted to tick 0 */

    m_selected.x += m_drop_x;
    m_selected.y += (m_drop_y - m_selected.y);
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
 */

void
seqroll::snap_x (int & x)
{
    int mod = (m_snap / m_zoom);
    if (mod <= 0)
        mod = 1;

    x -= (x % mod);
}

/**
 *  Sets the status to the given parameter, and the CC value to the given
 *  optional control parameter, which defaults to 0.  Unlike the same
 *  function in seqevent, this version does not redraw.
 */

void
seqroll::set_data_type (midibyte status, midibyte control)
{
    m_status = status;
    m_cc = control;
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
 */

bool
seqroll::on_expose_event (GdkEventExpose * e)
{
    draw_drawable
    (
        e->area.x, e->area.y, e->area.x, e->area.y,
        e->area.width, e->area.height
    );
    draw_selection_on_window();
    return true;
}

/**
 *  Implements the on-button-press event handling.
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
 *  Implements the on-button-release event handling.
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
 */

bool
seqroll::on_enter_notify_event (GdkEventCrossing *)
{
    m_seqkeys_wid.set_hint_state(true);
    return false;
}

/**
 *  Implements the on-leave-notify event handling.
 */

bool
seqroll::on_leave_notify_event (GdkEventCrossing *)
{
    m_seqkeys_wid.set_hint_state(false);
    return false;
}

/**
 *  Implements the on-focus event handling.
 */

bool
seqroll::on_focus_in_event (GdkEventFocus *)
{
    set_flags(Gtk::HAS_FOCUS);
    return false;
}

/**
 *  Implements the on-unfocus event handling.
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
 *  the point for playback.  We may HAVE A BUG with our new handling of
 *  triggers, or maybe these depend upon the proper playback mode.  In any
 *  case, the old functionality is preserved.  However, if there are notes
 *  selected, then these keys support selection movement.
 */

bool
seqroll::on_key_press_event (GdkEventKey * ev)
{
    bool result = false;
    bool startstop = OR_EQUIVALENT(ev->keyval, PREFKEY(start), PREFKEY(stop));
    if (startstop)
    {
        bool stop = PREFKEY(start) == PREFKEY(stop) ?
            perf().is_playing() : ev->keyval == PREFKEY(stop) ;

        if (stop)
            perf().stop_playing();
        else
            perf().start_playing();
    }
    else if (CAST_EQUIVALENT(ev->type, SEQ64_KEY_PRESS)) // this really needed?
    {
        /*
         * I think we should be able to move and remove notes while playing,
         * which is already supported using the mouse.
         *
         * if (! perf().is_playing)
         */

        if (OR_EQUIVALENT(ev->keyval, SEQ64_Delete, SEQ64_BackSpace))
        {
            m_seq.push_undo();
            m_seq.mark_selected();
            m_seq.remove_marked();
            perf().modify();
            result = true;
        }
        else if (ev->keyval == SEQ64_Home)
        {
            m_seq.set_orig_tick(0);
            result = true;
        }
        else if (ev->keyval == SEQ64_Left)
        {
            perf().modify();
            result = true;
            if (m_seq.any_selected_notes())
                m_seq.move_selected_notes(-m_snap, /*-48,*/ 0);
            else
                m_seq.set_orig_tick(m_seq.get_last_tick() - m_snap);
        }
        else if (ev->keyval == SEQ64_Right)
        {
            perf().modify();
            result = true;
            if (m_seq.any_selected_notes())
                m_seq.move_selected_notes(m_snap, /*48,*/ 0);
            else
                m_seq.set_orig_tick(m_seq.get_last_tick() + m_snap);
        }
        else if (ev->keyval == SEQ64_Down)
        {
            perf().modify();
            if (m_seq.any_selected_notes())
            {
                m_seq.move_selected_notes(0, -1);
                result = true;
            }
        }
        else if (ev->keyval == SEQ64_Up)
        {
            perf().modify();
            if (m_seq.any_selected_notes())
            {
                m_seq.move_selected_notes(0, 1);
                result = true;
            }
        }
#if SEQ64_USE_VI_SEQROLL_MODE       /* disabled, for programmers only! :-D  */
        else if (ev->keyval == SEQ64_h)
        {
            if (m_seq.any_selected_notes())
            {
                m_seq.move_selected_notes(-m_snap, /*-48,*/ 0);
                perf().modify();
                result = true;
            }
        }
        else if (ev->keyval == SEQ64_l)
        {
            if (m_seq.any_selected_notes())
            {
                m_seq.move_selected_notes(m_snap, /*48,*/ 0);
                perf().modify();
                result = true;
            }
        }
        else if (ev->keyval == SEQ64_j)
        {
            if (m_seq.any_selected_notes())
            {
                m_seq.move_selected_notes(0, -1);
                perf().modify();
                result = true;
            }
        }
        else if (ev->keyval == SEQ64_k)
        {
            if (m_seq.any_selected_notes())
            {
                m_seq.move_selected_notes(0, 1);
                perf().modify();
                result = true;
            }
        }
        else if (ev->keyval == SEQ64_i)
        {
            m_seq24_interaction.set_adding(true, *this);
            result = true;
        }
        else if (ev->keyval == SEQ64_I)             /* escape is stop-play  */
        {
            m_seq24_interaction.set_adding(false, *this);
            result = true;
        }
#endif  // SEQ64_USE_VI_SEQROLL_MODE
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
        if (ev->state & SEQ64_CONTROL_MASK)
        {
            if (OR_EQUIVALENT(ev->keyval, SEQ64_x, SEQ64_X))        /* cut */
            {
                m_seq.push_undo();
                m_seq.copy_selected();
                m_seq.mark_selected();
                m_seq.remove_marked();
                perf().modify();
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
                perf().modify();
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
        }
    }
    if (result)
        m_seq.set_dirty();             // redraw_events();

    return result;
}

/**
 *  Implements the on-size-allocate event handling.
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
 *  SEQ64_CONTROL_MASK or SEQ64_SHIFT_MASK.
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

