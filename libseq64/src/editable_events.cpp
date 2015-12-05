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
 * \file          editable_events.cpp
 *
 *  This module declares/defines the base class for an ordered container of
 *  MIDI editable_events.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-12-04
 * \updates       2015-12-05
 * \license       GNU GPLv2 or above
 *
 *  A MIDI editable event is encapsulated by the seq64::editable_events
 *  object.
 */

#include "editable_events.hpp"          /* seq64::editable_events       */
#include "sequence.hpp"                 /* seq64::sequence              */

namespace seq64
{

/*
 * We will get the default controller name from the controllers module.
 * We should also be able to look up the selected buss's entries for a
 * sequence, and load up the CC/name pairs on the fly.
 */

/**
 *  This constructor hooks into the sequence object.
 *
 * \param seq
 *      Provides a reference to the sequence object, which provides the events
 *      and some of the MIDI timing parameters.
 *
 * \param bpm
 *      Provides the beats/minute value, which the caller figures out how to
 *      get and provides in this parameter.
 */

editable_events::editable_events (sequence & seq, int bpm)
 :
    m_events            (),
    m_current_event     (m_events.end()),
    m_sequence          (seq),
    m_midi_parameters
    (
        bpm, seq.get_beats_per_bar(), seq.get_beat_width(), seq.get_ppqn()
    )
{
    // Empty body
}

/**
 *  This copy constructor initializes most of the class members.
 *
 * \param rhs
 *      Provides the editable_events object to be copied.
 */

editable_events::editable_events (const editable_events & rhs)
 :
    m_events            (rhs.m_events),
    m_current_event     (rhs.m_current_event),
    m_sequence          (rhs.m_sequence),
    m_midi_parameters   (rhs.m_midi_parameters)
{
    // Empty body
}

/**
 *  This principal assignment operator sets most of the class members.
 *
 * \param rhs
 *      Provides the editable_events object to be assigned.
 *
 * \return
 *      Returns a reference to "this" object, to support the serial assignment
 *      of editable_eventss.
 */

editable_events &
editable_events::operator = (const editable_events & rhs)
{
    if (this != &rhs)
    {
        m_events            = rhs.m_events;
        m_current_event     = rhs.m_current_event;
        m_midi_parameters   = rhs.m_midi_parameters;
        m_sequence.partial_assign(rhs.m_sequence);
    }
    return *this;
}

/**
 *  Adds an event, converted to an editable_event, to the internal event list.
 *
 * \param e
 *      Provides the regular event to be added to the list of editable events.
 *
 * \return
 *      Returns true if the insertion succeeded, as evidenced by an increment
 *      in container size.
 */

bool
editable_events::add (const event & e)
{
    editable_event ed(*this, e);        /* make the event "editable"    */
    return add(ed);
}

/**
 *  Adds an editable event to the internal event list.
 *
 *  For the std::multimap implementation, This is an option if we want to make
 *  sure the insertion succeed.
 *
 *      std::pair<Events::iterator, bool> result = m_events.insert(p);
 *      return result.second;
 *
 * \param e
 *      Provides the regular event to be added to the list of editable events.
 *
 * \return
 *      Returns true if the insertion succeeded, as evidenced by an increment
 *      in container size.
 */

bool
editable_events::add (const editable_event & e)
{
    size_t count = m_events.size();         /* save initial size            */
    event_list::event_key key(e);            /* create the key value         */

#if __cplusplus >= 201103L                  /* C++11                        */
    EventsPair p = std::make_pair(key, e);
#else
    EventsPair p = std::make_pair<event_key, event>(key, e);
#endif
    m_events.insert(p);                     /* std::multimap operation      */

    bool result = m_events.size() == (count + 1);
    return result;
}

/**
 *  Accesses the sequence's event-list, iterating through it from beginning to
 *  end, wrapping each event in the list in an editable event and inserting it
 *  into the editable-event container.
 *
 * \return
 *      Returns true if the size of the final editable_event container matches
 *      the size of the original events container.
 */

bool
editable_events::load_events ()
{
    bool result;
    int original_count = m_sequence.events().count();
    for
    (
        event_list::const_iterator ei = m_sequence.events().begin();
        ei != m_sequence.events().end(); ++ei
    )
    {
        if (! add(ei->second))
            break;
    }
    result = count () == original_count;
    return result;
}

/**
 *  Erases the sequence's event container and recreates it using the edited
 *  container of editable events.
 *
 *  Note that the old events are replaced only if the container of editable
 *  events is not empty.  There are safer ways for the user to erase all the
 *  events.
 *
 * \return
 *      Returns true if the size of the final event container matches
 *      the size of the original editable_events container.
 */

bool
editable_events::save_events ()
{
    bool result = count() > 0;
    if (result)
    {
        m_sequence.events().clear();
        for (const_iterator ei = events().begin(); ei != events().end(); ++ei)
        {
            event ev = ei->second;
            if (! m_sequence.add_event(ev))
                break;
        }
        result = m_sequence.events().count () == count();
    }
    return result;
}

}           // namespace seq64

/*
 * editable_events.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

