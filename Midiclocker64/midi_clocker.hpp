#ifndef SEQ64_MIDI_CLOCKER_HPP
#define SEQ64_MIDI_CLOCKER_HPP

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
 * \file          midi_clocker.hpp
 *
 *  This module defines the class for the midi_clocker.
 *
 * \library       midiclocker64 application
 * \author        TODO team; refactoring by Chris Ahlstrom
 * \date          2017-11-10
 * \updates       2017-11-12
 * \license       GNU GPLv2 or above
 *
 */

#include <string>
#include <jack/jack.h>

#include "easy_macros.h"

#ifndef PLATFORM_WINDOWS
#include <signal.h>
#endif

/**
 *  Provides short-hand for using a Msg value as a bit.
 */

#define MSG_AS_BIT(x)       (static_cast<short>(midi_clocker::Msg::NO_POSITION))

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *  Provides a classy wrapper for the MIDI clocker code.
 */

class midi_clocker
{
    /*
     * These are the two JACK callbacks this class uses.
     */

    friend int jack_process (jack_nframes_t, void *);
    friend void jack_shutdown (void *);

private:

    /**
     *  Bitwise flags used with m_msg_filter().
     *
     * \var NO_TRANSPORT
     *      Don't send start/stop/continue.
     *
     * \var NO_POSITION
     *      Don't send absolute song position.
     */

    enum class Msg { NO_TRANSPORT = 1, NO_POSITION = 2 };

    /**
     *  Operational state flags.
     *
     * \var INIT
     *      Indicate that...
     *
     * \var RUN
     *      Indicate that...
     *
     * \var EXIT
     *      Indicate that...
     */

    enum class Run { INIT, RUN, EXIT };

private:

    /**
     *  JACK connection structure.
     */

    jack_port_t * m_clk_out_port;
    jack_client_t * m_jack_client;

    volatile Run m_client_state;

    /**
     *  Application state.
     */

    jack_transport_state_t m_xstate;
    double m_clk_last_tick;
    int64_t m_song_pos_sync;

    /**
     *  Keeps track of transport locates.
     */

    jack_position_t m_last_xpos;

    int m_wake_main_read;
    int m_wake_main_write;

    /**
     *  Commandline options.
     */

    double m_jitter_level;
    double m_jitter_rand;
    uint32_t m_rand_seed;

    double m_user_bpm;
    bool m_force_bpm;

    /**
     *  If set, then tempo is in quarter notes per minute instead of beats per
     *  minute.
     */

    bool m_tempo_in_qnpm;

    /**
     *  Provies the bitwise flags, Msg::NO_...
     */

    short m_msg_filter;

    /**
     *  Seconds between the 'pos' and 'continue' message.
     */

    double m_resync_delay;

public:

    /**
     *  Provides a way to pass only one midi_clocker to the signal()
     *  callback function.
     */

    static midi_clocker * sm_self;

    static void catchsig (int sig);

public:

    midi_clocker ();
    bool initialize ();
    void port_connect (char * clkport);
    void run ();
    void cleanup (int sig);

public:

    void jitter_level (double jl)
    {
        m_jitter_level = jl / 100.f;
        if (m_jitter_level < 0.f || m_jitter_level > 0.2f)
        {
            errprint("Invalid jitter-level, should be 0 <= level <= 20.%%.");
            m_jitter_level = 0;
        }
    }

    void jitter_random (double jr)
    {
        m_jitter_rand = jr;
    }

    void random_seed (uint32_t rs)
    {
        m_rand_seed = rs;
    }

    void user_bpm (double ub)
    {
        m_user_bpm = ub;        // we should validate
    }

    void force_bpm (bool fb)
    {
        m_force_bpm = fb;
    }

    void tempo_in_qnpm (bool tiq)
    {
        m_tempo_in_qnpm = tiq;
    }

    void no_song_position ()
    {
        m_msg_filter |= MSG_AS_BIT(NO_POSITION);
    }

    void no_song_transport ()
    {
        m_msg_filter |= MSG_AS_BIT(NO_TRANSPORT);
    }

    void resync_delay (double rd)
    {
        m_resync_delay = rd;
        if (m_resync_delay < 0 || m_resync_delay > 20)
        {
            errprint
            (
                "Invalid resync-delay, should be 0 <= delay <= 20.0. "
                "Using 2.0sec."
            );
            m_resync_delay = 2.0;
        }
    }

private:

    float randf ();
    int clock_process (jack_nframes_t nframes);

    void wake_main_init ();
    void wake_main_now ();
    void wake_main_wait ();

    int pos_changed (jack_position_t * xp0, jack_position_t * xp1);
    void remember_pos (jack_position_t * xp0, jack_position_t * xp1);
    int64_t calc_song_pos (jack_position_t * xpos, int off);
    int64_t send_pos_message (void * port_buf, jack_position_t * xpos, int off);
    void send_rt_message (void * port_buf, jack_nframes_t time, uint8_t rt_msg);

    bool init_jack (const std::string & client_name);
    bool jack_portsetup ();

#ifdef USE_THESE_UNUSED_FUNCTIONS

    /*
     * Neither of these functions are used in the original jack_midi_clock
     * project.
     */

    bool jack_initialize (jack_client_t * client, const char * load_init);
    void jack_finish (void * arg);

#endif

};          // class midi_clocker

}           // namespace seq64

#endif      // SEQ64_MIDI_CLOCKER_HPP

/*
 * midi_clocker.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
