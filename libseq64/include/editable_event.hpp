#ifndef SEQ64_EDITABLE_EVENT_HPP
#define SEQ64_EDITABLE_EVENT_HPP

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
 * \file          editable_event.hpp
 *
 *  This module declares/defines the editable_event class for operating with
 *  MIDI editable_events.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-11-28
 * \updates       2015-11-28
 * \license       GNU GPLv2 or above
 *
 *  This module extends the event class to support conversions between events
 *  and human-readable (and editable) strings.
 */

#include "event.hpp"                    /* seq64::event */

namespace seq64
{

/**
 *  Provides for the management of MIDI editable events.
 */

class editable_event : public event
{

private:

    /**
     *  These values determine the major kind of event, which determines what
     *  types of events are possible for this editable event object.
     *  These tags are accompanied by category names in sm_category_names[].
     *  The enum values are cast to midibyte values for the purposes of using
     *  the lookup infrastructure.
     *
     * \var category_channel_message
     *      Indicates a channel event, with a code ranging from 0x80 through
     *      0xEF.  Some examples are note on/off, control change, and program
     *      change.  Values looked up in sm_event_names[].
     *
     * \var category_system_message
     *      Indicates a system event, with a code ranging from 0xF0 through
     *      0xFF.  Some examples are SysEx start/end, song position, and
     *      stop/start/continue/reset.  Values looked up in sm_event_names[].
     *
     * \var category_meta_event
     *      Indicates a meta event, and there is a second code that is used to
     *      look up the name of the meta event, in sm_meta_event_names[].
     *
     * \var category_prop_event
     *      Indicates a "proprietary", Sequencer64 event.  Indicates to look
     *      up the name of the event in sm_prop_event_names[].  Not sure if
     *      these kinds of events will be stored separately.
     */

    enum
    {
        category_channel_message,
        category_system_message,
        category_meta_event,
        category_prop_event,

    } category_t;

    /**
     *  Provides a type that contains the pair of values needed for the
     *  various lookup maps that are needed to manage editable events.
     */

    typedef struct
    {
        midibyte event_value;
        std::string event_name;

    } name_value_t;

    /**
     *  An array of event categories and their names.
     */

    static name_value_t sm_category_names [];

    /**
     *  An array of MIDI events and their names.
     */

    static name_value_t sm_event_names [];

    /**
     *  An array of Meta events and their names.
     */

    static name_value_t sm_meta_event_names [];

    /**
     *  An array of Sequencer64-specific events and their names.
     */

    static name_value_t sm_prop_event_names [];

    /**
     *  
     */

    /**
     *  Holds the name of the event, select
     */

    unsigned long m_xxx;

public:

    editable_event ();
    editable_event (const editable_event & rhs);
    editable_event & operator = (const editable_event & rhs);
    ~editable_event ();

    /*
     * Operator overload, the only one needed for sorting editable_events in a
     * list or a map.
     */

    bool operator < (const editable_event & rhseditable_event) const;

};

}           // namespace seq64

#endif      // SEQ64_EDITABLE_EVENT_HPP

/*
 * editable_event.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

