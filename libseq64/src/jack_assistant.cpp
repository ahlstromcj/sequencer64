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
 * \updates       2018-04-12
 * \license       GNU GPLv2 or above
 *
 *  This module was created from code that existed in the perform object.
 *  Moving it into is own module makes it easier to maintain and makes the
 *  perform object a little easier to grok.
 *
 *  For the summaries of the JACK functions used in this module, and how
 *  the code is supposed to operate, see the Sequencer64 developer's reference
 *  manual.  It discusses the following items:
 *
 *  -   JACK Position Bits
 *  -   jack_transport_reposition()
 *
 *  Only JackPositionBBT is supported so far.
 *
 * JACK clients and BPM:
 *
 *  Does a JACK client need to be JACK Master before it can foist BPM changes on
 *  other clients?  What are the conventions?
 *
 *      -   https://linuxmusicians.com/viewtopic.php?t=14913&start=15
 *      -   http://jackaudio.org/api/transport-design.html
 *
 *  Lastly, one might be curious as to the origin of the name
 *  "jack_assistant".  Well, it is simply so this class can be called
 *  "jack_ass" for short :-D.
 */

#include <stdio.h>
#include <string.h>                     /* strdup() <gasp!>             */

#include "easy_macros.hpp"              /* C++ additions to easy macros */
#include "jack_assistant.hpp"           /* this seq64::jack_ass class   */
#include "midifile.hpp"                 /* seq64::midifile class        */
#include "mutex.hpp"                    /* seq64::mutex, automutex      */
#include "perform.hpp"                  /* seq64::perform class         */
#include "settings.hpp"                 /* "rc" and "user" settings     */

#undef  SEQ64_USE_DEBUG_OUTPUT          /* define for experiments only  */
#define USE_JACK_BBT_OFFSET             /* another experiment           */

#ifdef SEQ64_JACK_SUPPORT

/*
 *  All library code in the Sequencer64 project is in the seq64 namespace.
 */

namespace seq64
{

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
 *  be instructive.
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
 *  Provides a dummy callback.
 *
 * \param nframes
 *      An unused parameter.
 *
 * \param arg
 *      Provides the jack_assistant pointer.
 *
 * \return
 *      Does nothing, but returns nframes.  If the arg parameter is null, then
 *      0 is returned.
 */

int
jack_dummy_callback (jack_nframes_t nframes, void * arg)
{
    jack_assistant * j = (jack_assistant *)(arg);
    if (is_nullptr(j))
        nframes = 0;

    return nframes;
}

/**
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
 *  though it can be seen when calling show_position() in the
 *  jack_transport_callback() function.
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
jack_transport_callback (jack_nframes_t nframes, void * arg)
{
    jack_assistant * j = (jack_assistant *)(arg);
    if (not_nullptr(j))
    {
        perform & p = j->m_jack_parent;
        if (! p.is_running())
        {
            /*
             * For start or for FF/RW/key-p when not running.  If we're stopped,
             * we need to start, otherwise we need to reposition the transport
             * marker.  Not sure if the code in the ! j->m_jack_master
             * clause is necessary, it's not in Seq32.
             */

            jack_position_t pos;
            jack_transport_state_t s = jack_transport_query(j->client(), &pos);
            if (! j->m_jack_master)
            {
                if (pos.beats_per_minute > 1.0)     /* a sanity check   */
                {
                    static double s_old_bpm = 0.0;
                    if (pos.beats_per_minute != s_old_bpm)
                    {
                        s_old_bpm = pos.beats_per_minute;
                        infoprintf("BPM = %f\n", pos.beats_per_minute);
                        j->parent().set_beats_per_minute(pos.beats_per_minute);
                    }
                }
            }
            if (s == JackTransportRolling || s == JackTransportStarting)
            {
                j->m_jack_transport_state_last = JackTransportStarting;
                if (p.start_from_perfedit())
                {
                    if (nframes == 0)
                    {
                        // no code
                    }
                    p.inner_start(true);
                }
                else
                    p.inner_start(p.song_start_mode());
            }
            else        /* don't start, just reposition transport marker */
            {
                long tick = get_current_jack_position((void *) j);
                long diff = tick - j->get_jack_stop_tick();
                if (diff != 0)
                {
                    p.set_reposition();
                    p.set_start_tick(tick);
                    j->set_jack_stop_tick(tick);
                }
            }
        }
    }
    return 0;
}

/**
 *  A more full-featured initialization for a JACK client, which is meant to
 *  be called by the init() function.  Do not call this function if the JACK
 *  client handle is already open.
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
 * helgrind:
 *
 *      Valgrind's helgrind tool shows
 *
\verbatim
          Possible data race during read of size 4 at 0xF854E58 by thread #1
             by 0x267602: seq64::create_jack_client(...)
          This conflicts with a previous write of size 4 by thread #2
             by 0x267602: seq64::create_jack_client(...)
\endverbatim
 *
 *      So we add a static mutex to use with our automutex.  Does not prevent
 *      that message..... WHY?
 *
 * We've never disabled the SEQ64_JACK_SESSION macro, and we like the
 * error-reporting we get by that method.  So we've commented out the
 * following code in favor of using the session-uuid code:
 *
 *  # ifdef SEQ64_JACK_SESSION
 *  # else
 *      jack_status_t * ps = NULL;
 *      result = jack_client_open(name, JackNullOption, ps);
 *  # endif
 *
 * \param clientname
 *      Provides the name of the client, used in the call to
 *      jack_client_open().  By default, this name is the macro SEQ64_PACKAGE
 *      (i.e.  "sequencer64").  The name scope is local to each server. Unless
 *      forbidden by the JackUseExactName option, the server will modify this
 *      name to create a unique variant, if needed.
 *
 * \param uuid
 *      The optional UUID to assign to the new client.  If empty, there is no
 *      UUID.
 *
 * \return
 *      Returns a pointer to the JACK client if JACK has opened the client
 *      connection successfully.  Otherwise, a null pointer is returned.
 */

jack_client_t *
create_jack_client
(
    const std::string & clientname,
    const std::string & uuid
)
{
    jack_client_t * result = nullptr;
    const char * name = clientname.c_str();
    jack_status_t status;
    jack_status_t * ps = &status;
    jack_options_t options = JackNoStartServer;
    if (uuid.empty())
    {
        result = jack_client_open(name, options, ps);
    }
    else
    {
        const char * uid = uuid.c_str();
        options = (jack_options_t) (JackNoStartServer | JackSessionID);
        result = jack_client_open(name, options, ps, uid);
    }
    apiprint("jack_client_open", clientname.c_str());
    if (not_nullptr(result))
    {
        if (status & JackServerStarted)
            (void) info_message("JACK server started now");
        else
            (void) info_message("JACK server already started");

        if (status & JackNameNotUnique)
        {
            char t[80];
            snprintf(t, sizeof t, "JACK client-name '%s' not unique", name);
            (void) info_message(t);
        }
        else
            show_jack_statuses(status);
    }
    else
        (void) error_message("JACK server not running?");

    return result;                      /* bad result handled by caller     */
}

/**
 *  Provides a list of JACK status bits, and a brief string to explain the
 *  status bit.  Terminated by a 0 value and an empty string.
 */

jack_status_pair_t
s_status_pairs [] =
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
show_jack_statuses (unsigned bits)
{
    jack_status_pair_t * jsp = &s_status_pairs[0];
    while (jsp->jf_bit != 0)
    {
        if (bits & jsp->jf_bit)
            (void) info_message(jsp->jf_meaning);

        ++jsp;
    }
}

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
 * \param bpmeasure
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
    midibpm bpminute,
    int ppqn,
    int bpmeasure,
    int beatwidth
) :
    m_jack_parent               (parent),
    m_jack_client               (nullptr),
    m_jack_client_name          (),
    m_jack_client_uuid          (),
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
    m_jack_frame_rate           (0),
    m_toggle_jack               (false),
    m_jack_stop_tick            (0),
    m_ppqn                      (0),
    m_beats_per_measure         (bpmeasure),    // m_bp_measure
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
 * \setter parent().toggle_song_start_mode()
 */

bool
jack_assistant::toggle_song_start_mode ()
{
    return parent().toggle_song_start_mode();
}

/**
 * \getter parent().song_start_mode()
 */

bool
jack_assistant::song_start_mode () const
{
    return parent().song_start_mode();
}

/**
 * \setter parent().start_from_perfedit()
 */

void
jack_assistant::set_start_from_perfedit (bool start)
{
    parent().start_from_perfedit(start);
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
    printf("[%s]\n", msg.c_str());
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
        apiprint("jack_get_client_name", "sync");
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
        apiprint("jack_get_client_name_by_uuid", "sync");
    }
    if (not_nullptr(actualname))
        m_jack_client_name = actualname;
    else
        m_jack_client_name = SEQ64_PACKAGE;

    char * actualuuid = jack_get_uuid_for_client_name
    (
        m_jack_client, m_jack_client_name.c_str()
    );
    apiprint("jack_get_uuid_for_client_name", "sync");
    if (not_nullptr(actualuuid))
        m_jack_client_uuid = actualuuid;
    else
        m_jack_client_uuid = rc().jack_session_uuid();

    std::string jinfo = "JACK client:uuid is ";
    jinfo += m_jack_client_name;
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
 *      The call to jack_timebase_callback() to supply jack with BBT, etc.
 *      would occasionally fail when the *pos information had zero or some
 *      garbage in the pos.frame_rate variable. This would occur when there
 *      was a rapid change of frame position by another client... i.e.
 *      qjackctl.  From the jack API:
 *
 *          "pos address of the position structure for the next cycle;
 *          pos->frame will be its frame number. If new_pos is FALSE, this
 *          structure contains extended position information from the current
 *          cycle.  If TRUE, it contains whatever was set by the requester.
 *          The timebase_callback's task is to update the extended information
 *          here."
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
        std::string package = rc().app_client_name() + "_transport";
        m_jack_running = true;              /* determined surely below      */
        m_jack_master = true;               /* ditto, too tricky, though    */
        m_jack_client = client_open(package);
        if (m_jack_client == NULL)
        {
            m_jack_running = false;
            m_jack_master = false;
            return error_message("JACK server not running, JACK sync disabled");
        }
        else
            m_jack_frame_rate = jack_get_sample_rate(m_jack_client);

        get_jack_client_info();
        jack_on_shutdown(m_jack_client, jack_shutdown_callback, (void *) this);
        apiprint("jack_on_shutdown", "sync");

        /*
         * Stazed JACK support uses only the jack_transport_callback().  Makes
         * sense, since seq24/32/64 are not "slow-sync" clients.
         */

        int jackcode = jack_set_process_callback    /* see notes in banner  */
        (
            m_jack_client, jack_transport_callback, (void *) this
        );
        apiprint("jack_set_process_callback", "sync");
        if (jackcode != 0)
        {
            m_jack_running = false;
            m_jack_master = false;
            return error_message("jack_set_process_callback() failed]");
        }

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
            apiprint("jack_set_session_callback", "sync");
            if (jackcode != 0)
            {
                m_jack_running = false;
                m_jack_master = false;
                return error_message("jack_set_session_callback() failed]");
            }
        }
#endif

        bool master_is_set = false;         /* flag to handle trickery  */
        bool cond = rc().with_jack_master_cond();
        if (rc().with_jack_master())        /* OR with 'cond' removed   */
        {
            /*
             * 'cond' is true if we want to fail if there is already a JACK
             * master, i.e. it is a conditional attempt to be JACK master.
             */

            jackcode = jack_set_timebase_callback
            (
                m_jack_client, cond, jack_timebase_callback, (void *) this
            );
            apiprint("jack_set_timebase_callback", "sync");
            if (jackcode == 0)
            {
                (void) info_message("JACK sync master");
                m_jack_master = true;
                master_is_set = true;
            }
            else
            {
                /*
                 * seq24 doesn't set this flag, but that seems incorrect.
                 */

                m_jack_running = false;
                m_jack_master = false;
                return error_message("jack_set_timebase_callback() failed");
            }
        }
        if (! master_is_set)
        {
            m_jack_master = false;
            (void) info_message("JACK sync slave");
        }
    }
    else
    {
        if (m_jack_running)
            (void) info_message("JACK sync still enabled");
        else
            (void) info_message("Initialized, running without JACK sync");
    }
    return m_jack_running;
}

/**
 *  Tears down the JACK infrastructure.
 *
 * \todo
 *      Note that we still need a way to call jack_release_timebase()  when
 *      the user turns off the "JACK Master" status of Sequencer64.
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

        apiprint("jack_deactivate", "sync");
        if (jack_deactivate(m_jack_client) != 0)
            (void) error_message("Can't deactivate JACK sync client");

        if (jack_client_close(m_jack_client) != 0)
            (void) error_message("Can't close JACK sync client");

        apiprint("deinit", "sync");
    }
    if (! m_jack_running)
        (void) info_message("JACK sync disabled");

    return m_jack_running;
}

/**
 *  Activate JACK here.  This function is called by perform::activate() after
 *  the master bus is activated successfully.
 *
 * \return
 *      Returns true if the m_jack_client pointer is null, which means only
 *      that we're not running JACK.  Also returns true if the pointer exists
 *      and the jack_active() call succeeds.
 *
 * \sideeffect
 *      The m_jack_running and m_jack_master flags are falsified in
 *      jack_activate() fails.
 */

bool
jack_assistant::activate ()
{
    bool result = true;
    if (not_nullptr(m_jack_client))
    {
        int rc = jack_activate(m_jack_client);
        result = rc == 0;
        apiprint("jack_activate", "sync");
        if (! result)
        {
            m_jack_running = m_jack_master = false;
            (void) error_message("Can't activate JACK sync client");
        }
        else
        {
            if (m_jack_running)
                (void) info_message("JACK sync enabled");
            else
            {
                m_jack_master = false;
                (void) error_message("error, JACK sync not enabled");
            }
        }
    }
    return result;
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
        apiprint("jack_transport_start", "sync");
    }
    else if (rc().with_jack())
        (void) error_message("Sync start: JACK not running");
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
        apiprint("jack_transport_stop", "sync");
    }
    else if (rc().with_jack())
        (void) error_message("Sync stop: JACK not running");
}

/**
 * \setter m_beats_per_minute
 *      For the future, changing the BPM (beats/minute) internally.  We
 *      should consider adding validation.  However,
 *      perform::set_beats_per_minute() does validate already.
 *      Also, since jack_transport_reposition() can be "called at any time by
 *      any client", we have removed the check for "is master".
 *      We do seem to see more "bad position structure" messages, though.
 *
 * \param bpminute
 *      Provides the beats/minute value to set.
 */

void
jack_assistant::set_beats_per_minute (midibpm bpminute)
{
    if (bpminute != m_beats_per_minute)
    {
        m_beats_per_minute = bpminute;
        if (not_nullptr(m_jack_client))
        {
            (void) jack_transport_query(m_jack_client, &m_jack_pos);
            m_jack_pos.beats_per_minute = bpminute;
            int jackcode = jack_transport_reposition(m_jack_client, &m_jack_pos);
            apiprint("jack_transport_reposition", "set bpm");
            if (jackcode != 0)
            {
                errprint("jack_transport_reposition(): bad position structure");
            }
        }
    }
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
 * Stazed:
 *
 *      The jack_frame calculation is all that is needed to change JACK
 *      position. The BBT calculation can be sent, but will be overridden by the
 *      first call to jack_timebase_callback() of any Master set. If no Master
 *      is set, then the BBT will display the new position but will not change
 *      it, even if the transport is rolling. There is no need to send BBT on
 *      position change -- the fact that jack_transport_locate() exists and only
 *      uses the frame position is proof that BBT is not needed! Upon further
 *      reflection, why not send BBT?  Because other programs do not... let's
 *      follow convention.  The calculation for jack_transport_locate(), works,
 *      is simpler, and does not send BBT. The calculation for
 *      jack_transport_reposition() will be commented out again.
 *      jack_BBT_position() is not necessary to change jack position!
 *
 *  Note that there are potentially a couple of divide-by-zero opportunities
 *  in this function.
 *
 *  Helgrind complains about a possible data race involving
 *  jack_transport_locate() when starting playing.
 *
 * \param songmode
 *      True if the caller wants to position while in Song mode.
 *
 * Alternate parameter to_left_tick (non-seq32 version):
 *
 *      If true, the current tick is set to the leftmost tick, instead of the
 *      0th tick.  Now used, but only if relocate is true.  One question is,
 *      do we want to perform this function if rc().with_jack_transport() is
 *      true?  Seems like we should be able to do it only if m_jack_master is
 *      true.
 *
 * \param tick
 *      If using Song mode for this call then this value is set as the
 *      "current tick" value.  If it's value is bad (SEQ64_NULL_MIDIPULSE),
 *      then this parameter is set to 0 before being used.
 */

void
jack_assistant::position (bool songmode, midipulse tick)
{

#ifdef SEQ64_JACK_SUPPORT

    /*
     * Let's follow the example of Stazed's tick_to_jack_frame() function.
     * One odd effect we want to solve is why Sequencer64 as JACK slave
     * is messing up the playback in Hydrogen (it oscillates around the
     * 0 marker).
     */

#ifdef PLATFORM_DEBUG_TMI
    if (tick == 0)
        printf("jack position() tick = 0\n");
#endif

    if (songmode)                               /* master in song mode  */
    {
        if (is_null_midipulse(tick))
            tick = 0;
        else
            tick *= 10;
    }
    else
        tick = 0;

    int ticks_per_beat = m_ppqn * 10;
    int beats_per_minute = parent().get_beats_per_minute();
    uint64_t tick_rate = (uint64_t(m_jack_frame_rate) * tick * 60.0);
    long tpb_bpm = ticks_per_beat * beats_per_minute * 4.0 / m_beat_width;
    uint64_t jack_frame = tick_rate / tpb_bpm;
    if (m_jack_master)
    {
        /*
         * We don't want to do this unless we are JACK Master.  Otherwise,
         * other JACK clients never advance if Sequencer64 won't advance.
         * However, according to JACK docs, "Any client can start or stop
         * playback, or seek to a new location."
         */

        if (jack_transport_locate(m_jack_client, jack_frame) != 0)
            (void) info_message("jack_transport_locate() failed");
    }

    if (parent().is_running())
        parent().set_reposition(false);

#endif  // SEQ64_JACK_SUPPORT

}

#ifdef USE_JACK_ASSISTANT_SET_POSITION

/**
 *  This function is currently unused, and has been macroed out.
 *
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
 * \param tick
 *      Provides the current position to be set.
 */

void
jack_assistant::set_position (midipulse tick)
{
    jack_position_t pos;
    pos.valid = JackPositionBBT;                // flag what will be modified
    pos.beats_per_bar = m_beats_per_measure;
    pos.beat_type = m_beat_width;
    pos.ticks_per_beat = m_ppqn * 10;
    pos.beats_per_minute = get_beats_per_minute();

    /*
     * pos.frame = frame;
     */

    tick *= 10;                     /* compute BBT info from frame number */
    pos.bar = int32_t(tick / long(pos.ticks_per_beat) / pos.beats_per_bar);
    pos.beat = int32_t(((tick / long(pos.ticks_per_beat)) % m_beat_width));
    pos.tick = int32_t((tick % (m_ppqn * 10)));
    pos.bar_start_tick = pos.bar * pos.beats_per_bar * pos.ticks_per_beat;
    ++pos.bar;
    ++pos.beat;

    /*
     * Modifying frame rate and frame cannot be set from clients, the server
     * sets them; see transport.h of JACK.
     *
     *  jack_nframes_t rate = jack_get_sample_rate(m_jack_client);
     *  pos.frame_rate = rate;
     *  pos.frame = (jack_nframes_t)
     *  (
     *      (tick * rate * 60.0) /
     *      (pos.ticks_per_beat * pos.beats_per_minute)
     *  );
     */

#ifdef USE_JACK_BBT_OFFSET
    pos.valid = (jack_position_bits_t)(pos.valid | JackBBTFrameOffset);
    pos.bbt_offset = 0;
#endif

    int jackcode = jack_transport_reposition(m_jack_client, &pos);
    apiprint("jack_transport_reposition", "sync");
    if (jackcode != 0)
    {
        errprint("jack_assistant::set_position(): bad position structure");
    }
}

#endif  // USE_JACK_ASSISTANT_SET_POSITION

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
    apiprint("jack_transport_query", "sync");

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

        parent().inner_start(parent().song_start_mode());       // shorten!
        break;

    case JackTransportLooping:

        // infoprint("[JackTransportLooping]");
        break;

    default:

        errprint("unknown JACK transport/sync state");
        break;
    }
    return result;
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
 *  This is the slow-sync callback, which the stazed code replaces with
 *  jack_transport_callback().
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
 *      Another note about JACK.  If another JACK client supplies tempo/BBT
 *      different from seq42 (as Master), the perfroll grid will be incorrect.
 *      Perfroll uses internal temp/BBT and cannot update on the fly. Even if
 *      seq42 could support tempo/BBT changes, all info would have to be
 *      available before the transport start, to work.  For this reason, the
 *      tempo/BBT info will be plugged from the seq42 internal settings here,
 *      always. This is the method used by probably all other JACK clients
 *      with some sort of time-line. The JACK API indicates that BBT is
 *      optional and AFIK, other sequencers only use frame & frame_rate from
 *      JACK for internal calculations. The tempo and BBT info is always
 *      internal. Also, if there is no Master set, then we would need to plug
 *      it here to follow the JACK frame anyways.
 *
 * \param pad
 *      Provides a JACK scratchpad for sharing certain items between the
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
        double jack_ticks_converted;                // MAY NEED long typedef
        double jack_ticks_delta;                    // MAY NEED long typedef
        pad.js_init_clock = false;                  // no init until a good lock
        m_jack_transport_state = jack_transport_query(m_jack_client, &m_jack_pos);

        /*
         *  Using the seq32 code here works to solve issue #48,
         *  non-JACK-Master playback not working if built for non-seq32 JACK
         *  transport.  So we scrapped the old code entirely.
         *
         *  As for the setting of beats/minute, we had thought that we
         *  wanted to force a change in BPM only if we are JACK Master,
         *  but this is not true, and prevents Sequencer64 from playing
         *  back when not the Master.
         */

        m_jack_pos.beats_per_bar = m_beats_per_measure;
        m_jack_pos.beat_type = m_beat_width;
        m_jack_pos.ticks_per_beat = m_ppqn * 10;
        m_jack_pos.beats_per_minute = parent().get_beats_per_minute();
        if
        (
            m_jack_transport_state_last == JackTransportStarting &&
            m_jack_transport_state == JackTransportRolling
        )
        {
            /*
             * This is a second time we get the frame number.
             */

            m_jack_frame_current =
                jack_get_current_transport_frame(m_jack_client);

            m_jack_frame_last = m_jack_frame_current;
            pad.js_dumping = true;

            /*
             * Here, Seq32 uses the tempo map if in song mode, instead of
             * making these calculations.
             */

            m_jack_tick = m_jack_pos.frame * m_jack_pos.ticks_per_beat *
                m_jack_pos.beats_per_minute / (m_jack_pos.frame_rate * 60.0);

            jack_ticks_converted = m_jack_tick * tick_multiplier();

            /*
             * And Seq32 continues here.
             */

            m_jack_parent.set_orig_ticks(long(jack_ticks_converted));
            pad.js_init_clock = true;
            pad.js_current_tick = pad.js_clock_tick = pad.js_total_tick =
                pad.js_ticks_converted_last = jack_ticks_converted;

            /*
             * We need to make sure another thread can't modify these values.
             */

            if (pad.js_looping && pad.js_playback_mode)
            {
                while (pad.js_current_tick >= m_jack_parent.get_right_tick())
                {
                    pad.js_current_tick -= m_jack_parent.left_right_size();
                }

                /*
                 * Not sure that either of these lines have any effect!
                 */

                m_jack_parent.off_sequences();
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
                /*
                 * Seq32 uses tempo map if in song mode here, instead.
                 */

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
jack_assistant::show_position (const jack_position_t & pos)
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
 *  Converts a JACK transport value to a human-readable string.
 *
 * \param state
 *      Provides the transport state value.
 *
 * \return
 *      Returns the state name.
 */

std::string
jack_assistant::get_state_name (const jack_transport_state_t & state)
{
    std::string result;
    switch (state)
    {
    case JackTransportStopped:

        result = "[JackTransportStopped]";
        break;

    case JackTransportRolling:

        result = "[JackTransportRolling]";
        break;

    case JackTransportStarting:

        result = "[JackTransportStarting]";
        break;

    case JackTransportLooping:

        result = "[JackTransportLooping]";
        break;

    default:

        errprint("[JackTransportUnknown]");
        break;
    }
    return result;
}

/**
 *  A member wrapper function for the new free function create_jack_client().
 *
 * \param clientname
 *      Provides the name of the client, used in the call to
 *      create_jack_client().  By default, this name is the macro SEQ64_PACKAGE
 *      (i.e.  "sequencer64").
 *
 * \return
 *      Returns a pointer to the JACK client if JACK has opened the client
 *      connection successfully.  Otherwise, a null pointer is returned.
 */

jack_client_t *
jack_assistant::client_open (const std::string & clientname)
{
    jack_client_t * result = create_jack_client
    (
        clientname, rc().jack_session_uuid()
    );
    return result;                      /* bad result handled by caller     */
}

#ifdef PLATFORM_DEBUG

jack_client_t *
jack_assistant::client () const
{
    static jack_client_t * s_preserved_client = nullptr;
    if (not_nullptr(s_preserved_client))
    {
        if (s_preserved_client != m_jack_client)
        {
            errprint("JACK sync client pointer corrupt, JACK disabled!");
            s_preserved_client = m_jack_client = nullptr;
        }
    }
    else
    {
        s_preserved_client = m_jack_client;
    }
    return m_jack_client;
}

#endif  // PLATFORM_DEBUG

/*
 *  JACK callbacks.
 */

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
 *  The second set of differences are in the "else" clause.  In the new code,
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
 * Stazed:
 *
 *  The call to jack_timebase_callback() to supply JACK with BBT, etc. would
 *  occasionally fail when the pos information had zero or some garbage in the
 *  pos.frame_rate variable. This would occur when there was a rapid change of
 *  frame position by another client... i.e. qjackctl.  From the JACK API:
 *
 *      pos	address of the position structure for the next cycle; pos->frame
 *      will be its frame number. If new_pos is FALSE, this structure contains
 *      extended position information from the current cycle.  If TRUE, it
 *      contains whatever was set by the requester.  The timebase_callback's
 *      task is to update the extended information here."
 *
 *  The "If TRUE" line seems to be the issue. It seems that qjackctl does not
 *  always set pos.frame_rate so we get garbage and some strange BBT
 *  calculations that display in qjackctl. So we need to set it here and just
 *  use m_jack_frame_rate for calculations instead of pos.frame_rate.
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
    jack_transport_state_t state,           // currently unused !!!
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
     *  Code from sooperlooper that we left out!
     */

    jack_assistant * jack = (jack_assistant *)(arg);
    pos->beats_per_minute = jack->get_beats_per_minute();
    pos->beats_per_bar = jack->get_beats_per_measure();
    pos->beat_type = jack->get_beat_width();
    pos->ticks_per_beat = jack->get_ppqn() * 10.0;

    long ticks_per_bar = long(pos->ticks_per_beat * pos->beats_per_bar);
    long ticks_per_minute = long(pos->beats_per_minute * pos->ticks_per_beat);
    double framerate = double(pos->frame_rate * 60.0);

    /**
     * \todo
     *      Shouldn't we process the first clause ONLY if new_pos is true?
     */

    if (new_pos || ! (pos->valid & JackPositionBBT))    // try the NEW code
    {
        double minute = pos->frame / framerate;
        long abs_tick = long(minute * ticks_per_minute);
        long abs_beat = 0;

        /*
         *  Handle 0 values of pos->ticks_per_beat and pos->beats_per_bar that
         *  occur at startup as JACK Master.
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

        int delta_tick = int(nframes * ticks_per_minute / framerate);
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

        if (jack->m_jack_master)
            pos->beats_per_minute = jack->parent().get_beats_per_minute();
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

/**
 *  This function gets the current JACK position.  The Seq32 version also uses
 *  its tempo map to adjust this, but Sequencer64 currently does not.
 *
 * \warning
 *      Currently valgrind flags j->client() as uninitialized.
 *
 * \param arg
 *      Provides the putative jack_assistant pointer, assumed to be not null.
 *
 * \return
 *      Returns the calculated tick position if no errors occur.  Otherwise,
 *      returns 0.
 */

long
get_current_jack_position (void * arg)
{
    jack_assistant * j = (jack_assistant *)(arg);
    double ppqn = double(j->get_ppqn());
    double ticks_per_beat = ppqn * 10;              // 192 * 10 = 1920
    double beats_per_minute = j->get_beats_per_measure();
    double beat_type = j->get_beat_width();
    if (not_nullptr(j->client()))
    {
        jack_nframes_t frame = jack_get_current_transport_frame(j->client());
        double jack_tick = frame * ticks_per_beat * beats_per_minute /
            (j->jack_frame_rate() * 60.0);              // need research

        return jack_tick * (ppqn / (ticks_per_beat * beat_type / 4.0));
    }
    else
    {
        j->error_message("Null JACK sync client");
        return 0;
    }
}

}           // namespace seq64

#endif      // SEQ64_JACK_SUPPORT

/*
 * jack_assistant.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

