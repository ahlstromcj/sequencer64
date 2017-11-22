/*
 * JACK-Transport MIDI Beat Clock Generator
 *
 * Copyright (C) 2013 Robin Gareus <robin@gareus.org>
 * Copyright (C) 2009 Gabriel M. Beddingfield <gabriel@teuton.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * \file          midi_clocker.cpp
 *
 *  This module defines the class for the midi_clocker.
 *
 * \library       midiclocker64 application
 * \author        TODO team; refactoring by Chris Ahlstrom
 * \date          2017-11-10
 * \updates       2017-11-19
 * \license       GNU GPLv2 or above
 *
 */

#include <stdio.h>                      /* C::read() and C::write()         */
#include <string.h>                     /* C::memset() function             */
#include <unistd.h>                     /* C::pipe() and C::sleep()         */
#include <math.h>                       /* C::rintf(), C::floor()           */

#include <jack/jack.h>                  /* C::jack_xxxxxxxxx() functions    */
#include <jack/midiport.h>              /* C::jack_midi_xxxx() functions    */
#include <sys/mman.h>

#include "event.hpp"                    /* Sequencer64 event symbols        */
#include "jack_assistant.hpp"           /* seq64::jack_assistant statics    */
#include "midi_clocker.hpp"             /* seq64::midi_clocker class        */

#ifndef PLATFORM_WINDOWS
#include <signal.h>
#endif

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *  A static member to (currently) hold one instance of midi_clocker.
 */

midi_clocker * midi_clocker::sm_self = nullptr;

/**
 *  A static function to pass to the signal() function.
 */

void
midi_clocker::catchsig (int /*sig*/)
{
#ifndef PLATFORM_WINDOWS
    signal(SIGHUP, catchsig);
#endif
    if (not_nullptr(midi_clocker::sm_self))
    {
        midi_clocker::sm_self->m_client_state = midi_clocker::Run::EXIT;
        midi_clocker::sm_self->wake_main_now();
    }
}

/**
 *  Constructor, also zeroes m_last_xpos and sets the sm_self member.
 */

midi_clocker::midi_clocker ()
 :
    m_clk_out_port      (nullptr),
    m_jack_client       (nullptr),
    m_client_state      (midi_clocker::Run::INIT),
    m_xstate            (JackTransportStopped),
    m_clk_last_tick     (0.0),
    m_song_pos_sync     (-1),
    m_last_xpos         (),
    m_wake_main_read    (-1),
    m_wake_main_write   (-1),
    m_jitter_level      (0.0),
    m_jitter_rand       (0.0),
    m_rand_seed         (1),
    m_user_bpm          (0.0),
    m_force_bpm         (false),
    m_tempo_in_qnpm     (true),
    m_msg_filter        (0),
    m_resync_delay      (2.0)
{
    memset(&m_last_xpos, 0, sizeof(jack_position_t));
    midi_clocker::sm_self = this;
}

/**
 *
 */

bool
midi_clocker::initialize ()
{
    bool result = init_jack("midiclocker64");
    if (result)
    {
        result = jack_portsetup();
        if (result)
        {
            if (mlockall(MCL_CURRENT | MCL_FUTURE))
            {
                errprint("Can not lock memory");
                result = false;
            }
            if (result)
            {
                if (jack_activate(m_jack_client))
                {
                    errprint("Cannot activate client.");
                    result = false;
                }
            }
        }
    }
    if (result)
    {
#ifndef PLATFORM_WINDOWS
        signal(SIGHUP, catchsig);
        signal(SIGINT, catchsig);
#endif

        m_rand_seed = jack_get_time();
        if (m_rand_seed == 0)
            m_rand_seed = 1;
    }
    return result;
}

/**
 *  All systems go.  processs() does the work in JACK realtime context
 */

void
midi_clocker::run ()
{
    wake_main_init();
    m_client_state = midi_clocker::Run::RUN;
    while (m_client_state != midi_clocker::Run::EXIT)
    {
        wake_main_wait();
    }
}

/**
 *  Provides a 31-bit Park-Miller-Carta pseudo-random number generator.
 */

float
midi_clocker::randf ()
{
    uint32_t lo = 16807 * (m_rand_seed & 0xffff);
    uint32_t hi = 16807 * (m_rand_seed >> 16);
    lo += (hi & 0x7fff) << 16;
    lo += hi >> 15;
    lo = (lo & 0x7fffffff) + (lo >> 31);
    return (m_rand_seed = lo) / 1073741824.0f - 1.0f;
}

/**
 *
 */

void
midi_clocker::wake_main_init ()
{
#ifndef PLATFORM_WINDOWS
    int pipefd[2] = {-1, -1};
    if (pipe(pipefd) == -1)
    {
        errprint("unable to create pipe for signaling main thread");
        return;
    }
    m_wake_main_read = pipefd[0];
    m_wake_main_write = pipefd[1];
#endif
}

/**
 *  Wake the main thread (for shutdown).  Call this function when the main
 *  application needs to shut down.
 */

void
midi_clocker::wake_main_now ()
{
#ifndef PLATFORM_WINDOWS
    char c = 0;
    ssize_t count = write(m_wake_main_write, &c, sizeof c);
    if (count == (-1))
    {
        errprint("wake_main_now(): write() failed");
    }
#endif
}

/**
 * Wait for a wake signal.
 * This blocks until either a signal is received or a wake
 * message is received on the pipe.
 */

void
midi_clocker::wake_main_wait ()
{
#ifndef PLATFORM_WINDOWS
    if (m_wake_main_read != -1)
    {
        char c = 0;
        ssize_t count = read(m_wake_main_read, &c, sizeof c);
        if (count == (-1))
        {
            errprint("wake_main_wait(): read() failed");
        }
    }
    else
        sleep(1);   /* fall back to using sleep() if pipe fd is invalid */
#else
    sleep(1);
#endif
}

/**
 *  Cleanup and exit.  Call this function only *after* everything has been
 *  initialized!
 */

void
midi_clocker::cleanup (int /*sig*/)
{
    if (m_jack_client)
    {
        jack_client_close(m_jack_client);
        m_jack_client = nullptr;
    }
}

/**
 *  Compare two BBT positions.
 */

int
midi_clocker::pos_changed (jack_position_t * xp0, jack_position_t * xp1)
{
    if (! (xp0->valid & JackPositionBBT))
        return -1;

    if (! (xp1->valid & JackPositionBBT))
        return -2;

    if (xp0->bar == xp1->bar && xp0->beat == xp1->beat && xp0->tick == xp1->tick)
        return 0;

    return 1;
}

/**
 *  Copy relevant BBT info from jack_position_t.
 */

void
midi_clocker::remember_pos (jack_position_t * xp0, jack_position_t * xp1)
{
    if (! (xp1->valid & JackPositionBBT))
        return;

    xp0->valid = xp1->valid;
    xp0->bar   = xp1->bar;
    xp0->beat  = xp1->beat;
    xp0->tick  = xp1->tick;
    xp0->bar_start_tick = xp1->bar_start_tick;
}

/**
 *  Calculate song position (14 bit integer) from current jack BBT info.
 *
 *  See "Song Position Pointer" at
 *  http://www.midi.org/techspecs/midimessages.php
 *
 *  Because this value is also used internally to sync/send start/continue
 *  realtime messages, a 64 bit integer is used to cover the full range of jack
 *  transport.
 *
 *      -   MIDI Beat Clock: 24 ticks per quarter note
 *      -   One MIDI-beat = six MIDI clocks -> 4 MIDI-beats per quarter note
 *          (JACK beat)
 *
 *  JACK counts bars and beats starting at 1.
 */

int64_t
midi_clocker::calc_song_pos (jack_position_t * xpos, int off)
{
    if (! (xpos->valid & JackPositionBBT))
        return -1;

    if (off < 0)
    {
        /* auto offset */

        if (xpos->bar == 1 && xpos->beat == 1 && xpos->tick == 0)
            off = 0;
        else
            off = rintf(xpos->beats_per_minute * 4.0 * m_resync_delay / 60.0);
    }

    /*
     * See note in banner.
     */

    int64_t pos = off +
        4 * ((xpos->bar - 1) * xpos->beats_per_bar + (xpos->beat - 1)) +
        floor(4.0 * xpos->tick / xpos->ticks_per_beat);

    return pos;
}

/**
 *  Send '0xf2' Song Position Pointer.  This is an internal 14 bit register
 *  that holds the number of MIDI beats (1 beat = six MIDI clocks) since the
 *  start of the song.
 */

int64_t
midi_clocker::send_pos_message (void * port_buf, jack_position_t * xpos, int off)
{
    /*
     * TMI:
     */

    printf("send_pos_message()\n");
    if (m_msg_filter & MSG_AS_BIT(NO_POSITION))
    {
        errprint("send_pos_message(): no position");
        return -1;
    }

    const int64_t bcnt = calc_song_pos(xpos, off);
    if (bcnt < 0 || bcnt >= 16384)
    {
        errprint("send_pos_message(): bcnt out of range");
        return -1;
    }

    uint8_t * buffer = jack_midi_event_reserve(port_buf, 0, 3);
    if (is_nullptr(buffer))
    {
        errprint("send_pos_message(): could not reserve MIDI event");
        return -1;
    }

    buffer[0] = EVENT_MIDI_SONG_POS;
    buffer[1] = bcnt & 0x7f;            // LSB
    buffer[2] = (bcnt >> 7) & 0x7f;     // MSB
    return bcnt;
}

/**
 *  Send 1 byte MIDI Message.
 *
 * \param port_buf
 *      buffer to write event to
 *
 * \param time
 *      sample offset of event
 *
 * \param rt_msg
 *      message byte
 */

void
midi_clocker::send_rt_message
(
    void * port_buf, jack_nframes_t time, uint8_t rt_msg
)
{
    /*
     * TMI:
     */

    printf("send_rt_message(%X)\n", unsigned(rt_msg));

    uint8_t * buffer = jack_midi_event_reserve(port_buf, time, 1);
    if (not_nullptr(buffer))
        buffer[0] = rt_msg;
}

/**
 *  JACK process callback.  Do the work: query jack-transport, send MIDI
 *  messages.
 *
 *  It is an industry convention that tempo, while reported as "beats per
 *  minute" is actually "quarter notes per minute" in many DAW's.  However,
 *  some DAW's/musicians actually use beats per minute (using the definition
 *  of "beat" as the denomitor of the time signature). While it appears that
 *  the JACK transport's intent is the latter, it's totally up to the DAW to
 *  define the tempo/note relationship. Currently Ardour does "quarter notes
 *  per minute."
 *
 *  Viz. https://community.ardour.org/node/1433 and
 *  http://www.steinberg.net/forums/viewtopic.php?t=56065
 *
 *  This function retrieves the midi_clocker pointer that was set up for this
 *  callback, and passed it to midi_clock::clock_process() so that we don't
 *  have to redirect to "mc" so often.
 */

int
jack_process (jack_nframes_t nframes, void * arg)
{
    midi_clocker * mc = (midi_clocker *) arg;
    return not_nullptr(mc) ? mc->clock_process(nframes) : -1 ;
}

#ifdef PLATFORM_GNU
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#endif

/**
 *  Does the actual work of the JACK process callback.
 *
 * \param nframes
 *      The frame number provided by JACK.
 *
 * \return
 *      Always returns 0.  This may be a bit too simplistic.
 */

int
midi_clocker::clock_process (jack_nframes_t nframes)
{
    /* query jack transport state */

    jack_position_t xpos;
    jack_transport_state_t xstate = jack_transport_query(m_jack_client, &xpos);
    void * port_buf = jack_port_get_buffer(m_clk_out_port, nframes);

    std::string statename = jack_assistant::get_state_name(xstate);
    printf("clock_process(%ld): %s\n", long(nframes), statename.c_str());
    jack_assistant::show_position(xpos);

    /* prepare MIDI buffer */

    jack_midi_clear_buffer(port_buf);
    if (m_client_state != midi_clocker::Run::RUN)
    {
        infoprint("clock_process():  not running, returning");
        return 0;
    }

    /* send position updates if stopped and located */

    if (xstate == JackTransportStopped && xstate == m_xstate)
    {
        if (pos_changed(&m_last_xpos, &xpos) > 0)
        {
            m_song_pos_sync = send_pos_message(port_buf, &xpos, -1);
        }
    }
    remember_pos(&m_last_xpos, &xpos);

    /* send RT messages start/stop/continue if transport state changed */

    if (xstate != m_xstate)
    {
        switch (xstate)
        {
        case JackTransportStopped:

            if (! (m_msg_filter & MSG_AS_BIT(NO_TRANSPORT)))
            {
                send_rt_message(port_buf, 0, EVENT_MIDI_STOP);
            }
            m_song_pos_sync = send_pos_message(port_buf, &xpos, -1);
            break;

        case JackTransportRolling:

            /*
             * Handle transport locate while rolling.  JACK transport state
             * changes: Rolling -> Starting -> Rolling
             */

            if
            (
                m_xstate == JackTransportStarting &&
                ! (m_msg_filter & MSG_AS_BIT(NO_POSITION))
            )
            {
                if (m_song_pos_sync < 0)
                {
                    /* send stop IFF not stopped, yet */

                    send_rt_message(port_buf, 0, EVENT_MIDI_STOP);
                }
                if (m_song_pos_sync != 0)
                {
                    /* re-set 'continue' message sync point */

                    m_song_pos_sync = send_pos_message(port_buf, &xpos, -1);
                    if (m_song_pos_sync < 0)
                    {
                        if (! (m_msg_filter & MSG_AS_BIT(NO_TRANSPORT)))
                        {
                            send_rt_message(port_buf, 0, EVENT_MIDI_CONTINUE);
                        }
                    }
                }
                else
                {
                    /* 'start' at 0, don't queue 'continue' message */

                    m_song_pos_sync = -1;
                }
                break;
            }

            /*
             * Potential fall-through
             */

        case JackTransportStarting:

            if (m_xstate == JackTransportStarting)
            {
                break;
            }
            if (xpos.frame == 0)
            {
                if (!(m_msg_filter & MSG_AS_BIT(NO_TRANSPORT)))
                {
                    send_rt_message(port_buf, 0, EVENT_MIDI_START);
                    m_song_pos_sync = 0;
                }
            }
            else
            {
                /*
                 * Only send Continue message here if song-position is not used.
                 * With song-pos it queued just-in-time.
                 */

                if
                (
                    ! (m_msg_filter & MSG_AS_BIT(NO_TRANSPORT)) &&
                    (m_msg_filter & MSG_AS_BIT(NO_POSITION))
                )
                {
                    send_rt_message(port_buf, 0, EVENT_MIDI_CONTINUE);
                }
            }
            break;

        default:

            break;
        }

        /* initial beat tick */

        if
        (
            (xstate == JackTransportRolling) &&
            ((xpos.frame == 0) || ((m_msg_filter & MSG_AS_BIT(NO_POSITION)) != 0))
        )
        {
            send_rt_message(port_buf, 0, EVENT_MIDI_CLOCK);
        }
        m_clk_last_tick = xpos.frame;
        m_xstate = xstate;
    }

    if (xstate != JackTransportRolling)
        return 0;

    /* calculate clock tick interval */

    double samples_per_beat = 0.0;
    jack_nframes_t bbt_offset = 0;
    if (m_force_bpm && m_user_bpm > 0)
    {
        samples_per_beat = (double) xpos.frame_rate * 60.0 / m_user_bpm;
    }
    else if (xpos.valid & JackPositionBBT)
    {
        samples_per_beat = (double) xpos.frame_rate * 60.0 / xpos.beats_per_minute;
        if (xpos.valid & JackBBTFrameOffset)
            bbt_offset = xpos.bbt_offset;
    }
    else if (m_user_bpm > 0)
    {
        samples_per_beat = (double) xpos.frame_rate * 60.0 / m_user_bpm;
    }
    else
    {
        return 0;           /* no tempo known */
    }

    /*
     * See note about "quarter notes per minute" in the function banner.
     * MIDI Beat Clock: Send 24 ticks per quarter note.
     */

    const double qn_per_beat = (m_tempo_in_qnpm) ? 1.0 : (xpos.beat_type / 4.0);
    const double samples_per_qn = samples_per_beat / qn_per_beat;
    const double clock_ticks = samples_per_qn / 24.0;

    int ticks_this_cycle = 0;      /* sent clock ticks for this cycle */
    for (;;)
    {
        const double next_tick =
            m_clk_last_tick + clock_ticks + m_jitter_rand;

        const int64_t next_tick_offset =
            llrint(next_tick) - xpos.frame - bbt_offset;

        if (next_tick_offset >= nframes)
            break;

        if (next_tick_offset >= 0)
        {
            if (m_song_pos_sync > 0 && ! (m_msg_filter & MSG_AS_BIT(NO_POSITION)))
            {
                /*
                 * Send 'continue' realtime message on time.
                 * 4 MIDI-beats per quarter note (jack beat)
                 */

                const int64_t sync = calc_song_pos(&xpos, 0);
                if (sync + ticks_this_cycle / 4 >= m_song_pos_sync)
                {
                    if (! (m_msg_filter & MSG_AS_BIT(NO_TRANSPORT)))
                    {
                        send_rt_message
                        (
                            port_buf, next_tick_offset, EVENT_MIDI_CONTINUE
                        );
                    }
                    m_song_pos_sync = -1;
                }
            }

            /* enqueue clock tick */

            send_rt_message(port_buf, next_tick_offset, EVENT_MIDI_CLOCK);
        }

        if (m_jitter_level > 0.0)
            m_jitter_rand = randf() * m_jitter_level * clock_ticks;
        else
            m_jitter_rand = 0.0;

        m_clk_last_tick = next_tick;
        ++ticks_this_cycle;
    }
    return 0;
}

/**
 * \callback
 *      If JACK server terminates.
 */

void
jack_shutdown (void * arg)
{
    midi_clocker * mc = (midi_clocker *) arg;
    errprint("received shutdown request from JACK");
    if (not_nullptr(mc))
    {
        mc->m_client_state = midi_clocker::Run::EXIT;
        mc->wake_main_now();
    }
}

/**
 *  Open a client connection to the JACK server.  Some of the code
 *  from the unused jack_initialize() function, to set up m_last_xpos and the
 *  jitter functionality, are now done elsewhere.
 *
 *  Compare this function to the create_jack_client() function in the
 *  libseq64/src/jack_assistant.cpp module.  That one can replace everything
 *  except setting the process and shutdown callbacks.
 */

bool
midi_clocker::init_jack (const std::string & clientname)
{
printf("init_jack()\n");
    jack_status_t status;
    m_jack_client = jack_client_open(clientname.c_str(), JackNullOption, &status);
    if (is_nullptr(m_jack_client))
    {
        fprintf(stderr, "jack_client_open() failed, status = 0x%2.0x\n", status);
        if (status & JackServerFailed)
        {
            errprint("Unable to connect to JACK server");
        }
        return false;
    }
    if (status & JackServerStarted)
    {
        infoprint("JACK server started");
    }
    else
    {
        warnprint("JACK server already started");
    }
    if (status & JackNameNotUnique)
    {
        const char * cn = jack_get_client_name(m_jack_client);
        fprintf(stderr, "JACK client name not unique: `%s'\n", cn);
    }
    int rc = jack_set_process_callback(m_jack_client, jack_process, (void *) this);
    if (rc != 0)
    {
        errprint("Unable to connect to set JACK process callback");
        return false;
    }

#ifndef PLATFORM_WINDOWS
    jack_on_shutdown(m_jack_client, jack_shutdown, (void *) this);
#endif

    return true;
}

/**
 *
 */

bool
midi_clocker::jack_portsetup ()
{
printf("jack_portsetup()\n");
    m_clk_out_port = jack_port_register
    (
        m_jack_client, "mclk_out", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0
    );
    if (is_nullptr(m_clk_out_port))
    {
        errprint("Cannot register mclk output port");
        return false;
    }
    return true;
}

/**
 *
 */

void
midi_clocker::port_connect (char * clkport)
{
printf("port_connect()\n");
    if
    (
        clkport &&
        jack_connect(m_jack_client, jack_port_name(m_clk_out_port), clkport)
    )
    {
        fprintf
        (
            stderr, "cannot connect port %s to %s\n",
            jack_port_name(m_clk_out_port), clkport
        );
    }
}

#ifdef USE_THESE_UNUSED_FUNCTIONS

/*
 * Neither of these functions are used in the original jack_midi_clock project.
 */

/**
 *  TODO:  parse the load_init parameter
 */

bool
midi_clocker::jack_initialize
(
    jack_client_t * client,
    const char * /*load_init*/ // TODO parse load_init
)
{
    memset(&m_last_xpos, 0, sizeof(jack_position_t));
    m_jack_client = client;
    jack_set_process_callback(client, process, 0);
    if (! jack_portsetup())
        return true;

    if (jack_activate(m_jack_client))
    {
        fprintf(stderr, "cannot activate client.\n");
        return true;
    }

    m_rand_seed = jack_get_time();
    if (m_rand_seed == 0)
        m_rand_seed = 1;

    m_client_state = midi_clocker::Run::RUN;
    return false;
}

/**
 *
 */

void
midi_clocker::jack_finish (void * arg)
{
    m_client_state = midi_clocker::Run::EXIT;
    m_jack_client = NULL;
}

#endif      // USE_THESE_UNUSED_FUNCTIONS

}           // namespace seq64

/*
* midi_clocker.cpp
*
* vim: sw=4 ts=4 wm=4 et ft=cpp
*/

