#ifndef SEQ64_EDITABLE_EVENTS_HPP
#define SEQ64_EDITABLE_EVENTS_HPP

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
 * \file          editable_events.hpp
 *
 *  This module declares/defines a sorted container for editable_events class
 *  for operating with an ordered collection MIDI editable_events in a
 *  user-interface.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-12-04
 * \updates       2015-12-05
 * \license       GNU GPLv2 or above
 *
 *  This module extends the event class to support conversions between events
 *  and human-readable (and editable) strings.
 */

#include <map>                          /* std::multimap                */

#include "event_list.hpp"               /* seq64::event_list::event_key */
#include "editable_event.hpp"           /* seq64::editable_event        */

namespace seq64
{

class sequence;

/**
 *  Provides for the management of an ordered collection MIDI editable events.
 */

class editable_events
{

private:

    /**
     *  Types to use to with the multimap implementation.  These typenames are
     *  identical to those used in event_list, but of course they are in the
     *  editable_events scope instead.
     */

    typedef event_list::event_key Key;
    typedef std::multimap<Key, editable_event> Events;
    typedef std::pair<Key, editable_event> EventsPair;
    typedef std::multimap<Key, editable_event>::iterator iterator;
    typedef std::multimap<Key, editable_event>::const_iterator const_iterator;

    /**
     *  Holds the editable_events.
     */

    Events m_events;

    /**
     *  Points to the current event.  From this event we get the current time
     *  and other parameters.  If the container were a plain map, we could
     *  instead use a key to access it.  But we can at least use an iterator,
     *  rather than a bare pointer.
     */

    iterator m_current_event;

    /**
     *  Provides a reference to the sequence containing the events to be
     *  edited.  Besides the events, this object also holds the beats/measure,
     *  beat-width, and the PPQN value.  The beats/minute have to be obtained
     *  from the application's perform object, and passed to the
     *  editable_events constructor by the caller.
     */

    sequence & m_sequence;

    /**
     *  Holds the current settings for the sequence (and usually for the whole
     *  MIDI tune as well).  It holds the beats/minute, beats/measure,
     *  beat-width, and PPQN values needed to properly convert MIDI pulse
     *  timestamps to time and measure values.
     */

    midi_timing m_midi_parameters;

private:

    editable_events ();                 /* unimplemented    */

public:

    editable_events (sequence & seq, int bpm);
    editable_events (const editable_events & rhs);
    editable_events & operator = (const editable_events & rhs);

    /**
     *  This destructor current is a rote virtual function override.
     */

    virtual ~editable_events ()         // VIRTUAL???
    {
        // Empty body
    }

public:

    /*

    bool load_events
    (
        const sequence & seq
    );

    bool save_events
    (
        const sequence & seq
    );

     * Other operations:
     *
     *      insert event
     *      remove event
     *      replace event
     *      get or format event data (for use in GUI)
     */

public:

    /**
     * \getter m_events.begin(), non-constant version.
     */

    iterator begin ()
    {
        return m_events.begin();
    }

    /**
     * \getter m_events.begin(), constant version.
     */

    const_iterator begin () const
    {
        return m_events.begin();
    }

    /**
     * \getter m_events.end(), non-constant version.
     */

    iterator end ()
    {
        return m_events.end();
    }

    /**
     * \getter m_events.end(), constant version.
     */

    const_iterator end () const
    {
        return m_events.end();
    }

    /**
     *  Returns the number of events stored in m_events.  We like returning
     *  an integer instead of size_t, and rename the function so nobody is
     *  fooled.
     */

    int count () const
    {
        return int(m_events.size());
    }

    bool add (const event & e, bool postsort = true);

    /**
     *  Provides a wrapper for the iterator form of erase(), which is the
     *  only one that sequence uses.
     */

    void remove (iterator ie)
    {
        m_events.erase(ie);
    }

    /**
     *  Provides a wrapper for clear().
     */

    void clear ()
    {
        m_events.clear();
    }

    /**
     * \getter m_current_event
     *      The caller must make sure the iterator is not Events::end().
     */

    iterator current_event () const
    {
        return m_current_event;
    }

    void current_event (iterator cei)
    {
        m_current_event = cei;
    }

private:

    // void format_timestamp ();

};          // class editable_events

}           // namespace seq64

#endif      // SEQ64_EDITABLE_EVENTS_HPP

/*
 * editable_events.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

