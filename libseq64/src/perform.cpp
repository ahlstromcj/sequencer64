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
 * \author        Seq24 team; modifications by Chris Ahlstrom and Tim Deagan
 * \date          2015-07-24
 * \updates       2017-03-17
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
 */

#include <sched.h>
#include <stdio.h>
#include <string.h>                     /* memset()                         */

#include "calculations.hpp"
#include "event.hpp"
#include "keystroke.hpp"
#include "midibus.hpp"
#include "perform.hpp"
#include "settings.hpp"                 /* seq64::rc() and choose_ppqn()    */

#if ! defined PLATFORM_WINDOWS
#include <time.h>                       /* struct timespec                  */
#endif

/**
 *  Indicates if the playing-screenset code is in force or not, for
 *  experimenting.  Without this patch, ignoring snapshots, it seems like
 *  mute-groups only work on screen-set 0, where as with the patch (again
 *  ignoring snapshots), they apply to the "in-view" screen-set.
 */

#define SEQ64_USE_TDEAGAN_CODE

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *  Purely internal constants used with the functions that implement MIDI
 *  control for the application.  Note how they specify different bit values,
 *  as it they could be masked together to signal multiple functions.
 *  This value signals the "replace" functionality.
 */

static const int c_status_replace  = 0x01;

/**
 *  This value signals the "snapshot" functionality.
 */

static const int c_status_snapshot = 0x02;

/**
 *  This value signals the "queue" functionality.
 */

static const int c_status_queue    = 0x04;

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
#ifdef SEQ64_STAZED_JACK_SUPPORT
    m_start_from_perfedit       (false),
    m_reposition                (false),
    m_excell_FF_RW              (1.0f),
    m_FF_RW_button_type         (FF_RW_NONE),
#endif
    m_mute_group                (),         // boolean array, size 32 * 32
#ifdef SEQ64_TOGGLE_PLAYING
    m_armed_saved               (false),
    m_armed_statuses            (),         // boolean array, size 1024
#endif
    m_tracks_mute_state         (),         // set's tracks [c_seqs_in_set]
    m_mode_group                (true),
    m_mode_group_learn          (false),
    m_mute_group_selected       (0),
    m_playing_screen            (0),        // vice m_screenset
    m_playscreen_offset         (0),
    m_seqs                      (),         // pointer array [c_max_sequence]
    m_seqs_active               (),         // boolean array [c_max_sequence]
    m_was_active_main           (),         // boolean array [c_max_sequence]
    m_was_active_edit           (),         // boolean array [c_max_sequence]
    m_was_active_perf           (),         // boolean array [c_max_sequence]
    m_was_active_names          (),         // boolean array [c_max_sequence]
    m_sequence_state            (),         // boolean array [c_max_sequence]
#ifdef SEQ64_STAZED_TRANSPOSE
    m_transpose                 (0),
#endif
    m_out_thread                (),
    m_in_thread                 (),
    m_out_thread_launched       (false),
    m_in_thread_launched        (false),
    m_running                   (false),
    m_is_pattern_playing        (false),
    m_inputing                  (true),
    m_outputing                 (true),
    m_looping                   (false),
    m_playback_mode             (false),
    m_ppqn                      (choose_ppqn(ppqn)),
    m_bpm                       (SEQ64_DEFAULT_BPM),
    m_beats_per_bar             (SEQ64_DEFAULT_BEATS_PER_MEASURE),
    m_beat_width                (SEQ64_DEFAULT_BEAT_WIDTH),
    m_clocks_per_metronome      (24),
    m_32nds_per_quarter         (8),
    m_us_per_quarter_note       (tempo_to_us(SEQ64_DEFAULT_BPM)),
    m_master_bus                (nullptr),
    m_master_clocks             (),         // vector<clock_e>
    m_master_inputs             (),         // vector<bool>
    m_one_measure               (m_ppqn * 4),
    m_left_tick                 (0),
    m_right_tick                (m_one_measure * 4),        // m_ppqn * 16
    m_starting_tick             (0),
    m_tick                      (0),
    m_jack_tick                 (0),
    m_usemidiclock              (false),
    m_midiclockrunning          (false),
    m_midiclocktick             (0),
    m_midiclockpos              (-1),
    m_dont_reset_ticks          (false),
    m_screen_set_notepad        (),         // string array [c_max_sets]
    m_midi_cc_toggle            (),         // midi_control []
    m_midi_cc_on                (),         // midi_control []
    m_midi_cc_off               (),         // midi_control []
    m_offset                    (0),
    m_control_status            (0),
    m_screenset                 (0),        // vice m_playing_screen
#ifdef SEQ64_USE_AUTO_SCREENSET_QUEUE
    m_auto_screenset_queue      (false),
#endif
    m_seqs_in_set               (c_seqs_in_set),
    m_max_sets                  (c_max_sets),
    m_sequence_count            (0),
    m_sequence_max              (c_max_sequence),
    m_sequence_high             (0),
#ifdef SEQ64_EDIT_SEQUENCE_HIGHLIGHT
    m_edit_sequence             (-1),
#endif
    m_is_modified               (false),
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
    for (int i = 0; i < m_sequence_max; ++i)
    {
        m_seqs[i] = nullptr;
        m_seqs_active[i] =                      /* seq24 0.9.3 addition     */
            m_sequence_state[i] =               /* ca 2016-11-27            */
            m_was_active_main[i] = m_was_active_edit[i] =
            m_was_active_perf[i] = m_was_active_names[i] = false;
    }
    for (int i = 0; i < m_sequence_max; ++i)    /* not c_gmute_tracks now   */
    {
#ifdef SEQ64_TOGGLE_PLAYING
        m_mute_group[i] = m_armed_statuses[i] = false;
#else
        m_mute_group[i] = false;
#endif
    }

    for (int i = 0; i < c_seqs_in_set; ++i)
        m_tracks_mute_state[i] = false;

    for (int i = 0; i < m_max_sets; ++i)
        m_screen_set_notepad[i].clear();

    midi_control zero;                          /* all members false or 0   */
    for (int i = 0; i < c_midi_controls_extended; ++i)
        m_midi_cc_toggle[i] = m_midi_cc_on[i] = m_midi_cc_off[i] = zero;

    /*
     * Not sure why we need to this, since it is done by the
     * keys_perform-derived object.
     *
     * set_all_key_events();
     * set_all_key_groups();
     */
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
    m_inputing = m_outputing = m_running = false;
    m_condition_var.signal();                   /* signal the end of play   */
    if (m_out_thread_launched)
        pthread_join(m_out_thread, NULL);

    if (m_in_thread_launched)
        pthread_join(m_in_thread, NULL);

    for (int seq = 0; seq < m_sequence_max; ++seq)      /* m_sequence_high  */
    {
        if (not_nullptr(m_seqs[seq]))
        {
            delete m_seqs[seq];
            m_seqs[seq] = nullptr;              /* not strictly necessary   */
        }
    }
}

/**
 *  Creates the mastermidibus.  We need to delay creation until launch time,
 *  so that settings can be obtained before determining just how to set up the
 *  application.
 *
 *  Once the master buss is created, we then copy the clocks and input setting
 *  that were read from the "rc" file, via the mastermidibus::port_settings()
 *  function, to use in determining whether to initialize and connect the
 *  input ports at start-up.  Seq24 wouldn't connect unconditionally, and
 *  Sequencer64 shouldn't, either.
 *
 *  However, the devices actually on the system at start time might be
 *  different from what was saved in the "rc" file after the last run of
 *  Sequencer64.
 *
 * \return
 *      Returns true if the creation succeeded.
 */

bool
perform::create_master_bus ()
{
    m_master_bus = new (std::nothrow) mastermidibus();
    bool result = not_nullptr(m_master_bus);
    if (result)
        master_bus().port_settings(m_master_clocks, m_master_inputs);

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
    if (create_master_bus())
    {

#ifdef SEQ64_JACK_SUPPORT
        init_jack_transport();
#endif

        master_bus().init(ppqn, m_bpm);     /* calls api_init() per API */

        /*
         * We may need to copy the actually input buss settings back to here,
         * as they can change.  LATER.  They get saved properly anyway,
         * because the optionsfile object gets the information from the
         * mastermidibus more directly.
         */

        if (activate())
        {
#if 0
            master_bus().swap();            /* reconcile with JACK ways */
#endif
            launch_input_thread();
            launch_output_thread();
        }
    }
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
    for (int s = 0; s < m_sequence_max; ++s)            /* m_sequence_high  */
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
        for (int s = 0; s < m_sequence_max; ++s)        /* m_sequence_high  */
            if (is_active(s))
                delete_sequence(s);             /* can set "is modified"    */

        std::string e;                          /* an empty string          */
        for (int sset = 0; sset < m_max_sets; ++sset)
            set_screen_set_notepad(sset, e);

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
 *  the >= operator.
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
        track = 0;
    else if (track >= m_seqs_in_set)
        track = m_seqs_in_set - 1;

    return track;
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
 *  If we're in group-learn mode, then this function gets the playing statuses
 *  of all of the sequences in the current play-screen, and copies them into
 *  the desired mute-group.
 *
 *  Then, no matter what, it makes the desired mute-group the selected
 *  mute-group.  Compare to set_and_copy_mute_group().
 *
 *  One thing to note is that, once saved, then, if used, it is applied
 *  to the current screen-set, even if it is not the screen-set whose
 *  playing status were saved.
 *
 * \param mutegroup
 *      The number of the desired mute group, clamped to be between 0 and
 *      m_seqs_in_set-1.  Obviously, it is the set whose state is to be
 *      stored, if in group-learn mode.
 */

void
perform::select_group_mute (int mutegroup)
{
    mutegroup = clamp_track(mutegroup);
    if (m_mode_group_learn)
    {
        int groupbase = mutegroup * m_seqs_in_set;      /* 1st seq in group */
        for (int s = 0; s < m_seqs_in_set; ++s)
        {
            int source = m_playscreen_offset + s;       /* m_screenset?     */
            int dest = groupbase + s;
            if (is_active(source))
                m_mute_group[dest] = m_seqs[source]->get_playing();
        }
    }
    m_mute_group_selected = mutegroup;
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
    mutegroup = clamp_track(mutegroup);
    int groupbase = mutegroup * m_seqs_in_set;
#ifdef SEQ64_USE_TDEAGAN_CODE
    int setbase = m_screenset * m_seqs_in_set;  /* was m_playscreen_offset  */
#else
    int setbase = m_playscreen_offset;          /* includes m_seqs_in_set   */
#endif
    m_mute_group_selected = mutegroup;          /* must set it before loop  */
    for (int s = 0; s < m_seqs_in_set; ++s)
    {
        int source = setbase + s;
        if (m_mode_group_learn && is_active(source))
        {
            int dest = groupbase + s;
            m_mute_group[dest] = m_seqs[source]->get_playing();
        }
        m_tracks_mute_state[s] = m_mute_group[mute_group_offset(s)];
    }
}

/**
 *  If m_mode_group is true, then this function operates.  It loops through
 *  every screen-set.  In each screen-set, it acts on each active sequence.
 *  If the active sequence is in the current "in-view" screen-set (m_screenset
 *  as opposed to m_playing_screen), and its m_track_mute_state[] is true,
 *  then the sequence is turned on, otherwise it is turned off.
 *
 * \change tdeagan 2015-12-22 via git pull.
 *      Replaced m_playing_screen with m_screenset.
 *
 *  It seems to us that the for (g) clause should have g range from 0 to
 *  m_max_sets, not m_seqs_in_set.
 */

void
perform::mute_group_tracks ()
{
    if (m_mode_group)
    {
        for (int g = 0; g < m_seqs_in_set; ++g)         /* was m_max_sets!! */
        {
            int seqoffset = g * m_seqs_in_set;
            for (int s = 0; s < m_seqs_in_set; ++s)
            {
                int seqnum = seqoffset + s;
                if (is_active(seqnum))
                {
#ifdef SEQ64_USE_TDEAGAN_CODE
                    bool on = (g == m_screenset) && m_tracks_mute_state[s];
#else
                    bool on = (g == m_playing_screen) && m_tracks_mute_state[s];
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
    for (int i = 0; i < m_sequence_max; ++i)    /* m_sequence_high  */
    {
        if (is_active(i))
        {
            m_seqs[i]->set_song_mute(flag);
            m_seqs[i]->set_playing(! flag); /* needed to show mute status!  */
        }
    }
}

/**
 *  Toggles the mutes status of all tracks in the current set of active
 *  patterns/sequences.  Covers tracks from 0 to m_sequence_max.
 */

void
perform::toggle_all_tracks ()
{
    for (int i = 0; i < m_sequence_max; ++i)    /* m_sequence_high  */
    {
        if (is_active(i))
        {
            m_seqs[i]->toggle_song_mute();
            m_seqs[i]->toggle_playing();        /* needed to show mute status */
        }
    }
}

#ifdef SEQ64_TOGGLE_PLAYING

/**
 *  Toggles the mutes status of all playing (currently unmuted) tracks in the
 *  current set of active patterns/sequences.  Covers tracks from 0 to
 *  m_sequence_max.  The statuses are preserved for restoration.
 *
 *  Note that this function operates only in Live mode; it is too confusing to
 *  use in Song mode.
 */

void
perform::toggle_playing_tracks ()
{
    if (song_start_mode())
        return;

    if (m_armed_saved)
    {
        m_armed_saved = false;
        for (int i = 0; i < m_sequence_max; ++i)    /* m_sequence_high  */
        {
            if (m_armed_statuses[i])
            {
                m_seqs[i]->toggle_song_mute();
                m_seqs[i]->toggle_playing();    /* needed to show mute status */
            }
        }
    }
    else
    {
        for (int i = 0; i < m_sequence_max; ++i)    /* m_sequence_high  */
        {
            bool armed_status = false;
            if (is_active(i))
                armed_status = m_seqs[i]->get_playing();

            m_armed_statuses[i] = armed_status;
            if (armed_status)
            {
                m_armed_saved = true;           /* at least one was armed     */
                m_seqs[i]->toggle_song_mute();
                m_seqs[i]->toggle_playing();    /* needed to show mute status */
            }
        }
    }
}

#endif  // SEQ64_TOGGLE_PLAYING

/**
 *  Provides for various settings of the song-mute status of all sequences in
 *  the song. The sequence::set_song_mute() and toggle_song_mute() functions
 *  do all the work, including mp-dirtying the sequence.
 *
 *  We've modified this function to call mute_all_tracks() and
 *  toggle_all_tracks() in order to consolidate the code and (cough cough) fix
 *  a bug in this functionality from the mainwnd menu.
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
    int seq = ss * m_seqs_in_set;
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
 *      The perform::m_one_measure member is currently hardwired to PPQN * 4.
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

#ifdef SEQ64_STAZED_JACK_SUPPORT

    if (is_jack_master())                       /* don't use in slave mode  */
        m_jack_asst.position(true, tick);       /* position_jack()          */
    else if (! is_jack_running())
        set_tick(tick);

    m_reposition = false;

#endif

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

#ifdef SEQ64_STAZED_JACK_SUPPORT
            if (setstart)
            {
                set_start_tick(m_left_tick);
                if (is_jack_master())                   // && is_jack_running())
                    position_jack(true, m_left_tick);
                else
                    set_tick(m_left_tick);

                m_reposition = false;
            }
#else
            if (setstart)
                set_start_tick(m_left_tick);
#endif
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
        errprintf
        (
            "install_sequence(): m_seqs[%d] not null, deleting old sequence\n",
            seqnum
        );
        delete m_seqs[seqnum];
        m_seqs[seqnum] = nullptr;
        if (m_sequence_count > 0)
        {
            --m_sequence_count;
        }
        else
        {
            errprint("install_sequence(): sequence counter already 0");
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
 */

void
perform::new_sequence (int seq)
{
    if (is_seq_valid(seq))
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
                    m_seqs[seq]->set_master_midi_bus(&master_bus());
                    modify();
                    if (buss_override != SEQ64_BAD_BUSS)
                        m_seqs[seq]->set_midi_bus(buss_override);
                }
            }
        }
    }
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
                m_seqs[seq]->set_name(std::string("Untitled"));
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
        m_was_active_main[seq] =
            m_was_active_edit[seq] =
            m_was_active_perf[seq] =
            m_was_active_names[seq] = true;
    }
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
perform::set_beats_per_minute (int bpm)
{
    if (bpm < SEQ64_MINIMUM_BPM)
        bpm = SEQ64_MINIMUM_BPM;
    else if (bpm > SEQ64_MAXIMUM_BPM)
        bpm = SEQ64_MAXIMUM_BPM;

#ifdef SEQ64_JACK_SUPPORT

    /*
     * This logic matches the original seq24, but is it really correct?
     */

    bool ok = ! (is_jack_running() && m_running);
    if (ok)
        m_jack_asst.set_beats_per_minute(bpm);
#else

    bool ok = ! m_running;

#endif

    if (ok)
    {
        master_bus().set_beats_per_minute(bpm);
        m_us_per_quarter_note = tempo_to_us(bpm);
        m_bpm = bpm;
    }
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
    if (seq >= 0 && seq < m_sequence_max)   /* m_sequence_high  */
        return true;
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
    bool ok = is_active(seq);
    if (ok)
    {
        const sequence * s = get_sequence(seq);
        ok = ! s->get_song_mute() && s->get_trigger_count() > 0;
    }
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
perform::is_sequence_in_edit (int seq)
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
 *  Copies the given string into m_screen_set_notepad[].
 *
 * \param screenset
 *      The ID number of the screen set, an index into the
 *      m_screen_set_notepad[] array.
 *
 * \param notepad
 *      Provides the string date to copy into the notepad.  Not sure why a
 *      pointer is used, instead of nice "const std::string &" parameter.  And
 *      this pointer isn't checked.  Fixed.
 */

void
perform::set_screen_set_notepad (int screenset, const std::string & notepad)
{
    if (is_screenset_valid(screenset))
    {
        if (notepad != m_screen_set_notepad[screenset])
        {
            m_screen_set_notepad[screenset] = notepad;
            modify();
        }
    }
}

/**
 *  Retrieves the given string from m_screen_set_notepad[].
 *
 * \param screenset
 *      The ID number of the screen set, an index into the
 *      m_screen_set_notepad[] array.  This value is validated.
 *
 * \return
 *      Returns a reference to the desired string, or to an empty string
 *      if the screen-set number is invalid.
 */

const std::string &
perform::get_screen_set_notepad (int screenset) const
{
    static std::string s_empty;
    if (is_screenset_valid(screenset))
        return m_screen_set_notepad[screenset];
    else
        return s_empty;
}

/**
 *  Sets the m_screenset value (the index or ID of the current screen set).
 *  It's not clear that we need to set the "is modified" flag just because we
 *  changed the screen set, so we don't.
 *
 *  As a new feature, we would like to queue-mute the previous screenset,
 *  and queue-unmute the newly-selected screenset.  Still working on getting
 *  it right.
 *
 * \param ss
 *      The index of the desired new screen set.  It is forced to range from
 *      0 to m_max_sets - 1.  The clamping seems weird, but hews to seq24.
 *      What it does is let the user wrap around the screen-sets in the user
 *      interface.
 */

void
perform::set_screenset (int ss)
{
    if (ss < 0)
        ss = m_max_sets - 1;
    else if (ss >= m_max_sets)
        ss = 0;

    if (ss != m_screenset)
    {
#ifdef SEQ64_USE_AUTO_SCREENSET_QUEUE
        if (m_auto_screenset_queue)
            swap_screenset_queues(m_screenset, ss);
        else
            m_screenset = ss;
#else
        m_screenset = ss;
#endif
    }
    set_offset(ss);             /* was called in mainwid::set_screenset() */
}

#ifdef SEQ64_USE_AUTO_SCREENSET_QUEUE

/**
 *  EXPERIMENTAL.  Doesn't quite work.  This may be due to a bug we found in
 *  mute_screenset(), on 2016-10-05, so we will revisit this functionality for
 *  0.9.19.
 *
 * \param flag
 *      If the flag is true:
 *      -#  Mute all tracks in order to start from a known status for all
 *          screen-sets.
 *      -#  Unmute screen-set 0 (the first screen-set).
 */

void
perform::set_auto_screenset (bool flag)
{
    m_auto_screenset_queue = flag;
}

/**
 *  EXPERIMENTAL.  Doesn't quite work.
 *
 *  Queues all of the sequences in the given screen-set.  Doesn't work, even
 *  after a lot of hacking on it, so disabled for now.
 *
 * \param ss0
 *      The original screenset, will be unqueued.
 *
 * \param ss1
 *      The destination screenset, will be queued.
 */

void
perform::swap_screenset_queues (int ss0, int ss1)
{
    if (is_pattern_playing())
    {
        int seq0 = ss0 * m_seqs_in_set;
        for (int s = 0; s < m_seqs_in_set; ++s, ++seq0)
        {
            if (is_active(seq0))
                m_seqs[seq0]->off_queued();         // toggle_queued();
        }

        int seq1 = ss1 * m_seqs_in_set;
        m_screenset = ss1;
        for (int s = 0; s < m_seqs_in_set; ++s, ++seq1)
        {
            if (is_active(seq1))
                m_seqs[seq1]->on_queued();          // toggle_queued();
        }
        set_playing_screenset();

#ifdef PLATFORM_DEBUG_XXX
        dump_mute_statuses("screen-set change");
#endif
    }
}

#endif  // SEQ64_USE_AUTO_SCREENSET_QUEUE

/**
 *  Sets the screen set that is active, based on the value of m_screenset.
 *  This function is called when one of the snapshot keys is pressed.
 *
 *  For each value up to m_seqs_in_set (32), the index of the current sequence
 *  in the current screen set (m_playing_screen) is obtained.  If the sequence
 *  is active and the sequence actually exists, it is processed; null
 *  sequences are no longer flagged as an error, they are just ignored.
 *
 *  Modifies m_playing_screen, m_playscreen_offset, stores the current
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
        {
            /*
             * \tricky
             *      These indices are what we want, don't change them.
             */

            m_tracks_mute_state[s] = m_seqs[source]->get_playing();
        }
    }
    m_playing_screen = m_screenset;
    m_playscreen_offset = m_screenset * m_seqs_in_set;
    mute_group_tracks();
}

/**
 *  Starts the playing of all the patterns/sequences.  This function just runs
 *  down the list of sequences and has them dump their events.  It skips
 *  sequences that have no playable MIDI events.
 *
 *  Note how often the "s" (sequence) pointer was used.  It was worth
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
    m_tick = tick;
    for (int s = 0; s < m_sequence_high; ++s)       /* modest speed up  */
    {
        if (is_active(s))
            m_seqs[s]->play_queue(tick, m_playback_mode);
    }
    master_bus().flush();                           /* flush MIDI buss  */
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
    for (int s = 0; s < m_sequence_max; ++s)        /* m_sequence_high  */
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
    if (is_active(seq))
        m_seqs[seq]->clear_triggers();
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
        for (int i = 0; i < m_sequence_max; ++i)        /* m_sequence_high  */
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
        for (int i = 0; i < m_sequence_max; ++i)        /* m_sequence_high  */
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
            for (int s = 0; s < m_sequence_max; ++s)    /* m_sequence_high  */
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
            for (int s = 0; s < m_sequence_max; ++s)    /* m_sequence_high */
            {
                if (is_active(s))                   /* oops, was "track"!   */
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
        for (int s = 0; s < m_sequence_max; ++s)    /* m_sequence_high */
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

#ifdef SEQ64_STAZED_JACK_SUPPORT

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

#else   // SEQ64_STAZED_JACK_SUPPORT

/*
 *  In this legacy version, the songmode parameter simply indicates which GUI,
 *  mainwnd ("live", false) or perfedit ("song", true) started the playback.
 *  However, if the song-mode is false, then we fall back to the value of
 *  song_start_mode(), in order not to violate user expectations from the
 *  setting on of the song-sart mode on mainwnd.
 *
 *  For cosmetic reasons, to stop transport line flicker on start, we tell
 *  position_jack() to position to the left tick, and tell start() to use the
 *  perfedit rewind.  Calling start() with false disables the perfedit mute
 *  control.  Calling start() with true causes a perfedit rewind.
 */

void
perform::start_playing (bool songmode)
{
    songmode = songmode || song_start_mode();
    position_jack(songmode);
    start_jack();
    start(songmode);

    /*
     * Let's let the output() function clear this, so that we can use this
     * flag in that function to control the next tick to play at resume time.
     *
     *      m_dont_reset_ticks = false;
     *
     * Shouldn't this be needed as well?  It is set in ALSA mode, but not JACK
     * mode. But DO NOT call set_running() here in JACK mode, it prevents
     * Sequencer64 from starting JACK transport!
     */

    if (! is_jack_running())
        set_running(true);

    is_pattern_playing(true);
}

#endif  // SEQ64_STAZED_JACK_SUPPORT

#ifdef SEQ64_STAZED_JACK_SUPPORT

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
    m_jack_asst.set_jack_mode(is_jack_running());    /* seqroll keybinding  */

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

#endif  // SEQ64_STAZED_JACK_SUPPORT

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
 * \param songmode
 *      Indicates that, if resuming play, it should play in Song mode (true)
 *      or Live mode (false).  See the comments for the start_playing()
 *      function.
 */

void
perform::pause_playing (bool songmode)
{
    m_dont_reset_ticks = true;
    stop_jack();
    if (is_jack_running())
    {
#ifdef SEQ64_STAZED_JACK_SUPPORT
        m_start_from_perfedit = songmode;   /* act like start_playing()     */
#endif
    }
    else
    {
        set_running(false);
        reset_sequences(true);              /* don't reset "last-tick"      */
        m_usemidiclock = false;
#ifdef SEQ64_STAZED_JACK_SUPPORT
        m_start_from_perfedit = false;      /* act like stop_playing()      */
#endif
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

#ifdef SEQ64_STAZED_JACK_SUPPORT
    m_start_from_perfedit = false;
#endif
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

#if ! defined SEQ64_STAZED_JACK_SUPPORT
    if (rc().with_jack_master())
        tick = SEQ64_NULL_MIDIPULSE;
#endif

    /*
     * TMI: printf("jack-ass position tick = %ld\n",tick);
     */

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
    bool result = master_bus().activate();      // make it initialize too!!!!!
    if (result)
        result = m_jack_asst.activate();

    return result;
}

/**
 *  If JACK is not running, call inner_start() with the given state.
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
        set_playback_mode(songmode);
        if (songmode)
            off_sequences();

        set_running(true);
        m_condition_var.signal();
    }
    m_condition_var.unlock();
}

/**
 *  Unconditionally, and without locking, clears the running status, resets
 *  the sequences, and sets m_usemidiclock false.  Note that we do need to set
 *  the running flag to false here, even when JACK is running.  Otherwise,
 *  JACK starts ping-ponging back and forth between positions under some
 *  circumstances.
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
#ifdef SEQ64_STAZED_JACK_SUPPORT
    start_from_perfedit(false);
    set_running(false);                 // rc().global_is_running() = false;
    reset_sequences();
    m_usemidiclock = midiclock;
#else
    set_running(false);
    if (! is_jack_running())
        reset_sequences();              /* sets the "last-tick" value   */

    m_usemidiclock = midiclock;
#endif
}

/**
 *  For all active patterns/sequences, set the playing state to false.
 *
 * EXPERIMENTAL: Replace "for (int s = 0; s < m_sequence_max; ++s)"
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
 *  For all active patterns/sequences, turn off its playing notes.
 *  Then flush the master MIDI buss.
 */

void
perform::all_notes_off ()
{
    /*
     * EXPERIMENTAL:
     *
     * for (int s = 0; s < m_sequence_max; ++s)
     */

    for (int s = 0; s < m_sequence_high; ++s)   /* modest speed-up  */
    {
        if (is_active(s))
            m_seqs[s]->off_playing_notes();
    }
    master_bus().flush();               /* flush the MIDI buss  */
}

/**
 *  For all active patterns/sequences, get its playing state, turn off the
 *  playing notes, set playing to false, zero the markers, and, if not in
 *  playback mode, restore the playing state.  Note that these calls are
 *  folded into one member function of the sequence class.  Finally, flush the
 *  master MIDI buss.
 *
 * \param pause
 *      Try to prevent notes from lingering on pause if true.  By default, it
 *      is false.
 */

void
perform::reset_sequences (bool pause)
{
    if (pause)
    {
        for (int s = 0; s < m_sequence_max; ++s)    /* m_sequence_high  */
        {
            if (is_active(s))
                m_seqs[s]->pause(m_playback_mode);  /* (new parameter)  */
        }
    }
    else
    {
        for (int s = 0; s < m_sequence_max; ++s)    /* m_sequence_high  */
        {
            if (is_active(s))
                m_seqs[s]->stop(m_playback_mode);
        }
    }
    master_bus().flush();                           /* flush the MIDI buss  */
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
        push_trigger_undo();
        s->split_trigger(tick);
        modify();
    }
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
perform::get_max_trigger ()
{
    midipulse result = 0;

    /*
     * EXPERIMENTAL:
     *
     * for (int s = 0; s < m_sequence_max; ++s)
     */

    for (int s = 0; s < m_sequence_high; ++s)           /* modest speed-up */
    {
        if (is_active(s))
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
    timeBeginPeriod(1);
    p->output_func();
    timeEndPeriod(1);
#else
    if (rc().priority())                        /* Not in MinGW RCB */
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
    while (m_outputing)
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
#if defined USE_SEQ24_0_9_3_CODE || defined SEQ64_STAZED_JACK_SUPPORT
        pad.js_clock_tick = 0;              // long probably offers more ticks
#else
        pad.js_clock_tick = 0.0;            // double
#endif
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
        }

        pad.js_jack_stopped = false;
        pad.js_dumping = false;
        pad.js_init_clock = true;
        pad.js_looping = m_looping;
        pad.js_playback_mode = m_playback_mode;
        pad.js_ticks_converted_last = 0.0;
#ifdef SEQ64_STAZED_JACK_SUPPORT
        pad.js_ticks_converted = 0.0;
        pad.js_ticks_delta = 0.0;
#endif

#if defined USE_SEQ24_0_9_3_CODE || defined SEQ64_STAZED_JACK_SUPPORT
        pad.js_delta_tick_frac = 0L;        // from seq24 0.9.3, long value
#endif

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
            pad.js_current_tick = long(m_starting_tick);    // midipulse
            pad.js_clock_tick = m_starting_tick;
            set_orig_ticks(m_starting_tick);                // what member?
        }

        int ppqn = master_bus().get_ppqn();

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

        while (m_running)
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
            int bpm  = master_bus().get_beats_per_minute();

            /*
             * Delta time to ticks; get delta ticks.
             *
             * seq24 0.9.3 changes delta_tick's type and adds some code --
             * delta_ticks_frac is in 1000th of a tick.  This code is meant to
             * correct for some clock drift.  However, this code breaks the
             * MIDI clock speed.  So let's try the "Stazed" version of the
             * code, from his seq32 project.  We get delta ticks,
             * delta_ticks_f is in 1000th of a tick.
             */

#if defined USE_SEQ24_0_9_3_CODE || defined SEQ64_STAZED_JACK_SUPPORT

            long long delta_tick_denom = 60000000LL;
            long long delta_tick_num = bpm * ppqn * delta_us +
                pad.js_delta_tick_frac;

            long delta_tick = long(delta_tick_num / delta_tick_denom);
            pad.js_delta_tick_frac = long(delta_tick_num % delta_tick_denom);

#else
            // delta_tick = double(bpm * ppqn * (delta_us / 60000000.0f));

            double delta_tick = delta_time_us_to_ticks(delta_us, bpm, ppqn);

#endif

            if (m_usemidiclock)
            {
                delta_tick = m_midiclocktick;       /* int to double */
                m_midiclocktick = 0;
            }
            if (m_midiclockpos >= 0)
            {
                delta_tick = 0;
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
                /*
                 * The default if JACK is not compiled in, or is not
                 * running.  Add the delta to the current ticks.
                 */

                pad.js_clock_tick += delta_tick;
                pad.js_current_tick += delta_tick;
                pad.js_total_tick += delta_tick;
                pad.js_dumping = true;
#ifdef SEQ64_JACK_SUPPORT
            }
#endif

#ifdef SEQ64_STAZED_JACK_SUPPORT

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

#endif
            /*
             * pad.js_init_clock will be true when we run for the first time,
             * or as soon as JACK gets a good lock on playback.
             */

            if (pad.js_init_clock)
            {
                master_bus().init_clock(midipulse(pad.js_clock_tick));
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
                    perfloop =
                        m_playback_mode ||
#ifdef SEQ64_STAZED_JACK_SUPPORT
                        start_from_perfedit() ||
#endif
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
                        pad.js_current_tick = double(ltick) + leftover_tick;
                    }
                    else
                        jack_position_once = false;
                }

#ifdef SEQ64_STAZED_JACK_SUPPORT

                /*
                 * Don't play during JackTransportStarting to avoid xruns on
                 * FF or RW.
                 */

                if (is_jack_running())
                {
                    if (m_jack_asst.transport_not_starting())
                        play(midipulse(pad.js_current_tick));       // play!
                }
                else
                    play(midipulse(pad.js_current_tick));           // play!
#else
                play(midipulse(pad.js_current_tick));               // play!
#endif

                /*
                 * The next line enables proper pausing in both old and seq32
                 * JACK builds.  Now unmacroed by SEQ64_STAZED_JACK_SUPPORT.
                 */

                set_jack_tick(pad.js_current_tick);
                master_bus().clock(midipulse(pad.js_clock_tick));   // MIDI clock

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
            int bpm  = master_bus().get_beats_per_minute();
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

#ifdef SEQ64_STAZED_JACK_SUPPORT

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
                else if (! m_dont_reset_ticks)          // EXPERIMENTAL
                    set_tick(0);                        // live mode default
            }
        }

        /*
         * This means we leave m_tick at stopped location if in slave mode or
         * if m_usemidiclock == true.
         */

        master_bus().flush();
        master_bus().stop();

        /*
         * In the new rtmidi version of the application (seq64), enabling this
         * code causes some conflicts between data access, and some how
         * jack_assistant::m_jack_client ends up being corrupted.
         */

#if USE_THIS_SEGFAULT_CAUSING_CODE
        if (is_jack_running())
            set_jack_stop_tick(get_current_jack_position((void *) this));
#endif

#else  // SEQ64_STAZED_JACK_SUPPORT

        if (is_jack_running())
            set_tick(0);

        master_bus().flush();
        master_bus().stop();

#endif  // SEQ64_STAZED_JACK_SUPPORT

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
 * \param ctrl
 *      The MIDI control value to use to perform an operation.
 *
 * \param state
 *      The state of the control, used with the following values:
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
        c_midi_control_record
        c_midi_control_solo         (for toggle, on, or off)
        c_midi_control_thru
        c_midi_control_14 to _19    (reserved for expansion)
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
 */

void
perform::handle_midi_control (int ctl, bool state)
{
    switch (ctl)
    {
    case c_midi_control_bpm_up:

        (void) increment_beats_per_minute();
        break;

    case c_midi_control_bpm_dn:

        (void) decrement_beats_per_minute();
        break;

    case c_midi_control_ss_up:

        (void) increment_screenset();
        break;

    case c_midi_control_ss_dn:

        (void) decrement_screenset();
        break;

    case c_midi_control_mod_replace:

        if (state)
            set_sequence_control_status(c_status_replace);
        else
            unset_sequence_control_status(c_status_replace);
        break;

    case c_midi_control_mod_snapshot:

        if (state)
            set_sequence_control_status(c_status_snapshot);
        else
            unset_sequence_control_status(c_status_snapshot);
        break;

    case c_midi_control_mod_queue:

        if (state)
            set_sequence_control_status(c_status_queue);
        else
            unset_sequence_control_status(c_status_queue);

    case c_midi_control_mod_gmute:

        if (state)
            set_mode_group_mute();              /* m_mode_group = true */
        else
            unset_mode_group_mute();
        break;

    case c_midi_control_mod_glearn:

        if (state)
            set_mode_group_learn();
        else
            unset_mode_group_learn();
        break;

    case c_midi_control_play_ss:                // Andy case; printf("play_ss\n");

        set_playing_screenset();
        break;

    default:

        /*
         * Based on the value of c_midi_track_crl (32 * 2) versus
         * m_seqs_in_set (32), maybe the first comparison should be
         * "ctl >= 2 * m_seqs_in_set".
         */

        if ((ctl >= m_seqs_in_set) && (ctl < c_midi_track_ctrl))
            select_and_mute_group(ctl - m_seqs_in_set);

        break;
    }
}

void
perform::handle_midi_control_ex (int ctl, midi_control::action a)
{
    switch (ctl)
    {
    case c_midi_control_playback:

        if (a == midi_control::action_toggle)
            pause_key();
        else if (a == midi_control::action_on)
            start_key();
        else if (a == midi_control::action_off)
            stop_key();
        break;

    case c_midi_control_record:                 /* arm for recording */

        // TODO
        break;

    case c_midi_control_solo:

        // TODO
        break;

    case c_midi_control_thru:

        // TODO
        break;
    }
}

/**
 *  This function encapsulates code in input_func() to make it easier to read
 *  and understand.
 *
 *  Here is the processing involved in this function ....
 *
 *  Incorporates pull request #24, arnaud-jacquemin, issue #23 "MIDI controller
 *  toggles wrong pattern".
 *
 * \change ca 2016-10-05
 *      Issue #35.  Changed "on" to "off".
 *
 *  QUESTIONS/TODO:
 *
 *      1. Why go above the sequence numbers, why not
 *         just go up to c_midi_track_ctrl?
 *
 *      2. What about our new extended controls?
 */

void
perform::midi_control_event (const event & ev)
{
    midibyte data[2] = { 0, 0 };
    midibyte status = ev.get_status();
    int offset = m_offset;
    ev.get_data(data[0], data[1]);

    // TODO: ACTIVATE this for-loop
    // for (int ctl = 0; ctl < g_midi_control_limit; ++ctl, ++offset)

    for (int ctl = 0; ctl < c_midi_controls; ++ctl, ++offset)
    {
        bool is_a_sequence = ctl < m_seqs_in_set;
        bool is_extended = ctl >= c_midi_controls &&
            ctl< c_midi_controls_extended;

        if (midi_control_toggle(ctl).match(status, data[0]))
        {
            if (midi_control_toggle(ctl).in_range(data[1]))
            {
                if (is_a_sequence)
                    sequence_playing_toggle(offset);
                else if (is_extended)
                    handle_midi_control_ex(ctl, midi_control::action_toggle);
            }
        }
        if (midi_control_on(ctl).match(status, data[0]))
        {
            if (midi_control_on(ctl).in_range(data[1]))
            {
                if (is_a_sequence)
                    sequence_playing_on(offset);
                else if (is_extended)
                    handle_midi_control_ex(ctl, midi_control::action_on);
                else
                    handle_midi_control(ctl, true);
            }
            else if (midi_control_on(ctl).inverse_active())
            {
                if (is_a_sequence)
                    sequence_playing_off(offset);
                else if (is_extended)
                    handle_midi_control_ex(ctl, midi_control::action_off);
                else
                    handle_midi_control(ctl, false);
            }
        }
        if (midi_control_off(ctl).match(status, data[0]))
        {
            if (midi_control_off(ctl).in_range(data[1]))  /* Issue #35 */
            {
                if (is_a_sequence)
                    sequence_playing_off(offset);
                else if (is_extended)
                    handle_midi_control_ex(ctl, midi_control::action_off);
                else
                    handle_midi_control(ctl, false);
            }
            else if (midi_control_off(ctl).inverse_active())
            {
                if (is_a_sequence)
                    sequence_playing_on(offset);
                else if (is_extended)
                    handle_midi_control_ex(ctl, midi_control::action_on);
                else
                    handle_midi_control(ctl, true);
            }
        }
    }
}

/**
 *  This function is called by input_thread_func().
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
 */

void
perform::input_func ()
{
    event ev;
    while (m_inputing)
    {
        if (master_bus().poll_for_midi() > 0)
        {
            do
            {
                if (master_bus().get_midi_event(&ev))
                {
                    /*
                     * Used when starting from the beginning of the song.
                     */

                    if (ev.get_status() == EVENT_MIDI_START) // MIDI Time Clock
                    {
#ifdef SEQ64_STAZED_JACK_SUPPORT
                        start(song_start_mode());
#else
                        stop();
                        start(false);
#endif
                        m_midiclockrunning = true;
                        m_usemidiclock = true;
                        m_midiclocktick = 0;
                        m_midiclockpos = 0;
                    }
                    else if (ev.get_status() == EVENT_MIDI_CONTINUE)
                    {
                        /*
                         * MIDI continue: start from current position.  This
                         * is sent immediately after EVENT_MIDI_SONG_POS, and
                         * is used for starting from other than beginning of
                         * the song, or to starting from previous location at
                         * EVENT_MIDI_STOP.
                         */

                        m_midiclockrunning = true;
#ifdef SEQ64_STAZED_JACK_SUPPORT
                        start(song_start_mode());
#else
                        start(false);
#endif
                    }
                    else if (ev.get_status() == EVENT_MIDI_STOP)
                    {
                        /*
                         * Just let the system pause.  Since we're not getting
                         * ticks after the stop, the song won't advance when
                         * start is received, we'll reset the position, or when
                         * continue is received, we won't reset the position.
                         * Should hold the stop position in case the next event
                         * is "continue".
                         */

                        m_midiclockrunning = false;
                        all_notes_off();

#ifdef SEQ64_STAZED_JACK_SUPPORT

                        /*
                         * inner_stop(true) = m_usemidiclock = true, i.e.
                         * hold m_tick position(output_func).  Set the
                         * position to last location on stop, for continue.
                         */

                        inner_stop(true);
                        m_midiclockpos = m_tick;
#endif
                    }
                    else if (ev.get_status() == EVENT_MIDI_CLOCK)
                    {
                        if (m_midiclockrunning)
                            m_midiclocktick += 8;   // a true constant?
                    }
                    else if (ev.get_status() == EVENT_MIDI_SONG_POS)
                    {
                        midibyte a, b;
                        ev.get_data(a, b);

#ifdef SEQ64_STAZED_JACK_SUPPORT                      /* see notes in banner */
                        m_midiclockpos = combine_bytes(a,b);
                        m_midiclockpos *= 48;
#else
                        m_midiclockpos = (int(a) << 7) && int(b);
#endif
                    }

                    /*
                     *  Filter system-wide messages.  If the master MIDI buss
                     *  is dumping, set the timestamp of the event and stream
                     *  it on the sequence.  Otherwise, use the event data to
                     *  control the sequencer, if it is valid for that action.
                     */

                    if (ev.get_status() <= EVENT_MIDI_SYSEX)
                    {
                        if (rc().show_midi())
                            ev.print();

                        /*
                         * "Dumping" is set when a seqedit window is open and
                         * the user has clicked the "record MIDI" or "thru
                         * MIDI" button.  In this case, if the seq32 support
                         * is in force, dump to it, else stream the event,
                         * with possibly multiple sequences set.  Otherwise,
                         * handle an incoming MIDI control event.
                         */

                        if (master_bus().is_dumping())
                        {
                            ev.set_timestamp(m_tick);
#ifdef USE_STAZED_MIDI_DUMP
                            master_bus().dump_midi_input(ev);
#else
                            master_bus().get_sequence()->stream_event(ev);
#endif
                        }
                        else            /* use it to control our sequencer */
                        {
                            midi_control_event(ev);     /* replaces big block */
                        }
                    }
                    if (ev.get_status() == EVENT_MIDI_SYSEX)
                    {
                        if (rc().show_midi())
                            ev.print();

                        if (rc().pass_sysex())
                            master_bus().sysex(&ev);
                    }
                }
            } while (master_bus().is_more_input());
        }
    }
    pthread_exit(0);
}

#ifdef SEQ64_STAZED_JACK_SUPPORT

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
   return short_14bit;
}

#endif  // SEQ64_STAZED_JACK_SUPPORT

/**
 *  For all active patterns/sequences, this function gets the playing
 *  status and saves it in m_sequence_state[i].  Inactive patterns get the
 *  value set to false.  Used in unsetting the snapshot status
 *  (c_status_snapshot).
 */

void
perform::save_playing_state ()
{
    /*
     * EXPERIMENTAL:
     *
     * for (int s = 0; s < m_sequence_max; ++s)
     */

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
    /*
     * EXPERIMENTAL:
     *
     * for (int s = 0; s < m_sequence_max; ++s)
     */

    for (int s = 0; s < m_sequence_high; ++s)       /* modest speed-up */
    {
        if (is_active(s))
            m_seqs[s]->set_playing(m_sequence_state[s]);
    }
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
 * \param status
 *      The status to be used.
 */

void
perform::unset_sequence_control_status (int status)
{
    if (status & c_status_snapshot)
        restore_playing_state();

    m_control_status &= ~status;
}

/**
 *  If the given sequence is active, then it is toggled.  If the
 *  m_control_status is c_status_queue, then the sequence's toggle_queued()
 *  function is called.  Otherwise, if it is c_status_replace, then the status
 *  is unset, and all sequences (?) are turned off.  Then the sequence's
 *  toggle-playing() function is called.
 *
 *  This function is called in sequence_key() to implement a toggling of the
 *  sequence of the pattern slot in the current screen-set that is represented
 *  by the keystroke.
 *
 *  This function is also called in midi_control_event() if the control number
 *  represents a sequence number in a screen-set, that is, it ranges from 0 to
 *  31.  This value is offset by the current screen-set number, m_offset
 *  before passing it to this function.
 *
 * \param seq
 *      The sequence number of the sequence to be potentially toggled.
 *      This value must be a valid and active sequence number.
 */

void
perform::sequence_playing_toggle (int seq)
{
    if (is_active(seq))
    {
        if (m_control_status & c_status_queue)
        {
            m_seqs[seq]->toggle_queued();
        }
        else
        {
            if ((m_control_status & c_status_replace) != 0)
            {
                unset_sequence_control_status(c_status_replace);
                off_sequences();
            }
            m_seqs[seq]->toggle_playing();
        }
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
        m_mode_group && (m_playing_screen == m_screenset) &&
        (seq >= m_playscreen_offset) && (seq < next_offset)
    );
}

/**
 *  Turn the playing of a sequence on or off, if it is active.  Used for the
 *  implementation of sequence_playing_on() and sequence_playing_off().
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
    if (is_active(seq))
    {
        if (seq_in_playing_screen(seq))
            m_tracks_mute_state[seq - m_playscreen_offset] = on;

        bool queued = m_seqs[seq]->get_queued();
        bool playing = m_seqs[seq]->get_playing();
        if (on)
            playing = ! playing;

        if (playing)
        {
            if ((m_control_status & c_status_queue) != 0)
            {
                if (! queued)
                    m_seqs[seq]->toggle_queued();
            }
            else
                m_seqs[seq]->set_playing(on);
        }
        else
        {
            if (queued && (m_control_status & c_status_queue) != 0)
                m_seqs[seq]->toggle_queued();
        }
    }
}

/*
 * Non-inline encapsulation functions start here.
 */

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
    seq += m_screenset * m_seqs_in_set;
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
        int bus = seq.get_midi_bus();
        int chan = seq.is_smf_0() ? 0 : seq.get_midi_channel() + 1;
        int bpb = int(seq.get_beats_per_bar());
        int bw = int(seq.get_beat_width());
        if (show_ui_sequence_number())                  /* new feature! */
            snprintf(tmp, sizeof tmp, "%-3d%d-%d %d/%d", sn, bus, chan, bpb, bw);
        else
            snprintf(tmp, sizeof tmp, "%d-%d %d/%d", bus, chan, bpb, bw);

        result = std::string(tmp);
    }
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
perform::set_input_bus (int bus, bool active)
{
    if (bus >= SEQ64_DEFAULT_BUSS_MAX)                  /* 32 busses    */
    {
        if (bus == PERFORM_KEY_LABELS_ON_SEQUENCE)
            show_ui_sequence_key(active);
        else if (bus == PERFORM_NUM_LABELS_ON_SEQUENCE)
            show_ui_sequence_number(active);

        for (int seq = 0; seq < m_sequence_max; seq++)  /* m_sequence_high */
        {
            sequence * s = get_sequence(seq);
            if (not_nullptr(s))
                s->set_dirty();
        }
    }
    else if (bus >= 0)
    {
        if (master_bus().set_input(bus, active))
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
 *      e_clock_off, e_clock_pos, and e_clock_mod.
 */

void
perform::set_clock_bus (int bus, clock_e clocktype)
{
    if (master_bus().set_clock(bus, clocktype))     /* checks bus index, too */
        set_clock(bus, clocktype);
}

/**
 *  Gets the event key for the given sequence.  If we're not in legacy mode,
 *  then we adjust for the screenset, so that screensets greater than 0 can
 *  also show the correct key name, instead of a question mark.
 *
 *  Legacy seq24 already responds to the toggling of the mute state via the
 *  shortcut keys even if screenset > 0, but it shows the question mark.
 *
 * \param seqnum
 *      The number of the sequence for which to return the event key.
 *
 * \return
 *      Returns the desired key.  If there is no such value, then the
 *      period ('?') character is returned.
 */

unsigned int
perform::lookup_keyevent_key (int seqnum)
{
    unsigned int result = (unsigned int)('?');
    if (! rc().legacy_format())
        seqnum -= m_offset;

    if (get_key_events_rev().count(seqnum) > 0)
        result = get_key_events_rev()[seqnum];
    else
        result = '?';                 /* '.' */

    return result;
}

/**
 *  Provided for mainwnd :: on_key_press_event() and mainwnd ::
 *  on_key_release_event() to call.  This function handles the keys for the
 *  functions of replace, queue, keep-queue, snapshots, toggling mute groups,
 *  group learn, and playing screenset.  For further keystroke processing, see
 *  mainwnd :: on_key_press_event().
 *
 *  Keys not handled here are handled in mainwnd:  bpm up & down; screenset up
 *  & down.
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
    unsigned int key = k.key();
    if (k.is_press())
    {
        /*
         * Also not handled here:  mute group key and mute group learn.
         */

        if (key == keys().replace())
            set_sequence_control_status(c_status_replace);
        else if (key == keys().queue() || key == keys().keep_queue())
            set_sequence_control_status(c_status_queue);
        else if (key == keys().snapshot_1() || key == keys().snapshot_2())
            set_sequence_control_status(c_status_snapshot);
        else if (key == keys().set_playing_screenset())
            set_playing_screenset();
        else if (key == keys().group_on())
            set_mode_group_mute();              /* m_mode_group = true */
        else if (key == keys().group_off())
            unset_mode_group_mute();
        else if (key == keys().group_learn())
            set_mode_group_learn();
        else
            result = false;
    }
    else
    {
        if (key == keys().replace())
            unset_sequence_control_status(c_status_replace);
        else if (key == keys().queue())
            unset_sequence_control_status(c_status_queue);
        else if (key == keys().snapshot_1() || key == keys().snapshot_2())
            unset_sequence_control_status(c_status_snapshot);
        else if (key == keys().group_learn())
            unset_mode_group_learn();
        else
            result = false;
    }
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
        if (is_active(drop_sequence))
        {
            sequence * s = get_sequence(drop_sequence);
            if (k.is_delete())
            {
                push_trigger_undo();
                s->del_selected_trigger();
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
    bool result = OR_EQUIVALENT(k.key(), keys().start(), keys().stop());
    if (! result)
        result = k.key() == keys().pause();

    if (result)
    {
        bool onekey = keys().start() == keys().stop();

#ifdef USE_CONSOLIDATED_PLAYBACK

        playback_action_t action = PLAYBACK_STOP;
        if (k.key() == keys().start())
        {
            if (onekey)
            {
                if (is_running())
                    action = PLAYBACK_PAUSE;            // why pause, not stop?
                else
                    action = PLAYBACK_START;
            }
            else if (! is_running())
                action = PLAYBACK_START;
        }
        else if (k.key() == keys().pause())
            action = PLAYBACK_PAUSE;

#else   // USE_CONSOLIDATED_PLAYBACK

        bool isplaying = false;
        if (k.key() == keys().start())
        {
            if (onekey)
            {
                if (is_running())
                {
                    pause_playing(songmode);            // why pause, not stop?
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
        else if (k.key() == keys().stop())
        {
            stop_playing();
        }
        else if (k.key() == keys().pause())
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

#endif  // USE_CONSOLIDATED_PLAYBACK

    }
    return result;
}

#ifdef USE_CONSOLIDATED_PLAYBACK

/**
 *  More rational new  function provided to unify the stop/start
 *  (space/escape) behavior of the various windows where playback can be
 *  started, paused, or stopped.  To be used in mainwnd, perfedit, and
 *  seqroll.  We want this function to be the one maintaining the various
 *  flags, if possible:  m_start_from_perfedit (seq32) and m_is_pattern_playing
 *  at a minimum.
 *
 * \param p
 *      Provides the playback action to perform.
 *
 * \param songmode
 *      Provides the "jack flag" needed by the mainwnd, seqroll, and perfedit
 *      windows.  Defaults to false, which disables Song mode, and enables
 *      Live mode.  But using Song mode seems to make the Pause key not work
 *      in the performance editor.
 *
 * \return
 *      Returns true if the patterns are playing, as opposed to not playing,
 *      by the end of this function.
 *
 * \sideeffect
 *      The m_is_pattern_playing flag is set to the return value for the
 *      caller.
 */

bool
perform::playback_action (playback_action_t p, bool songmode)
{
    bool isplaying = false;
    if (p == PLAYBACK_START)
    {
        if (! is_running())             /* what about a restart???? */
        {
            start_playing(songmode);
            isplaying = true;
        }
    }
    else if (p == PLAYBACK_STOP)
    {
        stop_playing();
    }
    else if (p == PLAYBACK_PAUSE)
    {
        if (is_running())
        {
            pause_playing(songmode);
        }
        else
        {
            start_playing(songmode);
            isplaying = true;
        }
    }
    is_pattern_playing(isplaying);
    return isplaying;
}

#endif  // USE_CONSOLIDATED_PLAYBACK

/**
 *  Shows all the triggers of all the sequences.
 */

void
perform::print_triggers () const
{
    for (int s = 0; s < m_sequence_max; ++s)    /* m_sequence_high */
    {
        if (is_active(s))
            m_seqs[s]->print_triggers();
    }
}

#ifdef SEQ64_STAZED_TRANSPOSE

/**
 *  Calls the apply_song_transpose() function for all active sequences.
 */

void
perform::apply_song_transpose ()
{
    /*
     * EXPERIMENTAL:
     *
     * for (int s = 0; s < m_sequence_max; ++s)
     */

    for (int s = 0; s < m_sequence_high; ++s)       /* modest speed-up */
    {
        if (is_active(s))
            get_sequence(s)->apply_song_transpose();
    }
}

#endif      // SEQ64_STAZED_TRANSPOSE

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

    /*
     * EXPERIMENTAL:
     *
     * for (int s = 0; s < m_sequence_max; ++s)
     */

    for (int s = 0; s < m_sequence_high; ++s)       /* modest speed-up */
    {
        if (is_active(s))
            result = s;
    }
    if (result >= 0)
        result = result / m_seqs_in_set;

    return result;
}

#ifdef SEQ64_STAZED_JACK_SUPPORT

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
            tick = m_tick - measure_ticks;
            if (tick < 0)
                tick = 0;
        }
        else                    // if (m_FF_RW_button_type == FF_RW_FORWARD)
            tick = m_tick + measure_ticks;
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

#endif  // SEQ64_STAZED_JACK_SUPPORT

#ifdef PLATFORM_DEBUG_XXX

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

#endif      // PLATFORM_DEBUG_XXX

}           // namespace seq64

/*
 * perform.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

