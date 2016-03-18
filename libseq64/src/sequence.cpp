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
 *  This module declares/defines the base class for handling the data and
 *  management of patterns/sequences.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2016-03-04
 * \license       GNU GPLv2 or above
 *
 *  The functionality of this class also includes handling some of the
 *  operations of pattern triggers.
 */

#include <stdlib.h>

#include "mastermidibus.hpp"
#include "scales.h"
#include "sequence.hpp"

#define SEQ64_DEFAULT_NOTE_VELOCITY     100

namespace seq64
{

/**
 *  A static clipboard for holding pattern/sequence events.
 */

event_list sequence::m_events_clipboard;

/**
 *  Principal constructor.
 */

sequence::sequence (int ppqn)
 :
    m_events                    (),
    m_triggers                  (*this),
    m_events_undo               (),
    m_events_redo               (),
    m_iterator_play             (),
    m_iterator_draw             (),
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
    m_dirty_main                (true),
    m_dirty_edit                (true),
    m_dirty_perf                (true),
    m_dirty_names               (true),
    m_editing                   (false),
    m_raise                     (false),
    m_name                      (),             // c_dummy:  leave it empty
    m_last_tick                 (0),
    m_queued_tick               (0),
    m_trigger_offset            (0),            // needed for record-keeping
    m_maxbeats                  (c_maxbeats),
    m_ppqn                      (0),            // set in constructor body
    m_seq_number                (-1),           // may be set later
    m_length                    (0),            // set in constructor body
    m_snap_tick                 (0),            // set in constructor body
    m_time_beats_per_measure    (4),
    m_time_beat_width           (4),
#ifdef SEQ64_HANDLE_TIMESIG_AND_TEMPO
    m_clocks_per_metronome      (24),
    m_32nds_per_quarter         (8),
    m_us_per_quarter_note       (0),
#endif
    m_rec_vol                   (0),
    m_musical_key               (SEQ64_KEY_OF_C),
    m_musical_scale             (int(c_scale_off)),
    m_background_sequence       (SEQ64_SEQUENCE_LIMIT),
    m_mutex                     ()
{
    m_ppqn = choose_ppqn(ppqn);
    m_length = 4 * m_ppqn;                      /* one bar's worth of ticks */
    m_snap_tick = m_ppqn / 4;
    m_triggers.set_ppqn(m_ppqn);
    m_triggers.set_length(m_length);
    for (int i = 0; i < c_midi_notes; i++)      /* no notes are playing now */
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
 *  A cut-down version of principal assignment operator.
 *  We're replacing that incomplete function (many members are not assigned)
 *  with the more accurately-named partial_assign() function.
 *
 *  It did not assign them all, so we created this partial_assign()
 *  function to do this work, and replaced operator =() with this function in
 *  client code.
 *
 * \threadsafe
 */

void
sequence::partial_assign (const sequence & rhs)
{
    if (this != &rhs)
    {
        automutex locker(m_mutex);
        m_events   = rhs.m_events;
        m_triggers = rhs.m_triggers;
        m_midi_channel = rhs.m_midi_channel;
        m_bus          = rhs.m_bus;
        m_masterbus    = rhs.m_masterbus;           /* a pointer, be aware! */
        m_playing      = false;
        m_name         = rhs.m_name;
        m_ppqn         = rhs.m_ppqn;
        m_length       = rhs.m_length;
        m_time_beats_per_measure = rhs.m_time_beats_per_measure;
        m_time_beat_width = rhs.m_time_beat_width;
        for (int i = 0; i < c_midi_notes; i++)      /* no notes are playing */
            m_playing_notes[i] = 0;

        zero_markers();                             /* reset to tick 0      */
        verify_and_link();
    }
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
 *  Pushes the event-list into the undo-list.
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
 *  event-list into the redo-list, puts the top of the undo-list into the
 *  event-list, pops from the undo-list, calls verify_and_link(), and then
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
 *  event-list into the undo-list, puts the top of the redo-list into the
 *  event-list, pops from the redo-list, calls verify_and_link(), and then
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
 *  Calls triggers::push_undo() with locking.
 *
 * \threadsafe
 */

void
sequence::push_trigger_undo ()
{
    automutex locker(m_mutex);
    m_triggers.push_undo();
}

/**
 *  Calls triggers::pop_undo() with locking.
 */

void
sequence::pop_trigger_undo ()
{
    automutex locker(m_mutex);
    m_triggers.pop_undo();
}

/**
 * \setter m_masterbus
 *
 * \threadsafe
 *
 * \param mmb
 *      Provides a pointer to the master MIDI buss for this sequence.  This
 *      should be a reference.
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
 *
 * \param beatspermeasure
 *      The new setting of the beats-per-bar value.
 */

void
sequence::set_beats_per_bar (int beatspermeasure)
{
    automutex locker(m_mutex);
    m_time_beats_per_measure = beatspermeasure;
    set_dirty_mp();
}

/**
 * \setter m_time_beat_width
 *
 * \threadsafe
 *
 * \param beatwidth
 *      The new setting of the beat width value.
 */

void
sequence::set_beat_width (int beatwidth)
{
    automutex locker(m_mutex);
    m_time_beat_width = beatwidth;
    set_dirty_mp();
}

/**
 * \setter m_rec_vol
 *
 * \threadsafe
 *
 * \param recvol
 *      The new setting of the recording volume setting.
 */

void
sequence::set_rec_vol (int recvol)
{
    automutex locker(m_mutex);
    m_rec_vol = recvol;
}

/**
 * \setter m_last_tick
 *
 * \threadsafe
 */

void
sequence::set_orig_tick (midipulse tick)
{
    automutex locker(m_mutex);
    m_last_tick = tick;
}

/**
 * \setter m_queued and m_queued_tick
 *
 *  Toggles the queued flag and sets the dirty-mp flag.  Also calculates
 *  the queued tick based on m_last_tick.  If m_length is bad (i.e. zero),
 *  then m_queued_tick is set to 0, to avoid an arithmetic error.
 *
 * \threadsafe
 */

void
sequence::toggle_queued ()
{
    automutex locker(m_mutex);
    set_dirty_mp();
    m_queued = ! m_queued;
    m_queued_tick = m_last_tick - mod_last_tick() + m_length;
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
 * \param tick
 *      Provides the current end-tick value.
 *
 * \param playback_mode
 *      Provides how playback is managed.  True indicates that it is
 *      performance/song-editor playback, controlled by the set of patterns
 *      and triggers set up in that editor, and saved with the song in seq24
 *      format.  False indicates that the playback is controlled by the main
 *      windows, in live mode.
 *
 * \threadsafe
 */

void
sequence::play (midipulse tick, bool playback_mode)
{
    automutex locker(m_mutex);
    bool trigger_turning_off = false;       /* turn off after frame play    */
    midipulse start_tick = m_last_tick;     /* modified in triggers::play() */
    midipulse end_tick = tick;              /* ditto !!!                    */
    if (m_song_mute)
    {
        set_playing(false);
    }
    else
    {
        if (playback_mode)                  /* song mode: on/off triggers   */
        {
            /*
             * A return value and side-effects.  Tells us if there's a change
             * in playing status based on triggers, and the ticks that bracket
             * the action.
             */

            trigger_turning_off = m_triggers.play(start_tick, end_tick);
        }
    }

    midipulse start_tick_offset = (start_tick + m_length - m_trigger_offset);
    midipulse end_tick_offset = (end_tick + m_length - m_trigger_offset);
    if (m_playing)                                  /* play notes in frame  */
    {
        midipulse times_played = m_last_tick / m_length;
        midipulse offset_base = times_played * m_length;
        event_list::iterator e = m_events.begin();
        while (e != m_events.end())
        {
            event & er = DREF(e);
            midipulse stamp = er.get_timestamp() + offset_base;
            if (stamp >= start_tick_offset && stamp <= end_tick_offset)
                put_event_on_bus(er);               /* frame still going    */
            else if (stamp > end_tick_offset)
                break;                              /* frame is done        */

            ++e;                                    /* go to next event     */
            if (e == m_events.end())                /* did we hit the end ? */
            {
                e = m_events.begin();               /* yes, start over      */
                offset_base += m_length;            /* for another go at it */
            }
        }
    }
    if (trigger_turning_off)                        /* triggers: "turn off" */
        set_playing(false);

    m_last_tick = end_tick + 1;                     /* for next frame       */
    m_was_playing = m_playing;
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
    remove_marked();                    /* prune out-of-range events    */
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
 *  without supplying an iterator from the event-list.
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
        if (not_nullptr(m_masterbus))
            m_masterbus->play(m_bus, &er, m_midi_channel);

        m_playing_notes[er.get_note()]--;                   // ugh
    }
    m_events.remove(i);                                     // erase(i)
}

/**
 *  A helper function, which does not lock/unlock, so it is unsafe to call
 *  without supplying an iterator from the event-list.
 *
 *  Finds the given event in m_events, and removes the first iterator
 *  matching that.
 *
 * \threadunsafe
 */

void
sequence::remove (event & e)
{
    /**
     * \todo Use find instead in sequence::remove()!
     */

    for (event_list::iterator i = m_events.begin(); i != m_events.end(); i++)
    {
        event & er = DREF(i);
        if (&e == &er)                  /* comparing pointers, not values */
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
 *  Unpaints all events in the event-list.
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
 *  Returns the 'box' of the selected items.  Note the common-code betweem
 *  this function and get_clipboard_box().
 *
 * \threadsafe
 */

void
sequence::get_selected_box
(
    midipulse & tick_s, int & note_h, midipulse & tick_f, int & note_l
)
{
    automutex locker(m_mutex);
    tick_s = m_maxbeats * m_ppqn;
    tick_f = 0;
    note_h = 0;
    note_l = SEQ64_MIDI_COUNT_MAX;
    event_list::iterator i;
    for (i = m_events.begin(); i != m_events.end(); i++)
    {
        if (DREF(i).is_selected())
        {
            midipulse time = DREF(i).get_timestamp();
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
 *  Returns the 'box' of the clipboard items.  Note the common-code betweem
 *  this function and get_selected_box().
 *
 * \threadsafe
 */

void
sequence::get_clipboard_box
(
    midipulse & tick_s, int & note_h, midipulse & tick_f, int & note_l
)
{
    automutex locker(m_mutex);
    tick_s = m_maxbeats * m_ppqn;
    tick_f = 0;
    note_h = 0;
    note_l = SEQ64_MIDI_COUNT_MAX;
    if (m_events_clipboard.count() == 0)
        tick_s = tick_f = note_h = note_l = 0;

    event_list::iterator i;
    for (i = m_events_clipboard.begin(); i != m_events_clipboard.end(); i++)
    {
        midipulse time = DREF(i).get_timestamp();
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
sequence::get_num_selected_notes () const
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
sequence::get_num_selected_events (midibyte status, midibyte cc) const
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
    midipulse a_tick_s, int a_note_h,
    midipulse a_tick_f, int a_note_l, select_action_e a_action
)
{
    int result = 0;
    automutex locker(m_mutex);
    for (event_list::iterator i = m_events.begin(); i != m_events.end(); i++)
    {
        event & er = DREF(i);
        if (er.get_note() <= a_note_h && er.get_note() >= a_note_l)
        {
            midipulse tick_s = 0;            // must be initialized
            midipulse tick_f = 0;            // must be initialized
            if (er.is_linked())
            {
                event * ev = er.get_linked();       // pointer
                if (er.is_note_off())
                {
                    tick_s = ev->get_timestamp();
                    tick_f = er.get_timestamp();
                }
                else if (er.is_note_on())
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
                    if (a_action == e_toggle_selection && er.is_note_on())
                    {
                        if (er.is_selected())       // don't toggle twice
                        {
                            er.unselect();
                            ev->unselect();
                            ++result;
                        }
                        else
                        {
                            er.select();
                            ev->select();
                            ++result;
                        }
                    }
                    if (a_action == e_remove_one)
                    {
                        remove(er);
                        remove(*ev);
                        reset_draw_marker();
                        ++result;
                        break;
                    }
                }
            }
            else
            {
                tick_s = tick_f = er.get_timestamp();
                if (tick_s >= a_tick_s - 16 && tick_f <= a_tick_f)
                {
                    if (a_action == e_select || a_action == e_select_one)
                    {
                        er.select();
                        ++result;
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
                            ++result;
                        }
                        else
                        {
                            er.select();
                            ++result;
                        }
                    }
                    if (a_action == e_remove_one)
                    {
                        remove(er);
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
    midipulse tick_s, midipulse tick_f,
    midibyte status, midibyte cc,
    select_action_e action
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
            midibyte d0, d1;
            er.get_data(d0, d1);
            if (event::is_desired_cc_or_not_cc(status, cc, d0))
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
                    remove(er);
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
 *
 * \param delta_tick
 *      Provides the amount of time to move the selected notes.
 *
 * \param delta_note
 *      Provides the amount of pitch to move the selected notes.
 */

void
sequence::move_selected_notes (midipulse delta_tick, int delta_note)
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
            int new_note = e.get_note() + delta_note;
            if (new_note >= 0 && new_note < c_num_keys)
            {
                bool noteon = e.is_note_on();
                midipulse timestamp = e.get_timestamp() + delta_tick;
                if (timestamp > m_length)
                    timestamp -= m_length;

                /*
                 * We will not even worry about this issue unless it bites us.
                 * Really unlikely in 64-bit code, and even in 32-bit code.
                 * See midibyte.hpp for more discussion.
                 */

                 if (timestamp < 0)
                    timestamp += m_length;

                if ((timestamp == 0) && ! noteon)
                    timestamp = m_length - 2;

                if ((timestamp == m_length) && noteon)
                    timestamp = 0;

                e.set_timestamp(timestamp);
                e.set_note(midibyte(new_note));
                e.select();
                add_event(e);
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
sequence::stretch_selected (midipulse delta_tick)
{
    automutex locker(m_mutex);
    unsigned first_ev = 0x7fffffff;       // INT_MAX /* timestamp lower limit */
    unsigned last_ev = 0x00000000;                   /* timestamp upper limit */
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
    unsigned old_len = last_ev - first_ev;
    unsigned new_len = old_len + delta_tick;
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
                    midipulse(ratio * (e->get_timestamp() - first_ev)) + first_ev
                );
                new_e.unmark();
                add_event(new_e);
            }
        }
        remove_marked();
        verify_and_link();
    }
}

/**
 *  Moves note off event.  If an event is not linked, this function now
 *  ignores the event's timestamp, rather than risk a segfault on a null
 *  pointer.
 *
 * \threadsafe
 *
 * \param delta_tick
 *      An offset for each linked event's timestamp.
 */

void
sequence::grow_selected (midipulse delta_tick)
{
    mark_selected();                    /* already locked inside    */
    automutex locker(m_mutex);
    for (event_list::iterator i = m_events.begin(); i != m_events.end(); i++)
    {
        event & er = DREF(i);
        if (er.is_marked() && er.is_note_on() && er.is_linked())
        {
            midipulse len = delta_tick;
            event * on = &er;
            event * off = er.get_linked();
            if (not_nullptr(off))
            {
                len += off->get_timestamp();
            }
            else
            {
                errprint("grow_selected(): null event link");
            }

            /*
             * If timestamp + delta is greater that m_length, we do round
             * robin magic.  See similar code in move_selected_notes().
             */

            if (len > m_length)
                len -= m_length;

            /*
             * Can this ever happen?
             */

            if (len < 0)
                len += m_length;

            if (len == 0)
                len = m_length - 2;

            on->unmark();
            if (not_nullptr(off))
            {
                /*
                 * Do we really need to copy the event?  Think about it.
                 */

                event e = *off;                         /* copy event */
                e.unmark();
                e.set_timestamp(len);
                add_event(e);
            }
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
sequence::increment_selected (midibyte astat, midibyte /*a_control*/)
{
    automutex locker(m_mutex);
    for (event_list::iterator i = m_events.begin(); i != m_events.end(); i++)
    {
        event & er = DREF(i);
        if (er.is_selected() && er.get_status() == astat)
        {
            if (event::is_two_byte_msg(astat))
                er.increment_data2();
            else if (event::is_one_byte_msg(astat))
                er.increment_data1();
        }
    }
}

/**
 *  Decrements events the match the given status and control values.
 *  The supported statuses are:
 *
 *  -   One-byte messages
 *      -   EVENT_PROGRAM_CHANGE
 *      -   EVENT_CHANNEL_PRESSURE
 *  -   Two-byte messages
 *      -   EVENT_NOTE_ON
 *      -   EVENT_NOTE_OFF
 *      -   EVENT_AFTERTOUCH
 *      -   EVENT_CONTROL_CHANGE
 *      -   EVENT_PITCH_WHEEL
 *
 * \threadsafe
 */

void
sequence::decrement_selected (midibyte astat, midibyte /*a_control*/)
{
    automutex locker(m_mutex);
    for (event_list::iterator i = m_events.begin(); i != m_events.end(); i++)
    {
        event & er = DREF(i);
        if (er.is_selected() && er.get_status() == astat)
        {
            if (event::is_two_byte_msg(astat))
                er.decrement_data2();
            else if (event::is_one_byte_msg(astat))
                er.decrement_data1();
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
#ifdef SEQ64_USE_EVENT_MAP
        if (DREF(i).is_selected())
            m_events_clipboard.add(DREF(i), false);      /* no post-sort */
#else
        if (DREF(i).is_selected())
            m_events_clipboard.push_back(DREF(i));
#endif
    }

    midipulse first_tick = DREF(m_events_clipboard.begin()).get_timestamp();
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
 *  Cuts the selected events.
 *
 * \threadsafe
 */

void
sequence::cut_selected (bool copyevents)
{
    push_undo();
    if (copyevents)
        copy_selected();

    mark_selected();
    remove_marked();
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
sequence::paste_selected (midipulse tick, int note)
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
 * \param tick_s
 *      Provides the starting tick value.
 *
 * \param tick_f
 *      Provides the ending tick value.
 *
 * \param status
 *      Provides the event status that is to be changed.
 *
 * \param cc
 *      Provides the event control value.
 *
 * \param data_s
 *      Provides the starting data value.
 *
 * \param data_f
 *      Provides the finishing data value.
 *
 * \return
 *      Returns true if the data was changed.
 */

bool
sequence::change_event_data_range
(
    midipulse tick_s, midipulse tick_f,
    midibyte status, midibyte cc,
    int data_s, int data_f
)
{
    automutex locker(m_mutex);
    bool result = false;
    bool have_selection = false;
    if (get_num_selected_events(status, cc))
        have_selection = true;

    for (event_list::iterator i = m_events.begin(); i != m_events.end(); i++)
    {
        midibyte d0, d1;
        event & er = DREF(i);
        er.get_data(d0, d1);
        bool match = er.get_status() == status;
        bool good;
        if (status == EVENT_CONTROL_CHANGE)
            good = match && d0 == cc;        /* correct status and correct cc */
        else
            good = match;                    /* correct status and not a cc   */

        midipulse tick = er.get_timestamp();
        if (! (tick >= tick_s && tick <= tick_f))       /* in range?         */
            good = false;

        if (have_selection && ! er.is_selected())       /* in selection?     */
            good = false;

        if (good)
        {
            if (tick_f == tick_s)
                tick_f = tick_s + 1;                    /* avoid divide-by-0 */

            int newdata =
            (
                (tick - tick_s) * data_f + (tick_f - tick) * data_s
            ) / (tick_f - tick_s);

            if (newdata < 0)
                newdata = 0;

            if (newdata >= SEQ64_MIDI_COUNT_MAX)
                newdata = SEQ64_MIDI_COUNT_MAX - 1;

            /*
             * I think we can assume, at this point, that this is a good
             * channel-message status byte.
             */

            if (event::is_one_byte_msg(status))         /* patch or pressure */
                d0 = newdata;
            else
                d1 = newdata;

            er.set_data(d0, d1);
            result = true;
        }
    }
    return result;
}

/**
 *  Adds a note of a given length and  note value, at a given tick
 *  location.  It adds a single note-on / note-off pair.
 *
 *  The paint parameter indicates if we care about the painted event,
 *  so then the function runs though the events and deletes the painted
 *  ones that overlap the ones we want to add.
 *
 * \threadsafe
 */

void
sequence::add_note
(
    midipulse tick,
    midipulse length,
    int note,
    bool paint
)
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
            e.set_data(note, SEQ64_DEFAULT_NOTE_VELOCITY);
            e.set_timestamp(tick);
            add_event(e);

            e.set_status(EVENT_NOTE_OFF);
            e.set_data(note, SEQ64_DEFAULT_NOTE_VELOCITY);
            e.set_timestamp(tick + length);
            add_event(e);
        }
    }
    verify_and_link();
}

/**
 *  Adds an event to the internal event list in a sorted manner.  Then it
 *  reset the draw-marker and sets the dirty flag.
 *
 *  Currently, when reading a MIDI file [see the midifile::parse() function],
 *  only the main events (notes, after-touch, pitch, program changes, etc.)
 *  are added with this function.  So, we can rely on reading only playable
 *  events into a sequence.  Well, actually, certain meta-events are also
 *  read, to obtain channel, buss, and more settings.  Also read for a
 *  sequence, if the global-sequence flag is not set, are the new key, scale,
 *  and background sequence parameters.
 *
 *  This module (sequencer) adds all of those events as well, but it
 *  can surely add other events.  We should assume that any events
 *  added by sequencer are playable/usable.
 *
 * \threadsafe
 *
 * \warning
 *      This pushing (and, in writing the MIDI file, the popping),
 *      causes events with identical timestamps to be written in
 *      reverse order.  Doesn't affect functionality, but it's puzzling
 *      until one understands what is happening.  Actually, this is true only
 *      in Seq24, we've fixed that behavior for Sequencer64.
 *
 * \param er
 *      Provide a reference to the event to be added; the event is copied into
 *      the events container.
 */

bool
sequence::add_event (const event & er)
{
    automutex locker(m_mutex);
    bool result = m_events.add(er);     /* post/auto-sorts by time & rank   */
    if (result)
    {
        reset_draw_marker();
        set_dirty();
    }
    else
    {
        errprint("sequence::add_event(): failed");
    }
    return result;
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
    midipulse tick, midibyte status,
    midibyte d0, midibyte d1, bool paint
)
{
    automutex locker(m_mutex);
    if (tick >= 0)
    {
        if (paint)
        {
            event_list::iterator i;
            for (i = m_events.begin(); i != m_events.end(); i++)
            {
                event & er = DREF(i);
                if (er.is_painted() && er.get_timestamp() == tick)
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
        if (paint)
            e.paint();                              // ???

        e.set_status(status);
        e.set_data(d0, d1);
        e.set_timestamp(tick);
        add_event(e);
    }
    verify_and_link();
}

/**
 *  Streams the given event.
 *
 * \threadsafe
 */

void
sequence::stream_event (event & ev)
{
    automutex locker(m_mutex);
    ev.mod_timestamp(m_length);                 /* adjust the tick */
    if (m_recording)
    {
        if (rc().is_pattern_playing())
        {
            add_event(ev);
            set_dirty();
        }
        else
        {
            if (ev.is_note_on())
            {
                push_undo();
                add_note(mod_last_tick(), m_snap_tick - 2, ev.get_note(), false);
                set_dirty();
                ++m_notes_on;
            }
            if (ev.is_note_off())
                --m_notes_on;

            if (m_notes_on <= 0)
                m_last_tick += m_snap_tick;
        }
    }
    if (m_thru)
        put_event_on_bus(ev);

    link_new();
    if (m_quantized_rec && rc().is_pattern_playing())
    {
        if (ev.is_note_off())
        {
            midipulse timestamp = ev.get_timestamp();
            midibyte note = ev.get_note();
            select_note_events(timestamp, note, timestamp, note, e_select);
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
    m_dirty_names = m_dirty_main = m_dirty_perf = true;
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
    e.set_data(a_note, SEQ64_MIDI_COUNT_MAX-1);
    if (not_nullptr(m_masterbus))
    {
        m_masterbus->play(m_bus, &e, m_midi_channel);
        m_masterbus->flush();
    }
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
    e.set_data(a_note, SEQ64_MIDI_COUNT_MAX-1);
    if (not_nullptr(m_masterbus))
    {
        m_masterbus->play(m_bus, &e, m_midi_channel);
        m_masterbus->flush();
    }
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
 *  Adds a trigger.  A pass-through function that calls triggers::add().
*/

void
sequence::add_trigger
(
    midipulse tick, midipulse len, midipulse offset, bool fixoffset
)
{
    automutex locker(m_mutex);
    m_triggers.add(tick, len, offset, fixoffset);
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
 *      The destination for the starting tick of the matching trigger.
 *
 * \param ender
 *      The destination for the ending tick of the matching trigger.
 *
 * \return
 *      Returns true if a trigger was found whose start/end ticks
 *      contained the position.  Otherwise, false is returned, and the
 *      start and end return parameters should not be used.
 */

bool
sequence::intersect_triggers
(
    midipulse position, midipulse & start, midipulse & ender
)
{
    automutex locker(m_mutex);
    return m_triggers.intersect(position, start, ender);
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
 *      The destination for the starting timestamp of the matching note.
 *
 * \param ender
 *      The destination for the ending timestamp of the matching note.
 *
 * \param note
 *      The destination for the note of the matching event.
 *      Why is this an int value???
 *
 * \return
 *      Returns true if a event was found whose start/end ticks
 *      contained the position.  Otherwise, false is returned, and the
 *      start and end return parameters should not be used.
 */

bool
sequence::intersect_notes
(
    midipulse position, midipulse position_note,
    midipulse & start, midipulse & ender, int & note
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
 *  If the given position is between the current notes's timestamp-start and
 *  timestamp-end values, the these values are copied to the posstart and posend
 *  parameters, respectively, and then we exit.
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
 *      The destination for the starting timestamp  of the matching trigger.
 *
 * \return
 *      Returns true if a event was found whose start/end timestamps
 *      contained the position.  Otherwise, false is returned, and the
 *      start and end return parameters should not be used.
 */

bool
sequence::intersect_events
(
    midipulse posstart, midipulse posend,
    midibyte status, midipulse & start
)
{
    automutex locker(m_mutex);
    midipulse poslength = posend - posstart;
    for (event_list::iterator on = m_events.begin(); on != m_events.end(); ++on)
    {
        event & eon = DREF(on);
        if (status == eon.get_status())
        {
            midipulse ts = eon.get_timestamp();
            if (ts <= posstart && posstart <= (ts + poslength))
            {
                start = eon.get_timestamp();    /* side-effect return value */
                return true;
            }
        }
    }
    return false;
}

/**
 *  Grows a trigger.
 *
 * \param tickfrom
 *      The desired from-value back which to expand the trigger, if necessary.
 *
 * \param tickto
 *      The desired to-value towards which to expand the trigger, if necessary.
 *
 * \param len
 *      The additional length to append to tickto for the check.
 *
 * \threadsafe
 */

void
sequence::grow_trigger (midipulse tickfrom, midipulse tickto, midipulse len)
{
    automutex locker(m_mutex);
    m_triggers.grow(tickfrom, tickto, len);
}

/**
 *  Deletes a trigger, that brackets the given tick, from the trigger-list.
 *
 * \threadsafe
 */

void
sequence::del_trigger (midipulse tick)
{
    automutex locker(m_mutex);
    m_triggers.remove(tick);
}

/**
 *  Sets m_trigger_offset and wraps it to m_length.  If m_length is 0, then
 *  m_trigger_offset is simply set to the parameter.
 *
 * \threadsafe
 *
 * \param trigger_offset
 *      The full trigger offset to set.
 */

void
sequence::set_trigger_offset (midipulse trigger_offset)
{
    automutex locker(m_mutex);
    if (m_length > 0)
    {
        m_trigger_offset = trigger_offset % m_length;
        m_trigger_offset += m_length;
        m_trigger_offset %= m_length;
    }
    else
        m_trigger_offset = trigger_offset;
}

/**
 *  Splits the trigger given by the parameter into two triggers.
 *  This is the private overload of split_trigger.
 *
 * \threadsafe
 *
 * \param trig
 *      Provides the original trigger, and also holds the changes made to
 *      that trigger as it is shortened.
 *
 * \param splittick
 *      The position just after where the original trigger will be
 *      truncated, and the new trigger begins.
 */

void
sequence::split_trigger (trigger & trig, midipulse splittick)
{
    automutex locker(m_mutex);
    m_triggers.split(trig, splittick);
}

/**
 *  Splits a trigger.
 *
 *  This is the public overload of split_trigger.
 *
 * \threadsafe
 */

void
sequence::split_trigger (midipulse splittick)
{
    automutex locker(m_mutex);
    m_triggers.split(splittick);
}

/**
 *  Adjusts trigger offsets to the length of ???,
 *  for all triggers, and undo triggers.
 *
 * \threadsafe
 *
 *  Might can get rid of this function?
 */

void
sequence::adjust_trigger_offsets_to_length (midipulse newlength)
{
    automutex locker(m_mutex);
    m_triggers.adjust_offsets_to_length(newlength);
}

/**
 *  Copies triggers to...
 *
 * \threadsafe
 */

void
sequence::copy_triggers (midipulse starttick, midipulse distance)
{
    automutex locker(m_mutex);
    m_triggers.copy(starttick, distance);
}

/**
 *  Moves triggers in the trigger-list.
 *
 *  Note the dependence on the m_length member being kept in sync with the
 *  parent's value of m_length.
 *
 * \threadsafe
 */

void
sequence::move_triggers (midipulse starttick, midipulse distance, bool direction)
{
    automutex locker(m_mutex);
    m_triggers.move(starttick, distance, direction);    // , m_length);
}

/**
 *  Gets the last-selected trigger's start tick.
 *
 * \threadsafe
 *
 * \return
 *      Returns the tick_start() value of the last-selected trigger.  If no
 *      triggers are selected, then -1 is returned.
 */

midipulse
sequence::selected_trigger_start ()
{
    automutex locker(m_mutex);
    return m_triggers.get_selected_start();
}

/**
 *  Gets the selected trigger's end tick.
 *
 * \threadsafe
 */

midipulse
sequence::selected_trigger_end ()
{
    automutex locker(m_mutex);
    return m_triggers.get_selected_end();
}

/**
 *  Moves selected triggers as per the given parameters.
 *
\verbatim
          min_tick][0                1][max_tick
                            2
\endverbatim
 *
 *  The \a which parameter has three possible values:
 *
 *  -#  If we are moving the 0, use first as offset.
 *  -#  If we are moving the 1, use the last as the offset.
 *  -#  If we are moving both (2), use first as offset.
 *
 * \threadsafe
 *
 * \param tick
 *      The tick at which the trigger starts.
 *
 * \param adjustoffset
 *      Set to true if the offset is to be adjusted.
 *
 * \param which
 *      Selects which movement will be done, as discussed above.
 *
 * \return
 *      Returns the value of triggers::move_selected(), which indicate
 *      that the movement could be made.  Used in
 *      Seq24PerfInput::handle_motion_key().
 */

bool
sequence::move_selected_triggers_to
(
    midipulse tick, bool adjustoffset, int which
)
{
    automutex locker(m_mutex);
    return m_triggers.move_selected(tick, adjustoffset, which);
}

/**
 *  Get the ending value of the last trigger in the trigger-list.
 *
 * \threadsafe
 */

midipulse
sequence::get_max_trigger ()
{
    automutex locker(m_mutex);
    return m_triggers.get_maximum();
}

/**
 *  Checks the list of triggers against the given tick.  If any
 *  trigger is found to bracket that tick, then true is returned.
 *
 *
 * \param tick
 *      Provides the tick of interest.
 *
 * \return
 *      Returns true if a trigger is found that brackets the given tick.
 */

bool
sequence::get_trigger_state (midipulse tick)
{
    automutex locker(m_mutex);
    return m_triggers.get_state(tick);
}

/**
 *  Checks the list of triggers against the given tick.  If any
 *  trigger is found to bracket that tick, then true is returned, and
 *  the trigger is marked as selected.
 *
 * \param tick
 *      Provides the tick of interest.
 *
 * \return
 *      Returns true if a trigger is found that brackets the given tick.
 */

bool
sequence::select_trigger (midipulse tick)
{
    automutex locker(m_mutex);
    return m_triggers.select(tick);
}

/**
 *      Unselects all triggers.
 *
 * \return
 *      Always returns false.
 */

bool
sequence::unselect_triggers ()
{
    automutex locker(m_mutex);
    return m_triggers.unselect();
}

/**
 *      Deletes the first selected trigger that is found.
 */

void
sequence::del_selected_trigger ()
{
    automutex locker(m_mutex);
    m_triggers.remove_selected();
}

/**
 *      Copies and deletes the first selected trigger that is found.
 */

void
sequence::cut_selected_trigger ()
{
    copy_selected_trigger();            /* locks itself */

    automutex locker(m_mutex);
    m_triggers.remove_selected();
}

/**
 *      Copies the first selected trigger that is found.
 */

void
sequence::copy_selected_trigger ()
{
    automutex locker(m_mutex);
    m_triggers.copy_selected();
}

/**
 *  If there is a copied trigger, then this function grabs it from the trigger
 *  clipboard and adds it.
 *
 *  Why isn't this protected by a mutex?  We will eventually enable this if
 *  anything bad happens, such as a deadlock, or corruption.
 */

void
sequence::paste_trigger ()
{
    /*
     * TODO: automutex locker(m_mutex);
     */

    m_triggers.paste();
}

/**
 *  Provides a helper function simplify and speed up
 *  perform::reset_sequences().  Note that, in live mode, the user controls
 *  playback, while otherwise JACK or the performance/song editor controls
 *  playback.  (We're still a bit confounded about these modes, alas.)
 *
 * \param live_mode
 *      True if live mode is on.  This means that JACK transport is not in
 *      control of playback.
 */

void
#ifdef USE_PAUSE_SUPPORT
sequence::reset (bool live_mode, bool pause)
#else
sequence::reset (bool live_mode)
#endif
{
    bool state = get_playing();
    off_playing_notes();
    set_playing(false);
#ifdef USE_PAUSE_SUPPORT
    if (pause)
        set_orig_tick(m_last_tick);
    else
        zero_markers();
#else
    zero_markers();                     /* sets the "last-tick" value   */
#endif
    if (! live_mode)
        set_playing(state);
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
 *  Sets the draw-trigger iterator to the beginning of the trigger list.
 *
 * \threadsafe
 */

void
sequence::reset_draw_trigger_marker ()
{
    automutex locker(m_mutex);
    m_triggers.reset_draw_trigger_marker();
}

/**
 *  A new function provided so that we can find the minimum and maximum notes
 *  with only one (not two) traversal of the event list.
 *
 * \todo
 *      For efficency, we should calculate this only when the event set
 *      changes, and save the results and return them if good.
 *
 * \threadsafe
 *
 * \param lowest
 *      A reference parameter to return the note with the lowest value.
 *      if there are no notes, then it is set to SEQ64_MIDI_COUNT_MAX-1.
 *
 * \param highest
 *      A reference parameter to return the note with the highest value.
 *      if there are no notes, then it is set to -1.
 *
 * \return
 *      If there are no notes in the list, then false is returned, and the
 *      results should be disregarded.
 */

bool
sequence::get_minmax_note_events (int & lowest, int & highest)
{
    automutex locker(m_mutex);
    bool result = false;
    int low = SEQ64_MIDI_COUNT_MAX-1;
    int high = -1;
    for (event_list::iterator i = m_events.begin(); i != m_events.end(); i++)
    {
        event & er = DREF(i);
        if (er.is_note_on() || er.is_note_off())
        {
            if (er.get_note() < low)
            {
                low = er.get_note();
                result = true;
            }
            else if (er.get_note() > high)
            {
                high = er.get_note();
                result = true;
            }
        }
    }
    lowest = low;
    highest = high;
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
    midipulse * a_tick_s, midipulse * a_tick_f,
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
            ++m_iterator_draw;
            return result;
        }
        else if
        (
            drawevent.is_note_on() && (! drawevent.is_linked())
        )
        {
            result = DRAW_NOTE_ON;
            ++m_iterator_draw;
            return result;
        }
        else if
        (
            drawevent.is_note_off() && (! drawevent.is_linked())
        )
        {
            result = DRAW_NOTE_OFF;
            ++m_iterator_draw;
            return result;
        }
        ++m_iterator_draw;  /* keep going until we hit null or find a NoteOn */
    }
    return DRAW_FIN;
}

/**
 *  Get the next event in the event list.  Then set the status and control
 *  character parameters using that event.
 */

bool
sequence::get_next_event (midibyte * a_status, midibyte * a_cc)
{
    while (m_iterator_draw != m_events.end())
    {
        midibyte j;
        event & drawevent = DREF(m_iterator_draw);
        *a_status = drawevent.get_status();
        drawevent.get_data(*a_cc, j);
        m_iterator_draw++;
        return true;                /* we have a good one; update and return */
    }
    return false;
}

/**
 *  Get the next event in the event list that matches the given status and
 *  control character.  Then set the rest of the parameters parameters
 *  using that event.
 *
 *  Note the usage of event::is_desired_cc_or_not_cc(status, cc, *d0); Either
 *  we have a control change with the right CC or it's a different type of
 *  event.
 */

bool
sequence::get_next_event
(
    midibyte status, midibyte cc,
    midipulse * tick, midibyte * d0, midibyte * d1,
    bool * selected
)
{
    while (m_iterator_draw != m_events.end())
    {
        event & drawevent = DREF(m_iterator_draw);
        if (drawevent.get_status() == status)     /* note on, so linked */
        {
            drawevent.get_data(*d0, *d1);
            *tick = drawevent.get_timestamp();
            *selected = drawevent.is_selected();
            if (event::is_desired_cc_or_not_cc(status, cc, *d0))
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
    midipulse * tick_on, midipulse * tick_off, bool * selected, midipulse * offset
)
{
    return m_triggers.next(tick_on, tick_off, selected, offset);
}

/**
 *  Clears all events from the event container.  Unsets the modified flag.
 *  Also see the new copy_events() function.
 */

void
sequence::remove_all ()
{
    automutex locker(m_mutex);
    m_events.clear();
    m_events.unmodify();
}

/**
 *  Returns the last tick played, and is used by the editor's idle function.
 *  If m_length is 0, this function returns m_last_tick - m_trigger_offset, to
 *  avoid an arithmetic exception.  Should we return 0 instead?
 *
 *  Note that seqroll calls this function to help get the location of the
 *  progress bar.  What does perfedit do?
 */

midipulse
sequence::get_last_tick ()
{
    if (m_length > 0)
        return (m_last_tick + m_length - m_trigger_offset) % m_length;
    else
        return m_last_tick - m_trigger_offset;
}

/**
 *  Sets the midibus number to dump to.
 *
 * \threadsafe
 */

void
sequence::set_midi_bus (char mb)
{
    automutex locker(m_mutex);
    off_playing_notes();            /* off notes except initial         */
    m_bus = mb;
    set_dirty();
}

/**
 *  Sets the length (m_length) and adjusts triggers for it, if desired.
 *  This function is called in seqedit::apply_length(), when the user
 *  selects a sequence length in measures.  That function calculates the
 *  length in ticks:
 *
\verbatim
    L = M x B x 4 x P / W
        L == length (ticks or pulses)
        M == number of measures
        B == beats per measure
        P == pulses per quarter-note
        W == beat width in beats per measure
\endverbatim
 *
 *  For our "b4uacuse" MIDI file, M can be about 100 measures, B is 4,
 *  P can be 192 (but we want to support higher values), and W is 4.
 *  So L = 100 * 4 * 4 * 192 / 4 = 76800 ticks.  Seems small.
 *
 * \threadsafe
 */

void
sequence::set_length (midipulse len, bool adjust_triggers)
{
    automutex locker(m_mutex);
    bool was_playing = get_playing();
    set_playing(false);                 /* turn everything off              */
    if (len < midipulse(m_ppqn / 4))
        len = midipulse(m_ppqn / 4);

    m_triggers.set_length(len);         /* must precede adjust call         */
    if (adjust_triggers)
        m_triggers.adjust_offsets_to_length(len);

    m_length = len;
    verify_and_link();
    reset_draw_marker();
    if (was_playing)                    /* start up and refresh             */
        set_playing(true);
}

/**
 *  Sets the playing state of this sequence.  When playing, and the
 *  sequencer is running, notes get dumped to the ALSA buffers.
 *
 * \param p
 *      Provides the playing status to set.  True means to turn on the
 *      playing, false means to turn it off, and turn off any notes still
 *      playing.
 */

void
sequence::set_playing (bool p)
{
    automutex locker(m_mutex);
    if (p != get_playing())
    {
        m_playing = p;
        if (! p)
            off_playing_notes();

        set_dirty();
    }
    m_queued = false;
}

/**
 * \setter m_recording and m_notes_on
 *
 * \threadsafe
 */

void
sequence::set_recording (bool r)
{
    automutex locker(m_mutex);
    m_recording = r;
    m_notes_on = 0;
}

/**
 * \setter m_snap_tick
 *
 * \threadsafe
 */

void
sequence::set_snap_tick (int st)
{
    automutex locker(m_mutex);
    m_snap_tick = st;
}

/**
 * \setter m_quantized_rec
 *
 * \threadsafe
 */

void
sequence::set_quantized_rec (bool qr)
{
    automutex locker(m_mutex);
    m_quantized_rec = qr;
}

/**
 * \setter m_thru
 *
 * \threadsafe
 */

void
sequence::set_thru (bool r)
{
    automutex locker(m_mutex);
    m_thru = r;
}

/**
 *  Sets the sequence name member, m_name.
 */

void
sequence::set_name (char * name)
{
    m_name = name;
    set_dirty_mp();
}

/**
 *  Sets the sequence name member, m_name.
 */

void
sequence::set_name (const std::string & name)
{
    m_name = name;
    set_dirty_mp();
}

/**
 *  Sets the m_midi_channel number
 *
 * \threadsafe
 */

void
sequence::set_midi_channel (midibyte ch)
{
    automutex locker(m_mutex);
    off_playing_notes();
    m_midi_channel = ch;
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
sequence::print_triggers ()
{
    m_triggers.print(m_name);
}

/**
 *  Takes an event that this sequence is holding, and places it on the midibus.
 *
 * \threadsafe
 */

void
sequence::put_event_on_bus (event & ev)
{
    automutex locker(m_mutex);
    midibyte note = ev.get_note();
    bool skip = false;
    if (ev.is_note_on())
        m_playing_notes[note]++;

    if (ev.is_note_off())
    {
        if (m_playing_notes[note] <= 0)
            skip = true;
        else
            m_playing_notes[note]--;
    }
    if (! skip)
        m_masterbus->play(m_bus, &ev, m_midi_channel);  // pointer!

    if (not_nullptr(m_masterbus))
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
    for (int x = 0; x < c_midi_notes; ++x)
    {
        while (m_playing_notes[x] > 0)
        {
            e.set_status(EVENT_NOTE_OFF);
            e.set_data(x, 0);
            m_masterbus->play(m_bus, &e, m_midi_channel);
            m_playing_notes[x]--;
        }
    }
    if (not_nullptr(m_masterbus))
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
    midibyte status, midibyte cc, bool inverse
)
{
    automutex locker(m_mutex);
    midibyte d0, d1;
    for (event_list::iterator i = m_events.begin(); i != m_events.end(); i++)
    {
        event & er = DREF(i);
        er.get_data(d0, d1);
        bool match = er.get_status() == status;
        bool canselect;
        if (status == EVENT_CONTROL_CHANGE)
            canselect = match && d0 == cc;  /* correct status and correct cc */
        else
            canselect = match;              /* correct status, cc irrelevant */

        if (canselect)
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
            (er.get_status() == EVENT_NOTE_ON ||
                er.get_status() == EVENT_NOTE_OFF) && er.is_marked()
        )
        {
            e = er;
            e.unmark();
            int  note = e.get_note();
            bool off_scale = false;
            if (transpose_table[note % SEQ64_OCTAVE_SIZE] == 0)
            {
                off_scale = true;
                note -= 1;
            }
            for (int x = 0; x < steps; ++x)
                note += transpose_table[note % SEQ64_OCTAVE_SIZE];

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

/**
 * Not deleting the ends, not selected.
 */

void
sequence::quantize_events
(
    midibyte status, midibyte cc,
    midipulse snap_tick, int divide, bool linked
)
{
    automutex locker(m_mutex);
    midibyte d0, d1;
    event_list quantized_events;
    mark_selected();
    for (event_list::iterator i = m_events.begin(); i != m_events.end(); i++)
    {
        event & er = DREF(i);
        er.get_data(d0, d1);
        bool match = er.get_status() == status;
        bool canselect;
        if (status == EVENT_CONTROL_CHANGE)
            canselect = match && d0 == cc;  /* correct status and correct cc */
        else
            canselect = match;              /* correct status, cc irrelevant */

        if (! er.is_marked())
            canselect = false;

        if (canselect)
        {
            event e = er;             /* copy event */
            er.select();
            e.unmark();

            midipulse timestamp = e.get_timestamp();
            midipulse timestamp_remander = (timestamp % snap_tick);
            midipulse timestamp_delta = 0;
            if (timestamp_remander < snap_tick / 2)
                timestamp_delta = - (timestamp_remander / divide);
            else
                timestamp_delta = (snap_tick - timestamp_remander) / divide;

            if ((timestamp_delta + timestamp) >= m_length)
                timestamp_delta = - e.get_timestamp() ;

            e.set_timestamp(e.get_timestamp() + timestamp_delta);
            quantized_events.add(e, false);         // will sort afterward
            if (er.is_linked() && linked)
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
 *  This function fills the given character list with MIDI data from the
 *  current sequence, preparatory to writing it to a file.
 *
 *  Note that some of the events might not come out in the same order they
 *  were stored in (we see that with program-change events.
 *
 * \param c
 *      Provides the std::list object to push events to the front, which thus
 *      inserts them in backwards order.  (These events are then popped back,
 *      which restores the order, with some exceptions).
 *
 * \param tracknumber
 *      Provides the track number.  This number is masked into the track
 *      information.
 */

void
sequence::fill_container (midi_container & c, int tracknumber)
{
    automutex locker(m_mutex);
    c.fill(tracknumber);
}

/**
 *  A member function to dump a summary of events stored in the event-list of
 *  a sequence.
 */

void
sequence::show_events () const
{
    printf
    (
        "sequence #%d '%s': channel %d, events %d\n",
        number(), name().c_str(), get_midi_channel(), event_count()
    );
    const event_list & evl = events();
    for (event_list::const_iterator i = evl.begin(); i != evl.end(); i++)
    {
        const event & er = DREF(i);
        std::string evdump = to_string(er);
        printf(evdump.c_str());
    }
}

/**
 *  Copies an external container of events into the current container,
 *  effectively replacing all of its events.  Compare this function to the
 *  remove_all() function.  Copying the container is a lot of work, but
 *  fairly fast, even with an std::multimap as the container.
 *
 * \threadsafe
 *      Note that we had to consolidate the replacement of all the events in
 *      the container in order to prevent the "Save to Sequence" button in the
 *      eventedit object from causing the application to segfault.  It would
 *      segfault when the mainwnd timer callback would fire, causing updates
 *      to the sequence's slot pixmap, which would then try to access deleted
 *      events.  Part of the issue was that note links were dropped when
 *      copying the events, so now we call verify_and_link() to hopefully
 *      reconstitute the links.
 *
 * \param newevents
 *      Provides the container of MIDI events that will completely replace the
 *      current container.  Normally this container is supplied by the event
 *      editor, via the eventslots class.
 */

void
sequence::copy_events (const event_list & newevents)
{
    automutex locker(m_mutex);
    m_events.clear();
    m_events = newevents;
    if (m_events.empty())
        m_events.unmodify();

    m_iterator_draw = m_events.begin();     /* same as in reset_draw_marker */
    if (m_events.count() > 1)               /* need at least two events     */
    {
        /*
         * Another option, if we have a new sequence length value (in pulses)
         * would be to call sequence::set_length(len, adjust_triggers).
         */

        verify_and_link();
    }

    /*
     * If we call this, we can get updates to be seen, but for longer
     * sequences, it causes a segfault in updating the sequence pixmap in the
     * mainwnd::timer_callback() function due to null events, when the event
     * editor has deleted some events.  Not even locking the drawing of the
     * sequence in mainwid helps.  RE-VERIFY!
     */

    set_dirty();
}

}           // namespace seq64

/*
 * sequence.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

