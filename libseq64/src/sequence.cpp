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
 * \updates       2016-08-08
 * \license       GNU GPLv2 or above
 *
 *  The functionality of this class also includes handling some of the
 *  operations of pattern triggers.
 *
 *  We had added null-pointer checks for the master MIDI buss pointer, but
 *  these just take up needless time, in most cases.
 */

#include <stdlib.h>

#include "mastermidibus.hpp"
#include "perform.hpp"
#include "scales.h"
#include "sequence.hpp"
#include "settings.hpp"                 /* seq64::rc() and choose_ppqn()    */

#ifdef USE_STAZED_LFO_SUPPORT
#include "calculations.hpp"
#endif

#define USE_NON_NOTE_EVENT_ADJUSTMENT

namespace seq64
{

/**
 *  A static clipboard for holding pattern/sequence events.  Being static
 *  allows for copy/paste between patterns.
 */

event_list sequence::m_events_clipboard;

/**
 *  Principal constructor.
 *
 * \param ppqn
 *      Provides the PPQN parameter to perhaps alter the default PPQN value of
 *      this sequence.
 */

sequence::sequence (int ppqn)
 :
    m_parent                    (nullptr),
    m_events                    (),
    m_triggers                  (*this),
#ifdef SEQ64_STAZED_UNDO_REDO
    m_events_undo_hold          (),
#endif
    m_events_undo               (),
    m_events_redo               (),
    m_iterator_draw             (m_events.begin()),
    m_channel_match             (false),        // a future stazed feature
    m_midi_channel              (0),
    m_bus                       (0),
    m_song_mute                 (false),
#ifdef SEQ64_STAZED_TRANSPOSE
    m_transposable              (true),
#endif
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
    m_queued_tick               (0),            // accessed by perform::play()
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
    m_note_on_velocity          (SEQ64_DEFAULT_NOTE_ON_VELOCITY),
    m_note_off_velocity         (SEQ64_DEFAULT_NOTE_OFF_VELOCITY),
#ifdef SEQ64_STAZED_UNDO_REDO
    m_have_undo                 (false),
    m_have_redo                 (false)
#endif
    m_musical_key               (SEQ64_KEY_OF_C),
    m_musical_scale             (int(c_scale_off)),
    m_background_sequence       (SEQ64_SEQUENCE_LIMIT),
    m_mutex                     (),
    m_note_off_margin           (2)
{
    m_ppqn = choose_ppqn(ppqn);
    m_length = 4 * m_ppqn;                      /* one bar's worth of ticks */
    m_snap_tick = m_ppqn / 4;
    m_triggers.set_ppqn(m_ppqn);
    m_triggers.set_length(m_length);
    for (int i = 0; i < c_midi_notes; ++i)      /* no notes are playing now */
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
 *
 * \param rhs
 *      Provides the source of the new member values.
 */

void
sequence::partial_assign (const sequence & rhs)
{
    if (this != &rhs)
    {
        automutex locker(m_mutex);
        m_parent        = rhs.m_parent;             /* a pointer, careful!  */
        m_events        = rhs.m_events;
        m_triggers      = rhs.m_triggers;
        m_midi_channel  = rhs.m_midi_channel;
#ifdef SEQ64_STAZED_TRANSPOSE
        m_transposable  = rhs.m_transposable;
#endif
        m_bus           = rhs.m_bus;
        m_masterbus     = rhs.m_masterbus;          /* a pointer, be aware! */
        m_playing       = false;
        m_name          = rhs.m_name;
        m_ppqn          = rhs.m_ppqn;
        m_length        = rhs.m_length;
        m_time_beats_per_measure = rhs.m_time_beats_per_measure;
        m_time_beat_width = rhs.m_time_beat_width;
        for (int i = 0; i < c_midi_notes; ++i)      /* no notes are playing */
            m_playing_notes[i] = 0;

        zero_markers();                             /* reset to tick 0      */
        verify_and_link();
    }
}

#ifdef SEQ64_STAZED_UNDO_REDO

void
sequence::set_hold_undo (bool hold)
{
    automutex locker(m_mutex);
    Events::iterator i;                 //  std::list<event>::iterator i;
    if (hold)
    {
        for (i = m_events.begin(); i != m_events.end(); ++i)
            m_list_undo_hold.push_back(*i);
    }
    else
       m_list_undo_hold.clear();
}

int
sequence::get_hold_undo ()
{
    return m_list_undo_hold.size();
}

#endif  // SEQ64_STAZED_UNDO_REDO

/**
 *  Returns the number of events stored in m_events.  Note that only playable
 *  events are counted in a sequence.  If a sequence class function provides a
 *  mutex, call m_events.count() instead.
 *
 * \threadsafe
 *
 * \return
 *      Returns m_events.count().
 */

int
sequence::event_count () const
{
    automutex locker(m_mutex);
    return int(m_events.count());
}

/**
 *  Pushes the event-list into the undo-list or the upcoming undo-hold-list.
 *
 * \threadsafe
 *
 * \param hold
 *      A new parameter for the stazed undo/redo support, not yet used.
 *      If true, then the events go into the undo-hold-list.
 */

void
sequence::push_undo (bool hold)
{
    automutex locker(m_mutex);
    if (hold)
    {
#ifdef SEQ64_STAZED_UNDO_REDO
        m_events_undo.push(m_list_undo_hold);
#endif
    }
    else
        m_events_undo.push(m_events);

#ifdef SEQ64_STAZED_UNDO_REDO
    set_have_undo();
#endif
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
    if (m_events_undo.size() > 0)               // stazed: m_list_undo
    {
        m_events_redo.push(m_events);           // move to triggers module?
        m_events = m_events_undo.top();
        m_events_undo.pop();
        verify_and_link();
        unselect();
    }

    /*
     * Shouldn't these be inside the if-statement?
     */

#ifdef SEQ64_STAZED_UNDO_REDO
    set_have_undo();
    set_have_redo();
#endif

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
    if (m_events_redo.size() > 0)               // move to triggers module?
    {
        m_events_undo.push(m_events);
        m_events = m_events_redo.top();
        m_events_redo.pop();
        verify_and_link();
        unselect();
    }

    /*
     * Shouldn't these be inside the if-statement?
     */

#ifdef SEQ64_STAZED_UNDO_REDO
    set_have_undo();
    set_have_redo();
#endif

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
    m_triggers.push_undo(); // todo:  see how stazed's sequence function works
}

/**
 *  Calls triggers::pop_undo() with locking.
 */

void
sequence::pop_trigger_undo ()
{
    automutex locker(m_mutex);
    m_triggers.pop_undo();  // todo:  see how stazed's sequence function works
}

#ifdef SEQ64_STAZED_UNDO_REDO

void
sequence::pop_trigger_redo ()
{
    automutex locker(m_mutex);
    if (m_list_trigger_redo.size() > 0)         // move to triggers module?
    {
        m_list_trigger_undo.push(m_list_trigger);
        m_list_trigger = m_list_trigger_redo.top();
        m_list_trigger_redo.pop();
    }
}

#endif  // SEQ64_STAZED_UNDO_REDO

/**
 * \setter m_masterbus
 *      Do we need to call set_dirty_mp() here?
 *
 * \threadsafe
 *
 * \param mmb
 *      Provides a pointer to the master MIDI buss for this sequence.  This
 *      should be a reference, but isn't, nor is it checked.
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

#ifdef USE_STAZED_SELECTION_EXTENSIONS

int
sequence::select_even_or_odd_notes (int note_len, bool even)
{
    int result = 0;
    unselect();

    automutex locker(m_mutex);
    for (iterator i = m_events.begin(); i != m_events.end(); ++i)
    {
        if (i->is_note_on() )
        {
            long tick = i->get_timestamp();
            if (tick % note_len == 0)
            {
                /*
                 * Note that from the user's point of view of even and odd,
                 * we start counting from 1, not 0.
                 */

                int is_even = (tick / note_len) % 2;
                if ((even && is_even) || (! even && ! is_even))
                {
                    i->select();
                    ++result;
                    if (i->is_linked())
                    {
                        event * note_off = i->get_linked();
                        note_off->select();
                        ++result;
                    }
                }
            }
        }
    }
    return result;
}

/**
 *  Selects events in range provided:
 *  tick start, note high, tick end, and  note low.
 *
 *  Be aware the the event::is_note() function is used, and that it includes
 *  Aftertouch events, which generally need to stick with their Note On
 *  counterparts.
 *
 *  If a "note" event is detected, then we skip it.  This is necessary since
 *  channel pressure and control change use d0 for seqdata, and d0 is returned
 *  by get_note().  This causes note selection to occasionally select them
 *  when their seqdata values are within range of the tick selection.  So
 *  therefore we want only Note Ons and Note Offs.
 *
 * \note
 *      The continuation below ("continue") is necessary since channel
 *      pressure and control change use d0 for seqdata [which is returned by
 *      get_note()]. This causes seqroll note selection to occasionally
 *      select them when their seqdata values are within the range of tick
 *      selection. So only, note ons and offs.  What about Aftertouch?
 *      We have the event::is_note() function for that.
 *
 * \param tick_s
 *      The starting tick.
 *
 * \param note_h
 *      The highest note selected.
 *
 * \param tick_f
 *      The ending, or finishing, tick.
 *
 * \param note_l
 *      The lowest note selected.
 *
 * \param action
 *      The action to perform on the selection.
 */

int
sequence::select_note_events
(
    long tick_s, int note_h, long tick_f, int note_l, select_action_e action
)
{
    int result = 0;
    long ticks = 0;
    long tickf = 0;
    automutex locker(m_mutex);
    for (iterator i = m_events.begin(); i != m_events.end(); i++ )
    {
        if (! i->is_note())         // HMMMM, includes Aftertouch
            continue;

        if (i->get_note() <= note_h && i->get_note() >= note_l)
        {
            if (i->is_linked())
            {
                event * ev = i->get_linked();
                if (i->is_note_off())
                {
                    ticks = ev->get_timestamp();
                    tickf = i->get_timestamp();
                }
                if (i->is_note_on())
                {
                    ticks = i->get_timestamp();
                    tickf = ev->get_timestamp();
                }

                bool valid_and = (ticks <= tick_f) && (tickf >= tick_s);
                bool valid_or  = (ticks <= tick_f) || (tickf >= tick_s);
                bool use_it =
                (
                    ((ticks <= tickf) && valid_and) ||
                    ((ticks > tickf)  && valid_or)
                );
			    if (use_it)
                {
                    /*
                     * Could use a switch statement here.
                     */

                    if (action == e_select || action == e_select_one)
                    {
                        i->select();
                        ev->select();
                        ++result;
                        if (action == e_select_one)
                            break;
                    }
                    if (action == e_is_selected)
                    {
                        if (i->is_selected())
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
                    if (action == e_deselect)
                    {
                        result = 0;
                        i->unselect();
                        ev->unselect();
                    }

                    /*
                     * Don't toggle twice.
                     */

                    if (action == e_toggle_selection && i->is_note_on())
                    {
                        if (i->is_selected())
                        {
                            i->unselect();
                            ev->unselect();
                            ++result;
                        }
                        else
                        {
                            i->select();
                            ev->select();
                            ++result;
                        }
                    }
                    if (action == e_remove_one)
                    {
                        remove(i);
                        remove(ev);
                        reset_draw_marker();
                        ++result;
                        break;
                    }
                }
            }
            else
            {
                /*
                 * If NOT linked note on/off, then it is junk...
                 */

                tick_s = tick_f = i->get_timestamp();
                if (tick_s >= tick_s - 16 && tick_f <= tick_f)
                {
                    /*
                     * Could use a switch statement here.
                     */

                    if (action == e_select || action == e_select_one)
                    {
                        i->select();
                        ++result;
                        if (action == e_select_one)
                            break;
                    }
                    if (action == e_is_selected)
                    {
                        if (i->is_selected())
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
                    if (action == e_deselect)
                    {
                        result = 0;
                        i->unselect();
                    }
                    if (action == e_toggle_selection)
                    {
                        if (i->is_selected())
                        {
                            i->unselect();
                            ++result;
                        }
                        else
                        {
                            i->select();
                            ++result;
                        }
                    }
                    if (action == e_remove_one)
                    {
                        remove(i);
                        reset_draw_marker();
                        ++result;
                        break;
                    }
                }
            }
        }
    }
    return result;
}

/**
 *  Used with seqevent when selecting Note On or Note Off, this function will
 *  select the opposite linked event.
 */

int
sequence::select_linked (long tick_s, long tick_f, midibyte status)
{
    int result = 0;
    automutex locker(m_mutex);
    for (iterator i = m_events.begin(); i != m_events.end(); ++i)
    {
        if
        (
            i->get_status() == status &&
            i->get_timestamp() >= tick_s &&
            i->get_timestamp() <= tick_f
        )
        {
            if (i->is_linked())
            {
                if (i->is_selected())
                    i->get_linked()->select();
                else
                    i->get_linked()->unselect();

                ++result;
            }
        }
    }
    return result;
}

/**
 *  Use selected note ons if any.
 */

int
sequence::select_event_handle
(
    long tick_s, long tick_f, unsigned char status, unsigned char cc, int dats
)
{
    int result = 0;
    bool have_selection = false;
    if (status == EVENT_NOTE_ON)                    // use a function!
    {
        if (get_num_selected_events(status, cc))
            have_selection = true;
    }
    for (iterator i = m_events.begin(); i != m_events.end(); ++i)
    {
        if
        (
            i->get_status() == status && i->get_timestamp() >= tick_s &&
            i->get_timestamp() <= tick_f
        )
        {
            unsigned char d0, d1;
            i->get_data(&d0, &d1);
            if (status == EVENT_CONTROL_CHANGE && d0 == cc)
            {
                if (d1 <= (dats + 2) && d1 >= (dats - 2))   // is it in range
                {
                    unselect();
                    i->select();
                    ++result;
                    break;
                }
            }
            if (status != EVENT_CONTROL_CHANGE)             // use a function!
            {
                if (is_note_msg(status) || status == EVENT_PITCH_WHEEL)
                {
                    if(d1 <= (dats+2) && d1 >= (dats-2))    // is it in range
                    {
                        if (have_selection)                 // note on only
                        {
                            if (i->is_selected())
                            {
                                unselect();                 // all events
                                i->select( );               // only this one
                                if (result)
                                {
                                    /*
                                     * If we have a marked (unselected) one,
                                     * then clear it.
                                     */

                                    for
                                    (
                                        iterator i = m_events.begin();
                                        i != m_events.end(); ++i)
                                    {
                                        if (i->is_marked())
                                        {
                                            i->unmark();
                                            break;
                                        }
                                    }
                                    --result;        // clear marked one
                                    // reset for marked flag at end
                                    have_selection = false;
                                }
                                ++result;            // for the selected one
                                break;
                            }
                            else        // NOT selected note on, but in range
                            {
                                if (! result)       // only mark the first one
                                {
                                    i->mark();      // marked for hold until done
                                    ++result;       // indicate we got one
                                }

                                // keep going until we find a selected one if
                                // any, or are done

                                continue;
                            }
                        }
                        else                      // NOT note on
                        {
                            unselect();
                            i->select();
                            ++result;
                            break;
                        }
                    }
                }
                else
                {
                    if (d0 <= (dats + 2) && d0 >= (dats - 2))   // is it in range
                    {
                        unselect();
                        i->select();
                        ++result;
                        break;
                    }
                }
            }
        }
    }

    /*
     * Is it a note on that is unselected but in range? Then use it...
     * have_selection will be set to false if we found a selected one in
     * range.
     */

    if (result && have_selection)
    {
        for (iterator i = m_events.begin(); i != m_events.end(); ++i)
        {
            if (i->is_marked())
            {
                unselect();
                i->unmark();
                i->select();
                break;
            }
        }
    }
    set_dirty();
    return result;
}

#endif  // USE_STAZED_SELECTION_EXTENSIONS

/**
 * \setter m_rec_vol
 *      If this velocity is greater than zero, then m_note_on_velocity will
 *      be set as well.
 *
 * \threadsafe
 *
 * \param recvol
 *      The new setting of the recording volume setting.  It is used only if
 *      it ranges from 0 to SEQ64_MAX_NOTE_ON_VELOCITY.
 */

void
sequence::set_rec_vol (int recvol)
{
    automutex locker(m_mutex);
    if (m_rec_vol >= 0 && m_rec_vol <= SEQ64_MAX_NOTE_ON_VELOCITY)
    {
        m_rec_vol = recvol;
        if (m_rec_vol > 0)
            m_note_on_velocity = m_rec_vol;
    }
}

/**
 * \setter m_queued and m_queued_tick
 *      Toggles the queued flag and sets the dirty-mp flag.  Also calculates
 *      the queued tick based on m_last_tick.
 *
 * \threadsafe
 */

void
sequence::toggle_queued ()
{
    automutex locker(m_mutex);
    m_queued = ! m_queued;
    m_queued_tick = m_last_tick - mod_last_tick() + m_length;
    set_dirty_mp();
}

/**
 * \setter m_queued
 *      Turns off (resets) the queued flag and sets the dirty-mp flag.
 *      Do we need to set m_queued_tick as in toggle_queued()?  Currently not
 *      used.
 *
 * \threadsafe
 */

void
sequence::off_queued ()
{
    automutex locker(m_mutex);
    m_queued = false;
#ifndef USE_STAZED_JACK_SUPPORT
    m_queued_tick = m_last_tick - mod_last_tick() + m_length;
#endif
    set_dirty_mp();
}

/**
 * \setter m_queued
 *      Turns on (sets) the queued flag and sets the dirty-mp flag.
 *      Do we need to set m_queued_tick as in toggle_queued()?  Currently not
 *      used.
 *
 * \threadsafe
 */

void
sequence::on_queued ()
{
    automutex locker(m_mutex);
    m_queued = true;
    m_queued_tick = m_last_tick - mod_last_tick() + m_length;
    set_dirty_mp();
}

/**
 *  The play() function dumps notes starting from the given tick, and it
 *  pre-buffers ahead.  This function is called by the sequencer thread,
 *  performance.  The tick comes in as global tick.
 *
 *  It turns the sequence off after we play in this frame.
 *
 * \note
 *      With pause support, the progress bar for the pattern/sequence editor
 *      does what we want:  pause with the pause button, and rewind with the
 *      stop button.  Works with JACK, with issues, but we'd like to have
 *      the stop button do a rewind in JACK, too.
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

#ifdef SEQ64_PAUSE_SUPPORT_THIS_WORKS       /* disable, it works wrongly    */

    /*
     * Note that this is currently the only reason for providing the m_parent
     * member.
     *
     * HOWEVER, enabling this code can make ALSA playback run slow!  And it
     * seems to disable the effect of the BPM control.  Not yet
     * sure why.  Therefore, this code is currently DISABLED, even though
     * it allows pause to work correctly.
     */

    if (not_nullptr(m_parent) && ! m_parent->is_jack_running())
        tick = m_parent->get_jack_tick();

#endif

    midipulse end_tick = tick;
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

#ifdef SEQ64_STAZED_TRANSPOSE
    int transpose = get_transposable() ? m_parent->get_transpose() : 0 ;
#endif

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
            {
#ifdef SEQ64_STAZED_TRANSPOSE
                if (transpose != 0 && er.is_note()) /* includes Aftertouch  */
                {
                    event transposed_event = er;    /* assign ALL members   */
                    transposed_event.transpose_note(transpose);
                    put_event_on_bus(transposed_event);
                }
                else
#endif
                    put_event_on_bus(er);           /* frame still going    */
            }
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
 *  This function verifies state: all note-ons have a note-off, and it links
 *  note-offs with their note-ons.
 *
 * \threadsafe
 */

void
sequence::verify_and_link ()
{
    automutex locker(m_mutex);
    m_events.verify_and_link(m_length);
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
 *  without supplying an iterator from the event-list.  We no longer
 *  bother checking the pointer.  If it is bad, all hope is lost.
 *  If the event is a note off, and that note is currently playing, then send
 *  a note off.
 *
 * \threadunsafe
 *
 * \param i
 *      Provides the iterator to the event to remove from the event list.
 */

void
sequence::remove (event_list::iterator i)
{
    event & er = DREF(i);
    if (er.is_note_off() && m_playing_notes[er.get_note()] > 0)
    {
        m_masterbus->play(m_bus, &er, m_midi_channel);
        --m_playing_notes[er.get_note()];                   // ugh
    }
    m_events.remove(i);                                     // erase(i)
}

/**
 *  A helper function, which does not lock/unlock, so it is unsafe to call
 *  without supplying an iterator from the event-list.
 *
 *  Finds the given event in m_events, and removes the first iterator
 *  matching that.  If there are events that would match after that, they
 *  remain in the container.  This matches seq24 behavior.
 *
 * \threadunsafe
 *
 * \param e
 *      Provides a reference to the event to be removed.
 */

void
sequence::remove (event & e)
{
    for (event_list::iterator i = m_events.begin(); i != m_events.end(); ++i)
    {
        event & er = DREF(i);
        if (&e == &er)                  /* comparing pointers, not values */
        {
            m_events.remove(i);
            break;
        }
    }
}

/**
 *  Removes marked events.  Note how this function forwards the call to
 *  m_event.remove_marked().
 *
 * \threadsafe
 *
 * \return
 *      Returns true if at least one event was removed.
 */

bool
sequence::remove_marked ()
{
    automutex locker(m_mutex);
    bool result = m_events.remove_marked();
    reset_draw_marker();
    return result;
}

/**
 *  Marks the selected events.
 *
 * \threadsafe
 *
 * \return
 *      Returns true if there were any events that got marked.
 */

bool
sequence::mark_selected ()
{
    automutex locker(m_mutex);
    bool result = m_events.mark_selected();
    reset_draw_marker();
    return result;
}

/**
 *  Removes selected events.  This is a new convenience function to fold in
 *  the push_undo() and mark_selected() calls.  It makes the process slightly
 *  faster, as well.
 *
 * \threadsafe
 *      Also makes the whole process threadsafe.
 */

void
sequence::remove_selected ()
{
    automutex locker(m_mutex);
    if (m_events.mark_selected())
    {
        push_undo();                            // m_events_undo.push(m_events);
        (void) m_events.remove_marked();
        reset_draw_marker();
    }
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
 *  this function and get_clipboard_box().  Also note we could return a
 *  boolean indicating if the return values were filled in.
 *
 * \threadsafe
 *
 * \param [out] tick_s
 *      Side-effect return reference for the start time.
 *
 * \param [out] note_h
 *      Side-effect return reference for the high note.
 *
 * \param [out] tick_f
 *      Side-effect return reference for the finish time.
 *
 * \param [out] note_l
 *      Side-effect return reference for the low note.
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
    for (event_list::iterator i = m_events.begin(); i != m_events.end(); ++i)
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
 *  this function and get_selected_box().  Also note we could return a boolean
 *  indicating if the return values were filled in.
 *
 * \threadsafe
 *
 * \param [out] tick_s
 *      Side-effect return reference for the start time.
 *
 * \param [out] note_h
 *      Side-effect return reference for the high note.
 *
 * \param [out] tick_f
 *      Side-effect return reference for the finish time.
 *
 * \param [out] note_l
 *      Side-effect return reference for the low note.
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
    {
        tick_s = tick_f = note_h = note_l = 0;
    }
    else
    {
        event_list::iterator i;
        for (i = m_events_clipboard.begin(); i != m_events_clipboard.end(); ++i)
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
 *  Counts the selected notes in the event list.
 *
 * \threadsafe
 *
 * \return
 *      Returns m_events.count_selected_notes().
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
 *
 * \param status
 *      The desired kind of event to count.
 *
 * \param cc
 *      The desired control-change to count, if the event is a control-change.
 *
 * \return
 *      Returns m_events.count_selected_events().
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
 *  Compare this function to the convenience function select_all_notes(),
 *  which doesn't use range information.
 *
 * \threadsafe
 *
 * \param tick_s
 *      The start time of the selection.
 *
 * \param note_h
 *      The high note of the selection.
 *
 * \param tick_f
 *      The finish time of the selection.
 *
 * \param note_l
 *      The low note of the selection.
 *
 * \param action
 *      The action to perform, one of e_select, e_select_one, e_is_selected,
 *      e_would_select, e_deselect, e_toggle_selection, and e_remove_one.
 *
 * \return
 *      Returns the number of events acted on, or 0 if no desired event was
 *      found.
 */

int
sequence::select_note_events
(
    midipulse tick_s, int note_h,
    midipulse tick_f, int note_l,
    select_action_e action
)
{
    int result = 0;
    automutex locker(m_mutex);
    for (event_list::iterator i = m_events.begin(); i != m_events.end(); ++i)
    {
        event & er = DREF(i);
        if (er.get_note() <= note_h && er.get_note() >= note_l)
        {
            midipulse stick = 0;                    // must be initialized
            midipulse ftick = 0;                    // must be initialized
            if (er.is_linked())
            {
                event * ev = er.get_linked();       // pointer
                if (er.is_note_off())
                {
                    stick = ev->get_timestamp();
                    ftick = er.get_timestamp();
                }
                else if (er.is_note_on())
                {
                    ftick = ev->get_timestamp();
                    stick = er.get_timestamp();
                }

                bool tand = (stick <= tick_f) && (ftick >= tick_s);
                bool tor = (stick <= tick_f) || (ftick >= tick_s);
                if (((stick <= ftick) && tand) || ((stick > ftick) && tor))
                {
                    if (action == e_select || action == e_select_one)
                    {
                        er.select();
                        ev->select();
                        ++result;
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
                    if (action == e_deselect)
                    {
                        result = 0;
                        er.unselect();
                        ev->unselect();
                    }
                    if (action == e_toggle_selection && er.is_note_on())
                    {
                        ++result;
                        if (er.is_selected())       // don't toggle twice
                        {
                            er.unselect();
                            ev->unselect();
                        }
                        else
                        {
                            er.select();
                            ev->select();
                        }
                    }
                    if (action == e_remove_one)
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
                stick = ftick = er.get_timestamp();
                if (stick >= tick_s - 16 && ftick <= tick_f)
                {
                    if (action == e_select || action == e_select_one)
                    {
                        er.select();
                        ++result;
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
                    if (action == e_deselect)
                    {
                        result = 0;
                        er.unselect();
                    }
                    if (action == e_toggle_selection)
                    {
                        ++result;
                        if (er.is_selected())
                            er.unselect();
                        else
                            er.select();
                    }
                    if (action == e_remove_one)
                    {
                        remove(er);
                        reset_draw_marker();
                        ++result;
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
 *
 * \param tick_s
 *      The start time of the selection.
 *
 * \param tick_f
 *      The finish time of the selection.
 *
 * \param status
 *      The desired event in the selection.
 *
 * \param cc
 *      The desired control-change in the selection, if the event is a
 *      control-change.
 *
 * \param action
 *      The desired selection action.
 *
 * \return
 *      Returns the number of events selected.
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
    for (event_list::iterator i = m_events.begin(); i != m_events.end(); ++i)
    {
        event & er = DREF(i);
        midipulse t = er.get_timestamp();
        if (er.get_status() == status && t >= tick_s && t <= tick_f)
        {
            midibyte d0, d1;
            er.get_data(d0, d1);
            if (event::is_desired_cc_or_not_cc(status, cc, d0))
            {
                if (action == e_select || action == e_select_one)
                {
                    er.select();
                    ++result;
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
                    ++result;
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
 *  Removes and adds selected notes in position.  Also currently moves any
 *  other events in the range of the selection.
 *
 *  Also, we've moved external calls to push_undo() into this function.
 *  The caller shouldn't have to do that.
 *
 *  Another thing this function does is wrap-around when movement occurs.
 *  Any events (except Note Off) that will start just after the END of the
 *  pattern will be wrapped around to the beginning of the pattern.
 *
 * Fixed:
 *
 *  Select all notes in a short pattern that starts at time 0 and has non-note
 *  events starting at time 0 (see contrib/midi/allofarow.mid); move them with
 *  the right arrow, and move them back with the left arrow; then view in the
 *  event editor, and see that the non-Note events have not moved back, and in
 *  fact move way too far to the right, actually to near the END marker.
 *  We've fixed that in the new adjust_timestamp() function.
 *
 *  This function checks for any marked events in seq24, but now we make sure
 *  the event is a Note On or Note Off event before dealing with it.  We now
 *  handle properly events like Program Change, Control Change, and Pitch
 *  Wheel. Remember that Aftertouch is treated like a note, as it has
 *  velocity. For non-Notes, event::get_note() returns m_data[0], and we don't
 *  want to adjust that.
 *
 * \param delta_tick
 *      Provides the amount of time to move the selected notes.  Note that it
 *      also applies to events.  Note-Off events are expanded to m_length if
 *      their timestamp would be 0.  All other events will wrap around to 0.
 *
 * \param delta_note
 *      Provides the amount of pitch to move the selected notes.  This value
 *      is applied only to Note (On and Off) events.  Also, if this value
 *      would bring a note outside the range of 0 to 127, that note is not
 *      changed and the event is not moved.
 */

void
sequence::move_selected_notes (midipulse delta_tick, int delta_note)
{
    automutex locker(m_mutex);
    push_undo();                                    /* do it for caller     */
    if (mark_selected())                            /* locked recursively   */
    {
        for (event_list::iterator i = m_events.begin(); i != m_events.end(); ++i)
        {
            event & er = DREF(i);
            if (er.is_marked())                     /* is it being moved ?  */
            {
                event e = er;                       /* copy event           */
                e.unmark();                         /* unmark the new event */
                int newnote = e.get_note() + delta_note;
                if (newnote >= 0 && newnote < c_num_keys)
                {
                    midipulse newts = e.get_timestamp() + delta_tick;
                    newts = adjust_timestamp(newts, e.is_note_off());
                    if (e.is_note())                /* Note On or Note Off  */
                        e.set_note(midibyte(newnote));

                    e.set_timestamp(newts);
                    e.select();                     /* keep it selected     */
                    add_event(e);
                    if (not_nullptr(m_parent))
                        m_parent->modify();         /* new, centralize      */
                }
            }
        }
        if (remove_marked())
            verify_and_link();
    }
}

/**
 *  A new function to consolidate the adjustment of timestamps in a pattern.
 *  If the timestamp is greater that m_length, we do round robin magic.  Taken
 *  from similar code in move_selected_notes() and grow_selected().
 *  Be careful using this function.
 *
 * \param t
 *      Provides the timestamp to be adjusted based on m_length.
 *
 * \param isnoteoff
 *      Used for "expanding" the timestamp from 0 to just less than m_length,
 *      if necessary.  Should be set to true only for Note Off events; it
 *      defaults to false, which means to wrap the events around the end of
 *      the sequence if necessary, and is used only in movement, not in growth.
 *
 * \return
 *      Returns the adjusted timestamp.
 */

midipulse
sequence::adjust_timestamp (midipulse t, bool isnoteoff)
{
    if (t > m_length)
        t -= m_length;

    if (t < 0)                          /* only if midipulse is signed  */
        t += m_length;

    if (isnoteoff)
    {
        if (t == 0)
            t = m_length - m_note_off_margin;
    }
    else                                /* if (wrap)                    */
    {
        if (t == m_length)
            t = 0;
    }
    return t;
}

/**
 *  A new function to consolidate the growth/shrinkage of timestamps in a
 *  pattern.  If the new (off) timestamp is less than the on-time, it is
 *  clipped to the snap value.  If it is greater than the length of the
 *  sequence, then it is clipped to the sequence length.  No wrap-around.
 *
 * \param ontime
 *      Provides the original time, which limits the amount of negative
 *      adjustment that can be done.
 *
 * \param offtime
 *      Provides the timestamp to be adjusted and clipped.
 *
 * \return
 *      Returns the adjusted timestamp.
 */

midipulse
sequence::clip_timestamp (midipulse ontime, midipulse offtime)
{
    if (offtime <= ontime)
        offtime = ontime + m_snap_tick - note_off_margin();
    else if (offtime >= m_length)
        offtime = m_length - note_off_margin();

    return offtime;
}

/**
 *  Performs a stretch operation on the selected events.  This should move
 *  a note off event, according to old comments, but it doesn't seem to do
 *  that.  See the grow_selected() function.  Rather, it moves any event in the
 *  selection.
 *
 *  Also, we've moved external calls to push_undo() into this function.
 *  The caller shouldn't have to do that.
 *
 * \threadsafe
 *
 * \param delta_tick
 *      Provides the amount of time to stretch the selected notes.
 */

void
sequence::stretch_selected (midipulse delta_tick)
{
    automutex locker(m_mutex);
    if (mark_selected())
    {
        unsigned first_ev = 0x7fffffff;             /* timestamp lower limit */
        unsigned last_ev = 0x00000000;              /* timestamp upper limit */
        push_undo();                                /* do it for caller      */
        for (event_list::iterator i = m_events.begin(); i != m_events.end(); ++i)
        {
            event & er = DREF(i);
            if (er.is_selected())
            {
                event * e = &er;
                if (e->get_timestamp() < midipulse(first_ev))
                    first_ev = e->get_timestamp();

                if (e->get_timestamp() > midipulse(last_ev))
                    last_ev = e->get_timestamp();
            }
        }
        unsigned old_len = last_ev - first_ev;
        unsigned new_len = old_len + delta_tick;
        if (new_len > 1)
        {
            float ratio = float(new_len) / float(old_len);
            mark_selected();                        /* locked recursively   */
            for
            (
                event_list::iterator i = m_events.begin();
                i != m_events.end(); ++i
            )
            {
                event & er = DREF(i);
                if (er.is_marked())
                {
                    event n = er;                   /* copy the event       */
                    midipulse t = er.get_timestamp();
                    n.set_timestamp(midipulse(ratio * (t-first_ev)) + first_ev);
                    n.unmark();
                    add_event(n);
                }
            }
            if (remove_marked())
                verify_and_link();
        }
    }
}

/**
 *  The original description was "Moves note off event."  But this also gets
 *  called when simply selecting a second note via a ctrl-left-click, even in
 *  seq24.  And, though it doesn't move Note Off events, it does reconstruct
 *  them.
 *
 *  This function is called when doing a ctrl-left mouse move on the selected
 *  notes or when using ctrl-left-arrow or ctrl-right-arrow to shrink or
 *  stretch the selected notes.  Using the mouse allows pretty much any amount
 *  of growth or shrinkage, but use the arrow keys limits the changes to the
 *  current snap value.
 *
 *  This function grows/shrinks only Note On events that are marked and linked.
 *  If an event is not linked, this function now ignores the event's timestamp,
 *  rather than risk a segfault on a null pointer.  Compare this function to
 *  the stretch_selected() and move_selected_notes() functions.
 *
 *  This function would strip out non-Notes, but now it at least preserves
 *  them and moves them, to try to preserve their relative position re the
 *  notes.
 *
 *  In any case, we want to mark the original off-event for deletion, otherwise
 *  we get duplicate off events, for example in the "Begin/End" pattern in the
 *  test.midi file.
 *
 *  This function now tries to prevent pathological growth, such as trying to
 *  shrink the notes to zero length or less, or stretch them beyond the length
 *  of the sequence.  Otherwise we get weird and unexpected results.
 *
 *  Also, we've moved external calls to push_undo() into this function.
 *  The caller shouldn't have to do that.
 *
 *  A comment on terminology:  The user "selects" notes, while the sequencer
 *  "marks" notes. The first thing this function does is mark all the selected
 *  notes.
 *
 * \threadsafe
 *
 * \param delta_tick
 *      An offset for each linked event's timestamp.
 */

void
sequence::grow_selected (midipulse delta_tick)
{
    automutex locker(m_mutex);
    push_undo();                                    /* do it for caller     */
    if (mark_selected())                            /* locked recursively   */
    {
        for (event_list::iterator i = m_events.begin(); i != m_events.end(); ++i)
        {
            event & er = DREF(i);
            if (er.is_note())
            {
                if (er.is_marked() && er.is_note_on() && er.is_linked())
                {
                    event * off = er.get_linked();
                    event e = *off;                 /* original off-event   */
                    midipulse ontime = er.get_timestamp();
                    midipulse offtime = off->get_timestamp();
                    midipulse newtime = clip_timestamp(ontime, offtime + delta_tick);
                    off->mark();                    /* kill old off event   */
                    er.unmark();                    /* keep old on event    */
                    e.unmark();                     /* keep new off event   */
                    e.set_timestamp(newtime);       /* new off-time         */
                    add_event(e);                   /* add fixed off event  */
                    if (not_nullptr(m_parent))
                        m_parent->modify();         /* new, centralize      */
                }
            }
            else if (er.is_marked())                /* non-Note event?      */
            {
#ifdef USE_NON_NOTE_EVENT_ADJUSTMENT                /* currenty defined     */
                event e = er;                       /* copy original event  */
                midipulse ontime = er.get_timestamp();
                midipulse newtime = clip_timestamp(ontime, ontime + delta_tick);
                e.set_timestamp(newtime);           /* adjust time-stamp    */
                add_event(e);                       /* add adjusted event   */
                if (not_nullptr(m_parent))
                    m_parent->modify();             /* new, centralize      */
#else
                er.unmark();                        /* unmark old version   */
#endif
            }
        }
        if (remove_marked())
            verify_and_link();
    }
}

#ifdef USE_STAZED_RANDOMIZE_SUPPORT

void
sequence::randomize_selected
(
    unsigned char status, unsigned char control, int plus_minus
)
{
    int random;
    unsigned char data[2];
    unsigned char datitem;
    int datidx = 0;
    automutex locker(m_mutex);
    push_undo();
    for (iterator i = m_events.begin(); i != m_events.end(); ++i)
    {
        if (i->is_selected() && i->get_status() == status)
        {
            i->get_data(data, data+1);
            if (event::is_two_byte_msg(status))
                datidx = 1;

            if (event::is_one_byte_message(status))
                datidx = 0;

            datitem = data[datidx];

            // See http://c-faq.com/lib/randrange.html

            random = (rand() / (RAND_MAX / ((2 * plus_minus) + 1) + 1)) -
                plus_minus;

            datitem += random;
            if (datitem > (SEQ64_MIDI_COUNT_MAX - 1))
                datitem = (SEQ64_MIDI_COUNT_MAX - 1);
            else if (datitem < 0)
                datitem = 0;

            data[datidx] = datitem;
            i->set_data(data[0], data[1]);
        }
    }
}

void
sequence::adjust_dathandle (unsigned char status, int data)
{
    unsigned char data[2];
    unsigned char datitem;;
    int datidx = 0;
    automutex locker(m_mutex);
    for (iterator i = m_events.begin(); i != m_events.end(); ++i)
    {
        if (i->is_selected() && i->get_status() == status)
        {
            i->get_data( ata, data+1);
            if (event::is_two_byte_msg(status))
                datidx = 1;

            if (event::is_one_byte_message(status))
                datidx = 0;

            datitem = data;
            if (datitem > (SEQ64_MIDI_COUNT_MAX - 1))
                datitem = (SEQ64_MIDI_COUNT_MAX - 1);
            else if (datitem < 0)
                datitem = 0;

            data[datidx] = datitem;
            i->set_data(data[0], data[1]);
        }
    }
}

#endif   // USE_STAZED_RANDOMIZE_SUPPORT

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
 *
 * \param astat
 *      The desired event.
 *
 *  Parameter "acontrol", the desired control-change, is unused.
 *  This might be a bug, or at least a missing feature.
 */

void
sequence::increment_selected (midibyte astat, midibyte /*acontrol*/)
{
    automutex locker(m_mutex);
    for (event_list::iterator i = m_events.begin(); i != m_events.end(); ++i)
    {
        event & er = DREF(i);
        if (er.is_selected())
        {
            if (er.get_status() == astat)   // && er.get_control == acontrol
            {
                if (event::is_two_byte_msg(astat))
                    er.increment_data2();
                else if (event::is_one_byte_msg(astat))
                    er.increment_data1();
            }
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
 *
 * \param astat
 *      The desired event.
 *
 *  Parameter "acontrol", the desired control-change, is unused.
 *  This might be a bug, or at least a missing feature.
 */

void
sequence::decrement_selected (midibyte astat, midibyte /*acontrol*/)
{
    automutex locker(m_mutex);
    for (event_list::iterator i = m_events.begin(); i != m_events.end(); ++i)
    {
        event & er = DREF(i);
        if (er.is_selected())
        {
            if (er.get_status() == astat)   // && er.get_control == acontrol
            {
                if (event::is_two_byte_msg(astat))
                    er.decrement_data2();
                else if (event::is_one_byte_msg(astat))
                    er.decrement_data1();
            }
        }
    }
}

/**
 *  Copies the selected events.  This function also has the danger, discovered
 *  by user 0rel, of events being modified after being added to the clipboard.
 *  So we add his reconstruction fix here as well.  To summarize the steps:
 *
 *      -#  Clear the m_events_clipboard.
 *      -#  Add all selected events in this clipboard to the sequence.
 *      -#  Normalize the timestamps of the events in the clip relative to the
 *          timestamp of the first selected event.  (Is this really needed?)
 *      -#  Reconstruct/reconstitute the m_events_clipboard.
 *
 *  This process is a bit easier to manage than erase/insert on events because
 *  std::multimap has no erase() function that returns the next valid
 *  iterator.  Also, we use a local clipboard first, to save on copying.
 *  We've enhanced the error-checking, too.
 *
 *  Finally, note that m_events_clipboard is a static member of sequence, so:
 *
 *      -#  Copying can be done between sequences.
 *      -#  Access to it needs to be protected by a mutex.
 *
 * \threadsafe
 */

void
sequence::copy_selected ()
{
    automutex locker(m_mutex);
    event_list clipbd;
    for (event_list::iterator i = m_events.begin(); i != m_events.end(); ++i)
    {
#ifdef SEQ64_USE_EVENT_MAP
        if (DREF(i).is_selected())
            clipbd.add(DREF(i), false);         /* no post-sort */
#else
        if (DREF(i).is_selected())
            clipbd.push_back(DREF(i));
#endif
    }
    if (clipbd.count() > 0)
    {
        midipulse first_tick = DREF(clipbd.begin()).get_timestamp();
        if (first_tick >= 0)
        {
            for (event_list::iterator i = clipbd.begin(); i != clipbd.end(); ++i)
            {
                midipulse t = DREF(i).get_timestamp();
                if (t >= first_tick)
                    DREF(i).set_timestamp(t - first_tick);
            }
        }
        m_events_clipboard = clipbd;
    }
    else
        m_events_clipboard.clear();
}

/**
 *  Cuts the selected events.  Pushes onto the undo stack, may copy the
 *  events, marks the selected events, and removes them.  Now also sets the
 *  dirty flag so that the caller doesn't have to.  Also raises the modify
 *  flag on the parent perform object.
 *
 * \threadsafe
 *
 * \param copyevents
 *      If true, copy the selected events before marking and removing them.
 */

void
sequence::cut_selected (bool copyevents)
{
    push_undo();
    if (copyevents)
        copy_selected();

    if (mark_selected())                            /* locked recursively   */
    {
        if (remove_marked())
        {
            set_dirty();                            /* do it for the caller */
            if (not_nullptr(m_parent))
                m_parent->modify();                 /* new, centralize      */
        }
    }
}

/**
 *  Pastes the selected notes (and only note events) at the given tick and
 *  the given note value.
 *
 *  Also, we've moved external calls to push_undo() into this function.
 *  The caller shouldn't have to do that.
 *
 *  The event_keys used to access/sort the multimap event_list is not updated
 *  after changing timestamp/rank of the stored events.  Regenerating all
 *  key/value pairs before merging them solves this issue, so that
 *  the order of events in the sequence will be preserved.  This action is not
 *  needed for moving or growing events.  Nor is it needed if the old
 *  std::list implementation of the event container is compiled in.  However,
 *  it is needed in any operation that modifies the timestamp of an event
 *  inside the container:
 *
 *      -   copy_selected()
 *      -   paste_selected()
 *      -   quantize_events() TODO TODO TODO!
 *
 *  The alternative to reconstructing the map is to erase-and-insert the
 *  events modified in the code above, rather than just tweaking their values,
 *  which have an effect on sorting for the event-map implementation.
 *  However, multimap does not provide an erase() function that returns the
 *  next valid iterator, which would complicate this method of operation.  So
 *  we're inclined to stick with this solution.
 *
 *  There was an issue with copy/pasting a whole sequence.  The pasted events
 *  did not go to their destination, but overlayed the original events.  This
 *  bugs also occurred in Seq24 0.9.2.  It occurs with the allofarow.mid file
 *  when doing Ctrl-A Ctrl-C Ctrl-V Move-Mouse Left-Click.  It turns out the
 *  original code was checking only the first event to see if it was a Note
 *  event.  For sequences that started with a Control Change or Program Change
 *  (or other non-Note events), the highest note was never modified, and none
 *  of the note events were adjusted.
 *
 *  Finally, we only want to transpose note events (i.e. alter m_data[0]),
 *  and not other kinds of events.  We still need to figure out what to do
 *  with aftertouch, though.  Currently likely to be covered by the processing
 *  of the note that it accompanies.
 *
 * \threadsafe
 *
 * \param tick
 *      The time destination for the paste. This represents the "x" coordinate
 *      of the upper left corner of the paste-box.  It will be converted to an
 *      offset, for example pasting every event 48 ticks forward from the
 *      original copy.
 *
 * \param note
 *      The note/pitch destination for the paste. This represents the "y"
 *      coordinate of the upper left corner of the paste-box.  It will be
 *      converted to an offset, for example pasting every event 7 notes
 *      higher than the original copy.
 */

void
sequence::paste_selected (midipulse tick, int note)
{
    automutex locker(m_mutex);
    event_list clipbd = m_events_clipboard;         /* copy the clipboard   */
    push_undo();                                    /* do it for caller     */
    if (clipbd.count() > 0)
    {
        for (event_list::iterator i = clipbd.begin(); i != clipbd.end(); ++i)
        {
            event & e = DREF(i);
            midipulse t = e.get_timestamp();
            e.set_timestamp(t + tick);
        }

        int highest_note = 0;
        for (event_list::iterator i = clipbd.begin(); i != clipbd.end(); ++i)
        {
            event & e = DREF(i);
            if (e.is_note_on() || e.is_note_off())
            {
                midibyte n = e.get_note();
                if (n > highest_note)
                    highest_note = n;
            }
        }

        int note_delta = note - highest_note;
        for (event_list::iterator i = clipbd.begin(); i != clipbd.end(); ++i)
        {
            event & e = DREF(i);
            if (e.is_note())                        /* Note On or Note Off  */
            {
                midibyte n = e.get_note();
                e.set_note(n + note_delta);
            }
        }

#ifdef SEQ64_USE_EVENT_MAP

        /*
         * \change 0rel 2016-06-12 fix, see the banner notes.
         */

        event_list clipbd_updated;
        for (event_list::iterator i = clipbd.begin(); i != clipbd.end(); ++i)
            clipbd_updated.add(DREF(i));

        clipbd = clipbd_updated;

#endif      // SEQ64_USE_EVENT_MAP

        m_events.merge(clipbd, false);          /* don't presort clipboard  */
        m_events.sort();                        /* uh, does nothing in map  */
        verify_and_link();
        reset_draw_marker();
        if (not_nullptr(m_parent))
            m_parent->modify();                 /* new, centralize it here  */
    }
}

/**
 *  Changes the event data range.  Changes only selected events, if any.
 *
 * \threadsafe
 *
 *  Let t == the current tick value; ts == tick start value; tf == tick
 *  finish value; ds = data start value; df == data finish value; d = the
 *  new data value.  Then
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
    bool have_selection = get_num_selected_events(status, cc) > 0;
    for (event_list::iterator i = m_events.begin(); i != m_events.end(); ++i)
    {
        midibyte d0, d1;
        event & er = DREF(i);
        er.get_data(d0, d1);
        bool match = er.get_status() == status;
        bool good;                          /* event::is_desired_cc_or_not_cc */
        if (status == EVENT_CONTROL_CHANGE)
            good = match && d0 == cc;       /* correct status and correct cc */
        else
            good = match;                   /* correct status and not a cc   */

        midipulse tick = er.get_timestamp();
        if (! (tick >= tick_s && tick <= tick_f))       /* in range?         */
            good = false;

        if (have_selection && ! er.is_selected())       /* in selection?     */
            good = false;

        if (good)
        {

#ifdef SEQ64_STAZED_UNDO_REDO
            if (! get_hold_undo())
                set_hold_undo(true);
#endif
            if (tick_f == tick_s)
                tick_f = tick_s + 1;                    /* avoid divide-by-0 */

            int newdata =
            (
                (tick - tick_s) * data_f + (tick_f - tick) * data_s
            ) / (tick_f - tick_s);

            if (newdata < 0)
                newdata = 0;

            if (newdata >= SEQ64_MIDI_COUNT_MAX)        /* 128              */
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

#ifdef USE_STAZED_LFO_SUPPORT

void
sequence::change_event_data_lfo
(
    double value, double range,
    double speed, double phase, int wave,
    unsigned char status,
    unsigned char cc
)
{
    automutex locker(m_mutex);
    bool have_selection = false; /* change only selected events, if any */
    if( get_num_selected_events(status, cc) )
        have_selection = true;

    for (iterator i = m_events.begin(); i != m_events.end(); ++i)
    {
        bool set = false;
        unsigned char d0, d1;
        i->get_data(&d0, &d1);

        /* correct status and not CC */

        if (status != EVENT_CONTROL_CHANGE && i->get_status() == status )
            set = true;

        /* correct status and correct cc */

        if
        (
            status == EVENT_CONTROL_CHANGE && i->get_status() == status &&
            d0 == cc
        )
            set = true;

        /* in selection? */

        if (have_selection && ! i->is_selected())
            set = false;

        if (set)
        {
            if (! get_hold_undo())
                set_hold_undo(true);

            int tick = i->get_timestamp();
            int angle = speed * double(tick) / double(m_length) *
                    double(m_time_beat_width) + phase;

            int newdata = value + wave_func(angle, wave) * range;
            if ( newdata < 0 )
                newdata = 0;

            if (newdata > (SEQ64_MIDI_COUNT_MAX - 1))
                newdata = (SEQ64_MIDI_COUNT_MAX - 1);

            if (status == EVENT_NOTE_ON)
                d1 = newdata;

            if (status == EVENT_NOTE_OFF)
                d1 = newdata;

            if (status == EVENT_AFTERTOUCH)
                d1 = newdata;

            if (status == EVENT_CONTROL_CHANGE)
                d1 = newdata;

            if (status == EVENT_PROGRAM_CHANGE)
                d0 = newdata;                           /* d0 == new patch  */

            if (status == EVENT_CHANNEL_PRESSURE)
                d0 = newdata;                           /* d0 == pressure   */

            if (status == EVENT_PITCH_WHEEL)
                d1 = newdata;

            i->set_data(d0, d1);
        }
    }
}

#endif   // USE_STAZED_LFO_SUPPORT

/**
 *  Adds a note of a given length and  note value, at a given tick
 *  location.  It adds a single note-on / note-off pair.
 *
 *  The paint parameter indicates if we care about the painted event,
 *  so then the function runs though the events and deletes the painted
 *  ones that overlap the ones we want to add.
 *
 *  Also note that push_undo() is not incorporated into this function, for
 *  the sake of speed.
 *
 *  Here, we could ignore events not on the sequence's channel, as an option.
 *  We have to be careful because this function can be used in painting notes.
 *
 * Stazed:
 *
 *      http://www.blitter.com/~russtopia/MIDI/~jglatt/tech/midispec.htm
 *
 *      Note Off: The first data is the note number. There are 128 possible
 *      notes on a MIDI device, numbered 0 to 127 (where Middle C is note
 *      number 60). This indicates which note should be released.  The second
 *      data byte is the velocity, a value from 0 to 127. This indicates how
 *      quickly the note should be released (where 127 is the fastest). It's
 *      up to a MIDI device how it uses velocity information. Often velocity
 *      will be used to tailor the VCA release time.  MIDI devices that can
 *      generate Note Off messages, but don't implement velocity features,
 *      will transmit Note Off messages with a preset velocity of 64.
 *
 *  Also, we now see that seq24 never used the recording-velocity member
 *  (m_rec_vol).  We use it to modify the new m_note_on_velocity member if
 *  the user changes it in the seqedit window.
 *
 * \threadsafe
 *
 * \param tick
 *      The time destination of the new note, in pulses.
 *
 * \param len
 *      The duration of the new note, in pulses.
 *
 * \param note
 *      The pitch destination of the new note.
 *
 * \param paint
 *      If true, repaint the whole set of events, in order to be left with
 *      a clean view of the inserted event.  The default is false.
 */

void
sequence::add_note (midipulse tick, midipulse len, int note, bool paint)
{
    if (tick >= 0 && note >= 0 && note < c_num_keys)
    {
        automutex locker(m_mutex);
        bool ignore = false;
        if (paint)                        /* see the banner above */
        {
            for
            (
                event_list::iterator i = m_events.begin();
                i != m_events.end(); ++i
            )
            {
                event & er = DREF(i);
                if
                (
                    er.is_painted() && er.is_note_on() &&
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
            (void) remove_marked();
        }
        if (! ignore)
        {
            event e;
            if (paint)
                e.paint();

            e.set_status(EVENT_NOTE_ON);
            e.set_data(note, m_note_on_velocity);
            e.set_timestamp(tick);
            add_event(e);

            e.set_status(EVENT_NOTE_OFF);
            e.set_data(note, m_note_off_velocity);
            e.set_timestamp(tick + len);
            add_event(e);
        }
        verify_and_link();
    }
}

#ifdef SEQ64_STAZED_CHORD_GENERATOR

/**
 *  Adds a chord of a given length and  note value, at a given tick
 *  location.  If SEQ64_STAZED_CHORD_GENERATOR is not defined, it
 *  devolves to add_note().
 *
 * \threadsafe
 *
 * \param tick
 *      The time destination of the new note, in pulses.
 *
 * \param len
 *      The duration of the new note, in pulses.
 *
 * \param note
 *      The pitch destination of the new note.
 *
 * \param paint
 *      If true, repaint to be left with just the inserted event.
 */

void
sequence::add_chord (int chord, midipulse tick, midipulse len, int note)
{
    push_undo();
    if (chord > 0 && chord < c_chord_number)
    {
        for (int i = 0; i < c_chord_size; ++i)
        {
            int cnote = c_chord_table[chord][i];
            if (cnote == -1)
                break;

            add_note(tick, len, note + cnote, false);
        }
    }
    else
        add_note(tick, len, note, true);
}

#endif

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
 *  Here, we could ignore events not on the sequence's channel, as an option.
 *  We have to be careful because this function can be used in painting events.
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
 *
 * \return
 *      Returns true if the event was added.
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
 *  The paint parameter indicates if we care about the painted event,
 *  so then the function runs though the events and deletes the painted
 *  ones that overlap the ones we want to add.
 *
 * \threadsafe
 *
 * \param tick
 *      The time destination of the event.
 *
 * \param status
 *      The type of event to add.
 *
 * \param d0
 *      The first data byte for the event.
 *
 * \param d1
 *      The second data byte for the event (if needed).
 *
 * \param paint
 *      If true, the inserted event is marked for painting.
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
            for (i = m_events.begin(); i != m_events.end(); ++i)
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
            (void) remove_marked();
        }
        event e;
        if (paint)
            e.paint();

        e.set_status(status);
        e.set_data(d0, d1);
        e.set_timestamp(tick);
        add_event(e);
    }
    verify_and_link();
}

/**
 *  Streams the given event.  The event's timestamp is adjusted, if needed.
 *  If recording:
 *
 *      -   If the pattern is playing, the event is added.
 *      -   If the pattern is playing and quantized record is in force, the
 *          note's timestamp is altered.
 *      -   If not playing, but the event is a Note On or Note Off, we add it
 *          and keep track of it.
 *
 *  If MIDI Thru is enabled, the event is put on the buss.
 *
 *  We are adding a feature where events are rejected if their channel
 *  doesn't match that of the sequence.  This has been a complaint of some
 *  people.  Could modify the add_event() and add_note() functions, but
 *  better to do it here for comprehensive event support.  Also have to make
 *  sure the event-channel is preserved before this function is called, and
 *  also need to make sure that the channel is appended on both playback and
 *  in saving of the MIDI file.
 *
 *  We are also adding the usage, at last, of the m_rec_vol member.
 *
 * \todo
 *      When we feel like debugging, we will replace the global is-playing
 *      call with the parent perform's is-running call.
 *
 * \threadsafe
 *
 * \param ev
 *      Provides the event to stream.
 *
 * \return
 *      Returns true if the event's channel matched that of this sequence,
 *      and the channel-matching feature was set to true.  Also returns true
 *      if we're not using channel-matching.  A return value of true means
 *      the event should be saved.
 */

bool
sequence::stream_event (event & ev)
{
    automutex locker(m_mutex);
    bool result = channel_match(ev);            /* set if channel matches   */
    if (result)
    {
        ev.set_status(ev.get_status());         /* clear the channel nybble */
        ev.mod_timestamp(m_length);             /* adjust the tick          */
        if (m_recording)
        {
            if (m_parent->is_pattern_playing()) /* m_parent->is_running()   */
            {
                if (m_rec_vol > 0 && ev.is_note_on())
                    ev.set_note_velocity(m_rec_vol);

                add_event(ev);                          /* more locking     */
                set_dirty();
            }
            else
            {
                /*
                 * Supports the step-edit feature, so we set the generic
                 * default note length and volume to the snap.
                 */

                if (ev.is_note_on())
                {
                    push_undo();
                    add_note                            /* more locking     */
                    (
                        mod_last_tick(), m_snap_tick - m_note_off_margin,
                        ev.get_note(), false
                    );
                    set_dirty();
                    ++m_notes_on;
                }
                else if (ev.is_note_off())
                    --m_notes_on;

                if (m_notes_on <= 0)
                    m_last_tick += m_snap_tick;
            }
        }
        if (m_thru)
            put_event_on_bus(ev);                       /* more locking     */

        link_new();                                     /* more locking     */
        if (m_quantized_rec && m_parent->is_pattern_playing())
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
    return result;
}

/**
 *  Sets the dirty flags for names, main, and performance.  These flags are
 *  meant for causing user-interface refreshes, not for performance
 *  modification.
 *
 *  m_dirty_names is set to false in is_dirty_names(); m_dirty_names is set to
 *  false in is_dirty_main(); m_dirty_names is set to false in
 *  is_dirty_perf().
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
 *  flag to false.  Not sure that we need to lock a boolean on modern
 *  processors.
 *
 * \threadsafe
 *
 * \return
 *      Returns the dirty status.
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
 *
 * \return
 *      Returns the dirty status.
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
 *
 * \return
 *      Returns the dirty status.
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
 *  Returns the value of the dirty edit flag, and sets that flag to false.
 *  The m_dirty_edit flag is set by the function set_dirty().
 *
 * \threadsafe
 *
 * \return
 *      Returns the dirty status.
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
 *
 * \param note
 *      The note to play.  It is not checked for range validity, for the sake
 *      of speed.
 */

void
sequence::play_note_on (int note)
{
    automutex locker(m_mutex);
    event e;
    e.set_status(EVENT_NOTE_ON);
    e.set_data(note, m_note_on_velocity);           // SEQ64_MIDI_COUNT_MAX-1
    m_masterbus->play(m_bus, &e, m_midi_channel);
    m_masterbus->flush();
}

/**
 *  Turns off a note from the piano roll on the main bus on the master MIDI
 *  buss.
 *
 * \threadsafe
 *
 * \param note
 *      The note to turn off.  It is not checked for range validity, for the
 *      sake of speed.
 */

void
sequence::play_note_off (int note)
{
    automutex locker(m_mutex);
    event e;
    e.set_status(EVENT_NOTE_OFF);
    e.set_data(note, m_note_off_velocity);          // SEQ64_MIDI_COUNT_MAX-1
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
 *  Adds a trigger.  A pass-through function that calls triggers::add().
 *  See that function for more details.
 *
 * \threadsafe
 *
 * \param tick
 *      The time destination of the trigger.
 *
 * \param len
 *      The duration of the trigger.
 *
 * \param offset
 *      The performance offset of the trigger.
 *
 * \param fixoffset
 *      If true, adjust the offset.
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
 *  respectively, and then we exit.  See triggers::intersect().
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
 *  This function examines each note in the event list.  If the given position
 *  is between the current notes on and off time values, values, the these
 *  values are copied to the start and end parameters, respectively, the note
 *  value is copied to the note parameter, and then we exit.
 *
 * \threadsafe
 *
 * \param position
 *      The position to examine.
 *
 * \param position_note
 *      I think this is the note value we might be looking for ???
 *
 * \param [out] start
 *      The destination for the starting timestamp of the matching note.
 *
 * \param [out] ender
 *      The destination for the ending timestamp of the matching note.
 *
 * \param [out] note
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
 *  Grows a trigger.  See triggers::grow() for more information.
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
 *  See triggers::remove().
 *
 * \threadsafe
 *
 * \param tick
 *      Provides the tick to be used for finding the trigger to be erased.
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
 *  Splits a trigger.  This is the public overload of split_trigger.
 *
 * \threadsafe
 *
 * \param splittick
 *      The time location of the split.
 */

void
sequence::split_trigger (midipulse splittick)
{
    automutex locker(m_mutex);
    m_triggers.split(splittick);
}

/**
 *  Adjusts trigger offsets to the length specified for all triggers, and undo
 *  triggers.
 *
 * \threadsafe
 *
 *  Might can get rid of this function?
 *
 * \param newlength
 *      The new length of the adjusted trigger.
 */

void
sequence::adjust_trigger_offsets_to_length (midipulse newlength)
{
    automutex locker(m_mutex);
    m_triggers.adjust_offsets_to_length(newlength);
}

/**
 *  Copies triggers to another location.
 *
 * \threadsafe
 *
 * \param starttick
 *      The current location of the triggers.
 *
 * \param distance
 *      The distance away from the current location to which to copy the
 *      triggers.
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
 *
 * \param starttick
 *      The current location of the triggers.
 *
 * \param distance
 *      The distance away from the current location to which to move the
 *      triggers.
 *
 * \param direction
 *      If true, the triggers are moved forward. If false, the triggers are
 *      moved backward.
 */

void
sequence::move_triggers (midipulse starttick, midipulse distance, bool direction)
{
    automutex locker(m_mutex);
    m_triggers.move(starttick, distance, direction);
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
 *
 * \return
 *      Returns the tick_end() value of the last-selected trigger.  If no
 *      triggers are selected, then -1 is returned.
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
    midipulse tick, bool adjustoffset, triggers::grow_edit_t which
)
{
    automutex locker(m_mutex);
    return m_triggers.move_selected(tick, adjustoffset, which);
}

/**
 *  Get the ending value of the last trigger in the trigger-list.
 *
 * \threadsafe
 *
 * \return
 *      Returns the maximum trigger value.
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
 *  Returns a copy of the triggers for this sequence.  This function is
 *  basically a threadsafe version of sequence::triggerlist().
 *
 * \return
 *      Returns of copy of m_triggers.triggerlist().
 */

triggers::List
sequence::get_triggers () const
{
    automutex locker(m_mutex);
    return triggerlist();
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
 *      Returns true if a trigger is found that brackets the given tick;
 *      this is the return value of m_triggers.select().
 */

bool
sequence::select_trigger (midipulse tick)
{
    automutex locker(m_mutex);
    return m_triggers.select(tick);
}

/**
 *  Unselects all triggers.
 *
 * \return
 *      Returns the m_triggers.unselect() return value.
 */

bool
sequence::unselect_triggers ()
{
    automutex locker(m_mutex);
    return m_triggers.unselect();
}

/**
 *  Deletes the first selected trigger that is found.
 */

void
sequence::del_selected_trigger ()
{
    automutex locker(m_mutex);
    m_triggers.remove_selected();
}

/**
 *  Copies and deletes the first selected trigger that is found.
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
#ifdef USE_STAZED_TRIGGER_EXTENSIONS
    set_trigger_paste_tick(-1);         /* clear any unpasted middle-click  */
#endif
    m_triggers.copy_selected();
}

#ifdef USE_STAZED_TRIGGER_EXTENSIONS

void
sequence::get_sequence_triggers (std::vector<trigger> & trigvect)
{
    automutex locker(m_mutex);
    trigvect.assign(m_list_trigger.begin(), m_list_trigger.end());
}

#endif  // USE_STAZED_TRIGGER_EXTENSIONS

/**
 *  If there is a copied trigger, then this function grabs it from the trigger
 *  clipboard and adds it.
 *
 *  Why isn't this protected by a mutex?  We will enable this if anything bad
 *  happens, such as a deadlock, or corruption, that we can prove happens
 *  here.
 *
 * \param paste_tick
 *      A new parameter that provides the tick for pasting, or
 *      SEQ64_NO_PASTE_TRIGGER (-1) if there is none.
 */

void
sequence::paste_trigger (midipulse paste_tick)
{
    automutex locker(m_mutex);          /* @new ca 2016-08-03   */
    m_triggers.paste(paste_tick);
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
sequence::reset (bool live_mode)
{
    bool state = get_playing();
    off_playing_notes();
    set_playing(false);
    zero_markers();                 /* sets the "last-tick" value   */
    if (! live_mode)
        set_playing(state);
}

/**
 *  A pause version of reset().  The reset() function is currently not called
 *  when pausing, but we still need the note-shutoff capability to prevent
 *  notes from lingering.  Not that we do not call set_playing(false)... it
 *  disarms the sequence, which we do not want upon pausing.
 */

void
sequence::pause ()
{
    if (get_playing())
        off_playing_notes();
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
    for (event_list::iterator i = m_events.begin(); i != m_events.end(); ++i)
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
 *
 * \param [out] tick_s
 *      Provides a pointer destination for the start time.
 *
 * \param [out] tick_f
 *      Provides a pointer destination for the finish time.
 *
 * \param [out] note
 *      Provides a pointer destination for the note pitch value
 *      Probably should be a midibyte value.
 *
 * \param [out] selected
 *      Provides a pointer destination for the selection status of the note.
 *
 * \param [out] velocity
 *      Provides a pointer destination for the note velocity.
 *      Probably should be a midibyte value.
 */

draw_type_t
sequence::get_next_note_event
(
    midipulse * tick_s, midipulse * tick_f,
    int * note, bool * selected, int * velocity
)
{
    draw_type_t result = DRAW_FIN;
    *tick_f = 0;
    while (m_iterator_draw != m_events.end())
    {
        event & drawevent = DREF(m_iterator_draw);
        *tick_s   = drawevent.get_timestamp();
        *note     = drawevent.get_note();
        *selected = drawevent.is_selected();
        *velocity = drawevent.get_note_velocity();

        /* note on, so its linked */

        if (drawevent.is_note_on() && drawevent.is_linked())
        {
            *tick_f = drawevent.get_linked()->get_timestamp();
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
sequence::get_next_event (midibyte * status, midibyte * cc)
{
    while (m_iterator_draw != m_events.end())
    {
        midibyte j;
        event & drawevent = DREF(m_iterator_draw);
        *status = drawevent.get_status();
        drawevent.get_data(*cc, j);
        ++m_iterator_draw;
        return true;                /* we have a good one; update and return */
    }
    return false;
}

/**
 *  Get the next event in the event list that matches the given status and
 *  control character.  Then set the rest of the parameters parameters
 *  using that event.  If the status is the new value EVENT_ANY, then any
 *  event will be obtained.
 *
 *  Note the usage of event::is_desired_cc_or_not_cc(status, cc, *d0); Either
 *  we have a control change with the right CC or it's a different type of
 *  event.
 *
 * \param status
 *      The type of event to be obtained.  The special value EVENT_ANY can be
 *      provided so that no event statuses are filtered.
 *
 * \param cc
 *      The continuous controller value that might be desired.
 *
 * \param tick
 *      A pointer return value for the tick value of the next event found.
 *
 * \param d0
 *      A pointer return value for the first data value of the event.
 *
 * \param d1
 *      A pointer return value for the second data value of the event.
 *
 * \param selected
 *      A pointer return value for the is-selected status of the event.
 *
 * \param type
 *      A stazed parameter for picking either all event or unselected events.
 */

#ifdef USE_STAZED_SELECTION_EXTENSIONS

bool
sequence::get_next_event
(
    midibyte status, midibyte cc,
    midipulse * tick, midibyte * d0, midibyte * d1, bool * selected,
    int evtype
)
{
    while (m_iterator_draw != m_events.end())
    {
        event & drawevent = DREF(m_iterator_draw);
        bool ok = drawevent.get_status() == status;
        if (ok)
        {
            if (evtype == EVENTS_UNSELECTED && drawevent.is_selected())
            {
                ++m_iterator_draw;
                continue;           /* keep trying to find one              */
            }
            if (evtype > EVENTS_UNSELECTED && ! drawevent.is_selected())
            {
                ++m_iterator_draw;
                continue;           /* keep trying to find one              */
            }
            drawevent.get_data(*d0, *d1);
            *tick = drawevent.get_timestamp();
            *selected = drawevent.is_selected();
            if (event::is_desired_cc_or_not_cc(status, cc, *d0))
            {
                ++m_iterator_draw;  /* good one, so update and return       */
                return true;
            }
        }
        ++m_iterator_draw;          /* keep going until null or a Note On   */
    }
    return false;
}

#else   // USE_STAZED_SELECTION_EXTENSIONS

bool
sequence::get_next_event
(
    midibyte status, midibyte cc,
    midipulse * tick, midibyte * d0, midibyte * d1, bool * selected
)
{
    while (m_iterator_draw != m_events.end())
    {
        event & drawevent = DREF(m_iterator_draw);
        bool ok = drawevent.get_status() == status;
        if (! ok)
            ok = status == EVENT_ANY;

        if (ok)
        {
            drawevent.get_data(*d0, *d1);
            *tick = drawevent.get_timestamp();
            *selected = drawevent.is_selected();
            if (event::is_desired_cc_or_not_cc(status, cc, *d0))
            {
                ++m_iterator_draw;  /* good one, so update and return       */
                return true;
            }
        }
        ++m_iterator_draw;          /* keep going until null or a Note On   */
    }
    return false;
}

#endif  // USE_STAZED_SELECTION_EXTENSIONS

/**
 *  Get the next trigger in the trigger list, and set the parameters based
 *  on that trigger.
 */

bool
sequence::get_next_trigger
(
    midipulse * tick_on, midipulse * tick_off, bool * selected,
    midipulse * offset
)
{
    return m_triggers.next(tick_on, tick_off, selected, offset);
}

/**
 *  Clears all events from the event container.  Unsets the modified flag.
 *  (Why?) Also see the new copy_events() function.
 */

void
sequence::remove_all ()
{
    automutex locker(m_mutex);
    m_events.clear();
    m_events.unmodify();
}

/**
 * \setter m_last_tick
 *      This function used to be called "set_orig_tick()", now renamed to
 *      match up with get_last_tick().
 *
 * \threadsafe
 */

void
sequence::set_last_tick (midipulse tick)
{
    automutex locker(m_mutex);
    m_last_tick = tick;
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
 *  Sets the playing state of this sequence.  When playing, and the sequencer
 *  is running, notes get dumped to the ALSA buffers.
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
sequence::print () const
{
    m_events.print();
}

/**
 *  Prints a list of the currently-held triggers.
 *
 * \threadunsafe
 */

void
sequence::print_triggers () const
{
    m_triggers.print(m_name);
}

/**
 *  Takes an event that this sequence is holding, and places it on the MIDI
 *  buss.  This function does not bother checking if m_masterbus is a null
 *  pointer.
 *
 * \param ev
 *      The event to put on the buss.
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
    {
        /*
         * \change ca 2016-03-19
         *      Move the flush call into this condition; why flush() unless
         *      actually playing an event?
         */

        m_masterbus->play(m_bus, &ev, m_midi_channel);
        m_masterbus->flush();
    }
}

/**
 *  Sends a note-off event for all active notes.  This function does not
 *  bother checking if m_masterbus is a null pointer.
 *
 * \threadsafe
 */

void
sequence::off_playing_notes ()
{
    automutex locker(m_mutex);
    for (int x = 0; x < c_midi_notes; ++x)
    {
        while (m_playing_notes[x] > 0)
        {
            event e;
            e.set_status(EVENT_NOTE_OFF);
            e.set_data(x, 0);
            m_masterbus->play(m_bus, &e, m_midi_channel);
            --m_playing_notes[x];
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
 *
 * \param status
 *      Provides the status value to be selected.
 *
 * \param cc
 *      If the status is EVENT_CONTROL_CHANGE, then data byte 0 must
 *      match this value.
 *
 * \param inverse
 *      If true, invert the selection.
 *
 * \return
 *      Always returns 0.
 */

int
sequence::select_events
(
    midibyte status, midibyte cc, bool inverse
)
{
    automutex locker(m_mutex);
    midibyte d0, d1;
    for (event_list::iterator i = m_events.begin(); i != m_events.end(); ++i)
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
 *
 *  Also, we've moved external calls to push_undo() into this function.
 *  The caller shouldn't have to do that.
 *
 * \note
 *      We noticed (ca 2016-06-10) that MIDI aftertouch events need to be
 *      transposed, but are not being transposed here.  Assuming they are
 *      selectable (another question!), the test for note-on and note-off is
 *      not sufficient, and so has been replaced by a call to
 *      event::is_note_msg().
 *
 * \param steps
 *      The number of steps to transpose the notes.
 *
 * \param scale
 *      The scale to make the notes adhere to while transposing.
 */

void
sequence::transpose_notes (int steps, int scale)
{
    event_list transposed_events;
    const int * transpose_table = nullptr;
    automutex locker(m_mutex);
    m_events_undo.push(m_events);                   /* do this for callers  */
    if (mark_selected())                            /* mark original notes  */
    {
        if (steps < 0)
        {
            transpose_table = &c_scales_transpose_dn[scale][0];     /* down */
            steps *= -1;
        }
        else
            transpose_table = &c_scales_transpose_up[scale][0];     /* up   */

        for (event_list::iterator i = m_events.begin(); i != m_events.end(); ++i)
        {
            event & er = DREF(i);
            if (er.is_marked() && er.is_note())     /* transposable event?  */
            {
                event e = er;
                e.unmark();
                int note = e.get_note();
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
                transposed_events.add(e, false);    /* will sort afterward  */
            }
            else
                er.unmark();                        /* ignore, no transpose */
        }
        (void) remove_marked();                     /* remove original notes */
        m_events.merge(transposed_events);          /* events get presorted  */
        verify_and_link();
    }
}

#ifdef USE_STAZED_SHIFT_SUPPORT

void
sequence::shift_notes (int ticks)
{
    if (mark_selected())
    {
        automutex locker(m_mutex);
        event_list shifted_events;
        push_undo();
        for (event_list::iterator i = m_events.begin(); i != m_events.end(); ++i)
        {
            event & er = DREF(i);
            if (er.is_marked() && er.is_note())     /* shiftable event?  */
            {
                event e = er;
                e.unmark();

                midipulse timestamp = e.get_timestamp() + ticks;
                if (timestamp < 0L)                     /* wraparound */
                    timestamp = m_length - ((-timestamp) % m_length);
                else
                    timestamp %= m_length;

                e.set_timestamp(timestamp);
                shifted_events.add(e);
            }
        }
        (void) remove_marked();
#ifndef SEQ64_USE_EVENT_MAP
        shifted_events.sort();
        m_events.merge(shifted_events);
#endif
        verify_and_link();
    }
}

#endif  // USE_STAZED_SHIFT_SUPPORT

#ifdef SEQ64_STAZED_TRANSPOSE

/**
 *  Applies the transpose value held by the master MIDI buss object, if
 *  non-zero, and if the sequence is set to be transposable.
 */

void
sequence::apply_song_transpose ()
{
    int transpose = get_transposable() ? m_parent->get_transpose() : 0 ;
    if (transpose != 0)
    {
        automutex locker(m_mutex);
        m_events_undo.push(m_events);   /* push_undo() without lock         */
        for (event_list::iterator i = m_events.begin(); i != m_events.end(); ++i)
        {
            event & er = DREF(i);
            if (er.is_note())           /* includes aftertouch events       */
                er.transpose_note(transpose);
        }
        set_dirty();
    }
}

/**
 * \setter m_transposable
 *      Changing this flag modifies the sequence and performance.  Note that
 *      when a sequence is being read from a MIDI file, it will not yet have a
 *      parent, so we have to check for that before setting the perform modify
 *      flag.
 */

void
sequence::set_transposable (bool flag)
{
    if (flag != m_transposable)
    {
        if (not_nullptr(m_parent))
            m_parent->modify();
    }
    m_transposable = flag;
}

#endif

/**
 *  Grabs the specified events, puts them into a list, quantizes them against
 *  the snap ticks, and merges them in to the event container.  One confusing
 *  things is why the original versions of the events don't seem to be
 *  deleted.
 *
 * \param status
 *      Indicates the type of event to be quantized.
 *
 * \param cc
 *      The desired control-change to count, if the event is a control-change.
 *
 * \param snap_tick
 *      Provides the maximum amount to move the events.  Actually, events are
 *      moved to the previous or next snap_tick value depend on whether they
 *      are halfway to the next one or not.
 *
 * \param divide
 *      A rough indicator of the amount of quantization.  The only values used
 *      in the application seem to be either 1 or 2.
 *
 * \param linked
 *      False by default, this parameter indicates if marked events are to be
 *      relinked, as far as we can tell.
 */

void
sequence::quantize_events
(
    midibyte status, midibyte cc,
    midipulse snap_tick, int divide, bool linked
)
{
    automutex locker(m_mutex);
    if (mark_selected())
    {
        event_list quantized_events;
        for (event_list::iterator i = m_events.begin(); i != m_events.end(); ++i)
        {
            event & er = DREF(i);
            midibyte d0, d1;
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
                event e = er;                   /* copy the event               */
                er.select();                    /* selected the original event  */
                e.unmark();                     /* unmark the copy of the event */

                midipulse t = e.get_timestamp();
                midipulse t_remainder = t % snap_tick;
                midipulse t_delta = 0;
                if (t_remainder < snap_tick / 2)
                    t_delta = -(t_remainder / divide);
                else
                    t_delta = (snap_tick - t_remainder) / divide;

                if ((t_delta + t) >= m_length)
                    t_delta = -e.get_timestamp();

                e.set_timestamp(e.get_timestamp() + t_delta);
                quantized_events.add(e, false);
                if (er.is_linked() && linked)
                {
                    event f = *er.get_linked();
                    midipulse ft = f.get_timestamp();
                    f.unmark();
                    er.get_linked()->select();
                    f.set_timestamp(ft + t_delta);
                    quantized_events.add(f, false);
                }
            }
        }
        (void) remove_marked();
        m_events.merge(quantized_events);       /* quantized events presorted   */
        verify_and_link();
    }
}

/**
 *  A new convenience function.
 */

void
sequence::push_quantize
(
    midibyte status, midibyte cc,
    midipulse snap_tick, int divide, bool linked
)
{
    automutex locker(m_mutex);
    m_events_undo.push(m_events);
    quantize_events(status, cc, snap_tick, divide, linked);
}

#ifdef USE_STAZED_MULTIPLY_PATTERN

void
sequence::multiply_pattern (float multiplier )
{
    automutex locker(m_mutex);
    push_undo();
    long orig_length = get_length();
    long new_length = orig_length * multiplier;
    if (new_length > orig_length)
        set_length(new_length);

    for (event_list::iterator i = m_events.begin(); i != m_events.end(); ++i)
    {
        event & er = DREF(i);
        long timestamp = er.get_timestamp();
        if (er.is_note_off())
            timestamp += c_note_off_margin;

        timestamp *= multiplier;
        if (er.is_note_off())
            timestamp -= c_note_off_margin;

        timestamp %= m_length;
        er.set_timestamp(timestamp);
    }
    verify_and_link();
    if (new_length < orig_length)
        set_length(new_length);
}

#endif  // USE_STAZED_MULTIPLY_PATTERN

/**
 *  This function fills the given MIDI container with MIDI data from the
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

void
sequence::fill_container (midi_container & c, int tracknumber)
{
    automutex locker(m_mutex);
    c.fill(tracknumber);
}
 */

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
    for (event_list::const_iterator i = evl.begin(); i != evl.end(); ++i)
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
    if (m_events.count() > 0)               /* need at least 1 (2?) events  */
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

    /*
     * MODIFY FIX?
     */

    set_dirty();
    if (not_nullptr(m_parent))
        m_parent->modify();                 /* new, centralize it here      */
}

/**
 * \setter m_parent
 *      Sets the "parent" of this sequence, so that it can get some extra
 *      information about the performance.  Remember that m_parent is not at
 *      all owned by the sequence.  We just don't want to do all the work
 *      necessary to make it a reference, at this time.
 *
 * \param p
 *      A pointer to the parent, assigned only if not already assigned.
 */

void
sequence::set_parent (perform * p)
{
    if (is_nullptr(m_parent) && not_nullptr(p))
        m_parent = p;
}

#ifdef USE_THIS_COOL_FUNCTION

/**
 *  Provides encapsulation for a series of called used in perform::play().
 *  Just an idea to considered for the future.
 *
 *  Starts the playing of a pattern/sequence.  This function just has the
 *  sequence dump its events.  It ignores the sequence if it has no playable
 *  MIDI events.
 *
 */

void
sequence::play_queue (midipulse tick, bool playbackmode)
{
    if (event_count() > 0)               /* playable events? */
    {
        if (check_queued_tick(tick))
        {
            play(s->get_queued_tick() - 1, playbackmode);
            toggle_playing();
        }
        play(tick, playbackmode);
    }
}

#endif  // USE_THIS_COOL_FUNCTION

}           // namespace seq64

/*
 * sequence.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

