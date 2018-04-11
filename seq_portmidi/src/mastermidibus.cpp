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
 * \updates       2017-03-23
 * \license       GNU GPLv2 or above
 *
 *  This file provides a Windows-only implementation of the mastermidibus
 *  class.  There is a lot of common code between these two versions!
 */

#include "easy_macros.h"                /* handy macros                     */
#include "event.hpp"                    /* seq64::event                     */
#include "mastermidibus_pm.hpp"         /* seq64::mastermidibus, PortMIDI   */
#include "midibus_pm.hpp"               /* seq64::midibus, PortMIDI         */
#include "portmidi.h"                   /* external PortMidi header file    */

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

mastermidibus::mastermidibus (int ppqn, midibpm bpm)
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
 *  Unlike the seq24 ALSA implementation, this version does not support the
 *  --manual-alsa-ports option.  It initializes as many input and output MIDI
 *  devices as are found by Pm_CountDevices(), and the flags
 *  PmDeviceInfo::input and output determine what category of MIDI device it
 *  is.
 *
 * \todo
 *      We still need to reset the PPQN and BPM values via the ALSA API
 *      if they are different here!  See the "rtmidi" implementation of
 *      this function.
 *
 * \param ppqn
 *      The PPQN value to which to initialize the master MIDI buss.
 *
 * \param bpm
 *      The BPM value to which to initialize the master MIDI buss, if
 *      applicable.
 */

void
mastermidibus::api_init (int ppqn, midibpm /*bpm*/)
{
    int num_devices = Pm_CountDevices();
    const PmDeviceInfo * dev_info = nullptr;
    int numouts = 0;
    int numins = 0;
    for (int i = 0; i < num_devices; ++i)
    {
        dev_info = Pm_GetDeviceInfo(i);

#ifdef PLATFORM_DEBUG   // _TMI
        fprintf
        (
            stderr, "[%s device %d: %s in:%d out:%d\n",
            dev_info->interf, i, dev_info->name,
            dev_info->input, dev_info->output
        );
#endif

        if (dev_info->output)
        {
            /*
             * The parameters here are bus ID, port ID, and client name.
             */

            midibus * m = new midibus
            (
                i, numouts, i, dev_info->name   // false, false NEEDED
            );
            m->is_virtual_port(false);
            m->is_input_port(false);
            m_outbus_array.add(m, clock(i));
            ++numouts;
        }
        else if (dev_info->input)
        {
            /*
             * The parameters here are bus ID, port ID, and client name.
             */

            midibus * m = new midibus
            (
                i, numins, i, dev_info->name    // true, false NEEDED
            );
            m->is_virtual_port(false);
            m->is_input_port(true);
            m_inbus_array.add(m, input(i));
            ++numins;
        }
    }

    set_beats_per_minute(c_beats_per_minute);
    set_ppqn(ppqn);                             // m_ppqn); SEQ64_DEFAULT_PPQN);
    set_sequence_input(false, nullptr);

#if 0                                           // what does this bus DO?
    m_bus_announce = new midibus
    (
        snd_seq_client_id(m_alsa_seq),
        SND_SEQ_CLIENT_SYSTEM, SND_SEQ_PORT_SYSTEM_ANNOUNCE,
        m_alsa_seq, "system", "announce",   // was "annouce" ca 2016-04-03
        0, m_queue, ppqn, bpm
    );
    m_bus_announce->set_input(true);
#endif

    m_outbus_array.set_all_clocks();
    m_inbus_array.set_all_inputs();
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
        if (m_inbus_array.poll_for_midi())
            return 1;

        millisleep(1);
        return 0;
    }
}

/**
 *  Test the ALSA sequencer to see if any more input is pending.
 *
 * \threadunsafe
 *      Why is this version not protected by a mutex?  The seq_alsamidi and
 *      seq_rtmidi versions are protected by one!
 */

bool
mastermidibus::api_is_more_input ()
{
    return m_inbus_array.poll_for_midi();
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
    int count = m_inbus_array.count();
    for (int i = 0; i < count; ++i)
    {
        midibus * m = m_inbus_array.bus(i);
        if (m->poll_for_midi())
        {
            int /*PmError*/ err = Pm_Read(m->m_pms, &event, 1);
            if (err < 0)
                printf("Pm_Read: %s\n", Pm_GetErrorText((PmError) err));

            if (m->m_inputing)
                result = true;
        }
    }
    if (! result)
        return false;

    in->set_status(Pm_MessageStatus(event.message));
    in->set_sysex_size(3);
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

