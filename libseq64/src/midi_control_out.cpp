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
 * \updates
 * \license       GNU GPLv2 or above
 *
 * The class contained in this file encapsulates most of the
 * functionality to send feedback to an external control surface in
 * order to reflect the state of sequencer64. This includes updates on
 * the playing and queueing status of the sequences.
 *
 */

#include "midi_control_out.hpp"
#include <sstream>

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{

midi_control_out::midi_control_out ()
 :
    m_master_bus(nullptr),
    m_buss(15),
    m_screenset_offset(0)
{
    event dummy_e;
    for (int i=0; i<32; ++i)
    {
        for (int a=0; a<seq_action_max; ++a)
        {
            m_seq_event[i][a] = dummy_e;
            m_seq_active[i][a] = false;
        }
    }
    for (int i=0; i<action_max; ++i)
    {
        m_event[i] = dummy_e;
        m_event_active[i] = false;
    }
}


std::string seq_action_to_str(midi_control_out::seq_action a)
{
    switch (a)
    {
    case midi_control_out::seq_action_arm: return "ARM";
    case midi_control_out::seq_action_mute: return "MUTE";
    case midi_control_out::seq_action_queue: return "QUEUE";
    case midi_control_out::seq_action_delete: return "DELETE";
    default: return "UNKNOWN";
    }
}


/* TODO: Need to handle screen sets. Since sequences themselves are
 * ignorant about the current screen set, maybe we can centralise this
 * knowledge inside this class, so before sending a sequence event, we
 * check here if the sequence is in the active screen set, otherwise
 * we drop the event. This requires that in the perform class, we do a
 * "repaint" each time the screen set is changed.
 *
 * TODO: For now, the size of the screenset is fixed to 32 in this function.
 *
 * Also, maybe consider adding an option to the config file, making
 * this behavior optional: So either absolute sequence actions (let
 * the receiver do the math...), or sending events relative (modulo)
 * the current screen set.
 *
 */
void midi_control_out::send_seq_event(int seq, seq_action what, bool flush)
{
    // adjust sequence number relative to current screen-set
    seq = seq - m_screenset_offset;

    if (seq < 0 || seq >= 32)
    {
        // outside current screen-set -> ignore
        return;
    } else {
        if (!m_seq_active[seq][what])
        {
            return;
        }
        event ev = m_seq_event[seq][what];
        if (not_nullptr(m_master_bus))
        {
            m_master_bus->play(m_buss, &ev, ev.get_channel());
            if (flush) {
                m_master_bus->flush();
            }
        }
    }
}

/**
 *  Clears all visible sequences by sending "delete" messages for all
 *  sequences ranging from 0 to 31.
 */
void midi_control_out::clear_sequences()
{
    printf ("CLEAR\n");
    for (int seq=0; seq<32; ++seq)
    {
        send_seq_event(seq, midi_control_out::seq_action_delete, false);
    }
    if (not_nullptr(m_master_bus))
    {
        m_master_bus->flush();
    }
}

event midi_control_out::get_seq_event(int seq, seq_action what) const
{
    if (seq < 0 || seq >= 32)
    {
        event dummy_event;
        return dummy_event;
    } else {
        return m_seq_event[seq][what];
    }
}

void midi_control_out::set_seq_event(int seq, seq_action what, event& ev)
{
    // printf("[set_seq_event] %i %i\n", seq, (int)what);
    m_seq_event[seq][what] = ev;
    m_seq_active[seq][what] = true;
}

void midi_control_out::set_seq_event(int seq, seq_action what, int *eva)
{
    if (what >= seq_action_max)
    {
        return;
    }

    // printf("[set_seq_event] %i %i\n", seq, (int)what);
    event ev;
    ev.set_channel(eva[1]);
    ev.set_status(eva[2]);
    ev.set_data(eva[3], eva[4]);
    m_seq_event[seq][what] = ev;
    if (eva[0]) {
      m_seq_active[seq][what] = true;
    } else {
      m_seq_active[seq][what] = false;
    }
}


bool midi_control_out::seq_event_is_active(int seq, seq_action what) const
{
    if (seq < 0 || seq >= 32)
    {
        return false;
    } else {
        return m_seq_active[seq][what];
    }
}

void midi_control_out::send_event(action what)
{
    if (event_is_active(what))
    {
        event ev = m_event[what];
        if (not_nullptr(m_master_bus))
        {
            m_master_bus->play(m_buss, &ev, ev.get_channel());
            m_master_bus->flush();
        }
    }
}

event midi_control_out::get_event(action what) const
{
    if (event_is_active(what))
    {
        return m_event[what];
    } else {
        event dummy_event;
        return dummy_event;
    }
}

std::string midi_control_out::get_event_str(action what) const
{
    if (!event_is_active(what))
    {
        return "[0 0 0 0]";
    } else {
        const event& ev(m_event[what]);
        midibyte d0, d1;
        ev.get_data(d0, d1);
        std::ostringstream str;
        str << "[" << (int)ev.get_channel() << " "
            << (int)ev.get_status() << " "
            << (int)d0 << " "
            << (int)d1 << "]";
        return str.str();
    }
}

void midi_control_out::set_event(action what, event& ev)
{
    if (what < action_max)
    {
        m_event[what] = ev;
        m_event_active[what] = true;
    }
}

void midi_control_out::set_event(action what, int *eva)
{
    if (what >= action_max)
    {
        return;
    }
    if (eva[0])
    {
        // printf("[set_event] %i\n", (int)what);
        event ev;
        ev.set_channel(eva[1]);
        ev.set_status(eva[2]);
        ev.set_data(eva[3], eva[4]);
        m_event[what] = ev;
        m_event_active[what] = true;
    } else {
        m_event_active[what] = false;
    }
}

bool midi_control_out::event_is_active(action what) const
{
    if (what < action_max)
    {
        return m_event_active[what];
    } else {
        return false;
    }
}

}
