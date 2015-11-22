#ifndef SEQ64_PERFORM_HPP
#define SEQ64_PERFORM_HPP

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
 * \file          perform.hpp
 *
 *  This module declares/defines the base class for handling many facets
 *  of performing (playing) a full MIDI song.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2015-11-22
 * \license       GNU GPLv2 or above
 *
 *  This class has way too many members.
 */

#include <vector>
#include <pthread.h>

#include "globals.h"                    /* globals, nullptr, & more         */

#ifndef PLATFORM_WINDOWS                /* see globals.h, platform_macros.h */
#include <unistd.h>
#endif

#ifdef SEQ64_JACK_SUPPORT
#include "jack_assistant.hpp"
#endif

#include "gui_assistant.hpp"            /* seq64::gui_assistant             */
#include "keys_perform.hpp"             /* seq64::keys_perform              */
#include "mastermidibus.hpp"            /* seq64::mastermidibus             */
#include "midi_control.hpp"             /* seq64::midi_control "struct"     */

/**
 *  We have offloaded the keybinding support to another class, derived
 *  from keys_perform.  The new feature does seem to work.
 */

#define PERFKEY(x)              m_mainperf->keys().x()
#define PERFKEY_ADDR(x)         m_mainperf->keys().at_##x()

/**
 *  Uses a function returning a reference.
 */

#define PREFKEY(x)              perf().keys().x()
#define PREFKEY_ADDR(x)         perf().keys().at_##x()

/**
 *  Used in the options module to indicate a "key-labels-on-sequence" setting.
 *  Kind of weird, but we'll follow it blindly in adding the new
 *  "num-labels-on-sequence" setting, since it allows for immediate updating
 *  of the user-interface when the File / Options / Keyboard / Show Keys or
 *  Show Sequence Number settings change.
 */

#define PERFORM_KEY_LABELS_ON_SEQUENCE  9998
#define PERFORM_NUM_LABELS_ON_SEQUENCE  9999

namespace seq64
{

class keystroke;
class sequence;

/**
 *      Provides for notification of events.  Provide a response to a
 *      group-learn change event.
 */

struct performcallback
{

/*
 * ca 2015-07-24
 * Eliminate this annoying warning.  Will do it for Microsoft's bloddy
 * compiler later.
 */

#ifdef PLATFORM_GNU
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

    virtual void on_grouplearnchange (bool state)
    {
        // Empty body
    }

};

/**
 *  This class supports the performance mode.
 *
 *  It has way too many data members, many of the public.
 *  Might be ripe for refactoring.
 */

class perform
{

    friend class keybindentry;
    friend class midifile;
    friend class optionsfile;           // needs cleanup
    friend class options;

#ifdef SEQ64_JACK_SUPPORT

    friend int jack_sync_callback       // accesses perform::inner_start()
    (
        jack_transport_state_t state,
        jack_position_t * pos,
        void * arg
    );

#endif

private:

    /**
     *  Provides a dummy, inactive midi_control object to handle
     *  out-of-range midi_control indicies.
     */

    static midi_control sm_mc_dummy;

    /**
     *  Support for a wide range of GUI-related operations.
     */

    gui_assistant & m_gui_support;

    /**
     *  Mute group support.
     */

    bool m_mute_group[c_gmute_tracks];
    bool m_tracks_mute_state[c_seqs_in_set];
    bool m_mode_group;
    bool m_mode_group_learn;
    int m_mute_group_selected;

    /**
     *  Playing screen support.
     */

    int m_playing_screen;

    /**
     *  Playing screen sequence number offset.  Saves some multiplications,
     *  should make the code easier to grok, and centralizes the use of
     *  c_seqs_in_set, which we want to be able to change at run-time, as a
     *  future enhancement.
     */

    int m_playscreen_offset;

    /**
     *  Provides a "vector" of patterns/sequences.
     *
     * \todo
     *      First, make the sequence array a vector, and second, put allof
     *      these flags into a structure and access those members indirectly.
     */

    sequence * m_seqs[c_max_sequence];

    bool m_seqs_active[c_max_sequence];
    bool m_was_active_main[c_max_sequence];
    bool m_was_active_edit[c_max_sequence];
    bool m_was_active_perf[c_max_sequence];
    bool m_was_active_names[c_max_sequence];
    bool m_sequence_state[c_max_sequence];

    /**
     *  Provides our MIDI buss.
     */

    mastermidibus m_master_bus;

private:

    /**
     *  Provides information for managing pthreads.
     */

    pthread_t m_out_thread;
    pthread_t m_in_thread;
    bool m_out_thread_launched;
    bool m_in_thread_launched;

    /*
     * More variables.
     */

    bool m_running;
    bool m_inputing;
    bool m_outputing;
    bool m_looping;

    /**
     *  Specifies the playback mode.  There are two, "live" and "song",
     *  but we're not yet sure what "true" indicates.  It is most likely:
     *
    \verbatim
            m_playback_mode == false:       live mode
            m_playback_mode == true:        playback/song  mode
    \endverbatim
     *
     */

    bool m_playback_mode;

    /**
     *  Holds the current PPQN for usage in various actions.
     */

    int m_ppqn;

    /**
     *  Holds the "one measure's worth" of pulses (ticks), which is
     *  normally m_ppqn * 4.  We can save some multiplications, and, more
     *  importantly, later define a more flexible definition of "one measure's
     *  worth" than simply four quarter notes.
     */

    int m_one_measure;

    /**
     *  Holds the position of the left (L) marker, and it is first defined as
     *  0.  Note that "tick" is actually "pulses".
     */

    long m_left_tick;

    /**
     *  Holds the position of the right (R) marker, and it is first defined as
     *  the end of the fourth measure.  Note that "tick" is actually "pulses".
     */

    long m_right_tick;

    /**
     *  Holds the starting tick for playing.  By default, this value is always
     *  reset to the value of the "left tick".  We want to eventually be able
     *  to leave it at the last playing tick, to support a "pause"
     *  functionality. Note that "tick" is actually "pulses".
     */

    long m_starting_tick;

    /**
     *  MIDI Clock support.
     */

    long m_tick;
    bool m_usemidiclock;
    bool m_midiclockrunning;            // stopped or started
    int m_midiclocktick;
    int m_midiclockpos;

private:

    std::string m_screen_set_notepad[c_max_sets];

    midi_control m_midi_cc_toggle[c_midi_controls];
    midi_control m_midi_cc_on[c_midi_controls];
    midi_control m_midi_cc_off[c_midi_controls];

    int m_offset;
    int m_control_status;
    int m_screenset;

    /**
     *  We will eventually replace c_seqs_in_set with this member, which
     *  defaults to the value of c_seqs_in_set.  This change will require some
     *  arrays to be dynamically allocated (vectors).
     */

    int m_seqs_in_set;                  /* replaces global c_seqs_in_set    */

    /**
     *  A replacement for the c_max_sets constant.  Again, currently set to
     *  the old value, which is used in hard-wired array sizes.  To make it
     *  variable will require a move from arrays to vectors.
     */

    int m_max_sets;

    /**
     *  Keeps track of created sequences, whether or not they are active.
     *  Used by the install_sequence() function.
     */

    int m_sequence_count;

    /**
     *  A replacement for the c_max_sequence constant.  However, this value is
     *  already 32 * 32 = 1024, and is probably enough for any usage.  Famous
     *  last words?
     */

    int m_sequence_max;

    /**
     *  It may be a good idea to eventually centralize all of the dirtiness of
     *  a performance here.  All the GUIs seem to use a perform object.
     *  IN PROGRESS.
     */

    bool m_is_modified;

    /**
     *  A condition variable to protect...
     */

    condition_var m_condition_var;

    /**
     *  A wrapper object for the JACK support of this application.
     */

#ifdef SEQ64_JACK_SUPPORT
    jack_assistant m_jack_asst;         // implements most of the JACK stuff
#endif

public:

    /*
     *  Can register here for events.  Used in mainwnd and perform.
     */

    std::vector<performcallback *> m_notify;

public:

    perform (gui_assistant & mygui, int ppqn = SEQ64_USE_DEFAULT_PPQN);
    ~perform ();

    /**
     * \getter m_is_modfied
     */

    bool is_modified () const
    {
        return m_is_modified;
    }

    /**
     * \setter m_is_modified
     *      This setter only sets the modified-flag to true.
     *      The setter that will, is_modified(), is private.  No one but
     *      perfrom and its friends should falsify this flag.
     */

    void modify ()
    {
        m_is_modified = true;
    }

    /**
     * \getter m_sequence_count
     *      It is better to call this getter before bothering to even try to
     *      use a sequence.  In many cases at startup, or when loading a file,
     *      there are no sequences yet, and still the code calls functions
     *      that try to access them.
     */

    int sequence_count () const
    {
        return m_sequence_count;
    }

    /**
     * \getter m_sequence_max
     */

    int sequence_max () const
    {
        return m_sequence_max;
    }

    /**
     * \getter m_gui_support
     *      The const getter.
     */

    const gui_assistant & gui () const
    {
        return m_gui_support;
    }

    /**
     * \getter m_gui_support
     *      The un-const getter.
     */

    gui_assistant & gui ()
    {
        return m_gui_support;
    }

    /**
     * \getter m_gui_support.keys()
     *      The const getter.
     */

    const keys_perform & keys () const
    {
        return gui().keys();
    }

    /**
     * \getter m_gui_support.keys()
     *      The un-const getter.
     */

    keys_perform & keys ()
    {
        return gui().keys();
    }

    /**
     * \getter m_master_bus
     */

    mastermidibus & master_bus ()
    {
        return m_master_bus;
    }

    /**
     * \getter m_running
     */

    bool is_running () const
    {
        return m_running;
    }

    /**
     * \getter m_mode_group_learn
     */

    bool is_learn_mode () const
    {
        return m_mode_group_learn;
    }

    /**
     *  Adds a pointer to an object to be notified by this perform object.
     */

    void enregister (performcallback * pfcb)
    {
        if (not_nullptr(pfcb))
            m_notify.push_back(pfcb);
    }

    void init ();
    void clear_all ();

    void launch_input_thread ();
    void launch_output_thread ();
    void init_jack ();
    void deinit_jack ();

    void new_sequence (int seq);                    /* seqmenu & mainwid    */
    void add_sequence (sequence * seq, int perf);   /* midifile             */
    void delete_sequence (int seq);                 /* seqmenu & mainwid    */

    bool is_sequence_in_edit (int seq);
    void clear_sequence_triggers (int seq);

    /**
     * \getter m_tick
     */

    long get_tick () const
    {
        return m_tick;
    }

    void set_left_tick (long tick, bool setstart = true);   // too long to inline

    /**
     * \getter m_left_tick
     */

    long get_left_tick () const
    {
        return m_left_tick;
    }

    /**
     * \setter m_starting_tick
     */

    void set_start_tick (long tick)
    {
        m_starting_tick = tick;
    }

    /**
     * \getter m_starting_tick
     */

    long get_starting_tick () const
    {
        return m_starting_tick;
    }

    void set_right_tick (long tick, bool setstart = true);  // too long to inline

    /**
     * \getter m_right_tick
     */

    long get_right_tick () const
    {
        return m_right_tick;
    }

    void move_triggers (bool direction);
    void copy_triggers ();
    void push_trigger_undo ();
    void pop_trigger_undo ();
    void split_trigger (int seqnum, long tick);
    long get_max_trigger ();

    /**
     *  Convenience function for perfedit's collapse functionality.
     */

    void collapse ()
    {
        push_trigger_undo();
        move_triggers(false);
        is_modified(true);
    }

    /**
     *  Convenience function for perfedit's copy functionality.
     */

    void copy ()
    {
        push_trigger_undo();
        copy_triggers();
    }

    /**
     *  Convenience function for perfedit's expand functionality.
     */

    void expand ()
    {
        push_trigger_undo();
        move_triggers(true);
        is_modified(true);
    }

    midi_control & midi_control_toggle (int seq);
    midi_control & midi_control_on (int seq);
    midi_control & midi_control_off (int seq);

    void handle_midi_control (int control, bool state);

    const std::string & get_screen_set_notepad (int screen_set) const;

    /**
     *  Returns the notepad text for the current screen-set.
     */

    const std::string & current_screen_set_notepad () const
    {
        return get_screen_set_notepad(m_screenset);
    }

    void set_screen_set_notepad (int screenset, const std::string & note);

    /**
     *  Sets the notepad text for the current screen-set.
     */

    void set_screen_set_notepad (const std::string & note)
    {
        set_screen_set_notepad(m_screenset, note);
    }

    void set_screenset (int ss);        // a little much to inline

    /**
     * \getter m_screenset
     */

    int get_screenset () const
    {
        return m_screenset;
    }

    void set_playing_screenset ();      // a little much to inline

    /**
     * \getter m_playing_screen
     */

    int get_playing_screenset () const
    {
        return m_playing_screen;
    }

    void mute_group_tracks ();
    void select_and_mute_group (int g_group);

    /**
     * \setter m_mode_group
     */

    void set_mode_group_mute ()
    {
        m_mode_group = true;
    }

    /**
     * \setter m_mode_group
     *      Unsets this member.
     */

    void unset_mode_group_mute ()
    {
        m_mode_group = false;
    }
    void select_group_mute (int g_mute);
    void set_mode_group_learn ();
    void unset_mode_group_learn ();
    bool is_group_learning (void)
    {
        return m_mode_group_learn;
    }
    void select_mute_group (int group);
    void start (bool state);
    void stop ();

    /*
     * bool jack_session_event (); Replaced by jack_assistant::session_event().
     */

    void start_jack ();
    void stop_jack ();
    void position_jack (bool state);

    void off_sequences ();
    void all_notes_off ();

    void set_active (int seq, bool active);
    void set_was_active (int seq);
    bool is_dirty_main (int seq);
    bool is_dirty_edit (int seq);
    bool is_dirty_perf (int seq);
    bool is_dirty_names (int seq);

    /**
     *  Checks the pattern/sequence for activity.
     *
     * \todo
     *      We should have the sequence object keep track of its own activity
     *      and access that via a reference or pointer.
     *
     * \param seq
     *      The pattern number.  It is checked for invalidity.  This can
     *      lead to "too many" (i.e. redundant) checks, but we're trying to
     *      centralize such checks in this function.
     *
     * \return
     *      Returns the value of the active-flag, or false if the sequence was
     *      invalid or null.
     */

    bool is_active (int seq)
    {
        return is_mseq_valid(seq) ? m_seqs_active[seq] : false ;
    }

    /**
     *  Retrieves the actual sequence, based on the pattern/sequence number.
     *
     * \param seq
     *      The prospective sequence number.
     *
     * \return
     *      Returns the value of m_seqs[seq] if seq is valid.  Otherwise, a
     *      null pointer is returned.
     */

    sequence * get_sequence (int seq)
    {
        return is_mseq_valid(seq) ? m_seqs[seq] : nullptr ;
    }

    void reset_sequences ();

    /**
     *  Plays all notes to the current tick.
     */

    void play (long tick);
    void set_orig_ticks (long tick);
    void set_beats_per_minute (int bpm);        /* more than just a setter  */
    int get_beats_per_minute ();                /* get BPM from the buss    */

    /**
     * \setter m_looping
     */

    void set_looping (bool looping)
    {
        m_looping = looping;
    }

    void set_sequence_control_status (int status);
    void unset_sequence_control_status (int status);
    void sequence_playing_toggle (int seq);
    void sequence_playing_on (int seq);
    void sequence_playing_off (int seq);
    void set_group_mute_state (int g_track, bool mute_state);
    bool get_group_mute_state (int g_track);
    void mute_all_tracks ();
    void output_func ();
    void input_func ();

    /**
     *  Calculates the offset into the screen sets.
     *
     *  Sets m_offset = offset * c_mainwnd_rows * c_mainwnd_cols;
     *
     * \param offset
     *      The desired offset.
     */

    void set_offset (int offset)
    {
        m_offset = offset * c_mainwnd_rows * c_mainwnd_cols;
    }

    void save_playing_state ();
    void restore_playing_state ();

    /*
     * Here follows a few forwarding functions for the keys_perform-derived
     * classes.
     */

    std::string key_name (unsigned int k) const
    {
        return keys().key_name(k);
    }

    keys_perform::SlotMap & get_key_events ()
    {
        return keys().get_key_events();
    }

    keys_perform::SlotMap & get_key_groups ()
    {
        return keys().get_key_groups();
    }

    keys_perform::RevSlotMap & get_key_events_rev ()
    {
        return keys().get_key_events_rev();
    }

    keys_perform::RevSlotMap & get_key_groups_rev ()
    {
        return keys().get_key_groups_rev();
    }

    /**
     * \accessor m_show_ui_sequency_key
     *      Provides access to keys().show_ui_sequence_key().
     *      Used in mainwid, options, optionsfile, userfile, and perform.
     */

    bool show_ui_sequence_key () const
    {
        return keys().show_ui_sequence_key();
    }
    void show_ui_sequence_key (bool flag)
    {
        keys().show_ui_sequence_key(flag);
    }

    /**
     * \accessor m_show_ui_sequency_number
     *      Provides access to keys().show_ui_sequence_number().
     *      Used in mainwid, optionsfile, and perform.
     */

    bool show_ui_sequence_number () const
    {
        return keys().show_ui_sequence_number();
    }
    void show_ui_sequence_number (bool flag)
    {
        keys().show_ui_sequence_number(flag);
    }

    /*
     * Getters of keyboard mapping for sequence and groups.
     * If not found, returns something "safe" [so use get_key()->count()
     * to see if it's there first]
     */

    unsigned int lookup_keyevent_key (long seqnum)
    {
        if (get_key_events_rev().count(seqnum) > 0)
            return get_key_events_rev()[seqnum];
        else
            return '?';
    }
    long lookup_keyevent_seq (unsigned int keycode)
    {
        if (get_key_events().count(keycode) > 0)
            return get_key_events()[keycode];
        else
            return 0;
    }
    unsigned int lookup_keygroup_key (long groupnum)
    {
        if (get_key_groups_rev().count(groupnum))
            return get_key_groups_rev()[groupnum];
        else
            return '?';
    }
    long lookup_keygroup_group (unsigned int keycode)
    {
        if (get_key_groups().count(keycode))
            return get_key_groups()[keycode];
        else
            return 0;
    }

    /**
     * \getter rc().is_pattern_playing()
     *      Provide a convenience function so that clients don't have to mess
     *      with a global variable when they're dealing with a perform object.
     */

    bool is_playing () const
    {
        return rc().is_pattern_playing();
    }

    /**
     *  Encapsulates a series of calls used in mainwnd.
     *  We've reversed the start() and start_jack() calls so that
     *  JACK is started first, to match all of the other use-cases for playing
     *  that we've found in the code.
     *
     * \todo
     *      Verify the usage and nature of this flag.
     */

    void start_playing (bool flag = false)
    {
        position_jack(flag);
        start_jack();
        start(flag);
        rc().is_pattern_playing(true);
    }

    /**
     *  Encapsulates a series of calls used in mainwnd.
     */

    void stop_playing ()
    {
        stop_jack();
        stop();
        rc().is_pattern_playing(false);
    }

    /**
     *  Encapsulates some calls used in mainwnd.
     */

    void learn_toggle ()
    {
        if (is_group_learning())
            unset_mode_group_learn();
        else
            set_mode_group_learn();
    }

    /**
     *  Encapsulates some calls used in mainwnd.  Actually does a lot of
     *  work in those function calls.
     */

    int decrement_beats_per_minute ()
    {
        int result = get_beats_per_minute() - 1;
        set_beats_per_minute(result);
        return result;
    }

    /**
     *  Encapsulates some calls used in mainwnd.  Actually does a lot of
     *  work in those function calls.
     */

    int increment_beats_per_minute ()
    {
        int result = get_beats_per_minute() + 1;
        set_beats_per_minute(result);
        return result;
    }

    /**
     *  Encapsulates some calls used in mainwnd.
     */

    int decrement_screenset ()
    {
        int result = get_screenset() - 1;
        set_screenset(result);
        return result;
    }

    /**
     *  Encapsulates some calls used in mainwnd.
     */

    int increment_screenset ()
    {
        int result = get_screenset() + 1;
        set_screenset(result);
        return result;
    }

    bool highlight (const sequence & seq) const;

    void sequence_key (int seq);                        // encapsulation
    std::string sequence_label(const sequence & seq);
    void set_input_bus (int bus, bool input_active);    // used in options
    bool mainwnd_key_event (const keystroke & k);
    bool perfroll_key_event (const keystroke & k, int drop_sequence);

private:

    bool seq_in_playing_screen (int seq);

    /**
     * \setter m_is_modified
     *      This setter is private.  The modify() setter, which is public, can
     *      only set m_is_motified to true.
     */

    void is_modified (bool flag)
    {
        m_is_modified = flag;
    }

    /**
     *  Checks the parameter against c_midi_controls.
     *
     * \param seq
     *      The value that should be in the c_midi_controls range.
     *
     * \return
     *      Returns true if the parameter is valid.  For this function, no
     *      error print-out is generated.
     */

    bool is_midi_control_valid (int seq) const
    {
        return seq < c_midi_controls;
    }

    /**
     *  Checks the screenset against m_max_sets.
     *
     * \param screenset
     *      The prospective screenset value.
     *
     * \return
     *      Returns true if the parameter is valid.  For this function, no
     *      error print-out is generated.
     */

    bool is_screenset_valid (int screenset) const
    {
        return screenset >= 0 && screenset < m_max_sets;
    }

    /**
     * \setter m_running
     */

    void set_running (bool running)
    {
        m_running = running;
    }

    /**
     * \setter m_playback_mode
     */

    void set_playback_mode (bool playbackmode)
    {
        m_playback_mode = playbackmode;
    }

    /**
     *  A helper function to calculate the index into the mute-group array,
     *  based on the desired track.
     */

    int mute_group_offset (int track)
    {
        return clamp_track(track) + m_mute_group_selected * c_seqs_in_set;
    }

    bool is_seq_valid (int seq) const;
    bool is_mseq_valid (int seq) const;
    bool install_sequence (sequence * seq, int seqnum);
    void inner_start (bool state);
    void inner_stop ();
    int clamp_track (int track) const;

    /**
     *  Pass-along function for keys().set_all_key_events.
     */

    void set_all_key_events ()
    {
        keys().set_all_key_events();
    }

    /**
     *  Pass-along function for keys().set_all_key_events.
     */

    void set_all_key_groups ()
    {
        keys().set_all_key_groups();
    }

    /**
     *  At construction time, this function sets up one keycode and one event
     *  slot.  It is called 32 times, corresponding to the pattern/sequence
     *  slots in the Patterns window.  It first removes the given key-code
     *  from the regular and reverse slot-maps.  Then it removes the
     *  sequence-slot from the regular and reverse slot-maps.  Finally, it
     *  adds the sequence-slot with a key value of key-code, and adds the
     *  key-code with a value of sequence-slot.
     */

    void set_key_event (unsigned int keycode, long sequence_slot)
    {
        keys().set_key_event(keycode, sequence_slot);
    }

    /**
     *  At construction time, this function sets up one keycode and one group
     *  slot.  It is called 32 times, corresponding the pattern/sequence slots
     *  in the Patterns window.  Compare it to the set_key_events() function.
     */

    void set_key_group (unsigned int keycode, long group_slot)
    {
        keys().set_key_group(keycode, group_slot);
    }

};

/**
 * Global functions defined in perform.cpp.
 */

extern void * output_thread_func (void * p);
extern void * input_thread_func (void * p);

}           // namespace seq64

#endif      // SEQ64_PERFORM_HPP

/*
 * perform.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

