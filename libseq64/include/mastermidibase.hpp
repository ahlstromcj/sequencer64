#ifndef SEQ64_MASTERMIDIBASE_HPP
#define SEQ64_MASTERMIDIBASE_HPP

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
 * \file          mastermidibase.hpp
 *
 *  This module declares/defines the Master MIDI Bus base class.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2016-11-23
 * \updates       2018-06-02
 * \license       GNU GPLv2 or above
 *
 *  The mastermidibase module is the base-class version of the mastermidibus
 *  module.  There's a lot of common code needed by the various
 *  implementations of the seq64::mastermidibus class:  ALSA, RtMidi, and
 *  PortMidi.
 */

#include <vector>                       /* for channel-filtered recording   */

#include "businfo.hpp"                  /* seq64::businfo & busarray        */
#include "midibus_common.hpp"
#include "mutex.hpp"
#include "user_midi_bus.hpp"

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{
    class event;
    class midibus;
    class sequence;

/**
 *  The class that "supervises" all of the midibus objects?
 */

class mastermidibase
{

    friend class perform;
    friend class midi_alsa_info;

protected:

    /**
     *  The maximum number of busses supported.  Set to c_max_busses
     *  (SEQ64_DEFAULT_BUSS_MAX = 32) for now.
     */

    int m_max_busses;

    /**
     *  MIDI buss announcer.
     */

    midibus * m_bus_announce;

    /**
     *  Encapsulates information about the input busses.
     */

    busarray m_inbus_array;

    /**
     *  Encapsulates information about the output busses.
     */

    busarray m_outbus_array;

    /**
     *  Saves the clock settings obtained from the "rc" (options) file so that
     *  they can be loaded into the mastermidibus once it is created.
     */

    std::vector<clock_e> m_master_clocks;

    /**
     *  Saves the input settings obtained from the "[midi-input] section of
     *  the "rc" (options) file, so that they can be loaded into the
     *  mastermidibus once it is created.  However, these items will be
     *  modified if the actual enumerated input ports do not match the port
     *  read from the "rc" file.
     */

    std::vector<bool> m_master_inputs;

    /**
     *  The ID of the MIDI queue.
     */

    int m_queue;

    /**
     *  Resolution in parts per quarter note.
     */

    int m_ppqn;

    /**
     *  BPM (beats per minute).  We had to lengthen this name; way too easy to
     *  confuse it with "bpm" for "beats per measure".
     */

    midibpm m_beats_per_minute;

    /**
     *  For dumping MIDI input to a sequence for recording.  This value is set
     *  to true when a sequence editor window is open and the user has
     *  clicked the "record MIDI" or "thru MIDI" button.
     */

    bool m_dumping_input;

    /**
     *  Used for the new "stazed" feature of filtering MIDI channels so that
     *  a sequence gets only the channels meant for it.  We want to make this
     *  a run-time, non-legacy option.
     */

    std::vector<sequence *> m_vector_sequence;

    /**
     *  If true, the m_vector_sequence container is used to divert incoming
     *  data to the sequence that has the channel it is meant for.
     */

    bool m_filter_by_channel;

    /**
     *  Points to the sequence object.
     */

    sequence * m_seq;

    /**
     *  The locking mutex.  This object is passed to an automutex object that
     *  lends exception-safety to the mutex locking.
     */

    mutex m_mutex;

public:

    mastermidibase
    (
        int ppqn    = SEQ64_USE_DEFAULT_PPQN,
        midibpm bpm = SEQ64_DEFAULT_BPM             /* c_beats_per_minute */
    );
    virtual ~mastermidibase ();

    /**
     *  Initialize the mastermidibus using the implementation-specific API
     *  function. A return value would be nice.
     *
     * \param ppqn
     *      The PPQN value to which to initialize the master MIDI buss.
     *
     * \param bpm
     *      The beats/minute value to which to initialize the master MIDI
     *      buss.
     */

    virtual void init (int ppqn, midibpm bpm)
    {
        m_ppqn = ppqn;
        m_beats_per_minute = bpm;
        api_init(ppqn, bpm);
    }

    /**
     *  Indicates that we have an announce buss entry to skip when filling in
     *  the device list with "user" entries.  Another potentially equivalent
     *  test is is_input_system_port(bus).
     *
     * \return
     *      Returns true if m_bus_announce is not the null pointer.
     */

    bool announce_bus_exists () const
    {
        return not_nullptr(m_bus_announce);
    }

    /**
     * \getter m_num_out_buses
     */

    int get_num_out_buses () const
    {
        return m_outbus_array.count();
    }

    /**
     * \getter m_num_in_buses
     */

    int get_num_in_buses () const
    {
        return m_inbus_array.count();
    }

    /**
     * \getter m_filter_by_channel
     */

    bool filter_by_channel () const
    {
        return m_filter_by_channel;
    }

    /**
     * \setter m_filter_by_channel
     */

    void filter_by_channel (bool flag)
    {
        m_filter_by_channel = flag;
    }

    /**
     * \getter m_beats_per_minute
     */

    midibpm get_beats_per_minute () const
    {
        return m_beats_per_minute;
    }

    /**
     * \getter m_beats_per_minute
     *      This is a second version.
     */

    midibpm get_bpm () const
    {
        return m_beats_per_minute;
    }

    /**
     * \getter m_ppqn
     */

    int get_ppqn () const
    {
        return m_ppqn;
    }

    /**
     * \getter m_dumping_input
     */

    bool is_dumping () const
    {
        return m_dumping_input;
    }

    /**
     * \getter m_seq
     *      Used only in perform::input_func() when not filtering MIDI input
     *      by channel.
     */

    sequence * get_sequence () const
    {
        return m_seq;
    }

    void start ();
    void stop ();
    void port_start (int client, int port);
    void port_exit (int client, int port);
    void play (bussbyte bus, event * e24, midibyte channel);
    void continue_from (midipulse tick);
    void init_clock (midipulse tick);
    void emit_clock (midipulse tick);
    void sysex (event * event);
    void print () const;
    void flush ();
    void panic ();                                          /* kepler34 func  */
    void set_sequence_input (bool state, sequence * seq);
    void dump_midi_input (event in);                        /* seq32 function */

    std::string get_midi_out_bus_name (bussbyte bus);
    std::string get_midi_in_bus_name (bussbyte bus);

    int poll_for_midi ();
    bool is_more_input ();
    bool get_midi_event (event * in);

    bool set_clock (bussbyte bus, clock_e clock_type);
    bool set_input (bussbyte bus, bool inputing);
    bool get_input (bussbyte bus);
    bool is_input_system_port (bussbyte bus);
    clock_e get_clock (bussbyte bus);

    void set_ppqn (int ppqn);
    void set_beats_per_minute (midibpm bpm);

protected:

    /**
     * \setter m_master_clocks, m_master_inputs.
     *      Used in the perform class to pass the settings read from the "rc"
     *      file to here.  There is an converse function defined below.
     */

    void set_port_statuses
    (
        const std::vector<clock_e> & clocks,
        const std::vector<bool> & inputs
    )
    {
        m_master_clocks = clocks;
        m_master_inputs = inputs;
    }

    /**
     * \getter m_master_clocks, m_master_inputs.
     *      Used in the perform class to pass the settings read from the "rc"
     *      file to here.  There is an converse function defined above.
     */

    void get_port_statuses
    (
        std::vector<clock_e> & clocks,
        std::vector<bool> & inputs
    )
    {
        clocks = m_master_clocks;
        inputs = m_master_inputs;
    }

    clock_e clock (int bus)
    {
        return bus < int(m_master_clocks.size()) ?
            m_master_clocks[bus] : e_clock_off ;    /* e_clock_disable */
    }

    bool input (int bus)
    {
        return bus < int(m_master_inputs.size()) ? m_master_inputs[bus] : false ;
    }

    /**
     *  Initializes and ctivates the busses, in a partly API-dependent manner.
     *  Currently re-implemented only in the rtmidi JACK API.
     */

    virtual bool activate ()
    {
        bool result = m_inbus_array.initialize();
        if (result)
            result = m_outbus_array.initialize();

        return result;
    }

    virtual void api_init (int ppqn, midibpm bpm) = 0;

    /**
     *  Provides MIDI API-specific functionality for the start() function.
     */

    virtual void api_start ()
    {
        // no code for base or portmidi
    }

    /**
     *  Provides MIDI API-specific functionality for the continue_from()
     *  function.
     */

    virtual void api_continue_from (midipulse /* tick */)
    {
        // no code for base or portmidi
    }

    /**
     *  Provides MIDI API-specific functionality for the init_clock()
     *  function.
     */

    virtual void api_init_clock (midipulse /* tick */)
    {
        // no code for base, alsa, or portmidi
    }

    /**
     *  Provides MIDI API-specific functionality for the stop() function.
     */

    virtual void api_stop ()
    {
        // no code for base or portmidi
    }

    /**
     *  Provides MIDI API-specific functionality for the set_ppqn() function.
     */

    virtual void api_set_ppqn (int /* ppqn */)
    {
        // no code for base or portmidi
    }

    /**
     *  Provides MIDI API-specific functionality for the
     *  set_beats_per_minute() function.
     */

    virtual void api_set_beats_per_minute (midibpm /* bpm */)
    {
        // no code for base
    }

    /**
     *  Provides MIDI API-specific functionality for the flush() function.
     */

    virtual void api_flush ()
    {
        // no code for base or portmidi
    }

    /**
     *  Provides MIDI API-specific functionality for the clock() function.
     */

    virtual void api_clock ()
    {
        // no code for base, alsmidi, or portmidi
    }

    virtual void api_port_start (int /* client */, int /* port */)
    {
        // no code for portmidi
    }

    virtual bool api_get_midi_event (event * inev) = 0;
    virtual int api_poll_for_midi ();

/*
 *  So far, there is no need for these API-specific functions.
 *
 *  virtual void api_sysex (event * ev) = 0;
 *  virtual void api_play (bussbyte bus, event * e24, midibyte channel) = 0;
 *  virtual void api_set_clock (bussbyte bus, clock_e clocktype) = 0;
 *  virtual void api_get_clock (bussbyte bus) = 0;
 *  virtual void api_set_input (bussbyte bus, bool inputting) = 0;
 *  virtual void api_get_input (bussbyte bus) = 0;
 */

private:

    bool save_clock (bussbyte bus, clock_e clock);
    bool save_input (bussbyte bus, bool inputing);
#if 0
    void swap ();
#endif

};          // class mastermidibase

}           // namespace seq64

#endif      // SEQ64_MASTERMIDIBASE_HPP

/*
 * mastermidibase.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

