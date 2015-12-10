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
 * \updates       2015-12-09
 * \license       GNU GPLv2 or above
 *
 *  A MIDI editable event is encapsulated by the seq64::editable_event
 *  object.
 */

#include "easy_macros.h"
#include "controllers.hpp"              /* seq64::c_controller_names[]      */
#include "editable_event.hpp"           /* seq64::editable_event            */
#include "editable_events.hpp"          /* seq64::editable_events multimap  */

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
    { (unsigned short)(EVENT_CONTROL_CHANGE),   "Control Change"    },  // 0xB0
    { (unsigned short)(EVENT_PROGRAM_CHANGE),   "Program Change"    },  // 0xC0
    { (unsigned short)(EVENT_CHANNEL_PRESSURE), "Channel Pressure"  },  // 0xD0
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
    { (unsigned short)(EVENT_MIDI_ACTIVE_SENS),   "Active sensing"  },  //   .
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
    { 0x00, "Sequence number"           },
    { 0x01, "Text event"                },
    { 0x02, "Copyright notice"          },
    { 0x03, "Track name"                },
    { 0x04, "Instrument name"           },
    { 0x05, "Lyrics"                    },
    { 0x06, "Marker"                    },
    { 0x07, "Cue point"                 },
    { 0x08, "Program name"              },
    { 0x09, "Device name"               },
    { 0x0A, "Text event 0A"             },
    { 0x0B, "Text event 0B"             },
    { 0x0C, "Text event 0C"             },
    { 0x0D, "Text event 0D"             },
    { 0x0E, "Text event 0E"             },
    { 0x0F, "Text event 0F"             },
    { 0x20, "MIDI channel"              },      // obsolete in MIDI
    { 0x21, "MIDI port"                 },      // obsolete in MIDI
    { 0x2F, "End of track"              },
    { 0x51, "Set tempo"                 },
    { 0x54, "SMPTE offset"              },
    { 0x58, "Time signature"            },
    { 0x59, "Key signature"             },
    { 0x7F, "Sequencer specific"        },      // includes seq24 prop values
    { SEQ64_END_OF_MIDIBYTE_TABLE, ""   }       // terminator
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
        counter++;
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
            counter++;
        }
    }
    return result;
}

/*
 * We will get the default controller name from the controllers module.
 * We should also be able to look up the selected buss's entries for a
 * sequence, and load up the CC/name pairs on the fly.
 */

/**
 *  This constructor simply initializes all of the class members.
 *
 *  editable_event::editable_event ()
 *   :
 *      event               (),
 *      m_category          (category_name),
 *      m_name_category     (),
 *      m_format_timestamp  (timestamp_measures),
 *      m_name_timestamp    (),
 *      m_name_status       (),
 *      m_name_meta         (),
 *      m_name_seqspec      (),
 *      m_name_channel      (),
 *      m_name_data         ()
 *  {
 *      // Empty body
 *  }
 *
 */

/**
 *  Principal constructor.
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

/**
 *  This principal assignment operator sets most of the class members.  This
 *  function is currently geared only toward support of the SMF 0
 *  channel-splitting feature.  Many of the member are not set to useful value
 *  when the MIDI file is read, so we don't handle them for now.
 *
 * \warning
 *      This function does not yet copy the SysEx data.  The inclusion
 *      of SysEx editable_events was not complete in Seq24, and it is still not
 *      complete in Sequencer64.  Nor does it currently bother with the
 *      links.
 *
 * \param rhs
 *      Provides the editable_event object to be assigned.
 *
 * \return
 *      Returns a reference to "this" object, to support the serial assignment
 *      of editable_events.
 *
editable_event &
editable_event::operator = (const editable_event & rhs)
{
    if (this != &rhs)
    {
        event::operator =(rhs);
        m_category          = rhs.m_category;
    //  m_parent            = rhs.m_parent;
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
 *
 */

/**
 * \setter m_category by value
 *      Also keeps the m_name_category member in synchrony.  Note that a bad
 *      value is translated to the value of category_name.
 */

void
editable_event::category (category_t c)
{
    if (c >= category_channel_message && c <= category_prop_event)
        m_category = c;
    else
        m_category = category_name;

    m_name_category = value_to_name(c, category_name);
}

/**
 * \setter m_category by value
 *      Also keeps the m_name_category member in synchrony, but looks up the
 *      name, rather than using the name parameter, to avoid storing
 *      abbreviations.  Note that a bad value is translated to the value of
 *      category_name.
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
 * \param ts
 *      Provides the timestamp in units of MIDI pulses.
 *
 *  The format of the string representation is of the format selected by the
 *  m_format_timestamp member and is set by the format_timestamp() function.
 */

void
editable_event::timestamp (midipulse ts)
{
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
 *  Converts the event into a string desribing the full event.
 *  We get the time-stamp as a string, make sure the event is fully analyzed so
 *  that all items and strings are set correctly.
 */

std::string
editable_event::stock_event_string ()
{
    char temp[80];
    std::string ts = format_timestamp();
    analyze();
    snprintf
    (
        temp, sizeof temp, "%9s %-11s %-10s %-20s",
        ts.c_str(), m_name_status.c_str(),
        m_name_channel.c_str(), m_name_data.c_str()
    );
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
 *      Proprietary/SeqSpec events (0xFF with 0x2424) aren't able to be
 *      detected due to lack of context here (and due to the fact that
 *      currently such events are not yet stored in a sequence, and the
 *      least-significant-byte gets masked off anyway.)
 *
 * Status:
 *
 *      We distinguish between channel and system messages, and then one- and
 *      two-byte messages, but don't yet distinguish the data values fully.
 */

void
editable_event::analyze ()
{
    midibyte status = get_status();
    if (status >= EVENT_NOTE_OFF && status <= EVENT_PITCH_WHEEL)
    {
        char tmp[32];
        midibyte channel = get_channel();
        midibyte d0, d1;
        get_data(d0, d1);
        category(category_channel_message);
        status = get_status() & EVENT_CLEAR_CHAN_MASK;

        /*
         * Get channel message name (e.g. "Program change");
         */

        m_name_status = value_to_name(status, category_channel_message);
        snprintf(tmp, sizeof tmp, "Channel %d", int(channel));
        m_name_channel = std::string(tmp);
        if (is_one_byte_msg(status))
            snprintf(tmp, sizeof tmp, "Data = { %d }", int(d0));
        else
        {
            if (is_note_msg(status))
                snprintf(tmp, sizeof tmp, "Key %d Velocity %d", int(d0), int(d1));
            else
                snprintf(tmp, sizeof tmp, "Data = { %d, %d }", int(d0), int(d1));
        }
        m_name_data = std::string(tmp);
    }
    else if (status >= EVENT_MIDI_SYSEX)    //  && status <= EVENT_MIDI_RESET
    {
        category(category_system_message);

        /*
         * Get system message name (e.g. "SysEx start");
         */

        m_name_status = value_to_name(status, category_system_message);
        m_name_channel.clear();
        m_name_data.clear();
    }
    else
    {
        // Would try to detect SysEx versus Meta message versus SeqSpec here.
        // Then set either m_name_meta and/or m_name_seqspec
    }

}

}           // namespace seq64

/*
 * editable_event.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

