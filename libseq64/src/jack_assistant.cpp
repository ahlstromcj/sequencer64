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
 * \file          jack_assistant.cpp
 *
 *  This module defines the helper class for using JACK in the performance
 *  mode.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-09-14
 * \updates       2015-09-17
 * \license       GNU GPLv2 or above
 *
 */

#include <stdio.h>

// #include <gtkmm/main.h>                // Gtk::Main

#include "jack_assistant.hpp"
#include "midifile.hpp"
#include "perform.hpp"

namespace seq64
{

/**
 *  This construction initializes a vast number of member variables, some
 *  of them public!
 */

jack_assistant::jack_assistant (perform & parent)
 :
    m_jack_parent               (parent),
    m_jack_client               (nullptr),
    m_jack_frame_current        (),
    m_jack_frame_last           (),
    m_jack_pos                  (),
    m_jack_transport_state      (),
    m_jack_transport_state_last (),
    m_jack_tick                 (0.0),
#ifdef SEQ64_JACK_SESSION
    m_jsession_ev               (nullptr),
#endif
    m_jack_running              (false),
    m_jack_master               (false)
{
    // no other code
}

/**
 *  The destructor ...
 */

jack_assistant::~jack_assistant ()
{
    // Anything to do?
}

/**
 *  Initializes JACK support, if SEQ64_JACK_SUPPORT is defined.
 *  Then we become a new client of the JACK server.
 *
 *  Who calls this routine?
 *
 * \return
 *      Returns true if JACK is now considered to be running (or if it was
 *      already running.).
 */

bool
jack_assistant::init ()
{
    if (global_with_jack_transport && ! m_jack_running)
    {
        m_jack_running = true;
        m_jack_master = true;

#ifdef SEQ64_JACK_SESSION
        if (global_jack_session_uuid.empty())
        {
            m_jack_client = jack_client_open(SEQ64_PACKAGE, JackNullOption, NULL);
        }
        else
        {
            m_jack_client = jack_client_open
            (
                SEQ64_PACKAGE, JackSessionID, NULL,
                global_jack_session_uuid.c_str()
            );
        }
#else
        m_jack_client = jack_client_open(SEQ64_PACKAGE, JackNullOption, NULL);
#endif

        if (m_jack_client == 0)
        {
            printf("JACK server is not running.\n[JACK sync disabled]\n");
            m_jack_running = false;
            return false;
        }

        /*
         * "this" should be the parent perform object.
         */

        jack_on_shutdown(m_jack_client, jack_shutdown, (void *) &m_jack_parent);
        jack_set_sync_callback
        (
            m_jack_client, jack_sync_callback, (void *) &m_jack_parent
        );

        /*
         * ca 2015-07-23
         * Implemented patch from freddix/seq24 GitHub project, to fix
         * JACK transport.  One line of code.
         */

        jack_set_process_callback(m_jack_client, jack_process_callback, NULL);

#ifdef SEQ64_JACK_SESSION
        if (jack_set_session_callback)
        {
            jack_set_session_callback
            (
                m_jack_client, jack_session_callback, (void *) &m_jack_parent
            );
        }
#endif
        /* true if we want to fail if there is already a master */

        bool cond = global_with_jack_master_cond;
        if
        (
            global_with_jack_master &&
            jack_set_timebase_callback
            (
                m_jack_client, cond, jack_timebase_callback, &m_jack_parent
            ) == 0
        )
        {
            printf("[JACK transport master]\n");
            m_jack_master = true;
        }
        else
        {
            printf("[JACK transport slave]\n");
            m_jack_master = false;
        }
        if (jack_activate(m_jack_client))
        {
            printf("Cannot register as JACK client\n");
            m_jack_running = false;
        }
    }
    return m_jack_running;
}

/**
 *  Tears down the JACK infrastructure.
 */

void
jack_assistant::deinit ()
{
    if (m_jack_running)
    {
        m_jack_running = false;
        m_jack_master = false;
        if (jack_release_timebase(m_jack_client))
        {
            errprint("Cannot release Timebase.");
        }
        if (jack_client_close(m_jack_client))
        {
            errprint("Cannot close JACK client.");
        }
    }
    if (! m_jack_running)
    {
        infoprint("[JACK sync disabled]");
    }
}

/**
 *  If JACK is supported, starts the JACK transport.
 */

void
jack_assistant::start ()
{
    if (m_jack_running)
        jack_transport_start(m_jack_client);
}

/**
 *  If JACK is supported, stops the JACK transport.
 */

void
jack_assistant::stop ()
{
    if (m_jack_running)
        jack_transport_stop(m_jack_client);
}

/**
 *  If JACK is supported and running, sets the position of the transport.
 *
 * \warning
 *      A lot of this code is effectively disabled by an early return
 *      statement.
 */

void
jack_assistant::position (bool a_state)
{
    if (m_jack_running)
    {
        jack_transport_locate(m_jack_client, 0);
    }
    return;

#ifdef WHY_IS_THIS_CODE_EFFECTIVELY_DISABLED
    jack_nframes_t rate = jack_get_sample_rate(m_jack_client);
    long current_tick = 0;
    if (a_state)
        current_tick = m_left_tick;

    jack_position_t pos;
    pos.valid = JackPositionBBT;
    pos.beats_per_bar = 4;
    pos.beat_type = 4;
    pos.ticks_per_beat = c_ppqn * 10;
    pos.beats_per_minute =  m_master_bus.get_bpm();

    /*
     * Compute BBT info from frame number.  This is relatively simple
     * here, but would become complex if we supported tempo or time
     * signature changes at specific locations in the transport timeline.
     */

    current_tick *= 10;
    pos.bar = int32_t
    (
        (current_tick / (long) pos.ticks_per_beat / pos.beats_per_bar);
    );

    pos.beat = int32_t(((current_tick / (long) pos.ticks_per_beat) % 4));
    pos.tick = int32_t((current_tick % (c_ppqn * 10)));
    pos.bar_start_tick = pos.bar * pos.beats_per_bar * pos.ticks_per_beat;
    pos.frame_rate = rate;
    pos.frame = (jack_nframes_t)
    (
        (current_tick * rate * 60.0) / (pos.ticks_per_beat * pos.beats_per_minute)
    );

    /*
     * ticks * 10 = jack ticks;
     * jack ticks / ticks per beat = num beats;
     * num beats / beats per minute = num minutes
     * num minutes * 60 = num seconds
     * num secords * frame_rate  = frame
     */

    pos.bar++;
    pos.beat++;
    jack_transport_reposition(m_jack_client, &pos);

#endif  // WHY_IS_THIS_CODE_EFFECTIVELY_DISABLED

}

/*
 *  Implemented second patch for JACK Transport from freddix/seq24 GitHub
 *  project.  Added the following function.
 */

int
jack_process_callback (jack_nframes_t nframes, void * arg)
{
    return 0;
}

/**
 *  This JACK synchronization callback informs the specified perform
 *  object of the current state and parameters of JACK.
 *
 * \param state
 *      The JACK Transport state.
 *
 * \param pos
 *      The JACK position value.
 *
 * \param arg
 *      The pointer to the perform object.  Currently not checked for
 *      nullity.
 */

int
jack_sync_callback
(
    jack_transport_state_t state,
    jack_position_t * pos,
    void * arg                          // unchecked, dynamic_cast?
)
{
    // perform * p = (perform *) arg;

    jack_assistant * jack = (jack_assistant *) arg;

    jack->m_jack_frame_current = jack_get_current_transport_frame(jack->m_jack_client);
    jack->m_jack_tick =
        jack->m_jack_frame_current * jack->m_jack_pos.ticks_per_beat *
        jack->m_jack_pos.beats_per_minute / (jack->m_jack_pos.frame_rate * 60.0) ;

    jack->m_jack_frame_last = jack->m_jack_frame_current;
    jack->m_jack_transport_state_last = state;
    jack->m_jack_transport_state = state;
    switch (state)
    {
    case JackTransportStopped:
        infoprint("[JackTransportStopped]");
        break;

    case JackTransportRolling:
        infoprint("[JackTransportRolling]");
        break;

    case JackTransportStarting:
        infoprint("[JackTransportStarting]");
        jack->parent().inner_start(global_jack_start_mode);
        break;

    case JackTransportLooping:
        infoprint("[JackTransportLooping]");
        break;

    default:
        break;
    }
    print_jack_pos(pos);
    return 1;
}

#ifdef SEQ64_JACK_SESSION

/**
 *  Writes the MIDI file named "<jack session dir>-file.mid" using a
 *  mididfile object, quits if told to by JACK, and can free the JACK
 *  session event.
 *
 *  ca 2015-07-24
 *  Just a note:  The OMA (OpenMandrivaAssociation) patch was already
 *  applied to seq24 v.0.9.2.  It put quotes around the --file argument.
 *
 *  Why are we using a Glib::ustring here?  Convenience.  But with
 *  C++11, we could use a lexical_cast<>.  No more ustring, baby!
 *
 *  It doesn't really matter; this function can call Gtk::Main::quit().
 *
 *  PROVIDE a hook to a perform object or GUI item to perfom the quit!!!!!
 *  TRY to move midifile into this library!!!!!!!!
 */

bool
jack_assistant::session_event ()
{
    std::string fname(m_jsession_ev->session_dir);
    fname += "file.mid";
    std::string cmd
    (
        "sequencer64 --file \"${SESSION_DIR}file.mid\" --jack_session_uuid "
    );
    cmd += m_jsession_ev->client_uuid;

    /*
     * MIDI file access!!!
     */

    midifile f(fname, ! global_legacy_format);
    f.write(&m_jack_parent);

    m_jsession_ev->command_line = strdup(cmd.c_str());
    jack_session_reply(m_jack_client, m_jsession_ev);

    /*
     * GUI framework access!!!
     *
     * TO BE FIXED!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
     *

    if (m_jsession_ev->type == JackSessionSaveAndQuit)
        Gtk::Main::quit();

     *
     *
     */

    jack_session_event_free(m_jsession_ev);
    return false;
}

/**
 *  Set the m_jsession_ev (event) value of the perform object.
 *
 *  Glib is then used to connect in perform::jack_session_event().
 *
 * \param arg
 *      The pointer to the perform object.  Currently not checked for
 *      nullity.
 */

void
jack_session_callback (jack_session_event_t * event, void * arg)
{
    // perform * p = (perform *) arg;

    jack_assistant * jack = (jack_assistant *) arg;
    jack->m_jsession_ev = event;

    // TO BE RECTIFIED SOON
    //
    // Glib::signal_idle().
    //      connect(sigc::mem_fun(*p, &perform::jack_session_event));
    //
    // TO BE FIXED!!!!!!!!!
}

#endif  // SEQ64_JACK_SESSION

/**
 *  Performance output function for JACK, called by the perform function
 *  of the same name.
 */

#ifdef THIS_FUNCTION_IS_READY

void
jack_assistant::output (jack_scratchpad & pad)
{
    double jack_ticks_converted = 0.0;
    double jack_ticks_converted_last = 0.0;
    double jack_ticks_delta = 0.0;
    if (m_jack_running)             // no init until we get a good lock
    {
        init_clock = false;
        m_jack_transport_state =
            jack_transport_query(m_jack_client, &m_jack_pos);

        m_jack_frame_current =
            jack_get_current_transport_frame(m_jack_client);

        if (m_jack_transport_state_last == JackTransportStarting &&
                m_jack_transport_state == JackTransportRolling)
        {

            m_jack_frame_last = m_jack_frame_current;
            dumping = true;
            m_jack_tick =
                m_jack_pos.frame * m_jack_pos.ticks_per_beat *
                m_jack_pos.beats_per_minute / (m_jack_pos.frame_rate * 60.0)
                ;

            jack_ticks_converted = m_jack_tick *        /* convert ticks */
            (
                (double) c_ppqn / (m_jack_pos.ticks_per_beat *
                m_jack_pos.beat_type / 4.0)
            );

            set_orig_ticks((long) jack_ticks_converted);
            current_tick = clock_tick = total_tick =
                jack_ticks_converted_last = jack_ticks_converted;

            init_clock = true;

            /*
             * We need to make sure another thread can't modify these
             * values.
             */

            if (m_looping && m_playback_mode)
            {
                if (current_tick >= get_right_tick())
                {
                    while (current_tick >= get_right_tick())
                    {
                        double size = get_right_tick() - get_left_tick();
                        current_tick = current_tick - size;
                    }
                    reset_sequences();
                    set_orig_ticks((long)current_tick);
                }
            }
        }
        if (m_jack_transport_state_last  ==  JackTransportRolling &&
                m_jack_transport_state  == JackTransportStopped)
        {
            m_jack_transport_state_last = JackTransportStopped;
            jack_stopped = true;
        }

        /*
         * Jack Transport is Rolling Now !!!
         * Transport is in a sane state if dumping == true.
         */

        if (dumping)
        {
            m_jack_frame_current =
                jack_get_current_transport_frame(m_jack_client);

            // if we are moving ahead

            if ((m_jack_frame_current > m_jack_frame_last))
            {
                m_jack_tick +=
                    (m_jack_frame_current - m_jack_frame_last)  *
                    m_jack_pos.ticks_per_beat *
                    m_jack_pos.beats_per_minute /
                    (m_jack_pos.frame_rate * 60.0);

                m_jack_frame_last = m_jack_frame_current;
            }
            jack_ticks_converted =      // convert ticks
                m_jack_tick *
                (
                    (double) c_ppqn /
                    (m_jack_pos.ticks_per_beat*m_jack_pos.beat_type/4.0)
                );

            jack_ticks_delta =
                jack_ticks_converted - jack_ticks_converted_last;

            clock_tick     += jack_ticks_delta;
            current_tick   += jack_ticks_delta;
            total_tick     += jack_ticks_delta;
            m_jack_transport_state_last = m_jack_transport_state;
            jack_ticks_converted_last = jack_ticks_converted;

#ifdef USE_DEBUGGING_OUTPUT
            jack_debug_print(current_tick, ticks_delta);
#endif  // USE_DEBUGGING_OUTPUT

        }               // if dumping (sane state)
    }                   // if m_jack_running
}

#endif  // THIS_FUNCTION_IS_READY

#ifdef USE_DEBUGGING_OUTPUT

void
jack_assistant::jack_debug_print
(
    double current_tick,
    double ticks_delta
)
{
            double jack_tick = (m_jack_pos.bar-1) *
                (m_jack_pos.ticks_per_beat * m_jack_pos.beats_per_bar ) +
                (m_jack_pos.beat-1) * m_jack_pos.ticks_per_beat +
                m_jack_pos.tick;
            long ptick, pbeat, pbar;
            long pbar = long
            (
                long(m_jack_tick) /
                (m_jack_pos.ticks_per_beat * m_jack_pos.beats_per_bar)
            );
            long pbeat = long
            (
                long(m_jack_tick) %
                (m_jack_pos.ticks_per_beat * m_jack_pos.beats_per_bar)
            );
            pbeat /= long(m_jack_pos.ticks_per_beat);
            long ptick = long(m_jack_tick) % long(m_jack_pos.ticks_per_beat);
            printf
            (
                "* current_tick[%lf] delta[%lf]"
                "* bbb [%2d:%2d:%4d] "
                "* jjj [%2d:%2d:%4d] "
                "* jtick[%8.3f] mtick[%8.3f] delta[%8.3f]\n"
                ,
                current_tick, ticks_delta,
                pbar+1, pbeat+1, ptick,
                m_jack_pos.bar, m_jack_pos.beat, m_jack_pos.tick,
                m_jack_tick, jack_tick, m_jack_tick-jack_tick
            );
}
#endif  // USE_DEBUGGING_OUTPUT

// #ifdef SEQ64_JACK_SUPPORT

/**
 *  This function...
 */

void
jack_timebase_callback
(
    jack_transport_state_t state,
    jack_nframes_t nframes,
    jack_position_t * pos,
    int new_pos,
    void * arg
)
{
    static double jack_tick;
    static jack_nframes_t current_frame;
    static jack_transport_state_t state_current;
    static jack_transport_state_t state_last;

    state_current = state;

    // perform * p = (perform *) arg;

    jack_assistant * jack = (jack_assistant *) arg;

    current_frame = jack_get_current_transport_frame(jack->m_jack_client);
    pos->valid = JackPositionBBT;
    pos->beats_per_bar = 4;
    pos->beat_type = 4;
    pos->ticks_per_beat = c_ppqn * 10;
    pos->beats_per_minute = jack->parent().get_bpm();

    /*
     * Compute BBT info from frame number.  This is relatively simple
     * here, but would become complex if we supported tempo or time
     * signature changes at specific locations in the transport timeline.
     * If we are in a new position....
     */

    if (state_last == JackTransportStarting &&
            state_current == JackTransportRolling)
    {
        double jack_delta_tick =
            (current_frame) *
            pos->ticks_per_beat *
            pos->beats_per_minute / (pos->frame_rate * 60.0);

        jack_tick = (jack_delta_tick < 0) ? -jack_delta_tick : jack_delta_tick;

        long ptick = 0, pbeat = 0, pbar = 0;
        pbar  = (long)
        (
            (long) jack_tick / (pos->ticks_per_beat * pos->beats_per_bar)
        );
        pbeat = (long)
        (
            (long) jack_tick % (long)(pos->ticks_per_beat * pos->beats_per_bar)
        );
        pbeat /= (long) pos->ticks_per_beat;
        ptick = (long) jack_tick % (long) pos->ticks_per_beat;
        pos->bar = pbar + 1;
        pos->beat = pbeat + 1;
        pos->tick = ptick;
        pos->bar_start_tick = pos->bar * pos->beats_per_bar * pos->ticks_per_beat;
    }
    state_last = state_current;
}

/**
 *  Shutdown JACK by clearing the perform::m_jack_running flag.
 */

void
jack_shutdown (void * arg)
{
    // perform * p = (perform *) arg;

    jack_assistant * jack = (jack_assistant *) arg;

    jack->m_jack_running = false;
    printf("JACK shut down.\nJACK sync Disabled.\n");
}

/**
 *  Print the JACK position.
 */

void
print_jack_pos (jack_position_t * jack_pos)
{
    return;                                              /* tricky! */
    printf("print_jack_pos()\n");
    printf("    bar  [%d]\n", jack_pos->bar);
    printf("    beat [%d]\n", jack_pos->beat);
    printf("    tick [%d]\n", jack_pos->tick);
    printf("    bar_start_tick   [%f]\n", jack_pos->bar_start_tick);
    printf("    beats_per_bar    [%f]\n", jack_pos->beats_per_bar);
    printf("    beat_type        [%f]\n", jack_pos->beat_type);
    printf("    ticks_per_beat   [%f]\n", jack_pos->ticks_per_beat);
    printf("    beats_per_minute [%f]\n", jack_pos->beats_per_minute);
    printf("    frame_time       [%f]\n", jack_pos->frame_time);
    printf("    next_time        [%f]\n", jack_pos->next_time);
}

}           // namespace seq64

/*
 * jack_assistant.cpp
 *
 * vim: sw=4 ts=4 wm=8 et ft=cpp
 */
