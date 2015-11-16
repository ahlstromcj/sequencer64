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
 * \updates       2015-11-05
 * \license       GNU GPLv2 or above
 *
 */

#ifdef  USE_SEQ42_PATCHES
#include <fstream>
#endif

#include <string.h>                    /* memcpy()  */

#include "easy_macros.h"
#include "event.hpp"

namespace seq64
{

/**
 *  This constructor simply initializes all of the class members.
 */

event::event ()
 :
    m_timestamp (0),
    m_status    (EVENT_NOTE_OFF),
    m_data      (),                 /* a two-element array  */
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
 *  The start_sysex() function does what we need.
 */

event::~event ()
{
    start_sysex();                      /* tricky code */
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
 *      occurrences of the SEQ64_USE_EVENT_MAP macro. (This actually works
 *      better than a list, for loading MIDI event, we have found).
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
        return get_rank() < rhs.get_rank();
    else
        return m_timestamp < rhs.m_timestamp;
}

/**
 *  Sets the m_status member to the value of a_status.  If a_status is a
 *  non-channel event, then the channel portion of the status is cleared using
 *  a bitwise AND against EVENT_CLEAR_CHAN_MASK.
 *
 *  Is this a better way to do it?
 *
 *      m_status = (unsigned char)(status) & EVENT_CLEAR_CHAN_MASK;
 *
 *  Found in yet another fork of seq24:
 *
 *      // ORL fait de la merde
 *
 *  He also provided a very similar routine: set_status_midibus().
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
 *  Appends SYSEX data to a new buffer.  First, a buffer of size m_size+dsize
 *  is created.  The existing SYSEX data (stored in m_sysex) is copied to this
 *  buffer.  Then the data represented by data and dsize is appended to that
 *  data buffer.  Then the original SYSEX buffer, m_sysex, is deleted, and
 *  m_sysex is assigned to the new buffer.
 *
 * \param data
 *      Provides the additional SYSEX data.  If not provided, nothing is done,
 *      and false is returned.
 *
 * \param dsize
 *      Provides the size of the additional SYSEX data.  If not provided,
 *      nothing is done.
 *
 * \return
 *      Returns false if there was an EVENT_SYSEX_END byte in the appended
 *      data, or if an error occurred, and the caller needs to stop trying to
 *      process the data.
 */

bool
event::append_sysex (unsigned char * data, long dsize)
{
    bool result = false;
    if (not_nullptr(data) && (dsize > 0))
    {
        unsigned char * buffer = new unsigned char[m_size + dsize];
        if (not_nullptr(buffer))
        {
            if (not_nullptr(m_sysex))
            {
                memcpy(buffer, m_sysex, m_size);    /* copy original data   */
                delete [] m_sysex;
            }
            memcpy(&buffer[m_size], data, dsize);   /* append the new data  */
            m_size += dsize;
            m_sysex = buffer;                       /* save the pointer     */
            result = true;
        }
        else
        {
            errprint("event::append_sysex(): bad allocation");
        }
        for (int i = 0; i < dsize; ++i)
        {
            if (data[i] == EVENT_SYSEX_END)
            {
                result = false;
                break;                                  // done, already false now
            }
        }
    }
    else
    {
        errprint("event::append_sysex(): null parameters");
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

