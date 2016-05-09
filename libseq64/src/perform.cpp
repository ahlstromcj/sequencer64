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
 *  This module defines the base class for the performance mode.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2016-05-08
 * \license       GNU GPLv2 or above
 *
 *  This class is probably the single most important class in Sequencer64, as
 *  it supports sequences, playback, JACK, and more.
 */

#include <sched.h>
#include <stdio.h>

#ifndef PLATFORM_WINDOWS
#include <time.h>                       /* struct timespec                  */
#endif

#include "calculations.hpp"
#include "event.hpp"
#include "keystroke.hpp"
#include "midibus.hpp"
#include "perform.hpp"

/**
 *  Provide a "not-a-number" macro.
 * -nan(0x8000000000000)
 */

#define SEQ64_JACK_NAN      0x8000000000000

namespace seq64
{

/**
 *  Purely internal constants used with the functions that implement MIDI
 *  control for the application.  Note how they specify different bit values,
 *  as it they could be masked together to signal multiple functions.
 *
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
    m_gui_support               (mygui),
    m_mute_group                (),         // boolean array
    m_tracks_mute_state         (),         // boolean array
    m_mode_group                (true),
    m_mode_group_learn          (false),
    m_mute_group_selected       (0),
    m_playing_screen            (0),
    m_playscreen_offset         (0),
    m_seqs                      (),         // pointer array
    m_seqs_active               (),         // boolean array
    m_was_active_main           (),         // boolean array
    m_was_active_edit           (),         // boolean array
    m_was_active_perf           (),         // boolean array
    m_was_active_names          (),         // boolean array
    m_sequence_state            (),         // boolean array
    m_master_bus                (),         // will call its init() later
    m_out_thread                (),
    m_in_thread                 (),
    m_out_thread_launched       (false),
    m_in_thread_launched        (false),
    m_running                   (false),
    m_inputing                  (true),
    m_outputing                 (true),
    m_looping                   (false),
    m_playback_mode             (false),
    m_ppqn                      (choose_ppqn(ppqn)),
    m_beats_per_bar             (SEQ64_DEFAULT_BEATS_PER_MEASURE),
    m_beat_width                (SEQ64_DEFAULT_BEAT_WIDTH),
    m_one_measure               (m_ppqn * 4),
    m_left_tick                 (0),
    m_right_tick                (m_one_measure * 4),        // m_ppqn * 16
    m_starting_tick             (0),
    m_tick                      (0),
#ifdef SEQ64_PAUSE_SUPPORT
    m_jack_tick                 (0),
#endif
    m_usemidiclock              (false),
    m_midiclockrunning          (false),
    m_midiclocktick             (0),
    m_midiclockpos              (-1),
    m_is_paused                 (false),
    m_screen_set_notepad        (),         // string array of size c_max_sets
    m_midi_cc_toggle            (),         // midi_control array
    m_midi_cc_on                (),         // midi_control array
    m_midi_cc_off               (),         // midi_control array
    m_offset                    (0),
    m_control_status            (0),
    m_screenset                 (0),
    m_seqs_in_set               (c_seqs_in_set),
    m_max_sets                  (c_max_sets),
    m_sequence_count            (0),
    m_sequence_max              (c_max_sequence),
    m_edit_sequence             (-1),
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
    m_notify                    ()          // vector of pointers, public!
{
    for (int i = 0; i < m_sequence_max; ++i)
    {
        m_seqs[i] = nullptr;

        /*
         * seq24 0.9.3 (!) added initialization of these.
         */

        m_seqs_active[i] = m_was_active_main[i] = m_was_active_edit[i] =
            m_was_active_perf[i] = m_was_active_names[i] = false;
    }
    midi_control zero;                  /* all members false or 0   */
    for (int i = 0; i < c_midi_controls; ++i)
    {
        m_midi_cc_toggle[i] = zero;
        m_midi_cc_on[i] = zero;
        m_midi_cc_off[i] = zero;
    }
    set_all_key_events();
    set_all_key_groups();
}

/**
 *  The destructor sets some running flags to false, signals this condition,
 *  then joins the input and output threads if the were launched. Finally, any
 *  active or inactive (but allocated) patterns/sequences are deleted, and
 *  their pointers nullified.
 */

perform::~perform ()
{
    m_inputing = m_outputing = m_running = false;
    m_condition_var.signal();                   /* signal the end of play   */
    if (m_out_thread_launched)
        pthread_join(m_out_thread, NULL);

    if (m_in_thread_launched)
        pthread_join(m_in_thread, NULL);

    for (int seq = 0; seq < m_sequence_max; ++seq)
    {
        if (not_nullptr(m_seqs[seq]))
        {
            delete m_seqs[seq];
            m_seqs[seq] = nullptr;              /* not strictly necessary   */
        }
    }
}

/**
 *  Calls the MIDI buss and JACK initialization functions and the input/output
 *  thread-launching functions.  This function is called in main().  We
 *  collected all the calls here as a simplification, and renamed it because
 *  it is more than just initialization.  This function must be called after
 *  the perform constructor and after the configuration file and command-line
 *  configuration overrides.
 *
 * \param ppqn
 *      Provides the PPQN value, which is either the default value (192) or is
 *      read from the "user" configuration file.
 */

void
perform::launch (int ppqn)
{
    m_master_bus.init(ppqn);
    launch_input_thread();
    launch_output_thread();
#ifdef SEQ64_JACK_SUPPORT
    init_jack();
#endif
}

/**
 *  Clears all of the patterns/sequences.  The mainwnd module calls this
 *  function.  Note that perform now handles the "is modified" flag on behalf
 *  of all external objects, to centralize and simplify the dirtying of a MIDI
 *  tune.
 *
 *  Anything else to clear?  What about all the other sequence flags?  We can
 *  beef up delete_sequence() for them, at some point.
 */

void
perform::clear_all ()
{
    reset_sequences();
    for (int i = 0; i < m_sequence_max; ++i)
        if (is_active(i))
            delete_sequence(i);             /* can set "is modified"    */

    std::string e;                          /* an empty string          */
    for (int i = 0; i < m_max_sets; ++i)
        set_screen_set_notepad(i, e);

    is_modified(false);                     /* new, we start afresh     */
}

/**
 *  Provides common code to keep the track value valid.  Note the bug we
 *  found, where we checked for track > m_seqs_in_set, but set it to
 *  m_seqs_in_set - 1 in that case!
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
    else if (track >= m_seqs_in_set)        /* bug: was just ">" !!! */
        track = m_seqs_in_set - 1;

    return track;
}

/**
 *  This function sets the mute state of an element in the m_mute_group array.
 *  The index value is the track number offset by the number of the selected
 *  mute group (which seems equivalent to a set number) times the number of
 *  sequences in a set.
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
    int index = mute_group_offset(gtrack);
    m_mute_group[index] = muted;
}

/**
 *  The opposite of set_group_mute_state(), it gets the value of the desired
 *  track.  Uses the mute_group_offset function.
 *
 * \param gtrack
 *      The number of the track for which the state is to be obtained.
 *      Like set_group_mute_state(), this value is offset by adding
 *      m_mute_group_selected * m_seqs_in_set.
 *
 * \return
 *      Returns the value of m_mute_group[gtrack + set offset]
 */

bool
perform::get_group_mute_state (int gtrack)
{
    int index = mute_group_offset(gtrack);
    return m_mute_group[index];
}

/**
 *  Makes some checks on all of the active sequences, and sets the group mute
 *  flag, m_mute_group_selected, to the clamped g-mute value.  Null sequences
 *  are no longer flagged as an error, they are just ignored.
 *
 * \param a_g_mute
 *      The number of the mute group, clamped to be between 0 and
 *      m_seqs_in_set-1.
 */

void
perform::select_group_mute (int a_g_mute)
{
    int gmute = clamp_track(a_g_mute);
    int j = gmute * m_seqs_in_set;
    int k = m_playscreen_offset;
    if (m_mode_group_learn)
    {
        for (int i = 0; i < m_seqs_in_set; ++i)
        {
            if (is_active(i + k))
                m_mute_group[i + j] = m_seqs[i+k]->get_playing();
        }
    }
    m_mute_group_selected = gmute;
}

/**
 *  Sets the group-mute mode, then the group-learn mode, then notifies all of
 *  the notification subscribers.
 */

void
perform::set_mode_group_learn ()
{
    set_mode_group_mute();
    m_mode_group_learn = true;
    for (size_t x = 0; x < m_notify.size(); ++x)
        m_notify[x]->on_grouplearnchange(true);
}

/**
 *  Notifies all of the notification subscribers that group-learn is being
 *  turned off.  Then unsets the group-learn mode flag.
 */

void
perform::unset_mode_group_learn ()
{
    for (size_t x = 0; x < m_notify.size(); ++x)
        m_notify[x]->on_grouplearnchange(false);

    m_mode_group_learn = false;
}

/**
 *  Makes some checks and sets the group mute flag, m_mute_group_selected, to
 *  the clamped g-mute value, if all goes well (no null sequences are
 *  encountered).  Null sequences are no longer flagged as an error, they are
 *  just ignored.
 *
 *  Will need to study this one more closely.
 *
 * \param a_group
 *      Provides the group to mute.  Note that this parameter is essentially a
 *      track or sequence number.
 */

void
perform::select_mute_group (int a_group)
{
    int group = clamp_track(a_group);
    int j = group * m_seqs_in_set;

    /*
     * \change tdeagan 2015-12-22 via git pull:
     *
     * git checkout -b TDeagan-mute_groups master
     * git pull https://github.com/TDeagan/sequencer64.git mute_groups
     */

    int k = m_screenset * m_seqs_in_set;    /* replaces m_playscreen_offset */
    m_mute_group_selected = group;
    for (int i = 0; i < m_seqs_in_set; ++i)
    {
        if ((m_mode_group_learn) && (is_active(i + k)))
            m_mute_group[i + j] = m_seqs[i + k]->get_playing();

        int index = mute_group_offset(i);
        m_tracks_mute_state[i] = m_mute_group[index];
    }
}

/**
 *  Will need to study this one more closely.
 *
 * \change 2016-05-06
 *      It seems to us that the for (i) clause should have i range from 0 to
 *      m_max_sets, not m_seqs_in_set.  So let's do it, pre-emptively.
 */

void
perform::mute_group_tracks ()
{
    if (m_mode_group)
    {
        for (int i = 0; i < m_max_sets; ++i)            // see note in banner
        {
            for (int j = 0; j < m_seqs_in_set; ++j)
            {
                int seqnum = i * m_seqs_in_set + j;
                if (is_active(seqnum))
                {
    /*
     * \change tdeagan 2015-12-22 via git pull.  Replaced m_playing_screen
     *      with m_screenset.
     */
                    if ((i == m_screenset) && m_tracks_mute_state[j])
                        sequence_playing_on(seqnum);
                    else
                        sequence_playing_off(seqnum);
                }
            }
        }
    }
}

/**
 *  Select a mute group and then mutes the track in the group.
 *
 * \param group
 *      Provides the group number for the group to be muted.
 */

void
perform::select_and_mute_group (int group)
{
    select_mute_group(group);
    mute_group_tracks();
}

/**
 *  Mutes all tracks in the current set of active patterns/sequences.
 *  Covers tracks from 0 to m_sequence_max.
 */

void
perform::mute_all_tracks ()
{
    for (int i = 0; i < m_sequence_max; ++i)
    {
        if (is_active(i))
            m_seqs[i]->set_song_mute(true);
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
            "install_sequence(): m_seqs[%d] not null, removing old sequence\n",
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
#if SEQ64_PAUSE_SUPPORT
        seq->set_parent(this);
#endif
        ++m_sequence_count;
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
                    m_seqs[seq]->set_master_midi_bus(&m_master_bus);
                    modify();
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
            if (not_nullptr(m_seqs[seq]))
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
            if (not_nullptr(m_seqs[seq]))
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
            if (not_nullptr(m_seqs[seq]))
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
            if (not_nullptr(m_seqs[seq]))
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
        m_master_bus.set_beats_per_minute(bpm);
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
    if (seq >= 0 && seq < m_sequence_max)
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
            errprintf("is_mseq_valid(): m_seqs[%d] is null\n", seq);
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
 *  Retrieves a reference to a value from m_midi_cc_toggle[].
 *
 * \param seq
 *      Provides a control value (such as c_midi_control_bpm_up) to use to
 *      retrieve the desired midi_control object.  Note that this value is
 *      unsigned simply to make the legality check of the parameter
 *      easier.
 *
 * \return
 *      Returns the "toggle" value if the sequence is valid, and a reference
 *      to sm_mc_dummy otherwise.
 */

midi_control &
perform::midi_control_toggle (int seq)
{
    return is_midi_control_valid(seq) ? m_midi_cc_toggle[seq] : sm_mc_dummy ;
}

/**
 *  Retrieves a reference to a value from m_midi_cc_on[].
 *
 * \param seq
 *      Provides a control value (such as c_midi_control_bpm_up) to use to
 *      retrieve the desired midi_control object.
 *
 * \return
 *      Returns the "on" value if the sequence is valid, and a reference
 *      to sm_mc_dummy otherwise.
 */

midi_control &
perform::midi_control_on (int seq)
{
    return is_midi_control_valid(seq) ? m_midi_cc_on[seq] : sm_mc_dummy ;
}

/**
 *  Retrieves a reference to a value from m_midi_cc_off[].
 *
 * \param seq
 *      Provides a control value (such as c_midi_control_bpm_up) to use to
 *      retrieve the desired midi_control object.
 *
 * \return
 *      Returns the "off" value if the sequence is valid, and a reference
 *      to sm_mc_dummy otherwise.
 */

midi_control &
perform::midi_control_off (int seq)
{
    return is_midi_control_valid(seq) ? m_midi_cc_off[seq] : sm_mc_dummy ;
}

/**
 *  Copies the given string into m_screen_set_notepad[].
 *
 * \param screenset
 *      The ID number of the string set, an index into the
 *      m_screen_set_xxx[] arrays.
 *
 * \param notepad
 *      Provides the string date to copy into the notepad.
 *      Not sure why a pointer is used, instead of nice "const std::string
 *      &" parameter.  And this pointer isn't checked.  Fixed.
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
 *      The ID number of the string set, an index into the
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
 * \param ss
 *      The index of the desired string set.  It is forced to range from
 *      0 to m_max_sets - 1.
 */

void
perform::set_screenset (int ss)
{
    if (ss < 0)
        ss = m_max_sets - 1;
    else if (ss >= m_max_sets)
        ss = 0;

    if (ss != m_screenset)
        m_screenset = ss;
}

/**
 *  Sets the screen set that is active, based on the value of
 *  m_playing_screen.
 *
 *  For each value up to m_seqs_in_set (32), the index of the current
 *  sequence in the currently screen set (m_playing_screen) is obtained.
 *  If it is active and the sequence actually exists.  Null sequences are no
 *  longer flagged as an error, they are just ignored.
 *
 *  Modifies m_playing_screen, and mutes the group tracks.
 */

void
perform::set_playing_screenset ()
{
    for (int i = 0; i < m_seqs_in_set; ++i)
    {
        int j = i + m_playscreen_offset;    /* m_playing_screen*m_seqs_in_set */
        if (is_active(j))
            m_tracks_mute_state[i] = m_seqs[j]->get_playing();
    }
    m_playing_screen = m_screenset;
    m_playscreen_offset = m_playing_screen * m_seqs_in_set;
    mute_group_tracks();
}

/**
 *  Starts the playing of all the patterns/sequences.  This function just runs
 *  down the list of sequences and has them dump their events.  It skips
 *  sequences that have no playable MIDI events.
 *
 * \param tick
 *      Provides the tick at which to start playing.
 */

void
perform::play (midipulse tick)
{
    m_tick = tick;
    for (int i = 0; i < m_sequence_max; ++i)
    {
        if (is_active(i))
        {
            sequence * s = m_seqs[i];
            if (s->event_count() > 0)               /* playable events? */
            {
                if (s->check_queued_tick(tick))
                {
                    s->play(s->get_queued_tick() - 1, m_playback_mode);
                    s->toggle_playing();
                }
                s->play(tick, m_playback_mode);
            }
        }
    }
    m_master_bus.flush();                           /* flush MIDI buss  */
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
    for (int s = 0; s < m_sequence_max; ++s)
    {
        if (is_active(s))
            m_seqs[s]->set_last_tick(tick);
    }
}

/**
 *  Clears the patterns/sequence for the given sequence, if it is active.
 *
 * \param seq
 *      Provides the desired sequence.  Hopefull, the is_active() function
 *      validates this value.
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
        for (int i = 0; i < m_sequence_max; ++i)
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
 */

void
perform::push_trigger_undo ()
{
    for (int i = 0; i < m_sequence_max; ++i)
    {
        if (is_active(i))
            m_seqs[i]->push_trigger_undo();
    }
}

/**
 *  For every active sequence, call that sequence's pop_trigger_undo()
 *  function.
 */

void
perform::pop_trigger_undo ()
{
    for (int i = 0; i < m_sequence_max; ++i)
    {
        if (is_active(i))
            m_seqs[i]->pop_trigger_undo();
    }
}

/**
 *  If the left tick is less than the right tick, then, for each sequence
 *  that is active, its triggers are copied, offset by the difference
 *  between the right and left.
 *
 *  This copies the triggers between the L marker and R marker to the R
 *  marker.
 */

void
perform::copy_triggers ()
{
    if (m_left_tick < m_right_tick)
    {
        midipulse distance = m_right_tick - m_left_tick;
        for (int i = 0; i < m_sequence_max; ++i)
        {
            if (is_active(i))
                m_seqs[i]->copy_triggers(m_left_tick, distance);
        }
    }
}

/**
 *  Encapsulates a series of calls used in mainwnd.  We've reversed the
 *  start() and start_jack() calls so that JACK is started first, to match all
 *  of the other use-cases for playing that we've found in the code.
 *
 *  Note that the complementary function, stop_playing(), is an inline
 *  function defined in the header file.
 *
 * \note
 *      It would be nice to know why the following code snippet disables the
 *      mute/unmute functionality of the performance/song editor:
 *
\verbatim
        position_jack(false);
        start_jack();
        start(false);
\endverbatim
 *
 *      The jack_assistant::position() function doesn't use the boolean
 *      parameter at present; that code is effectively disabled.
 *
 *      Okay, now it does, if the "relocate" parameter is true.  See
 *      perform::position_jack() and jack_assistant::position().  This
 *      parameter, when true, allows the "klick" application to get
 *      proper position data.
 *
 *      The perform::start() function passes its boolean flag to
 *      perform::inner_start(), which sets the playback mode to that flag; if
 *      that flag is false, that turns off "song" mode.  So that explains why
 *      mute/unmute is disabled.
 *
 *  -   seqroll and mainwnd: position_jack(false); start(false); start_jack().
 *  -   perfedit: position_jack(true); start_jack(); start(true).
 *
 * \param songmode
 *      Indicates if the caller wants to start the playback in JACK mode.
 *      In the seq42 (yes, "42", not "24") code at GitHub, this flag was
 *      identical to the "global_jack_start_mode" flag, which is true for
 *      Song mode, and false for Live mode.  False disables Song mode, and
 *      is the default, which matches seq24.  If the previous state was
 *      "paused", then we start it in Live mode, which works because Song
 *      mode has already turned on the sequences.  This method is not quite
 *      intuitive, because it is really following Live mode.
 */

void
perform::start_playing (bool songmode)
{
    if (songmode)
    {
        position_jack(true);
        start_jack();
        start(true);
    }
    else
    {
        position_jack(false);
        start(false);
        start_jack();
    }

    /*
     * Let's let the output() function clear this, so that we can use this
     * flag in that function to control the next tick to play at resume time.
     *
     *      m_is_paused = false;
     *
     * Shouldn't this be needed as well?  It is set in ALSA mode, but not JACK
     * mode. But DO NOT call set_running() here in JACK mode, it prevents
     * Sequencer64 from starting JACK transport!
     */

    if (! is_jack_running())
        set_running(true);

    rc().is_pattern_playing(true);      /* cannot deprecate this flag yet   */
}

/**
 *  Pause playback, so that progress bars stay where they are, and playback
 *  always resumes where it left off, even in ALSA mode.
 *
 *  Currently almost the same as stop_playing(), but expanded as noted in the
 *  comments so that we ultimately have more granular control over what
 *  happens.  We're researching the whole sequence of stopping and starting,
 *  and it can be tricky to make correct changes.
 *
 *  We still need to make restarting pick up at the same place in ALSA mode;
 *  in JACK mode, JACK transport takes care of that feature.
 */

void
perform::pause_playing ()
{
#ifdef SEQ64_JACK_SUPPORT
    m_jack_asst.stop();                 /* stop_jack()                  */
    if (! is_jack_running())            /* stop() { inner_stop(); }     */
    {
#endif
        set_running(false);
        reset_sequences(true);          /* reset "last-tick" for pause  */
        m_usemidiclock = false;
#ifdef SEQ64_JACK_SUPPORT
    }
#endif
    m_is_paused = true;
    rc().is_pattern_playing(false);     /* cannot yet deprecate this    */
}

/**
 *  Encapsulates a series of calls used in mainwnd.  Stops playback,
 *  turns off the (new) m_is_paused flag, and set the "is-pattern-playing"
 *  flag to false.  Do we need to reset the m_running flag here?
 */

void
perform::stop_playing ()
{
    stop_jack();
    stop();
    m_is_paused = false;
    rc().is_pattern_playing(false);
}

/**
 *  If JACK is supported and running, sets the position of the transport.
 *
 * If we run "klick -j -P" and then start Sequencer64, we get:
 *
 *      klick: src/metronome_jack.cc:52: virtual void
 *      MetronomeJack::process_callback(sample_t*, nframes_t): Assertion
 *      `pos.beat > 0 && pos.beat <= pos.beats_per_bar' failed.
 *
 * Let's try enabling the relocate parameter if we're JACK Master.  This
 * removes the error from klick, but it clicks at an extremely fast rate!
 */

void
perform::position_jack (bool state)
{
#ifdef SEQ64_JACK_SUPPORT
    m_jack_asst.position(state, rc().with_jack_master());
#endif
}

/**
 *  If JACK is not running, call inner_start() with the given state.
 *
 * \param state
 *      What does this state mean?
 */

void
perform::start (bool state)
{
#ifdef SEQ64_JACK_SUPPORT
    if (! is_jack_running())
#endif
        inner_start(state);
}

/**
 *  If JACK is not running, call inner_stop().
 *
 *  The logic seems backward here, in that we call inner_stop() if JACK is
 *  not running.  Or perhaps we misunderstand the meaning of m_jack_running?
 */

void
perform::stop ()
{
#ifdef SEQ64_JACK_SUPPORT
    if (! is_jack_running())
#endif
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
 *      beginning of the sequence, even if just pausing.
 *
 * \param state
 *      Sets the playback mode, and, if true, turns off all of the sequences.
 */

void
perform::inner_start (bool state)
{
    m_condition_var.lock();
    if (! is_running())
    {
        set_playback_mode(state);
        if (state)
            off_sequences();

        set_running(true);
        m_condition_var.signal();
    }
    m_condition_var.unlock();
}

/**
 *  Unconditionally, and without locking, clears the running status,
 *  resets the sequences, and sets m_usemidiclock false.  Note that we do need
 *  to set the running flag to false here, even when JACK is running.
 *  Otherwise, JACK starts ping-ponging back and forth between positions under
 *  some circumstances.
 *
 *  However, if JACK is running, we do not want to reset the sequences... this
 *  causes the progress bar for each sequence to remove to near the end of the
 *  sequence.
 */

void
perform::inner_stop ()
{
    set_running(false);
    if (! is_jack_running())
        reset_sequences();              /* sets the "last-tick" value   */

    m_usemidiclock = false;
}

/**
 *  For all active patterns/sequences, set the playing state to false.
 */

void
perform::off_sequences ()
{
    for (int i = 0; i < m_sequence_max; ++i)
    {
        if (is_active(i))
            m_seqs[i]->set_playing(false);
    }
}

/**
 *  For all active patterns/sequences, turn off its playing notes.
 *  Then flush the MIDI buss.
 */

void
perform::all_notes_off ()
{
    for (int i = 0; i < m_sequence_max; ++i)
    {
        if (is_active(i))
            m_seqs[i]->off_playing_notes();
    }
    m_master_bus.flush();               /* flush the MIDI buss  */
}

/**
 *  For all active patterns/sequences, get its playing state, turn off the
 *  playing notes, set playing to false, zero the markers, and, if not in
 *  playback mode, restore the playing state.  Note that these calls could be
 *  folded into one member function of the sequence class. [Done!]
 *
 *  Finally, flush the MIDI buss.
 */

void
perform::reset_sequences (bool pause)
{
    if (pause)
    {
        /*
         * Try to prevent notes from lingering on pause.
         * We should also remove the pause parameter from reset.
         */

        for (int s = 0; s < m_sequence_max; ++s)
        {
            if (is_active(s))
                m_seqs[s]->pause();
        }
    }
    else
    {
        for (int s = 0; s < m_sequence_max; ++s)
        {
            if (is_active(s))
                m_seqs[s]->reset(m_playback_mode);
        }
    }
    m_master_bus.flush();                           /* flush the MIDI buss  */
}

/**
 *  Creates the output thread using output_thread_func().
 *
 *  This might be a good candidate for a small thread class derived from a
 *  small base class.
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
 *  Creates the input thread using input_thread_func().
 *
 *  This might be a good candidate for a small thread class derived from a
 *  small base class.
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
 */

void
perform::split_trigger (int seqnum, midipulse tick)
{
    push_trigger_undo();
    get_sequence(seqnum)->split_trigger(tick);
    modify();
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
    for (int i = 0; i < m_sequence_max; ++i)
    {
        if (is_active(i))
        {
            midipulse t = m_seqs[i]->get_max_trigger();
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
 *      output_func() is called.
 */

void *
output_thread_func (void * myperf)
{
#ifndef PLATFORM_WINDOWS                /* Not in MinGW RCB */
    if (rc().priority())
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
    }
#endif

    perform * p = (perform *) myperf;

#ifdef PLATFORM_WINDOWS
    timeBeginPeriod(1);
    p->output_func();
    timeEndPeriod(1);
#else
    p->output_func();
#endif
    return 0;
}

/**
 *  Performance output function.  This function is called by the free function
 *  output_thread_func().  Here's how it works:
 *
 *      -   It runs while m_outputing is true.
 *      -   MORE TO COME
 *
 * \change ca 2016-01-26
 *      Hurray, seq24 is coming back to life!  We see that there is a fix for
 *      clock tick drift here, which relies on using long and long long
 *      values.  See the Changelog for seq24 0.9.3.
 */

void
perform::output_func ()
{
    while (m_outputing)
    {
        m_condition_var.lock();
        while (! m_running)
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
        if (m_is_paused)
        {
#ifdef SEQ64_PAUSE_SUPPORT
            pad.js_current_tick = get_jack_tick();
#endif
            m_is_paused = false;
        }
        else
        {
            pad.js_current_tick = 0.0;      // tick and tick fraction
            pad.js_total_tick = 0.0;
#ifdef USE_SEQ24_0_9_3_CODE
            pad.js_clock_tick = 0;          // long probably offers more ticks
#else
            pad.js_clock_tick = 0.0;        // double
#endif
        }

        pad.js_jack_stopped = false;
        pad.js_dumping = false;
        pad.js_init_clock = true;
        pad.js_looping = m_looping;
        pad.js_playback_mode = m_playback_mode;
        pad.js_ticks_converted_last = 0.0;
#ifdef USE_SEQ24_0_9_3_CODE
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
        if (rc().stats())                   // \new ca 2016-01-24
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
         * starting from the m_starting_tick offset.
         */

#ifdef SEQ64_JACK_SUPPORT
        bool ok = m_playback_mode && ! is_jack_running();
#else
        bool ok = m_playback_mode;
#endif

        if (ok)
        {
            pad.js_current_tick = long(m_starting_tick);    // midipulse
            pad.js_clock_tick = m_starting_tick;
            set_orig_ticks(m_starting_tick);                // what member?
        }

        int ppqn = m_master_bus.get_ppqn();

#ifdef SEQ64_STATISTICS_SUPPORT

#ifndef PLATFORM_WINDOWS
        clock_gettime(CLOCK_REALTIME, &last);   // get start time position
        if (rc().stats())
            stats_last_clock_us = (last.tv_sec*1000000) + (last.tv_nsec/1000);
#else
        last = timeGetTime();                   // get start time position
        if (rc().stats())
            stats_last_clock_us = last * 1000;
#endif

#else   // SEQ64_STATISTICS_SUPPORT

#ifndef PLATFORM_WINDOWS
        clock_gettime(CLOCK_REALTIME, &last);   // get start time position
#else
        last = timeGetTime();                   // get start time position
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
#ifndef PLATFORM_WINDOWS
                clock_gettime(CLOCK_REALTIME, &stats_loop_start);
#else
                stats_loop_start = timeGetTime();
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
            int bpm  = m_master_bus.get_beats_per_minute();

            /*
             * Delta time to ticks; get delta ticks.
             *
             * seq24 0.9.3 changes delta_tick's type and adds some code --
             * delta_ticks_frac is in 1000th of a tick.  This code is meant to
             * correct for some clock drift.  However, this code breaks the
             * MIDI clock speed.  So let's revert to our original code, by not
             * defining USE_SEQ24_0_9_3_CODE.  I'm thinking we can just keep
             * the delta_tick value as a double, but accumulate at a higher
             * resolution and calculate delta_tick from that.
             */

#ifdef USE_SEQ24_0_9_3_CODE
            long long delta_tick_num = bpm * ppqn * delta_us +
                pad.js_delta_tick_frac;

            long long delta_tick_denom = 60000000LL;

            /*
             * Store as a double value instead of long.
             *
             *  long delta_tick = long(delta_tick_num / delta_tick_denom);
             *
             * This seems to get truncated.  Convert to double instead.
             *
             *  double ll_delta_tick = double(delta_tick_num / delta_tick_denom);
             *
             * But even with this change, we still get wide variations:
             *
             *  delta_tick: seq24 0.9.3 --> 2.24403, normal --> 1.57286
             *  delta_tick: seq24 0.9.3 --> 1.82496, normal --> 1.58093
             *  delta_tick: seq24 0.9.3 --> 2.40896, normal --> 1.584
             *  delta_tick: seq24 0.9.3 --> 1.98643, normal --> 1.57747
             */

            double ll_delta_tick = double(delta_tick_num) / 60000000.0;
            pad.js_delta_tick_frac = long(delta_tick_num % delta_tick_denom);
#else
            // FYI: delta_tick = double(bpm * ppqn * (delta_us / 60000000.0f));

            double delta_tick = delta_time_us_to_ticks(delta_us, bpm, ppqn);
#endif

#ifdef USE_EXPERIMENTAL_DEBUG_OUTPUT
            printf
            (
                "delta_tick: seq24 0.9.3 --> %g, normal --> %g\n",
                ll_delta_tick, delta_tick
            );
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

            /*
             * init_clock will be true when we run for the first time, or
             * as soon as JACK gets a good lock on playback.
             */

            if (pad.js_init_clock)
            {
                m_master_bus.init_clock(midipulse(pad.js_clock_tick));
                pad.js_init_clock = false;
            }
            if (pad.js_dumping)
            {
                if (m_looping && m_playback_mode)
                {
                    midipulse rtick = get_right_tick();
                    if (pad.js_current_tick >= rtick)
                    {
                        midipulse ltick = get_left_tick();
                        double leftover_tick = pad.js_current_tick - rtick;
                        play(rtick - 1);                            // play!
                        reset_sequences();                          // reset!
                        set_orig_ticks(ltick);
                        pad.js_current_tick = double(ltick) + leftover_tick;
                    }
                }
                play(midipulse(pad.js_current_tick));               // play!

#ifdef SEQ64_PAUSE_SUPPORT
                set_jack_tick(pad.js_current_tick);
#endif
                m_master_bus.clock(midipulse(pad.js_clock_tick));   // MIDI clock

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
#ifndef PLATFORM_WINDOWS
                            long current_us = (current.tv_sec * 1000000) +
                                (current.tv_nsec / 1000);
#else
                            long current_us = current * 1000;
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

#ifndef PLATFORM_WINDOWS
            clock_gettime(CLOCK_REALTIME, &current);
            delta.tv_sec  = current.tv_sec  - last.tv_sec;
            delta.tv_nsec = current.tv_nsec - last.tv_nsec;
            long elapsed_us = (delta.tv_sec * 1000000) + (delta.tv_nsec / 1000);
#else
            current = timeGetTime();
            delta = current - last;
            long elapsed_us = delta * 1000;
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
#ifndef PLATFORM_WINDOWS
                clock_gettime(CLOCK_REALTIME, &stats_loop_finish);
                delta.tv_sec  = stats_loop_finish.tv_sec-stats_loop_start.tv_sec;
                delta.tv_nsec = stats_loop_finish.tv_nsec-stats_loop_start.tv_nsec;
                long delta_us = (delta.tv_sec*1000000) + (delta.tv_nsec/1000);
#else
                stats_loop_finish = timeGetTime();
                delta = stats_loop_finish - stats_loop_start;
                long delta_us = delta * 1000;
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
            int bpm  = m_master_bus.get_beats_per_minute();
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
         * begins again, without some other changes.
         */

#ifdef SEQ64_PAUSE_SUPPORT
        if (is_jack_running())
#endif
            m_tick = 0;

        m_master_bus.flush();
        m_master_bus.stop();
    }
    pthread_exit(0);
}

/**
 *  Set up the performance, and set the process to realtime privileges.
 */

void *
input_thread_func (void * myperf)
{
#ifndef PLATFORM_WINDOWS                // MinGW RCB
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
               "(FIFO), you need to be root.\n"
            );
            pthread_exit(0);
        }
    }
#endif

    perform * p = (perform *) myperf;

#ifdef PLATFORM_WINDOWS
    timeBeginPeriod(1);
    p->input_func();
    timeEndPeriod(1);
#else
    p->input_func();
#endif
    return 0;
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
 */

void
perform::handle_midi_control (int ctrl, bool state)
{
    switch (ctrl)
    {
    case c_midi_control_bpm_up:                 // printf("bpm up\n");
        set_beats_per_minute(get_beats_per_minute() + 1);
        break;

    case c_midi_control_bpm_dn:                 // printf("bpm dn\n");
        set_beats_per_minute(get_beats_per_minute() - 1);
        break;

    case c_midi_control_ss_up:                  // printf("ss up\n");
        set_screenset(get_screenset() + 1);
        break;

    case c_midi_control_ss_dn:                  // printf("ss dn\n");
        set_screenset(get_screenset() - 1);
        break;

    case c_midi_control_mod_replace:            // printf("replace\n");
        if (state)
            set_sequence_control_status(c_status_replace);
        else
            unset_sequence_control_status(c_status_replace);
        break;

    case c_midi_control_mod_snapshot:           // printf("snapshot\n");
        if (state)
            set_sequence_control_status(c_status_snapshot);
        else
            unset_sequence_control_status(c_status_snapshot);
        break;

    case c_midi_control_mod_queue:              // printf("queue\n");
        if (state)
            set_sequence_control_status(c_status_queue);
        else
            unset_sequence_control_status(c_status_queue);

    case c_midi_control_mod_gmute:              // Andy case; printf("gmute\n");
        if (state)
            set_mode_group_mute();
        else
            unset_mode_group_mute();
        break;

    case c_midi_control_mod_glearn:             // Andy case; printf("glearn\n");
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
         * Based on the value of c_midi_track_crl (32 *2) versus
         * m_seqs_in_set (32), maybe the first comparison should be
         * "ctrl >= 2 * m_seqs_in_set".
         */

        if ((ctrl >= m_seqs_in_set) && (ctrl < c_midi_track_ctrl))
            select_and_mute_group(ctrl - m_seqs_in_set);

        break;
    }
}

/**
 *
 *  This function is called by input_thread_func().
 */

void
perform::input_func ()
{
    event ev;
    while (m_inputing)
    {
        if (m_master_bus.poll_for_midi() > 0)
        {
            do
            {
                if (m_master_bus.get_midi_event(&ev))
                {
                    if (ev.get_status() == EVENT_MIDI_START) // MIDI Time Clock
                    {
                        stop();
                        start(false);
                        m_midiclockrunning = true;
                        m_usemidiclock = true;
                        m_midiclocktick = 0;
                        m_midiclockpos = 0;
                    }
                    else if (ev.get_status() == EVENT_MIDI_CONTINUE)
                    {
                        // MIDI continue: start from current position.

                        m_midiclockrunning = true;
                        start(false);
                        // m_usemidiclock = true;   ???
                    }
                    else if (ev.get_status() == EVENT_MIDI_STOP)
                    {
                        /*
                         * Just let the system pause.  Since we're not getting
                         * ticks after the stop, the song won't advance when
                         * start is received, we'll reset the position, or when
                         * continue is received, we won't reset the position.
                         */

                        m_midiclockrunning = false;
                        all_notes_off();
                    }
                    else if (ev.get_status() == EVENT_MIDI_CLOCK)
                    {
                        if (m_midiclockrunning)
                            m_midiclocktick += 8;   // a true constant?
                    }
                    else if (ev.get_status() == EVENT_MIDI_SONG_POS)
                    {
                        midibyte a, b;     // not tested (todo: test it!)
                        ev.get_data(a, b);
                        m_midiclockpos = (int(a) << 7) && int(b);
                    }

                    /*
                     *  Filter system-wide messages.  If the master MIDI buss
                     *  is dumping, set the timestamp of the event and stream
                     *  it on the sequence.  Otherwise, use the event data to
                     *  control the sequencer, if it is valid for that action.
                     */

                    if (ev.get_status() <= EVENT_SYSEX)
                    {
                        if (rc().show_midi())
                            ev.print();

                        if (m_master_bus.is_dumping())
                        {
                            ev.set_timestamp(m_tick);
                            m_master_bus.get_sequence()->stream_event(ev);
                        }
                        else
                        {
                            for (int i = 0; i < c_midi_controls; ++i)
                            {
                                midibyte data[2] = { 0, 0 };
                                midibyte status = ev.get_status();
                                ev.get_data(data[0], data[1]);
                                if (midi_control_toggle(i).match(status, data[0]))
                                {
                                    if (midi_control_toggle(i).in_range(data[1]))
                                    {
                                        if (i < m_seqs_in_set)
                                            sequence_playing_toggle(i+m_offset);
                                    }
                                }
                                if (midi_control_on(i).match(status, data[0]))
                                {
                                    if (midi_control_on(i).in_range(data[1]))
                                    {
                                        if (i < m_seqs_in_set)
                                            sequence_playing_on(i+m_offset);
                                        else
                                            handle_midi_control(i, true);
                                    }
                                    else if (midi_control_on(i).inverse_active())
                                    {
                                        if (i < m_seqs_in_set)
                                            sequence_playing_off(i+m_offset);
                                        else
                                            handle_midi_control(i, false);
                                    }
                                }
                                if (midi_control_off(i).match(status, data[0]))
                                {
                                    if (midi_control_on(i).in_range(data[1]))
                                    {
                                        if (i < m_seqs_in_set)
                                            sequence_playing_off(i+m_offset);
                                        else
                                            handle_midi_control(i, false);
                                    }
                                    else if (midi_control_off(i).inverse_active())
                                    {
                                        if (i < m_seqs_in_set)
                                            sequence_playing_on(i+m_offset);
                                        else
                                            handle_midi_control(i, true);
                                    }
                                }
                            }
                        }
                    }
                    if (ev.get_status() == EVENT_SYSEX)
                    {
                        if (rc().show_midi())
                            ev.print();

                        if (rc().pass_sysex())
                            m_master_bus.sysex(&ev);
                    }
                }
            } while (m_master_bus.is_more_input());
        }
    }
    pthread_exit(0);
}

/**
 *  For all active patterns/sequences, this function gets the playing
 *  status and saves it in m_sequence_state[i].  Inactive patterns get the
 *  value set to false.
 */

void
perform::save_playing_state ()
{
    for (int i = 0; i < m_sequence_max; ++i)
    {
        if (is_active(i))
            m_sequence_state[i] = m_seqs[i]->get_playing();
        else
            m_sequence_state[i] = false;
    }
}

/**
 *  For all active patterns/sequences, this function gets the playing
 *  status from m_sequence_state[i] and sets it for the sequence.
 */

void
perform::restore_playing_state ()
{
    for (int i = 0; i < m_sequence_max; ++i)
    {
        if (is_active(i))
            m_seqs[i]->set_playing(m_sequence_state[i]);
    }
}

/**
 *  If the given status is present in the c_status_snapshot, the playing
 *  state is saved.  Then the given status is OR'd into the
 *  m_control_status.
 */

void
perform::set_sequence_control_status (int status)
{
    if (status & c_status_snapshot)
        save_playing_state();

    m_control_status |= status;
}

/**
 *  If the given status is present in the c_status_snapshot, the playing
 *  state is restored.  Then the given status is reversed in
 *  m_control_status.
 */

void
perform::unset_sequence_control_status (int status)
{
    if (status & c_status_snapshot)
        restore_playing_state();

    m_control_status &= (~status);
}

/**
 *
 */

void
perform::sequence_playing_toggle (int sequence)
{
    if (is_active(sequence))
    {
        if (m_control_status & c_status_queue)
        {
            m_seqs[sequence]->toggle_queued();
        }
        else
        {
            if (m_control_status & c_status_replace)
            {
                unset_sequence_control_status(c_status_replace);
                off_sequences();
            }
            m_seqs[sequence]->toggle_playing();
        }
    }
}

/**
 *  A helper function for determining if the mode group is in force,
 *  the playing screenset is the same as the current screenset, and the sequence
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
 *  Turn off the playing of a sequence, if it is active.
 *
 * \param seq
 *      The number of the sequence to be turned on.
 *
 */

void
perform::sequence_playing_on (int seq)
{
    if (is_active(seq))
    {
        if (seq_in_playing_screen(seq))
            m_tracks_mute_state[seq - m_playscreen_offset] = true;

        bool queued = m_seqs[seq]->get_queued();
        if (! m_seqs[seq]->get_playing())
        {
            if (m_control_status & c_status_queue)
            {
                if (! queued)
                    m_seqs[seq]->toggle_queued();
            }
            else
                m_seqs[seq]->set_playing(true);
        }
        else
        {
            if (queued && (m_control_status & c_status_queue))
                m_seqs[seq]->toggle_queued();
        }
    }
}

/**
 *  Turn off the playing of a sequence, if it is active.
 *
 * \param seq
 *      The number of the sequence to be turned off.
 */

void
perform::sequence_playing_off (int seq)
{
    if (is_active(seq))
    {
        if (seq_in_playing_screen(seq))
            m_tracks_mute_state[seq - m_playscreen_offset] = false;

        bool queued = m_seqs[seq]->get_queued();
        if (m_seqs[seq]->get_playing())
        {
            if (m_control_status & c_status_queue)
            {
                if (! queued)
                    m_seqs[seq]->toggle_queued();
            }
            else
                m_seqs[seq]->set_playing(false);
        }
        else
        {
            if (queued && (m_control_status & c_status_queue))
                m_seqs[seq]->toggle_queued();
        }
    }
}

/*
 * Non-inline encapsulation functions start here.
 */

/**
 *  Handle a sequence key to toggle the playing of an active pattern in
 *  the selected screen-set.
 */

void
perform::sequence_key (int seq)
{
    int offset = get_screenset() * c_mainwnd_rows * c_mainwnd_cols;
    if (is_active(seq + offset))
        sequence_playing_toggle(seq + offset);
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

        for (int seq = 0; seq < m_sequence_max; seq++)
        {
            sequence * s = get_sequence(seq);
            if (not_nullptr(s))
                s->set_dirty();
        }
    }
    else if (bus >= 0)
        master_bus().set_input(bus, active);
}

/**
 *  Provided for mainwnd::on_key_press_event() and
 *  mainwnd::on_key_release_event() to call.
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
        if (key == keys().replace())
            set_sequence_control_status(c_status_replace);
        else if (key == keys().queue() || key == keys().keep_queue())
            set_sequence_control_status(c_status_queue);
        else if (key == keys().snapshot_1() || key == keys().snapshot_2())
            set_sequence_control_status(c_status_snapshot);
        else if (key == keys().set_playing_screenset())
            set_playing_screenset();
        else if (key == keys().group_on())
            set_mode_group_mute();
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
 *  Provided for perfroll::on_key_press_event() and
 *  perfroll::on_key_release_event() to call.
 *
 *  The "is modified" flag is raised if something is deleted, but we cannot
 *  yet handle the case where we undo all the changes.  So, for now,
 *  we play it safe with the user, even if the user gets annoyed because he
 *  knows that he undid all the changes.
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
            if (k.is_delete())
            {
                push_trigger_undo();
                get_sequence(drop_sequence)->del_selected_trigger();
                modify();
                result = true;
            }
            else if (k.mod_control())               /* SEQ64_CONTROL_MASK   */
            {
                if (k.is_letter('x'))                           /* cut      */
                {
                    push_trigger_undo();
                    get_sequence(drop_sequence)->cut_selected_trigger();
                    modify();
                    result = true;
                }
                else if (k.is_letter('c'))                      /* copy     */
                {
                    get_sequence(drop_sequence)->copy_selected_trigger();
                    result = true;
                }
                else if (k.is_letter('v'))                      /* paste    */
                {
                    push_trigger_undo();
                    get_sequence(drop_sequence)->paste_trigger();
                    modify();
                    result = true;
                }
                else if (k.is_letter('z'))                      /* undo     */
                {
                    /*
                     * Can we support undo here?
                     *
                     * result = true;
                     */
                }
            }
        }
    }
    return result;
}

/**
 *  Invoke the start key functionality.  Meant to be used by GUIs to unify the
 *  treatment of keys versus buttons.
 *
 * \param songmode
 *      The live/play mode parameter to be passed along to the key processor.
 */

void
perform::start_key (bool songmode)
{
    (void) playback_key_event(keys().start(), songmode);
}

/**
 *  Invoke the pause key functionality.  Meant to be used by GUIs to unify the
 *  treatment of keys versus buttons.
 *
 * \param songmode
 *      The live/play mode parameter to be passed along to the key processor,
 *      when starting playback.
 */

void
perform::pause_key (bool songmode)
{
#ifdef SEQ64_PAUSE_SUPPORT
    (void) playback_key_event(keys().pause(), songmode);
#else
    (void) playback_key_event(keys().start(), songmode);
#endif
}

/**
 *  Invoke the stop key functionality.  Meant to be used by GUIs to unify the
 *  treatment of keys versus buttons.
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
 * \param k
 *      Provides the encapsulated keystroke to check.
 *
 * \param songmode
 *      Provides the "jack flag" needed by the mainwnd, seqroll, and perfedit
 *      windows.  Defaults to false, which disables Song mode, and enables
 *      Live mode.
 *
 * \return
 *      Returns true if the keystroke matched the start, stop, or (new) pause
 *      keystrokes.
 */

bool
perform::playback_key_event (const keystroke & k, bool songmode)
{
#ifdef SEQ64_PAUSE_SUPPORT
#define PAUSEKEY    keys().pause()
#else
#define PAUSEKEY    0
#endif

    bool result = OR_EQUIVALENT(k.key(), keys().start(), keys().stop());
    if (! result)
        result = k.key() == PAUSEKEY;

    if (result)
    {
        bool onekey = keys().start() == keys().stop();
        if (k.key() == keys().start())
        {
            if (onekey)
            {
                if (is_running())
                    pause_playing();
                else
                    start_playing(songmode);
            }
            else if (! is_running())
                start_playing(songmode);
        }
        else if (k.key() == keys().stop())
        {
            stop_playing();
        }
#ifdef SEQ64_PAUSE_SUPPORT
        else if (k.key() == PAUSEKEY)
        {
            if (is_running())
                pause_playing();
            else
                start_playing(songmode);
        }
#endif
    }
    return result;
}

/**
 *  Gets the max-tick value of all active sequences.  We can't seem to
 *  find a way to get a good value from the perform object itself, so we get
 *  it from the sequences.  This approach kind of works, but the result is
 *  slow.  We could optimize it a little by saving the indices of the first
 *  and last active sequence.
 *
 * \return
 *      If no active sequence is found, 0 is returned.  Otherwise, the
 *      maximum get_last_tick() value of the active sequences is returned.
 */

midipulse
perform::get_max_tick () const
{
#ifdef SEQ64_PAUSE_SUPPORT
    midipulse result = 0;
    if (is_jack_running())
    {
        result = get_tick();
    }
    else
    {
        for (int s = 0; s < m_sequence_max; ++s)
        {
            if (is_active(s))
            {
                midipulse current = m_seqs[s]->get_last_tick();
                if (current > result)
                    result = current;
            }
        }
    }
    return result;
#else
    return get_tick();
#endif
}

/**
 *  Shows all the triggers of all the sequences.
 */

void
perform::print_triggers () const
{
    for (int s = 0; s < m_sequence_max; ++s)
    {
        if (is_active(s))
            m_seqs[s]->print_triggers();
    }
}

}           // namespace seq64

/*
 * perform.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

