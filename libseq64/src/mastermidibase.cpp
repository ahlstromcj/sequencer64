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
 * \file          mastermidibase.cpp
 *
 *  This module declares/defines the base class for handling MIDI I/O via
 *  the ALSA system.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2016-11-23
 * \updates       2018-06-02
 * \license       GNU GPLv2 or above
 *
 *  This file provides a base-class implementation for various master MIDI
 *  buss support classes.  There is a lot of common code between these MIDI
 *  buss classes.
 */

#include "calculations.hpp"             /* seq64::extract_port_names()      */
#include "easy_macros.h"
#include "event.hpp"                    /* seq64::event                     */
#include "mastermidibase.hpp"           /* seq64::mastermidibase            */
#include "sequence.hpp"                 /* seq64::sequence                  */
#include "settings.hpp"                 /* seq64::rc() and choose_ppqn()    */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *  The mastermidibase default constructor fills the array with our busses.
 *
 * \param ppqn
 *      Provides the PPQN value for this object.  However, in most cases, the
 *      default, SEQ64_USE_DEFAULT_PPQN should be specified.  Then the caller
 *      of this constructor should call mastermidibase::set_ppqn() to set up
 *      the proper PPQN value.
 *
 * \param bpm
 *      Provides the beats per minute value, which defaults to
 *      c_beats_per_minute.
 */

mastermidibase::mastermidibase
(
    int ppqn,
    midibpm bpm
) :
    m_max_busses        (c_max_busses),
    m_bus_announce      (nullptr),      /* used only for ALSA announce bus  */
    m_inbus_array       (),
    m_outbus_array      (),
    m_master_clocks     (),
    m_master_inputs     (),
    m_queue             (0),
    m_ppqn              (choose_ppqn(ppqn)),
    m_beats_per_minute  (bpm),          /* beats per minute                 */
    m_dumping_input     (false),
    m_vector_sequence   (),             /* stazed feature                   */
    m_filter_by_channel (false),        /* set based on configuration       */
    m_seq               (nullptr),
    m_mutex             ()
{
    // Empty body now
}

/**
 *  The virtual destructor deletes all of the output busses, clears out the
 *  ALSA events, stops and frees the queue, and closes ALSA for this
 *  application.
 *
 *  Valgrind indicates we might have issues caused by the following functions:
 *
 *      -   snd_config_hook_load()
 *      -   snd_config_update_r() via snd_seq_open()
 *      -   _dl_init() and other GNU function
 *      -   init_gtkmm_internals() [version 2.4]
 */

mastermidibase::~mastermidibase ()
{
    if (not_nullptr(m_bus_announce))
    {
        delete m_bus_announce;
        m_bus_announce = nullptr;
    }
}

/**
 *  Starts all of the configured output busses up to m_num_out_buses.  Calls
 *  the implementation-specific API function for starting.
 *
 * \threadsafe
 */

void
mastermidibase::start ()
{
    automutex locker(m_mutex);
    api_start();
    m_outbus_array.start();
}

/**
 *  Gets the MIDI output busses running again.  This function calls the
 *  implementation-specific API function, and then calls
 *  midibus::continue_from() for all of the MIDI output busses.
 *
 * \threadsafe
 *
 * \param tick
 *      Provides the tick value to continue from.
 */

void
mastermidibase::continue_from (midipulse tick)
{
    automutex locker(m_mutex);
    api_continue_from(tick);
    m_outbus_array.continue_from(tick);
}

/**
 *  Initializes the clock of each of the MIDI output busses.  Calls the
 *  implementation-specific API function, and then calls midibus::init_clock()
 *  for each of the MIDI output busses.
 *
 * \threadsafe
 *
 * \param tick
 *      Provides the tick value with which to initialize the buss clock.
 */

void
mastermidibase::init_clock (midipulse tick)
{
    automutex locker(m_mutex);
    api_init_clock(tick);
    m_outbus_array.init_clock(tick);
}

/**
 *  Stops each of the MIDI output busses.  Then calls the
 *  implementation-specific API function to finalize the stoppage. (See the
 *  ALSA implementation in the seq_alsamidi library, for example.  It is the
 *  original Sequencer64 implementation.)
 *
 * \threadsafe
 */

void
mastermidibase::stop ()
{
    automutex locker(m_mutex);
    m_outbus_array.stop();
    api_stop();
}

/**
 *  Generates the MIDI clock for each of the output busses.  Also calls the
 *  api_clock() function, which does nothing for the <i> original </i> ALSA
 *  implementation and the PortMidi implementation.
 *
 * \threadsafe
 *
 * \param tick
 *      Provides the tick value with which to set the buss clock.
 */

void
mastermidibase::emit_clock (midipulse tick)
{
    automutex locker(m_mutex);

    /*
     * Doesn't do anything: api_clock();
     */

    m_outbus_array.clock(tick);
}

/**
 *  Set the PPQN value (parts per quarter note). Then call the
 *  implementation-specific API function to complete the PPQN setting.
 *
 * \threadsafe
 *
 * \param ppqn
 *      The PPQN value to be set.
 */

void
mastermidibase::set_ppqn (int ppqn)
{
    automutex locker(m_mutex);
    m_ppqn = choose_ppqn(ppqn);                     /* m_ppqn = ppqn        */
    api_set_ppqn(ppqn);
}

/**
 *  Set the BPM value (beats per minute).  Then call the
 *  implementation-specific API function to complete the BPM setting.
 *
 * \threadsafe
 *
 * \param bpm
 *      Provides the beats-per-minute value to set.
 */

void
mastermidibase::set_beats_per_minute (midibpm bpm)
{
    automutex locker(m_mutex);
    m_beats_per_minute = bpm;
    api_set_beats_per_minute(bpm);
}

/**
 *  Flushes our local queue events out  The implementation-specific API
 *  function is called.  For example, ALSA provides a function to "drain" the
 *  output.
 *
 * \threadsafe
 */

void
mastermidibase::flush ()
{
    automutex locker(m_mutex);
    api_flush();
}

/**
 *  Stops all notes on all channels on all busses.  Adapted from Oli Kester's
 *  Kepler34 project.
 */

void
mastermidibase::panic ()
{
    event e;
    e.set_status(EVENT_NOTE_OFF);
    flush();
    for (int bus = 0; bus < SEQ64_DEFAULT_BUSS_MAX; ++bus)
    {
        for (int channel = 0; channel < SEQ64_MIDI_CHANNEL_MAX; ++channel)
        {
            for (int note = 0; note < SEQ64_MIDI_COUNT_MAX; ++note)
            {
                e.set_data(note, SEQ64_MAX_NOTE_ON_VELOCITY);   /* 0 better? */
                play(bus, &e, channel);
            }
        }
    }
}

/**
 *  Handle the sending of SYSEX events.  The event is sent to all MIDI output
 *  busses.  Then flush() is called.
 *
 *  There's currently no implementation-specific API function for this call.
 *
 * \threadsafe
 *
 * \param ev
 *      Provides the event pointer to be set.
 */

void
mastermidibase::sysex (event * ev)
{
    automutex locker(m_mutex);
    m_outbus_array.sysex(ev);
    flush();                /* recursive locking! */
}

/**
 *  Handle the playing of MIDI events on the MIDI buss given by the
 *  parameter, as long as it is a legal buss number.
 *
 *  There's currently no implementation-specific API function here.
 *
 * \threadsafe
 *
 * \param bus
 *      The buss to start play on.  Ooh, we just noticed that value should be
 *      checked before usage!
 *
 * \param e24
 *      The seq24 event to play on the buss.  For speed, we don't bother to
 *      check the pointer.
 *
 * \param channel
 *      The channel on which to play the event.
 */

void
mastermidibase::play (bussbyte bus, event * e24, midibyte channel)
{
    automutex locker(m_mutex);
    m_outbus_array.play(bus, e24, channel);
}

/**
 *  Set the clock for the given (legal) buss number.  The legality checks
 *  are a little loose, however.
 *
 *  There's currently no implementation-specific API function here.
 *
 * \threadsafe
 *
 * \param bus
 *      The buss to start play on.  Checked before usage.
 *
 * \param clocktype
 *      The type of clock to be set, either "off", "pos", or "mod", as noted
 *      in the midibus_common module.
 */

bool
mastermidibase::set_clock (bussbyte bus, clock_e clocktype)
{
    automutex locker(m_mutex);
    bool result = m_outbus_array.set_clock(bus, clocktype);
    if (result)
        result = save_clock(bus, clocktype);    /* save into the vector */

    return result;
}

/**
 *  Saves the given clock value in m_master_clocks[bus].
 *
 * \param bus
 *      Provides the desired buss to be set.
 *
 * \param clock
 *      Provides the clocking value to set.
 *
 * \return
 *      Returns true if the buss value is valid.
 */

bool
mastermidibase::save_clock (bussbyte bus, clock_e clock)
{
    bool result = bus < int(m_master_clocks.size());
    if (result)
        m_master_clocks[bus] = clock;

    return result;
}

/**
 *  Gets the clock setting for the given (legal) buss number.
 *
 *  There's currently no implementation-specific API function here.
 *
 * \param bus
 *      Provides the buss number to read.  Checked before usage.
 *
 * \return
 *      If the buss number is legal, and the buss is active, then its clock
 *      setting is returned.  Otherwise, e_clock_disabled is returned.
 */

clock_e
mastermidibase::get_clock (bussbyte bus)
{
    return m_outbus_array.get_clock(bus);
}

/**
 *  Set the status of the given input buss, if a legal buss number.
 *  Why is another buss-count constant, and a global one at that, being
 *  used?  And I thought there was only one input buss anyway!  Well,
 *  there is only one ALSA input buss, but more can be used with JACK,
 *  apparently.
 *
 *  There's currently no implementation-specific API function here.
 *
 * \threadsafe
 *
 * \param bus
 *      Provides the buss number.
 *
 * \param inputing
 *      True if the input bus will be inputting MIDI data.
 *
 * \return
 *      Returns true if the input buss array item could be set and then saved
 *      into the status container.
 */

bool
mastermidibase::set_input (bussbyte bus, bool inputing)
{
    automutex locker(m_mutex);
    bool result = m_inbus_array.set_input(bus, inputing);
    if (result)
        result = save_input(bus, inputing);     /* save into the vector */

    return result;
}

/**
 *  Saves the input status (as selected in the MIDI Input tab).  Now, we were
 *  checking this bus number against the size of the vector as gotten from the
 *  perform object, which it got the from the "rc" file's [midi-input]
 *  section.  However, the "rc" file won't necessarily match what is on the
 *  system now.  So we might have to adjust.
 *
 *  Do we also have to adjust the perform's vector?
 *
 * \param bus
 *      Provides the buss number.
 *
 * \param inputing
 *      True if the input bus will be inputting MIDI data.
 *
 * \return
 *      Returns true, always.
 */

bool
mastermidibase::save_input (bussbyte bus, bool inputing)
{
    int currentcount = int(m_master_inputs.size());
    if (bus < currentcount)
        m_master_inputs[bus] = inputing;
    else
    {
        for (int i = currentcount; i <= bus; ++i)
        {
            bool value = false;
            if (i == int(bus))
                value = inputing;

            m_master_inputs.push_back(value);
        }
    }
    return true;
}

/**
 *  Get the input for the given (legal) buss number.
 *
 *  There's currently no implementation-specific API function here.
 *
 * \param bus
 *      Provides the buss number.
 *
 * \return
 *      Returns the value of the busarray::get_input(bus) call.
 */

bool
mastermidibase::get_input (bussbyte bus)
{
    return m_inbus_array.get_input(bus);
}

/**
 *  Get the system-buss status for the given (legal) buss number.
 *
 * \param bus
 *      Provides the buss number.
 *
 * \return
 *      Returns the value of the busarray::get_input(bus) call.
 */

bool
mastermidibase::is_input_system_port (bussbyte bus)
{
    return m_inbus_array.is_system_port(bus);
}

/**
 *  Get the MIDI output buss name for the given (legal) buss number.
 *  This function is used for display purposes, and is also written to the
 *  options ("rc") file.
 *
 *  This function adds the retrieval of client and port numbers that are not
 *  needed in the portmidi implementation, but seem generally useful to
 *  support in all implementations.
 *
 *  Also, if the client name is already part of the port name, as in
 *  "client:client port 0", then we
 *  remove the "client:" portion to make the listing look cleaner.
 *
 * \param bus
 *      Provides the output buss number.  Checked before usage.
 *      Actually should now be an index number
 *
 * \return
 *      Returns the buss name as a standard C++ string.  Also contains an
 *      indication that the buss is disconnected or unconnected.  If the buss
 *      number is illegal, this string is empty.
 */

std::string
mastermidibase::get_midi_out_bus_name (bussbyte bus)
{
    return m_outbus_array.get_midi_bus_name(bus);
}

/**
 *  Get the MIDI input buss name for the given (legal) buss number.
 *
 *  This function adds the retrieval of client and port numbers that are not
 *  needed in the portmidi implementation, but seem generally useful to
 *  support in all implementations.
 *
 * \param bus
 *      Provides the input buss number.
 *
 * \return
 *      Returns the buss name as a standard C++ string.  Also contains an
 *      indication that the buss is disconnected or unconnected.
 */

std::string
mastermidibase::get_midi_in_bus_name (bussbyte bus)
{
    return m_inbus_array.get_midi_bus_name(bus);
}

/**
 *  Print some information about the available MIDI input and output busses.
 */

void
mastermidibase::print () const
{
    m_inbus_array.print();
    m_outbus_array.print();
}

/**
 *  Initiate a poll() on the existing poll descriptors.
 *  This base-class implementation could be made identical to
 *  portmidi's poll_for_midi() function, maybe.  But currently it is better
 *  just call the implementation-specific API function.
 *
 * \warning
 *      Do we need to use a mutex lock? No! It causes a deadlock!!!
 *
 * \return
 *      Returns the result of the poll, or 0 if the API is not supported.
 */

int
mastermidibase::poll_for_midi ()
{
    return api_poll_for_midi();
}

/**
 *  Provides a default implementation of api_poll_for_midi().  This
 *  implementation adds a millisecond of sleep time unless more than two
 *  events are pending.  Some input devices may need to override this
 *  function.
 *
 *  For a quick threadsafe check, call is_more_input() instead.  But see the
 *  warning in the non-API poll_for_midi() function.
 *
 * \return
 *      Returns the number of events found by the first successful poll of the
 *      array of input busses (m_inbus_array).
 */

int
mastermidibase::api_poll_for_midi ()
{
    int result = m_inbus_array.poll_for_midi();
    if (result > 0)
    {
        if (result <= 2)
            millisleep(1);              /* is this sensible?    */
    }
    else
    {
        millisleep(1);
    }
    return result;
}

/**
 *  Test the sequencer to see if any more input is pending.  Calls the
 *  implementation-specific API function.
 *
 *  Note that the ALSA implementation calls a single "input-pending" function,
 *  while the PortMidi implementation loops through all of the input midibus
 *  objects, calling the poll_for_midi() function of each.
 *
 * \threadsafe
 *
 * \return
 *      Returns true if ALSA is supported, and the returned size is greater
 *      than 0, or false otherwise.
 */

bool
mastermidibase::is_more_input ()
{
    automutex locker(m_mutex);
    return m_inbus_array.poll_for_midi() > 0;
}

/**
 *  Start the given MIDI port.  This function is called by
 *  api_get_midi_event() when the ALSA event SND_SEQ_EVENT_PORT_START is
 *  received.  Unlike port_exit(), the port_start() function does rely on
 *  API-specific code, so we do need to create a virtual api_port_start()
 *  function to implement the port-start event.
 *
 *  \threadsafe
 *      Quite a lot is done during the lock for the ALSA implimentation.
 *
 * \param client
 *      Provides the client number, which is actually an ALSA concept.
 *
 * \param port
 *      Provides the client port, which is actually an ALSA concept.
 */

void
mastermidibase::port_start (int client, int port)
{
    automutex locker(m_mutex);
    api_port_start(client, port);
}

/**
 *  Turn off the given port for the given client.  Both the input and output
 *  busses for the given client are stopped: that is, set to inactive.
 *
 *  This function is called by api_get_midi_event() when the ALSA event
 *  SND_SEQ_EVENT_PORT_EXIT is received.  Since port_exit() has no direct
 *  API-specific code in it, we do not need to create a virtual
 *  api_port_exit() function to implement the port-exit event.
 *
 * \threadsafe
 *
 * \param client
 *      The client to be matched and acted on.  This value is actually an ALSA
 *      concept.
 *
 * \param port
 *      The port to be acted on.  Both parameter must be matched before the
 *      buss is made inactive.  This value is actually an ALSA concept.
 */

void
mastermidibase::port_exit (int client, int port)
{
    automutex locker(m_mutex);
    m_outbus_array.port_exit(client, port);
    m_inbus_array.port_exit(client, port);
}

/**
 *  Grab a MIDI event via the currently-selected MIDI API.
 *
 * \param ev
 *      The event to be set based on the found input event.
 */

bool
mastermidibase::get_midi_event (event * ev)
{
    return api_get_midi_event(ev);
}

/**
 *  Set the input sequence object, and set the m_dumping_input value to
 *  the given state.
 *
 *  The portmidi version only sets m_seq and m_dumping_input, but it seems
 *  like all the code below would apply to any mastermidibus.
 *
 * \threadsafe
 *
 * \param state
 *      Provides the dumping-input (recording) state to be set.  This value,
 *      as used in seqedit, can represent the state of the thru button or the
 *      record button.
 *
 * \param seq
 *      Provides the sequence object to be logged as the mastermidibase's
 *      sequence.  Can also be used to set a null pointer, to disable the
 *      sequence setting.
 */

void
mastermidibase::set_sequence_input (bool state, sequence * seq)
{
    automutex locker(m_mutex);
    if (m_filter_by_channel)
    {
        if (not_nullptr(seq))
        {
            if (state)
            {
                /*
                 * Add the sequence if not already added.
                 */

                bool have_seq_already = false;
                for (size_t i = 0; i < m_vector_sequence.size(); ++i)
                {
                    if (m_vector_sequence[i] == seq)
                    {
                        have_seq_already = true;
                        break;
                    }
                }
                if (! have_seq_already)
                    m_vector_sequence.push_back(seq);
            }
            else
            {
                /*
                 * If the sequence is already in the vector, remove it.
                 */

                for (size_t i = 0; i < m_vector_sequence.size(); ++i)
                {
                    if (m_vector_sequence[i] == seq)
                    {
                        m_vector_sequence.erase(m_vector_sequence.begin() + i);
                        break;
                    }
                }
            }
            if (m_vector_sequence.size() != 0)
                m_dumping_input = true;
        }
        else if (! state)
        {
            /*
             * No sequence and false state means we don't want to record, so
             * clear the vector.
             */

            m_vector_sequence.clear();
        }
    }
    else
    {
        m_seq = seq;
        m_dumping_input = state;
    }
}

/**
 *  This function augments the recording functionality by looking for a
 *  sequence that has a matching channel number, logging the event to that
 *  sequence, and then immediately exiting.  It should be called only if
 *  m_filter_by_channel is set.
 *
 *  If we have more than one sequence recording, and the channel-match feature
 *  [the sequence::channels_match() function] is disabled, then only the first
 *  sequence will get the events.  So now we add an addition call to the new
 *  sequence::channel_match() function.
 *
 * \param ev
 *      The event that was recorded, passed as a copy.  (Do we really need a
 *      copy?)
 */

void
mastermidibase::dump_midi_input (event ev)
{
    size_t sz = m_vector_sequence.size();
    if (sz > 0)
    {
        for (size_t i = 0; i < sz; ++i)
        {
            if (is_nullptr(m_vector_sequence[i]))          // error check
            {
                errprint("dump_midi_input(): bad sequence");
                continue;
            }
            else if (m_vector_sequence[i]->stream_event(ev))
            {
                /*
                 * Did we find a match to the sequence channel?  Then don't
                 * bother with the remaining sequences.  Otherwise, pass the
                 * event to any other recording sequences.
                 */

                if (m_vector_sequence[i]->channel_match())
                    break;
            }
        }
    }
    else
    {
        errprint("dump_midi_input(): no sequences");
    }
}

}           // namespace seq64

/*
 * mastermidibase.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

