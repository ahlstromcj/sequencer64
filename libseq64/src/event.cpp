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
 * \updates       2016-07-30
 * \license       GNU GPLv2 or above
 *
 *  A MIDI event (i.e. "track event") is encapsulated by the seq64::event
 *  object.
 *
 *      -   Varinum delta time stamp.
 *      -   Event byte:
 *          -   MIDI event.
 *              -   Channel event (0x80 to 0xE0, channel in low nybble).
 *                  -   Data byte 1.
 *                  -   Data byte 2 (for all but patch and channel pressure).
 *              -   Non-channel event (0xF0 to 0xFF).
 *                  -   SysEx (0xF0), discussed below, includes data bytes.
 *                  -   Song Position (0xF2) includes data bytes.
 *                  -   Song Select (0xF3) includes data bytes.
 *                  -   The rest of the non-channel events don't include data
 *                      byte.
 *          -   Meta event (0xFF).
 *              -   Meta type byte.
 *              -   Varinum length.
 *              -   Data bytes.
 *          -   SysEx event.
 *              -   Start byte (0xF0) or continuation/escape byte (0xF7).
 *              -   Varinum length???
 *              -   Data bytes (not yet fully supported in event class).
 *              -   End byte (0xF7).
 *
 *  Running status is used, where status bytes of MIDI channel messages can be
 *  omitted if the preceding event is a MIDI channel message with the same
 *  status.  Running status continues across delta-times.
 *
 *  In Seq24/Sequencer64, none of the non-channel events are stored in an
 *  event object.  There is some provisional support for storing SysEx, but
 *  none of the support functions are yet called.  In mastermidibus, there is
 *  support for recording SysEx data, but it is macro'ed out.  In rc_settings,
 *  there is an option for SysEx.  The midibus and perform objects also deal
 *  with Sysex.  But the midifile module does not read it -- it skips SysEx.
 *  Apparently, it does serve as a Thru for incoming SysEx, though.
 *  See this message thread:
 *
 *     http://sourceforge.net/p/seq24/mailman/message/1049609/
 *
 *  In Seq24/Sequencer64, the Meta events are handled directly, and they
 *  set up sequence parameters.  None of a meta event is stored in an event
 *  object.  If we want to add the ability to insert meta events, we will
 *  have to provide a metaevent class that can be stored in the event
 *  container.
 */

#include <string.h>                    /* memcpy()  */

#include "app_limits.h"
#include "easy_macros.h"
#include "event.hpp"

namespace seq64
{

/**
 *  This constructor simply initializes all of the class members.
 */

event::event ()
 :
    m_timestamp     (0),
    m_status        (EVENT_NOTE_OFF),
    m_channel       (EVENT_NULL_CHANNEL),
    m_data          (),                     /* a two-element array  */
#ifdef USE_STAZED_SYSEX_SUPPORT
    m_sysex         (),                     /* an std::vector       */
#else
    m_sysex         (nullptr),
#endif
    m_sysex_size    (0),
    m_linked        (nullptr),
    m_has_link      (false),
    m_selected      (false),
    m_marked        (false),
    m_painted       (false)
{
    m_data[0] = m_data[1] = 0;
}

/**
 *  This copy constructor initializes most of the class members.  This
 *  function is currently geared only toward support of the SMF 0
 *  channel-splitting feature.  Many of the members are not set to useful
 *  values when the MIDI file is read, so we don't handle them for now.
 *
 *  Note that now events are also copied when creating the editable_events
 *  container, so this function is even more important.  The event links, for
 *  linking Note Off events to their respective Note On events, are dropped.
 *  Generally, they will need to be reconstituted by calling the
 *  event_list::verify_and_link() function.
 *
 * \warning
 *      This function does not yet copy the SysEx data.  The inclusion
 *      of SysEx events was not complete in Seq24, and it is still not
 *      complete in Sequencer64.  Nor does it currently bother with the
 *      links, as noted above.
 *
 * \param rhs
 *      Provides the event object to be copied.
 */

event::event (const event & rhs)
 :
    m_timestamp     (rhs.m_timestamp),
    m_status        (rhs.m_status),
    m_channel       (rhs.m_channel),
    m_data          (),                     /* a two-element array      */
    m_sysex         (nullptr),              /* pointer, not yet handled */
    m_sysex_size    (rhs.m_sysex_size),
    m_linked        (nullptr),              /* pointer, not yet handled */
    m_has_link      (false),                /* must indicate that fact  */
    m_selected      (rhs.m_selected),
    m_marked        (rhs.m_marked),
    m_painted       (rhs.m_painted)
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
        m_timestamp     = rhs.m_timestamp;
        m_status        = rhs.m_status;
        m_channel       = rhs.m_channel;
        m_data[0]       = rhs.m_data[0];
        m_data[1]       = rhs.m_data[1];
        m_sysex         = nullptr;
        m_sysex_size    = rhs.m_sysex_size;         /* 0 instead?       */
        m_linked        = nullptr;
        m_has_link      = rhs.m_has_link;           /* false instead?   */
        m_selected      = rhs.m_selected;           /* false instead?   */
        m_marked        = rhs.m_marked;             /* false instead?   */
        m_painted       = rhs.m_painted;            /* false instead?   */
    }
    return *this;
}

/**
 *  This destructor explicitly deletes m_sysex and sets it to null.
 *  The restart_sysex() function does what we need.
 */

event::~event ()
{
    restart_sysex();                    /* tricky code */
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
 *      Because of this mis-feature, and the very slow speed of loading a MIDI
 *      file when Sequencer64 is built for debugging, we are exploring using
 *      an std::mulitmap instead of an std::list.  Search for occurrences of
 *      the SEQ64_USE_EVENT_MAP macro. (This actually works better than a
 *      list, for loading MIDI event, we have found, but may cause the upper
 *      limit of the number of playing sequences to drop a little, due to the
 *      overhead of incrementing multimap iterators versus list iterators).
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

#ifdef SEQ64_STAZED_TRANSPOSE

/**
 *  Transpose the note, if possible.
 *
 * \param tn
 *      The amount (positive or negative) to transpose a note.  If the result
 *      is out of range, the transposition is not performed.
 */

void
event::transpose_note (int tn)
{
    int note = int(m_data[0]) + tn;
    if (note >= 0 && note < SEQ64_MIDI_COUNT_MAX)
        m_data[0] = midibyte(note);
}

#endif

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
 *  Stazed:
 *
 *      The record parameter, if true, does not clear channel portion on record
 *      for channel specific recording. The channel portion is cleared in
 *      sequence::stream_event() by calling set_status() (a_record = false)
 *      after the matching channel is determined.  Otherwise, we use a bitwise
 *      AND to clear the channel portion of the status.  All events will be
 *      stored without the channel nybble.  This is necessary since the channel
 *      is appended by midibus::play() based on the track.
 *
 * \param status
 *      The status byte, perhaps read from a MIDI file or edited in the
 *      sequencer's event editor.  Sometime, this byte will have the channel
 *      nybble masked off.  If that is the case, the eventcode/channel
 *      overload of this function is more appropriate.
 *
 * \param record
 *      If true, we want to keep the channel byte.  Used in recording events on
 *      the channel associated with a given sequence.  The default value is
 *      false.  This is a "stazed" feature.
 */

void
event::set_status (midibyte status, bool record)
{
    if (record)
    {
        m_status = status;
    }
    else
    {
        if (status >= 0xF0)
        {
            m_status = status;
            m_channel = EVENT_NULL_CHANNEL;     /* i.e. "not applicable"    */
        }
        else
        {
            m_status = status & EVENT_CLEAR_CHAN_MASK;
            m_channel = status & EVENT_GET_CHAN_MASK;
        }
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
event::restart_sysex ()
{
#ifdef USE_STAZED_SYSEX_SUPPORT
    m_sysex.clear();
#else
    if (not_nullptr(m_sysex))
        delete [] m_sysex;

    m_sysex = nullptr;
#endif

    m_sysex_size = 0;
}

/**
 *  Appends SYSEX data to a new buffer.  First, a buffer of size
 *  m_sysex_size+dsize is created.  The existing SYSEX data (stored in
 *  m_sysex) is copied to this buffer.  Then the data represented by data and
 *  dsize is appended to that data buffer.  Then the original SYSEX buffer,
 *  m_sysex, is deleted, and m_sysex is assigned to the new buffer.
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

#ifdef USE_STAZED_SYSEX_SUPPORT

bool
event::append_sysex (midibyte * data, int dsize)
{
    bool result = true;
    for (int i = 0; i < dsize; ++i)
    {
        m_sysex.push_back(data[i]);
        if (data[i] == EVENT_SYSEX_END)
            result = false;
    }
    return result;
}

#else   // USE_STAZED_SYSEX_SUPPORT

bool
event::append_sysex (midibyte * data, int dsize)
{
    bool result = false;
    if (not_nullptr(data) && (dsize > 0))
    {
        midibyte * buffer = new midibyte[m_sysex_size + dsize];
        if (not_nullptr(buffer))
        {
            if (not_nullptr(m_sysex))
            {
                memcpy(buffer, m_sysex, m_sysex_size);  /* copy original data */
                delete [] m_sysex;
            }
            memcpy(&buffer[m_sysex_size], data, dsize); /* append new data    */
            m_sysex_size += dsize;
            m_sysex = buffer;                           /* save the pointer   */
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

#endif  // USE_STAZED_SYSEX_SUPPORT

/**
 *  Prints out the timestamp, data size, the current status byte, any SYSEX
 *  data if present, or the two data bytes for the status byte.
 */

void
event::print () const
{
    printf
    (
        "[%06ld] [%04d] %02X ",
        m_timestamp, m_sysex_size, unsigned(m_status)
    );
    if (m_status == EVENT_SYSEX)
    {
        for (int i = 0; i < m_sysex_size; i++)
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
 *
 * \param ev
 *      The event to put on show.
 *
 * \return
 *      Returns the string representation of the event parameter.
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

