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
 * \file          event_list.cpp
 *
 *  This module declares/defines a class for handling
 *  MIDI events in a list container.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-09-19
 * \updates       2015-10-02
 * \license       GNU GPLv2 or above
 *
 */

#include <stdio.h>                      // printf()
#include "easy_macros.h"
#include "event_list.hpp"

namespace seq64
{

/*
 * Section: event_key
 */

#ifdef USE_EVENT_MAP

/**
 *  Principal event_key constructor.
 */

event_list::event_key::event_key (unsigned long tstamp, int rank)
 :
    m_timestamp (tstamp),
    m_rank      (rank)
{
    // Empty body
}

/**
 *  Event-based constructor.  This constructor makes it even easier to
 *  create an event_key.  Note that the call to event::get_rank() makes a
 *  simple calculation based on the status of the event.
 */

event_list::event_key::event_key (const event & e)
 :
    m_timestamp (e.get_timestamp()),
    m_rank      (e.get_rank())
{
    // Empty body
}

/**
 *  Provides the minimal operator needed to sort event using an event_key.
 */

bool
event_list::event_key::operator < (const event_key & rhs) const
{
    if (m_timestamp == rhs.m_timestamp)
        return (m_rank < rhs.m_rank);
    else
        return (m_timestamp < rhs.m_timestamp);
}

#endif  // USE_EVENT_MAP

/*
 * Section: event_key
 */

/**
 *  Principal constructor.
 */

event_list::event_list ()
 :
    m_events    ()
{
    // No code needed
}

/**
 *  Copy constructor.
 */

event_list::event_list (const event_list & rhs)
 :
    m_events    (rhs.m_events)
{
    // No code needed
}

/**
 *  Principal assignment operator.  Follows the stock rules for such an
 *  operator, just assigning member values.
 */

event_list &
event_list::operator = (const event_list & rhs)
{
    if (this != &rhs)
    {
        m_events = rhs.m_events;
    }
    return *this;
}

/**
 *  A rote destructor.
 */

event_list::~event_list ()
{
    // No code needed
}

/**
 *  Adds an event to the internal event list in an optionally sorted
 *  manner. It is a wrapper, wrapper for insert() or push_front(), with an
 *  option to call sort().
 *
 *  For the std::multimap implementation, This is an option if we want to make
 *  sure the insertion succeed.
 *
 *      std::pair<Events::iterator, bool> result = m_events.insert(p);
 *      return result.second;
 *
 * \warning
 *      This pushing (and, in writing the MIDI file, the popping),
 *      causes events with identical timestamps to be written in
 *      reverse order.  Doesn't affect functionality, but it's puzzling
 *      until one understands what is happening.  That's why we're
 *      exploring using a multimap as the container.
 */

void
event_list::add (const event & e, bool postsort)
{

#ifdef USE_EVENT_MAP
    event_key key(e);
#if __cplusplus >= 201103L
    EventsPair p = std::make_pair(key, e);
#else
    EventsPair p = std::make_pair<event_key, event>(key, e);
#endif
    m_events.insert(p);
#else
    m_events.push_front(e);
#endif

    if (postsort)
        sort();                         /* by time-stamp and "rank" */
}

#ifdef USE_EVENT_MAP

/**
 *  Provides a merge operation for the event multimap analogous to the merge
 *  operation for the event list.  We have certain constraints to
 *  preserve, as the following discussion shows.
 *
 *  For std::list, sequence merges list T into list A by first calling
 *  T.sort(), and then A.merge(T).  The merge() operation merges T into A
 *  by transferring all of its elements, at their respective ordered
 *  positions, into A.  Both containers must already be ordered.
 *
 *  The merge effectively removes all the elements in T (which becomes
 *  empty), and inserts them into their ordered position within container
 *  (which expands in size by the number of elements transferred). The
 *  operation is performed without constructing nor destroying any
 *  element, whether T is an lvalue or an rvalue, or whether the
 *  value-type supports move-construction or not.
 *
 *  Each element of T is inserted at the position that corresponds to its
 *  value according to the strict weak ordering defined by operator <. The
 *  resulting order of equivalent elements is stable (i.e. equivalent
 *  elements preserve the relative order they had before the call, and
 *  existing elements precede those equivalent inserted from x).  The
 *  function does nothing if (&x == this).
 *
 *  For std::multimap, sorting is automatic.  However, unless move-construction
 *  is supported, merging will be less efficient than for the list
 *  version.  Also, we need a way to include duplicates of each event, so
 *  we need to use a multi-map.  Once all this setup, merging is really
 *  just insertion.
 */

void
event_list::merge (event_list & el, bool presort)
{
    if (presort)
        el.sort();

    int initialsize = count();
    int addedsize = el.count();
    m_events.insert(el.events().begin(), el.events().end());
    if (count() != (initialsize + addedsize))
    {
        char tmp[64];
        snprintf
        (
            tmp, sizeof(tmp), "WARNING: %d + %d inserted = %d",
            initialsize, addedsize, count()
        );
        warnprint(tmp);
    }
}

#else   // USE_EVENT_MAP

void
event_list::merge (event_list & el, bool presort)
{
    if (presort)
        el.m_events.sort();

    m_events.merge(el.m_events);
}

#endif  // USE_EVENT_MAP

/**
 *  Links a new event.  This function checks for a note on, then look for
 *  its note off.  This function is provided in the event_list because it
 *  does not depend on any external data.  Also note that any desired
 *  thread-safety must be provided by the caller.
 */

void
event_list::link_new ()
{
    bool endfound = false;
    for (Events::iterator on = m_events.begin(); on != m_events.end(); ++on)
    {
        event & eon = dref(on);
        if (eon.is_note_on() && ! eon.is_linked())  /* note on, unlinked?i  */
        {
            Events::iterator off = on;              /* point to note on     */
            off++;                                  /* get next element     */
            endfound = false;
            while (off != m_events.end())
            {
                event & eoff = dref(off);
                if                  /* off event, == notes, and not linked  */
                (
                    eoff.is_note_off() &&
                    eoff.get_note() == eoff.get_note() &&
                    ! eoff.is_linked()
                )
                {
                    eon.link(&eoff);                /* link backward        */
                    eoff.link(&eon);                /* link forward         */
                    endfound = true;                /* note on fulfilled    */
                    break;
                }
                off++;
            }
            if (! endfound)
            {
                off = m_events.begin();
                while (off != on)
                {
                    event & eoff = dref(off);
                    if              /* off event, == notes, and not linked  */
                    (
                        eoff.is_note_off() &&
                        eoff.get_note() == eon.get_note() &&
                        ! eoff.is_linked()
                    )
                    {
                        eon.link(&eoff);            /* link backward        */
                        eoff.link(&eon);            /* link forward         */
                        endfound = true;            /* note on fulfilled    */
                        break;
                    }
                    off++;
                }
            }
        }
    }
}

/**
 *  This function verifies state: all note-ons have an off, and it links
 *  note-offs with their note-ons.
 *
 * \threadsafe
 */

void
event_list::verify_and_link (long slength)
{
    clear_links();
    for (event_list::iterator on = m_events.begin(); on != m_events.end(); on++)
    {
        event & eon = dref(on);
        if (eon.is_note_on())          /* note on, look for its note off */
        {
            event_list::iterator off = on;      /* get next possible off node */
            off++;
            bool endfound = false;
            while (off != m_events.end())
            {
                event & eoff = dref(off);
                if              /* is a off event, == notes, and isn't marked  */
                (
                    eoff.is_note_off() &&
                    eoff.get_note() == eon.get_note() &&
                    ! eoff.is_marked()
                )
                {
                    eon.link(&eoff);                    /* link + mark */
                    eoff.link(&eon);
                    eon.mark();
                    eoff.mark();
                    endfound = true;
                    break;
                }
                off++;
            }
            if (! endfound)
            {
                off = m_events.begin();
                while (off != on)
                {
                    event & eoff = dref(off);
                    if
                    (
                        eoff.is_note_off() &&
                        eoff.get_note() == eon.get_note() &&
                        ! eoff.is_marked()
                    )
                    {
                        eon.link(&eoff);                /* link + mark */
                        eoff.link(&eon);
                        eon.mark();
                        eoff.mark();
                        endfound = true;
                        break;
                    }
                    off++;
                }
            }
        }
    }
    unmark_all();
    mark_out_of_range(slength);
}

/**
 *  Clears all event links and unmarks them all.
 */

void
event_list::clear_links ()
{
    for (Events::iterator i = m_events.begin(); i != m_events.end(); ++i)
    {
        event & e = dref(i);
        e.clear_link();
        e.unmark();
    }
}

/**
 *  Marks all selected events.
 */

void
event_list::mark_selected ()
{
    for (Events::iterator i = m_events.begin(); i != m_events.end(); ++i)
    {
        event & e = dref(i);
        if (e.is_selected())
            e.mark();
    }
}

/**
 *  Unmarks all events.
 */

void
event_list::unmark_all ()
{
    for (Events::iterator i = m_events.begin(); i != m_events.end(); ++i)
        dref(i).unmark();
}

/**
 *  Marks all events that have a time-stamp that is out of range.
 *  Used for killing (pruning) those events not in range.  If the current
 *  time-stamp is greater than the length, then the event is marked for
 *  pruning.
 */

void
event_list::mark_out_of_range (long slength)
{
    for (Events::iterator i = m_events.begin(); i != m_events.end(); ++i)
    {
        event & e = dref(i);
        if (e.get_timestamp() >= slength || e.get_timestamp() < 0)
        {
            e.mark();                            /* we have to prune it */
            if (e.is_linked())
                e.get_linked()->mark();
        }
    }
}

/**
 *  Unpaints all list-events.
 */

void
event_list::unpaint_all ()
{
    for (Events::iterator i = m_events.begin(); i != m_events.end(); ++i)
        dref(i).unpaint();
}

/**
 *  Counts the selected note-on events in the event list.
 */

int
event_list::count_selected_notes ()
{
    int result = 0;
    for (Events::iterator i = m_events.begin(); i != m_events.end(); ++i)
    {
        if (dref(i).is_note_on() && dref(i).is_selected())
            ++result;
    }
    return result;
}

/**
 *  Counts the selected events, with the given status, in the event list.
 *  If the event is a control change (CC), then it must also match the
 *  given CC value.
 */

int
event_list::count_selected_events (unsigned char status, unsigned char cc)
{
    int result = 0;
    for (Events::iterator i = m_events.begin(); i != m_events.end(); ++i)
    {
        event & e = dref(i);
        if (e.get_status() == status)
        {
            unsigned char d0, d1;
            e.get_data(&d0, &d1);               /* get the two data bytes */
            if
            (
                (status == EVENT_CONTROL_CHANGE && d0 == cc) ||
                (status != EVENT_CONTROL_CHANGE)
            )
            {
                if (e.is_selected())
                    result++;
            }
        }
    }
    return result;
}

/**
 *  Selects all events, unconditionally.
 */

void
event_list::select_all ()
{
    for (Events::iterator i = m_events.begin(); i != m_events.end(); ++i)
        dref(i).select();
}

/**
 *  Deselects all events, unconditionally.
 */

void
event_list::unselect_all ()
{
    for (Events::iterator i = m_events.begin(); i != m_events.end(); ++i)
        dref(i).unselect();
}

/**
 *  Prints a list of the currently-held events.
 */

void
event_list::print ()
{
    printf("events[%ld]\n\n", m_events.size());
    for (Events::iterator i = m_events.begin(); i != m_events.end(); ++i)
        dref(i).print();

    printf("events[%d]\n\n", count());
}

}           // namespace seq64

/*
 * sequence.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
