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
 * \updates       2015-11-25
 * \license       GNU GPLv2 or above
 *
 *  This module is almost exclusively user-interface code.  There are some
 *  pointers yet that could be replaced by references, and a number of minor
 *  issues that could be fixed.
 *
 *  Adjustments to the performance window can be made with the highlighting
 *  option.  Sequences that don't have events show up as black-on-yellow.
 *  This feature is enabled by default.  To disable this feature, configure
 *  the build with the "--disable-highlight" option.
 *
 * \todo
 *      When bringing up this dialog, and starting play from it, some
 *      extra horizontal lines are drawn for some of the sequences.  This
 *      happens even in seq24, so this is long standing behavior.  Is it
 *      useful, and how?  Where is it done?  In perfroll?
 */

#include <gtkmm/adjustment.h>

#include "click.hpp"                    /* SEQ64_CLICK_LEFT(), etc.    */
#include "font.hpp"
#include "perform.hpp"
#include "perfnames.hpp"

namespace seq64
{

/**
 *  Principal constructor for this user-interface object.
 *
 *  Weird is that the window (x,y) are set to (c_names_x, 100), when c_names_y
 *  is 22 (now 24) in globals.h.
 */

perfnames::perfnames (perform & p, Gtk::Adjustment & vadjust)
 :
    gui_drawingarea_gtk2    (p, adjustment_dummy(), vadjust, c_names_x, 100),
    seqmenu                 (p),
    m_names_chars           (24),
    m_char_w                (font_render().char_width()),   /* 6            */
    m_setbox_w              (m_char_w * 2),
    m_namebox_w             (m_char_w * 22),                /* 24?          */
    m_names_x               (m_names_chars * m_char_w),     /* c_names_x    */
    m_names_y               (c_names_y),
    m_xy_offset             (2),
    m_seqs_in_set           (c_seqs_in_set),
    m_sequence_max          (c_max_sequence),
    m_sequence_offset       (0),
    m_sequence_active       ()                              /* an array     */
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
 *  Draw the given sequence.  This function has to be prepared to handle an
 *  almost endless list of sequences, including unused ones, to draw them all
 *  with compatible styles.  The sequences are grouped by set-number.  The
 *  set-number occurs every 32 sequences in the leftmost column of the window.
 */

void
perfnames::draw_sequence (int seqnum)
{
    int yloc = m_names_y * (seqnum - m_sequence_offset);
    if (seqnum < m_sequence_max)                    /* less than "infinity" */
    {
        /*
         * 1. Render the set number, or a blank box, in leftmost column. If
         *    the y height of the first draw_rectangle is m_names_y + 1, then
         *    we get a black line for the blank tracks, looks ugly.
         */

        char snb[8];                                /* set-number buffer    */
        snprintf(snb, sizeof(snb), "%2d", seqnum / m_seqs_in_set);
        draw_rectangle(black(), 0, yloc, m_names_x, m_names_y);     /* + 1  */
        if (seqnum % m_seqs_in_set == 0)
            render_string(m_xy_offset, yloc + m_xy_offset, snb, font::WHITE);
        else
            draw_rectangle(white(), 1, yloc, m_setbox_w + 1, m_names_y);

        sequence * seq = perf().get_sequence(seqnum);
        Color fg = grey();
        font::Color col = font::BLACK;
        if (perf().is_active(seqnum))
        {
            /*
             * Make sure that the rectangle drawn with the proper background
             * colors, otherwise just the name is properly colored.  Weird,
             * the highlight() call crashes if put just after the call to
             * get_sequence() above!
             */

            if (perf().highlight(*seq))
            {
                fg = yellow();
                col = font::BLACK_ON_YELLOW;
            }
            else if (perf().is_smf_0(*seq))
            {
                fg = dark_cyan();
                col = font::BLACK_ON_CYAN;
            }
            else
                fg = white();
        }

        /*
         * 2. Render the column with the name of the sequence.  The channel
         *    number ranges from 1 to 16, but SMF 0 is indicated on-screen by
         *    a channel number of 0.
         */

        draw_rectangle
        (
            fg, m_setbox_w + 3, yloc + 1,
            m_names_x - 3 - m_setbox_w, m_names_y - 1
        );
        if (perf().is_active(seqnum))
        {
            char temp[32];
            int chan = seq->is_smf_0() ? 0 : seq->get_midi_channel() + 1 ;
            m_sequence_active[seqnum] = true;
            snprintf(temp, sizeof(temp), "%-14.14s   %2d", seq->get_name(), chan);
            render_string(5 + m_setbox_w, yloc + 2, temp, col);

            /*
             * Get the label format from the perform object, for consistency.
             */

            std::string label = perf().sequence_label(*seq);
            render_string(m_setbox_w + 5, yloc + 12, label, col);

            bool muted = seq->get_song_mute();
            draw_rectangle(black(), m_namebox_w + 2, yloc, 10, m_names_y, muted);
            if (muted)
            {
                if (perf().highlight(*seq))
                    col = font::YELLOW_ON_BLACK;
                else if (perf().is_smf_0(*seq))
                    col = font::CYAN_ON_BLACK;
                else
                    col = font::WHITE;
            }
            render_string(m_namebox_w + 5, yloc + 2, "M", col);
        }
    }
    else
    {
        /*
         * This "else" shouldn't legally be possible!
         */

        draw_rectangle(grey(), 0, yloc + 1, m_names_x, m_names_y);
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
perfnames::on_button_press_event (GdkEventButton * ev)
{
    int y = int(ev->y);
    int seqnum = convert_y(y);
    current_sequence(seqnum);
    if (SEQ64_CLICK_LEFT(ev->button))
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
perfnames::on_expose_event (GdkEventExpose *)
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
    if (SEQ64_CLICK_RIGHT(p0->button))
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
    if (CAST_EQUIVALENT(ev->direction, SEQ64_SCROLL_UP))
        val -= m_vadjust.get_step_increment();
    else if (CAST_EQUIVALENT(ev->direction, SEQ64_SCROLL_DOWN))
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

