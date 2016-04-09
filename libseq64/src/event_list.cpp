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
 * \updates       2016-04-09
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

#ifdef SEQ64_USE_EVENT_MAP

/**
 *  Principal event_key constructor.
 *
 * \param tstamp
 *      The time-stamp is the primary part of the key.  It is the most
 *      important key item.
 *
 * \param rank
 *      Rank is an arbitrary number used to prioritize events that have the
 *      same time-stamp.  See the event::get_rank() function for more
 *      information.
 */

event_list::event_key::event_key (midipulse tstamp, int rank)
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
 *
 * \param rhs
 *      Provides the event key to be copied.
 */

event_list::event_key::event_key (const event & rhs)
 :
    m_timestamp (rhs.get_timestamp()),
    m_rank      (rhs.get_rank())
{
    // Empty body
}

/**
 *  Provides the minimal operator needed to sort events using an event_key.
 *
 * \param rhs
 *      Provides the event key to be compared against.
 *
 * \return
 *      Returns true if the rank and timestamp of the current object are less
 *      than those of rhs.
 */

bool
event_list::event_key::operator < (const event_key & rhs) const
{
    if (m_timestamp == rhs.m_timestamp)
        return (m_rank < rhs.m_rank);
    else
        return (m_timestamp < rhs.m_timestamp);
}

#endif  // SEQ64_USE_EVENT_MAP

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
 *
 * \param rhs
 *      Provides the event list to be copied.
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
 *
 * \param rhs
 *      Provides the event list to be assigned.
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
 *
 * \param e
 *      Provides the event to be added to the list.
 *
 * \param postsort
 *      If true, and the std::list implementation has been built in, then the
 *      event list is sorted after the addition.  This is a time-consuming
 *      operation.
 *
 * \return
 *      Returns true if the insertion succeeded, as evidenced by an increment
 *      in container size.
 */

bool
event_list::add (const event & e, bool postsort)
{
    size_t count = m_events.size();

#ifdef SEQ64_USE_EVENT_MAP

    event_key key(e);
#if __cplusplus >= 201103L              /* C++11                    */
    EventsPair p = std::make_pair(key, e);
#else
    EventsPair p = std::make_pair<event_key, event>(key, e);
#endif
    m_events.insert(p);                 /* std::multimap operation  */

#else

    m_events.push_front(e);             /* std::list operation      */

#endif

    bool result = m_events.size() == (count + 1);
    if (result)
        m_is_modified = true;

    if (postsort)
        sort();                         /* by time-stamp and "rank" */

    return result;
}

#ifdef SEQ64_USE_EVENT_MAP

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
 *  just insertion.  And, since sorting isn't needed, the multimap actually
 *  turns out to be faster.
 *
 * \param el
 *      Provides the event list to be merged into the current event list.
 *
 * \param presort
 *      If true, the events are presorted.  This is a requirement for merging
 *      an std::list, but is a no-op for the std::multimap implementation.
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

#else   // SEQ64_USE_EVENT_MAP

void
event_list::merge (event_list & el, bool presort)
{
    if (presort)
        el.m_events.sort();

    m_events.merge(el.m_events);
}

#endif  // SEQ64_USE_EVENT_MAP

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
        if (eon.is_note_on() && ! eon.is_linked())  /* note on, unlinked?   */
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
 *
 * \param slength
 *      Provides the length beyond which events will be pruned.
 */

void
event_list::verify_and_link (midipulse slength)
{
    clear_links();
    for (event_list::iterator on = m_events.begin(); on != m_events.end(); on++)
    {
        event & eon = dref(on);
        if (eon.is_note_on())               /* Note On, find its Note Off   */
        {
            event_list::iterator off = on;  /* get next possible Note Off   */
            off++;
            bool endfound = false;
            while (off != m_events.end())
            {
                event & eoff = dref(off);
                if                          /* Off, == notes, not marked    */
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
 *  Marks all events.  Not yet used, but might come in handy with the event
 *  editor dialog.
 */

void
event_list::mark_all ()
{
    for (Events::iterator i = m_events.begin(); i != m_events.end(); ++i)
        dref(i).mark();
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
 *
 * \note
 *      This code was comparing the timestamp as greater than or equal to the
 *      sequence length.  However, being equal is fine.  This may explain why
 *      the midifile code would add one tick to the length of the last note
 *      when processing the end-of-track.
 *
 * \param slength
 *      Provides the length beyond which events will be pruned.
 */

void
event_list::mark_out_of_range (midipulse slength)
{
    for (Events::iterator i = m_events.begin(); i != m_events.end(); ++i)
    {
        event & e = dref(i);
        if (e.get_timestamp() > slength)
        {
            e.mark();                           /* we have to prune it  */
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
event_list::count_selected_notes () const
{
    int result = 0;
    for (Events::const_iterator i = m_events.begin(); i != m_events.end(); ++i)
    {
        if (dref(i).is_note_on() && dref(i).is_selected())
            ++result;
    }
    return result;
}

/**
 *  Indicates that at least one note is selected.  Acts like
 *  event_list::count_selected_notes(), but stops after finding a selected
 *  note. We could add a flag to count_selected_notes() to break, I suppose.
 */

bool
event_list::any_selected_notes () const
{
    bool result = false;
    for (Events::const_iterator i = m_events.begin(); i != m_events.end(); ++i)
    {
        if (dref(i).is_note_on() && dref(i).is_selected())
        {
            result = true;
            break;
        }
    }
    return result;
}

/**
 *  Counts the selected events, with the given status, in the event list.
 *  If the event is a control change (CC), then it must also match the
 *  given CC value.
 */

int
event_list::count_selected_events (midibyte status, midibyte cc) const
{
    int result = 0;
    for (Events::const_iterator i = m_events.begin(); i != m_events.end(); ++i)
    {
        const event & e = dref(i);
        if (e.get_status() == status)
        {
            midibyte d0, d1;
            e.get_data(d0, d1);                 /* get the two data bytes */
            if (event::is_desired_cc_or_not_cc(status, cc, d0))
            {
                if (e.is_selected())
                    ++result;
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
event_list::print () const
{
    printf("events[%ld]\n\n", m_events.size());
    for (Events::const_iterator i = m_events.begin(); i != m_events.end(); ++i)
        dref(i).print();

    printf("events[%d]\n\n", count());
}

}           // namespace seq64

/*
 * event_list.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

