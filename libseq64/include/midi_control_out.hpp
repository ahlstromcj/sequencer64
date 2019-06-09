#ifndef SEQ64_MIDI_CONTROL_OUT_HPP
#define SEQ64_MIDI_CONTROL_OUT_HPP

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
 * \file          midi_control_out.hpp
 *
 *  This module declares/defines the class for handling MIDI control
 *  <i>output</i> of the application.
 *
 * \library       sequencer64 application
 * \author        Igor Angst
 * \date          2018-03-28
 * \updates       2019-06-09
 * \license       GNU GPLv2 or above
 *
 * The class contained in this file encapsulates most of the
 * functionality to send feedback to an external control surface in
 * order to reflect the state of sequencer64. This includes updates on
 * the playing and queueing status of the sequences.
 *
 */

#include <vector>                       /* std::vector<>                    */

#include "globals.h"
#include "mastermidibus.hpp"
#include "event.hpp"

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{

class perform;

/**
 *  Provides some management support for MIDI control... on output.  Many thanks
 *  to igorangst!
 */

class midi_control_out
{

public:

    /**
     *  Provides the kind of MIDI control event that is sent out.
     *
     * \var action_play
     *      Sequence is playing.
     *
     * \var action_mute
     *      Sequence is muted.
     *
     * \var action_queue
     *      Sequence is queued.
     *
     * \var action_delete
     *      Sequence is deleted from its slot.
     *
     * \var action_max
     *      Marker for the maximum value of actions.
     */

    typedef enum
    {
        seq_action_arm     = 0,
        seq_action_mute    = 1,
        seq_action_queue   = 2,
        seq_action_delete  = 3,
        seq_action_max     = 4

    } seq_action;

    /**
     *  Provides codes for various other actions.
     */

    typedef enum
    {
        action_play          = 0,
        action_stop          = 1,
        action_pause         = 2,
        action_queue_on      = 3,
        action_queue_off     = 4,
        action_oneshot_on    = 5,
        action_oneshot_off   = 6,
        action_replace_on    = 7,
        action_replace_off   = 8,
        action_snap1_store   = 9,
        action_snap1_restore = 10,
        action_snap2_store   = 11,
        action_snap2_restore = 12,
        action_learn_on      = 13,
        action_learn_off     = 14,
        action_max           = 15

    } action;

    /**
     *  Manifest constants for optionsfile to use as array indices.
     */

    typedef enum
    {
        out_enabled = 0,
        out_channel = 1,
        out_status  = 2,
        out_data_1  = 3,
        out_data_2  = 4,
        out_max     = 5

    } out_index;

    /**
     *  Provides a type to hold a MIDI-control-out event and its status.
     */

    typedef struct
    {
        event apt_action_event;
        bool apt_action_status;

    } action_pair_t;

    /**
     *
     */

    typedef std::vector<action_pair_t> actions; // [seq_action_max];

    /**
     *  Provides a type for a vector of action pairs, which can be essentially
     *  unlimited in size.
     */

    typedef std::vector<actions> action_list;

private:

    /**
     *  Provides the MIDI output master bus.
     */

    mastermidibus * m_master_bus;

    /**
     *  Provides the MIDI output buss, that is the port number for MIDI output.
     *  Currently, this value is hard-wired to 15, and the user must be sure to
     *  avoid using this buss value for music.
     */

    bussbyte m_buss;                    /* SEQ64_MIDI_CONTROL_OUT_BUSS      */

    /**
     *  Provides the events to be sent out for sequence status changes.
     */

    action_list m_seq_events;

    /**
     *  Provides the events to be sent out for non-sequence actions.
     */

    action_pair_t m_event[action_max];

    /**
     *  True if the respective action is active (i.e. has been set in
     *  the configuration file).
     */

    /**
     *  Holds the screenset size, to use rather than calling the container.
     */

    int m_screenset_size;

    /**
     *  Current screen set offset. Since the sequences dispatch the output
     *  messages, and sequences don't know about screen-sets, we need to do the
     *  math in this class in order to send screen-set relative events out to
     *  external controllers. For now, the size of the screen-set is hard-wired
     *  to 32.
     *
     *  TODO: Make this behavior configurable via optionsfile
     */

    int m_screenset_offset;

public:

    midi_control_out ();

    void initialize (int count, int buss = SEQ64_MIDI_CONTROL_OUT_BUSS);

    void set_master_bus (mastermidibus * mmbus)
    {
        m_master_bus = mmbus;
    }

    bussbyte buss () const
    {
        return m_buss;
    }

    int screenset_size () const
    {
        return m_screenset_size;
    }

    /**
     *  Set the current screen-set offset
     *
     * \param offset
     *      New screen-set offset
     */

    void set_screenset_offset (int offset)
    {
        m_screenset_offset = offset;
    }

    /**
     * Send out notification about playing status of a sequence.
     *
     * \param seq
     *      The index of the sequence
     *
     * \param what
     *      The status action of the sequence
     *
     * \param flush
     *      Flush MIDI buffer after sending (default true)
     */

    void send_seq_event (int seq, seq_action what, bool flush = false);

    /**
     *  Clears all visible sequences by sending "delete" messages for all
     *  sequences ranging from 0 to 31.
     */

    void clear_sequences();

    /**
     * Getter for sequence action events.
     *
     * \param seq
     *      The index of the sequence.
     *
     * \param what
     *      The action to be notified.
     *
     * \return
     *      The MIDI event to be sent.
     */

    event get_seq_event (int seq, seq_action what) const;

    /**
     * Register a MIDI event for a given sequence action.
     *
     * \param seq
     *      The index of the sequence.
     *
     * \param what
     *      The action to be notified.
     *
     * \param ev
     *      The MIDI event to be sent.
     */

    void set_seq_event (int seq, seq_action what, event & ev);

    /**
     * Register a MIDI event for a given sequence action.
     *
     * \param seq
     *      The index of the sequence.
     *
     * \param what
     *      The action to be notified.
     *
     * \param ev
     *      Raw int array representing The MIDI event to be sent.
     */

    void set_seq_event (int seq, seq_action what, int * ev);

    /**
     * Checks if a sequence status event is active.
     *
     * \param seq
     *      The index of the sequence.
     *
     * \param what
     *      The action to be notified.
     *
     * \return
     *      Returns true if the respective event is active.
     */

    bool seq_event_is_active (int seq, seq_action what) const;

    /**
     * Send out notification about non-sequence actions.
     *
     * \param what
     *      The action to be notified.
     */

    void send_event (action what);

    /**
     * Getter for non-sequence action events.
     *
     * \param what
     *      The action to be notified.
     *
     * \return
     *      The MIDI event to be sent.
     */

    event get_event (action what) const;

    /**
     * Getter for non-sequence action events.
     *
     * \param what
     *      The action to be notified.
     *
     * \return
     *      The MIDI event in a config-compatible string
     */

    std::string get_event_str (action what) const;

    /**
     * Register a MIDI event for a given non-sequence action.
     *
     * \param what
     *      The action to be notified.
     *
     * \param event
     *      The MIDI event to be sent.
     */

    void set_event (action what, event & ev);

    /**
     * Register a MIDI event for a given non-sequence action.
     *
     * \param what
     *      The action to be notified.
     *
     * \param ev
     *      Raw int data representing the MIDI event to be sent.
     */

    void set_event (action what, int * ev);

    /**
     * Checks if an event is active.
     *
     * \param what
     *      The action to be notified.
     *
     * \return
     *      Returns true if the respective event is active.
     */

    bool event_is_active (action what) const;

};          // class midi_control_out

/*
 *  Free functions related to midi_control_out.
 */

extern std::string seq_action_to_string (midi_control_out::seq_action a);
extern std::string action_to_string (midi_control_out::action a);

}           // namespace seq64

#endif      // SEQ64_MIDI_CONTROL_OUT_HPP

/*
 * midi_control_out.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

