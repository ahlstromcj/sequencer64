#ifndef SEQ64_EVENT_HPP
#define SEQ64_EVENT_HPP

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
 * \file          event.hpp
 *
 *  This module declares/defines the event class for operating with
 *  MIDI events.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2016-02-07
 * \license       GNU GPLv2 or above
 *
 *  This module also declares/defines the various constants, status-byte
 *  values, or data values for MIDI events.
 *
 *  This class is now a base class, so that we can manage "editable events".
 */

#include <string>                       /* used in to_string()          */

#include "midibyte.hpp"                 /* seq64::midibyte typedef      */

/**
 *  Defines the number of data bytes in MIDI status data.
 */

#define SEQ64_MIDI_DATA_BYTE_COUNT      2

namespace seq64
{

/**
 *  This highest bit of the status byte is always 1.
 */

const midibyte EVENT_STATUS_BIT         = 0x80;

/**
 *  Channel Voice Messages.
 *
 *  The following MIDI events are channel messages.  The comments represent
 *  the one or two data-bytes of the message.
 *
 *  Note that Channel Mode Messages use the same code as the Control Change,
 *  but uses reserved controller numbers ranging from 122 to 127.
 */

const midibyte EVENT_NOTE_OFF           = 0x80;      // 0kkkkkkk 0vvvvvvv
const midibyte EVENT_NOTE_ON            = 0x90;      // 0kkkkkkk 0vvvvvvv
const midibyte EVENT_AFTERTOUCH         = 0xA0;      // 0kkkkkkk 0vvvvvvv
const midibyte EVENT_CONTROL_CHANGE     = 0xB0;      // 0ccccccc 0vvvvvvv
const midibyte EVENT_PROGRAM_CHANGE     = 0xC0;      // 0ppppppp
const midibyte EVENT_CHANNEL_PRESSURE   = 0xD0;      // 0vvvvvvv
const midibyte EVENT_PITCH_WHEEL        = 0xE0;      // 0lllllll 0mmmmmmm

/**
 *  System Messages.
 *
 *  The following MIDI events have no channel.  We have included redundant
 *  constant variables for the SysEx Start and End bytes just to make it
 *  clear that they are part of this sequence of values, though usually
 *  treated separately.
 *
 *  Only the following constants are followed by some data bytes:
 *
 *      -   EVENT_MIDI_SYSEX            = 0xF0
 *      -   EVENT_MIDI_QUARTER_FRAME    = 0xF1      // undefined?
 *      -   EVENT_MIDI_SONG_POS         = 0xF2
 *      -   EVENT_MIDI_SONG_SELECT      = 0xF3
 */

const midibyte EVENT_MIDI_SYSEX         = 0xF0;      // redundant, see below
const midibyte EVENT_MIDI_QUARTER_FRAME = 0xF1;      // undefined?
const midibyte EVENT_MIDI_SONG_POS      = 0xF2;
const midibyte EVENT_MIDI_SONG_SELECT   = 0xF3;      // not used
const midibyte EVENT_MIDI_SONG_F4       = 0xF4;      // undefined
const midibyte EVENT_MIDI_SONG_F5       = 0xF5;      // undefined
const midibyte EVENT_MIDI_TUNE_SELECT   = 0xF6;      // not used in seq24
const midibyte EVENT_MIDI_SYSEX_END     = 0xF7;      // redundant, see below
const midibyte EVENT_MIDI_CLOCK         = 0xF8;
const midibyte EVENT_MIDI_SONG_F9       = 0xF9;      // undefined
const midibyte EVENT_MIDI_START         = 0xFA;
const midibyte EVENT_MIDI_CONTINUE      = 0xFB;
const midibyte EVENT_MIDI_STOP          = 0xFC;
const midibyte EVENT_MIDI_SONG_FD       = 0xFD;      // undefined
const midibyte EVENT_MIDI_ACTIVE_SENS   = 0xFE;      // not used in seq24
const midibyte EVENT_MIDI_RESET         = 0xFF;      // not used in seq24

/**
 *  0xFF is a MIDI "escape code" used in MIDI files to introduce a MIDI meta
 *  event.
 */

const midibyte EVENT_MIDI_META         = 0xFF;      // an escape code

/**
 *  A MIDI System Exclusive (SYSEX) message starts with F0, followed
 *  by the manufacturer ID (how many? bytes), a number of data bytes, and
 *  ended by an F7.
 */

const midibyte EVENT_SYSEX             = 0xF0;
const midibyte EVENT_SYSEX_END         = 0xF7;
const midibyte EVENT_SYSEX_CONTINUE    = 0xF7;

/**
 *  This value of 0xFF is Sequencer64's channel value that indicates that
 *  the event's m_channel value is bogus.  However, it also means that the
 *  channel is encoded in the m_status byte itself.  This is our work around
 *  to be able to hold a multi-channel SMF 0 track in a sequence.  In a
 *  Sequencer64 SMF 0 track, every event has a channel.  In a Sequencer64 SMF
 *  1 track, the events do not have a channel.  Instead, the channel is a
 *  global value of the sequence, and is stuffed into each event when the
 *  event is played or is written to a MIDI file.
 */

const midibyte EVENT_NULL_CHANNEL      = 0xFF;

/**
 *  These file masks are used to obtain or to mask off the channel data from a
 *  status byte.
 */

const midibyte EVENT_GET_CHAN_MASK     = 0x0F;
const midibyte EVENT_CLEAR_CHAN_MASK   = 0xF0;

/**
 *  Provides events for management of MIDI events.
 *
 *  A MIDI event consists of 3 bytes:
 *
 *      -#  Status byte, 1sssnnn, where the sss bits specify the type of
 *          message, and the nnnn bits denote the channel number.
 *          The status byte always starts with 0.
 *      -#  The first data byte, 0xxxxxxx, where the data byte always
 *          start with 0, and the xxxxxxx values range from 0 to 127.
 *      -#  The second data byte, 0xxxxxxx.
 *
 *  This class may have too many member functions.
 */

class event
{

private:

    /**
     *  Provides the MIDI timestamp in ticks, otherwise known as the "pulses"
     *  in "pulses per quarter note" (PPQN).
     */

    midipulse m_timestamp;

    /**
     *  This is the status byte without the channel.  The channel will be
     *  appended on the MIDI bus.  The high nibble = type of event; The
     *  low nibble = channel.  Bit 7 is present in all status bytes.
     */

    midibyte m_status;

    /**
     *  In order to be able to handle MIDI channel-splitting of an SMF 0 file,
     *  we need to store the channel, even if we override it when playing the
     *  MIDI data.  This member adds another 4 bytes to the event object, most
     *  likely.
     */

    midibyte m_channel;

    /**
     *  The two bytes of data for the MIDI event.  Remember that the
     *  most-significant bit of a data byte is always 0.
     */

    midibyte m_data[SEQ64_MIDI_DATA_BYTE_COUNT];

    /**
     *  Points to the data buffer for SYSEX messages.
     *  This really ought to be a Boost or STD scoped pointer.  Currently, it
     *  doesn't seem to be used.
     */

    midibyte * m_sysex;

    /**
     *  Gives the size of the SYSEX message.
     */

    int m_sysex_size;

    /**
     *  This event is used to link Note Ons and Offs together.
     */

    event * m_linked;

    /**
     *  Indicates that a link has been made.  This item is used [via
     *  the get_link() and link() accessors] in the sequence class.
     */

    bool m_has_link;

    /**
     *  Answers the question "is this event selected in editing."
     */

    bool m_selected;

    /**
     *  Answers the question "is this event marked in processing."
     */

    bool m_marked;

    /**
     *  Answers the question "is this event being painted."
     */

    bool m_painted;

public:

    event ();
    event (const event & rhs);
    event & operator = (const event & rhs);
    virtual ~event ();

    /*
     * Operator overload, the only one needed for sorting events in a list
     * or a map.
     */

    bool operator < (const event & rhsevent) const;

    /**
     * \setter m_timestamp
     */

    void set_timestamp (midipulse time)
    {
        m_timestamp = time;
    }

    /**
     * \getter m_timestamp
     */

    midipulse get_timestamp () const
    {
        return m_timestamp;
    }

    /**
     * \getter m_channel
     */

    midibyte get_channel () const
    {
        return m_channel;
    }

    /**
     *  Checks the channel number to see if the event's channel matches it, or
     *  if the event has no channel.  Used in the SMF 0 track-splitting code.
     */

    bool check_channel (int channel) const
    {
        return m_channel == EVENT_NULL_CHANNEL || midibyte(channel) == m_channel;
    }

    /**
     *  Static test for channel messages/statuses.  This function requires that
     *  the channel data have already been masked off.
     *
     * \param m
     *      The channel status or message byte to be tested.
     *
     * \return
     *      Returns true if the byte represents a MIDI channel message.
     */

    static bool is_channel_msg (midibyte m)
    {
        return
        (
            m == EVENT_NOTE_ON        || m == EVENT_NOTE_OFF ||
            m == EVENT_AFTERTOUCH     || m == EVENT_CONTROL_CHANGE ||
            m == EVENT_PROGRAM_CHANGE || m == EVENT_CHANNEL_PRESSURE ||
            m == EVENT_PITCH_WHEEL
        );
    }

    /**
     *  Static test for channel messages that have only one data byte.
     *  The rest have two.
     *
     * \param m
     *      The channel status or message byte to be tested.
     *
     * \return
     *      Returns true if the byte represents a MIDI channel message that
     *      has only one data byte.  However, if this function returns false,
     *      it might not be a channel message at all, so be careful.
     */

    static bool is_one_byte_msg (midibyte m)
    {
        return m == EVENT_PROGRAM_CHANGE || m == EVENT_CHANNEL_PRESSURE;
    }

    /**
     *  Static test for channel messages that have two data bytes.
     *
     * \param m
     *      The channel status or message byte to be tested.
     *
     * \return
     *      Returns true if the byte represents a MIDI channel message that
     *      has two data bytes.  However, if this function returns false,
     *      it might not be a channel message at all, so be careful.
     */

    static bool is_two_byte_msg (midibyte m)
    {
        return
        (
            m == EVENT_NOTE_ON        || m == EVENT_NOTE_OFF   ||
            m == EVENT_CONTROL_CHANGE || m == EVENT_AFTERTOUCH ||
            m == EVENT_PITCH_WHEEL
        );
    }

    /**
     *  Static test for messages that involve notes and velocity.
     *
     * \param m
     *      The channel status or message byte to be tested.
     *
     * \return
     *      Returns true if the byte represents a MIDI note message.
     */

    static bool is_note_msg (midibyte m)
    {
        return
        (
            m == EVENT_NOTE_ON || m == EVENT_NOTE_OFF || m == EVENT_AFTERTOUCH
        );
    }

    /**
     *  Static test for channel messages that are either not control-change
     *  messages, or are and match the given controller value.
     *
     * \note
     *      The old logic was the first line, but can be simplified to the
     *      second line; the third line shows the abstract representation.
     *      Also made sure of this using a couple truth tables.
     *
\verbatim
        (m != EVENT_CONTROL_CHANGE) || (m == EVENT_CONTROL_CHANGE && d == cc)
        (m != EVENT_CONTROL_CHANGE) || (d == cc)
        a || (! a && b)  =>  a || b
\endverbatim
     *
     * \param m
     *      The channel status or message byte to be tested.
     *
     * \param cc
     *      The desired cc value, which the datum must match, if the message is
     *      a control-change message.
     *
     * \param datum
     *      The current datum, to be compared to cc, if the message is a
     *      control-change message.
     *
     * \return
     *      Returns true if the message is not a control-change, or if it is
     *      and the cc and datum parameters match.
     */

    static inline bool is_desired_cc_or_not_cc
    (
        midibyte m, midibyte cc, midibyte datum
    )
    {
        return (m != EVENT_CONTROL_CHANGE) || (datum == cc);
    }

    /**
     *  Calculates the value of the current timestamp modulo the given
     *  parameter.
     *
     * \param a_mod
     *      The value to mod the timestamp against.
     *
     * \return
     *      Returns a value ranging from 0 to a_mod-1.
     */

    void mod_timestamp (midipulse a_mod)
    {
        m_timestamp %= a_mod;
    }

    void set_status (midibyte status);
    void set_status (midibyte eventcode, midibyte channel);

    /**
     *  Sets the channel "nybble", without modifying the status "nybble".
     *  Note that the sequence channel generally overrides this value.
     *
     * \param channel
     *      The channel byte.
     */

    void set_channel (midibyte channel)
    {
        m_channel = channel;
    }

    /**
     * \getter m_status
     */

    midibyte get_status () const
    {
        return m_status;
    }

    /**
     *  Clears the most-significant-bit of the d1 parameter, and sets it into
     *  the first byte of m_data.
     *
     * \param d1
     *      The byte value to set.  We should make these all "midibytes".
     */

    void set_data (midibyte d1)
    {
        m_data[0] = d1 & 0x7F;
        m_data[1] = 0;                  /* not strictly necessary   */
    }

    /**
     *  Clears the most-significant-bit of both parameters, and sets them into
     *  the first and second bytes of m_data.
     *
     * \param d1
     *      The first byte value to set.
     *
     * \param d2
     *      The second byte value to set.
     */

    void set_data (midibyte d1, midibyte d2)
    {
        m_data[0] = d1 & 0x7F;
        m_data[1] = d2 & 0x7F;
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

    void get_data (midibyte & d0, midibyte & d1) const
    {
        d0 = m_data[0];
        d1 = m_data[1];
    }

    /**
     *  Increments the first data byte (m_data[1]) and clears the most
     *  significant bit.
     */

    void increment_data1 ()
    {
        m_data[0] = (m_data[0] + 1) & 0x7F;
    }

    /**
     *  Decrements the first data byte (m_data[1]) and clears the most
     *  significant bit.
     */

    void decrement_data1 ()
    {
        m_data[0] = (m_data[0] - 1) & 0x7F;
    }

    /**
     *  Increments the second data byte (m_data[1]) and clears the most
     *  significant bit.
     */

    void increment_data2 ()
    {
        m_data[1] = (m_data[1] + 1) & 0x7F;
    }

    /**
     *  Decrements the second data byte (m_data[1]) and clears the most
     *  significant bit.
     */

    void decrement_data2 ()
    {
        m_data[1] = (m_data[1] - 1) & 0x7F;
    }

    void restart_sysex ();
    bool append_sysex (midibyte * data, int len);

    /**
     * \getter m_sysex
     */

    midibyte * get_sysex () const
    {
        return m_sysex;
    }

    /**
     * \setter m_sysex_size
     */

    void set_sysex_size (int len)
    {
        m_sysex_size = len;
    }

    /**
     * \getter m_sysex_size
     */

    int get_sysex_size () const
    {
        return m_sysex_size;
    }

    /**
     *  Sets m_has_link and sets m_link to the provided event pointer.
     */

    void link (event * a_event)
    {
        m_has_link = true;
        m_linked = a_event;
    }

    /**
     * \getter m_linked
     */

    event * get_linked () const
    {
        return m_linked;
    }

    /**
     * \getter m_has_link
     */

    bool is_linked () const
    {
        return m_has_link;
    }

    /**
     * \setter m_has_link
     */

    void clear_link ()
    {
        m_has_link = false;
    }

    /**
     * \setter m_painted
     */

    void paint ()
    {
        m_painted = true;
    }

    /**
     * \setter m_painted
     */

    void unpaint ()
    {
        m_painted = false;
    }

    /**
     * \getter m_painted
     */

    bool is_painted () const
    {
        return m_painted;
    }

    /**
     * \setter m_marked
     */

    void mark ()
    {
        m_marked = true;
    }

    /**
     * \setter m_marked
     */

    void unmark ()
    {
        m_marked = false;
    }

    /**
     * \getter m_marked
     */

    bool is_marked () const
    {
        return m_marked;
    }

    /**
     * \setter m_selected
     */

    void select ()
    {
        m_selected = true;
    }

    /**
     * \setter m_selected
     */

    void unselect ()
    {
        m_selected = false;
    }

    /**
     * \getter m_selected
     */

    bool is_selected () const
    {
        return m_selected;
    }

    /**
     *  Sets m_status to EVENT_MIDI_CLOCK;
     */

    void make_clock ()
    {
        m_status = EVENT_MIDI_CLOCK;
    }

    /**
     * \getter m_data[]
     */

    midibyte data (int index) const    /* index not checked, for speed */
    {
        return m_data[index];
    }

    /**
     *  Assuming m_data[] holds a note, get the note number, which is in the
     *  first data byte, m_data[0].
     */

    midibyte get_note () const
    {
        return m_data[0];
    }

    /**
     *  Sets the note number, clearing off the most-significant-bit and
     *  assigning it to the first data byte, m_data[0].
     */

    void set_note (midibyte note)
    {
        m_data[0] = note & 0x7F;
    }

    /**
     * \getter m_data[1], the note velocity.
     */

    midibyte get_note_velocity () const
    {
        return m_data[1];
    }

    /**
     *  Sets the note velocity, with is held in the second data byte,
     *  m_data[1].
     */

    void set_note_velocity (int a_vel)
    {
        m_data[1] = a_vel & 0x7F;
    }

    /**
     *  Returns true if m_status is EVENT_NOTE_ON.
     */

    bool is_note_on () const
    {
        return m_status == EVENT_NOTE_ON;
    }

    /**
     *  Returns true if m_status is EVENT_NOTE_OFF.
     */

    bool is_note_off () const
    {
        return m_status == EVENT_NOTE_OFF;
    }

    void print ();

    /**
     *  This function is used in sorting MIDI status events (e.g. note
     *  on/off, aftertouch, control change, etc.)  The sort order is not
     *  determined by the actual status values.
     */

    int get_rank () const;

};          // class event

/*
 * Global functions in the seq64 namespace.
 */

extern std::string to_string (const event & ev);

}           // namespace seq64

#endif      // SEQ64_EVENT_HPP

/*
 * event.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

