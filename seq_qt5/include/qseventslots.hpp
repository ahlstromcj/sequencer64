#ifndef SEQ64_QSEVENTSLOTS_HPP
#define SEQ64_QSEVENTSLOTS_HPP

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
 * \file          qseventslots.hpp
 *
 *  This module declares/defines the base class for displaying events in their
 *  editing slots.
 *
 * \library       sequencer64 application
 * \author        Chris Ahlstrom
 * \date          2018-08-13
 * \updates       2018-08-13
 * \license       GNU GPLv2 or above
 *
 *  This class supports the left side of the Qt 5 version of the Event Editor
 *  window.  One big difference from the Gtkmm-2.4 version is that a table
 *  widget will be used to display the events.
 */

#include "editable_events.hpp"          /* seq64::editable_events container  */

/**
 *  Indicates that an event index is not useful.
 */

#define SEQ64_NULL_EVENT_INDEX          (-1)

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{
    class qseqeventframe;
    class perform;
    class sequence;

/**
 *  This class implements the left-side list of events in the pattern
 *  event-edit window.
 */

class qseventslots
{
    friend class qseqeventframe;

private:

    /**
     *  Provides a link to the qseqeventframe that created this object.
     */

    qseqeventframe & m_parent;

    /**
     *  Provides a reference to the sequence that this dialog is meant to view
     *  or modify.
     */

    sequence & m_seq;

    /**
     *  Holds the editable events for this sequence.  This container is what
     *  is edited, and any changes made to it are not saved to the container
     *  until the user pushes the "save" button.
     */

    editable_events m_event_container;

    /**
     *  The current number of events in the edited container.
     */

    int m_event_count;

    /**
     *  Holds the previous length of the edited sequence, in MIDI pulses, so
     *  that we can detect changes in the length of the sequence.
     */

    midipulse m_last_max_timestamp;

    /**
     *  Holds the current number of measures, for display purposes.
     */

    int m_measures;

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

    qseventslots
    (
        perform & p,
        qseqeventframe & parent,
        sequence & seq
    );

    /**
     *  Let's provide a do-nothing virtual destructor.
     */

    virtual ~qseventslots ()
    {
        // I got nothin'
    }

    /**
     * \getter m_event_container.get_length()
     */

    midipulse get_length () const
    {
        return m_event_container.get_length();
    }

    /**
     * \getter m_event_count
     *      Returns the number of total events in the sequence represented by
     *      the qseventslots object.
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
     *      Returns the current number of rows (events) in the qseventslots's
     *      display.
     */

    int line_count () const
    {
        return m_line_count;
    }

    /**
     * \getter m_line_maximum
     *      Returns the maximum number of rows (events) in the qseventslots's
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

    /**
     * \getter m_seq const version
     */

    const sequence & seq () const
    {
        return m_seq;
    }

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

    int convert_y (int y);
    void draw_event (editable_events::iterator ei, int index);
    void draw_events ();
    void page_movement (int new_value);
    void page_topper (editable_events::iterator newcurrent);
    int decrement_top ();
    int increment_top ();
    int decrement_current ();
    int increment_current ();
    int decrement_bottom ();
    int increment_bottom ();
    int calculate_measures () const;

};          // class qseventslots

}           // namespace seq64

#endif      // SEQ64_QSEVENTSLOTS_HPP

/*
 * qseventslots.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

