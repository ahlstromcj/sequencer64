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
 * \updates       2015-10-09
 * \license       GNU GPLv2 or above
 *
 *  This module is almost exclusively user-interface code.  There are some
 *  pointers yet that could be replaced by references, and a number of minor
 *  issues that could be fixed.
 */

#include <gtkmm/adjustment.h>

#include "click.hpp"                    /* SEQ64_CLICK_IS_LEFT(), etc.    */
#include "font.hpp"
#include "perform.hpp"
#include "perfnames.hpp"

/**
 *  Adjustments to the performance window.  Sequences that don't have
 *  events show up as black-on-yellow.  This feature is enabled by
 *  default.  To disable this feature, configure the build with the
 *  "--disable-highlight" option.
 *
 *  #define SEQ64_HIGHLIGHT_EMPTY_SEQS  // undefine for normal empty seqs
 */

namespace seq64
{

/**
 *  Principal constructor for this user-interface object.
 *
 *  Weird is that the window (x,y) are set to (c_names_x, 100), when c_names_y
 *  is 22 in globals.h.
 */

perfnames::perfnames (perform & p, Gtk::Adjustment & vadjust)
 :
    gui_drawingarea_gtk2    (p, adjustment_dummy(), vadjust, c_names_x, 100),
    seqmenu                 (p),
    m_names_x               (c_names_x),
    m_names_y               (c_names_y),
    m_seqs_in_set           (c_seqs_in_set),
    m_sequence_max          (c_max_sequence),
    m_sequence_offset       (0),
    m_sequence_active       ()              // an array
{
    m_vadjust.signal_value_changed().connect
    (
        mem_fun(*(this), &perfnames::change_vert)
    );

    /*
     * \todo Change this to a dynamic container.
     */

    for (int i = 0; i < m_sequence_max; ++i)
        m_sequence_active[i] = false;
}

/**
 *  Change the vertial offset of a sequence/pattern.
 */

void
perfnames::change_vert ()
{
    if (m_sequence_offset != int(m_vadjust.get_value()))
    {
        m_sequence_offset = int(m_vadjust.get_value());
        queue_draw();
    }
}

/**
 *  Draw the given sequence.
 */

void
perfnames::draw_sequence (int seqnum)
{
    int i = seqnum - m_sequence_offset;
    if (seqnum < m_sequence_max)
    {
        sequence * seq = perf().get_sequence(seqnum);

#if SEQ64_HIGHLIGHT_EMPTY_SEQS

        /*
         * Setting seqempty to seq->event_count() == 0 here causes a
         * seqfault in pthread code!  Setting it further on does work.
         */

        bool seqempty = false;
#endif
        m_gc->set_foreground(black());
        m_window->draw_rectangle
        (
            m_gc, true, 0, (m_names_y * i) , m_names_x, m_names_y + 1
        );
        if (seqnum % m_seqs_in_set == 0)
        {
            char ss[4];
            snprintf(ss, sizeof(ss), "%2d", seqnum / m_seqs_in_set);
            m_gc->set_foreground(white());
            p_font_renderer->render_string_on_drawable
            (
                m_gc, 2, m_names_y * i + 2, m_window, ss, font::WHITE
            );
        }
        else
        {
            m_gc->set_foreground(white());
            m_window->draw_rectangle
            (
                m_gc, true, 1, (m_names_y * (i)), (6 * 2) + 1, m_names_y
            );
        }
        if (perf().is_active(seqnum))
        {
#if SEQ64_HIGHLIGHT_EMPTY_SEQS
            seqempty = seq->event_count() == 0;     // this works fine!
            if (seqempty)
                m_gc->set_foreground(yellow());
            else
#endif
                m_gc->set_foreground(white());
        }
        else
            m_gc->set_foreground(grey());

        m_window->draw_rectangle
        (
            m_gc, true, 6 * 2 + 3,
            (m_names_y * i) + 1, m_names_x - 3 - (6 * 2), m_names_y - 1
        );
        if (perf().is_active(seqnum))
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
                m_gc, 5 + 6 * 2, m_names_y * i + 2, m_window, temp, col
            );
            snprintf
            (
                temp, sizeof(temp), "%d-%d %ld/%ld",
                seq->get_midi_bus(), seq->get_midi_channel() + 1,
                seq->get_bpm(), seq->get_bw()
            );
            p_font_renderer->render_string_on_drawable
            (
                m_gc, 5 + 6 * 2, m_names_y * i + 12, m_window, temp, col
            );

            bool muted = seq->get_song_mute();
            m_gc->set_foreground(black());
            m_window->draw_rectangle
            (
                m_gc, muted, 6 * 2 + 6 * 20 + 2, (m_names_y * i), 10, m_names_y
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
                    m_gc, 5 + 6 * 2 + 6 * 20, m_names_y * i + 2,
                    m_window, "M", col
                );
            }
            else
            {
                p_font_renderer->render_string_on_drawable
                (
                    m_gc, 5 + 6 * 2 + 6 * 20, m_names_y * i + 2,
                    m_window, "M", col
                );
            }
        }
    }
    else
    {
        m_gc->set_foreground(grey());
        m_window->draw_rectangle
        (
            m_gc, true, 0, (m_names_y * i) + 1 , m_names_x, m_names_y
        );
    }
}

/**
 *  Converts a y-value into a sequence number and returns it.
 */

int
perfnames::convert_y (int y)
{
    int seq = y / m_names_y + m_sequence_offset;
    if (seq >= m_sequence_max)
        seq = m_sequence_max - 1;
    else if (seq < 0)
        seq = 0;

    return seq;
}

/**
 *  Provides the callback for a button press, and it handles only a left
 *  mouse button.
 */

bool
perfnames::on_button_press_event (GdkEventButton * a_e)
{
    int y = int(a_e->y);
    int seqnum = convert_y(y);
    current_sequence(seqnum);
    if (SEQ64_CLICK_IS_LEFT(a_e->button))
    {
        if (perf().is_active(seqnum))
        {
            // TODO:  use reference here

            sequence * seq = perf().get_sequence(seqnum);
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
    gui_drawingarea_gtk2::on_realize();
    m_pixmap = Gdk::Pixmap::create
    (
        m_window, m_names_x, m_names_y * m_sequence_max + 1, -1
    );
}

/**
 *  Handles an on-expose event.  It draws all of the sequences.
 */

bool
perfnames::on_expose_event (GdkEventExpose * a_e)
{
    int seqs = (m_window_y / m_names_y) + 1;
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
    if (SEQ64_CLICK_IS_RIGHT(p0->button))
        popup_menu();

    return false;
}

/**
 *  Handle the scrolling of the window.
 */

bool
perfnames::on_scroll_event (GdkEventScroll * ev)
{
    double val = m_vadjust.get_value();
    if (ev->direction == GDK_SCROLL_UP)
        val -= m_vadjust.get_step_increment();
    else if (ev->direction == GDK_SCROLL_DOWN)
        val += m_vadjust.get_step_increment();

    m_vadjust.clamp_page(val, val + m_vadjust.get_page_size());
    return true;
}

/**
 *  Handles a size-allocation event.  It first calls the base-class
 *  version of this function.
 */

void
perfnames::on_size_allocate (Gtk::Allocation & a)
{
    gui_drawingarea_gtk2::on_size_allocate(a);
    m_window_x = a.get_width();                     /* side-effect  */
    m_window_y = a.get_height();                    /* side-effect  */
}

/**
 *  Redraws sequences that have been modified.
 */

void
perfnames::redraw_dirty_sequences ()
{
    int y_f = m_window_y / m_names_y;
    for (int y = 0; y <= y_f; y++)
    {
        int seq = y + m_sequence_offset;
        if (seq < m_sequence_max)
        {
            bool dirty = (perf().is_dirty_names(seq));
            if (dirty)
                draw_sequence(seq);
        }
    }
}

}           // namespace seq64

/*
 * perfnames.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
