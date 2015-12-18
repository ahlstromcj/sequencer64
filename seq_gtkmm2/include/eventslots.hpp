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
 *  This module declares/defines the base class for performance names.
 *
 * \library       sequencer64 application
 * \author        Chris Ahlstrom
 * \date          2015-12-05
 * \updates       2015-12-18
 * \license       GNU GPLv2 or above
 *
 *  This class supports the left side of the Performance window (also known
 *  as the Song window).
 */

#include "globals.h"
#include "editable_events.hpp"          /* seq64::editable_events container  */
#include "gui_drawingarea_gtk2.hpp"

namespace Gtk
{
    class Adjustment;
}

namespace seq64
{

class eventedit;
class perform;
class sequence;

/**
 *  This class implements the left-side keyboard in the patterns window.
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
     *  hardwired to 24 at present.
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
     *  Provides the width of the "slot" box.
     */

    int m_slots_box_w;

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
     *  Provides the horizontal and vertical offsets of the text relative to
     *  the names box.  Currently hardwired.
     */

    int m_xy_offset;

    /**
     *  The current number of events in the edited container.
     */

    int m_event_count;

    /**
     *  Counts the number of displayed events, which depends on how many
     *  events there are and the size of the event list.
     */

    int m_display_count;

    /**
     *  The index of the event that is 0th in the visible list of events.
     *  It is used in numbering the events that are shown in the event-slot
     *  frame.
     */

    int m_top_event_index;

    /**
     *  Indicates the number of events visible based on the current size of
     *  the visible editable event list.  Starts out around 42, and should
     *  stay at m_top_event_index + 42 as long as the eventedit window is not
     *  resized vertically.
     */

    int m_bottom_event_index;

    /**
     *  Indicates the index of the current event.
     */

    int m_current_event_index;

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

public:

    eventslots
    (
        perform & p,
        eventedit & parent,
        sequence & seq,
        Gtk::Adjustment & vadjust
    );

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
     * \getter m_display_count
     *      Returns the number of rows (events) in the eventslots's display.
     */

    int display_count () const
    {
        return m_display_count;
    }

    /**
     * \getter m_top_event_index
     *      This value will almost certainly be 0.
     */

    int top_index () const
    {
        return m_top_event_index;
    }

    /**
     * \getter m_bottom_event_index
     *      This value will be around 40 to 50, but will stay constant unless
     *      we allow resizing of the eventedit window.
     */

    int bottom_index () const
    {
        return m_bottom_event_index;
    }

    /**
     * \getter m_current_event_index
     */

    int current_index () const
    {
        return m_current_event_index;
    }

private:

    bool load_events ();
    void set_current_event (const editable_events::iterator ei, int index);
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
    void select_event (int event_index);
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

    /**
     *  This function does nothing.
     */

    void update_pixmap ()
    {
        // Empty body
    }

    /**
     *  This function does nothing.
     */

    void draw_area ()
    {
        // Empty body
    }

    /**
     *  Redraw the given sequence.
     */

    void redraw ()
    {
        // draw_event(eventindex);
    }

private:    // Gtk callbacks

    void on_realize ();
    bool on_expose_event (GdkEventExpose * ev);
    bool on_button_press_event (GdkEventButton * ev);
    bool on_button_release_event (GdkEventButton * ev);
    bool on_focus_in_event (GdkEventFocus * ev);
    bool on_focus_out_event (GdkEventFocus * ev);
    bool on_key_press_event (GdkEventKey * ev);
    bool on_scroll_event (GdkEventScroll * ev);
    void on_size_allocate (Gtk::Allocation &);

};

}           // namespace seq64

#endif      // SEQ64_EVENTSLOTS_HPP

/*
 * eventslots.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

