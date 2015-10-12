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
 *  This module declares/defines the base class for handling MIDI I/O via
 *  the ALSA system.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-30
 * \updates       2015-10-11
 * \license       GNU GPLv2 or above
 *
 *  This file provides a Linux-only implementation of MIDI support.
 *  There is a lot of common code between these two versions!
 */

#include "easy_macros.h"

#ifdef SEQ64_HAVE_LIBASOUND
#include <sys/poll.h>
#ifdef SEQ64_LASH_SUPPORT
#include "lash.hpp"
#endif
#endif

#include "mastermidibus.hpp"
#include "midibus.hpp"

namespace seq64
{

/**
 *  The mastermidibus constructor fills the array with our busses.
 */

mastermidibus::mastermidibus ()
 :
    m_alsa_seq          (nullptr),  // one pointer
    m_num_out_buses     (0),        // or c_max_busses, or what?
    m_num_in_buses      (0),        // or c_max_busses, or 1, or what?
    m_buses_out         (),         // array of c_max_busses midibus pointers
    m_buses_in          (),         // array of c_max_busses midibus pointers
    m_bus_announce      (nullptr),  // one pointer
    m_buses_out_active  (),         // array of c_max_busses booleans
    m_buses_in_active   (),         // array of c_max_busses booleans
    m_buses_out_init    (),         // array of c_max_busses booleans
    m_buses_in_init     (),         // array of c_max_busses booleans
    m_init_clock        (),         // array of c_max_busses clock_e values
    m_init_input        (),         // array of c_max_busses booleans
    m_queue             (0),
    m_ppqn              (0),
    m_bpm               (0),
    m_num_poll_descriptors (0),
    m_poll_descriptors  (nullptr),
    m_dumping_input     (false),
    m_seq               (nullptr),
    m_mutex             ()
{
    for (int i = 0; i < c_max_busses; ++i)        // why the global?
    {
        m_buses_in_active[i] = false;
        m_buses_out_active[i] = false;
        m_buses_in_init[i] = false;
        m_buses_out_init[i] = false;
        m_init_clock[i] = e_clock_off;
        m_init_input[i] = false;
    }

#ifdef SEQ64_HAVE_LIBASOUND

    /*
     * Open the sequencer client.  This line of code results in a loss of
     * 4 bytes somewhere in snd_seq_open(), as discovered via valgrind.
     */

    int result = snd_seq_open(&m_alsa_seq, "default",  SND_SEQ_OPEN_DUPLEX, 0);
    if (result < 0)
    {
        errprint("snd_seq_open() error");
        exit(1);
    }
    snd_seq_set_client_name(m_alsa_seq, "seq24");   /* client's name  */
    m_queue = snd_seq_alloc_queue(m_alsa_seq);      /* client's queue */
#endif

#ifdef SEQ64_LASH_SUPPORT
    /*
     * Notify LASH of our client ID so that it can restore connections.
     */

    if (not_nullptr(global_lash_driver))
        global_lash_driver->set_alsa_client_id(snd_seq_client_id(m_alsa_seq));
#endif
}

/**
 *  The destructor deletes all of the output busses, clears out the ALSA
 *  events, stops and frees the queue, and closes ALSA for this
 *  application.
 */

mastermidibus::~mastermidibus ()
{
    for (int i = 0; i < m_num_out_buses; i++)
    {
        if (not_nullptr(m_buses_out[i]))
        {
            delete m_buses_out[i];
            m_buses_out[i] = nullptr;
        }
    }
#ifdef SEQ64_HAVE_LIBASOUND
    snd_seq_event_t ev;
    snd_seq_ev_clear(&ev);                          /* kill timer */
    snd_seq_stop_queue(m_alsa_seq, m_queue, &ev);
    snd_seq_free_queue(m_alsa_seq, m_queue);
    snd_seq_close(m_alsa_seq);                      /* close client */
#endif
    if (not_nullptr(m_poll_descriptors))
    {
        delete [] m_poll_descriptors;
        m_poll_descriptors = nullptr;
    }
}

/**
 *  Initialize the mastermidibus.  It initializes 16 MIDI output busses, a
 *  hardwired constant, 16.  Only one MIDI input buss is initialized.
 */

void
mastermidibus::init ()
{
#ifdef SEQ64_HAVE_LIBASOUND
    snd_seq_client_info_t * cinfo;          /* client info */
    snd_seq_port_info_t * pinfo;            /* port info   */
    snd_seq_client_info_alloca(&cinfo);
    snd_seq_client_info_set_client(cinfo, -1);
    if (global_manual_alsa_ports)
    {
        int num_buses = 16;
        for (int i = 0; i < num_buses; ++i)
        {
            m_buses_out[i] = new midibus
            (
                snd_seq_client_id(m_alsa_seq), m_alsa_seq, i+1, m_queue
            );
            m_buses_out[i]->init_out_sub();
            m_buses_out_active[i] = true;
            m_buses_out_init[i] = true;
        }
        m_num_out_buses = num_buses;
        m_num_in_buses = 1;                 /* only one in, or 0? */
        m_buses_in[0] = new midibus
        (
            snd_seq_client_id(m_alsa_seq), m_alsa_seq, m_num_in_buses, m_queue
        );
        m_buses_in[0]->init_in_sub();
        m_buses_in_active[0] = true;
        m_buses_in_init[0] = true;
    }
    else
    {
        /* While the next client for the sequencer is available */

        while (snd_seq_query_next_client(m_alsa_seq, cinfo) >= 0)
        {
            int client = snd_seq_client_info_get_client(cinfo);
            snd_seq_port_info_alloca(&pinfo);           /* will fill pinfo */
            snd_seq_port_info_set_client(pinfo, client);
            snd_seq_port_info_set_port(pinfo, -1);
            while (snd_seq_query_next_port(m_alsa_seq, pinfo) >= 0)
            {
                /* While the next port is available, get its capability */

                int cap =  snd_seq_port_info_get_capability(pinfo);
                if
                (
                    snd_seq_client_id(m_alsa_seq) !=
                        snd_seq_port_info_get_client(pinfo) &&
                    snd_seq_port_info_get_client(pinfo) != SND_SEQ_CLIENT_SYSTEM
                )
                {
                    if                              /* the outputs */
                    (
                        (cap & SND_SEQ_PORT_CAP_SUBS_WRITE) != 0 &&
                        snd_seq_client_id(m_alsa_seq) !=
                            snd_seq_port_info_get_client(pinfo)
                    )
                    {
                        m_buses_out[m_num_out_buses] = new midibus
                        (
                            snd_seq_client_id(m_alsa_seq),
                            snd_seq_port_info_get_client(pinfo),
                            snd_seq_port_info_get_port(pinfo),
                            m_alsa_seq,
                            snd_seq_client_info_get_name(cinfo),
                            snd_seq_port_info_get_name(pinfo),
                            m_num_out_buses,
                            m_queue
                        );
                        if (m_buses_out[m_num_out_buses]->init_out())
                        {
                            m_buses_out_active[m_num_out_buses] = true;
                            m_buses_out_init[m_num_out_buses] = true;
                        }
                        else
                            m_buses_out_init[m_num_out_buses] = true;

                        m_num_out_buses++;
                    }
                    if                              /* the inputs */
                    (
                        (cap & SND_SEQ_PORT_CAP_SUBS_READ) != 0 &&
                        snd_seq_client_id(m_alsa_seq) !=
                            snd_seq_port_info_get_client(pinfo)
                    )
                    {
                        m_buses_in[m_num_in_buses] = new midibus
                        (
                            snd_seq_client_id(m_alsa_seq),
                            snd_seq_port_info_get_client(pinfo),
                            snd_seq_port_info_get_port(pinfo),
                            m_alsa_seq,
                            snd_seq_client_info_get_name(cinfo),
                            snd_seq_port_info_get_name(pinfo),
                            m_num_in_buses,
                            m_queue
                        );
                        m_buses_in_active[m_num_in_buses] = true;
                        m_buses_in_init[m_num_in_buses] = true;
                        m_num_in_buses++;
                    }
                }
            }
        } /* end loop for clients */
    }
    set_bpm(c_bpm);
    set_ppqn(c_ppqn);

    /*
     * Get the number of MIDI input poll file descriptors.
     */

    m_num_poll_descriptors = snd_seq_poll_descriptors_count(m_alsa_seq, POLLIN);
    m_poll_descriptors = new pollfd[m_num_poll_descriptors]; /* allocate into */
    snd_seq_poll_descriptors                    /* get input poll descriptors */
    (
        m_alsa_seq, m_poll_descriptors, m_num_poll_descriptors, POLLIN
    );
    set_sequence_input(false, nullptr);

    /* Set the input and output buffer sizes */

    snd_seq_set_output_buffer_size(m_alsa_seq, c_midibus_output_size);
    snd_seq_set_input_buffer_size(m_alsa_seq, c_midibus_input_size);
    m_bus_announce = new midibus
    (
        snd_seq_client_id(m_alsa_seq),
        SND_SEQ_CLIENT_SYSTEM, SND_SEQ_PORT_SYSTEM_ANNOUNCE,
        m_alsa_seq, "system", "annouce", 0, m_queue
    );
    m_bus_announce->set_input(true);
    for (int i = 0; i < m_num_out_buses; i++)
        set_clock(i, m_init_clock[i]);

    for (int i = 0; i < m_num_in_buses; i++)
        set_input(i, m_init_input[i]);

#endif  // SEQ64_HAVE_LIBASOUND
}

/**
 *  Starts all of the configured output busses up to m_num_out_buses.
 *
 * \threadsafe
 */

void
mastermidibus::start ()
{
#ifdef SEQ64_HAVE_LIBASOUND
    automutex locker(m_mutex);
    snd_seq_start_queue(m_alsa_seq, m_queue, NULL);     /* start timer */
    for (int i = 0; i < m_num_out_buses; i++)
        m_buses_out[i]->start();
#endif
}

/**
 *  Gets the output busses running again.
 *
 * \threadsafe
 */

void
mastermidibus::continue_from (long a_tick)
{
#ifdef SEQ64_HAVE_LIBASOUND
    automutex locker(m_mutex);
    snd_seq_start_queue(m_alsa_seq, m_queue, NULL);     /* start timer */
    for (int i = 0; i < m_num_out_buses; i++)
        m_buses_out[i]->continue_from(a_tick);
#endif
}

/**
 *  Initializes the clock of each of the output busses.
 *
 * \threadsafe
 */

void
mastermidibus::init_clock (long a_tick)
{
    automutex locker(m_mutex);
    for (int i = 0; i < m_num_out_buses; i++)
        m_buses_out[i]->init_clock(a_tick);
}

/**
 *  Stops each of the output busses.
 *
 * \threadsafe
 */

void
mastermidibus::stop ()
{
    automutex locker(m_mutex);
    for (int i = 0; i < m_num_out_buses; i++)
        m_buses_out[i]->stop();

#ifdef SEQ64_HAVE_LIBASOUND
    snd_seq_drain_output(m_alsa_seq);
    snd_seq_sync_output_queue(m_alsa_seq);
    snd_seq_stop_queue(m_alsa_seq, m_queue, NULL); /* start timer */
#endif
}

/**
 *  Generates the MIDI clock for each of the output busses.
 *
 * \threadsafe
 */

void
mastermidibus::clock (long a_tick)
{
    automutex locker(m_mutex);
    for (int i = 0; i < m_num_out_buses; i++)
        m_buses_out[i]->clock(a_tick);
}

/**
 *  Set the PPQN value (parts per quarter note).  This is done by creating
 *  an ALSA tempo structure, adding tempo information to it, and then
 *  setting the ALSA sequencer object with this information.
 *
 * \threadsafe
 */

void
mastermidibus::set_ppqn (int a_ppqn)
{
#ifdef SEQ64_HAVE_LIBASOUND
    automutex locker(m_mutex);
    snd_seq_queue_tempo_t * tempo;
    snd_seq_queue_tempo_alloca(&tempo);         /* allocate tempo struct */

    /*
     * Fill the tempo structure with the current tempo information.  Then
     * set the ppqn value.  Finally, give the tempo structure to the ALSA
     * queue.
     */

    m_ppqn = a_ppqn;
    snd_seq_get_queue_tempo(m_alsa_seq, m_queue, tempo);
    snd_seq_queue_tempo_set_ppq(tempo, m_ppqn);
    snd_seq_set_queue_tempo(m_alsa_seq, m_queue, tempo);
#endif
}

/**
 *  Set the BPM value (beats per minute).  This is done by creating
 *  an ALSA tempo structure, adding tempo information to it, and then
 *  setting the ALSA sequencer object with this information.
 *
 * \threadsafe
 */

void
mastermidibus::set_bpm (int a_bpm)
{
#ifdef SEQ64_HAVE_LIBASOUND
    automutex locker(m_mutex);
    snd_seq_queue_tempo_t *tempo;
    snd_seq_queue_tempo_alloca(&tempo);          /* allocate tempo struct */

    /*
     * Fill the tempo structure with the current tempo information, set
     * the BPM value, put it in the tempo structure, and give the tempo
     * structure to the ALSA queue.
     */

    snd_seq_get_queue_tempo(m_alsa_seq, m_queue, tempo);
    m_bpm = a_bpm;
    snd_seq_queue_tempo_set_tempo(tempo, 60000000 / m_bpm);
    snd_seq_set_queue_tempo(m_alsa_seq, m_queue, tempo);
#endif
}

/**
 *  Flushes our local queue events out into ALSA.
 *
 * \threadsafe
 */

void
mastermidibus::flush ()
{
#ifdef SEQ64_HAVE_LIBASOUND
    automutex locker(m_mutex);
    snd_seq_drain_output(m_alsa_seq);
#endif
}

/**
 *  Handle the sending of SYSEX events.
 *
 * \threadsafe
 */

void
mastermidibus::sysex (event * a_ev)
{
    automutex locker(m_mutex);
    for (int i = 0; i < m_num_out_buses; i++)
        m_buses_out[i]->sysex(a_ev);

    flush();
}

/**
 *  Handle the playing of MIDI events on the MIDI buss given by the
 *  parameter, as long as it is a legal buss number.
 *
 * \threadsafe
 */

void
mastermidibus::play (unsigned char a_bus, event * a_e24, unsigned char a_channel)
{
    automutex locker(m_mutex);
    if (m_buses_out_active[a_bus] && a_bus < m_num_out_buses)
        m_buses_out[a_bus]->play(a_e24, a_channel);
}

/**
 *  Set the clock for the given (legal) buss number.  The legality checks
 *  are a little loose, however.
 *
 * \threadsafe
 */

void
mastermidibus::set_clock (unsigned char a_bus, clock_e a_clock_type)
{
    automutex locker(m_mutex);
    if (a_bus < c_max_busses)
        m_init_clock[a_bus] = a_clock_type;

    if (m_buses_out_active[a_bus] && a_bus < m_num_out_buses)
        m_buses_out[a_bus]->set_clock(a_clock_type);
}

/**
 *  Get the clock for the given (legal) buss number.
 */

clock_e
mastermidibus::get_clock (unsigned char a_bus)
{
    if (m_buses_out_active[a_bus] && a_bus < m_num_out_buses)
        return m_buses_out[a_bus]->get_clock();

    return e_clock_off;
}

/**
 *  Set the status of the given input buss, if a legal buss number.
 *
 *  Why is another buss-count constant, and a global one at that, being
 *  used?  And I thought there was only one input buss anyway!
 *
 * \threadsafe
 */

void
mastermidibus::set_input (unsigned char a_bus, bool a_inputing)
{
    automutex locker(m_mutex);
    if (a_bus < c_max_busses)         // should be m_num_in_buses I believe!!!
        m_init_input[a_bus] = a_inputing;

    if (m_buses_in_active[a_bus] && a_bus < m_num_in_buses)
        m_buses_in[a_bus]->set_input(a_inputing);
}

/**
 *  Get the input for the given (legal) buss number.
 */

bool
mastermidibus::get_input (unsigned char a_bus)
{
    if (m_buses_in_active[a_bus] && a_bus < m_num_in_buses)
        return m_buses_in[a_bus]->get_input();

    return false;
}

/**
 *  Get the MIDI output buss name for the given (legal) buss number.
 */

std::string
mastermidibus::get_midi_out_bus_name (int a_bus)
{
    if (m_buses_out_active[a_bus] && a_bus < m_num_out_buses)
    {
        return m_buses_out[a_bus]->get_name();
    }
    else
    {
        char tmp[64];                           /* copy names */
        if (m_buses_out_init[a_bus])
        {
            snprintf
            (
                tmp, sizeof(tmp), "[%d] %d:%d (disconnected)",
                a_bus, m_buses_out[a_bus]->get_client(),
                m_buses_out[a_bus]->get_port()
            );
        }
        else
            snprintf(tmp, sizeof(tmp), "[%d] (unconnected)", a_bus);

        return std::string(tmp);
    }
}

/**
 *  Get the MIDI input buss name for the given (legal) buss number.
 */

std::string
mastermidibus::get_midi_in_bus_name (int a_bus)
{
    if (m_buses_in_active[a_bus] && a_bus < m_num_in_buses)
    {
        return m_buses_in[a_bus]->get_name();
    }
    else
    {
        char tmp[64];                       /* copy names */
        if (m_buses_in_init[a_bus])
        {
            snprintf
            (
                tmp, sizeof(tmp), "[%d] %d:%d (disconnected)",
                a_bus, m_buses_in[a_bus]->get_client(),
                m_buses_in[a_bus]->get_port()
            );
        }
        else
            snprintf(tmp, sizeof(tmp), "[%d] (unconnected)", a_bus);

        return std::string(tmp);
    }
}

/**
 *  Print some information about the available MIDI output busses.
 */

void
mastermidibus::print ()
{
    printf("Available Buses\n");
    for (int i = 0; i < m_num_out_buses; i++)
        printf("%s\n", m_buses_out[i]->m_name.c_str());
}

/**
 *  Initiate a poll() on the existing poll descriptors.
 */

int
mastermidibus::poll_for_midi ()
{
#ifdef SEQ64_HAVE_LIBASOUND
    int result = poll(m_poll_descriptors, m_num_poll_descriptors, 1000);
#else
    int result = 0;
#endif
    return result;
}

/**
 *  Test the ALSA sequencer to see if any more input is pending.
 *
 * \threadsafe
 *
 *  Does this function really need to be locked?
 */

bool
mastermidibus::is_more_input ()
{
#ifdef SEQ64_HAVE_LIBASOUND
    automutex locker(m_mutex);
    int size = snd_seq_event_input_pending(m_alsa_seq, 0);
#else
    int size = 0;
#endif
    return size > 0;
}

/**
 *  Start the given ALSA MIDI port.
 *
 *  \threadsafe
 *      Quite a lot is done during the lock!
 */

void
mastermidibus::port_start (int a_client, int a_port)
{
#ifdef SEQ64_HAVE_LIBASOUND
    automutex locker(m_mutex);
    snd_seq_client_info_t * cinfo;                      /* client info */
    snd_seq_client_info_alloca(&cinfo);
    snd_seq_get_any_client_info(m_alsa_seq, a_client, cinfo);

    snd_seq_port_info_t * pinfo;                        /* port info */
    snd_seq_port_info_alloca(&pinfo);
    snd_seq_get_any_port_info(m_alsa_seq, a_client, a_port, pinfo);

    int cap = snd_seq_port_info_get_capability(pinfo);  /* get its capability */
    if (snd_seq_client_id(m_alsa_seq) != snd_seq_port_info_get_client(pinfo))
    {
        if                                              /* the outputs */
        (
            (cap & (SND_SEQ_PORT_CAP_SUBS_WRITE | SND_SEQ_PORT_CAP_WRITE)) ==
                (SND_SEQ_PORT_CAP_SUBS_WRITE | SND_SEQ_PORT_CAP_WRITE) &&
            snd_seq_client_id(m_alsa_seq) != snd_seq_port_info_get_client(pinfo)
        )
        {
            bool replacement = false;
            int bus_slot = m_num_out_buses;
            for (int i = 0; i < m_num_out_buses; i++)
            {
                if
                (
                    m_buses_out[i]->get_client() == a_client  &&
                    m_buses_out[i]->get_port() == a_port &&
                    m_buses_out_active[i] == false
                )
                {
                    replacement = true;
                    bus_slot = i;
                }
            }
            m_buses_out[bus_slot] = new midibus
            (
                snd_seq_client_id(m_alsa_seq),
                snd_seq_port_info_get_client(pinfo),
                snd_seq_port_info_get_port(pinfo),
                m_alsa_seq,
                snd_seq_client_info_get_name(cinfo),
                snd_seq_port_info_get_name(pinfo),
                m_num_out_buses,
                m_queue
            );
            m_buses_out[bus_slot]->init_out();
            m_buses_out_active[bus_slot] = true;
            m_buses_out_init[bus_slot] = true;
            if (! replacement)
                m_num_out_buses++;
        }
        if                                          /* the inputs */
        (
            (cap & (SND_SEQ_PORT_CAP_SUBS_READ | SND_SEQ_PORT_CAP_READ)) ==
                (SND_SEQ_PORT_CAP_SUBS_READ | SND_SEQ_PORT_CAP_READ) &&
            snd_seq_client_id(m_alsa_seq) != snd_seq_port_info_get_client(pinfo)
        )
        {
            bool replacement = false;
            int bus_slot = m_num_in_buses;
            for (int i = 0; i < m_num_in_buses; i++)
            {
                if
                (
                    m_buses_in[i]->get_client() == a_client  &&
                    m_buses_in[i]->get_port() == a_port &&
                    m_buses_in_active[i] == false
                )
                {
                    replacement = true;
                    bus_slot = i;
                }
            }
            m_buses_in[bus_slot] = new midibus
            (
                snd_seq_client_id(m_alsa_seq),
                snd_seq_port_info_get_client(pinfo),
                snd_seq_port_info_get_port(pinfo),
                m_alsa_seq,
                snd_seq_client_info_get_name(cinfo),
                snd_seq_port_info_get_name(pinfo),
                m_num_in_buses,
                m_queue
            );
            m_buses_in_active[bus_slot] = true;
            m_buses_in_init[bus_slot] = true;
            if (! replacement)
                m_num_in_buses++;
        }
    }                                           /* end loop for clients */

    /*
     * Get the number of MIDI input poll file descriptors.
     */

    m_num_poll_descriptors = snd_seq_poll_descriptors_count(m_alsa_seq, POLLIN);
    m_poll_descriptors = new pollfd[m_num_poll_descriptors]; /* allocate info */
    snd_seq_poll_descriptors                        /* get input descriptors */
    (
        m_alsa_seq, m_poll_descriptors, m_num_poll_descriptors, POLLIN
    );
#endif  // SEQ64_HAVE_LIBASOUND
}

/**
 *  Turn off the given port for the given client.
 *
 * \threadsafe
 */

void
mastermidibus::port_exit (int a_client, int a_port)
{
#ifdef SEQ64_HAVE_LIBASOUND
    automutex locker(m_mutex);
    for (int i = 0; i < m_num_out_buses; i++)
    {
        if (m_buses_out[i]->get_client() == a_client &&
            m_buses_out[i]->get_port() == a_port)
        {
            m_buses_out_active[i] = false;
        }
    }
    for (int i = 0; i < m_num_in_buses; i++)
    {
        if (m_buses_in[i]->get_client() == a_client &&
            m_buses_in[i]->get_port() == a_port)
        {
            m_buses_in_active[i] = false;
        }
    }
#endif
}

/**
 *  Grab a MIDI event.
 *
 * \threadsafe
 */

bool
mastermidibus::get_midi_event (event * a_in)
{
#ifdef SEQ64_HAVE_LIBASOUND
    automutex locker(m_mutex);
    snd_seq_event_t * ev;
    bool sysex = false;
    bool result = false;
    unsigned char buffer[0x1000];       /* temporary buffer for midi data */
    snd_seq_event_input(m_alsa_seq, &ev);
    if (! global_manual_alsa_ports)
    {
        switch (ev->type)
        {
        case SND_SEQ_EVENT_PORT_START:
        {
            port_start(ev->data.addr.client, ev->data.addr.port);
            result = true;
            break;
        }
        case SND_SEQ_EVENT_PORT_EXIT:
        {
            port_exit(ev->data.addr.client, ev->data.addr.port);
            result = true;
            break;
        }
        case SND_SEQ_EVENT_PORT_CHANGE:
        {
            result = true;
            break;
        }
        default:
            break;
        }
    }
    if (result)
        return false;

    snd_midi_event_t * midi_ev;                         /* alsa midi parser */
    snd_midi_event_new(sizeof(buffer), &midi_ev);
    long bytes = snd_midi_event_decode(midi_ev, buffer, sizeof(buffer), ev);
    if (bytes <= 0)
        return false;

    a_in->set_timestamp(ev->time.tick);
    a_in->set_status(buffer[0]);
    a_in->set_size(bytes);

    /*
     *  We will only get EVENT_SYSEX on the first packet of MIDI data;
     *  the rest we have to poll for.
     */

#if 0
    if ( buffer[0] == EVENT_SYSEX )
    {
        a_in->start_sysex();            /* set up for sysex if needed */
        sysex = a_in->append_sysex(buffer, bytes);
    }
    else
    {
#endif
        /* some keyboards send Note On with velocity 0 for Note Off */

        a_in->set_data(buffer[1], buffer[2]);
        if (a_in->get_status() == EVENT_NOTE_ON && a_in->get_note_velocity() == 0)
            a_in->set_status(EVENT_NOTE_OFF);

        sysex = false;
#if 0
    }
#endif

    while (sysex)       /* sysex messages might be more than one message */
    {
        snd_seq_event_input(m_alsa_seq, &ev);
        long bytes = snd_midi_event_decode(midi_ev, buffer, sizeof(buffer), ev);
        if (bytes > 0)
            sysex = a_in->append_sysex(buffer, bytes);
        else
            sysex = false;
    }
    snd_midi_event_free(midi_ev);
#endif  // SEQ64_HAVE_LIBASOUND
    return true;
}

/**
 *  Set the input sequence object, and set the m_dumping_input value to
 *  the given state.
 *
 * \threadsafe
 */

void
mastermidibus::set_sequence_input (bool a_state, sequence * a_seq)
{
    automutex locker(m_mutex);
    m_seq = a_seq;
    m_dumping_input = a_state;
}

}           // namespace seq64

/*
 * mastermidibus.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
