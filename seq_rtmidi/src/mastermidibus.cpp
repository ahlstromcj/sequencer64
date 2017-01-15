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
 * \updates       2017-01-14
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
    // Empty body
}

/**
 *  Initializes the RtMidi implementation.  Two different styles are
 *  supported.  If the --manual-alsa-ports option is in force, then 16 virtual
 *  output ports and one virtual input port are created.  They are given names
 *  that make it clear which application (seq64) has set them up.  They are
 *  not connected to anything.  The user will have to use a connection GUI
 *  (such as qjackctl) or a session manager to make the connections.
 *
 *  Otherwise, the system MIDI input and output ports are scanned (via the
 *  rtmidi_info member) and passed to the midibus constructor calls.  For
 *  every MIDI input port found on the system, this function creates a
 *  corresponding output port, and connects to the system MIDI input.  For
 *  example, for an input port found called "qmidiarp:in 1", we want to create
 *  a "shadow" output port called "seq64:qmidiarp in 1".
 *
 *  For every MIDI output found on the system this function creates a
 *  corresponding input port, and connects it to the system MIDI output.  For
 *  For example, for an output port found called "qmidiarp:out 1", we want to
 *  create a "shadow" input port called "seq64:qmidiarp out 1".
 *
 *  Are these good conventions, or potentially confusing to users?  They match
 *  what the legacy seq24 and sequencer64 do for ALSA.
 *
 * \todo
 *      MAKE SURE THIS REVERSAL DOES NOT BREAK ALSA PLAYBACK!
 *
 * \param ppqn
 *      Provides the (possibly new) value of PPQN to set.  ALSA has a function
 *      that sets its idea of the PPQN.  JACK, as far as we know, does not.
 *
 * \param bpm
 *      Provides the (possibly new) value of BPM (beats per minute) to set.
 *      ALSA has a function that sets its idea of the BPM.  JACK, as far as we
 *      know, does not.
 */

void
mastermidibus::api_init (int ppqn, int bpm)
{
    m_midi_scratch.api_set_ppqn(ppqn);
    m_midi_scratch.api_set_beats_per_minute(bpm);
    if (rc().manual_alsa_ports())
    {
        m_midi_scratch.clear();

        int num_buses = SEQ64_ALSA_OUTPUT_BUSS_MAX;     /* not just ALSA!   */
        for (int i = 0; i < num_buses; ++i)             /* output busses    */
        {
            /*
             * This code creates a midibus in the conventional manner.  Then
             * the busarray::add() function makes a new businfo object with
             * the desired "output" and "isvirtual" parameters; the businfo
             * object then decides whether to call init_in(), init_out(),
             * init_in_sub(), or init_out_sub().
             */

            midibus * m = new midibus
            (
                m_midi_scratch, i, SEQ64_MIDI_VIRTUAL_PORT, SEQ64_MIDI_OUTPUT
            );
            m_outbus_array.add(m, SEQ64_MIDI_OUTPUT, SEQ64_MIDI_VIRTUAL_PORT);
            m_midi_scratch.add_output(m);               /* must come 2nd    */
        }
        midibus * m = new midibus
        (
            m_midi_scratch, 0, SEQ64_MIDI_VIRTUAL_PORT, SEQ64_MIDI_INPUT
        );
        m_inbus_array.add(m, SEQ64_MIDI_INPUT, SEQ64_MIDI_VIRTUAL_PORT);
        m_midi_scratch.add_input(m);                    /* must come 2nd    */

#ifdef PLATFORM_DEBUG_XXX
        std::string plist = m_midi_scratch.port_list();
        printf
        (
            "%d virtual ports created:\n%s\n",
            m_midi_scratch.full_port_count(), plist.c_str()
        );
#endif
    }
    else
    {
        unsigned nports = m_midi_scratch.get_port_count();
#ifdef PLATFORM_DEBUG_XXX
        std::string plist = m_midi_scratch.port_list();
        printf
        (
            "%d rtmidi ports found:\n%s\n",
            m_midi_scratch.full_port_count(), plist.c_str()
        );
#endif
        if (nports > 0)
        {
            m_midi_scratch.midi_mode(SEQ64_MIDI_INPUT);
            unsigned inports = m_midi_scratch.get_port_count();
            for (unsigned i = 0; i < inports; ++i)
            {
                midibus * m = new midibus
                (
                    m_midi_scratch, i, SEQ64_MIDI_NORMAL_PORT, SEQ64_MIDI_OUTPUT
                );
                m_outbus_array.add(m, SEQ64_MIDI_OUTPUT, SEQ64_MIDI_NORMAL_PORT);
            }

            m_midi_scratch.midi_mode(SEQ64_MIDI_OUTPUT);
            unsigned outports = m_midi_scratch.get_port_count();
            for (unsigned i = 0; i < outports; ++i)
            {
                midibus * m = new midibus
                (
                    m_midi_scratch, i, SEQ64_MIDI_NORMAL_PORT, SEQ64_MIDI_INPUT
                );
                m_outbus_array.add(m, SEQ64_MIDI_INPUT, SEQ64_MIDI_NORMAL_PORT);
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
 *  Test the ALSA sequencer to see if any more input is pending.  Similar to
 *  api_poll_for_midi(), except it is threadsafe.  We got some cleanup to do!
 *
 * \threadsafe
 */

bool
mastermidibus::api_is_more_input ()
{
    automutex locker(m_mutex);
    return m_inbus_array.poll_for_midi();
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
}

}           // namespace seq64

/*
 * mastermidibus.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

