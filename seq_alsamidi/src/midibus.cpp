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
 * \updates       2016-11-21
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

#ifdef SEQ64_HAVE_LIBASOUND

/**
 *  Initialize this static member.
 */

int midibus::m_clock_mod = 16 * 4;

/**
 *  Provides a constructor with client number, port number, ALSA sequencer
 *  support, name of client, name of port.
 *
 *  This constructor is the one that seems to be the one that is used for
 *  the MIDI input and output busses, when the [manual-alsa-ports] option is
 *  not in force.  Also used for the announce buss, and in the
 *  mastermidibus::port_start() function.
 *
 * \param localclient
 *      Provides the local-client number.
 *
 * \param destclient
 *      Provides the destination-client number.
 *
 * \param destport
 *      Provides the destination-client port.
 *
 * \param seq
 *      Provides the ALSA sequence that will work with this buss.
 *
 * \param client_name
 *      Provides the client name, but this parameter is unused.
 *
 * \param port_name
 *      Provides the port name.
 *
 * \param id
 *      Provides the ID code for this bus.  It is an index into the midibus
 *      definitions array, and is also used in the constructed human-readable
 *      buss name.
 *
 * \param queue
 *      Provides the queue ID.
 *
 * \param ppqn
 *      Provides the PPQN value.
 */

midibus::midibus
(
    int localclient,
    int destclient,
    int destport,
    snd_seq_t * seq,
    const char * /* client_name */ ,      // unused parameter
    const char * port_name,
    int id,
    int queue,
    int ppqn
) :
    m_id                (id),
    m_clock_type        (e_clock_off),
    m_inputing          (false),
    m_ppqn              (0),
    m_seq               (seq),
    m_dest_addr_client  (destclient),
    m_dest_addr_port    (destport),
    m_local_addr_client (localclient),
    m_local_addr_port   (-1),
    m_queue             (queue),
    m_name              (),
    m_lasttick          (0),
    m_mutex             ()
{
    char alias[64];
    const std::string & bussname = usr().bus_name(m_id);
    if (! bussname.empty())
        snprintf(alias, sizeof alias, "%s", bussname.c_str());
    else
        snprintf(alias, sizeof alias, "%s", port_name);

    char name[80];
    snprintf                            /* copy the client name parts */
    (
        name, sizeof name, "[%d] %d:%d %s",
        m_id, m_dest_addr_client, m_dest_addr_port, alias
    );
    m_name = name;
    m_ppqn = choose_ppqn(ppqn);
}

/**
 *  Secondary constructor.  Similar to the principal constructor, but
 *  labels the buss by number more than by name.
 *
 *  This constructor is the one that seems to be the one that is used for
 *  the MIDI input and output busses when the [manual-alsa-ports] option is in
 *  effect.
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
 *      buss name.
 *
 * \param queue
 *      Provides the queue ID.
 *
 * \param ppqn
 *      Provides the PPQN value.
 */

midibus::midibus
(
    int localclient,
    snd_seq_t * seq,
    int id,
    int queue,
    int ppqn
) :
    m_id                (id),
    m_clock_type        (e_clock_off),
    m_inputing          (false),
    m_ppqn              (0),
    m_seq               (seq),
    m_dest_addr_client  (-1),
    m_dest_addr_port    (-1),
    m_local_addr_client (localclient),
    m_local_addr_port   (-1),
    m_queue             (queue),
    m_name              (),
    m_lasttick          (0),
    m_mutex             ()
{
    /*
     * Copy the client name.  It used to be "seq24", but this is now a new
     * application.
     *
     * \todo
     *      We need to make a single macro for this name at some point.
     */

    char name[64];
    snprintf(name, sizeof name, "[%d] sequencer64 %d", m_id, m_id);
    m_name = name;
    m_ppqn = choose_ppqn(ppqn);
}

#endif   // SEQ64_HAVE_LIBASOUND

/**
 *  A rote empty destructor.
 */

midibus::~midibus()
{
    // empty body
}

/**
 *  Initialize the MIDI output port.
 *
 * \return
 *      Returns true unless setting up ALSA MIDI failed in some way.
 */

bool
midibus::init_out ()
{
#ifdef SEQ64_HAVE_LIBASOUND
    int result = snd_seq_create_simple_port         /* create ports     */
    (
        m_seq, m_name.c_str(),
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
            stderr,
            "snd_seq_connect_to(%d:%d) error\n",
            m_dest_addr_client, m_dest_addr_port
        );
        return false;
    }
#endif  // SEQ64_HAVE_LIBASOUND
    return true;
}

/**
 *  Initialize the MIDI input port.
 *
 * \return
 *      Returns true unless setting up ALSA MIDI failed in some way.
 */

bool
midibus::init_in ()
{
#ifdef SEQ64_HAVE_LIBASOUND
    int result = snd_seq_create_simple_port             /* create ports */
    (
        m_seq, "sequencer64 in",                        /* "seq24 in"   */
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

    snd_seq_port_subscribe_set_queue(subs, m_queue);
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
#endif  // SEQ64_HAVE_LIBASOUND
    return true;
}

/**
 *  Initialize the output in a different way?
 *
 * \return
 *      Returns true unless setting up the ALSA port failed in some way.
 */

bool
midibus::init_out_sub ()
{
#ifdef SEQ64_HAVE_LIBASOUND
    int result = snd_seq_create_simple_port             /* create ports */
    (
        m_seq, m_name.c_str(),
        SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ,
        SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_APPLICATION
    );
    m_local_addr_port = result;
    if (result < 0)
    {
        errprint("snd_seq_create_simple_port(write) error");
        return false;
    }
#endif
    return true;
}

/**
 *  Initialize the output in a different way?
 *
 * \return
 *      Returns true unless setting up the ALSA port failed in some way.
 */

bool
midibus::init_in_sub ()
{
#ifdef SEQ64_HAVE_LIBASOUND
    int result = snd_seq_create_simple_port             /* create ports */
    (
        m_seq, "sequencer64 in",                        /* "seq24 in"   */
        SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE,
        SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_APPLICATION
    );
    m_local_addr_port = result;
    if (result < 0)
    {
        errprint("snd_seq_create_simple_port(write) error");
        return false;
    }
#endif
    return true;
}

/**
 *  Deinitialize the MIDI input.  Set the input and the output ports.
 *  The destination port is actually our local port.
 *
 * \return
 *      Returns true, unless an error occurs.
 */

bool
midibus::deinit_in  ()
{
#ifdef SEQ64_HAVE_LIBASOUND
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

    snd_seq_port_subscribe_set_queue(subs, m_queue);        /* master queue */
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
#endif  // SEQ64_HAVE_LIBASOUND
    return true;
}

/**
 *  Prints m_name.
 */

void
midibus::print ()
{
    printf("%s" , m_name.c_str());
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
midibus::play (event * e24, midibyte channel)
{
#ifdef SEQ64_HAVE_LIBASOUND
    automutex locker(m_mutex);
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
#endif  // SEQ64_HAVE_LIBASOUND
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
midibus::sysex (event * e24)
{
#ifdef SEQ64_HAVE_LIBASOUND
    automutex locker(m_mutex);
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
#endif  // SEQ64_HAVE_LIBASOUND
}

/**
 *  Flushes our local queue events out into ALSA.
 */

void
midibus::flush ()
{
#ifdef SEQ64_HAVE_LIBASOUND
    automutex locker(m_mutex);
    snd_seq_drain_output(m_seq);
#endif
}

/**
 *  Initialize the clock, continuing from the given tick.
 *
 * \param tick
 *      The starting tick.
 */

void
midibus::init_clock (midipulse tick)
{
#ifdef SEQ64_HAVE_LIBASOUND
    if (m_clock_type == e_clock_pos && tick != 0)
    {
        continue_from(tick);
    }
    else if (m_clock_type == e_clock_mod || tick == 0)
    {
        start();

        /*
         * The next equation is effectively (m_ppqn / 4) * 16 * 4,
         * or m_ppqn * 16.  Note that later we have pp16th = (m_ppqn / 4).
         */

        midipulse clock_mod_ticks = (m_ppqn / 4) * m_clock_mod;
        midipulse leftover = (tick % clock_mod_ticks);
        midipulse starting_tick = tick - leftover;

        /*
         * Was there anything left? Then wait for next beat (16th note)
         * to start clocking.
         */

        if (leftover > 0)
            starting_tick += clock_mod_ticks;

        m_lasttick = starting_tick - 1;
    }
#endif  // SEQ64_HAVE_LIBASOUND
}

/**
 *  Continue from the given tick.
 *
 * \param tick
 *      The continuing tick.
 */

void
midibus::continue_from (midipulse tick)
{
#ifdef SEQ64_HAVE_LIBASOUND

    /*
     * Tell the device that we are going to start at a certain position.
     */

    midipulse pp16th = m_ppqn / 4;
    midipulse leftover = tick % pp16th;
    long beats = tick / pp16th;
    midipulse starting_tick = tick - leftover;

    /*
     * Was there anything left? Then wait for next beat (16th note) to
     * start clocking.
     */

    if (leftover > 0)
        starting_tick += pp16th;

    m_lasttick = starting_tick - 1;
    if (m_clock_type != e_clock_off)
    {
        snd_seq_event_t ev;
        ev.type = SND_SEQ_EVENT_CONTINUE;

        snd_seq_event_t evc;
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
        flush();
        snd_seq_event_output(m_seq, &ev);
    }
#endif  // SEQ64_HAVE_LIBASOUND
}

/**
 *  This function gets the MIDI clock a-runnin', if the clock type is not
 *  e_clock_off.
 */

void
midibus::start ()
{
#ifdef SEQ64_HAVE_LIBASOUND
    m_lasttick = -1;
    if (m_clock_type != e_clock_off)
    {
        snd_seq_event_t ev;
        ev.type = SND_SEQ_EVENT_START;
        snd_seq_ev_set_fixed(&ev);
        snd_seq_ev_set_priority(&ev, 1);
        snd_seq_ev_set_source(&ev, m_local_addr_port);  /* set the source   */
        snd_seq_ev_set_subs(&ev);
        snd_seq_ev_set_direct(&ev);                     /* it's immediate   */
        snd_seq_event_output(m_seq, &ev);               /* pump into queue  */
    }
#endif  // SEQ64_HAVE_LIBASOUND
}

/**
 *  Set status to of "inputting" to the given value.  If the parameter is
 *  true, then init_in() is called; otherwise, deinit_in() is called.
 *
 * \param inputing
 *      The inputing value to set.
 */

void
midibus::set_input (bool inputing)
{
    if (m_inputing != inputing)
    {
        m_inputing = inputing;
        if (inputing)
            init_in();
        else
            deinit_in();
    }
}

/**
 *  Stop the MIDI buss.
 */

void
midibus::stop ()
{
#ifdef SEQ64_HAVE_LIBASOUND
    m_lasttick = -1;
    if (m_clock_type != e_clock_off)
    {
        snd_seq_event_t ev;
        ev.type = SND_SEQ_EVENT_STOP;
        snd_seq_ev_set_fixed(&ev);
        snd_seq_ev_set_priority(&ev, 1);
        snd_seq_ev_set_source(&ev, m_local_addr_port);  /* set the source   */
        snd_seq_ev_set_subs(&ev);
        snd_seq_ev_set_direct(&ev);                     /* it's immediate   */
        snd_seq_event_output(m_seq, &ev);               /* pump into queue  */
    }
#endif  // SEQ64_HAVE_LIBASOUND
}

/**
 *  Generates the MIDI clock, starting at the given tick value.
 *
 * \threadsafe
 *
 * \param tick
 *      Provides the starting tick.
 */

void
midibus::clock (midipulse tick)
{
#ifdef SEQ64_HAVE_LIBASOUND
    automutex locker(m_mutex);
    if (m_clock_type != e_clock_off)
    {
        bool done = m_lasttick >= tick;
        int ct = clock_ticks_from_ppqn(m_ppqn);         /* ppqn / 24    */
        while (! done)
        {
            ++m_lasttick;
            done = m_lasttick >= tick;
            if ((m_lasttick % ct) == 0)                 /* tick time?           */
            {
                /*
                 * Set the event tag to 127 so the sequences won't remove it.
                 */

                snd_seq_event_t ev;
                ev.type = SND_SEQ_EVENT_CLOCK;
                ev.tag = 127;
                snd_seq_ev_set_fixed(&ev);
                snd_seq_ev_set_priority(&ev, 1);
                snd_seq_ev_set_source(&ev, m_local_addr_port); /* set source    */
                snd_seq_ev_set_subs(&ev);
                snd_seq_ev_set_direct(&ev);             /* it's immediate       */
                snd_seq_event_output(m_seq, &ev);       /* pump it into queue   */
            }
        }
        flush();            /* and send out */
    }
#endif  // SEQ64_HAVE_LIBASOUND
}

#if REMOVE_QUEUED_ON_EVENTS_CODE

/**
 *  Deletes events in the queue.  This function is not used anywhere, and
 *  there was no comment about the intent/context of this function.
 */

void
midibus::remove_queued_on_events (int tag)
{
    automutex locker(m_mutex);
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

