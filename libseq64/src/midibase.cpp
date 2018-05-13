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
 * \file          midibase.cpp
 *
 *  This module declares/defines the base class for handling MIDI I/O via
 *  various MIDI APIs.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2016-11-25
 * \updates       2018-05-09
 * \license       GNU GPLv2 or above
 *
 *  This file provides a cross-platform implementation of MIDI support.
 *
 *  Elements of a MIDI buss:
 *
 *      -   Client.  This is the application:  sequencer64, seq64portmidi, or
 *          seq64rtmidi.
 *      -   Buss.   This is the main MIDI item, such as MIDI Through (14)
 *          or TiMidity (128).  The buss numbers are provided by the system.
 *          Currently, the buss name is empty.
 *      -   Port.  This is one of the items provided by the buss, and the
 *          number usually starts at 0.  The port numbers are provided by the
 *          system.  Currently, the port name includes the buss name as
 *          provided by the system, as a single unit.
 *      -   Index.  This number is the order of the input or output MIDI
 *          device as enumerated by the system lookup code, and always starts
 *          at 0.
 */

#include "globals.h"
#include "calculations.hpp"             /* clock_ticks_from_ppqn()          */
#include "event.hpp"                    /* seq64::event (MIDI event)        */
#include "midibase.hpp"                 /* seq64::midibase for ALSA         */
#include "settings.hpp"                 /* seq64::rc() and choose_ppqn()    */

#ifdef PLATFORM_WINDOWS
#include <windows.h>                    /* Sleep()                          */
#else
#include <unistd.h>                     /* usleep() or select()             */
#endif

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *  Initialize this static member.
 */

int midibase::m_clock_mod = 16 * 4;

/**
 *  Creates a normal MIDI port, which will correspond to an existing system
 *  MIDI port, such as one provided by Timidity or a running JACK application,
 *  or a virtual port, which has a name made up by the application.  Provides
 *  a constructor with client number, port number, name of client, name of
 *  port.
 *
 *  This constructor is the one that seems to be the one that is used for
 *  the MIDI input and output busses, when the [manual-alsa-ports] option is
 *  <i> not </i> in force.  Also used for the announce buss, and in the
 *  mastermidibase::port_start() function.
 *
 * \param appname
 *      Provides the the name of the application.  The derived class will
 *      determine this name.
 *
 * \param busname
 *      Provides the ALSA client name or the MIDI subsystem name (e.g.
 *      "TiMidity").  If empty, a name will be assembled by the derived class
 *      at port-setup time.
 *
 * \param portname
 *      Provides the port name.  This item defaults to empty, which means the
 *      port name should be obtained via the API, or be assembled by the
 *      derived class at port-setup time.
 *
 * \param index
 *      Provides the ordinal of this buss/port, mostly for display purposes.
 *
 * \param bus_id
 *      Provides the ID code for this bus.  It is an index into the midibus
 *      definitions array, and is also used in the constructed human-readable
 *      buss name.  Defaults to SEQ64_NO_BUS.
 *
 *          -   ALSA (seq24).  This is the ALSA buss number, ranging from 1 on
 *              upwards.  If SEQ64_NO_BUS, the derived class will get the buss
 *              ID at port-setup time.
 *          -   PortMidi.  This number is not yet used in PortMidi.  Perhaps
 *              this should be used instead of the queue parameter.
 *          -   RtMidi.  Like ALSA, we will use this as a buss number.
 *
 * \param port_id
 *      Indicates the port ID.  Defaults to SEQ64_NO_PORT.  If SEQ64_NO_PORT,
 *      the derived class will get the port ID at port-setup time.
 *
 * \param queue
 *      Provides the queue ID.  It has different meanings in each of the MIDI
 *      implementations.  Defaults to SEQ64_NO_QUEUE.
 *
 *          -   ALSA (seq24).  This is the ALSA queue number, which is an ALSA
 *              concept.
 *          -   PortMidi.  This is the PortMidi buss number, sort of.  It is
 *              the PmDeviceID value.
 *          -   RtMidi.  Not sure yet if it will have meaning here.
 *
 * \param ppqn
 *      Provides the PPQN value.  Defaults to SEQ64_USE_DEFAULT_PPQN.
 *
 * \param bpm
 *      Provides the BPM value.  Defaults to SEQ64_DEFAULT_BPM.
 *
 * \param makevirtual
 *      Indicates that the port represented by this object is to be virtual.
 *      Defaults to false.  This could also be set via the init_in(),
 *      init_out(), init_in_sub(), or init_out_sub() routines.  Doing it here
 *      seems okay.
 *
 * \param isinput
 *      Indicates that this midibus represents and input port, as opposed to
 *      an output port.
 *
 * \param makesystem
 *      Indicates that the port represented by this object is a system port.
 *      Currently true only for ALSA system ports (timer or announce ports).
 */

midibase::midibase
(
    const std::string & appname,        // application name
    const std::string & busname,        // can be empty
    const std::string & portname,       // can be empty
    int index,                          // just an ordinal for display
    int bus_id,                         // an index in some implementations
    int port_id,                        // an index in some implementations
    int queue,
    int ppqn,
    midibpm bpm,
    bool makevirtual,
    bool isinput,
    bool makesystem
) :
    m_bus_index         (index),
    m_bus_id            (bus_id),
    m_port_id           (port_id),
    m_clock_type        (e_clock_off),
    m_inputing          (false),
    m_ppqn              (choose_ppqn(ppqn)),
    m_bpm               (bpm),
    m_queue             (queue),
    m_bus_name          (busname),
    m_port_name         (portname),
    m_lasttick          (0),
    m_is_virtual_port   (makevirtual),
    m_is_input_port     (isinput),
    m_is_system_port    (makesystem),
    m_mutex             ()
{
    if (! makevirtual)
    {
        if (! busname.empty() && ! portname.empty())
        {
            set_name(appname, busname, portname);
        }
        else
        {
            errprint("programmer error in midibase()");
        }
    }
}

/**
 *  A rote empty destructor.
 */

midibase::~midibase()
{
    // empty body
}

/**
 *  Sets the name of the buss by assembling the name components obtained from
 *  the system in a straightforward manner:
 *
 *      [0] 128:2 seq64:seq64 port 2
 *
 *  We want to see if the user has configured a port name. If so, and this is
 *  an output port, then the buss name is overridden by the entry in the "usr"
 *  configuration file.  Otherwise, we fall back to the parameters.  Note that
 *  this has been tweaked versus Seq24, where the "usr" devices were also
 *  applied to the input ports.  Also note that the "usr" device names should
 *  be kept short, and the actual buss name from the system is shown in
 *  brackets.
 *
 * \param appname
 *      This is the name of the client, or application.  Not to be confused
 *      with the ALSA client-name, which is actually a buss or subsystem name.
 *
 * \param busname
 *      Provides the name of the sub-system, such as "Midi Through" or
 *      "TiMidity".
 *
 * \param portname
 *      Provides the name of the port.  In ALSA, this is something like
 *      "busname port X".
 */

void
midibase::set_name
(
    const std::string & appname,
    const std::string & busname,
    const std::string & portname
)
{
    char name[128];
    if (is_virtual_port())
    {
        /*
         * See banner.  Let's also assign any "usr" names to the virtual ports
         * as well.
         */

        std::string bname = usr().bus_name(m_bus_index);
        if (is_output_port() && ! bname.empty())
        {
            snprintf
            (
                name, sizeof name, "%s [%s]", bname.c_str(), portname.c_str()
            );
            bus_name(bname);
        }
        else
        {
            snprintf
            (
                name, sizeof name, "[%d] %d:%d %s:%s",
                get_bus_index(), get_bus_id(), get_port_id(),
                appname.c_str(), portname.c_str()
            );
            bus_name(appname);
            port_name(portname);
        }
    }
    else
    {
        /*
         * See banner.
         *
         * Old: std::string bname = usr().bus_name(get_port_id());
         */

        char alias[128];
        std::string bname = usr().bus_name(m_bus_index);
        if (is_output_port() && ! bname.empty())
        {
            snprintf
            (
                alias, sizeof alias, "%s [%s]", bname.c_str(), portname.c_str()
            );
            bus_name(bname);
        }
        else if (! busname.empty())
        {
            snprintf
            (
                alias, sizeof alias, "%s:%s", busname.c_str(), portname.c_str()
            );
            bus_name(busname);              // bus_name(alias);
        }
        else
            snprintf(alias, sizeof alias, "%s", portname.c_str());

        snprintf                            /* copy the client name parts */
        (
            name, sizeof name, "[%d] %d:%d %s",
            get_bus_index(), get_bus_id(), get_port_id(), alias
        );
    }
    display_name(name);
}

/**
 *  Sets the name of the buss in a different way.  If the port is virtual,
 *  this function just calls set_name().  Otherwise, it reassembles the name
 *  so that it refers to a port found on the system, but modified to make it a
 *  unique application port.  For example:
 *
 *      [0] 128:0 yoshimi:midi in
 *
 *  is transformed to this:
 *
 *      [0] 128:0 seq64:yoshimi midi in
 *
 *  As a side-effect, the "short" portname is changed, from (for example)
 *  "midi in" to "yoshimi midi in".
 *
 * \param appname
 *      This is the name of the client, or application.  Not to be confused
 *      with the ALSA/JACK client-name, which is actually a buss or subsystem
 *      name.
 *
 * \param busname
 *      Provides the name of the sub-system, such as "Midi Through",
 *      "TiMidity", or "seq64".
 *
 * \param portname
 *      Provides the name of the port.  In JACK, this should be the full port
 *      name, such as "qmidiarp:in".
 */

void
midibase::set_alt_name
(
    const std::string & appname,
    const std::string & busname,
    const std::string & portname
)
{
    if (is_virtual_port())
    {
        set_name(appname, busname, portname);
    }
    else
    {
        std::string bname = busname;
        std::string pname = portname;
        std::size_t colonpos = pname.find_first_of(":");
        if (colonpos != std::string::npos)
            pname[colonpos] = ' ';

        char alias[128];
        snprintf                            /* copy the client name parts */
        (
            alias, sizeof alias, "[%d] %d:%d %s:%s",
            get_bus_index(), get_bus_id(), get_port_id(),
            bname.c_str(), pname.c_str()
        );
        bus_name(bname);
        port_name(pname);
        display_name(alias);
    }
}

/**
 *  Sets the name of the buss in yet another different way, suitable for the
 *  multiclient mode of some APIs (such as JACK).  If the port is virtual,
 *  this function just calls set_name().  Otherwise, it reassembles the name
 *  so that it refers to a port found on the system, but modified to make it a
 *  unique client port.  For example:
 *
 *      [0] 128:0 yoshimi:midi in
 *
 *  is transformed to this:
 *
 *      [0] 128:0 seq64-yoshimi:midi in
 *
 *  The name in the latter is the original buss name, "seq64" plus the remote
 *  port's buss name (extracted from the long name), plus the remote port's
 *  short port name (extracted from the long name).
 *
 * Internal parameter:
 *
 * \param appname
 *      This is the name of the client, or application.  Not to be confused
 *      with the ALSA client-name, which is actually a buss or subsystem name.
 *
 * \param localbusname
 *      Provides the name of the sub-system, such as "Midi Through",
 *      "TiMidity", "yoshimi", or "seq64".  It is assumed this parameter has
 *      already been set properly.
 *
 * \param remoteportname
 *      Provides the name of the port.  In JACK, this should be the long port
 *      name, such as "qmidiarp:in" or "yoshimi:midi in".  It is assumed this
 *      parameter has already been set properly.
 */

void
midibase::set_multi_name
(
    const std::string & appname,
    const std::string & localbusname,
    const std::string & remoteportname
)
{
    if (is_virtual_port())
    {
        set_name(appname, localbusname, remoteportname);
    }
    else
    {
        std::string bname = localbusname;
        std::string rbname = extract_bus_name(remoteportname);
        std::string rpname = extract_port_name(remoteportname);
        bname += "-";
        bname += rbname;
        bus_name(bname);
        port_name(rpname);

        char alias[128];
        snprintf                            /* copy the client name parts */
        (
            alias, sizeof alias, "[%d] %d:%d %s:%s",
            get_bus_index(), get_bus_id(), get_port_id(),
            bus_name().c_str(), port_name().c_str()
        );
        display_name(alias);
    }
}

/**
 * \getter m_bus_name and m_port_name
 *      Concatenates the bus and port names into a string of the form
 *      "busname:portname".  If either name is empty, an empty string is
 *      returned.
 */

std::string
midibase::connect_name () const
{
    std::string result = m_bus_name;
    if (! result.empty() && ! m_port_name.empty())
    {
        result += ":";
        result += m_port_name;
    }
    return result;
}

/**
 *  Polls for MIDI events.
 *
 *  EXPERIMENTAL FIX FOR PORTMIDI BUG BUT NEEDED FOR ALL.
 *
 * \return
 *      Returns a value greater than 0 if MIDI events are available.
 *      Otherwise 0 is returned, or -1 for some APIs (ALSA) when an internal
 *      error occurs.
 */

int
midibase::poll_for_midi ()
{
    return m_inputing ? api_poll_for_midi() : 0 ;
}

/**
 *  Obtains a MIDI event.
 *
 * \param inev
 *      Points the event to be filled with the MIDI event data.
 *
 * \return
 *      Returns true if an event was found, thus making the return parameter
 *      useful.
 */

bool
midibase::get_midi_event (event * inev)
{
    return api_get_midi_event(inev);
}

/**
 *  Initialize the MIDI output port.
 *
 * \return
 *      Returns true unless setting up MIDI failed in some way.
 */

bool
midibase::init_out ()
{
    return api_init_out();
}

/**
 *  Initialize the MIDI input port.
 *
 * \return
 *      Returns true unless setting up MIDI failed in some way.
 */

bool
midibase::init_in ()
{
    return api_init_in();
}

/**
 *  Initialize the output in a different way?
 *
 * \return
 *      Returns true unless setting up the ALSA port failed in some way.
 */

bool
midibase::init_out_sub ()
{
    return api_init_out_sub();      // no portmidi implementation
}

/**
 *  Initialize the output in a different way?
 *
 * \return
 *      Returns true unless setting up the ALSA port failed in some way.
 */

bool
midibase::init_in_sub ()
{
    return api_init_in_sub();       // no portmidi implementation
}

/**
 *  Deinitialize the MIDI input.  Set the input and the output ports.
 *  The destination port is actually our local port.
 *
 * \return
 *      Returns true, unless an error occurs.
 */

bool
midibase::deinit_in ()
{
    return api_deinit_in();
}

/**
 *  Prints m_name.
 */

void
midibase::print ()
{
    printf("%s:%s", m_bus_name.c_str(), m_port_name.c_str());
}

/**
 *  This play() function takes a native event, encodes it to a MIDI
 *  sequencer event, sets the broadcasting to the subscribers, sets the
 *  direct-passing mode to send the event without queueing, and puts it in the
 *  queue.
 *
 * \threadsafe
 *
 * \param e24
 *      The event to be played on this bus.  For speed, we don't bother to
 *      check the pointer.
 *
 * \param channel
 *      The channel of the playback.
 */

void
midibase::play (event * e24, midibyte channel)
{
    automutex locker(m_mutex);
    api_play(e24, channel);
}

/**
 *  Takes a native SYSEX event, encodes it to an ALSA event, and then
 *  puts it in the queue.
 *
 * \param e24
 *      The event to be handled.
 */

void
midibase::sysex (event * e24)
{
    automutex locker(m_mutex);
    api_sysex(e24);
}

/**
 *  Flushes our local queue events out into ALSA.
 */

void
midibase::flush ()
{
    automutex locker(m_mutex);
    api_flush();
}

/**
 *  Initialize the clock, continuing from the given tick.  This function
 *  doesn't depend upon the MIDI API in use.  Here, e_clock_off and
 *  e_clock_disabled have the same effect... none.
 *
 * \param tick
 *      The starting tick.
 */

void
midibase::init_clock (midipulse tick)
{
    if (m_clock_type == e_clock_pos && tick != 0)
    {
        continue_from(tick);
    }
    else if (m_clock_type == e_clock_mod || tick == 0)
    {
        start();

        /*
         * The next equation is effectively (m_ppqn / 4) * 16 * 4,
         * or m_ppqn * 16.  Note that later we have pp16th = (m_ppqn / 4).
         */

        midipulse clock_mod_ticks = (m_ppqn / 4) * m_clock_mod;
        midipulse leftover = (tick % clock_mod_ticks);
        midipulse starting_tick = tick - leftover;

        /*
         * Was there anything left? Then wait for next beat (16th note)
         * to start clocking.
         */

        if (leftover > 0)
            starting_tick += clock_mod_ticks;

        m_lasttick = starting_tick - 1;
    }
}

/**
 *  Continue from the given tick.  Tell the device that we are going to start
 *  at a certain position (starting_tick).  If there is anything left, then
 *  wait for next beat (16th note) to start clocking.
 *
 * \param tick
 *      The continuing tick.
 */

void
midibase::continue_from (midipulse tick)
{
    midipulse pp16th = m_ppqn / 4;
    midipulse leftover = tick % pp16th;
    midipulse beats = tick / pp16th;
    midipulse starting_tick = tick - leftover;
    if (leftover > 0)
        starting_tick += pp16th;

    m_lasttick = starting_tick - 1;
    if (clock_enabled())
    {
        api_continue_from(tick, beats);
    }
}

/**
 *  This function gets the MIDI clock a-runnin', if the clock type is not
 *  e_clock_off or e_clock_disabled.
 */

void
midibase::start ()
{
    m_lasttick = -1;
    if (clock_enabled())
    {
        api_start();
    }
}

/**
 *  Set status to of "inputting" to the given value.  If the parameter is
 *  true, then init_in() is called; otherwise, deinit_in() is called.
 *
 * \param inputing
 *      The inputing value to set.  For input system ports, it is always set
 *      to true, no matter how it is configured in the "rc" file.
 */

bool
midibase::set_input (bool inputing)     // not part of portmidi
{
    bool result = true;
    if (m_is_system_port)
    {
        m_inputing = true;
        result = init_in();
    }
    else if (m_inputing != inputing)
    {
        m_inputing = inputing;
        if (inputing)
        {
            if (m_is_virtual_port)
                result = init_in_sub();
            else
                result = init_in();
        }
        else
            result = deinit_in();
    }
    return result;
}

/**
 *  Stop the MIDI buss.
 */

void
midibase::stop ()
{
    m_lasttick = -1;

    /*
     * Hmmmmm.
     *
     * if (clock_enabled())
     */

    if (m_clock_type != e_clock_off)
    {
        api_stop();
    }
}

/**
 *  Generates the MIDI clock, starting at the given tick value.  The number
 *  of ticks needed is calculated.
 *
 * \threadsafe
 *
 * \param tick
 *      Provides the starting tick.
 */

void
midibase::clock (midipulse tick)
{
    automutex locker(m_mutex);
    if (clock_enabled())
    {
        bool done = m_lasttick >= tick;
        int ct = clock_ticks_from_ppqn(m_ppqn);         /* ppqn / 24        */
        while (! done)
        {
            ++m_lasttick;
            done = m_lasttick >= tick;
            if ((m_lasttick % ct) == 0)                 /* tick time yet?   */
                api_clock(tick);
        }
        api_flush();                                    /* and send it out  */
    }
}

/**
 *  A static debug function, enabled only for trouble-shooting.
 *
 * \param context
 *      Human readable context information (e.g. "ALSA").
 *
 * \param tick
 *      Provides the current tick value.
 */

void
midibase::show_clock (const std::string & context, midipulse tick)
{
    fprintf(stderr, "%s clock [%ld]\n", context.c_str(), tick);
    fflush(stderr);
}

/**
 * Shows most midibase members.
 */

void
midibase::show_bus_values ()
{
#ifdef SEQ64_SHOW_API_CALLS
    const char * vport = is_virtual_port() ? "virtual" : "non-virtual" ;
    const char * iport = is_input_port() ? "input" : "output" ;
    const char * sport = is_system_port() ? "system" : "device" ;
    printf
    (
        "display name:      %s\n"
        "connect name:      %s\n"
        "bus : port name:   %s : %s\n"
        "bus type:          %s %s %s\n"
        "clock & inputing:  %d & %s\n"
        ,
        display_name().c_str(), connect_name().c_str(),
        m_bus_name.c_str(), m_port_name.c_str(),
        vport, iport, sport,
        int(get_clock()), get_input() ? "yes" : "no"
    );
#endif
}

/*
 *  This free-function in the seq64 namespace provides a way to suspend a
 *  thread for a small amount of time.
 *
 * \linux
 *      We can use the usleep(3) function.
 *
 * \unix
 *    In POSIX, select() can return early if any signal occurs.  We don't
 *    correct for that here at this time.  Actually, it is a convenient
 *    feature, and we wish that Sleep() would provide it.
 *
 * \win32
 *    In Windows, the Sleep(0) function does not sleep, but it does cede
 *    control of the thread to the operating system, which then schedules
 *    another thread to run.
 *
 * \warning
 *    Please note that this function isn't all that accurate for small
 *    sleep values, due to the time taken to set up the operation, and
 *    resolution issues in many operating systems.
 *
 * \param ms
 *    The number of milliseconds to "sleep".
 *
 */

void
millisleep (unsigned long ms)
{
#if defined PLATFORM_LINUX
    (void) usleep((useconds_t) (ms * 1000));
#elif defined PLATFORM_UNIX
   struct timeval tv;
   struct timeval * tvptr = &tv;
   tv.tv_usec = long(ms % 1000) * 1000;
   tv.tv_sec = long(ms / 1000;
   (void) select(0, 0, 0, 0, tvptr);
#elif defined PLATFORM_WINDOWS              // or PLATFORM_MINGW
   Sleep((DWORD) ms);
#endif
}

}           // namespace seq64

/*
 * midibase.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

