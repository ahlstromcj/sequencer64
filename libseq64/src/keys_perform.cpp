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
 * \file          keys_perform.cpp
 *
 *  This module defines base-class keyboard interface items, VERY TENTATIVE
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-09-13
 * \updates       2015-09-13
 * \license       GNU GPLv2 or above
 *
 */

#include "gdk_basic_keys.h"
#include "keys_perform.hpp"

namespace seq64
{

/**
 *  This construction initializes a vast number of member variables, some
 *  of them public!
 */

keys_perform::keys_perform ()
 :
    m_show_ui_sequence_key      (true),
    m_key_events                (),
    m_key_groups                (),
    m_key_events_rev            (),
    m_key_groups_rev            (),
    m_key_bpm_up                (GDK_KEY_apostrophe),
    m_key_bpm_dn                (GDK_KEY_semicolon),
    m_key_replace               (GDK_KEY_Control_L),
    m_key_queue                 (GDK_KEY_Control_R),
    m_key_keep_queue            (GDK_KEY_backslash),
    m_key_snapshot_1            (GDK_KEY_Alt_L),
    m_key_snapshot_2            (GDK_KEY_Alt_R),
    m_key_screenset_up          (GDK_KEY_bracketright),
    m_key_screenset_dn          (GDK_KEY_bracketleft),
    m_key_set_playing_screenset (GDK_KEY_Home),
    m_key_group_on              (GDK_KEY_igrave),
    m_key_group_off             (GDK_KEY_apostrophe),       // a repeat
    m_key_group_learn           (GDK_KEY_Insert),
    m_key_start                 (GDK_KEY_space),
    m_key_stop                  (GDK_KEY_Escape)
{
    // set_all_key_events();
    // set_all_key_groups();
}

/**
 *  The destructor sets some running flags to false, signals this
 *  condition, then joins the input and output threads if the were
 *  launched. Finally, any active patterns/sequences are deleted.
 */

keys_perform::~keys_perform ()
{
    // what to do?
}

/**
 *  Copies fields from the transfer structure in this object.
 */

void
keys_perform::set_keys (const keys_perform_transfer & kpt)
{
    m_key_bpm_up                = kpt.kpt_bpm_up;
    m_key_bpm_dn                = kpt.kpt_bpm_dn;
    m_key_replace               = kpt.kpt_replace;
    m_key_queue                 = kpt.kpt_queue;
    m_key_keep_queue            = kpt.kpt_keep_queue;
    m_key_snapshot_1            = kpt.kpt_snapshot_1;
    m_key_snapshot_2            = kpt.kpt_snapshot_2;
    m_key_screenset_up          = kpt.kpt_screenset_up;
    m_key_screenset_dn          = kpt.kpt_screenset_dn;
    m_key_set_playing_screenset = kpt.kpt_set_playing_screenset;
    m_key_group_on              = kpt.kpt_group_on;
    m_key_group_off             = kpt.kpt_group_off;
    m_key_group_learn           = kpt.kpt_group_learn;
    m_key_start                 = kpt.kpt_start;
    m_key_stop                  = kpt.kpt_stop;
}

/**
 *  Copies fields from this object into the transfer structure.
 */

void
keys_perform::get_keys (keys_perform_transfer & kpt)
{
     kpt.kpt_bpm_up                  = m_key_bpm_up;
     kpt.kpt_bpm_dn                  = m_key_bpm_dn;
     kpt.kpt_replace                 = m_key_replace;
     kpt.kpt_queue                   = m_key_queue;
     kpt.kpt_keep_queue              = m_key_keep_queue;
     kpt.kpt_snapshot_1              = m_key_snapshot_1;
     kpt.kpt_snapshot_2              = m_key_snapshot_2;
     kpt.kpt_screenset_up            = m_key_screenset_up;
     kpt.kpt_screenset_dn            = m_key_screenset_dn;
     kpt.kpt_set_playing_screenset   = m_key_set_playing_screenset;
     kpt.kpt_group_on                = m_key_group_on;
     kpt.kpt_group_off               = m_key_group_off;
     kpt.kpt_group_learn             = m_key_group_learn;
     kpt.kpt_start                   = m_key_start;
     kpt.kpt_stop                    = m_key_stop;
}

/**
 *  At construction time, this function sets up one keycode and one event
 *  slot.
 *
 *  It is called 32 times, corresponding the pattern/sequence slots in the
 *  Patterns window.
 */

void
keys_perform::set_key_event (unsigned int keycode, long sequence_slot)
{
    SlotMap::iterator it1 = m_key_events.find(keycode);
    if (it1 != m_key_events.end())
    {
        RevSlotMap::iterator i = m_key_events_rev.find(it1->second);
        if (i != m_key_events_rev.end())
            m_key_events_rev.erase(i);          /* unhook previous binding  */

        m_key_events.erase(it1);                /* unhook the reverse one   */
    }
    RevSlotMap::iterator it2 = m_key_events_rev.find(sequence_slot);
    if (it2 != m_key_events_rev.end())
    {
        SlotMap::iterator i = m_key_events.find(it2->second);
        if (i != m_key_events.end())
            m_key_events.erase(i);              /* unhook previous binding  */

        m_key_events_rev.erase(it2);            /* unhook the reverse one   */
    }
    m_key_events[keycode] = sequence_slot;      /* set the new binding      */
    m_key_events_rev[sequence_slot] = keycode;
}

/**
 *  At construction time, this function sets up one keycode and one group
 *  slot.
 *
 *  It is called 32 times, corresponding the pattern/sequence slots in the
 *  Patterns window.
 */

void
keys_perform::set_key_group (unsigned int keycode, long group_slot)
{
    SlotMap::iterator it1 = m_key_groups.find(keycode);
    if (it1 != m_key_groups.end())
    {
        RevSlotMap::iterator i = m_key_groups_rev.find(it1->second);
        if (i != m_key_groups_rev.end())
            m_key_groups_rev.erase(i);          /* unhook previous binding  */

        m_key_groups.erase(it1);
    }
    RevSlotMap::iterator it2 = m_key_groups_rev.find(group_slot);
    if (it2 != m_key_groups_rev.end())
    {
        SlotMap::iterator i = m_key_groups.find(it2->second);
        if (i != m_key_groups.end())
            m_key_groups.erase(i);              /* unhook previous binding  */

        m_key_groups_rev.erase(it2);
    }
    m_key_groups[keycode] = group_slot;         /* set the new binding      */
    m_key_groups_rev[group_slot] = keycode;
}

}           // namespace seq64

/*
 * keys_perform.cpp
 *
 * vim: sw=4 ts=4 wm=8 et ft=cpp
 */
