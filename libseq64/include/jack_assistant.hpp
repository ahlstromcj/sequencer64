#ifndef SEQ64_JACK_ASSISTANT_HPP
#define SEQ64_JACK_ASSISTANT_HPP

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
 * \file          jack_assistant.hpp
 *
 *  This module declares/defines the base class for handling many facets
 *  of performing (playing) a full MIDI song using JACK.
 *
 * \library       sequencer64 application
 * \author        Chris Ahlstrom
 * \date          2015-09-17
 * \updates       2015-10-20
 * \license       GNU GPLv2 or above
 *
 *  This class contains a number of functions that used to reside in the
 *  still-large perform module.
 */

#include "globals.h"                    /* globals, nullptr, and more       */

#include <jack/jack.h>
#include <jack/transport.h>

#ifdef SEQ64_JACK_SESSION
#include <jack/session.h>
#endif

namespace seq64
{

class perform;                          /* jack_assistant parent is perform */

/**
 *  Provide a temporary structure for passing data and results between a
 *  perform and jack_assistant object.  The jack_assistant class already
 *  has access to the members of perform, but it needs access to and
 *  modification of local variables in perform::output_func().
 */

struct jack_scratchpad
{
    double js_current_tick;
    double js_total_tick;
    double js_clock_tick;
    bool js_jack_stopped;
    bool js_dumping;
    bool js_init_clock;
    bool js_looping;                    /* perform::m_looping       */
    bool js_playback_mode;              /* perform::m_playback_mode */
};

/**
 *  This class provides the performance mode JACK support.
 */

class jack_assistant
{

    friend void jack_session_callback (jack_session_event_t * ev, void * arg);
    friend int jack_sync_callback
    (
        jack_transport_state_t state,
        jack_position_t * pos,
        void * arg
    );
    friend void jack_shutdown (void * arg);
    friend void jack_timebase_callback
    (
        jack_transport_state_t state,
        jack_nframes_t nframes,
        jack_position_t * pos,
        int new_pos,
        void * arg
    );

private:

    perform & m_jack_parent;
    jack_client_t * m_jack_client;
    jack_nframes_t m_jack_frame_current;
    jack_nframes_t m_jack_frame_last;
    jack_position_t m_jack_pos;
    jack_transport_state_t m_jack_transport_state;
    jack_transport_state_t m_jack_transport_state_last;
    double m_jack_tick;

#ifdef SEQ64_JACK_SESSION
    jack_session_event_t * m_jsession_ev;
#endif

    bool m_jack_running;
    bool m_jack_master;
    int m_ppqn;

public:

    jack_assistant (perform & parent, int ppqn = SEQ64_USE_DEFAULT_PPQN);
    ~jack_assistant ();

    /**
     * \getter m_jack_running
     */

    bool is_running () const
    {
        return m_jack_running;
    }

    /**
     * \getter m_jack_master
     */

    bool is_master () const
    {
        return m_jack_master;
    }

    /**
     * \getter m_jack_parent
     *      Needed for external callbacks.
     */

    perform & parent ()
    {
        return m_jack_parent;
    }

    bool init ();                       // init_jack ();
    void deinit ();                     // deinit_jack ();

#ifdef SEQ64_JACK_SESSION
    bool session_event ();              // jack_session_event ();
#endif

    void start ();                      // start_jack();
    void stop ();                       // stop();
    void position (bool a_state);       // position_jack();
    bool output (jack_scratchpad & pad);

private:

    void info_message (const std::string & msg);
    void error_message (const std::string & msg);

#ifdef SEQ64_USE_DEBUG_OUTPUT
    void jack_debug_print
    (
        double current_tick,
        double ticks_delta
    );
#endif

};

/**
 *  Global functions for JACK support and JACK sessions.
 */

extern int jack_sync_callback
(
    jack_transport_state_t state,
    jack_position_t * pos,
    void * arg
);
extern void print_jack_pos (jack_position_t * jack_pos);
extern void jack_shutdown (void * arg);
extern void jack_timebase_callback
(
    jack_transport_state_t state,
    jack_nframes_t nframes,
    jack_position_t * pos,
    int new_pos,
    void * arg
);

/*
 * ca 2015-07-23
 * Implemented second patch for JACK Transport from freddix/seq24
 * GitHub project.  Added the following function.
 */

extern int jack_process_callback (jack_nframes_t nframes, void * arg);

#ifdef SEQ64_JACK_SESSION
extern void jack_session_callback (jack_session_event_t * ev, void * arg);
#endif

}           // namespace seq64

#endif      // SEQ64_JACK_ASSISTANT_HPP

/*
 * jack_assistant.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

