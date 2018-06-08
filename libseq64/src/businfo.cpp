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
 * \updates       2018-05-31
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
    m_init_clock    (e_clock_off),      /* could end up disabled as well */
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
 *
 * \param bus
 *      Provides a pointer to the MIDI buss object to be represented by this
 *      object.
 */

businfo::businfo (midibus * bus)
 :
    m_bus           (bus),
    m_active        (false),
    m_initialized   (false),
    m_init_clock    (e_clock_off),      /* could end up disabled as well */
    m_init_input    (false)
{
    // See the initialize() function
}

/**
 *  Copy constructor.  Currently it does not replicate the pointed-to object.
 *
 * \param rhs
 *      The source object to be copied.
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

/**
 *  This function is called when the businfo object is added to the busarray.
 *  It relies on the perform::launch() function to actually activate() all of
 *  the ports that have been flagged as "activated" here.
 *
 * is_input_port():
 *
 *      Indicates if the midibus represents an input port (true) versus an
 *      output port (false).  The way the mastermidibus currently works, it
 *      creates the API MIDI input objects there, so it does not need to be
 *      done here.  This falls under the heading of "tricky code".
 *
 * is_virtual_port():
 *
 *      Indicates if the midibus represents a manual/virtual port (true)
 *      versus a normal port (false).
 *
 *  The rules for port initialization follow those of seq24 for MIDI busses:
 *
 *      -   Manual (virtual) input and output ports always get their init
 *          functions called.  They are unconditionally marked as "active"
 *          and "initialized".
 *      -   Normal output ports are marked as "active" and "initialized" if
 *          init_out() succeeds.
 *      -   Normal input ports don't have init_in() called, but are marked
 *          as "active" and "initialized" anyway.  The settings from the "rc"
 *          file determine which inputs will operate.
 *
 * \return
 *      Returns true if the buss is value, and it could be initialized (as an
 *      output port or a virtual output port.
 */

bool
businfo::initialize ()
{
    bool result = not_nullptr(bus());
    if (result)
    {
        /*
         *  If bus has been "disabled" (e_clock_disable), skip this port and
         *  return true.  However, we still have a potential conflict
         *  between "active", "initialized", and "disabled".
         *
         *  "Active" is used for:  enabling play(), set_clock(), get_clock(),
         *  get_midi_bus_name(), set_input(), get_input(), is_system_port(),
         *  replacement_port().
         *
         *  "Initialized" is used for: . . .
         *
         *  "Disabled" is currently used to making an OS-disabled,
         *  non-openable port a non-fatal error.
         */

        if (bus()->port_disabled())
        {
            // int bussnumber = bus()->get_bus_index();
            // result = mymasterbus.set_clock(bussnumber, e_clock_disabled);
        }
        else
        {
            if (! bus()->is_input_port())       /* not built in master bus  */
            {
                if (bus()->is_virtual_port())
                    result = bus()->init_out_sub();
                else
                    result = bus()->init_out();
            }
            if (result)
                activate();                     /* "initialized" & "active" */
        }
    }
    else
    {
        errprint("businfo(): null midibus pointer provided");
    }
    return result;
}

/**
 *  Print some information about the MIDI bus.
 */

void
businfo::print () const
{
    std::string flags;
    if (bus()->is_virtual_port())
        flags += " virtual";
    else if (bus()->is_system_port())
        flags += " system";
    else
        flags += " normal";

    if (bus()->is_input_port())
        flags += " input";
    else
        flags += " output";

    if (active())
        flags += " active";
    else
        flags += " inactive";

    if (initialized())
        flags += " initialized";
    else
        flags += " uninitialized";

    if (bus()->is_input_port())
    {
        flags += " ";
        flags += init_input() ? "inputting" : "not inputting" ;
    }
    else
    {
        flags += " clock ";
        if (init_clock() == e_clock_off)
            flags += "Off";
        else if (init_clock() == e_clock_pos)
            flags += "Pos";
        else if (init_clock() == e_clock_mod)
            flags += "Mod";
        else if (init_clock() == e_clock_disabled)
            flags += "Disabled";
        else
            flags += "illegal!";
    }
    printf
    (
        "  %s:%s %s\n",
        bus()->bus_name().c_str(), bus()->port_name().c_str(), flags.c_str()
    );
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

/**
 *  Removes components from the container.
 *
 * \question
 *  However, now that we swap containers, we cannot call this functionality,
 *  because it deletes the bus's midibus pointer and nullifies it.
 *  But we do call it, and it seems to work.
 */

busarray::~busarray ()
{
    std::vector<businfo>::iterator bi;
    for (bi = m_container.begin(); bi != m_container.end(); ++bi)
        bi->remove();
}

/**
 *  Creates and adds a new midibus object to the list.  Then the clock value
 *  is set.  This function is meant for output ports.
 *
 *  We need to belay the initialization until later, when we know the
 *  configured clock settings for the output ports.  So initialization has
 *  been removed from the constructor and moved to the initialize() function.
 *
 * \param bus
 *      The midibus to be hooked into the array of busses.
 *
 * \param clock
 *      The clocking value for the bus.
 *
 * \return
 *      Returns true if the bus was added successfully, though, really, it
 *      cannot fail.
 */

bool
busarray::add (midibus * bus, clock_e clock)
{
    size_t count = m_container.size();
    businfo b(bus);
    b.init_clock(clock);
    m_container.push_back(b);
#ifdef SEQ64_SHOW_API_CALLS
    printf
    (
        "Added output bus %s, clock %d\n",
        bus->display_name().c_str(), int(clock)
    );
#endif
    return m_container.size() == (count + 1);
}

/**
 *  Creates and adds a new midibus object to the list.  Then the inputing value
 *  is set.  This function is meant for input ports.
 *
 *  We need to belay the initialization until later, when we know the
 *  configured inputing settings for the input ports.  So initialization has
 *  been removed from the constructor and moved to the initialize() function.
 *  However, now we know the configured status and can apply it right away.
 *
 * \param bus
 *      The midibus to be hooked into the array of busses.
 *
 * \param inputing
 *      The input flag value for the bus.  If true, this value indicates that
 *      the user has selected this bus to be the input MIDI bus.
 *
 * \return
 *      Returns true if the bus was added successfully, though, really, it
 *      cannot fail.
 */

bool
busarray::add (midibus * bus, bool inputing)
{
    size_t count = m_container.size();
    businfo b(bus);

    /*
     * If we do this here, the copy of b never gets set!
     *
     * m_container.push_back(b);
     */

    if (inputing)
    {
        bool was_inputing = bus->get_input();
        if (! was_inputing)
            bus->set_input(inputing);       /* will call init_in()          */
    }
    b.init_input(inputing);                 /* just sets the flag (again)   */
    m_container.push_back(b);               /* now we can push a copy       */
#ifdef SEQ64_SHOW_API_CALLS
    printf
    (
        "Added input bus %s, inputing = %s\n",
        bus->display_name().c_str(), inputing ? "yes" : "no"
    );
#endif
    return m_container.size() == (count + 1);
}

/**
 *  Initializes all busses.  Not sure we need this function.
 *
 * \return
 *      Returns true if all busses initialized successfully.
 */

bool
busarray::initialize ()
{
    bool result = true;
    std::vector<businfo>::iterator bi;
    for (bi = m_container.begin(); bi != m_container.end(); ++bi)
    {
        if (! bi->initialize())
            result = false;
    }
    return result;
}

/**
 *  Starts all of the busses; used for output busses only, but no check is
 *  made at present.
 */

void
busarray::start ()
{
    std::vector<businfo>::iterator bi;
    for (bi = m_container.begin(); bi != m_container.end(); ++bi)
        bi->start();
}

/**
 *  Stops all of the busses; used for output busses only, but no check is made
 *  at present.
 */

void
busarray::stop ()
{
    std::vector<businfo>::iterator bi;
    for (bi = m_container.begin(); bi != m_container.end(); ++bi)
        bi->stop();
}

/**
 *  Continues from the given tick for all of the busses; used for output
 *  busses only.
 *
 * \param tick
 *      Provides the tick value for all busses to continue from.
 */

void
busarray::continue_from (midipulse tick)
{
    std::vector<businfo>::iterator bi;
    for (bi = m_container.begin(); bi != m_container.end(); ++bi)
        bi->continue_from(tick);
}

/**
 *  Initializes the clocking at the given tick for all of the busses; used
 *  for output busses only.
 *
 * \param tick
 *      Provides the tick value for all busses use as the clock tick.
 */

void
busarray::init_clock (midipulse tick)
{
    std::vector<businfo>::iterator bi;
    for (bi = m_container.begin(); bi != m_container.end(); ++bi)
        bi->init_clock(tick);
}

/**
 *  Clocks at the given tick for all of the busses; used for output busses
 *  only.
 *
 * \param tick
 *      Provides the tick value for all busses use as the clock tick.
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
 *
 * \param ev
 *      Provides the SysEx event to handle.
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
 *
 * \param bus
 *      The MIDI buss on which to play the event.  The buss number must be
 *      valid (in range) and the bus must be active.
 *
 * \param e24
 *      A pointer to the event to be played.
 *
 * \param channel
 *      The MIDI channel on which to play the event.  Sequencer64 controls
 *      the actual channel of playback, no matter what the channel specified
 *      in the event.
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
 *
 * \param bus
 *      The MIDI bus for which the clock is to be set.
 *
 * \param clocktype
 *      Provides the type of clocking for the buss.
 *
 * \return
 *      Returns true if the change was made.
 */

bool
busarray::set_clock (bussbyte bus, clock_e clocktype)
{
    /*
     * Getting the current clock setting is essentially equivalent to:
     *
     *      m_container[bus].bus()->get_clock();
     */

    clock_e current = get_clock(bus);
    bool result = bus < count() && current != clocktype;
    if (result)
    {
        result = m_container[bus].active() || current == e_clock_disabled;
        if (result)
        {
            m_container[bus].init_clock(clocktype);

            /*
             * Already done in the call above.
             *
             * m_container[bus].bus()->set_clock(clocktype);
             */
        }
    }
    return result;
}

/**
 *  Sets the clock type for all busses, usually the output buss.
 *  Note that the settings to apply are added when the add() call is made.
 */

void
busarray::set_all_clocks ()
{
    std::vector<businfo>::iterator bi;
    for (bi = m_container.begin(); bi != m_container.end(); ++bi)
        bi->bus()->set_clock(bi->init_clock());
}

/**
 *  Gets the clock type for the given bus, usually the output buss.
 *
 * \param bus
 *      The MIDI bus for which the clock is to be set.
 *
 * \return
 *      Returns the clock value set for the desired buss.  If the buss is
 *      invalid, e_clock_off is returned.  If the buss is not active, we still
 *      get the existing clock value.  The theory here is that we don't want
 *      to junk the current clock value; it could alter what was read from the
 *      "rc" file.
 */

clock_e
busarray::get_clock (bussbyte bus)
{
    if (bus < count())
    {
#ifdef USE_ACTIVITY_FLAG_CHECK
        if (m_container[bus].active())
            return m_container[bus].bus()->get_clock();
        else
            return e_clock_disabled;
#else
        return m_container[bus].bus()->get_clock();
#endif
    }
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
        clock_e current = get_clock(bus);
        if (m_container[bus].active() || current == e_clock_disabled)
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

            if (m_container[bus].bus()->port_disabled())
                status = "disabled";

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
 *  Print some information about the available MIDI input or output busses.
 */

void
busarray::print () const
{
    printf("Available busses:\n");
    std::vector<businfo>::const_iterator bi;
    for (bi = m_container.begin(); bi != m_container.end(); ++bi)
        bi->print();
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
 *  Set the status of the given input buss, if a legal buss number.  There's
 *  currently no implementation-specific API function called directly here.
 *  What happens is that midibase::set_input() uses the \a inputing parameter
 *  to decide whether to call init_in() or deinit_in(), and these functions
 *  ultimately lead to an API specific called.
 *
 *  Note that the call to midibase::set_input() will set its m_inputing flag,
 *  and then call init_in() or deinit_in() if that flag changed. This change
 *  is important, so we have to call midibase::set_input() first. Then the
 *  call to businfo::init_input() will set that flag again (plus another
 *  flag).  A bit confusing in sequence and in function naming.
 *
 *  This function should be used only for the input busarray, obviously.
 *
 * \threadsafe
 *
 * \param bus
 *      Provides the buss number.
 *
 * \param inputing
 *      True if the input bus will be inputting MIDI data.
 *
 * \return
 *      Returns true if the buss number is valid and was active, and so could
 *      be set.
 */

bool
busarray::set_input (bussbyte bus, bool inputing)
{
    bool result = bus < count();
    if (result)
    {
        if (m_container[bus].active())
            result = m_container[bus].bus()->set_input(inputing);

        m_container[bus].init_input(inputing);
    }
    return result;
}

/**
 *  Set the status of all input busses.  There's no implementation-specific
 *  API function here.  This function should be used only for the input
 *  busarray, obviously.  Note that the input settings used here were stored
 *  when the add() function was called.  They can be changed by the user via
 *  the Options / MIDI Input tab.
 */

void
busarray::set_all_inputs ()
{
    std::vector<businfo>::iterator bi;
    for (bi = m_container.begin(); bi != m_container.end(); ++bi)
        bi->bus()->set_input(bi->init_input());
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
 *      If the buss is a system buss, always returns true.  Otherwise, if the
 *      buss is inactive, returns false. Otherwise, the buss's get_input()
 *      status is returned.
 */

bool
busarray::get_input (bussbyte bus)
{
    bool result = false;
    if (bus < count() && m_container[bus].active())
    {
        if (m_container[bus].bus()->is_system_port())
            result = true;
        else
            result = m_container[bus].bus()->get_input();
    }
    return result;
}

/**
 *  Get the system-port status for the given (legal) buss number.
 *
 * \param bus
 *      Provides the buss number.
 *
 * \return
 *      Returns the selected buss's is-system-port status.  If the buss number
 *      is out of range, then false is returned.
 */

bool
busarray::is_system_port (bussbyte bus)
{
    bool result = false;
    if (bus < count() && m_container[bus].active())
        result = m_container[bus].bus()->is_system_port();

    return result;
}

/**
 *  Initiate a poll() on the existing poll descriptors.  This is a primitive
 *  poll, which exits when some data is obtained.  It also applies only to the
 *  input busses.
 *
 *  One issue is that we have no way of knowing here which MIDI input device
 *  has MIDI input events waiting.  Should we randomize the order of polling
 *  in order to avoid starving some input devices?
 *
 * \return
 *      Returns the number of MIDI events detected on one of the busses.  Note
 *      that this is no longer a boolean value.
 */

int
busarray::poll_for_midi ()
{
    int result = 0;
    std::vector<businfo>::iterator bi;
    for (bi = m_container.begin(); bi != m_container.end(); ++bi)
    {
        result = bi->bus()->poll_for_midi();
        if (result > 0)
            break;
    }
    return result;
}

/**
 *  Gets the first MIDI event in finds on an input bus.
 *
 *  Note that this function risks starving the second input device if more
 *  than one is enabled in Sequencer64.  We will figure that one out later.
 *
 * \param inev
 *      A pointer to the event to be modified by incoming data, if any.
 *
 * \return
 *      Returns true if an event's data was copied into the event pointer.
 */

bool
busarray::get_midi_event (event * inev)
{
    std::vector<businfo>::iterator bi;
    for (bi = m_container.begin(); bi != m_container.end(); ++bi)
    {
        if (bi->bus()->get_midi_event(inev))
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
 * \param bus
 *      The buss to be affected.
 *
 * \param port
 *      The prot to be affected.
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

/**
 *  This free function swaps the contents of two busarray objects.
 *
 * \param buses0
 *      Provides the first buss in the swap.
 *
 * \param buses1
 *      Provides the second buss in the swap.
 */

void
swap (busarray & buses0, busarray & buses1)
{
    busarray temp = buses0;
    buses0 = buses1;
    buses1 = temp;
}

}           // namespace seq64

/*
 * businfo.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

