/*
 *  This file is part of seq24/sequencer24.
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
 * \file          mastermidibus_portmidi.cpp
 *
 *  This module declares/defines the base class for MIDI I/O under one of
 *  Windows' audio frameworks.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2015-09-11
 * \license       GNU GPLv2 or above
 *
 *  This file provides a Windows-only implementation of the mastermidibus class.
 *  There is a lot of common code between these two versions!
 */

#include "easy_macros.h"
#include "midibus_portmidi.hpp"

#ifdef PLATFORM_WINDOWS                // covers this whole module

/**
 *  This constructor fills the array for our busses.  The only member
 *  "missing" from this Windows version is the "m_alsa_seq" member of the
 *  Linux version.
 */

mastermidibus::mastermidibus ()
 :
    m_num_out_buses     (0),        // or c_max_busses, or what?
    m_num_in_buses      (0),        // or c_max_busses, or 1, or what?
    m_buses_out         (),         // array of c_max_busses midibus pointers
    m_buses_in          (),         // array of c_max_busses midibus pointers
    m_bus_announce      (nullptr),  // one pointer
    m_buses_out_active  (),         // array of c_max_busses booleans
    m_buses_in_active   (),         // array of c_max_busses booleans
    m_buses_out_init    (),         // array of c_max_busses booleans
    m_buses_in_init     (),         // array of c_max_busses booleans
    m_init_clock        (),         // array of c_max_busses clock_e values
    m_init_input        (),         // array of c_max_busses booleans
    m_queue             (0),
    m_ppqn              (0),
    m_bpm               (0),
    m_num_poll_descriptors (0),
    m_poll_descriptors  (nullptr),
    m_dumping_input     (false),
    m_seq               (nullptr),
    m_mutex             ()
{
    for (int i = 0; i < c_max_busses; ++i)        // why the global?
    {
        m_buses_in_active[i] = false;
        m_buses_out_active[i] = false;
        m_buses_in_init[i] = false;
        m_buses_out_init[i] = false;
        m_init_clock[i] = e_clock_off;
        m_init_input[i] = false;
    }
    Pm_Initialize();
}

/**
 *  The destructor deletes all of the output busses, and terminates the
 *  Windows MIDI manager.
 */

mastermidibus::~mastermidibus ()
{
    for (int i = 0; i < m_num_out_buses; i++)
    {
        if (not_nullptr(m_buses_out[i]))
        {
            delete m_buses_out[i];
            m_buses_out[i] = nullptr;
        }
    }
    for (int i = 0; i < m_num_in_buses; i++)
    {
        if (not_nullptr(m_buses_in[i]))
        {
            delete m_buses_in[i];
            m_buses_in[i] = nullptr;
        }
    }
    Pm_Terminate();
}


void
mastermidibus::init ()
{

    int num_devices = Pm_CountDevices();
    const PmDeviceInfo * dev_info = nullptr;
    for (int i = 0; i < num_devices; ++i)
    {
        dev_info = Pm_GetDeviceInfo(i);

#ifdef PLATFORM_DEBUG
        fprintf
        (
            stderr,
            "[0x%x] [%s] [%s] input[%d] output[%d]\n",
            i, dev_info->interf, dev_info->name,
               dev_info->input, dev_info->output
        );
#endif

        if (dev_info->output)
        {
            m_buses_out[m_num_out_buses] = new midibus
            (
                m_num_out_buses, i, dev_info->name
            );
            if (m_buses_out[m_num_out_buses]->init_out())
            {
                m_buses_out_active[m_num_out_buses] = true;
                m_buses_out_init[m_num_out_buses] = true;
                m_num_out_buses++;
            }
            else
            {
                delete m_buses_out[m_num_out_buses];
                m_buses_out[m_num_out_buses] = nullptr;
            }
        }
        if (dev_info->input)
        {
            m_buses_in[m_num_in_buses] = new midibus
            (
                m_num_in_buses, i, dev_info->name
            );
            if (m_buses_in[m_num_in_buses]->init_in())
            {
                m_buses_in_active[m_num_in_buses] = true;
                m_buses_in_init[m_num_in_buses] = true;
                m_num_in_buses++;
            }
            else
            {
                delete m_buses_in[m_num_in_buses];
                m_buses_in[m_num_in_buses] = nullptr;
            }
        }
    }

    set_bpm(c_bpm);
    set_ppqn(c_ppqn);

    /* MIDI input poll descriptors */

    set_sequence_input(false, NULL);
    for (int i = 0; i < m_num_out_buses; i++)
        set_clock(i, m_init_clock[i]);

    for (int i = 0; i < m_num_in_buses; i++)
        set_input(i, m_init_input[i]);
}

/**
 *  Starts all of the configured output busses up to m_num_out_buses.
 *
 * \threadsafe
 */

void
mastermidibus::start ()
{
    automutex locker(m_mutex);
    for (int i = 0; i < m_num_out_buses; i++)
        m_buses_out[i]->start();
}

/**
 *  Gets the output busses running again.
 *
 * \threadsafe
 */

void
mastermidibus::continue_from (long a_tick)
{
    automutex locker(m_mutex);
    for (int i = 0; i < m_num_out_buses; i++)
        m_buses_out[i]->continue_from(a_tick);
}

/**
 *  Initializes the clock of each of the output busses.
 *
 * \threadsafe
 */

void
mastermidibus::init_clock (long a_tick)
{
    automutex locker(m_mutex);
    for (int i = 0; i < m_num_out_buses; i++)
        m_buses_out[i]->init_clock(a_tick);
}

/**
 *  Stops each of the output busses.
 *
 * \threadsafe
 */

void
mastermidibus::stop ()
{
    automutex locker(m_mutex);
    for (int i = 0; i < m_num_out_buses; i++)
        m_buses_out[i]->stop();
}

/**
 *  Generates the MIDI clock for each of the output busses.
 *
 * \threadsafe
 */

void
mastermidibus::clock (long a_tick)
{
    automutex locker(m_mutex);
    for (int i = 0; i < m_num_out_buses; i++)
        m_buses_out[i]->clock(a_tick);
}

/**
 *  Set the PPQN value (parts per quarter note) member.
 *
 * \threadsafe
 */

void
mastermidibus::set_ppqn (int a_ppqn)
{
    automutex locker(m_mutex);
    m_ppqn = a_ppqn;
}

/**
 *  Set the BPM value (beats per minute) member.
 *
 * \threadsafe
 */

void
mastermidibus::set_bpm (int a_bpm)
{
    automutex locker(m_mutex);
    m_bpm = a_bpm;
}

/**
 *  Flushes our local queue events out; but the Windows version does
 *  nothing.
 */

void
mastermidibus::flush ()
{
    // empty body
}

/**
 *  Handle the sending of SYSEX events.
 *
 * \threadsafe
 */

void
mastermidibus::sysex (event * a_ev)
{
    automutex locker(m_mutex);
    for (int i = 0; i < m_num_out_buses; i++)
        m_buses_out[i]->sysex(a_ev);

    flush();
}

/**
 *  Handle the playing of MIDI events on the MIDI buss given by the
 *  parameter, as long as it is a legal buss number.
 *
 * \threadsafe
 */

void
mastermidibus::play (unsigned char a_bus, event * a_e24, unsigned char a_channel)
{
    automutex locker(m_mutex);
    if (m_buses_out_active[a_bus] && a_bus < m_num_out_buses)
    {
        m_buses_out[a_bus]->play(a_e24, a_channel);
    }
}

/**
 *  Set the clock for the given (legal) buss number.
 *
 * \threadsafe
 */

void
mastermidibus::set_clock (unsigned char a_bus, clock_e a_clock_type)
{
    automutex locker(m_mutex);
    if (a_bus < c_max_busses)
        m_init_clock[a_bus] = a_clock_type;

    if (m_buses_out_active[a_bus] && a_bus < m_num_out_buses)
        m_buses_out[a_bus]->set_clock(a_clock_type);
}

/**
 *  Get the clock for the given (legal) buss number.
 */

clock_e
mastermidibus::get_clock (unsigned char a_bus)
{
    if (m_buses_out_active[a_bus] && a_bus < m_num_out_buses)
    {
        return m_buses_out[a_bus]->get_clock();
    }
    return e_clock_off;
}

/**
 *  Set the clock mod to the given value, if legal.
 */

void
midibus::set_clock_mod (int a_clock_mod)
{
    if (a_clock_mod != 0)
        m_clock_mod = a_clock_mod;
}

/**
 *  Get the clock mod.
 */

int
midibus::get_clock_mod ()
{
    return m_clock_mod;
}

/**
 *  Set the status of the given input buss, if a legal buss number.
 *
 *  Why is another buss-count constant, and a global one at that, being
 *  used?  And I thought there was only one input buss anyway!
 *
 * \threadsafe
 */

void
mastermidibus::set_input (unsigned char a_bus, bool a_inputing)
{
    automutex locker(m_mutex);
    if (a_bus < c_max_busses)         // should be m_num_in_buses I believe!!!
        m_init_input[a_bus] = a_inputing;

    if (m_buses_in_active[a_bus] && a_bus < m_num_in_buses)
        m_buses_in[a_bus]->set_input(a_inputing);
}

/**
 *  Get the input for the given (legal) buss number.
 */

bool
mastermidibus::get_input (unsigned char a_bus)
{
    if (m_buses_in_active[a_bus] && a_bus < m_num_in_buses)
        return m_buses_in[a_bus]->get_input();

    return false;
}

/**
 *  Get the MIDI output buss name for the given (legal) buss number.
 */

std::string
mastermidibus::get_midi_out_bus_name (int a_bus)
{
    if (m_buses_out_active[a_bus] && a_bus < m_num_out_buses)
        return m_buses_out[a_bus]->get_name();

    return "get_midi_out_bus_name(): error";
}

/**
 *  Get the MIDI input buss name for the given (legal) buss number.
 */

std::string
mastermidibus::get_midi_in_bus_name (int a_bus)
{
    if (m_buses_in_active[a_bus] && a_bus < m_num_in_buses)
        return m_buses_in[a_bus]->get_name();

    return "get_midi_in_bus_name(): error";
}

/**
 *  Print some information about the available MIDI output busses.
 */

void
mastermidibus::print ()
{
    printf("Available Buses\n");
    for (int i = 0; i < m_num_out_buses; i++)
        printf("%s\n", m_buses_out[i]->m_name.c_str());
}

/**
 *  Initiate a poll() on the existing poll descriptors.  This is a
 *  primitive poll, which exits when some data is obtained.
 */

int
mastermidibus::poll_for_midi ()
{
    for (;;)
    {
        for (int i = 0; i < m_num_in_buses; i++)
        {
            if (m_buses_in[i]->poll_for_midi())
                return 1;
        }
        Sleep(1);                      // yield processor for 1 millisecond
        return 0;
    }
}

/**
 *  Test the ALSA sequencer to see if any more input is pending.
 *
 * \threadsafe
 */

bool
mastermidibus::is_more_input ()
{
    automutex locker(m_mutex);
    int size = 0;
    for (int i = 0; i < m_num_in_buses; i++)
    {
        if (m_buses_in[i]->poll_for_midi())
            size = 1;
    }
    return size > 0;
}

// No mastermidibus::port_start(), port_exit() in Windows version.

/**
 *  Grab a MIDI event.
 *
 * \threadsafe
 */

bool
mastermidibus::get_midi_event (event *a_in)
{
    automutex locker(m_mutex);
    bool result = false;
    PmEvent event;
    PmError err;
    for (int i = 0; i < m_num_in_buses; i++)
    {
        if (m_buses_in[i]->poll_for_midi())
        {
            err = Pm_Read(m_buses_in[i]->m_pms, &event, 1);
            if (err < 0)
                printf("Pm_Read: %s\n", Pm_GetErrorText(err));

            if (m_buses_in[i]->m_inputing)
                result = true;
        }
    }
    if (! result)
        return false;

    a_in->set_status(Pm_MessageStatus(event.message));
    a_in->set_size(3);
    a_in->set_data(Pm_MessageData1(event.message), Pm_MessageData2(event.message));

    /* some keyboards send Note On with velocity 0 for Note Off */

    if (a_in->get_status() == EVENT_NOTE_ON && a_in->get_note_velocity() == 0x00)
        a_in->set_status(EVENT_NOTE_OFF);

    // Why no "sysex = false" here, like in Linux version?

    return true;
}

/**
 *  Set the input sequence object, and set the m_dumping_input value to
 *  the given state.
 *
 * \threadsafe
 */

void
mastermidibus::set_sequence_input (bool a_state, sequence * a_seq)
{
    automutex locker(m_mutex);
    m_seq = a_seq;
    m_dumping_input = a_state;
}

#endif   // PLATFORM_WINDOWS

/*
 * mastermidibus_portmidi.cpp
 *
 * vim: sw=4 ts=4 wm=8 et ft=cpp
 */
