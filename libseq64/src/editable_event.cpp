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
 * \file          editable_event.cpp
 *
 *  This module declares/defines the base class for MIDI editable_events.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2018-04-22
 * \license       GNU GPLv2 or above
 *
 *  A MIDI editable event is encapsulated by the seq64::editable_event
 *  object.
 */

#include <stdlib.h>                     /* atoi(3) and atof(3) for 32-bit   */
#include "easy_macros.h"
#include "editable_event.hpp"           /* seq64::editable_event            */
#include "editable_events.hpp"          /* seq64::editable_events multimap  */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *  Initializes the array of event/name pairs for the MIDI events categories.
 *  Terminated by an empty string, the latter being the preferred test, for
 *  consistency with the other arrays and because 0 is often a legitimate code
 *  value.
 */

const editable_event::name_value_t
editable_event::sm_category_names [] =
{
    { (unsigned short)(category_channel_message),   "Channel Message"   },
    { (unsigned short)(category_system_message),    "System Message"    },
    { (unsigned short)(category_meta_event),        "Meta Event"        },
    { (unsigned short)(category_prop_event),        "Proprietary Event" },
    { SEQ64_END_OF_MIDIBYTE_TABLE,                  ""                  }
};

/**
 *  Initializes the array of event/name pairs for the channel MIDI events.
 *  Terminated by an empty string.
 */

const editable_event::name_value_t
editable_event::sm_channel_event_names [] =
{
    { (unsigned short)(EVENT_NOTE_OFF),         "Note Off"          },  // 0x80
    { (unsigned short)(EVENT_NOTE_ON),          "Note On"           },  // 0x90
    { (unsigned short)(EVENT_AFTERTOUCH),       "Aftertouch"        },  // 0xA0
    { (unsigned short)(EVENT_CONTROL_CHANGE),   "Control"           },  // 0xB0
    { (unsigned short)(EVENT_PROGRAM_CHANGE),   "Program"           },  // 0xC0
    { (unsigned short)(EVENT_CHANNEL_PRESSURE), "Ch Pressure"       },  // 0xD0
    { (unsigned short)(EVENT_PITCH_WHEEL),      "Pitch Wheel"       },  // 0xE0
    { SEQ64_END_OF_MIDIBYTE_TABLE,              ""                  }   // end
};

/**
 *  Initializes the array of event/name pairs for the system MIDI events.
 *  Terminated by an empty string.
 */

const editable_event::name_value_t
editable_event::sm_system_event_names [] =
{
    { (unsigned short)(EVENT_MIDI_SYSEX),         "SysEx Start"     },  // 0xF0
    { (unsigned short)(EVENT_MIDI_QUARTER_FRAME), "Quarter Frame"   },  //   .
    { (unsigned short)(EVENT_MIDI_SONG_POS),      "Song Position"   },  //   .
    { (unsigned short)(EVENT_MIDI_SONG_SELECT),   "Song Select"     },  //   .
    { (unsigned short)(EVENT_MIDI_SONG_F4),       "F4"              },
    { (unsigned short)(EVENT_MIDI_SONG_F5),       "F5"              },
    { (unsigned short)(EVENT_MIDI_TUNE_SELECT),   "Tune Request"    },
    { (unsigned short)(EVENT_MIDI_SYSEX_END),     "SysEx End"       },
    { (unsigned short)(EVENT_MIDI_CLOCK),         "Clock"           },
    { (unsigned short)(EVENT_MIDI_SONG_F9),       "F9"              },
    { (unsigned short)(EVENT_MIDI_START),         "Start"           },
    { (unsigned short)(EVENT_MIDI_CONTINUE),      "Continue"        },
    { (unsigned short)(EVENT_MIDI_STOP),          "Stop"            },  //   .
    { (unsigned short)(EVENT_MIDI_SONG_FD),       "FD"              },  //   .
    { (unsigned short)(EVENT_MIDI_ACTIVE_SENSE),  "Active sensing"  },  //   .
    { (unsigned short)(EVENT_MIDI_RESET),         "Reset"           },  // 0xFF
    { SEQ64_END_OF_MIDIBYTE_TABLE,                ""                }   // end
};

/**
 *  Initializes the array of event/name pairs for all of the Meta events.
 *  Terminated only by the empty string.
 */

const editable_event::name_value_t
editable_event::sm_meta_event_names [] =
{
    { 0x00, "Seq Number"                },      // FF 00 02 ss ss (16-bit)
    { 0x01, "Text Event"                },      // FF 01 len text
    { 0x02, "Copyright"                 },      // FF 02 len text
    { 0x03, "Track Name"                },      // FF 03 len text
    { 0x04, "Instrument Name"           },      // FF 04 len text
    { 0x05, "Lyric"                     },      // FF 05 len text
    { 0x06, "Marker"                    },      // FF 06 len text
    { 0x07, "Cue Point"                 },      // FF 07 len text
    { 0x08, "Program Name"              },      // FF 08 len text
    { 0x09, "Device Name"               },      // FF 09 len text

    /*
     * The following events are normally not documented, so let's save some
     * lookup time.
     *
     * { 0x0A, "Text Event 0A"          },
     * { 0x0B, "Text Event 0B"          },
     * { 0x0C, "Text Event 0C"          },
     * { 0x0D, "Text Event 0D"          },
     * { 0x0E, "Text Event 0E"          },
     * { 0x0F, "Text Event 0F"          },
     */

    { 0x20, "MIDI Channel"              },      // FF 20 01 cc (obsolete)
    { 0x21, "MIDI Port"                 },      // FF 21 01 pp (obsolete)
    { 0x2F, "Track End"                 },      // FF 2F 00 (mandatory event)
    { 0x51, "Tempo"                     },      // FF 51 03 tt tt tt (set tempo)
    { 0x54, "SMPTE Offset"              },      // FF 54 05 hh mm ss fr ff
    { 0x58, "Time Sig"                  },      // FF 58 04 nn dd cc bb
    { 0x59, "Key Sig"                   },      // FF 59 02 sf mi
    { 0x7F, "Seq Spec"                  },      // FF 7F len id data (seq24 prop)
    { 0xFF, "Illegal meta event"        },      // indicator of problem
    { SEQ64_END_OF_MIDIBYTE_TABLE, ""   }       // terminator
};

/**
 *  Initializes the array of event/length pairs for all of the Meta events.
 *  Terminated only by the empty string.
 */

const editable_event::meta_length_t
editable_event::sm_meta_lengths [] =
{
    { 0x00, 2   },                              // "Seq Number"

    /*
     * Since meta_event_length() returns 0 by default, we can save some lookup
     * time.
     *
     * { 0x01, 0   },                           // "Text Event"
     * { 0x02, 0   },                           // "Copyright"
     * { 0x03, 0   },                           // "Track Name"
     * { 0x04, 0   },                           // "Instrument Name"
     * { 0x05, 0   },                           // "Lyric"
     * { 0x06, 0   },                           // "Marker"
     * { 0x07, 0   },                           // "Cue Point"
     * { 0x08, 0   },                           // "Program Name"
     * { 0x09, 0   },                           // "Device Name"
     */

    /*
     * The following events are normally not documented, so let's save some
     * more lookup time.
     *
     * { 0x0A, 0    },                          // "Text Event 0A"
     * { 0x0B, 0    },                          // "Text Event 0B"
     * { 0x0C, 0    },                          // "Text Event 0C"
     * { 0x0D, 0    },                          // "Text Event 0D"
     * { 0x0E, 0    },                          // "Text Event 0E"
     * { 0x0F, 0    },                          // "Text Event 0F"
     */

    { 0x20, 1   },                              // "MIDI Channel"
    { 0x21, 1   },                              // "MIDI Port"
    { 0x2F, 0   },                              // "Track End"
    { 0x51, 3   },                              // "Tempo"
    { 0x54, 5   },                              // "SMPTE Offset"
    { 0x58, 4   },                              // "Time Sig"
    { 0x59, 2   },                              // "Key Sig"
    { 0x7F, 0   },                              // "Seq Spec"
    { 0xFF, 0   },                              // "Illegal meta event"
    { SEQ64_END_OF_MIDIBYTE_TABLE, 0 }          // terminator
};

/**
 *  Initializes the array of event/name pairs for all of the
 *  seq24/sequencer64-specific events.  Terminated only by the empty string.
 *  Note that the numbers reflect the masking off of the high-order bits by
 *  0x242400FF.
 */

const editable_event::name_value_t
editable_event::sm_prop_event_names [] =
{
    { 0x01, "Buss number"               },
    { 0x02, "Channel number"            },
    { 0x03, "Clocking"                  },
    { 0x04, "Old triggers"              },
    { 0x05, "Song notes"                },
    { 0x06, "Time signature"            },
    { 0x07, "Beats per minute"          },
    { 0x08, "Trigger data"              },
    { 0x09, "Song mute group data"      },
    { 0x10, "Song MIDI control"         },
    { 0x11, "Key"                       },
    { 0x12, "Scale"                     },
    { 0x13, "Background sequence"       },
    { SEQ64_END_OF_MIDIBYTE_TABLE, ""   }                      // terminator
};

/**
 *  Contains pointers (references cannot be stored in an array)  to the
 *  desired array for a given category.  This code could be considered a bit
 *  rococo.
 */

const editable_event::name_value_t * const
editable_event::sm_category_arrays [] =
{
    editable_event::sm_category_names,
    editable_event::sm_channel_event_names,
    editable_event::sm_system_event_names,
    editable_event::sm_meta_event_names,
    editable_event::sm_prop_event_names
};

/**
 *  Provides a static lookup function that returns the name, if any,
 *  associated with a midibyte value.
 *
 * \param value
 *      The MIDI byte value to look up.
 *
 * \param cat
 *      The category of the MIDI byte.  Each category calls a different name
 *      array into play.
 *
 *  \return
 *      Returns the name associated with the value.  If there is no such name,
 *      then an empty string is returned.
 */

std::string
editable_event::value_to_name
(
    midibyte value,
    editable_event::category_t cat
)
{
    std::string result;
    const editable_event::name_value_t * const table =
        editable_event::sm_category_arrays[cat];

    if (cat == category_channel_message)
        value &= EVENT_CLEAR_CHAN_MASK;

    midibyte counter = 0;
    while (table[counter].event_value != SEQ64_END_OF_MIDIBYTE_TABLE)
    {
        if (value == table[counter].event_value)
        {
            result = table[counter].event_name;
            break;
        }
        ++counter;
    }
    return result;
}

/**
 *  Provides a static lookup function that returns the value, if any,
 *  associated with a name string.  The string_match() function, which can
 *  match abbreviations, case-insensitively, is used to make the string
 *  comparisons.
 *
 * \param name
 *      The string value to look up.
 *
 * \param cat
 *      The category of the MIDI byte.  Each category calls a different name
 *      array into play.
 *
 *  \return
 *      Returns the value associated with the name.  If there is no such value,
 *      then SEQ64_END_OF_MIDIBYTE_TABLE is returned.
 */

unsigned short
editable_event::name_to_value
(
    const std::string & name,
    editable_event::category_t cat
)
{
    unsigned short result = SEQ64_END_OF_MIDIBYTE_TABLE;
    if (! name.empty())
    {
        const editable_event::name_value_t * const table =
            editable_event::sm_category_arrays[cat];

        midibyte counter = 0;
        while (table[counter].event_value != SEQ64_END_OF_MIDIBYTE_TABLE)
        {
            if (strings_match(table[counter].event_name, name))
            {
                result = table[counter].event_value;
                break;
            }
            ++counter;
        }
    }
    return result;
}

/**
 *  Provides a static lookup function that takes a meta-event number and
 *  returns the expected length of the data for that event.
 *
 * \param value
 *      The MIDI byte value to look up.
 *
 *  \return
 *      Returns the length associated with the meta event.  If the expected
 *      length is actually 0, or is variable, then 0 is returned.
 */

unsigned short
editable_event::meta_event_length (midibyte value)
{
    unsigned short result = 0;
    midibyte counter = 0;
    while (sm_meta_lengths[counter].event_value != SEQ64_END_OF_MIDIBYTE_TABLE)
    {
        if (value == sm_meta_lengths[counter].event_value)
        {
            result = sm_meta_lengths[counter].event_length;
            break;
        }
        ++counter;
    }
    return result;
}

/**
 *  Principal constructor.
 *
 *  The default constructor is hidden and unimplemented.  We will get the
 *  default controller name from the controllers module.  We should also be
 *  able to look up the selected buss's entries for a sequence, and load up
 *  the CC/name pairs on the fly.
 *
 * \param parent
 *      Provides the overall editable-events object that manages the whole set
 *      of editable-event.
 */

editable_event::editable_event (const editable_events & parent)
 :
    event               (),
    m_parent            (parent),
    m_category          (category_name),
    m_name_category     (),
    m_format_timestamp  (timestamp_measures),
    m_name_timestamp    (),
    m_name_status       (),
    m_name_meta         (),
    m_name_seqspec      (),
    m_name_channel      (),
    m_name_data         ()
{
    // Empty body
}

/**
 *  Event constructor.  This function basically adds all of the extra
 *  editable_event stuff to a standard event, so that the resulting
 *  editable_event is container-ready.
 */

editable_event::editable_event
(
    const editable_events & parent,
    const event & ev
) :
    event               (ev),
    m_parent            (parent),
    m_category          (category_name),
    m_name_category     (),
    m_format_timestamp  (timestamp_measures),
    m_name_timestamp    (),
    m_name_status       (),
    m_name_meta         (),
    m_name_seqspec      (),
    m_name_channel      (),
    m_name_data         ()
{
    // analyze();               // DO IT NOW OR LATER?
}

/**
 *  This copy constructor initializes most of the class members.  This
 *  function is currently geared only toward support of the SMF 0
 *  channel-splitting feature.  Many of the members are not set to useful
 *  values when the MIDI file is read, so we don't handle them for now.
 *
 * \warning
 *      This function does not yet copy the SysEx data.  The inclusion
 *      of SysEx editable_events was not complete in Seq24, and it is still not
 *      complete in Sequencer64.  Nor does it currently bother with the
 *      links.
 *
 * \param rhs
 *      Provides the editable_event object to be copied.
 */

editable_event::editable_event (const editable_event & rhs)
 :
    event               (rhs),
    m_parent            (rhs.m_parent),
    m_category          (rhs.m_category),
    m_name_category     (rhs.m_name_category),
    m_format_timestamp  (rhs.m_format_timestamp),
    m_name_timestamp    (rhs.m_name_timestamp),
    m_name_status       (rhs.m_name_status),
    m_name_meta         (rhs.m_name_meta),
    m_name_seqspec      (rhs.m_name_seqspec),
    m_name_channel      (rhs.m_name_channel),
    m_name_data         (rhs.m_name_data)
{
    // Empty body
}

/*
 *  This principal assignment operator sets the class members.
 *
 * \param rhs
 *      Provides the editable_event object to be assigned.
 *
 * \return
 *      Returns a reference to "this" object, to support the serial assignment
 *      of editable_events.
 */

editable_event &
editable_event::operator = (const editable_event & rhs)
{
    if (this != &rhs)
    {
        event::operator =(rhs);
    //  m_parent            = rhs.m_parent;         // cannot copy a reference
        m_category          = rhs.m_category;
        m_name_category     = rhs.m_name_category;
        m_format_timestamp  = rhs.m_format_timestamp;
        m_name_timestamp    = rhs.m_name_timestamp;
        m_name_status       = rhs.m_name_status;
        m_name_meta         = rhs.m_name_meta;
        m_name_seqspec      = rhs.m_name_seqspec;
        m_name_channel      = rhs.m_name_channel;
        m_name_data         = rhs.m_name_data;
    }
    return *this;
}

/**
 * \setter m_category by value
 *      Also keeps the m_name_category member in synchrony.  Note that a bad
 *      value is translated to the enum value category_name.
 *
 * \param c
 *      Provides the category value to set.
 */

void
editable_event::category (category_t c)
{
    if (c >= category_channel_message && c <= category_prop_event)
        m_category = c;
    else
        m_category = category_name;

    std::string name = value_to_name(c, category_name);
    if (! name.empty())
        m_name_category = name;
}

/**
 * \setter m_category by name
 *      Also keeps the m_name_category member in synchrony, but looks up the
 *      name, rather than using the name parameter, to avoid storing
 *      abbreviations.  Note that a bad value is translated to the value of
 *      category_name.
 *
 * \param name
 *      Provides the category name for the category value to set.
 */

void
editable_event::category (const std::string & name)
{
    unsigned short catcode = name_to_value(name, category_name);
    if (catcode < SEQ64_END_OF_MIDIBYTE_TABLE)
        m_category = category_t(catcode);
    else
        m_category = category_name;

    m_name_category = value_to_name(m_category, category_name);
}

/**
 * \setter event::set_timestamp()
 *      Implemented to allow a uniform naming convention that is not
 *      slavish to the get/set crowd [this ain't Java].  Plus, we also
 *      have to set the string version at the same time.
 *
 *  The format of the string representation is of the format selected by the
 *  m_format_timestamp member and is set by the format_timestamp() function.
 *
 * \param ts
 *      Provides the timestamp in units of MIDI pulses.
 */

void
editable_event::timestamp (midipulse ts)
{
    event::set_timestamp(ts);
    format_timestamp();
}

/**
 * \setter event::set_timestamp() [string version]
 *
 *  The format of the string representation is of the format selected by the
 *  m_format_timestamp member and is set by the format_timestamp() function.
 *
 * \param ts_string
 *      Provides the timestamp in units of MIDI pulses.
 */

void
editable_event::timestamp (const std::string & ts_string)
{
    midipulse ts = m_parent.string_to_pulses(ts_string);
    event::set_timestamp(ts);
    format_timestamp();
}

/**
 *  Formats the current timestamp member as a string.  The format of the
 *  string representation is of the format selected by the m_format_timestamp
 *  member.
 */

std::string
editable_event::format_timestamp ()
{
    if (m_format_timestamp == timestamp_measures)
        m_name_timestamp = time_as_measures();
    else if (m_format_timestamp == timestamp_time)
        m_name_timestamp = time_as_minutes();
    else if (m_format_timestamp == timestamp_pulses)
        m_name_timestamp = time_as_pulses();
    else
        m_name_timestamp = "unsupported category in editable event";

    return m_name_timestamp;
}

/**
 *  Converts the current time-stamp to a string representation in units of
 *  measures, beats, and divisions.  Cannot be inlined because of a circular
 *  dependency between the editable_event and editable_events classes.
 */

std::string
editable_event::time_as_measures ()
{
    return pulses_to_measurestring(get_timestamp(), parent().timing());
}

/**
 *  Converts the current time-stamp to a string representation in units of
 *  hours, minutes, seconds, and fraction.  Cannot be inlined because of a
 *  circular dependency between the editable_event and editable_events
 *  classes.
 */

std::string
editable_event::time_as_minutes ()
{
    return pulses_to_timestring(get_timestamp(), parent().timing());
}

/**
 *  Converts a string into an event status, along with timestamp and data
 *  bytes.  Currently, this function handles only the following two messages:
 *
 *      -   category_channel_message
 *      -   category_system_message
 *
 *  After all of the numbering member items have been set, they are converted
 *  and assigned to the string versions via a call to the analyze() function.
 *
 * \param ts
 *      Provides the time-stamp string of the event.
 *
 * \param s
 *      Provides the name of the event, such as "Program Change".
 *
 * \param sd0
 *      Provides the string defining the first data byte of the event.  For
 *      Meta events, this might have multiple values, though we support only
 *      Set Tempo and Time Signature at present.
 *
 * \param sd1
 *      Provides the string defining the second data byte of the event, if
 *      applicable to the event.  Some meta event may provide multiple values
 *      in this string.
 */

void
editable_event::set_status_from_string
(
    const std::string & ts,
    const std::string & s,
    const std::string & sd0,
    const std::string & sd1
)
{
    unsigned short value = name_to_value(s, category_channel_message);
    timestamp(ts);
    if (value != SEQ64_END_OF_MIDIBYTE_TABLE)
    {
        midibyte newstatus = midibyte(value);
        midibyte d0 = string_to_midibyte(sd0);
        set_status(newstatus, get_channel());   /* pass along code & channel */
        if (is_one_byte_msg(newstatus))
        {
            set_data(d0);
        }
        else if (is_two_byte_msg(newstatus))
        {
            midibyte d1 = string_to_midibyte(sd1);
            set_data(d0, d1);
        }
    }
    else
    {
        value = name_to_value(s, category_meta_event);
        if (value != SEQ64_END_OF_MIDIBYTE_TABLE)
        {
            /*
             * Handle Meta or SysEx events, setting that status to 0xFF and
             * the meta-type (in the m_channel member) to the meta event
             * type-value, then filling in m_sysex based on the field values
             * in the sd0 parameter.
             */

            set_meta_status(value);
            if (value == EVENT_META_SET_TEMPO)                      /* 0x51 */
            {
                /*
                 * The Tempo data 0 field consists of one double BPM value.
                 * We convert it to a tempo-in-microseconds value, then
                 * populate a 3-byte array with it.  Then we need to create an
                 * event from it.
                 */

                double bpm = atof(sd0.c_str());
                if (bpm > 0.0f)
                {
                    midibyte t[4];
                    double tempo_us = tempo_us_from_bpm(bpm);
                    tempo_us_to_bytes(t, tempo_us);
                    (void) set_sysex(t, 3);         /* add ex-data bytes    */
                }
            }
            else if (value == EVENT_META_TIME_SIGNATURE)            /* 0x51 */
            {
                /*
                 * The Time Signature data 0 field consists of a string like
                 * "4/4".  The data 1 field has two values for metronome
                 * support.  First, parse the "nn/dd" string; the slash
                 * (solidus) is required.  Then get the cc and bb metronome
                 * values, if present.  Otherwise, hardwired them to values of
                 * 0x18 and 0x08.
                 */

                std::string::size_type pos = sd0.find_first_of("/");
                if (pos != std::string::npos)
                {
                    int nn = atoi(sd0.c_str());
                    int dd = nn;
                    int cc = 0x18;
                    int bb = 0x08;
                    ++pos;

                    std::string sd0_partial = sd0.substr(pos);  // drop "nn/"
                    dd = atoi(sd0_partial.c_str());             // get dd
                    if (dd > 0)
                    {
                        pos = sd0.find_first_of(" ", pos);      // bypass dd
                        if (pos != std::string::npos)
                        {
                            pos = sd0.find_first_of("0123456789x", pos);
                            if (pos != std::string::npos)
                            {
                                cc = int(strtol(&sd0[pos], NULL, 0));
                                pos = sd0.find_first_of(" ", pos);
                                if (pos != std::string::npos)
                                {
                                    pos = sd0.find_first_of("0123456789x", pos);
                                    if (pos != std::string::npos)
                                        bb = int(strtol(&sd0[pos], NULL, 0));
                                }
                            }
                        }
                        midibyte t[4];
                        t[0] = midibyte(nn);
                        t[1] = midibyte(dd);
                        t[2] = midibyte(cc);
                        t[3] = midibyte(bb);
                        (void) set_sysex(t, 4);     /* add ex-data bytes    */
                    }
                }
            }
            else
            {
                /*
                 * Parse the string of (potentially) hex digits.
                 *
                 * TODO:
                 *
                 * However, we still need to determine the length
                 * value and allocate the midibyte array ahead of time, or add
                 * a function to set sysex.
                 */

                std::string::size_type pos = sd0.find_first_of("0123456789x");
                while (pos != std::string::npos)
                {
                    // TODO
                }
            }
        }
    }
    analyze();                          /* create the strings   */
}

/**
 *  Converts the event into a string desribing the full event.  We get the
 *  time-stamp as a string, make sure the event is fully analyzed so that all
 *  items and strings are set correctly.
 *
 * \return
 *      Returns a human-readable string describing this event.  This string is
 *      displayed in an event list, such as in the eventedit module.
 */

std::string
editable_event::stock_event_string ()
{
    char temp[64];
    std::string ts = format_timestamp();
    analyze();
    if (is_ex_data())
    {
        if (is_tempo() || is_time_signature())
        {
            snprintf
            (
                temp, sizeof temp, "%9s %-11s %-10s",
                ts.c_str(), m_name_status.c_str(), m_name_data.c_str()
            );
        }
        else
        {
            snprintf
            (
                temp, sizeof temp, "%9s %-11s %-12s",
                ts.c_str(), m_name_status.c_str(), m_name_data.c_str()
            );
        }
    }
    else
    {
        snprintf
        (
            temp, sizeof temp, "%9s %-11s %-10s %-20s",
            ts.c_str(), m_name_status.c_str(),
            m_name_channel.c_str(), m_name_data.c_str()
        );
    }
    return std::string(temp);
}

/**
 *  Analyzes an editable-event to make all the settings it needs.  Used in the
 *  constructors.  Some of the setters indirectly set the appropriate string
 *  representation, as well.
 *
 * Category:
 *
 *      This function can figure out if the status byte implies a channel
 *      message or a system message, and set the category string as well.
 *      However, at this time, detection of Meta events (0xFF) or
 *      Proprietary/SeqSpec events (0xFF with 0x2424) doesn't work due to lack
 *      of context here (and due to the fact that currently such events are
 *      not yet stored in a Sequencer64 sequence/track, and the
 *      least-significant-byte gets masked off anyway.)
 *
 * Status:
 *
 *      We distinguish between channel and system messages, and then one- and
 *      two-byte messages, but don't yet distinguish the data values fully.
 *
 * Sysex and Meta events:
 *
 *      We are starting to support events with statuses ranging from 0xF0 to
 *      0xFF, with a concentration on Set Tempo and Time Signature events.
 *      We want them to be full-fledged Sequencer64 events.
 *
 *      The 0xFF byte represents a Meta event, not a Reset event, when we're
 *      dealing with data from a MIDI file, as we are here. And we need to get
 *      the next byte after the status byte.
 *
 *      We want Set Tempo events to appear as "Tempo 120.0" and Time Signature
 *      events to appear as "Time Sig 4/4".
 */

void
editable_event::analyze ()
{
    midibyte status = get_status();
    char tmp[32];
    if (status >= EVENT_NOTE_OFF && status <= EVENT_PITCH_WHEEL)
    {
        midibyte channel = get_channel();
        midibyte d0, d1;
        get_data(d0, d1);
        category(category_channel_message);
        status = get_status() & EVENT_CLEAR_CHAN_MASK;

        /*
         * Get channel message name (e.g. "Program change");
         */

        m_name_status = value_to_name(status, category_channel_message);
        snprintf(tmp, sizeof tmp, "Chan %d", int(channel));
        m_name_channel = std::string(tmp);
        if (is_one_byte_msg(status))
            snprintf(tmp, sizeof tmp, "Data %d", int(d0));
        else
        {
            if (is_note_msg(status))
            {
                snprintf(tmp, sizeof tmp, "Key %d Vel %d", int(d0), int(d1));
            }
            else
            {
                snprintf
                (
                    tmp, sizeof tmp, "Data %d, %d", int(d0), int(d1)
                );
            }
        }
        m_name_data = std::string(tmp);
    }
    else if (status >= EVENT_MIDI_SYSEX)
    {
        if (status == EVENT_MIDI_META)          /* not EVENT_MIDI_RESET */
        {
            midibyte metatype = get_channel();  /* \tricky              */
            category(category_meta_event);
            m_name_status = value_to_name(metatype, category_meta_event);
            m_name_channel.clear();             /* will not be output   */
            m_name_data = ex_data_string();
        }
        else
        {
            category(category_system_message);

            /*
             * Get system message name (e.g. "SysEx start");
             */

            m_name_status = value_to_name(status, category_system_message);
            m_name_channel.clear();
            m_name_data.clear();
        }
    }
    else
    {
        // Would try to detect SysEx versus Meta message versus SeqSpec here.
        // Then set either m_name_meta and/or m_name_seqspec.
        // ALso see eventslots::set_current_event().
    }
}

/**
 *  Assuming the event is a Meta event or a SysEx, this function returns a
 *  short string representation of the event data, usable in the eventeditor
 *  class or elsewhere.  Most SysEx events will only show the first few bytes;
 *  we could make a SysEx viewer/editor for handling long events.
 *
 * \return
 *      Returns the data string.  If empty, the data is bad in some way, or
 *      the event is not a Meta event.
 */

std::string
editable_event::ex_data_string () const
{
    std::string result;
    char tmp[32];
    if (is_tempo())
    {
        snprintf(tmp, sizeof tmp, "%6.2f", tempo());
        result = tmp;
    }
    else if (is_time_signature())
    {
        if (get_sysex_size() > 0)
        {
            int nn = get_sysex()[0];
            int dd = get_sysex()[1];            /* hopefully a power of 2   */
            int cc = get_sysex()[2];
            int bb = get_sysex()[3];
            snprintf(tmp, sizeof tmp, "%d/%d 0x%X 0x%X", nn, dd, cc, bb);
            result += tmp;
        }
    }
    else
    {
        std::string data;
        int limit = get_sysex_size();
        if (limit > 4)
            limit = 4;                          /* we have space limits     */

        for (int i = 0; i < limit; ++i)
        {
            snprintf(tmp, sizeof tmp, "%2X ", get_sysex()[i]);
            result += tmp;
        }
        if (get_sysex_size() > 4)
            result += "...";
    }
    return result;
}

}           // namespace seq64

/*
 * editable_event.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

