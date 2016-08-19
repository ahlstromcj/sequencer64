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
 * \date          2015-07-23
 * \updates       2016-08-18
 * \license       GNU GPLv2 or above
 *
 *  This class contains a number of functions that used to reside in the
 *  still-large perform module.
 */

#include "globals.h"                    /* globals, nullptr, and more       */
#include "midibyte.hpp"                 /* midipulse typedef                */

#ifdef SEQ64_JACK_SUPPORT

#include <jack/jack.h>
#include <jack/transport.h>
#ifdef SEQ64_JACK_SESSION
#include <jack/session.h>
#endif

/**
 * EXPERIMENTAL.  Builds, but not ready for actual usage.
 */

#undef  USE_STAZED_JACK_SUPPORT

#else       // ! SEQ64_JACK_SUPPORT

#undef SEQ64_JACK_SESSION

#endif      // SEQ64_JACK_SUPPORT


/**
 * Define this macro to use the new seq24 v. 0.9.3 delta-tick calculation
 * code.  This code doesn't quite work for generating the proper rate of MIDI
 * clocks, and so have disabled that code until we can figure out what it is
 * we're doing wrong. Do not enable it unless you are willing to test it.
 */

#undef  USE_SEQ24_0_9_3_CODE            /* DEFINE ONLY FOR EXPERIMENTING!   */

namespace seq64
{

class perform;                          /* jack_assistant parent is perform */

/**
 *  Provide a temporary structure for passing data and results between a
 *  perform and jack_assistant object.  The jack_assistant class already
 *  has access to the members of perform, but it needs access to and
 *  modification of "local" variables in perform::output_func().
 *  This scratchpad is useful even if JACK support is not enabled.
 */

class jack_scratchpad
{

public:

    double js_current_tick;             /**< Holds current location.        */
    double js_total_tick;               /**< Current location ignoring L/R. */
#ifdef USE_SEQ24_0_9_3_CODE
    long js_clock_tick;                 /* changed in seq24 0.9.3           */
#else
    double js_clock_tick;               /**< Identical to js_total_tick.    */
#endif
    bool js_jack_stopped;               /**< Flags perform::inner_stop().   */
    bool js_dumping;                    /**< Non-JACK playback in progress? */
    bool js_init_clock;                 /**< We now have a good JACK lock.  */
    bool js_looping;                    /**< seqedit loop button is active. */
    bool js_playback_mode;              /**< Song mode (versus live mode).  */
#ifdef USE_STAZED_JACK_SUPPORT
    double js_ticks_converted;          /**< Keeps track of ...?            */
    double js_ticks_delta;              /**< Minor difference in tick.      */
#endif
    double js_ticks_converted_last;     /**< Keeps track of position?       */
#if defined USE_SEQ24_0_9_3_CODE || defined USE_STAZED_JACK_SUPPORT
    long js_delta_tick_frac;            /* seq24 0.9.3                      */
#endif

};

#ifdef SEQ64_JACK_SUPPORT

/**
 *  Provides an internal type to make it easier to display a specific and
 *  accurate human-readable message when a JACK operation fails.
 */

typedef struct
{
    /**
     *  Holds one of the bit-values from jack_status_t, which is defined as an
     *  "enum JackStatus" type.
     */

    unsigned jf_bit;

    /**
     *  Holds a textual description of the corresponding status bit.
     */

    std::string jf_meaning;

} jack_status_pair_t;

/**
 *  This class provides the performance mode JACK support.
 */

class jack_assistant
{
    friend int jack_process_callback (jack_nframes_t nframes, void * arg);
    friend void jack_shutdown_callback (void * arg);
    friend int jack_sync_callback
    (
        jack_transport_state_t state,
        jack_position_t * pos,
        void * arg
    );
    friend void jack_timebase_callback
    (
        jack_transport_state_t state,
        jack_nframes_t nframes,
        jack_position_t * pos,
        int new_pos,
        void * arg
    );

#ifndef USE_STAZED_JACK_SUPPORT
    friend long get_current_jack_position (void * arg);
#endif

#ifdef SEQ64_JACK_SESSION
    friend void jack_session_callback (jack_session_event_t * ev, void * arg);
#endif

private:

    /**
     *  Pairs the JACK status bits with human-readable descriptions of each
     *  one.
     */

    static jack_status_pair_t sm_status_pairs [];

    /**
     *  Provides the perform object that needs this JACK assistant/scratchpad
     *  class.
     */

    perform & m_jack_parent;

    /**
     *  Provides a handle into JACK, so that the application, as a JACK
     *  client, can issue commands and retrieve status information from JACK.
     */

    jack_client_t * m_jack_client;

    /**
     *  A new member to hold the actual name of the client assigned by JACK.
     *  We might show this in the user-interface at some point.
     */

    std::string m_jack_client_name;

    /**
     *  A new member to hold the actual UUID of the client assigned by JACK.
     *  We might show this in the user-interface at some point.
     */

    std::string m_jack_client_uuid;

    /**
     *  Holds the current frame number obtained from JACK transport, via a
     *  call to jack_get_current_transport_frame().
     */

    jack_nframes_t m_jack_frame_current;

    /**
     *  Holds the last frame number we got from JACK, so that progress can be
     *  tracked.  Also used in incrementing m_jack_tick.
     */

    jack_nframes_t m_jack_frame_last;

#ifdef USE_STAZED_JACK_SUPPORT

    /**
     *  Holds the current frame rate.  Just in case.
     */

    jack_nframes_t m_jack_frame_rate;

#endif

    /**
     *  Provides positioning information on JACK playback.  This structure is
     *  filled via a call to jack_transport_query().  It holds, among other
     *  items, the frame rate (often 48000), the ticks/beat, and the
     *  beats/minute.
     */

    jack_position_t m_jack_pos;

    /**
     *  Holds the JACK transport state.  Common values are
     *  JackTransportStopped, JackTransportRolling, and JackTransportLooping.
     */

    jack_transport_state_t m_jack_transport_state;

    /**
     *  Holds the last JACK transport state.
     */

    jack_transport_state_t m_jack_transport_state_last;

    /**
     *  The tick/pulse value derived from the current frame number, the
     *  ticks/beat value, the beats/minute value, and the frame rate.
     */

    double m_jack_tick;

#ifdef SEQ64_JACK_SESSION

    /**
     *  Provides a kind of handle to the JACK session manager.  Used in the
     *  session_event() function.
     */

    jack_session_event_t * m_jsession_ev;
#endif

    /**
     *  Indicates if JACK Sync has been enabled successfully.
     */

    bool m_jack_running;

    /**
     *  Indicates if JACK Sync has been enabled successfully, with the
     *  application running as JACK Master.
     */

    bool m_jack_master;

#ifdef USE_STAZED_JACK_SUPPORT

    /**
     *  TBD.
     */

    bool m_toggle_jack;

    /**
     *  TBD.
     */

    midipulse m_jack_stop_tick;

    /**
     *  TBD.
     */

    bool m_follow_transport;

#endif  // USE_STAZED_JACK_SUPPORT

    /**
     *  Holds the global PPQN value for the Sequencer64 session.  It is used
     *  for calculating ticks/beat (pulses/beat) and for setting the tick
     *  position.
     *
     */

    int m_ppqn;

    /**
     *  Holds the song's beats/measure value for using in setting JACK
     *  position.
     */

    int m_beats_per_measure;

    /**
     *  Holds the song's beat width value (denominator of the time signature)
     *  for using in setting JACK position.
     */

    int m_beat_width;

    /**
     *  Holds the song's beats/minute (BPM) value for using in setting JACK
     *  position.
     */

    int m_beats_per_minute;

public:

    jack_assistant
    (
        perform & parent,
        int bpminute    = SEQ64_DEFAULT_BPM,
        int ppqn        = SEQ64_USE_DEFAULT_PPQN,
        int bpm         = SEQ64_DEFAULT_BEATS_PER_MEASURE,
        int beatwidth   = SEQ64_DEFAULT_BEAT_WIDTH
    );
    ~jack_assistant ();

    /**
     * \getter m_jack_parent
     *      Needed for external callbacks.
     */

    perform & parent ()
    {
        return m_jack_parent;
    }

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
     * \getter m_ppqn
     */

    int get_ppqn () const
    {
        return m_ppqn;
    }

    /**
     * \getter m_beat_width
     */

    int get_beat_width () const
    {
        return m_beat_width;
    }

    /**
     * \setter m_beat_width
     *
     * \param bw
     *      Provides the beat-width (denominator of the time signature)
     *      value to set.
     */

    void set_beat_width (int bw)
    {
        m_beat_width = bw;
    }

    /**
     * \getter m_beats_per_measure
     */

    int get_beats_per_measure () const
    {
        return m_beats_per_measure;
    }

    /**
     * \setter m_beats_per_measure
     *
     * \param bpm
     *      Provides the beats/measure (numerator of the time signature)
     *      value to set.
     */

    void set_beats_per_measure (int bpm)
    {
        m_beats_per_measure = bpm;
    }

    /**
     * \getter m_beats_per_minute
     */

    int get_beats_per_minute () const
    {
        return m_beats_per_minute;
    }

    /**
     * \setter m_beats_per_minute
     *      For the future, changing the BPM (beats/minute) internally.  We
     *      should consider adding validation.  However,
     *      perform::set_beats_per_minute() does validate already.
     *
     * \param bpminute
     *      Provides the beats/minute value to set.
     */

    void set_beats_per_minute (int bpminute)
    {
        m_beats_per_minute = bpminute;
    }

    /**
     * \getter m_jack_transport_state
     */

    jack_transport_state_t transport_state () const
    {
        return m_jack_transport_state;
    }

    bool init ();                       // init_jack ();
    bool deinit ();                     // deinit_jack ();

#ifdef SEQ64_JACK_SESSION
    bool session_event ();              // jack_session_event ();
#endif

    void start ();                      // start_jack();
    void stop ();                       // stop();

#ifdef USE_STAZED_JACK_SUPPORT
    void position (bool state, midipulse tick);
#else
    void position                       // position_jack();
    (
        bool to_left_tick,              // instead of current tick
        bool relocate = false           // enable "dead code"
    );
#endif

    bool output (jack_scratchpad & pad);

    /**
     * \setter m_ppqn
     *      For the future, changing the PPQN internally.  We should consider
     *      adding validation.  But it is used by perform.
     *
     * \param ppqn
     *      Provides the PPQN value to set.
     */

    void set_ppqn (int ppqn)
    {
        m_ppqn = ppqn;
    }

    /**
     * \getter m_jack_tick
     */

    double get_jack_tick () const
    {
        return m_jack_tick;
    }

    /**
     * \getter m_jack_pos
     */

    const jack_position_t & get_jack_pos () const
    {
        return m_jack_pos;
    }

#ifdef USE_STAZED_JACK_SUPPORT

    void toggle_jack_mode ()
    {
        set_jack_mode(! m_jack_running);
    }

    void set_jack_mode (bool mode)
    {
        m_toggle_jack = mode;
    }

    bool get_jack_mode () const
    {
        return m_toggle_jack;
    }

    /**
     * \getter m_jack_stop_tick
     */

    midipulse get_jack_stop_tick () const
    {
        return m_jack_stop_tick;
    }

    /**
     * \setter m_jack_stop_tick
     */

    void set_jack_stop_tick (long tick)
    {
        m_jack_stop_tick = tick;
    }

    /**
     * \getter m_jack_frame_rate
     */

    jack_nframes_t jack_frame_rate () const
    {
        return m_jack_frame_rate;
    }

#endif  // USE_STAZED_JACK_SUPPORT

#ifdef USE_STAZED_TRANSPORT

    bool get_follow_transport () const
    {
        return m_follow_transport;
    }

    void set_follow_transport (bool aset)
    {
        m_follow_transport = aset;
    }

    void toggle_follow_transport ()
    {
        set_follow_transport(! m_follow_transport);
    }

    void song_start_mode ();
    void toggle_song_start_mode ();
    void set_start_from_perfedit (bool start);

#endif  // USE_STAZED_TRANSPORT

    /**
     * \getter m_jack_client
     */

    jack_client_t * client () const
    {
        return m_jack_client;
    }

    /**
     * \getter m_jack_client_name
     */

    const std::string & client_name () const
    {
        return m_jack_client_name;
    }

    /**
     * \getter m_jack_client_uuid
     */

    const std::string & client_uuid () const
    {
        return m_jack_client_uuid;
    }

private:

    /**
     * \setter m_jack_running
     *
     * \param flag
     *      Provides the is-running value to set.
     */

    void set_jack_running (bool flag)
    {
        m_jack_running = flag;
    }

    /**
     *  Convenience function for internal use.  Should we change 4.0 to a
     *  member value?  What does it mean?
     *
     * \return
     *      Returns the multiplier to convert a JACK tick value according to
     *      the PPQN, ticks/beat, and beat-type settings.
     */

    double tick_multiplier () const
    {
        double denom = (m_jack_pos.ticks_per_beat * m_jack_pos.beat_type / 4.0);
        return double(m_ppqn) / denom;
    }

    bool info_message (const std::string & msg);
    bool error_message (const std::string & msg);
    jack_client_t * client_open (const std::string & clientname);
    void get_jack_client_info ();
    void show_statuses (unsigned bits);
    void show_position (const jack_position_t & pos) const;
    int sync (jack_transport_state_t state = (jack_transport_state_t)(-1));
    void set_position (midipulse currenttick);

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

extern void jack_shutdown_callback (void * arg);
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

#ifdef USE_STAZED_JACK_SUPPORT

extern long get_current_jack_position (void * arg);

#endif

#ifdef SEQ64_JACK_SESSION
extern void jack_session_callback (jack_session_event_t * ev, void * arg);
#endif

#endif  // SEQ64_JACK_SUPPORT

}           // namespace seq64

#endif      // SEQ64_JACK_ASSISTANT_HPP

/*
 * jack_assistant.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

