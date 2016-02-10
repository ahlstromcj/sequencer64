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
 * \updates       2016-02-09
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

jack_assistant::jack_assistant
(
    perform & parent,
    int bpminute,
    int ppqn,
    int bpm,
    int beatwidth
) :
    m_jack_parent               (parent),
    m_jack_client               (nullptr),
    m_jack_frame_current        (0),
    m_jack_frame_last           (0),
    m_jack_pos                  (),
    m_jack_transport_state      (JackTransportStopped),
    m_jack_transport_state_last (JackTransportStopped),
    m_jack_tick                 (0.0),
#ifdef SEQ64_JACK_SESSION
    m_jsession_ev               (nullptr),
#endif
    m_jack_running              (false),
    m_jack_master               (false),
    m_ppqn                      (0),
    m_beats_per_measure         (bpm),
    m_beat_width                (beatwidth),
    m_beats_per_minute          (bpminute)
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
 *  A sync callback is needed for polling of slow-sync clients.  But
 *  seq24/sequencer64 are not slow-sync clients.  We don't really need to be a
 *  slow-sync client, as far as we can tell.  We can't get JACK working
 *  exactly the way it does in seq24 without the callback in place.  Plus, it
 *  does things important to the setup of JACK.  So now this setup is
 *  permanent.
 *
 * Jack transport settings:
 *
 *      There are three settings:  On, Master, and Master Conditional.
 *      Currently, they can all be selected in the user-interface's File /
 *      Options / JACK/LASH page.  We really want only the proper combinations
 *      to be set, for clarity (the user-interface now takes care of this.  We
 *      need to initialize if any of them are set, and the
 *      rc_settings::with_jack() function tells us that.
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
    if (rc().with_jack() && ! m_jack_running)
    {
        std::string package = SEQ64_PACKAGE;
        m_jack_running = true;              /* determined surely below      */
        m_jack_master = true;               /* ditto, too tricky, though    */
        m_jack_client = client_open(package);
        if (m_jack_client == NULL)
            return error_message("JACK server not running, JACK sync disabled");

        jack_on_shutdown(m_jack_client, jack_shutdown_callback, (void *) this);

        int jackcode = jack_set_sync_callback
        (
            m_jack_client, jack_sync_callback, (void *) this
        );
        if (jackcode != 0)
            return error_message("jack_set_sync_callback() failed");

        /*
         * Although they say this code is needed to get JACK transport to work
         * properly, seq24 doesn't use this.  But it doesn't hurt to set it up.
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
                 * seq24 doesn't set this flag, but that seems incorrect.
                 */

                m_jack_master = false;
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

        if (m_jack_running)
            (void) info_message("JACK sync now enabled");
        else
            (void) error_message("Initialization error, JACK sync not enabled");
    }
    else
    {
        if (m_jack_running)
            (void) info_message("JACK sync already enabled!");
        else
            (void) info_message("Initialized, Running without JACK");
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
        if (m_jack_master)
        {
            m_jack_master = false;
            if (jack_release_timebase(m_jack_client) != 0)
                (void) error_message("Cannot release JACK timebase");
        }

        /*
         * New:  Simply to be symmetric with the startup flow.  Not yet sure
         * why jack_activate() was needed, but assume that jack_deactivate() is
         * thus important as well.
         */

        if (jack_deactivate(m_jack_client) != 0)
            (void) error_message("Cannot deactivate JACK client");

        if (jack_client_close(m_jack_client) != 0)
            (void) error_message("Cannot close JACK client");
    }
    if (! m_jack_running)
        (void) info_message("JACK sync now disabled");
}

/**
 *  If JACK is supported, starts the JACK transport.  This function assumes
 *  that m_jack_client is not null, if m_jack_running is true.
 *
 *  Found this note in the Hydrogen code:
 *
 *      When jack_transport_start() is called, it takes effect from the next
 *      processing cycle.  The location info from the timebase_master, if
 *      there is one, will not be available until the _next_ next cycle.  The
 *      code must therefore wait one cycle before syncing up with
 *      timebase_master.
 */

void
jack_assistant::start ()
{
    if (m_jack_running)
        jack_transport_start(m_jack_client);
    else if (rc().with_jack())
        (void) error_message("Transport Start: JACK not running");
}

/**
 *  If JACK is supported, stops the JACK transport.  This function assumes
 *  that m_jack_client is not null, if m_jack_running is true.
 */

void
jack_assistant::stop ()
{
    if (m_jack_running)
        jack_transport_stop(m_jack_client);
    else if (rc().with_jack())
        (void) error_message("Transport Stop: JACK not running");
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
 *  These repositions reset the progress bars.  We don't always want that, it
 *  should be an option.  TODO.
 *
 * jack_transport_reposition():
 *
 *      Requests a new transport position.  The new position takes effect in
 *      two process cycles. If there are slow-sync clients and the transport
 *      is already rolling, it will enter the JackTransportStarting state and
 *      begin invoking their sync_callbacks until ready. This function is
 *      realtime-safe.
 *
 *      It's pos parameter provides the requested new transport position. Fill
 *      pos->valid to specify which fields should be taken into account. If
 *      you mark a set of fields as valid, you are expected to fill them all.
 *      Note that "frame" is always assumed, and generally needs to be set:
 *
 *         http://comments.gmane.org/gmane.comp.audio.jackit/18705
 *
 *      Returns 0 if a valid request, EINVAL if the position structure is
 *      rejected.
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
 *      frame 0.  EXPERIMENTAL, enables dead code from seq24.  Seems to work
 *      if set to true when we are the JACK Master.  Enabling this code makes
 *      "klick -j -P" work, after a fashion.  It clicks, but at a way too
 *      rapid rate.
 */

void
jack_assistant::position (bool to_left_tick, bool relocate)
{
    if (m_jack_running)
    {
        if (relocate)                           // false by default
        {
            /*
             * This seems to be needed to prevent klick from aborting.
             * Otherwise, it has no effect on klick.
             */

            midipulse currenttick = 0;
            if (to_left_tick)
                currenttick = parent().get_left_tick();

            set_position(currenttick);          // doesn't quite work
        }
        else
        {
            if (jack_transport_locate(m_jack_client, 0) != 0)
                (void) info_message("jack_transport_locate() failed");
        }
    }
}

/**
 *  Provides the code that was effectively commented out in the
 *  perform::position_jack() function.  We might be able to use it in other
 *  functions.
 *
 *  Computing the  BBT information from the frame number is relatively simple
 *  here, but would become complex if we supported tempo or time signature
 *  changes at specific locations in the transport timeline.
 *
 \verbatim
        ticks * 10 = jack ticks;
        jack ticks / ticks per beat = num beats;
        num beats / beats per minute = num minutes
        num minutes * 60 = num seconds
        num secords * frame_rate  = frame
 \endverbatim
 */

void
jack_assistant::set_position (midipulse currenttick)
{
    jack_nframes_t rate = jack_get_sample_rate(m_jack_client);
    jack_position_t pos;

    /*
     * Sufficient to edit all the fields we want????
     */

    pos.valid = JackPositionBBT;        // flags what will be modified here

    pos.beats_per_bar = m_beats_per_measure;
    pos.beat_type = m_beat_width;
    pos.ticks_per_beat = m_ppqn * 10;
    pos.beats_per_minute = parent().get_beats_per_minute();

    /*
     * Compute BBT info from frame number.
     */

    currenttick *= 10;
    pos.bar = int32_t
    (
        currenttick / long(pos.ticks_per_beat) / pos.beats_per_bar
    );
    pos.beat = int32_t(((currenttick / long(pos.ticks_per_beat)) % m_beat_width));
    pos.tick = int32_t((currenttick % (m_ppqn * 10)));
    pos.bar_start_tick = pos.bar * pos.beats_per_bar * pos.ticks_per_beat;

    /*
     * I think we will need to add to the "valid" flags; see transport.h of
     * JACK.
     */

    pos.frame_rate = rate;
    pos.frame = (jack_nframes_t)
    (
        (currenttick * rate * 60.0) / (pos.ticks_per_beat * pos.beats_per_minute)
    );
    pos.bar++;
    pos.beat++;

    int jackcode = jack_transport_reposition(m_jack_client, &pos);
    if (jackcode != 0)
    {
        errprint("jack_assistant::set_position(): bad position structure");
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
 *  (CURRENTLY BUGGY!)
 *
 *  Note that we should consider massaging the following jack_position_t
 *  members to set them to 0 (or 0.0) if less than 1.0 or 0.5:
 *
 *      -   bar_start_tick
 *      -   ticks_per_beat
 *      -   beats_per_minute
 *      -   frame_time
 *      -   next_time
 *      -   audio_frames_per_video_frame
 *
 *  Also, why does bbt_offset start at 2128362496?
 */

int
jack_assistant::sync (jack_transport_state_t state)
{
    int result = 0;                     /* seq24 always returns 1   */
    m_jack_frame_current = jack_get_current_transport_frame(m_jack_client);

    (void) jack_transport_query(m_jack_client, &m_jack_pos);

    jack_nframes_t rate = m_jack_pos.frame_rate;
    if (rate == 0)
    {
        /*
         * The actual frame rate might be something like 48000.  Try to make
         * it work somehow, for now.
         */

        errprint("jack_assistant::sync(): zero frame rate");
        rate = 48000;
    }
    else
        result = 1;

    m_jack_tick = m_jack_frame_current * m_jack_pos.ticks_per_beat *
        m_jack_pos.beats_per_minute / (rate * 60.0) ;

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
 *
 * \return
 *      Returns 0 on success, non-zero on error.
 */

int
jack_process_callback (jack_nframes_t /* nframes */, void * /* arg */ )
{
#ifdef SAMPLE_AUDIO_CODE    // disabled, shown only for reference & learning
	jack_transport_state_t ts = jack_transport_query(client, NULL);
	if (ts == JackTransportRolling)
    {
        jack_default_audio_sample_t * in;
        jack_default_audio_sample_t * out;
		if (client_state == Init)
			client_state = Run;

		in = jack_port_get_buffer(input_port, nframes);
		out = jack_port_get_buffer(output_port, nframes);
		memcpy(out, in, sizeof (jack_default_audio_sample_t) * nframes);
	}
    else if (ts == JackTransportStopped)
    {
		if (client_state == Run)
			client_state = Exit;
	}
#endif
    return 0;
}

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
    int result = 0;
    jack_assistant * jack = (jack_assistant *)(arg);
    if (not_nullptr(jack))
    {
        result = jack->sync(state);         /* use the new member function */
    }
    else
    {
        errprint("jack_sync_callback(): null JACK pointer");
    }
    return result;
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
 *  of the same name.  This code comes from perform::output_func() from seq24.
 *
 * \note
 *      Follow up on this note found "out there":  "Maybe I'm wrong but if I
 *      understood correctly, recent jack1 transport no longer goes into
 *      Jack_Transport_Starting state before going to Jack_Transport_Rolling
 *      (this was deliberately dropped), but seq24 currently needs this to
 *      start off with jack transport."  On the other hand, some people have
 *      no issues.  This may have been due to the lack of m_jack_pos
 *      initialization.
 *
 * \param pad
 *      Provide a JACK scratchpad for sharing certain items between the
 *      perform object and the jack_assistant object.
 *
 * \return
 *      Returns true if JACK is running.
 */

bool
jack_assistant::output (jack_scratchpad & pad)
{
    if (m_jack_running)
    {
        double jack_ticks_converted;                // = 0.0;
        double jack_ticks_delta;                    // = 0.0;
        pad.js_init_clock = false;                  // no init until a good lock
        m_jack_transport_state = jack_transport_query(m_jack_client, &m_jack_pos);
        m_jack_frame_current = jack_get_current_transport_frame(m_jack_client);

        bool ok = m_jack_pos.frame_rate > 1000;         /* usually 48000       */
        if (! ok)
            info_message("jack_assistant::output(): small frame rate");

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
            m_jack_frame_last = m_jack_frame_current;
            pad.js_dumping = true;          // info_message("Start playback");
            m_jack_tick = m_jack_pos.frame *
                m_jack_pos.ticks_per_beat *
                m_jack_pos.beats_per_minute / (m_jack_pos.frame_rate * 60.0);

            jack_ticks_converted = m_jack_tick *        /* convert ticks */
            (
                double(m_ppqn) /                        // 4.0 --> member below
                (m_jack_pos.ticks_per_beat * m_jack_pos.beat_type / 4.0)
            );
            m_jack_parent.set_orig_ticks(long(jack_ticks_converted));
            pad.js_init_clock = true;
            pad.js_current_tick = pad.js_clock_tick = pad.js_total_tick =
                pad.js_ticks_converted_last = jack_ticks_converted;

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
                        double lrsize = m_jack_parent.get_right_tick() -
                            m_jack_parent.get_left_tick();

                        pad.js_current_tick -= lrsize;
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
            pad.js_jack_stopped = true;     // info_message("Stop playback");
        }

        /*
         * Jack Transport is Rolling Now !!!  Transport is in a sane state if
         * dumping == true.
         */

        if (pad.js_dumping)
        {
            m_jack_frame_current =
                jack_get_current_transport_frame(m_jack_client);

            if (m_jack_frame_current > m_jack_frame_last)   /* moving ahead? */
            {
                if (m_jack_pos.frame_rate > 1000)           /* usually 48000 */
                {
                    m_jack_tick += (m_jack_frame_current - m_jack_frame_last) *
                        m_jack_pos.ticks_per_beat *
                        m_jack_pos.beats_per_minute /
                        (m_jack_pos.frame_rate * 60.0);
                }
                else
                    info_message("jack_assistant::output() 2: zero frame rate");

                m_jack_frame_last = m_jack_frame_current;
            }

            jack_ticks_converted = m_jack_tick *
            (
                double(m_ppqn) /
                    (m_jack_pos.ticks_per_beat * m_jack_pos.beat_type / 4.0)
            );
            jack_ticks_delta = jack_ticks_converted - pad.js_ticks_converted_last;
            pad.js_clock_tick += jack_ticks_delta;
            pad.js_current_tick += jack_ticks_delta;
            pad.js_total_tick += jack_ticks_delta;
            m_jack_transport_state_last = m_jack_transport_state;
            pad.js_ticks_converted_last = jack_ticks_converted;

#ifdef SEQ64_USE_DEBUG_OUTPUT
            jack_debug_print(pad.js_current_tick, jack_ticks_delta);
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
 *  bits that are set in the given parameter.  For reference, here are the
 *  enumeration values from /usr/include/jack/types.h:
 *
\verbatim
        JackFailure         = 0x01
        JackInvalidOption   = 0x02
        JackNameNotUnique   = 0x04
        JackServerStarted   = 0x08
        JackServerFailed    = 0x10
        JackServerError     = 0x20
        JackNoSuchClient    = 0x40
        JackLoadFailure     = 0x80
        JackInitFailure     = 0x100
        JackShmFailure      = 0x200
        JackVersionError    = 0x400
        JackBackendError    = 0x800
        JackClientZombie    = 0x1000
\endverbatim
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
            (void) info_message(jsp->jf_meaning);   // .c_str());

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
    const char * name = clientname.c_str();
    jack_status_t status;
    jack_status_t * pstatus = &status;          // or NULL

#ifdef SEQ64_JACK_SESSION
    if (rc().jack_session_uuid().empty())
    {
        result = jack_client_open(name, JackNullOption, pstatus);   // 0x800000
    }
    else
    {
        const char * uuid = rc().jack_session_uuid().c_str();
        result = jack_client_open(name, JackSessionID, pstatus, uuid);
    }
#else
    result = jack_client_open(name, JackNullOption, pstatus);       // 0x800000
#endif

    if (not_nullptr(result) && not_nullptr(pstatus))
    {
        if (status & JackServerStarted)
            (void) info_message("JACK server started now");
        else
            (void) info_message("JACK server already started");

        if (status & JackNameNotUnique)
            (void) info_message("JACK client-name NOT unique");

        show_statuses(status);
    }
    return result;
}

#ifdef SEQ64_USE_DEBUG_OUTPUT

/**
 *  Debugging code for JACK.
 */

void
jack_assistant::jack_debug_print (double current_tick, double ticks_delta)
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
 *  The JACK timebase function defined here sets the JACK position structure.
 *  The original version of the function worked properly with Hydrogen, but
 *  not with Klick.  The new code seems to work with both.  More testing and
 *  clarification is needed.  This new code was "discovered" in the
 *  source-code for the "SooperLooper" project:
 *
 *          http://essej.net/sooperlooper/
 *
 *  The first difference with the new code is that it handles the case where
 *  the JACK position is moved (new_pos == true).  If this is true, and the
 *  JackPositionBBT bit is off in pos->valid, then the new BBT value is set.
 *
 *  The seconds set of differences are in the "else" clause.  In the new code,
 *  it is very simple: calculate the new tick value, back it off by the number
 *  of ticks in a beat, and perhaps go to the first beat of the next bar.
 *
 *  In the old code (complex!), the simple BBT adjustment is always made.
 *  This changes (perhaps) the beats_per_bar, beat_type, etc.  We
 *  need to make these settings use the actual global values for beats set for
 *  Sequencer64.  Then, if transitioning from JackTransportStarting to
 *  JackTransportRolling (instead of checking new_pos!), the BBT values (bar,
 *  beat, and tick) are finally adjusted.  Here are the steps, with old and new
 *  steps noted:
 *
 *      -#  Calculate the "delta" ticks based on the current frame, the
 *          ticks_per_beat, the beats_per_minute, and the frame_rate.  The old
 *          code saves this in a local, the new code assigns it to pos->tick.
 *      -#  Old code: save this delta as a positive value.
 *      -#  Figure out the settings and modify bar, beat, tick, and
 *          bar_start_tick.  The old and new code seem to have the same intent,
 *          but it seems like the new code is faster and also correct.
 *          -   Old code:  Calculations are made by division and mod
 *              operations.
 *          -   New code:  Calculations are made by increments and decrements
 *              in a while loop.
 *
 * \param state
 *      Indicates the current state of JACK transport.
 *
 * \param nframes
 *      The number of JACK frames in the current time period.
 *
 * \param pos
 *      Provides the position structure to be filled in, the
 *      address of the position structure for the next cycle; pos->frame will
 *      be its frame number. If new_pos is FALSE, this structure contains
 *      extended position information from the current cycle. If TRUE, it
 *      contains whatever was set by the requester. The timebase_callback's
 *      task is to update the extended information here.
 *
 * \param new_pos
 *      TRUE (non-zero) for a newly requested pos, or for the first cycle
 *      after the timebase_callback is defined.  This is usually 0 in
 *      Sequencer64 at present, and 1 if one, say, presses "rewind" in
 *      qjackctl.
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
    if (is_nullptr(pos))
    {
        errprint("jack_timebase_callback(): null position pointer");
        return;
    }

    /*
     * @change ca 2016-02-09
     *      Code from sooperlooper that we left out!
     */

    jack_assistant * jack = (jack_assistant *)(arg);
    pos->beats_per_minute = jack->m_beats_per_minute;
    pos->beats_per_bar = jack->m_beats_per_measure;
    pos->beat_type = jack->m_beat_width;
    pos->ticks_per_beat = jack->m_ppqn * 10.0;

    long ticks_per_bar = long(pos->ticks_per_beat * pos->beats_per_bar);
    long ticks_per_minute = long(pos->beats_per_minute * pos->ticks_per_beat);
    if (new_pos || ! (pos->valid & JackPositionBBT))    // try the NEW code
    {
        double minute = pos->frame / (double(pos->frame_rate * 60.0));
        long abs_tick = long(minute * ticks_per_minute);
        long abs_beat = 0;

        /*
         * @change ca 2016-02-09
         *      Handle 0 values of pos->ticks_per_beat and pos->beats_per_bar
         *      that occur at startup as JACK Master.
         */

        if (pos->ticks_per_beat > 0)                    // 0 at startup!
            abs_beat = long(abs_tick / pos->ticks_per_beat);

        if (pos->beats_per_bar > 0)                     // 0 at startup!
            pos->bar = int(abs_beat / pos->beats_per_bar);
        else
            pos->bar = 0;

        pos->beat = int(abs_beat - (pos->bar * pos->beats_per_bar) + 1);
        pos->tick = int(abs_tick - (abs_beat * pos->ticks_per_beat));
        pos->bar_start_tick = int(pos->bar * ticks_per_bar);
        pos->bar++;                             /* adjust start to bar 1 */
    }
    else
    {
        /*
         * Try this code, which computes the BBT (beats/bars/ticks) based on
         * the previous period.  It works!  "klick -j -P" follows Sequencer64
         * when the latter is JACK Master!  Note that the tick is delta'ed.
         */

        int delta_tick = int(nframes * ticks_per_minute / (pos->frame_rate * 60));
        pos->tick += delta_tick;
        while (pos->tick >= pos->ticks_per_beat)
        {
            pos->tick -= int(pos->ticks_per_beat);
            if (++pos->beat > pos->beats_per_bar)
            {
                pos->beat = 1;
                ++pos->bar;
                pos->bar_start_tick += ticks_per_bar;
            }
        }
    }
    pos->valid = JackPositionBBT;
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

#ifdef SEQ64_USE_DEBUG_OUTPUT

/**
 *  Print the JACK position.
 *
 * \param pos
 *      The JACK position to print.
 */

void
print_jack_pos (jack_position_t & pos, const std::string & tag)
{
    printf
    (
        "print_jack_pos(): '%s'\n"
        "    B:B:T = %d:%d:%d; bar_start_tick = %f\n"
        "    beats/bar = %f; beat_type = %f\n"
        "    ticks/beat = %f; beats/minute %f\n"
        "    frame = %d; frame_time = %f; frame_rate = %d\n",
        tag.c_str(),
        pos.bar, pos.beat, pos.tick, pos.bar_start_tick,
        pos.beats_per_bar, pos.beat_type,
        pos.ticks_per_beat, pos.beats_per_minute,
        int(pos.frame), pos.frame_time, int(pos.frame_rate)
    );
}

#endif

#endif      // SEQ64_JACK_SUPPORT

}           // namespace seq64

/*
 * jack_assistant.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

