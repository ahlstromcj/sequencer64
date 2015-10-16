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
 * \file          fruityperfroll_input.cpp
 *
 *  This module declares/defines the base class for the Performance window
 *  mouse input.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-10-13
 * \updates       2015-10-13
 * \license       GNU GPLv2 or above
 *
 */

#include <gtkmm/button.h>
#include <gdkmm/cursor.h>

#include "click.hpp"                    /* SEQ64_CLICK_IS_LEFT(), etc.    */
#include "perform.hpp"
#include "fruityperfroll_input.hpp"
#include "perfroll.hpp"
#include "sequence.hpp"

namespace seq64
{

/**
 *  Updates the mouse pointer, implementing a context-sensitive mouse.
 *  Note that perform::convert_xy() returns its values via side-effects on the
 *  last two parameters.
 */

void
FruityPerfInput::updateMousePtr (perfroll & roll)
{
    perform & p = roll.perf();
    long droptick;
    int dropseq;
    roll.convert_xy(m_current_x, m_current_y, droptick, dropseq);
    if (p.is_active(dropseq))
    {
        long start, end;
        if (p.get_sequence(dropseq)->intersectTriggers(droptick, start, end))
        {
            int wscalex = c_perfroll_size_box_click_w * c_perf_scale_x;
            int ymod = m_current_y % c_names_y;
            if
            (
                start <= droptick && droptick <= (start + wscalex) &&
                (ymod <= c_perfroll_size_box_click_w + 1)
            )
            {
                roll.get_window()->set_cursor(Gdk::Cursor(Gdk::RIGHT_PTR));
            }
            else if
            (
                droptick <= end && (end - wscalex) <= droptick &&
                ymod >= (c_names_y - c_perfroll_size_box_click_w - 1)
            )
            {
                roll.get_window()->set_cursor(Gdk::Cursor(Gdk::LEFT_PTR));
            }
            else
                roll.get_window()->set_cursor(Gdk::Cursor(Gdk::CENTER_PTR));
        }
        else
            roll.get_window()->set_cursor(Gdk::Cursor(Gdk::PENCIL));
    }
    else
        roll.get_window()->set_cursor(Gdk::Cursor(Gdk::CROSSHAIR));
}

/**
 *  Handles a button-press event in the Fruity manner.
 */

bool
FruityPerfInput::on_button_press_event (GdkEventButton * a_ev, perfroll & roll)
{
    perform & p = roll.perf();
    roll.grab_focus();
    int & dropseq = roll.m_drop_sequence;       /* reference needed         */
    if (p.is_active(dropseq))
    {
        p.get_sequence(dropseq)->unselect_triggers();
        roll.draw_all();
    }
    roll.m_drop_x = int(a_ev->x);
    roll.m_drop_y = int(a_ev->y);
    m_current_x = int(a_ev->x);
    m_current_y = int(a_ev->y);
    roll.convert_xy                             /* side-effects             */
    (
        roll.m_drop_x, roll.m_drop_y, roll.m_drop_tick, dropseq
    );
    if (SEQ64_CLICK_IS_LEFT(a_ev->button))
    {
        on_left_button_pressed(a_ev, roll);
    }
    else if (SEQ64_CLICK_IS_RIGHT(a_ev->button))
    {
        on_right_button_pressed(a_ev, roll);
    }
    else if (SEQ64_CLICK_IS_MIDDLE(a_ev->button))   /* left-ctrl, middle    */
    {
        if (p.is_active(dropseq))
        {
            long droptick = roll.m_drop_tick;
            bool state = p.get_sequence(dropseq)->get_trigger_state(droptick);
            if (state)
                roll.split_trigger(dropseq, droptick);
        }
    }
    updateMousePtr(roll);
    return true;
}

/**
 *  Handles the left button of the mouse.
 *
 *  It can handle splitting triggers (?), adding notes, and the following
 *  clicks to resize the event, or move it, depending on where clicked:
 *
 *      -   clicked left side: begin a grow/shrink for the left side
 *      -   clicked right side: grow/shrink the right side
 *      -   clicked in the middle - move it
 *
 *  I don't get it, though... all three buttons are handled in the generic
 *  button-press callback.
 */

void
FruityPerfInput::on_left_button_pressed (GdkEventButton * a_ev, perfroll & roll)
{
    perform & p = roll.perf();
    int dropseq = roll.m_drop_sequence;
    if (a_ev->state & GDK_CONTROL_MASK)
    {
        if (p.is_active(dropseq))
        {
            bool state = p.get_sequence(dropseq)->
                get_trigger_state(roll.m_drop_tick);

            if (state)
                roll.split_trigger(dropseq, roll.m_drop_tick);
        }
    }
    else                /* add a new note */
    {
        long tick = roll.m_drop_tick;
        m_adding_pressed = true;
        if (p.is_active(dropseq))
        {
            long seqlength = p.get_sequence(dropseq)->get_length();
            bool state = p.get_sequence(dropseq)->get_trigger_state(tick);
            if (state)  /* resize event or move it based on where clicked */
            {
                m_adding_pressed = false;
                p.push_trigger_undo();
                p.get_sequence(dropseq)->select_trigger(tick);
                long starttick = p.get_sequence(dropseq)->selected_trigger_start();
                long endtick = p.get_sequence(dropseq)->selected_trigger_end();
                int wscalex = c_perfroll_size_box_click_w * c_perf_scale_x;
                int ydrop = roll.m_drop_y % c_names_y;
                if
                (
                    tick >= starttick && tick <= (starttick + wscalex) &&
                    ydrop <= (c_perfroll_size_box_click_w + 1)
                )
                {           /* clicked left side: grow/shrink left side     */
                    roll.m_growing = true;
                    roll.m_grow_direction = true;
                    roll.m_drop_tick_trigger_offset = roll.m_drop_tick -
                        p.get_sequence(dropseq)->selected_trigger_start();
                }
                else if
                (
                    tick >= (endtick - wscalex) && tick <= endtick &&
                    ydrop >= (c_names_y - c_perfroll_size_box_click_w - 1)
                )
                {           /* clicked right side: grow/shrink right side   */
                    roll.m_growing = true;
                    roll.m_grow_direction = false;
                    roll.m_drop_tick_trigger_offset = roll.m_drop_tick -
                        p.get_sequence(dropseq)->selected_trigger_end();
                }
                else        /* clicked in the middle - move it              */
                {
                    roll.m_moving = true;
                    roll.m_drop_tick_trigger_offset = roll.m_drop_tick -
                         p.get_sequence(dropseq)->selected_trigger_start();
                }
                roll.draw_all();
            }
            else                                    /* add a trigger        */
            {
                tick -= (tick % seqlength);         /* snap to sequlength   */
                p.push_trigger_undo();
                p.get_sequence(dropseq)->add_trigger(tick, seqlength);
                roll.draw_all();
            }
        }
    }
}

/**
 *  Handles the right button of the mouse.
 *
 *  I don't get it, though... all three buttons are handled in the generic
 *  button-press callback.
 */

void
FruityPerfInput::on_right_button_pressed (GdkEventButton * a_ev, perfroll & roll)
{
    perform & p = roll.perf();
    long tick = roll.m_drop_tick;
    int dropseq = roll.m_drop_sequence;
    if (p.is_active(dropseq))
    {
        bool state = p.get_sequence(dropseq)->get_trigger_state(tick);
        if (state)
        {
            p.push_trigger_undo();
            p.get_sequence(dropseq)->del_trigger(tick);
        }
    }
}

/**
 *  Handles a button-release event.  Why is m_adding_pressed modified
 *  conditionally when the same modification is then made unconditionally?
 */

bool
FruityPerfInput::on_button_release_event (GdkEventButton * a_ev, perfroll & roll)
{
    m_current_x = (int) a_ev->x;
    m_current_y = (int) a_ev->y;

    /*
     * if (SEQ64_CLICK_IS_LEFT(a_ev->button) ||
     *      SEQ64_CLICK_IS_RIGHT(a_ev->button))
     *  m_adding_pressed = false;                   // done here...
     */

    perform & p = roll.perf();
    roll.m_moving = false;
    roll.m_growing = false;
    m_adding_pressed = false;                       // and here...???
    if (p.is_active(roll.m_drop_sequence))
        roll.draw_all();

    updateMousePtr(roll);
    return true;
}

/**
 *  Handles a Fruity motion-notify event.
 */

bool
FruityPerfInput::on_motion_notify_event (GdkEventMotion * a_ev, perfroll & roll)
{
    perform & p = roll.perf();
    int dropseq = roll.m_drop_sequence;
    int x = int(a_ev->x);
    m_current_x = int(a_ev->x);
    m_current_y = int(a_ev->y);
    if (m_adding_pressed)
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
                p.get_sequence(dropseq)->move_selected_triggers_to(tick, true);

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
    updateMousePtr(roll);
    return true;
}

}           // namespace seq64

/*
 * fruityperfroll_input.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

