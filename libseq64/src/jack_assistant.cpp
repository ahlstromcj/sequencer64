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
 * \updates       2016-01-18
 * \license       GNU GPLv2 or above
 *
 *  This module was created from code that existed in the perform object.
 *  Moving it into is own module makes it easier to maintain and makes the
 *  perform object a little easier to grok.
 *
 *  For the summaries of the JACK functions used in this module, and how
 *  the code is supposed to operate, see the Sequencer64 developer's reference
 *  manual.
 */

#include <stdio.h>

#include "jack_assistant.hpp"
#include "midifile.hpp"
#include "perform.hpp"

namespace seq64
{

#ifdef SEQ64_JACK_SUPPORT

/**
 *  This constructor initializes a number of member variables, some
 *  of them public!
 *
 *  Note that the perform object currently calls jack_assistant::init(), but
 *  that call could be made here instead.
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
 *  The destructor doesn't need to do anything yet.  The perform object
 *  currently calls jack_assistant::deinit(), but that call could be made here
 *  instead.
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
 *  Note the USE_JACK_SYNC_CALLBACK macro.  A sync callback is needed for
 *  polling of slow-sync clients.  But seq24/sequencer64 are not slow-sync
 *  clients.  Therefore, let's conditionally comment out the sync callback
 *  code.  One of the author's of JACK notes that seq24 is wrong to set up a
 *  sync callback.  CURRENTLY NOT COMMENTED OUT!!!
 *
 * Jack transport settings:
 *
 *      There are three settings:  On, Master, and Master Conditional.
 *      Currently, they can all be selected in the user-interface's File /
 *      Options / JACK/LASH page.  We really want only one to be set, for
 *      clarity.  They should be radio buttons.  We need to initialize if any
 *      of them are set.
 *
 * jack_set_process_callback() patch:
 *
 *      Implemented first patch from freddix/seq24 GitHub project, to fix JACK
 *      transport.  One line of code.  Well, we added some error-checking. :-)
 *      Found some old notes on the Web the this patch really only works (to
 *      prevent seq24 freeze) if seq24 is set as JACK Master, or if another
 *      client application, such as Qtractor, is running as JACK Master (and
 *      then seq24 will apparently follow it).
 *
 * \return
 *      Returns true if JACK is now considered to be running (or if it was
 *      already running.)
 */

bool
jack_assistant::init ()
{
    bool can_initialize =
    (
        rc().with_jack_transport() ||
        rc().with_jack_master() ||
        rc().with_jack_master_cond()
    );
    if (can_initialize && ! m_jack_running)
    {
        std::string package = SEQ64_PACKAGE;
        m_jack_running = true;              /* determined surely below      */
        m_jack_master = true;               /* ditto, too tricky, though    */
        m_jack_client = client_open(package);
        if (m_jack_client == NULL)
            return error_message("JACK server not running, JACK sync disabled");

        jack_on_shutdown(m_jack_client, jack_shutdown_callback, (void *) this);

#ifdef USE_JACK_SYNC_CALLBACK
        int jackcode = jack_set_sync_callback
        (
            m_jack_client, jack_sync_callback, (void *) this
        );
        if (jackcode != 0)
            return error_message("jack_set_sync_callback() failed");
#else
        /*
         * If we disable the sync callback, we don't get feedback re
         * the start and stop of playback.  Perhaps sync() needs to be
         * called additionally in start(), stop(), and in the process
         * callback?  Search for "EXPERIMENTAL".
         */

        int jackcode = jack_set_sync_callback(m_jack_client, NULL, NULL);
        if (jackcode != 0)
            return error_message("jack_set_sync_callback(NULL) failed");

        (void) sync();                  /* obtains some JACK numbers */
#endif

        /*
         * Although they say this code is needed to get JACK transport to work
         * properly, seq24 doesn't use this, so for now we comment this out
         * with a macro.
         */

        jackcode = jack_set_process_callback    /* see notes in banner */
        (
            m_jack_client, jack_process_callback, NULL  // (void *) this
        );
        if (jackcode != 0)
            return error_message("jack_set_process_callback() failed]");

        /*
         * Some possible code:
         *
         * jackcode = jack_set_xrun_callback
         * (
         *      m_jack_client, jack_xrun_callback, (void *) this
         * );
         */

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
        bool cond = rc().with_jack_master_cond();
        if (rc().with_jack_master() || cond)
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
            return error_message("Cannot activate as JACK client");
    }
    if (m_jack_running)
        (void) info_message("Initialized. JACK sync now enabled");
    else
        (void) error_message("Initialization error. JACK sync not enabled");

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
        if (m_jack_master)
        {
            m_jack_master = false;
            if (jack_release_timebase(m_jack_client) != 0)
                (void) error_message("Cannot release JACK timebase");
        }

        /*
         * New:  Simply to be symmetric with the startup flow.  Not yet sure
         * why jack_activate() was needed, but assume that jack_deactivate() is
         * thus important as well.  However, commented out since not in seq24.
         */

        if (jack_deactivate(m_jack_client) != 0)
            (void) error_message("Cannot deactivate JACK client");

        if (jack_client_close(m_jack_client) != 0)
            (void) error_message("Cannot close JACK client");

        m_jack_running = false;
    }
    if (! m_jack_running)
        (void) info_message("Deinitialized. JACK sync now disabled");
}

/**
 *  If JACK is supported, starts the JACK transport.  This function assumes
 *  that m_jack_client is not null, if m_jack_running is true.
 */

void
jack_assistant::start ()
{
    if (m_jack_running)
    {
        jack_transport_start(m_jack_client);

        /*
         * EXPERIMENTAL!!!!!
         * (void) sync();
         */
    }
    else
        (void) error_message("Transport Start: JACK not running!");
}

/**
 *  If JACK is supported, stops the JACK transport.  This function assumes
 *  that m_jack_client is not null, if m_jack_running is true.
 */

void
jack_assistant::stop ()
{
    if (m_jack_running)
    {
        jack_transport_stop(m_jack_client);

        /*
         * EXPERIMENTAL!!!!!
         * (void) sync();
         */
    }
    else
        (void) error_message("Transport Stop: JACK not running!");
}

/**
 *  If JACK is supported and running, sets the position of the transport to
 *  the new frame number, frame 0.  This new position takes effect in two
 *  process cycles. If there are slow-sync clients and the transport is
 *  already rolling, it will enter the JackTransportStarting state and begin
 *  invoking their sync_callbacks until ready. This function is realtime-safe.
 *
 *      http://jackaudio.org/files/docs/html/transport-design.html
 *
 *  This position() function is called via perform::position_jack() in the
 *  mainwnd, perfedit, perfroll, and seqroll graphical user-interface support
 *  objects.
 *
 *  The code that was disabled sets the current tick to 0 or, if state was
 *  true, to the leftmost tick (which is probably the position of the L
 *  marker).  The current tick is then converted to a frame number, and then
 *  we locate the transport to that position.  We're going to enable this
 *  code, but make it dependent on a new boolean parameter that defaults to
 *  false, in anticipation of trying it out later.
 *
 * \warning
 *      A lot of this code is effectively disabled by an early return
 *      statement.
 *
 * \param to_left_tick
 *      If true, the current tick is set to the leftmost tick, instead of the
 *      0th tick.  Now used, but only if relocate is true.
 *      One question is, do we want to perform this function if
 *      rc().with_jack_transport() is true?  Seems like we should be able to
 *      do it only if m_jack_master is true.
 *
 * \param relocate
 *      If true (it defaults to false), then we allow the relocation of the
 *      JACK transport to the current_tick or the left tick, rather than to
 *      frame 0..
 */

void
jack_assistant::position (bool to_left_tick, bool relocate )
{
    if (m_jack_running)
    {
        if (relocate)                           // false by default
        {
            jack_nframes_t rate = jack_get_sample_rate(m_jack_client);
            midipulse currenttick = 0;
            if (to_left_tick)
                currenttick = parent().get_left_tick();

            jack_position_t pos;
            pos.valid = JackPositionBBT;
            pos.beats_per_bar = 4;              // DEFAULT_BEATS_PER_MEASURE
            pos.beat_type = 4;                  // DEFAULT_BEAT_WIDTH
            pos.ticks_per_beat = m_ppqn * 10;
            pos.beats_per_minute = parent().get_beats_per_minute();

            /*
             * Compute BBT info from frame number.  This is relatively simple
             * here, but would become complex if we supported tempo or time
             * signature changes at specific locations in the transport timeline.
             */

            currenttick *= 10;
            pos.bar = int32_t
            (
                (currenttick / long(pos.ticks_per_beat) / pos.beats_per_bar)
            );
            pos.beat = int32_t(((currenttick / (long) pos.ticks_per_beat) % 4));
            pos.tick = int32_t((currenttick % (m_ppqn * 10)));
            pos.bar_start_tick = pos.bar * pos.beats_per_bar * pos.ticks_per_beat;
            pos.frame_rate = rate;
            pos.frame = (jack_nframes_t)
            (
                (currenttick * rate * 60.0) /
                    (pos.ticks_per_beat * pos.beats_per_minute)
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
        }
        else
        {
            if (jack_transport_locate(m_jack_client, 0) != 0)
                (void) info_message("jack_transport_locate() failed");
        }
    }
}

/**
 *  A helper function for syncing up with JACK parameters.  Sequencer64 is not
 *  a slow-sync client, so that callback is not really needed, but we probably
 *  need this sub-function here to start out with the right values for
 *  interacting with JACK.
 *
 *  Note the call to jack_transport_query().  This call is <i> not </i> is
 *  seq24, but seems to be needed in sequencer64 because we put m_jack_pos in
 *  the initializer list, which sets all its fields to 0.  Seq24 accesses
 *  m_jack_pos before it ever gets set, but its fields have values.  These
 *  values are bogus, but are consistent from run to run on my computer, and
 *  allow seq24 to follow another JACK Master, on some computers.  It explains
 *  why people had different experiences with JACK sync.
 *
 *  If we explicity call jack_transport_query() here, without changing the \a
 *  state parameter, then sequencer64 also can follow another JACK Master.
 */

int
jack_assistant::sync (jack_transport_state_t state)
{
    int result = 0;                     /* seq24 always returns 1   */
    m_jack_frame_current = jack_get_current_transport_frame(m_jack_client);
    (void) jack_transport_query(m_jack_client, &m_jack_pos);    // see banner
    if (m_jack_pos.frame_rate != 0)
    {
        result = 1;
        m_jack_tick = m_jack_frame_current * m_jack_pos.ticks_per_beat *
            m_jack_pos.beats_per_minute / (m_jack_pos.frame_rate * 60.0) ;
    }
    else
    {
        /*
         * The actual frame rate might be something like 48000.  Try to make
         * it work somehow, for now.
         */

        errprint("jack_assistant::sync(): zero frame rate");
        m_jack_tick = m_jack_frame_current * m_jack_pos.ticks_per_beat *
            m_jack_pos.beats_per_minute / (48000 * 60.0);
    }
    m_jack_frame_last = m_jack_frame_current;
    m_jack_transport_state_last = m_jack_transport_state = state;
    switch (state)
    {
    case JackTransportStopped:
        // infoprint("[JackTransportStopped]");
        break;

    case JackTransportRolling:
        // infoprint("[JackTransportRolling]");
        break;

    case JackTransportStarting:
        // infoprint("[JackTransportStarting]");
        parent().inner_start(rc().jack_start_mode());
        break;

    case JackTransportLooping:
        // infoprint("[JackTransportLooping]");
        break;

    default:
        break;
    }
    return result;
}

/*
 *  Implemented second patch for JACK Transport from freddix/seq24 GitHub
 *  project.  Added the following function.  This function is supposed to
 *  allow seq24/sequencer64 to follow JACK transport.
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
    /*
     * EXPERIMENTAL!!!!!
     * (void) sync();
     */

    return 0;
}

#ifdef USE_JACK_SYNC_CALLBACK

/**
 *  This JACK synchronization callback informs the specified perform
 *  object of the current state and parameters of JACK.
 *
 *  The transport state will be:
 *
 *      -   JackTransportStopped when a new position is requested.
 *      -   JackTransportStarting when the transport is waiting to start.
 *      -   JackTransportRolling when the timeout has expired, and the
 *          position is now a moving target.
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
    int result = jack->sync(state);     /* use the new member function */
    if (result == 1)
        print_jack_pos(pos);

    return 1;
}

#endif  // USE_JACK_SYNC_CALLBACK

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
 *  Why are we using a Glib::ustring here?  Convenience.  But with C++11, we
 *  could use a lexical_cast<>.  No more ustring, baby!  It doesn't really
 *  matter; this function can call Gtk::Main::quit(), via the parent's
 *  gui().quit() function.
 *
 * \return
 *      Always returns false.
 */

bool
jack_assistant::session_event ()
{
    if (not_nullptr(m_jsession_ev))
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
    }
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
 * \todo
 *      Follow up on this note found "out there":  "Maybe I'm wrong but if I
 *      understood correctly, recent jack1 transport no longer goes into
 *      Jack_Transport_Starting state before going to Jack_Transport_Rolling
 *      (this was deliberately dropped), but seq24 currently needs this to
 *      start off with jack transport."  On the other hand, some people have
 *      no issues.
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
        m_jack_transport_state = jack_transport_query(m_jack_client, &m_jack_pos);

        /*
         * TODO:  Check for m_jack_pos->valid field.  Check for
         *        m_jack_transport_state == JackTransportRolling?
         *        m_jack_transport_state == JackTransportStopped?
         */

        m_jack_frame_current =
            jack_get_current_transport_frame(m_jack_client);

        /*
         * TODO:
         *      Need to verify the m_jack_pos.frame_rate is > 0!
         *      Same for m_jack_pos.ticks_per_beat and m_jack_pos.beat_type.
         */

        bool ok = m_jack_pos.frame_rate != 0;           // > 0;
        if (! ok)
            info_message("jack_assistant::output(): zero frame rate");

        /*
         * Question:  Do we really need to check for the starting state here
         * before we move on?  Should we use an OR?
         */

        if
        (
            m_jack_transport_state_last == JackTransportStarting &&
            m_jack_transport_state == JackTransportRolling
        )
        {
            /*
             * (void) info_message("Start playback");
             */

            m_jack_frame_last = m_jack_frame_current;
            pad.js_dumping = true;
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
            /*
             * (void) info_message("Stop playback");
             */

            m_jack_transport_state_last = JackTransportStopped;
            pad.js_jack_stopped = true;
        }

        /*
         * Jack Transport is Rolling Now !!!  Transport is in a sane state if
         * dumping == true.
         */

        if (pad.js_dumping)
        {
            m_jack_frame_current =
                jack_get_current_transport_frame(m_jack_client);

            if (m_jack_frame_current > m_jack_frame_last)   /* moving ahead?  */
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
        "JackFailure, overall operation failed"
    },
    {
        JackInvalidOption,
        "JackInvalidOption, operation contained an invalid or unsupported option"
    },
    {
        JackNameNotUnique,
        "JackNameNotUnique, the client name was not unique"
    },
    {
        JackServerStarted,
        "JackServerStarted, JACK started by this operation, not running already"
    },
    {
        JackServerFailed,
        "JackServerFailed, unable to connect to the JACK server"
    },
    {
        JackServerError,
        "JackServerError, communication error with the JACK server"
    },
    {
        JackNoSuchClient,
        "JackNoSuchClient, requested client does not exist"
    },
    {
        JackLoadFailure,
        "JackLoadFailure, unable to load internal client"
    },
    {
        JackInitFailure,
        "JackInitFailure, unable to initialize client"
    },
    {
        JackShmFailure,
        "JackShmFailure, unable to access shared memory"
    },
    {
        JackVersionError,
        "JackVersionError, client's protocol version does not match"
    },
    {
        JackBackendError,
        "JackBackendError, a JACK back-end error occurred"
    },
    {
        JackClientZombie,
        "JackClientZombie, a JACK zombie process exists"
    },
    {                                   /* terminator */
        0, ""
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
    jack_status_t status;
    const char * name = clientname.c_str();

#ifdef SEQ64_JACK_SESSION
    if (rc().jack_session_uuid().empty())
    {
        result = jack_client_open(name, JackNullOption, &status);
    }
    else
    {
        const char * uuid = rc().jack_session_uuid().c_str();
        result = jack_client_open(name, JackSessionID, &status, uuid);
    }
#else
    result = jack_client_open(name, JackNullOption, &status);
#endif

    if (status & JackServerStarted)
        (void) info_message("JACK server started now");
    else
        (void) info_message("JACK server already started");

    if (status & JackNameNotUnique)
        (void) info_message("JACK client-name NOT unique");

    show_statuses(status);
    return result;
}

/**
 *  Another init() helper function to keep init() clean and easy to read.
 *
 * \return
 *      Returns true if the function succeeded.

bool
jack_assistant::
 */

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
    static bool s_set_state_last = true;
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
     * Compute BBT (Bar:Beats.ticks) info from frame number.  This is
     * relatively simple here, but would become complex if we supported tempo
     * or time signature changes at specific locations in the transport
     * timeline.  If we are in a new position....
     *
     * \new ca 2016-01-13
     *      Initialize the last state, if not already initialized.
     */

    if (s_set_state_last)
    {
        s_state_last = jack_transport_query(jack->m_jack_client, NULL);
        s_set_state_last = false;
    }
        
    /*
     * Question:  Do we really need to check for the starting state here
     * before we move on?  Should we use an OR?
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
        infoprint("jack_timebase_callback(): zero frame rate");
    }

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
jack_shutdown_callback (void * arg)
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

#endif  // SEQ64_JACK_SUPPORT

}           // namespace seq64

/*
 * jack_assistant.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

