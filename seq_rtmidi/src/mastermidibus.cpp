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
 *  This module declares/defines the base class for MIDI I/O under the
 *  RtMidi framework.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2016-12-13
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
    mastermidibase      (ppqn, bpm),
    m_midi_scratch      ()
{
    // Empty body
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
 *  Initializes the RtMidi implementation.  Two different styles are
 *  supported.  If the --manual-alsa-ports option is in force, then 16 virtual
 *  output ports and one virtual input port are created.  Otherwise, the
 *  system MIDI input and output ports are scanned (via the rtmidi_info
 *  member) and passed to the midibus constructor calls.
 *
 *  Will need to add bpm as a parameter (for ALSA input at least).
 *
 *  Code currently roughly similar to midi_probe().  Assumes only one compiled
 *  API at present.
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
            std::string portname = tmp;
            m_buses_out[i] = new midibus
            (
                m_midi_scratch, SEQ64_APP_NAME /*clientname*/, portname, i,
                i, i, /* bus and port ID */ SEQ64_NO_QUEUE, ppqn, bpm
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

        std::string portname = "0";
        m_num_in_buses = 1;
        m_buses_in[0] = new midibus
        (
            m_midi_scratch, SEQ64_APP_NAME /*client name*/, portname, 0,
            0, 0, /* bus and port ID */ SEQ64_NO_QUEUE, ppqn, bpm
        );
        m_buses_in[0]->init_in_sub();
        m_buses_in_active[0] = m_buses_in_init[0] = true;
    }
    else
    {
        unsigned nports = m_midi_scratch.get_all_port_info();
#ifdef PLATFORM_DEBUG
        std::string plist = m_midi_scratch.port_list();
        printf("RtMidi ports found:\n%s\n", plist.c_str());
#endif
        if (nports > 0)
        {
            m_midi_scratch.midi_mode(SEQ64_MIDI_INPUT);
            unsigned inports = m_midi_scratch.get_port_count();
            m_num_in_buses = 0;
            for (unsigned i = 0; i < inports; ++i)
            {
                midibus * nextbus = new midibus
                (
                    m_midi_scratch, SEQ64_APP_NAME, m_num_in_buses, ppqn, bpm
                );
                if (nextbus->init_in())
                {
                    m_buses_in[m_num_in_buses] = nextbus;
                    m_buses_in_active[m_num_in_buses] = true;
                    m_buses_in_init[m_num_in_buses] = true;
                    ++m_num_in_buses;
                }
                else
                    delete nextbus;     // m_buses_in[m_num_in_buses] = nullptr;
            }

            m_midi_scratch.midi_mode(SEQ64_MIDI_OUTPUT);
            unsigned outports = m_midi_scratch.get_port_count();
            m_num_out_buses = 0;
            for (unsigned i = 0; i < outports; ++i)
            {
                midibus * nextbus = new midibus
                (
                    m_midi_scratch, SEQ64_APP_NAME, m_num_out_buses, ppqn, bpm
                );
                if (nextbus->init_out())
                {
                    m_buses_out[m_num_out_buses] = nextbus;
                    m_buses_out_active[m_num_out_buses] = true;
                    m_buses_out_init[m_num_out_buses] = true;
                    ++m_num_out_buses;
                }
                else
                    delete nextbus;
            }
        }
    }
    set_beats_per_minute(c_beats_per_minute);
    set_ppqn(ppqn);

    /*
     * Poll descriptor code moved to midi_alsa_info constructor.  MIDI
     * announce bus code not currently in place.
     */

    for (int i = 0; i < m_num_out_buses; ++i)
        set_clock(i, m_init_clock[i]);

    for (int i = 0; i < m_num_in_buses; ++i)
        set_input(i, m_init_input[i]);
}

/**
 *  Initiate a poll() on the existing poll descriptors.  This is a
 *  primitive poll, which exits when some data is obtained.
 */

int
mastermidibus::api_poll_for_midi ()
{
    for (;;)                    // why this clause???
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

