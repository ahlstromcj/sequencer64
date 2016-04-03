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
 * \updates       2016-03-04
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

#include "calculations.hpp"             /* tempo_from_beats_per_minute()    */
#include "event.hpp"
#include "mastermidibus.hpp"
#include "midibus.hpp"

/**
 *  Macros to make capabilities-checking more readable.
 */

#define CAP_READ(cap)       (((cap) & SND_SEQ_PORT_CAP_SUBS_READ) != 0)
#define CAP_WRITE(cap)      (((cap) & SND_SEQ_PORT_CAP_SUBS_WRITE) != 0)

/**
 *  These checks need both bits to be set.  Intermediate macros used for
 *  readability.
 */

#define CAP_R_BITS      (SND_SEQ_PORT_CAP_SUBS_READ | SND_SEQ_PORT_CAP_READ)
#define CAP_W_BITS      (SND_SEQ_PORT_CAP_SUBS_WRITE | SND_SEQ_PORT_CAP_WRITE)

#define CAP_FULL_READ(cap)  (((cap) & CAP_R_BITS) == CAP_R_BITS)
#define CAP_FULL_WRITE(cap) (((cap) & CAP_W_BITS) == CAP_W_BITS)

#define ALSA_CLIENT_CHECK(pinfo) \
    (snd_seq_client_id(m_alsa_seq) != snd_seq_port_info_get_client(pinfo))

namespace seq64
{

/**
 *  The mastermidibus default constructor fills the array with our busses.
 *
 * \param ppqn
 *      Provides the PPQN value for this object.  However, in most cases, the
 *      default, SEQ64_USE_DEFAULT_PPQN should be specified.  Then the caller
 *      of this constructor should call mastermidibus::set_ppqn() to set up
 *      the proper PPQN value.
 *
 * \param bpm
 *      Provides the beats per minute value, which defaults to
 *      c_beats_per_minute.
 */

mastermidibus::mastermidibus (int ppqn, int bpm)
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
    m_beats_per_minute  (bpm),      // beats per minute
    m_num_poll_descriptors (0),
    m_poll_descriptors  (nullptr),
    m_dumping_input     (false),
    m_seq               (nullptr),
    m_mutex             ()
{
    m_ppqn = choose_ppqn(ppqn);
    for (int i = 0; i < c_max_busses; ++i)
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
    else
    {
        /*
         * Tried to reduce apparent memory leaks from libasound, but this call
         * changed nothing.
         *
         * (void) snd_config_update_free_global();
         */
    }

    /*
     * Set the client's name for ALSA.  It used to be "seq24".  Then set up our
     * ALSA client's queue.
     */

    snd_seq_set_client_name(m_alsa_seq, "sequencer64");
    m_queue = snd_seq_alloc_queue(m_alsa_seq);
#endif

    /*
     * Notify LASH of our client ID so that it can restore connections.
     */

#ifdef SEQ64_LASH_SUPPORT
    if (not_nullptr(lash_driver()))
        lash_driver()->set_alsa_client_id(snd_seq_client_id(m_alsa_seq));
#endif
}

/**
 *  The destructor deletes all of the output busses, clears out the ALSA
 *  events, stops and frees the queue, and closes ALSA for this
 *  application.
 *
 *  Valgrind indicates we might have issues caused by the following functions:
 *
 *      -   snd_config_hook_load()
 *      -   snd_config_update_r() via snd_seq_open()
 *      -   _dl_init() and other GNU function
 *      -   init_gtkmm_internals() [version 2.4]
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
    for (int i = 0; i < m_num_in_buses; i++)
    {
        if (not_nullptr(m_buses_in[i]))
        {
            delete m_buses_in[i];
            m_buses_in[i] = nullptr;
        }
    }

#ifdef SEQ64_HAVE_LIBASOUND
    snd_seq_event_t ev;
    snd_seq_ev_clear(&ev);                          /* kill timer           */
    snd_seq_stop_queue(m_alsa_seq, m_queue, &ev);
    snd_seq_free_queue(m_alsa_seq, m_queue);
    snd_seq_close(m_alsa_seq);                      /* close client         */
    (void) snd_config_update_free_global();         /* additional cleanup   */
#endif

    /*
     * Still more cleanup, not in seq24.
     */

    if (not_nullptr(m_poll_descriptors))
    {
        delete [] m_poll_descriptors;
        m_poll_descriptors = nullptr;
    }
    if (not_nullptr(m_bus_announce))
    {
        delete m_bus_announce;
        m_bus_announce = nullptr;
    }
}

/**
 *  Initialize the mastermidibus.  It initializes 16 MIDI output busses, a
 *  hardwired constant, 16.  Only one MIDI input buss is initialized.
 */

void
mastermidibus::init (int ppqn)
{
#ifdef SEQ64_HAVE_LIBASOUND
    snd_seq_client_info_t * cinfo;          /* client info */
    snd_seq_port_info_t * pinfo;            /* port info   */
    snd_seq_client_info_alloca(&cinfo);
    snd_seq_client_info_set_client(cinfo, -1);
    if (rc().manual_alsa_ports())
    {
        int num_buses = 16;
        for (int i = 0; i < num_buses; ++i)
        {
            if (not_nullptr(m_buses_out[i]))
            {
                delete m_buses_out[i];
                errprintf("mmbus::init() manual: m_buses_out[%d] not null\n", i);
            }
            m_buses_out[i] = new midibus
            (
                snd_seq_client_id(m_alsa_seq), m_alsa_seq, i+1, m_queue
            );
            m_buses_out[i]->init_out_sub();
            m_buses_out_active[i] = true;
            m_buses_out_init[i] = true;
        }
        m_num_out_buses = num_buses;
        if (not_nullptr(m_buses_in[0]))
        {
            delete m_buses_in[0];
            errprint("mmbus::init() manual: m_buses_[0] not null");
        }
        m_num_in_buses = 1;
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
        /*
         * While the next client for the sequencer is available, get the client
         * from cinfo.  Fill pinfo.
         */

        while (snd_seq_query_next_client(m_alsa_seq, cinfo) >= 0)
        {
            int client = snd_seq_client_info_get_client(cinfo);
            snd_seq_port_info_alloca(&pinfo);           /* will fill pinfo */
            snd_seq_port_info_set_client(pinfo, client);
            snd_seq_port_info_set_port(pinfo, -1);
            while (snd_seq_query_next_port(m_alsa_seq, pinfo) >= 0)
            {
                /*
                 * While the next port is available, get its capability.
                 */

                int cap = snd_seq_port_info_get_capability(pinfo);
                if
                (
                    ALSA_CLIENT_CHECK(pinfo) &&
                    snd_seq_port_info_get_client(pinfo) != SND_SEQ_CLIENT_SYSTEM
                )
                {
                    /*
                     * Why are we doing the ALSA client check again here?
                     */

                    if (CAP_WRITE(cap) && ALSA_CLIENT_CHECK(pinfo)) /* outputs */
                    {
                        if (not_nullptr(m_buses_out[m_num_out_buses]))
                        {
                            delete m_buses_out[m_num_out_buses];
                            errprintf
                            (
                                "mmbus::init(): m_buses_out[%d] not null\n",
                                m_num_out_buses
                            );
                        }
                        m_buses_out[m_num_out_buses] = new midibus
                        (
                            snd_seq_client_id(m_alsa_seq),
                            snd_seq_port_info_get_client(pinfo),
                            snd_seq_port_info_get_port(pinfo), m_alsa_seq,
                            snd_seq_client_info_get_name(cinfo),
                            snd_seq_port_info_get_name(pinfo),
                            m_num_out_buses, m_queue
                        );
                        if (m_buses_out[m_num_out_buses]->init_out())
                        {
                            m_buses_out_active[m_num_out_buses] = true;
                            m_buses_out_init[m_num_out_buses] = true;
                        }
                        else
                            m_buses_out_init[m_num_out_buses] = true;

                        ++m_num_out_buses;
                    }
                    if (CAP_READ(cap) && ALSA_CLIENT_CHECK(pinfo)) /* inputs */
                    {
                        if (not_nullptr(m_buses_in[m_num_in_buses]))
                        {
                            delete m_buses_in[m_num_in_buses];
                            errprintf
                            (
                                "mmbus::init(): m_buses_in[%d] not null\n",
                                m_num_in_buses
                            );
                        }
                        m_buses_in[m_num_in_buses] = new midibus
                        (
                            snd_seq_client_id(m_alsa_seq),
                            snd_seq_port_info_get_client(pinfo),
                            snd_seq_port_info_get_port(pinfo), m_alsa_seq,
                            snd_seq_client_info_get_name(cinfo),
                            snd_seq_port_info_get_name(pinfo),
                            m_num_in_buses, m_queue
                        );
                        m_buses_in_active[m_num_in_buses] = true;
                        m_buses_in_init[m_num_in_buses] = true;
                        ++m_num_in_buses;
                    }
                }
            }
        }                                       /* end loop for clients */
    }
    set_beats_per_minute(m_beats_per_minute);

    /*
     * set_bpm( c_bpm );
     * set_ppqn(c_ppqn);
     */

    set_ppqn(ppqn);

    /*
     * Get the number of MIDI input poll file descriptors.  Allocate the
     * poll-descriptors array.  Then get the input poll-descriptors into the
     * array
     */

    m_num_poll_descriptors = snd_seq_poll_descriptors_count(m_alsa_seq, POLLIN);
    m_poll_descriptors = new pollfd[m_num_poll_descriptors];
    snd_seq_poll_descriptors
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
        m_alsa_seq, "system", "announce",   // was "annouce" ca 2016-04-03
        0, m_queue
    );
    m_bus_announce->set_input(true);
    for (int i = 0; i < m_num_out_buses; ++i)
        set_clock(i, m_init_clock[i]);

    for (int i = 0; i < m_num_in_buses; ++i)
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
 *  Gets the output busses running again, if ALSA support is enabled.
 *
 * \threadsafe
 *
 * \param tick
 *      Provides the tick value to continue from.
 */

void
mastermidibus::continue_from (midipulse tick)
{
#ifdef SEQ64_HAVE_LIBASOUND
    automutex locker(m_mutex);
    snd_seq_start_queue(m_alsa_seq, m_queue, NULL);     /* start timer */
    for (int i = 0; i < m_num_out_buses; ++i)
        m_buses_out[i]->continue_from(tick);
#endif
}

/**
 *  Initializes the clock of each of the output busses.
 *
 * \threadsafe
 *
 * \param tick
 *      Provides the tick value with which to initialize the buss clock.
 */

void
mastermidibus::init_clock (midipulse tick)
{
    automutex locker(m_mutex);
    for (int i = 0; i < m_num_out_buses; ++i)
        m_buses_out[i]->init_clock(tick);
}

/**
 *  Stops each of the output busses.  If ALSA support is enable, also drains
 *  the output, synchronizes the output queue, and then stop the queue.
 *
 * \threadsafe
 */

void
mastermidibus::stop ()
{
    automutex locker(m_mutex);
    for (int i = 0; i < m_num_out_buses; ++i)
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
 *
 * \param tick
 *      Provides the tick value with which to set the buss clock.
 */

void
mastermidibus::clock (midipulse tick)
{
    automutex locker(m_mutex);
    for (int i = 0; i < m_num_out_buses; i++)
        m_buses_out[i]->clock(tick);
}

/**
 *  Set the PPQN value (parts per quarter note).  This is done by creating an
 *  ALSA tempo structure, adding tempo information to it, and then setting the
 *  ALSA sequencer object with this information.  Fills the tempo structure
 *  with the current tempo information.  Then sets the ppqn value.  Finally,
 *  gives the tempo structure to the ALSA queue.
 *
 * \threadsafe
 *
 * \param ppqn
 *      The PPQN value to be set.
 */

void
mastermidibus::set_ppqn (int ppqn)
{
#ifdef SEQ64_HAVE_LIBASOUND
    automutex locker(m_mutex);
    m_ppqn = ppqn;
    snd_seq_queue_tempo_t * tempo;
    snd_seq_queue_tempo_alloca(&tempo);             /* allocate tempo struct */
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
 *  We fill the ALSA tempo structure (snd_seq_queue_tempo_t) with the current
 *  tempo information, set the BPM value, put it in the tempo structure, and
 *  give the tempo value to the ALSA queue.
 *
 * \threadsafe
 *
 * \param bpm
 *      Provides the beats-per-minute value to set.
 */

void
mastermidibus::set_beats_per_minute (int bpm)
{

#ifdef SEQ64_HAVE_LIBASOUND
    automutex locker(m_mutex);
    m_beats_per_minute = bpm;
    snd_seq_queue_tempo_t *tempo;
    snd_seq_queue_tempo_alloca(&tempo);          /* allocate tempo struct */
    snd_seq_get_queue_tempo(m_alsa_seq, m_queue, tempo);
    snd_seq_queue_tempo_set_tempo(tempo, int(tempo_from_beats_per_minute(bpm)));
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
 *
 * \param ev
 *      Provides the event pointer to be set.
 */

void
mastermidibus::sysex (event * ev)
{
    automutex locker(m_mutex);
    for (int i = 0; i < m_num_out_buses; i++)
        m_buses_out[i]->sysex(ev);

    flush();                /* recursive locking! */
}

/**
 *  Handle the playing of MIDI events on the MIDI buss given by the
 *  parameter, as long as it is a legal buss number.
 *
 * \threadsafe
 *
 * \param bus
 *      The buss to start play on.
 *
 * \param e24
 *      The seq24 event to play on the buss.
 *
 * \param channel
 *      The channel on which to play the event.
 */

void
mastermidibus::play (bussbyte bus, event * e24, midibyte channel)
{
    automutex locker(m_mutex);
    if (m_buses_out_active[bus] && bus < m_num_out_buses)
        m_buses_out[bus]->play(e24, channel);
}

/**
 *  Set the clock for the given (legal) buss number.  The legality checks
 *  are a little loose, however.
 *
 * \threadsafe
 *
 * \param bus
 *      The buss to start play on.
 *
 * \param clocktype
 *      The type of clock to be set, either "off", "pos", or "mod", as noted
 *      in the midibus_common module.
 */

void
mastermidibus::set_clock (bussbyte bus, clock_e clocktype)
{
    automutex locker(m_mutex);
    if (bus < c_max_busses)
        m_init_clock[bus] = clocktype;

    if (m_buses_out_active[bus] && bus < m_num_out_buses)
        m_buses_out[bus]->set_clock(clocktype);
}

/**
 *  Gets the clock setting for the given (legal) buss number.
 *
 * \param bus
 *      Provides the buss number to read.
 *
 * \return
 *      If the buss number is legal, and the buss is active, then its clock
 *      setting is returned.  Otherwise, e_clock_off is returned.
 */

clock_e
mastermidibus::get_clock (bussbyte bus)
{
    if (m_buses_out_active[bus] && bus < m_num_out_buses)
        return m_buses_out[bus]->get_clock();

    return e_clock_off;
}

/**
 *  Set the status of the given input buss, if a legal buss number.
 *
 *  Why is another buss-count constant, and a global one at that, being
 *  used?  And I thought there was only one input buss anyway!
 *
 * \threadsafe
 *
 * \param bus
 *      Provides the buss number.
 */

void
mastermidibus::set_input (bussbyte bus, bool inputing)
{
    automutex locker(m_mutex);
    if (bus < c_max_busses)         // should be m_num_in_buses I believe!!!
        m_init_input[bus] = inputing;

    if (m_buses_in_active[bus] && bus < m_num_in_buses)
        m_buses_in[bus]->set_input(inputing);
}

/**
 *  Get the input for the given (legal) buss number.
 *
 * \param bus
 *      Provides the buss number.
 *
 * \return
 *      Always returns false.
 */

bool
mastermidibus::get_input (bussbyte bus)
{
    if (m_buses_in_active[bus] && bus < m_num_in_buses)
        return m_buses_in[bus]->get_input();

    return false;
}

/**
 *  Get the MIDI output buss name for the given (legal) buss number.
 *
 * \param bus
 *      Provides the output buss number.
 *
 * \return
 *      Returns the buss name as a standard C++ string, truncated to 80-1
 *      characters.  Also contains an indication that the buss is disconnected
 *      or unconnected.
 */

std::string
mastermidibus::get_midi_out_bus_name (int bus)
{
    if (m_buses_out_active[bus] && bus < m_num_out_buses)
    {
        return m_buses_out[bus]->get_name();
    }
    else
    {
        char tmp[80];                           /* copy names */
        if (m_buses_out_init[bus])
        {
            snprintf
            (
                tmp, sizeof(tmp), "[%d] %d:%d (disconnected)",
                bus, m_buses_out[bus]->get_client(),
                m_buses_out[bus]->get_port()
            );
        }
        else
            snprintf(tmp, sizeof(tmp), "[%d] (unconnected)", bus);

        return std::string(tmp);
    }
}

/**
 *  Get the MIDI input buss name for the given (legal) buss number.
 *
 * \param bus
 *      Provides the input buss number.
 *
 * \return
 *      Returns the buss name as a standard C++ string, truncated to 80-1
 *      characters.  Also contains an indication that the buss is disconnected
 *      or unconnected.
 */

std::string
mastermidibus::get_midi_in_bus_name (int bus)
{
    if (m_buses_in_active[bus] && bus < m_num_in_buses)
    {
        return m_buses_in[bus]->get_name();
    }
    else
    {
        char tmp[80];                       /* copy names */
        if (m_buses_in_init[bus])
        {
            snprintf
            (
                tmp, sizeof(tmp), "[%d] %d:%d (disconnected)",
                bus, m_buses_in[bus]->get_client(),
                m_buses_in[bus]->get_port()
            );
        }
        else
            snprintf(tmp, sizeof(tmp), "[%d] (unconnected)", bus);

        return std::string(tmp);
    }
}

/**
 *  Print some information about the available MIDI output busses.
 */

void
mastermidibus::print ()
{
    printf("Available busses:\n");
    for (int i = 0; i < m_num_out_buses; i++)
        printf("%s\n", m_buses_out[i]->m_name.c_str());
}

/**
 *  Initiate a poll() on the existing poll descriptors.
 *
 * \return
 *      Returns the result of the poll, or 0 if ALSA is not supported.
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
 * \return
 *      Returns true if ALSA is supported, and the returned size is greater
 *      than 0, or false otherwise.
 */

bool
mastermidibus::is_more_input ()
{
#ifdef SEQ64_HAVE_LIBASOUND
    automutex locker(m_mutex);
    return snd_seq_event_input_pending(m_alsa_seq, 0) > 0;
#else
    return false;
#endif
}

/**
 *  Start the given ALSA MIDI port.
 *
 *  \threadsafe
 *      Quite a lot is done during the lock!
 *
 * \param client
 *      Provides the ALSA client number.
 *
 * \param port
 *      Provides the ALSA client port.
 */

void
mastermidibus::port_start (int client, int port)
{
#ifdef SEQ64_HAVE_LIBASOUND
    automutex locker(m_mutex);
    snd_seq_client_info_t * cinfo;                      /* client info        */
    snd_seq_client_info_alloca(&cinfo);
    snd_seq_get_any_client_info(m_alsa_seq, client, cinfo);
    snd_seq_port_info_t * pinfo;                        /* port info          */
    snd_seq_port_info_alloca(&pinfo);
    snd_seq_get_any_port_info(m_alsa_seq, client, port, pinfo);

    int cap = snd_seq_port_info_get_capability(pinfo);  /* get its capability */
    if (ALSA_CLIENT_CHECK(pinfo))
    {
        if (CAP_FULL_WRITE(cap) && ALSA_CLIENT_CHECK(pinfo)) /* outputs */
        {
            bool replacement = false;
            int bus_slot = m_num_out_buses;
            for (int i = 0; i < m_num_out_buses; i++)
            {
                if
                (
                    m_buses_out[i]->get_client() == client  &&
                    m_buses_out[i]->get_port() == port &&
                    ! m_buses_out_active[i]
                )
                {
                    replacement = true;
                    bus_slot = i;
                }
            }
            if (not_nullptr(m_buses_out[bus_slot]))
            {
                delete m_buses_out[bus_slot];
                errprintf
                (
                    "mastermidibus::port_start(): m_buses_out[%d] not null\n",
                    bus_slot
                );
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
        if (CAP_FULL_READ(cap) && ALSA_CLIENT_CHECK(pinfo)) /* inputs */
        {
            bool replacement = false;
            int bus_slot = m_num_in_buses;
            for (int i = 0; i < m_num_in_buses; i++)
            {
                if
                (
                    m_buses_in[i]->get_client() == client  &&
                    m_buses_in[i]->get_port() == port && ! m_buses_in_active[i]
                )
                {
                    replacement = true;
                    bus_slot = i;
                }
            }
            if (not_nullptr(m_buses_in[bus_slot]))
            {
                delete m_buses_in[bus_slot];
                errprintf
                (
                    "mmbus::port_start(): m_buses_in[%d] not null\n", bus_slot
                );
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
 *  Turn off the given port for the given client.  Both the input and output
 *  busses for the given client are stopped, and set to inactive.
 *
 * \threadsafe
 *
 * \param client
 *      The client to be matched and acted on.
 *
 * \param port
 *      The port to be acted on.  Both parameter must be match before the buss
 *      is made inactive.
 */

void
mastermidibus::port_exit (int client, int port)
{
#ifdef SEQ64_HAVE_LIBASOUND
    automutex locker(m_mutex);
    for (int i = 0; i < m_num_out_buses; i++)
    {
        if (m_buses_out[i]->get_client() == client &&
            m_buses_out[i]->get_port() == port)
        {
            m_buses_out_active[i] = false;
        }
    }
    for (int i = 0; i < m_num_in_buses; i++)
    {
        if (m_buses_in[i]->get_client() == client &&
            m_buses_in[i]->get_port() == port)
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
 *
 * \param inev
 *      The event to be set based on the found input event.
 */

bool
mastermidibus::get_midi_event (event * inev)
{
#ifdef SEQ64_HAVE_LIBASOUND
    automutex locker(m_mutex);
    snd_seq_event_t * ev;
    bool sysex = false;
    bool result = false;
    midibyte buffer[0x1000];       /* temporary buffer for midi data */
    snd_seq_event_input(m_alsa_seq, &ev);
    if (! rc().manual_alsa_ports())
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

    snd_midi_event_t * midi_ev;                         /* ALSA midi parser */
    snd_midi_event_new(sizeof(buffer), &midi_ev);
    long bytes = snd_midi_event_decode(midi_ev, buffer, sizeof(buffer), ev);
    if (bytes <= 0)
        return false;

    inev->set_timestamp(ev->time.tick);
    inev->set_status(buffer[0]);
    inev->set_sysex_size(bytes);

    /*
     *  We will only get EVENT_SYSEX on the first packet of MIDI data;
     *  the rest we have to poll for.
     */

#if USE_SYSEX_PROCESSING
    if (buffer[0] == EVENT_SYSEX 
    {
        inev->restart_sysex();          /* set up for sysex if needed */
        sysex = inev->append_sysex(buffer, bytes);
    }
    else
    {
#endif
        /* some keyboards send Note On with velocity 0 for Note Off */

        inev->set_data(buffer[1], buffer[2]);
        if (inev->get_status() == EVENT_NOTE_ON && inev->get_note_velocity() == 0)
            inev->set_status(EVENT_NOTE_OFF);

        sysex = false;
#if USE_SYSEX_PROCESSING
    }
#endif

    while (sysex)       /* sysex messages might be more than one message */
    {
        snd_seq_event_input(m_alsa_seq, &ev);
        long bytes = snd_midi_event_decode(midi_ev, buffer, sizeof(buffer), ev);
        if (bytes > 0)
            sysex = inev->append_sysex(buffer, bytes);
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
 *
 * \param state
 *      Provides the dumping-input state to be set.
 *
 * \param seq
 *      Provides the sequence object to be logged as the mastermidibus's
 *      sequence.  Can also be used to set a null pointer, to disable the
 *      sequence setting.
 */

void
mastermidibus::set_sequence_input (bool state, sequence * seq)
{
    automutex locker(m_mutex);
    m_seq = seq;
    m_dumping_input = state;
}

}           // namespace seq64

/*
 * mastermidibus.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

