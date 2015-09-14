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
 * \updates       2015-09-14
 * \license       GNU GPLv2 or above
 *
 *  This class has way too many members.
 */

#include <map>                          /* std::map */

namespace seq64
{

/**
 *  Provides a data-transfer structure to make it easier to fill in a
 *  keys_perform object's members using sscanf().
 */

struct keys_perform_transfer
{
   unsigned int kpt_bpm_up;
   unsigned int kpt_bpm_dn;
   unsigned int kpt_screenset_up;
   unsigned int kpt_screenset_dn;
   unsigned int kpt_set_playing_screenset;
   unsigned int kpt_group_on;
   unsigned int kpt_group_off;
   unsigned int kpt_group_learn;
   unsigned int kpt_replace;
   unsigned int kpt_queue;
   unsigned int kpt_keep_queue;
   unsigned int kpt_snapshot_1;
   unsigned int kpt_snapshot_2;
   bool kpt_show_ui_sequence_key;
   unsigned int kpt_start;
   unsigned int kpt_stop;
};

/**
 *  This class supports the performance mode.
 *
 *  It has way too many data members, many of the public.
 *  Might be ripe for refactoring.
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

    typedef std::map<unsigned int, long> SlotMap;       // lookup

    /**
     *  This typedef is like SlotMap, but used for lookup in the other
     *  direction.
     */

    typedef std::map<long, unsigned int> RevSlotMap;    // reverse lookup

private:

    bool m_show_ui_sequence_key;

    /*
     *  Do not access these directly, use set/lookup functions declared
     *  below.
     */

    SlotMap m_key_events;
    SlotMap m_key_groups;
    RevSlotMap m_key_events_rev; // reverse lookup, keep in sync!!
    RevSlotMap m_key_groups_rev; // reverse lookup, keep in sync!!

    /**
     *  Provides key assignments for some key sequencer features.
     *
     *  Used in mainwnd, options, optionsfile, perfedit, seqroll,
     *  userfile, and perform.
     */

    unsigned int m_key_bpm_up;
    unsigned int m_key_bpm_dn;
    unsigned int m_key_replace;
    unsigned int m_key_queue;
    unsigned int m_key_keep_queue;
    unsigned int m_key_snapshot_1;
    unsigned int m_key_snapshot_2;
    unsigned int m_key_screenset_up;
    unsigned int m_key_screenset_dn;
    unsigned int m_key_set_playing_screenset;
    unsigned int m_key_group_on;
    unsigned int m_key_group_off;
    unsigned int m_key_group_learn;
    unsigned int m_key_start;
    unsigned int m_key_stop;

public:

    keys_perform ();
    ~keys_perform ();

    void set_keys (const keys_perform_transfer & kpt);
    void get_keys (keys_perform_transfer & kpt);

    unsigned int bpm_up () const
    {
        return m_key_bpm_up;
    }
    void bpm_up (unsigned int x)
    {
        m_key_bpm_up = x;
    }

    unsigned int bpm_dn () const
    {
        return m_key_bpm_dn;
    }
    void bpm_dn (unsigned int x)
    {
        m_key_bpm_dn = x;
    }

    unsigned int replace () const
    {
        return m_key_replace;
    }
    void replace (unsigned int x)
    {
        m_key_replace = x;
    }

    unsigned int queue () const
    {
        return m_key_queue;
    }
    void queue (unsigned int x)
    {
        m_key_queue = x;
    }

    unsigned int keep_queue () const
    {
        return m_key_keep_queue;
    }
    void keep_queue (unsigned int x)
    {
        m_key_keep_queue = x;
    }

    unsigned int snapshot_1 () const
    {
        return m_key_snapshot_1;
    }
    void snapshot_1 (unsigned int x)
    {
        m_key_snapshot_1 = x;
    }

    unsigned int snapshot_2 () const
    {
        return m_key_snapshot_2;
    }
    void snapshot_2 (unsigned int x)
    {
        m_key_snapshot_2 = x;
    }

    unsigned int screenset_up () const
    {
        return m_key_screenset_up;
    }
    void screenset_up (unsigned int x)
    {
        m_key_screenset_up = x;
    }

    unsigned int screenset_dn () const
    {
        return m_key_screenset_dn;
    }
    void screenset_dn (unsigned int x)
    {
        m_key_screenset_dn = x;
    }

    unsigned int set_playing_screenset () const
    {
        return m_key_set_playing_screenset;
    }
    void set_playing_screenset (unsigned int x)
    {
        m_key_set_playing_screenset = x;
    }

    unsigned int group_on () const
    {
        return m_key_group_on;
    }
    void group_on (unsigned int x)
    {
        m_key_group_on = x;
    }

    unsigned int group_off () const
    {
        return m_key_group_off;
    }
    void group_off (unsigned int x)
    {
        m_key_group_off = x;
    }

    unsigned int group_learn () const
    {
        return m_key_group_learn;
    }
    void group_learn (unsigned int x)
    {
        m_key_group_learn = x;
    }

    unsigned int start () const
    {
        return m_key_start;
    }
    void start (unsigned int x)
    {
        m_key_start = x;
    }

    unsigned int stop () const
    {
        return m_key_stop;
    }
    void stop (unsigned int x)
    {
        m_key_stop = x;
    }

    SlotMap & get_key_events ()
    {
        return m_key_events;
    }

    SlotMap & get_key_groups ()
    {
        return m_key_groups;
    }

    /**
     * \accessor m_show_ui_sequency_key
     *
     *  Used in mainwid, options, optionsfile, userfile, and perform.
     */

    bool show_ui_sequence_key () const
    {
        return m_show_ui_sequence_key;
    }
    void show_ui_sequence_key (bool flag)
    {
        m_show_ui_sequence_key = flag;
    }

    /*
     * Getters of keyboard mapping for sequence and groups.
     * If not found, returns something "safe" [so use get_key()->count()
     * to see if it's there first]
     */

    unsigned int lookup_keyevent_key (long seqnum)
    {
        if (m_key_events_rev.count(seqnum) > 0)
            return m_key_events_rev[seqnum];
        else
            return '?';
    }
    long lookup_keyevent_seq (unsigned int keycode)
    {
        if (m_key_events.count(keycode) > 0)
            return m_key_events[keycode];
        else
            return 0;
    }
    unsigned int lookup_keygroup_key (long groupnum)
    {
        if (m_key_groups_rev.count(groupnum) > 0)
            return m_key_groups_rev[groupnum];
        else
            return '?';
    }
    long lookup_keygroup_group (unsigned int keycode)
    {
        if (m_key_groups.count(keycode) > 0)
            return m_key_groups[keycode];
        else
            return 0;
    }

    virtual void set_all_key_events ()
    {
        // no code in base class
    }
    virtual void set_all_key_groups ()
    {
        // no code in base class
    }

    void set_key_event (unsigned int keycode, long sequence_slot);
    void set_key_group (unsigned int keycode, long group_slot);

protected:

    unsigned int * at_bpm_up ()
    {
        return &m_key_bpm_up;
    }
    unsigned int * at_bpm_dn ()
    {
        return & m_key_bpm_dn;
    }
    unsigned int * at_replace ()
    {
        return &m_key_replace;
    }
    unsigned int * at_queue ()
    {
        return &m_key_queue;
    }
    unsigned int * at_keep_queue ()
    {
        return &m_key_keep_queue;
    }
    unsigned int * at_snapshot_1 ()
    {
        return &m_key_snapshot_1;
    }
    unsigned int * at_snapshot_2 ()
    {
        return &m_key_snapshot_2;
    }
    unsigned int * at_screenset_up ()
    {
        return &m_key_screenset_up;
    }
    unsigned int * at_screenset_dn ()
    {
        return &m_key_screenset_dn;
    }
    unsigned int * at_set_playing_screenset ()
    {
        return &m_key_set_playing_screenset;
    }
    unsigned int * at_group_on ()
    {
        return &m_key_group_on;
    }
    unsigned int * at_group_off ()
    {
        return &m_key_group_off;
    }
    unsigned int * at_group_learn ()
    {
        return &m_key_group_learn;
    }
    unsigned int * at_start ()
    {
        return &m_key_start;
    }
    unsigned int * at_stop ()
    {
        return &m_key_stop;
    }
    bool * at_show_ui_sequence_key ()
    {
        return &m_show_ui_sequence_key;
    }

};

}           // namespace seq64

#endif      // SEQ64_KEYS_PERFORM_HPP

/*
 * keys_perform.hpp
 *
 * vim: sw=4 ts=4 wm=8 et ft=cpp
 */

