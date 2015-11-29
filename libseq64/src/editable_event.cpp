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
 * \updates       2015-11-28
 * \license       GNU GPLv2 or above
 *
 *  A MIDI editable event is encapsulated by the seq64::editable_event
 *  object.
 */


#include "easy_macros.h"
#include "controllers.hpp"              /* seq64::c_controller_names[]  */
#include "editable_event.hpp"

namespace seq64
{

/**
 *
 */

editable_event::name_value_t editable_event::sm_category_names [] =
{
    { midibyte(category_channel_message),   "Channel message"   },
    { midibyte(category_system_message),    "System message"    },
    { midibyte(category_meta_event),        "Meta event"        },
    { midibyte(category_prop_event),        "Prop event"        },
    { 0,                                    ""                  }
};

/**
 *  Initializes the array of event/name pairs for all of the MIDI events.
 *  Terminated by either a null byte-code or an empty string, the latter being
 *  the preferred test, for consistency with the other arrays.
 */

editable_event::name_value_t editable_event::sm_event_names [] =
{
    { EVENT_NOTE_OFF,          "Note off"           },
    { EVENT_NOTE_ON,           "Note on"            },
    { EVENT_AFTERTOUCH,        "Aftertouch"         },
    { EVENT_CONTROL_CHANGE,    "Control change"     },
    { EVENT_PROGRAM_CHANGE,    "Program change"     },
    { EVENT_CHANNEL_PRESSURE,  "Channel pressure"   },
    { EVENT_PITCH_WHEEL,       "Pitch wheel"        },
    { EVENT_MIDI_SYSEX,        "SysEx start"        },
    { EVENT_MIDI_QUAR_FRAME,   "Quarter frame"      },
    { EVENT_MIDI_SONG_POS,     "Song position"      },
    { EVENT_MIDI_SONG_SELECT,  "Song select"        },
    { EVENT_MIDI_SONG_F4,      "F4"                 },
    { EVENT_MIDI_SONG_F5,      "F5"                 },
    { EVENT_MIDI_TUNE_SELECT,  "Tune request"       },
    { EVENT_MIDI_SYSEX_END,    "SysEx end"          },
    { EVENT_MIDI_CLOCK,        "Clock"              },
    { EVENT_MIDI_SONG_F9,      "F9"                 },
    { EVENT_MIDI_START,        "Start"              },
    { EVENT_MIDI_CONTINUE,     "Continue"           },
    { EVENT_MIDI_STOP,         "Stop"               },
    { EVENT_MIDI_SONG_FD,      "FD"                 },
    { EVENT_MIDI_ACTIVE_SENS,  "Active sensing"     },
    { EVENT_MIDI_RESET,        "Reset"              },
    { 0,                       ""                   }   // terminator
};

/**
 *  Initializes the array of event/name pairs for all of the Meta events.
 *  Terminated only by the empty string.  Well, FF is another usable marker
 *  for this one.
 */

editable_event::name_value_t editable_event::sm_meta_event_names [] =
{
    { 0x00, "Sequence number"       },
    { 0x01, "Text event"            },
    { 0x02, "Copyright notice"      },
    { 0x03, "Track name"            },
    { 0x04, "Instrument name"       },
    { 0x05, "Lyrics"                },
    { 0x06, "Marker"                },
    { 0x07, "Cue point"             },
    { 0x08, "Program name"          },
    { 0x09, "Device name"           },
    { 0x0A, "Text event 0A"         },
    { 0x0B, "Text event 0B"         },
    { 0x0C, "Text event 0C"         },
    { 0x0D, "Text event 0D"         },
    { 0x0E, "Text event 0E"         },
    { 0x0F, "Text event 0F"         },
    { 0x20, "MIDI channel"          },                     // obsolete
    { 0x21, "MIDI port"             },                     // obsolete
    { 0x2F, "End of track"          },
    { 0x51, "Set tempo"             },
    { 0x54, "SMPTE offset"          },
    { 0x58, "Time signature"        },
    { 0x59, "Key signature"         },
    { 0x7F, "Sequencer specific"    },
    { 0xFF, ""                      }                      // terminator
};

/**
 *  Initializes the array of event/name pairs for all of the
 *  seq24/sequencer64-specific events.
 *  Terminated only by the empty string.  Well, FF is another usable marker
 *  for this one.
 */

editable_event::name_value_t editable_event::sm_prop_event_names [] =
{
    { 0x01, "Buss number"           },
    { 0x02, "Channel number"        },
    { 0x03, "Clocking"              },
    { 0x04, "Old triggers"          },
    { 0x05, "Song notes"            },
    { 0x06, "Time signature"        },
    { 0x07, "Beats per minute"      },
    { 0x08, "Trigger data"          },
    { 0x09, "Song mute group data"  },
    { 0x10, "Song MIDI control"     },
    { 0x11, "Key"                   },
    { 0x12, "Scale"                 },
    { 0x13, "Background sequence"   },
    { 0xFF, ""                      }                      // terminator
};

/*
 * We will get the default controller name from the controllers module.
 * We should also be able to look up the selected buss's entries for a
 * sequence, and load up the CC/name pairs on the fly.
 */

/**
 *  This constructor simply initializes all of the class members.
 */

editable_event::editable_event ()
 :
    event           ()
{
    // Empty body
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
    event           (rhs.event)
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
 */

editable_event &
editable_event::operator = (const editable_event & rhs)
{
    if (this != &rhs)
    {
        operator =(rhs.event);
    }
    return *this;
}

/**
 *  This destructor explicitly deletes m_sysex and sets it to null.
 *  The restart_sysex() function does what we need.
 */

editable_event::~editable_event ()
{
    // Empty body
}


}           // namespace seq64

/*
 * editable_event.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

