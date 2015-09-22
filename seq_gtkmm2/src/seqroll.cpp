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
 *  roll of the patterns editor.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2015-09-16
 * \license       GNU GPLv2 or above
 *
 */

#include <gtkmm/accelkey.h>
#include <gtkmm/adjustment.h>
#include <gtkmm/menu.h>

#include "event.hpp"
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
    perform * a_perf,
    sequence * a_seq,
    int a_zoom,
    int a_snap,
    seqdata * a_seqdata_wid,
    seqevent * a_seqevent_wid,
    seqkeys * a_seqkeys_wid,
    int a_pos,
    Gtk::Adjustment * a_hadjust,
    Gtk::Adjustment * a_vadjust
) :
    m_gc                    (),
    m_window                (),
    m_black                 (Gdk::Color("black")),
    m_white                 (Gdk::Color("white")),
    m_grey                  (Gdk::Color("gray")),
    m_dk_grey               (Gdk::Color("gray50")),
    m_orange                (Gdk::Color("orange")),
    m_pixmap                (),
    m_mainperf              (a_perf),
    m_window_x              (10),       // why so small?
    m_window_y              (10),
    m_current_x             (0),
    m_current_y             (0),
    m_drop_x                (0),
    m_drop_y                (0),
    m_vadjust               (a_vadjust),
    m_hadjust               (a_hadjust),
    m_background            (),         // another pixmap
    m_old                   (),
    m_selected              (),
    m_seq                   (a_seq),
    m_clipboard             (new sequence()),
    m_seqdata_wid           (a_seqdata_wid),
    m_seqevent_wid          (a_seqevent_wid),
    m_seqkeys_wid           (a_seqkeys_wid),
    m_fruity_interaction    (),
    m_seq24_interaction     (),
    m_pos                   (a_pos),
    m_zoom                  (a_zoom),
    m_snap                  (a_snap),
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
    Glib::RefPtr<Gdk::Colormap> colormap = get_default_colormap();
    colormap->alloc_color(m_black);
    colormap->alloc_color(m_white);
    colormap->alloc_color(m_grey);
    colormap->alloc_color(m_dk_grey);
    colormap->alloc_color(m_orange);

//  m_clipboard = new sequence();

    add_events
    (
        Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK |
        Gdk::POINTER_MOTION_MASK | Gdk::KEY_PRESS_MASK |
        Gdk::KEY_RELEASE_MASK | Gdk::FOCUS_CHANGE_MASK |
        Gdk::ENTER_NOTIFY_MASK | Gdk::LEAVE_NOTIFY_MASK |
        Gdk::SCROLL_MASK
    );
    set_double_buffered(false);
}

/**
 *  Provides a destructor to delete allocated objects.
 */

seqroll::~seqroll ()
{
    delete m_clipboard;
}

/**
 *  This function sets the given sequence onto the piano roll of the
 *  pattern editor, so that the musician can have another pattern to play
 *  against.  The a_state parameter sets the boolean
 *  m_drawing_background_seq.
 */

void
seqroll::set_background_sequence (bool a_state, int a_seq)
{
    m_drawing_background_seq = a_state;
    m_background_sequence = a_seq;
    if (m_ignore_redraw)
        return;

    update_background();
    update_pixmap();
    queue_draw();
}

/*
 *  use m_zoom and "i % m_seq->get_bpm() == 0"
 *
 *  int numberLines = 128 / m_seq->get_bw() / m_zoom;
 *  int distance = c_ppqn / 32;
 */

/**
 *  Update the sizes of items based on zoom, PPQN, BPM, BW (beat width) and
 *  more.
 */

void
seqroll::update_sizes ()
{
    m_hadjust->set_lower(0);                            /* set default size */
    m_hadjust->set_upper(m_seq->get_length());
    m_hadjust->set_page_size(m_window_x * m_zoom);

    /*
     * The horizontal step increment is 1 semiquaver (1/16) note per zoom
     * level.
     */

    m_hadjust->set_step_increment((c_ppqn / 4) * m_zoom);

    /*
     * The page increment is always one bar.
     */

    int page_increment = int
    (
        double(c_ppqn) * double(m_seq->get_bpm()) * (4.0 / double(m_seq->get_bw()))
    );
    m_hadjust->set_page_increment(page_increment);

    int h_max_value = (m_seq->get_length() - (m_window_x * m_zoom));
    if (m_hadjust->get_value() > h_max_value)
        m_hadjust->set_value(h_max_value);

    m_vadjust->set_lower(0);
    m_vadjust->set_upper(c_num_keys);
    m_vadjust->set_page_size(m_window_y / c_key_y);
    m_vadjust->set_step_increment(12);
    m_vadjust->set_page_increment(12);

    int v_max_value = c_num_keys - (m_window_y / c_key_y);
    if (m_vadjust->get_value() > v_max_value)
    {
        m_vadjust->set_value(v_max_value);
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
    m_scroll_offset_ticks = (int) m_hadjust->get_value();
    m_scroll_offset_x = m_scroll_offset_ticks / m_zoom;
    if (m_ignore_redraw)
        return;

    update_background();
    update_pixmap();
    force_draw();
}

/**
 *  Change the vertical scrolling offset and redraw.
 */

void
seqroll::change_vert ()
{
    m_scroll_offset_key = (int) m_vadjust->get_value();
    m_scroll_offset_y = m_scroll_offset_key * c_key_y;
    if (m_ignore_redraw)
        return;

    update_background();
    update_pixmap();
    force_draw();
}

/**
 *  This function basically resets the whole widget as if it was realized
 *  again.  It's almost identical to the change_horz() function!
 */

void
seqroll::reset ()
{
    m_scroll_offset_ticks = (int) m_hadjust->get_value();
    m_scroll_offset_x = m_scroll_offset_ticks / m_zoom;
    if (m_ignore_redraw)
        return;

    update_sizes();
    update_background();
    update_pixmap();
    queue_draw();
}

/**
 *  Redraws unless m_ignore_redraw is true.
 */

void
seqroll::redraw ()
{
    if (m_ignore_redraw)
        return;

    m_scroll_offset_ticks = (int) m_hadjust->get_value();
    m_scroll_offset_x = m_scroll_offset_ticks / m_zoom;
    update_background();
    update_pixmap();
    force_draw();
}

/**
 *  Redraws events unless m_ignore_redraw is true.
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
 *  Updates the background of this window.
 */

void
seqroll::update_background ()
{
    m_gc->set_foreground(m_white);              /* clear background */
    m_background->draw_rectangle(m_gc, true, 0, 0, m_window_x, m_window_y);

    m_gc->set_foreground(m_grey);               /* draw horz grey lines */
    m_gc->set_line_attributes
    (
        1, Gdk::LINE_ON_OFF_DASH, Gdk::CAP_NOT_LAST, Gdk::JOIN_MITER
    );
    gint8 dash = 1;
    m_gc->set_dashes(0, &dash, 1);
    for (int i = 0; i < (m_window_y / c_key_y) + 1; i++)
    {
        int remkeys = c_num_keys - i;               /* remaining keys?      */
        int octkey = OCTAVE_SIZE - m_key;           /* used three times     */
        int modkey = (remkeys - m_scroll_offset_key + octkey);
        if (global_interactionmethod == e_fruity_interaction)
        {
            if ((modkey % OCTAVE_SIZE) == 0)
            {
                m_gc->set_foreground(m_dk_grey);    /* draw horz lines at C */
                m_gc->set_line_attributes
                (
                    1, Gdk::LINE_SOLID, Gdk::CAP_NOT_LAST, Gdk::JOIN_MITER
                );
            }
            else if ((modkey % OCTAVE_SIZE) == (OCTAVE_SIZE-1))
            {
                m_gc->set_foreground(m_grey); /* horz grey lines, other notes */
                m_gc->set_line_attributes
                (
                    1, Gdk::LINE_ON_OFF_DASH, Gdk::CAP_NOT_LAST, Gdk::JOIN_MITER
                );
            }
        }
        m_background->draw_line(m_gc, 0, i * c_key_y, m_window_x, i * c_key_y);
        if (m_scale != c_scale_off)
        {
            if (! c_scales_policy[m_scale][(modkey - 1) % OCTAVE_SIZE])
            {
                m_background->draw_rectangle
                (
                    m_gc, true, 0, i * c_key_y + 1, m_window_x, c_key_y - 1
                );
            }
        }
    }

    /*
     * int measure_length_64ths = m_seq->get_bpm() * 64 / m_seq->get_bw();
     * int measures_per_line = (256 / measure_length_64ths) / (32 / m_zoom);
     * if ( measures_per_line <= 0
     */

    int measures_per_line = 1;
    int ticks_per_measure =  m_seq->get_bpm() * (4 * c_ppqn) / m_seq->get_bw();
    int ticks_per_beat = (4 * c_ppqn) / m_seq->get_bw();
    int ticks_per_step = 6 * m_zoom;
    int ticks_per_m_line =  ticks_per_measure * measures_per_line;
    int end_tick = (m_window_x * m_zoom) + m_scroll_offset_ticks;
    int start_tick = m_scroll_offset_ticks -
         (m_scroll_offset_ticks % ticks_per_step);

    m_gc->set_foreground(m_grey);
    for (int i = start_tick; i < end_tick; i += ticks_per_step)
    {
        int base_line = i / m_zoom;
        if (i % ticks_per_m_line == 0)
        {
            m_gc->set_foreground(m_black);      /* solid line on every beat */
            m_gc->set_line_attributes
            (
                1, Gdk::LINE_SOLID, Gdk::CAP_NOT_LAST, Gdk::JOIN_MITER
            );
        }
        else if (i % ticks_per_beat == 0)
        {
            m_gc->set_foreground(m_dk_grey);
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

            int i_snap = i - (i % m_snap);
            if (i == i_snap)
                m_gc->set_foreground(m_dk_grey);
            else
                m_gc->set_foreground(m_grey);

            gint8 dash = 1;
            m_gc->set_dashes(0, &dash, 1);
        }
        m_background->draw_line
        (
            m_gc, base_line - m_scroll_offset_x, 0,
            base_line - m_scroll_offset_x, m_window_y
        );
    }
    m_gc->set_line_attributes                   /* reset line style */
    (
        1, Gdk::LINE_SOLID, Gdk::CAP_NOT_LAST, Gdk::JOIN_MITER
    );
}

/**
 *  Sets the zoom to the given value, and then resets the view.
 */

void
seqroll::set_zoom (int a_zoom)
{
    if (m_zoom != a_zoom)
    {
        m_zoom = a_zoom;
        reset();
    }
}

/**
 *  Sets the music scale to the given value, and then resets the view.
 */

void
seqroll::set_scale (int a_scale)
{
    if (m_scale != a_scale)
    {
        m_scale = a_scale;
        reset();
    }
}

/**
 *  Sets the music key to the given value, and then resets the view.
 */

void
seqroll::set_key (int a_key)
{
    if (m_key != a_key)
    {
        m_key = a_key;
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
    m_window->draw_drawable
    (
        m_gc, m_pixmap, m_old_progress_x, 0, m_old_progress_x, 0, 1, m_window_y
    );
    m_old_progress_x = (m_seq->get_last_tick() / m_zoom) - m_scroll_offset_x;
    if (m_old_progress_x != 0)
    {
        m_gc->set_foreground(m_black);
        m_window->draw_line
        (
            m_gc, m_old_progress_x, 0, m_old_progress_x, m_window_y
        );
    }
}

/**
 *  Draws events on the given drawable area.
 */

void
seqroll::draw_events_on (Glib::RefPtr<Gdk::Drawable> a_draw)
{
    long tick_s;
    long tick_f;
    int note;
    int note_x, note_y, note_width, note_height;
    bool selected;
    int velocity;
    draw_type dt;
    int start_tick = m_scroll_offset_ticks ;
    int end_tick = (m_window_x * m_zoom) + m_scroll_offset_ticks;
    sequence * seq = nullptr;
    for (int method = 0; method < 2; ++method)
    {
        if (method == 0 && m_drawing_background_seq)
        {
            if (m_mainperf->is_active(m_background_sequence))
                seq = m_mainperf->get_sequence(m_background_sequence);
            else
                method++;
        }
        else if (method == 0)
            method++;

        if (method == 1)
            seq = m_seq;

        m_gc->set_foreground(m_black);          /* draw boxes from sequence */
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
                (tick_s >= start_tick && tick_s <= end_tick) ||
                ((dt == DRAW_NORMAL_LINKED) &&
                    (tick_f >= start_tick && tick_f <= end_tick))
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
                        if (note_width < 1) note_width = 1;
                    }
                    else
                        note_width = (m_seq->get_length() - tick_s) / m_zoom;
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
                m_gc->set_foreground(m_black);

                /* draw boxes from sequence */

                if (method == 0)
                    m_gc->set_foreground(m_dk_grey);

                a_draw->draw_rectangle
                (
                    m_gc, true, note_x, note_y, note_width, note_height
                );
                if (tick_f < tick_s)
                {
                    a_draw->draw_rectangle
                    (
                        m_gc, true, 0, note_y, tick_f / m_zoom, note_height
                    );
                }

                if (note_width > 3)     /* draw inside box if there is room */
                {
                    if (selected)
                        m_gc->set_foreground(m_orange);
                    else
                        m_gc->set_foreground(m_white);

                    if (method == 1)
                    {
                        if (tick_f >= tick_s)
                        {
                            a_draw->draw_rectangle
                            (
                                m_gc, true, note_x + 1 + in_shift,
                                note_y + 1, note_width - 3 + length_add,
                                note_height - 3
                            );
                        }
                        else
                        {
                            a_draw->draw_rectangle
                            (
                                m_gc, true, note_x + 1 + in_shift,
                                note_y + 1, note_width,
                                note_height - 3
                            );
                            a_draw->draw_rectangle
                            (
                                m_gc, true, 0,
                                note_y + 1, (tick_f / m_zoom) - 3 + length_add,
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
        m_gc->set_line_attributes
        (
            1, Gdk::LINE_SOLID, Gdk::CAP_NOT_LAST, Gdk::JOIN_MITER
        );

        m_window->draw_drawable                         /* replace old */
        (
            m_gc, m_pixmap, m_old.x, m_old.y, m_old.x, m_old.y,
            m_old.width + 1, m_old.height + 1
        );
    }
    if (m_selecting)
    {
        xy_to_rect(m_drop_x, m_drop_y, m_current_x, m_current_y, &x, &y, &w, &h);
        x -= m_scroll_offset_x;
        y -= m_scroll_offset_y;

        m_old.x = x;
        m_old.y = y;
        m_old.width = w;
        m_old.height = h + c_key_y;

        m_gc->set_foreground(m_black);
        m_window->draw_rectangle(m_gc, false, x, y, w, h + c_key_y);
    }
    if (m_moving || m_paste)
    {
        int delta_x = m_current_x - m_drop_x;
        int delta_y = m_current_y - m_drop_y;
        x = m_selected.x + delta_x;
        y = m_selected.y + delta_y;
        x -= m_scroll_offset_x;
        y -= m_scroll_offset_y;
        m_gc->set_foreground(m_black);
        m_window->draw_rectangle
        (
            m_gc, false, x, y, m_selected.width, m_selected.height
        );
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
        m_gc->set_foreground(m_black);
        m_window->draw_rectangle(m_gc, false, x, y, width, m_selected.height);

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
    m_window->draw_drawable(m_gc, m_pixmap, 0, 0, 0, 0, m_window_x, m_window_y);
    draw_selection_on_window();
}

/*
 *  This function takes the given x and y screen coordinates, and returns
 *  the note and the tick via the pointer parameters.  This function is
 *  the "inverse" of convert_tn().
 */

void
seqroll::convert_xy (int a_x, int a_y, long * a_tick, int * a_note)
{
    *a_tick = a_x * m_zoom;
    *a_note = (c_rollarea_y - a_y - 2) / c_key_y;
}

/**
 * This function takes the given note and tick, and returns the screen
 * coordinates via the pointer parameters.  This function is the "inverse"
 * of convert_xy().
 */

void
seqroll::convert_tn (long a_ticks, int a_note, int * a_x, int * a_y)
{
    *a_x = a_ticks /  m_zoom;
    *a_y = c_rollarea_y - ((a_note + 1) * c_key_y) - 1;
}

/**
 *  This function checks the mins / maxes, and then fills in the x, y,
 *  width, and height values.
 */

void
seqroll::xy_to_rect
(
    int a_x1, int a_y1, int a_x2, int a_y2,
    int * a_x, int * a_y, int * a_w, int * a_h)
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
    if (a_y1 < a_y2)
    {
        *a_y = a_y1;
        *a_h = a_y2 - a_y1;
    }
    else
    {
        *a_y = a_y2;
        *a_h = a_y1 - a_y2;
    }
}

/**
 *  Converts a tick/note box to an x/y rectangle.
 */

void
seqroll::convert_tn_box_to_rect
(
    long a_tick_s, long a_tick_f, int a_note_h, int a_note_l,
    int * a_x, int * a_y, int * a_w, int * a_h
)
{
    int x1, y1, x2, y2;
    convert_tn(a_tick_s, a_note_h, &x1, &y1);   /* convert box to X,Y values */
    convert_tn(a_tick_f, a_note_l, &x2, &y2);
    xy_to_rect(x1, y1, x2, y2, a_x, a_y, a_w, a_h);
    *a_h += c_key_y;
}

/**
 *  Starts a paste operation.
 */

void
seqroll::start_paste()
{
    long tick_s;
    long tick_f;
    int note_h;
    int note_l;
    snap_x(&m_current_x);
    snap_y(&m_current_x);
    m_drop_x = m_current_x;
    m_drop_y = m_current_y;
    m_paste = true;

    /* get the box that selected elements are in */

    m_seq->get_clipboard_box(&tick_s, &note_h, &tick_f, &note_l);
    convert_tn_box_to_rect
    (
        tick_s, tick_f, note_h, note_l,
        &m_selected.x, &m_selected.y, &m_selected.width, &m_selected.height
    );

    /* adjust for clipboard being shifted to tick 0 */

    m_selected.x += m_drop_x;
    m_selected.y += (m_drop_y - m_selected.y);
}

/**
 *  Performs a 'snap' operation on the y coordinate.
 */

void
seqroll::snap_y (int * a_y)
{
    *a_y = *a_y - (*a_y % c_key_y);
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
seqroll::snap_x (int * a_x)
{
    int mod = (m_snap / m_zoom);
    if (mod <= 0)
        mod = 1;

    *a_x = *a_x - (*a_x % mod);
}

/**
 *  Sets the status to the given parameter, and the CC value to the given
 *  optional control parameter, which defaults to 0.  Unlike the same
 *  function in seqevent, this version does not redraw.
 */

void
seqroll::set_data_type (unsigned char a_status, unsigned char a_control = 0)
{
    m_status = a_status;
    m_cc = a_control;
}

/**
 *  Implements the on-realize event handling.
 */

void
seqroll::on_realize ()
{
    Gtk::DrawingArea::on_realize();
    set_flags(Gtk::CAN_FOCUS);
    m_window = get_window();
    m_gc = Gdk::GC::create(m_window);
    m_window->clear();
    m_hadjust->signal_value_changed().connect
    (
        mem_fun(*this, &seqroll::change_horz)
    );
    m_vadjust->signal_value_changed().connect
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
    m_window->draw_drawable
    (
        m_gc, m_pixmap, e->area.x, e->area.y, e->area.x, e->area.y,
        e->area.width, e->area.height
    );
    draw_selection_on_window();
    return true;
}

/**
 *  Implements the on-button-press event handling.
 */

bool
seqroll::on_button_press_event (GdkEventButton * a_ev)
{
    bool result;
    switch (global_interactionmethod)
    {
    case e_fruity_interaction:
        result = m_fruity_interaction.on_button_press_event(a_ev, *this);
        break;

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
 *  Implements the on-button-release event handling.
 */

bool
seqroll::on_button_release_event (GdkEventButton * a_ev)
{
    bool result;
    switch (global_interactionmethod)
    {
    case e_fruity_interaction:
        result = m_fruity_interaction.on_button_release_event(a_ev, *this);
        break;

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
 *  Implements the on-motion-notify event handling.
 */

bool
seqroll::on_motion_notify_event (GdkEventMotion * a_ev)
{
    bool result;
    switch (global_interactionmethod)
    {
    case e_fruity_interaction:
        result = m_fruity_interaction.on_motion_notify_event(a_ev, *this);
        break;

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
 *  Implements the on-enter-notify event handling.
 */

bool
seqroll::on_enter_notify_event (GdkEventCrossing * a_p0)
{
    m_seqkeys_wid->set_hint_state(true);
    return false;
}

/**
 *  Implements the on-leave-notify event handling.
 */

bool
seqroll::on_leave_notify_event (GdkEventCrossing * a_p0)
{
    m_seqkeys_wid->set_hint_state(false);
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
 */

bool
seqroll::on_key_press_event (GdkEventKey * a_p0)
{
    bool result = false;
    bool dont_toggle = PERFKEY(start) != PERFKEY(stop);
    if
    (
        a_p0->keyval ==  PERFKEY(start) &&
        (dont_toggle || ! global_is_pattern_playing)
    )
    {
        m_mainperf->position_jack(false);
        m_mainperf->start(false);
        m_mainperf->start_jack();
        global_is_pattern_playing = true;
    }
    else if
    (
        a_p0->keyval ==  PERFKEY(stop) &&
        (dont_toggle || global_is_pattern_playing)
    )
    {
        m_mainperf->stop_jack();
        m_mainperf->stop();
        global_is_pattern_playing = false;
    }
    if (a_p0->type == GDK_KEY_PRESS)
    {
        if (a_p0->keyval ==  GDK_Delete || a_p0->keyval == GDK_BackSpace)
        {
            m_seq->push_undo();
            m_seq->mark_selected();
            m_seq->remove_marked();
            result = true;
        }
        if (! global_is_pattern_playing)
        {
            if (a_p0->keyval == GDK_Home)
            {
                m_seq->set_orig_tick(0);
                result = true;
            }
            if (a_p0->keyval == GDK_Left)
            {
                m_seq->set_orig_tick(m_seq->get_last_tick() - m_snap);
                result = true;
            }
            if (a_p0->keyval == GDK_Right)
            {
                m_seq->set_orig_tick(m_seq->get_last_tick() + m_snap);
                result = true;
            }
        }
        if (a_p0->state & GDK_CONTROL_MASK)
        {
            if (a_p0->keyval == GDK_x || a_p0->keyval == GDK_X)     /* cut */
            {
                m_seq->push_undo();
                m_seq->copy_selected();
                m_seq->mark_selected();
                m_seq->remove_marked();
                result = true;
            }
            if (a_p0->keyval == GDK_c || a_p0->keyval == GDK_C)     /* copy */
            {
                m_seq->copy_selected();
                result = true;
            }
            if (a_p0->keyval == GDK_v || a_p0->keyval == GDK_V)     /* paste */
            {
                start_paste();
                result = true;
            }
            if (a_p0->keyval == GDK_z || a_p0->keyval == GDK_Z)     /* Undo */
            {
                m_seq->pop_undo();
                result = true;
            }
            if (a_p0->keyval == GDK_a || a_p0->keyval == GDK_A) /* select all */
            {
                m_seq->select_all();
                result = true;
            }
        }
    }
    if (result)
        m_seq->set_dirty();             // redraw_events();

    return result;
}

/**
 *  Implements the on-size-allocate event handling.
 */

void
seqroll::on_size_allocate (Gtk::Allocation & a_r)
{
    Gtk::DrawingArea::on_size_allocate(a_r);
    m_window_x = a_r.get_width();
    m_window_y = a_r.get_height();
    update_sizes();
}

/**
 *  Implements the on-scroll event handling.  This scroll event only
 *  handles basic scrolling without any modifier keys such as
 *  GDK_CONTROL_MASK or GDK_SHIFT_MASK.
 */

bool
seqroll::on_scroll_event (GdkEventScroll * a_ev)
{
    guint modifiers;                /* used to filter out caps/num lock etc. */
    modifiers = gtk_accelerator_get_default_mod_mask();
    if ((a_ev->state & modifiers) != 0)
        return false;

    double val = m_vadjust->get_value();
    if (a_ev->direction == GDK_SCROLL_UP)
        val -= m_vadjust->get_step_increment() / 6;
    else if (a_ev->direction == GDK_SCROLL_DOWN)
        val += m_vadjust->get_step_increment() / 6;
    else
        return true;

    m_vadjust->clamp_page(val, val + m_vadjust->get_page_size());
    return true;
}

}           // namespace seq64

/*
 * seqroll.cpp
 *
 * vim: sw=4 ts=4 wm=8 et ft=cpp
 */
