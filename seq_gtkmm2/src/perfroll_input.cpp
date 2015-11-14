/*
 *  This file is part of seq24/sequencer64.
 *
 *  seq24 is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General)mm Public License as published by
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
 * \file          perfroll_input.cpp
 *
 *  This module declares/defines the base class for the Performance window
 *  mouse input.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2015-11-14
 * \license       GNU GPLv2 or above
 *
 */

#include <gtkmm/button.h>
#include <gdkmm/cursor.h>

#include "click.hpp"                    /* SEQ64_CLICK_LEFT(), etc.    */
#include "perform.hpp"
#include "perfroll_input.hpp"
#include "perfroll.hpp"
#include "sequence.hpp"

/**
 *  Duplicates what is at the top of the perfroll.cpp module.  FIX LATER.
 */

static int s_perfroll_size_box_click_w = 4; /* s_perfroll_size_box_w + 1 */

namespace seq64
{

/**
 *  A popup menu (which one?) calls this.  What does it mean?
 */

void
Seq24PerfInput::set_adding (bool adding, perfroll & roll)
{
    if (adding)
    {
        roll.get_window()->set_cursor(Gdk::Cursor(Gdk::PENCIL));
        m_adding = true;
    }
    else
    {
        roll.get_window()->set_cursor(Gdk::Cursor(Gdk::LEFT_PTR));
        m_adding = false;
    }
}

/**
 *  Handles the normal variety of button-press event.
 *
 *  Is there any easy way to use ctrl-left-click as the middle button
 *  here?
 */

bool
Seq24PerfInput::on_button_press_event (GdkEventButton * ev, perfroll & roll)
{
    perform & p = roll.perf();
    int & dropseq = roll.m_drop_sequence;
    roll.grab_focus();
    if (p.is_active(dropseq))
    {
        p.get_sequence(dropseq)->unselect_triggers();
        roll.draw_all();
    }
    roll.m_drop_x = int(ev->x);
    roll.m_drop_y = int(ev->y);
    roll.convert_xy                                 /* side-effects */
    (
        roll.m_drop_x, roll.m_drop_y, roll.m_drop_tick, dropseq
    );

    if (SEQ64_CLICK_LEFT(ev->button))
    {
        long tick = roll.m_drop_tick;
        if (m_adding)         /* add a new note if we didn't select anything */
        {
            m_adding_pressed = true;
            if (p.is_active(dropseq))
            {
                long seqlength = p.get_sequence(dropseq)->get_length();
                bool state = p.get_sequence(dropseq)->get_trigger_state(tick);
                if (state)
                {
                    p.push_trigger_undo();
                    p.get_sequence(dropseq)->del_trigger(tick);
                }
                else
                {
                    tick -= (tick % seqlength);    // snap to sequence length
                    p.push_trigger_undo();
                    p.get_sequence(dropseq)->add_trigger(tick, seqlength);
                    roll.draw_all();
                }
            }
        }
        else
        {
            if (p.is_active(dropseq))
            {
                p.push_trigger_undo();
                p.get_sequence(dropseq)->select_trigger(tick);

                long tick0 = p.get_sequence(dropseq)->selected_trigger_start();
                long tick1 = p.get_sequence(dropseq)->selected_trigger_end();
                int wscalex = s_perfroll_size_box_click_w * c_perf_scale_x;
                int ydrop = roll.m_drop_y % c_names_y;
                if
                (
                    tick >= tick0 && tick <= (tick0 + wscalex) &&
                    ydrop <= s_perfroll_size_box_click_w + 1
                )
                {
                    roll.m_growing = true;
                    roll.m_grow_direction = true;
                    roll.m_drop_tick_trigger_offset = roll.m_drop_tick -
                         p.get_sequence(dropseq)->selected_trigger_start();
                }
                else if
                (
                    tick >= (tick1 - wscalex) && tick <= tick1 &&
                    ydrop >= c_names_y - s_perfroll_size_box_click_w - 1
                )
                {
                    roll.m_growing = true;
                    roll.m_grow_direction = false;
                    roll.m_drop_tick_trigger_offset = roll.m_drop_tick -
                        p.get_sequence(dropseq)->selected_trigger_end();
                }
                else
                {
                    roll.m_moving = true;
                    roll.m_drop_tick_trigger_offset = roll.m_drop_tick -
                         p.get_sequence(dropseq)->selected_trigger_start();
                }
                roll.draw_all();
            }
        }
    }
    else if (SEQ64_CLICK_RIGHT(ev->button))
    {
        set_adding(true, roll);
    }
    else if (SEQ64_CLICK_MIDDLE(ev->button))                   /* split    */
    {
        if (p.is_active(dropseq))
        {
            bool state = p.get_sequence(dropseq)->
                get_trigger_state(roll.m_drop_tick);

            if (state)
                roll.split_trigger(dropseq, roll.m_drop_tick);
        }
    }
    return true;
}

/**
 *  Handles various button-release events.
 *
 *  Any use for the middle-button or ctrl-left-click we can add?
 */

bool
Seq24PerfInput::on_button_release_event (GdkEventButton * ev, perfroll & roll)
{
    if (SEQ64_CLICK_LEFT(ev->button))
    {
        if (m_adding)
            m_adding_pressed = false;
    }
    else if (SEQ64_CLICK_RIGHT(ev->button))
    {
        /*
         * Minor new feature.  If the Super (Mod4, Windows) key is
         * pressed when release, keep the adding-state in force.  One
         * can then use the unadorned left-click key to add material.  Right
         * click to reset the adding mode.  This feature is enabled only
         * if allowed by Options / Mouse  (but is true by default).
         * See the same code in seq24seqroll.cpp.
         */

        bool addmode_exit = ! rc().allow_mod4_mode();
        if (! addmode_exit)
            addmode_exit = ! (ev->state & SEQ64_MOD4_MASK); /* Mod4 held? */

        if (addmode_exit)
        {
            m_adding_pressed = false;
            set_adding(false, roll);
        }
    }

    perform & p = roll.perf();
    roll.m_moving = roll.m_growing = m_adding_pressed = false;
    m_effective_tick = 0;
    if (p.is_active(roll.m_drop_sequence))
        roll.draw_all();

    return true;
}

/**
 *  Handles the normal motion-notify event.
 */

bool
Seq24PerfInput::on_motion_notify_event (GdkEventMotion * ev, perfroll & roll)
{
    int x = int(ev->x);
    perform & p = roll.perf();
    int dropseq = roll.m_drop_sequence;
    if (m_adding && m_adding_pressed)
    {
        long tick;
        roll.convert_x(x, tick);
        if (p.is_active(dropseq))
        {
            long seqlength = p.get_sequence(dropseq)->get_length();
            tick -= (tick % seqlength);

            long length = seqlength;
            p.get_sequence(dropseq)->grow_trigger(roll.m_drop_tick, tick, length);
            roll.draw_all();
        }
    }
    else if (roll.m_moving || roll.m_growing)
    {
        if (p.is_active(dropseq))
        {
            long tick;
            roll.convert_x(x, tick);
            tick -= roll.m_drop_tick_trigger_offset;
            tick -= tick % roll.m_snap;
            if (roll.m_moving)
            {
                p.get_sequence(dropseq)->move_selected_triggers_to(tick, true);
            }
            if (roll.m_growing)
            {
                if (roll.m_grow_direction)
                    p.get_sequence(dropseq)->
                        move_selected_triggers_to(tick, false, 0);
                else
                    p.get_sequence(dropseq)->
                        move_selected_triggers_to(tick - 1, false, 1);
            }
            roll.draw_all();
        }
    }
    return true;
}

/**
 *  Handles the keystroke motion-notify event for moving a pattern back and
 *  forth in the performance.
 *
 *  What happens when the mouse is used to drag the pattern is that, first,
 *  roll.m_drop_tick is set by left-clicking into the pattern to select it.
 *  As the pattern is dragged, the drop-tick value does not change, but the
 *  tick (converted from the moving x value) does.
 *
 *  Then the button-handler sets roll.m_moving = true, and calculates
 *  roll.m_drop_tick_trigger_offset = roll.m_drop_tick -
 *  p.get_sequence(dropseq)->selected_trigger_start();
 *
 *  The motion handler sees that roll.m_moving is true, gets the new tick
 *  value from the new x value, offsets it, and calls
 *  p.get_sequence(dropseq)->move_selected_triggers_to(tick, true).
 *
 *  When the user releases the left button, then roll.m_growing is turned of
 *  and the roll draw_all()'s.
 *
 * \param is_left
 *      False denotes the right arrow key, and true denotes the left arrow
 *      key.
 *
 * \param roll
 *      Provides a reference to the parent roll, which keeps track of most of
 *      the information about the status of the window.
 *
 * \return
 *      Returns true if there was some action able to happen that would
 *      necessitate a window update.  We've updated triggers::move_selected()
 *      [called indirectly near the end of this routine] to return false if no
 *      more movement could be made.  This prevents this routine from moving
 *      way ahead after movement of the selected (in the user-interface)
 *      trigger stops.
 */

bool
Seq24PerfInput::handle_motion_key (bool is_left, perfroll & roll)
{
    bool result = roll.m_drop_sequence > 0;
    if (result)
    {
        perform & p = roll.perf();
        int dropseq = roll.m_drop_sequence;
        long droptick = roll.m_drop_tick;
        if (m_effective_tick == 0)
            m_effective_tick = droptick;

        if (is_left)
        {
            /*
             * This needlessly prevents a selection from moving leftward
             * from its original position.  We'll just put up with the
             * annoyance of absorbed decrements and document it in
             * sequencer64-doc.
             *
             * if (m_effective_tick < droptick)
             *     m_effective_tick = droptick;
             */

            m_effective_tick -= roll.m_snap;
            if (m_effective_tick <= 0)
                m_effective_tick += roll.m_snap;    /* retrench */
        }
        else
        {
            m_effective_tick += roll.m_snap;

            /*
             * What is the upper boundary here?
             */
        }

        long tick = m_effective_tick - roll.m_drop_tick_trigger_offset;
        tick -= tick % roll.m_snap;
        result = p.get_sequence(dropseq)->move_selected_triggers_to(tick, true);
    }
    return result;
}

}           // namespace seq64

/*
 * perfroll_input.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

