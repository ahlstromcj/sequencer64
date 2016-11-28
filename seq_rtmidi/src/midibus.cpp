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
 * \file          midibus.cpp
 *
 *  This module declares/defines the base class for MIDI I/O under one of
 *  Windows' audio frameworks.
 *
 * \library       sequencer64 application
 * \author        Chris Ahlstrom
 * \date          2016-11-21
 * \updates       2016-11-26
 * \license       GNU GPLv2 or above
 *
 *  This file provides a cross-platform implementation of the midibus class.
 *  Based initially on the PortMidi version which has the following
 *  "features":
 *
 *      -   No concept of a buss-name or a port-name, though it does have a
 *          client-name.  The ALSA version has an ID, a client address, a
 *          client port, and a user-configurable alias.
 *      -   It has a poll_for_midi() function.
 *      -   It does not have the following functions:
 *          -   init_out_sub()
 *          -   init_in_sub()
 *          -   deinit_in()
 *
 *  PortMidi function calls to replace:
 *
 *      -   Pm_Close()
 *      -   Pm_Poll()
 *      -   Pm_OpenOutput()
 *      -   Pm_OpenInput()
 *      -   Pm_Message()
 *      -   Pm_Write()
 *
 *  Now let's survey the test files of the rtmidi project to determine what
 *  functions we need to use.
 *
 *      -   MIDI input.
 *          -#  Create an rtmidi_in object, midiin.
 *          -#  MIDI input.  Need to define a callback of the format
 *              c(double delta, std::vector<midibyte> * msg, void * userdata).
 *              Set it with midiin->set_callback(&mycallback).
 *          -#  Set status to ignore sysex, timing, or active sensing messages:
 *              midiin->ignoreTypes(false, false, false); that is, don't
 *              ignore.
 *          -#  If desired, one can do midiin->open_virtual_port().
 *          -#  Otherwise: unsigned numberofports = midiin->get_port_count().
 *          -#  std::string portname = midiin->get_port_name(portnumber);
 *          -#  Finally, midiin->open_port().
 *          -#  To get messages:
 *              -   A polling loop: stamp = midiin->get_message().
 *              -   Use the callback to process each message.
 *      -   MIDI clock in/out.  See rtmidi-master/tests/midiclock.cpp.
 *          Lots of push_back() of message bytes followed by send_message()
 *          for the MIDI-clock out.
 *      -   MIDI output.
 *          -#  Create an rtmidi_out object, midiout.
 *          -#  If desired, one can do midiout->open_virtual_port().
 *          -#  Otherwise: unsigned numberofports = midiout->get_port_count().
 *          -#  std::string portname = midiin->get_port_name(portnumber);
 *          -#  Finally, midiout->open_port().
 *          -#  To send a message, push_back() the message bytes (or use array
 *              notation).  Then midiout->send_message().
 */

#include "event.hpp"                    /* seq64::event and macros          */
#include "midibus.hpp"                  /* seq64::midibus for rtmidi        */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *  Principal constructor.
 */

midibus::midibus
(
    const std::string & clientname,
    const std::string & portname,
    int bus_id,
    int port_id,
    int queue,
    int ppqn
 :
    midibase        (clientname, portname, bus_id, port_id, queue, ppqn),
    m_rt_midi       (nullptr)
{
    /*
     * Copy the client names.
     *
    char tmp[64];
    snprintf(tmp, sizeof(tmp), "[%d] %s", m_id, client_name);
    m_name = tmp;
     *
     */
}

/**
 *  The destructor closes out the Windows MIDI infrastructure.
 */

midibus::~midibus ()
{
    if (not_nullptr(m_rt_midi))
    {
//      Pm_Close(m_rt_midi);
        m_rt_midi = nullptr;
    }
}

/**
 *  Polls for MIDI events.  This is the API implementation for RtMidi.
 *  It tests that the queue number (formerly m_pm) is valid first.  It assumes
 *  that the RtMidi pointer m_rt_midi is valid, for speed.
 *
 * \return
 *      Returns 0 if the polling succeeded, and 1 if it failed.
 */

int
midibus::api_poll_for_midi ()
{
    return 0;
}

/**
 *  Initializes the MIDI output port, for RtMidi.
 *
 *  Currently, we use the default values for the rtmidi API, the queue number,
 *  and the queue size.
 *
 * \return
 *      Returns true if the output port was successfully opened.
 */

bool midibus::api_init_out ()
{
    bool result = true;
    try
    {
        m_rt_midi = new rtmidi_out(RTMIDI_API_UNSPECIFIED, connect_name());
    }
    catch (const rterror & err)
    {
        err.print_message();
        result = false;
    }
    return result;
}

/**
 *  Initializes the MIDI input port, for RtMidi.
 *
 * \return
 *      Returns true if the input port was successfully opened.
 */

bool midibus::api_init_in ()
{
    bool result = true;
    try
    {
        m_rt_midi = new rtmidi_in(RTMIDI_API_UNSPECIFIED, connect_name());
    }
    catch (const rterror & err)
    {
        err.print_message();
        result = false;
    }
    return result;
}

/**
 *  Takes a native event, and encodes to a Windows message, and writes it to
 *  the queue.  It fills a small byte buffer, sets the MIDI channel, make a
 *  message of it, and writes the message.
 *
 * \param e24
 *      The MIDI event to play.
 *
 * \param channel
 *      The channel on which to play the event.
 */

void
midibus::api_play (event * e24, midibyte channel)
{
    midibyte buffer[3];                /* temp for midi data */
    buffer[0] = e24->get_status();
    buffer[0] += (channel & 0x0F);
    e24->get_data(buffer[1], buffer[2]);

    PmEvent event;
    event.timestamp = 0;
    event.message = Pm_Message(buffer[0], buffer[1], buffer[2]);
    /* PmError err = */ Pm_Write(m_rt_midi, &event, 1);
}

/**
 *  Continue from the given tick.  This function implements only the
 *  RtMidi-specific code.
 *
 * \param tick
 *      The tick to continue from; unused in the RtMidi API implementation.
 *
 * \param beats
 *      The calculated beats.  This calculation is made in the
 *      midibase::continue_from() function.
 */

void
midibus::api_continue_from (midipulse /* tick */, midipulse beats)
{
    PmEvent event;
    event.timestamp = 0;
    event.message = Pm_Message(EVENT_MIDI_CONTINUE, 0, 0);
    Pm_Write(m_rt_midi, &event, 1);
    event.message = Pm_Message
    (
        EVENT_MIDI_SONG_POS, (beats & 0x3F80 >> 7), (beats & 0x7F)
    );
    Pm_Write(m_rt_midi, &event, 1);
}

/**
 *  Sets the MIDI clock a-runnin', if the clock type is not e_clock_off.
 *  This function is called by midibase::start().
 */

void
midibus::api_start ()
{
    PmEvent event;
    event.timestamp = 0;
    event.message = Pm_Message(EVENT_MIDI_START, 0, 0);
    Pm_Write(m_rt_midi, &event, 1);
}

/**
 *  Stops the MIDI clock, if the clock-type is not e_clock_off.
 *  This function is called by midibase::stop().
 */

void
midibus::api_stop ()
{
    PmEvent event;
    event.timestamp = 0;
    event.message = Pm_Message(EVENT_MIDI_STOP, 0, 0);
    Pm_Write(m_rt_midi, &event, 1);
}

/**
 *  Generates MIDI clock.
 *  This function is called by midibase::clock().
 *
 * \param tick
 *      The clock tick value, not used in the API implementation of this
 *      function for RtMidi.
 */

void
midibus::api_clock (midipulse /* tick */)
{
    PmEvent event;
    event.timestamp = 0;
    event.message = Pm_Message(EVENT_MIDI_CLOCK, 0, 0);
    Pm_Write(m_rt_midi, &event, 1);
}

}           // namespace seq64

/*
 * midibus.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
