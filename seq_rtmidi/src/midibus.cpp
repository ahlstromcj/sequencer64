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
 * \updates       2016-12-01
 * \license       GNU GPLv2 or above
 *
 *  This file provides a cross-platform implementation of the midibus class.
 *  Based on our refactored version of the RtMidi project included in this
 *  library.
 *
 *  PortMidi function calls to replace with RtMidi work-arounds:
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
 *
 *  What's the difference between a regular port and a virtual port?
 */

#include "event.hpp"                    /* seq64::event and macros          */
#include "midibus_rm.hpp"               /* seq64::midibus for rtmidi        */

/**
 *  A monaifest constant for the RtMidi version of the api_poll_for_midi()
 *  function.
 */

#define SEQ64_RTMIDI_POLL_FOR_MIDI_SLEEP_MS     10

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
    int ppqn,
    bool makevirtual
) :
    midibase
    (
        clientname, portname, bus_id, port_id, queue, ppqn, makevirtual
    ),
    m_rt_midi       (nullptr)
{
    // Empty body
}

/**
 *  The destructor closes out the RtMidi MIDI infrastructure.
 */

midibus::~midibus ()
{
    if (not_nullptr(m_rt_midi))
    {
        delete m_rt_midi;           // Pm_Close(m_rt_midi)
        m_rt_midi = nullptr;
    }
}

/**
 *  Polls for MIDI events.  This is the API implementation for RtMidi.
 *
 *  NOTE:  This should work only for input busses, so we need to insure this
 *  at some point!!!!!!!!!!!!!!
 *
 *  NOT TRUE:
 *
 *  It tests that the queue number (formerly m_pm) is valid first.  It assumes
 *  that the RtMidi pointer m_rt_midi is valid, for speed.
 *
 * \return
 *      Returns 0 if the polling succeeded, and 1 if it failed.
 */

int
midibus::api_poll_for_midi ()
{
    std::vector<midibyte> msg;
    double stamp = m_rt_midi->get_message(msg);
    if (stamp > 0.0)
    {
        int nbytes = msg.size();
        millisleep(SEQ64_RTMIDI_POLL_FOR_MIDI_SLEEP_MS);
        return (nbytes == 0) ? 1 : 0;
    }
    else
        return 0;
}

/**
 *  Initializes the MIDI output port, for RtMidi.
 *
 *  Currently, we use the default values for the rtmidi API, the queue number,
 *  and the queue size.
 *
 *  -#  If desired, one can do midiout->open_virtual_port().
 *  -#  Or unsigned numberofports = midiout->get_port_count().
 *  -#  std::string portname = midiin->get_port_name(portnumber);
 *  -#  Finally, midiout->open_port().
 *
 *  One question is how does RtMidi distinguish between input and output
 *  ports?  Right now, the input and output init functions are basically
 *  identical.  The only different is that one creates an rtmidi_in
 *  object, and the other creates an rtmidi_out object.  So we will
 *  consolidate the common parts.
 *
 * \return
 *      Returns true if the output port was successfully opened.
 */

bool
midibus::api_init_out ()
{
    bool result = true;
    try
    {
        m_rt_midi = new rtmidi_out(RTMIDI_API_UNSPECIFIED, connect_name());
        result = api_init(m_rt_midi);
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

bool
midibus::api_init_in ()
{
    bool result = true;
    try
    {
        m_rt_midi = new rtmidi_in(RTMIDI_API_UNSPECIFIED, connect_name());
        result = api_init(m_rt_midi);
    }
    catch (const rterror & err)
    {
        err.print_message();
        result = false;
    }
    return result;
}

/**
 *  Common code for api_init_in() and api_init_out().  The caller sets up the
 *  try-catch block.
 *
 * \param rtm
 *      Provides the rt_midi_in or rt_midi_out object to be initialized.
 *
 * \return
 *      Returns true if the input/output port was successfully opened.
 */

bool
midibus::api_init (rtmidi * rtm)
{
    bool result = true;
    if (is_virtual_port())
    {
        rtm->open_virtual_port(port_name());
    }
    else
    {
        unsigned portcount = rtm->get_port_count();
        if (get_port_id() <= int(portcount))            /* numbering re 1 */
        {
            std::string portname = rtm->get_port_name(get_port_id());
            if (! portname.empty())
                port_name(portname);

            rtm->open_port(get_port_id());

            /*
             * Set up not to ignore SysEx, timing, or active-sensing messages.
             */

            rtm->ignore_types(false, false, false);
        }
        else
            result = false;
    }
    return result;
}

/**
 *  Takes a native event, and encodes to a Windows message, and writes it to
 *  the queue.  It fills a small byte buffer, sets the MIDI channel, make a
 *  message of it, and writes the message.
 *
 *  Again, DO WE NEED to distinguish between input and output here?
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
    std::vector<midibyte> msg;
    midibyte d0, d1;
    midibyte status = e24->get_status();
    status += (channel & 0x0F);
    e24->get_data(d0, d1);
    msg.push_back(status);
    msg.push_back(d0);
    msg.push_back(d1);
    m_rt_midi->send_message(msg);
}

/**
 *  Continue from the given tick.  This function implements only the
 *  RtMidi-specific code.
 *
 *  Note that, unlike in PortMidi, here we do not deal with zeroing the event
 *  timestamp.
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
    std::vector<midibyte> msg;
    midibyte d0 = (beats & 0x3F80) >> 7;
    midibyte d1 = (beats & 0x7F);
    msg.push_back(EVENT_MIDI_CONTINUE);
    msg.push_back(0);
    msg.push_back(0);
    m_rt_midi->send_message(msg);
    msg.clear();
    msg.push_back(EVENT_MIDI_SONG_POS);
    msg.push_back(d0);
    msg.push_back(d1);
    m_rt_midi->send_message(msg);
}

/**
 *  Sets the MIDI clock a-runnin', if the clock type is not e_clock_off.
 *  This function is called by midibase::start().  No timestamp handling.
 */

void
midibus::api_start ()
{
    std::vector<midibyte> msg;
    msg.push_back(EVENT_MIDI_START);
    msg.push_back(0);
    msg.push_back(0);
    m_rt_midi->send_message(msg);
}

/**
 *  Stops the MIDI clock, if the clock-type is not e_clock_off.
 *  This function is called by midibase::stop().  No timestamp handling.
 */

void
midibus::api_stop ()
{
    std::vector<midibyte> msg;
    msg.push_back(EVENT_MIDI_STOP);
    msg.push_back(0);
    msg.push_back(0);
    m_rt_midi->send_message(msg);
}

/**
 *  Generates MIDI clock.  This function is called by midibase::clock().  No
 *  timestamp handling.
 *
 * \param tick
 *      The clock tick value, not used in the API implementation of this
 *      function for RtMidi.
 */

void
midibus::api_clock (midipulse /* tick */)
{
    std::vector<midibyte> msg;
    msg.push_back(EVENT_MIDI_CLOCK);
    msg.push_back(0);
    msg.push_back(0);
    m_rt_midi->send_message(msg);
}

}           // namespace seq64

/*
 * midibus.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
