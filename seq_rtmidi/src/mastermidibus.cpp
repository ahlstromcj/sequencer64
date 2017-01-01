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
 * \updates       2017-01-01
 * \license       GNU GPLv2 or above
 *
 *  This file provides a Windows-only implementation of the mastermidibus
 *  class.  There is a lot of common code between these two versions!
 */

#include "easy_macros.h"

#ifdef SEQ64_HAVE_LIBASOUND
#include <sys/poll.h>
#endif

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
    m_midi_scratch      (RTMIDI_API_UNSPECIFIED, SEQ64_APP_NAME, ppqn, bpm)
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
 *
 * \param ppqn
 *      Provides the (possibly new) value of PPQN to set.
 *
 * \param bpm
 *      Provides the (possibly new) value of BPM (beats per minute)  to set.
 */

void
mastermidibus::api_init (int ppqn, int bpm)
{
    m_midi_scratch.api_set_ppqn(ppqn);
    m_midi_scratch.api_set_beats_per_minute(bpm);
    if (rc().manual_alsa_ports())
    {
        int num_buses = SEQ64_ALSA_OUTPUT_BUSS_MAX;
        for (int i = 0; i < num_buses; ++i)             /* output busses    */
        {
#ifdef USE_BUS_ARRAY_CODE
            /*
             * This code creates a midibus in the conventional manner.  Then
             * the busarray::add() function makes a new businfo object with
             * the desired "output" and "isvirtual" parameters; the businfo
             * object then decides whether to call init_in(), init_out(),
             * init_in_sub(), or init_out_sub().
             */

            midibus * m = new midibus                   /* virtual port     */
            (
                m_midi_scratch, SEQ64_APP_NAME, i  /* index and port */
            );
            m_outbus_array.add(m, false, true);         /* output & virtual */
#else
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
                m_midi_scratch, SEQ64_APP_NAME, i  /* index and port */
            );
            m_buses_out[i]->init_out_sub();
            m_buses_out_active[i] = m_buses_out_init[i] = true;
#endif
        }
#ifndef USE_BUS_ARRAY_CODE
        m_num_out_buses = num_buses;
#endif
#ifdef USE_BUS_ARRAY_CODE
        midibus * m = new midibus                       /* virtual port     */
        (
            m_midi_scratch, SEQ64_APP_NAME, 0           /* index and port */
        );
        m_inbus_array.add(m, true, true);               /* input & virtual  */
#else
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
            m_midi_scratch, SEQ64_APP_NAME, 0           /* index and port */
        );
        m_buses_in[0]->init_in_sub();
        m_buses_in_active[0] = m_buses_in_init[0] = true;
#endif
    }
    else
    {
        unsigned nports = m_midi_scratch.get_all_port_info();
#ifdef PLATFORM_DEBUG_XXX
        std::string plist = m_midi_scratch.port_list();
        printf("rtmidi ports found:\n%s\n", plist.c_str());
#endif
        if (nports > 0)
        {
            m_midi_scratch.midi_mode(SEQ64_MIDI_INPUT);
            unsigned inports = m_midi_scratch.get_port_count();
#ifndef USE_BUS_ARRAY_CODE
            m_num_in_buses = 0;
#endif
            for (unsigned i = 0; i < inports; ++i)
            {
#ifdef USE_BUS_ARRAY_CODE
                midibus * m = new midibus(m_midi_scratch, i);
                m_inbus_array.add(m, true, false);        /* input, nonvirt */
#else
                midibus * m = new midibus(m_midi_scratch, m_num_in_buses);
                if (m->init_in())
                {
                    m_buses_in[m_num_in_buses] = m;     /* different!     */
                    m_buses_in_active[m_num_in_buses] = true;
                    m_buses_in_init[m_num_in_buses] = true;
                    ++m_num_in_buses;
                }
                else
                    delete m;
#endif
            }

            m_midi_scratch.midi_mode(SEQ64_MIDI_OUTPUT);
            unsigned outports = m_midi_scratch.get_port_count();
#ifndef USE_BUS_ARRAY_CODE
            m_num_out_buses = 0;
#endif
            for (unsigned i = 0; i < outports; ++i)
            {
#ifdef USE_BUS_ARRAY_CODE
                midibus * m = new midibus(m_midi_scratch, i);
                m_outbus_array.add(m, false, false);     /* output, nonvirt */
#else
                midibus * m = new midibus(m_midi_scratch, m_num_out_buses);
                if (m->init_out())
                {
                    m_buses_out[m_num_out_buses] = m;
                    m_buses_out_active[m_num_out_buses] = true;
                    m_buses_out_init[m_num_out_buses] = true;
                    ++m_num_out_buses;
                }
                else
                    delete m;
#endif
            }
        }
    }
    set_beats_per_minute(bpm);                  // c_beats_per_minute
    set_ppqn(ppqn);

    /*
     * Poll descriptor code moved to midi_alsa_info constructor.  MIDI
     * announce bus code not currently in place.
     */

#if 0                               // we need an announce-bus constructor
    m_bus_announce = new midibus
    (
        snd_seq_client_id(m_alsa_seq),
        SND_SEQ_CLIENT_SYSTEM, SND_SEQ_PORT_SYSTEM_ANNOUNCE,
        m_alsa_seq, "system", "announce",   // was "annouce" ca 2016-04-03
        0, m_queue, ppqn, bpm
    );
    m_bus_announce->set_input(true);
#endif

#ifdef USE_BUS_ARRAY_CODE
    m_outbus_array.set_all_clocks();
    m_inbus_array.set_all_inputs();
#else
    for (int i = 0; i < m_num_out_buses; ++i)
        set_clock(i, m_init_clock[i]);

    for (int i = 0; i < m_num_in_buses; ++i)
        set_input(i, m_init_input[i]);
#endif
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
#ifdef USE_BUS_ARRAY_CODE
        if (m_inbus_array.poll_for_midi())
            return 1;
#else
        for (int i = 0; i < m_num_in_buses; ++i)
        {
            if (m_buses_in[i]->poll_for_midi())
                return 1;
        }
#endif
        millisleep(1);
        return 0;
    }
}

/**
 *  Test the ALSA sequencer to see if any more input is pending.  Similar to
 *  api_poll_for_midi(), except it is threadsafe.  We got some cleanup to do!
 *
 * \threadsafe
 */

bool
mastermidibus::api_is_more_input ()
{
    automutex locker(m_mutex);
#ifdef USE_BUS_ARRAY_CODE
    return m_inbus_array.poll_for_midi();
#else
    int size = 0;
    for (int i = 0; i < m_num_in_buses; ++i)
    {
        if (m_buses_in[i]->poll_for_midi())
            size = 1;
    }
    return size > 0;
#endif
}

/**
 *  Grab a MIDI event.
 *
 * \threadsafe
 */

bool
mastermidibus::api_get_midi_event (event * inev)
{
    return m_midi_scratch.api_get_midi_event(inev);

#if 0
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

    /*
     * Some keyboards send Note On with velocity 0 for Note Off.
     */

    if (in->is_note_on() && in->get_note_velocity() == 0x00)
        in->set_status(EVENT_NOTE_OFF);

    return true; // Why no "sysex = false" here, like in Linux version?
#endif  // 0
}

}           // namespace seq64

/*
 * mastermidibus.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

