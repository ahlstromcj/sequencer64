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
 * \updates       2019-07-09
 * \license       GNU GPLv2 or above
 *
 *  This class still has way too many members, even with the JACK and
 *  key-binding support moved to separate modules.  Items that could
 *  profitably be partitioned into separate modules are:
 *
 *      -   Mute-group support.
 *      -   MIDI control support.
 *      -   The remaining portions of trigger support.
 *      -   Sequence array parameters could be usefully combined into
 *          an array of structures.
 *
 *  Important global MIDI parameters:
 *
 *      -   m_master_bus
 *      -   m_beats_per_bar
 *      -   m_beat_width
 *      -   m_tempo_track_number
 *
 *  User jean-emmanuel added a new MIDI control for setting the screen-set
 *  directly by number.  To handle this, a value parameter was added to
 *  handle_midi_control_ex().
 *
 * \todo
 *      One big TO DO with this and other classes is to use std::unique_ptr
 *      instead of raw pointers, to make truly exception-safe destructors.
 */

#include "globals.h"                    /* globals, nullptr, & more         */
#include "jack_assistant.hpp"           /* optional seq64::jack_assistant   */
#include "gui_assistant.hpp"            /* seq64::gui_assistant             */
#include "keys_perform.hpp"             /* seq64::keys_perform              */
#include "mastermidibus.hpp"            /* seq64::mastermidibus for ALSA    */
#include "midi_control.hpp"             /* seq64::midi_control "struct"     */
#include "midi_control_out.hpp"         /* seq64::midi_control_out          */
#include "playlist.hpp"                 /* seq64::playlist, 0.96 and above  */
#include "sequence.hpp"                 /* seq64::sequence                  */

#ifdef SEQ64_SONG_BOX_SELECT
#include <functional>                   /* std::function, function objects  */
#include <set>                          /* std::set, arbitary selection     */
#endif

#include <memory>                       /* std::unique_ptr<>                */
#include <vector>                       /* std::vector<>                    */
#include <pthread.h>                    /* pthread_t C structure            */

/**
 *  This value is used to indicated that the queued-replace (queued-solo)
 *  feature is reset and not in force.
 */

#define SEQ64_NO_QUEUED_SOLO            (-1)

/**
 *  This value indicates that the value of perform::m_mute_group_selected
 *  should not be used.
 */

#define SEQ64_NO_MUTE_GROUP_SELECTED    (-1)

/**
 *  A parameter value for track/sequence number incorporated from
 *  Stazed's seq32 project.
 */

#define SEQ64_ALL_TRACKS                (-1)

/*
 *  All Sequencer64 library code is in the seq64 namespace.
 */

/**
 *  We have offloaded the keybinding support to another class, derived
 *  from keys_perform.  These macros make the code easier to read, or a least
 *  shorter.  :-)
 */

#define PERFKEY(x)              m_mainperf->keys().x()
#define PERFKEY_ADDR(x)         m_mainperf->keys().at_##x()

/**
 *  Uses a function returning a reference.  These macros make the code easier
 *  to read, or a least shorter.  :-)
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

#define PERFORM_KEY_LABELS_ON_SEQUENCE  254     // 9998
#define PERFORM_NUM_LABELS_ON_SEQUENCE  255     // 9999

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{
    class keystroke;

/**
 *  These were purely internal constants used with the functions that
 *  implement MIDI control (and also some keystroke control) for the
 *  application.  However, we now have to expose them for the Qt5
 *  implementation, until we can entirely reconcile/refactor the
 *  Kepler34-based body of code.  Note how they specify different bit values,
 *  as it they could be masked together to signal multiple functions.
 *
 *  This value signals the "replace" functionality.  If this bit is set, then
 *  perform::sequence_playing_toggle() unsets this status and calls
 *  perform::off_sequences(), which calls sequence::set_playing(false) for all
 *  active sequences.
 *
 *  It works like this:
 *
 *      -#  The user presses the Replace key, or the MIDI control message for
 *          c_midi_control_mod_replace is received.
 *      -#  This bit is OR'd into perform::m_control_status.  This status bit
 *          is used in perform::sequence_playing_toggle().
 *          -   Called in perform::sequence_key() so that keystrokes in
 *              the main window toggle patterns in the main window.
 *          -   Called in peform::toggle_other_seqs() to implement
 *              Shift-click to toggle all other patterns but the one
 *              clicked.
 *          -   Called in seqmenu::toggle_current_sequence(), called in
 *              mainwid to implement clicking on a pattern.
 *          -   Also used in MIDI control to toggle patterns 0 to 31,
 *              offset by the screen-set.
 *          -   perform::sequence_playing_off(), similarly used in MIDI control.
 *          -   perform::sequence_playing_on(), similarly used in MIDI control.
 *      -#  When the key is released, this bit is AND'd out of
 *          perform::m_control_status.
 *
 *      Both the MIDI control and the keystroke set the sequence to be
 *      "replaced".
 */

const int c_status_replace = 0x01;

/**
 *  This value signals the "snapshot" functionality.  By default,
 *  perform::sequence_playing_toggle() calls sequence::toggle_playing() on the
 *  given sequence number, plus what is noted for c_status_snapshot.
 *  It works like this:
 *
 *      -#  The user presses the Snapshot key.
 *      -#  This bit is OR'd into perform::m_control_status.
 *      -#  The playing state of the patterns is saved by
 *          perform::save_playing_state().
 *      -#  When the key is released, this bit is AND'd out of
 *          perform::m_control_status.
 *      -#  The playing state of the patterns is restored by
 *          perform::restore_playing_state().
 */

const int c_status_snapshot = 0x02;

/**
 *  This value signals the "queue" functionality.  If this bit is set, then
 *  perform::sequence_playing_toggle() calls sequence::toggle_queued() on the
 *  given sequence number.  The regular queue key (configurable in File /
 *  Options / Keyboard) sets this bit when pressed, and unsets it when
 *  released.  The keep-queue key sets it, but it is not unset until the
 *  regular queue key is pressed and released.
 */

const int c_status_queue = 0x04;

/**
 *  This value signals the Kepler34 "one-shot" functionality.  If this bit
 *  is set, then perform::sequence_playing_toggle() calls
 *  sequence::toggle_oneshot() on the given sequence number.
 */

const int c_status_oneshot = 0x08;

/**
 *      Provides for notification of events.  Provide a response to a
 *      group-learn change event.
 */

struct performcallback
{

/*
 * ca 2015-07-24
 * Eliminate this annoying warning.  Will do it for Microsoft's bloddy
 * compiler later.  Actually, this pragma affect any module that includes this
 * header file.
 */

#ifdef PLATFORM_GNU
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

    /**
     *  A do-nothing callback.  "state" is an Unused parameter.
     */

    virtual void on_grouplearnchange (bool /* state */)
    {
        // Empty body
    }

};

/**
 *  This class supports the performance mode.  It has way too many data
 *  members.  It has way too many friends. Might be ripe for refactoring.
 *  That has its own dangers, of course.  One thing to do soon is remove the
 *  need to having GUI classes as friends.  Will make some necessary setters
 *  public.
 */

class perform
{
    friend class jack_assistant;
    friend class keybindentry;
    friend class mainwnd;
    friend class midifile;
    friend class options;
    friend class optionsfile;           // needs cleanup
    friend class perfedit;
    friend class perfroll;
    friend class playlist;              // new feature for 0.96.0
    friend class qperfeditframe64;
    friend class qsliveframe;
    friend class qsmainwnd;
    friend class sequence;              // for setting tempo from events
    friend class wrkfile;
    friend void * input_thread_func (void * myperf);
    friend void * output_thread_func (void * myperf);

#ifdef SEQ64_JACK_SUPPORT

    friend int jack_sync_callback       // accesses perform::inner_start()
    (
        jack_transport_state_t state,
        jack_position_t * pos,
        void * arg
    );

    friend int jack_transport_callback (jack_nframes_t nframes, void * arg);
    friend void jack_shutdown (void * arg);
    friend void jack_timebase_callback
    (
        jack_transport_state_t state, jack_nframes_t nframes,
        jack_position_t * pos, int new_pos, void * arg
    );
    friend long get_current_jack_position (void * arg);

#endif  // SEQ64_JACK_SUPPORT

public:

    /**
     *  In many cases, when we check a key action that perform will do, it is
     *  sufficient to return a boolean.  But, in some cases, we need to indicate
     *  what was changed (e.g. via a keystroke).  This enumeration provides
     *  return values that a (GUI) caller can use to decided which values to get
     *  and then change the user-interface to indicate the new value.
     *
     *  See the keyboard_group_action() function and the "[keyboard-control]"
     *  and "[keyboard-group]" configuration sections of the "rc" file.
     */

    enum action_t
    {
        ACTION_NONE,            /**< The keystroke was not handled by perform.  */
        ACTION_SEQ_TOGGLE,      /**< For perform::sequence_playing_toggle().    */
        ACTION_GROUP_MUTE,      /**< See mainwnd::on_key_press_event().  ???    */
        ACTION_BPM,             /**< Applies to any BPM change, including tap.  */
        ACTION_SCREENSET,       /**< The keystroke altered the active set.      */
        ACTION_GROUP_LEARN,     /**< See mainwnd::on_key_press_event().  ???    */
        ACTION_C_STATUS,        /**< For replace, queue, snapshot, oneshot.     */

        // Might not be useful:
        //
        // ACTION_PLAYBACK
        // ACTION_SONG_MODE,
        // ACTION_MENU_MODE,
        // ACTION_SONG_RECORD,
        // ACTION_ONESHOT_QUEUE,
        // ACTION_SONG_EDIT,
        // ACTION_EVENT_EDIT,
        // ACTION_SLASH_SHIFT,
        // ACTION_EXTENDED,
    };

public:

#ifdef SEQ64_SONG_BOX_SELECT

    /**
     *  Provides a type to hold the unique shift-selected sequence numbers.
     *  Although this can be considered a GUI function, it makes sense to
     *  let perform manage it and encapsulate it.
     */

    typedef std::set<int> Selection;

    /**
     *  Provides a function type that can be applied to each sequence number
     *  in a Selection.  Generally, the caller will bind a member function to
     *  use in operate_on_set().  The first parameter is a sequence number
     *  (obtained from the Selection).  The caller can bind additional
     *  placeholders or parameters, if desired.
     */

#ifdef PLATFORM_CPP_11
    typedef std::function<void(int)> SeqOperation;
#endif

#endif

    /**
     *  Provides settings for tempo recording.  Currently not used, though the
     *  functionality of logging and recording tempo is in place.
     */

    enum record_tempo_op_t
    {
        RECORD_TEMPO_LOG_EVENT,
        RECORD_TEMPO_ON,
        RECORD_TEMPO_OFF
    };

    /**
     *  Provides settings for muting.
     */

    enum mute_op_t
    {
        MUTE_TOGGLE     = -1,
        MUTE_OFF        =  0,
        MUTE_ON         =  1
    };

    /**
     *  Provides setting for the fast-forward and rewind functionality.
     */

    enum ff_rw_button_t
    {
        FF_RW_REWIND    = -1,
        FF_RW_NONE      =  0,
        FF_RW_FORWARD   =  1
    };

private:

    /**
     *  Provides a dummy, inactive midi_control object to handle
     *  out-of-range midi_control indicies.
     */

    static midi_control sm_mc_dummy;

    /**
     *  Provides an optional play-list, loosely patterned after Stazed's Seq32
     *  play-list. Important: This object is now owned by perform.
     */

    std::unique_ptr<playlist> m_play_list;

    /**
     *  If true, playback is done in Song mode, not Live mode.  This is
     *  a replacement for the global setting, but is essentially a global
     *  setting itself, and is saved to and restored from the "rc"
     *  configuration file.  Sometimes called "JACK start mode", it used
     *  to be a JACK setting, but now applies to any playback.  Do not confuse
     *  this setting with m_playback_mode, which has a similar meaning but is
     *  more transitory.  Probably, the concept needs some clean-up.
     */

    bool m_song_start_mode;

    /**
     *  Indicates that, no matter what the current Song/Live setting, the
     *  playback was started from the perfedit window.
     */

    bool m_start_from_perfedit;

    /**
     *  It seems that this member, if true, forces a repositioning to the left
     *  (L) tick marker.
     */

    bool m_reposition;

    /**
     *  Provides an "acceleration" factor for the fast-forward and rewind
     *  functionality.  It starts out at 1.0, and can range up to 60.0, being
     *  multiplied by 1.1 by the FF/RW timeout function.
     */

    float m_excell_FF_RW;

    /**
     *  Indicates whether the fast-forward or rewind key is in effect in the
     *  perfedit window.  It has values of FF_RW_REWIND, FF_RW_NONE, or
     *  FF_RW_FORWARD.  This was a free (global in a namespace) int in
     *  perfedit.
     */

    ff_rw_button_t m_FF_RW_button_type;

    /**
     *  Mute group support.  This value determines whether a particular track
     *  will be muted or unmuted, and it can handle all tracks available in
     *  the application (currently c_max_sets * c_seqs_in_set, i.e. 1024).
     *  Note that the current state of playing can be "learned", and stored
     *  herein as the desired state for the track.
     */

    bool m_mute_group[c_max_sequence];

    /**
     *  Preserves the mute groups from the "rc" file, so that they won't
     *  necessarily be overwritten by the mute groups contained in a
     *  Sequencer64 MIDI file.
     */

    bool m_mute_group_rc[c_max_sequence];

    /**
     *  Indicates if the m_saved_armed_statuses[] values are the saved state
     *  of the sequences, and can be restored.
     */

    bool m_armed_saved;

    /**
     *  Holds the "global" saved status of the playing tracks, for restoration
     *  after saving.
     */

    bool m_armed_statuses[c_max_sequence];

    /**
     *  We have replaced c_seqs_in_set with this member, which defaults to the
     *  value of c_seqs_in_set, but is grabbed from user_settings now.  This
     *  change requires some arrays to be dynamically allocated (vectors).
     *  This cannot be a constant, because we may need to change it after
     *  creating the perform object.
     */

    int m_seqs_in_set;                  /* replaces global c_seqs_in_set    */

    /**
     *  Since we can increase the number of sequences in a set, we need to be
     *  able to decrease the number of sets or groups we can store.  This
     *  value is the maximum number of sequences we can store (c_max_sequence)
     *  divided by the number of sequences in a set.
     *
     *  Groups are a set of sequence-states.  They are held in a linear array
     *  of size c_max_sequence, subdivided into groups of size m_seqs_in_set.
     */

    int m_max_groups;

    /**
     *  Holds the current mute states of each track.  Unlike the
     *  m_mute_group[] array, this holds the current state, rather than the
     *  state desired by activating a mute group, and it applies to only one
     *  screen-set.
     *
     *      bool m_tracks_mute_state[c_seqs_in_set];
     *
     *  Please be aware that vector<bool> might be optimized to use bits,
     *  and so taking the address of an element of this vector will not work.
     *  But we don't need that kind of access, so we are safe here.  We might
     *  try using a char at some point, to see how performance is affected.
     */

    std::vector<bool> m_tracks_mute_state;

    /**
     *  If true, indicates that a mode group is selected, and playing statuses
     *  will be "memorized".  This value starts out true.  It is altered by
     *  the c_midi_control_mod_gmute handler or when the keys().group_off()
     *  or the keys().group_on() keys are struck.
     */

    bool m_mode_group;

    /**
     *  If true, indicates that a group learn is selected, which also
     *  "memorizes" a mode group, and notifies subscribers of a group-learn
     *  change.
     */

    bool m_mode_group_learn;

    /**
     *  Selects a group to mute.  A "group" is essentially a "set" that is
     *  selected for the saving and restoring of the status of all patterns in
     *  that set.  We're going to add a value of -1
     *  (SEQ64_NO_MUTE_GROUP_SELECTED) to indicate the value should not be
     *  used.
     */

    int m_mute_group_selected;

    /**
     *  If true, indicates that non-zero mute-groups were present in this MIDI
     *  file.  We need to know if valid mute-groups are present when deciding
     *  whether or not to write them to the "rc" file.
     */

    bool m_midi_mute_group_present;

    /**
     *  Provides a "vector" of patterns/sequences.
     *
     * \todo
     *      First, make the sequence array a vector, and second, put all of
     *      these flags into a structure and access those members indirectly.
     */

    sequence * m_seqs[c_max_sequence];

    /**
     *  Each boolean value in this array is set to true if a sequence is
     *  active, meaning that it will be used to hold some kind of MIDI data,
     *  even if only Meta events.  This array can have "holes" with inactive
     *  sequences, so every sequence needs to be checked before using it.
     */

    bool m_seqs_active[c_max_sequence];

    /**
     *  Each boolean value in this array is set to true if a sequence was
     *  active, meaning that it was found to be active at the time we were
     *  setting it to inactive.  This value seems to be used only in
     *  maintaining dirtiness-status; did some process modify the sequence?
     *  Was it's mute/unmute status changed?
     */

    bool m_was_active_main[c_max_sequence];

    /**
     *  Each boolean value in this array is set to true if a sequence was
     *  active, meaning that it was found to be active at the time we were
     *  setting it to inactive.  This value seems to be used only in
     *  maintaining dirtiness-status for editing the mute/unmute status during
     *  pattern editing.
     */

    bool m_was_active_edit[c_max_sequence];

    /**
     *  Each boolean value in this array is set to true if a sequence was
     *  active, meaning that it was found to be active at the time we were
     *  setting it to inactive.  This value seems to be used only in
     *  maintaining dirtiness-status for editing the mute/unmute status during
     *  performance/song editing.
     */

    bool m_was_active_perf[c_max_sequence];

    /**
     *  Each boolean value in this array is set to true if a sequence was
     *  active, meaning that it was found to be active at the time we were
     *  setting it to inactive.  This value seems to be used only in
     *  maintaining dirtiness-status for editing the mute/unmute status during
     *  performance names editing.  Not sure that it serves a real purpose;
     *  perhaps created with an eye to editing the pattern name in the song
     *  editor?
     */

    bool m_was_active_names[c_max_sequence];

    /**
     *  Saves the current playing state of each pattern.
     */

    bool m_sequence_state[c_max_sequence];

    /**
     *  Saves the current playing state only for the current set.
     *  This is used in the new queue-replace (queue-solo) feature.
     *
     *      bool m_screenset_state[c_seqs_in_set];
     */

    std::vector<bool> m_screenset_state;

    /**
     *  A value not equal to -1 (it ranges from 0 to 31) indicates we're now
     *  using the saved screen-set state to control the queue-replace
     *  (queue-solo) status of sequence toggling.  This value is set to -1
     *  when queue mode is exited.  See the SEQ64_NO_QUEUED_SOLO value.
     */

    int m_queued_replace_slot;

    /**
     *  Holds the global MIDI transposition value.
     */

    int m_transpose;

private:

    /**
     *  Provides information for managing pthreads.  Provides a "handle" to
     *  the output thread.
     */

    pthread_t m_out_thread;

    /**
     *  Provides a "handle" to the input thread.
     */

    pthread_t m_in_thread;

    /**
     *  Indicates that the output thread has been started.
     */

    bool m_out_thread_launched;

    /**
     *  Indicates that the input thread has been started.
     */

    bool m_in_thread_launched;

    /**
     *  Indicates that playback is running.  However, this flag is conflated
     *  with some JACK support, and we have to supplement it with another
     *  flag, m_is_pattern_playing.
     */

    bool m_is_running;

    /**
     *  Indicates that a pattern is playing.  It replaces rc_settings ::
     *  is_pattern_playing(), which is gone, since the perform object is now
     *  visible to all classes that care about it.
     */

    bool m_is_pattern_playing;

    /**
     *  Indicates that events are being written to the MIDI input busses in
     *  the input thread.
     */

    bool m_inputing;

    /**
     *  Indicates that events are being written to the MIDI output busses in
     *  the output thread.
     */

    bool m_outputing;

    /**
     *  Indicates that status of the "loop" button in the performance editor.
     *  If true, the performance will loop between the L and R markers in the
     *  performance editor.
     */

    bool m_looping;

    /**
     *  Indicates to record live sequence-trigger changes into the Song data.
     */

    bool m_song_recording;

    /**
     *  Indicates to resume notes if the sequence is toggled after a Note On.
     */

    bool m_resume_note_ons;

    /**
     *  The global current tick, moved out from the output function so that
     *  position can be set.
     */

    double m_current_tick;

    /**
     *  Specifies the playback mode.  There are two, "live" and "song",
     *  indicated by the following values:
     *
\verbatim
        m_playback_mode == false:       live mode
        m_playback_mode == true:        playback/song mode
\endverbatim
     *
     */

    bool m_playback_mode;

    /**
     *  Holds the current PPQN for usage in various actions.
     */

    int m_ppqn;

    /**
     *  Holds the current BPM (beats per minute) for later usage.
     */

    midibpm m_bpm;

    /**
     *  Holds the beats/bar value as obtained from the MIDI file.
     *  The default value is SEQ64_DEFAULT_BEATS_PER_MEASURE (4).
     */

    int m_beats_per_bar;

    /**
     *  Holds the beat width value as obtained from the MIDI file.
     *  The default value is SEQ64_DEFAULT_BEAT_WIDTH (4).
     */

    int m_beat_width;

    /**
     *  Holds the number of the official tempo track for this performance.
     *  Normally 0, it can be changed to any value from 1 to 1023 via the
     *  tempo-track-number setting in the "rc" file, and that can be overriden
     *  by the c_tempo_track SeqSpec possibly present in the song's MIDI file.
     */

    int m_tempo_track_number;

    /**
     *  Augments the beats/bar and beat-width with the additional values
     *  included in a Time Signature meta event.  This value provides the
     *  number of MIDI clocks between metronome clicks.  The default value of
     *  this item is 24.  It can also be read from some SMF 1 files, such as
     *  our hymne.mid example.
     */

    int m_clocks_per_metronome;

    /**
     *  Augments the beats/bar and beat-width with the additional values
     *  included in a Time Signature meta event.  Useful in export.  A
     *  duplicate of the same member in the sequence class.
     */

    int m_32nds_per_quarter;

    /**
     *  Augments the beats/bar and beat-width with the additional values
     *  included in a Tempo meta event.  Useful in export.  A duplicate of the
     *  same member in the sequence class.
     */

    long m_us_per_quarter_note;

    /**
     *  Provides our MIDI buss.  We changed this item to a pointer so that we
     *  can delay the creation of this object until after all settings have
     *  been read.
     */

    mastermidibus * m_master_bus;

    /**
     *  Provides storage for this "rc" configuration option so that the
     *  perform object can set it in the master buss once that has been
     *  created.
     */

    bool m_filter_by_channel;

    /**
     *  Saves the clock settings obtained from the "rc" (options) file so that
     *  they can be loaded into the mastermidibus once it is created.
     */

    std::vector<clock_e> m_master_clocks;

    /**
     *  Saves the input settings obtained from the "rc" (options) file so that
     *  they can be loaded into the mastermidibus once it is created.
     */

    std::vector<bool> m_master_inputs;

    /**
     *  Holds the "one measure's worth" of pulses (ticks), which is normally
     *  m_ppqn * 4.  We can save some multiplications, and, more importantly,
     *  later define a more flexible definition of "one measure's worth" than
     *  simply four quarter notes.
     */

    midipulse m_one_measure;

    /**
     *  Holds the position of the left (L) marker, and it is first defined as
     *  0.  Note that "tick" is actually "pulses".
     */

    midipulse m_left_tick;

    /**
     *  Holds the position of the right (R) marker, and it is first defined as
     *  the end of the fourth measure.  Note that "tick" is actually "pulses".
     */

    midipulse m_right_tick;

    /**
     *  Holds the starting tick for playing.  By default, this value is always
     *  reset to the value of the "left tick".  We want to eventually be able
     *  to leave it at the last playing tick, to support a "pause"
     *  functionality. Note that "tick" is actually "pulses".
     */

    midipulse m_starting_tick;

    /**
     *  MIDI Clock support.  The m_tick member holds the tick to be used in
     *  displaying the progress bars and the maintime pill.  It is mutable
     *  because sometimes we want to adjust it in a const function for pause
     *  functionality.
     */

    mutable midipulse m_tick;

    /**
     *  Let's try to save the last JACK pad structure tick for re-use with
     *  resume after pausing.
     */

    midipulse m_jack_tick;

    /**
     *  More MIDI clock support.
     */

    bool m_usemidiclock;

    /**
     *  More MIDI clock support.
     */

    bool m_midiclockrunning;            // stopped or started

    /**
     *  More MIDI clock support.
     */

    int m_midiclocktick;

    /**
     *  More MIDI clock support.
     */

    int m_midiclockpos;

    /**
     *  Support for pause, which does not reset the "last tick" when playback
     *  stops/starts.  All this member is used for is keeping the last tick
     *  from being reset.
     */

    bool m_dont_reset_ticks;

private:

    /**
     *  Used in the mainwnd class to set the notepad text for the given set.
     */

    std::string m_screenset_notepad[c_max_sets];

    /**
     *  Provides the settings of MIDI Toggle, as read from the "rc" file.
     */

    midi_control m_midi_cc_toggle[c_midi_controls_extended];

    /**
     *  Provides the settings of MIDI On, as read from the "rc" file.
     */

    midi_control m_midi_cc_on[c_midi_controls_extended];

    /**
     *  Provides the settings of MIDI Off, as read from the "rc" file.
     */

    midi_control m_midi_cc_off[c_midi_controls_extended];

    /**
     *  Provides the class encapsulating MIDI control output.
     */

    midi_control_out * m_midi_ctrl_out;

    /**
     *  Indicates that the "[midi-control-out]" section is present but
     *  disabled.
     */

    bool m_midi_ctrl_out_disabled;

    /**
     *  Holds the OR'ed control status values.  Need to learn more about this
     *  one.  It is used in the replace, snapshot, and queue functionality.
     */

    int m_control_status;

    /**
     *  Indicates the number of the currently-selected screen-set.  This is
     *  merely the screen-set that is in view.  The fix of tdeagan substitutes
     *  the "in-view" screen-set for the "playing" screen-set.
     */

    int m_screenset;

    /**
     *  Holds the current sequence-number offset for the current screen-set.
     *  Saves some multiplications.  It is used in the MIDI control of the
     *  playback status of the sequences in the current screen-set.  It is
     *  also used to offset the sequence numbers so that the control
     *  (mute/unmute) keys can be shown on any screen-set.
     */

    int m_screenset_offset;

    /**
     *  Playing screen support.  In seq24, this value is altered by
     *  set_playing_screenset(), which is called by
     *  handle_midi_control(c_midi_control_play_ss, state).
     */

    int m_playscreen;

    /**
     *  Playing screen sequence number offset.  Saves some multiplications,
     *  should make the code easier to grok, and centralizes the use of
     *  c_seqs_in_set/m_seqs_in_set, which we want to be able to change at
     *  run-time, as a future enhancement.
     */

    int m_playscreen_offset;

    /**
     *  A replacement for the c_max_sets constant.  Again, currently set to
     *  the old value, which is used in hard-wired array sizes.  To make it
     *  variable will require a move from arrays to vectors.
     */

    int m_max_sets;

    /**
     *  Keeps track of created sequences, whether or not they are active.
     *  Used by the install_sequence() function.  Note that this value is
     *  not a suitable replacement for c_max_sequence/m_sequence_max, because
     *  there can be inactive sequences amidst the active sequences.
     *  See the m_sequence_limit member.
     */

    int m_sequence_count;

    /**
     *  A replacement for the c_max_sequence constant.  However, this value is
     *  already 32 * 32 = 1024, and is probably enough for any usage.  Famous
     *  last words?
     */

    int m_sequence_max;

    /**
     *  Indicates the highest-number sequence.  This value starts as 0,
     *  to indicate no sequences loaded, and then contains the highest
     *  sequence number hitherto loaded, plus 1 so that it can be used as
     *  a for-loop limit similar to m_sequence_max.  It's maximum value
     *  should be m_sequence_max (c_max_sequence).
     *
     *  Currently meant only for limited context to try to squeeze a little
     *  extra speed out of playback.  There's no easy way to lower this
     *  value when the highest sequence is deleted, though.
     */

    int m_sequence_high;

#ifdef SEQ64_EDIT_SEQUENCE_HIGHLIGHT

    /**
     *  Hold the number of the currently-in-edit sequence.  Moving this
     *  status from seqmenu into perform for better centralized management.
     */

    int m_edit_sequence;

#endif

    /**
     *  It may be a good idea to eventually centralize all of the dirtiness of
     *  a performance here.  All the GUIs seem to use a perform object.
     */

    bool m_is_modified;

#ifdef SEQ64_SONG_BOX_SELECT

    /**
     *  Provides a set holding all of the sequences numbers that have been
     *  shift-selected.  If we ever enable box-selection, this container will
     *  support that as well.
     */

    Selection m_selected_seqs;

#endif

    /**
     *  A condition variable to protect playback.  It is signalled if playback
     *  has been started.  The output thread function waits on this variable
     *  until m_is_running and m_outputing are false.  This variable is also
     *  signalled in the perform destructor.
     */

    condition_var m_condition_var;

#ifdef SEQ64_JACK_SUPPORT

    /**
     *  A wrapper object for the JACK support of this application.  It
     *  implements most of the JACK stuff.
     */

    jack_assistant m_jack_asst;

#endif

    /*
     * Not sure that we need this code; we'll think about it some more.  One
     * issue with it is that we really can't keep good track of the modify
     * flag in this case, in general.
     */

    /*
     * Used for undo track modification support.
     */

    bool m_have_undo;

    /**
     *  Holds the "track" numbers or the "all tracks" values for undo
     *  operations.  See the push_trigger_undo() function.
     */

    std::vector<int> m_undo_vect;

    /**
     * Used for redo track modification support.
     */

    bool m_have_redo;

    /**
     *  Holds the "track" numbers or the "all tracks" values for redo
     *  operations.  See the pop_trigger_undo() function.
     */

    std::vector<int> m_redo_vect;

    /*
     *  Can register here for events.  Used in mainwnd and perform.
     *  Now wrapped in the enregister() function, so no longer public.
     */

    std::vector<performcallback *> m_notify;

    /**
     *  Support for a wide range of GUI-related operations.
     */

    gui_assistant & m_gui_support;

public:

    perform (gui_assistant & mygui, int ppqn = SEQ64_USE_DEFAULT_PPQN);
    ~perform ();

    /*
     * Start of playlist accessors.
     */

    /**
     *  Get the number of playlists.
     */

    int playlist_count () const
    {
        return bool(m_play_list) ? m_play_list->list_count() : 0 ;
    }

    /**
     *  Get the number of songs in the current playlist.
     */

    int song_count () const
    {
        return bool(m_play_list) ? m_play_list->song_count() : 0 ;
    }

    /**
     *  Reset to the beginning of the playlist and song.
     */

    bool playlist_reset () const
    {
        return bool(m_play_list) ? m_play_list->reset() : false ;
    }

    bool open_playlist (const std::string & pl, bool show_on_stdout = false);
    bool remove_playlist_and_clear ();

    /**
     *  Runs the playlist test.
     */

    void playlist_test ()
    {
        if (bool(m_play_list))
            m_play_list->test();
    }

    /**
     *  Gets the playlist full-path specification.
     */

    std::string playlist_filename () const
    {
        return bool(m_play_list) ? m_play_list->name() : "" ;
    }

    /**
     *  Get the MIDI control number for the current playlist.
     */

    int playlist_midi_number () const
    {
        return bool(m_play_list) ? m_play_list->list_midi_number() : (-1) ;
    }

    /**
     *  Get the human name (title) for the current playlist.
     */

    std::string playlist_name () const
    {
        return bool(m_play_list) ? m_play_list->list_name() : "" ;
    }

    /**
     *  Gets the playlist mode, which is true if the playlist object exists
     *  and is active.
     */

    bool playlist_mode () const
    {
        return bool(m_play_list) ? m_play_list->mode() : false ;
    }

    /**
     *  Sets the play-list mode.  Even if a playlist is loaded,
     *  the user may need to toggle it active/inactive.
     *
     * \param on
     *      Set to true to active the play-list.  If there is no play-list,
     *      nothing happens.
     */

    void playlist_mode (bool on)
    {
        if (bool(m_play_list))
            m_play_list->mode(on);
    }

    /**
     *  \return
     *      Returns the default directory for songs in the current play-list.
     */

    std::string file_directory () const
    {
        static std::string s_dummy;
        return bool(m_play_list) ? m_play_list->file_directory() : s_dummy ;
    }

    /**
     *  \return
     *      Returns the actual directory for songs in the current play-list.
     *      Some songs might provide their own directory to use.
     */

    std::string song_directory () const
    {
        static std::string s_dummy;
        return bool(m_play_list) ? m_play_list->song_directory() : s_dummy ;
    }

    /**
     * \return
     *      Returns true if the current song provides its own directory to
     *      override the default directory specified by the current playlist
     *      section.
     */

    bool is_own_song_directory () const
    {
        return bool(m_play_list) ?
            m_play_list->is_own_song_directory() : false ;
    }

    /**
     *
     */

    std::string song_filename () const
    {
        static std::string s_dummy;
        return bool(m_play_list) ? m_play_list->song_filename() : s_dummy ;
    }

    /**
     *
     */

    int song_midi_number () const
    {
        return bool(m_play_list) ? m_play_list->song_midi_number() : (-1) ;
    }

    /**
     * \return
     *      Returns the current play-list song if it exists, otherwise an empty
     *      string is returned.
     */

    std::string playlist_song () const
    {
        return bool(m_play_list) ? m_play_list->current_song() : "" ;
    }

    /**
     *
     */

    bool open_current_song ()
    {
        return bool(m_play_list) ? m_play_list->open_current_song() : false ;
    }

    /**
     *
     */

    bool open_select_list_by_index (int index, bool opensong = true)
    {
        return bool(m_play_list) ?
            m_play_list->open_select_list_by_index(index, opensong) : false ;
    }

    /**
     *
     */

    bool open_select_list_by_midi (int ctrl, bool opensong = true)
    {
        return bool(m_play_list) ?
            m_play_list->select_list_by_midi(ctrl, opensong) : false ;
    }

    /**
     *  Meant for the user-interface.
     */

    bool add_song
    (
        int index, int midinumber,
        const std::string & name,
        const std::string & directory
    )
    {
        bool result = bool(m_play_list);
        if (result)
            result = m_play_list->add_song(index, midinumber, name, directory);

        return result;
    }

    /**
     *
     */

    bool open_next_list (bool opensong = true)
    {
        return bool(m_play_list) ?
            m_play_list->open_next_list(opensong) : false ;
    }

    /**
     *
     */

    bool open_previous_list (bool opensong = true)
    {
        return bool(m_play_list) ?
            m_play_list->open_previous_list(opensong) : false ;
    }

    /**
     *
     */

    bool open_select_song_by_index (int index, bool opensong = true)
    {
        return bool(m_play_list) ?
            m_play_list->open_select_song_by_midi(index, opensong) : false ;
    }

    /**
     *
     */

    bool open_select_song_by_midi (int ctrl, bool opensong = true)
    {
        return bool(m_play_list) ?
            m_play_list->open_select_song_by_midi(ctrl, opensong) : false ;
    }

    /**
     *
     */

    bool open_next_song (bool opensong = true)
    {
        return bool(m_play_list) ?
            m_play_list->open_next_song(opensong) : false ;
    }

    /**
     *
     */

    bool open_previous_song (bool opensong = true)
    {
        return bool(m_play_list) ?
            m_play_list->open_previous_song(opensong) : false ;
    }

    const std::string & playlist_error_message () const;

    /*
     * End of playlist accessors.
     */

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
     *      The setter that can falsify it, is_modified(), is private.  No one
     *      but perform and its friends should falsify this flag.
     */

    void modify ()
    {
        m_is_modified = true;
    }

    /**
     * \getter m_ppqn
     */

    int get_ppqn () const
    {
        return m_ppqn;
    }

    /**
     * \getter m_bpm
     */

    midibpm bpm () const
    {
        return m_bpm;
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
     * \getter m_sequence_high
     */

    int sequence_high () const
    {
        return m_sequence_high;
    }

    /**
     * \getter m_sequence_max
     */

    int sequence_max () const
    {
        return m_sequence_max;
    }

    /**
     * \getter m_max_groups
     */

    int group_max () const
    {
        return m_max_groups;
    }

    /**
     * \getter m_control_status
     *
     * \return
     *      Returns true if the m_control_status value is non-zero, which
     *      means that there is a queue, replace, or snapshot functionality in
     *      progress.
     */

    bool is_control_status () const
    {
        return m_control_status != 0;
    }

    /**
     * \getter m_midi_mute_group_present
     */

    bool midi_mute_group_present () const
    {
        return m_midi_mute_group_present;
    }

#ifdef SEQ64_EDIT_SEQUENCE_HIGHLIGHT

    /**
     * \setter m_edit_sequence
     *
     * \param seqnum
     *      Pass in -1 to disable the edit-sequence number unconditionally.
     *      Use unset_edit_sequence() to disable it if it matches the current
     *      edit-sequence number.
     */

    void set_edit_sequence (int seqnum)
    {
        m_edit_sequence = seqnum;
    }

    /**
     * \setter m_edit_sequence
     *
     *      Disables the edit-sequence number if it matches the parameter.
     *
     * \param seqnum
     *      The sequence number of the sequence to unset.
     */

    void unset_edit_sequence (int seqnum)
    {
        if (is_edit_sequence(seqnum))
            set_edit_sequence(-1);
    }

    /**
     * \getter m_edit_sequence
     *
     * \param seqnum
     *      Tests the parameter against m_edit_sequence.  Returns true
     *      if that member is not -1, and the parameter matches it.
     */

    bool is_edit_sequence (int seqnum) const
    {
        return (m_edit_sequence != (-1)) && (seqnum == m_edit_sequence);
    }

#endif  // SEQ64_EDIT_SEQUENCE_HIGHLIGHT

    /**
     * \getter m_beats_per_bar
     */

    int get_beats_per_bar () const
    {
        return m_beats_per_bar;
    }

    /**
     * \setter m_beats_per_bar
     *
     * \param bpm
     *      Provides the value for beats/measure.  Also used to set the
     *      beats/measure in the JACK assistant object.
     */

    void set_beats_per_bar (int bpm)
    {
        m_beats_per_bar = bpm;
#ifdef SEQ64_JACK_SUPPORT
        m_jack_asst.set_beats_per_measure(bpm);
#endif
    }

    /**
     * \getter m_beat_width
     */

    int get_beat_width () const
    {
        return m_beat_width;
    }

    /**
     * \setter m_beat_width
     *
     * \param bw
     *      Provides the value for beat-width.  Also used to set the
     *      beat-width in the JACK assistant object.
     */

    void set_beat_width (int bw)
    {
        m_beat_width = bw;
#ifdef SEQ64_JACK_SUPPORT
        m_jack_asst.set_beat_width(bw);
#endif
    }

    /**
     * \getter m_tempo_track_number
     */

    int get_tempo_track_number () const
    {
        return m_tempo_track_number;
    }

    /**
     * \setter m_tempo_track_number
     *
     * \param tempotrack
     *      Provides the value for beat-width.  Also used to set the
     *      beat-width in the JACK assistant object.
     */

    void set_tempo_track_number (int tempotrack)
    {
        if (tempotrack >= 0 && tempotrack < SEQ64_SEQUENCE_MAXIMUM)
            m_tempo_track_number = tempotrack;
    }

    /**
     * \setter m_clocks_per_metronome
     */

    void clocks_per_metronome (int cpm)
    {
        m_clocks_per_metronome = cpm;       // needs validation
    }

    /**
     * \getter m_clocks_per_metronome
     */

    int clocks_per_metronome () const
    {
        return m_clocks_per_metronome;
    }

    /**
     * \setter m_32nds_per_quarter
     */

    void set_32nds_per_quarter (int tpq)
    {
        m_32nds_per_quarter = tpq;              // needs validation
    }

    /**
     * \getter m_32nds_per_quarter
     */

    int get_32nds_per_quarter () const
    {
        return m_32nds_per_quarter;
    }

    /**
     * \setter m_us_per_quarter_note
     */

    void us_per_quarter_note (long upqn)
    {
        m_us_per_quarter_note = upqn;       // needs validation
    }

    /**
     * \getter m_us_per_quarter_note
     */

    long us_per_quarter_note () const
    {
        return m_us_per_quarter_note;
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
     *      Obviously, this is a dangerous function, but we've got ya
     *      covered.
     */

    mastermidibus & master_bus ()
    {
        return *m_master_bus;
    }

    /**
     * \setter m_master_bus.filter_by_channel()
     */

    void filter_by_channel (bool flag)
    {
        m_filter_by_channel = flag;
        if (not_nullptr(m_master_bus))
            m_master_bus->filter_by_channel(flag);
    }

    /**
     * \getter m_is_running
     *      Could also be called "is_playing()".
     */

    bool is_running () const
    {
        return m_is_running;
    }

    /**
     * \setter m_is_pattern_playing
     */

    bool is_pattern_playing () const
    {
        return m_is_pattern_playing;
    }

    /**
     * \setter m_is_pattern_playing
     */

    void is_pattern_playing (bool flag)
    {
        m_is_pattern_playing = flag;
    }

    /**
     * \setter m_song_start_mode
     */

    bool toggle_song_start_mode ()
    {
        m_song_start_mode = ! m_song_start_mode;    // m_playback_mode
        return m_song_start_mode;
    }

    /**
     * \setter m_song_start_mode
     */

    void song_start_mode (bool flag)
    {
        m_song_start_mode = flag;
    }

    /**
     * \getter m_song_start_mode
     */

    bool song_start_mode () const
    {
        return m_song_start_mode;
    }

    /**
     * \getter m_jack_asst.is_running()
     *      This function is useful for announcing the status of JACK in
     *      user-interface items that only have access to the perform object.
     */

    bool is_jack_running () const
    {
#ifdef SEQ64_JACK_SUPPORT
        return m_jack_asst.is_running();
#else
        return false;
#endif
    }

    /**
     * \getter m_jack_asst.is_master()
     *      Also now includes is_jack_running(), since one cannot be JACK
     *      Master if JACK is not running.
     */

    bool is_jack_master () const
    {
#ifdef SEQ64_JACK_SUPPORT
        return m_jack_asst.is_running() && m_jack_asst.is_master();
#else
        return false;
#endif
    }

    /**
     *  Adds a pointer to an object to be notified by this perform object.
     *
     * \param pfcb
     *      Provides the pointer to the performance callback.
     */

    void enregister (performcallback * pfcb)
    {
        if (not_nullptr(pfcb))
            m_notify.push_back(pfcb);
    }

    void toggle_jack_mode ()
    {
#ifdef SEQ64_JACK_SUPPORT
        m_jack_asst.toggle_jack_mode();
#endif
    }

    bool set_jack_mode (bool mode);

    /**
     * \getter m_jack_asst.get_jack_mode()
     */

    bool get_toggle_jack () const
    {
#ifdef SEQ64_JACK_SUPPORT
        return m_jack_asst.get_jack_mode();          // m_toggle_jack;
#else
        return false;
#endif
    }

    /**
     * \setter m_jack_asst.set_jack_stop_tick()
     */

    void set_jack_stop_tick (midipulse tick)
    {
#ifdef SEQ64_JACK_SUPPORT
        m_jack_asst.set_jack_stop_tick(tick);
#endif
    }

    unsigned short combine_bytes (midibyte b0, midibyte b1);
    void FF_rewind ();
    bool FF_RW_timeout ();          /* called by free-function of same name */

    /**
     * \setter m_start_from_perfedit
     */

    void start_from_perfedit (bool flag)
    {
        m_start_from_perfedit = flag;
    }

    /**
     * \getter m_start_from_perfedit
     */

    bool start_from_perfedit () const
    {
        return m_start_from_perfedit;
    }

    /**
     * \getter m_jack_asst.set_follow_transport()
     */

    void set_follow_transport (bool flag)
    {
#ifdef SEQ64_JACK_SUPPORT
        m_jack_asst.set_follow_transport(flag);
#endif
    }

    /**
     * \getter m_jack_asst.get_follow_transport()
     */

    bool get_follow_transport () const
    {
#ifdef SEQ64_JACK_SUPPORT
        return m_jack_asst.get_follow_transport();
#else
        return false;
#endif
    }

    /**
     *
     */

    bool follow () const
    {
        return is_running() && get_follow_transport();
    }

    /**
     * \setter m_jack_asst.toggle_follow_transport()
     */

    void toggle_follow_transport ()
    {
#ifdef SEQ64_JACK_SUPPORT
        m_jack_asst.toggle_follow_transport();
#endif
    }

    /**
     *  Convenience function for following progress in seqedit.
     */

    bool follow_progress () const
    {
#ifdef SEQ64_JACK_SUPPORT
        return m_is_running && m_jack_asst.get_follow_transport();
#else
        return m_is_running;
#endif
    }

    /**
     * \getter m_reposition
     */

    void set_reposition (bool postype = true)
    {
        m_reposition = postype;
    }

    /**
     * \getter m_FF_RW_button_type
     */

    ff_rw_button_t ff_rw_type ()
    {
        return m_FF_RW_button_type;
    }

    /**
     * \getter m_FF_RW_button_type
     */

    void ff_rw_type (ff_rw_button_t button_type)
    {
        m_FF_RW_button_type = button_type;
    }

    /**
     *  Sets the rewind status.
     *
     * \param press
     *      If true, the status is set to FF_RW_REWIND, otherwise it is set to
     *      FF_RW_NONE.
     */

    void rewind (bool press)
    {
        ff_rw_type(press ? FF_RW_REWIND : FF_RW_NONE);
    }

    /**
     *  Sets the fast-forward status.
     *
     * \param press
     *      If true, the status is set to FF_RW_FORWARD, otherwise it is set
     *      to FF_RW_NONE.
     */

    void fast_forward (bool press)
    {
        ff_rw_type(press ? FF_RW_FORWARD : FF_RW_NONE);
    }

    void reposition (midipulse tick);

public:

    /**
     * \getter m_master_bus->set_sequence_input()
     */

     void set_sequence_input (bool active, sequence * s)
     {
        if (not_nullptr_2(m_master_bus, s))
            m_master_bus->set_sequence_input(active, s);
     }

    void set_recording (bool rec_active, bool thru_active, sequence * s);
    void set_recording (bool rec_active, int seq, bool toggle = false);
    void set_quantized_recording (bool rec_active, sequence * s);
    void set_quantized_recording (bool rec_active, int seq, bool toggle = false);

    /*
     * New from jfrey-xx on GitHub.
     */

    void overwrite_recording
    (
        bool oactive, int seq, bool toggle = false
    );
    void set_thru (bool rec_active, bool thru_active, sequence * s);
    void set_thru (bool thru_active, int seq, bool toggle = false);
    bool selected_trigger
    (
        int seqnum, midipulse droptick,
        midipulse & tick0, midipulse & tick1
    );

#ifdef SEQ64_SONG_BOX_SELECT

#ifdef PLATFORM_CPP_11
    bool selection_operation (SeqOperation func);
#endif
    void box_insert (int dropseq, midipulse droptick);
    void box_delete (int dropseq, midipulse droptick);
    void box_toggle_sequence (int dropseq, midipulse droptick);
    void box_unselect_sequences (int dropseq);
    void box_move_triggers (midipulse tick);
    void box_offset_triggers (midipulse offset);

    /**
     * \getter m_selected_seqs.empty()
     */

    bool box_selection_empty () const
    {
        return m_selected_seqs.empty();
    }

    /**
     *
     */

    void box_selection_clear ()
    {
        m_selected_seqs.clear();
    }

#endif

    bool clear_all ();
    void launch (int ppqn);
    void finish ();
    bool new_sequence (int seq);                    /* seqmenu & mainwid    */
    void add_sequence (sequence * seq, int perf);   /* midifile             */
    void delete_sequence (int seq);                 /* seqmenu & mainwid    */
    bool is_sequence_in_edit (int seq) const;
    void print_busses () const;

    /**
     * \getter m_tick
     */

    midipulse get_tick () const
    {
        return m_tick;
    }

    void set_tick (midipulse tick);

    /**
     * \getter m_jack_tick
     */

    midipulse get_jack_tick () const
    {
        return m_jack_tick;
    }

    /**
     * \setter m_jack_tick
     *
     * \param tick
     *      Provides the current JACK tick (pulse) value to set.
     */

    void set_jack_tick (midipulse tick)
    {
        m_jack_tick = tick;
    }

    void set_left_tick (midipulse tick, bool setstart = true);

    /**
     * \getter m_left_tick
     */

    midipulse get_left_tick () const
    {
        return m_left_tick;
    }

    /**
     * \setter m_starting_tick
     *
     * \param tick
     *      Provides the starting JACK tick (pulse) value to set.
     */

    void set_start_tick (midipulse tick)
    {
        m_starting_tick = tick;
    }

    /**
     * \setter m_starting_tick
     */

    midipulse get_start_tick () const
    {
        return m_starting_tick;
    }

    void set_right_tick (midipulse tick, bool setstart = true);

    /**
     * \getter m_right_tick
     */

    midipulse get_right_tick () const
    {
        return m_right_tick;
    }

    /**
     *  Convenience function for JACK support when loop in song mode.
     *
     * \return
     *      Returns the difference between the right and left tick, cast to
     *      double.
     */

    double left_right_size () const
    {
        return double(m_right_tick - m_left_tick);
    }

public:

    /**
     *  Checks the pattern/sequence for activity.
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

    bool is_active (int seq) const
    {
        return is_mseq_valid(seq) ? m_seqs_active[seq] : false ;
    }

    bool is_seq_valid (int seq) const;
    bool is_mseq_valid (int seq) const;
    bool is_mseq_available (int seq) const;
    bool screenset_is_active (int screenset);
    void apply_song_transpose ();

    /**
     * \setter m_transpose
     *      For sanity's sake, the values are restricted to +-64.
     */

    void set_transpose (int t)
    {
        if (t >= SEQ64_TRANSPOSE_DOWN_LIMIT && t <= SEQ64_TRANSPOSE_UP_LIMIT)
            m_transpose = t;
    }

    /**
     * \getter m_transpose
     */

    int get_transpose () const
    {
        return m_transpose;
    }

    /**
     * \getter m_master_bus.get_beats_per_minute
     *      Retrieves the BPM setting of the master MIDI buss.
     *
     *  This result should be the same as the value of the m_bpm member.
     *  This function returns that value in a roundabout way.
     *
     * \return
     *      Returns the value of beats/minute from the master buss.
     */

    midibpm get_beats_per_minute ()
    {
        return not_nullptr(m_master_bus) ?
            m_master_bus->get_beats_per_minute() : 0.0 ;
    }

    bool reload_mute_groups (std::string & errmessage);
    bool clear_mute_groups ();
    void set_sequence_control_status (int status);
    void unset_sequence_control_status (int status);
    void unset_queued_replace (bool clearbits = true);
    void sequence_playing_toggle (int seq);
    void sequence_playing_change (int seq, bool on);
    void set_keep_queue (bool activate);
    bool is_keep_queue () const;

    /**
     *  Calls sequence_playing_change() with a value of true.
     *
     * \param seq
     *      The sequence number of the sequence to turn on.
     */

    void sequence_playing_on (int seq)
    {
        sequence_playing_change(seq, true);
    }

    /**
     *  Calls sequence_playing_change() with a value of false.
     *
     * \param seq
     *      The sequence number of the sequence to turn off.
     */

    void sequence_playing_off (int seq)
    {
        sequence_playing_change(seq, false);
    }

    void mute_all_tracks (bool flag = true);

    void toggle_all_tracks ();

    /**
     * \getter m_armed_saved
     */

    bool armed_saved () const
    {
        return m_armed_saved;
    }

    void toggle_playing_tracks ();
    void mute_screenset (int ss, bool flag = true);
    void output_func ();
    void input_func ();
    void set_group_mute_state (int gtrack, bool muted);
    bool get_group_mute_state (int gtrack);
    int mute_group_offset (int track);

    /**
     * \getter m_screenset_offset
     */

    int screenset_offset () const
    {
        return m_screenset_offset;
    }

    /**
     *  Translates a pattern number to a slot number re the current screenset
     *  offset.
     *
     * \param s
     *      Provides the sequence number of interest.  This value should range
     *      from 0 to 1023 (c_max_seqs).
     *
     * \return
     *      Returns the "normalized" value.  Do not use it if less than zero.
     */

    int slot_number (int s)
    {
        return s - m_screenset_offset;
    }

    void save_playing_state ();
    void restore_playing_state ();
    void save_current_screenset (int repseq);
    void clear_current_screenset ();

    /**
     * Here follows a few forwarding functions for the keys_perform-derived
     * classes.
     *
     * \param k
     *      The key number for which to return the string name of the key.
     */

    std::string key_name (unsigned k) const
    {
        return keys().key_name(k);
    }

    /**
     *  Forwarding function for key events.
     */

    keys_perform::SlotMap & get_key_events ()
    {
        return keys().get_key_events();
    }

    /**
     *  Returns the number of times the given key appears in the SlotMap,
     *  either 0 or 1.
     *
     * \param k
     *      The key value to be checked.
     */

    int get_key_count (unsigned k) const
    {
        return keys().get_key_count(k);
    }

    /**
     *  Forwarding function for key groups.
     */

    keys_perform::SlotMap & get_key_groups ()
    {
        return keys().get_key_groups();
    }

    /**
     *  Forwarding function for reverse key events.
     */

    keys_perform::RevSlotMap & get_key_events_rev ()
    {
        return keys().get_key_events_rev();
    }

    /**
     *  Forwarding function for reverse key groups.
     */

    keys_perform::RevSlotMap & get_key_groups_rev ()
    {
        return keys().get_key_groups_rev();
    }

    /**
     * \getter m_show_ui_sequency_key
     *      Provides access to keys().show_ui_sequence_key().
     *      Used in mainwid, options, optionsfile, userfile, and perform.
     */

    bool show_ui_sequence_key () const
    {
        return keys().show_ui_sequence_key();
    }

    /**
     * \setter m_show_ui_sequency_key
     *
     * \param flag
     *      Provides the flag to set into keys().show_ui_sequence_key().
     */

    void show_ui_sequence_key (bool flag)
    {
        keys().show_ui_sequence_key(flag);
    }

    /**
     * \getter m_show_ui_sequency_number
     *      Provides access to keys().show_ui_sequence_number().
     *      Used in mainwid, optionsfile, and perform.
     */

    bool show_ui_sequence_number () const
    {
        return keys().show_ui_sequence_number();
    }

    /**
     * \getter m_show_ui_sequency_number
     *
     * \param flag
     *      Provides the value to set into keys().show_ui_sequence_number().
     */

    void show_ui_sequence_number (bool flag)
    {
        keys().show_ui_sequence_number(flag);
    }

    /*
     * Getters of keyboard mapping for sequence and groups.
     * If not found, returns something "safe" [so use get_key()->count()
     * to see if it's there first]
     */

    unsigned lookup_keyevent_key (int seqnum);
    unsigned lookup_slot_key (int slotnum);

    /**
     *  Gets the sequence number for the given event key.  The inverse of
     *  lookup_keyevent_key().
     *
     * \param keycode
     *      The number of the event key for which to return the configured
     *      sequence number.
     *
     * \return
     *      Returns the desired sequence.  If there is no such value, then
     *      a sequence number of 0 is returned.
     */

    int lookup_keyevent_seq (unsigned keycode)
    {
        return keys().lookup_keyevent_seq(keycode);
    }

    /**
     *  Gets the group key for the given sequence.
     *
     * \param groupnum
     *      The number of the group for which to return the group key.
     *
     * \return
     *      Returns the desired key.  If there is no such value, then the
     *      default character is returned.
     */

    unsigned lookup_keygroup_key (int groupnum)
    {
        return keys().lookup_keygroup_key(groupnum);    /* '.' or ' ' */
    }

    /**
     *  Gets the group number for the given group key.  The inverse of
     *  lookup_keygroup_key().
     *
     * \param keycode
     *      The number of the group key for which to return the configured
     *      sequence number.
     *
     * \return
     *      Returns the desired group number.  If there is no such value, then
     *      a group number of 0 is returned.
     */

    int lookup_keygroup_group (unsigned keycode)
    {
        return keys().lookup_keygroup_group(keycode);
    }

    void start_playing (bool songmode = false);
    void pause_playing (bool songmode = false);
    void stop_playing ();
    void start_key (bool songmode = false);
    void pause_key (bool songmode = false);
    void stop_key ();

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

    midibpm decrement_beats_per_minute ();
    midibpm increment_beats_per_minute ();
    midibpm page_decrement_beats_per_minute ();
    midibpm page_increment_beats_per_minute ();
    int decrement_screenset (int amount = 1);
    int increment_screenset (int amount = 1);

    /**
     *  True if a sequence is empty and should be highlighted.  This setting
     *  is currently a build-time option, but could be made a run-time option
     *  later.
     *
     * \param seq
     *      Provides a reference to the desired sequence.
     */

#if SEQ64_HIGHLIGHT_EMPTY_SEQS

    bool highlight (const sequence & seq) const
    {
        return seq.event_count() == 0;
    }

#else

    bool highlight (const sequence & /*seq*/) const
    {
        return false;
    }

#endif

    /**
     *  True if the sequence is an SMF 0 sequence.
     *
     * \param seq
     *      Provides a reference to the desired sequence.
     */

    bool is_smf_0 (const sequence & seq) const
    {
        return seq.is_smf_0();
    }

    /**
     *  Retrieves the actual sequence, based on the pattern/sequence number.
     *  This is the const version.  Note that it is more efficient to call
     *  this function and check the result than to call is_active() and then
     *  call this function.
     *
     * \param seq
     *      The prospective sequence number.
     *
     * \return
     *      Returns the value of m_seqs[seq] if seq is valid.  Otherwise, a
     *      null pointer is returned.
     */

    const sequence * get_sequence (int seq) const
    {
        return is_mseq_valid(seq) ? m_seqs[seq] : nullptr ;
    }

    /**
     *  Retrieves the actual sequence, based on the pattern/sequence number.
     *  This is the non-const version.  Note that it is more efficient to call
     *  this function and check the result than to call is_active() and then
     *  call this function.
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

    void sequence_key (int seq);                            // encapsulation
    std::string sequence_label (const sequence & seq);
    std::string sequence_label (int seqnumb);               // for qperfnames
    std::string sequence_title (const sequence & seq);
    std::string main_window_title (const std::string & fn = "");
    std::string sequence_window_title (const sequence & seq);
    void set_input_bus (bussbyte bus, bool input_active);   // used in options
    void set_clock_bus (bussbyte bus, clock_e clocktype);   // used in options
    bool mainwnd_key_event (const keystroke & k);
    bool keyboard_control_press (unsigned key);
    bool keyboard_group_c_status_press (unsigned key);
    bool keyboard_group_c_status_release (unsigned key);
    bool keyboard_group_press (unsigned key);
    bool keyboard_group_release (unsigned key);
    action_t keyboard_group_action (unsigned key);

    bool perfroll_key_event (const keystroke & k, int drop_sequence);
    bool playback_key_event (const keystroke & k, bool songmode = false);

    /*
     * More trigger functions.
     */

    void clear_sequence_triggers (int seq);
    void print_triggers () const;
    void move_triggers (bool direction);
    void copy_triggers ();
    void push_trigger_undo (int track = SEQ64_ALL_TRACKS);
    void pop_trigger_undo ();
    void pop_trigger_redo ();
    bool get_trigger_state (int seqnum, midipulse tick) const;
    void add_trigger (int seqnum, midipulse tick);
    void delete_trigger (int seqnum, midipulse tick);
    void add_or_delete_trigger (int seqnum, midipulse tick);
    void split_trigger (int seqnum, midipulse tick);
    void paste_trigger (int seqnum, midipulse tick);
    void paste_or_split_trigger (int seqnum, midipulse tick);
    bool intersect_triggers (int seqnum, midipulse tick);
    midipulse get_max_trigger () const;

    bool is_dirty_main (int seq);
    bool is_dirty_edit (int seq);
    bool is_dirty_perf (int seq);
    bool is_dirty_names (int seq);
    bool is_exportable (int seq) const;
    bool needs_update (int seq = -1);
    int set_screenset (int ss);
    void announce_playscreen ();
    void announce_exit ();

    /**
     * \getter m_screenset
     */

    int screenset () const
    {
        return m_screenset;
    }

    /**
     * \getter m_playscreen
     */

    int get_playing_screenset () const
    {
        return m_playscreen;
    }

    bool toggle_other_seqs (int seqnum, bool isshiftkey);   /* mainwid      */
    bool toggle_other_names (int seqnum, bool isshiftkey);  /* perfnames    */
    bool toggle_sequences (int seqnum, bool isshiftkey);    /* (q)perfnames */
    bool are_any_armed ();

    /**
     * \setter m_max_sets
     *      This setter is needed to modify the value after reading the "user"
     *      file.  Other than that, it should not be used.  We may find a way
     *      to enforce that, later.
     */

    void max_sets (int sets)
    {
        m_max_sets = sets;
    }

    /**
     * \setter m_seqs_in_set
     *      This setter modifies the current value based on the current values
     *      of the settings found in the user_settings module.
     */

    void seqs_in_set (int seqs)
    {
        m_seqs_in_set = seqs;
    }

    /*
     * This is a long-standing request from user's, adapted from Kepler34.
     */

    bool song_recording () const
    {
        return m_song_recording;
    }

    bool resume_note_ons () const
    {
        return m_resume_note_ons;
    }

    void resume_note_ons (bool f)
    {
        m_resume_note_ons = f;
    }

#ifdef SEQ64_SONG_BOX_SELECT

    void select_triggers_in_range
    (
        int seq_low, int seq_high,
        midipulse tick_start, midipulse tick_finish
    );

#endif

    bool select_trigger (int dropseq, midipulse droptick);
    void unselect_all_triggers ();

public:

    /**
     *  A better name for get_screen_set_notepad(), adapted from Kepler34.
     *  However, we will still refer to them as "sets".
     */

    const std::string & get_bank_name (int bank) const
    {
        return get_screenset_notepad(bank);
    }

    /**
     * \setter m_looping
     *
     * \param looping
     *      The boolean value to set for looping, used in the performance
     *      editor.
     */

    void set_looping (bool looping)
    {
        m_looping = looping;
    }

    /**
     *  Deals with the colors used to represent specific sequences.  We don't
     *  want perform knowing the details of the palette color, just treat it
     *  as an integer.
     */

    int get_sequence_color (int seqnum) const
    {
        return is_active(seqnum) ? m_seqs[seqnum]->color() : (-1) ;
    }

    /**
     *
     */

    void set_sequence_color (int seqnum, int c)
    {
        if (is_active(seqnum))
            m_seqs[seqnum]->color(c);
    }

    /**
     * \getter m_have_undo
     */

    bool have_undo () const
    {
        return m_have_undo;
    }

    /**
     * \setter m_have_undo
     *      Note that, if the \a undo parameter is true, then we mark the
     *      performance as modified.  Once it is set, it remains set, unless
     *      cleared by saving the file.
     */

    void set_have_undo (bool undo)
    {
        m_have_undo = undo;
        if (undo)
            modify();
    }

    /**
     * \getter m_have_redo
     */

    bool have_redo () const
    {
        return m_have_redo;
    }

    /**
     * \setter m_have_redo
     */

    void set_have_redo (bool redo)
    {
        m_have_redo = redo;
    }

public:         // GUI-support functions

    /*
     * Deals with the editing mode of the specific sequence.
     */

    edit_mode_t seq_edit_mode (int seq) const
    {
        const sequence * sp = get_sequence(seq);
        if (not_nullptr(sp))
            return sp->edit_mode();
        else
            return edit_mode_t(0);
    }

    /*
     * This overload deals with the editing mode of the specific sequence,
     * but the seqeuence ID is replaced with a reference to the sequence
     * itself.
     */

    edit_mode_t seq_edit_mode (const sequence & s) const
    {
        return s.edit_mode();
    }

    /**
     *  A pass-along function to set the edit-mode of the given sequence.
     *  Was private, but a class can have too many friends.
     *
     * \param seq
     *      Provides the sequence number.  If the sequence is not active
     *      (available), then nothing is done.
     *
     * \param ed
     *      Provides the edit mode, which is "note" or "drum", and which
     *      determines if the duration of events matters (note) or not (drum).
     */

    void seq_edit_mode (int seq, edit_mode_t ed)
    {
        sequence * sp = get_sequence(seq);
        if (not_nullptr(sp))
            sp->edit_mode(ed);
    }

    /**
     *  Overload.
     */

    void seq_edit_mode (sequence & s, edit_mode_t ed)
    {
        s.edit_mode(ed);
    }

    /**
     *  Returns the notepad text for the current screen-set.
     */

    const std::string & current_screenset_notepad () const
    {
        return get_screenset_notepad(m_screenset);
    }

    void set_screenset_notepad
    (
        int screenset,
        const std::string & note,
        bool is_load_modification = false
    );

    /**
     *  Sets the notepad text for the current screen-set.
     *
     * \param note
     *      The string value to set into the notepad text.
     */

    void set_screenset_notepad (const std::string & note)
    {
        set_screenset_notepad(m_screenset, note);
    }

    void start (bool state);
    void stop ();

    /**
     *  If JACK is supported, starts the JACK transport.
     */

    void start_jack ()
    {
#ifdef SEQ64_JACK_SUPPORT
        m_jack_asst.start();
#endif
    }

    /**
     *  If JACK is supported, stops the JACK transport.
     */

    void stop_jack ()
    {
#ifdef SEQ64_JACK_SUPPORT
        m_jack_asst.stop();
#endif
    }

    void song_recording_stop ();

    /**
     *
     */

    void song_recording (bool f)
    {
        m_song_recording = f;
        if (! f)
            song_recording_stop();
    }

    /**
     * \getter m_playback_mode
     */

    bool playback_mode ()
    {
        return m_playback_mode;
    }

    /**
     * \setter m_playback_mode
     *
     * \param playbackmode
     *      The value of the playback mode flag to be set.
     */

    void playback_mode (bool playbackmode)
    {
        m_playback_mode = playbackmode;
    }

    /**
     * \getter m_mode_group_learn
     */

    bool is_group_learning ()
    {
        return m_mode_group_learn;
    }

    void set_beats_per_minute (midibpm bpm);    /* more than just a setter  */
    void set_ppqn (int p);
    void panic ();                              /* from kepler43        */

private:

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

    midi_control & midi_control_toggle (int ctl);
    midi_control & midi_control_on (int ctl);
    midi_control & midi_control_off (int ctl);
    bool midi_control_event (const event & ev);
    bool midi_control_record (const event & ev);
    bool handle_midi_control (int control, bool state);
    bool handle_midi_control_ex (int control, midi_control::action a, int v);
    bool handle_midi_control_event (const event & ev, int ctrl, int offset = 0);
    bool handle_playlist_control (int ctl, midi_control::action a, int v);
    const std::string & get_screenset_notepad (int screenset) const;
    bool any_group_unmutes () const;
    void print_group_unmutes () const;
    void mute_group_tracks ();
    void select_and_mute_group (int g_group);
    void set_song_mute (mute_op_t op);
    void set_playing_screenset ();
    void set_midi_control_out (midi_control_out * ctrl_out);

    midi_control_out * get_midi_control_out () const
    {
        return m_midi_ctrl_out;
    }

    bool midi_control_out_disabled () const
    {
        return m_midi_ctrl_out_disabled;
    }

    void midi_control_out_disabled (bool flag)
    {
        m_midi_ctrl_out_disabled = flag;
    }

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

    void select_group_mute (int gmute);
    void set_mode_group_learn ();
    void unset_mode_group_learn ();
    bool load_mute_group (int gmute, int gm [c_max_groups]);
    bool save_mute_group (int gmute, int gm [c_max_groups]) const;
    void set_and_copy_mute_group (int group);

    bool activate ();
    void position_jack (bool state, midipulse tick = 0);
    void off_sequences ();
    void unqueue_sequences (int current_seq);
    void all_notes_off ();
    void set_active (int seq, bool active);
    void set_was_active (int seq);
    void reset_sequences (bool pause = false);

    /**
     *  Plays all notes to the current tick.
     */

    void play (midipulse tick);
    void set_orig_ticks (midipulse tick);
    int max_active_set () const;

    /*
     * See launch() instead.
     */

    void launch_input_thread ();
    void launch_output_thread ();
    bool init_jack_transport ();
    bool deinit_jack_transport ();
    bool seq_in_playing_screen (int seq);

    /**
     * \setter m_is_modified
     *
     * \param flag
     *      The value of the modified flag to be set.
     */

    void is_modified (bool flag)
    {
        m_is_modified = flag;
    }

    /**
     *  Checks the parameter against c_midi_controls_extended.  We were
     *  checking against c_midi_track_ctrl as well, but that was a bug.  This
     *  function is meant to check that the supplied sequence number does not
     *  exceed the value of c_midi_controls_extended (32 * 2 + 10 + 10 = 84).
     *  The track (sequence or pattern) controls rangoe from 0 to 64.  Next
     *  come the "c_midi_control" values:  bpm_up, bpm_dn, ..., play_ss, plus
     *  some extended controls that are relatively new, and, lastly,
     *  c_midi_controls_extended itself.
     *
     * \param seq
     *      The sequence number value that should be inside the
     *      c_midi_controls_extended range.  This value can specify not only a
     *      sequence number, but larger control values as well, so the
     *      function and parameter are mildly mis-named.
     *
     * \return
     *      Returns true if the sequence number is valid for accessing the
     *      MIDI control values.  For this function, no error print-out is
     *      generated.
     */

    bool valid_midi_control_seq (int seq) const
    {
        return seq < c_midi_controls_extended;
    }

    /**
     * \getter m_max_sets
     */

    int max_sets () const
    {
        return m_max_sets;
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
     * \setter m_is_running
     *
     * \param running
     *      The value of the running flag to be set.
     */

    void is_running (bool running)
    {
        m_is_running = running;
    }

    /**
     *  Calculates the screen-set offset index.  It supports variset mode
     *  (which is active if m_seqs_in_set != c_seq_in_set).
     *
     * \param ss
     *      Provides the screen-set number, ranging from 0 to c_max_sets-1.
     *      This value is not validated, for speed.
     *
     * \return
     *      Returns the product of \a ss and m_seqs_in_set.
     */

    int screenset_offset (int ss)
    {
        return ss * m_seqs_in_set;
    }

    bool install_sequence (sequence * seq, int seqnum);
    void inner_start (bool state);
    void inner_stop (bool midiclock = false);
    int clamp_track (int track) const;
    int clamp_group (int group) const;

    /**
     *  At construction time, this function sets up one keycode and one event
     *  slot.  It is called 32 times, corresponding to the pattern/sequence
     *  slots in the Patterns window.  It first removes the given key-code
     *  from the regular and reverse slot-maps.  Then it removes the
     *  sequence-slot from the regular and reverse slot-maps.  Finally, it
     *  adds the sequence-slot with a key value of key-code, and adds the
     *  key-code with a value of sequence-slot.
     *
     * \param keycode
     *      The keycode for which to set the sequence slot.
     *
     * \param sequence_slot
     *      The sequence slot to be set.
     */

    void set_key_event (unsigned keycode, int sequence_slot)
    {
        keys().set_key_event(keycode, sequence_slot);
    }

    /**
     *  At construction time, this function sets up one keycode and one group
     *  slot.  It is called 32 times, corresponding the pattern/sequence slots
     *  in the Patterns window.  Compare it to the set_key_events() function.
     *
     * \param keycode
     *      The keycode for which to set the group slot.
     *
     * \param group_slot
     *      The group slot to be set.
     */

    void set_key_group (unsigned keycode, int group_slot)
    {
        keys().set_key_group(keycode, group_slot);
    }

#ifdef PLATFORM_DEBUG_TMI
    void dump_mute_statuses (const std::string & tag);
#endif

private:

    bool log_current_tempo ();
    bool create_master_bus ();

#ifdef USE_STAZED_PARSE_SYSEX               // specific to Seq32
    void parse_sysex (event a_e);           // copy, or reference???
#endif

    /**
     *  Pre-allocates the desired number of clocks.  This function and calls
     *  to set_clock() are a more fool-proof option for reading the clocks
     *  from the "rc" file.  Might eventually do this for inputs as well.
     *  LATER.
     */

    void preallocate_clocks (int busscount)
    {
        for (int b = 0; b < busscount; ++b)
            add_clock(e_clock_off);
    }

    /**
     *  Saves the clock settings read from the "rc" file so that they can be
     *  passed to the mastermidibus after it is created.
     *
     * \param clocktype
     *      The clock value read from the "rc" file.
     */

    void add_clock (clock_e clocktype)
    {
        m_master_clocks.push_back(clocktype);
    }

    /**
     *  Sets a single clock item, if in the currently existing range.
     *  Mostly meant for use by the Options / MIDI Input tab.
     */

    void set_clock (bussbyte bus, clock_e clocktype)
    {
        if (bus < int(m_master_clocks.size()))
            m_master_clocks[bus] = clocktype;
    }

    /**
     *  Gets a single clock item, if in the currently existing range.
     *  Meant for use by the optionsfile::write() function.

    clock_e get_clock (bussbyte bus) const
    {
        return bus < bussbyte(m_master_clocks.size()) ?
            m_master_clocks[bus] : e_clock_off ;
    }
     * STILL THINKING ABOUT THIS.
     */

    /**
     * \getter m_master_bus->get_clock(bus);
     */

    clock_e get_clock (bussbyte bus) const
    {
        return m_master_bus->get_clock(bus);
    }

    /**
     *  Saves the input settings read from the "rc" file so that they can be
     *  passed to the mastermidibus after it is created.
     *
     * \param flag
     *      The input flag read from the "rc" file.
     */

    void add_input (bool flag)
    {
        m_master_inputs.push_back(flag);
    }

    /**
     *  Sets a single input item, if in the currently existing range.
     *  Mostly meant for use by the Options / MIDI Input tab.
     */

    void set_input (bussbyte bus, bool inputing)
    {
        if (bus < bussbyte(m_master_inputs.size()))
            m_master_inputs[bus] = inputing;
    }

    /**
     * \getter m_master_inputs[bus]
     *
     *  Changed from:
     *
     *      return not_nullptr(m_master_bus) ?
     *          m_master_bus->get_input(bus) : false ;

    bool get_input (bussbyte bus) const
    {
        return bus < bussbyte(m_master_inputs.size()) ?
            m_master_inputs[bus] : false ;
    }
     * STILL THINKING ABOUT THIS.
     */

    /**
     * \getter m_master_bus->get_input(bus);
     */

    bool get_input (bussbyte bus) const
    {
        return m_master_bus->get_input(bus);
    }

    /**
     * \getter m_master_bus->is_input_system_port(bus)
     */

    bool is_input_system_port (bussbyte bus)
    {
        return not_nullptr(m_master_bus) ?
            m_master_bus->is_input_system_port(bus) : false ;
    }

    /**
     * \setter m_midi_mute_group_present
     */

    void midi_mute_group_present (bool flag)
    {
        m_midi_mute_group_present = flag;
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

