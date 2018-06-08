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
 *  refactored RtMidi framework.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2017-05-31
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

mastermidibus::mastermidibus (int ppqn, midibpm bpm)
 :
    mastermidibase      (ppqn, bpm),
    m_midi_master
    (
        rc().with_jack_midi() ? RTMIDI_API_UNIX_JACK : RTMIDI_API_LINUX_ALSA,
        rc().application_name(), ppqn, bpm
    ),
    m_use_jack_polling  (rc().with_jack_midi())
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
 *  This code creates a midibus in the conventional manner.  Then the
 *  busarray::add() function makes a new businfo object with the desired
 *  "output" and "isvirtual" parameters; the businfo object then decides
 *  whether to call init_in(), init_out(), init_in_sub(), or init_out_sub().
 *
 *  Are these good conventions, or potentially confusing to users?  They
 *  match what the legacy seq24 and sequencer64 do for ALSA.
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
mastermidibus::api_init (int ppqn, midibpm bpm)
{
    m_midi_master.api_set_ppqn(ppqn);
    m_midi_master.api_set_beats_per_minute(bpm);
    if (rc().manual_alsa_ports())                       /* virtual ports    */
    {
        int num_buses = SEQ64_ALSA_OUTPUT_BUSS_MAX;     /* not just ALSA!   */
        m_midi_master.clear();                          /* ignore system    */
        for (int i = 0; i < num_buses; ++i)             /* output busses    */
        {
            midibus * m = new midibus
            (
                m_midi_master, i, SEQ64_MIDI_VIRTUAL_PORT,
                SEQ64_MIDI_OUTPUT_PORT,

                /*
                 * ca 2017-03-12 Let's put in an invalid value or
                 * somehow get a client number from the (ALSA) API....
                 */

                i /* bussoverride */                    /* breaks ALSA?     */
            );
            m_outbus_array.add(m, clock(i));            /* must come 1st    */
            m_midi_master.add_output(m);                /* must come 2nd    */
        }
        midibus * m = new midibus
        (
            m_midi_master, 0, SEQ64_MIDI_VIRTUAL_PORT, SEQ64_MIDI_INPUT_PORT
        );
        m_inbus_array.add(m, input(0));                 /* must come 1st    */
        m_midi_master.add_input(m);                     /* must come 2nd    */
        port_list("virtual");
    }
    else
    {
        unsigned nports = m_midi_master.full_port_count();
        bool swap_io = rc().with_jack_midi();
        bool isinput = swap_io ? SEQ64_MIDI_OUTPUT_PORT : SEQ64_MIDI_INPUT_PORT;
        bool isoutput = swap_io ? SEQ64_MIDI_INPUT_PORT : SEQ64_MIDI_OUTPUT_PORT;
        port_list("rtmidi");
        if (nports > 0)
        {
            m_midi_master.midi_mode(SEQ64_MIDI_INPUT_PORT);     /* ugh! */
            unsigned inports = m_midi_master.get_port_count();
            for (unsigned i = 0; i < inports; ++i)
            {
                bool isvirtual = m_midi_master.get_virtual(i);
                bool issystem = m_midi_master.get_system(i);
                midibus * m = new midibus
                (
                    m_midi_master, i, isvirtual, isinput,
                    SEQ64_NO_BUS, issystem
                );
                if (swap_io)
                    m_outbus_array.add(m, clock(i));    /* must come 1st    */
                else
                    m_inbus_array.add(m, input(i));     /* must come 1st    */

                m_midi_master.add_bus(m);               /* must come 2nd    */
            }

            m_midi_master.midi_mode(SEQ64_MIDI_OUTPUT_PORT);    /* ugh! */
            unsigned outports = m_midi_master.get_port_count();
            for (unsigned i = 0; i < outports; ++i)
            {
                bool isvirtual = m_midi_master.get_virtual(i);
                bool issystem = m_midi_master.get_system(i);
                midibus * m = new midibus
                (
                    m_midi_master, i, isvirtual, isoutput,
                    SEQ64_NO_BUS, issystem
                );
                if (swap_io)
                    m_inbus_array.add(m, input(i));     /* must come 1st    */
                else
                    m_outbus_array.add(m, clock(i));    /* must come 1st    */

                m_midi_master.add_bus(m);               /* must come 2nd    */
            }
        }
    }
    set_beats_per_minute(bpm);                          // c_beats_per_minute
    set_ppqn(ppqn);

    /*
     * Deferred until later in startup.  See the comment here in the
     * seq_alsamidi version of this module.
     *
     * m_outbus_array.set_all_clocks();
     * m_inbus_array.set_all_inputs();
     */
}

/**
 *  Shows a list of discovered ports in debug mode.
 *
 * \param tag
 *      Provides a string label indicate the context of this list.
 */

#ifdef PLATFORM_DEBUG   // _TMI

void
mastermidibus::port_list (const std::string & tag)
{
    std::string plist = m_midi_master.port_list();
    printf
    (
        "%d %s ports created:\n%s\n",
        m_midi_master.full_port_count(), tag.c_str(), plist.c_str()
    );
}

#else

void
mastermidibus::port_list (const std::string & )
{
    // No code if not debugging
}

#endif  // PLATFORM_DEBUG_TMI

/**
 *  Activates the mastermidibase code and the rtmidi_info object via its
 *  api_connect() function.
 */

bool
mastermidibus::activate ()
{
    bool result = mastermidibase::activate();
    if (result)
        result = m_midi_master.api_connect();      /* activates, too    */

    return result;
}

/**
 *  Initiate a poll() on the existing poll descriptors.  This is a
 *  primitive poll, which exits when some data is obtained, or sleeps a
 *  millisecond in note data is obtained.
 *
 * \return
 *      Returns the number of input MIDI events waiting.
 */

int
mastermidibus::api_poll_for_midi ()
{
    if (m_use_jack_polling)
        return mastermidibase::api_poll_for_midi(); /* default poll */
    else
        return m_midi_master.api_poll_for_midi();
}

/**
 *  Grab a MIDI event.  For the ALSA implementation, this call
 *
 * \threadsafe
 */

bool
mastermidibus::api_get_midi_event (event * inev)
{
    if (m_use_jack_polling)
        return m_inbus_array.get_midi_event(inev);
    else
        return m_midi_master.api_get_midi_event(inev);
}

}           // namespace seq64

/*
 * mastermidibus.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

