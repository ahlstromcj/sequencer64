#ifndef SEQ64_EVENTSLOTS_HPP
#define SEQ64_EVENTSLOTS_HPP

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
 * \file          eventslots.hpp
 *
 *  This module declares/defines the base class for displaying events in their
 *  editing slots.
 *
 * \library       sequencer64 application
 * \author        Chris Ahlstrom
 * \date          2015-12-05
 * \updates       2017-04-30
 * \license       GNU GPLv2 or above
 *
 *  This class supports the left side of the Event Editor window.
 */

#include "editable_events.hpp"          /* seq64::editable_events container  */
#include "gui_drawingarea_gtk2.hpp"

/**
 *  Indicates that an event index is not useful.
 */

#define SEQ64_NULL_EVENT_INDEX          (-1)

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace Gtk
{
    class Adjustment;
}

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{
    class eventedit;
    class perform;
    class sequence;

/**
 *  This class implements the left-side list of events in the pattern
 *  event-edit window.
 */

class eventslots : public gui_drawingarea_gtk2
{

    friend class eventedit;

private:

    /**
     *  Provides a link to the eventedit that created this object.
     */

    eventedit & m_parent;

    /**
     *  Provides a reference to the sequence that this dialog is meant to view
     *  or modify.
     */

    sequence & m_seq;

    /**
     *  Holds the editable events for this sequence.
     */

    editable_events m_event_container;

    /**
     *  Provides the number of the characters in the name box.  Pretty much
     *  hardwired to 64 at present.  It helps determine the m_slots_x value
     *  (the width of the eventslots list).
     */

    int m_slots_chars;

    /**
     *  Provides the "real" width of a character.  This value is obtained from
     *  a font-renderer accessor function.
     */

    int m_char_w;

    /**
     *  Provides the width of the "set number" box.  This used to be hardwired
     *  to 6 * 2 (character-width times two).
     */

    int m_setbox_w;

    /**
     *  Provides the width of the names box, which is the width of a character
     *  for 24 characters.
     */

    int m_slots_x;

    /**
     *  Provides the height of the names box, which is hardwired to 24 pixels.
     *  This value was once 22 pixels, but we need a little extra room for our
     *  new font.  This extra room is compatible enough with the old font, as
     *  well.
     */

    int m_slots_y;

    /**
     *  The current number of events in the edited container.
     */

    int m_event_count;

    /**
     *  Counts the number of displayed events, which depends on how many
     *  events there are (m_event_count) and the size of the event list
     *  (m_line_maximum).
     */

    int m_line_count;

    /**
     *  Counts the maximum number of displayed events, which depends on
     *  the size of the event list (and thus the size of the dialog box for
     *  the event editor).
     */

    int m_line_maximum;

    /**
     *  Provides a little overlap for paging through the frame.
     */

    int m_line_overlap;

    /**
     *  The index of the event that is 0th in the visible list of events.
     *  It is used in numbering the events that are shown in the event-slot
     *  frame.  Do not confuse it with m_current_index, which is relative to
     *  the frame, not the container-beginning.
     */

    int m_top_index;

    /**
     *  Indicates the index of the current event within the frame.
     *  This event will also be pointed to by the m_current_event iterator.
     *  Do not confuse it with m_top_index, which is relative to the
     *  container-beginning, not the frame.
     */

    int m_current_index;

    /**
     *  Provides the top "pointer" to the start of the editable-events section
     *  that is being shown in the user-interface.
     */

    editable_events::iterator m_top_iterator;

    /**
     *  Provides the bottom "pointer" to the end of the editable-events section
     *  that is being shown in the user-interface.
     */

    editable_events::iterator m_bottom_iterator;

    /**
     *  Provides the "pointer" to the event currently in focus.
     */

    editable_events::iterator m_current_iterator;

    /**
     *  Indicates the event index that matches the index value of the vertical
     *  pager.
     */

    int m_pager_index;

public:

    eventslots
    (
        perform & p,
        eventedit & parent,
        sequence & seq,
        Gtk::Adjustment & vadjust
    );

    /**
     *  Let's provide a do-nothing virtual destructor.
     */

    virtual ~eventslots ()
    {
        // I got nothin'
    }

    /**
     * \getter m_event_count
     *      Returns the number of total events in the sequence represented by
     *      the eventslots object.
     */

    int event_count () const
    {
        return m_event_count;
    }

    /**
     * \getter m_event_count
     */

    int count () const
    {
        return m_event_count;
    }

    /**
     * \getter m_line_count
     *      Returns the current number of rows (events) in the eventslots's
     *      display.
     */

    int line_count () const
    {
        return m_line_count;
    }

    /**
     * \getter m_line_maximum
     *      Returns the maximum number of rows (events) in the eventslots's
     *      display.
     */

    int line_maximum () const
    {
        return m_line_maximum;
    }

    /**
     *  Provides the "page increment" or "line increment" of the frame,
     *  This value is the current line-maximum of the frame minus its
     *  overlap value.
     */

    int line_increment () const
    {
        return m_line_maximum - m_line_overlap;
    }

    /**
     * \getter m_top_index
     */

    int top_index () const
    {
        return m_top_index;
    }

    /**
     * \getter m_current_index
     */

    int current_index () const
    {
        return m_current_index;
    }

    /**
     * \getter m_pager_index
     */

    int pager_index () const
    {
        return m_pager_index;
    }

private:

    bool load_events ();
    void set_current_event
    (
        const editable_events::iterator ei,
        int index,
        bool full_redraw = true
    );
    bool insert_event (const editable_event & edev);
    bool insert_event
    (
        const std::string & evtimestamp,
        const std::string & evname,
        const std::string & evdata0,
        const std::string & evdata1
    );
    bool delete_current_event ();
    bool modify_current_event
    (
        const std::string & evtimestamp,
        const std::string & evname,
        const std::string & evdata0,
        const std::string & evdata1
    );
    bool save_events ();
    void select_event
    (
        int event_index = SEQ64_NULL_EVENT_INDEX,
        bool full_redraw = true
    );
    void set_text
    (
        const std::string & evcategory,
        const std::string & evtimestamp,
        const std::string & evname,
        const std::string & evdata0,
        const std::string & evdata1
    );

    void enqueue_draw ();
    int convert_y (int y);
    void draw_event (editable_events::iterator ei, int index);
    void draw_events ();
    void change_vert ();
    void page_movement (int new_value);
    void page_topper (editable_events::iterator newcurrent);
    int decrement_top ();
    int increment_top ();
    int decrement_current ();
    int increment_current ();
    int decrement_bottom ();
    int increment_bottom ();

private:    // Gtk callbacks

    void on_realize ();
    bool on_expose_event (GdkEventExpose * ev);
    bool on_button_press_event (GdkEventButton * ev);
    bool on_button_release_event (GdkEventButton * ev);
    bool on_focus_in_event (GdkEventFocus * ev);
    bool on_focus_out_event (GdkEventFocus * ev);
    bool on_scroll_event (GdkEventScroll * ev);
    void on_size_allocate (Gtk::Allocation &);
    void on_move_up ();
    void on_move_down ();
    void on_frame_up ();
    void on_frame_down ();
    void on_frame_home ();
    void on_frame_end ();

};

}           // namespace seq64

#endif      // SEQ64_EVENTSLOTS_HPP

/*
 * eventslots.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

