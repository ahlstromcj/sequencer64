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
 * \updates       2016-12-03
 * \license       GNU GPLv2 or above
 *
 *  This file provides a Windows-only implementation of the mastermidibus
 *  class.  There is a lot of common code between these two versions!
 */

#include "event.hpp"                    /* seq64::event                     */
#include "mastermidibus_rm.hpp"         /* seq64::mastermidibus, RtMIDI     */
#include "midibus_rm.hpp"               /* seq64::midibus, RtMIDI           */
#include "settings.hpp"                 /* seq64::rc() and choose_ppqn()    */

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
    // Pm_Initialize();
}

/**
 *  The destructor deletes all of the output busses, and terminates the
 *  Windows MIDI manager.
 */

mastermidibus::~mastermidibus ()
{
    // Pm_Terminate();
}

/**
 *  Initializes the RtMidi implementation.
 *
 *  Will need to add bpm as a parameter (for ALSA input at least)
 */

void
mastermidibus::api_init (int ppqn, int bpm)
{
    if (rc().manual_alsa_ports())
    {
        int num_buses = SEQ64_ALSA_OUTPUT_BUSS_MAX;
        for (int i = 0; i < num_buses; ++i)         /* output busses    */
        {
            if (not_nullptr(m_buses_out[i]))
            {
                delete m_buses_out[i];
                errprintf("manual: m_buses_out[%d] not null\n", i);
            }
            char tmp[4];
            snprintf(tmp, sizeof tmp, "%d", i);
            std::string clientname = "rtmidi out";
            std::string portname = tmp;
            m_buses_out[i] = new midibus
            (
                clientname, portname, i, i, /* i is bus ID and port ID here */
                SEQ64_NO_QUEUE, ppqn, bpm
            );
            m_buses_out[i]->init_out_sub();
            m_buses_out_active[i] = m_buses_out_init[i] = true;
        }
        m_num_out_buses = num_buses;
        if (not_nullptr(m_buses_in[0]))
        {
            delete m_buses_in[0];
            errprint("manual: m_buses_[0] not null");
        }

        /*
         * Input buss.  Only the first element is set up.  The rest are used
         * only for non-manual ALSA ports in the else-class below.
         */

        std::string clientname = "rtmidi out";
        std::string portname = "0";
        m_num_in_buses = 1;
        m_buses_in[0] = new midibus
        (
            clientname, portname, 0,
            SEQ64_NO_PORT, SEQ64_NO_QUEUE, ppqn
        );
        m_buses_in[0]->init_in_sub();
        m_buses_in_active[0] = m_buses_in_init[0] = true;
    }
    else
    {
        /*
         * Code currently similar to midi_probe().  Assumes only one compiled
         * API at present.
         */

        rtmidi_in in;
        unsigned nports = in.get_port_count();
        m_num_in_buses = 0;
        for (unsigned i = 0; i < nports; ++i)
        {
            std::string clientname = "rtmidi in";
            std::string portname = in.get_port_name(i);
            m_buses_in[m_num_in_buses] = new midibus
            (
                clientname, portname, m_num_in_buses,
                SEQ64_NO_PORT, SEQ64_NO_QUEUE, ppqn
            );
            if (m_buses_in[m_num_in_buses]->init_in())
            {
                m_buses_in_active[m_num_in_buses] = true;
                m_buses_in_init[m_num_in_buses] = true;
                ++m_num_in_buses;
            }
            else
            {
                delete m_buses_in[m_num_in_buses];
                m_buses_in[m_num_in_buses] = nullptr;
            }
        }

        rtmidi_out out;
        nports = out.get_port_count();
        m_num_out_buses = 0;
        for (unsigned i = 0; i < nports; ++i)
        {
            std::string clientname = "rtmidi out";
            std::string portname = out.get_port_name(i);
            m_buses_out[m_num_out_buses] = new midibus
            (
                clientname, portname, m_num_out_buses,
                SEQ64_NO_PORT, SEQ64_NO_QUEUE, ppqn
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
    }

    set_beats_per_minute(c_beats_per_minute);       // ????????
    set_ppqn(ppqn);     // m_ppqn);   // SEQ64_DEFAULT_PPQN);

    /*
     * MIDI input poll descriptors
     *
     *
        m_bus_announce = new midibus
        (
            "system", "announce",   // clientname and portname
            0, m_queue, ppqn
        );
        m_bus_announce->set_input(true);

        set_sequence_input(false, NULL);
        for (int i = 0; i < m_num_out_buses; i++)
            set_clock(i, m_init_clock[i]);

        for (int i = 0; i < m_num_in_buses; i++)
            set_input(i, m_init_input[i]);
     *
     */
}

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
    automutex locker(m_mutex);
    int size = 0;
    for (int i = 0; i < m_num_in_buses; ++i)
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
    for (int i = 0; i < m_num_in_buses; ++i)
    {
        if (m_buses_in[i]->poll_for_midi())
        {
//          int /*PmError*/ err = Pm_Read(m_buses_in[i]->m_pms, &event, 1);
//          if (err < 0)
//              printf("Pm_Read: %s\n", Pm_GetErrorText((PmError) err));

            if (m_buses_in[i]->m_inputing)
                result = true;
        }
    }
    if (! result)
        return false;

//  in->set_status(Pm_MessageStatus(event.message));
//  in->set_sysex_size(3);
//  in->set_data(Pm_MessageData1(event.message), Pm_MessageData2(event.message));

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

