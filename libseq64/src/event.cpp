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
 * \file          event.cpp
 *
 *  This module declares/defines the base class for MIDI events.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2015-09-20
 * \license       GNU GPLv2 or above
 *
 */

#include <string.h>                    // memcpy()

#include "easy_macros.h"
#include "event.hpp"

namespace seq64
{

/*
 * Section: event
 */

/**
 *  This constructor simply initializes all of the class members.
 */

event::event ()
 :
    m_timestamp (0),
    m_status    (EVENT_NOTE_OFF),
    m_data      (),                 // small array
    m_sysex     (nullptr),
    m_size      (0),
    m_linked    (nullptr),
    m_has_link  (false),
    m_selected  (false),
    m_marked    (false),
    m_painted   (false)
{
    m_data[0] = 0;
    m_data[1] = 0;
}

/**
 *  This destructor explicitly deletes m_sysex and sets it to null.
 */

event::~event ()
{
    if (not_nullptr(m_sysex))
        delete [] m_sysex;

    m_sysex = nullptr;
}

/**
 *  If the current timestamp equal the event's timestamp, then this
 *  function returns true if the current rank is less than the event's
 *  rank.
 *
 *  Otherwise, it returns true if the current timestamp is less than
 *  the event's timestamp.
 *
 * \warning
 *      The less-than operator is supposed to support a "strict weak
 *      ordering", and is supposed to leave equivalent values in the same
 *      order they were before the sort.  However, every time we load and
 *      save our sample MIDI file, events get reversed.  Here are
 *      program-changes that get reversed:
 *
\verbatim
        Save N:     0070: 6E 00 C4 48 00 C4 0C 00  C4 57 00 C4 19 00 C4 26
        Save N+1:   0070: 6E 00 C4 26 00 C4 19 00  C4 57 00 C4 0C 00 C4 48
\endverbatim
 *
 *      The 0070 is the offset within the versions of the
 *      b4uacuse-seq24.midi file.
 *
 *      Because of this mis-feature, and the very slow speed of loading a
 *      MIDI file when Sequencer64 is built for debugging, we are
 *      exploring using an std::map instead of an std::list.  Search for
 *      occurrences of the USE_EVENT_MAP macro. (This actually works better
 *      than a list, we have found).
 *
 * \param rhs
 *      The object to be compared against.
 *
 * \return
 *      Returns true if the time-stamp and "rank" are less than those of the
 *      comparison object.
 */

bool
event::operator < (const event & rhs) const
{
    if (m_timestamp == rhs.m_timestamp)
        return (get_rank() < rhs.get_rank());
    else
        return (m_timestamp < rhs.m_timestamp);
}

/**
 *  Sets the m_status member to the value of a_status.  If a_status is a
 *  non-channel event, then the channel portion of the status is cleared using
 *  a bitwise AND against EVENT_CLEAR_CHAN_MASK..
 */

void
event::set_status (char status)
{
    if ((unsigned char)(status) >= 0xF0)
        m_status = status;
    else
        m_status = char(status & EVENT_CLEAR_CHAN_MASK);
}

/**
 *  Sets m_status to EVENT_MIDI_CLOCK;
 */

void
event::make_clock ()
{
    m_status = (unsigned char)(EVENT_MIDI_CLOCK);
}

/**
 *  Clears the most-significant-bit of the d1 parameter, and sets it
 *  into the first byte of m_data.
 *
 * \param d1
 *      The byte value to set.  We should make these all "midibytes".
 */

void
event::set_data (char d1)
{
    m_data[0] = d1 & 0x7F;
}

/**
 *  Clears the most-significant-bit of both parameters, and sets them
 *  into the first and second bytes of m_data.
 *
 * \param d1
 *      The first byte value to set.  We should make these all "midibytes".
 *
 * \param d2
 *      The second byte value to set.  We should make these all "midibytes".
 */

void
event::set_data (char d1, char d2)
{
    m_data[0] = d1 & 0x7F;
    m_data[1] = d2 & 0x7F;
}

/**
 *  Increments the second data byte (m_data[1]) and clears the most
 *  significant bit.
 */

void
event::increment_data2 ()
{
    m_data[1] = (m_data[1] + 1) & 0x7F;
}

/**
 *  Decrements the second data byte (m_data[1]) and clears the most
 *  significant bit.
 */

void
event::decrement_data2 ()
{
    m_data[1] = (m_data[1] - 1) & 0x7F;
}

/**
 *  Increments the first data byte (m_data[1]) and clears the most
 *  significant bit.
 */

void
event::increment_data1 ()
{
    m_data[0] = (m_data[0] + 1) & 0x7F;
}

/**
 *  Decrements the first data byte (m_data[1]) and clears the most
 *  significant bit.
 */

void
event::decrement_data1 ()
{
    m_data[0] = (m_data[0] - 1) & 0x7F;
}

/**
 *  Retrieves the two data bytes from m_data[] and copies each into its
 *  respective parameter.
 *
 * \param d0 [out]
 *      The return reference for the first byte.
 *
 * \param d1 [out]
 *      The return reference for the first byte.
 */

void
event::get_data (unsigned char & d0, unsigned char & d1)
{
    d0 = m_data[0];
    d1 = m_data[1];
}

/**
 *  Deletes and clears out the SYSEX buffer.
 */

void
event::start_sysex ()
{
    if (not_nullptr(m_sysex))
        delete [] m_sysex;

    m_sysex = nullptr;
    m_size = 0;
}

/**
 *  Appends SYSEX data to a new buffer.  First, a buffer of size
 *  m_size+a_size is created.  The existing SYSEX data (stored in m_sysex)
 *  is copied to this buffer.  Then the data represented by a_data and
 *  a_size is appended to that data buffer.  Then the original SYSEX
 *  buffer, m_sysex, is deleted, and m_sysex is assigned to the new
 *  buffer..
 *
 * \warning
 *      This function does not check any pointers.
 *
 * \param a_data
 *      Provides the additional SYSEX data.
 *
 * \param a_size
 *      Provides the size of the additional SYSEX data.
 *
 * \return
 *      Returns false if there was an EVENT_SYSEX_END byte in the appended
 *      data.
 */

bool
event::append_sysex (unsigned char * a_data, long a_size)
{
    bool result = true;
    unsigned char * buffer = new unsigned char[m_size + a_size];
    memcpy(buffer, m_sysex, m_size);                // copy old and append
    memcpy(&buffer[m_size], a_data, a_size);
    delete [] m_sysex;
    m_size = m_size + a_size;
    m_sysex = buffer;
    for (int i = 0; i < a_size; i++)
    {
        if (a_data[i] == EVENT_SYSEX_END)
        {
            result = false;
            break;                                  // done, already false now
        }
    }
    return result;
}

/**
 *  Prints out the timestamp, data size, the current status byte, any SYSEX
 *  data if present, or the two data bytes for the status byte.
 */

void
event::print ()
{
    printf("[%06ld] [%04lX] %02X ", m_timestamp, m_size, m_status);
    if (m_status == EVENT_SYSEX)
    {
        for (int i = 0; i < m_size; i++)
        {
            if (i % 16 == 0)
                printf("\n    ");

            printf("%02X ", m_sysex[i]);
        }
        printf("\n");
    }
    else
    {
        printf("%02X %02X\n", m_data[0], m_data[1]);
    }
}

/**
 *  The ranking, from high to low, is note off, note on, aftertouch, channel
 *  pressure, and pitch wheel, control change, and program changes.  The lower
 *  the ranking the more upfront an item comes in the sort order.
 *
 * \return
 *      Returns the rank of the current m_status byte.
 */

int
event::get_rank () const
{
    switch (m_status)
    {
    case EVENT_NOTE_OFF:
        return 0x100;

    case EVENT_NOTE_ON:
        return 0x090;

    case EVENT_AFTERTOUCH:
    case EVENT_CHANNEL_PRESSURE:
    case EVENT_PITCH_WHEEL:
        return 0x050;

    case EVENT_CONTROL_CHANGE:
        return 0x010;

    case EVENT_PROGRAM_CHANGE:
        return 0x000;

    default:
        return 0;
    }
}

}           // namespace seq64

/*
 * event.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
