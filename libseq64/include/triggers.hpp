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
 * \updates       2017-10-22
 * \license       GNU GPLv2 or above
 *
 *  By segregating trigger support into its own module, the sequence class is
 *  a bit easier to understand.
 */

#include <string>
#include <list>
#include <stack>

/**
 *  Indicates that there is no paste-trigger.  This is a new feature from the
 *  stazed/seq32 code.
 */

#define SEQ64_NO_PASTE_TRIGGER          (-1)

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

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

    midipulse m_tick_start;

    /**
     *  Provides the ending tick for this trigger.
     */

    midipulse m_tick_end;

    /**
     *  Provides the offset for this trigger.
     */

    midipulse m_offset;

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
     *
     * \param rhs
     *      The "right-hand side" of the less-than operation.
     *
     * \return
     *      Returns true if m_tick_start is less than rhs's.
     */

    bool operator < (const trigger & rhs)
    {
        return m_tick_start < rhs.m_tick_start;
    }

    /**
     * \getter m_tick_end and m_tick_start.
     *      We've seen that some of the calculations of trigger length are
     *      wrong, being 1 tick less than the true length of the trigger in
     *      pulses.  This function calculates trigger length the correct way.
     */

    midipulse length () const
    {
        return m_tick_end - m_tick_start + 1;
    }

    /**
     * \getter m_tick_start
     */

    midipulse tick_start () const
    {
        return m_tick_start;
    }

    /**
     * \setter m_tick_start
     */

    void tick_start (midipulse s)
    {
        m_tick_start = s;
    }

    /**
     * \setter m_tick_start
     */

    void increment_tick_start (midipulse s)
    {
        m_tick_start += s;
    }

    /**
     * \setter m_tick_start
     */

    void decrement_tick_start (midipulse s)
    {
        m_tick_start -= s;
    }

    /**
     * \getter m_tick_end
     */

    midipulse tick_end () const
    {
        return m_tick_end;
    }

    /**
     * \setter m_tick_end
     */

    void tick_end (midipulse e)
    {
        m_tick_end = e;
    }

    /**
     * \setter m_tick_end
     */

    void increment_tick_end (midipulse s)
    {
        m_tick_end += s;
    }

    /**
     * \setter m_tick_end
     */

    void decrement_tick_end (midipulse s)
    {
        m_tick_end -= s;
    }

    /**
     * \getter m_offset
     */

    midipulse offset () const
    {
        return m_offset;
    }

    /**
     * \setter m_offset
     */

    void offset (midipulse o)
    {
        m_offset = o;
    }

    /**
     * \setter m_offset
     */

    void increment_offset (midipulse s)
    {
        m_offset += s;
    }

    /**
     * \setter m_offset
     */

    void decrement_offset (midipulse s)
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
    friend class midi_container;
    friend class midifile;
    friend class sequence;
    friend class Seq24PerfInput;        /* we need better encapsulation */
    friend class FruityPerfInput;       /* we need better encapsulation */

public:

    /**
     *  Provides a typedef introduced by Stazed to make the trigger grow/move
     *  code easier to understand.
     */

    enum grow_edit_t
    {
        GROW_START  = 0,    /**< Grow the start of the trigger.         */
        GROW_END    = 1,    /**< Grow the end of the trigger.           */
        GROW_MOVE   = 2     /**< Move the entire trigger block.         */
    };

private:

    /**
     *  Exposes the triggers type, currently needed for midi_container only.
     */

    typedef std::list<trigger> List;

    /**
     *  Provides a stack for use with the undo/redo features of the
     *  trigger support.
     */

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
     *  Holds a count of the selected triggers, for better control over
     *  selections.
     */

    int m_number_selected;

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
     *  The tick point for pasting.  Set to -1 if not in force.  This is a new
     *  feature from stazed's Seq32 project.
     */

    midipulse m_paste_tick;

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
     *      This is the const version
     */

    const List & triggerlist () const
    {
        return m_triggers;
    }

    /**
     * \getter m_triggers
     */

    List & triggerlist ()
    {
        return m_triggers;
    }

    /**
     * \getter m_triggers.size()
     */

    int count () const
    {
        return int(m_triggers.size());
    }

    /**
     * \getter m_number_selected
     */

    int number_selected () const
    {
        return m_number_selected;
    }

    void push_undo ();
    void pop_undo ();
    void pop_redo ();
    void print (const std::string & seqname) const;
#ifdef USE_SONG_RECORDING
    bool play (midipulse & starttick, midipulse & endtick, bool resume = false );
#else
    bool play (midipulse & starttick, midipulse & endtick);
#endif
    void add
    (
        midipulse tick, midipulse len,
        midipulse offset = 0, bool adjustoffset = true
    );
    void adjust_offsets_to_length (midipulse newlen);
    void split (midipulse tick);

#ifdef SEQ64_SONG_BOX_SELECT
    void half_split (midipulse tick);
    void exact_split (midipulse tick);
#endif

    void grow (midipulse tickfrom, midipulse tickto, midipulse length);
    void remove (midipulse tick);
    bool get_state (midipulse tick) const;
    bool select (midipulse tick);
    bool unselect (midipulse tick);
    bool unselect ();
    bool intersect (midipulse position, midipulse & start, midipulse & end);
    bool intersect (midipulse position);

    void remove_selected ();
    void copy_selected ();
    void paste (midipulse paste_tick = SEQ64_NO_PASTE_TRIGGER);
    bool move_selected
    (
        midipulse tick, bool adjustoffset, grow_edit_t which = GROW_MOVE
    );

    midipulse get_selected_start ();
    midipulse get_selected_end ();
    midipulse get_maximum ();
    void move (midipulse starttick, midipulse distance, bool direction);
    void copy (midipulse starttick, midipulse distance);

    /**
     *  Clears the whole list of triggers, and zeroes the number selected.
     */

    void clear ()
    {
        m_triggers.clear();
        m_number_selected = 0;
    }

    bool next
    (
        midipulse & tick_on, midipulse & tick_off,
        bool & selected, midipulse & tick_offset
    );
    trigger next_trigger ();

    /**
     *  Sets the draw-trigger iterator to the beginning of the trigger list.
     */

    void reset_draw_trigger_marker ()
    {
        m_iterator_draw_trigger = m_triggers.begin();
    }

    void set_trigger_paste_tick (midipulse tick)
    {
        m_paste_tick = tick;
    }

    midipulse get_trigger_paste_tick () const
    {
        return m_paste_tick;
    }

private:

#ifdef SEQ64_SONG_BOX_SELECT
    void offset_selected_by (midipulse tick, grow_edit_t editmode);
#endif

    midipulse adjust_offset (midipulse offset);
    void split (trigger & t, midipulse splittick);
    void select (trigger & t);
    void unselect (trigger & t);

};          // class triggers

}           // namespace seq64

#endif      // SEQ64_TRIGGERS_HPP

/*
 * triggers.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

