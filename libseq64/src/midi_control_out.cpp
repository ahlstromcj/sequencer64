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
 * The class contained in this file encapsulates most of the functionality to
 * send feedback to an external control surface in order to reflect the state of
 * sequencer64. This includes updates on the playing and queueing status of the
 * sequences.
 */

#include <sstream>                      /* std::ostringstream class         */

#include "midi_control_out.hpp"         /* seq64::midi_control_out class    */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{

midi_control_out::midi_control_out ()
 :
    m_master_bus        (nullptr),
    m_buss              (SEQ64_MIDI_CONTROL_OUT_BUSS),
    m_seq_events        (),             /* [32][seq_action_max] vector      */
    m_events            (),             /* [action_max] vector              */
    m_is_blank          (true),
    m_screenset_size    (0),
    m_screenset_offset  (0)
{
    initialize(SEQ64_DEFAULT_SET_SIZE);
}

/**
 *  Reinitializes an empty set of MIDI-control-out values.  It first clears any
 *  existing values from the vectors.
 *
 *  Next, it loads an action-pair with "empty" values.  It the creates an array
 *  of these pairs.
 *
 *  Finally, it pushes the desired number of action-pair arrays into an
 *  action_list, which is, for example, a vector of 32 elements, each containing
 *  4 pairs of event + status.  A vector of vector of pairs.
 *
 * \param count
 *      The number of controls to allocate.  Normally, this is 32, but larger
 *      values can now be handled.
 *
 * \param buss
 *      The buss number, which can range from 0 to 31, and defaults to
 *      SEQ64_MIDI_CONTROL_OUT_BUSS (15).
 */

void
midi_control_out::initialize (int count, int buss)
{
    event dummy_event;
    actions actionstemp;
    action_pair_t apt;
    dummy_event.set_status(0);
    dummy_event.set_channel(0);
    apt.apt_action_event = dummy_event;
    apt.apt_action_status = false;
    m_seq_events.clear();
    m_is_blank = true;
    if (count > 0)
    {
        if (buss >= 0 && buss < SEQ64_DEFAULT_BUSS_MAX)
            m_buss = bussbyte(buss);

        m_screenset_size = count;
        for (int a = 0; a < seq_action_max; ++a)
            actionstemp.push_back(apt);     /* a blank action-pair vector */

        for (int c = 0; c < count; ++c)
            m_seq_events.push_back(actionstemp);

        for (int a = 0; a < action_max; ++a)
            m_events[a] = apt;
    }
    else
        m_screenset_size = 0;
}

/**
 *  A "to_string" function for the seq_action enumeration.
 */

std::string
seq_action_to_string (midi_control_out::seq_action a)
{
    switch (a)
    {
    case midi_control_out::seq_action_arm:
        return "arm";

    case midi_control_out::seq_action_mute:
        return "mute";

    case midi_control_out::seq_action_queue:
        return "queue";

    case midi_control_out::seq_action_delete:
        return "delete";

    default:
        return "unknown";
    }
}

/**
 *  A "to_string" function for the action enumeration.
 */

std::string
action_to_string (midi_control_out::action a)
{
    switch (a)
    {
    case midi_control_out::action_play:
        return "play";

    case midi_control_out::action_stop:
        return "stop";

    case midi_control_out::action_pause:
        return "pause";

    case midi_control_out::action_queue_on:
        return "queue on";

    case midi_control_out::action_queue_off:
        return "queue off";

    case midi_control_out::action_oneshot_on:
        return "oneshot on";

    case midi_control_out::action_oneshot_off:
        return "oneshot off";

    case midi_control_out::action_replace_on:
        return "replace on";

    case midi_control_out::action_replace_off:
        return "replace off";

    case midi_control_out::action_snap1_store:
        return "snap1 store";

    case midi_control_out::action_snap1_restore:
        return "snap1 restore";

    case midi_control_out::action_snap2_store:
        return "snap2 store";

    case midi_control_out::action_snap2_restore:
        return "snap2 restore";

    case midi_control_out::action_learn_on:
        return "learn on";

    case midi_control_out::action_learn_off:
        return "learn off";

    default:
        return "unknown";
    }
}

/**
 * \todo
 *      Need to handle screen sets. Since sequences themselves are ignorant about
 *      the current screen set, maybe we can centralise this knowledge inside
 *      this class, so before sending a sequence event, we check here if the
 *      sequence is in the active screen set, otherwise we drop the event. This
 *      requires that in the perform class, we do a "repaint" each time the
 *      screen set is changed.  For now, the size of the screenset is fixed to 32
 *      in this function.
 *
 * Also, maybe consider adding an option to the config file, making this behavior
 * optional: So either absolute sequence actions (let the receiver do the
 * math...), or sending events relative (modulo) the current screen set.
 */

void
midi_control_out::send_seq_event (int seq, seq_action what, bool flush)
{
    seq -= m_screenset_offset;      // adjust relative to current screen-set
    if (seq >= 0 && seq < screenset_size())
    {
        if (m_seq_events[seq][what].apt_action_status)
        {
            event ev = m_seq_events[seq][what].apt_action_event;
            if (not_nullptr(m_master_bus))
            {
#ifdef PLATFORM_DEBUG_TMI
                std::string act = seq_action_to_string(what);
                std::string evstring = to_string(ev);
                printf
                (
                    "send_seq_event(%s): %s\n", act.c_str(), evstring.c_str()
                );
#endif
                m_master_bus->play(m_buss, &ev, ev.get_channel());
                if (flush)
                    m_master_bus->flush();
            }
        }
    }
}

/**
 *  Clears all visible sequences by sending "delete" messages for all
 *  sequences ranging from 0 to 31.
 */

void
midi_control_out::clear_sequences ()
{
    for (int seq = 0; seq < screenset_size(); ++seq)
        send_seq_event(seq, midi_control_out::seq_action_delete, false);

    if (not_nullptr(m_master_bus))
        m_master_bus->flush();
}

/**
 *
 */

event
midi_control_out::get_seq_event (int seq, seq_action what) const
{
    static event s_dummy_event;
    return seq >= 0 && seq < screenset_size() ?
        m_seq_events[seq][what].apt_action_event : s_dummy_event;
}

/**
 *
 */

void
midi_control_out::set_seq_event (int seq, seq_action what, event & ev)
{
    m_seq_events[seq][what].apt_action_event = ev;
    m_seq_events[seq][what].apt_action_status = true;
    m_is_blank = false;
}

/**
 *
 */

void
midi_control_out::set_seq_event (int seq, seq_action what, int * eva)
{
    if (what < seq_action_max)
    {
        event ev;
        ev.set_channel(eva[out_channel]);
        ev.set_status(eva[out_status]);
        ev.set_data(eva[out_data_1], eva[out_data_2]);
        m_seq_events[seq][what].apt_action_event = ev;
        m_seq_events[seq][what].apt_action_status = bool(eva[out_enabled]);
        m_is_blank = false;
    }
}

/**
 *
 */

bool
midi_control_out::seq_event_is_active (int seq, seq_action what) const
{
    return (seq >= 0 && seq < screenset_size()) ?
        m_seq_events[seq][what].apt_action_status : false ;
}

/**
 *
 */

void
midi_control_out::send_event (action what)
{
    if (event_is_active(what))
    {
        event ev = m_events[what].apt_action_event;
        if (not_nullptr(m_master_bus))
        {
#ifdef PLATFORM_DEBUG_TMI
                std::string act = action_to_string(what);
                std::string evstring = to_string(ev);
                printf
                (
                    "send_event(%s): %s\n", act.c_str(), evstring.c_str()
                );
#endif
            m_master_bus->play(m_buss, &ev, ev.get_channel());
            m_master_bus->flush();
        }
    }
}

/**
 *
 */

event
midi_control_out::get_event (action what) const
{
    static event s_dummy_event;
    return event_is_active(what) ?
        m_events[what].apt_action_event : s_dummy_event ;
}

/**
 *
 */

std::string
midi_control_out::get_event_str (action what) const
{
    if (what < action_max)              /* not event_is_active(what)!!  */
    {
        event ev(m_events[what].apt_action_event);
        midibyte d0, d1;
        ev.get_data(d0, d1);
        std::ostringstream str;
        str
            << "[" << int(ev.get_channel()) << " "
            << int(ev.get_status()) << " " << int(d0) << " " << int(d1) << "]"
            ;
        return str.str();
    }
    else
        return std::string("[0 0 0 0]");
}

/**
 *
 */

void
midi_control_out::set_event (action what, event & ev)
{
    if (what < action_max)
    {
        m_events[what].apt_action_event = ev;
        m_events[what].apt_action_status = true;
        m_is_blank = false;
    }
}

/**
 *  An overload taking an array of 5 integers.
 *
 * \param what
 *      Provides the action code to be set.
 *
 * \param eva
 *      A pointer to an array of 5 integers.  These are used to see if the event
 *      is enabled, and to provide parameters for reconstructing the event.
 */

void
midi_control_out::set_event (action what, int * eva)
{
    if (what < action_max)
    {
        event ev;
        ev.set_channel(eva[out_channel]);
        ev.set_status(eva[out_status]);
        ev.set_data(eva[out_data_1], eva[out_data_2]);
        m_events[what].apt_action_event = ev;
        m_events[what].apt_action_status = bool(eva[out_enabled]);
    }
}

/**
 *
 */

bool
midi_control_out::event_is_active (action what) const
{
    return what < action_max ?  m_events[what].apt_action_status : false;
}

}           // namespace seq64

/*
 * midi_control_out.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

