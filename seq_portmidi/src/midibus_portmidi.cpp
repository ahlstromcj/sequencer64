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
 * \file          midibus_portmidi.cpp
 *
 *  This module declares/defines the base class for MIDI I/O under one of
 *  Windows' audio frameworks.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2015-11-10
 * \license       GNU GPLv2 or above
 *
 *  This file provides a Windows-only implementation of the midibus class.
 */

#include "midibus_portmidi.hpp"

#ifdef PLATFORM_WINDOWS                // covers this whole module

/**
 *  Initialize this static member.
 */

int midibus::m_clock_mod = 16 * 4;

/**
 *  Principal constructor.
 */

midibus::midibus (char a_id, char a_pm_num, const char * a_client_name)
 :
    m_id            (a_id),
    m_pm_num        (a_pm_num),
    m_clock_type    (e_clock_off),
    m_inputing      (false),
    m_name          (),
    m_lasttick      (0),
    m_mutex         (),
    m_pms           (nullptr)
{
    /* copy the client names */

    char tmp[64];
    snprintf(tmp, sizeof(tmp), "[%d] %s", m_id, a_client_name);
    m_name = tmp;
}

/**
 *  Secondary constructor.
 */

midibus::midibus (char a_id, int a_queue)
 :
    m_id            (a_id),
    m_pm_num        (a_pm_num),
    m_clock_type    (e_clock_off),
    m_inputing      (false),
    m_name          (),
    m_lasttick      (0),
    m_mutex         (),
    m_pms           (nullptr)
{
    /*
     * Not a member: m_queue = a_queue;
     */

    /* synthesize the client names */

    char tmp[64];
    snprintf(tmp, sizeof(tmp), "[%d] seq24 %d", m_id, m_id);
    m_name = tmp;
}

/**
 *  The destructor closes out the Windows MIDI infrastructure.
 */

midibus::~midibus ()
{
    if (not_nullptr(m_pms))
    {
        Pm_Close(m_pms);
        m_pms = nullptr;
    }
}

/**
 *  Polls for MIDI events.
 */

int
midibus::poll_for_midi ()
{
    if (m_pm_num)
    {
        PmError err = Pm_Poll(m_pms);
        if (err == FALSE)
        {
            return 0;
        }
        if (err == TRUE)
        {
            return 1;
        }
        errprintf("Pm_Poll: %s\n", Pm_GetErrorText(err));
    }
    return 0;
}

/**
 *  Initialize the MIDI output port.
 */

bool midibus::init_out ()
{
    PmError err = Pm_OpenOutput(&m_pms, m_pm_num, NULL, 100, NULL, NULL, 0);
    if (err != pmNoError)
    {
        errprintf("Pm_OpenOutput: %s\n", Pm_GetErrorText(err));
        return false;
    }
    return true;
}

/**
 *  Initialize the MIDI input port.
 */

bool midibus::init_in ()
{
    PmError err = Pm_OpenInput(&m_pms, m_pm_num, NULL, 100, NULL, NULL);
    if (err != pmNoError)
    {
        errprintf("Pm_OpenInput: %s\n", Pm_GetErrorText(err));
        return false;
    }
    return true;
}

/**
 *  Prints m_name.
 */

void
midibus::print ()
{
    printf("%s" , m_name.c_str());
}

/**
 *  Takes a native event, and encodes to a Windows message, and writes it
 *  to the queue.
 */

void
midibus::play (event * a_e24, unsigned char a_channel)
{
    automutex locker(m_mutex);
    PmEvent event;
    event.timestamp = 0;

    /* fill buffer and set midi channel */

    unsigned char buffer[3];                /* temp for midi data */
    buffer[0] = a_e24->get_status();
    buffer[0] += (a_channel & 0x0F);
    a_e24->get_data(&buffer[1], &buffer[2]);
    event.message = Pm_Message(buffer[0], buffer[1], buffer[2]);
    /*PmError err = */ Pm_Write(m_pms, &event, 1);
}

/**
 *  min() for long values.
 */

inline long
min (long a, long b)
{
    return (a < b) ? a : b ;
}

/**
 *  For Windows, this event does nothing for handling SYSEx messages.
 */

void
midibus::sysex (event * a_e24)
{
#if 0
    automutex locker(m_mutex);
#endif
}

/**
 *  This function does nothing in Windows.
 */

void
midibus::flush ()
{
    // empty body
}

/**
 *  Initialize the clock, continuing from the given tick.
 */

void
midibus::init_clock (long a_tick)
{
    if (m_clock_type == e_clock_pos && a_tick != 0)
    {
        continue_from(a_tick);
    }
    else if (m_clock_type == e_clock_mod || a_tick == 0)
    {
        start();

        /*
         * \todo
         *      Use an m_ppqn member variable and the usual adjustments.
         */

        long clock_mod_ticks = (usr().midi_ppqn() / 4) * m_clock_mod;
        long leftover = (a_tick % clock_mod_ticks);
        long starting_tick = a_tick - leftover;

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
 *  Continue from the given tick.
 */

void
midibus::continue_from (long a_tick)
{
    /*
     * Tell the device that we are going to start at a certain position.
     */

    long pp16th = (usr().midi_ppqn() / 4);
    long leftover = (a_tick % pp16th);
    long beats = (a_tick / pp16th);
    long starting_tick = a_tick - leftover;

    /*
     * Was there anything left? Then wait for next beat (16th note) to
     * start clocking.
     */

    if (leftover > 0)
        starting_tick += pp16th;

    m_lasttick = starting_tick - 1;
    if (m_clock_type != e_clock_off)
    {
        PmEvent event;
        event.timestamp = 0;
        event.message = Pm_Message(EVENT_MIDI_CONTINUE, 0, 0);
        Pm_Write(m_pms, &event, 1);
        event.message = Pm_Message
        (
            EVENT_MIDI_SONG_POS, (beats & 0x3F80 >> 7), (beats & 0x7F)
        );
        Pm_Write(m_pms, &event, 1);
    }
}

/**
 *  Sets the MIDI clock a-runnin', if the clock type is not e_clock_off.
 */

void
midibus::start ()
{
    m_lasttick = -1;
    if (m_clock_type != e_clock_off)
    {
        PmEvent event;
        event.timestamp = 0;
        event.message = Pm_Message(EVENT_MIDI_START, 0, 0);
        Pm_Write(m_pms, &event, 1);
    }
}

/**
 *  Stops the MIDI clock, if the clock-type is not e_clock_off.
 */

void
midibus::stop ()
{
    m_lasttick = -1;
    if (m_clock_type != e_clock_off)
    {
        PmEvent event;
        event.timestamp = 0;
        event.message = Pm_Message(EVENT_MIDI_STOP, 0, 0);
        Pm_Write(m_pms, &event, 1);
    }
}

/**
 *  Generates MIDI clock.
 */

void
midibus::clock (long a_tick)
{
    automutex locker(m_mutex);
    if (m_clock_type != e_clock_off)
    {
        bool done = false;
        if (m_lasttick >= a_tick)
            done = true;

        while (! done)
        {
            m_lasttick++;
            if (m_lasttick >= a_tick)
                done = true;

            if (m_lasttick % (usr().midi_ppqn() / 24) == 0) /* tick time? */
            {
                PmEvent event;
                event.timestamp = 0;
                event.message = Pm_Message(EVENT_MIDI_CLOCK, 0, 0);
                Pm_Write(m_pms, &event, 1);
            }
        }
    }
}

#endif   // PLATFORM_WINDOWS

/*
 * midibus_portmidi.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
