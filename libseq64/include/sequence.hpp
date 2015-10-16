#ifndef SEQ64_SEQUENCE_HPP
#define SEQ64_SEQUENCE_HPP

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
 * \file          sequence.hpp
 *
 *  This module declares/defines the base class for handling
 *  patterns/sequences.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2015-10-15
 * \license       GNU GPLv2 or above
 *
 */

#include <string>
#include <list>
#include <stack>

#include "easy_macros.h"
#include "event_list.hpp"

#ifdef PLATFORM_WINDOWS
#include "midibus_portmidi.hpp"
#else
#include "midibus.hpp"
#endif

#include "midi_container.hpp"           /* seq64::midi_container    */
#include "mutex.hpp"

namespace seq64
{

/**
 *  Provides a set of methods for drawing certain items.
 *
 * \var DRAW_FIN
 *
 * \var DRAW_NORMAL_LINKED
 *
 * \var DRAW_NOTE_ON
 *
 * \var DRAW_NOTE_OFF
 */

enum draw_type
{
    DRAW_FIN = 0,
    DRAW_NORMAL_LINKED,
    DRAW_NOTE_ON,
    DRAW_NOTE_OFF
};

/**
 *  This class is used in playback.  Making its members public makes it
 *  really "just" a structure.
 */

class trigger
{

public:

    long m_tick_start;
    long m_tick_end;
    long m_offset;
    bool m_selected;

public:

    /**
     *  Initializes the trigger structure.
     */

    trigger () :
        m_tick_start    (0),
        m_tick_end      (0),
        m_offset        (0),
        m_selected      (false)
    {
        // Empty body
    }

    /**
     *  This operator compares only the m_tick_start members.
     */

    bool operator < (const trigger & rhs)
    {
        return m_tick_start < rhs.m_tick_start;
    }

};

/**
 *  The sequence class is firstly a receptable for a single track of MIDI
 *  data read from a MIDI file or edited into a pattern.  More members than
 *  you can shake a stick at.
 */

class sequence
{

public:

    /**
     *  Exposes the triggers, currently needed for midi_container only.
     */

    typedef std::list<trigger> Triggers;

    /**
     *  This enumeration is used in selecting events and note.  Se the
     *  select_note_events() and select_events() functions.
     *
     * \var e_select
     *
     * \var e_select_one
     *
     * \var e_is_selected
     *
     * \var e_would_select
     *
     * \var e_deselect
     *      To deselect the event under the cursor.
     *
     * \var e_toggle_selection
     *      To toggle the selection of the event under the cursor.
     *
     * \var e_remove_one
     *      To remove one note under the cursor.
     */

    enum select_action_e
    {
        e_select,
        e_select_one,
        e_is_selected,
        e_would_select,
        e_deselect,
        e_toggle_selection,
        e_remove_one
    };

private:

    typedef std::stack<event_list> EventStack;
    typedef std::stack<Triggers> TriggerStack;

private:

    static event_list m_events_clipboard;

    /**
     *  This list holds the current pattern/sequence events.
     */

    event_list m_events;
    Triggers m_triggers;
    trigger m_trigger_clipboard;
    EventStack m_events_undo;
    EventStack m_events_redo;
    TriggerStack m_triggers_undo;
    TriggerStack m_triggers_redo;

    /* markers */

    event_list::iterator m_iterator_play;
    event_list::iterator m_iterator_draw;
    Triggers::iterator m_iterator_play_trigger;
    Triggers::iterator m_iterator_draw_trigger;

    /* contains the proper MIDI channel */

    char m_midi_channel;
    char m_bus;

    /* song playback mode mute */

    bool m_song_mute;

    /* polyphonic step edit note counter */

    int m_notes_on;

    /* outputs to sequence to this Bus on midichannel */

    mastermidibus * m_masterbus;

    /* map for noteon, used when muting, to shut off current messages */

    int m_playing_notes[c_midi_notes];

    /* states */

    bool m_was_playing;
    bool m_playing;
    bool m_recording;
    bool m_quantized_rec;
    bool m_thru;
    bool m_queued;
    bool m_trigger_copied;

    /* flag indicates that contents has changed from a recording */

    bool m_dirty_main;
    bool m_dirty_edit;
    bool m_dirty_perf;
    bool m_dirty_names;

    /* anything editing currently ? */

    bool m_editing;
    bool m_raise;

    /* named sequence */

    std::string m_name;

    /* where were we */

    long m_last_tick;
    long m_queued_tick;
    long m_trigger_offset;
    const int m_maxbeats;
    int m_ppqn;

    /* length of sequence in pulses should be powers of two in bars */

    long m_length;
    long m_snap_tick;

    /* these are just for the editor to mark things in correct time */

    long m_time_beats_per_measure;
    long m_time_beat_width;
    long m_rec_vol;

    /**
     *  Provides locking for the sequence.  Made mutable for use in
     *  certain locked getter functions.
     */

    mutable mutex m_mutex;

public:

    sequence (int ppqn = SEQ64_USE_DEFAULT_PPQN);
    ~sequence ();

    sequence & operator = (const sequence & rhs);

    /**
     * \getter m_events
     */

    event_list & events ()
    {
        return m_events;
    }

    /**
     * \getter m_triggers
     */

    Triggers & triggers ()
    {
        return m_triggers;
    }

    int event_count () const;
    void push_undo ();
    void pop_undo ();
    void pop_redo ();
    void push_trigger_undo ();
    void pop_trigger_undo ();
    void set_name (const std::string & name);
    void set_name (char * name);
    void set_measures (long length_measures);
    long get_measures ();
    void set_bpm (long beats_per_measure);

    /**
     * \getter m_time_beats_per_measure
     */

    long get_bpm () const
    {
        return m_time_beats_per_measure;
    }

    void set_bw (long beat_width);

    /**
     * \getter m_time_beat_width
     *
     * \threadsafe
     */

    long get_bw () const
    {
        return m_time_beat_width;
    }

    void set_rec_vol (long rec_vol);

    /**
     * \setter m_song_mute
     */

    void set_song_mute (bool mute)
    {
        m_song_mute = mute;
    }

    /**
     * \getter m_song_mute
     */

    bool get_song_mute () const
    {
        return m_song_mute;
    }

    /**
     * \getter m_name pointer
     */

    const char * get_name () const
    {
        return m_name.c_str();
    }

    /**
     * \getter m_name
     */

    const std::string & name () const
    {
        return m_name;
    }

    /**
     * \setter m_editing
     */

    void set_editing (bool edit)
    {
        m_editing = edit;
    }

    /**
     * \getter m_editing
     */

    bool get_editing () const
    {
        return m_editing;
    }

    /**
     * \setter m_raise
     */

    void set_raise (bool edit)
    {
        m_raise = edit;
    }

    /**
     * \getter m_raise
     */

    bool get_raise (void) const
    {
        return m_raise;
    }

    void set_length (long len, bool adjust_triggers = true); /* in ticks */

    /**
     * \getter m_length
     */

    long get_length () const
    {
        return m_length;
    }

    long get_last_tick ();
    void set_playing (bool);

    /**
     * \getter m_playing
     */

    bool get_playing () const
    {
        return m_playing;
    }

    void toggle_playing ();
    void toggle_queued ();
    void off_queued ();

    /**
     * \getter m_queued
     */

    bool get_queued () const
    {
        return m_queued;
    }

    /**
     * \getter m_queued_tick
     */

    long get_queued_tick () const
    {
        return m_queued_tick;
    }

    void set_recording (bool);

    /**
     * \getter m_recording
     */

    bool get_recording () const
    {
        return m_recording;
    }

    void set_snap_tick (int st);
    void set_quantized_rec (bool qr);

    /**
     * \getter m_quantized_rec
     */

    bool get_quantized_rec () const
    {
        return m_quantized_rec;
    }

    void set_thru (bool);

    /**
     * \getter m_thru
     */

    bool get_thru () const
    {
        return m_thru;
    }

    bool is_dirty_main ();
    bool is_dirty_edit ();
    bool is_dirty_perf ();
    bool is_dirty_names ();
    void set_dirty_mp ();
    void set_dirty ();

    /**
     * \getter m_midi_channel
     */

    unsigned char get_midi_channel () const
    {
        return m_midi_channel;
    }

    void set_midi_channel (unsigned char ch);
    void print ();
    void print_triggers ();
    void play (long tick, bool playback_mode);
    void set_orig_tick (long tick);
    void add_event (const event * e);
    void add_trigger
    (
        long tick, long length,
        long offset = 0, bool adjust_offset = true
    );
    void split_trigger (long tick);
    void grow_trigger (long tick_from, long tick_to, long length);
    void del_trigger (long tick);
    bool get_trigger_state (long tick);
    bool select_trigger (long tick);
    bool unselect_triggers ();
    bool intersectTriggers (long position, long & start, long & end);
    bool intersectNotes
    (
        long position, long position_note,
        long & start, long & end, long & note
    );
    bool intersectEvents (long posstart, long posend, long status, long & start);
    void del_selected_trigger ();
    void cut_selected_trigger ();
    void copy_selected_trigger ();
    void paste_trigger ();
    void move_selected_triggers_to
    (
        long tick, bool adjust_offset, int which = 2
    );
    long selected_trigger_start ();
    long selected_trigger_end ();
    long get_max_trigger ();
    void move_triggers (long start_tick, long distance, bool direction);
    void copy_triggers (long start_tick, long distance);
    void clear_triggers ();

    /**
     * \getter m_trigger_offset
     */

    long get_trigger_offset () const
    {
        return m_trigger_offset;
    }

    void set_midi_bus (char mb);

    /**
     * \getter m_bus
     */

    char get_midi_bus () const
    {
        return m_bus;
    }

    void set_master_midi_bus (mastermidibus * mmb);
    int select_note_events
    (
        long tick_s, int note_h,
        long tick_f, int note_l, select_action_e action
    );
    int select_events
    (
        long tick_s, long tick_f,
        unsigned char status, unsigned char cc, select_action_e action
    );
    int select_events
    (
        unsigned char status, unsigned char cc, bool inverse = false
    );
    int get_num_selected_notes ();
    int get_num_selected_events (unsigned char status, unsigned char cc);
    void select_all ();
    void copy_selected ();
    void paste_selected (long tick, int note);
    void get_selected_box
    (
        long & tick_s, int & note_h, long & tick_f, int & note_l
    );
    void get_clipboard_box
    (
        long & tick_s, int & note_h, long & tick_f, int & note_l
    );
    void move_selected_notes (long delta_tick, int delta_note);
    void add_note (long tick, long length, int note, bool paint = false);
    void add_event
    (
        long tick, unsigned char status,
        unsigned char d0, unsigned char d1, bool paint = false
    );
    void stream_event (event * ev);
    void change_event_data_range
    (
        long tick_s, long tick_f,
        unsigned char status, unsigned char cc,
        int d_s, int d_f
    );
    void increment_selected (unsigned char status, unsigned char control);
    void decrement_selected (unsigned char status, unsigned char control);
    void grow_selected (long delta_tick);
    void stretch_selected (long delta_tick);
    void remove_marked ();
    void mark_selected ();
    void unpaint_all ();
    void unselect ();
    void verify_and_link ();
    void link_new ();
    void zero_markers ();
    void play_note_on (int note);
    void play_note_off (int note);
    void off_playing_notes ();
    void reset_draw_marker ();
    void reset_draw_trigger_marker ();
    draw_type get_next_note_event
    (
        long * tick_s, long * tick_f, int * note,
        bool * selected, int * velocity
    );
    int get_lowest_note_event ();
    int get_highest_note_event ();
    bool get_next_event
    (
        unsigned char status, unsigned char cc,
        long * tick, unsigned char * d0, unsigned char * d1,
        bool * selected
    );
    bool get_next_event (unsigned char * status, unsigned char * cc);
    bool get_next_trigger
    (
        long * tick_on, long * tick_off,
        bool * selected, long * tick_offset
    );
    void fill_container (midi_container & c, int tracknumber);
    void quantize_events
    (
        unsigned char status, unsigned char cc,
        long snap_tick, int divide, bool linked = false
    );
    void transpose_notes (int steps, int scale);

private:

#if 0
    /*
     * Used in fill_list().
     */

    void add_list_var (midi_container & c, long v);
    void add_long_list (midi_container & c, long x);
#endif  // 0

    void put_event_on_bus (event * ev);
    void remove_all ();
    void set_trigger_offset (long trigger_offset);
    void split_trigger (trigger & trig, long split_tick);
    void adjust_trigger_offsets_to_length (long new_len);
    long adjust_offset (long offset);
    void remove (event_list::iterator i);
    void remove (event * e);

};

}           // namespace seq64

#endif      // SEQ64_SEQUENCE_HPP

/*
 * sequence.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
