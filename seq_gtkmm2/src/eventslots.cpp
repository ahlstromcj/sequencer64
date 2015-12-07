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
 * \file          eventslots.cpp
 *
 *  This module declares/defines the base class for displaying events in their
 *  editing slotss.
 *
 * \library       sequencer64 application
 * \author        Chris Ahlstrom
 * \date          2015-12-05
 * \updates       2015-12-06
 * \license       GNU GPLv2 or above
 *
 *  This module is user-interface code.  It is loosely based on the workings
 *  of the perfnames class.
 */

#include <gtkmm/adjustment.h>

#include "click.hpp"                    /* SEQ64_CLICK_LEFT(), etc.    */
#include "font.hpp"
#include "eventedit.hpp"
#include "perform.hpp"
#include "eventslots.hpp"

namespace seq64
{

/**
 *  Principal constructor for this user-interface object.
 */

eventslots::eventslots
(
    perform & p,
    eventedit & parent,         // truly necessary???
    sequence & seq,
    Gtk::Adjustment & vadjust
) :
    gui_drawingarea_gtk2    (p, adjustment_dummy(), vadjust, c_names_x, 100),
//  seqmenu                 (p),
    m_parent                (parent),
    m_seq                   (seq),
    m_event_container       (seq, p.get_beats_per_minute()),
    m_slots_chars           (64),                           // 24
    m_char_w                (font_render().char_width()),
    m_setbox_w              (m_char_w * 2),
    m_slots_box_w           (m_char_w * 62),                // 22
    m_slots_x               (m_slots_chars * m_char_w),
    m_slots_y               (c_names_y),
    m_xy_offset             (2),
    m_event_count           (0),
    m_event_offset          (0)
{
    m_vadjust.signal_value_changed().connect
    (
        mem_fun(*(this), &eventslots::change_vert)
    );
    if (m_event_container.load_events())
        m_event_count = m_event_container.count();
}

/**
 *  Change the vertial offset of a sequence/pattern.
 */

void
eventslots::change_vert ()
{
    if (m_event_offset != int(m_vadjust.get_value()))
    {
        m_event_offset = int(m_vadjust.get_value());
        enqueue_draw();
    }
}

/**
 *  Wraps queue_draw().
 */

void
eventslots::enqueue_draw ()
{
    queue_draw();
}

/**
 *  Draw the given slot/event.  The slot contains the event details in
 *  (so far) one line of text in the box:
 *
 *  | timestamp | event kind | channel
 *      | data 0 name + value
 *      | data 1 name + value
 *
 *  Currently, this view shows only events that get copied to the sequence's
 *  event list.  This rules out the following items from the view:
 *
 *      -   MThd (song header)
 *      -   MTrk and Meta TrkEnd (track marker, a sequence has only one track)
 *      -   SeqNr (sequence number)
 *      -   SeqSpec (but there are three that might appear, see below)
 *      -   Meta TrkName
 *
 *  The events that are shown in this view are:
 *
 *      -   One-data-value events:
 *          -   Program Change
 *          -   Channel Pressure
 *      -   Two-data-value events:
 *          -   Note Off
 *          -   Note On
 *          -   Aftertouch
 *          -   Control Change
 *          -   Pitch Wheel
 *      -   Other:
 *          -   SysEx events, with partial show of data bytes
 *          -   SeqSpec events (TBD):
 *              -   Key
 *              -   Scale
 *              -   Background sequence
 *
 *  The index of the event is shown in the editor portion of the eventedit
 *  dialog.
 */

void
eventslots::draw_event (int eventindex)
{
    int yloc = m_slots_y * (eventindex - m_event_offset);
    if (eventindex < m_event_count)
    {

#ifdef DRAW_EVENT_INDEX

        /*
         * 1. Render the event index in the leftmost column.
         */

        char snb[8];
        snprintf(snb, sizeof(snb), "%2d", eventindex / m_seqs_in_set);
        draw_rectangle(black(), 0, yloc, m_slots_x, m_slots_y);     /* + 1  */
        render_string(m_xy_offset, yloc + m_xy_offset, snb, font::WHITE);
//          draw_rectangle(white(), 1, yloc, m_setbox_w + 1, m_slots_y);

#endif  // DRAW_EVENT_INDEX

        Color fg = grey();
        font::Color col = font::BLACK;

        /*
         * Make sure that the rectangle drawn with the proper background
         * colors, otherwise just the name is properly colored.
         */

        if (true)           // if (eventindex == currentindex)
        {
            fg = yellow();
            col = font::BLACK_ON_YELLOW;
        }
        else if (false)     // if (a sysex event or selected event range)
        {
            fg = dark_cyan();
            col = font::BLACK_ON_CYAN;
        }
        else
            fg = white();

        /*
         * 2. Render the column with the name of the sequence.  The channel
         *    number ranges from 1 to 16, but SMF 0 is indicated on-screen by
         *    a channel number of 0.
         */

        draw_rectangle
        (
            fg, m_setbox_w + 3, yloc + 1,
            m_slots_x - 3 - m_setbox_w, m_slots_y - 1
        );
#ifdef THIS_IS_READY
        editable_event & evp = m_event_container.begin().second;
        std::string temp = evp.stock_event_string();
#endif
        std::string temp = "12345 Note On Channel 10 Key 127 Velocity 127";
        render_string(5 + m_setbox_w, yloc + 2, temp, col);
        render_string(m_slots_box_w + 5, yloc + 2, "X", col);
    }
}

/**
 *  Converts a y-value into a sequence number and returns it.
 */

int
eventslots::convert_y (int y)
{
    int edev = y / m_slots_y + m_event_offset;
    if (edev >= m_event_count)
        edev = m_event_count - 1;
    else if (edev < 0)
        edev = 0;

    return edev;
}

/**
 *  Provides the callback for a button press, and it handles only a left
 *  mouse button.
 */

bool
eventslots::on_button_press_event (GdkEventButton * ev)
{
    int y = int(ev->y);
    // int eventnum = convert_y(y);
//  current_event(seqnum);
    if (SEQ64_CLICK_LEFT(ev->button))
    {
        enqueue_draw();
    }
    return true;
}

/**
 *  Handles the callback when the window is realized.  It first calls the
 *  base-class version of on_realize().  Then it allocates any additional
 *  resources needed.
 */

void
eventslots::on_realize ()
{
    gui_drawingarea_gtk2::on_realize();
    m_pixmap = Gdk::Pixmap::create
    (
        m_window, m_slots_x, m_slots_y * m_event_count + 1, -1
    );
}

/**
 *  Handles an on-expose event.  It draws all of the sequences.
 */

bool
eventslots::on_expose_event (GdkEventExpose *)
{
    int seqs = (m_window_y / m_slots_y) + 1;
    for (int i = 0; i < seqs; i++)
    {
        int sequence = i + m_event_offset;
        draw_event(sequence);
    }
    return true;
}

/**
 *  Handles a button-release for the right button, bringing up a popup
 *  menu.
 */

bool
eventslots::on_button_release_event (GdkEventButton * p0)
{
//  if (SEQ64_CLICK_RIGHT(p0->button))
//      popup_menu();

    return false;
}

/**
 *  Handle the scrolling of the window.
 */

bool
eventslots::on_scroll_event (GdkEventScroll * ev)
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
eventslots::on_size_allocate (Gtk::Allocation & a)
{
    gui_drawingarea_gtk2::on_size_allocate(a);
    m_window_x = a.get_width();                     /* side-effect  */
    m_window_y = a.get_height();                    /* side-effect  */
}

/**
 *  Redraws sequences that have been modified.
 */

void
eventslots::redraw_dirty_events ()
{
    int y_f = m_window_y / m_slots_y;
    for (int y = 0; y <= y_f; y++)
    {
        int seq = y + m_event_offset;
        if (seq < m_event_count)
        {
            bool dirty = (perf().is_dirty_names(seq));
            if (dirty)
                draw_event(seq);
        }
    }
}

}           // namespace seq64

/*
 * eventslots.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

