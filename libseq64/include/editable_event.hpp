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
 * \updates       2015-11-29
 * \license       GNU GPLv2 or above
 *
 *  This module extends the event class to support conversions between events
 *  and human-readable (and editable) strings.
 */

#include "event.hpp"                    /* seq64::event */

/**
 *  Provides an integer value that is larger than any midibyte value, to be
 *  used to terminate a array of items keyed by a midibyte value.
 */

#define SEQ64_END_OF_MIDIBYTE_TABLE     0x100       /* one more than 0xFF   */

namespace seq64
{

/**
 *  Provides for the management of MIDI editable events.  It makes the
 *  following members of an event modifiable using human-readable strings:
 *
 *      -   m_timestamp
 *      -   m_status
 *      -   m_channel
 *      -   m_data[]
 *
 *  Eventually, it would be nice to be able to edit, or at least view, the
 *  SysEx events and the Meta events.  Those two will require extensions to
 *  make events out of them (SysEx is partly supported).
 *
 *  To the concepts of event, the editable_event class adds a category field
 *  and strings to represent all of these members.
 */

class editable_event : public event
{

public:

    /**
     *  These values determine the major kind of event, which determines what
     *  types of events are possible for this editable event object.
     *  These tags are accompanied by category names in sm_category_names[].
     *  The enum values are cast to midibyte values for the purposes of using
     *  the lookup infrastructure.
     *
     * \var category_name
     *      Indicates that the lookup needs to be done on the category names,
     *      as listed in sm_category_names[].
     *
     * \var category_channel_message
     *      Indicates a channel event, with a value ranging from 0x80 through
     *      0xEF.  Some examples are note on/off, control change, and program
     *      change.  Values are looked up in sm_channel_event_names[].
     *
     * \var category_system_message
     *      Indicates a system event, with a value ranging from 0xF0 through
     *      0xFF.  Some examples are SysEx start/end, song position, and
     *      stop/start/continue/reset.  Values are looked up in
     *      sm_system_event_names[].
     *
     * \var category_meta_event
     *      Indicates a meta event, and there is a second value that is used to
     *      look up the name of the meta event, in sm_meta_event_names[].
     *
     * \var category_prop_event
     *      Indicates a "proprietary", Sequencer64 event.  Indicates to look
     *      up the name of the event in sm_prop_event_names[].  Not sure if
     *      these kinds of events will be stored separately.
     */

    typedef enum
    {
        category_name,              /* sm_category_names[]          */
        category_channel_message,   /* sm_channel_event_names[]     */
        category_system_message,    /* sm_system_event_names[]      */
        category_meta_event,        /* sm_meta_event_names[]        */
        category_prop_event         /* sm_prop_event_names[]        */

    } category_t;

    /**
     *  Provides a type that contains the pair of values needed for the
     *  various lookup maps that are needed to manage editable events.
     *
     * \var event_value
     *      Holds a midibyte value (0x00 to 0xFF) or
     *      SEQ64_END_OF_MIDIBYTE_TABLE to indicate the end of an array of
     *      name_value_t items.
     *
     * \var event_name
     *      Holds the human-readable name for an event code or other numeric
     *      value in an array of name_value_t items.
     */

    typedef struct
    {
        unsigned short event_value;
        std::string event_name;

    } name_value_t;

    /**
     *  An array of event categories and their names.
     */

    static const name_value_t sm_category_names [];

    /**
     *  An array of MIDI channel events and their names.  We split channel and
     *  system messages into two arrays, for semantic reasons and for faster
     *  linear lookups.
     */

    static const name_value_t sm_channel_event_names [];

    /**
     *  An array of MIDI system events and their names.  We split channel and
     *  system messages into two arrays, for semantic reasons and for faster
     *  linear lookups.
     */

    static const name_value_t sm_system_event_names [];

    /**
     *  An array of Meta events and their names.
     */

    static const name_value_t sm_meta_event_names [];

    /**
     *  An array of Sequencer64-specific events and their names.
     */

    static const name_value_t sm_prop_event_names [];

    /**
     *  Provides for fast access (no ifs) to the correct name array for the
     *  given category.  Too bad that an array of references is not possible.
     */

    static const name_value_t * const sm_category_arrays [];

    /*
     *  Static lookup functions, described in the cpp module.
     */

    static std::string value_to_name (midibyte value, category_t cat);
    static unsigned short name_to_value
    (
        const std::string & name,
        category_t cat
    );

private:

    /**
     *  Indicates the overall category of this event, which will be
     *  category_channel_message, category_system_message,
     *  category_meta_event, and category_prop_event.  The category_name value
     *  is not set here, since that category is used only for looking up the
     *  human-readable form of the category.
     */

    category_t m_category;

    /**
     *  Holds the name of the event category for this event.
     */

    std::string m_name_category;

    /**
     *  Holds the string version of the MIDI pulses time-stamp.
     */

    std::string m_name_timestamp;

    /**
     *  Holds the name of the status value for this event.  It will include
     *  the names of the channel messages and the system messages.  The latter
     *  includes SysEx and Meta messages.
     */

    std::string m_name_status;

    /**
     *  Holds the name of the meta message, if applicable.  If not applicable,
     *  this name will be empty.
     */

    std::string m_name_meta;

    /**
     *  If we eventually implement the editing of the Seq24/Sequencer64
     *  "proprietary" meta sequencer-specific events, the name of the SeqSpec
     *  will be stored here.
     */

    std::string m_name_seqspec;

public:

    editable_event ();
    editable_event (const editable_event & rhs);
    editable_event & operator = (const editable_event & rhs);

    /**
     *  This destructor current is a rote virtual function override.
     */

    virtual ~editable_event ()
    {
        // Empty body
    }

    /*
     * Operator overload, the only one needed for sorting editable_events in a
     * list or a map.

    bool operator < (const editable_event & rhseditable_event) const;
     */

public:

    /**
     * \getter m_category
     */

    category_t category () const
    {
        return m_category;
    }

    void category (category_t c);

    /**
     * \getter m_category
     */

    const std::string & category_string () const
    {
        return m_name_category;
    }

    void category (const std::string & cs);

    /**
     * \getter m_name_timestamp
     */

    const std::string & timestamp_string () const
    {
        return m_name_timestamp;
    }

    /**
     * \getter event::get_timestamp()
     *      Implemented to allow a uniform naming convention that is not
     *      slavish to the get/set crowd [this ain't Java].
     */

    midipulse timestamp () const
    {
        return event::get_timestamp();
    }
    void timestamp (midipulse ts);

};          // class editable_event

}           // namespace seq64

#endif      // SEQ64_EDITABLE_EVENT_HPP

/*
 * editable_event.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

