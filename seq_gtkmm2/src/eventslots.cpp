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
 * \updates       2015-12-10
 * \license       GNU GPLv2 or above
 *
 *  This module is user-interface code.  It is loosely based on the workings
 *  of the perfnames class.
 *
 *  Now, we have an issue when loading one of the larger sequences in our main
 *  test tune, where X stops the application and Gtk says it got a bad memory
 *  allocation.  So we need to page through the sequence.
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
    eventedit & parent,
    sequence & seq,
    Gtk::Adjustment & vadjust
) :
    gui_drawingarea_gtk2    (p, adjustment_dummy(), vadjust, 360, 10),
    m_parent                (parent),
    m_seq                   (seq),
    m_event_container       (seq, p.get_beats_per_minute()),
    m_slots_chars           (64),                               // 24
    m_char_w                (font_render().char_width()),
    m_setbox_w              (m_char_w * 2),
    m_slots_box_w           (m_char_w * 62),                    // 22
    m_slots_x               (m_slots_chars * m_char_w),
    m_slots_y               (font_render().char_height() + 4),  // c_names_y
    m_xy_offset             (2),
    m_event_count           (0),
    m_display_count         (0),
    m_top_event_index       (0),
    m_bottom_event_index    (42),   // depends on dialog height
    m_current_event_index   (0),
    m_top_iterator          (),
    m_bottom_iterator       (),
    m_current_iterator      ()
{
    m_vadjust.signal_value_changed().connect
    (
        mem_fun(*(this), &eventslots::change_vert)
    );
    load_events();
}

/**
 *  Grabs the event list from the sequence and uses it to fill the
 *  editable-event list.  Determines how many events can be shown in the
 *  GUI [later] and adjusts the top and bottom editable-event iterators to
 *  shows the first page of events.
 *
 * \return
 *      Returns true if the event iterators were able to be set up as valid.
 */

bool
eventslots::load_events ()
{
    bool result = m_event_container.load_events();
    if (result)
    {
        m_event_count = m_event_container.count();
        if (m_event_count > 0)
        {
            int count = m_bottom_event_index + 1;
            if (m_event_count < count)
                count = m_event_count;

            m_display_count = count;

            editable_events::iterator ei;
            ei = 
                m_current_iterator =
                m_bottom_iterator =
                m_top_iterator =
                m_event_container.begin();

            for ( ; count > 0; --count)
            {
                ei++;
                if (ei != m_event_container.end())
                    m_bottom_iterator = ei;
                else
                    break;
            }
        }
        else
        {
            result = false;
            m_current_iterator =
                m_bottom_iterator =
                m_top_iterator =
                m_event_container.end();
        }
    }
    return result;
}

/**
 *  Set the current event.
 */

void
eventslots::set_current_event (const editable_events::iterator ei, int index)
{
    char tmp[16];
    midibyte d0, d1;
    const editable_event & ev = ei->second;
    ev.get_data(d0, d1);
    snprintf(tmp, sizeof tmp, "data[0] = 0x%02x", int(d0));
    std::string data_0(tmp);
    snprintf(tmp, sizeof tmp, "data[1] = 0x%02x", int(d1));
    std::string data_1(tmp);
    set_text(ev.category_string(), ev.status_string(), data_0, data_1);
    m_current_event_index = index;
    m_current_iterator = ei;
}

/**
 *  Sets the text in the parent dialog, eventedit.
 *
 * \todo
 *      Actually, the sequence items are available in the parent, not here
 *      from the event.
 */

void
eventslots::set_text
(
    const std::string & evcategory,
    const std::string & evname,
    const std::string & evdata0,
    const std::string & evdata1
)
{
    m_parent.set_event_category(evcategory);
    m_parent.set_event_name(evname);
    m_parent.set_event_data_0(evdata0);
    m_parent.set_event_data_1(evdata1);
}

/**
 *  Change the vertial offset of a sequence/pattern.
 */

void
eventslots::change_vert ()
{
    if (m_top_event_index != int(m_vadjust.get_value()))
    {
        m_top_event_index = int(m_vadjust.get_value());
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
eventslots::draw_event (editable_events::iterator ei, int index)
{
    if (index < m_display_count)
    {
        int yloc = m_slots_y * (index - m_top_event_index);
        Color fg = grey();
        font::Color col = font::BLACK;

        /*
         * Make sure that the rectangle drawn with the proper background
         * colors, otherwise just the name is properly colored.
         */

        if (index == m_current_event_index)
        {
            fg = yellow();
            col = font::BLACK_ON_YELLOW;
        }
#ifdef USE_FUTURE_CODE
        else if (false)     // if (a sysex event or selected event range)
        {
            fg = dark_cyan();
            col = font::BLACK_ON_CYAN;
        }
#endif
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
        editable_event & evp = ei->second;
        char tmp[4];
        snprintf(tmp, sizeof tmp, "%2d", index);
        std::string temp = tmp;
        temp += "-";
        temp += evp.stock_event_string();
        render_string(5 + m_setbox_w, yloc + 2, temp, col);
        render_string(m_slots_box_w + 5, yloc + 2, "X", col);
    }
}

/**
 *  Converts a y-value into an event index relative to 0 (the top of the
 *  eventslots window/pixmap) and returns it.
 *
 * \param y
 *      The y coordinate of the position of the mouse click in the eventslot
 *      window/pixmap.
 *
 * \return
 *      Returns the index of the event position in the user-interface, which
 *      should range from 0 to m_bottom_event_index.
 */

int
eventslots::convert_y (int y)
{
    int event_index = y / m_slots_y + m_top_event_index;
    if (event_index >= m_bottom_event_index)
        event_index = m_bottom_event_index;
    else if (event_index < 0)
        event_index = 0;

    return event_index;
}

/**
 *  Provides the callback for a button press, and it handles only a left
 *  mouse button.
 */

bool
eventslots::on_button_press_event (GdkEventButton * ev)
{
    int y = int(ev->y);
    int event_index = convert_y(y);
    if (SEQ64_CLICK_LEFT(ev->button))
    {
        bool ok = true;
        int i = m_top_event_index;
        editable_events::iterator ei = m_top_iterator;
        while (i++ < event_index)
        {
            if (ei != m_event_container.end())
            {
                ++ei;
                ok = ei != m_event_container.end();
                if (! ok)
                    break;
            }
        }
        if (ok)
        {
            set_current_event(ei, i - 1);
            enqueue_draw();
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
eventslots::on_realize ()
{
    gui_drawingarea_gtk2::on_realize();
    m_pixmap = Gdk::Pixmap::create
    (
        m_window, m_slots_x, m_slots_y * m_display_count + 1, -1
    );

    /*
     * Hmmmm, leaves the event name field empty, not sure why!
     */

    set_current_event(m_top_iterator, 0);
    grab_focus();
}

/**
 *  Handles an on-expose event.  It draws all of the sequences.
 */

bool
eventslots::on_expose_event (GdkEventExpose *)
{
    /*
     * Need to change this to calculate the number of displayable events.
     *
     * int seqs = (m_window_y / m_slots_y) + 1;
     */

    if (m_display_count > 0)
    {
        editable_events::iterator ei = m_top_iterator;
        for (int event = 0; event < m_display_count; ++event)
        {
            // int sequence = i + m_top_event_index;

            if (ei != m_event_container.end())
            {
                draw_event(ei, event);
                ++ei;
            }
            else
                break;
        }
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
#if 0
    int y_f = m_window_y / m_slots_y;
    for (int y = 0; y <= y_f; y++)
    {
        int seq = y + m_top_event_index;
        if (seq < m_event_count)
        {
            bool dirty = (perf().is_dirty_names(seq));
            if (dirty)
                draw_event(seq);
        }
    }
#endif
}

}           // namespace seq64

/*
 * eventslots.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

