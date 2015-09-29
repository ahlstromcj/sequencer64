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
 * \file          perfnames.cpp
 *
 *  This module declares/defines the base class for performance names.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2015-09-29
 * \license       GNU GPLv2 or above
 *
 */

#include <gtkmm/adjustment.h>

#include "font.hpp"
#include "perform.hpp"
#include "perfnames.hpp"

/**
 *  Adjustments to the performance window.  Sequences that don't have
 *  events show up as black-on-yellow.  This feature is enabled by
 *  default.  To disable this feature, configure the build with the
 *  --disable-highlight option.
 *
 *      #define SEQ64_HIGHLIGHT_EMPTY_SEQS  // undefine for normal empty seqs
 */

namespace seq64
{

/**
 *  Principal constructor for this user-interface object.
 */

perfnames::perfnames (perform * a_perf, Gtk::Adjustment * a_vadjust)
 :
    Gtk::DrawingArea    (),
    seqmenu             (*a_perf),
    m_gc                (),
    m_window            (),
    m_black             (Gdk::Color("black")),
    m_white             (Gdk::Color("white")),
    m_grey              (Gdk::Color("grey")),
    m_yellow            (Gdk::Color("yellow")),
    m_pixmap            (),
    m_mainperf          (a_perf),
    m_window_x          (),
    m_window_y          (),
    m_vadjust           (a_vadjust),
    m_sequence_offset   (0),
    m_sequence_active   ()              // an array
{
    add_events
    (
        Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK | Gdk::SCROLL_MASK
    );
    set_size_request(c_names_x, 100);                   /* set default size */

    /*
     *  In the constructor you can only allocate colors;
     *  get_window() returns 0 because this window has not be realized.
     */

    Glib::RefPtr<Gdk::Colormap> colormap = get_default_colormap();
    colormap->alloc_color(m_black);
    colormap->alloc_color(m_white);
    colormap->alloc_color(m_grey);
    colormap->alloc_color(m_yellow);
    m_vadjust->signal_value_changed().connect
    (
        mem_fun(*(this), &perfnames::change_vert)
    );
    set_double_buffered(false);
    for (int i = 0; i < c_max_sequence; ++i)
        m_sequence_active[i] = false;
}

/**
 *  Change the vertial offset of a sequence/pattern.
 */

void
perfnames::change_vert ()
{
    if (m_sequence_offset != int(m_vadjust->get_value()))
    {
        m_sequence_offset = int(m_vadjust->get_value());
        queue_draw();
    }
}

/**
 *  This function does nothing.
 */

void
perfnames::update_pixmap ()
{
    // Empty body
}

/**
 *  This function does nothing.
 */

void
perfnames::draw_area ()
{
    // Empty body
}

/**
 *  Redraw the given sequence.
 */

void
perfnames::redraw (int sequence)
{
    draw_sequence(sequence);
}

/**
 *  Draw the given sequence.
 */

void
perfnames::draw_sequence (int seqnum)
{
    int i = seqnum - m_sequence_offset;
    if (seqnum < c_max_sequence)
    {
        sequence * seq = m_mainperf->get_sequence(seqnum);

#if SEQ64_HIGHLIGHT_EMPTY_SEQS

        /*
         * Setting seqempty to seq->event_count() == 0 here causes a
         * seqfault in pthread code!  Setting it further on does work.
         */

        bool seqempty = false;
#endif
        m_gc->set_foreground(m_black);
        m_window->draw_rectangle
        (
            m_gc, true, 0, (c_names_y * i) , c_names_x, c_names_y + 1
        );
        if (seqnum % c_seqs_in_set == 0)
        {
            char ss[3];
            snprintf(ss, sizeof(ss), "%2d", seqnum / c_seqs_in_set);
            m_gc->set_foreground(m_white);
            p_font_renderer->render_string_on_drawable
            (
                m_gc, 2, c_names_y * i + 2, m_window, ss, font::WHITE
            );
        }
        else
        {
            m_gc->set_foreground(m_white);
            m_window->draw_rectangle
            (
                m_gc, true, 1, (c_names_y * (i)), (6 * 2) + 1, c_names_y
            );
        }
        if (m_mainperf->is_active(seqnum))
        {
#if SEQ64_HIGHLIGHT_EMPTY_SEQS
            seqempty = seq->event_count() == 0;     // this works fine!
            if (seqempty)
                m_gc->set_foreground(m_yellow);
            else
#endif
                m_gc->set_foreground(m_white);
        }
        else
            m_gc->set_foreground(m_grey);

        m_window->draw_rectangle
        (
            m_gc, true, 6 * 2 + 3,
            (c_names_y * i) + 1, c_names_x - 3 - (6 * 2), c_names_y - 1
        );
        if (m_mainperf->is_active(seqnum))
        {
            char temp[50];
            m_sequence_active[seqnum] = true;
            font::Color col;
#if SEQ64_HIGHLIGHT_EMPTY_SEQS
            if (seqempty)
                col = font::BLACK_ON_YELLOW;
            else
#endif
                col = font::BLACK;

            snprintf
            (
                temp, sizeof(temp), "%-14.14s   %2d",
                seq->get_name(), seq->get_midi_channel() + 1
            );
            p_font_renderer->render_string_on_drawable
            (
                m_gc, 5 + 6 * 2, c_names_y * i + 2, m_window, temp, col
            );
            snprintf
            (
                temp, sizeof(temp), "%d-%d %ld/%ld",
                seq->get_midi_bus(), seq->get_midi_channel() + 1,
                seq->get_bpm(), seq->get_bw()
            );
            p_font_renderer->render_string_on_drawable
            (
                m_gc, 5 + 6 * 2, c_names_y * i + 12, m_window, temp, col
            );

            bool muted = seq->get_song_mute();
            m_gc->set_foreground(m_black);
            m_window->draw_rectangle
            (
                m_gc, muted, 6 * 2 + 6 * 20 + 2, (c_names_y * i), 10, c_names_y
            );
            if (muted)
            {
#if SEQ64_HIGHLIGHT_EMPTY_SEQS
                if (seqempty)
                    col = font::YELLOW_ON_BLACK;
                else
#endif
                    col = font::WHITE;

                p_font_renderer->render_string_on_drawable
                (
                    m_gc, 5 + 6 * 2 + 6 * 20, c_names_y * i + 2,
                    m_window, "M", col
                );
            }
            else
            {
                p_font_renderer->render_string_on_drawable
                (
                    m_gc, 5 + 6 * 2 + 6 * 20, c_names_y * i + 2,
                    m_window, "M", col
                );
            }
        }
    }
    else
    {
        m_gc->set_foreground(m_grey);
        m_window->draw_rectangle
        (
            m_gc, true, 0, (c_names_y * i) + 1 , c_names_x, c_names_y
        );
    }
}

/**
 *  Converts a y-value into a sequence number, returned via the second
 *  parameter.
 */

void
perfnames::convert_y (int a_y, int * a_seq)
{
    *a_seq = a_y / c_names_y;
    *a_seq += m_sequence_offset;
    if (*a_seq >= c_max_sequence)
        *a_seq = c_max_sequence - 1;

    if (*a_seq < 0)
        *a_seq = 0;
}

/**
 *  Provides the callback for a button press, and it handles only a left
 *  mouse button.
 */

bool
perfnames::on_button_press_event (GdkEventButton * a_e)
{
    int y = int(a_e->y);                            /* int x = (int) a_e->x; */
    int seqnum;
    convert_y(y, &seqnum);
    current_sequence(seqnum);
    if (a_e->button == 1)                           /* left mouse button */
    {
        if (m_mainperf->is_active(seqnum))
        {
            sequence * seq = m_mainperf->get_sequence(seqnum);
            bool muted = seq->get_song_mute();
            seq->set_song_mute(! muted);
            queue_draw();
        }
    }
    return true;
}

/**
 *  Handles the callback when the window is realized.  It first calls the
 *  base-class version of on_realize().  Then it allocates any additional
 *  resources needed.
 */

void
perfnames::on_realize ()
{
    Gtk::DrawingArea::on_realize();
    m_window = get_window();
    m_gc = Gdk::GC::create(m_window);
    m_window->clear();
    m_pixmap = Gdk::Pixmap::create
    (
        m_window, c_names_x, c_names_y * c_max_sequence + 1, -1
    );
}

/**
 *  Handles an on-expose event.  It draws all of the sequences.
 */

bool
perfnames::on_expose_event (GdkEventExpose * a_e)
{
    int seqs = (m_window_y / c_names_y) + 1;
    for (int i = 0; i < seqs; i++)
    {
        int sequence = i + m_sequence_offset;
        draw_sequence(sequence);
    }
    return true;
}

/**
 *  Handles a button-release for the right button, bringing up a popup
 *  menu.
 */

bool
perfnames::on_button_release_event (GdkEventButton * p0)
{
    if (p0->button == 3)                        /* right mouse button   */
        popup_menu();

    return false;
}

/**
 *  Handle the scrolling of the window.
 */

bool
perfnames::on_scroll_event (GdkEventScroll * a_ev)
{
    double val = m_vadjust->get_value();
    if (a_ev->direction == GDK_SCROLL_UP)
        val -= m_vadjust->get_step_increment();
    if (a_ev->direction == GDK_SCROLL_DOWN)
        val += m_vadjust->get_step_increment();

    m_vadjust->clamp_page(val, val + m_vadjust->get_page_size());
    return true;
}

/**
 *  Handles a size-allocation event.  It first calls the base-class
 *  version of this function.
 */

void
perfnames::on_size_allocate (Gtk::Allocation & a_r)
{
    Gtk::DrawingArea::on_size_allocate(a_r);
    m_window_x = a_r.get_width();
    m_window_y = a_r.get_height();
}

/**
 *  Redraws sequences that have been modified.
 */

void
perfnames::redraw_dirty_sequences ()
{
    int y_s = 0;
    int y_f = m_window_y / c_names_y;
    for (int y = y_s; y <= y_f; y++)
    {
        int seq = y + m_sequence_offset;
        if (seq < c_max_sequence)
        {
            bool dirty = (m_mainperf->is_dirty_names(seq));
            if (dirty)
                draw_sequence(seq);
        }
    }
}

}           // namespace seq64

/*
 * perfnames.cpp
 *
 * vim: sw=4 ts=4 wm=8 et ft=cpp
 */
