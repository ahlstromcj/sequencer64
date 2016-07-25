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
 * \updates       2016-07-23
 * \license       GNU GPLv2 or above
 *
 *  This module was created from code that existed in the perform object.
 *  Moving it into is own module makes it easier to maintain and makes the
 *  perform object a little easier to grok.
 *
 *  For the summaries of the JACK functions used in this module, and how
 *  the code is supposed to operate, see the Sequencer64 developer's reference
 *  manual.
 *
 * JACK Position Bits to support in Sequencer64, their values, their purpose,
 * and the jack_position_t field they manage:
 *
 *  -   JackPositionBBT = 0x10. Bar, Beat, Tick.  The fields managed are bar,
 *      beat, tick, bar_start_tick, beats_per_bar, beat_type, ticks_per_beat,
 *      beats_per_minute.
 *  -   JackPositionTimecode = 0x20. External timecode.  The fields managed
 *      are frame_time and next_time.
 *  -   JackBBTFrameOffset = 0x40. Offset of BBT information. The sole field
 *      managed is bbt_offset, the frame offset for the BBT fields. The given
 *      bar, beat, and tick values actually refer to a time frame_offset
 *      frames before the start of the cycle.  It should be assumed to be 0 if
 *      JackBBTFrameOffset is not set. If JackBBTFrameOffset is set and this
 *      value is zero, the BBT time refers to the first frame of this cycle.
 *      If the value is positive, the BBT time refers to a frame that many
 *      frames before the start of the cycle.
 *
 *  Only JackPositionBBT is supported so far.  Applications that support
 *  JackPositionBBT are encouraged to also fill the JackBBTFrameOffset-managed
 *  field (bbt_offset).  We are experimenting with this for now; there's not a
 *  lot of material out there on the Web.
 *
 *  Lastly, one might be curious as to the origin of the name
 *  "jack_assistant".  Well, it is simply so this class can be called
 *  "jack_ass" for short :-D.
 */

#include <stdio.h>

#include "jack_assistant.hpp"
#include "midifile.hpp"
#include "perform.hpp"
#include "settings.hpp"

#undef  SEQ64_USE_DEBUG_OUTPUT          /* define for EXPERIMENTS only  */
#define USE_JACK_BBT_OFFSET             /* another EXPERIMENT           */

#ifdef SEQ64_JACK_SUPPORT

namespace seq64
{

#undef USE_JACK_DEBUG_PRINT
#ifdef USE_JACK_DEBUG_PRINT

/**
 *  Debugging code for JACK.  We made this static so that we can hide it in
 *  this module and enable it without enabling all of the debug code available
 *  in the Sequencer64 code base.  As a side-effect, we added a couple of
 *  const accessors to jack_assistant so that outsiders can monitor some of
 *  its status.
 *
 *  Now, this is really too much output, so you'll have to enable it
 *  separately by defining USE_JACK_DEBUG_PRINT.  The difference in output
 *  between this function and what jack_assitant::show_position() report may
 *  be instructive.  LATER.
 *
 * \param jack
 *      The jack_assistant object for which to show debugging data.
 *
 * \param current_tick
 *      The current time location.
 *
 * \param tick_delta
 *      The change in ticks to show.
 */

static void
jack_debug_print
(
    const jack_assistant & jack,
    double current_tick,
    double ticks_delta
)
{
    static long s_output_counter = 0;
    if ((s_output_counter++ % 100) == 0)
    {
        const jack_position_t & p = jack.get_jack_pos();
        double jtick = jack.get_jack_tick();

        /*
         * double jack_tick = (p.bar-1) * (p.ticks_per_beat * p.beats_per_bar ) +
         *  (p.beat-1) * p.ticks_per_beat + p.tick;
         */

        long pbar = long(jtick) / long(p.ticks_per_beat * p.beats_per_bar);
        long pbeat = long(jtick) % long(p.ticks_per_beat * p.beats_per_bar);
        pbeat /= long(p.ticks_per_beat);
        long ptick = long(jtick) % long(p.ticks_per_beat);
        printf
        (
            "* curtick=%4.2f delta=%4.2f BBT=%ld:%ld:%ld "
            "jbbt=%d:%d:%d jtick=%4.2f\n"   //  mtick=%4.2f cdelta=%4.2f\n"
            ,
            current_tick, ticks_delta, pbar+1, pbeat+1, ptick,
            p.bar, p.beat, p.tick, jtick    // , jack_tick, jtick - jack_tick
        );
    }
}

#endif  // USE_JACK_DEBUG_PRINT

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
 *
 * \param bpminute
 *      The beats/minute to set up JACK to use (applies to Master setup).
 *
 * \param ppqn
 *      The parts-per-quarter-note setting in force for the present tune.
 *
 * \param bpm
 *      The beats/measure (time signature numerator) in force for the present
 *      tune.
 *
 * \param beatwidth
 *      The beat-width (time signature denominator)  in force for the present
 *      tune.
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
    m_jack_client_name          (),
    m_jack_client_uuid          (),
    m_jack_frame_current        (0),
    m_jack_frame_last           (0),
#ifdef USE_STAZED_JACK_SUPPORT
    m_jack_frame_rate           (0),
#endif
    m_jack_pos                  (),
    m_jack_transport_state      (JackTransportStopped),
    m_jack_transport_state_last (JackTransportStopped),
    m_jack_tick                 (0.0),
#ifdef SEQ64_JACK_SESSION
    m_jsession_ev               (nullptr),
#endif
    m_jack_running              (false),
    m_jack_master               (false),
#ifdef USE_STAZED_JACK_SUPPORT
    m_jack_stop_tick            (0),
#endif
    m_ppqn                      (0),
    m_beats_per_measure         (bpm),          // m_bp_measure
    m_beat_width                (beatwidth),    // m_bw
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
 *  Tries to obtain the best information on the JACK client and the UUID
 *  assigned to this client.  Sets m_jack_client_name and m_jack_client_info
 *  as side-effects.
 */

void
jack_assistant::get_jack_client_info ()
{
    char * actualname = nullptr;
    if (rc().jack_session_uuid().empty())
    {
        actualname = jack_get_client_name(m_jack_client);
    }
    else
    {
        /*
         * Currently, this doesn't seem to work, no matter what
         * is supplied as a UUID.  A null pointer is returned.  Still
         * investigating.
         */

        const char * uuid = rc().jack_session_uuid().c_str();
        actualname = jack_get_client_name_by_uuid(m_jack_client, uuid);
    }
    if (not_nullptr(actualname))
        m_jack_client_name = actualname;
    else
        m_jack_client_name = SEQ64_PACKAGE;

    char * actualuuid = jack_get_uuid_for_client_name
    (
        m_jack_client, m_jack_client_name.c_str()
    );
    if (not_nullptr(actualuuid))
        m_jack_client_uuid = actualuuid;
    else
        m_jack_client_uuid = rc().jack_session_uuid();

    std::string jinfo = m_jack_client_name;
    if (! m_jack_client_uuid.empty())
    {
        jinfo += ":";
        jinfo += m_jack_client_uuid;
    }
    (void) info_message(jinfo);
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
 *  STAZED:
 *      The call to jack_timebase_callback() to supply jack with BBT, etc would
 *      occasionally fail when the *pos information had zero or some garbage in
 *      the pos.frame_rate variable. This would occur when there was a rapid
 *      change of frame position by another client... i.e.  qjackctl.  From the
 *      jack API:
 *
 *      "pos address of the position structure for the next cycle; pos->frame
 *      will be its frame number. If new_pos is FALSE, this structure contains
 *      extended position information from the current cycle.  If TRUE, it
 *      contains whatever was set by the requester.  The timebase_callback's
 *      task is to update the extended information here."
 *
 *      The "If TRUE" line seems to be the issue. It seems that qjackctl does
 *      not always set pos.frame_rate so we get garbage and some strange BBT
 *      calculations that display in qjackctl. So we need to set it here and
 *      just use m_jack_frame_rate for calculations instead of pos.frame_rate.
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
        {
            m_jack_master = false;
            return error_message("JACK server not running, JACK sync disabled");
        }
#ifdef USE_STAZED_JACK_SUPPORT
        else
            m_jack_frame_rate = jack_get_sample_rate(m_jack_client);
#endif

        get_jack_client_info();
        jack_on_shutdown(m_jack_client, jack_shutdown_callback, (void *) this);

#ifndef USE_STAZED_JACK_SUPPORT

        /*
         * Stazed JACK support uses only the jack_process_callback().  Makes
         * sense, since seq24/32/64 are not "slow-sync" clients.
         */

        int jackcode = jack_set_sync_callback
        (
            m_jack_client, jack_sync_callback, (void *) this
        );
        if (jackcode != 0)
            return error_message("jack_set_sync_callback() failed");
#endif

        /*
         * Although they say this code is needed to get JACK transport to work
         * properly, seq24 doesn't use this.  But it doesn't hurt to set it up.
         * The Stazed code does use it.
         */

        jackcode = jack_set_process_callback        /* see notes in banner  */
        (
#ifdef USE_STAZED_JACK_SUPPORT
            m_jack_client, jack_process_callback, (void *) this
#else
            m_jack_client, jack_process_callback, NULL
#endif
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

                m_jack_master = false;      // m_jack_running = false too?
                return error_message("jack_set_timebase_callback() failed");
            }
        }
        if (! master_is_set)
        {
            (void) info_message("JACK transport slave");
            m_jack_master = false;
        }
        if (jack_activate(m_jack_client) != 0)
        {
            m_jack_running = false;             // USE_STAZED_JACK_SUPPORT
            return error_message("Cannot activate as JACK client");
        }

        if (m_jack_running)
            (void) info_message("JACK sync now enabled");
        else
            (void) error_message("Initialization error, JACK sync not enabled");
    }
    else
    {
        if (m_jack_running)
            (void) info_message("JACK sync still enabled");
        else
            (void) info_message("Initialized, but running without JACK");
    }
    return m_jack_running;
}

/**
 *  Tears down the JACK infrastructure.
 *
 * \return
 *      Returns the value of m_jack_running, which should be false.
 */

bool
jack_assistant::deinit ()
{
    if (m_jack_running)
    {
        m_jack_running = false;
#ifdef USE_STAZED_JACK_SUPPORT
        m_jack_master = false;
        if (jack_release_timebase(m_jack_client) != 0)
            (void) error_message("Cannot release JACK timebase");
#else
        if (m_jack_master)
        {
            m_jack_master = false;
            if (jack_release_timebase(m_jack_client) != 0)
                (void) error_message("Cannot release JACK timebase");
        }
#endif

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
        (void) info_message("JACK sync disabled");

    return m_jack_running;
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
 * Stazed:
 *
 *      The jack_frame calculation is all that is needed to change JACK
 *      position The BBT calc can be sent but will be overridden by the first
 *      call to jack_timebase_callback() of any Master set. If no Master is
 *      set, then the BBT will display the new position but will not change
 *      even if the transport is rolling. There is no need to send BBT on
 *      position change - the fact that the function jack_transport_locate()
 *      exists and only uses the frame position is proof that BBT is not
 *      needed! Upon further reflection, why not send BBT?  Because other
 *      programs do not.... let's follow convention.  The below calculation for
 *      jack_transport_locate(), works, is simpler and does not send BBT. The
 *      calculation for jack_transport_reposition() will be commented out
 *      again.  The BBT call to jack_BBT_position() is not necessary to
 *      change jack position!
 *
 * \warning
 *      A lot of this code is effectively disabled by an early return
 *      statement.
 *
 * \param to_left_tick
 *      If true, the current tick is set to the leftmost tick, instead of the
 *      0th tick.  Now used, but only if relocate is true.  One question is,
 *      do we want to perform this function if rc().with_jack_transport() is
 *      true?  Seems like we should be able to do it only if m_jack_master is
 *      true.
 *
 * \param relocate
 *      If true (it defaults to false), then we allow the relocation of the
 *      JACK transport to the current_tick or the left tick, rather than to
 *      frame 0.  EXPERIMENTAL, enables dead code from seq24.  Seems to work
 *      if set to true when we are the JACK Master.  Enabling this code makes
 *      "klick -j -P" work, after a fashion.  It clicks, but at a way too
 *      rapid rate.
 */

#ifdef USE_STAZED_JACK_SUPPORT

void
jack_assistant::position (bool to_left_tick, bool relocate)
{
    long current_tick = 0;
    if (to_left_tick)                   /* actually master in song mode */
        current_tick = to_left_tick;

    current_tick *= 10;

    int ticks_per_beat = m_ppqn * 10;                   /* 192 * 10 = 1920 */
    int beats_per_minute =  m_master_bus.get_bpm();
    uint64_t tick_rate = uint64_t(m_jack_frame_rate * current_tick * 60.0);
    long tpb_bpm = ticks_per_beat * beats_per_minute / (m_beat_width / 4.0 );
    uint64_t jack_frame = tick_rate / tpb_bpm;
    jack_transport_locate(m_jack_client,jack_frame);
    if (! j->is_running())   // or p.is_jack_running() or global_is_running ?
        m_reposition = false;
}

#else   // USE_STAZED_JACK_SUPPORT

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

#endif  // USE_STAZED_JACK_SUPPORT

/**
 *  Provides the code that was effectively commented out in the
 *  perform::position_jack() function.  We might be able to use it in other
 *  functions.
 *
 *  Computing the BBT information from the frame number is relatively simple
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
 *
 * \param currenttick
 *      Provides the current position to be set.
 */

void
jack_assistant::set_position (midipulse currenttick)
{
    jack_position_t pos;
    pos.valid = JackPositionBBT;                // flag what will be modified
    pos.beats_per_bar = m_beats_per_measure;
    pos.beat_type = m_beat_width;
    pos.ticks_per_beat = m_ppqn * 10;
    pos.beats_per_minute = get_beats_per_minute();
    currenttick *= 10;              /* compute BBT info from frame number */
    pos.bar = int32_t
    (
        currenttick / long(pos.ticks_per_beat) / pos.beats_per_bar
    );
    pos.beat = int32_t(((currenttick / long(pos.ticks_per_beat)) % m_beat_width));
    pos.tick = int32_t((currenttick % (m_ppqn * 10)));
    pos.bar_start_tick = pos.bar * pos.beats_per_bar * pos.ticks_per_beat;
    pos.bar++;
    pos.beat++;

    /*
     * Modifying frame rate and frame cannot be set from clients, the server
     * sets them; see transport.h of JACK.
     *
     *  jack_nframes_t rate = jack_get_sample_rate(m_jack_client);
     *  pos.frame_rate = rate;
     *  pos.frame = (jack_nframes_t)
     *  (
     *      (currenttick * rate * 60.0) /
     *      (pos.ticks_per_beat * pos.beats_per_minute)
     *  );
     */

#ifdef USE_JACK_BBT_OFFSET
    pos.valid = (jack_position_bits_t)(pos.valid | JackBBTFrameOffset);
    pos.bbt_offset = 0;
#endif

    int jackcode = jack_transport_reposition(m_jack_client, &pos);
    if (jackcode != 0)
    {
        errprint("jack_assistant::set_position(): bad position structure");
    }
}

/**
 *  A helper function for syncing up with JACK parameters.  Sequencer64 is not
 *  a slow-sync client (and Stazed support doesn't use it), so that callback
 *  is not really needed, but we probably need this sub-function here to
 *  start out with the right values for interacting with JACK.
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
 *
 * \param state
 *      The JACK transport state to be set.
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

        errprint("jack_assistant::sync(): unknown JACK transport state");
        break;
    }
    return result;
}

/*
 *  Implemented second patch for JACK Transport from freddix/seq24 GitHub
 *  project.  Added the following function.  This function is supposed to
 *  allow seq24/sequencer64 to follow JACK transport.
 *
 *  For more advanced ideas, see the MetronomeJack::process_callback()
 *  function in the klick project.  It plays a metronome tick after
 *  calculating if it needs to or not.  (Maybe we could use it to provide our
 *  own tick for recording patterns.)
 *
 *  If debugging is enabled in the build, then this function outputs the
 *  current JACK position information every couple of seconds, which can be
 *  useful to examine the interactions with other JACK clients.
 *
 *  The code enabled via USE_JACK_BBT_OFFSET sets the JACK
 *  position fieldl bbt_offset to 0.  It doesn't seem to have any effect,
 *  though it can be send when calling show_position() in the
 *  jack_process_callback() function.
 *
 * Stazed:
 *
 *      This process callback is called by JACK whether stopped or rolling.
 *      Assuming every JACK cycle...  "...client supplied function that is
 *      called by the engine anytime there is work to be done".  There seems to
 *      be no definition of '...work to be done'.  nframes = buffer_size -- is
 *      not used.
 *
 * \param nframes
 *      Unused.
 *
 * \param arg
 *      Used for debug output now.  Note that this function will be called
 *      very often, and this pointer will currently not be used unless
 *      debugging is turned on.
 *
 * \return
 *      Returns 0 on success, non-zero on error.
 */

int
jack_process_callback (jack_nframes_t /* nframes */, void * arg)
{
    const jack_assistant * j = (jack_assistant *)(arg);
    if (not_nullptr(j))
    {

#ifdef USE_STAZED_JACK_SUPPORT

        perform & p = j->m_jack_parent;

        /*
         * For start or for FF/RW/key-p when not running.  If we're stopped, we
         * need to start, otherwise we need to reposition the transport marker.
         */

        if (! p.is_running())               // or j->is_running() ?
        {
            jack_transport_state_t s =
                jack_transport_query(j->client(), nullptr);

            if (s == JackTransportRolling || s == JackTransportStarting)
            {
                j->m_jack_transport_state_last = JackTransportStarting;
                if (p.start_from_perfedit())
                    p.inner_start(true);
                else
                    p.inner_start(rc().jack_start_mode());
                    // global_song_start_mode);
            }
            else        /* don't start, just reposition transport marker */
            {
                long tick = get_current_jack_position((void *) j);
                long diff = tick - j->get_jack_stop_tick();
                if (diff != 0)
                {
                    p.set_reposition();         // a perform option
                    p.set_start_tick(tick);     // p.set_starting_tick(tick);
                    j->set_jack_stop_tick(tick);
                }
            }
        }

#endif  // USE_STAZED_JACK_SUPPORT

#ifdef SAMPLE_AUDIO_CODE    // disabled, shown only for reference & learning
        jack_transport_state_t ts = jack_transport_query(jack->client(), NULL);
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
    }
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
 *  This is the slow-sync callback, which stazed replaces with
 *  jack_process_callback().
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
 *      start off with JACK transport."  On the other hand, some people have
 *      no issues.  This may have been due to the lack of m_jack_pos
 *      initialization.
 *
 * Stazed:
 *
 *      Another note about JACK....  If another JACK client is supplying
 *      tempo/BBT info that is different from seq42 (as Master), the perfroll
 *      grid will be incorrect. Perfroll uses internal temp/BBT and cannot
 *      update on the fly. Even if seq42 could support tempo/BBT changes, all
 *      info would have to be available before the transport start, to work.
 *      For this reason, the tempo/BBT info will be plugged from the seq42
 *      internal settings here... always. This is the method used by probably
 *      all other JACK clients with some sort of time-line. The JACK API
 *      indicates that BBT is optional and AFIK, other sequencers only use
 *      frame & frame_rate from JACK for internal calculations. The tempo and
 *      BBT info is always internal. Also, if there is no Master set, then we
 *      would need to plug it here to follow the JACK frame anyways.
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
        double jack_ticks_converted;
        double jack_ticks_delta;
        pad.js_init_clock = false;                  // no init until a good lock
        m_jack_transport_state = jack_transport_query(m_jack_client, &m_jack_pos);
#ifdef USE_STAZED_JACK_SUPPORT
        m_jack_pos.beats_per_bar = m_beats_per_measure;
        m_jack_pos.beat_type = m_beat_width;
        m_jack_pos.ticks_per_beat = m_ppqn * 10;
        m_jack_pos.beats_per_minute = parent().master_bus().get_bpm();
#else

        bool ok = m_jack_pos.frame_rate > 1000;         /* usually 48000       */
        if (! ok)
            info_message("jack_assistant::output(): small frame rate");
#endif

        if
        (
            m_jack_transport_state_last == JackTransportStarting &&     // OR?
            m_jack_transport_state == JackTransportRolling
        )
        {
            m_jack_frame_current =
                jack_get_current_transport_frame(m_jack_client);

            m_jack_frame_last = m_jack_frame_current;
            pad.js_dumping = true;          // info_message("Start playback");
            m_jack_tick = m_jack_pos.frame * m_jack_pos.ticks_per_beat *
                m_jack_pos.beats_per_minute / (m_jack_pos.frame_rate * 60.0);

            jack_ticks_converted = m_jack_tick * tick_multiplier();
            m_jack_parent.set_orig_ticks(long(jack_ticks_converted));
            pad.js_init_clock = true;
            pad.js_current_tick = pad.js_clock_tick = pad.js_total_tick =
                pad.js_ticks_converted_last = jack_ticks_converted;

            /*
             * We need to make sure another thread can't modify these
             * values.  Also, maybe some of the parent (perform) values need to
             * move, to the scratch-pad, if not used directly in the perform
             * object.  Why the "double" value here?
             */

            if (pad.js_looping && pad.js_playback_mode)
            {
                while (pad.js_current_tick >= m_jack_parent.get_right_tick())
                {
                    pad.js_current_tick -= m_jack_parent.left_right_size();
                }
#ifdef USE_STAZED_JACK_SUPPORT
                m_jack_parent.off_sequences();
#else
                m_jack_parent.reset_sequences();            /* seq24 */
#endif
                m_jack_parent.set_orig_ticks(long(pad.js_current_tick));
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
                        m_jack_pos.ticks_per_beat * m_jack_pos.beats_per_minute /
                        (m_jack_pos.frame_rate * 60.0);
                }
                else
                    info_message("jack_assistant::output() 2: zero frame rate");

                m_jack_frame_last = m_jack_frame_current;
            }

            jack_ticks_converted = m_jack_tick * tick_multiplier();
            jack_ticks_delta = jack_ticks_converted - pad.js_ticks_converted_last;
            pad.js_clock_tick += jack_ticks_delta;
            pad.js_current_tick += jack_ticks_delta;
            pad.js_total_tick += jack_ticks_delta;
            m_jack_transport_state_last = m_jack_transport_state;
            pad.js_ticks_converted_last = jack_ticks_converted;

#ifdef USE_JACK_DEBUG_PRINT
            jack_debug_print(*this, pad.js_current_tick, jack_ticks_delta);
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
 *
 * \param bits
 *      The mask of the bits to be shown in the output.
 */

void
jack_assistant::show_statuses (unsigned bits)
{
    jack_status_pair_t * jsp = &sm_status_pairs[0];
    while (jsp->jf_bit != 0)
    {
        if (bits & jsp->jf_bit)
            (void) info_message(jsp->jf_meaning);

        ++jsp;
    }
}

/**
 *  Shows a one-line summary of a JACK position structure.  This function is
 *  meant for experimenting and learning.
 *
 *  The fields of this structure are as follows.  Only the fields we care
 *  about are shown.
 *
\verbatim
    jack_nframes_t      frame_rate:     current frame rate (per second)
    jack_nframes_t      frame:          frame number, always present
    jack_position_bits_t valid:         which other fields are valid
\endverbatim
 *
\verbatim
JackPositionBBT:
    int32_t             bar:            current bar
    int32_t             beat:           current beat-within-bar
    int32_t             tick:           current tick-within-beat
    double              bar_start_tick
    float               beats_per_bar:  time signature "numerator"
    float               beat_type:      time signature "denominator"
    double              ticks_per_beat
    double              beats_per_minute
\endverbatim
 *
\verbatim
JackBBTFrameOffset:
    jack_nframes_t      bbt_offset;     frame offset for the BBT fields
\endverbatim
 *
 *  Only the most "important" and time-varying fields are shown. The format
 *  output is brief and inscrutable unless you read this format example:
 *
\verbatim
    nnnnn frame B:B:T N/D TPB BPM BBT
      ^     ^     ^   ^ ^  ^   ^   ^
      |     |     |   | |  |   |   |
      |     |     |   | |  |   |    -------- bbt_offset (frame), even if invalid
      |     |     |   | |  |    ------------ beats_per_minute
      |     |     |   | |   ---------------- ticks_per_beat (PPQN * 10?)
      |     |     |   |  ------------------- beat_type (denominator)
      |     |     |    --------------------- beats_per_bar (numerator)
      |     |      ------------------------- bar : beat : tick
      |      ------------------------------- frame (number)
       ------------------------------------- the "valid" bits
\endverbatim
 *
 *  The "valid" field is shown as bits in the same bit order as shown here, but
 *  represented as a five-character string, "nnnnn", n = 0 or 1:
 *
\verbatim
    JackVideoFrameOffset = 0x100
    JackAudioVideoRatio  = 0x080
    JackBBTFrameOffset   = 0x040
    JackPositionTimecode = 0x020
    JackPositionBBT      = 0x010
\endverbatim
 *
 *  We care most about nnnnn = "00101" in our experiments (the most common
 *  output will be "00001").  And we don't worry about non-integer
 *  measurements... we truncate them to integers.  Change the output format if
 *  you want to play with non-Western timings.
 *
 * \param pos
 *      The JACK position structure to dump.
 */

void
jack_assistant::show_position (const jack_position_t & pos) const
{
    char temp[80];
    std::string nnnnn = "00000";
    if (pos.valid & JackVideoFrameOffset)
        nnnnn[0] = '1';

    if (pos.valid & JackAudioVideoRatio)
        nnnnn[1] = '1';

    if (pos.valid & JackBBTFrameOffset)
        nnnnn[2] = '1';

    if (pos.valid & JackPositionTimecode)
        nnnnn[3] = '1';

    if (pos.valid & JackPositionBBT)
        nnnnn[4] = '1';

    snprintf
    (
        temp, sizeof temp, "%s %8ld %03d:%d:%04d %d/%d %5d %3d %d",
        nnnnn.c_str(), long(pos.frame),
        int(pos.bar), int(pos.beat), int(pos.tick),
        int(pos.beats_per_bar), int(pos.beat_type),
        int(pos.ticks_per_beat), int(pos.beats_per_minute),
        int(pos.bbt_offset)
    );
    infoprint(temp);                    /* no output in release mode */
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
 * JackOpenOptions:
 *
 *      JackSessionID | JackServerName | JackNoStartServer | JackUseExactName
 *
 *      Only the first is used at present.
 *
 * \param clientname
 *      Provides the name of the client, used in the call to
 *      jack_client_open().  By default, this name is the macro SEQ64_PACKAGE
 *      (i.e.  "sequencer64").  The name scope is local to each server. Unless
 *      forbidden by the JackUseExactName option, the server will modify this
 *      name to create a unique variant, if needed.
 *
 * \return
 *      Returns a pointer to the JACK client if JACK has opened the client
 *      connection successfully.  Otherwise, a null pointer is returned.
 */

jack_client_t *
jack_assistant::client_open (const std::string & clientname)
{
    jack_client_t * result = nullptr;
    const char * name = clientname.c_str();
    jack_status_t status;

#ifdef SEQ64_JACK_SESSION
    jack_status_t * pstatus = &status;
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
    jack_status_t * pstatus = NULL;
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
    return result;                      /* bad result handled by caller     */
}

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
 *  Stazed:
 *
 *      The call to jack_timebase_callback() to supply JACK with BBT, etc. would
 *      occasionally fail when the pos information had zero or some garbage in
 *      the pos.frame_rate variable. This would occur when there was a rapid
 *      change of frame position by another client... i.e. qjackctl.  From the
 *      JACK API:
 *
 *          pos	address of the position structure for the next cycle;
 *          pos->frame will be its frame number. If new_pos is FALSE, this
 *          structure contains extended position information from the current
 *          cycle.  If TRUE, it contains whatever was set by the requester.
 *          The timebase_callback's task is to update the extended information
 *          here."
 *
 *          The "If TRUE" line seems to be the issue. It seems that qjackctl
 *          does not always set pos.frame_rate so we get garbage and some
 *          strange BBT calculations that display in qjackctl. So we need to
 *          set it here and just use m_jack_frame_rate for calculations instead
 *          of pos.frame_rate.
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
    pos->beats_per_minute = jack->get_beats_per_minute();
    pos->beats_per_bar = jack->get_beats_per_measure();
    pos->beat_type = jack->get_beat_width();
    pos->ticks_per_beat = jack->get_ppqn() * 10.0;

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
#ifdef USE_JACK_BBT_OFFSET
    pos->bbt_offset = 0;
    pos->valid = (jack_position_bits_t)
    (
        pos->valid | JackBBTFrameOffset | JackPositionBBT
    );
#else
    pos->valid = JackPositionBBT;
#endif
}

/**
 *  This callback is to shut down JACK by clearing the jack_assistant ::
 *  m_jack_running flag.
 *
 * \param arg
 *      Points to the jack_assistant in charge of JACK support for the perform
 *      object.
 */

void
jack_shutdown_callback (void * arg)
{
    jack_assistant * jack = (jack_assistant *)(arg);
    if (not_nullptr(jack))
    {
        jack->set_jack_running(false);
        infoprint("[JACK shutdown. JACK sync disabled.]");
    }
    else
    {
        errprint("jack_shutdown_callback(): null JACK pointer");
    }
}

#ifdef USE_STAZED_JACK_SUPPORT

long
get_current_jack_position (void * arg)
{
//  perform * p = (perform *) arg;
    jack_assistant * j= (jack_assistant *)(arg);
    double ppqn = double(j->get_ppqn());
//  double ppqn = double(c_ppqn);
    double ticks_per_beat = ppqn * 10;              // 192 * 10 = 1920
    double beats_per_minute = j->get_beats_per_measure();
    double beat_type = j->get_beat_width();
    jack_nframes_t current_frame = jack_get_current_transport_frame(j->client());
    double jack_tick = current_frame * ticks_per_beat * beats_per_minute /
        (j->m_jack_frame_rate * 60.0);  ///// need accessor and research

    return jack_tick * (ppqn / (ticks_per_beat * beat_type / 4.0));
}

#endif      // USE_STAZED_JACK_SUPPORT

}           // namespace seq64

#endif      // SEQ64_JACK_SUPPORT

/*
 * jack_assistant.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

