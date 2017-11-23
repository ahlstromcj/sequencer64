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
 * \updates       2017-11-04
 * \license       GNU GPLv2 or above
 *
 *  This module also declares/defines the various constants, status-byte
 *  values, or data values for MIDI events.  This class is also a base class,
 *  so that we can manage "editable events".
 *
 *  Note the new inline free function is_note_off_velocity().
 *
 *  One thing we need to add to this event class is a way to encapsulate
 *  Meta events.  First, we use the existing event::SysexContainer to hold
 *  this data.
 */

#include <string>                       /* used in to_string()          */
#include <vector>                       /* SYSEX data stored in vector  */

#include "midibyte.hpp"                 /* seq64::midibyte typedef      */
#include "seq64_features.h"             /* feature macros               */

/**
 *  Defines the number of data bytes in MIDI status data.
 */

#define SEQ64_MIDI_DATA_BYTE_COUNT      2

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

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
 *
 *  The EVENT_ANY (0x00) value may prove to be useful in allowing any event to
 *  be dealt with.  Not sure yet, but the cost is minimal.
 */

const midibyte EVENT_ANY                = 0x00;      // our own value
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
 *
 *  A MIDI System Exclusive (SYSEX) message starts with F0, followed
 *  by the manufacturer ID (how many? bytes), a number of data bytes, and
 *  ended by an F7.
 */

const midibyte EVENT_MIDI_SYSEX          = 0xF0;    // redundant, see below
const midibyte EVENT_MIDI_QUARTER_FRAME  = 0xF1;    // system common > 0 bytes
const midibyte EVENT_MIDI_SONG_POS       = 0xF2;    // 2 data bytes
const midibyte EVENT_MIDI_SONG_SELECT    = 0xF3;    // 1 data byte, not used
const midibyte EVENT_MIDI_SONG_F4        = 0xF4;    // undefined
const midibyte EVENT_MIDI_SONG_F5        = 0xF5;    // undefined
const midibyte EVENT_MIDI_TUNE_SELECT    = 0xF6;    // 0 data bytes, not used
const midibyte EVENT_MIDI_SYSEX_END      = 0xF7;    // redundant, see below
const midibyte EVENT_MIDI_SYSEX_CONTINUE = 0xF7;    // redundant, see below
const midibyte EVENT_MIDI_CLOCK          = 0xF8;    // no data bytes
const midibyte EVENT_MIDI_SONG_F9        = 0xF9;    // undefined
const midibyte EVENT_MIDI_START          = 0xFA;    // no data bytes
const midibyte EVENT_MIDI_CONTINUE       = 0xFB;    // no data bytes
const midibyte EVENT_MIDI_STOP           = 0xFC;    // no data bytes
const midibyte EVENT_MIDI_SONG_FD        = 0xFD;    // undefined
const midibyte EVENT_MIDI_ACTIVE_SENSE   = 0xFE;    // 0 data bytes, not used
const midibyte EVENT_MIDI_RESET          = 0xFF;    // 0 data bytes, not used

/**
 *  0xFF is a MIDI "escape code" used in MIDI files to introduce a MIDI meta
 *  event.  Note that it has the same code (0xFF) as the Reset message, but
 *  the Meta message is read from a MIDI file, while the Reset message is sent
 *  to the sequencer by other MIDI participants.
 */

const midibyte EVENT_MIDI_META           = 0xFF;    // an escape code

/**
 *  Provides values for the currently-supported Meta events:
 *
 *      -   Set Tempo (0x51)
 *      -   Time Signature (0x58)
 */

const midibyte EVENT_META_SET_TEMPO      = 0x51;
const midibyte EVENT_META_TIME_SIGNATURE = 0x58;

/**
 *  As a "type" (overloaded on channel) value for a Meta event, 0xFF indicates
 *  an illegal meta type.
 */

const midibyte EVENT_META_ILLEGAL        = 0xFF;      // a problem code

/**
 *  This value of 0xFF is Sequencer64's channel value that indicates that
 *  the event's m_channel value is bogus.  However, it also means that the
 *  channel, if applicable to the event, is encoded in the m_status byte
 *  itself.  This is our work around to be able to hold a multi-channel SMF 0
 *  track in a sequence.  In a Sequencer64 SMF 0 track, every event has a
 *  channel.  In a Sequencer64 SMF 1 track, the events do not have a channel.
 *  Instead, the channel is a global value of the sequence, and is stuffed
 *  into each event when the event is played or is written to a MIDI file.
 */

const midibyte EVENT_NULL_CHANNEL        = 0xFF;

/**
 *  These file masks are used to obtain or to mask off the channel data from a
 *  status byte.
 */

const midibyte EVENT_GET_CHAN_MASK       = 0x0F;
const midibyte EVENT_CLEAR_CHAN_MASK     = 0xF0;

/**
 *  Variable from the "stazed" extras.  We reversed the parts of each token
 *  for consistency with the macros defined above.
 */

const int EVENTS_ALL                     = -1;
const int EVENTS_UNSELECTED              =  0;

/**
 *  This free function is used in the midifile module and in the
 *  event::is_note_off_recorded() member function.
 *
 * \param status
 *      The type of event, which might be EVENT_NOTE_ON.
 */

inline bool
is_note_off_velocity (midibyte status, midibyte data)
{
    return status == EVENT_NOTE_ON && data == 0;
}

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

public:

    /**
     *  Provides a type definition for a vector of midibytes.  This type will
     *  also hold the generally small amounts of data needed for Meta events,
     *  but doesn't help us encapsulate derived values, such as tempo.
     */

    typedef std::vector<midibyte> SysexContainer;

private:

    /**
     *  Provides the MIDI timestamp in ticks, otherwise known as the "pulses"
     *  in "pulses per quarter note" (PPQN).
     */

    midipulse m_timestamp;

    /**
     *  This is the status byte without the channel. The channel is included
     *  when recording MIDI, but, once a sequence with a matching channel is
     *  found, the channel nybble is cleared for storage.  The channel will be
     *  added back on the MIDI bus upon playback.  The high nybble = type of
     *  event; The low nybble = channel.  Bit 7 is present in all status
     *  bytes.
     *
     *  Note that, for status values of 0xF0 (Sysex) or 0xFF (Meta), special
     *  handling of the event can occur.  We would like to eventually use
     *  inheritance to keep the event class simple.  For now, search for
     *  "tempo" and "sysex" to tease out their implementations. Sigh.
     */

    midibyte m_status;

    /**
     *  In order to be able to handle MIDI channel-splitting of an SMF 0 file,
     *  we need to store the channel, even if we override it when playing the
     *  MIDI data.  This member adds another 4 bytes to the event object, most
     *  likely.
     *
     *  Overload:  For Meta events, where is_meta() is true, this value holds
     *  the type of Meta event. See the editable_event::sm_meta_event_names[]
     *  array.  Note that EVENT_META_ILLEGAL (0xFF) indicates and illegal Meta
     *  event.
     */

    midibyte m_channel;

    /**
     *  The two bytes of data for the MIDI event.  Remember that the
     *  most-significant bit of a data byte is always 0.  A one-byte message
     *  uses only the 0th index.
     */

    midibyte m_data[SEQ64_MIDI_DATA_BYTE_COUNT];

    /**
     *  The data buffer for SYSEX messages.  Adapted from Stazed's Seq32
     *  project on GitHub.  This object will also hold the generally small
     *  amounts of data needed for Meta events.
     */

    SysexContainer m_sysex;

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
     *
     * \param time
     *      Provides the time value, in ticks, to set as the timestamp.
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
     *
     * \param channel
     *      The channel to check.
     *
     * \return
     *      Returns true if the given channel matches the event's channel.
     */

    bool check_channel (int channel) const
    {
        return m_channel == EVENT_NULL_CHANNEL || midibyte(channel) == m_channel;
    }

    /**
     *  Static test for the channel message/statuse values: Note On, Note Off,
     *  Aftertouch, Control Change, Program Change, Channel Pressure, and
     *  Pitch Wheel.  This function requires that the channel data have
     *  already been masked off.
     *
     * \param m
     *      The channel status or message byte to be tested, with the channel
     *      bits masked off.
     *
     *  We could add an optional boolean to cause the channel nybble to be
     *  explicitly cleared.
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
     *  Static test for channel messages that have only one data byte: Program
     *  Change and Channel Pressure.  The rest of the channel messages have
     *  two data bytes.  This function requires that the channel data have
     *  already been masked off.
     *
     * \param m
     *      The channel status or message byte to be tested, with the channel
     *      bits masked off.
     *
     *  We could add an optional boolean to cause the channel nybble to be
     *  explicitly cleared.
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
     *  Static test for channel messages that have two data bytes: Note On,
     *  Note Off, Control Change, Aftertouch, and Pitch Wheel.  This function
     *  requires that the channel data have already been masked off.
     *
     * \param m
     *      The channel status or message byte to be tested, with the channel
     *      bits masked off.
     *
     *  We could add an optional boolean to cause the channel nybble to be
     *  explicitly cleared.
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
     *  Static test for a SysEx message.
     *
     * \param m
     *      The status/message byte to test, with the channel bits masked off.
     */

    static bool is_sysex_msg (midibyte m)
    {
        return m == EVENT_MIDI_SYSEX;
    }

    /**
     *  Static test for messages that involve notes and velocity: Note On,
     *  Note Off, and Aftertouch.  This function requires that the channel
     *  nybble has already been masked off.
     *
     * \param m
     *      The channel status or message byte to be tested, with the channel
     *      bits masked off.
     *
     *  We could add an optional boolean to cause the channel nybble to be
     *  explicitly cleared.
     *
     * \return
     *      Returns true if the byte represents a MIDI note message.
     */

    static bool is_note_msg (midibyte m)
    {
        return m == EVENT_NOTE_ON || m == EVENT_NOTE_OFF || m == EVENT_AFTERTOUCH;
    }

    /**
     *  Static test for messages that involve notes only: Note On and
     *  Note Off.
     *
     * \param m
     *      The channel status or message byte to be tested, with the channel
     *      bits masked off.
     *
     * \return
     *      Returns true if the byte represents a MIDI note on/off message.
     */

    static bool is_strict_note_msg (midibyte m)
    {
        return m == EVENT_NOTE_ON || m == EVENT_NOTE_OFF;
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
     *      The channel status or message byte to be tested, with the channel
     *      bits masked off.
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
     * \param modtick
     *      The tick value to mod the timestamp against.
     *
     * \return
     *      Returns a value ranging from 0 to _mod-1.
     */

    void mod_timestamp (midipulse modtick)
    {
        m_timestamp %= modtick;
    }

    void set_status (midibyte status);
    void set_status (midibyte eventcode, midibyte channel);
    void set_meta_status (midibyte metatype);
    void set_status_keep_channel (midibyte eventcode);

    /**
     *  Sets the channel "nybble", without modifying the status "nybble".
     *  It actually just sets the m_channel member.  Note that the sequence
     *  channel generally overrides this value in the usage of the event.
     *
     * \param channel
     *      The channel byte to be set.  It is masked to ensure the value
     *      ranges from 0x0 to 0xF.  This update should be safe, but we could
     *      allow EVENT_NULL_CHANNEL if issues are uncovered.
     */

    void set_channel (midibyte channel)
    {
        m_channel = (channel == EVENT_NULL_CHANNEL) ?
            EVENT_NULL_CHANNEL : (channel & EVENT_GET_CHAN_MASK) ;
    }

    /**
     * \getter m_status
     *      Note that we have ensured that status ranges from 0x80 to 0xFF.
     */

    midibyte get_status () const
    {
        return m_status;
    }

    /**
     *  Returns true if the event's status is *not* a control-change, but
     *  does match the given status.
     *
     * \param status
     *      The status to be checked.
     */

    bool non_cc_match (midibyte status)
    {
        return status != EVENT_CONTROL_CHANGE && m_status == status;
    }

    /**
     *  Returns true if the event's status is a control-change that matches
     *  the given status, and has a control value matching the given
     *  control-change value.
     *
     * \param st
     *      The status to be checked.
     *
     * \param cc
     *      The control-change value to be checked against the events current
     *      "d0" value.
     */

    bool cc_match (midibyte st, midibyte cc)
    {
        return st == EVENT_CONTROL_CHANGE && m_status == st && m_data[0] == cc;
    }

    /**
     *  Clears the most-significant-bit of the d1 parameter, and sets it into
     *  the first byte of m_data.  The second byte of data is zeroed.  The
     *  data bytes are in a two-byte array member, m_data.  This setter is
     *  useful for Program Change and Channel Pressure events.
     *
     * \param d1
     *      The byte value to set as the first data byte.
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
     *  Retrieves only the first  data byte from m_data[] and copies it into
     *  the parameter.
     *
     * \param d0 [out]
     *      The return reference for the first byte.
     */

    void get_data (midibyte & d0) const
    {
        d0 = m_data[0];
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
     *  Increments the first data byte (m_data[0]) and clears the most
     *  significant bit.
     */

    void increment_data1 ()
    {
        m_data[0] = (m_data[0] + 1) & 0x7F;
    }

    /**
     *  Decrements the first data byte (m_data[0]) and clears the most
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

    bool append_sysex (midibyte * data, int len);
    bool append_sysex (midibyte data);
    bool append_meta_data (midibyte metatype, midibyte * data, int len);
    void restart_sysex ();              // kind of useless

    /**
     *  Resets and adds ex data.
     *
     * \param data
     *      Provides the SysEx/Meta data.  If not provided, nothing is done,
     *      and false is returned.
     *
     * \param len
     *      The number of bytes to set.
     *
     * \return
     *      Returns true if the function succeeded.
     */

    bool set_sysex (midibyte * data, int len)
    {
        m_sysex.clear();
        return append_sysex(data, len);
    }

    /**
     * \getter m_sysex from stazed, non-const version for use by midibus.
     */

    SysexContainer & get_sysex ()
    {
        return m_sysex;
    }

    /**
     * \getter m_sysex from stazed
     */

    const SysexContainer & get_sysex () const
    {
        return m_sysex;
    }

    /**
     * \setter m_sysex from stazed
     */

    void set_sysex_size (int len)
    {
        if (len == 0)
            m_sysex.clear();
        else
            m_sysex.resize(len);
    }

    /**
     * \getter m_sysex.size()
     */

    int get_sysex_size () const
    {
        return int(m_sysex.size());
    }

    /**
     *  Sets m_has_link and sets m_link to the provided event pointer.
     *
     * \param ev
     *      Provides a pointer to the event value to set.  If null, then
     *      m_has_link is set to false, to guarantee that is_linked() is
     *      correct.
     */

    void link (event * ev)
    {
        m_linked = ev;
        m_has_link = not_nullptr(ev);
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
     * \setter m_has_link and m_linked
     */

    void clear_link ()
    {
        m_has_link = false;
        m_linked = nullptr;
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
     *
     * \param note
     *      Provides the note value to set.
     */

    void set_note (midibyte note)
    {
        m_data[0] = note & 0x7F;
    }

#ifdef SEQ64_STAZED_TRANSPOSE
    void transpose_note (int tn);
#endif

    /**
     * \getter m_data[1], the note velocity.
     */

    midibyte get_note_velocity () const
    {
        return m_data[1];
    }

    /**
     *  Sets the note velocity, which is held in the second data byte, and
     *  clearing off the most-significant-bit, storing it in m_data[1].
     *
     * \param vel
     *      Provides the velocity value to set.
     */

    void set_note_velocity (int vel)
    {
        m_data[1] = vel & 0x7F;
    }

    /**
     *  Check for the Note On value in m_status.  Currently assumes that the
     *  channel nybble has already been stripped.
     *
     * \return
     *      Returns true if m_status is EVENT_NOTE_ON.
     */

    bool is_note_on () const
    {
        return m_status == EVENT_NOTE_ON;
    }

    /**
     *  Check for the Note Off value in m_status.  Currently assumes that the
     *  channel nybble has already been stripped.
     *
     * \return
     *      Returns true if m_status is EVENT_NOTE_OFF.
     */

    bool is_note_off () const
    {
        return m_status == EVENT_NOTE_OFF;
    }

    /**
     *  Returns true if m_status is a Note On, Note Off, or Aftertouch message.
     *  All of these are notes, associated with a MIDI key value.  Uses the
     *  static function is_note_msg().
     *
     * \return
     *      The return value of is_note_msg() is returned.
     */

    bool is_note () const
    {
        return is_note_msg(m_status);
    }

    /**
     *  Some keyboards send Note On with velocity 0 for Note Off, so we
     *  provide this function to test that during recording.
     *  The channel nybble is masked off before the test, but this is
     *  unnecessary since the velocity byte doesn't contain the channel!
     *  And it is a nasty bug!
     *
     *  "(m_data[1] & EVENT_CLEAR_CHAN_MASK) == 0" is wrong!
     *
     *  "m_status == EVENT_NOTE_ON && m_data[1] == 0" replaced by an inline
     *  function call for robustness.
     *
     * \return
     *      Returns true if the event is a Note On event with velocity of 0.
     */

    bool is_note_off_recorded () const
    {
        return is_note_off_velocity(m_status, m_data[1]);
    }

    void adjust_note_off ();

    /**
     *  Indicates if the m_status value is a one-byte message (Program Change
     *  or Channel Pressure.  Channel is stripped, because sometimes we keep
     *  the channel.
     */

    bool is_one_byte () const
    {
        return is_one_byte_msg(m_status);   // && 0xF0);
    }

    /**
     *  Indicates if the m_status value is a two-byte message (everything
     *  except Program Change, Channel Pressure, and ).
     *  Channel is stripped, because sometimes we keep
     *  the channel.
     */

    bool is_two_bytes () const
    {
        return is_two_byte_msg(m_status);   // && 0xF0);
    }

    /**
     *  Indicates if the event is a System Exclusive event or not.
     *  We're overloading the SysEx support to handle Meta events as well.
     *  Perhaps we need to split this support out at some point.
     */

    bool is_sysex () const
    {
        return m_status == EVENT_MIDI_SYSEX;
    }

    /**
     *  Indicates if the event is a Meta event or not.
     *  We're overloading the SysEx support to handle Meta events as well.
     */

    bool is_meta () const
    {
        return m_status == EVENT_MIDI_META;
    }

    /**
     *  Indicates if we need to use extended data (SysEx or Meta).
     */

    bool is_ex_data () const
    {
        return m_status == EVENT_MIDI_META || m_status == EVENT_MIDI_SYSEX;
    }

    /**
     *  Indicates if the event is a tempo event.  See sm_meta_event_names[].
     */

    bool is_tempo () const
    {
        return is_meta() && m_channel == EVENT_META_SET_TEMPO;      /* 0x51 */
    }

    midibpm tempo () const;
    void set_tempo (midibpm tempo);

    /**
     *  Indicates if the event is a Time Signature event.  See
     *  sm_meta_event_names[].
     */

    bool is_time_signature () const
    {
        return is_meta() && m_channel == EVENT_META_TIME_SIGNATURE; /* 0x58 */
    }

    void print () const;

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
extern event create_tempo_event (midipulse tick, midibpm tempo);

}           // namespace seq64

#endif      // SEQ64_EVENT_HPP

/*
 * event.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

