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
 * \file          sequence.cpp
 *
 *  This module declares/defines the base class for handling
 *  patterns/sequences.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2015-10-04
 * \license       GNU GPLv2 or above
 *
 */

#include <stdlib.h>

#include "mastermidibus.hpp"
#include "sequence.hpp"

namespace seq64
{


/**
 *  A static clipboard for holding pattern/sequence events.
 */

event_list sequence::m_events_clipboard;

/**
 *  Principal constructor.
 */

sequence::sequence ()
 :
    m_events                    (),
    m_triggers                  (),
    m_trigger_clipboard         (),
    m_events_undo               (),
    m_events_redo               (),
    m_triggers_undo             (),
    m_triggers_redo             (),
    m_iterator_play             (),
    m_iterator_draw             (),
    m_iterator_play_trigger     (),
    m_iterator_draw_trigger     (),
    m_midi_channel              (0),
    m_bus                       (0),
    m_song_mute                 (false),
    m_notes_on                  (0),
    m_masterbus                 (nullptr),
    m_playing_notes             (),             // an array
    m_was_playing               (false),
    m_playing                   (false),
    m_recording                 (false),
    m_quantized_rec             (false),
    m_thru                      (false),
    m_queued                    (false),
    m_trigger_copied            (false),
    m_dirty_main                (true),
    m_dirty_edit                (true),
    m_dirty_perf                (true),
    m_dirty_names               (true),
    m_editing                   (false),
    m_raise                     (false),
    m_name                      (c_dummy),
    m_last_tick                 (0),
    m_queued_tick               (0),
    m_trigger_offset            (0),
    m_length                    (4*c_ppqn),
    m_snap_tick                 (c_ppqn/4),
    m_time_beats_per_measure    (4),
    m_time_beat_width           (4),
    m_rec_vol                   (0),
    m_mutex                     ()
{
    for (int i = 0; i < c_midi_notes; i++)      /* no notes are playing */
        m_playing_notes[i] = 0;
}

/**
 *  A rote destructor.
 */

sequence::~sequence ()
{
    // Empty body
}

/**
 *  Principal assignment operator.  Follows the stock rules for such an
 *  operator, but does a little more then just assign member values.
 *
 * \threadsafe
 */

sequence &
sequence::operator = (const sequence & rhs)
{
    if (this != &rhs)
    {
        automutex locker(m_mutex);
        m_events   = rhs.m_events;
        m_triggers = rhs.m_triggers;
        // m_trigger_clipboard
        // m_events_undo
        // m_events_redo
        // m_triggers_undo
        // m_triggers_redo
        // m_iterator_play
        // m_iterator_draw
        // m_iterator_play_trigger
        // m_iterator_draw_trigger
        m_midi_channel = rhs.m_midi_channel;
        m_bus          = rhs.m_bus;
        // m_song_mute
        // AND A FEW MORE!
        m_masterbus    = rhs.m_masterbus;
        m_name         = rhs.m_name;
        m_length       = rhs.m_length;
        m_playing      = false;
        m_time_beats_per_measure = rhs.m_time_beats_per_measure;
        m_time_beat_width = rhs.m_time_beat_width;
        for (int i = 0; i < c_midi_notes; i++)      /* no notes are playing */
            m_playing_notes[i] = 0;

        zero_markers();                             /* reset */
        verify_and_link();
    }
    // verify_and_link();
    return *this;
}

/**
 *  Returns the number of events stored in m_events.
 *
 * \threadsafe
 */

int
sequence::event_count () const
{
    automutex locker(m_mutex);
    return int(m_events.count());
}

/**
 *  Pushes the list-event into the undo-list.
 *
 * \threadsafe
 */

void
sequence::push_undo ()
{
    automutex locker(m_mutex);
    m_events_undo.push(m_events);
}

/**
 *  If there are items on the undo list, this function pushes the
 *  list-event into the redo-list, puts the top of the undo-list into the
 *  list-event, pops from the undo-list, calls verify_and_link(), and then
 *  calls unselect.
 *
 * \threadsafe
 */

void
sequence::pop_undo ()
{
    automutex locker(m_mutex);
    if (m_events_undo.size() > 0)
    {
        m_events_redo.push(m_events);
        m_events = m_events_undo.top();
        m_events_undo.pop();
        verify_and_link();
        unselect();
    }
}

/**
 *  If there are items on the redo list, this function pushes the
 *  list-event into the undo-list, puts the top of the redo-list into the
 *  list-event, pops from the redo-list, calls verify_and_link(), and then
 *  calls unselect.
 *
 * \threadsafe
 */

void
sequence::pop_redo ()
{
    automutex locker(m_mutex);
    if (m_events_redo.size() > 0)
    {
        m_events_undo.push(m_events);
        m_events = m_events_redo.top();
        m_events_redo.pop();
        verify_and_link();
        unselect();
    }
}

/**
 *  Pushes the list-trigger into the trigger undo-list, then flags each
 *  item in the undo-list as unselected.
 *
 * \threadsafe
 */

void
sequence::push_trigger_undo ()
{
    automutex locker(m_mutex);
    m_triggers_undo.push(m_triggers);
    for
    (
        Triggers::iterator i = m_triggers_undo.top().begin();
        i != m_triggers_undo.top().end(); i++
    )
    {
        i->m_selected = false;
    }
}

/**
 *  If the trigger undo-list has any items, the list-trigger is pushed
 *  9nto the redo list, the top of the undo-list is coped into the
 *  list-trigger, and then pops from the undo-list.
 */

void
sequence::pop_trigger_undo ()
{
    automutex locker(m_mutex);
    if (m_triggers_undo.size() > 0)
    {
        m_triggers_redo.push(m_triggers);
        m_triggers = m_triggers_undo.top();
        m_triggers_undo.pop();
    }
}

/**
 * \setter m_masterbus
 *
 * \threadsafe
 */

void
sequence::set_master_midi_bus (mastermidibus * mmb)
{
    automutex locker(m_mutex);
    m_masterbus = mmb;
}

/**
 * \setter m_time_beats_per_measure
 *
 * \threadsafe
 */

void
sequence::set_bpm (long beats_per_measure)
{
    automutex locker(m_mutex);
    m_time_beats_per_measure = beats_per_measure;
    set_dirty_mp();
}

/**
 * \setter m_time_beat_width
 *
 * \threadsafe
 */

void
sequence::set_bw (long beat_width)
{
    automutex locker(m_mutex);
    m_time_beat_width = beat_width;
    set_dirty_mp();
}

/**
 * \setter m_rec_vol
 *
 * \threadsafe
 */

void
sequence::set_rec_vol (long rec_vol)
{
    automutex locker(m_mutex);
    m_rec_vol = rec_vol;
}

/**
 *  Adds an event to the internal event list in a sorted manner.  Then it
 *  reset the draw-marker and sets the dirty flag.
 *
 *  Currently, when reading a MIDI file (see the midifile module's parse
 *  function), only the main events (notes, after-touch, pitch, program
 *  changes, etc.) are added with this function.  So, we can rely on
 *  reading only playable events into a sequence.
 *
 *  This module (sequencer) adds all of those events as well, but it
 *  can surely add other events.  We should assume that any events
 *  added by sequencer are playable.
 *
 * \threadsafe
 *
 * \warning
 *      This pushing (and, in writing the MIDI file, the popping),
 *      causes events with identical timestamps to be written in
 *      reverse order.  Doesn't affect functionality, but it's puzzling
 *      until one understands what is happening.
 */

void
sequence::add_event (const event * ep)     // TODO: use reference
{
    automutex locker(m_mutex);
    m_events.add(*ep);                  /* post-sorts by time & rank    */
    reset_draw_marker();
    set_dirty();
}

/**
 * \setter m_last_tick
 *
 * \threadsafe
 */

void
sequence::set_orig_tick (long tick)
{
    automutex locker(m_mutex);
    m_last_tick = tick;
}

/**
 * \setter m_queued and m_queued_tick
 *
 *  Toggles the queued flag and sets the dirty-mp flag.  Also calculates
 *  the queued tick based on m_last_tick.
 *
 * \threadsafe
 */

void
sequence::toggle_queued ()
{
    automutex locker(m_mutex);
    set_dirty_mp();
    m_queued = ! m_queued;
    m_queued_tick = m_last_tick - (m_last_tick % m_length) + m_length;
}

/**
 * \setter m_queued
 *
 *  Toggles the queued flag and sets the dirty-mp flag.
 *
 * \threadsafe
 */

void
sequence::off_queued ()
{
    automutex locker(m_mutex);
    set_dirty_mp();
    m_queued = false;
}

/**
 *  The play() function dumps notes starting from the given tick, and it
 *  pre-buffers ahead.  This function is called by the sequencer thread,
 *  performance.  The tick comes in as global tick.
 *
 *  It turns the sequence off after we play in this frame.
 *
 * \threadsafe
 */

void
sequence::play (long tick, bool playback_mode)
{
    automutex locker(m_mutex);
    bool trigger_turning_off = false;       /* turns off after frame play */
    long times_played = m_last_tick / m_length;
    long offset_base = times_played * m_length;
    long start_tick = m_last_tick;
    long end_tick = tick;
    long trigger_offset = 0;
    if (m_song_mute)
    {
        set_playing(false);
    }
    else
    {
        if (playback_mode)          /* if using in-sequence on/off triggers */
        {
            bool trigger_state = false;
            long trigger_tick = 0;
            for
            (
                Triggers::iterator i = m_triggers.begin();
                i != m_triggers.end(); i++
            )
            {
                if (i->m_tick_start <= end_tick)
                {
                    trigger_state = true;
                    trigger_tick = i->m_tick_start;
                    trigger_offset = i->m_offset;
                }
                if (i->m_tick_end <= end_tick)
                {
                    trigger_state = false;
                    trigger_tick = i->m_tick_end;
                    trigger_offset = i->m_offset;
                }
                if (i->m_tick_start > end_tick || i->m_tick_end > end_tick)
                    break;
            }

            /* Had triggers in the slice, not equal to current state. */

            if (trigger_state != m_playing)
            {
                if (trigger_state)                  /* we are turning on */
                {
                    if (trigger_tick < m_last_tick)
                        start_tick = m_last_tick;
                    else
                        start_tick = trigger_tick;

                    set_playing(true);
                }
                else
                {
                    end_tick = trigger_tick;        /* on, and turning off */
                    trigger_turning_off = true;
                }
            }
            if (m_triggers.size() == 0 && m_playing)
                set_playing(false);
        }
    }
    set_trigger_offset(trigger_offset);

    long start_tick_offset = (start_tick + m_length - m_trigger_offset);
    long end_tick_offset = (end_tick + m_length - m_trigger_offset);
    if (m_playing)                              /* play the notes in frame */
    {
        event_list::iterator e = m_events.begin();
        while (e != m_events.end())
        {
            event & er = DREF(e);
            if
            (
                (er.get_timestamp() + offset_base) >= (start_tick_offset) &&
                (er.get_timestamp() + offset_base) <= (end_tick_offset)
            )
            {
                put_event_on_bus(&er);
            }
            else if ((er.get_timestamp() + offset_base) >  end_tick_offset)
            {
                break;
            }
            e++;                                    /* advance              */
            if (e == m_events.end())                /* did we hit the end ? */
            {
                e = m_events.begin();
                offset_base += m_length;
            }
        }
    }
    if (trigger_turning_off)            /* if triggers say should turn off  */
        set_playing(false);

    m_last_tick = end_tick + 1;                 /* update for next frame    */
    m_was_playing = m_playing;
}

/**
 *  Resets everything to zero.  This function is used when the sequencer stops.
 *
 * \threadsafe
 */

void
sequence::zero_markers ()
{
    automutex locker(m_mutex);
    m_last_tick = 0;            // m_masterbus->flush( );
}

/**
 *  This function verifies state: all note-ons have an off, and it links
 *  note-offs with their note-ons.
 *
 * \threadsafe
 */

void
sequence::verify_and_link ()
{
    automutex locker(m_mutex);
    m_events.verify_and_link(m_length);
    remove_marked();                    // prune out-of-range events
}

/**
 *  Links a new event.
 *
 * \threadsafe
 */

void
sequence::link_new ()
{
    automutex locker(m_mutex);
    m_events.link_new();
}

/**
 *  A helper function, which does not lock/unlock, so it is unsafe to call
 *  without supplying an iterator from the list-event.
 *
 *  If it's a note off, and that note is currently playing, then send a
 *  note off.
 *
 * \threadunsafe
 */

void
sequence::remove (event_list::iterator i)
{
    event & er = DREF(i);
    if (er.is_note_off() && m_playing_notes[er.get_note()] > 0)
    {
        m_masterbus->play(m_bus, &er, m_midi_channel);
        m_playing_notes[er.get_note()]--;                   // ugh
    }
    m_events.remove(i);                                     // erase(i)
}

/**
 *  A helper function, which does not lock/unlock, so it is unsafe to call
 *  without supplying an iterator from the list-event.
 *
 *  Finds the given event in m_events, and removes the first iterator
 *  matching that.
 *
 * \threadunsafe
 */

void
sequence::remove (event * e)
{
    /**
     * \todo Use find instead in sequence::remove()!
     */

    for (event_list::iterator i = m_events.begin(); i != m_events.end(); i++)
    {
        event & er = DREF(i);
        if (e == &er)                   /* comparing pointers, not values */
        {
            m_events.remove(i);
            return;
        }
    }
}

/**
 *  Removes marked events.  Note how this function handles removing a
 *  value to avoid incrementing a now-invalid iterator.
 *
 * \threadsafe
 */

void
sequence::remove_marked ()
{
    automutex locker(m_mutex);
    event_list::iterator i = m_events.begin();
    while (i != m_events.end())
    {
        if (DREF(i).is_marked())
        {
            event_list::iterator t = i;
            t++;
            remove(i);
            i = t;
        }
        else
            i++;
    }
    reset_draw_marker();
}

/**
 *  Marks the selected events.
 *
 * \threadsafe
 */

void
sequence::mark_selected ()
{
    automutex locker(m_mutex);
    m_events.mark_selected();
    reset_draw_marker();
}

/**
 *  Unpaints all list-events.
 *
 * \threadsafe
 */

void
sequence::unpaint_all ()
{
    automutex locker(m_mutex);
    m_events.unpaint_all();
}

/**
 *  Returns the 'box' of the selected items.
 */

void
sequence::get_selected_box
(
    long & tick_s, int & note_h, long & tick_f, int & note_l
)
{
    automutex locker(m_mutex);
    tick_s = c_maxbeats * c_ppqn;
    tick_f = 0;
    note_h = 0;
    note_l = MIDI_COUNT_MAX;
    event_list::iterator i;
    for (i = m_events.begin(); i != m_events.end(); i++)
    {
        if (DREF(i).is_selected())
        {
            long time = DREF(i).get_timestamp();

            /*
             * Can't check on/off here, it screws up the seqevent
             * selection, which has no "off".
             */

            if (time < tick_s)
                tick_s = time;

            if (time > tick_f)
                tick_f = time;

            int note = DREF(i).get_note();
            if (note < note_l)
                note_l = note;

            if (note > note_h)
                note_h = note;
        }
    }
}

/**
 *  Returns the 'box' of selected items.
 */

void
sequence::get_clipboard_box
(
    long & tick_s, int & note_h, long & tick_f, int & note_l
)
{
    automutex locker(m_mutex);
    tick_s = c_maxbeats * c_ppqn;
    tick_f = 0;
    note_h = 0;
    note_l = MIDI_COUNT_MAX;
    if (m_events_clipboard.count() == 0)
        tick_s = tick_f = note_h = note_l = 0;

    event_list::iterator i;
    for (i = m_events_clipboard.begin(); i != m_events_clipboard.end(); i++)
    {
        long time = DREF(i).get_timestamp();
        if (time < tick_s)
            tick_s = time;

        if (time > tick_f)
            tick_f = time;

        int note = DREF(i).get_note();
        if (note < note_l)
            note_l = note;

        if (note > note_h)
            note_h = note;
    }
}

/**
 *  Counts the selected notes in the event list.
 *
 * \threadsafe
 */

int
sequence::get_num_selected_notes ()
{
    automutex locker(m_mutex);
    return m_events.count_selected_notes();
}

/**
 *  Counts the selected events, with the given status, in the event list.
 *  If the event is a control change (CC), then it must also match the
 *  given CC value.
 *
 * \threadsafe
 */

int
sequence::get_num_selected_events (unsigned char status, unsigned char cc)
{
    automutex locker(m_mutex);
    return m_events.count_selected_events(status, cc);
}

/**
 *  This function selects events in range of tick start, note high, tick end,
 *  and note low.  Returns the number selected.
 *
 * \threadsafe
 */

int
sequence::select_note_events
(
    long a_tick_s, int a_note_h,
    long a_tick_f, int a_note_l, select_action_e a_action
)
{
    int result = 0;
    automutex locker(m_mutex);
    for (event_list::iterator i = m_events.begin(); i != m_events.end(); i++)
    {
        event & er = DREF(i);
        if (er.get_note() <= a_note_h && er.get_note() >= a_note_l)
        {
            long tick_s = 0;            // must be initialized
            long tick_f = 0;            // must be initialized
            if (er.is_linked())
            {
                event * ev = er.get_linked();
                if (er.is_note_off())
                {
                    tick_s = ev->get_timestamp();
                    tick_f = er.get_timestamp();
                }
                if (er.is_note_on())
                {
                    tick_f = ev->get_timestamp();
                    tick_s = er.get_timestamp();
                }

                bool tick_and = (tick_s <= a_tick_f) && (tick_f >= a_tick_s);
                bool tick_or = (tick_s <= a_tick_f) || (tick_f >= a_tick_s);
                if
                (
                    ((tick_s <= tick_f) && tick_and) ||
                    ((tick_s > tick_f) && tick_or)
                )
                {
                    if (a_action == e_select || a_action == e_select_one)
                    {
                        er.select();
                        ev->select();
                        result++;
                        if (a_action == e_select_one)
                            break;
                    }
                    if (a_action == e_is_selected)
                    {
                        if (er.is_selected())
                        {
                            result = 1;
                            break;
                        }
                    }
                    if (a_action == e_would_select)
                    {
                        result = 1;
                        break;
                    }
                    if (a_action == e_deselect)
                    {
                        result = 0;
                        er.unselect();
                        ev->unselect();
                    }
                    if (a_action == e_toggle_selection &&
                            er.is_note_on()) // don't toggle twice
                    {
                        if (er.is_selected())
                        {
                            er.unselect();
                            ev->unselect();
                            result ++;
                        }
                        else
                        {
                            er.select();
                            ev->select();
                            result ++;
                        }
                    }
                    if (a_action == e_remove_one)
                    {
                        remove(&er);
                        remove(ev);
                        reset_draw_marker();
                        result++;
                        break;
                    }
                }
            }
            else
            {
                tick_s = tick_f = er.get_timestamp();
                if (tick_s  >= a_tick_s - 16 && tick_f <= a_tick_f)
                {
                    if (a_action == e_select || a_action == e_select_one)
                    {
                        er.select();
                        result++;
                        if (a_action == e_select_one)
                            break;
                    }
                    if (a_action == e_is_selected)
                    {
                        if (er.is_selected())
                        {
                            result = 1;
                            break;
                        }
                    }
                    if (a_action == e_would_select)
                    {
                        result = 1;
                        break;
                    }
                    if (a_action == e_deselect)
                    {
                        result = 0;
                        er.unselect();
                    }
                    if (a_action == e_toggle_selection)
                    {
                        if (er.is_selected())
                        {
                            er.unselect();
                            result ++;
                        }
                        else
                        {
                            er.select();
                            result ++;
                        }
                    }
                    if (a_action == e_remove_one)
                    {
                        remove(&er);
                        reset_draw_marker();
                        result++;
                        break;
                    }
                }
            }
        }
    }
    return result;
}

/**
 *  Select all events in the given range, and returns the number
 *  selected.  Note that there is also an overloaded version of this
 *  function.
 *
 * \threadsafe
 */

int
sequence::select_events
(
    long tick_s, long tick_f,
    unsigned char status, unsigned char cc, select_action_e action
)
{
    int result = 0;
    automutex locker(m_mutex);
    for (event_list::iterator i = m_events.begin(); i != m_events.end(); i++)
    {
        event & er = DREF(i);
        if
        (
            er.get_status() == status &&
            er.get_timestamp() >= tick_s && er.get_timestamp() <= tick_f
        )
        {
            unsigned char d0, d1;
            er.get_data(&d0, &d1);
            if
            (
                (status == EVENT_CONTROL_CHANGE && d0 == cc) ||
                (status != EVENT_CONTROL_CHANGE)
            )
            {
                if (action == e_select || action == e_select_one)
                {
                    er.select();
                    result++;
                    if (action == e_select_one)
                        break;
                }
                if (action == e_is_selected)
                {
                    if (er.is_selected())
                    {
                        result = 1;
                        break;
                    }
                }
                if (action == e_would_select)
                {
                    result = 1;
                    break;
                }
                if (action == e_toggle_selection)
                {
                    if (er.is_selected())
                        er.unselect();
                    else
                        er.select();
                }
                if (action == e_deselect)
                    er.unselect();

                if (action == e_remove_one)
                {
                    remove(&er);
                    reset_draw_marker();
                    result++;
                    break;
                }
            }
        }
    }
    return result;
}

/**
 *  Selects all events, unconditionally.
 *
 * \threadsafe
 */

void
sequence::select_all ()
{
    automutex locker(m_mutex);
    m_events.select_all();
}

/**
 *  Deselects all events, unconditionally.
 *
 * \threadsafe
 */

void
sequence::unselect ()
{
    automutex locker(m_mutex);
    m_events.unselect_all();
}

/**
 *  Removes and adds reads selected in position.
 */

void
sequence::move_selected_notes (long delta_tick, int delta_note)
{
    automutex locker(m_mutex);
    mark_selected();
    for (event_list::iterator i = m_events.begin(); i != m_events.end(); i++)
    {
        event & er = DREF(i);
        if (er.is_marked())                       /* is it being moved ?    */
        {
            event e = er;                         /* copy event             */
            e.unmark();
            if
            (
                (e.get_note() + delta_note) >= 0 &&
                (e.get_note() + delta_note) < c_num_keys
            )
            {
                bool noteon = e.is_note_on();
                long timestamp = e.get_timestamp() + delta_tick;
                if (timestamp > m_length)
                    timestamp = timestamp - m_length;

                if (timestamp < 0)
                    timestamp = m_length + timestamp;

                if ((timestamp == 0) && !noteon)
                    timestamp = m_length - 2;

                if ((timestamp == m_length) && noteon)
                    timestamp = 0;

                e.set_timestamp(timestamp);
                e.set_note(e.get_note() + delta_note);
                e.select();
                add_event(&e);
            }
        }
    }
    remove_marked();
    verify_and_link();
}

/**
 *  Performs a stretch operation on the selected events.  This should move
 *  a note off event, according to old comments, but it doesn't seem to do
 *  that.  See the grow_selected() function.
 *
 * \threadsafe
 */

void
sequence::stretch_selected (long delta_tick)
{
    automutex locker(m_mutex);
    int first_ev = 0x7fffffff;                  /* timestamp lower limit */
    int last_ev = 0x00000000;                   /* timestamp upper limit */
    for (event_list::iterator i = m_events.begin(); i != m_events.end(); i++)
    {
        event & er = DREF(i);
        if (er.is_selected())
        {
            event * e = &er;
            if (e->get_timestamp() < first_ev)
                first_ev = e->get_timestamp();

            if (e->get_timestamp() > last_ev)
                last_ev = e->get_timestamp();
        }
    }
    int old_len = last_ev - first_ev;
    int new_len = old_len + delta_tick;
    if (new_len > 1)
    {
        float ratio = float(new_len) / float(old_len);
        mark_selected();
        for (event_list::iterator i = m_events.begin(); i != m_events.end(); i++)
        {
            event & er = DREF(i);
            if (er.is_marked())
            {
                event * e = &er;                  /* copy & scale event */
                event new_e = *e;
                new_e.set_timestamp
                (
                    long((e->get_timestamp() - first_ev) * ratio) + first_ev
                );
                new_e.unmark();
                add_event(&new_e);
            }
        }
        remove_marked();
        verify_and_link();
    }
}

/**
 *  Moves note off event.
 *
 * \threadsafe
 */

void
sequence::grow_selected (long delta_tick)
{
    mark_selected();                    /* already locked */
    automutex locker(m_mutex);
    for (event_list::iterator i = m_events.begin(); i != m_events.end(); i++)
    {
        event & er = DREF(i);
        if (er.is_marked() && er.is_note_on() && er.is_linked())
        {
            event * on = &er;
            event * off = er.get_linked();
            long length = off->get_timestamp() + delta_tick;

            /*
             * If timestamp + delta is greater that m_length, we do round
             * robin magic.
             */

            if (length > m_length)
                length = length - m_length;

            if (length < 0)
                length = m_length + length;

            if (length == 0)
                length = m_length - 2;

            on->unmark();
            event e = *off;                         /* copy event */
            e.unmark();
            e.set_timestamp(length);
            add_event(&e);
        }
    }
    remove_marked();
    verify_and_link();
}

/**
 *  Increments events the match the given status and control values.
 *  The supported statuses are:
 *
 *      -   EVENT_NOTE_ON
 *      -   EVENT_NOTE_OFF
 *      -   EVENT_AFTERTOUCH
 *      -   EVENT_CONTROL_CHANGE
 *      -   EVENT_PITCH_WHEEL
 *      -   EVENT_PROGRAM_CHANGE
 *      -   EVENT_CHANNEL_PRESSURE
 *
 * \threadsafe
 */

void
sequence::increment_selected (unsigned char astat, unsigned char /*a_control*/)
{
    automutex locker(m_mutex);
    for (event_list::iterator i = m_events.begin(); i != m_events.end(); i++)
    {
        event & er = DREF(i);
        if (er.is_selected() && er.get_status() == astat)
        {
            if
            (
                astat == EVENT_NOTE_ON || astat == EVENT_NOTE_OFF ||
                astat == EVENT_CONTROL_CHANGE || astat == EVENT_PITCH_WHEEL ||
                astat == EVENT_AFTERTOUCH
            )
            {
                er.increment_data2();
            }
            else if
            (
                astat == EVENT_PROGRAM_CHANGE || astat == EVENT_CHANNEL_PRESSURE
            )
            {
                er.increment_data1();
            }
        }
    }
}

/**
 *  Decrements events the match the given status and control values.
 *  The supported statuses are:
 *
 *      -   EVENT_NOTE_ON
 *      -   EVENT_NOTE_OFF
 *      -   EVENT_AFTERTOUCH
 *      -   EVENT_CONTROL_CHANGE
 *      -   EVENT_PITCH_WHEEL
 *      -   EVENT_PROGRAM_CHANGE
 *      -   EVENT_CHANNEL_PRESSURE
 *
 * \threadsafe
 */

void
sequence::decrement_selected (unsigned char astat, unsigned char /*a_control*/)
{
    automutex locker(m_mutex);
    for (event_list::iterator i = m_events.begin(); i != m_events.end(); i++)
    {
        event & er = DREF(i);
        if (er.is_selected() && er.get_status() == astat)
        {
            if
            (
                astat == EVENT_NOTE_ON || astat == EVENT_NOTE_OFF ||
                astat == EVENT_AFTERTOUCH || astat == EVENT_CONTROL_CHANGE ||
                astat == EVENT_PITCH_WHEEL
            )
            {
                er.decrement_data2();
            }
            else if
            (
                astat == EVENT_PROGRAM_CHANGE || astat == EVENT_CHANNEL_PRESSURE
            )
            {
                er.decrement_data1();
            }
        }
    }
}

/**
 *  Copies the selected events.
 *
 * \threadsafe
 */

void
sequence::copy_selected ()
{
    automutex locker(m_mutex);
    m_events_clipboard.clear();
    for (event_list::iterator i = m_events.begin(); i != m_events.end(); ++i)
    {
#ifdef USE_EVENT_MAP
        if (DREF(i).is_selected())
            m_events_clipboard.add(DREF(i), false);      /* no post-sort */
#else
        if (DREF(i).is_selected())
            m_events_clipboard.push_back(DREF(i));
#endif
    }

//  long first_tick = (*m_events_clipboard.begin()).get_timestamp();

    long first_tick = DREF(m_events_clipboard.begin()).get_timestamp();
    for
    (
        event_list::iterator i = m_events_clipboard.begin();
        i != m_events_clipboard.end(); i++
    )
    {
        DREF(i).set_timestamp(DREF(i).get_timestamp() - first_tick);
    }
}

/**
 *  Pastes the selected notes (and only note events) at the given tick and
 *  the given note value.
 *
 *  I wonder if we can get away with just getting a reference to
 *  m_events_clipboard, rather than copying the whole thing, for speed.
 *
 * \threadsafe
 */

void
sequence::paste_selected (long tick, int note)
{
    automutex locker(m_mutex);
    event_list clipbd = m_events_clipboard;     /* copy clipboard           */
    for (event_list::iterator i = clipbd.begin(); i != clipbd.end(); i++)
        DREF(i).set_timestamp(DREF(i).get_timestamp() + tick);

    event & er = DREF(clipbd.begin());
    if (er.is_note_on() || er.is_note_off())
    {
        int highest_note = 0;
        for (event_list::iterator i = clipbd.begin(); i != clipbd.end(); i++)
        {
            if (DREF(i).get_note() > highest_note)
                highest_note = DREF(i).get_note();
        }
        for (event_list::iterator i = clipbd.begin(); i != clipbd.end(); i++)
        {
            DREF(i).set_note(DREF(i).get_note() - (highest_note - note));
        }
    }
    m_events.merge(clipbd, false);              /* don't presort clipboard  */
    m_events.sort();
    verify_and_link();
    reset_draw_marker();
}

/**
 *  Changes the event data range.  Changes only selected events, if any.
 *
 * \threadsafe
 *
 *  Let t == the current tick value; ts == tick start value; tf == tick
 *  finish value; ds = data start value; df == data finish value; d = the
 *  new data value.
 *
 *  Then
 *
\verbatim
             df (t - ts) + ds (tf - t)
        d = --------------------------
                    tf  -   ts
\endverbatim
 *
 *  If this were an interpolation formula it would be:
 *
\verbatim
                            t -  ts
        d = ds + (df - ds) ---------
                            tf - ts
\endverbatim
 *
 *  Something is not quite right; to be investigated.
 *
 *  \param tick_s
 *      Provides the starting tick value.
 *
 *  \param tick_f
 *      Provides the ending tick value.
 *
 *  \param status
 *      Provides the event status that is to be changed.
 *
 *  \param cc
 *      Provides the event control value.
 *
 *  \param data_s
 *      Provides the starting data value.
 *
 *  \param data_f
 *      Provides the finishing data value.
 */

void
sequence::change_event_data_range
(
    long tick_s, long tick_f,
    unsigned char status, unsigned char cc,
    int data_s, int data_f
)
{
    automutex locker(m_mutex);
    bool have_selection = false;
    if (get_num_selected_events(status, cc))
        have_selection = true;

    for (event_list::iterator i = m_events.begin(); i != m_events.end(); i++)
    {
        unsigned char d0, d1;
        bool set = false;
        event & er = DREF(i);
        er.get_data(&d0, &d1);
        if (status != EVENT_CONTROL_CHANGE && er.get_status() == status)
            set = true;                     /* correct status and not CC     */

        if                                  /* correct status and correct cc */
        (
            status == EVENT_CONTROL_CHANGE &&
            er.get_status() == status &&
            d0 == cc
        )
        {
            set = true;
        }

        long tick = er.get_timestamp();
        if (! (tick >= tick_s && tick <= tick_f))       /* in range? */
            set = false;

        if (have_selection && (!er.is_selected()))    /* in selection? */
            set = false;

        if (set)
        {
            if (tick_f == tick_s)
                tick_f = tick_s + 1;                /* avoid divide by 0 */

            int newdata =
            (
                (tick - tick_s) * data_f + (tick_f - tick) * data_s
            ) / (tick_f - tick_s);

            if (newdata < 0)
                newdata = 0;

            if (newdata >= MIDI_COUNT_MAX)
                newdata = MIDI_COUNT_MAX - 1;

            if (status == EVENT_NOTE_ON)
                d1 = newdata;
            else if (status == EVENT_NOTE_OFF)
                d1 = newdata;
            else if (status == EVENT_AFTERTOUCH)
                d1 = newdata;
            else if (status == EVENT_CONTROL_CHANGE)
                d1 = newdata;
            else if (status == EVENT_PROGRAM_CHANGE)
                d0 = newdata;                           /* d0 == new patch */
            else if (status == EVENT_CHANNEL_PRESSURE)
                d0 = newdata;                           /* d0 == pressure  */
            else if (status == EVENT_PITCH_WHEEL)
                d1 = newdata;

            er.set_data(d0, d1);
        }
    }
}

/**
 *  Adds a note of a given length and  note value, at a given tick
 *  location.  It adds a single note-on / note-off pair.
 *
 *  The a_paint parameter indicates if we care about the painted event,
 *  so then the function runs though the events and deletes the painted
 *  ones that overlap the ones we want to add.
 *
 * \threadsafe
 */

void
sequence::add_note (long tick, long length, int note, bool paint)
{
    automutex locker(m_mutex);
    if (tick >= 0 && note >= 0 && note < c_num_keys)
    {
        bool ignore = false;
        if (paint)                        /* see the banner above */
        {
            for
            (
                event_list::iterator i = m_events.begin();
                i != m_events.end(); i++
            )
            {
                event & er = DREF(i);
                if
                (
                    er.is_painted() &&
                    er.is_note_on() &&
                    er.get_timestamp() == tick
                )
                {
                    if (er.get_note() == note)
                    {
                        ignore = true;
                        break;
                    }
                    er.mark();
                    if (er.is_linked())
                        er.get_linked()->mark();

                    set_dirty();
                }
            }
            remove_marked();
        }
        if (! ignore)
        {
            event e;
            if (paint)
                e.paint();

            e.set_status(EVENT_NOTE_ON);
            e.set_data(note, 100);
            e.set_timestamp(tick);
            add_event(&e);

            e.set_status(EVENT_NOTE_OFF);
            e.set_data(note, 100);
            e.set_timestamp(tick + length);
            add_event(&e);
        }
    }
    verify_and_link();
}

/**
 *  Adds a event of a given status value and data values, at a given tick
 *  location.
 *
 *  The a_paint parameter indicates if we care about the painted event,
 *  so then the function runs though the events and deletes the painted
 *  ones that overlap the ones we want to add.
 *
 * \threadsafe
 */

void
sequence::add_event
(
    long a_tick, unsigned char a_status,
    unsigned char a_d0, unsigned char a_d1, bool a_paint
)
{
    automutex locker(m_mutex);
    if (a_tick >= 0)
    {
        if (a_paint)
        {
            event_list::iterator i;
            for (i = m_events.begin(); i != m_events.end(); i++)
            {
                event & er = DREF(i);
                if (er.is_painted() && er.get_timestamp() == a_tick)
                {
                    er.mark();
                    if (er.is_linked())
                        er.get_linked()->mark();

                    set_dirty();
                }
            }
            remove_marked();
        }
        event e;
        if (a_paint)
            e.paint();                              // ???

        e.set_status(a_status);
        e.set_data(a_d0, a_d1);
        e.set_timestamp(a_tick);
        add_event(&e);
    }
    verify_and_link();
}

/**
 *  Streams the given event.
 *
 * \threadsafe
 */

void
sequence::stream_event (event * ev)
{
    automutex locker(m_mutex);
    ev->mod_timestamp(m_length);              /* adjust the tick */
    if (m_recording)
    {
        if (global_is_pattern_playing)
        {
            add_event(ev);
            set_dirty();
        }
        else
        {
            if (ev->is_note_on())
            {
                push_undo();
                add_note
                (
                    m_last_tick % m_length, m_snap_tick - 2,
                    ev->get_note(), false
                );
                set_dirty();
                m_notes_on++;
            }
            if (ev->is_note_off())
                m_notes_on--;

            if (m_notes_on <= 0)
                m_last_tick += m_snap_tick;
        }
    }
    if (m_thru)
    {
        put_event_on_bus(ev);
    }
    link_new();
    if (m_quantized_rec && global_is_pattern_playing)
    {
        if (ev->is_note_off())
        {
            select_note_events
            (
                ev->get_timestamp(), ev->get_note(),
                ev->get_timestamp(), ev->get_note(), e_select
            );
            quantize_events(EVENT_NOTE_ON, 0, m_snap_tick, 1 , true);
        }
    }
}

/**
 *  Sets the dirty flags for names, main, and performance.
 *
 * \threadunsafe
 */

void
sequence::set_dirty_mp ()
{
    m_dirty_names =  m_dirty_main =  m_dirty_perf = true;
}

/**
 *  Call set_dirty_mp() and then sets the dirty flag for editing.
 *
 * \threadsafe
 */

void
sequence::set_dirty ()
{
    set_dirty_mp();
    m_dirty_edit = true;
}

/**
 *  Returns the value of the dirty names (heh heh) flag, and sets that
 *  flag to false.
 *
 * \threadsafe
 */

bool
sequence::is_dirty_names ()
{
    automutex locker(m_mutex);
    bool result = m_dirty_names;
    m_dirty_names = false;
    return result;
}

/**
 *  Returns the value of the dirty main flag, and sets that flag to false
 *  (i.e. resets it).  This flag signals that a redraw is needed from
 *  recording.
 *
 * \threadsafe
 */

bool
sequence::is_dirty_main ()
{
    automutex locker(m_mutex);
    bool result = m_dirty_main;
    m_dirty_main = false;
    return result;
}

/**
 *  Returns the value of the dirty performance flag, and sets that
 *  flag to false.
 *
 * \threadsafe
 */

bool
sequence::is_dirty_perf ()
{
    automutex locker(m_mutex);
    bool result = m_dirty_perf;
    m_dirty_perf = false;
    return result;
}

/**
 *  Returns the value of the dirty edit flag, and sets that
 *  flag to false.
 *
 * \threadsafe
 */

bool
sequence::is_dirty_edit ()
{
    automutex locker(m_mutex);
    bool result = m_dirty_edit;
    m_dirty_edit = false;
    return result;
}

/**
 *  Plays a note from the piano roll on the main bus on the master MIDI
 *  buss.  It flushes a note to the midibus to preview its sound, used by
 *  the virtual piano.
 *
 * \threadsafe
 */

void
sequence::play_note_on (int a_note)
{
    automutex locker(m_mutex);
    event e;
    e.set_status(EVENT_NOTE_ON);
    e.set_data(a_note, MIDI_COUNT_MAX-1);
    m_masterbus->play(m_bus, &e, m_midi_channel);
    m_masterbus->flush();
}

/**
 *  Turns off a note from the piano roll on the main bus on the master MIDI
 *  buss.
 *
 * \threadsafe
 */

void
sequence::play_note_off (int a_note)
{
    automutex locker(m_mutex);
    event e;
    e.set_status(EVENT_NOTE_OFF);
    e.set_data(a_note, MIDI_COUNT_MAX-1);
    m_masterbus->play(m_bus, &e, m_midi_channel);
    m_masterbus->flush();
}

/**
 *  Clears the whole list of triggers.
 *
 * \threadsafe
 */

void
sequence::clear_triggers ()
{
    automutex locker(m_mutex);
    m_triggers.clear();
}

/**
 *  Adds a trigger.  If a_state = true, the range is on.
 *  If a_state = false, the range is off.
 *
 *  What is this?
 *
\verbatim
   is      ie
   <      ><        ><        >
   es             ee
   <               >
   XX

   es ee
   <   >
   <>

   es    ee
   <      >
   <    >

   es     ee
   <       >
   <    >
\endverbatim
*/

void
sequence::add_trigger
(
    long a_tick, long a_length, long a_offset, bool a_adjust_offset
)
{
    automutex locker(m_mutex);
    trigger e;
    if (a_adjust_offset)
        e.m_offset = adjust_offset(a_offset);
    else
        e.m_offset = a_offset;

    e.m_selected = false;
    e.m_tick_start = a_tick;
    e.m_tick_end = a_tick + a_length - 1;

    Triggers::iterator i = m_triggers.begin();
    while (i != m_triggers.end())
    {
        if                              /* Is it inside the new one ? erase */
        (
            i->m_tick_start >= e.m_tick_start &&
            i->m_tick_end <= e.m_tick_end
        )
        {
            m_triggers.erase(i);
            i = m_triggers.begin();
            continue;
        }
        else if                         /* Is the event's end inside? */
        (
            i->m_tick_end >= e.m_tick_end &&
            i->m_tick_start <= e.m_tick_end
        )
        {
            i->m_tick_start = e.m_tick_end + 1;
        }
        else if                     /* is the last start inside the new end? */
        (
            i->m_tick_end   >= e.m_tick_start &&
            i->m_tick_start <= e.m_tick_start
        )
        {
            i->m_tick_end = e.m_tick_start - 1;
        }
        ++i;
    }
    m_triggers.push_front(e);
    m_triggers.sort();
}

/**
 *  This function examines each trigger in the trigger list.  If the given
 *  position is between the current trigger's tick-start and tick-end
 *  values, the these values are copied to the start and end parameters,
 *  respectively, and then we exit.
 *
 * \threadsafe
 *
 * \param position
 *      The position to examine.
 *
 * \param start
 *      The destination for the starting tick (m_tick_start) of the
 *      matching trigger.
 *
 * \param end
 *      The destination for the ending tick (m_tick_end) of the
 *      matching trigger.
 *
 * \return
 *      Returns true if a trigger was found whose start/end ticks
 *      contained the position.  Otherwise, false is returned, and the
 *      start and end return parameters should not be used.
 */

bool
sequence::intersectTriggers (long position, long & start, long & end)
{
    automutex locker(m_mutex);
    Triggers::iterator i = m_triggers.begin();
    while (i != m_triggers.end())
    {
        if (i->m_tick_start <= position && position <= i->m_tick_end)
        {
            start = i->m_tick_start;
            end = i->m_tick_end;
            return true;
        }
        ++i;
    }
    return false;
}

/**
 *  This function examines each note in the event list.
 *
 *  If the given position is between the current notes on and off time
 *  values, values, the these values are copied to the start and end
 *  parameters, respectively, the note value is copied to the note
 *  parameter, and then we exit.
 *
 * \threadsafe
 *
 * \param position
 *      The position to examine.
 *
 * \param position_note
 *      I think this is the note value we might be looking for ???
 *
 * \param start
 *      The destination for the starting tick (m_tick_start) of the
 *      matching trigger.
 *
 * \param end
 *      The destination for the ending tick (m_tick_end) of the
 *      matching trigger.
 *
 * \param note
 *      The destination for the note of the matching event.
 *
 * \return
 *      Returns true if a event was found whose start/end ticks
 *      contained the position.  Otherwise, false is returned, and the
 *      start and end return parameters should not be used.
 */

bool
sequence::intersectNotes
(
    long position, long position_note, long & start, long & ender, long & note
)
{
    automutex locker(m_mutex);
    event_list::iterator on = m_events.begin();
    event_list::iterator off = m_events.begin();
    while (on != m_events.end())
    {
        event & eon = DREF(on);
        if (position_note == eon.get_note() && eon.is_note_on())
        {
            off = on;               /* find next "off" event for the note   */
            ++off;
            event & eoff = DREF(off);
            while
            (
                off != m_events.end() &&
                (eon.get_note() != eoff.get_note() || eoff.is_note_on())
            )
            {
                ++off;
            }
            if
            (
                eon.get_note() == eoff.get_note() &&
                eoff.is_note_off() &&
                eon.get_timestamp() <= position &&
                position <= eoff.get_timestamp()
            )
            {
                start = eon.get_timestamp();
                ender = eoff.get_timestamp();
                note = eon.get_note();
                return true;
            }
        }
        ++on;
    }
    return false;
}

/**
 *  This function examines each non-note event in the event list.
 *
 *  If the given
 *  position is between the current trigger's tick-start and tick-end
 *  values, the these values are copied to the start and end parameters,
 *  respectively, and then we exit.
 *
 * \threadsafe
 *
 * \param posstart
 *      The starting position to examine.
 *
 * \param posend
 *      The ending position to examine.
 *
 * \param status
 *      The desired status value.
 *
 * \param start
 *      The destination for the starting tick (m_tick_start) of the
 *      matching trigger.
 *
 * \return
 *      Returns true if a event was found whose start/end ticks
 *      contained the position.  Otherwise, false is returned, and the
 *      start and end return parameters should not be used.
 */

bool
sequence::intersectEvents
(
    long posstart, long posend, long status, long & start
)
{
    automutex locker(m_mutex);
    for (event_list::iterator on = m_events.begin(); on != m_events.end(); on++)
    {
        event & eon = DREF(on);
        if (status == eon.get_status())
        {
            if
            (
                eon.get_timestamp() <= posstart &&
                posstart <= (eon.get_timestamp() + (posend - posstart))
            )
            {
                start = eon.get_timestamp();
                return true;
            }
        }
    }
    return false;
}

/**
 *  Grows a trigger.
 *
 * \threadsafe
 */

void
sequence::grow_trigger (long a_tick_from, long a_tick_to, long a_length)
{
    automutex locker(m_mutex);
    for (Triggers::iterator i = m_triggers.begin(); i != m_triggers.end(); i++)
    {
        /* Find our pair */

        if (i->m_tick_start <= a_tick_from && i->m_tick_end >= a_tick_from)
        {
            long start = i->m_tick_start;
            long end   = i->m_tick_end;
            if (a_tick_to < start)
                start = a_tick_to;

            if ((a_tick_to + a_length - 1) > end)
                end = (a_tick_to + a_length - 1);

            add_trigger(start, end - start + 1, i->m_offset);
            break;
        }
    }
}

/**
 *  Deletes a trigger, that brackets the given tick, from the trigger-list.
 *
 * \threadsafe
 */

void
sequence::del_trigger (long a_tick)
{
    automutex locker(m_mutex);
    Triggers::iterator i = m_triggers.begin();
    while (i != m_triggers.end())
    {
        if (i->m_tick_start <= a_tick && i->m_tick_end >= a_tick)
        {
            m_triggers.erase(i);
            break;
        }
        ++i;
    }
}

/**
 *  Sets m_trigger_offset and wraps it to m_length.
 *
 * \threadsafe
 */

void
sequence::set_trigger_offset (long a_trigger_offset)
{
    automutex locker(m_mutex);
    m_trigger_offset = (a_trigger_offset % m_length);
    m_trigger_offset += m_length;
    m_trigger_offset %= m_length;
}

/**
 *  Splits the trigger given by the parameter into two triggers.  The
 *  original trigger ends 1 tick before the a_split_tick parameter,
 *  and the new trigger starts at a_split_tick and ends where the original
 *  trigger ended.
 *
 *  This is the private overload of split_trigger.
 *
 * \threadsafe
 *
 * \param trig
 *      Provides the original trigger, and also holds the changes made to
 *      that trigger as it is shortened.
 *
 * \param a_split_tick
 *      The position just after where the original trigger will be
 *      truncated, and the new trigger begins.
 */

void
sequence::split_trigger (trigger & trig, long a_split_tick)
{
    automutex locker(m_mutex);
    long new_tick_end   = trig.m_tick_end;
    long new_tick_start = a_split_tick;
    trig.m_tick_end = a_split_tick - 1;

    long length = new_tick_end - new_tick_start;
    if (length > 1)
        add_trigger(new_tick_start, length + 1, trig.m_offset);
}

/**
 *  Splits a trigger.
 *
 *  This is the public overload of split_trigger.
 *
 * \threadsafe
 */

void
sequence::split_trigger (long a_tick)
{
    automutex locker(m_mutex);
    Triggers::iterator i = m_triggers.begin();
    while (i != m_triggers.end())
    {
        /* trigger greater than L and R */

        if (i->m_tick_start <= a_tick && i->m_tick_end >= a_tick)
        {
            long tick = i->m_tick_end - i->m_tick_start;
            tick += 1;
            tick /= 2;
            split_trigger(*i, i->m_tick_start + tick);
            break;
        }
        ++i;
    }
}

/**
 *  Not sure what these diagrams are for yet.
 *
\verbatim
      |...|...|...|...|...|...|...

      0123456789abcdef0123456789abcdef
      [      ][      ][      ][      ][      ][

      [  ][      ][  ][][][][][      ]  [  ][  ]
      0   4       4   0 7 4 2 0         6   2
      0   4       4   0 1 4 6 0         2   6 inverse offset

      [              ][              ][              ]
      [  ][      ][  ][][][][][      ]  [  ][  ]
      0   c       4   0 f c a 8         e   a
      0   4       c   0 1 4 6 8         2   6  inverse offset

      [                              ][
      [  ][      ][  ][][][][][      ]  [  ][  ]
      k   g f c a 8
      0   4       c   g h k m n       inverse offset

      0123456789abcdefghijklmonpq
      ponmlkjihgfedcba9876543210
      0fedcba9876543210fedcba9876543210fedcba9876543210fedcba9876543210
\endverbatim
 *
 */

/**
 *  Adjusts trigger offsets to the length of ???,
 *  for all triggers, and undo triggers.
 *
 * \threadsafe
 */

void
sequence::adjust_trigger_offsets_to_length(long a_new_len)
{
    automutex locker(m_mutex);
    for (Triggers::iterator i = m_triggers.begin(); i != m_triggers.end(); i++)
    {
        i->m_offset = adjust_offset(i->m_offset);
        i->m_offset = m_length - i->m_offset;           // flip

        long inverse_offset = m_length - (i->m_tick_start % m_length);
        long local_offset = (inverse_offset - i->m_offset);
        local_offset %= m_length;

        long inverse_offset_new = a_new_len - (i->m_tick_start % a_new_len);
        long new_offset = inverse_offset_new - local_offset;
        i->m_offset = (new_offset % a_new_len);
        i->m_offset = a_new_len - i->m_offset;
    }
}

/**
 *  Not sure what these diagrams are for yet.
 *
\verbatim

... a
[      ][      ]
...
... a
...

5   7    play
3        offset
8   10   play

X...X...X...X...X...X...X...X...X...X...
L       R
[        ] [     ]  []  orig
[                    ]

        <<
        [     ]    [  ][ ]  [] split on the R marker, shift first
        [     ]        [     ]
        delete middle
        [     ][ ]  []         move ticks
        [     ][     ]

        L       R
        [     ][ ] [     ]  [] split on L
        [     ][             ]

        [     ]        [ ] [     ]  [] increase all after L
        [     ]        [             ]
\endverbatim
 *
 */

/**
 *  Copies triggers to...
 *
 * \threadsafe
 */

void
sequence::copy_triggers (long a_start_tick, long a_distance)
{
    long from_start_tick = a_start_tick + a_distance;
    long from_end_tick = from_start_tick + a_distance - 1;
    automutex locker(m_mutex);
    move_triggers(a_start_tick, a_distance, true);
    for (Triggers::iterator i = m_triggers.begin(); i != m_triggers.end(); i++)
    {
        if
        (
            i->m_tick_start >= from_start_tick &&
            i->m_tick_start <= from_end_tick
        )
        {
            trigger e;
            e.m_offset = i->m_offset;
            e.m_selected = false;
            e.m_tick_start  = i->m_tick_start - a_distance;
            if (i->m_tick_end <= from_end_tick)
                e.m_tick_end = i->m_tick_end - a_distance;

            if (i->m_tick_end > from_end_tick)
                e.m_tick_end = from_start_tick - 1;

            e.m_offset += (m_length - (a_distance % m_length));
            e.m_offset %= m_length;
            if (e.m_offset < 0)
                e.m_offset += m_length;

            m_triggers.push_front(e);
        }
    }
    m_triggers.sort();
}

/**
 *  Moves triggers in the trigger-list.
 *
 * \threadsafe
 */

void
sequence::move_triggers (long a_start_tick, long a_distance, bool a_direction)
{
    long a_end_tick = a_start_tick + a_distance;
    automutex locker(m_mutex);
    for (Triggers::iterator i = m_triggers.begin(); i != m_triggers.end(); i++)
    {
        /* trigger greater than L and R */

        if (i->m_tick_start < a_start_tick && i->m_tick_end > a_start_tick)
        {
            if (a_direction)                            /* forward */
                split_trigger(*i, a_start_tick);
            else                                        /* back    */
                split_trigger(*i, a_end_tick);
        }

        /* triggers on L */

        if (i->m_tick_start < a_start_tick && i->m_tick_end > a_start_tick)
        {
            if (a_direction)                            /* forward */
                split_trigger(*i, a_start_tick);
            else                                        /* back    */
                i->m_tick_end = a_start_tick - 1;
        }

        /* In betweens */

        if
        (
            i->m_tick_start >= a_start_tick &&
            i->m_tick_end <= a_end_tick && ! a_direction
        )
        {
            m_triggers.erase(i);
            i = m_triggers.begin();
        }

        /* triggers on R */

        if (i->m_tick_start < a_end_tick && i->m_tick_end > a_end_tick)
        {
            if (!a_direction)                           /* forward */
                i->m_tick_start = a_end_tick;
        }
    }
    for (Triggers::iterator i = m_triggers.begin(); i != m_triggers.end(); i++)
    {
        if (a_direction)                                /* forward */
        {
            if (i->m_tick_start >= a_start_tick)
            {
                i->m_tick_start += a_distance;
                i->m_tick_end   += a_distance;
                i->m_offset += a_distance;
                i->m_offset %= m_length;
            }
        }
        else                                            /* back    */
        {
            if (i->m_tick_start >= a_end_tick)
            {
                i->m_tick_start -= a_distance;
                i->m_tick_end   -= a_distance;
                i->m_offset += (m_length - (a_distance % m_length));
                i->m_offset %= m_length;
            }
        }
        i->m_offset = adjust_offset(i->m_offset);
    }
}

/**
 *  Gets the selected trigger's start tick.
 *
 * \threadsafe
 */

long
sequence::get_selected_trigger_start_tick ()
{
    long result = -1;
    automutex locker(m_mutex);
    for (Triggers::iterator i = m_triggers.begin(); i != m_triggers.end(); i++)
    {
        if (i->m_selected)
            result = i->m_tick_start;
    }
    return result;
}

/**
 *  Gets the selected trigger's end tick.
 *
 * \threadsafe
 */

long
sequence::get_selected_trigger_end_tick ()
{
    long result = -1;
    automutex locker(m_mutex);
    for (Triggers::iterator i = m_triggers.begin(); i != m_triggers.end(); i++)
    {
        if (i->m_selected)
            result = i->m_tick_end;
    }
    return result;
}

/**
 *  Moves selected triggers as per the given parameters.
 *
\verbatim
          min_tick][0                1][max_tick
                            2
\endverbatim
 *
 *  -   If we are moving the 0, use first as offset.
 *  -   If we are moving the 1, use the last as the offset.
 *  -   If we are moving both (2), use first as offset.
 *
 * \threadsafe
 */

void
sequence::move_selected_triggers_to
(
    long a_tick, bool a_adjust_offset, int a_which
)
{
    automutex locker(m_mutex);
    long min_tick = 0;
    long max_tick = 0x7ffffff;
    Triggers::iterator s = m_triggers.begin();
    for (Triggers::iterator i = m_triggers.begin(); i != m_triggers.end(); i++)
    {
        if (i->m_selected)
        {
            s = i;
            if (i != m_triggers.end() && ++i != m_triggers.end())
                max_tick = i->m_tick_start - 1;

            /* See the list of options in the function banner. */

            long a_delta_tick = 0;
            if (a_which == 1)
            {
                a_delta_tick = a_tick - s->m_tick_end;
                if (a_delta_tick > 0 && (a_delta_tick+s->m_tick_end) > max_tick)
                    a_delta_tick = ((max_tick) - s->m_tick_end);

                if                      /* not past the first */
                (
                    a_delta_tick < 0 &&
                    (
                        a_delta_tick+s->m_tick_end <= (s->m_tick_start+c_ppqn/8)
                    )
                )
                {
                    a_delta_tick = (s->m_tick_start+c_ppqn/8) - s->m_tick_end;
                }
            }
            if (a_which == 0)
            {
                a_delta_tick = a_tick - s->m_tick_start;
                if
                (
                    a_delta_tick < 0 &&
                    (a_delta_tick + s->m_tick_start) < min_tick
                )
                {
                    a_delta_tick = ((min_tick) - s->m_tick_start);
                }

                /* not past last */

                if
                (
                    a_delta_tick > 0 &&
                    (a_delta_tick + s->m_tick_start >= (s->m_tick_end-c_ppqn/8))
                )
                {
                    a_delta_tick = (s->m_tick_end-c_ppqn/8) - s->m_tick_start;
                }
            }
            if (a_which == 2)
            {
                a_delta_tick = a_tick - s->m_tick_start;
                if
                (
                    a_delta_tick < 0 &&
                    (a_delta_tick + s->m_tick_start) < min_tick
                )
                {
                    a_delta_tick = ((min_tick) - s->m_tick_start);
                }
                if
                (
                    a_delta_tick > 0 &&
                    (a_delta_tick + s->m_tick_end) > max_tick
                )
                {
                    a_delta_tick = ((max_tick) - s->m_tick_end);
                }
            }

            if (a_which == 0 || a_which == 2)
                s->m_tick_start += a_delta_tick;

            if (a_which == 1 || a_which == 2)
                s->m_tick_end   += a_delta_tick;

            if (a_adjust_offset)
            {
                s->m_offset += a_delta_tick;
                s->m_offset = adjust_offset(s->m_offset);
            }
            break;
        }
        else
            min_tick = i->m_tick_end + 1;
    }
}

/**
 *  Get the ending value of the last trigger in the trigger-list.
 *
 * \threadsafe
 */

long
sequence::get_max_trigger ()
{
    long result = 0;
    automutex locker(m_mutex);
    if (m_triggers.size() > 0)
        result = m_triggers.back().m_tick_end;

    return result;
}

/**
 *  Adjusts the given offset by mod'ing it with m_length and adding
 *  m_length if needed, and returning the result.
 */

long
sequence::adjust_offset (long a_offset)
{
    a_offset %= m_length;
    if (a_offset < 0)
        a_offset += m_length;

    return a_offset;
}

bool
sequence::get_trigger_state (long a_tick)
{
    automutex locker(m_mutex);
    bool result = false;
    Triggers::iterator i;
    for (i = m_triggers.begin(); i != m_triggers.end(); i++)
    {
        if (i->m_tick_start <= a_tick && i->m_tick_end >= a_tick)
        {
            result = true;
            break;
        }
    }
    return result;
}

bool
sequence::select_trigger (long a_tick)
{
    automutex locker(m_mutex);
    bool result = false;
    Triggers::iterator i;
    for (i = m_triggers.begin(); i != m_triggers.end(); i++)
    {
        if (i->m_tick_start <= a_tick && i->m_tick_end >= a_tick)
        {
            i->m_selected = true;
            result = true;
        }
    }
    return result;
}

/**
 *  Always returns false!
 */

bool
sequence::unselect_triggers ()
{
    automutex locker(m_mutex);
    bool result = false;
    Triggers::iterator i;
    for (i = m_triggers.begin(); i != m_triggers.end(); i++)
        i->m_selected = false;

    return result;
}

void
sequence::del_selected_trigger ()
{
    automutex locker(m_mutex);
    Triggers::iterator i;
    for (i = m_triggers.begin(); i != m_triggers.end(); i++)
    {
        if (i->m_selected)
        {
            m_triggers.erase(i);
            break;
        }
    }
}

void
sequence::cut_selected_trigger ()
{
    copy_selected_trigger();
    del_selected_trigger();
}

void
sequence::copy_selected_trigger ()
{
    automutex locker(m_mutex);
    for (Triggers::iterator i = m_triggers.begin(); i != m_triggers.end(); i++)
    {
        if (i->m_selected)
        {
            m_trigger_clipboard = *i;
            m_trigger_copied = true;
            break;
        }
    }
}

void
sequence::paste_trigger ()
{
    if (m_trigger_copied)
    {
        long length =
            m_trigger_clipboard.m_tick_end - m_trigger_clipboard.m_tick_start + 1;

        add_trigger                     // paste at copy end
        (
            m_trigger_clipboard.m_tick_end + 1, length,
            m_trigger_clipboard.m_offset + length
        );
        m_trigger_clipboard.m_tick_start = m_trigger_clipboard.m_tick_end + 1;
        m_trigger_clipboard.m_tick_end =
            m_trigger_clipboard.m_tick_start + length - 1;

        m_trigger_clipboard.m_offset += length;
        m_trigger_clipboard.m_offset =
            adjust_offset(m_trigger_clipboard.m_offset);
    }
}

/**
 *  This refreshes the play marker to the last tick. It resets the draw marker
 *  so that calls to get_next_note_event() will start from the first event.
 *
 * \threadsafe
 */

void
sequence::reset_draw_marker ()
{
    automutex locker(m_mutex);
    m_iterator_draw = m_events.begin();
}

/**
 * \threadsafe
 */

void
sequence::reset_draw_trigger_marker ()
{
    automutex locker(m_mutex);
    m_iterator_draw_trigger = m_triggers.begin();
}

/**
 * \threadsafe
 */

int
sequence::get_lowest_note_event ()
{
    automutex locker(m_mutex);
    int result = MIDI_COUNT_MAX-1;
    for (event_list::iterator i = m_events.begin(); i != m_events.end(); i++)
    {
        event & er = DREF(i);
        if (er.is_note_on() || er.is_note_off())
            if (er.get_note() < result)
                result = er.get_note();
    }
    return result;
}

/**
 * \threadsafe
 */

int
sequence::get_highest_note_event ()
{
    automutex locker(m_mutex);
    int result = 0;
    for (event_list::iterator i = m_events.begin(); i != m_events.end(); i++)
    {
        event & er = DREF(i);
        if (er.is_note_on() || er.is_note_off())
            if (er.get_note() > result)
                result = er.get_note();
    }
    return result;
}

/**
 *  Each call to seqdata() fills the passed references with a events
 *  elements, and returns true.  When it has no more events, returns a
 *  false.
 */

draw_type
sequence::get_next_note_event
(
    long * a_tick_s, long * a_tick_f,
    int * a_note, bool * a_selected, int * a_velocity
)
{
    draw_type result = DRAW_FIN;
    *a_tick_f = 0;
    while (m_iterator_draw != m_events.end())
    {
        event & drawevent = DREF(m_iterator_draw);
        *a_tick_s   = drawevent.get_timestamp();
        *a_note     = drawevent.get_note();
        *a_selected = drawevent.is_selected();
        *a_velocity = drawevent.get_note_velocity();

        /* note on, so its linked */

        if (drawevent.is_note_on() && drawevent.is_linked())
        {
            *a_tick_f = drawevent.get_linked()->get_timestamp();
            result = DRAW_NORMAL_LINKED;
            m_iterator_draw++;
            return result;
        }
        else if
        (
            drawevent.is_note_on() && (! drawevent.is_linked())
        )
        {
            result = DRAW_NOTE_ON;
            m_iterator_draw++;
            return result;
        }
        else if
        (
            drawevent.is_note_off() && (! drawevent.is_linked())
        )
        {
            result = DRAW_NOTE_OFF;
            m_iterator_draw++;
            return result;
        }
        m_iterator_draw++;  /* keep going until we hit null or find a NoteOn */
    }
    return DRAW_FIN;
}

/**
 *  Get the next event in the event list.  Then set the status and control
 *  character parameters using that event.
 */

bool
sequence::get_next_event (unsigned char * a_status, unsigned char * a_cc)
{
    unsigned char j;
    while (m_iterator_draw != m_events.end())
    {
        event & drawevent = DREF(m_iterator_draw);
        *a_status = drawevent.get_status();
        drawevent.get_data(a_cc, &j);
        m_iterator_draw++;      /* we have a good one; update and return */
        return true;
    }
    return false;
}

/**
 *  Get the next event in the event list that matches the given status and
 *  control character.  Then set the rest of the parameters parameters
 *  using that event.
 */

bool
sequence::get_next_event
(
    unsigned char a_status, unsigned char a_cc,
    long * a_tick, unsigned char * a_D0, unsigned char * a_D1,
    bool * a_selected
)
{
    while (m_iterator_draw != m_events.end())
    {
        event & drawevent = DREF(m_iterator_draw);
        if (drawevent.get_status() == a_status)     /* note on, so linked */
        {
            drawevent.get_data(a_D0, a_D1);
            *a_tick = drawevent.get_timestamp();
            *a_selected = drawevent.is_selected();

            /*
             *  Either we have a control change with the right CC or it's a
             *  different type of event.
             */

            if
            (
                (a_status == EVENT_CONTROL_CHANGE && *a_D0 == a_cc) ||
                (a_status != EVENT_CONTROL_CHANGE)
            )
            {
                m_iterator_draw++; /* we have a good one update and return */
                return true;
            }
        }
        m_iterator_draw++;  /* keep going until we hit null or find a NoteOn */
    }
    return false;
}

/**
 *  Get the next trigger in the trigger list, and set the parameters based
 *  on that trigger.
 */

bool
sequence::get_next_trigger
(
    long * a_tick_on, long * a_tick_off, bool * a_selected, long * a_offset
)
{
    while (m_iterator_draw_trigger != m_triggers.end())
    {
        *a_tick_on  = (*m_iterator_draw_trigger).m_tick_start;
        *a_selected = (*m_iterator_draw_trigger).m_selected;
        *a_offset = (*m_iterator_draw_trigger).m_offset;
        *a_tick_off = (*m_iterator_draw_trigger).m_tick_end;
        m_iterator_draw_trigger++;
        return true;
    }
    return false;
}

void
sequence::remove_all ()
{
    automutex locker(m_mutex);
    m_events.clear();
}

/**
 *  Returns the last tick played, and is used by the editor's idle function.
 */

long
sequence::get_last_tick ()
{
    return (m_last_tick + (m_length - m_trigger_offset)) % m_length;
}

/**
 *  Sets the midibus number to dump to.
 *
 * \threadsafe
 */

void
sequence::set_midi_bus (char a_mb)
{
    automutex locker(m_mutex);
    off_playing_notes();            /* off notes except initial         */
    m_bus = a_mb;
    set_dirty();
}

/**
 *  Sets the length (m_length) and adjusts triggers for it if desired.
 *
 * \threadsafe
 */

void
sequence::set_length (long a_len, bool a_adjust_triggers)
{
    automutex locker(m_mutex);
    bool was_playing = get_playing();
    set_playing(false);             /* turn everything off */
    if (a_len < (c_ppqn / 4))
        a_len = (c_ppqn / 4);

    if (a_adjust_triggers)
        adjust_trigger_offsets_to_length(a_len);

    m_length = a_len;
    verify_and_link();
    reset_draw_marker();
    if (was_playing)                /* start up and refresh */
        set_playing(true);
}

/**
 *  Sets the playing state of this sequence.  When playing, and the
 *  sequencer is running, notes get dumped to the ALSA buffers.
 *
 * \param a_p
 *      Provides the playing status to set.  True means to turn on the
 *      playing, false means to turn it off, and turn off any notes still
 *      playing.
 */

void
sequence::set_playing (bool a_p)
{
    automutex locker(m_mutex);
    if (a_p != get_playing())
    {
        m_playing = a_p;
        if (! a_p)
            off_playing_notes();

        set_dirty();
    }
    m_queued = false;
}

/**
 *  Toggles the playing status of this sequence.
 */

void
sequence::toggle_playing ()
{
    set_playing(! get_playing());
}

/**
 * \setter m_recording and m_notes_on
 *
 * \threadsafe
 */

void
sequence::set_recording (bool a_r)
{
    automutex locker(m_mutex);
    m_recording = a_r;
    m_notes_on = 0;
}

/**
 * \setter m_snap_tick
 *
 * \threadsafe
 */

void
sequence::set_snap_tick (int a_st)
{
    automutex locker(m_mutex);
    m_snap_tick = a_st;
}

/**
 * \setter m_quantized_rec
 *
 * \threadsafe
 */

void
sequence::set_quantized_rec (bool a_qr)
{
    automutex locker(m_mutex);
    m_quantized_rec = a_qr;
}

/**
 * \setter m_thru
 *
 * \threadsafe
 */

void
sequence::set_thru (bool a_r)
{
    automutex locker(m_mutex);
    m_thru = a_r;
}

/**
 *  Sets the sequence name member, m_name.
 */

void
sequence::set_name (char * a_name)
{
    m_name = a_name;
    set_dirty_mp();
}

/**
 *  Sets the sequence name member, m_name.
 */

void
sequence::set_name (const std::string & a_name)
{
    m_name = a_name;
    set_dirty_mp();
}

/**
 *  Sets the m_midi_channel number
 *
 * \threadsafe
 */

void
sequence::set_midi_channel (unsigned char a_ch)
{
    automutex locker(m_mutex);
    off_playing_notes();
    m_midi_channel = a_ch;
    set_dirty();
}

/**
 *  Prints a list of the currently-held events.
 *
 * \threadunsafe
 */

void
sequence::print ()
{
    m_events.print();
}

/**
 *  Prints a list of the currently-held triggers.
 *
 * \threadunsafe
 */

void
sequence::print_triggers()
{
    printf("[%s]\n", m_name.c_str());
    for (Triggers::iterator i = m_triggers.begin(); i != m_triggers.end(); i++)
    {
        printf
        (
            "  tick_start[%ld] tick_end[%ld] off[%ld]\n",
            i->m_tick_start, i->m_tick_end, i->m_offset
        );
    }
}

/**
 *  Takes an event that this sequence is holding, and places it on the midibus.
 *
 * \threadsafe
 */

void
sequence::put_event_on_bus (event * a_e)
{
    automutex locker(m_mutex);
    unsigned char note = a_e->get_note();
    bool skip = false;
    if (a_e->is_note_on())
        m_playing_notes[note]++;

    if (a_e->is_note_off())
    {
        if (m_playing_notes[note] <= 0)
            skip = true;
        else
            m_playing_notes[note]--;
    }
    if (! skip)
        m_masterbus->play(m_bus, a_e,  m_midi_channel);

    m_masterbus->flush();
}

/**
 *  Sends a note-off event for all active notes.
 *
 * \threadsafe
 */

void
sequence::off_playing_notes ()
{
    automutex locker(m_mutex);
    event e;
    for (int x = 0; x < c_midi_notes; x++)
    {
        while (m_playing_notes[x] > 0)
        {
            e.set_status(EVENT_NOTE_OFF);
            e.set_data(x, 0);
            m_masterbus->play(m_bus, &e, m_midi_channel);
            m_playing_notes[x]--;
        }
    }
    m_masterbus->flush();
}

/**
 *  Select all events with the given status, and returns the number
 *  selected.  Note that there is also an overloaded version of this
 *  function.
 *
 * \threadsafe
 *
 * \warning
 *      This used to be a void function, so it just returns 0 for now.
 */

int
sequence::select_events
(
    unsigned char status, unsigned char cc, bool inverse
)
{
    automutex locker(m_mutex);
    unsigned char d0, d1;
    for (event_list::iterator i = m_events.begin(); i != m_events.end(); i++)
    {
        bool set = false;
        event & er = DREF(i);
        er.get_data(&d0, &d1);

        /* correct status and not CC */

        if (status != EVENT_CONTROL_CHANGE && er.get_status() == status)
            set = true;

        /* correct status and correct cc */

        if
        (
            status == EVENT_CONTROL_CHANGE &&
            er.get_status() == status && d0 == cc
        )
        {
            set = true;
        }
        if (set)
        {
            if (inverse)
            {
                if (! er.is_selected())
                    er.select();
                else
                    er.unselect();
            }
            else
                er.select();
        }
    }
    return 0;
}

/**
 *  Transposes notes by the given steps, in accordance with the given
 *  scale.  If the scale value is 0, this is "no scale", which is the
 *  chromatic scale, where all 12 notes, including sharps and flats, are
 *  part of the scale.
 */

void
sequence::transpose_notes (int steps, int scale)
{
    event e;
    event_list transposed_events;
    const int * transpose_table = nullptr;
    automutex locker(m_mutex);
    mark_selected();                                /* mark original notes  */
    if (steps < 0)
    {
        transpose_table = &c_scales_transpose_dn[scale][0];     /* down */
        steps *= -1;
    }
    else
        transpose_table = &c_scales_transpose_up[scale][0];     /* up   */

    for (event_list::iterator i = m_events.begin(); i != m_events.end(); i++)
    {
        event & er = DREF(i);
        if                                          /* is it being moved ?  */
        (
            (er.get_status() ==  EVENT_NOTE_ON ||
                er.get_status() ==  EVENT_NOTE_OFF) && er.is_marked()
        )
        {
            e = er;
            e.unmark();
            int  note = e.get_note();
            bool off_scale = false;
            if (transpose_table[note % OCTAVE_SIZE] == 0)
            {
                off_scale = true;
                note -= 1;
            }
            for (int x = 0; x < steps; ++x)
                note += transpose_table[note % OCTAVE_SIZE];

            if (off_scale)
                note += 1;

            e.set_note(note);
            transposed_events.add(e, false);        /* will sort afterward  */
        }
    }
    remove_marked();                    /* remove original selected notes   */
    m_events.merge(transposed_events);  /* transposed events get presorted  */
    verify_and_link();
}

/*
 * Not deleting the ends, not selected.
 */

void
sequence::quantize_events
(
    unsigned char a_status, unsigned char a_cc,
    long a_snap_tick,  int a_divide, bool a_linked
)
{
    automutex locker(m_mutex);
    unsigned char d0, d1;
    event_list quantized_events;
    mark_selected();
    for (event_list::iterator i = m_events.begin(); i != m_events.end(); i++)
    {
        bool set = false;
        event & er = DREF(i);
        er.get_data(&d0, &d1);

        /* correct status and not CC */

        if (a_status != EVENT_CONTROL_CHANGE && er.get_status() == a_status)
            set = true;

        /* correct status and correct cc */

        if
        (   a_status == EVENT_CONTROL_CHANGE &&
            er.get_status() == a_status && d0 == a_cc
        )
        {
            set = true;
        }
        if (! er.is_marked())
            set = false;

        if (set)
        {
            event e = er;             /* copy event */
            er.select();
            e.unmark();

            long timestamp = e.get_timestamp();
            long timestamp_remander = (timestamp % a_snap_tick);
            long timestamp_delta = 0;
            if (timestamp_remander < a_snap_tick / 2)
                timestamp_delta = - (timestamp_remander / a_divide);
            else
                timestamp_delta = (a_snap_tick - timestamp_remander) / a_divide;
            if ((timestamp_delta + timestamp) >= m_length)
                timestamp_delta = - e.get_timestamp() ;

            e.set_timestamp(e.get_timestamp() + timestamp_delta);
            quantized_events.add(e, false);         // will sort afterward
            if (er.is_linked() && a_linked)
            {
                event f = *er.get_linked();
                f.unmark();
                er.get_linked()->select();
                f.set_timestamp(f.get_timestamp() + timestamp_delta);
                quantized_events.add(f, false);     // will sort afterward
            }
        }
    }
    remove_marked();
    m_events.merge(quantized_events);       // quantized events get presorted
    verify_and_link();
}

/**
 *  This was a <i> global </i> internal function called addListVar().
 *  Let's at least make it a private static member now, and hew to the
 *  naming conventions of this class.
 */

void
sequence::add_list_var (CharList * a_list, long a_var)
{
    long buffer;
    buffer = a_var & 0x7F;

    /*
     * We shift it right 7, and, if there are still set bits, we encode
     * into buffer in reverse order.
     */

    while ((a_var >>= 7))
    {
        buffer <<= 8;
        buffer |= ((a_var & 0x7F) | 0x80);
    }
    for (;;)
    {
        a_list->push_front((char) buffer & 0xFF);
        if (buffer & 0x80)
            buffer >>= 8;
        else
            break;
    }
}

/**
 *  This was a <i> global </i> internal function called addLongList().
 *  Let's at least make it a private static member now, and hew to the
 *  naming conventions of this class.
 */

void
sequence::add_long_list (CharList * a_list, long a_x)
{
    a_list->push_front((a_x & 0xFF000000) >> 24);
    a_list->push_front((a_x & 0x00FF0000) >> 16);
    a_list->push_front((a_x & 0x0000FF00) >> 8);
    a_list->push_front((a_x & 0x000000FF));
}

/**
 *  This function fills the given character list with MIDI data from the
 *  current sequence, preparatory to writing it to a file.
 *
 *  Note that some of the events might not come out in the same order they
 *  were stored in (we see that with program-change events.
 */

void
sequence::fill_list (CharList * a_list, int a_pos)
{
    automutex locker(m_mutex);
    *a_list = CharList();                               /* copy empty list  */
    add_list_var(a_list, 0);                            /* sequence number  */
    a_list->push_front(char(0xFF));
    a_list->push_front(0x00);
    a_list->push_front(0x02);
    a_list->push_front(char((a_pos & 0xFF00) >> 8));
    a_list->push_front(char(a_pos & 0x00FF));
    add_list_var(a_list, 0);                            /* track name       */
    a_list->push_front(char(0xFF));
    a_list->push_front(0x03);

    int length =  m_name.length();
    if (length > 0x7F)
        length = 0x7f;

    a_list->push_front(length);
    for (int i = 0; i < length; i++)
        a_list->push_front(m_name.c_str()[i]);

    long timestamp = 0;
    long delta_time = 0;
    long prev_timestamp = 0;
    for (event_list::iterator i = m_events.begin(); i != m_events.end(); i++)
    {
        event & er = DREF(i);
        const event & e = er;
        timestamp = e.get_timestamp();
        delta_time = timestamp - prev_timestamp;
        prev_timestamp = timestamp;
        add_list_var(a_list, delta_time);             /* encode delta_time  */

        /* timestamp is encoded, now do the status and data */

        unsigned char d0 = e.m_data[0];
        unsigned char d1 = e.m_data[1];
        a_list->push_front(e.m_status | m_midi_channel);    /* add channel */
        switch (e.m_status & 0xF0)
        {
        case 0x80:
        case 0x90:
        case 0xA0:
        case 0xB0:
        case 0xE0:

            a_list->push_front(d0);
            a_list->push_front(d1);
            break;

        case 0xC0:
        case 0xD0:

            a_list->push_front(d0);
            break;

        default:
            break;
        }
    }

    int num_triggers = m_triggers.size();
    add_list_var(a_list, 0);
    a_list->push_front(char(0xFF));
    a_list->push_front(char(0x7F));
    add_list_var(a_list, (num_triggers * 3 * 4) + 4);
    add_long_list(a_list, c_triggers_new);

    Triggers::iterator t = m_triggers.begin();
    for (int i = 0; i < num_triggers; i++)
    {
        add_long_list(a_list, t->m_tick_start);
        add_long_list(a_list, t->m_tick_end);
        add_long_list(a_list, t->m_offset);
        t++;
    }
    add_list_var(a_list, 0);                              /* bus */
    a_list->push_front(char(0xFF));
    a_list->push_front(char(0x7F));
    a_list->push_front(char(0x05));
    add_long_list(a_list, c_midibus);
    a_list->push_front(m_bus);

    add_list_var(a_list, 0);                              /* timesig */
    a_list->push_front(char(0xFF));
    a_list->push_front(char(0x7F));
    a_list->push_front(char(0x06));
    add_long_list(a_list, c_timesig);
    a_list->push_front(m_time_beats_per_measure);
    a_list->push_front(m_time_beat_width);

    add_list_var(a_list, 0);                              /* channel */
    a_list->push_front(char(0xFF));
    a_list->push_front(char(0x7F));
    a_list->push_front(char(0x05));
    add_long_list(a_list, c_midich);
    a_list->push_front(m_midi_channel);

    delta_time = m_length - prev_timestamp;             /* meta track end */
    add_list_var(a_list, delta_time);
    a_list->push_front(char(0xFF));
    a_list->push_front(char(0x2F));
    a_list->push_front(char(0x00));
}

}           // namespace seq64

/*
 * sequence.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
