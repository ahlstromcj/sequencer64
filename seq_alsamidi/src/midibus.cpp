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
 *  This module declares/defines the base class for handling MIDI I/O via
 *  the ALSA system.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2017-08-26
 * \license       GNU GPLv2 or above
 *
 *  This file provides a Linux-only implementation of MIDI support.
 */

#include "globals.h"
#include "calculations.hpp"             /* clock_ticks_from_ppqn()          */
#include "event.hpp"                    /* seq64::event (MIDI event)        */
#include "midibus.hpp"                  /* seq64::midibus for ALSA          */
#include "settings.hpp"                 /* seq64::rc() and choose_ppqn()    */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *  Creates a normal ALSA MIDI port, which will correspond to an existing
 *  system ALSA port, such as one provided by Timidity.  Provides a
 *  constructor with client number, port number, ALSA sequencer support, name
 *  of client, name of port.
 *
 *  This constructor is the one that is used for the MIDI input and output
 *  busses, when the [manual-alsa-ports] option is <i> not </i> in force.  It
 *  is also used for the announce buss, and in the mastermidibus::port_start()
 *  function.
 *
 *  There's currently some overlap between local/dest client and port numbers
 *  and the buss and port numbers of the new midibase interface.
 *
 *  Also, note that the optionsfile module uses the master buss to get the
 *  buss names when it writes the file.
 *
 * \param localclient
 *      Provides the local-client number.  Also known as the user-client
 *      number.  The ALSA snd_seq_client_id() function assigns user-client
 *      numbers ranging from 128 to 191.
 *
 * \param destclient
 *      Provides the destination-client number.  This is the actual buss
 *      number (e.g. 0, 1, 14, 128, 131, etc.)
 *
 * \param destport
 *      Provides the destination-client port.
 *
 * \param seq
 *      Provides the ALSA sequence that will work with this buss.
 *
 * \param client_name
 *      Provides the client name; but this parameter was unused, but now
 *      enabled for better port naming in the Options dialog.  This is actual
 *      the buss though "buss" is also kind of misleading.
 *
 * \param port_name
 *      Provides the port name.
 *
 * \param index
 *      This is the order of the buss in the lookup, used for display and
 *      labelling.  Starts from 0.
 *
 * \param queue
 *      Provides the queue ID.
 *
 * \param ppqn
 *      Provides the PPQN value.
 *
 * \param bpm
 *      Provides the BPM value.
 */

midibus::midibus
(
    int localclient,
    int destclient,                     // is this the major ifx number?
    int destport,
    snd_seq_t * seq,
    const std::string & clientname,     // the ALSA "client" (buss) name
    const std::string & portname,
    int index,                          // just an ordinal for display
    int queue,
    int ppqn,
    midibpm bpm
) :
    midibase
    (
        rc().application_name(), clientname, portname, index,
        destclient,                     // bus_id
        destport,                       // SEQ64_NO_PORT
        queue, ppqn, bpm
    ),
    m_seq               (seq),
    m_dest_addr_client  (destclient),   // actually the buss ID
    m_dest_addr_port    (destport),     // actually the port ID
    m_local_addr_client (localclient),
    m_local_addr_port   (-1),
    m_input_port_name   (rc().app_client_name() + " in")
{
    // Functionality moved into the base class
}

/**
 *  Virtual port constructor, a secondary constructor.  This constructor is
 *  used for the MIDI input and output busses when the [manual-alsa-ports]
 *  option is in effect.  In effect, it is meant to create a virtual port.
 *  This is indicated by passing "true" as the final parameter of the
 *  base-class constructor.  It is similar to the principal constructor, but
 *  labels the buss by number more than by name.
 *
 * \param localclient
 *      Provides the local-client number.
 *
 * \param seq
 *      Provides the sequence that will work with this buss.
 *
 * \param id
 *      Provides the ID code for this bus.  It is an index into the midibus
 *      definitions array, and is also used in the constructed human-readable
 *      buss name.  This is also the port ID; currently the buss-number and
 *      port-number implement the same concept.
 *
 * \param queue
 *      Provides the queue ID.
 *
 * \param ppqn
 *      Provides the PPQN value.
 *
 * \param bpm
 *      Provides the BPM value.
 */

midibus::midibus
(
    int localclient,
    snd_seq_t * seq,
    int index,                          // just an ordinal for display
    int bus_id,                         // might just remove this parameter!!!
    int queue,
    int ppqn,
    midibpm bpm
) :
    midibase
    (
        rc().application_name(), "", "", index, bus_id, index+1, /* port ID */
        queue, ppqn, bpm, true /* virtual */
    ),
    m_seq               (seq),
    m_dest_addr_client  (SEQ64_NO_BUS),
    m_dest_addr_port    (SEQ64_NO_PORT),
    m_local_addr_client (localclient),
    m_local_addr_port   (SEQ64_NO_PORT),
    m_input_port_name   (rc().app_client_name() + " in")
{
    // Functionality moved to the base class
}

/**
 *  A rote empty destructor.
 */

midibus::~midibus()
{
    // empty body
}

/**
 *  Initialize the MIDI output port.  This initialization is done when the
 *  "manual ALSA ports" option is not in force.
 *
 *  This initialization is like the "open_virtual_port()" function of the
 *  RtMidi library, with the addition of the snd_seq_connect_to() call involving
 *  the local and destination ports.
 *
 * \return
 *      Returns true unless setting up ALSA MIDI failed in some way.
 */

bool
midibus::api_init_out ()
{
    int result = snd_seq_create_simple_port         /* create ports     */
    (
        m_seq, bus_name().c_str(),
        SND_SEQ_PORT_CAP_NO_EXPORT | SND_SEQ_PORT_CAP_READ,
        SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_APPLICATION
    );
    m_local_addr_port = result;
    if (result < 0)
    {
        errprint("snd_seq_create_simple_port(write) error");
        return false;
    }
    result = snd_seq_connect_to                     /* connect to port  */
    (
        m_seq, m_local_addr_port, m_dest_addr_client, m_dest_addr_port
    );
    if (result < 0)
    {
        fprintf
        (
            stderr, "snd_seq_connect_to(%d:%d) error\n",
            m_dest_addr_client, m_dest_addr_port
        );
        return false;
    }
    return true;
}

/**
 *  Initialize the MIDI input port.
 *
 * \return
 *      Returns true unless setting up ALSA MIDI failed in some way.
 */

bool
midibus::api_init_in ()
{
    int result = snd_seq_create_simple_port             /* create ports */
    (
        m_seq,
        m_input_port_name.c_str(),
        SND_SEQ_PORT_CAP_NO_EXPORT | SND_SEQ_PORT_CAP_WRITE,
        SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_APPLICATION
    );
    m_local_addr_port = result;
    if (result < 0)
    {
        errprint("snd_seq_create_simple_port(read) error");
        return false;
    }

    snd_seq_port_subscribe_t * subs;
    snd_seq_port_subscribe_alloca(&subs);

    snd_seq_addr_t sender;
    sender.client = m_dest_addr_client;
    sender.port = m_dest_addr_port;
    snd_seq_port_subscribe_set_sender(subs, &sender);   /* destination        */

    snd_seq_addr_t dest;
    dest.client = m_local_addr_client;
    dest.port = m_local_addr_port;
    snd_seq_port_subscribe_set_dest(subs, &dest);       /* local              */

    /*
     * Use the master queue, and get ticks, then subscribe.
     */

    snd_seq_port_subscribe_set_queue(subs, queue_number());
    snd_seq_port_subscribe_set_time_update(subs, 1);
    result = snd_seq_subscribe_port(m_seq, subs);
    if (result < 0)
    {
        fprintf
        (
            stderr, "snd_seq_connect_from(%d:%d) error\n",
            m_dest_addr_client, m_dest_addr_port
        );
        return false;
    }
    return true;
}

/**
 *  Gets information directly from ALSA.  The problem this function solves is
 *  that the midibus constructor for a virtual ALSA port doesn't not have all
 *  of the information it needs at that point.  Here, we can get this
 *  information and get the actual data we need to rename the port to
 *  something accurate.
 *
 * \return
 *      Returns true if all of the information could be obtained.  If false is
 *      returned, then the caller should not use the side-effects.
 *
 * \sideeffect
 *      Passes back the values found.
 */

bool
midibus::set_virtual_name (int portid, const std::string & portname)
{
    bool result = not_nullptr(m_seq);
    if (result)
    {
        snd_seq_client_info_t * cinfo;                  /* client info      */
        snd_seq_client_info_alloca(&cinfo);             /* will fill cinfo  */
        snd_seq_get_client_info(m_seq, cinfo);          /* filled!          */

        int cid = snd_seq_client_info_get_client(cinfo);
        const char * cname = snd_seq_client_info_get_name(cinfo);
        result = not_nullptr(cname);
        if (result)
        {
            std::string clientname = cname;
            std::string pname = portname;
            set_port_id(portid);
            pname += " ";
            pname += std::to_string(portid);
            set_bus_id(cid);
            set_name(rc().application_name(), clientname, pname);
        }
    }
    return result;
}

/**
 *  Initialize the output in a different way.  This version of initialization is
 *  used by mastermidibus in the "manual ALSA ports" clause.  The "sub" in the
 *  function name is short for "subscription".
 *
 * \return
 *      Returns true unless setting up the ALSA port failed in some way.
 */

bool
midibus::api_init_out_sub ()
{
    std::string portname = port_name();
    if (portname.empty())
        portname = rc().app_client_name() + " out";

    int result = snd_seq_create_simple_port             /* create ports */
    (
        m_seq, portname.c_str(),
        SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ,
        SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_APPLICATION
    );
    m_local_addr_port = result;
    if (result < 0)
    {
        errprint("snd_seq_create_simple_port(write) error");
        return false;
    }
    else
        set_virtual_name(result, portname);

    return true;
}

/**
 *  Initialize the output in a different way?
 *
 * \return
 *      Returns true unless setting up the ALSA port failed in some way.
 */

bool
midibus::api_init_in_sub ()
{
    std::string portname = port_name();
    if (portname.empty())
        portname = m_input_port_name;

    int result = snd_seq_create_simple_port             /* create ports */
    (
        m_seq,
        portname.c_str(),
        SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE,
        SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_APPLICATION
    );
    m_local_addr_port = result;
    if (result < 0)
    {
        errprint("snd_seq_create_simple_port(write) error");
        return false;
    }
    else
        set_virtual_name(result, portname);

    return true;
}

/**
 *  Deinitialize the MIDI input.  Set the input and the output ports.
 *  The destination port is actually our local port.  This function is called
 *  in midibase::set_input() when input on the port is turned off.
 *
 * \return
 *      Returns true, unless an error occurs.
 */

bool
midibus::api_deinit_in ()
{
    snd_seq_port_subscribe_t * subs;
    snd_seq_port_subscribe_alloca(&subs);

    snd_seq_addr_t sender;                                  /* output       */
    sender.client = m_dest_addr_client;
    sender.port = m_dest_addr_port;
    snd_seq_port_subscribe_set_sender(subs, &sender);

    snd_seq_addr_t dest;                                    /* input        */
    dest.client = m_local_addr_client;
    dest.port = m_local_addr_port;
    snd_seq_port_subscribe_set_dest(subs, &dest);

    snd_seq_port_subscribe_set_queue(subs, queue_number()); /* master queue */
    snd_seq_port_subscribe_set_time_update(subs, 1);        /* get ticks    */

    int result = snd_seq_unsubscribe_port(m_seq, subs);     /* subscribe    */
    if (result < 0)
    {
        fprintf
        (
            stderr, "snd_seq_unsubscribe_port(%d:%d) error\n",
            m_dest_addr_client, m_dest_addr_port
        );
        return false;
    }
    return true;
}

/**
 *  Defines the size of the MIDI event buffer, which should be large enough to
 *  accomodate the largest MIDI message to be encoded.
 *  A local define for visibility.
 */

#define SEQ64_MIDI_EVENT_SIZE_MAX   10

/**
 *  This play() function takes a native event, encodes it to an ALSA MIDI
 *  sequencer event, sets the broadcasting to the subscribers, sets the
 *  direct-passing mode to send the event without queueing, and puts it in the
 *  queue.
 *
 * \threadsafe
 *
 * \param e24
 *      The event to be played on this bus.  For speed, we don't bother to
 *      check the pointer.
 *
 * \param channel
 *      The channel of the playback.
 */

void
midibus::api_play (event * e24, midibyte channel)
{
    midibyte buffer[4];                             /* temp for MIDI data   */
    buffer[0] = e24->get_status();                  /* fill buffer          */
    buffer[0] += (channel & 0x0F);
    e24->get_data(buffer[1], buffer[2]);            /* set MIDI data        */

    snd_midi_event_t * midi_ev;                     /* ALSA MIDI parser     */
    snd_midi_event_new(SEQ64_MIDI_EVENT_SIZE_MAX, &midi_ev);

    snd_seq_event_t ev;
    snd_seq_ev_clear(&ev);                          /* clear event          */
    snd_midi_event_encode(midi_ev, buffer, 3, &ev); /* encode 3 raw bytes   */
    snd_midi_event_free(midi_ev);                   /* free the parser      */
    snd_seq_ev_set_source(&ev, m_local_addr_port);  /* set source           */
    snd_seq_ev_set_subs(&ev);
    snd_seq_ev_set_direct(&ev);                     /* it is immediate      */
    snd_seq_event_output(m_seq, &ev);               /* pump into the queue  */
}

/**
 *  min() for long values.
 *
 * \param a
 *      First operand.
 *
 * \param b
 *      Second operand.
 *
 * \return
 *      Returns the minimum value of a and b.
 */

inline long
min (long a, long b)
{
    return (a < b) ? a : b ;
}

/**
 *  Defines the value used for sleeping, in microseconds.  Defined locally
 *  simply for visibility.  Why 80000?
 */

#define SEQ64_USLEEP_US     80000

/**
 *  Takes a native SYSEX event, encodes it to an ALSA event, and then
 *  puts it in the queue.
 *
 * \param e24
 *      The event to be handled.
 */

void
midibus::api_sysex (event * e24)
{
    snd_seq_event_t ev;
    snd_seq_ev_clear(&ev);                              /* clear event      */
    snd_seq_ev_set_priority(&ev, 1);
    snd_seq_ev_set_source(&ev, m_local_addr_port);      /* set source       */
    snd_seq_ev_set_subs(&ev);
    snd_seq_ev_set_direct(&ev);                         /* it's immediate   */

    /*
     *  Replaced by a vector of midibytes:
     *
     *      midibyte * data = e24->get_sysex();
     *
     *  This is a bit tricky, and relies on the standard property of
     *  std::vector where all n elements of the vector are guaranteed to be
     *  stored contiguously (in order to be accessible via random-access
     *  iterators).
     */

    event::SysexContainer & data = e24->get_sysex();
    int data_size = e24->get_sysex_size();
    for (int offset = 0; offset < data_size; offset += c_midibus_sysex_chunk)
    {
        int data_left = data_size - offset;
        snd_seq_ev_set_sysex
        (
            &ev, min(data_left, c_midibus_sysex_chunk), &data[offset]
        );
        snd_seq_event_output_direct(m_seq, &ev);        /* pump into queue  */
        usleep(SEQ64_USLEEP_US);
        flush();
    }
}

/**
 *  Flushes our local queue events out into ALSA.
 */

void
midibus::api_flush ()
{
    snd_seq_drain_output(m_seq);
}

/**
 *  Continue from the given tick.
 *
 * \param tick
 *      The continuing tick, unused in the ALSA implementation here.
 *      The midibase::continue_from() function uses it.
 *
 * \param beats
 *      The beats value calculated by midibase::continue_from().
 */

#ifdef USE_THIS_SEQ24_CODE

void
midibus::api_continue_from (midipulse tick, midipulse beats)
{

#else

void
midibus::api_continue_from (midipulse /*tick*/, midipulse beats)
{

#endif

#ifdef USE_THIS_SEQ24_CODE
    /*
     * Tell the device that we are going to start at a certain position.
     * Was there anything left over? Then wait for next beat (16th note) to start
     * clocking.
     */

    long pp16th = c_ppqn / 4;
    long beats = tick / pp16th;
    long leftover = tick % pp16th;
    long starting_tick = tick - leftover;
    if (leftover > 0)
        starting_tick += pp16th;

    m_lasttick = starting_tick - 1;
    if (clock_enabled())
    {
        ... covers the rest of the statements....
#endif	// USE_THIS_SEQ24_CODE

    snd_seq_event_t ev;
    snd_seq_ev_clear(&ev);                          /* clear event      */
    ev.type = SND_SEQ_EVENT_CONTINUE;

    snd_seq_event_t evc;
    snd_seq_ev_clear(&evc);                         /* clear event      */
    evc.type = SND_SEQ_EVENT_SONGPOS;
    evc.data.control.value = beats;
    snd_seq_ev_set_fixed(&ev);
    snd_seq_ev_set_fixed(&evc);
    snd_seq_ev_set_priority(&ev, 1);
    snd_seq_ev_set_priority(&evc, 1);
    snd_seq_ev_set_source(&evc, m_local_addr_port); /* set the source   */
    snd_seq_ev_set_subs(&evc);
    snd_seq_ev_set_source(&ev, m_local_addr_port);
    snd_seq_ev_set_subs(&ev);
    snd_seq_ev_set_direct(&ev);                     /* it's immediate   */
    snd_seq_ev_set_direct(&evc);
    snd_seq_event_output(m_seq, &evc);              /* pump into queue  */
    api_flush();
    snd_seq_event_output(m_seq, &ev);

#ifdef SEQ64_SHOW_API_CALLS
    if (tick > 0)
    {
        printf
        (
            "midibus::continue_from(%ld) local port %d\n", tick, m_local_addr_port
        );
    }
#endif
}

/**
 *  This function gets the MIDI clock a-runnin', if the clock type is not
 *  e_clock_off.
 */

void
midibus::api_start ()
{
    snd_seq_event_t ev;
    snd_seq_ev_clear(&ev);                          /* memsets it to 0  */
    ev.type = SND_SEQ_EVENT_START;
    snd_seq_ev_set_fixed(&ev);
    snd_seq_ev_set_priority(&ev, 1);
    snd_seq_ev_set_source(&ev, m_local_addr_port);  /* set the source   */
    snd_seq_ev_set_subs(&ev);
    snd_seq_ev_set_direct(&ev);                     /* it's immediate   */
    snd_seq_event_output(m_seq, &ev);               /* pump into queue  */
}

/**
 *  Stop the MIDI buss.
 */

void
midibus::api_stop ()
{
    snd_seq_event_t ev;
    snd_seq_ev_clear(&ev);                          /* memsets it to 0      */
    ev.type = SND_SEQ_EVENT_STOP;
    snd_seq_ev_set_fixed(&ev);
    snd_seq_ev_set_priority(&ev, 1);
    snd_seq_ev_set_source(&ev, m_local_addr_port);  /* set the source       */
    snd_seq_ev_set_subs(&ev);
    snd_seq_ev_set_direct(&ev);                     /* it's immediate       */
    snd_seq_event_output(m_seq, &ev);               /* pump into queue      */
}

/**
 *  Generates the MIDI clock, starting at the given tick value.  Note that we
 *  set the event tag to 127 so that Sequencer64 sequences/patterns won't
 *  remove it.
 *
 * \threadsafe
 *
 * \param tick
 *      Provides the starting tick, unused in the ALSA implementation.
 */

void
midibus::api_clock (midipulse tick)
{
    if (tick >= 0)
    {
#ifdef PLATFORM_DEBUG_TMI
        midibase::show_clock("midibus ALSA", tick);
#endif
    }

    snd_seq_event_t ev;
    snd_seq_ev_clear(&ev);                          /* clear event          */
    ev.type = SND_SEQ_EVENT_CLOCK;
    ev.tag = 127;
    snd_seq_ev_set_fixed(&ev);
    snd_seq_ev_set_priority(&ev, 1);
    snd_seq_ev_set_source(&ev, m_local_addr_port);  /* set source           */
    snd_seq_ev_set_subs(&ev);
    snd_seq_ev_set_direct(&ev);                     /* it's immediate       */
    snd_seq_event_output(m_seq, &ev);               /* pump it into queue   */
}

#if REMOVE_QUEUED_ON_EVENTS_CODE

/**
 *  Deletes events in the queue.  This function is not used anywhere, and
 *  there was no comment about the intent/context of this function.
 */

void
midibus::remove_queued_on_events (int tag)
{
    snd_seq_remove_events_t * remove_events;
    snd_seq_remove_events_malloc(&remove_events);
    snd_seq_remove_events_set_condition
    (
        remove_events, SND_SEQ_REMOVE_OUTPUT | SND_SEQ_REMOVE_TAG_MATCH |
            SND_SEQ_REMOVE_IGNORE_OFF
    );
    snd_seq_remove_events_set_tag(remove_events, tag);
    snd_seq_remove_events(m_seq, remove_events);
    snd_seq_remove_events_free(remove_events);
}

#endif      // REMOVE_QUEUED_ON_EVENTS_CODE

}           // namespace seq64

/*
 * midibus.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

