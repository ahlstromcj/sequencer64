#ifndef SEQ64_KEYS_PERFORM_HPP
#define SEQ64_KEYS_PERFORM_HPP

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
 * \file          keys_perform.hpp
 *
 *  This module declares/defines the base class for keystrokes that might
 *  depend on the GUI framework.
 *
 * \library       sequencer64 application
 * \author        Chris Ahlstrom
 * \date          2015-09-13
 * \updates       2018-03-24
 * \license       GNU GPLv2 or above
 *
 * Stazed:
 *
 *      Additional keystrokes are provided by the JACK and transport support
 *      of seq32:
 *
 *      -   Song mode (key F1):
 *          Toggle between Live versus Song mode.  Provides an easy way to
 *          toggle without having to traverse menus.
 *      -   Toggle JACK (key F2):
 *          Toggle between using JACK and ALSA.  Provides an easy way to
 *          toggle without having to traverse menus.
 *      -   Menu mode (key F3):
 *          Enables and disables the main menu of the main window, to allow
 *          for the usage of Alt keys in playback hot-keys.
 *      -   Follow transport (key F4):
 *          Toggles whether the application follows JACK transport or not.
 *      -   Fast forward (key f):
 *          Allows for fast forward.
 *      -   Rewind (key r):
 *          Allows for rewind functionality.
 *      -   Position pointer (key p):
 *          Moves the tick position to the current mouse location.
 *      -   There are many more keystrokes if some of the Seq32 build
 *          options are enabled.
 */

#include <map>                          /* std::map                         */
#include <string>                       /* std::string                      */

#include "easy_macros.h"

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *  Provides a data-transfer structure to make it easier to fill in a
 *  keys_perform object's members using sscanf().
 */

struct keys_perform_transfer
{
    unsigned kpt_bpm_up;
    unsigned kpt_bpm_dn;
    unsigned kpt_screenset_up;
    unsigned kpt_screenset_dn;
    unsigned kpt_set_playing_screenset;
    unsigned kpt_group_on;
    unsigned kpt_group_off;
    unsigned kpt_group_learn;
    unsigned kpt_replace;
    unsigned kpt_queue;
    unsigned kpt_keep_queue;
    unsigned kpt_snapshot_1;
    unsigned kpt_snapshot_2;
    unsigned kpt_start;
    unsigned kpt_stop;
    bool kpt_show_ui_sequence_key;

    /*
     * Sequencer64 additions
     */

    bool kpt_show_ui_sequence_number;
    unsigned kpt_pattern_edit;
    unsigned kpt_pattern_shift;     /* variset support  */
    unsigned kpt_event_edit;
    unsigned kpt_tap_bpm;
    unsigned kpt_pause;

    /*
     * Seq32 (stazed) additions
     */

    unsigned kpt_song_mode;
    unsigned kpt_toggle_jack;
    unsigned kpt_menu_mode;
    unsigned kpt_follow_transport;
    unsigned kpt_fast_forward;
    unsigned kpt_rewind;
    unsigned kpt_pointer_position;
    unsigned kpt_toggle_mutes;

#ifdef SEQ64_SONG_RECORDING

    /*
     * Kepler34 additions
     */

    unsigned kpt_song_record;       // still thinking about this one.
    unsigned kpt_oneshot_queue;     // still thinking about this one.

#endif

};

/**
 *  This class supports the performance mode.  It provides a way a mapping
 *  keystrokes to sequencer actions and song settings.
 */

class keys_perform
{
    friend class options;               /* for member address access    */
    friend class perform;               /* for map-types access         */
    friend class optionsfile;           /* for map-types access         */

protected:

    /**
     *  This typedef defines a map in which the key is the keycode,
     *  that is, the integer value of a keystroke, and the value is the
     *  pattern/sequence number or slot.
     */

    typedef std::map<unsigned, int> SlotMap;        // lookup

    /**
     *  This typedef is like SlotMap, but used for lookup in the other
     *  direction.
     */

    typedef std::map<int, unsigned> RevSlotMap;     // reverse lookup

private:

    /**
     *  If set, shows the shortcut-keys on each filled pattern slot in the
     *  main window.
     */

    bool m_key_show_ui_sequence_key;

    /**
     *  If set, shows the sequence number on each filled pattern and empty
     *  pattern slot in the main window.  Also shows the sequence number as
     *  part of the sequence name in the performance window (song editor).
     *  Always disabled in legacy mode.
     */

    bool m_key_show_ui_sequence_number;

    /**
     *  Holds the mapping of keys to the pattern slots.  Do not access
     *  directly, use the set/lookup functions declared below.
     */

    SlotMap m_key_events;

    /**
     *  Holds the mapping of keys to the mute groups.  Do not access directly,
     *  use the set/lookup functions declared below.
     */

    SlotMap m_key_groups;

    /**
     *  Holds the reverse mapping of the pattern slots to the keys.  Do not
     *  access directly, use the set/lookup functions declared below.
     */

    RevSlotMap m_key_events_rev;        // reverse lookup, keep in sync!!

    /**
     *  Holds the reverse mapping of the mute groups to the keys.  Do not
     *  access directly, use the set/lookup functions declared below.
     */

    RevSlotMap m_key_groups_rev;        // reverse lookup, keep in sync!!

    /**
     *  With larger set sizes, we have to support fewer mute-groups (unless we
     *  decide to allow 32 sets even for larger sets, upping the pattern count
     *  from 1024 to 3072 or 4096).  This number is logged by the perform
     *  object to allow us to know that we can actually use the desired mute
     *  group.
     */

    int m_group_max;

    /**
     *  Provides key assignments for some key sequencer features.  Used in
     *  mainwnd, options, optionsfile, perfedit, seqroll, userfile, and
     *  perform.
     *
     *  We could instead use the keys_perform_transfer structure instead of
     *  all these individual members.
     */

    unsigned m_key_bpm_up;                  /**< BPM up, apostrophe!!!  */
    unsigned m_key_bpm_dn;                  /**< BPM down, semicolon.   */
    unsigned m_key_replace;                 /**< Replace, Ctrl-L.       */
    unsigned m_key_queue;                   /**< Queue, Ctrl-R.         */
    unsigned m_key_keep_queue;              /**< Keep queue, backslash. */
    unsigned m_key_snapshot_1;              /**< Snapshot 1, Alt-L.     */
    unsigned m_key_snapshot_2;              /**< Snapshot 1, Alt-R.     */
    unsigned m_key_screenset_up;            /**< Set up, Right-].       */
    unsigned m_key_screenset_dn;            /**< Set down, Left-[.      */
    unsigned m_key_set_playing_screenset;   /**< Set set, Home key.     */
    unsigned m_key_group_on;                /**< Group on, igrave key.  */
    unsigned m_key_group_off;               /**< Group off, apostrophe! */
    unsigned m_key_group_learn;             /**< Group learn, Insert.   */
    unsigned m_key_start;                   /**< Start play, Space key. */
    unsigned m_key_pause;                   /**< Pause play, Period.    */
    unsigned m_key_song_mode;               /**< Song versus Live mode. */
    unsigned m_key_toggle_jack;             /**< Toggle JACK connect.   */
    unsigned m_key_menu_mode;               /**< Menu enabled/disabled. */
    unsigned m_key_follow_transport;        /**< Toggle following JACK. */
    unsigned m_key_rewind;                  /**< Start rewind.          */
    unsigned m_key_fast_forward;            /**< Start fast-forward.    */
    unsigned m_key_pointer_position;        /**< Set progress to mouse. */
    unsigned m_key_toggle_mutes;            /**< Toggle all patterns.   */
    unsigned m_key_tap_bpm;                 /**< To tap out the BPM.    */
    unsigned m_key_pattern_edit;            /**< Show pattern editor.   */
    unsigned m_key_pattern_shift;           /**< Shift pattern hotkey.  */
    unsigned m_key_event_edit;              /**< Show event editor.     */
    unsigned m_key_stop;                    /**< Stop play, Escape.     */

#ifdef SEQ64_SONG_RECORDING
    unsigned m_key_song_record;             /**< Turn on song-record.   */
    unsigned m_key_oneshot_queue;           /**< Turn on 1-shot record. */
#endif

public:

    keys_perform ();
    virtual ~keys_perform ();

    void set_keys (const keys_perform_transfer & kpt);
    void get_keys (keys_perform_transfer & kpt);

    /**
     * \getter m_key_bpm_up
     */

    unsigned bpm_up () const
    {
        return m_key_bpm_up;
    }

    /**
     * \setter m_key_bpm_up
     *
     * \param x
     *      The key value to assign to the operation.
     */

    void bpm_up (unsigned x)
    {
        m_key_bpm_up = x;
    }

    /**
     * \getter m_key_bpm_dn
     */

    unsigned bpm_dn () const
    {
        return m_key_bpm_dn;
    }

    /**
     * \setter m_key_bpm_dn
     *
     * \param x
     *      The key value to assign to the operation.
     */

    void bpm_dn (unsigned x)
    {
        m_key_bpm_dn = x;
    }

    /**
     * \getter m_key_replace
     */

    unsigned replace () const
    {
        return m_key_replace;
    }

    /**
     * \setter m_key_replace
     *
     * \param x
     *      The key value to assign to the operation.
     */

    void replace (unsigned x)
    {
        m_key_replace = x;
    }

    /**
     * \getter m_key_queue
     */

    unsigned queue () const
    {
        return m_key_queue;
    }

    /**
     * \setter m_key_queue
     *
     * \param x
     *      The key value to assign to the operation.
     */

    void queue (unsigned x)
    {
        m_key_queue = x;
    }

    /**
     * \getter m_key_keep_queue
     */

    unsigned keep_queue () const
    {
        return m_key_keep_queue;
    }

    /**
     * \setter m_key_keep_queue
     *
     * \param x
     *      The key value to assign to the operation.
     */

    void keep_queue (unsigned x)
    {
        m_key_keep_queue = x;
    }

    /**
     * \getter m_key_snapshot_1
     */

    unsigned snapshot_1 () const
    {
        return m_key_snapshot_1;
    }

    /**
     * \setter m_key_snapshot_1
     *
     * \param x
     *      The key value to assign to the operation.
     */

    void snapshot_1 (unsigned x)
    {
        m_key_snapshot_1 = x;
    }

    /**
     * \getter m_key_snapshot_2
     */

    unsigned snapshot_2 () const
    {
        return m_key_snapshot_2;
    }

    /**
     * \setter m_key_snapshot_2
     *
     * \param x
     *      The key value to assign to the operation.
     */

    void snapshot_2 (unsigned x)
    {
        m_key_snapshot_2 = x;
    }

    /**
     * \getter m_key_screenset_up
     */

    unsigned screenset_up () const
    {
        return m_key_screenset_up;
    }

    /**
     * \setter m_key_screenset_up
     *
     * \param x
     *      The key value to assign to the operation.
     */

    void screenset_up (unsigned x)
    {
        m_key_screenset_up = x;
    }

    /**
     * \getter m_key_screenset_dn
     */

    unsigned screenset_dn () const
    {
        return m_key_screenset_dn;
    }

    /**
     * \setter m_key_screenset_dn
     *
     * \param x
     *      The key value to assign to the operation.
     */

    void screenset_dn (unsigned x)
    {
        m_key_screenset_dn = x;
    }

    /**
     * \getter m_key_playing_screenset
     */

    unsigned set_playing_screenset () const
    {
        return m_key_set_playing_screenset;
    }

    /**
     * \setter m_key_playing_screenset
     *
     * \param x
     *      The key value to assign to the operation.
     */

    void set_playing_screenset (unsigned x)
    {
        m_key_set_playing_screenset = x;
    }

    /**
     * \getter m_key_group_on
     */

    unsigned group_on () const
    {
        return m_key_group_on;
    }

    /**
     * \setter m_key_group_on
     *
     * \param x
     *      The key value to assign to the operation.
     */

    void group_on (unsigned x)
    {
        m_key_group_on = x;
    }

    /**
     * \getter m_key_group_off
     */

    unsigned group_off () const
    {
        return m_key_group_off;
    }

    /**
     * \setter m_key_group_off
     *
     * \param x
     *      The key value to assign to the operation.
     */

    void group_off (unsigned x)
    {
        m_key_group_off = x;
    }

    /**
     * \getter m_key_group_learn
     */

    unsigned group_learn () const
    {
        return m_key_group_learn;
    }

    /**
     * \setter m_key_group_learn
     *
     * \param x
     *      The key value to assign to the operation.
     */

    void group_learn (unsigned x)
    {
        m_key_group_learn = x;
    }

    /**
     * \getter m_key_start
     */

    unsigned start () const
    {
        return m_key_start;
    }

    /**
     * \setter m_key_start
     *
     * \param x
     *      The key value to assign to the operation.
     */

    void start (unsigned x)
    {
        m_key_start = x;
    }

    /**
     * \getter m_key_pause
     */

    unsigned pause () const
    {
        return m_key_pause;
    }

    /**
     * \setter m_key_pause
     *
     * \param x
     *      The key value to assign to the operation.
     */

    void pause (unsigned x)
    {
        m_key_pause = x;
    }

    /**
     * \getter m_key_pattern_edit
     */

    unsigned pattern_edit () const
    {
        return m_key_pattern_edit;
    }

    /**
     * \setter m_key_pattern_edit
     *
     * \param x
     *      The key value to assign to the operation.
     */

    void pattern_edit (unsigned x)
    {
        m_key_pattern_edit = x;
    }

    /**
     * \getter m_key_pattern_shift
     */

    unsigned pattern_shift () const
    {
        return m_key_pattern_shift;
    }

    /**
     * \setter m_key_pattern_shift
     *
     * \param x
     *      The key value to assign to the operation.
     */

    void pattern_shift (unsigned x)
    {
        m_key_pattern_shift = x;
    }

    /**
     * \getter m_key_event_edit
     */

    unsigned event_edit () const
    {
        return m_key_event_edit;
    }

    /**
     * \setter m_key_event_edit
     *
     * \param x
     *      The key value to assign to the operation.
     */

    void event_edit (unsigned x)
    {
        m_key_event_edit = x;
    }

    /**
     * \getter m_key_stop
     */

    unsigned stop () const
    {
        return m_key_stop;
    }

    /**
     * \setter m_key_stop
     *
     * \param x
     *      The key value to assign to the operation.
     */

    void stop (unsigned x)
    {
        m_key_stop = x;
    }

    unsigned song_mode () const
    {
        return m_key_song_mode;
    }

    void song_mode (unsigned key)
    {
        m_key_song_mode = key;
    }

    unsigned menu_mode () const
    {
        return m_key_menu_mode;
    }

    void menu_mode (unsigned key)
    {
        m_key_menu_mode = key;
    }

    unsigned follow_transport () const
    {
        return m_key_follow_transport;
    }

    void follow_transport (unsigned key)
    {
        m_key_follow_transport = key;
    }

    unsigned fast_forward () const
    {
        return m_key_fast_forward;
    }

    void fast_forward (unsigned key)
    {
        m_key_fast_forward = key;
    }

    unsigned rewind () const
    {
        return m_key_rewind;
    }

    void rewind (unsigned key)
    {
        m_key_rewind = key;
    }

    unsigned pointer_position () const
    {
        return m_key_pointer_position;
    }

    void pointer_position (unsigned key)
    {
        m_key_pointer_position = key;
    }

    unsigned toggle_mutes () const
    {
        return m_key_toggle_mutes;
    }

    void toggle_mutes (unsigned key)
    {
        m_key_toggle_mutes = key;
    }

    unsigned toggle_jack () const
    {
        return m_key_toggle_jack;
    }

    void toggle_jack (unsigned key)
    {
        m_key_toggle_jack = key;
    }

    unsigned tap_bpm () const
    {
        return m_key_tap_bpm;
    }

    void tap_bpm (unsigned key)
    {
        m_key_tap_bpm = key;
    }

#ifdef SEQ64_SONG_RECORDING

    unsigned song_record () const
    {
        return m_key_song_record;
    }

    void song_record (unsigned key)
    {
        m_key_song_record = key;
    }

    unsigned oneshot_queue () const
    {
        return m_key_oneshot_queue;
    }

    void oneshot_queue (unsigned key)
    {
        m_key_oneshot_queue = key;
    }

#endif  // SEQ64_SONG_RECORDING

    /**
     * \getter m_key_show_ui_sequency_key
     *
     *  Used in mainwid, options, optionsfile, userfile, and perform.
     */

    bool show_ui_sequence_key () const
    {
        return m_key_show_ui_sequence_key;
    }

    /**
     * \setter m_key_show_ui_sequency_key
     *
     * \param flag
     *      The flag for showing the sequence key characters in each pattern
     *      slot.
     */

    void show_ui_sequence_key (bool flag)
    {
        m_key_show_ui_sequence_key = flag;
    }

    /**
     * \getter m_key_show_ui_sequence_number
     *
     *  Used in mainwid, options, optionsfile, userfile, and perform.
     */

    bool show_ui_sequence_number () const
    {
        return m_key_show_ui_sequence_number;
    }

    /**
     * \setter m_key_show_ui_sequence_key
     *
     * \param flag
     *      The flag for showing the sequence number in each pattern
     *      slot.
     */

    void show_ui_sequence_number (bool flag)
    {
        m_key_show_ui_sequence_number = flag;
    }

    /**
     * \getter m_key_events
     */

    SlotMap & get_key_events ()
    {
        return m_key_events;
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
        return m_key_events.count(k);
    }

    /**
     * \getter m_key_groups
     */

    SlotMap & get_key_groups ()
    {
        return m_key_groups;
    }

    /**
     * \getter m_key_events_rev
     */

    RevSlotMap & get_key_events_rev ()
    {
        return m_key_events_rev;
    }

    /**
     * \getter m_key_groups_rev
     */

    RevSlotMap & get_key_groups_rev ()
    {
        return m_key_groups_rev;
    }

    /*
     * Getters of keyboard mapping for sequence and groups.  If not found,
     * returns something "safe" [so use get_key()->count() to see if it's
     * there first]
     */

    unsigned lookup_keyevent_key (int seqnum);
    unsigned lookup_keygroup_key (int groupnum);
    int lookup_keyevent_seq (unsigned keycode);
    int lookup_keygroup_group (unsigned keycode);

    virtual std::string key_name (unsigned key) const;

    /**
     *  Provides base class functionality.  Must be called by the
     *  derived-class's override of this function.
     */

    virtual void set_all_key_events ()
    {
        m_key_events.clear();
        m_key_events_rev.clear();
    }

    /**
     *  Provides base class functionality.  Must be called by the
     *  derived-class's override of this function.
     */

    virtual void set_all_key_groups ()
    {
        m_key_groups.clear();
        m_key_groups_rev.clear();
    }

    void set_key_event (unsigned keycode, int sequence_slot);
    void set_key_group (unsigned keycode, int group_slot);

    /**
     * \getter m_group_max
     */

    int group_max () const
    {
        return m_group_max;
    }

protected:

    /**
     * \setter m_group_max
     */

    void group_max (int groupcount)
    {
        m_group_max = groupcount;
    }

    /**
     * The following are tricky ways to get at address of the key and group
     * operation values so that we don't directly expose the members to
     * manipulation.  They are used in the options module, and, for brevity,
     * are accessed using the PREFKEY_ADDR() macro.
     */

    /**
     * \getter m_key_bpm_up
     *  Address getter for the bpm_up operation.
     */

    unsigned * at_bpm_up ()
    {
        return &m_key_bpm_up;
    }

    /**
     * \getter m_key_bpm_dn
     *  Address getter for the bpm_dn operation.
     */

    unsigned * at_bpm_dn ()
    {
        return & m_key_bpm_dn;
    }

    /**
     * \getter m_key_replace
     *  Address getter for the replace operation.
     */

    unsigned * at_replace ()
    {
        return &m_key_replace;
    }

    /**
     * \getter m_key_queue
     *  Address getter for the queue operation.
     */

    unsigned * at_queue ()
    {
        return &m_key_queue;
    }

    /**
     * \getter m_key_keep_queue
     *  Address getter for the keep_queue operation.
     */

    unsigned * at_keep_queue ()
    {
        return &m_key_keep_queue;
    }

    /**
     * \getter m_key_snapshot_1
     *  Address getter for the snapshot_1 operation.
     */

    unsigned * at_snapshot_1 ()
    {
        return &m_key_snapshot_1;
    }

    /**
     * \getter m_key_snapshot_2
     *  Address getter for the snapshot_2 operation.
     */

    unsigned * at_snapshot_2 ()
    {
        return &m_key_snapshot_2;
    }

    /**
     * \getter m_key_screenset_up
     *  Address getter for the screenset_up operation.
     */

    unsigned * at_screenset_up ()
    {
        return &m_key_screenset_up;
    }

    /**
     * \getter m_key_screenset_dn
     *  Address getter for the screenset_dn operation.
     */

    unsigned * at_screenset_dn ()
    {
        return &m_key_screenset_dn;
    }

    /**
     * \getter m_key_playing_screenset
     *  Address getter for the set_playing_screenset operation.
     */

    unsigned * at_set_playing_screenset ()
    {
        return &m_key_set_playing_screenset;
    }

    /**
     * \getter m_key_group_on
     *  Address getter for the group_on operation.
     */

    unsigned * at_group_on ()
    {
        return &m_key_group_on;
    }

    /**
     * \getter m_key_group_off
     *  Address getter for the group_off operation.
     */

    unsigned * at_group_off ()
    {
        return &m_key_group_off;
    }

    /**
     * \getter m_key_group_learn
     *  Address getter for the group_learn operation.
     */

    unsigned * at_group_learn ()
    {
        return &m_key_group_learn;
    }

    /**
     * \getter m_key_start
     *  Address getter for the start operation.
     */

    unsigned * at_start ()
    {
        return &m_key_start;
    }

    /**
     * \getter m_key_pause
     *  Address getter for the pause operation.
     */

    unsigned * at_pause ()
    {
        return &m_key_pause;
    }

    /**
     * \getter m_key_song_mode
     *  Address getter for the song-mode operation.
     */

    unsigned * at_song_mode ()
    {
        return &m_key_song_mode;
    }

    /**
     * \getter m_key_toggle_jack
     *  Address getter for the toggle-jack operation.
     */

    unsigned * at_toggle_jack ()
    {
        return &m_key_toggle_jack;
    }

    /**
     * \getter m_key_menu_mode
     *  Address getter for the menu-mode operation.
     */

    unsigned * at_menu_mode ()
    {
        return &m_key_menu_mode;
    }

    /**
     * \getter m_key_follow_transport
     *  Address getter for the follow-transport operation.
     */

    unsigned * at_follow_transport ()
    {
        return &m_key_follow_transport;
    }

    /**
     * \getter m_key_fast_forward
     *  Address getter for the fast-forward operation.
     */

    unsigned * at_fast_forward ()
    {
        return &m_key_fast_forward;
    }

    /**
     * \getter m_key_rewind
     *  Address getter for the rewind operation.
     */

    unsigned * at_rewind ()
    {
        return &m_key_rewind;
    }

    /**
     * \getter m_key_pointer_position
     *  Address getter for the pointer operation.
     */

    unsigned * at_pointer_position ()
    {
        return &m_key_pointer_position;
    }

    /**
     * \getter m_key_toggle_mutes
     *  Address getter for the toggle-mutes operation.
     */

    unsigned * at_toggle_mutes ()
    {
        return &m_key_toggle_mutes;
    }

    /**
     * \getter m_key_tap_bpm
     *  Address getter for the tap_bpm operation.
     */

    unsigned * at_tap_bpm ()
    {
        return &m_key_tap_bpm;
    }

#ifdef SEQ64_SONG_RECORDING

    /**
     * \getter m_key_song_record
     *  Address getter for the song-record operation.
     */

    unsigned * at_song_record ()
    {
        return &m_key_song_record;
    }

    /**
     * \getter m_key_oneshot_queue
     *  Address getter for the oneshot-record operation.
     */

    unsigned * at_oneshot_queue ()
    {
        return &m_key_oneshot_queue;
    }

#endif

    /**
     * \getter m_key_pattern_edit
     *  Address getter for the pattern-edit operation.
     */

    unsigned * at_pattern_edit ()
    {
        return &m_key_pattern_edit;
    }

    /**
     * \getter m_key_pattern_shift
     *  Address getter for the pattern-shift operation.
     */

    unsigned * at_pattern_shift ()
    {
        return &m_key_pattern_shift;
    }

    /**
     * \getter m_key_event_edit
     *  Address getter for the event-edit operation.
     */

    unsigned * at_event_edit ()
    {
        return &m_key_event_edit;
    }

    /**
     * \getter m_key_stop
     *  Address getter for the stop operation.
     */

    unsigned * at_stop ()
    {
        return &m_key_stop;
    }

    /**
     * \getter m_key_show_ui_sequence_key
     *  Address getter for the show_ui_sequence_key value.
     */

    bool * at_show_ui_sequence_key ()
    {
        return &m_key_show_ui_sequence_key;
    }

    /**
     * \getter m_key_show_ui_sequence_number
     *  Address getter for the show_ui_sequence_number value.
     */

    bool * at_show_ui_sequence_number ()
    {
        return &m_key_show_ui_sequence_number;
    }

    /*
     * These functions set up the keys common to all GUI frameworks.
     */

    virtual void set_basic_key_events ();
    virtual void set_basic_key_groups ();

};          // class keys_perform

/**
 * Free functions for keyboard support.  The implementation of this function
 * will ultimately depend on the GUI environment; currently it is GTK 2.x, so
 * the implementation is in seq_gtkmm2/src/keys_perform_gtk2.cpp.
 */

inline bool
invalid_key (unsigned key)
{
    return (key == 0) || (key > 0xffff);
}

extern void keyval_normalize (keys_perform_transfer & k);

}           // namespace seq64

#endif      // SEQ64_KEYS_PERFORM_HPP

/*
 * keys_perform.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

