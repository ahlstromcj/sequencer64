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
 *  This module declares/defines a class for handling MIDI events in a list
 *  container.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-09-19
 * \updates       2020-06-11
 * \license       GNU GPLv2 or above
 *
 *  This container now can indicate if certain Meta events (time-signaure or
 *  tempo) have been added to the container.
 *
 *  This module also defines the  event_list::event_key object.  Although the
 *  main MIDI container are now back to using std::list (with sorting after
 *  loading), the editable_events object is now back to using std::multimap,
 *  for easier management and automatic sorting of events.  See
 *  SEQ64_USE_EVENT_MAP.
 */

#include <stdio.h>                      /* C::printf()                  */

#include "easy_macros.h"
#include "event_list.hpp"

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{

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

/*
 * Section: event_list
 */

/**
 *  Principal constructor.
 */

event_list::event_list ()
 :
    m_events                (),
    m_is_modified           (false),
    m_has_tempo             (false),
    m_has_time_signature    (false)
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
    m_events                (rhs.m_events),
    m_is_modified           (rhs.m_is_modified),
    m_has_tempo             (rhs.m_has_tempo),
    m_has_time_signature    (rhs.m_has_time_signature)
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
        m_events                = rhs.m_events;
        m_is_modified           = rhs.m_is_modified;
        m_has_tempo             = rhs.m_has_tempo;
        m_has_time_signature    = rhs.m_has_time_signature;
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
 *  Provides the length of the events in MIDI pulses.  This function gets the
 *  iterator for the last element and returns its length value.
 *
 * \return
 *      Returns the timestamp of the latest event in the container.
 */

midipulse
event_list::get_length () const
{
    midipulse result = 0;
    if (count() > 0)
    {
        const_reverse_iterator lci = m_events.rbegin(); /* get last element */
#ifdef SEQ64_USE_EVENT_MAP
        result = lci->second.get_timestamp();           /* get length value */
#else
        result = lci->get_timestamp();                  /* get length value */
#endif
    }
    return result;
}

/**
 *  Adds an event to the internal event list without sorting.  It is a
 *  wrapper, wrapper for insert() or push_front(), with an option to call
 *  sort().
 *
 *  The add() function without sorting, useful to speed up the initial
 *  container loading into the event-list.
 *
 *  For the std::multimap implementation, This is an option if we want to make
 *  sure the insertion succeed.
 *
 *  If the std::list implementation has been built in, then the event list is
 *  not sorted after the addition.  This is a time-consuming operation.
 *
 *  We also have to raise some new flags if the event is a Set Tempo or
 *  Time Signature event, so that we do not force the current tempo and
 *  time-signature when writing the MIDI file.
 *
 * \warning
 *      This pushing (and, in writing the MIDI file, the popping),
 *      causes events with identical timestamps to be written in
 *      reverse order.  Doesn't affect functionality, but it's puzzling
 *      until one understands what is happening.  That's why we're
 *      now preferring to use a multimap as the container.
 *
 * \param e
 *      Provides the event to be added to the list.
 *
 * \return
 *      Returns true.  We assume the insertion succeeded, and no longer care
 *      about an increment in container size.  It's a multimap, so it always
 *      inserts, and if we don't have memory left, all bets are off anyway.
 */

bool
event_list::append (const event & e)
{
#ifdef SEQ64_USE_EVENT_MAP

    event_key key(e);

#ifdef PLATFORM_CPP_11
    EventsPair p = std::make_pair(key, e);
#else
    EventsPair p = std::make_pair<event_key, event>(key, e);
#endif

    m_events.insert(p);                 /* std::multimap operation  */

#else   // SEQ64_USE_EVENT_MAP

    m_events.push_front(e);             /* std::list operation      */

#endif

    m_is_modified = true;
    if (e.is_tempo())
        m_has_tempo = true;

    if (e.is_time_signature())
        m_has_time_signature = true;

    return true;
}

#ifdef SEQ64_USE_EVENT_MAP

/**
 *  Provides a merge operation for the event multimap analogous to the merge
 *  operation for the event list.  We have certain constraints to preserve, as
 *  the following discussion shows.
 *
 *  For std::list, sequence merges list T into list A by first calling
 *  T.sort(), and then A.merge(T).  The merge() operation merges T into A by
 *  transferring all of its elements, at their respective ordered positions,
 *  into A.  Both containers must already be ordered.
 *
 *  The merge effectively removes all the elements in T (which becomes empty),
 *  and inserts them into their ordered position within container (which
 *  expands in size by the number of elements transferred). The operation is
 *  performed without constructing nor destroying any element, whether T is an
 *  lvalue or an rvalue, or whether the value-type supports move-construction
 *  or not.
 *
 *  Each element of T is inserted at the position that corresponds to its
 *  value according to the strict weak ordering defined by operator <. The
 *  resulting order of equivalent elements is stable (i.e. equivalent elements
 *  preserve the relative order they had before the call, and existing
 *  elements precede those equivalent inserted from x).  The function does
 *  nothing if (&x == this).
 *
 *  For std::multimap, sorting is automatic.  However, unless
 *  move-construction is supported, merging will be less efficient than for
 *  the list version.  Also, we need a way to include duplicates of each
 *  event, so we need to use a multi-map.  Once all this setup, merging is
 *  really just insertion.  And, since sorting isn't needed, the multimap
 *  actually turns out to be faster.
 *
 * \param el
 *      Provides the event list to be merged into the current event list.
 *
 * \param presort
 *      If true, the events are presorted.  This is a requirement for merging
 *      an std::list, but is a no-op for the std::multimap implementation.
 */

void
event_list::merge (event_list & el, bool /*presort*/ )
{
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
        el.sort();                          // el.m_events.sort();

    m_events.merge(el.m_events);
}

#endif  // SEQ64_USE_EVENT_MAP

/**
 *  Links a new event.  This function checks for a note on, then looks for
 *  its note off.  This function is provided in the event_list because it
 *  does not depend on any external data.  Also note that any desired
 *  thread-safety must be provided by the caller.
 */

void
event_list::link_new ()
{
    for (Events::iterator on = m_events.begin(); on != m_events.end(); ++on)
    {
        event & eon = dref(on);
        if (eon.is_note_on() && ! eon.is_linked())  /* note on, unlinked?   */
        {
            Events::iterator off = on;              /* point to note on     */
            ++off;                                  /* get next element     */
#ifdef SEQ64_USE_STAZED_NEW_LINK_EXTENSION
            bool endfound = false;
#endif
            while (off != m_events.end())
            {
                event & eoff = dref(off);
                if                          /* Off, == notes, not linked    */
                (
                    eoff.is_note_off() &&
                    eoff.get_note() == eon.get_note() && ! eoff.is_linked()
                )
                {
                    eon.link(&eoff);                /* link backward        */
                    eoff.link(&eon);                /* link forward         */
#ifdef SEQ64_USE_STAZED_NEW_LINK_EXTENSION
                    endfound = true;                /* note on fulfilled    */
#endif
                    break;
                }
                ++off;
            }

#ifdef SEQ64_USE_STAZED_NEW_LINK_EXTENSION

            /*
             * This code, meant to allow wraparound of notes in a pattern, is
             * problematic.  Commenting it out for now.  A possible
             * alternative is to generated a Note Off event timestamped at the
             * end of the pattern.
             */

            if (! endfound)
            {
                off = m_events.begin();
                while (off != on)
                {
                    event & eoff = dref(off);
                    if              /* off event, == notes, and not linked  */
                    (
                        eoff.is_note_off() &&
                        eoff.get_note() == eon.get_note() && ! eoff.is_linked()
                    )
                    {
                        eon.link(&eoff);            /* link backward        */
                        eoff.link(&eon);            /* link forward         */
                        endfound = true;            /* note on fulfilled    */
                        break;
                    }
                    ++off;
                }
            }
#endif
        }
    }
}

/**
 *  This function verifies state: all note-ons have an off, and it links
 *  note-offs with their note-ons.
 *
 * Stazed (seq32):
 *
 *      This function now deletes any notes that are >= m_length, so any
 *      resize or move of notes must modify for wrapping if Note Off is >=
 *      m_length.
 *
 *  THINK ABOUT IT:  If we're in legacy merge mode for a loop, the Note Off is
 *  actually earlier than the Note On.  And in replace mode, the Note On is
 *  cleared, leaving us with a dangling Note Off event.  We should consider, in
 *  both modes, automatically adding the Note Off at the end of the loop and
 *  ignoring the next note off on the same note from the keyboard.  Careful!
 *
 * \threadunsafe
 *      As in most case, the caller will use an automutex to call this
 *      function safely.
 *
 * \param slength
 *      Provides the length beyond which events will be pruned.
 */

void
event_list::verify_and_link (midipulse slength)
{
    clear_links();
    for (event_list::iterator on = m_events.begin(); on != m_events.end(); ++on)
    {
        event & eon = dref(on);
        if (eon.is_note_on())               /* Note On, find its Note Off   */
        {
            event_list::iterator off = on;  /* next possible Note Off...    */
            ++off;                          /* ...starting here             */
            bool endfound = false;
            while (off != m_events.end())
            {
                event & eoff = dref(off);
                if                          /* Off, == notes, not linked    */
                (
                    eoff.is_note_off() &&
                    eoff.get_note() == eon.get_note() && ! eoff.is_linked()
                )
                {
                    /*
                     * See THINK ABOUT IT in the function banner.
                     */

                    eon.link(&eoff);                    /* link + mark */
                    eoff.link(&eon);
                    endfound = true;
                    break;
                }
                ++off;
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
                        eoff.get_note() == eon.get_note() && ! eoff.is_linked()
                    )
                    {
                        eon.link(&eoff);                /* link + mark */
                        eoff.link(&eon);
                        endfound = true;
                        break;
                    }
                    ++off;
                }
            }
        }
    }
    mark_out_of_range(slength);
    remove_marked();                        /* prune out-of-range events    */

    /*
     *  Link the tempos in a separate pass (it makes the logic easier and the
     *  amount of time should be unnoticeable to the user.
     */

    link_tempos();
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

#ifdef USE_FILL_TIME_SIG_AND_TEMPO

/**
 *  Scans the event-list for any tempo or time_signature events.
 *  The use may have deleted them and is depending on a setting made in the
 *  user-interface.  So we must set/unset the flags before saving.  This check
 *  was added to fix issue #141.
 */

void
event_list::scan_meta_events ()
{
    m_has_tempo = false;
    m_has_time_signature = false;
    for (Events::iterator i = m_events.begin(); i != m_events.end(); ++i)
    {
        event & e = dref(i);
        if (e.is_tempo())
            m_has_tempo = true;

        if (e.is_time_signature())
            m_has_time_signature = true;
    }
}

#endif  // USE_FILL_TIME_SIG_AND_TEMPO

/**
 *  This function tries to link tempo events.  Native support for temp tracks
 *  is a new feature of seq64.  These links are only in one direction: forward
 *  in time, to the next tempo event, if any.
 *
 *  Also, at present, tempo events are not markable.
 *
 * \threadunsafe
 *      As in most case, the caller will use an automutex to call this
 *      function safely.
 */

void
event_list::link_tempos ()
{
    clear_tempo_links();
    for (event_list::iterator t = m_events.begin(); t != m_events.end(); ++t)
    {
        event & e = dref(t);
        if (e.is_tempo())
        {
            event_list::iterator t2 = t;    /* next possible Set Tempo...   */
            ++t2;                           /* ...starting here             */
            while (t2 != m_events.end())
            {
                event & et2 = dref(t2);
                if (et2.is_tempo())
                {
                    e.link(&et2);                   /* link + mark          */
                    break;                          /* tempos link one way  */
                }
                ++t2;
            }
        }
    }
}

/**
 *  Clears all tempo event links.
 */

void
event_list::clear_tempo_links ()
{
    for (Events::iterator i = m_events.begin(); i != m_events.end(); ++i)
    {
        event & e = dref(i);
        if (e.is_tempo())
            e.clear_link();
    }
}

/**
 *  Marks all selected events.
 *
 * \return
 *      Returns true if there was even one event selected and marked.
 */

bool
event_list::mark_selected ()
{
    bool result = false;
    for (Events::iterator i = m_events.begin(); i != m_events.end(); ++i)
    {
        event & e = dref(i);
        if (e.is_selected())
        {
            e.mark();
            result = true;
        }
    }
    return result;
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
        bool prune = e.get_timestamp() >= slength;   /* WAS ">=", SEE BANNER */
        if (! prune)
            prune = e.get_timestamp() < 0;          /* added back, seq24    */

        if (prune)
        {
            e.mark();
            if (e.is_linked())
                e.get_linked()->mark();
        }
    }
}

/**
 *  Removes marked events.  Note how this function handles removing a
 *  value to avoid incrementing a now-invalid iterator.
 *
 * \threadsafe
 *
 * \return
 *      Returns true if at least one event was removed.
 */

bool
event_list::remove_marked ()
{
    bool result = false;
    Events::iterator i = m_events.begin();
    while (i != m_events.end())
    {
        if (DREF(i).is_marked())
        {
            event_list::iterator t = i;
            ++t;
            remove(i);
            i = t;
            result = true;
        }
        else
            ++i;
    }
    return result;
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
 *  note.
 *
 * \return
 *      Returns true if at least one note is selected.
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
 *  given CC value.  The one exception is tempo events, which are always
 *  selectable.
 *
 * \param status
 *      The desired status value to count.
 *
 * \param cc
 *      The desired control-change to count.  Used only if the status
 *      parameter indicates a control-change event.
 *
 * \return
 *      Returns the number of selected events.
 */

int
event_list::count_selected_events (midibyte status, midibyte cc) const
{
    int result = 0;
    for (Events::const_iterator i = m_events.begin(); i != m_events.end(); ++i)
    {
        const event & e = dref(i);
        if (e.is_tempo())
        {
            if (e.is_selected())
                ++result;
        }
        else if (e.get_status() == status)
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
 *  Indicates that at least one matching event is selected.  Acts like
 *  event_list::count_selected_events(), but stops after finding a selected
 *  note.
 *
 * \return
 *      Returns true if at least one matching event is selected.
 */

bool
event_list::any_selected_events (midibyte status, midibyte cc) const
{
    bool result = false;
    for (Events::const_iterator i = m_events.begin(); i != m_events.end(); ++i)
    {
        const event & e = dref(i);
        if (e.is_tempo())
        {
            if (e.is_selected())
            {
                result = true;
                break;
            }
        }
        else if (e.get_status() == status)
        {
            midibyte d0, d1;
            e.get_data(d0, d1);                 /* get the two data bytes */
            if (event::is_desired_cc_or_not_cc(status, cc, d0))
            {
                if (e.is_selected())
                {
                    result = true;
                    break;
                }
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
 *  Prints a list of the currently-held events.  Useful for debugging.
 */

void
event_list::print (const std::string & tag) const
{
    if (count() > 0)
    {
        printf("%d events %s:\n", count(), tag.c_str());
        for
        (
            Events::const_iterator i = m_events.begin();
            i != m_events.end(); ++i
        )
        {
            dref(i).print();
        }
    }
}

/**
 *  Prints a list of the currently-held events.  Useful for debugging.
 */

void
event_list::print_notes (const std::string & tag) const
{
    if (count() > 0)
    {
        printf("Notes %s:\n", tag.c_str());
        for
        (
            Events::const_iterator i = m_events.begin();
            i != m_events.end(); ++i
        )
        {
            dref(i).print_note();
        }
    }
}

}           // namespace seq64

/*
 * event_list.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

