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
 * \updates       2015-11-27
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
    m_channel   (EVENT_NULL_CHANNEL),
    m_data      (),                     /* a two-element array  */
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
 *  This copy constructor initializes most of the class members.
 *  This function is currently geared only toward support of the SMF 0
 *  channel-splitting feature.  Many of the member are not set to useful value
 *  when the MIDI file is read, so we don't handle them for now.
 *
 * \warning
 *      This function does not yet copy the SysEx data.  The inclusion
 *      of SysEx events was not complete in Seq24, and it is still not
 *      complete in Sequencer64.  Nor does it currently bother with the
 *      links.
 *
 * \param rhs
 *      Provides the event object to be copied.
 */

event::event (const event & rhs)
 :
    m_timestamp (rhs.m_timestamp),
    m_status    (rhs.m_status),
    m_channel   (rhs.m_channel),
    m_data      (),                     /* a two-element array      */
    m_sysex     (nullptr),              /* pointer, not yet handled */
    m_size      (rhs.m_size),
    m_linked    (nullptr),              /* pointer, not yet handled */
    m_has_link  (rhs.m_has_link),
    m_selected  (rhs.m_selected),
    m_marked    (rhs.m_marked),
    m_painted   (rhs.m_painted)
{
    m_data[0] = rhs.m_data[0];
    m_data[1] = rhs.m_data[1];
}

/**
 *  This principal assignment operator sets most of the class members.  This
 *  function is currently geared only toward support of the SMF 0
 *  channel-splitting feature.  Many of the member are not set to useful value
 *  when the MIDI file is read, so we don't handle them for now.
 *
 * \warning
 *      This function does not yet copy the SysEx data.  The inclusion
 *      of SysEx events was not complete in Seq24, and it is still not
 *      complete in Sequencer64.  Nor does it currently bother with the
 *      links.
 *
 * \param rhs
 *      Provides the event object to be assigned.
 *
 * \return
 *      Returns a reference to "this" object, to support the serial assignment
 *      of events.
 */

event &
event::operator = (const event & rhs)
{
    if (this != &rhs)
    {
        m_timestamp = rhs.m_timestamp;
        m_status    = rhs.m_status;
        m_channel   = rhs.m_channel;
        m_data[0]   = rhs.m_data[0];
        m_data[1]   = rhs.m_data[1];
        m_sysex     = nullptr;
        m_size      = rhs.m_size;
        m_linked    = nullptr;
        m_has_link  = rhs.m_has_link;
        m_selected  = rhs.m_selected;
        m_marked    = rhs.m_marked;
        m_painted   = rhs.m_painted;
    }
    return *this;
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
 *  Sets the m_status member to the value of status.  If a_status is a
 *  channel event, then the channel portion of the status is cleared using
 *  a bitwise AND against EVENT_CLEAR_CHAN_MASK.
 *
 *  Found in yet another fork of seq24:
 *
 *      // ORL fait de la merde
 *
 *  He also provided a very similar routine: set_status_midibus().
 *
 * \param status
 *      The status byte, perhaps read from a MIDI file or edited in the
 *      sequencer's event editor.  Sometime, this byte will have the channel
 *      nybble masked off.  If that is the case, the eventcode/channel
 *      overload of this function is more appropriate.
 */

void
event::set_status (midibyte status)
{
    if (status >= 0xF0)
    {
        m_status = status;
        m_channel = EVENT_NULL_CHANNEL;         /* i.e. "not applicable"    */
    }
    else
    {
        m_status = status & EVENT_CLEAR_CHAN_MASK;
        m_channel = status & EVENT_GET_CHAN_MASK;
    }
}

/**
 *  This overload is useful when synthesizing events, such as converting a
 *  Note On event with a velocity of zero to a Note Off event.
 *
 * \param eventcode
 *      The status byte, perhaps read from a MIDI file.  This byte is
 *      assumed to have already had its low nybble cleared by masking against
 *      EVENT_CLEAR_CHAN_MASK.
 *
 * \param channel
 *      The channel byte.  Combined with the event-code, this makes a valid
 *      MIDI "status" byte.  This byte is assume to have already had its high
 *      nybble cleared by masking against EVENT_GET_CHAN_MASK.
 */

void
event::set_status (midibyte eventcode, midibyte channel)
{
    m_status = eventcode;               /* already masked against 0xF0      */
    m_channel = channel;                /* already masked against 0x0F      */
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
event::append_sysex (midibyte * data, long dsize)
{
    bool result = false;
    if (not_nullptr(data) && (dsize > 0))
    {
        midibyte * buffer = new midibyte[m_size + dsize];
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

/**
 *  A free function to convert an event into an informative string, just
 *  enough to save some debugging time.  Nothing fancy.  If you want that, use
 *  the midicvt project.
 */

std::string
to_string (const event & ev)
{
    std::string result("event: ");
    midibyte d0, d1;
    ev.get_data(d0, d1);
    char temp[128];
    snprintf
    (
        temp, sizeof temp,
        "[%04lu] status = 0x%02X; channel = 0x%02X; data = [0x%02X, 0x%02X]\n",
        ev.get_timestamp(), ev.get_status(), ev.get_channel(), d0, d1
    );
    result += std::string(temp);
    return result;
}

}           // namespace seq64

/*
 * event.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

