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
 * \updates       2015-10-13
 * \license       GNU GPLv2 or above
 *
 *  This class has way too many members.
 */

#include <vector>                       // std::vector
#include <pthread.h>
#include "globals.h"                    // globals, nullptr, & config headers

#ifndef PLATFORM_WINDOWS                // see globals.h, platform_macros.h
#include <unistd.h>
#endif

#ifdef SEQ64_JACK_SUPPORT
#include "jack_assistant.hpp"
#endif

#include "gui_assistant.hpp"            // seq64::gui_assistant
#include "keys_perform.hpp"             // seq64::keys_perform
#include "mastermidibus.hpp"            // seq64::mastermidibus

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
 */

#define PERFORM_KEY_LABELS_ON_SEQUENCE  9999

namespace seq64
{

class keystroke;
class sequence;

/*
 * This class (actually a struct) contains sequences that make up a live
 * set.
 */

class midi_control
{

public:

    bool m_active;
    bool m_inverse_active;
    long m_status;
    long m_data;
    long m_min_value;
    long m_max_value;
};

/*
 *  Pseudo control value for associating MIDI events (I think)
 *  with automation of some of the controls in seq24.  The lowest value is
 *  c_seqs_in_set * 2 = 64.
 *
 *  I think the reason for that value is to perhaps handle two sets or
 *  something like that.  Will figure it out later.
 */

const int c_midi_track_ctrl           = c_seqs_in_set * 2;
const int c_midi_control_bpm_up       = c_midi_track_ctrl;
const int c_midi_control_bpm_dn       = c_midi_track_ctrl + 1;
const int c_midi_control_ss_up        = c_midi_track_ctrl + 2;
const int c_midi_control_ss_dn        = c_midi_track_ctrl + 3;
const int c_midi_control_mod_replace  = c_midi_track_ctrl + 4;
const int c_midi_control_mod_snapshot = c_midi_track_ctrl + 5;
const int c_midi_control_mod_queue    = c_midi_track_ctrl + 6;
const int c_midi_control_mod_gmute    = c_midi_track_ctrl + 7;
const int c_midi_control_mod_glearn   = c_midi_track_ctrl + 8;
const int c_midi_control_play_ss      = c_midi_track_ctrl + 9;
const int c_midi_controls             = c_midi_track_ctrl + 10;

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
     *  Provides a vector of patterns/sequences.
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
     *  but we're not yet sure what "true" indicates.
     */

    bool m_playback_mode;
    const int m_ppqn;

    long m_left_tick;
    long m_right_tick;
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
    int m_screen_set;
    int m_sequence_max;

    condition_var m_condition_var;

#ifdef SEQ64_JACK_SUPPORT
    jack_assistant m_jack_asst;         // implements most of the JACK stuff
#endif

public:

    /*
     *  Can register here for events.  Used in mainwnd and perform.
     */

    std::vector<performcallback *> m_notify;

public:

    perform (gui_assistant & mygui);
    ~perform ();

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

    void add_sequence (sequence * a_seq, int a_perf);
    void delete_sequence (int a_num);
    bool is_sequence_in_edit (int a_num);
    void clear_sequence_triggers (int a_seq);

    /**
     *  Provides common code to check for the bounds of a sequence number.
     *
     * \return
     *      Returns true if the sequence number is valid.
     */

    bool is_sequence_valid (int a_sequence) const
    {
        return a_sequence >= 0 || a_sequence < c_max_sequence;
    }

    /**
     *  Provides common code to check for the bounds of a sequence number.
     *
     * \return
     *      Returns true if the sequence number is invalid.
     */

    bool is_sequence_invalid (int a_sequence) const
    {
        return ! is_sequence_valid(a_sequence);
    }

    long get_tick () const
    {
        return m_tick;
    }

    void set_left_tick (long a_tick);       // too long to inline

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

    void set_starting_tick (long a_tick)
    {
        m_starting_tick = a_tick;
    }

    /**
     * \getter m_starting_tick
     */

    long get_starting_tick () const
    {
        return m_starting_tick;
    }

    void set_right_tick (long a_tick);          // too long to inline

    /**
     * \getter m_right_tick
     */

    long get_right_tick () const
    {
        return m_right_tick;
    }

    void move_triggers (bool a_direction);
    void copy_triggers ();
    void push_trigger_undo ();
    void pop_trigger_undo ();

    void print ();

    midi_control * get_midi_control_toggle (unsigned int a_seq);
    midi_control * get_midi_control_on (unsigned int a_seq);
    midi_control * get_midi_control_off (unsigned int a_seq);

    void handle_midi_control (int a_control, bool a_state);

    const std::string & get_screen_set_notepad (int a_screen_set) const;

    /**
     *  Returns the notepad text for the current screen-set.
     */

    const std::string & current_screen_set_notepad () const
    {
        return get_screen_set_notepad(m_screen_set);
    }

    void set_screen_set_notepad (int screenset, const std::string & note);

    /**
     *  Sets the notepad text for the current screen-set.
     */

    void set_current_screen_set_notepad (const std::string & note)
    {
        set_screen_set_notepad(m_screen_set, note);
    }

    void set_screenset (int a_ss);      // a little much to inline

    /**
     * \getter m_screen_set
     */

    int get_screenset () const
    {
        return m_screen_set;
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
    void select_and_mute_group (int a_g_group);

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
    void select_group_mute (int a_g_mute);
    void set_mode_group_learn ();
    void unset_mode_group_learn ();
    bool is_group_learning (void)
    {
        return m_mode_group_learn;
    }
    void select_mute_group (int a_group);
    void start (bool a_state);
    void stop ();

    /*
     * bool jack_session_event (); Replaced by jack_assistant::session_event().
     */

    void start_jack ();
    void stop_jack ();
    void position_jack (bool a_state);

    void off_sequences ();
    void all_notes_off ();

    void set_active (int a_sequence, bool a_active);
    void set_was_active (int a_sequence);
    bool is_active (int a_sequence);
    bool is_dirty_main (int a_sequence);
    bool is_dirty_edit (int a_sequence);
    bool is_dirty_perf (int a_sequence);
    bool is_dirty_names (int a_sequence);

    void new_sequence (int a_sequence);
    sequence * get_sequence (int a_sequence);
    void reset_sequences ();

    /**
     *  Plays all notes to the current tick.
     */

    void play (long a_tick);
    void set_orig_ticks (long a_tick);
    void set_bpm (int a_bpm);           /* more than just a setter  */
    int get_bpm ();                     /* get BPM from the buss    */

    /**
     * \setter m_looping
     */

    void set_looping (bool a_looping)
    {
        m_looping = a_looping;
    }

    void set_sequence_control_status (int a_status);
    void unset_sequence_control_status (int a_status);
    void sequence_playing_toggle (int a_sequence);
    void sequence_playing_on (int a_sequence);
    void sequence_playing_off (int a_sequence);
    void set_group_mute_state (int a_g_track, bool a_mute_state);
    bool get_group_mute_state (int a_g_track);
    void mute_all_tracks ();
    void output_func ();
    void input_func ();
    long get_max_trigger ();

    /**
     *  Calculates the offset into the screen sets.
     *
     *  Sets m_offset = a_offset * c_mainwnd_rows * c_mainwnd_cols;
     *
     * \param a_offset
     *      The desired offset.
     */

    void set_offset (int a_offset)
    {
        m_offset = a_offset * c_mainwnd_rows * c_mainwnd_cols;
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
     *
     *  Used in mainwid, options, optionsfile, userfile, and perform.
     */

    bool show_ui_sequence_key () const
    {
        return keys().show_ui_sequence_key();
    }
    void show_ui_sequence_key (bool flag)
    {
        keys().show_ui_sequence_key(flag);
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
        g_rc_settings.is_pattern_playing(true);
    }

    /**
     *  Encapsulates a series of calls used in mainwnd.
     */

    void stop_playing ()
    {
        stop_jack();
        stop();
        g_rc_settings.is_pattern_playing(false);
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

    int decrement_bpm ()
    {
        int result = get_bpm() - 1;
        set_bpm(result);
        return result;
    }

    /**
     *  Encapsulates some calls used in mainwnd.  Actually does a lot of
     *  work in those function calls.
     */

    int increment_bpm ()
    {
        int result = get_bpm() + 1;
        set_bpm(result);
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

    void sequence_key (int seq);                        // encapsulation
    void set_input_bus (int bus, bool input_active);    // used in options
    bool mainwnd_key_event (const keystroke & k);
    bool perfroll_key_event (const keystroke & k, int drop_sequence);

private:

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

    void inner_start (bool a_state);
    void inner_stop ();
    void set_all_key_events ();
    void set_all_key_groups ();
    void set_key_event (unsigned int keycode, long sequence_slot);
    void set_key_group (unsigned int keycode, long group_slot);
    int clamp_track (int track) const;

};

/**
 * Global functions defined in perform.cpp.
 */

extern void * output_thread_func (void * a_p);
extern void * input_thread_func (void * a_p);

}           // namespace seq64

#endif      // SEQ64_PERFORM_HPP

/*
 * perform.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

