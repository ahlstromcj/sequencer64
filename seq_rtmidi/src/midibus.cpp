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
 * \updates       2016-11-21
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
 */

#include "midibus.hpp"                  /* seq64::midibus for rtmidi        */

/**
 *  Initialize this static member.
 */

int midibus::m_clock_mod = 16 * 4;

/**
 *  Principal constructor.
 */

midibus::midibus (char id, char pm_num, const char * client_name)
 :
    m_id            (id),
    m_pm_num        (pm_num),
    m_clock_type    (e_clock_off),
    m_inputing      (false),
    m_name          (),
    m_lasttick      (0),
    m_mutex         (),
    m_pms           (nullptr)
{
    /* copy the client names */

    char tmp[64];
    snprintf(tmp, sizeof(tmp), "[%d] %s", m_id, client_name);
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
midibus::play (event * e24, unsigned char channel)
{
    automutex locker(m_mutex);
    PmEvent event;
    event.timestamp = 0;

    /* fill buffer and set midi channel */

    unsigned char buffer[3];                /* temp for midi data */
    buffer[0] = e24->get_status();
    buffer[0] += (channel & 0x0F);
    e24->get_data(&buffer[1], &buffer[2]);
    event.message = Pm_Message(buffer[0], buffer[1], buffer[2]);
    /*PmError err = */ Pm_Write(m_pms, &event, 1);
}

/**
 *  min() for long values.
 */

inline midipulse
min (midipulse a, midipulse b)
{
    return (a < b) ? a : b ;
}

/**
 *  For Windows, this event does nothing for handling SYSEx messages.
 */

void
midibus::sysex (event * e24)
{
    // no code at present
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
midibus::init_clock (midipulse tick)
{
    if (m_clock_type == e_clock_pos && tick != 0)
    {
        continue_from(tick);
    }
    else if (m_clock_type == e_clock_mod || tick == 0)
    {
        start();

        /*
         * \todo
         *      Use an m_ppqn member variable and the usual adjustments.
         */

        midipulse clock_mod_ticks = (usr().midi_ppqn() / 4) * m_clock_mod;
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
 *  Continue from the given tick.
 */

void
midibus::continue_from (midipulse tick)
{
    /*
     * Tell the device that we are going to start at a certain position.
     */

    midipulse pp16th = (usr().midi_ppqn() / 4);
    midipulse leftover = (tick % pp16th);
    midipulse beats = (tick / pp16th);
    midipulse starting_tick = tick - leftover;

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
midibus::clock (midipulse tick)
{
    automutex locker(m_mutex);
    if (m_clock_type != e_clock_off)
    {
        bool done = false;
        if (m_lasttick >= tick)
            done = true;

        while (! done)
        {
            ++m_lasttick;
            if (m_lasttick >= tick)
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

/*
 * midibus.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
