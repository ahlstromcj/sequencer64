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
 * \file          mastermidibus.cpp
 *
 *  This module declares/defines the base class for MIDI I/O under one of
 *  Windows' audio frameworks.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2016-11-24
 * \license       GNU GPLv2 or above
 *
 *  This file provides a Windows-only implementation of the mastermidibus
 *  class.  There is a lot of common code between these two versions!
 */

#include "easy_macros.h"                /* handy macros                     */
#include "event.hpp"                    /* seq64::event                     */
#include "mastermidibus.hpp"            /* seq64::mastermidibus, PortMIDI   */
#include "midibus.hpp"                  /* seq64::midibus, PortMIDI         */
#include "portmidi.h"

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *  The base-class constructor fills the array for our busses.
 *
 * \param ppqn
 *      Provides the PPQN value for this object.  However, in most cases, the
 *      default value, SEQ64_USE_DEFAULT_PPQN should be specified.
 *
 * \param bpm
 *      Provides the beats per minute value, which defaults to
 *      c_beats_per_minute.
 */

mastermidibus::mastermidibus (int ppqn, int bpm)
 :
    mastermidibase      (ppqn, bpm)
{
    Pm_Initialize();
}

/**
 *  The destructor deletes all of the output busses, and terminates the
 *  Windows MIDI manager.
 */

mastermidibus::~mastermidibus ()
{
    Pm_Terminate();
}

/**
 *  Provides the PortMidi implementation needed for the init() function.
 */

void
mastermidibus::api_init ()
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
                ++m_num_out_buses;
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

    set_beats_per_minute(c_beats_per_minute);
    set_ppqn(m_ppqn);   // SEQ64_DEFAULT_PPQN);

    /* MIDI input poll descriptors */

    set_sequence_input(false, NULL);
    for (int i = 0; i < m_num_out_buses; i++)
        set_clock(i, m_init_clock[i]);

    for (int i = 0; i < m_num_in_buses; i++)
        set_input(i, m_init_input[i]);
}

/**
 *  Get the MIDI output buss name for the given (legal) buss number.

std::string
mastermidibus::get_midi_out_bus_name (int bus)
{
    if (m_buses_out_active[bus] && bus < m_num_out_buses)
        return m_buses_out[bus]->get_name();

    return "get_midi_out_bus_name(): error";
}
 */

/**
 *  Get the MIDI input buss name for the given (legal) buss number.

std::string
mastermidibus::get_midi_in_bus_name (int bus)
{
    if (m_buses_in_active[bus] && bus < m_num_in_buses)
        return m_buses_in[bus]->get_name();

    return "get_midi_in_bus_name(): error";
}
 */

/**
 *  Initiate a poll() on the existing poll descriptors.  This is a
 *  primitive poll, which exits when some data is obtained.
 */

int
mastermidibus::api_poll_for_midi ()
{
    for (;;)
    {
        for (int i = 0; i < m_num_in_buses; ++i)
        {
            if (m_buses_in[i]->poll_for_midi())
                return 1;
        }
        // Sleep(1);                      // yield processor for 1 millisecond
        millisleep(1);
        return 0;
    }
}

/**
 *  Test the ALSA sequencer to see if any more input is pending.
 *
 * \threadsafe
 */

bool
mastermidibus::api_is_more_input ()
{
    int size = 0;
    for (int i = 0; i < m_num_in_buses; i++)
    {
        if (m_buses_in[i]->poll_for_midi())
            size = 1;
    }
    return size > 0;
}

/**
 *  Grab a MIDI event.
 *
 * \threadsafe
 */

bool
mastermidibus::api_get_midi_event (event * in)
{
    bool result = false;
    PmEvent event;
    for (int i = 0; i < m_num_in_buses; i++)
    {
        if (m_buses_in[i]->poll_for_midi())
        {
            int /*PmError*/ err = Pm_Read(m_buses_in[i]->m_pms, &event, 1);
            if (err < 0)
                printf("Pm_Read: %s\n", Pm_GetErrorText((PmError) err));

            if (m_buses_in[i]->m_inputing)
                result = true;
        }
    }
    if (! result)
        return false;

    in->set_status(Pm_MessageStatus(event.message));

    // TO BE FIXED!!!!!!!!!!!!!!!!!!
    // in->set_size(3); !!!!!!!!!!!!

    in->set_data(Pm_MessageData1(event.message), Pm_MessageData2(event.message));

    /* some keyboards send Note On with velocity 0 for Note Off */

    if (in->get_status() == EVENT_NOTE_ON && in->get_note_velocity() == 0x00)
        in->set_status(EVENT_NOTE_OFF);

    // Why no "sysex = false" here, like in Linux version?

    return true;
}

}           // namespace seq64

/*
 * mastermidibus.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
