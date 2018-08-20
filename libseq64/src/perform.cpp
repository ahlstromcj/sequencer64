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
 * \file          perform.cpp
 *
 *  This module defines the base class for the performance of MIDI patterns.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom and others
 * \date          2015-07-24
 * \updates       2018-08-19
 * \license       GNU GPLv2 or above
 *
 *  This class is probably the single most important class in Sequencer64, as
 *  it supports sequences, playback, JACK, and more.
 *
 * Requests for more MIDI control:
 *
 *  "On a rather selfish note, I'd love to be able to send these mods from my
 *  shiny new (AKAI APC MINI) control surface:
 *
 *      - Arm for Recording
 *      - Solo On (compare it to the Replace functionality)
 *      - Solo Off
 *
 *  "Solo might not even be necessary as we have Replace. It would make sense
 *  if it allowed a temporary solo which could then be released and the
 *  original state would return. I don't exactly know how this would work,
 *  however."
 *
 *  "A MIDI mod to toggle the recording of incoming MIDI data would be an
 *  amazing step toward not having to touch the mouse. I guess a mod for MIDI
 *  pass through would also be required?"
 *
 *  "And then, actually, Start/Stop via MIDI would be something very
 *  valuable."
 *
 *  "Waiting OSC avaibility, I use
 *  https://github.com/Excds/seq24-launchpad-mapper for my Launchpad Mini."
 *
 *  Summarizing these state-saving buffers:
 *
 *      -   m_armed_statuses[c_max_sequence].
 *          Used in perform::toggle_playing_tracks(), a feature copped from
 *          the Seq32 project. Flagged by m_armed_saved.
 *      -   m_seqs_active[c_max_sequence] (seq24).
 *          Indicates if a pattern has any data in it, i.e. it is not empty,
 *          whether it is muted or not.
 *      -   m_was_active_main[c_max_sequence] (seq24).
 *          Used in perform::is_dirty_main().
 *      -   m_was_active_edit[c_max_sequence] (seq24).
 *          Used in perform::is_dirty_edit().
 *      -   m_was_active_perf[c_max_sequence] (seq24).
 *          Used in perform::is_dirty_perf().
 *      -   m_was_active_names[c_max_sequence] (seq24).
 *          Used in perform::is_dirty_names().
 *      -   m_sequence_state[c_max_sequence] (seq24).
 *          Used in unsetting the snapshot status (c_status_snapshot).
 *          perform::save_playing_state() uses this to preserve the playing
 *          status.
 *      -   m_screenset_state[m_seqs_in_set].
 *          Holds the state of playing in the current screen-set, to determine
 *          which patterns follow the queued-replace (queued-solo) feature,
 *
 *  m_playscreen.  In seq24, this value (called m_playing_screen) is used for
 *
 *      -   select_group_mute(), to get the sequence/pattern offset for
 *          copying the pattern playing status into the mute-group array.
 *      -   select_mute_group(), very similar.  In sequencer64, this is
 *          called set_and_copy_mute_group() to avoid confusion.
 *          Also, tdeagan's code swaps m_screenset for this value.
 *      -   mute_group_tracks(), to implement the mute-group operation.
 *          Also, tdeagan's code swaps m_screenset for this value.
 *      -   sequence_playing_on()/_off().  If this value equals
 *          m_screenset and the sequence is within the playing screen,
 *          then its mute state is set to true/false (on/off).
 *      -   set_playing_screenset(), where this value is set to
 *          m_screenset.  This function is called when
 *          -   c_midi_control_play_ss is performed.
 *          -   The main window hot-key for screen-set is pressed.
 *
 *  m_screenset.  In seq24, this value (called m_screen_set) is used for
 *
 *      -   In set_screenset().  The value is clipped to 0 to 31.
 *      -   set_playing_screenset(), as noted above.
 *      -   sequence_playing_on()/_off(), as noted above.
 *
 *  set_playing_screenset().  This function is called when
 *
 *      -   c_midi_control_play_ss is performed.
 *      -   The main window hot-key for screen-set is pressed.
 *
 *      I think we may need to call this after calling set_screenset(seq),
 *      where a number is available.
 *
 *  set_screenset().  This function is called
 *
 *      -   In decrement_ and increment_screenset().
 *
 *      In mainwnd::timer_callback(), the mainwid screen-set is set to match
 *      the new value, which can be altered by the screen-set up and down
 *      hot-keys.
 *
 *  User jean-emmanuel added a new MIDI control for setting the screen-set
 *  directly by number.
 *
 * TODO: seq32's tick_to_jack_frame () etc. for tempo.
 *
 * MIDI CLOCK Support:
 *
 *    On output:
 *
 *        perform::m_usemidiclock starts at false;
 *        It is set to false in pause_playing();
 *        It is set to the midiclock parameter of inner_stop();
 *        If m_usemidiclock is true:
 *            It affects m_midiclocktick in output;
 *            The position in output cannot be repositioned;
 *            The tick location cannot be changed;
 *
 *    On input:
 *
 *    -   If MIDI Start is received, m_midiclockrunning and m_usemidiclock
 *        become true, and m_midiclocktick and m_midiclockpos become 0.
 *    -   If MIDI Continue is received, m_midiclockrunning is set to true and
 *        we start according to song-mode.
 *    -   If MIDI Stop is received, m_midiclockrunning is set to false,
 *        m_midiclockpos is set to the current tick (!), all_notes_off(), and
 *        inner_stop(true) [sets m_usemidiclock = true].
 *    -   If MIDI Clock is received, and m_midiclockrunning is true, then
 *        m_midiclocktick += 24 [SEQ64_MIDI_CLOCK_INCREMENT];
 *    -   If MIDI Song Position is received, then m_midiclockpos is set as per
 *        in data in this event.
 *    -   MIDI Active Sense and MIDI Reset are currently filtered by the JACK
 *        implementation.
 */

#include <sched.h>
#include <stdio.h>
#include <string.h>                     /* memset()                         */

#include "calculations.hpp"
#include "cmdlineopts.hpp"              /* seq64::parse_mute_groups()       */
#include "event.hpp"
#include "keystroke.hpp"
#include "midibus.hpp"
#include "perform.hpp"
#include "settings.hpp"                 /* seq64::rc()                      */

#if defined PLATFORM_WINDOWS
#include <windows.h>                    /* Muahhhahahahahah!                */
#include <mmsystem.h>                   /* Windows timeBeginPeriod()        */
#else
#include <time.h>                       /* struct timespec                  */
#endif

/**
 *  Indicates if the playing-screenset code is in force or not, for
 *  experimenting.  Without this patch, ignoring snapshots, it seems like
 *  mute-groups only work on screen-set 0, where as with the patch (again
 *  ignoring snapshots), they apply to the "in-view" (or "current", or
 *  "active") screen-set.
 *
 *  Temporarily disabled for some deeper research.
 */

#define SEQ64_USE_TDEAGAN_CODE_XXX

/**
 *  The amount to increment the MIDI clock pulses.  MIDI clock normal comes
 *  out at 24 PPQN, so I am not sure why this is 8.
 */

#define SEQ64_MIDI_CLOCK_INCREMENT      8

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *  Instantiate the dummy midi_control object, which is used in lieu
 *  of a null pointer.  We're taking code that basically works already, in the
 *  sense that it never seems to access a null pointer.  So we're not even
 *  risking data transfers between this dummy object and the ones we really
 *  want to use.
 *
 *  However, it would be nice to be able to detect any errors that occur.
 *  How?
 */

midi_control perform::sm_mc_dummy;

/**
 *  This construction initializes a vast number of member variables, some
 *  of them public (but we're working on that)!
 *
 *  Also note that we have a little issue with the fact that various sequences
 *  (patterns) can potentially have different beats/measure and beat-width
 *  values.
 *
 *  Currently, when reading the MIDI file, the beats/minute value is obtained
 *  from the MIDI file, if present, and this value is passed to
 *  perform::set_beats_per_minute(), which forwards it to the master MIDI buss
 *  and JACK assistant objects.  This Tempo setting comes from both the
 *  Tempo meta event in track 0, and from the Seq24's c_bpm SeqSpec section!
 *  This setting is now also made for the two Time Signature values.
 *
 *  But note that Sequencer64 now scales the c_bpm value so that two extra
 *  digits of precision can be saved with the MIDI file.  We went throughout
 *  the code, changing BPM from an integer to a double.
 *
 * \param mygui
 *      Provides access to the GUI assistant that holds many things,
 *      including the containers of keys and the "events" they
 *      provide.  This is a base-class reference; for a real class, see
 *      the gui_assistant_gtk2 class in the seq_gtkmm2 GUI-specific library.
 *      Note that we access the m_gui_support member using the gui()
 *      accessor function.
 *
 * \param ppqn
 *      The default, choosable, or actual PPQN value.
 */

perform::perform (gui_assistant & mygui, int ppqn)
 :
    m_song_start_mode           (false),    // set later during options read
    m_start_from_perfedit       (false),
    m_reposition                (false),
    m_excell_FF_RW              (1.0f),
    m_FF_RW_button_type         (FF_RW_NONE),
    m_mute_group                (),         // boolean array, size 32 * 32
    m_mute_group_rc             (),         // boolean array, size 32 * 32
    m_armed_saved               (false),
    m_armed_statuses            (),         // boolean array, size 1024
    m_seqs_in_set               (usr().seqs_in_set()),      // c_seqs_in_set
    m_max_groups                (c_max_sequence / m_seqs_in_set),
    m_tracks_mute_state         (m_seqs_in_set, false),     // sets track state
    m_mode_group                (true),     // why true????
    m_mode_group_learn          (false),
    m_mute_group_selected       (SEQ64_NO_MUTE_GROUP_SELECTED),
    m_midi_mute_group_present   (false),
    m_seqs                      (),         // pointer array [c_max_sequence]
    m_seqs_active               (),         // boolean array [c_max_sequence]
    m_was_active_main           (),         // boolean array [c_max_sequence]
    m_was_active_edit           (),         // boolean array [c_max_sequence]
    m_was_active_perf           (),         // boolean array [c_max_sequence]
    m_was_active_names          (),         // boolean array [c_max_sequence]
    m_sequence_state            (),         // boolean array [c_max_sequence]
    m_screenset_state           (m_seqs_in_set, false),    // boolean vector
    m_queued_replace_slot       (SEQ64_NO_QUEUED_SOLO),
    m_transpose                 (0),
    m_out_thread                (),
    m_in_thread                 (),
    m_out_thread_launched       (false),
    m_in_thread_launched        (false),
    m_is_running                (false),
    m_is_pattern_playing        (false),
    m_inputing                  (true),
    m_outputing                 (true),
    m_looping                   (false),
#ifdef SEQ64_SONG_RECORDING
    m_song_recording            (false),
    m_song_record_snap          (false),
    m_resume_note_ons           (false),
    m_current_tick              (0.0),
#endif
    m_playback_mode             (false),
    m_ppqn                      (choose_ppqn(ppqn)),    /* may change later */
    m_bpm                       (SEQ64_DEFAULT_BPM),    /* now a double     */
    m_beats_per_bar             (SEQ64_DEFAULT_BEATS_PER_MEASURE),
    m_beat_width                (SEQ64_DEFAULT_BEAT_WIDTH),
    m_clocks_per_metronome      (24),
    m_32nds_per_quarter         (8),
    m_us_per_quarter_note       (tempo_us_from_bpm(SEQ64_DEFAULT_BPM)),
    m_master_bus                (nullptr),
    m_filter_by_channel         (false),                /* "rc" option      */
    m_master_clocks             (),                     /* vector<clock_e>  */
    m_master_inputs             (),                     /* vector<bool>     */
    m_one_measure               (m_ppqn * 4),           /* may change later */
    m_left_tick                 (0),
    m_right_tick                (m_one_measure * 4),    /* m_ppqn * 16      */
    m_starting_tick             (0),
    m_tick                      (0),
    m_jack_tick                 (0),
    m_usemidiclock              (false),
    m_midiclockrunning          (false),
    m_midiclocktick             (0),
    m_midiclockpos              (-1),
    m_dont_reset_ticks          (false),
    m_screenset_notepad         (),         // string array [c_max_sets]
    m_midi_cc_toggle            (),         // midi_control []
    m_midi_cc_on                (),         // midi_control []
    m_midi_cc_off               (),         // midi_control []
    m_control_status            (0),
    m_screenset                 (0),        // vice m_playscreen
    m_screenset_offset          (0),
    m_playscreen                (0),        // vice m_screenset
    m_playscreen_offset         (0),
    m_max_sets                  (usr().max_sets()),     // c_max_sets
    m_sequence_count            (0),
    m_sequence_max              (c_max_sequence),
    m_sequence_high             (-1),
#ifdef SEQ64_EDIT_SEQUENCE_HIGHLIGHT
    m_edit_sequence             (-1),
#endif
    m_is_modified               (false),
#ifdef SEQ64_SONG_BOX_SELECT
    m_selected_seqs             (),                     // Selection, std::set
#endif
    m_condition_var             (),
#ifdef SEQ64_JACK_SUPPORT
    m_jack_asst
    (
        *this,                              // we are the parent
        SEQ64_DEFAULT_BPM,                  // may get updated later
        m_ppqn,                             // probably updated later
        SEQ64_DEFAULT_BEATS_PER_MEASURE,    // may get updated later
        SEQ64_DEFAULT_BEAT_WIDTH            // may get updated later
    ),
#endif
    m_have_undo                 (false),
    m_undo_vect                 (),          // vector of int
    m_have_redo                 (false),
    m_redo_vect                 (),          // vector of int
    m_notify                    (),          // vector of callback pointers
    m_gui_support               (mygui)
{
    keys().group_max(m_max_groups);
    for (int i = 0; i < m_sequence_max; ++i)
    {
        m_seqs[i] = nullptr;
        m_seqs_active[i] =                      /* seq24 0.9.3 addition     */
            m_sequence_state[i] =               /* ca 2016-11-27            */
            m_was_active_main[i] = m_was_active_edit[i] =
            m_was_active_perf[i] = m_was_active_names[i] = false;
    }
    for (int i = 0; i < c_max_sequence; ++i)    /* not c_gmute_tracks now   */
    {
        m_mute_group[i] = m_mute_group_rc[i] = m_armed_statuses[i] = false;
    }
    for (int i = 0; i < m_max_sets; ++i)
        m_screenset_notepad[i].clear();

    midi_control zero;                          /* all members false or 0   */
    for (int i = 0; i < c_midi_controls_extended; ++i)
        m_midi_cc_toggle[i] = m_midi_cc_on[i] = m_midi_cc_off[i] = zero;
}

/**
 *  The destructor sets some running flags to false, signals this condition,
 *  then joins the input and output threads if the were launched. Finally, any
 *  active or inactive (but allocated) patterns/sequences are deleted, and
 *  their pointers nullified.
 *
 *  Note that we could use m_sequence_high to replace m_sequence_max in the
 *  for-loop, but who cares, we are exiting!
 */

perform::~perform ()
{
    m_inputing = m_outputing = m_is_running = false;
    m_condition_var.signal();                       /* signal end of play   */
    if (m_out_thread_launched)
        pthread_join(m_out_thread, NULL);

    if (m_in_thread_launched)
        pthread_join(m_in_thread, NULL);

    for (int seq = 0; seq < m_sequence_high; ++seq) /* m_sequence_max       */
    {
        if (not_nullptr(m_seqs[seq]))
        {
            delete m_seqs[seq];
            m_seqs[seq] = nullptr;                  /* not really necessary */
        }
    }

    if (not_nullptr(m_master_bus))
        delete(m_master_bus);
}

/**
 * \setter ppqn
 *      Also sets other related members.
 *
 *      Might also have to run though ALL patterns and user-interface objects
 *      to fix them.
 */

void
perform::set_ppqn (int p)
{
    m_ppqn = p;
    m_master_bus->set_ppqn(p);
#ifdef SEQ64_JACK_SUPPORT
    m_jack_asst.set_ppqn(p);
#endif
    m_one_measure = p * 4;                  // simplistic!
    m_right_tick = m_one_measure * 4;       // ditto
    // set_dirty_settings();
}

/**
 *  Creates the mastermidibus.  We need to delay creation until launch time,
 *  so that settings can be obtained before determining just how to set up the
 *  application.
 *
 *  Once the master buss is created, we then copy the clocks and input setting
 *  that were read from the "rc" file, via the mastermidibus ::
 *  set_port_statuses() function, to use in determining whether to initialize
 *  and connect the input ports at start-up.  Seq24 wouldn't connect
 *  unconditionally, and Sequencer64 shouldn't, either.
 *
 *  However, the devices actually on the system at start time might be
 *  different from what was saved in the "rc" file after the last run of
 *  Sequencer64.
 *
 *  For output, both apps have always connected to all ports automatically.
 *  But we want to support disabling some output ports, both in the "rc"
 *  file and via the operating system indicating that it cannot open an
 *  output port.  So how do we get the port-settings from the OS?  Probably
 *  at initialization time.  See the mastermidibus constructor for PortMidi.
 *
 * \return
 *      Returns true if the creation succeeded, or if the buss already exists.
 */

bool
perform::create_master_bus ()
{
    bool result = not_nullptr(m_master_bus);
    if (! result)
    {
#ifdef USE_DEFAULT_ARGS
        m_master_bus = new(std::nothrow) mastermidibus();   /* default args */
#else
        /*
         * TEST TEST TEST!!!!
         */

        m_master_bus = new(std::nothrow) mastermidibus(m_ppqn, m_bpm);
#endif
        result = not_nullptr(m_master_bus);
        if (result)
        {
            m_master_bus->filter_by_channel(m_filter_by_channel);
            m_master_bus->set_port_statuses(m_master_clocks, m_master_inputs);
        }
    }
    return result;
}

/**
 *  Calls the MIDI buss and JACK initialization functions and the input/output
 *  thread-launching functions.  This function is called in main().  We
 *  collected all the calls here as a simplification, and renamed it because
 *  it is more than just initialization.  This function must be called after
 *  the perform constructor and after the configuration file and command-line
 *  configuration overrides.  The original implementation, where the master
 *  buss was an object, was too inflexible to handle a JACK implementation.
 *
 * \param ppqn
 *      Provides the PPQN value, which is either the default value (192) or is
 *      read from the "user" configuration file.
 *
 * \todo
 *      We probably need a bpm parameter for consistency at some point.
 */

void
perform::launch (int ppqn)
{
    if (create_master_bus())                /* also calls set_port_statuses()   */
    {

#ifdef SEQ64_JACK_SUPPORT
        init_jack_transport();
#endif

        if (ppqn == SEQ64_USE_FILE_PPQN)
            ppqn = SEQ64_DEFAULT_PPQN;

        m_master_bus->init(ppqn, m_bpm);    /* calls api_init() per API     */

        /*
         * We may need to copy the actually input buss settings back to here,
         * as they can change.  LATER.  They get saved properly anyway,
         * because the optionsfile object gets the information from the
         * mastermidibus more directly.  Actually indirectly. :-)
         */

        if (activate())
        {
            launch_input_thread();
            launch_output_thread();
        }
    }
}

/**
 *  The rough opposite of launch(); it doesn't stop the threads.  A minor
 *  simplification for the main() routine, hides the JACK support macro.
 *  We might need to add code to stop any ongoing outputing.
 *
 *  Also gets the settings made/changed while the application was running from
 *  the mastermidibase class to here.  This action is the converse of calling
 *  the set_port_statuses() function defined in the mastermidibase module.
 */

void
perform::finish ()
{
    (void) deinit_jack_transport();
    if (not_nullptr(m_master_bus))
        m_master_bus->get_port_statuses(m_master_clocks, m_master_inputs);
}

#ifdef SEQ64_SONG_BOX_SELECT

/**
 *  A prosaic implementation of calling a function on the set of stored
 *  sequences.  Used for redrawing selected sequences in the graphical user
 *  interface.
 *
 * \param func
 *      The (bound) function to call for each sequence in the set.  It has two
 *      parameters, the sequence number and a pulse value.  The sequence
 *      number parameter is a place-holder and it obtained here.  The pulse
 *      parameter is bound by the caller to create func().
 *
 * \return
 *      Returns true if at least one set item was found to operate on.
 */

#if __cplusplus >= 201103L                  /* C++11                        */

bool
perform::selection_operation (SeqOperation func)
{
    bool result = false;
    Selection::iterator s;
    for (s = m_selected_seqs.begin(); s != m_selected_seqs.end(); ++s)
        func(*s);

    return result;
}

#endif

/**
 *  Selects the desired trigger for this sequence.  If this is the first
 *  selection, then the sequence is inserted into the box container.
 *
 * \param dropseq
 *      The sequence to operate on.
 *
 * \param droptick
 *      Indicates the trigger to be selected.
 */

void
perform::box_insert (int dropseq, midipulse droptick)
{
    sequence * s = get_sequence(dropseq);
    if (not_nullptr(s))
    {
        bool can_add_seq = s->selected_trigger_count() == 0;
        if (s->select_trigger(droptick))            /* able to select?      */
        {
            if (can_add_seq)
                m_selected_seqs.insert(dropseq);
        }
    }
}

/**
 *  Unselects only the desired trigger for this sequence.  If there are no
 *  more selected triggers for this sequence, then the sequence is erased from
 *  the box container.
 *
 * \param dropseq
 *      The sequence to operate on.
 *
 * \param droptick
 *      Indicates the trigger to be unselected.
 */

void
perform::box_delete (int dropseq, midipulse droptick)
{
    sequence * s = get_sequence(dropseq);
    if (not_nullptr(s))
    {
        s->unselect_trigger(droptick);
        if (s->trigger_count() == 0)
            m_selected_seqs.erase(dropseq);
    }
}

/**
 *  If the sequence is not in the "box set", add it.  Otherwise, we are
 *  "reselecting" the sequence, so remove it from the list of selections.
 *  Used in the performance window's on-button-press event.
 *
 * \param dropseq
 *      The number of the sequence where "the mouse was clicked", in the
 *      performance roll.
 */

void
perform::box_toggle_sequence (int dropseq, midipulse droptick)
{
    Selection::const_iterator s = m_selected_seqs.find(dropseq);
    if (s != m_selected_seqs.end())
        box_delete(*s, droptick);
    else
        box_insert(dropseq, droptick);
}

/**
 *  If the current sequence is not part of the selection, then we need to
 *  unselect all sequences.
 */

void
perform::box_unselect_sequences (int dropseq)
{
    if (m_selected_seqs.find(dropseq) == m_selected_seqs.end())
    {
         unselect_all_triggers();
         m_selected_seqs.clear();
    }
}

/**
 *  Moves the box-selected set of triggers to the given tick.
 *
 * \param tick
 *      The destination location for the trigger.
 */

void
perform::box_move_triggers (midipulse tick)
{
    Selection::const_iterator s;
    for (s = m_selected_seqs.begin(); s != m_selected_seqs.end(); ++s)
    {
        sequence * selseq = get_sequence(*s);
        if (not_nullptr(selseq))                            /* not needed */
            selseq->move_triggers(tick, true);
    }
}

/**
 *  Offset the box-selected set of triggers by the given tick amount.
 *
 * \param tick
 *      The destination location for the trigger.
 */

void
perform::box_offset_triggers (midipulse offset)
{
    Selection::const_iterator s;
    for (s = m_selected_seqs.begin(); s != m_selected_seqs.end(); ++s)
    {
        sequence * selseq = get_sequence(*s);
        if (not_nullptr(selseq))                            /* not needed */
            selseq->offset_triggers(offset);
    }
}

#endif  // SEQ64_SONG_BOX_SELECT

/**
 *  Encapsulates getting the trigger limits without putting the burden on the
 *  caller.  The more code moved out of the user-interface, the better.
 *
 * \param seqnum
 *      The number of the sequence of interest.
 *
 * \param droptick
 *      The tick location, basically where the mouse was clicked.
 *
 * \param [out] tick0
 *      The output location for the start of the trigger.
 *
 * \param [out] tick1
 *      The output location for the end of the trigger.
 *
 * \return
 *      Returns true if the sequence is valid and we can select the trigger.
 */

bool
perform::selected_trigger
(
    int seqnum, midipulse droptick,
    midipulse & tick0, midipulse & tick1
)
{
    bool result = false;
    sequence * s = get_sequence(seqnum);
    if (not_nullptr(s))
    {
        result = s->select_trigger(droptick);
        tick0 = s->selected_trigger_start();
        tick1 = s->selected_trigger_end();
    }
    return result;
}

/**
 *  Clears all of the patterns/sequences.  The mainwnd module calls this
 *  function.  Note that perform now handles the "is modified" flag on behalf
 *  of all external objects, to centralize and simplify the dirtying of a MIDI
 *  tune.
 *
 *  Anything else to clear?  What about all the other sequence flags?  We can
 *  beef up delete_sequence() for them, at some point.
 *
 *  Added stazed code from 1.0.5 to abort clearing if any of the sequences are
 *  in editing.
 *
 * \return
 *      Returns true if the clear-all operation could be performed.  If false,
 *      then at least one active sequence was in editing mode.
 */

bool
perform::clear_all ()
{
    bool result = true;
    for (int s = 0; s < m_sequence_high; ++s)           /* m_sequence_max   */
    {
        if (is_active(s) && m_seqs[s]->get_editing())   /* stazed check     */
        {
            result = false;
            break;
        }
    }
    if (result)
    {
        reset_sequences();
        for (int s = 0; s < m_sequence_high; ++s)       /* m_sequence_max   */
            if (is_active(s))
                delete_sequence(s);             /* can set "is modified"    */

        std::string e;                          /* an empty string          */
        for (int sset = 0; sset < m_max_sets; ++sset)
            set_screenset_notepad(sset, e);

        set_have_undo(false);
        m_undo_vect.clear();                    /* ca 2016-08-16            */
        set_have_redo(false);
        m_redo_vect.clear();                    /* ca 2016-08-16            */
        is_modified(false);                     /* new, we start afresh     */
    }
    return result;
}

/**
 *  Provides common code to keep the track value valid.  Fixed the bug we
 *  found, where we checked for track > m_seqs_in_set, instead of using
 *  the >= operator.  Supports variset mode.
 *
 * \param track
 *      The track value to be checked and rectified as necessary.
 *
 * \return
 *      Returns the track parameter, clamped between 0 and m_seqs_in_set-1,
 *      inclusive.
 */

int
perform::clamp_track (int track) const
{
    if (track < 0)
    {
        track = 0;
        errprint("clamped track to 0");
    }
    else if (track >= m_seqs_in_set)
    {
        track = m_seqs_in_set - 1;
        errprintf("clamped track number to %d\n", track);
    }
    return track;
}

/**
 *  Provides common code to keep the group value valid even in variset mode.
 *  Fixed the bug we found, where we checked for track > m_seqs_in_set,
 *  instead of using the >= operator.  We now compare against the new
 *  c_max_groups value (32).  But, if the set-size is larger, then we have
 *  fewer mute groups available.
 *
 * \param group
 *      The group value to be checked and rectified as necessary.
 *
 * \return
 *      Returns the group parameter, clamped between 0 and c_max_sets-1,
 *      inclusive.  We use c_max_groups rather than m_max_sets because the new
 *      varisets feature reduces the number of sets (in general); this check
 *      is basically a sanity check.  Additionally, if varisets mode is in
 *      force (i.e. there are more than 32 sequences in a set), the group
 *      number is clamped to the maximum number of sets under that constraint.
 */

int
perform::clamp_group (int group) const
{
    if (group < 0)
    {
        group = 0;
        errprint("clamped group to 0");
    }
    else if (group >= c_max_groups)
    {
        group = c_max_groups - 1;
        errprintf("clamped group number to %d\n", group);
    }
    if (group >= m_max_groups)
        group = m_max_groups - 1;

    return group;
}

/**
 * \getter m_mute_group[]
 *
 * \return
 *      Returns true if there are any unmute statuses in the mute-group
 *      array.  If they're all zero, we don't need to save them.
 */

bool
perform::any_group_unmutes () const
{
    bool result = false;
    const bool * mp = &m_mute_group[0];
    for (int i = 0; i < m_sequence_max; ++i, ++mp)      /* c_gmute_tracks */
    {
        if (*mp)
        {
            result = true;
            break;
        }
    }
    return result;
}

/**
 *  This function is a way to dump the mute-group settings in a way
 *  independent of the code in the optionsfile module, for debugging.
 */

void
perform::print_group_unmutes () const
{
    const bool * mp = &m_mute_group[0];
    int set_number = 0;
    for (int i = 0; i < m_sequence_max; ++i, ++mp)      /* c_gmute_tracks   */
    {
        if ((i % m_seqs_in_set) == 0)
        {
            printf("\n[%2d]", set_number);
            ++set_number;
        }
        if ((i % SEQ64_SET_KEYS_COLUMNS) == 0)          /* 8                */
            printf(" ");

        printf("%d", *mp ? 1 : 0);
    }
    printf("\n");
}

/**
 *  If we're in group-learn mode, then this function gets the playing statuses
 *  of all of the sequences in the current play-screen, and copies them into
 *  the desired mute-group.  Then, no matter what, it makes the desired
 *  mute-group the selected mute-group.  Compare to set_and_copy_mute_group().
 *
 *  One thing to note is that, once saved, then, if used, it is applied
 *  to the current screen-set, even if it is not the screen-set whose
 *  playing status were saved.
 *
 *  Also note that this function now supports variset mode, via
 *  screen_offset().
 *
 * \param mutegroup
 *      The number of the desired mute group, clamped to be between 0 and
 *      m_seqs_in_set-1.  Obviously, it is the set whose state is to be
 *      stored, if in group-learn mode.
 */

void
perform::select_group_mute (int mutegroup)
{
    mutegroup = clamp_group(mutegroup);
    if (m_mode_group_learn)
    {
        int groupbase = screenset_offset(mutegroup);    /* 1st seq in group */
        for (int s = 0; s < m_seqs_in_set; ++s)         /* variset issue    */
        {
            int source = m_playscreen_offset + s;       /* m_screenset? No. */
            int dest = groupbase + s;
            if (is_active(source))
            {
                bool status = m_seqs[source]->get_playing();
                m_mute_group[dest] = status;
#ifdef PLATFORM_DEBUG_TMI
                printf
                (
                    "1: setting m_mute_group[%d] to seq #%d status %s\n",
                    dest, source, status ? "true" : "false"
                );
#endif
            }
        }
    }
    m_mute_group_selected = mutegroup;
#ifdef PLATFORM_DEBUG_TMI
    printf("mute-group %d selection\n", mutegroup);
#endif
}

#ifdef SEQ64_SONG_BOX_SELECT

/**
 *  Selects the set of triggers bounded by a low and high sequence number and
 *  a low and high tick selection.  If there is an inactive sequence in this
 *  range, it is simply ignored.
 *
 * \param seq_low
 *      The low sequence number in the pattern range.
 *
 * \param seq_high
 *      The high sequence number in the pattern range.
 *
 * \param tick_start
 *      The earliest tick in the time range.
 *
 * \param tick_finish
 *      The last tick in the time range.
 */

void
perform::select_triggers_in_range
(
    int seq_low, int seq_high,
    midipulse tick_start, midipulse tick_finish
)
{
    for (int seq = seq_low; seq <= seq_high; ++seq)
    {
        sequence * s = get_sequence(seq);
        if (not_nullptr(s))
        {
            for (long tick = tick_start; tick <= tick_finish; ++tick)
                s->select_trigger(tick);
        }
    }
}

#endif      // SEQ64_SONG_BOX_SELECT

/**
 *  Selectes a trigger for the given sequence.
 *
 * \param dropseq
 *      The sequence number that was in play for the location of the mouse in
 *      the (for example) perfedit roll.
 *
 * \param droptick
 *      The location of the mouse horizonally in the perfroll.
 *
 * \return
 *      Returns true if a trigger was there to select, and was selected.
 */

bool
perform::select_trigger (int dropseq, midipulse droptick)
{
    sequence * s = get_sequence(dropseq);
    bool result = not_nullptr(s);
    if (result)
        result = s->select_trigger(droptick);

    return result;
}

/**
 *  Calls sequence::unselect_triggers() for all active sequences.
 */

void
perform::unselect_all_triggers ()
{
    for (int seq = 0; seq < m_sequence_high; ++seq)     // c_max_sequence
    {
        sequence * s = get_sequence(seq);
        if (not_nullptr(s))
            s->unselect_triggers();
    }
}

/**
 *  Combines select_group_mute() and set_group_mute_state() so that the
 *  optionsfile class can load the groups without altering the
 *  m_mute_group_selected item; doing that is a bit misleading. In addition, a
 *  copy is saved in a second mute-group array in case we want to avoid
 *  "corrupting" the "rc" mute-groups with mute-groups from the MIDI file.
 *
 *  It also uses the constant c_seqs_in_set instead of the variable
 *  m_seqs_in_set so that the loading always handles 32 x 32 = 1024 settings.
 *
 *  Replaces "set_group_mute_state(g, gm[s] != 0)" and mute_group_offset(),
 *  which depend on the configured "seqs-in-set" value.  Here, we are always
 *  tied to the legacy 32 x 32 sizes, to load and save to the configuration.
 *
 * \param gmute
 *      The mute-group to load.  If out-of-range (0 to c_max_groups), no
 *      setting is made.  There's no need to call clamp_group(); it is useful
 *      in playing, not in loading the configuration.
 *
 * \param gm
 *      The array of c_seqs_in_set values to be used for the load.  The pointer
 *      is not checked.
 *
 * \return
 *      Returns true if the group was valid.
 */

bool
perform::load_mute_group (int gmute, int gm [c_seqs_in_set])
{
    bool result = gmute >= 0 && gmute < c_max_groups;
    if (result)
    {
        int groupoffset = gmute * c_seqs_in_set;
        for (int s = 0; s < c_seqs_in_set; ++s)
        {
            int track = groupoffset + s;
            m_mute_group[track] = m_mute_group_rc[track] = gm[s] != 0;
        }
    }
    return result;
}

/**
 *  The converse of load_mute_group().  However, it has a wrinkle in what it
 *  saves.  First, if there are no unmuted patterns in any of the mute-groups,
 *  then the original mute-groups read from the "rc" file are saved back.
 *  Second, this saving is also done if the e_mute_group_preserve flag is
 *  set.  If the e_mute_group_stomp flag is set, then the active mute-group
 *  statuses are written to the "rc" file.
 *
 * \param gmute
 *      The mute-group to save.  If out-of-range (0 to c_max_groups), no
 *      saving is done.
 *
 * \param gm
 *      The array of c_seqs_in_set values to be used for the save.  The
 *      pointer is not checked.
 *
 * \return
 *      Returns true if the group was valid.
 */

bool
perform::save_mute_group (int gmute, int gm [c_seqs_in_set]) const
{
    bool result = (gmute >= 0 && gmute < c_max_groups);
    if (result)
    {
        mute_group_handling_t mgh = rc().mute_group_saving();
        int groupoffset = gmute * c_seqs_in_set;
        bool savemaingroup = (mgh == e_mute_group_stomp) && any_group_unmutes();
        if (savemaingroup)
        {
            for (int s = 0; s < c_seqs_in_set; ++s)
                gm[s] = m_mute_group[groupoffset + s] ? 1 : 0 ;
        }
        else
        {
            for (int s = 0; s < c_seqs_in_set; ++s)
                gm[s] = m_mute_group_rc[groupoffset + s] ? 1 : 0 ;
        }
    }
    return result;
}

/**
 *  This function sets the mute state of an element in the m_mute_group
 *  array.  The index value is the track number offset by the number of
 *  the selected mute group (which is equivalent to a set number) times
 *  the number of sequences in a set.  This function is used in midifile
 *  and optionsfile when parsing the file to get the initial mute-groups.
 *
 * \bug
 *      We were not using the group track value if it was zero, but that is a
 *      legitimate value.
 *
 * \param gtrack
 *      The number of the track to be muted/unmuted.
 *
 * \param muted
 *      This boolean indicates the state to which the track should be set.
 */

void
perform::set_group_mute_state (int gtrack, bool muted)
{
    int grouptrack = mute_group_offset(gtrack);
    if (grouptrack >= 0)
        m_mute_group[grouptrack] = muted;
}

/**
 *  The opposite of set_group_mute_state(), it gets the value of the
 *  desired track.  Uses the mute_group_offset() function.  This function
 *  is used in midifile and optionsfile when writing the file to get the
 *  initial mute-groups.
 *
 * \bug
 *      We were not using the group track value if it was zero, but that is a
 *      legitimate value.
 *
 * \param gtrack
 *      The number of the track for which the state is to be obtained.  Like
 *      set_group_mute_state(), this value is offset by adding
 *      m_mute_group_selected * m_seqs_in_set.
 *
 * \return
 *      Returns the desired m_mute_group[] value.
 */

bool
perform::get_group_mute_state (int gtrack)
{
    bool result = false;
    int grouptrack = mute_group_offset(gtrack);
    if (grouptrack >= 0)
        result = m_mute_group[grouptrack];

    return result;
}

/**
 *  A helper function to calculate the index into the mute-group array,
 *  based on the desired track.  Remember that the mute-group array,
 *  m_mute_group[c_max_sequence], determines which tracks are muted/unmuted.
 *  Also remember that m_mute_group_selected now determines which
 *  "seqs-in-set" set is selected.
 *
 *  Old definition:
 *
 *      return clamp_track(track) + m_mute_group_selected * m_seqs_in_set;
 *
 * \param trackoffset
 *      The number of the desired track.  This is basically one of the hot-key
 *      related values.  Traditionally, this value ranges from 0 to 31
 *      (c_seqs_in_set-1), but now the variset mode is supported.
 *
 * \return
 *      Returns a track value from 0 to 1023 if the group is valid for the
 *      current seqs-in-set count and a mute-group has been selected.
 *      Otherwise, a SEQ64_NO_MUTE_GROUP_SELECTED (-1) is returned.  The
 *      caller must check this value before using it.
 */

int
perform::mute_group_offset (int trackoffset)
{
    int result = SEQ64_NO_MUTE_GROUP_SELECTED;
    if (m_mute_group_selected != SEQ64_NO_MUTE_GROUP_SELECTED)
    {
        if (trackoffset >= 0 && trackoffset < m_seqs_in_set)
        {
            int offset = m_mute_group_selected * m_seqs_in_set;
            result = trackoffset + offset;
            if (result >= c_max_sequence)
                result = SEQ64_NO_MUTE_GROUP_SELECTED;
        }
    }
    return result;
}

/**
 *  Sets the group-mute mode, then the group-learn mode, then notifies all of
 *  the notification subscribers.  This function is called via a MIDI control
 *  c_midi_control_mod_glearn and via the group-learn keystroke.
 */

void
perform::set_mode_group_learn ()
{
    set_mode_group_mute();                              /* m_mode_group=true */
    m_mode_group_learn = true;
    for (size_t x = 0; x < m_notify.size(); ++x)
        m_notify[x]->on_grouplearnchange(true);
}

/**
 *  Notifies all of the notification subscribers that group-learn is being
 *  turned off.  Then unsets the group-learn mode flag.  This function is
 *  called via a MIDI control c_midi_control_mod_glearn, via the group-learn
 *  keystroke, and in mainwnd::on_key_press_event(), to end the group-learn
 *  mode.
 *
 *  Shouldn't this function also call this one, to perfectly complement
 *  set_mode_group_learn: unset_mode_group_mute().  Too tricky.
 */

void
perform::unset_mode_group_learn ()
{
    for (size_t x = 0; x < m_notify.size(); ++x)
        m_notify[x]->on_grouplearnchange(false);

    m_mode_group_learn = false;
}

/**
 *  When in group-learn mode, for active sequences, the mute-group settings
 *  are set based on the playing status of each sequence.  Then the mute-group
 *  is stored in m_tracks_mute_state[], which holds states for only the number
 *  of sequences in a set.
 *
 *  Compare to select_group_mute(); its main difference is that it will
 *  at least copy the states even if not in group-learn mode.  And, if in
 *  group-learn mode, it will grab the playing states of the sequences before
 *  copying them.
 *
 *  This function is used only once, in select_and_mute_group().  It used to
 *  be called just select_mute_group(), but that's too easy to confuse with
 *  select_group_mute().
 *
 * \change tdeagan 2015-12-22 via git pull:
 *      git pull https://github.com/TDeagan/sequencer64.git mute_groups
 *      m_screenset replaces m_playscreen_offset.
 *
 * \param mutegroup
 *      Provides the mute-group to select.
 */

void
perform::set_and_copy_mute_group (int mutegroup)
{
    mutegroup = clamp_group(mutegroup);
    int groupbase = screenset_offset(mutegroup);

#ifdef SEQ64_USE_TDEAGAN_CODE
    int setbase = screenset_offset(m_screenset);
#else
    int setbase = m_playscreen_offset;          /* includes m_seqs_in_set   */
#endif

    m_mute_group_selected = mutegroup;          /* must set it before loop  */
    for (int s = 0; s < m_seqs_in_set; ++s)     /* variset support          */
    {
        int source = setbase + s;
        if (m_mode_group_learn && is_active(source))
        {
            bool status = m_seqs[source]->get_playing();
            int dest = groupbase + s;
            m_mute_group[dest] = status;        /* learn the pattern state  */
        }
        int offset = mute_group_offset(s);
        if (offset >= 0)
        {
            int mmg = m_mute_group[offset];
            m_tracks_mute_state[s] = mmg;
        }
        else
            break;
    }
}

/**
 *  If m_mode_group is true, then this function operates.  It loops through
 *  every screen-set.  In each screen-set, it acts on each active sequence.
 *  If the active sequence is in the current "in-view" screen-set (m_screenset
 *  as opposed to m_playscreen, and its m_track_mute_state[] is true,
 *  then the sequence is turned on, otherwise it is turned off.  The result is
 *  that the in-view screen-set is activated as per the mute states, while all
 *  other screen-sets are muted.
 *
 * \change tdeagan 2015-12-22 via git pull.
 *      Replaced m_playscreen with m_screenset.
 *
 *  We are disabling Tim's fix for a bit in order to make sure we're doing
 *  what Seq24 does with the playing set key. (Home).
 *
 *  It seems to us that the for (g) clause should have g range from 0 to
 *  m_max_sets, not m_seqs_in_set.  Done.
 */

void
perform::mute_group_tracks ()
{
    if (m_mode_group)
    {
        for (int g = 0; g < m_max_sets; ++g)
        {
            int seqoffset = screenset_offset(g);
            for (int s = 0; s < m_seqs_in_set; ++s)
            {
                int seqnum = seqoffset + s;
                if (is_active(seqnum))          /* semi-redundant check */
                {
#ifdef SEQ64_USE_TDEAGAN_CODE
                    bool on = (g == m_screenset) && m_tracks_mute_state[s];
#else
                    bool on = (g == m_playscreen) && m_tracks_mute_state[s];
#endif
                    sequence_playing_change(seqnum, on);
                }
            }
        }
    }
}

/**
 *  Select a mute group and then mutes the track in the group.  Called in
 *  perform and in mainwnd.
 *
 * \param group
 *      Provides the group number for the group to be muted.
 */

void
perform::select_and_mute_group (int group)
{
    set_and_copy_mute_group(group);
    mute_group_tracks();
}

/**
 *  Clears all the group-mute items, whether they came from the "rc" file
 *  or from the most recently-loaded Sequencer64 MIDI file.
 *
 * \sideeffect
 *      If true is returned, the modify flag is set, so that the user has the
 *      option to save a MIDI file that contained mute-groups that are no
 *      longer wanted.
 *
 * \return
 *      Returns true if any of the statuses changed from true to false.
 */

bool
perform::clear_mute_groups ()
{
    bool result = false;
    for (int seq = 0; seq < c_max_sequence; ++seq)
    {
        if (m_mute_group[seq])
        {
            result = true;
            modify();
        }
        m_mute_group[seq] = false;
    }
    return result;
}

/**
 *  Mutes/unmutes all tracks in the current set of active patterns/sequences.
 *  Covers tracks from 0 to m_sequence_max.
 *
 *  We have to also set the sequence's playing status, in opposition to the
 *  mute status, in order to see the sequence status change on the
 *  user-interface.   HMMMMMM.
 *
 * \param flag
 *      If true (the default), the song-mute of the sequence is turned on.
 *      Otherwise, it is turned off.
 */

void
perform::mute_all_tracks (bool flag)
{
    for (int i = 0; i < m_sequence_high; ++i)       /* m_sequence_max       */
    {
        if (is_active(i))
        {
            m_seqs[i]->set_song_mute(flag);
            m_seqs[i]->set_playing(! flag);         /* to show mute status  */
        }
    }
}

/**
 *  Toggles the mutes status of all tracks in the current set of active
 *  patterns/sequences.  Covers tracks from 0 to m_sequence_max.
 *
 *  Note that toggle_playing() now has two default parameters used by the new
 *  song-recording feature, which are currently not used here.
 */

void
perform::toggle_all_tracks ()
{
    for (int i = 0; i < m_sequence_high; ++i)
    {
        if (is_active(i))
        {
            m_seqs[i]->toggle_song_mute();
            m_seqs[i]->toggle_playing();        /* needed to show mute status */
        }
    }
}

/**
 *  Toggles the mutes status of all playing (currently unmuted) tracks in the
 *  current set of active patterns/sequences on all screen-sets.  Covers
 *  tracks from 0 to m_sequence_max.  The statuses are preserved for
 *  restoration.
 *
 *  Note that this function operates only in Live mode; it is too confusing to
 *  use in Song mode.  Also note that toggle_playing() now has two default
 *  parameters used by the new song-recording feature, which are currently not
 *  used here.
 */

void
perform::toggle_playing_tracks ()
{
    if (song_start_mode())
        return;

    if (are_any_armed())
    {
        if (m_armed_saved)
        {
            m_armed_saved = false;
            for (int i = 0; i < m_sequence_high; ++i)
            {
                if (m_armed_statuses[i])
                {
                    m_seqs[i]->toggle_song_mute();
                    m_seqs[i]->toggle_playing();        /* to show mute status  */
                }
            }
        }
        else
        {
            bool armed_status = false;
            for (int i = 0; i < m_sequence_high; ++i)   /* m_sequence_max       */
            {
                if (is_active(i))
                {
                    armed_status = m_seqs[i]->get_playing();
                    m_armed_statuses[i] = armed_status;
                    if (armed_status)
                    {
                        m_armed_saved = true;           /* one was armed        */
                        m_seqs[i]->toggle_song_mute();  /* toggle the arming    */
                        m_seqs[i]->toggle_playing();    /* to show mute status  */
                    }
                }
            }
        }
    }
    else
    {
        /*
         * If no sequences are armed, then turn them all on, as a convenience
         * to the user.
         */

        mute_all_tracks(false);
    }
}

/**
 *  Indicates if any sequences are armed (playing).
 *
 * \return
 *      Returns true if even one sequence is armed.
 */

bool
perform::are_any_armed ()
{
    bool result = false;
    for (int i = 0; i < m_sequence_high; ++i)   /* m_sequence_max       */
    {
        if (is_active(i))
        {
            result = m_seqs[i]->get_playing();
            if (result)
                break;                          /* one armed is enough  */
        }
    }
    return result;
}

/**
 *  Provides for various settings of the song-mute status of all sequences in
 *  the song. The sequence::set_song_mute() and toggle_song_mute() functions
 *  do all the work, including mp-dirtying the sequence.
 *
 *  We've modified this function to call mute_all_tracks() and
 *  toggle_all_tracks() in order to consolidate the code and (cough cough) fix
 *  a bug in this functionality from the mainwnd menu.
 *
 * \question
 *      Do we want to replace the call to toggle_all_tracks() with a call to
 *      toggle_playing_tracks()?
 *
 * \param op
 *      Provides the "flag" that indicates if this function is to set mute on,
 *      off, or to toggle the mute status.
 */

void
perform::set_song_mute (mute_op_t op)
{
    switch (op)
    {
    case MUTE_ON:
        mute_all_tracks(true);
        break;

    case MUTE_OFF:
        mute_all_tracks(false);
        break;

    case MUTE_TOGGLE:
        toggle_all_tracks();
        break;
    }
}

/**
 *  Mutes/unmutes all tracks in the desired screen-set.
 *
 * \param ss
 *      The screen-set to be operated upon.
 *
 * \param flag
 *      If true (the default), the song-mute of the sequence is turned on.
 *      Otherwise, it is turned off.
 */

void
perform::mute_screenset (int ss, bool flag)
{
    int seq = screenset_offset(ss);
    for (int i = 0; i < m_seqs_in_set; ++i, ++seq)
    {
        if (is_active(seq))
        {
            m_seqs[seq]->set_song_mute(flag);
            m_seqs[seq]->set_playing(! flag); /* needed to show mute status!  */
        }
    }
}

/**
 *  Set the left marker at the given tick.  We let the caller determine if
 *  this setting is a modification.  If the left tick is later than the right
 *  tick, the right tick is move to one measure past the left tick.
 *
 * \todo
 *      The perform::m_one_measure member is currently hardwired to m_ppqn*4.
 *
 * \param tick
 *      The tick (MIDI pulse) at which to place the left tick.  If the left
 *      tick is greater than or equal to the right tick, then the right ticked
 *      is moved forward by one "measure's length" (m_ppqn * 4) past the left
 *      tick.
 *
 * \param setstart
 *      If true (the default, and long-standing implicit setting), then the
 *      starting tick is also set to the left tick.
 */

void
perform::set_left_tick (midipulse tick, bool setstart)
{
    m_left_tick = tick;
    if (setstart)
        set_start_tick(tick);

#ifdef SEQ64_JACK_SUPPORT
    if (is_jack_master())                       /* don't use in slave mode  */
        position_jack(true, tick);
    else if (! is_jack_running())
        set_tick(tick);
#else
    set_tick(tick);
#endif

    m_reposition = false;
    if (m_left_tick >= m_right_tick)
        m_right_tick = m_left_tick + m_one_measure;
}

/**
 *  Set the right marker at the given tick.  This setting is made only if the
 *  tick parameter is at or beyond the first measure.  We let the caller
 *  determine is this setting is a modification.
 *
 * \param tick
 *      The tick (MIDI pulse) at which to place the right tick.  If less than
 *      or equal to the left tick setting, then the left tick is backed up by
 *      one "measure's worth" (m_ppqn * 4) worth of ticks from the new right
 *      tick.
 *
 * \param setstart
 *      If true (the default, and long-standing implicit setting), then the
 *      starting tick is also set to the left tick, if that got changed.
 */

void
perform::set_right_tick (midipulse tick, bool setstart)
{
    if (tick >= m_one_measure)
    {
        m_right_tick = tick;
        if (m_right_tick <= m_left_tick)
        {
            m_left_tick = m_right_tick - m_one_measure;
            if (setstart)
                set_start_tick(m_left_tick);

            /*
             * Do this no matter the value of setstart, to match stazed's
             * implementation.
             */

            if (is_jack_master())
                position_jack(true, m_left_tick);
            else
                set_tick(m_left_tick);

            m_reposition = false;
        }
    }
}

/**
 *  A private helper function for add_sequence() and new_sequence().  It is
 *  common code and using it prevents inconsistences.  It assumes values have
 *  already been checked.  It does not set the "is modified" flag, since
 *  adding a sequence by loading a MIDI file should not set it.  Compare
 *  new_sequence(), used by mainwid and seqmenu, with add_sequence(), used by
 *  midifile.
 *
 * \param seq
 *      The pointer to the pattern/sequence to add.
 *
 * \param seqnum
 *      The sequence number of the pattern to be added.  Not validated, to
 *      save some time.
 *
 * \return
 *      Returns true if a sequence was removed, or the sequence was
 *      successfully added.  In other words, if a real change in sequence
 *      pointers occurred.  It is up to the caller to decide if the change
 *      warrants setting the "is modified" flag.
 */

bool
perform::install_sequence (sequence * seq, int seqnum)
{
    bool result = false;
    if (not_nullptr(m_seqs[seqnum]))
    {
        errprintf("m_seqs[%d] not null, deleting old sequence\n", seqnum);
        delete m_seqs[seqnum];
        m_seqs[seqnum] = nullptr;
        if (m_sequence_count > 0)
        {
            --m_sequence_count;
        }
        else
        {
            errprint("sequence counter already 0");
        }
        result = true;                  /* a modification occurred  */
    }
    m_seqs[seqnum] = seq;
    if (not_nullptr(seq))
    {
        set_active(seqnum, true);
        seq->set_parent(this);
        ++m_sequence_count;
        if (seqnum >= m_sequence_high)
            m_sequence_high = seqnum + 1;

        result = true;                  /* a modification occurred  */
    }
    return result;
}

/**
 *  Adds a pattern/sequence pointer to the list of patterns.  No check is made
 *  for a null pointer, but the install_sequence() call will make sure such a
 *  pointer is officially logged.
 *
 *  This function checks for the preferred sequence number.  This is the
 *  number that was specified by the Sequence Number meta-event for the
 *  current track.  If the preferred sequence number is in the valid range (0
 *  to m_sequence_max) and it is not active, add it and activate it.
 *  Otherwise, iterate through all patterns from prefnum to m_sequence_max and
 *  add and activate the first one that is not active, and then finish.
 *
 *  Finally, note that this function is used only by midifile, when reading in
 *  a MIDI song.  Therefore, the "is modified" flag is <i> not </i> set by
 *  this function; loading a sequence from a file is not a modification that
 *  should lead to a prompt for saving the file later.
 *
 * \todo
 *      Shouldn't we wrap around the sequence list if we can't find an empty
 *      sequence slot after prefnum?
 *
 * \todo
 *      This function needs some deeper analysis against the original, in my
 *      opinion.
 *
 * \warning
 *      The logic of the if-statement in this function was such that \a
 *      prefnum could be out-of-bounds in the else-clause.  We reworked
 *      the logic to be airtight.  This bug was caught by gcc 4.8.3 on CentOS,
 *      but not on gcc 4.9.3 on Debian Sid!
 *
 * \param seq
 *      The pointer to the pattern/sequence to add.
 *
 * \param prefnum
 *      The preferred sequence number of the pattern, as explained above.  If
 *      this value is out-of-range, then it is basically ignored.
 */

void
perform::add_sequence (sequence * seq, int prefnum)
{
    if (! is_seq_valid(prefnum))
        prefnum = 0;

    if (is_active(prefnum))                 /* look for next inactive one   */
    {
        for (int i = prefnum; i < m_sequence_max; ++i)
        {
            if (! is_active(i))
            {
                (void) install_sequence(seq, i);
                break;
            }
        }
    }
    else
        (void) install_sequence(seq, prefnum);
}

/**
 *  Creates a new pattern/sequence for the given slot, and sets the new
 *  pattern's master MIDI bus address.  Then it activates the pattern [this is
 *  done in the install_sequence() function].  It doesn't deal with thrown
 *  exceptions.
 *
 *  This function is called by the seqmenu and mainwid objects to create a new
 *  sequence.  We now pass this sequence to install_sequence() to better
 *  handle potential memory leakage, and to make sure the sequence gets
 *  counted.  Also, adding a new sequence from the user-interface is a
 *  significant modification, so the "is modified" flag gets set.
 *
 * \change ca 2016-05-15
 *      If enabled, wire in the MIDI buss override.
 *
 * \param seq
 *      The prospective sequence number of the new sequence.
 *
 * \return
 *      Returns true if the sequence number is valid.  Do not use the
 *      sequence if false is returned, it will be null.
 */

bool
perform::new_sequence (int seq)
{
    bool result = is_seq_valid(seq);
    if (result)
    {
        sequence * seqptr = new sequence();     /* std::nothrow */
        if (not_nullptr(seqptr))
        {
            if (install_sequence(seqptr, seq))
            {
                if (is_mseq_valid(seq))
                {
                    /*
                     * Add the buss override, if specifed.  We can't set it
                     * until after we have assigned the master MIDI buss,
                     * otherwise we get a segfault.
                     */

                    char buss_override = usr().midi_buss_override();
                    m_seqs[seq]->set_master_midi_bus(m_master_bus);
                    modify();
                    if (buss_override != SEQ64_BAD_BUSS)
                        m_seqs[seq]->set_midi_bus(buss_override);
                }
            }
        }
    }
    return result;
}

/**
 *  Deletes a pattern/sequence by number.  We now also solidify the deletion
 *  by setting the pointer to null after deletion, so it will blow up if
 *  accidentally accessed.  The final act is to raise the "is modified" flag,
 *  since deleting an existing sequence is always a significant modification.
 *
 *  Now, this function obviously sets the "active" flag for the sequence to
 *  false.  But there are a few other flags that are not modified; shouldn't
 *  we also falsify them here?
 *
 * \param seq
 *      The sequence number of the sequence to be deleted.  It is validated.
 */

void
perform::delete_sequence (int seq)
{
    if (is_mseq_valid(seq))                         /* check for null, etc. */
    {
        set_active(seq, false);
        if (! m_seqs[seq]->get_editing())           /* clarify this!        */
        {
            m_seqs[seq]->set_playing(false);
            delete m_seqs[seq];
            m_seqs[seq] = nullptr;
            modify();                               /* it is dirty, man     */
        }
    }
}

/**
 *  Sets or unsets the active state of the given pattern/sequence number.
 *  If setting it active, the sequence::number() setter is called. It won't
 *  modify the sequence's internal copy of the sequence number if it has
 *  already been set.
 *
 * \param seq
 *      Provides the prospective sequence number.
 *
 * \param active
 *      True if the sequence is to be set to the active state.
 */

void
perform::set_active (int seq, bool active)
{
    if (is_mseq_valid(seq))
    {
        if (m_seqs_active[seq] && ! active)
            set_was_active(seq);

        m_seqs_active[seq] = active;
        if (active)
        {
            m_seqs[seq]->number(seq);
            if (m_seqs[seq]->name().empty())
                m_seqs[seq]->set_name();
        }
    }
}

/**
 *  Sets was-active flags:  main, edit, perf, and names.
 *  Why do we need this routine?
 *
 * \param seq
 *      The pattern number.  It is checked for validity.
 */

void
perform::set_was_active (int seq)
{
    if (is_seq_valid(seq))
    {
        m_was_active_main[seq] = m_was_active_edit[seq] =
            m_was_active_perf[seq] = m_was_active_names[seq] = true;
    }
}

/**
 *  Tests to see if the screen-set is active.  By "active", we mean that
 *  the screen-set has at least one active pattern.
 *
 * \param screenset
 *      The number of the screen-set to check, re 0.
 *
 * \return
 *      Returns true if the screen-set has an active pattern.
 */

bool
perform::screenset_is_active (int screenset)
{
    bool result = false;
    int seqsinset = usr().seqs_in_set();
    int seqnum = screenset * seqsinset;
    for (int seq = 0; seq < seqsinset; ++seq, ++seqnum)
    {
        if (is_active(seqnum))
        {
            result = true;
            break;
        }
    }
    return result;
}

/**
 *  Checks the pattern/sequence for main-dirtiness.  See the
 *  sequence::is_dirty_main() function.
 *
 * \param seq
 *      The pattern number.  It is checked for validity.
 *
 * \return
 *      Returns the was-active-main flag value, before setting it to
 *      false.  Returns false if the pattern was invalid.
 */

bool
perform::is_dirty_main (int seq)
{
    bool was_active = false;
    if (sequence_count() > 0)
    {
        if (is_active(seq))
        {
            was_active = m_seqs[seq]->is_dirty_main();
        }
        else
        {
            was_active = m_was_active_main[seq];
            m_was_active_main[seq] = false;
        }
    }
    return was_active;
}

/**
 *  Checks the pattern/sequence for edit-dirtiness.
 *
 * \param seq
 *      The pattern number.  It is checked for validity.
 *
 * \return
 *      Returns the was-active-edit flag value, before setting it to
 *      false. Returns false if the pattern was invalid.
 */

bool
perform::is_dirty_edit (int seq)
{
    bool was_active = false;
    if (sequence_count() > 0)
    {
        if (is_active(seq))
        {
            was_active = m_seqs[seq]->is_dirty_edit();
        }
        else
        {
            was_active = m_was_active_edit[seq];
            m_was_active_edit[seq] = false;
        }
    }
    return was_active;
}

/**
 *  Checks the pattern/sequence for perf-dirtiness.
 *
 * \param seq
 *      The pattern number.  It is checked for validity.
 *
 * \return
 *      Returns the was-active-perf flag value, before setting it to
 *      false.  Returns false if the pattern/sequence number was invalid.
 */

bool
perform::is_dirty_perf (int seq)
{
    bool was_active = false;
    if (sequence_count() > 0)
    {
        if (is_active(seq))
        {
            was_active = m_seqs[seq]->is_dirty_perf();
        }
        else
        {
            was_active = m_was_active_perf[seq];
            m_was_active_perf[seq] = false;
        }
    }
    return was_active;
}

/**
 *  Checks the pattern/sequence for names-dirtiness.
 *
 * \param seq
 *      The pattern number.  It is checked for validity.
 *
 * \return
 *      Returns the was-active-names flag value, before setting it to
 *      false.  Returns false if the pattern/sequence number was invalid.
 */

bool
perform::is_dirty_names (int seq)
{
    bool was_active = false;
    if (sequence_count() > 0)
    {
        if (is_active(seq))
        {
            was_active = m_seqs[seq]->is_dirty_names();
        }
        else
        {
            was_active = m_was_active_names[seq];
            m_was_active_names[seq] = false;
        }
    }
    return was_active;
}

/**
 *  Sets the value of the BPM into the master MIDI buss, after making
 *  sure it is squelched to be between 20 and 500.  Replaces
 *  perform::set_bpm() from seq24.
 *
 *  The value is set only if neither JACK nor this performance object are
 *  running.
 *
 *  It's not clear that we need to set the "is modified" flag just because we
 *  changed the beats per minute.  This setting does get saved to the MIDI
 *  file, with the c_bpmtag.
 *
 * \param bpm
 *      Provides the beats/minute value to be set.  It is clamped, if
 *      necessary, between the values SEQ64_MINIMUM_BPM to SEQ64_MAXIMUM_BPM.
 *      They provide a wide range of speeds, well beyond what normal music
 *      needs.
 */

void
perform::set_beats_per_minute (midibpm bpm)
{
    if (bpm < SEQ64_MINIMUM_BPM)
        bpm = SEQ64_MINIMUM_BPM;
    else if (bpm > SEQ64_MAXIMUM_BPM)
        bpm = SEQ64_MAXIMUM_BPM;

    if (bpm != m_bpm)
    {

#ifdef SEQ64_JACK_SUPPORT

        /*
         * This logic matches the original seq24, but is it really correct?
         * Well, we fixed it so that, whether JACK transport is in force or
         * not, we can modify the BPM and have it stick.  No test for JACK
         * Master or for JACK and normal running status needed.
         */

        m_jack_asst.set_beats_per_minute(bpm);

#endif

        m_master_bus->set_beats_per_minute(bpm);
        m_us_per_quarter_note = tempo_us_from_bpm(bpm);
        m_bpm = bpm;

        /*
         * Do we need to adjust the BPM of all of the sequences, including the
         * potential tempo track???  It is "merely" the putative main tempo of
         * the MIDI tune.  Actually, this value can now be recorded as a Set
         * Tempo event by user action in the main window (and, later, by
         * incoming MIDI Set Tempo events).
         */
    }
}

/**
 *  Encapsulates some calls used in mainwnd.  Actually does a lot of
 *  work in those function calls.
 *
 * \return
 *      Returns the resultant BPM, as a convenience.
 */

midibpm
perform::decrement_beats_per_minute ()
{
    midibpm result = get_beats_per_minute() - usr().bpm_step_increment();
    set_beats_per_minute(result);
    return result;
}

/**
 *  Encapsulates some calls used in mainwnd.  Actually does a lot of
 *  work in those function calls.
 *
 * \return
 *      Returns the resultant BPM, as a convenience.
 */

midibpm
perform::increment_beats_per_minute ()
{
    midibpm result = get_beats_per_minute() + usr().bpm_step_increment();
    set_beats_per_minute(result);
    return result;
}

/**
 *  Provides additional coarse control over the BPM value, which comes into
 *  force when the Page-Up/Page-Down keys are pressed.
 *
 *  Encapsulates some calls used in mainwnd.  Actually does a lot of
 *  work in those function calls.
 *
 * \return
 *      Returns the resultant BPM, as a convenience.
 */

midibpm
perform::page_decrement_beats_per_minute ()
{
    midibpm result = get_beats_per_minute() - usr().bpm_page_increment();
    set_beats_per_minute(result);
    return result;
}

/**
 *  Provides additional coarse control over the BPM value, which comes into
 *  force when the Page-Up/Page-Down keys are pressed.
 *
 *  Encapsulates some calls used in mainwnd.  Actually does a lot of
 *  work in those function calls.
 *
 * \return
 *      Returns the resultant BPM, as a convenience.
 */

midibpm
perform::page_increment_beats_per_minute ()
{
    midibpm result = get_beats_per_minute() + usr().bpm_page_increment();
    set_beats_per_minute(result);
    return result;
}

/**
 *  Used by callers to insert tempo events.  Note that, if the current tick
 *  position is past the end of pattern 0's length, then the length of the
 *  tempo track pattern (0 by default) is increased in order to hold the tempo
 *  event.
 *
 *  Note that we allow the user to change the tempo track from the default of
 *  0 to any pattern from 0 to 1023.  This is done in the File / Options /
 *  MIDI Clock tab, and is saved to the "rc" file.
 *
 * \return
 *      Returns true if the tempo-track sequence exists.
 */

bool
perform::log_current_tempo ()
{
	sequence * seq = get_sequence(get_tempo_track_number());
	bool result = not_nullptr(seq);
	if (result)
	{
		midipulse tick = get_tick();
		midibpm bpm = get_beats_per_minute();
		seq64::event e = create_tempo_event(tick, bpm);   /* event.cpp */
		if (seq->add_event(e))
        {
            seq->link_tempos();
            seq->set_dirty();
            modify();
            if (tick > seq->get_length())
                seq->set_length(tick);
        }
	}
	return result;
}

/**
 *  Encapsulates some calls used in mainwnd.  The value set here will
 *  represent the "active" screen-set in multi-window mode.
 *
 * \param amount
 *      Indicates the amount the screenset is to be decremented.  The default
 *      value is 1.
 *
 * \return
 *      Returns the decremented screen-set value.
 */

int
perform::decrement_screenset (int amount)
{
    int result = screenset() - amount;
    return set_screenset(result);
}

/**
 *  Encapsulates some calls used in mainwnd.  The value set here will
 *  represent the "active" screen-set in multi-window mode.
 *
 * \param amount
 *      Indicates the amount the screenset is to be incremented.  The default
 *      value is 1.
 *
 * \return
 *      Returns the incremented screen-set value.
 */

int
perform::increment_screenset (int amount)
{
    int result = screenset() + amount;
    return set_screenset(result);
}

/**
 *  Provides common code to check for the bounds of a sequence number.
 *  Also see the function is_mseq_valid(), which also checks the pointer
 *  stored in the m_seq[] array.
 *
 *  We considered checking the \a seq param against sequence_count(), but
 *  this function is called while creating sequences that add to that count,
 *  so we continue checking against the "container" size.  Also, it is
 *  possible to have holes in the array representing inactive sequences,
 *  so that sequencer_count() would be too limiting.
 *
 * \param seq
 *      The sequencer number, in interval [0, m_sequence_max).
 *
 * \return
 *      Returns true if the sequence number is valid.
 */

bool
perform::is_seq_valid (int seq) const
{
    if (seq >= 0 && seq < m_sequence_max)   /* do not use m_sequence_high   */
    {
        return true;
    }
    else
    {
        if (SEQ64_IS_NULL_SEQUENCE(seq))
        {
            errprint("is_seq_valid(): unassigned sequence number");
        }
        else if (! SEQ64_IS_DISABLED_SEQUENCE(seq))
        {
            errprintf("is_seq_valid(): seq = %d\n", seq);
        }
        return false;
    }
}

/**
 *  Indicates that the desired sequence is active, unmuted, and has
 *  a non-zero trigger count.
 *
 * \param seq
 *      The index of the desired sequence.
 *
 * \return
 *      Returns true if the sequence has the three properties noted above.
 */

bool
perform::is_exportable (int seq) const
{
    const sequence * s = get_sequence(seq);
    bool ok = not_nullptr(s);
    if (ok)
        ok = ! s->get_song_mute() && s->trigger_count() > 0;

    return ok;
}

/**
 *  Validates the sequence number, which is important since they're currently
 *  used as array indices.  It also evaluates the m_seq[seq] pointer value.
 *
 * \note
 *      Since we can have holes in the sequence array, where there are
 *      inactive sequences, we check if the sequence is even active before
 *      emitting a message about a null pointer for the sequence.  We only
 *      want to see messages that indicate actual problems.
 *
 * \param seq
 *      Provides the sequence number to be checked.  It is checked for
 *      validity.  We cannot compare the sequence number versus the
 *      sequence_count(), because the current implementation can have inactive
 *      holes (with null pointers) interspersed with active pointers.
 *
 * \return
 *      Returns true if the sequence number is valid as per is_seq_valid(),
 *      and the sequence pointer is not null.
 */

bool
perform::is_mseq_valid (int seq) const
{
    bool result = is_seq_valid(seq);
    if (result)
    {
        result = not_nullptr(m_seqs[seq]);
        if (! result && m_seqs_active[seq])
        {
            errprintf("is_mseq_valid(): active m_seqs[%d] is null\n", seq);
        }
    }
    return result;
}

/**
 *  A check that is even strong than is_mseq_valid(), this function checks for
 *  the pattern number being valid, the pattern being inactive, and not in
 *  editing mode.  A convenience function for use in user-interfaces.
 *
 * \param seq
 *      Provides the sequence number to be checked.
 *
 * \return
 *      Returns true if the sequence number is valid as per is_seq_valid(),
 *      the sequence is active, and not in editing mode.
 */

bool
perform::is_mseq_available (int seq) const
{
    return is_seq_valid(seq) && ! is_active(seq) && ! is_sequence_in_edit(seq);
}

/**
 *  Check if the pattern/sequence, given by number, has an edit in
 *  progress.
 *
 * \param seq
 *      Provides the sequence number to be checked.
 *
 * \return
 *      Returns truen if the sequence's get_editing() call returns true.
 *      Otherwise, false is returned, which can also indicate an illegal
 *      sequence number.
 */

bool
perform::is_sequence_in_edit (int seq) const
{
    if (is_mseq_valid(seq))
        return m_seqs[seq]->get_editing();
    else
        return false;
}

/**
 *  Retrieves a reference to a value from m_midi_cc_toggle[].  Recall that the
 *  midi_control object specifies if a control is active, inversely-active,
 *  what status byte initiates it, what data byte initiates it, and the
 *  min/max values.  Note that the status byte determines what category of
 *  event it is (e.g. note on/off versus a continuous controller), and the
 *  data byte indicates the note value or the type of continous controller.
 *
 * \param ctl
 *      Provides the index to pass to valid_midi_control_seq() to obtain a
 *      control value (such as c_midi_control_bpm_up) to use to retrieve the
 *      desired midi_control object.
 *
 * \return
 *      Returns the "toggle" value if the control value is valid, or a
 *      reference to sm_mc_dummy otherwise.
 */

midi_control &
perform::midi_control_toggle (int ctl)
{
    return valid_midi_control_seq(ctl) ? m_midi_cc_toggle[ctl] : sm_mc_dummy ;
}

/**
 *  Retrieves a reference to a value from m_midi_cc_on[].
 *
 * \param ctl
 *      Provides the index to pass to valid_midi_control_seq() to obtain a
 *      control value (such as c_midi_control_bpm_up) to use to retrieve the
 *      desired midi_control object.
 *
 * \return
 *      Returns the "on" value if the control value is valid, and a reference
 *      to sm_mc_dummy otherwise.
 */

midi_control &
perform::midi_control_on (int ctl)
{
    return valid_midi_control_seq(ctl) ? m_midi_cc_on[ctl] : sm_mc_dummy ;
}

/**
 *  Retrieves a reference to a value from m_midi_cc_off[].
 *
 * \param ctl
 *      Provides a control value (such as c_midi_control_bpm_up) to use to
 *      retrieve the desired midi_control object.
 *
 * \return
 *      Returns the "off" value if the control value is valid, and a reference
 *      to sm_mc_dummy otherwise.
 */

midi_control &
perform::midi_control_off (int ctl)
{
    return valid_midi_control_seq(ctl) ? m_midi_cc_off[ctl] : sm_mc_dummy ;
}

/**
 *  Copies the given string into m_screenset_notepad[].
 *
 * \param screenset
 *      The ID number of the screen-set, an index into the
 *      m_screenset_notepad[] array.
 *
 * \param notepad
 *      Provides the string date to copy into the notepad.  Not sure why a
 *      pointer is used, instead of nice "const std::string &" parameter.  And
 *      this pointer isn't checked.  Fixed.
 *
 * \param is_load_modification
 *      If true (the default is false), we do not want to set the modify flag,
 *      otherwise the user is prompted to save even if no changes have
 *      occurred.
 */

void
perform::set_screenset_notepad
(
    int screenset, const std::string & notepad, bool is_load_modification
)
{
    if (is_screenset_valid(screenset))
    {
        if (notepad != m_screenset_notepad[screenset])
        {
            m_screenset_notepad[screenset] = notepad;
            if (! is_load_modification)
                modify();
        }
    }
}

/**
 *  Retrieves the given string from m_screenset_notepad[].
 *
 * \param screenset
 *      The ID number of the screen-set, an index into the
 *      m_screenset_notepad[] array.  This value is validated.
 *
 * \return
 *      Returns a reference to the desired string, or to an empty string
 *      if the screen-set number is invalid.
 */

const std::string &
perform::get_screenset_notepad (int screenset) const
{
    static std::string s_empty;
    if (is_screenset_valid(screenset))
        return m_screenset_notepad[screenset];
    else
        return s_empty;
}

/**
 *  Sets the m_screenset value (the index or ID of the current screen-set).
 *  It's not clear that we need to set the "is modified" flag just because we
 *  changed the screen-set, so we don't.
 *
 *  This function is called when incrementing and decrementing the screenset.
 *  Its counterpart, set_playing_screenset(), is called when the hot-key or the
 *  MIDI control for setting the screenset is called.
 *
 *  As a new feature, we would like to queue-mute the previous screenset,
 *  and queue-unmute the newly-selected screenset.  Still working on getting
 *  it right.  Aborted.
 *
 * \param ss
 *      The index of the desired new screen-set.  It is forced to range from
 *      0 to m_max_sets - 1.  The clamping seems weird, but hews to seq24.
 *      What it does is let the user wrap around the screen-sets in the user
 *      interface.  The value set here will represent the "active" screen-set
 *      in multi-window mode.
 *
 * \return
 *      Returns the actual final value of the screen-set that was set, i.e. the
 *      m_screenset member value.
 */

int
perform::set_screenset (int ss)
{
    if (ss < 0)
        ss = m_max_sets - 1;
    else if (ss >= m_max_sets)
        ss = 0;

    if ((ss != m_screenset) && is_screenset_valid(ss))
    {
        m_screenset = ss;
        m_screenset_offset = screenset_offset(ss);
        unset_queued_replace();                 /* clear this new feature   */
    }
    return m_screenset;
}

/**
 *  Sets the screen-set that is active, based on the value of m_screenset.
 *  This function is called when one of the snapshot keys is pressed.
 *
 *  For each value up to m_seqs_in_set (32), the index of the current sequence
 *  in the current screen-set (m_playscreen) is obtained.  If the sequence
 *  is active and the sequence actually exists, it is processed; null
 *  sequences are no longer flagged as an error, they are just ignored.
 *
 *  Modifies m_playscreen, m_playscreen_offset, stores the current
 *  playing-status of each sequence in m_tracks_mute_state[], and then calls
 *  mute_group_tracks(), turns on unmuted tracks in the current screen-set.
 *
 *  Basically, this function retrieves and saves the playing status of the
 *  sequences in the current play-screen, sets the play-screen to the current
 *  screen-set, and then mutes the previous play-screen.  It is called via the
 *  c_midi_control_play_ss value or via the set-playing-screen-set keystroke.
 */

void
perform::set_playing_screenset ()
{
    for (int s = 0; s < m_seqs_in_set; ++s)
    {
        int source = m_playscreen_offset + s;
        if (is_active(source))
            m_tracks_mute_state[s] = m_seqs[source]->get_playing();
    }
    m_playscreen = m_screenset;
    m_playscreen_offset = screenset_offset(m_playscreen);
    mute_group_tracks();
}

/**
 *  Starts the playing of all the patterns/sequences.  This function just runs
 *  down the list of sequences and has them dump their events.  It skips
 *  sequences that have no playable MIDI events.
 *
 *  Note how often the "sp" (sequence) pointer was used.  It was worth
 *  offloading all these calls to a new sequence function.  Hence the new
 *  sequence::play_queue() function.
 *
 *  Finally, we stop the looping at m_sequence_high rather than
 *  m_sequence_max, to save a little time.
 *
 * \param tick
 *      Provides the tick at which to start playing.  This value is also
 *      copied to m_tick.
 */

void
perform::play (midipulse tick)
{
    set_tick(tick);
    for (int seq = 0; seq < m_sequence_high; ++seq)
    {
        sequence * s = get_sequence(seq);
        if (not_nullptr(s))
#ifdef SEQ64_SONG_RECORDING
            s->play_queue(tick, m_playback_mode, m_resume_note_ons);
#else
            s->play_queue(tick, m_playback_mode);
#endif
    }
    if (not_nullptr(m_master_bus))
        m_master_bus->flush();                      /* flush MIDI buss  */
}

/**
 *  For every pattern/sequence that is active, sets the "original tick"
 *  value for the pattern.  This is really the "last tick" value, so we
 *  renamed sequence::set_orig_tick() to sequence::set_last_tick().
 *
 * \param tick
 *      Provides the last-tick value to be set for each sequence that is
 *      active.
 */

void
perform::set_orig_ticks (midipulse tick)
{
    for (int s = 0; s < m_sequence_high; ++s)       /* m_sequence_max   */
    {
        if (is_active(s))
            m_seqs[s]->set_last_tick(tick);         /* set_orig_tick()  */
    }
}

/**
 *  Clears the patterns/sequence for the given sequence, if it is active.
 *
 * \param seq
 *      Provides the desired sequence.  The is_active() function validates
 *      this value.
 */

void
perform::clear_sequence_triggers (int seq)
{
    sequence * s = get_sequence(seq);
    if (not_nullptr(s))
        s->clear_triggers();
}

/**
 *  If the left tick is less than the right tick, then, for each sequence
 *  that is active, its triggers are moved by the difference between the
 *  right and left in the specified direction.
 *
 * \param direction
 *      Specifies the desired direction; false = left, true = right.
 */

void
perform::move_triggers (bool direction)
{
    if (m_left_tick < m_right_tick)
    {
        midipulse distance = m_right_tick - m_left_tick;
        for (int i = 0; i < m_sequence_high; ++i)       /* m_sequence_max   */
        {
            if (is_active(i))
                m_seqs[i]->move_triggers(m_left_tick, distance, direction);
        }
    }
}

/**
 *  For every active sequence, call that sequence's push_trigger_undo()
 *  function.  Too bad we cannot yet keep track of all the undoes for the sake
 *  of properly handling the "is modified" flag.
 *
 *  This function now has a new parameter.  Not added to this function is the
 *  seemingly redundant undo-push the seq32 code does; is this actually a
 *  seq42 thing?
 *
 *  Also, there is still an issue with our undo-handling for a single track.
 *  See pop_trigger_undo().
 *
 * \param track
 *      A new parameter (found in the stazed seq32 code) that allows this
 *      function to operate on a single track.  A parameter value of
 *      SEQ64_ALL_TRACKS (-1, the default) implements the original behavior.
 */

void
perform::push_trigger_undo (int track)
{
    m_undo_vect.push_back(track);                       /* stazed   */
    if (track == SEQ64_ALL_TRACKS)
    {
        for (int i = 0; i < m_sequence_high; ++i)       /* m_sequence_max   */
        {
            if (is_active(i))
                m_seqs[i]->push_trigger_undo();
        }
    }
    else
    {
        if (is_active(track))
            m_seqs[track]->push_trigger_undo();
    }
    set_have_undo(true);                                /* stazed   */
}

/**
 *  For every active sequence, call that sequence's pop_trigger_undo()
 *  function.
 *
 * \todo
 *      Look at seq32/src/perform.cpp and the perform ::
 *      push_trigger_undo(track) function, which has a track parameter that
 *      has a -1 values the supports all tracks.  It requires two new vectors
 *      (one for undo, one for redo), two new flags (likewise).  We've put
 *      this code in place, no longer macroed out, now permanent.
 */

void
perform::pop_trigger_undo ()
{
    if (! m_undo_vect.empty())
    {
        int track = m_undo_vect.back();
        m_undo_vect.pop_back();
        m_redo_vect.push_back(track);
        if (track == SEQ64_ALL_TRACKS)
        {
            for (int s = 0; s < m_sequence_high; ++s)   /* m_sequence_max   */
            {
                if (is_active(s))
                    m_seqs[s]->pop_trigger_undo();
            }
        }
        else
        {
            if (is_active(track))
                m_seqs[track]->pop_trigger_undo();
        }
        set_have_undo(! m_undo_vect.empty());
        set_have_redo(! m_redo_vect.empty());
    }
}

/**
 *  For every active sequence, call that sequence's pop_trigger_redo()
 *  function.
 */

void
perform::pop_trigger_redo ()
{
    if (! m_redo_vect.empty())
    {
        int track = m_redo_vect.back();
        m_redo_vect.pop_back();
        m_undo_vect.push_back(track);
        if (track == SEQ64_ALL_TRACKS)
        {
            for (int s = 0; s < m_sequence_high; ++s)   /* m_sequence_max  */
            {
                if (is_active(s))                       /* oops, was "track"! */
                    m_seqs[s]->pop_trigger_redo();
            }
        }
        else
        {
            if (is_active(track))
                m_seqs[track]->pop_trigger_redo();
        }
        set_have_undo(! m_undo_vect.empty());
        set_have_redo(! m_redo_vect.empty());
    }
}

/**
 *  If the left tick is less than the right tick, then, for each sequence that
 *  is active, its triggers are copied, offset by the difference between the
 *  right and left.  This copies the triggers between the L marker and R
 *  marker to the R marker.
 */

void
perform::copy_triggers ()
{
    if (m_left_tick < m_right_tick)
    {
        midipulse distance = m_right_tick - m_left_tick;
        for (int s = 0; s < m_sequence_high; ++s)       /* m_sequence_max   */
        {
            if (is_active(s))
                m_seqs[s]->copy_triggers(m_left_tick, distance);
        }
    }
}

/**
 *  Encapsulates a series of calls used in mainwnd.  We've reversed the
 *  start() and start_jack() calls so that JACK is started first, to match all
 *  of the other use-cases for playing that we've found in the code.  Note
 *  that the complementary function, stop_playing(), is an inline function
 *  defined in the header file.
 *
 *  The perform::start() function passes its boolean flag to
 *  perform::inner_start(), which sets the playback mode to that flag; if that
 *  flag is false, that turns off "song" mode.  So that explains why
 *  mute/unmute is disabled.
 *
 * Playback use cases:
 *
 *      These use cases are meant to apply to either a Seq32 or a regular build
 *      of Sequencer64, eventually.  Currently, the regular build does not have
 *      a concept of a "global" perform song-mode flag.
 *
 *      -#  mainwnd.
 *          -#  Play.  If the perform song-mode is "Song", then use that mode.
 *              Otherwise, use "Live" mode.
 *          -#  Stop.  This action is modeless here.  In ALSA, it will cause
 *              a rewind (but currently seqroll doesn't rewind until Play is
 *              clicked, a minor bug).
 *          -#  Pause.  Same processing as Play or Stop, depending on current
 *              status.  When stopping, the progress bars in seqroll and
 *              perfroll remain at their current point.
 *      -#  perfedit.
 *          -#  Play.  Override the current perform song-mode to use "Song".
 *          -#  Stop.  Revert the perfedit setting, in case play is restarted
 *              or resumed via mainwnd.
 *          -#  Pause.  Same processing as Play or Stop, depending on current
 *              status.
 *       -# ALSA versus JACK.  One issue here is that, if JACK isn't "running"
 *          at all (i.e. we are in ALSA mode), then we cannot be JACK Master.
 *
 *  Helgrind shows a read/write race condition in m_start_from_perfedit
 *  bewteen jack_transport_callback() and start_playing() here.  Is inline
 *  function access of a boolean atomic?
 *
 * \param songmode
 *      Indicates if the caller wants to start the playback in Song mode
 *      (sometimes erroneously referred to as "JACK mode").  In the seq32 code
 *      at GitHub, this flag was identical to the "global_jack_start_mode"
 *      flag, which is true for Song mode, and false for Live mode.  False
 *      disables Song mode, and is the default, which matches seq24.
 *      Generally, we pass true in this parameter if we're starting playback
 *      from the perfedit window.  It alters the m_start_from_perfedit member,
 *      not the m_song_start_mode member (which replaces the global flag now).
 */

void
perform::start_playing (bool songmode)
{
    m_start_from_perfedit = songmode;
    songmode = songmode || song_start_mode();
    if (songmode)
    {
       /*
        * Allow to start at key-p position if set; for cosmetic reasons,
        * to stop transport line flicker on start, position to the left
        * tick.
        *
        *   m_jack_asst.position(true, m_left_tick);    // position_jack()
        *
        * The "! m_repostion" doesn't seem to make sense.
        */

       if (is_jack_master() && ! m_reposition)
           position_jack(true, m_left_tick);
    }
    else
    {
        if (is_jack_master())
            position_jack(false);
    }
    start_jack();
    start(songmode);                                    /* song mode       */
}

/**
 *  Encapsulates behavior needed by perfedit.  Note that we moved some of the
 *  code from perfedit::set_jack_mode() [the seq32 version] to this function.
 *
 * \param jack_button_active
 *      Indicates if the perfedit JACK button shows it is active.
 *
 * \return
 *      Returns true if JACK is running currently, and false otherwise.
 */

bool
perform::set_jack_mode (bool jack_button_active)
{
    if (! is_running())                         /* was global_is_running    */
    {
        if (jack_button_active)
            init_jack_transport();
        else
            deinit_jack_transport();
    }
#ifdef SEQ64_JACK_SUPPORT
    m_jack_asst.set_jack_mode(is_jack_running());    /* seqroll keybinding  */
#endif

    /*
     *  For setting the transport tick to display in the correct location.
     *  FIXME: does not work for slave from disconnected; need JACK position.
     */

    if (song_start_mode())
    {
        set_reposition(false);
        set_start_tick(get_left_tick());
    }
    else
        set_start_tick(get_tick());

    return is_jack_running();
}

/**
 *  Pause playback, so that progress bars stay where they are, and playback
 *  always resumes where it left off, at least in ALSA mode, which doesn't
 *  have to worry about being a "slave".
 *
 *  Currently almost the same as stop_playing(), but expanded as noted in the
 *  comments so that we ultimately have more granular control over what
 *  happens.  We're researching the whole sequence of stopping and starting,
 *  and it can be tricky to make correct changes.
 *
 *  We still need to make restarting pick up at the same place in ALSA mode;
 *  in JACK mode, JACK transport takes care of that feature.
 *
 * \change ca 2016-10-11
 *      User layk noted this call, and it makes sense to not do this here,
 *      since it is unknown at this point what the actual status is.  Note
 *      that we STILL need to FOLLOW UP on calls to pause_playing() and
 *      stop_playing() in perfedit, mainwnd, etc.
 *
 *      is_pattern_playing(false);
 *
 *      But what about is_running()?
 *
 * \param songmode
 *      Indicates that, if resuming play, it should play in Song mode (true)
 *      or Live mode (false).  See the comments for the start_playing()
 *      function.
 */

void
perform::pause_playing (bool songmode)
{
    m_dont_reset_ticks = true;
    is_running(! is_running());
    stop_jack();
    if (is_jack_running())
    {
        m_start_from_perfedit = songmode;   /* act like start_playing()     */
    }
    else
    {
        reset_sequences(true);              /* don't reset "last-tick"      */
        m_usemidiclock = false;
        m_start_from_perfedit = false;      /* act like stop_playing()      */
    }
}

/**
 *  Encapsulates a series of calls used in mainwnd.  Stops playback, turns off
 *  the (new) m_dont_reset_ticks flag, and set the "is-pattern-playing" flag
 *  to false.  With stop, reset the start-tick to either the left-tick or the
 *  0th tick (to be determined, currently resets to 0).
 */

void
perform::stop_playing ()
{
    stop_jack();
    stop();
    m_dont_reset_ticks = m_start_from_perfedit = false;
}

/**
 *  If JACK is supported and running, sets the position of the transport.
 *
 * \param songmode
 *      If true, playback is to be in Song mode.  Otherwise, it is to be in
 *      Live mode.
 *
 * \param tick
 *      Provides the pulse position to be set.  The default value is 0.
 */

void
perform::position_jack (bool songmode, midipulse tick)
{
#ifdef SEQ64_JACK_SUPPORT
    m_jack_asst.position(songmode, tick);
#endif
}

/**
 *  Performs a controlled activation of the jack_assistant and other JACK
 *  modules. Currently does work only for JACK; the activate() calls for other
 *  APIs just return true without doing anything.
 */

bool
perform::activate ()
{
    bool result = m_master_bus->activate();

#ifdef SEQ64_JACK_SUPPORT
    if (result)
        result = m_jack_asst.activate();
#endif

    return result;
}

/**
 *  If JACK is not running, call inner_start() with the given state.
 *
 * \question
 *      Should we also call song_start_mode(songmode) here?
 *
 * \param songmode
 *      If true, playback is to be in Song mode.  Otherwise, it is to be in
 *      Live mode.
 */

void
perform::start (bool songmode)
{
#ifdef SEQ64_JACK_SUPPORT
    if (! is_jack_running())
#endif
        inner_start(songmode);
}

/**
 *  If JACK is not running, call inner_stop().  The logic seems backward here,
 *  in that we call inner_stop() if JACK is not running.  Or perhaps we
 *  misunderstand the meaning of m_jack_running?
 *
 * Stazed:
 *
 *      This function's sole purpose was to prevent inner_stop() from being
 *      called internally when JACK was running... potentially twice.
 *      inner_stop() was called by output_func() when JACK sent a
 *      JackTransportStopped message. If seq42 initiated the stop, then
 *      stop_jack() was called which then triggered the JackTransportStopped
 *      message to output_func() which then triggered the bool stop_jack to
 *      call inner_stop().  The output_func() call to inner_stop() is only
 *      necessary when some other JACK client sends a jack_transport_stop
 *      message to JACK, not when it is initiated by seq42.  The method of
 *      relying on JACK to call inner_stop() when internally initiated caused
 *      a (very) obscure apparent freeze if you press and hold the start/stop
 *      key if set to toggle. This occurs because of the delay between
 *      JackTransportStarting and JackTransportStopped if both triggered in
 *      rapid succession by holding the toggle key down.  The variable
 *      global_is_running gets set false by a delayed inner_stop() from JACK
 *      after the start (true) is already sent. This means the global is set
 *      to true when JACK is actually off (false). Any subsequent presses to
 *      the toggle key send a stop message because the global is set to true.
 *      Because JACK is not running, output_func() is not running to send the
 *      inner_stop() call which resets the global to false. Thus an apparent
 *      freeze as the toggle key endlessly sends a stop, but inner_stop()
 *      never gets called to reset. Whoo! So, to fix this we just need to
 *      call inner_stop() directly rather than wait for JACK to send a
 *      delayed stop, only when running. This makes the whole purpose of this
 *      stop() function unneeded. The check for m_jack_running is commented
 *      out and this function could be removed. It is being left for future
 *      generations to ponder!!!
 */

void
perform::stop ()
{
    if (! is_jack_running())
        inner_stop();
}

/**
 *  Locks on m_condition_var.  Then, if not is_running(), the playback
 *  mode is set to the given state.  If that state is true, call
 *  off_sequences().  Set the running status, and signal the condition.
 *  Then unlock.
 *
 * Minor issue:
 *
 *      In ALSA mode, restarting the sequence moves the progress bar to the
 *      beginning of the sequence, even if just pausing.  This is fixed by
 *      compiling with SEQ64_PAUSE_SUPPORT, which disables calling
 *      off_sequences() when starting playback from the song editor /
 *      performance window.
 *
 * \param songmode
 *      Sets the playback mode, and, if true, turns off all of the sequences
 *      before setting the is-running condition.
 */

void
perform::inner_start (bool songmode)
{
    m_condition_var.lock();
    if (! is_running())
    {
        playback_mode(songmode);
        if (songmode)
            off_sequences();

        is_running(true);
        m_condition_var.signal();
    }
    m_condition_var.unlock();
}

/**
 *  Unconditionally, and without locking, clears the running status and resets
 *  the sequences.  Sets m_usemidiclock to the given value.  Note that we do
 *  need to set the running flag to false here, even when JACK is running.
 *  Otherwise, JACK starts ping-ponging back and forth between positions under
 *  some circumstances.
 *
 *  However, if JACK is running, we do not want to reset the sequences... this
 *  causes the progress bar for each sequence to move to near the end of the
 *  sequence.
 *
 * \param midiclock
 *      If true, indicates that the MIDI clock should be used.
 */

void
perform::inner_stop (bool midiclock)
{
    start_from_perfedit(false);
    is_running(false);
    reset_sequences();
    m_usemidiclock = midiclock;
}

/**
 *  For all active patterns/sequences, set the playing state to false.
 *
 *  Replaces "for (int s = 0; s < m_sequence_max; ++s)"
 */

void
perform::off_sequences ()
{
    for (int s = 0; s < m_sequence_high; ++s)       /* modest speed-up */
    {
        if (is_active(s))
            m_seqs[s]->set_playing(false);
    }
}

/**
 *  Does a toggle-queueing for all of the sequences in the current screenset,
 *  for all sequences that are on, and for the currently hot-keyed sequence.
 *
 *  This function supports the new queued-replace feature.  This feature is
 *  activated when the keep-queue feature is turned on via its hot-key, and
 *  then the replace hot-key is used on a pattern.  When that happens, the
 *  current sequence is queued to be toggled, and all unmuted sequences are
 *  queued to toggle off at the same time.  Thus, this is a kind of
 *  queued-solo feature.
 *
 *  This function assumes we have called save_current_screenset() first,
 *  so that the soloing can be exactly toggled.  Only sequences that were
 *  initially on should be toggled.
 *
 * \param current_seq
 *      This number is that of the sequence/pattern whose hot-key was struck.
 *      We don't want to toggle this one off, just on.
 */

void
perform::unqueue_sequences (int current_seq)
{
    for (int s = 0; s < m_seqs_in_set; ++s)
    {
        int seq = m_screenset_offset + s;           /* not play-screen      */
        if (is_active(seq))
        {
            if (seq == current_seq)
            {
                if (! m_seqs[seq]->get_playing())
                    m_seqs[seq]->toggle_queued();
            }
            else if (m_screenset_state[s])          /* state of current set */
                m_seqs[seq]->toggle_queued();
        }
    }
}

/**
 *  For all active patterns/sequences, turn off its playing notes.
 *  Then flush the master MIDI buss.
 */

void
perform::all_notes_off ()
{
    for (int s = 0; s < m_sequence_high; ++s)   /* a modest speed-up    */
    {
        if (is_active(s))
            m_seqs[s]->off_playing_notes();
    }
    if (not_nullptr(m_master_bus))
    {
        m_master_bus->flush();                  /* flush the MIDI buss  */
    }
}

/**
 *  Similar to all_notes_off(), but also sends Note Off events directly to the
 *  active busses.  Adapted from Oli Kester's Kepler34 project.
 */

void
perform::panic ()
{
    stop_playing();
    inner_stop();                               /* EXPERIMENT           */
    for (int s = 0; s < m_sequence_high; ++s)   /* a modest speed-up    */
    {
        sequence * sptr = get_sequence(s);
        if (not_nullptr(sptr))
            sptr->off_playing_notes();
    }
    if (not_nullptr(m_master_bus))
        m_master_bus->panic();                  /* flush the MIDI buss  */

    set_tick(0);
}

/**
 *  For all active patterns/sequences, get its playing state, turn off the
 *  playing notes, set playing to false, zero the markers, and, if not in
 *  playback mode, restore the playing state.  Note that these calls are
 *  folded into one member function of the sequence class.  Finally, flush the
 *  master MIDI buss.
 *
 *  Could use a member function pointer to avoid having to code two loops.
 *  We did it.
 *
 * \param pause
 *      Try to prevent notes from lingering on pause if true.  By default, it
 *      is false.
 */

void
perform::reset_sequences (bool pause)
{
    void (sequence::* f) (bool) = pause ? &sequence::pause : &sequence::stop ;
    for (int s = 0; s < m_sequence_high; ++s)           /* m_sequence_max   */
    {
        if (is_active(s))
            (m_seqs[s]->*f)(m_playback_mode);           /* (new parameter)  */
    }
    m_master_bus->flush();                              /* flush MIDI buss  */
}

/**
 *  Creates the output thread using output_thread_func().  This might be a
 *  good candidate for a small thread class derived from a small base class.
 */

void
perform::launch_output_thread ()
{
    int err = pthread_create(&m_out_thread, NULL, output_thread_func, this);
    if (err != 0)
    {
        /* TODO: error handling */
    }
    else
        m_out_thread_launched = true;
}

/**
 *  Creates the input thread using input_thread_func().  This might be a good
 *  candidate for a small thread class derived from a small base class.
 */

void
perform::launch_input_thread ()
{
    int err = pthread_create(&m_in_thread, NULL, input_thread_func, this);
    if (err != 0)
    {
        /* TODO: error handling */
    }
    else
        m_in_thread_launched = true;
}

/**
 *  Pass-along to sequence::get_trigger_state().
 *
 * \param seqnum
 *      The index to the desired sequence.
 *
 * \param tick
 *      The time-location at which to get the trigger state.
 *
 * \return
 *      Returns true if the sequence indicated by \a seqnum exists and it's
 *      trigger state is true.
 */

bool
perform::get_trigger_state (int seqnum, midipulse tick) const
{
    const sequence * s = get_sequence(seqnum);
    return not_nullptr(s) ? s->get_trigger_state(tick) : false ;
}

/**
 *  Adds a trigger on behalf of a sequence.
 *
 * \param seqnum
 *      Indicates the sequence that needs to have its trigger handled.
 *
 * \param tick
 *      The MIDI pulse number at which the trigger should be handled.
 */

void
perform::add_trigger (int seqnum, midipulse tick)
{
    sequence * s = get_sequence(seqnum);
    if (not_nullptr(s))
    {
        midipulse seqlength = s->get_length();
#ifdef SEQ64_SONG_RECORDING
        if (song_record_snap())         /* snap to the length of sequence   */
#endif
            tick -= tick % seqlength;

        push_trigger_undo(seqnum);
        s->add_trigger(tick, seqlength);
        modify();
    }
}

/**
 *	Delete the existing specified trigger.
 *
 * \param seqnum
 *      Indicates the sequence that needs to have its trigger handled.
 *
 * \param tick
 *      The MIDI pulse number at which the trigger should be handled.
 */

void
perform::delete_trigger (int seqnum, midipulse tick)
{
    sequence * s = get_sequence(seqnum);
    if (not_nullptr(s))
    {
        push_trigger_undo(seqnum);
        s->delete_trigger(tick);
        modify();
    }
}

/**
 *	Add a new trigger if nothing is selected, otherwise delete the existing
 *	trigger.
 *
 * \param seqnum
 *      Indicates the sequence that needs to have its trigger handled.
 *
 * \param tick
 *      The MIDI pulse number at which the trigger should be handled.
 */

void
perform::add_or_delete_trigger (int seqnum, midipulse tick)
{
    sequence * s = get_sequence(seqnum);
    if (not_nullptr(s))
    {
        bool state = s->get_trigger_state(tick);
        push_trigger_undo(seqnum);
        if (state)
        {
            s->delete_trigger(tick);
        }
        else
        {
            midipulse seqlength = s->get_length();
            s->add_trigger(tick, seqlength);
        }
        modify();
    }
}

/**
 *  Convenience function for perfroll's split-trigger functionality.
 *
 * \param seqnum
 *      Indicates the sequence that needs to have its trigger split.
 *
 * \param tick
 *      The MIDI pulse number at which the trigger should be split.
 */

void
perform::split_trigger (int seqnum, midipulse tick)
{
    sequence * s = get_sequence(seqnum);
    if (not_nullptr(s))
    {
        push_trigger_undo(seqnum);
#ifdef SEQ64_SONG_BOX_SELECT
        s->half_split_trigger(tick);
#else
        s->split_trigger(tick);
#endif
        modify();
    }
}

/**
 *  Convenience function for perfroll's paste-trigger functionality.
 *
 * \param seqnum
 *      Indicates the sequence that needs to have its trigger pasted.
 *
 * \param tick
 *      The MIDI pulse number at which the trigger should be pasted.
 */

void
perform::paste_trigger (int seqnum, midipulse tick)
{
    sequence * s = get_sequence(seqnum);
    if (not_nullptr(s))
    {
        push_trigger_undo(seqnum);
        s->paste_trigger(tick);
        modify();
    }
}

/**
 *  Convenience function for perfroll's paste-or-split-trigger functionality.
 *
 * \param seqnum
 *      Indicates the sequence that needs to have its trigger handled.
 *
 * \param tick
 *      The MIDI pulse number at which the trigger should be handled.
 */

void
perform::paste_or_split_trigger (int seqnum, midipulse tick)
{
    sequence * s = get_sequence(seqnum);
    if (not_nullptr(s))
    {
        bool state = s->get_trigger_state(tick);
        push_trigger_undo(seqnum);
        if (state)
            s->split_trigger(tick);
        else
            s->paste_trigger(tick);

        modify();
    }
}

/**
 *  Finds the trigger intersection.
 *
 * \param seqnum
 *      The number of the sequence in question.
 *
 * \param tick
 *      The time-location desired.
 *
 * \return
 *      Returns true if the sequence exists and the
 *      sequence::intersect_triggers() function returned true.
 */

bool
perform::intersect_triggers (int seqnum, midipulse tick)
{
    bool result = false;
    sequence * s = get_sequence(seqnum);
    if (not_nullptr(s))
        result = s->intersect_triggers(tick);

    return result;
}

/**
 *  Locates the largest trigger value among the active sequences.
 *
 * \return
 *      Returns the highest trigger value, or zero.  It is not clear why
 *      this function doesn't return a "no trigger found" value. Is there
 *      always at least one trigger, at 0?
 */

midipulse
perform::get_max_trigger () const
{
    midipulse result = 0;
    for (int s = 0; s < m_sequence_high; ++s)           /* modest speed-up */
    {
        if (is_active(s))               // sequence * s = get_sequence(seqnum);
        {
            midipulse t = m_seqs[s]->get_max_trigger();
            if (t > result)
                result = t;
        }
    }
    return result;
}

/**
 *  Set up the performance, set the process to realtime privileges, and then
 *  start the output function.
 *
 * \param myperf
 *      Provides the perform object instance that is to be used.  Its
 *      output_func() is called.  Currently, this parameter is not validated,
 *      for speed.
 *
 * \return
 *      Always returns nullptr.
 */

void *
output_thread_func (void * myperf)
{
    perform * p = (perform *) myperf;

#ifdef PLATFORM_WINDOWS
    timeBeginPeriod(1);                         /* WinMM.dll function   */
    p->output_func();
    timeEndPeriod(1);
#else
    if (rc().priority())                        /* Not in MinGW RCB     */
    {
        struct sched_param schp;
        memset(&schp, 0, sizeof(sched_param));
        schp.sched_priority = 1;
        if (sched_setscheduler(0, SCHED_FIFO, &schp) != 0)
        {
            errprint
            (
                "output_thread_func: couldn't sched_setscheduler(FIFO), "
                "need root priviledges."
            );
            pthread_exit(0);
        }
        else
        {
            infoprint("[Output priority set to 1]");
        }
    }
    p->output_func();
#endif

    return nullptr;
}

/**
 *  Initializes JACK support, if SEQ64_JACK_SUPPORT is defined.  Who calls
 *  this routine?  The main() routine of the application [via launch()],
 *  and the options module, when the Connect button is pressed.
 *
 * \return
 *      Returns the result of the init() call; true if JACK sync is now
 *      running.  If JACK support is not built into the application, then
 *      this function returns false, to indicate that JACK is (definitely)
 *      not running.
 */

bool
perform::init_jack_transport ()
{
#ifdef SEQ64_JACK_SUPPORT
    return m_jack_asst.init();
#else
    return false;
#endif
}

/**
 *  Tears down the JACK infrastructure.  Called by launch() and in the
 *  options module, when the Disconnect button is pressed.  This function
 *  operates only while Sequencer64 is not outputing, otherwise we have a
 *  race condition that can lead to a crash.
 *
 * \return
 *      Returns the result of the init() call; false if JACK sync is now
 *      no longer running.  If JACK support is not built into the
 *      application, then this function returns true, to indicate that
 *      JACK is (definitely) not running.
 */

bool
perform::deinit_jack_transport ()
{
#ifdef SEQ64_JACK_SUPPORT
    return m_jack_asst.deinit();
#else
    return true;
#endif
}

/**
 *  Performance output function.  This function is called by the free function
 *  output_thread_func().  Here's how it works:
 *
 *      -   It runs while m_outputing is true.
 *      -   MORE TO COME.  Yeah, a lot more to come.  It is a complex
 *          function.
 *
 * \change ca 2016-01-26
 *      Hurray, seq24 is coming back to life!  We see that there is a fix for
 *      clock tick drift here, which relies on using long and long long
 *      values.  See the Changelog for seq24 0.9.3.
 *
 * \warning
 *      Valgrind shows that output_func() is being called before the JACK
 *      client pointer is being initialized!!!
 */

void
perform::output_func ()
{
    while (m_outputing)         /* PERHAPS we should LOCK this variable */
    {
        m_condition_var.lock();
        while (! is_running())
        {
            m_condition_var.wait();
            if (! m_outputing)              /* if stopping, kill thread */
                break;
        }
        m_condition_var.unlock();

#ifdef PLATFORM_WINDOWS
        long last;                          // beginning time
        long current;                       // current time
#ifdef SEQ64_STATISTICS_SUPPORT
        long stats_loop_start = 0;
        long stats_loop_finish = 0;
#endif
        long delta;                         // difference between last & current
#else                                       // not Windows
        struct timespec last;               // beginning time
        struct timespec current;            // current time
#ifdef SEQ64_STATISTICS_SUPPORT
        struct timespec stats_loop_start;
        struct timespec stats_loop_finish;
#endif
        struct timespec delta;              // difference between last & current
#endif

        jack_scratchpad pad;
        pad.js_total_tick = 0.0;            // double
        pad.js_clock_tick = 0;              // long probably offers more ticks
        if (m_dont_reset_ticks)
        {
            pad.js_current_tick = get_jack_tick();

            /*
             * We still need this flag, so move this setting until later.
             *
             * m_dont_reset_ticks = false;
             */
        }
        else
        {
            pad.js_current_tick = 0.0;      // tick and tick fraction
            pad.js_total_tick = 0.0;
#ifdef SEQ64_SONG_RECORDING
            m_current_tick = 0.0;
#endif
        }

        pad.js_jack_stopped = false;
        pad.js_dumping = false;
        pad.js_init_clock = true;
        pad.js_looping = m_looping;
        pad.js_playback_mode = m_playback_mode;
        pad.js_ticks_converted_last = 0.0;
        pad.js_ticks_converted = 0.0;
        pad.js_ticks_delta = 0.0;
        pad.js_delta_tick_frac = 0L;        // from seq24 0.9.3, long value

        /*
         * Not sure that we really need this feature.  Will have to see if
         * anyone wants it.  This is currently not a complete commenting out
         * of the feature at this time.
         */

#ifdef SEQ64_STATISTICS_SUPPORT
        midipulse stats_total_tick = 0;
        long stats_loop_index = 0;
        long stats_min = 0x7FFFFFFF;
        long stats_max = 0;
        long stats_avg = 0;
        long stats_last_clock_us = 0;
        long stats_clock_width_us = 0;
        long stats_all[100];                // why 100?
        long stats_clock[100];
        if (rc().stats())                   // \change ca 2016-01-24
        {
            for (int i = 0; i < 100; ++i)
            {
                stats_all[i] = 0;
                stats_clock[i] = 0;
            }
        }
#endif  // SEQ64_STATISTICS_SUPPORT

        /*
         * If we are in the performance view (song editor), we care about
         * starting from the m_starting_tick offset.  However, if the pause
         * key is what is resuming playback, then we do not want to reset the
         * position.  So how to detect that situation, since m_is_pause is now
         * false?
         */

#ifdef SEQ64_JACK_SUPPORT
        bool ok = m_playback_mode && ! is_jack_running();
#else
        bool ok = m_playback_mode;
#endif

        ok = ok && ! m_dont_reset_ticks;
        m_dont_reset_ticks = false;
        if (ok)
        {
#ifdef SEQ64_SONG_RECORDING
            m_current_tick = double(m_starting_tick);
#endif
            pad.js_current_tick = long(m_starting_tick);    // midipulse
            pad.js_clock_tick = m_starting_tick;
            set_orig_ticks(m_starting_tick);                // what member?
        }

        int ppqn = m_master_bus->get_ppqn();

#ifdef SEQ64_STATISTICS_SUPPORT

#ifdef PLATFORM_WINDOWS
        last = timeGetTime();                   // get start time position
        if (rc().stats())
            stats_last_clock_us = last * 1000;
#else
        clock_gettime(CLOCK_REALTIME, &last);   // get start time position
        if (rc().stats())
            stats_last_clock_us = (last.tv_sec*1000000) + (last.tv_nsec/1000);
#endif

#else   // SEQ64_STATISTICS_SUPPORT

#ifdef PLATFORM_WINDOWS
        last = timeGetTime();                   // get start time position
#else
        clock_gettime(CLOCK_REALTIME, &last);   // get start time position
#endif

#endif  // SEQ64_STATISTICS_SUPPORT

        while (is_running())
        {
            /**
             * -# Get delta time (current - last).
             * -# Get delta ticks from time.
             * -# Add to current_ticks.
             * -# Compute prebuffer ticks.
             * -# Play from current tick to prebuffer.
             */

#ifdef SEQ64_STATISTICS_SUPPORT
            if (rc().stats())
            {
#ifdef PLATFORM_WINDOWS
                stats_loop_start = timeGetTime();
#else
                clock_gettime(CLOCK_REALTIME, &stats_loop_start);
#endif
            }
#endif  // SEQ64_STATISTICS_SUPPORT

            /*
             * Get the delta time.
             */

#ifdef PLATFORM_WINDOWS
            current = timeGetTime();
            delta = current - last;
            long delta_us = delta * 1000;
#else
            clock_gettime(CLOCK_REALTIME, &current);
            delta.tv_sec  = current.tv_sec - last.tv_sec;       // delta!
            delta.tv_nsec = current.tv_nsec - last.tv_nsec;     // delta!
            long delta_us = (delta.tv_sec * 1000000) + (delta.tv_nsec / 1000);
#endif
            midibpm bpm  = m_master_bus->get_beats_per_minute();

            /*
             * Delta time to ticks; get delta ticks.  seq24 0.9.3 changes
             * delta_tick's type and adds code -- delta_ticks_frac is in
             * 1000th of a tick.  This code is meant to correct for clock
             * drift.  However, this code breaks the MIDI clock speed.  So we
             * use the "Stazed" version of the code, from seq32.  We get delta
             * ticks, delta_ticks_f is in 1000th of a tick.
             */

            long long delta_tick_denom = 60000000LL;
            long long delta_tick_num = bpm * ppqn * delta_us +
                pad.js_delta_tick_frac;

            long delta_tick = long(delta_tick_num / delta_tick_denom);
            pad.js_delta_tick_frac = long(delta_tick_num % delta_tick_denom);
            if (m_usemidiclock)
            {
                delta_tick = m_midiclocktick;       /* int to double */
                m_midiclocktick = 0;
            }
            if (m_midiclockpos >= 0)
            {
                delta_tick = 0;
#ifdef SEQ64_SONG_RECORDING
                m_current_tick = double(m_midiclockpos);
#endif
                pad.js_clock_tick = pad.js_current_tick = pad.js_total_tick =
                    m_midiclockpos;

                m_midiclockpos = -1;
            }

#ifdef SEQ64_JACK_SUPPORT
            bool jackrunning = m_jack_asst.output(pad);     // offloaded code
            if (jackrunning)
            {
                // No additional code needed besides the output() call above.
            }
            else
            {
#endif

#ifdef USE_THIS_STAZED_CODE_WHEN_READY
                /*
                 * If we reposition key-p, FF, rewind, adjust delta_tick for
                 * change then reset to adjusted starting.  We have to grab
                 * the clock tick if looping is unchecked while we are
                 * running the performance; we have to initialize the MIDI
                 * clock (send EVENT_MIDI_SONG_POS); we have to restart at
                 * the left marker; and reset the tempo list (which Seq64
                 * doesn't have).
                 */

                if (m_playback_mode && && ! m_usemidiclock && m_reposition)
                {
                    current_tick = clock_tick;
                    delta_tick = m_starting_tick - clock_tick;
                    init_clock = true;
                    m_starting_tick = m_left_tick;
                    m_reposition = false;
                    m_reset_tempo_list = true;
                }
#endif  // USE_THIS_STAZED_CODE_WHEN_READY

                /*
                 * The default if JACK is not compiled in, or is not
                 * running.  Add the delta to the current ticks.
                 */

                pad.js_clock_tick += delta_tick;
                pad.js_current_tick += delta_tick;
                pad.js_total_tick += delta_tick;
                pad.js_dumping = true;
#ifdef SEQ64_SONG_RECORDING
                m_current_tick = double(pad.js_current_tick);
#endif
#ifdef SEQ64_JACK_SUPPORT
            }
#endif

            /*
             * If we reposition key-p from perfroll, reset to adjusted
             * start.
             */

            bool change_position =
                m_playback_mode && ! is_jack_running() && ! m_usemidiclock;

            if (change_position)
                change_position = m_reposition;

            if (change_position)
            {
                set_orig_ticks(m_starting_tick);
                m_starting_tick = m_left_tick;      // restart at left marker
                m_reposition = false;
            }

            /*
             * pad.js_init_clock will be true when we run for the first time,
             * or as soon as JACK gets a good lock on playback.
             */

            if (pad.js_init_clock)
            {
                m_master_bus->init_clock(midipulse(pad.js_clock_tick));
                pad.js_init_clock = false;
            }
            if (pad.js_dumping)
            {
                /*
                 * This is a mess we will have to sort out.  If looping, then
                 * we ought to play if any of the tested flags are true.
                 */

                bool perfloop = m_looping;
                if (perfloop)
                {
                    perfloop = m_playback_mode || start_from_perfedit() ||
                        song_start_mode();
                }
                if (perfloop)
                {
                    /*
                     * This stazed JACK code works better than the original
                     * code, so it is now permanent code.
                     */

                    static bool jack_position_once = false;
                    midipulse rtick = get_right_tick();     /* can change? */
                    if (pad.js_current_tick >= rtick)
                    {
                        if (is_jack_master() && ! jack_position_once)
                        {
                            position_jack(true, m_left_tick);
                            jack_position_once = true;
                        }
                        double leftover_tick = pad.js_current_tick - rtick;

                        /*
                         * Do not play during starting to avoid xruns on
                         * fast-forward or rewind.
                         */

                        if (is_jack_running())
                        {
#ifdef SEQ64_JACK_SUPPORT
                            if (m_jack_asst.transport_not_starting())
                                play(rtick - 1);                    // play!
#endif
                        }
                        else
                            play(rtick - 1);                        // play!

                        midipulse ltick = get_left_tick();
                        reset_sequences();                          // reset!
                        set_orig_ticks(ltick);
#ifdef SEQ64_SONG_RECORDING
                        m_current_tick = double(ltick) + leftover_tick;
#endif
                        pad.js_current_tick = double(ltick) + leftover_tick;
                    }
                    else
                        jack_position_once = false;
                }

                /*
                 * Don't play during JackTransportStarting to avoid xruns on
                 * FF or RW.
                 */

                if (is_jack_running())
                {
#ifdef SEQ64_JACK_SUPPORT
                    if (m_jack_asst.transport_not_starting())
#endif
                        play(midipulse(pad.js_current_tick));       // play!
                }
                else
                    play(midipulse(pad.js_current_tick));           // play!

                /*
                 * The next line enables proper pausing in both old and seq32
                 * JACK builds.
                 */

                set_jack_tick(pad.js_current_tick);

                /*
                 * ca 2017-04-03 issue #67.
                 * Somehow we are calling the wrong function, not the one we
                 * need to emit the MIDI clock.
                 *
                 * m_master_bus->clock(midipulse(pad.js_clock_tick));
                 */

                m_master_bus->emit_clock(midipulse(pad.js_clock_tick));

#ifdef SEQ64_STATISTICS_SUPPORT
                if (rc().stats())
                {
                    while (stats_total_tick <= pad.js_total_tick)
                    {
                        /*
                         * Uses inline function for c_ppqn / 24.  Checks to see
                         * if there was a tick.  What's up with the constants
                         * 100 and 300?
                         */

                        int ct = clock_ticks_from_ppqn(m_ppqn);
                        if ((stats_total_tick % ct) == 0)
                        {
#ifdef PLATFORM_WINDOWS
                            long current_us = current * 1000;
#else
                            long current_us = (current.tv_sec * 1000000) +
                                (current.tv_nsec / 1000);
#endif
                            stats_clock_width_us = current_us-stats_last_clock_us;
                            stats_last_clock_us = current_us;

                            int index = stats_clock_width_us / 300;
                            if (index >= 100)
                                index = 99;

                            stats_clock[index]++;
                        }
                        stats_total_tick++;
                    }
                }
#endif  // SEQ64_STATISTICS_SUPPORT
            }

            /**
             *  Figure out how much time we need to sleep, and do it.
             */

            last = current;

#ifdef PLATFORM_WINDOWS
            current = timeGetTime();
            delta = current - last;
            long elapsed_us = delta * 1000;
#else
            clock_gettime(CLOCK_REALTIME, &current);
            delta.tv_sec  = current.tv_sec  - last.tv_sec;
            delta.tv_nsec = current.tv_nsec - last.tv_nsec;
            long elapsed_us = (delta.tv_sec * 1000000) + (delta.tv_nsec / 1000);
#endif

            /**
             * Now we want to trigger every c_thread_trigger_width_us, and it
             * took us delta_us to play().  Also known as the "sleeping_us".
             */

            delta_us = c_thread_trigger_width_us - elapsed_us;

            /**
             * Check MIDI clock adjustment.  Note that we replaced
             * "60000000.0f / m_ppqn / bpm" with a call to a function.  We
             * also removed the "f" specification from the constants.
             */

            double dct = double_ticks_from_ppqn(m_ppqn);
            double next_total_tick = pad.js_total_tick + dct;
            double next_clock_delta = next_total_tick - pad.js_total_tick - 1;
            double next_clock_delta_us =
                next_clock_delta * pulse_length_us(bpm, m_ppqn);

            if (next_clock_delta_us < (c_thread_trigger_width_us * 2.0))
                delta_us = long(next_clock_delta_us);

            if (delta_us > 0)
            {
#ifdef PLATFORM_WINDOWS
                delta = delta_us / 1000;
                Sleep(delta);
#else
                delta.tv_sec = delta_us / 1000000;
                delta.tv_nsec = (delta_us % 1000000) * 1000;
                nanosleep(&delta, NULL);    /* nanosleep() is Linux */
#endif
            }
#ifdef SEQ64_STATISTICS_SUPPORT
            else
            {
                if (rc().stats())
                {
                    errprint("Underrun");
                }
            }
#endif  // SEQ64_STATISTICS_SUPPORT

#ifdef SEQ64_STATISTICS_SUPPORT
            if (rc().stats())
            {
#ifdef PLATFORM_WINDOWS
                stats_loop_finish = timeGetTime();
                delta = stats_loop_finish - stats_loop_start;
                long delta_us = delta * 1000;
#else
                clock_gettime(CLOCK_REALTIME, &stats_loop_finish);
                delta.tv_sec  = stats_loop_finish.tv_sec-stats_loop_start.tv_sec;
                delta.tv_nsec = stats_loop_finish.tv_nsec-stats_loop_start.tv_nsec;
                long delta_us = (delta.tv_sec*1000000) + (delta.tv_nsec/1000);
#endif
                int index = delta_us / 100;         // why the 100?
                if (index >= 100)
                    index = 99;

                stats_all[index]++;
                if (delta_us > stats_max)
                    stats_max = delta_us;

                if (delta_us < stats_min)
                    stats_min = delta_us;

                stats_avg += delta_us;
                stats_loop_index++;
                if (stats_loop_index > 200)         // what is 200?
                {
                    stats_loop_index = 0;
                    stats_avg /= 200;
                    printf
                    (
                        "stats_avg[%ld]us stats_min[%ld]us stats_max[%ld]us\n",
                        stats_avg, stats_min, stats_max
                    );
                    stats_min = 0x7FFFFFFF;
                    stats_max = 0;
                    stats_avg = 0;
                }
            }
#endif  // SEQ64_STATISTICS_SUPPORT

            if (pad.js_jack_stopped)
                inner_stop();
        }
#ifdef SEQ64_STATISTICS_SUPPORT
        if (rc().stats())
        {
            printf("\n\n-- trigger width --\n");
            for (int i = 0; i < 100; ++i)
            {
                printf("[%3d][%8ld]\n", i * 100, stats_all[i]);
            }
            printf("\n\n-- clock width --\n");

            midibpm bpm  = m_master_bus->get_beats_per_minute();
            printf
            (
                "optimal: [%d us]\n", int(clock_tick_duration_bogus(bpm, m_ppqn))
            );
            for (int i = 0; i < 100; ++i)
            {
                printf("[%3d][%8ld]\n", i * 300, stats_clock[i]);
            }
        }
#endif  // SEQ64_STATISTICS_SUPPORT

        /*
         * Disabling this setting allows all of the progress bars (seqroll,
         * perfroll, and the slots in the mainwid) to stay visible where
         * they paused.  However, the progress still restarts when playback
         * begins again, without some other changes.  m_tick is the progress
         * play tick that displays the progress bar.
         */

        if (m_playback_mode)
        {
            if (is_jack_master())                       // running Song Master
                position_jack(m_playback_mode, m_left_tick);
        }
        else
        {
            if (is_jack_master())                       // running Live Master
                position_jack(m_playback_mode, 0);      // ca 2016-01-21
        }
        if (! m_usemidiclock)                           // stop by MIDI event?
        {
            if (! is_jack_running())
            {
                if (m_playback_mode)
                    set_tick(m_left_tick);              // song mode default
                else if (! m_dont_reset_ticks)
                    set_tick(0);                        // live mode default
            }
        }

        /*
         * This means we leave m_tick at stopped location if in slave mode or
         * if m_usemidiclock == true.
         */

        m_master_bus->flush();
        m_master_bus->stop();

        /*
         * In the new rtmidi version of the application (seq64), enabling this
         * code causes some conflicts between data access, and some how
         * jack_assistant::m_jack_client ends up being corrupted.
         */

#if USE_THIS_SEGFAULT_CAUSING_CODE
        if (is_jack_running())
            set_jack_stop_tick(get_current_jack_position((void *) this));
#endif

    }
    pthread_exit(0);
}

/**
 *  Set up the performance, and set the process to realtime privileges.
 *
 * \param myperf
 *      Provides the perform object instance that is to be used.  Its
 *      output_func() is called.  Currently, this parameter is not validated,
 *      for speed.
 *
 * \return
 *      Always returns nullptr.
 */

void *
input_thread_func (void * myperf)
{
    if (not_nullptr(myperf))
    {
        perform * p = (perform *) myperf;

#ifdef PLATFORM_WINDOWS
        timeBeginPeriod(1);
        p->input_func();
        timeEndPeriod(1);
#else                                   // MinGW RCB
        if (rc().priority())
        {
            struct sched_param schp;
            memset(&schp, 0, sizeof(sched_param));
            schp.sched_priority = 1;
            if (sched_setscheduler(0, SCHED_FIFO, &schp) != 0)
            {
                printf
                (
                    "input_thread_func: couldn't sched_setscheduler"
                   "(FIFO), need root priviledges."
                );
                pthread_exit(0);
            }
            else
            {
                infoprint("[Input priority set to 1]");
            }
        }
        p->input_func();
#endif
    }
    return nullptr;
}

/**
 *  Handle the MIDI Control values that provide some automation for the
 *  application.
 *
\verbatim
        c_midi_control_mod_replace
        c_midi_control_mod_snapshot
        c_midi_control_mod_queue
        c_midi_control_mod_gmute
        c_midi_control_mod_glearn
\endverbatim
 *
 *  Other values supported:
 *
\verbatim
        c_midi_control_bpm_up
        c_midi_control_bpm_dn
        c_midi_control_ss_up
        c_midi_control_ss_dn
        c_midi_control_play_ss
\endverbatim
 *
 *  We have added the following extended values:
 *
\verbatim
        c_midi_control_playback     (for pause/toggle, start, and stop)
        c_midi_control_song_record  (for performance record toggle/on/off)
        c_midi_control_solo         (for toggle, on, or off)
        c_midi_control_thru         (see record below)
        c_midi_control_bpm_page_up
        c_midi_control_bpm_page_dn
        c_midi_control_ss_set
        c_midi_control_record       (see thru above)
        c_midi_control_quan_record  (quantized record, see thru above)
        c_midi_control_reset_seq    (resets the sequence)
\endverbatim
 *
 *  The extended values will actually be handled by a new function,
 *  handle_midi_control_ex().
 *
 *  c_midi_control_solo probably will need a parameter.
 *
 *  Values from 32 through 2*32 are normalized by subtracting 32 and passed to
 *  the select_and_mute_group() function.  Otherwise, the following apply:
 *
 *  We also reserve a few control values above that for expansion.
 *
 * \param ctl
 *      The MIDI control value to use to perform an operation.
 *
 * \param state
 *      The state of the control, used with the following values:
 *
 * \return
 *      Returns true if the event was handled.  Mostly rote, for conformance
 *      with the newer handle_midi_control_ex() function.
 */

bool
perform::handle_midi_control (int ctl, bool state)
{
    bool result = false;
    switch (ctl)
    {
    case c_midi_control_bpm_up:

        result = true;
        (void) increment_beats_per_minute();
        break;

    case c_midi_control_bpm_dn:

        result = true;
        (void) decrement_beats_per_minute();
        break;

    /*
     * Handled in handle_midi_control_ex().  If we get here, ignore these
     * extended controls.
     */

    case c_midi_control_bpm_page_up:
    case c_midi_control_bpm_page_dn:

        result = false;
        /* printf("BPM UP/DOWN ignored\n"); */
        break;

    case c_midi_control_ss_up:

        result = true;
        (void) increment_screenset();
        break;

    case c_midi_control_ss_dn:

        result = true;
        (void) decrement_screenset();
        break;

    case c_midi_control_mod_replace:

        result = true;
        if (state)
            set_sequence_control_status(c_status_replace);
        else
            unset_sequence_control_status(c_status_replace);
        break;

    case c_midi_control_mod_snapshot:

        result = true;
        if (state)
            set_sequence_control_status(c_status_snapshot);
        else
            unset_sequence_control_status(c_status_snapshot);
        break;

    case c_midi_control_mod_queue:

        result = true;
        if (state)
            set_sequence_control_status(c_status_queue);
        else
            unset_sequence_control_status(c_status_queue);
        break;

    /*
     * TODO:  c_status_oneshot
     */

    case c_midi_control_mod_gmute:

        result = true;
        if (state)
            set_mode_group_mute();              /* m_mode_group = true */
        else
            unset_mode_group_mute();
        break;

    case c_midi_control_mod_glearn:

        result = true;
        if (state)
            set_mode_group_learn();
        else
            unset_mode_group_learn();
        break;

    case c_midi_control_play_ss:

        set_playing_screenset();
        result = true;
        break;

    default:

        /*
         * Based on the value of c_midi_track_crl (32 * 2) versus
         * m_seqs_in_set (32), maybe the first comparison should be
         * "ctl >= 2 * m_seqs_in_set".
         *
         * TODO: This can now vary, so we need to re-evaluate here!
         *
         * if ((ctl >= m_seqs_in_set) && (ctl < c_midi_track_ctrl))
         */

        if ((ctl >= c_max_sets) && (ctl < c_midi_track_ctrl))
            select_and_mute_group(ctl - m_seqs_in_set);

        result = true;
        break;
    }
    return result;
}

/**
 *  Provides operation of the new MIDI controls.
 *
 * \param ctl
 *      The MIDI control value to use to perform an operation.
 *
 * \param a
 *      The action of the control.
 *
 * \param v
 *      The value of the control (ie.: note velocity / control change value).
 *      Also called a "parameter" in the comments.
 *
 * \return
 *      Returns true if the control was an extended control and was acted on.
 */

bool
perform::handle_midi_control_ex (int ctl, midi_control::action a, int v)
{
    bool result = false;

    /*
     * TMI: printf("ctl %d, action %d, value %d\n", ctl, int(a), v);
     */

    switch (ctl)
    {
    case c_midi_control_playback:

        if (a == midi_control::action_toggle)
        {
            pause_key();
            result = true;
        }
        else if (a == midi_control::action_on)
        {
            start_key();
            result = true;
        }
        else if (a == midi_control::action_off)
        {
            stop_key();
            result = true;
        }
        break;

    case c_midi_control_song_record:                /* arm for recording */

#ifdef SEQ64_SONG_RECORDING
        if (a == midi_control::action_toggle)
        {
            song_recording(! song_recording());
            result = true;
        }
        else if (a == midi_control::action_on)
        {
            song_recording(true);
            result = true;
        }
        else if (a == midi_control::action_off)
        {
            song_recording(false);
            result = true;
        }
#endif  // SEQ64_SONG_RECORDING
        break;

    case c_midi_control_solo:

        if (a == midi_control::action_toggle)
        {
            // TODO
            result = true;
        }
        else if (a == midi_control::action_on)
        {
            // TODO
            result = true;
        }
        else if (a == midi_control::action_off)
        {
            // TODO
            result = true;
        }
        break;

    case c_midi_control_thru:

        if (a == midi_control::action_toggle)
        {
            set_thru(false, v, true);           /* toggles */
            result = true;
        }
        else if (a == midi_control::action_on)
        {
            set_thru(true, v);
            result = true;
        }
        else if (a == midi_control::action_off)
        {
            set_thru(false, v);
            result = true;
        }
        break;

    case c_midi_control_bpm_page_up:

        /*
         * TODO:  Handle inversion
         */

        if (a == midi_control::action_on)
        {
            (void) page_increment_beats_per_minute();
            result = true;
        }
        break;

    case c_midi_control_bpm_page_dn:

        /*
         * TODO:  Handle inversion
         */

        if (a == midi_control::action_on)
        {
            (void) page_decrement_beats_per_minute();
            result = true;
        }
        break;

    case c_midi_control_ss_set:

        set_screenset(v);
        result = true;
        break;

    case c_midi_control_record:

        if (a == midi_control::action_toggle)
        {
            set_recording(false, v, true);                  /* toggles */
            result = true;
        }
        else if (a == midi_control::action_on)
        {
            set_recording(true, v);
            result = true;
        }
        else if (a == midi_control::action_off)
        {
            set_recording(false, v);
            result = true;
        }
        break;

    case c_midi_control_quan_record:

        if (a == midi_control::action_toggle)
        {
            set_quantized_recording(false, v, true);        /* toggles */
            result = true;
        }
        else if (a == midi_control::action_on)
        {
            set_quantized_recording(true, v);
            result = true;
        }
        else if (a == midi_control::action_off)
        {
            set_quantized_recording(false, v);
            result = true;
        }
        break;

    /*
     * Based on jfrey-xx's pull request #150, this uses the last available
     * control for MIDI control of the overwrite versus reset recording
     * functionality.
     */

    case c_midi_control_reset_seq:

        if (a == midi_control::action_toggle)
        {
            overwrite_recording(false, v, true);        /* toggles */
            result = true;
        }
        else if (a == midi_control::action_on)
        {
            overwrite_recording(true, v);
            result = true;
        }
        else if (a == midi_control::action_off)
        {
            overwrite_recording(false, v);
            result = true;
        }
        break;

    default:

        break;
    }
    return result;
}

/**
 *  Checks the event to see if it is a c_midi_control_record event, and
 *  performs the requested action (toggle, on, off) if so.  This function is
 *  used for a quick check while recording, so we don't have scan 84 items
 *  before adding a musical MIDI event, but still can detect a
 *  recording-status change command.
 *
 *  We handle record, but also need to handle quan_record and thru.
 *  midi_control_event() iterates through all values.  We need to "iterate"
 *  between the record, quan_record, and thru values only.
 *
 * \param ev
 *      Provides the MIDI event to potentially trigger a recording-control
 *      action.
 *
 * \return
 *      Returns true if the event was an active recording control event.
 */

bool
perform::midi_control_record (const event & ev)
{
    bool result = handle_midi_control_event(ev, c_midi_control_record);
    if (! result)
        result = handle_midi_control_event(ev, c_midi_control_thru);

    if (! result)
        result = handle_midi_control_event(ev, c_midi_control_quan_record);

    return result;
}

/**
 *  Encapsulates code used by seqedit::record_change_callback().
 *
 * \param record_active
 *      Provides the current status of the Record button.
 *
 * \param thru_active
 *      Provides the current status of the Thru button.
 *
 * \param s
 *      The sequence that the seqedit window represents.  This pointer is
 *      checked.
 */

void
perform::set_recording (bool record_active, bool thru_active, sequence * s)
{
    if (not_nullptr(s))
    {
        if (! thru_active)
            set_sequence_input(record_active, s);

        s->set_recording(record_active);
    }
}

/**
 *  Encapsulates code used by seqedit::record_change_callback().  However,
 *  this function depends on the sequence, not the seqedit, for obtaining
 *  the thru status.
 *
 * \param record_active
 *      Provides the current status of the Record button.
 *
 * \param seq
 *      The sequence number; the resulting pointer is checked.
 *
 * \param toggle
 *      If true, ignore the first flag and let the sequence toggle its
 *      setting.  Passed along to sequence::set_input_recording().
 */

void
perform::set_recording (bool record_active, int seq, bool toggle)
{
    sequence * s = get_sequence(seq);
    if (not_nullptr(s))
        s->set_input_recording(record_active, toggle);
}

/**
 *  Sets quantized recording in the way used by seqedit.
 *
 * \param record_active
 *      The setting desired for the quantized-recording flag.
 *
 * \param s
 *      Provides the pointer to the sequence to operate upon.  Checked for
 *      validity.
 */

void
perform::set_quantized_recording (bool record_active, sequence * s)
{
    if (not_nullptr(s))
        s->set_quantized_recording(record_active);
}

/**
 *  Sets quantized recording.  This isn't quite consistent with setting
 *  regular recording, which uses sequence::set_input_recording().
 *
 * \param record_active
 *      Provides the current status of the Record button.
 *
 * \param seq
 *      The sequence number; the resulting pointer is checked.
 *
 * \param toggle
 *      If true, ignore the first flag and let the sequence toggle its
 *      setting.  Passed along to sequence::set_input_recording().
 */

void
perform::set_quantized_recording (bool record_active, int seq, bool toggle)
{
    sequence * s = get_sequence(seq);
    if (not_nullptr(s))
    {
        if (toggle)
            s->set_quantized_recording(! s->get_quantized_rec());
        else
            s->set_quantized_recording(record_active);
    }
}


/**
 *  Set recording for overwrite.
 *
 * \todo
 *      Might probably as well
 *      create(bool rec_active, bool thru_active, sequence * s).
 *
 * Pull request #150:
 *
 *      Ask for a reset explicitly upon toggle-on, since we don't have the GUI
 *      to control for progress.
 *
 * \param oactive
 *      Provides the current status of the overwrite mode.
 *
 * \param seq
 *      The sequence number; the resulting pointer is checked.
 *
 * \param toggle
 *      If true, ignore the first flag and let the sequence toggle its
 *      setting.  Passed along to sequence::set_overwrite_rec().
 */

void
perform::overwrite_recording (bool oactive, int seq, bool toggle)
{
    sequence * s = get_sequence(seq);
    if (not_nullptr(s))
    {
        if (toggle)
            oactive = ! s->overwrite_recording();

        /*
         * On overwrite, the sequence will reset no matter what is here.
         */

        if (oactive)
            s->loop_reset(true);

        s->overwrite_recording(oactive);
    }
}

/**
 *  Encapsulates code used by seqedit::thru_change_callback().
 *
 * \param record_active
 *      Provides the current status of the Record button.
 *
 * \param thru_active
 *      Provides the current status of the Thru button.
 *
 * \param s
 *      The sequence that the seqedit window represents.  This pointer is
 *      checked.
 */

void
perform::set_thru (bool record_active, bool thru_active, sequence * s)
{
    if (not_nullptr(s))
    {
        if (! record_active)
            set_sequence_input(thru_active, s);

        s->set_thru(thru_active);
    }
}

/**
 *  Encapsulates code used by seqedit::thru_change_callback().  However, this
 *  function depends on the sequence, not the seqedit, for obtaining the
 *  recording status.
 *
 * \param thru_active
 *      Provides the current status of the Thru button.
 *
 * \param seq
 *      The sequence number; the resulting pointer is checked.
 *
 * \param toggle
 *      If true, ignore the first flag and let the sequence toggle its
 *      setting.  Passed along to sequence::set_input_thru().
 */

void
perform::set_thru (bool thru_active, int seq, bool toggle)
{
    sequence * s = get_sequence(seq);
    if (not_nullptr(s))
        s->set_input_thru(thru_active, toggle);
}

/**
 *  This function encapsulates code in input_func() to make it easier to read
 *  and understand.
 *
 *  Here is the processing involved in this function .... TODO.
 *
 *  Incorporates pull request #24, arnaud-jacquemin, issue #23 "MIDI
 *  controller toggles wrong pattern".
 *
 *  Note that the event::get_status() function returns a value with the
 *  channel nybble stripped off.
 *
 * \param ev
 *      Provides the MIDI event to potentially trigger a control action.
 *
 * \return
 *      Returns true if the event was actually handled.  TODO.
 */

bool
perform::midi_control_event (const event & ev)
{
    bool result = false;
    int offset = m_screenset_offset;
    for (int ctl = 0; ctl < g_midi_control_limit; ++ctl, ++offset)  /* 84 */
    {
        result = handle_midi_control_event(ev, ctl, offset);
        if (result)
            break;      /* differs from legacy behavior, which keeps going */
    }
    return result;
}

/**
 *  Code extracted from midi_control_event() to be re-used for handling
 *  shorter lists of events.
 *
 * \param ev
 *      Provides the MIDI event to potentially trigger a control action.
 *
 * \param ctl
 *      The specific MIDI control to check.
 *
 * \param offset
 *      The offset into the control list, used only for changing the playing
 *      status of a sequence/pattern in the current screen-set.
 *
 * \return
 *      Returns true if the event was matched and handled.
 */

bool
perform::handle_midi_control_event (const event & ev, int ctl, int offset)
{
    bool result = false;
    bool is_a_sequence = ctl < m_seqs_in_set;
    bool is_ext = ctl >= c_midi_controls && ctl < c_midi_controls_extended;
    midibyte status = ev.get_status();
    midibyte d0 = 0, d1 = 0;                    /* do we need to zero them? */
    ev.get_data(d0, d1);
    if (midi_control_toggle(ctl).match(status, d0))
    {
        if (midi_control_toggle(ctl).in_range(d1))
        {
            if (is_a_sequence)
            {
                sequence_playing_toggle(offset);
                result = true;
            }
            else if (is_ext)
            {
                result = handle_midi_control_ex
                (
                    ctl, midi_control::action_toggle, d1
                );
            }
        }
    }
    if (midi_control_on(ctl).match(status, d0))
    {
        if (midi_control_on(ctl).in_range(d1))
        {
            if (is_a_sequence)
            {
                sequence_playing_on(offset);
                result = true;
            }
            else if (is_ext)
            {
                result = handle_midi_control_ex
                (
                    ctl, midi_control::action_on, d1
                );
            }
            else
                result = handle_midi_control(ctl, true);
        }
        else if (midi_control_on(ctl).inverse_active())
        {
            if (is_a_sequence)
            {
                sequence_playing_off(offset);
                result = true;
            }
            else if (is_ext)
            {
                result = handle_midi_control_ex
                (
                    ctl, midi_control::action_off, d1
                );
            }
            else
                result = handle_midi_control(ctl, false);
        }
    }
    if (midi_control_off(ctl).match(status, d0))
    {
        if (midi_control_off(ctl).in_range(d1))  /* Issue #35 */
        {
            if (is_a_sequence)
            {
                sequence_playing_off(offset);
                result = true;
            }
            else if (is_ext)
            {
                result = handle_midi_control_ex
                (
                    ctl, midi_control::action_off, d1
                );
            }
            else
                result = handle_midi_control(ctl, false);
        }
        else if (midi_control_off(ctl).inverse_active())
        {
            if (is_a_sequence)
            {
                sequence_playing_on(offset);
                result = true;
            }
            else if (is_ext)
            {
                result = handle_midi_control_ex
                (
                    ctl, midi_control::action_on, d1
                );
            }
            else
                result = handle_midi_control(ctl, true);
        }
    }
    return result;
}

/**
 *  This function is called by input_thread_func().  It handles certain MIDI
 *  input events.
 *
 * Stazed:
 *
 *      http://www.blitter.com/~russtopia/MIDI/~jglatt/tech/midispec/ssp.htm
 *
 *      Example: If a Song Position value of 8 is received, then a sequencer
 *      (or drum box) should cue playback to the third quarter note of the
 *      song.  (8 MIDI beats * 6 MIDI clocks per MIDI beat = 48 MIDI Clocks.
 *      Since there are 24 MIDI Clocks in a quarter note, the first quarter
 *      occurs on a time of 0 MIDI Clocks, the second quarter note occurs upon
 *      the 24th MIDI Clock, and the third quarter note occurs on the 48th
 *      MIDI Clock).
 *
 *      8 MIDI beats * 6 MIDI clocks per MIDI beat = 48 MIDI Clocks.
 *
 * EVENT_MIDI_START:
 *
 *      Starts the MIDI Time Clock.  Kepler34 does "stop();
 *      set_playback_mode(false); start();" in its version of this event.
 *      This sets the playback mode to Live mode. This behavior seems
 *      reasonable, though function names Sequencer64 uses are different.
 *
 * EVENT_MIDI_CONTINUE:
 *
 *      MIDI continue: start from current position.  This is sent immediately
 *      after EVENT_MIDI_SONG_POS, and is used for starting from other than
 *      beginning of the song, or for starting from previous location at
 *      EVENT_MIDI_STOP. Again, converted to Kepler34 mode of setting the
 *      playback mode to Live mode.
 *
 * EVENT_MIDI_STOP:
 *
 *      Do nothing, just let the system pause.  Since we're not getting ticks
 *      after the stop, the song won't advance when start is received, we'll
 *      reset the position. Or, when continue is received, we won't reset the
 *      position.  We do an inner_stop(); the m_midiclockpos member holds the
 *      stop position in case the next event is "continue".  This feature is
 *      not in Kepler34.
 *
 * EVENT_MIDI_CLOCK:
 *
 *      MIDI beat clock (MIDI timing clock or simply MIDI clock) is a clock
 *      signal broadcast via MIDI to ensure that MIDI-enabled devices stay in
 *      synchronization. It is not MIDI timecode.  Unlike MIDI timecode, MIDI
 *      beat clock is tempo-dependent. Clock events are sent at a rate of 24
 *      ppqn (pulses per quarter note). Those pulses maintain a synchronized
 *      tempo for synthesizers that have BPM-dependent voices and for
 *      arpeggiator synchronization.  Location information can be specified
 *      using MIDI Song Position Pointer.  Many simple MIDI devices ignore
 *      this message.
 *
 * EVENT_MIDI_SONG_POS:
 *
 *      MIDI song position pointer message tells a MIDI device to cue to a
 *      point in the MIDI sequence to be ready to play.  This message consists
 *      of three bytes of data. The first byte, the status byte, is 0xF2 to
 *      flag a song position pointer message. Two bytes follow the status
 *      byte.  These two bytes are combined in a 14-bit value to show the
 *      position in the song to cue to. The top bit of each of the two bytes
 *      is not used.  Thus, the value of the position to cue to is between
 *      0x0000 and 0x3FFF. The position represents the MIDI beat, where a
 *      sequence always starts on beat zero and each beat is a 16th note.
 *      Thus, the device will cue to a specific 16th note.  Also see the
 *      combine_bytes() function.
 *
 * EVENT_MIDI_SYSEX:
 *
 *      These messages are system-wide messages.  We filter system-wide
 *      messages.  If the master MIDI buss is dumping, set the timestamp of
 *      the event and stream it on the sequence.  Otherwise, use the event
 *      data to control the sequencer, if it is valid for that action.
 *
 *      "Dumping" is set when a seqedit window is open and the user has
 *      clicked the "record MIDI" or "thru MIDI" button.  In this case, if the
 *      seq32 support is in force, dump to it, else stream the event, with
 *      possibly multiple sequences set.  Otherwise, handle an incoming MIDI
 *      control event.
 *
 *      Also available (but macroed out) is Stazed's parse_sysex() function.
 *      It seems specific to certain Yamaha devices, but might prove useful
 *      later.
 *
 *  For events less than or equal to SysEx, we call midi_control_event()
 *  to handle the MIDI controls that Sequencer64 supports.  (These are
 *  configurable in the "rc" configuration file.)
 */

void
perform::input_func ()
{
    event ev;
    while (m_inputing)              /* perhaps we should lock this variable */
    {
        if (m_master_bus->poll_for_midi() > 0)
        {
            do
            {
                if (m_master_bus->get_midi_event(&ev))
                {
                    /*
                     * Used when starting from the beginning of the song.
                     * Obey the MIDI time clock.  Comments moved to the
                     * banner.
                     */

                    if (ev.get_status() == EVENT_MIDI_START)
                    {
                        stop();                             // Kepler34
                        song_start_mode(false);             // Kepler34
                        start(song_start_mode());
                        m_midiclockrunning = m_usemidiclock = true;
                        m_midiclocktick = m_midiclockpos = 0;
                    }
                    else if (ev.get_status() == EVENT_MIDI_CONTINUE)
                    {
                        m_midiclockrunning = true;
                        song_start_mode(false);             // Kepler34
                        start(song_start_mode());
                    }
                    else if (ev.get_status() == EVENT_MIDI_STOP)
                    {
                        m_midiclockrunning = false;
                        all_notes_off();
                        inner_stop(true);
                        m_midiclockpos = get_tick();
                    }
                    else if (ev.get_status() == EVENT_MIDI_CLOCK)
                    {
                        if (m_midiclockrunning)
                            m_midiclocktick += SEQ64_MIDI_CLOCK_INCREMENT;  // 8
                    }
                    else if (ev.get_status() == EVENT_MIDI_SONG_POS)
                    {
                        midibyte d0, d1;                // see note in banner
                        ev.get_data(d0, d1);
                        m_midiclockpos = combine_bytes(d0, d1);
                    }
#if 0               // currently filtered in midi_jack
                    else if
                    (
                        ev.get_status() == EVENT_MIDI_ACTIVE_SENSE ||
                        ev.get_status() == EVENT_MIDI_RESET
                    )
                    {
                        /*
                         * For now, we ignore these events on input. See
                         * GitHub sequencer64-packages/issues/4.  MIGHT NOT BE
                         * A VALID FIX.  STILL INVESTIGATING.
                         */

                        return;
                    }
#endif

                    /*
                     * Send out the current event, if "dumping".
                     */

                    if (ev.get_status() <= EVENT_MIDI_SYSEX)
                    {
                        /*
                         * Test for MIDI control events even if "dumping".
                         * Otherwise, we cannot handle any more control events
                         * once recording is turned on.  WARNING:  This can
                         * slow down recording, so we check only for recording
                         * status now.
                         */

                        if (m_master_bus->is_dumping())
                        {
                            /*
                             * Check for all events, not just record-control,
                             * to prevent unwanted recordings. Issue #150?
                             *
                             * if (! midi_control_record())
                             */

                            if (! midi_control_event(ev))
                            {
                                ev.set_timestamp(get_tick());
                                if (rc().show_midi())
                                    ev.print();

                                if (m_filter_by_channel)
                                    m_master_bus->dump_midi_input(ev);
                                else
                                    m_master_bus->get_sequence()->stream_event(ev);
                            }
                        }
                        else
                        {
                            if (rc().show_midi())
                                ev.print();

                            (void) midi_control_event(ev);
                        }

#ifdef USE_STAZED_PARSE_SYSEX               // more code to incorporate!!!
                        if (global_use_sysex)
                        {
                            if (FF_RW_button_type != FF_RW_RELEASE)
                            {
                                if (ev.is_note_off())
                                {
                                    /*
                                     * Notes 91 G5 & 96 C6 on YPT = FF/RW keys
                                     */

                                    midibyte n = ev.get_note();
                                    if (n == 91 || n == 96)
                                        FF_RW_button_type = FF_RW_RELEASE;
                                }
                            }
                        }
#endif
                    }
                    if (ev.get_status() == EVENT_MIDI_SYSEX)
                    {
#ifdef USE_STAZED_PARSE_SYSEX               // more code to incorporate!!!
                        if (global_use_sysex)
                            parse_sysex(ev);
#endif
                        if (rc().show_midi())
                            ev.print();

                        if (rc().pass_sysex())
                            m_master_bus->sysex(&ev);
                    }
                }
            } while (m_master_bus->is_more_input());
        }
    }
    pthread_exit(0);
}

/**
 *  Combines bytes into an unsigned-short value.
 *
 *  http://www.blitter.com/~russtopia/MIDI/~jglatt/tech/midispec/wheel.htm
 *
 *  Two data bytes follow the status. The two bytes should be combined
 *  together to form a 14-bit value. The first data byte's bits 0 to 6 are
 *  bits 0 to 6 of the 14-bit value. The second data byte's bits 0 to 6 are
 *  really bits 7 to 13 of the 14-bit value. In other words, assuming that a
 *  C program has the first byte in the variable First and the second data
 *  byte in the variable Second, here's how to combine them into a 14-bit
 *  value (actually 16-bit since most computer CPUs deal with 16-bit, not
 *  14-bit, integers).
 *
 *  I think Kepler64 got the bytes backward.
 *
 * \param b0
 *      The first byte to be combined.
 *
 * \param b1
 *      The second byte to be combined.
 *
 * \return
 *      Returns the bytes basically OR'd together.
 */

unsigned short
perform::combine_bytes (midibyte b0, midibyte b1)
{
   unsigned short short_14bit = (unsigned short)(b1);
   short_14bit <<= 7;
   short_14bit |= (unsigned short)(b0);
   return short_14bit * 48;
}

#ifdef USE_STAZED_PARSE_SYSEX               // more code to incorporate!!!

/**
 *  Too hardware-specific.
 */

enum sysex_YPT
{
    SYS_YPT300_START,
    SYS_YPT300_STOP,
    SYS_YPT300_TOP,             //  Beginning of song
    SYS_YPT300_FAST_FORWARD,
    SYS_YPT300_REWIND,
    SYS_YPT300_METRONOME        //  or anything else
};

/**
 *  http://www.indiana.edu/~emusic/etext/MIDI/chapter3_MIDI9.shtml
 *
 *  A System Exclusive code set begins with 11110000 (240 decimal or F0 hex),
 *  followed by the manufacturer ID#, then by an unspecified number of data
 *  bytes of any ranges from 0-127), and then ends with 11110111 (decimal 247
 *  or F7 hex), meaning End of SysEx message.
 *
 *  No other codes are to be transmitted during a SysEx message (except a
 *  system real time message). Normally, after the manufacturer ID, each maker
 *  will have its own instrument model subcode, so a Yamaha DX7 will ignore a
 *  Yamaha SY77's patch dump. In addition, most instruments have a SysEx ID #
 *  setting so more than one of the same instruments can be on a network but
 *  not necessarily respond to a patch dump not intended for it.
 *
 * Layout of YPT-300 sysex messages (EVENT_SYSEX):
 *
 *      -   Byte 0          0xF0
 *      -   Byte 1          0x43        Yamaha_ID
 *      -   Bytes 2-5       0x73015015  YPT_model_subcode
 *      -   Byte 6          0x00
 *      -   Byte 7          0xnn        Message we look for, enum 0 to 5
 *      -   Byte 8          0x00
 *      -   Byte 9          0xF7
 */

void
perform::parse_sysex (event a_e)            // copy, or reference???
{
    const unsigned char c_yamaha_ID         = 0x43;         // byte 1
    const unsigned long c_YPT_model_subcode = 0x73015015;   // bytes 2 - 5
    unsigned char * data = a_e.get_sysex();
    long data_size = a_e.get_size();
    if (data_size < 10)                     // sanity check
        return;

    if (data[1] != c_yamaha_ID) /* check manufacturer ID, could use others. */
        return;

    unsigned long subcode = 0;              /* Check the model subcode */
    subcode += (data[2] << 24);
    subcode += (data[3] << 16);
    subcode += (data[4] << 8);
    subcode += (data[5]);
    if (subcode != c_YPT_model_subcode)
        return;

    switch(data[7])                         /* We are good to go */
    {
    case SYS_YPT300_START:
        m_start_from_perfedit = true;       // assume song mode start
        start_playing();
        break;

    case SYS_YPT300_STOP:
        set_reposition(true);               // allow to continue where stopped
        stop_playing();
        break;

    case SYS_YPT300_TOP:                    // beginning of song or left marker
        if (is_jack_running())
        {
            set_reposition();
            set_starting_tick(m_left_tick);
            position_jack(true, m_left_tick);
        }
        else
        {
            set_reposition();
            set_starting_tick(m_left_tick);
        }
        break;

    case SYS_YPT300_FAST_FORWARD:
        FF_RW_button_type = FF_RW_FORWARD;
        gtk_timeout_add(120, FF_RW_timeout,this);
        break;

    case SYS_YPT300_REWIND:
        FF_RW_button_type = FF_RW_REWIND;
        gtk_timeout_add(120, FF_RW_timeout,this);
        break;

    default:
        break;
    }
}

#endif  // USE_STAZED_PARSE_SYSEX

/**
 *  For all active patterns/sequences, this function gets the playing
 *  status and saves it in m_sequence_state[i].  Inactive patterns get the
 *  value set to false.  Used in unsetting the snapshot status
 *  (c_status_snapshot).
 */

void
perform::save_playing_state ()
{
    for (int s = 0; s < m_sequence_high; ++s)       /* modest speed-up */
    {
        if (is_active(s))
            m_sequence_state[s] = m_seqs[s]->get_playing();
        else
            m_sequence_state[s] = false;
    }
}

/**
 *  For all active patterns/sequences, this function gets the playing
 *  status from m_sequence_state[i] and sets it for the sequence.  Used in
 *  unsetting the snapshot status (c_status_snapshot).
 */

void
perform::restore_playing_state ()
{
    for (int s = 0; s < m_sequence_high; ++s)       /* modest speed-up */
    {
        if (is_active(s))
            m_seqs[s]->set_playing(m_sequence_state[s]);
    }
}

/**
 *  For all active patterns/sequences in the current (playing) screen-set,
 *  this function gets the playing status and saves it in m_sequence_state[i].
 *  Inactive patterns get the value set to false.  Used in saving the
 *  screen-set state during the queued-replace (queued-sol) operation, which
 *  occurs when the c_status_replace is performed while c_status_queue is
 *  active.
 *
 * \param repseq
 *      Provides the number of the pattern for which the replace functionality
 *      is invoked.  This pattern will set to "playing" whether it is on or
 *      off, so that it can stay active while toggling between "solo" and
 *      "playing with the rest of the patterns".
 */

void
perform::save_current_screenset (int repseq)
{
    for (int s = 0; s < m_seqs_in_set; ++s)
    {
        int source = m_screenset_offset + s;
        if (is_active(source))
        {
            bool on = m_seqs[source]->get_playing() || (source == repseq);
            m_screenset_state[s] = on;
        }
        else
            m_screenset_state[s] = false;
    }
}

/**
 *  Clears the m_screenset_state[] array.  Needed when disabling the queue
 *  mode.
 */

void
perform::clear_current_screenset ()
{
    for (int s = 0; s < m_seqs_in_set; ++s)
        m_screenset_state[s] = false;
}

/**
 *  If the given status is present in the c_status_snapshot, the playing state
 *  is saved.  Then the given status is OR'd into the m_control_status.
 *
 * \param status
 *      The status to be used.
 */

void
perform::set_sequence_control_status (int status)
{
    if (status & c_status_snapshot)
        save_playing_state();

    m_control_status |= status;
}

/**
 *  If the given status is present in the c_status_snapshot, the playing state
 *  is restored.  Then the given status is reversed in m_control_status.
 *
 *  If the given status includes c_status_queue, this is a signal to stop
 *  queuing (which is already in place elsewhere).  It also unsets the new
 *  queue-replace feature.
 *
 * \param status
 *      The status to be used.
 */

void
perform::unset_sequence_control_status (int status)
{
    if (status & c_status_snapshot)
        restore_playing_state();

    if (status & c_status_queue)
        unset_queued_replace();

    m_control_status &= ~status;
}

/**
 *  Helper function that clears the queued-replace feature.  This also clears
 *  the queue mode; we shall see if this disrupts any user's workflow.
 *
 * \param clearbits
 *      If true (the default), then clear the queue and replace status bits.
 *      If the user is simply replacing the current replace pattern with
 *      another pattern, we pass false for this parameter.
 */

void
perform::unset_queued_replace (bool clearbits)
{
    if (m_queued_replace_slot != SEQ64_NO_QUEUED_SOLO)
    {
        m_queued_replace_slot = SEQ64_NO_QUEUED_SOLO;
        clear_current_screenset();
        if (clearbits)
            m_control_status &= ~(c_status_queue|c_status_replace);
    }
}

/**
 *  If the given sequence is active, then it is toggled as per the current
 *  value of m_control_status.  If m_control_status is c_status_queue, then
 *  the sequence's toggle_queued() function is called.  This is the "mod
 *  queue" implementation.
 *
 *  Otherwise, if it is c_status_replace, then the status is unset, and all
 *  sequences are turned off.  Then the sequence's toggle-playing() function
 *  is called, which should turn it back on.  This is the "mod replace"
 *  implementation; it is like a Solo.  But can it be undone?
 *
 *  This function is called in sequence_key() to implement a toggling of the
 *  sequence of the pattern slot in the current screen-set that is represented
 *  by the keystroke.
 *
 *  This function is also called in midi_control_event() if the control number
 *  represents a sequence number in a screen-set, that is, it ranges from 0 to
 *  31.  This value should be offset by the current screen-set number,
 *  m_screenset_offset, before passing it to this function.
 *
 *  This function now also supports the new queued-replace (queued-solo)
 *  feature.
 *
 * \param seq
 *      The sequence number of the sequence to be potentially toggled.
 *      This value must be a valid and active sequence number. If in
 *      queued-replace mode, and if this pattern number is different from the
 *      currently-stored number (m_queued_replace_slot), then we clear the
 *      currently stored set of patterns and set new stored patterns.
 */

void
perform::sequence_playing_toggle (int seq)
{
    sequence * s = get_sequence(seq);
    if (not_nullptr(s))                     // if (is_active(seq))
    {
        bool is_queue = (m_control_status & c_status_queue) != 0;
        bool is_replace = (m_control_status & c_status_replace) != 0;

#ifdef SEQ64_SONG_RECORDING                 // enables one-shot as well

        /*
         * One-shots are allowed only if we are not playing this sequence.
         */

        bool is_oneshot = (m_control_status & c_status_oneshot) != 0;
        if (is_oneshot && ! s->get_playing())
        {
            s->toggle_one_shot();           // why not just turn on???
        }
        else

#endif

        if (is_queue && is_replace)
        {
            if (m_queued_replace_slot != SEQ64_NO_QUEUED_SOLO)
            {
                if (seq != m_queued_replace_slot)
                {
                    unset_queued_replace(false);    /* do not clear bits    */
                    save_current_screenset(seq);
                }
            }
            else
                save_current_screenset(seq);

            unqueue_sequences(seq);
            m_queued_replace_slot = seq;
        }
        else if (is_queue)
        {
            s->toggle_queued();
        }
        else
        {
            if (is_replace)
            {
                unset_sequence_control_status(c_status_replace);
                off_sequences();
            }
            s->toggle_playing();
        }

#ifdef SEQ64_SONG_RECORDING

        /*
         * If we're in song playback, temporarily block the events until the
         * next sequence boundary. And if we're recording, add "Live" sequence
         * playback changes to the Song/Performance data as triggers.
         *
         * \todo
         *      Would be nice to delay song-recording start to the next queue,
         *      if queuing is active for this sequence.
         */

        if (m_playback_mode)
            s->song_playback_block(true);

        if (song_recording())
        {
            midipulse seq_length = s->get_length();
            midipulse tick = get_tick();
            bool trigger_state = s->get_trigger_state(tick);
            if (trigger_state)              /* if sequence already playing  */
            {
                /*
                 * If this play is us recording live, end the new trigger
                 * block here.
                 */

                if (s->song_recording())
                {
                    s->song_recording_stop(tick);
                }
                else        /* ...else need to trim block already in place  */
                {
                    s->exact_split_trigger(tick);
                    s->delete_trigger(tick);
                }
            }
            else            /* if not playing, start recording a new strip  */
            {
                if (m_song_record_snap)     /* snap to length of sequence   */
                    tick -= tick % seq_length;

                push_trigger_undo();
                s->song_recording_start(tick, m_song_record_snap);
            }
        }
#endif  // SEQ64_SONG_RECORDING

    }
}

/**
 *  A helper function for determining if the mode group is in force, the
 *  playing screenset is the same as the current screenset, and the sequence
 *  is in the range of the playing screenset.
 *
 * \param seq
 *      Provides the index of the desired sequence.
 *
 * \return
 *      Returns true if the sequence adheres to the conditions noted above.
 */

bool
perform::seq_in_playing_screen (int seq)
{
    int next_offset = m_playscreen_offset + m_seqs_in_set;
    return
    (
        m_mode_group && (m_playscreen == m_screenset) &&
        (seq >= m_playscreen_offset) && (seq < next_offset)
    );
}

/**
 *  Turn the playing of a sequence on or off.  Used for the implementation of
 *  sequence_playing_on() and sequence_playing_off().
 *
 *  Kepler34's version seems slightly different, may need more study.
 *
 * \param seq
 *      The number of the sequence to be turned off.
 *
 * \param on
 *      True if the sequence is to be turned on, false if it is to be turned
 *      off.
 */

void
perform::sequence_playing_change (int seq, bool on)
{
    sequence * s = get_sequence(seq);
    if (not_nullptr(s))
    {
        if (seq_in_playing_screen(seq))
            m_tracks_mute_state[seq - m_playscreen_offset] = on;

        bool queued = s->get_queued();
        bool playing = s->get_playing();
        bool q_in_progress = (m_control_status & c_status_queue) != 0;
        if (on)
            playing = ! playing;

        if (playing)
        {
            if (q_in_progress)
            {
                if (! queued)
                    s->toggle_queued();
            }
            else
                s->set_playing(on);
        }
        else
        {
            if (queued && q_in_progress)
                s->toggle_queued();
        }
    }
}

/*
 * Non-inline encapsulation functions start here.
 */

/**
 *  Sets or unsets the keep-queue functionality, to be used by the new "Q"
 *  button in the main window.
 */

void
perform::set_keep_queue (bool activate)
{
    if (activate)
        set_sequence_control_status(c_status_queue);
    else
        unset_sequence_control_status(c_status_queue);
}

/**
 *  Returns true if the c_status_queue bit is set.
 */

bool
perform::is_keep_queue () const
{
    return (m_control_status & c_status_queue) != 0;
}

/**
 *  Handle a sequence key to toggle the playing of an active pattern in
 *  the selected screen-set.  This function is use in mainwnd when toggling
 *  the mute/unmute setting using keyboard keys.
 *
 * \param seq
 *      The sequence's control-key number, which is relative to the current
 *      screen-set.
 */

void
perform::sequence_key (int seq)
{
    seq += screenset_offset(m_screenset);   /* m_playscreen_offset !!!  */
    if (is_active(seq))
        sequence_playing_toggle(seq);
}

/**
 *  Provides a way to format the sequence parameters string for display in the
 *  mainwid or perfnames modules.  This string goes on the bottom-left of
 *  those user-interface elements.
 *
 *  The format of this string is something like the following example,
 *  depending on the "show sequence numbers" option.  The values shown are, in
 *  this order, sequence number (if allowed), buss number, channel number,
 *  beats per bar, and beat width.
 *
\verbatim
        No sequence number:     31-16 4/4
        Sequence number:        9  31-16 4/4
\endverbatim
 *
 *  The sequence number and buss number are re 0, while the channel number is
 *  displayed re 1, unless it is an SMF 0 null channel (0xFF), in which case
 *  it is 0.
 *
 *          "%-3d%d-%d %d/%d"  (old)
 *
 * \note
 *      Later, we could add the sequence hot-key to this string, though
 *      showing that is not much use in perfnames.  Also, this function is a
 *      stilted mix of direct access and access through sequence number.
 *
 * \param seq
 *      Provides the reference to the sequence, use for getting the sequence
 *      parameters to be written to the label string.
 *
 * \return
 *      Returns the filled in label if the sequence is active.
 *      Otherwise, an empty string is returned.
 */

std::string
perform::sequence_label (const sequence & seq)
{
    std::string result;
    int sn = seq.number();
    if (is_active(sn))
    {
        char tmp[32];
        bussbyte bus = seq.get_midi_bus();
        int chan = seq.is_smf_0() ? 0 : seq.get_midi_channel() + 1;
        int bpb = int(seq.get_beats_per_bar());
        int bw = int(seq.get_beat_width());
        if (show_ui_sequence_number())                  /* new feature! */
            snprintf(tmp, sizeof tmp, "%-3d %d-%d %d/%d", sn, bus, chan, bpb, bw);
        else
            snprintf(tmp, sizeof tmp, "%d-%d %d/%d", bus, chan, bpb, bw);

        result = std::string(tmp);
    }
    return result;
}

/**
 *  A pass-through to the other sequence_label() function.
 *
 * \param seq
 *      Provides the reference to the sequence, use for getting the sequence
 *      parameters to be written to the label string.
 *
 * \return
 *      Returns the filled in label if the sequence is active.
 *      Otherwise, an empty string is returned.
 */

std::string
perform::sequence_label (int seqnum)
{
    const sequence * s = get_sequence(seqnum);
    return not_nullptr(s) ? sequence_label(*s) : std::string("") ;
}

/**
 *  Creates the sequence title, adjusting it for scaling down.  This title is
 *  used in the slots to show the (possibly shortened) pattern title. Note
 *  that the sequence title will also show the sequence length, in measures,
 *  if the show_ui_sequence_key() option is active.
 *
 * \param seq
 *      Provides the reference to the sequence, use for getting the sequence
 *      parameters to be written to the label string.
 *
 * \return
 *      Returns the filled in label if the sequence is active.
 *      Otherwise, an empty string is returned.
 */

std::string
perform::sequence_title (const sequence & seq)
{
    std::string result;
    int sn = seq.number();
    if (is_active(sn))
    {
        if (usr().window_scaled_down())
        {
            char temp[12];
            snprintf(temp, sizeof temp, "%.11s", seq.title().c_str());
            result = std::string(temp);
        }
        else
        {
            char temp[16];
            snprintf(temp, sizeof temp, "%.14s", seq.title().c_str());
            result = std::string(temp);
        }
    }
    return result;
}

/**
 *  Creates a sequence ("seqedit") window title, a longer version of
 *  sequence_title().
 *
 * \param seq
 *      Provides the reference to the sequence, use for getting the sequence
 *      parameters to be written to the string.
 *
 * \return
 *      Returns the filled in label if the sequence is active.
 *      Otherwise, an incomplete string is returned.
 *
 */

std::string
perform::sequence_window_title (const sequence & seq)
{
	std::string result = SEQ64_APP_NAME;
    int sn = seq.number();
    if (is_active(sn))
    {
        int ppqn = seq.get_ppqn();					/* choose_ppqn(m_ppqn);	*/
        char temp[32];
        snprintf(temp, sizeof temp, " (%d ppqn)", ppqn);
        result += " #";
        result += seq.seq_number();
        result += " \"";
        result += sequence_title(seq);
        result += "\"";
        result += temp;
    }
    else
    {
        result += "[inactive]";
    }
    return result;
}

/**
 *  Creates the main window title.  Unlike the disabled code in
 *  mainwnd::update_window_title(), this code does not (yet) handle
 *  conversions to UTF-8.
 *
 * \return
 *      Returns the filled-in main window title.
 */

std::string
perform::main_window_title (const std::string & file_name)
{
	std::string result = SEQ64_APP_NAME + std::string(" - ");
	std::string itemname = "unnamed";
	int ppqn = choose_ppqn(m_ppqn);
	char temp[32];
	snprintf(temp, sizeof temp, " (%d ppqn) ", ppqn);
    if (file_name.empty())
    {
        if (! rc().filename().empty())
        {
            std::string name = shorten_file_spec(rc().filename(), 56);
#ifdef USE_UTF8_CONVERSION
            itemname = Glib::filename_to_utf8(name);
#else
            itemname = name;
#endif
        }
    }
    else
    {
        itemname = file_name;
    }
	result += itemname + std::string(temp);
    return result;
}

/**
 *  Sets the input bus, and handles the special "key labels on sequence" and
 *  "sequence numbers on sequence" functionality.  This function is called by
 *  options::input_callback().
 *
 *  Note that the mastermidibus::set_input() function passes the setting along
 *  to the input busarray.
 *
 * \tricky
 *      See the bus parameter.  We should provide two separate functions for
 *      this feature, but it is already combined into one input-callback
 *      function with a lot of other functionality in the options module.
 *
 * \param bus
 *      If this value is greater than SEQ64_DEFAULT_BUSS_MAX (32), then it is
 *      treated as a user-interface flag (PERFORM_KEY_LABELS_ON_SEQUENCE or
 *      PERFORM_NUM_LABELS_ON_SEQUENCE) that causes all the sequences to be
 *      dirtied, and thus get redrawn with the new user-interface setting.
 *
 * \param active
 *      Indicates whether the buss or the user-interface feature is active or
 *      inactive.
 */

void
perform::set_input_bus (bussbyte bus, bool active)
{
    if (bus >= SEQ64_DEFAULT_BUSS_MAX)                  /* 32 busses        */
    {
        if (bus == PERFORM_KEY_LABELS_ON_SEQUENCE)
            show_ui_sequence_key(active);
        else if (bus == PERFORM_NUM_LABELS_ON_SEQUENCE)
            show_ui_sequence_number(active);

        for (int seq = 0; seq < m_sequence_high; ++seq) /* m_sequence_max  */
        {
            sequence * s = get_sequence(seq);
            if (not_nullptr(s))
                s->set_dirty();
        }
    }
    else
    {
        if (m_master_bus->set_input(bus, active))
            set_input(bus, active);
    }
}

/**
 *  Sets the clock value, as specified in the Options / MIDI Clocks tab.
 *  Note that the call to mastermidibus::set_clock() also sets the clock in
 *  the output busarray.
 *
 * \param bus
 *      The bus index to be set.
 *
 * \param clocktype
 *      Indicates whether the buss or the user-interface feature is
 *      e_clock_off, e_clock_pos, e_clock_mod, or (new) e_clock_disabled.
 */

void
perform::set_clock_bus (bussbyte bus, clock_e clocktype)
{
    if (m_master_bus->set_clock(bus, clocktype))    /* checks bus index too */
        set_clock(bus, clocktype);
}

/**
 *  Gets the event key for the given sequence.  If we're not in legacy mode,
 *  then we adjust for the screenset, so that screensets greater than 0 can
 *  also show the correct key name, instead of a question mark (or blank).
 *
 *  Legacy seq24 already responds to the toggling of the mute state via the
 *  shortcut keys even if screenset > 0, but it shows the question mark.
 *
 * \todo
 *      In the context of pattern keys, we should replace c_seqs_in_set with a
 *      better-named value; if sets are actually larger than that, due to the
 *      "sets" option, then we simply repeat the pattern here using a modifier
 *      key ["shifted", see lookup_slot_key()]; in other words, we're stuck on
 *      using 32 pattern hot-keys.
 *
 * \param seqnum
 *      The number of the sequence for which to return the event key.
 *
 * \return
 *      Returns the desired key.  If there is no such value, then the
 *      space (' ') character is returned.  It used to be the question mark.
 */

unsigned
perform::lookup_keyevent_key (int seqnum)
{
    unsigned result = unsigned(' ');
    if (! rc().legacy_format())
        seqnum -= m_screenset_offset;

    if (seqnum >= c_max_keys)
        seqnum -= c_max_keys;

    if (seqnum < c_max_keys)
        result = keys().lookup_keyevent_key(seqnum);

    return result;
}

/**
 *  Like lookup_keyevent_key(), but assumes the slot number has already been
 *  correctly calculated.
 *
 * \param slot
 *      The number of the pattern/sequence for which to return the event
 *      key.  This value can range from 0 to c_seqs_in_set - 1 up to
 *      (3 * c_seqs_in_set) - 1, since we can support 32 hotkeys, plus these
 *      hot-keys "shifted" once and twice.  This value is relative to
 *      m_screeset_offset, and then is modded re c_seqs_in_set, so that it
 *      always ranges from 0 to 31.
 *
 * \return
 *      Returns the desired key. This will always work, due to the mod
 *      operation.
 */

unsigned
perform::lookup_slot_key (int slot)
{
    if (slot >= 0 && slot < (3 * c_max_sequence))
    {
        slot %= c_max_keys;                         // c_seqs_in_set;
        return keys().lookup_keyevent_key(slot);
    }
    else
    {
        errprintf("perform::lookup_slot_key(%d) error\n", slot);
        return 0;
    }
}

/**
 *  Provided for mainwnd :: on_key_press_event() and mainwnd ::
 *  on_key_release_event() to call.  This function handles the keys for the
 *  functions of replace, queue, keep-queue, snapshots, toggling mute groups,
 *  group learn, and playing screenset.  For further keystroke processing, see
 *  mainwnd :: on_key_press_event().
 *
 *  Keys not handled here are handled in mainwnd, where GUI elements exist to
 *  manage these items:
 *
 *      -   bpm up & down
 *      -   screenset up & down
 *      -   mute group key
 *      -   mute group learn
 *
 * seq24 handles the following keys in two "on_key" events:
 *
 *  Release:
 *
 *      -   Replace unset
 *      -   Queue unset
 *      -   Snapshot 1 and snapshot 2 unset
 *      -   Group learn unset
 *
 *  Press:
 *
 *      -   BPM dn and BMP up *
 *      -   Replace set
 *      -   Queue and keep-queue set
 *      -   Snapshot 1 and snapshot 2 set
 *      -   Screen-set dn and screen-set up *
 *      -   Set playing screen-set
 *      -   Group on and group off
 *      -   Group learn set
 *      -   Select and mute the group *
 *      -   Start and stop keys *
 *      -   Pattern mute/unmute keys *
 *
 *  Note that the asterisk indicates we handle it elsewhere. Screen-set down
 *  and up are handled in mainwnd by calling decrement_screenset() and
 *  increment_screenset(), and mainwid::set_screenset().  But that latter call
 *  is not made when MIDI control is in force, which might be an ISSUE.
 *
 * \param k
 *      The keystroke object to be handled.
 *
 * \return
 *      Returns true if the key was handled.
 */

bool
perform::mainwnd_key_event (const keystroke & k)
{
    bool result = true;
    unsigned key = k.key();
    if (k.is_press())
    {
        if (! keyboard_group_c_status_press(key))
        {
            if (! keyboard_group_press(key))
            {
                if (key == keys().set_playing_screenset())
                    set_playing_screenset();
                else
                    result = false;
            }
        }
    }
    else
    {
        if (! keyboard_group_c_status_release(key))
        {
            if (! keyboard_group_release(key))
            {
                result = false;
            }
        }
    }
    return result;
}

/**
 *  Still need to work on this one.
 *  See mainwnd.cpp line 3312 or thereabouts.
 */

bool
perform::keyboard_control_press (unsigned key)
{
    bool result = true;
    if (get_key_count(key) != 0)
    {
        // sequence_key(lookup_keyevent_key(kevent));      // kevent == seq???
        int seqnum = lookup_keyevent_seq(key);
        int keynum = seqnum;            // + m_call_seq_shift * c_seqs_in_set;
        sequence_key(keynum);
    }
    else
        result = false;

    return result;
}

/**
 *  Categories of keyboard actions:
 *
 *  -   [xxxxxxxxx]
 *      -   perform::c_status "events".
 *          -   c_status_replace.
 *          -   c_status_queue.
 *          -   c_status_snapshot.
 *          -   c_status_oneshot.
 *          -   Used by:
 *              -   mainwnd::on_key_press_event() [perform::mainwnd_key_event()]
 *      -   perform groups
 *          -   On.
 *          -   Off.
 *          -   Learn.
 *          -   Used by:
 *              -   mainwnd::on_key_press_event() [perform::mainwnd_key_event()]
 *      -   perform::playback_key_event().
 *      -   perform::set_playing_screenset().
 *          -   Start.
 *          -   Stop.
 *          -   Pause.
 *      -   GUI framework specific
 */

bool
perform::keyboard_group_c_status_press (unsigned key)
{
    bool result = true;
    if (key == keys().replace())
        set_sequence_control_status(c_status_replace);
    else if (key == keys().queue() || key == keys().keep_queue())
        set_sequence_control_status(c_status_queue);
    else if (key == keys().snapshot_1() || key == keys().snapshot_2())
        set_sequence_control_status(c_status_snapshot);
    else if (key == keys().oneshot_queue())
        set_sequence_control_status(c_status_oneshot);
    else
        result = false;

    return result;
}

/**
 *
 */

bool
perform::keyboard_group_c_status_release (unsigned key)
{
    bool result = true;
    if (key == keys().replace())
        unset_sequence_control_status(c_status_replace);
    else if (key == keys().queue())
        unset_sequence_control_status(c_status_queue);
    else if (key == keys().snapshot_1() || key == keys().snapshot_2())
        unset_sequence_control_status(c_status_snapshot);
    else if (key == keys().oneshot_queue())
        unset_sequence_control_status(c_status_oneshot);
    else
        result = false;

    return result;
}

/**
 *
 */

bool
perform::keyboard_group_press (unsigned key)
{
    bool result = true;
    if (key == keys().group_on())
        set_mode_group_mute();                  /* m_mode_group = true */
    else if (key == keys().group_off())
        unset_mode_group_mute();
    else if (key == keys().group_learn())
        set_mode_group_learn();
    else
        result = false;

    return result;
}

/**
 *
 */

bool
perform::keyboard_group_release (unsigned key)
{
    bool result = true;
    if (key == keys().group_learn())
        unset_mode_group_learn();
    else
        result = false;

    return result;
}

/**
 *
 */

perform::action_t
perform::keyboard_group_action (unsigned key)
{
    action_t result = ACTION_NONE;
    if (key == keys().bpm_dn())
    {
        (void) decrement_beats_per_minute();
        result = ACTION_BPM;
    }
    else if (key == keys().bpm_up())
    {
        (void) increment_beats_per_minute();
        result = ACTION_BPM;
    }
    else if (key == keys().tap_bpm())
    {
        result = ACTION_BPM;            // make sure the tap records the BPM
    }
    else if (key == keys().screenset_dn())  // || k.is(SEQ64_Page_Down)) ???
    {
        (void) decrement_screenset();
        result = ACTION_SCREENSET;
    }
    else if (key == keys().screenset_up())  // || k.is(SEQ64_Page_Up)) ???
    {
        (void) increment_screenset();
        result = ACTION_SCREENSET;
    }

    // more to come
    //  perform::ACTION_SEQ_TOGGLE:
    //  perform::ACTION_GROUP_MUTE:
    //  perform::ACTION_GROUP_LEARN:
    //  perform::ACTION_C_STATUS:

    return result;
}

/**
 *  Provided for perfroll :: on_key_press_event() and perfroll ::
 *  on_key_release_event() to call.  It handles the Ctrl keys for cut, copy,
 *  paste, and undo.
 *
 *  The "is modified" flag is raised if something is deleted, but we cannot
 *  yet handle the case where we undo all the changes.  So, for now,
 *  we play it safe with the user, even if the user gets annoyed because he
 *  knows that he undid all the changes.
 *
 * \param k
 *      The keystroke object to be handled.
 *
 * \param drop_sequence
 *      Provides the index of the sequence whose selected trigger is to be
 *      cut, copied, or pasted.  Undo and redo are now supported.
 *
 * \return
 *      Returns true if the key was handled.
 */

bool
perform::perfroll_key_event (const keystroke & k, int drop_sequence)
{
    bool result = false;
    if (k.is_press())
    {
        sequence * s = get_sequence(drop_sequence);
        if (not_nullptr(s))
        {
            if (k.is_delete())
            {
                push_trigger_undo();
                s->delete_selected_triggers();
                modify();
                result = true;
            }
            else if (k.mod_control())               /* SEQ64_CONTROL_MASK   */
            {
                if (k.is_letter('x'))                           /* cut      */
                {
                    push_trigger_undo();                        /* needed?  */
                    s->cut_selected_trigger();
                    modify();
                    result = true;
                }
                else if (k.is_letter('c'))                      /* copy     */
                {
                    s->copy_selected_trigger();
                    result = true;
                }
                else if (k.is_letter('v'))                      /* paste    */
                {
                    push_trigger_undo();
                    s->paste_trigger();
                    modify();
                    result = true;
                }
                else if (k.is_letter('z'))                      /* undo     */
                {
                    pop_trigger_undo();                 /* perfedit::undo() */
                    result = true;
                }
                else if (k.is_letter('r'))                      /* redo     */
                {
                    pop_trigger_redo();                 /* perfedit::redo() */
                    result = true;
                }
            }
        }
    }
    return result;
}

/**
 *  This code handles the use of the Shift key to toggle the mute state of all
 *  other sequences.  See mainwid::on_button_release_event().  If the Shift
 *  key is pressed, toggle the mute state of all other sequences.  Inactive
 *  sequences are skipped.  Compare it to toggle_other_names().
 *
 * \param seqnum
 *      The sequence that is being clicked on.  It must be active in order to
 *      allow toggling.
 *
 * \param isshiftkey
 *      Indicates if the shift-key functionality for toggling all of the other
 *      sequences is active.
 *
 * \return
 *      Returns true if the full toggling was able to be performed.
 */

bool
perform::toggle_other_seqs (int seqnum, bool isshiftkey)
{
    bool result = is_active(seqnum);
    if (result)
    {
        result = isshiftkey;
        if (result)
        {
            for (int s = 0; s < m_sequence_max; ++s)
            {
                if (s != seqnum)
                    sequence_playing_toggle(s);
            }
        }

        /*
         * \change ca 2017-04-18 Issue #71.
         * This code causes issue #71, where Live mode does not work correctly,
         * and it also toggles the muting status in the song/ performance window!
         * This code was never meant to be activated :-(
         *
         * else
         * {
         *     sequence * seq = get_sequence(seqnum);
         *     if (not_nullptr(seq))
         *         seq->toggle_song_mute();
         * }
         */
    }
    return result;
}

/**
 *  This code handles the use of the Shift key to toggle the mute state of all
 *  other sequences.  See perfnames::on_button_press_event().  If the Shift
 *  key is pressed, toggle the mute state of all other sequences.
 *  Inactive sequences are skipped.  Compare it to toggle_other_seqs().
 *
 * \param seqnum
 *      The sequence that is being clicked on.  It must be active in order to
 *      allow toggling.
 *
 * \param isshiftkey
 *      Indicates if the shift-key functionality for toggling all of the other
 *      sequences is active.
 *
 * \return
 *      Returns true if the toggling was able to be performed.
 */

bool
perform::toggle_other_names (int seqnum, bool isshiftkey)
{
    bool result = is_active(seqnum);
    if (result)
    {
        if (isshiftkey)
        {
            for (int s = 0; s < m_sequence_high; ++s)
            {
                if (s != seqnum)
                {
                    sequence * seq = get_sequence(s);
                    if (not_nullptr(seq))
                        seq->toggle_song_mute();
                }
            }
        }
        else
        {
            get_sequence(seqnum)->toggle_song_mute();   /* already tested   */
        }
    }
    return result;
}

/**
 *  Toggles sequences.  Useful in perfnames, taken from
 *  perfnames::on_button_press_event() so that it can be re-used in qperfnames.
 */

bool
perform::toggle_sequences (int seqnum, bool isshiftkey)
{
#define USE_THIS_WORKING_CODE
#ifdef USE_THIS_WORKING_CODE
    bool result = toggle_other_names(seqnum, isshiftkey);
#else
    bool result = false;
    if (is_active(seqnum))
    {
        if (isshiftkey)
        {
            /*
             *  If the Shift key is pressed, toggle the mute state of all
             *  other sequences.  Inactive sequences are skipped.
             */

            for (int s = 0; s < m_sequence_high; ++s)
            {
                if (s != seqnum)
                {
                    sequence * seq = get_sequence(s);
                    if (not_nullptr(seq))
                    {
                        bool muted = seq->get_song_mute();
                        seq->set_song_mute(! muted);
                        result = true;
                    }
                }
            }
        }
        else
        {
            sequence * seq = get_sequence(seqnum);
            if (not_nullptr(seq))
            {
                bool muted = seq->get_song_mute();
                seq->set_song_mute(! muted);
                result = true;
            }
        }
    }
#endif
    return result;
}

/**
 *  Invoke the start key functionality.  Meant to be used by GUIs to unify the
 *  treatment of keys versus buttons.  Also handy in the extended MIDI
 *  controls that people have requested.
 *
 * \param songmode
 *      The live/play mode parameter to be passed along to the key processor.
 *      Defaults to false (live mode).
 */

void
perform::start_key (bool songmode)
{
    (void) playback_key_event(keys().start(), songmode);
}

/**
 *  Invoke the pause key functionality.  Meant to be used by GUIs to unify the
 *  treatment of keys versus buttons.  Also handy in the extended MIDI
 *  controls that people have requested.
 *
 * \param songmode
 *      The live/play mode parameter to be passed along to the key processor,
 *      when starting playback.  Defaults to false (live mode).
 */

void
perform::pause_key (bool songmode)
{
    (void) playback_key_event(keys().pause(), songmode);
}

/**
 *  Invoke the stop key functionality.  Meant to be used by GUIs to unify the
 *  treatment of keys versus buttons.  Also handy in the extended MIDI
 *  controls that people have requested.
 */

void
perform::stop_key ()
{
    (void) playback_key_event(keys().stop());
}

/**
 *  New function provided to unify the stop/start (space/escape) behavior of
 *  the various windows where playback can be started, paused, or stopped.  To
 *  be used in mainwnd, perfedit, and seqroll.
 *
 *  The start/end key may be the same key (e.g. Space) to allow toggling when
 *  the same key is mapped to both triggers.
 *
 *  Checking is_running() may not work completely in JACK.
 *
 * \change layk 2016-10-11
 *      Issue #42 to prevent inadvertent step-edit in sequence ::
 *      stream_event().  We did it slightly different to save a little code;
 *      also found a spot that was missed.
 *
 * \param k
 *      Provides the encapsulated keystroke to check.
 *
 * \param songmode
 *      Provides the "jack flag" needed by the mainwnd, seqroll, and perfedit
 *      windows.  Defaults to false, which disables Song mode, and enables
 *      Live mode.  But using Song mode seems to make the pause key not work
 *      in the performance editor.
 *
 * \return
 *      Returns true if the keystroke matched the start, stop, or (new) pause
 *      keystrokes.  Generally, no further keystroke processing is needed in
 *      this case.
 */

bool
perform::playback_key_event (const keystroke & k, bool songmode)
{
    bool result = k.is(keys().start(), keys().stop());
    if (! result)
        result = k.is(keys().pause());

    if (result)
    {
        bool onekey = keys().start() == keys().stop();
        bool isplaying = false;
        if (k.is(keys().start()))
        {
            if (onekey)
            {
                if (is_running())
                {
                    /*
                     * It is inconsistent to do this, I think:
                     * pause_playing(songmode);  // why pause, not stop?
                     */

                    stop_playing();
                }
                else
                {
                    start_playing(songmode);
                    isplaying = true;
                }
            }
            else if (! is_running())
            {
                start_playing(songmode);
                isplaying = true;
            }
        }
        else if (k.is(keys().stop()))
        {
            stop_playing();
        }
        else if (k.is(keys().pause()))
        {
            if (is_running())
                pause_playing(songmode);
            else
            {
                start_playing(songmode);
                isplaying = true;
            }
        }
        is_pattern_playing(isplaying);
    }
    return result;
}

/**
 *  Shows all the triggers of all the sequences.
 */

void
perform::print_triggers () const
{
    for (int s = 0; s < m_sequence_high; ++s)
    {
        if (is_active(s))
            m_seqs[s]->print_triggers();
    }
}

/**
 *  Shows all the triggers of all the sequences.
 */

void
perform::print_busses () const
{
    if (not_nullptr(m_master_bus))
        m_master_bus->print();
}

/**
 *  Calls the apply_song_transpose() function for all active sequences.
 */

void
perform::apply_song_transpose ()
{
    for (int s = 0; s < m_sequence_high; ++s)       /* modest speed-up */
    {
        sequence * seq = get_sequence(s);
        if (not_nullptr(seq))
            seq->apply_song_transpose();
    }
}

/**
 *  Reloads the mute groups from the "rc" file.
 *
 * \param errmessage
 *      A pass-back parameter for any error message the file-processing might
 *      cause.
 *
 * \return
 *      Returns true if the reload succeeded.
 */

bool
perform::reload_mute_groups (std::string & errmessage)
{
    return parse_mute_groups(*this, errmessage);
}

/**
 *  Checks the whole universe of sequences to determine the current
 *  last-active set, that is, the highest set that has any active sequences in
 *  it.
 *
 * \return
 *      Returns the value of the highest active set.  A value of 0 represents
 *      the first set.  If no sequences are active, then -1 is returned.
 */

int
perform::max_active_set () const
{
    int result = -1;
    for (int s = 0; s < m_sequence_high; ++s)       /* modest speed-up */
    {
        if (is_active(s))
            result = s;
    }
    if (result >= 0)
        result /= m_seqs_in_set;

    return result;
}

/**
 *  Implements the fast-forward or rewind functionality imported from seq32.
 *  It changes m_tick by a quarter of the number of ticks in a standard measure,
 *  with m_excell_FF_RW (defaults to one) to factor the difference.
 */

void
perform::FF_rewind ()
{
    if (m_FF_RW_button_type == FF_RW_NONE)
        return;

    long tick = 0;
    long measure_ticks = measures_to_ticks(m_beats_per_bar, m_ppqn, m_beat_width);
    if (measure_ticks >= m_ppqn)
    {
        /*
         * The factor was 0.25, now 1.0, but might be better as a
         * configuragble item in the "usr" configuration file.
         */

        measure_ticks = long(measure_ticks * 1.00 * m_excell_FF_RW);
        if (m_FF_RW_button_type == FF_RW_REWIND)
        {
            tick = get_tick() - measure_ticks;
            if (tick < 0)
                tick = 0;
        }
        else                    // if (m_FF_RW_button_type == FF_RW_FORWARD)
            tick = get_tick() + measure_ticks;
    }
    else
    {
        errprint("perform::FF_rewind() programmer error");
    }
    if (is_jack_running())
    {
        position_jack(true, tick);
    }
    else
    {
        set_start_tick(tick);               /* this sets the progress line */
        set_reposition();
    }
}

/**
 *  Encapsulates some repositioning code needed to move the position to the
 *  mouse pointer location in perfroll.  Used only in perfroll ::
 *  on_key_press_event() to implement the Seq32 pointer-position feature.
 *
 * \param tick
 *      Provides the position value to be set.
 */

void
perform::reposition (midipulse tick)
{
    set_reposition();
    set_start_tick(tick);
    if (is_jack_running())
        position_jack(true, tick);
}

/**
 *  This version for song-recording not only logs m_tick, it also does JACK
 *  positioning (if applicable), calls the master bus's continue_from()
 *  function, and sets m_current_tick as well.
 *
 * \todo
 *      Do we really need m_current_tick???
 */

void
perform::set_tick (midipulse tick)
{

#ifdef PLATFORM_DEBUG_TMI

    /*
     * Display the tick values; normally this is too much information.
     */

    static midipulse s_last_tick = 0;
    midipulse difference = tick - s_last_tick;
    if (difference > 100)
    {
        s_last_tick = tick;
        printf("perform tick = %ld\n", m_tick);
        fflush(stdout);
    }
    if (tick == 0)
        s_last_tick = 0;

#endif  // PLATFORM_DEBUG_TMI

    m_tick = tick;

    /*
     * \change ca 2017-12-30 Issue #123
     *      This code, when enabled, causes the progress to be continually
     *      reset to 0 when JACK Transport is active.
     *
     *  if (m_jack_asst.is_running())
     *      position_jack(tick);
     *  master_bus().continue_from(tick);
     */

#ifdef SEQ64_SONG_RECORDING
    m_current_tick = tick;
#endif
}

/**
 *  Convenience function.  This function is used in the free function version
 *  of FF_RW_timeout() as a callback to the gtk_timeout() function.  It
 *  multiplies m_excell_FF_RW by 1.1 as long as one of the fast-forward or
 *  rewind keys is held, and is less than 60.
 *
 * \return
 *      Returns true if one of the fast-forward or rewind keys was held,
 *      leaving m_excell_FF_RW at the last value it had.  Otherwise, it resets
 *      the value to 1, and returns false.
 */

bool
perform::FF_RW_timeout ()
{
    if (m_FF_RW_button_type != FF_RW_NONE)
    {
        FF_rewind();
        if (m_excell_FF_RW < 60.0f)
            m_excell_FF_RW *= 1.1f;

        return true;
    }
    m_excell_FF_RW = 1.0;
    return false;
}

#ifdef SEQ64_SONG_RECORDING

/**
 *  Calls sequence::song_recording_stop(m_current_tick) for all sequences.
 *  Should be called only when not recording the performance data.  This is a
 *  Kepler34 feature.
 */

void
perform::song_recording_stop ()
{
    for (int i = 0; i < m_sequence_high; ++i)   /* m_sequence_max       */
    {
        sequence * s = get_sequence(i);
        if (not_nullptr(s))
            s->song_recording_stop(m_current_tick);     // TODO!!!!
    }
}

#endif  // SEQ64_SONG_RECORDING

#ifdef PLATFORM_DEBUG_TMI

/**
 *  Dumps the status of all tracks in all active sets in a compact format.
 *  The format is 32 lines of 32 characters each, with each character
 *  representing the most important of flags:
 *
 *      -   "o" armed/unmuted
 *      -   "-" unarmed/muted
 *      -   " " inactive
 *      -   "t" NOT transposable
 *      -   "q" queued
 *      -   "p" playing
 *      -   "r" recording
 *      -   "0" SMF 0 format track
 */

void
perform::dump_mute_statuses (const std::string & tag)
{
    puts(tag.c_str());
    puts(" ================================");      /* includes the newline */
    int setmax = max_active_set();
    if (setmax < 0)
        setmax = 0;                                 /* show at least one    */

    int currseq = 0;
    for (int ss = 0; ss <= setmax; ++ss)
    {
        putc('|', stdout);
        for (int seq = 0; seq < m_seqs_in_set; ++seq, ++currseq)
        {
            char c = ' ';
            if (is_active(currseq))
            {
                const sequence * s = m_seqs[currseq];
                c = s->get_song_mute() ? '-' : 'o' ;
                if (! s->get_transposable())
                    c = 't';

                if (s->get_queued())
                    c = 'q';

                if (s->get_playing())
                    c = 'p';

                if (s->get_recording())
                    c = 'r';

                if (s->is_smf_0())
                    c = '0';
            }
            putc(c, stdout);
        }
        puts("|");                                  /* includes the newline */
    }
    puts(" ================================");      /* includes the newline */
}

#endif      // PLATFORM_DEBUG_TMI

}           // namespace seq64

/*
 * perform.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

