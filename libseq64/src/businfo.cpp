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
 * \file          businfo.cpp
 *
 *  This module declares/defines the base class for handling MIDI I/O via
 *  the ALSA system.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2016-12-31
 * \updates       2017-01-22
 * \license       GNU GPLv2 or above
 *
 *  This file provides a base-class implementation for various master MIDI
 *  buss support classes.  There is a lot of common code between these MIDI
 *  buss classes.
 */

#include "easy_macros.h"
#include "businfo.hpp"                  /* seq64::businfo           */
#include "event.hpp"                    /* seq64::event             */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{

/*
 * class businfo
 */

/**
 *  A new class to consolidate a number of bus-related arrays into one array.
 *  There will be in input instance and an output instance of this object
 *  contained by mastermidibus.
 */

businfo::businfo ()
 :
    m_bus           (nullptr),
    m_active        (false),
    m_initialized   (false),
    m_init_clock    (e_clock_off),
    m_init_input    (false)
{
    // Empty body
}

/**
 *  Principal constructor.
 *
 * is_input_port():
 *
 *      Indicates if the midibus represents an input port (true) versus an
 *      output port (false).
 *
 * is_virtual_port():
 *
 *      Indicates if the midibus represents a virtual port (true) versus a
 *      normal port (false).
 */

businfo::businfo (midibus * bus)
 :
    m_bus           (bus),
    m_active        (false),
    m_initialized   (false),
    m_init_clock    (e_clock_off),
    m_init_input    (false)
{
    if (not_nullptr(bus))
    {
        bool ok;
        if (bus->is_input_port())
        {
            if (bus->is_virtual_port())
                ok = m_bus->init_in_sub();
            else
            {
                /*
                 * \note
                 *      This call is commented out in seq64!  For non-virtual
                 *      "input" ports, we probably want to create an output
                 *      port to connect to the input port.  TO DO!
                 */

                ok = m_bus->init_in();
            }
        }
        else
        {
            if (bus->is_virtual_port())
                ok = m_bus->init_out_sub();
            else
            {
                /*
                 * \note
                 *      This call is commented out in seq64!  For non-virtual
                 *      "output" ports, we probably want to create an input
                 *      port to connect to the output port.  TO DO!
                 */

                ok = m_bus->init_out();
            }
        }
        if (ok)
            activate();
    }
    else
    {
        errprint("businfo(): null midibus pointer provided");
    }
}

/**
 *  Copy constructor.  Currently it does not replicate the pointed-to object.
 */

businfo::businfo (const businfo & rhs)
 :
    m_bus           (rhs.m_bus),
    m_active        (rhs.m_active),
    m_initialized   (rhs.m_initialized),
    m_init_clock    (rhs.m_init_clock),
    m_init_input    (rhs.m_init_input)
{
    // No code needed
}

/*
 * class busarray
 */

/**
 *  A new class to hold a number of MIDI busses and flags for more controlled
 *  access than using arrays of booleans and pointers.
 */

busarray::busarray ()
 :
    m_container     ()
{
    // Empty body
}

busarray::~busarray ()
{
    std::vector<businfo>::iterator bi;
    for (bi = m_container.begin(); bi != m_container.end(); ++bi)
        bi->remove();
}

/**
 *  Creates, initializes, and adds a new midibus object to the list.
 */

bool
busarray::add (midibus * bus)
{
    size_t count = m_container.size();
    m_container.push_back(businfo(bus));
    return m_container.size() == (count + 1);
}

/**
 *  Starts all of the busses; used for output busses only.
 */

void
busarray::start ()
{
    std::vector<businfo>::iterator bi;
    for (bi = m_container.begin(); bi != m_container.end(); ++bi)
        bi->start();
}

/**
 *  Stops all of the busses; used for output busses only.
 */

void
busarray::stop ()
{
    std::vector<businfo>::iterator bi;
    for (bi = m_container.begin(); bi != m_container.end(); ++bi)
        bi->stop();
}

/**
 *  Continues from the given tick for all of the busses; used for output busses
 *  only.
 */

void
busarray::continue_from (midipulse tick)
{
    std::vector<businfo>::iterator bi;
    for (bi = m_container.begin(); bi != m_container.end(); ++bi)
        bi->continue_from(tick);
}

/**
 *  Initializes the clocking at the given tick for all of the busses; used for
 *  output busses only.
 */

void
busarray::init_clock (midipulse tick)
{
    std::vector<businfo>::iterator bi;
    for (bi = m_container.begin(); bi != m_container.end(); ++bi)
        bi->init_clock(tick);
}

/**
 *  Clocks at the given tick for all of the busses; used for output busses only.
 */

void
busarray::clock (midipulse tick)
{
    std::vector<businfo>::iterator bi;
    for (bi = m_container.begin(); bi != m_container.end(); ++bi)
        bi->clock(tick);
}

/**
 *  Handles SysEx events; used for output busses.
 */

void
busarray::sysex (event * ev)
{
    std::vector<businfo>::iterator bi;
    for (bi = m_container.begin(); bi != m_container.end(); ++bi)
        bi->sysex(ev);
}

/**
 *  Plays an event, if the bus is proper.
 */

void
busarray::play (bussbyte bus, event * e24, midibyte channel)
{
    if (bus < count() && m_container[bus].active())
        m_container[bus].bus()->play(e24, channel);
}

/**
 *  Sets the clock type for the given bus, usually the output buss.
 *  This code is a bit more restrictive than the original code in
 *  mastermidibus::set_clock().
 */

void
busarray::set_clock (bussbyte bus, clock_e clocktype)
{
    if (bus < count() && m_container[bus].active())
    {
        m_container[bus].init_clock(clocktype);
        m_container[bus].bus()->set_clock(clocktype);
    }
}

/**
 *  Sets the clock type for all busses, usually the output buss.
 */

void
busarray::set_all_clocks ()
{
    std::vector<businfo>::iterator bi;
    for (bi = m_container.begin(); bi != m_container.end(); ++bi)
    {
        bi->bus()->set_clock(bi->init_clock());
    }
}

/**
 *  Gets the clock type for the given bus, usually the output buss.
 */

clock_e
busarray::get_clock (bussbyte bus)
{
    if (bus < count() && m_container[bus].active())
        return m_container[bus].bus()->get_clock();
    else
        return e_clock_off;
}

/**
 *  Get the MIDI output buss name (i.e. the full display name) for the given
 *  (legal) buss number.
 *
 *  This function adds the retrieval of client and port numbers that are not
 *  needed in the portmidi implementation, but seem generally useful to
 *  support in all implementations.  It's main use is to display the
 *  full portname in one of two forms:
 *
 *      -   "[0] 0:0 clientname:portname"
 *      -   "[0] 0:0 portname"
 *
 *  The second version is chosen if "clientname" is already included in the
 *  port name, as many MIDI clients do that.  However, the name gets
 *  modified to reflect the remote system port to which it will connect.
 *
 * \param bus
 *      Provides the output buss number.  Checked before usage.
 *      Actually should now be an index number
 *
 * \return
 *      Returns the buss name as a standard C++ string, truncated to 80-1
 *      characters.  Also contains an indication that the buss is disconnected
 *      or unconnected.  If the buss number is illegal, this string is empty.
 */

std::string
busarray::get_midi_bus_name (int bus)
{
    std::string result;
    if (bus < count())
    {
        if (m_container[bus].active())
        {
            std::string busname = m_container[bus].bus()->bus_name();
            std::string portname = m_container[bus].bus()->port_name();
            std::size_t len = busname.size();
            int test = busname.compare(0, len, portname, 0, len);
            if (test == 0)
            {
                char tmp[80];
                snprintf
                (
                    tmp, sizeof tmp, "[%d] %d:%d %s",
                    bus, m_container[bus].bus()->get_bus_id(),
                    m_container[bus].bus()->get_port_id(), portname.c_str()
                );
                result = tmp;
            }
            else
                result = m_container[bus].bus()->display_name();
        }
        else
        {
            char tmp[80];                           /* copy names */
            std::string status = "virtual";
            if (m_container[bus].initialized())
                status = "disconnected";

            snprintf
            (
                tmp, sizeof tmp, "%s (%s)",
                m_container[bus].bus()->display_name().c_str(), status.c_str()
            );
            result = tmp;
        }
    }
    return result;
}

/**
 *  Print some information about the available MIDI output busses.
 */

void
busarray::print ()
{
    printf("Available busses:\n");
    std::vector<businfo>::iterator bi;
    for (bi = m_container.begin(); bi != m_container.end(); ++bi)
    {
        printf
        (
            "%s:%s\n",
            bi->bus()->bus_name().c_str(),
            bi->bus()->port_name().c_str()
        );
    }
}

/**
 *  Turn off the given port for the given client.  Both the busses for the given
 *  client are stopped: that is, set to inactive.
 *
 *  This function is called by api_get_midi_event() when the ALSA event
 *  SND_SEQ_EVENT_PORT_EXIT is received.  Since port_exit() has no direct
 *  API-specific code in it, we do not need to create a virtual
 *  api_port_exit() function to implement the port-exit event.
 *
 * \param client
 *      The client to be matched and acted on.  This value is actually an ALSA
 *      concept.
 *
 * \param port
 *      The port to be acted on.  Both parameter must be matched before the
 *      buss is made inactive.  This value is actually an ALSA concept.
 */

void
busarray::port_exit (int client, int port)
{
    std::vector<businfo>::iterator bi;
    for (bi = m_container.begin(); bi != m_container.end(); ++bi)
    {
        if (bi->bus()->match(client, port))
           bi->deactivate();
    }
}

/**
 *  Set the status of the given input buss, if a legal buss number.
 *  There's currently no implementation-specific API function here.
 *  This function should be used only for the input busarray, obviously.
 *
 * \threadsafe
 *
 * \param bus
 *      Provides the buss number.
 *
 * \param inputing
 *      True if the input bus will be inputting MIDI data.
 */

void
busarray::set_input (bussbyte bus, bool inputing)
{
    if (bus < count())
    {
        m_container[bus].init_input(inputing);
        if (m_container[bus].active())
            m_container[bus].bus()->set_input(inputing);
    }
}

/**
 *  Set the status of all input busses.  There's currently no
 *  implementation-specific API function here.  This function should be used
 *  only for the input busarray, obviously.
 *
 * \param inputing
 *      True if the input bus will be inputting MIDI data.
 */

void
busarray::set_all_inputs ()
{
    std::vector<businfo>::iterator bi;
    for (bi = m_container.begin(); bi != m_container.end(); ++bi)
    {
        bi->bus()->set_input(bi->init_input());
    }
}

/**
 *  Get the input for the given (legal) buss number.
 *
 *  There's currently no implementation-specific API function here.
 *
 * \param bus
 *      Provides the buss number.
 *
 * \return
 *      Always returns false.
 */

bool
busarray::get_input (bussbyte bus)
{
    if (bus < count() && m_container[bus].active())
        return m_container[bus].bus()->get_input();

    return false;
}

/**
 *  Initiate a poll() on the existing poll descriptors.  This is a
 *  primitive poll, which exits when some data is obtained.  It also applies
 *  only to the input busses.
 */

bool
busarray::poll_for_midi ()
{
    std::vector<businfo>::iterator bi;
    for (bi = m_container.begin(); bi != m_container.end(); ++bi)
    {
        if (bi->bus()->poll_for_midi())
            return true;
    }
    return false;
}

/**
 *  Provides a function to use in api_port_start(), to determine if the port
 *  is to be a "replacement" port.  This function is meant only for the output
 *  buss (so far).
 *
 *  Still need to determine exactly what this function needs to do.
 *
 * \return
 *      Returns -1 if no matching port is found, otherwise it returns the
 *      replacement-port number.
 */

int
busarray::replacement_port (int bus, int port)
{
    int result = -1;                    //  int bus_slot = count();
    int counter = 0;
    std::vector<businfo>::iterator bi;
    for (bi = m_container.begin(); bi != m_container.end(); ++bi, ++counter)
    {
        if (bi->bus()->match(bus, port) && ! bi->active())
        {
            result = counter;
            break;
        }
    }
    if (result >= 0)
    {
        if (not_nullptr(bi->bus()))
        {
#ifdef USE_NO_ERASE
            delete bi->bus();
#else
            m_container.erase(bi);      /* deletes the pointer as well */
#endif
            errprintf("port_start(): bus_out[%d] not null\n", result);
        }
    }
    return result;
}

}           // namespace seq64

/*
 * businfo.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

