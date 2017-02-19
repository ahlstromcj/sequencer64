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
 *  the rtmidi frameworks.
 *
 * \library       sequencer64 application
 * \author        Chris Ahlstrom
 * \date          2016-11-21
 * \updates       2017-02-19
 * \license       GNU GPLv2 or above
 *
 *  This file provides a cross-platform implementation of the midibus class.
 *  Based on our super-heavily refactored version of the RtMidi project
 *  included in this library.  Currently only ALSA and JACK are supported.
 */

#include "event.hpp"                    /* seq64::event and macros          */
#include "midibus_rm.hpp"               /* seq64::midibus for rtmidi        */
#include "rtmidi.hpp"                   /* RtMidi updated API header file   */
#include "rtmidi_info.hpp"              /* seq64::rtmidi_info (new)         */
#include "settings.hpp"                 /* seq64::rc_settings ...           */

/**
 *  A manifest constant for the RtMidi version of the api_poll_for_midi()
 *  function.
 */

#define SEQ64_RTMIDI_POLL_FOR_MIDI_SLEEP_MS     10

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *  Normal-port and virtual-port constructor.
 *
 * \param rt
 *      Provides the rtmidi_info object to use to obtain the
 *      client ID (buss ID), port ID, and port name, as obtained via calls to
 *      the ALSA, JACK, Core MIDI, or Windows MM subsystems.
 *      We need it to provide the single ALSA "handle" needed in the in
 *      Sequencer64 buss model, where the master MIDI buss provides it to be
 *      used by all the MIDI buss objects.
 *
 * \param appname
 *      The application name needed to construct a good bus/port name.
 *
 * \param index
 *      This is the index into the rtmidi object, and is used to get the
 *      desired client and port information.  It is an index into the
 *      info container held by the rtmidi object.
 *
 * \param makevirtual
 *      Indicates that the port is virtual, as opposed to normal.
 *
 * \param isinput
 *      Indicates that the port is an input port, as opposed to an output port.
 *
 * \param bus_id
 *      Optional buss ID, if not equal to the index parameter.
 */

midibus::midibus
(
    rtmidi_info & rt,
    int index,
    bool makevirtual,
    bool isinput,
    int bussoverride,
    bool makesystem
) :
    midibase
    (
        rt.app_name(),
        rt.get_bus_name(index),
        rt.get_port_name(index),
        index,
        bussoverride != SEQ64_NO_BUS ? bussoverride : rt.get_bus_id(index),
        index,
        rt.global_queue(),
        rt.ppqn(), rt.bpm(),
        makevirtual,
        isinput,
        makesystem
    ),
    m_rt_midi       (nullptr),
    m_master_info   (rt)
{
    if (makevirtual)
    {
        if (bus_name().empty())
            bus_name(rc().application_name());

        if (port_name().empty())
        {
            std::string pname = rc().application_name();
            pname += " midi ";
            pname += isinput ? "in " : "out ";
            pname += std::to_string(get_port_id());
            port_name(pname);
        }
    }

    int portcount = rt.get_port_count();
    if (index < portcount)
    {
        int id = rt.get_port_id(index);
        if (id >= 0)
            set_port_id(id);

        id = rt.get_bus_id(index);
        if (id >= 0)
            set_bus_id(id);

        set_name                /* change what was set in base class    */
        (
            rt.app_name(), rt.get_bus_name(index), rt.get_port_name(index)
        );
    }

#ifdef SEQ64_SHOW_API_CALLS
    printf("midibus '%s' created\n", display_name().c_str());
#endif

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
 *  Connects to another port.  If the port is an input port, but is not
 *  configured (by the user or the "rc" configuration file), then it is not
 *  connected, and this is not an error.  Output ports are always connected.
 *
 *  Note that the api_connect() function will errprint is own errors.  But if
 *  we expected to be able to connect, and have a null rt_midi pointer, then
 *  this is a reported error.
 *
 * \return
 *      Returns true if the connection was made succuessfully, or there was no
 *      need to make the connection.
 */

bool
midibus::api_connect ()
{
    bool result = true;
    if (get_input() || is_output_port())
    {
        result = not_nullptr(m_rt_midi);
        if (result)
        {
            result = m_rt_midi->api_connect();
        }
        else
        {
            char temp[80];
            snprintf
            (
                temp, sizeof temp, "null rtmidi pointer, port '%s'",
                display_name().c_str()
            );
            errprintfunc(temp);
        }
    }
    return result;
}

/**
 *  Polls for MIDI events.  This is the API implementation for RtMidi.
 *
 *  This should work only for input busses, so we need to insure this at some
 *  point.  Currently, this is the domain of the master bus.  We also should
 *  make this routine just check the input queue size and then read the queue.
 *  Note that the ALSA handle checks incoming MIDI events and either passes
 *  them to the callback function or pushes them onto the input queue.
 *
 * \return
 *      Returns 0 if the polling succeeded, and 1 if it failed.  If the buss
 *      hasn't been initialized, it has a null m_rt_midi pointer, and will
 *      return 0.  This can happen normally when a MIDI input port is
 *      configured to be disabled.
 */

int
midibus::api_poll_for_midi ()
{
    return not_nullptr(m_rt_midi) ? m_rt_midi->api_poll_for_midi() : 0 ;
}

/**
 *  Gets a MIDI event.
 *
 * \param inev
 *      The location to deposit the MIDI event data.
 *
 * \return
 *      Returns true if a MIDI event was obtained.
 */

bool
midibus::api_get_midi_event (event * inev)
{
    return not_nullptr(m_rt_midi) ? m_rt_midi->api_get_midi_event(inev) : false ;
}

/**
 *  Initializes the MIDI output port.
 *
 *  Currently, we use the default values for the rtmidi API, the queue number,
 *  and the queue size.
 *
 * \return
 *      Returns true if the output port was successfully opened.
 */

bool
midibus::api_init_out ()
{
    bool result = false;
    try
    {
        m_rt_midi = new rtmidi_out(*this, m_master_info);
        result = m_rt_midi->api_init_out();
    }
    catch (const rterror & err)
    {
        err.print_message();
    }
    return result;
}

/**
 *  Initializes the MIDI virtual output port.
 *
 * \return
 *      Returns true if the output port was successfully opened.
 */

bool
midibus::api_init_out_sub ()
{
    bool result = false;
    try
    {
        m_rt_midi = new rtmidi_out(*this, m_master_info);
        result = m_rt_midi->api_init_out_sub();
    }
    catch (const rterror & err)
    {
        err.print_message();
    }
    return result;
}

/**
 *  Initializes the MIDI input port.
 *
 * \return
 *      Returns true if the input port was successfully opened.
 */

bool
midibus::api_init_in ()
{
    bool result = false;
    try
    {
        m_rt_midi = new rtmidi_in(*this, m_master_info);
        result = m_rt_midi->api_init_in();
    }
    catch (const rterror & err)
    {
        err.print_message();
    }
    return result;
}

/**
 *  Initializes the MIDI virtual input port.
 *
 * \return
 *      Returns true if the input port was successfully opened.
 */

bool
midibus::api_init_in_sub ()
{
    bool result = false;
    try
    {
        m_rt_midi = new rtmidi_in(*this, m_master_info);
        result = m_rt_midi->api_init_in_sub();
    }
    catch (const rterror & err)
    {
        err.print_message();
    }
    return result;
}

/**
 *  Forwards the de-initialization call to the API object that implements
 *  it.  We don't bother checking the m_rt_midi pointer.  If it is null,
 *  it is the programmer's fault.
 *
 * \return
 *      Returns the result of the m_rt_midi->api_deinit_in() call.
 */

bool
midibus::api_deinit_in ()
{
    return m_rt_midi->api_deinit_in();
}

/**
 *  Takes a native event, and encodes to a Windows message, and writes it to
 *  the queue.  It fills a small byte buffer, sets the MIDI channel, make a
 *  message of it, and writes the message.
 *
 *  Again, DO WE NEED to distinguish between input and output here?
 *
 *  Note that we're doing a double-forwarding here, which may lower
 *  throughput.
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
    m_rt_midi->api_play(e24, channel);
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
midibus::api_continue_from (midipulse tick, midipulse beats)
{
    m_rt_midi->api_continue_from(tick, beats);
}

/**
 *  Sets the MIDI clock a-runnin', if the clock type is not e_clock_off.
 *  This function is called by midibase::start().  No timestamp handling.
 */

void
midibus::api_start ()
{
    m_rt_midi->api_start();
}

/**
 *  Stops the MIDI clock, if the clock-type is not e_clock_off.
 *  This function is called by midibase::stop().  No timestamp handling.
 */

void
midibus::api_stop ()
{
    m_rt_midi->api_stop();
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
midibus::api_clock (midipulse tick)
{
    m_rt_midi->api_clock(tick);
}

}           // namespace seq64

/*
 * midibus.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
