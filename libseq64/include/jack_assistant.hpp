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
 *  of performing (playing) a full MIDI song.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2015-09-16
 * \license       GNU GPLv2 or above
 *
 *  This class has way too many members.
 */

#include "globals.h"               // globals, nullptr, and config headers

#include <jack/jack.h>
#include <jack/transport.h>

#ifdef SEQ64_JACK_SESSION
#include <jack/session.h>
#endif

namespace seq64
{

/**
 *  This class provides the performance mode JACK support.
 */

class jack_assistant
{

// Are these function needed by jack_assistant?
//
#ifdef SEQ64_JACK_SUPPORT

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

#endif  // SEQ64_JACK_SUPPORT

private:

private:

    jack_client_t * m_jack_client;
    jack_nframes_t m_jack_frame_current;
    jack_nframes_t m_jack_frame_last;
    jack_position_t m_jack_pos;
    jack_transport_state_t m_jack_transport_state;
    jack_transport_state_t m_jack_transport_state_last;
    double m_jack_tick;

#ifdef SEQ64_JACK_SESSION

public:

    jack_session_event_t * m_jsession_ev;

private:

#endif  // SEQ64_JACK_SESSION

    bool m_jack_running;
    bool m_jack_master;

public:

    jack_assistant ();
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

    void init_jack ();
    void deinit_jack ();

#ifdef SEQ64_JACK_SESSION
    bool jack_session_event ();
#endif

    void start_jack ();
    void stop_jack ();
    void position_jack (bool a_state);

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
 * vim: sw=4 ts=4 wm=8 et ft=cpp
 */

