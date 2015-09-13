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
 * \updates       2015-09-13
 * \license       GNU GPLv2 or above
 *
 *  This file provides a Linux-only implementation of MIDI support.
 */

#include "midibus.hpp"

namespace seq64
{

#ifdef SEQ64_HAVE_LIBASOUND

#include <sys/poll.h>

/**
 *  Initialize this static member.
 */

int midibus::m_clock_mod = 16 * 4;

/**
 *  Provides a constructor with client number, port number, ALSA sequencer
 *  support, name of client, name of port.
 */

midibus::midibus
(
    int a_localclient,
    int a_destclient,
    int a_destport,
    snd_seq_t * a_seq,
    const char * /* a_client_name */ ,      // unused parameter
    const char * a_port_name,
    int a_id,
    int a_queue
) :
    m_id                (a_id),
    m_clock_type        (e_clock_off),
    m_inputing          (false),
    m_seq               (a_seq),
    m_dest_addr_client  (a_destclient),
    m_dest_addr_port    (a_destport),
    m_local_addr_client (a_localclient),
    m_local_addr_port   (-1),
    m_queue             (a_queue),
    m_name              (),
    m_lasttick          (0),
    m_mutex             ()
{
    char name[64];
    if (global_user_midi_bus_definitions[m_id].alias.length() > 0)
    {
        snprintf
        (
            name, sizeof(name), "(%s)",
            global_user_midi_bus_definitions[m_id].alias.c_str()
        );
    }
    else
    {
        snprintf(name, sizeof(name), "(%s)", a_port_name);
    }

    /* copy the client names */

    char tmp[64];
    snprintf
    (
        tmp, 59, "[%d] %d:%d %s",
        m_id, m_dest_addr_client, m_dest_addr_port, name
    );
    m_name = tmp;
}

/**
 *  Secondary constructor.
 */

midibus::midibus
(
    int a_localclient,
    snd_seq_t * a_seq,
    int a_id,
    int a_queue
) :
    m_id                (a_id),
    m_clock_type        (e_clock_off),
    m_inputing          (false),
    m_seq               (a_seq),
    m_dest_addr_client  (-1),
    m_dest_addr_port    (-1),
    m_local_addr_client (a_localclient),
    m_local_addr_port   (-1),
    m_queue             (a_queue),
    m_name              (),
    m_lasttick          (0),
    m_mutex             ()
{
    /* copy the client name */

    char tmp[64];
    snprintf(tmp, sizeof(tmp), "[%d] seq24 %d", m_id, m_id);
    m_name = tmp;
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
 */

bool midibus::init_out ()
{

#ifdef SEQ64_HAVE_LIBASOUND

    int result = snd_seq_create_simple_port         /* create ports */
    (
        m_seq,
        m_name.c_str(),
        SND_SEQ_PORT_CAP_NO_EXPORT | SND_SEQ_PORT_CAP_READ,
        SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_APPLICATION
    );
    m_local_addr_port = result;

    if (result < 0)
    {
        errprint("snd_seq_create_simple_port(write) error");
        return false;
    }
    result = snd_seq_connect_to                     /* connect to port */
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
 */

bool midibus::init_in ()
{

#ifdef SEQ64_HAVE_LIBASOUND

    int result = snd_seq_create_simple_port             /* create ports */
    (
        m_seq, "seq24 in",
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
    snd_seq_addr_t dest;

    /* the destination port is actually our local port */

    sender.client = m_dest_addr_client;
    sender.port = m_dest_addr_port;
    dest.client = m_local_addr_client;
    dest.port = m_local_addr_port;

    /* set in and out ports */

    snd_seq_port_subscribe_set_sender(subs, &sender);
    snd_seq_port_subscribe_set_dest(subs, &dest);

    /* use the master queue, and get ticks */

    snd_seq_port_subscribe_set_queue(subs, m_queue);
    snd_seq_port_subscribe_set_time_update(subs, 1);

    /* subscribe */

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
 */

bool midibus::init_out_sub ()
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

#endif  // SEQ64_HAVE_LIBASOUND

    return true;
}

/**
 *  Initialize the output in a different way?
 */

bool midibus::init_in_sub ()
{

#ifdef SEQ64_HAVE_LIBASOUND

    int result = snd_seq_create_simple_port             /* create ports */
    (
        m_seq, "seq24 in",
        SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE,
        SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_APPLICATION
    );
    m_local_addr_port = result;
    if (result < 0)
    {
        errprint("snd_seq_create_simple_port(write) error");
        return false;
    }

#endif  // SEQ64_HAVE_LIBASOUND

    return true;
}

/**
 *  Deinitialize the MIDI input?
 */

bool midibus::deinit_in()
{

#ifdef SEQ64_HAVE_LIBASOUND

    int result;

    snd_seq_port_subscribe_t *subs;
    snd_seq_port_subscribe_alloca(&subs);
    snd_seq_addr_t sender, dest;

    /* the destinatino port is actually our local port */

    sender.client = m_dest_addr_client;
    sender.port = m_dest_addr_port;
    dest.client = m_local_addr_client;
    dest.port = m_local_addr_port;

    /* set in and out ports */

    snd_seq_port_subscribe_set_sender(subs, &sender);
    snd_seq_port_subscribe_set_dest(subs, &dest);

    /* use the master queue, and get ticks */

    snd_seq_port_subscribe_set_queue(subs, m_queue);
    snd_seq_port_subscribe_set_time_update(subs, 1);

    /* subscribe */

    result = snd_seq_unsubscribe_port(m_seq, subs);
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
 *  This play() function takes a native event, encodes it to ALSA event,
 *  and puts it in the queue.
 */

void
midibus::play (event * a_e24, unsigned char a_channel)
{
#ifdef SEQ64_HAVE_LIBASOUND
    automutex locker(m_mutex);
    snd_seq_event_t ev;
    snd_midi_event_t *midi_ev;      /* ALSA MIDI parser   */
    unsigned char buffer[3];        /* temp for MIDI data */

    /* fill buffer and set midi channel */

    buffer[0] = a_e24->get_status();
    buffer[0] += (a_channel & 0x0F);
    a_e24->get_data(&buffer[1], &buffer[2]);
    snd_midi_event_new(10, &midi_ev);

    /* clear event */

    snd_seq_ev_clear(&ev);
    snd_midi_event_encode(midi_ev, buffer, 3, &ev);
    snd_midi_event_free(midi_ev);

    /* set source */

    snd_seq_ev_set_source(&ev, m_local_addr_port);
    snd_seq_ev_set_subs(&ev);
    snd_seq_ev_set_direct(&ev);     // its immediate

    /* pump it into the queue */

    snd_seq_event_output(m_seq, &ev);
#endif  // SEQ64_HAVE_LIBASOUND
}

/**
 *  min() for long values.
 */

inline long
min (long a, long b)
{
    return (a < b) ? a : b ;
}

/**
 *  Takes a native SYSEX event, encodes it to an ALSA event, and then
 *  puts it in the queue.
 */

void
midibus::sysex (event * a_e24)
{
#ifdef SEQ64_HAVE_LIBASOUND
    automutex locker(m_mutex);
    snd_seq_event_t ev;

    /* clear event */

    snd_seq_ev_clear(&ev);
    snd_seq_ev_set_priority(&ev, 1);

    /* set source */

    snd_seq_ev_set_source(&ev, m_local_addr_port);
    snd_seq_ev_set_subs(&ev);
    snd_seq_ev_set_direct(&ev);         // its immediate

    unsigned char * data = a_e24->get_sysex();
    long data_size = a_e24->get_size();
    for (long offset = 0; offset < data_size; offset += c_midibus_sysex_chunk)
    {
        long data_left = data_size - offset;
        snd_seq_ev_set_sysex
        (
            &ev, min(data_left, c_midibus_sysex_chunk), &data[offset]
        );

        /* pump it into the queue */

        snd_seq_event_output_direct(m_seq, &ev);
        usleep(80000);
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
 */

void
midibus::init_clock (long a_tick)
{
#ifdef SEQ64_HAVE_LIBASOUND
    if (m_clock_type == e_clock_pos && a_tick != 0)
    {
        continue_from(a_tick);
    }
    else if (m_clock_type == e_clock_mod || a_tick == 0)
    {
        start();

        long clock_mod_ticks = (c_ppqn / 4) * m_clock_mod;
        long leftover = (a_tick % clock_mod_ticks);
        long starting_tick = a_tick - leftover;

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
 */

void
midibus::continue_from (long a_tick)
{
#ifdef SEQ64_HAVE_LIBASOUND

    /*
     * Tell the device that we are going to start at a certain position.
     */

    long pp16th = (c_ppqn / 4);
    long leftover = (a_tick % pp16th);
    long beats = (a_tick / pp16th);
    long starting_tick = a_tick - leftover;

    /*
     * Was there anything left? Then wait for next beat (16th note) to
     * start clocking.
     */

    if (leftover > 0)
        starting_tick += pp16th;

    m_lasttick = starting_tick - 1;
    if (m_clock_type != e_clock_off)
    {
        snd_seq_event_t evc;
        snd_seq_event_t ev;
        ev.type = SND_SEQ_EVENT_CONTINUE;
        evc.type = SND_SEQ_EVENT_SONGPOS;
        evc.data.control.value = beats;
        snd_seq_ev_set_fixed(&ev);
        snd_seq_ev_set_fixed(&evc);
        snd_seq_ev_set_priority(&ev, 1);
        snd_seq_ev_set_priority(&evc, 1);
        snd_seq_ev_set_source(&evc, m_local_addr_port); /* set source */
        snd_seq_ev_set_subs(&evc);
        snd_seq_ev_set_source(&ev, m_local_addr_port);
        snd_seq_ev_set_subs(&ev);
        snd_seq_ev_set_direct(&ev);                     /* it's immediate */
        snd_seq_ev_set_direct(&evc);
        snd_seq_event_output(m_seq, &evc);              /* pump it into queue */
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
        snd_seq_ev_set_source(&ev, m_local_addr_port);  /* set source */
        snd_seq_ev_set_subs(&ev);
        snd_seq_ev_set_direct(&ev);                     /* it's immediate */
        snd_seq_event_output(m_seq, &ev);               /* pump it into queue */
    }
#endif  // SEQ64_HAVE_LIBASOUND
}

/**
 *  Set status to of "inputting" to the given value.  If the parameter is
 *  true, then init_in() is called; otherwise, deinit_in() is called.
 */

void
midibus::set_input (bool a_inputing)
{
    if (m_inputing != a_inputing)
    {
        m_inputing = a_inputing;
        if (m_inputing)
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
        snd_seq_ev_set_source(&ev, m_local_addr_port);  /* set source */
        snd_seq_ev_set_subs(&ev);
        snd_seq_ev_set_direct(&ev);                     /* it's immediate */
        snd_seq_event_output(m_seq, &ev);               /* pump it into queue */
    }
#endif  // SEQ64_HAVE_LIBASOUND
}

/**
 *  Generates the MIDI clock, starting at the given tick value.
 */

void
midibus::clock (long a_tick)
{
#ifdef SEQ64_HAVE_LIBASOUND
    automutex locker(m_mutex);
    if (m_clock_type != e_clock_off)
    {
        bool done = false;
        if (m_lasttick >= a_tick)
            done = true;

        while (!done)
        {
            m_lasttick++;
            if (m_lasttick >= a_tick)
                done = true;

            if (m_lasttick % (c_ppqn / 24) == 0)        /* tick time? */
            {
                /*
                 * Set the event tag to 127 so the sequences won't remove it.
                 */

                snd_seq_event_t ev;
                ev.type = SND_SEQ_EVENT_CLOCK;
                ev.tag = 127;
                snd_seq_ev_set_fixed(&ev);
                snd_seq_ev_set_priority(&ev, 1);
                snd_seq_ev_set_source(&ev, m_local_addr_port); /* set source */
                snd_seq_ev_set_subs(&ev);
                snd_seq_ev_set_direct(&ev);                 /* it's immediate */
                snd_seq_event_output(m_seq, &ev);       /* pump it into queue */
            }
        }
        flush();            /* and send out */
    }
#endif  // SEQ64_HAVE_LIBASOUND
}

#if 0

/**
 *  Deletes events in the queue.
 */

void
midibus::remove_queued_on_events (int a_tag)
{
    automutex locker(m_mutex);
    snd_seq_remove_events_t * remove_events;
    snd_seq_remove_events_malloc(&remove_events);
    snd_seq_remove_events_set_condition
    (
        remove_events, SND_SEQ_REMOVE_OUTPUT | SND_SEQ_REMOVE_TAG_MATCH |
            SND_SEQ_REMOVE_IGNORE_OFF
    );
    snd_seq_remove_events_set_tag(remove_events, a_tag);
    snd_seq_remove_events(m_seq, remove_events);
    snd_seq_remove_events_free(remove_events);
}

#endif  // 0

}           // namespace seq64

/*
 * midibus.cpp
 *
 * vim: sw=4 ts=4 wm=8 et ft=cpp
 */
