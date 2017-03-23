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
 * \updates       2016-03-21
 * \license       GNU GPLv2 or above
 *
 *  This module extends the event class to support conversions between events
 *  and human-readable (and editable) strings.
 */

#include <map>                          /* std::multimap                */

#include "event_list.hpp"               /* seq64::event_list::event_key */
#include "editable_event.hpp"           /* seq64::editable_event        */

/**
 *  Provides a brief, searchable notation for the use of the
 *  editable_events::dref() function.  Comparable to the DREF() macro in the
 *  event_list module.
 */

#define EEDREF(e)       editable_events::dref(e)

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{

class sequence;

/**
 *  Provides for the management of an ordered collection MIDI editable events.
 */

class editable_events
{
    friend class eventslots;            /* part of ed-event user-interface */

private:

#ifdef SEQ64_USE_EVENT_MAP

    /**
     *  Types to use to with the multimap implementation.  These typenames are
     *  identical to those used in event_list, but of course they are in the
     *  editable_events scope instead.  See the event_list class.
     */

    typedef event_list::event_key Key;
    typedef std::pair<Key, editable_event> EventsPair;
    typedef std::multimap<Key, editable_event> Events;
    typedef std::multimap<Key, editable_event>::iterator iterator;
    typedef std::multimap<Key, editable_event>::const_iterator const_iterator;

#else

    /**
     *  Types to use to with the list implementation.  These typenames are
     *  identical to those used in event_list, but of course they are in the
     *  editable_events scope instead.  See the event_list class.
     */

    typedef std::list<editable_event> Events;
    typedef std::list<editable_event>::iterator iterator;
    typedef std::list<editable_event>::const_iterator const_iterator;

#endif  // SEQ64_USE_EVENT_MAP

    /**
     *  Holds the editable_events.
     */

    Events m_events;

    /**
     *  Points to the current event, which is the event that has just been
     *  inserted.  (From this event we can get the current time and other
     *  parameters.)  If the container were a plain map, we could instead use
     *  a key to access it.  But we can at least use an iterator, rather than
     *  a bare pointer.
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

    editable_events (sequence & seq, midibpm bpm);
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

    /**
     * \getter m_midi_parameters
     */

    const midi_timing & timing () const
    {
        return m_midi_parameters;
    }

    /**
     *  Calculates the MIDI pulses (divisions) from a string using one of the
     *  free functions of the calculations module.
     */

    midipulse string_to_pulses (const std::string & ts_string) const
    {
        return seq64::string_to_pulses(ts_string, timing());
    }

    bool load_events ();
    bool save_events ();

    /*
     * Other operations:
     *
     *      replace event
     *      get or format event data (for use in GUI)
     */

    /**
     * \getter m_events
     */

    Events & events ()
    {
        return m_events;
    }

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
     *  Dereference access for list or map.
     *
     * \param ie
     *      Provides the iterator to the event to which to get a reference.
     */

    static editable_event & dref (iterator ie)
    {
#ifdef SEQ64_USE_EVENT_MAP
        return ie->second;
#else
        return *ie;
#endif
    }

    /**
     *  Dereference const access for list or map.
     *
     * \param ie
     *      Provides the iterator to the event to which to get a reference.
     */

    static const editable_event & dref (const_iterator ie)
    {
#ifdef SEQ64_USE_EVENT_MAP
        return ie->second;
#else
        return *ie;
#endif
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

    bool add (const event & e);
    bool add (const editable_event & e);

    /**
     *  Provides a wrapper for the iterator form of erase(), which is the
     *  only one that the editable_events container uses.
     */

    bool replace (iterator ie, const editable_event & e)
    {
        m_events.erase(ie);
        return add(e);
    }

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

private:

    /**
     * \setter m_current_event
     *
     * \param cei
     *      Provide an iterator to the event to set as the current event.
     */

    void current_event (iterator cei)
    {
        m_current_event = cei;
    }

#ifdef USE_VERIFY_AND_LINK                  /* not yet ready */
    void clear_links ();
    void verify_and_link (midipulse slength);
    void mark_all ();
    void unmark_all ();
    void mark_out_of_range (midipulse slength);
#endif

};          // class editable_events

}           // namespace seq64

#endif      // SEQ64_EDITABLE_EVENTS_HPP

/*
 * editable_events.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

