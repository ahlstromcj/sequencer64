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
 * \updates       2017-06-02
 * \license       GNU GPLv2 or above
 *
 *  This file provides a Windows-only implementation of the mastermidibus
 *  class.  There is a lot of common code between these two versions!
 */

#include "event.hpp"                    /* seq64::event                     */
#include "mastermidibus_pm.hpp"         /* seq64::mastermidibus, PortMIDI   */
#include "midibus_pm.hpp"               /* seq64::midibus, PortMIDI         */
#include "portmidi.h"                   /* external PortMidi header file    */
#include "porttime.h"                   /* Pt_Time_To_Pulses()              */
#include "pmutil.h"                     /* Pm_Dequeue()                     */

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
    /**
     * New features. Turn off exiting upon errors so that the application
     * has a chance to come up and display the error(s).  Set BPM and PPQN
     * in the PortMidi module.  The ppqn parameter defaults to -1 here.
     */

    Pm_set_exit_on_error(FALSE);
    Pt_Set_Midi_Timing(double(bpm), ppqn);              /* do this first    */
    Pm_Initialize();                                    /* do this second   */
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
 *  Here, we want to first make sure that the ports that the OS cannot access
 *  are disable, before we activate them.  Otherwise, they fail and prevent
 *  working ports from operating.
 */

bool
mastermidibus::activate ()
{
    bool result = mastermidibase::activate();
    Pm_print_devices();
    return result;
}

/**
 *  Provides the PortMidi implementation needed for the init() function.
 *  Unlike the seq24 ALSA implementation, this version does NOT support the
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
    int num_devices = Pm_device_count();    /* Pm_CountDevices()    */
    int numouts = 0;
    int numins = 0;
    for (int i = 0; i < num_devices; ++i)
    {
        const PmDeviceInfo * dev_info = Pm_GetDeviceInfo(i);
        if (dev_info->output)
        {
            /*
             * The parameters here are the bus index (within the input or output
             * busarry), the bus ID (currently identical to the bus index,
             * hmmmmm), the port ID, and the client name.
             */

            midibus * m = new midibus(numouts, numouts, i, dev_info->name);
            m->is_input_port(false);
            m->is_virtual_port(false);
            m_outbus_array.add(m, clock(numouts));      /* not i    */
            ++numouts;
        }
        else if (dev_info->input)
        {
            /*
             * The parameters here are bus index, bus ID, port ID, and client
             * name.
             */

            midibus * m = new midibus(numins, numins, i, dev_info->name);
            m->is_input_port(true);
            m->is_virtual_port(false);
            m_inbus_array.add(m, input(numins));        /* not i    */
            ++numins;
        }
    }

    set_beats_per_minute(c_beats_per_minute);
    set_ppqn(ppqn);
    set_sequence_input(false, nullptr);

#ifdef USE_ANNOUNCE_BUS_WITH_PORTMIDI           /* what does this bus DO?   */
    m_bus_announce = new midibus
    (
        snd_seq_client_id(m_alsa_seq),
        SND_SEQ_CLIENT_SYSTEM, SND_SEQ_PORT_SYSTEM_ANNOUNCE,
        m_alsa_seq, "system", "announce",
        0, m_queue, ppqn, bpm
    );
    m_bus_announce->set_input(true);
#endif

    m_outbus_array.set_all_clocks();
    m_inbus_array.set_all_inputs();
}

/**
 *  Grab a MIDI event.  This function ssumes that [api_]poll_for_midi() has
 *  been called to "prime the pump".
 *
 * \param in
 *      Provides the destination point for the event to be filled.
 *
 * \return
 *      Returns true if there was no error and an event was obtained.
 */

bool
mastermidibus::api_get_midi_event (event * in)
{
    bool result = false;
    int count = m_inbus_array.count();
    for (int i = 0; i < count; ++i)
    {
        midibus * m = m_inbus_array.bus(i);
        if (m->m_inputing)
        {
            PmEvent pme;
            PmInternal * midi = (PmInternal *) m->m_pms;
            PmError err = Pm_Dequeue(midi->queue, &pme);
            if (err == pmBufferOverflow)        /* ignore data retrieved    */
            {
                result = false; /* pm_errmsg(pmBufferOverflow, deviceid);   */
            }
            else if (err == 0)                  /* empty queue              */
            {
                result = false;
            }
            else
            {
                /*
                 * We don't need to do this.  The perform input loop
                 * sets the timestamp.  Let's hope that loop can keep up!
                 *
                 * midipulse ts = midipulse(Pt_Time_To_Pulses(pme.timestamp));
                 * in->set_timestamp(ts);
                 */

                in->set_status_keep_channel(Pm_MessageStatus(pme.message));
                in->set_data
                (
                    Pm_MessageData1(pme.message), Pm_MessageData2(pme.message)
                );
                if (in->is_note_off_recorded())
                {
                    midibyte channel = Pm_MessageStatus(pme.message) &
                        EVENT_GET_CHAN_MASK;

                    midibyte status = EVENT_NOTE_OFF | channel;
                    in->set_status_keep_channel(status);
                }
                result = true;
            }
        }
    }
    if (! result)
        return false;

    /*
     * Some keyboards send Note On with velocity 0 for Note Off.  The event
     * class already has this check available.
     *
     * if (in->get_status() == EVENT_NOTE_ON && in->get_note_velocity() == 0x00)
     */

    if (in->is_note_off_recorded())
        in->set_status(EVENT_NOTE_OFF);

    return true;                        /* Why no "sysex = false"?  */
}

/**
 *  Not yet implemented.
 */

void
mastermidibus::api_set_ppqn (int /*ppqn*/)
{
    // TODO
}

/**
 *  Not yet implemented.
 */

void
mastermidibus::api_set_beats_per_minute (midibpm /*bpm*/)
{
    // TODO
}

}           // namespace seq64

/*
 * mastermidibus.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

