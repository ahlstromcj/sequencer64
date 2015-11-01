#ifndef SEQ64_TRIGGERS_HPP
#define SEQ64_TRIGGERS_HPP

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
 * \file          triggers.hpp
 *
 *  This module declares/defines the base class for handling
 *  triggers used with patterns/sequences.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-10-30
 * \updates       2015-11-01
 * \license       GNU GPLv2 or above
 *
 *  By segregating trigger support into its own module, the sequence class is a
 *  bit easier to understand.
 */

#include <string>
#include <list>
#include <stack>

// #include "easy_macros.h"

namespace seq64
{

class sequence;

/**
 *  This class hold a single trigger for a sequence object.
 *  This class is used in playback, and is contained in the triggers class.
 */

class trigger
{

private:

    /**
     *  Provides the starting tick for this trigger.
     */

    long m_tick_start;

    /**
     *  Provides the ending tick for this trigger.
     */

    long m_tick_end;

    /**
     *  Provides the offset for this trigger.
     */

    long m_offset;

    /**
     *  Indicates that the trigger is part of a selection.
     */

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

    /**
     * \getter m_tick_start
     */

    long tick_start () const
    {
        return m_tick_start;
    }

    /**
     * \setter m_tick_start
     */

    void tick_start (long s)
    {
        m_tick_start = s;
    }

    /**
     * \setter m_tick_start
     */

    void increment_tick_start (long s)
    {
        m_tick_start += s;
    }

    /**
     * \setter m_tick_start
     */

    void decrement_tick_start (long s)
    {
        m_tick_start -= s;
    }

    /**
     * \getter m_tick_end
     */

    long tick_end () const
    {
        return m_tick_end;
    }

    /**
     * \setter m_tick_end
     */

    void tick_end (long e)
    {
        m_tick_end = e;
    }

    /**
     * \setter m_tick_end
     */

    void increment_tick_end (long s)
    {
        m_tick_end += s;
    }

    /**
     * \setter m_tick_end
     */

    void decrement_tick_end (long s)
    {
        m_tick_end -= s;
    }

    /**
     * \getter m_offset
     */

    long offset () const
    {
        return m_offset;
    }

    /**
     * \setter m_offset
     */

    void offset (long o)
    {
        m_offset = o;
    }

    /**
     * \setter m_offset
     */

    void increment_offset (long s)
    {
        m_offset += s;
    }

    /**
     * \setter m_offset
     */

    void decrement_offset (long s)
    {
        m_offset -= s;
    }

    /**
     * \getter m_selected
     */

    bool selected () const
    {
        return m_selected;
    }

    /**
     * \setter m_selected
     */

    void selected (bool s)
    {
        m_selected = s;
    }

};          // class trigger

/**
 *  The triggers class is a receptable the triggers that can be used with a
 *  sequence object.
 */

class triggers
{

public:

    /**
     *  Exposes the triggers type, currently needed for midi_container only.
     */

    typedef std::list<trigger> List;

private:

    typedef std::stack<List> Stack;

private:

    /**
     *  Holds a reference to the parent sequence object that owns this trigger
     *  object.
     */

    sequence & m_parent;

    /**
     *  This list holds the current pattern/triggers events.
     */

    List m_triggers;

    /**
     *  This item holds a single copied trigger, to be pasted later.
     */

    trigger m_clipboard;

    /**
     *  Handles the undo list for a series of operations on triggers.
     */

    Stack m_undo_stack;

    /**
     *  Handles the redo list for a series of operations on triggers.
     */

    Stack m_redo_stack;

    /**
     *  An iterator for cycling through the triggers during playback.
     */

    List::iterator m_iterator_play_trigger;

    /**
     *  An iterator for cycling through the triggers during drawing.
     */

    List::iterator m_iterator_draw_trigger;

    /**
     *  Set to true if there is an active trigger in the trigger clipboard.
     */

    bool m_trigger_copied;

    /**
     *  Offset.
     */

    long m_trigger_offset;

    /**
     *  Holds the value of the PPQN from the parent sequence, for easy access.
     *  This should not change, but we have to set it after construction, and
     *  so we provide a setter for it, set_ppqn(), called by the sequence
     *  constructor.
     */

    int m_ppqn;

    /**
     *  Holds the value of the length from the parent sequence, for easy access.
     *  This might change, we're not yet sure.
     */

    int m_length;

public:

    triggers (sequence & parent);
    ~triggers ();

    triggers & operator = (const triggers & rhs);

    /**
     * \setter m_ppqn
     *      We have to set this value after construction for best safety.
     */

    void set_ppqn (int ppqn)
    {
        if (ppqn > 0)
            m_ppqn = ppqn;
    }

    /**
     * \setter m_length
     *      We have to set this value after construction for best safety.
     *      Also, there a chance that the length of the parent might change
     *      from time to time.  Currently, only the sequence constructor and
     *      midifile call this function.
     */

    void set_length (int len)
    {
        if (len > 0)
            m_length = len;
    }

    /**
     * \getter m_triggers
     */

    List & triggerlist ()
    {
        return m_triggers;
    }
    void push_undo ();
    void pop_undo ();
    void print (const std::string & seqname);
    bool play (long & start_tick, long & end_tick);

    void add
    (
        long tick, long len, long offset = 0, bool adjustoffset = true
    );
    void adjust_offsets_to_length (long newlen);
    void split (long tick);
    void split (trigger & trig, long split_tick);
    void grow (long tick_from, long tick_to, long length);
    void remove (long tick);
    bool get_state (long tick);
    bool select (long tick);
    bool unselect ();
    bool intersect (long position, long & start, long & end);
    void remove_selected ();
    void copy_selected ();
    void paste ();
    bool move_selected
    (
        long tick, bool adjustoffset, int which = 2
    );
    long get_selected_start ();
    long get_selected_end ();
    long get_maximum ();
    void move (long start_tick, long distance, bool direction);
    void copy (long start_tick, long distance);

    /**
     *  Clears the whole list of triggers.
     */

    void clear ()
    {
        m_triggers.clear();
    }

    /**
     * \getter m_trigger_offset

    long get_trigger_offset () const
    {
        return m_trigger_offset;
    }
     */

    bool next
    (
        long * tick_on, long * tick_off,
        bool * selected, long * tick_offset
    );
    trigger next_trigger ();

    /**
     *  Sets the draw-trigger iterator to the beginning of the trigger list.
     */

    void reset_draw_trigger_marker ()
    {
        m_iterator_draw_trigger = m_triggers.begin();
    }

private:

//  void set_trigger_offset (long trigger_offset);
    long adjust_offset (long offset);

};          // class triggers

}           // namespace seq64

#endif      // SEQ64_TRIGGERS_HPP

/*
 * triggers.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

