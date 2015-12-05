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
 *  Formats the current timestamp member as a string.  The format of the
 *  string representation is of the format selected by the m_format_timestamp
 *  member.
 *
 *      std::string pulses_to_measurestring (midipulse, const midi_timing &)
 *
 *      bool pulses_to_midi_measures
 *      (
 *          midipulse, const midi_timing &, midi_measures &
 *      );

void
editable_events::format_timestamp ()
{
    if (m_format_timestamp == timestamp_measures)
    {
    }
    else if (m_format_timestamp == timestamp_time)
    {
    }
    else if (m_format_timestamp == timestamp_pulses)
    {
        char tmp[32];
        snprintf(tmp, sizeof tmp, "%lu", (unsigned long)(get_timestamp()));
        m_name_timestamp = std::string(tmp);
    }
}
 */

}           // namespace seq64

/*
 * editable_events.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

