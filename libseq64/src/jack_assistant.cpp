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
 * \updates       2016 01-12
 * \license       GNU GPLv2 or above
 *
 *  This module was created from code that existed in the perform object.
 *  Moving it into is own module makes it easier to maintain and makes the
 *  perform object a little easier to grok.
 *
 *  Here are summaries of the JACK functions used in this module:
 *
 *      -   jack_client_open(const char *, jack_options_t, jack_status_t *).
 *          Open a client session with a JACK server. More complex and
 *          powerful than jack_client_new(). Clients choose which of several
 *          servers to connect, and how to start the server automatically, if
 *          not already running. There is also an option for JACK to generate
 *          a unique client name.
 *      -   jack_on_shutdown().
 *          Registers a function to call when the JACK server shuts down the
 *          client thread. The function must be an asynchonrous POSIX signal
 *          handler: only async-safe functions, executed from another thread.
 *          A typical function might set a flag or write to a pipe so that the
 *          rest of the application knows that the JACK client thread has shut
 *          down.  Clients do not need to call this function. It only helps
 *          clients understand what is going on. It should be called before
 *          jack_client_activate().
 *      -   jack_set_sync_callback().
 *          Register/unregister as a slow-sync client, who cannot respond
 *          immediately to transport position changes.  The callback is run at
 *          the first opportunity after registration: if the client is active,
 *          this is the next process cycle, otherwise it is the first cycle
 *          after jack_activate().  After that, it runs as per
 *          JackSyncCallback rules.  Clients that don't set this callback are
 *          assumed ready immediately any time the transport wants to start.
 *      -   jack_set_process_callback().
 *          Tells the JACK server to call the callback whenever there is work.
 *          The function must be suitable for real-time execution, it cannot
 *          call functions that might block for a long time: malloc(), free(),
 *          printf(), pthread_mutex_lock(), sleep(), wait(), poll(), select(),
 *          pthread_join(), pthread_cond_wait(), etc.  In the current class,
 *          this function is a do-nothing function.
 *      -   jack_set_session_callback().
 *          Tells the JACK server to call the callback when a session event is
 *          delivered.  Setting more than one session callback per process is
 *          probably a design error.  For a multiclient application, it's more
 *          sensible to create a JACK client with only one session callback.
 *      -   jack_activate().
 *          Tells the JACK server that the application is ready to start
 *          processing.
 *      -   jack_release_timebase().
 *      -   jack_client_close().
 *      -   jack_transport_start().
 *          Starts the JACK transport rolling.  Any client can make this
 *          request at any time. It takes effect no sooner than the next
 *          process cycle, perhaps later if there are slow-sync clients. This
 *          function is realtime-safe.  No return code.
 *      -   jack_transport_stop().
 *          Starts the JACK transport rolling.  Any client can make this
 *          request at any time.  This function is realtime-safe.  No return
 *          code.
 *      -   jack_transport_locate().
 *          Repositions the transport to a new frame number.  May be called at
 *          any time by any client. The new position takes effect in two
 *          process cycles. If there are slow-sync clients and the transport is
 *          already rolling, it will enter the JackTransportStarting state and
 *          begin invoking their sync_callbacks until ready. This function is
 *          realtime-safe.
 *      -   jack_transport_reposition().
 *          Request a new transport position.  May be called at any time by any
 *          client. The new position takes effect in two process cycles. If
 *          there are slow-sync clients and the transport is already rolling,
 *          it will enter the JackTransportStarting state and begin invoking
 *          their sync_callbacks until ready. This function is realtime-safe.
 *          This call, made in the position() function, is currently disabled.
 *
 */

#include <stdio.h>

#include "jack_assistant.hpp"
#include "midifile.hpp"
#include "perform.hpp"

namespace seq64
{

/**
 *  This constructor initializes a number of member variables, some
 *  of them public!
 *
 * \param parent
 *      Provides a reference to the main perform object that needs to
 *      control JACK event.
 */

jack_assistant::jack_assistant (perform & parent, int ppqn)
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
    m_jack_master               (false),
    m_ppqn                      (0)
{
    m_ppqn = choose_ppqn(ppqn);
}

/**
 *  The destructor doesn't need to do anything yet.
 */

jack_assistant::~jack_assistant ()
{
    /*
     * Anything to do?  Call deinit()?
     */
}

/**
 *  Common-code for console messages.  Adds markers and a newline.
 *
 * \param msg
 *      The message to print, sans the newline.
 *
 * \return
 *      Returns true.
 */

bool
jack_assistant::info_message (const std::string & msg)
{
    std::string temp = "[";
    temp += msg;
    temp += "]\n";
    printf(temp.c_str());
    return true;
}

/**
 *  Common-code for error messages.  Adds markers, and sets m_jack_running to
 *  false.
 *
 * \param msg
 *      The message to print, sans the newline.
 *
 * \return
 *      Returns false for convenience/brevity in setting function return
 *      values.
 */

bool
jack_assistant::error_message (const std::string & msg)
{
    (void) info_message(msg);
    m_jack_running = false;
    return false;
}

/**
 *  Initializes JACK support.  Then we become a new client of the JACK server.
 *
 * \return
 *      Returns true if JACK is now considered to be running (or if it was
 *      already running.)
 */

bool
jack_assistant::init ()
{
    if (rc().with_jack_transport() && ! m_jack_running)
    {
        std::string package = SEQ64_PACKAGE;
        m_jack_running = true;              /* determined surely below      */
        m_jack_master = true;               /* ditto, too tricky, though    */
        m_jack_client = client_open(package);
        if (m_jack_client == NULL)
            return error_message("JACK server not running, JACK sync disabled");

        jack_on_shutdown(m_jack_client, jack_shutdown, (void *) this);
        int jackcode = jack_set_sync_callback
        (
            m_jack_client, jack_sync_callback, (void *) this
        );
        if (jackcode != 0)
            return error_message("jack_set_sync_callback() failed");

        /*
         * Implemented first patch from freddix/seq24 GitHub project, to fix
         * JACK transport.  One line of code.  Well, we added some
         * error-checking. :-)
         */

        jackcode = jack_set_process_callback
        (
            m_jack_client, jack_process_callback, NULL
        );
        if (jackcode != 0)
            return error_message("jack_set_process_callback() failed]");

#ifdef SEQ64_JACK_SESSION
        if (jack_set_session_callback)
        {
            jackcode = jack_set_session_callback
            (
                m_jack_client, jack_session_callback, (void *) this
            );
            if (jackcode != 0)
                return error_message("jack_set_session_callback() failed]");
        }
#endif

        bool master_is_set = false;         /* flag to handle trickery  */
        if (rc().with_jack_master())
        {
            /*
             * 'cond' is true if we want to fail if there is already a JACK
             * master, i.e. it is a conditional attempt to be JACK master.
             *
             * \change ca 2016-01-12
             *      Found an error; we don't want to pass a perform object,
             *      we want a jack_assistant object.
             *
             *      (void *) &m_jack_parent
             */

            bool cond = rc().with_jack_master_cond();
            jackcode = jack_set_timebase_callback
            (
                m_jack_client, cond, jack_timebase_callback, (void *) this
            );
            if (jackcode == 0)
            {
                (void) info_message("JACK transport master");
                m_jack_master = true;
                master_is_set = true;
            }
            else
            {
                /*
                 * seq24 doesn't set this here: m_jack_master = false;
                 */

                return error_message("jack_set_timebase_callback() failed");
            }
        }
        if (! master_is_set)
        {
            (void) info_message("JACK transport slave");
            m_jack_master = false;
        }
        if (jack_activate(m_jack_client) != 0)
            return error_message("Cannot register as JACK client");
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
        if (jack_release_timebase(m_jack_client) != 0)
            (void) error_message("Cannot release timebase");

        if (jack_client_close(m_jack_client) != 0)
            (void) error_message("Cannot close JACK client");
    }

    /*
     * No need for this message.  We are likely exiting the application.
     *
     * if (! m_jack_running)
     *     (void) info_message("JACK sync disabled");
     */
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
 *  If JACK is supported, stops the JACK transport.  Should it also set
 *  m_jack_running to false?  Let's do that, just for consistency.
 */

void
jack_assistant::stop ()
{
    if (m_jack_running)
    {
        jack_transport_stop(m_jack_client);
        m_jack_running = false;             /* \change ca 2015-01-10 */
    }
}

/**
 *  If JACK is supported and running, sets the position of the transport.
 *
 *      http://jackaudio.org/files/docs/html/transport-design.html
 *
 *  This function is called via perform::position_jack() in the mainwnd,
 *  perfedit, perfroll, and seqroll graphical user-interface support objects.
 *
 * \warning
 *      A lot of this code is effectively disabled by an early return
 *      statement.
 *
 * \param state
 *      If true, the current tick is set to the leftmost tick.
 */

void
jack_assistant::position (bool /* state */ )
{
    if (m_jack_running)
    {
        if (jack_transport_locate(m_jack_client, 0) != 0)
            (void) info_message("jack_transport_locate() failed");
    }
    return;

#ifdef WHY_IS_THIS_CODE_EFFECTIVELY_DISABLED

    /*
     * Probably because it is hardwired.  We probably should fix this up and
     * somehow make Sequencer64 support JACK fully.
     */

    jack_nframes_t rate = jack_get_sample_rate(m_jack_client);
    midipulse currenttick = 0;
    if (state)
        currenttick = m_left_tick;

    jack_position_t pos;
    pos.valid = JackPositionBBT;
    pos.beats_per_bar = 4;              // DEFAULT_BEATS_PER_MEASURE
    pos.beat_type = 4;                  // DEFAULT_BEAT_WIDTH
    pos.ticks_per_beat = m_ppqn * 10;
    pos.beats_per_minute = m_master_bus.get_beats_per_minute();

    /*
     * Compute BBT info from frame number.  This is relatively simple
     * here, but would become complex if we supported tempo or time
     * signature changes at specific locations in the transport timeline.
     */

    currenttick *= 10;
    pos.bar = int32_t
    (
        (currenttick / long(pos.ticks_per_beat) / pos.beats_per_bar);
    );

    pos.beat = int32_t(((currenttick / (long) pos.ticks_per_beat) % 4));
    pos.tick = int32_t((currenttick % (m_ppqn * 10)));
    pos.bar_start_tick = pos.bar * pos.beats_per_bar * pos.ticks_per_beat;
    pos.frame_rate = rate;
    pos.frame = (jack_nframes_t)
    (
        (currenttick * rate * 60.0) / (pos.ticks_per_beat * pos.beats_per_minute)
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
 *
 * \param nframes
 *      Unused.
 *
 * \param arg
 *      Unused.
 */

int
jack_process_callback (jack_nframes_t /* nframes */, void * /* arg */ )
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
 *      The pointer to the jack_assistant object.  Currently not checked for
 *      nullity, nor dynamic-casted.
 *
 * \return
 *      Returns 1 if the function works, and 0 if something was wrong.
 */

int
jack_sync_callback
(
    jack_transport_state_t state,
    jack_position_t * pos,
    void * arg
)
{
    jack_assistant * jack = (jack_assistant *)(arg);
    if (is_nullptr(jack))
    {
        errprint("jack_sync_callback(): null JACK pointer");
        return 0;
    }
    jack->m_jack_frame_current =
        jack_get_current_transport_frame(jack->m_jack_client);

    if (jack->m_jack_pos.frame_rate != 0)
    {
        jack->m_jack_tick = jack->m_jack_frame_current *
            jack->m_jack_pos.ticks_per_beat *
            jack->m_jack_pos.beats_per_minute /
            (jack->m_jack_pos.frame_rate * 60.0) ;

        jack->m_jack_frame_last = jack->m_jack_frame_current;
        jack->m_jack_transport_state_last = jack->m_jack_transport_state = state;
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
            jack->parent().inner_start(rc().jack_start_mode());
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
    else
    {
        static bool s_report_it = true;
        if (s_report_it)
        {
            errprint("jack_sync_callback(): zero frame rate [reported once]");
            s_report_it = false;
        }
        return 0;
    }
}

#ifdef SEQ64_JACK_SESSION

/**
 *  Writes the MIDI file named "<jack session dir>-file.mid" using a
 *  midifile object, quits if told to by JACK, and can free the JACK
 *  session event.
 *
 *  ca 2015-07-24
 *  Just a note:  The OMA (OpenMandrivaAssociation) patch was already
 *  applied to seq24 v.0.9.2.  It put quotes around the --file argument.
 *  However, the --file option doesn't work, so let's change that line.
 *
 *      sequencer64 --file \"${SESSION_DIR}file.mid\" --jack_session_uuid 
 *
 *  Why are we using a Glib::ustring here?  Convenience.  But with
 *  C++11, we could use a lexical_cast<>.  No more ustring, baby!
 *
 *  It doesn't really matter; this function can call Gtk::Main::quit(), via
 *  the parent's gui().quit() function.
 *
 * \return
 *      Always returns false.
 */

bool
jack_assistant::session_event ()
{
    std::string fname(m_jsession_ev->session_dir);
    fname += "file.mid";
    std::string cmd("sequencer64 --jack_session_uuid ");
    cmd += m_jsession_ev->client_uuid;
    cmd += " \"${SESSION_DIR}file.mid\"";

    midifile f(fname, rc().legacy_format(), usr().global_seq_feature());
    f.write(m_jack_parent);
    m_jsession_ev->command_line = strdup(cmd.c_str());
    jack_session_reply(m_jack_client, m_jsession_ev);
     if (m_jsession_ev->type == JackSessionSaveAndQuit)
        m_jack_parent.gui().quit();

    jack_session_event_free(m_jsession_ev);
    return false;
}

/**
 *  Set the m_jsession_ev (event) value of the perform object.
 *
 *  Glib is then used to connect in perform::jack_session_event().  However,
 *  the perform object's GUI-support interface is used instead of the
 *  following, so that the libseq64 library can be independent of a specific
 *  GUI framework:
 *
 *      Glib::signal_idle().
 *          connect(sigc::mem_fun(*jack, &jack_assistant::session_event));
 *
 * \param ev
 *      The JACK event to be set.
 *
 * \param arg
 *      The pointer to the jack_assistant object.  Currently not checked
 *      for nullity.
 */

void
jack_session_callback (jack_session_event_t * ev, void * arg)
{
    jack_assistant * jack = (jack_assistant *)(arg);
    jack->m_jsession_ev = ev;
    jack->parent().gui().jack_idle_connect(*jack);      // see note above
}

#endif  // SEQ64_JACK_SESSION

/**
 *  Performance output function for JACK, called by the perform function
 *  of the same name.
 *
 *  This code comes from perform::output_func() from seq24.
 *
 * \param pad
 *      Provide a JACK scratchpad, whatever that is.
 *
 * \return
 *      Returns true if JACK is running.
 */

bool
jack_assistant::output (jack_scratchpad & pad)
{
    if (m_jack_running)
    {
        double jack_ticks_converted = 0.0;
        double jack_ticks_converted_last = 0.0;
        double jack_ticks_delta = 0.0;
        pad.js_init_clock = false;      // no init until we get a good lock
        m_jack_transport_state =
            jack_transport_query(m_jack_client, &m_jack_pos);

        m_jack_frame_current =
            jack_get_current_transport_frame(m_jack_client);

        /*
         * TODO:
         *      Need to verify the m_jack_pos.frame_rate is > 0!
         *      Same for m_jack_pos.ticks_per_beat and m_jack_pos.beat_type.
         */

        bool ok = m_jack_pos.frame_rate > 0;
        if (! ok)
        {
            static bool s_report_it = true;
            if (s_report_it)
            {
                (void) info_message("jack output(): zero frame rate");
                s_report_it = false;
            }
        }
        if
        (
            m_jack_transport_state_last == JackTransportStarting &&
            m_jack_transport_state == JackTransportRolling
        )
        {
            m_jack_frame_last = m_jack_frame_current;
            pad.js_dumping = true;
            (void) info_message("Start playback");
            m_jack_tick = m_jack_pos.frame * m_jack_pos.ticks_per_beat *
                m_jack_pos.beats_per_minute / (m_jack_pos.frame_rate * 60.0);

            jack_ticks_converted = m_jack_tick *        /* convert ticks */
            (
                double(m_ppqn) / (m_jack_pos.ticks_per_beat *
                    m_jack_pos.beat_type / 4.0)         /* why 4?        */
            );
            m_jack_parent.set_orig_ticks(long(jack_ticks_converted));
            pad.js_current_tick = pad.js_clock_tick = pad.js_total_tick =
                jack_ticks_converted_last = jack_ticks_converted;

            pad.js_init_clock = true;

            /*
             * We need to make sure another thread can't modify these
             * values.  Also, maybe some of the parent (perform) values need to
             * move, to the scratch-pad, if not used directly in the perform
             * object.  Why the "double" value?
             */

            if (pad.js_looping && pad.js_playback_mode)
            {
                if (pad.js_current_tick >= m_jack_parent.get_right_tick())
                {
                    while (pad.js_current_tick >= m_jack_parent.get_right_tick())
                    {
                        double size = m_jack_parent.get_right_tick() -
                            m_jack_parent.get_left_tick();

                        pad.js_current_tick -= - size;
                    }
                    m_jack_parent.reset_sequences();
                    m_jack_parent.set_orig_ticks(long(pad.js_current_tick));
                }
            }
        }
        if
        (
            m_jack_transport_state_last == JackTransportRolling &&
            m_jack_transport_state == JackTransportStopped
        )
        {
            m_jack_transport_state_last = JackTransportStopped;
            pad.js_jack_stopped = true;
            (void) info_message("Stop playback");
        }

        /*
         * Jack Transport is Rolling Now !!!
         * Transport is in a sane state if dumping == true.
         */

        if (pad.js_dumping)
        {
            m_jack_frame_current = jack_get_current_transport_frame(m_jack_client);

            /* if we are moving ahead... */

            if (m_jack_frame_current > m_jack_frame_last)
            {
                m_jack_tick +=
                    (m_jack_frame_current - m_jack_frame_last)  *
                    m_jack_pos.ticks_per_beat *
                    m_jack_pos.beats_per_minute /
                    (m_jack_pos.frame_rate * 60.0);

                m_jack_frame_last = m_jack_frame_current;
            }
            jack_ticks_converted =      /* convert ticks            */
                m_jack_tick *
                (
                    double(m_ppqn) /
                        (m_jack_pos.ticks_per_beat * m_jack_pos.beat_type / 4.0)
                );

            jack_ticks_delta = jack_ticks_converted - jack_ticks_converted_last;
            pad.js_clock_tick += jack_ticks_delta;
            pad.js_current_tick += jack_ticks_delta;
            pad.js_total_tick += jack_ticks_delta;
            m_jack_transport_state_last = m_jack_transport_state;
            jack_ticks_converted_last = jack_ticks_converted;

#ifdef SEQ64_USE_DEBUG_OUTPUT
            jack_debug_print(pad.js_current_tick, jack_ticks_delta);
            long ptick, pbeat, pbar;
            pbar  = (long) ((long) m_jack_tick /
                    (m_jack_pos.ticks_per_beat *  m_jack_pos.beats_per_bar ));

            pbeat = (long) ((long) m_jack_tick %
                    (long) (m_jack_pos.ticks_per_beat *  m_jack_pos.beats_per_bar ));
            pbeat = pbeat / (long) m_jack_pos.ticks_per_beat;
            ptick = (long) m_jack_tick % (long) m_jack_pos.ticks_per_beat;
#endif

        }                               /* if dumping (sane state)  */
    }                                   /* if m_jack_running        */
    return m_jack_running;
}

/**
 *  Provides a list of JACK status bits, and a brief string to explain the
 *  status bit.  Terminated by a 0 value and an empty string.
 */

jack_status_pair_t jack_assistant::sm_status_pairs [] =
{
    {
        JackFailure,
        "Overall operation failed."
    },
    {
        JackInvalidOption,
        "The operation contained an invalid or unsupported option."
    },
    {
        JackNameNotUnique,
        "The client name was not unique."
    },
    {
        JackServerStarted,
        "JACK started by this operation, rather than running already."
    },
    {
        JackServerFailed,
        "Unable to connect to the JACK server."
    },
    {
        JackServerError,
        "Communication error with the JACK server."
    },
    {
        JackNoSuchClient,
        "Requested client does not exist."
    },
    {
        JackLoadFailure,
        "Unable to load internal client."
    },
    {
        JackInitFailure,
        "Unable to initialize client."
    },
    {
        JackShmFailure,
        "Unable to access shared memory."
    },
    {
        JackVersionError,
        "Client's protocol version does not match."
    },
    {
        JackBackendError,
        "A JACK back-end error occurred."
    },
    {
        JackClientZombie,
        "A JACK zombie process exists."
    },
    {                                   /* terminator */
        0,
        ""
    }
};

/**
 *  Loops through the full set of JACK bits, showing the information for any
 *  bits that are set in the given parameter.
 */

void
jack_assistant::show_statuses (unsigned bits)
{
    /*
     * infoprintf("JACK status bits returned = 0x%x\n", bits);
     */

    jack_status_pair_t * jsp = &sm_status_pairs[0];
    while (jsp->jf_bit != 0)
    {
        /*
         * infoprintf("Status bit = 0x%x\n", jsp->jf_bit);
         */

        if (bits & jsp->jf_bit)
            (void) info_message(jsp->jf_meaning.c_str());

        ++jsp;
    }
}

/**
 *  A more full-featured initialization for a JACK client, which is meant to
 *  be called by the init() function.
 *
 * Status bits for jack_status_t return pointer:
 *
 *      JackNameNotUnique means that the client name was not unique. With
 *      JackUseExactName, this is fatal. Otherwise, the name was modified by
 *      appending a dash and a two-digit number in the range "-01" to "-99".
 *      The jack_get_client_name() function returns the exact string used. If
 *      the specified client_name plus these extra characters would be too
 *      long, the open fails instead.
 *
 *      JackServerStarted means that the JACK server was started as a result
 *      of this operation. Otherwise, it was running already. In either case
 *      the caller is now connected to jackd, so there is no race condition.
 *      When the server shuts down, the client will find out.
 *
 * \return
 *      Returns true if JACK ...
 */

jack_client_t *
jack_assistant::client_open (const std::string & clientname)
{
    jack_client_t * result = nullptr;
    jack_status_t status_code;

#ifdef SEQ64_JACK_SESSION
    if (rc().jack_session_uuid().empty())
    {
        result = jack_client_open
        (
            clientname.c_str(), JackNullOption, &status_code
        );
    }
    else
    {
        result = jack_client_open
        (
            clientname.c_str(), JackSessionID, &status_code,
            rc().jack_session_uuid().c_str()
        );
    }
#else
    result = jack_client_open
    (
        clientname.c_str(), JackNullOption, &status_code
    );
#endif

    show_statuses(status_code);
    return result;
}

#ifdef SEQ64_USE_DEBUG_OUTPUT

/**
 *  Debugging code for JACK.
 */

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
        m_jack_pos.tick
        ;
    long pbar = long
    (
        long(m_jack_tick) /
        long(m_jack_pos.ticks_per_beat * m_jack_pos.beats_per_bar)
    );
    long pbeat = long
    (
        long(m_jack_tick) %
        long(m_jack_pos.ticks_per_beat * m_jack_pos.beats_per_bar)
    );
    pbeat /= long(m_jack_pos.ticks_per_beat);
    long ptick = long(m_jack_tick) % long(m_jack_pos.ticks_per_beat);
    printf
    (
        "* current_tick[%f] delta[%f]"
        "* bbb [%2ld:%2ld:%4ld] "
        "* jjj [%2d:%2d:%4d] "
        "* jtick[%8.3f] mtick[%8.3f] delta[%8.3f]\n"
        ,
        current_tick, ticks_delta,
        pbar+1, pbeat+1, ptick,
        m_jack_pos.bar, m_jack_pos.beat, m_jack_pos.tick,
        m_jack_tick, jack_tick, m_jack_tick-jack_tick
    );
}

#endif  // SEQ64_USE_DEBUG_OUTPUT

/**
 *  This function sets the JACK position structure.
 *
 * \param state
 *      Indicates the current state of JACK transport.
 *
 * \param nframes
 *      The number of JACK frames.
 *
 * \param pos
 *      Provides the position structure to be filled in.
 *
 * \param new_pos
 *      The new positions to be set.
 *
 * \param arg
 *      Provides the jack_assistant pointer, currently unchecked for nullity.
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
    static double s_jack_tick;
    static jack_nframes_t s_last_frame;
    static jack_nframes_t s_current_frame;
    static jack_transport_state_t s_state_last;
    static jack_transport_state_t s_state_current;
    jack_assistant * jack = (jack_assistant *)(arg);
    s_state_current = state;

    s_current_frame = jack_get_current_transport_frame(jack->m_jack_client);
    if (is_nullptr(pos))
    {
        errprint("jack_timebase_callback(): null position pointer");
        return;
    }
    pos->valid = JackPositionBBT;
    pos->beats_per_bar = 4;                     // hardwired!
    pos->beat_type = 4;                         // hardwired!
    pos->ticks_per_beat = jack->m_ppqn * 10;    // why 10?
    pos->beats_per_minute = jack->parent().get_beats_per_minute();

    /*
     * Compute BBT info from frame number.  This is relatively simple
     * here, but would become complex if we supported tempo or time
     * signature changes at specific locations in the transport timeline.
     * If we are in a new position....
     */

    if
    (
        s_state_last == JackTransportStarting &&
        s_state_current == JackTransportRolling
    )
    {
        s_jack_tick = 0.0;
        s_last_frame = s_current_frame;
    }

    if (pos->frame_rate > 0)
    {
        if (s_current_frame > s_last_frame)
        {
            double jack_delta_tick = (s_current_frame - s_last_frame) *
                pos->ticks_per_beat * pos->beats_per_minute /
                (pos->frame_rate * 60.0);

            s_jack_tick += jack_delta_tick;
            s_last_frame = s_current_frame;
        }
    }
    else
    {
        static bool s_report_it = true;
        if (s_report_it)
        {
            errprint("jack_timebase_callback(): zero frame rate");
            s_report_it = false;
        }
    }

//      double jack_delta_tick = (s_current_frame) * pos->ticks_per_beat *
//          pos->beats_per_minute / (pos->frame_rate * 60.0);
//      s_jack_tick = (jack_delta_tick < 0) ? -jack_delta_tick : jack_delta_tick;

    long ptick = 0, pbeat = 0, pbar = 0;
    long ticks_per_bar = long(pos->ticks_per_beat * pos->beats_per_bar);
    if (ticks_per_bar > 0)
    {
        pbar = long(long(s_jack_tick) / ticks_per_bar);
        pbeat = long(long(s_jack_tick) % ticks_per_bar);
        pbeat /= long(pos->ticks_per_beat);
        ptick = long(s_jack_tick) % long(pos->ticks_per_beat);
        pos->bar = pbar + 1;
        pos->beat = pbeat + 1;
        pos->tick = ptick;
        pos->bar_start_tick = pos->bar * pos->beats_per_bar *
            pos->ticks_per_beat;
    }
    else
    {
        errprint("jack_timebase_callback(): zero values");
    }

    s_state_last = s_state_current;

}

/**
 *  This callback is to shutdown JACK by clearing the
 *  jack_assistant::m_jack_running flag.
 *
 * \param arg
 *      Points to the jack_assistant in charge of JACK support for the perform
 *      object.
 */

void
jack_shutdown (void * arg)
{
    jack_assistant * jack = (jack_assistant *)(arg);
    jack->m_jack_running = false;
    infoprint("[JACK shutdown]");
}

/**
 *  Print the JACK position.
 *
 * \param pos
 *      The JACK position to print.
 */

void
print_jack_pos (jack_position_t * pos)
{
    return;                                              /* tricky! */
    printf
    (
        "print_jack_pos()\n"
        "    bar              [%d]\n"
        "    beat             [%d]\n"
        "    tick             [%d]\n"
        "    bar_start_tick   [%f]\n"
        "    beats_per_bar    [%f]\n"
        "    beat_type        [%f]\n"
        "    ticks_per_beat   [%f]\n"
        "    beats_per_minute [%f]\n"
        "    frame_time       [%f]\n"
        "    next_time        [%f]\n",
        pos->bar, pos->beat, pos->tick, pos->bar_start_tick, pos->beats_per_bar,
        pos->beat_type, pos->ticks_per_beat, pos->beats_per_minute,
        pos->frame_time, pos->next_time
    );
}

}           // namespace seq64

/*
 * jack_assistant.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

