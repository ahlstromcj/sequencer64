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
 * \updates       2016-07-16
 * \license       GNU GPLv2 or above
 *
 */

#include <gtkmm/button.h>
#include <gdkmm/cursor.h>

#include "click.hpp"                    /* SEQ64_CLICK_LEFT(), etc.    */
#include "gui_key_tests.hpp"            /* seq64::is_no_modifier() etc. */
#include "fruityperfroll_input.hpp"
#include "perform.hpp"
#include "perfroll.hpp"
#include "sequence.hpp"

/**
 *  Duplicates what is at the top of the perfroll.cpp module.  FIX LATER.
 */

static int s_perfroll_size_box_click_w = 4; /* s_perfroll_size_box_w + 1 */

namespace seq64
{

/**
 *  Updates the mouse pointer, implementing a context-sensitive mouse.
 *  Note that perform::convert_xy() returns its values via side-effects on the
 *  last two parameters.
 *
 * \param roll
 *      The song editor piano roll that is the "parent" of this class.
 */

void
FruityPerfInput::update_mouse_pointer (perfroll & roll)
{
    perform & p = roll.perf();
    midipulse droptick;
    int dropseq;
    roll.convert_xy(m_current_x, m_current_y, droptick, dropseq);
    if (p.is_active(dropseq))
    {
        midipulse start;
        midipulse end;
        if (p.get_sequence(dropseq)->intersect_triggers(droptick, start, end))
        {
            int wscalex = s_perfroll_size_box_click_w * c_perf_scale_x;
            int ymod = m_current_y % c_names_y;
            if
            (
                start <= droptick && droptick <= (start + wscalex) &&
                (ymod <= s_perfroll_size_box_click_w + 1)
            )
            {
                roll.get_window()->set_cursor(Gdk::Cursor(Gdk::RIGHT_PTR));
            }
            else if
            (
                droptick <= end && (end - wscalex) <= droptick &&
                ymod >= (c_names_y - s_perfroll_size_box_click_w - 1)
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
 *
 * \param ev
 *      The button-press event to process.
 *
 * \param roll
 *      The song editor piano roll that is the "parent" of this class.
 *
 * \return
 *      Returns true if a modification occurred.
 */

bool
FruityPerfInput::on_button_press_event (GdkEventButton * ev, perfroll & roll)
{
    bool result = false;
    perform & p = roll.perf();
    roll.grab_focus();
    int & dropseq = roll.m_drop_sequence;       /* reference needed         */
    if (p.is_active(dropseq))
    {
        p.get_sequence(dropseq)->unselect_triggers();
        roll.draw_all();
    }
    roll.m_drop_x = int(ev->x);
    roll.m_drop_y = int(ev->y);
    m_current_x = int(ev->x);
    m_current_y = int(ev->y);
    roll.convert_xy                             /* side-effects             */
    (
        roll.m_drop_x, roll.m_drop_y, roll.m_drop_tick, dropseq
    );
    if (SEQ64_CLICK_LEFT(ev->button))
    {
        result = on_left_button_pressed(ev, roll);
    }
    else if (SEQ64_CLICK_RIGHT(ev->button))
    {
        result = on_right_button_pressed(ev, roll);
    }
    else if (SEQ64_CLICK_MIDDLE(ev->button))   /* left-ctrl???, middle    */
    {
        if (p.is_active(dropseq))
        {
            midipulse droptick = roll.m_drop_tick;
            bool state = p.get_sequence(dropseq)->get_trigger_state(droptick);
            if (state)
            {
                roll.split_trigger(dropseq, droptick);
                result = true;
            }
        }
    }
    update_mouse_pointer(roll);
    return result;
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
 *  button-press callback.  Oh, this is just a helper function.
 *
 * \param ev
 *      The left-button-press event to process.
 *
 * \param roll
 *      The song editor piano roll that is the "parent" of this class.
 *
 * \return
 *      Now returns true if a modification occurred.
 */

bool
FruityPerfInput::on_left_button_pressed (GdkEventButton * ev, perfroll & roll)
{
    bool result = false;
    perform & p = roll.perf();
    int dropseq = roll.m_drop_sequence;
    if (is_ctrl_key(ev))
    {
        if (p.is_active(dropseq))
        {
            bool state = p.get_sequence(dropseq)->
                get_trigger_state(roll.m_drop_tick);

            if (state)
            {
                roll.split_trigger(dropseq, roll.m_drop_tick);
                result = true;
            }
        }
    }
    else                /* add a new note */
    {
        midipulse tick = roll.m_drop_tick;
        m_adding_pressed = true;
        if (p.is_active(dropseq))
        {
            sequence * seq = p.get_sequence(dropseq);
            midipulse seqlength = seq->get_length();
            bool state = seq->get_trigger_state(tick);
            if (state)  /* resize event or move it based on where clicked */
            {
                m_adding_pressed = false;
                p.push_trigger_undo();
                seq->select_trigger(tick);
                midipulse starttick = seq->selected_trigger_start();
                midipulse endtick = seq->selected_trigger_end();
                int wscalex = s_perfroll_size_box_click_w * c_perf_scale_x;
                int ydrop = roll.m_drop_y % c_names_y;
                if
                (
                    tick >= starttick && tick <= (starttick + wscalex) &&
                    ydrop <= (s_perfroll_size_box_click_w + 1)
                )
                {           /* clicked left side: grow/shrink left side     */
                    roll.m_growing = true;
                    roll.m_grow_direction = true;
                    roll.m_drop_tick_trigger_offset = roll.m_drop_tick -
                        seq->selected_trigger_start();
                }
                else if
                (
                    tick >= (endtick - wscalex) && tick <= endtick &&
                    ydrop >= (c_names_y - s_perfroll_size_box_click_w - 1)
                )
                {           /* clicked right side: grow/shrink right side   */
                    roll.m_growing = true;
                    roll.m_grow_direction = false;
                    roll.m_drop_tick_trigger_offset = roll.m_drop_tick -
                        seq->selected_trigger_end();
                }
                else        /* clicked in the middle - move it              */
                {
                    roll.m_moving = true;
                    roll.m_drop_tick_trigger_offset = roll.m_drop_tick -
                         seq->selected_trigger_start();
                }
                roll.draw_all();
            }
            else                                    /* add a trigger        */
            {
                tick -= (tick % seqlength);         /* snap to seqlength    */
                p.push_trigger_undo();
                seq->add_trigger(tick, seqlength);
                result = true;
                roll.draw_all();
            }
        }
    }
    return result;
}

/**
 *  Handles the right button of the mouse.
 *
 *  I don't get it, though... all three buttons are handled in the generic
 *  button-press callback.  Oh, this is a helper function.
 *
 * \param ev
 *      The right-button-press event to process.
 *
 * \param roll
 *      The song editor piano roll that is the "parent" of this class.
 *
 * \return
 *      Returns true if a modification occurred.
 */

bool
FruityPerfInput::on_right_button_pressed (GdkEventButton * ev, perfroll & roll)
{
    bool result = false;
    perform & p = roll.perf();
    midipulse tick = roll.m_drop_tick;
    int dropseq = roll.m_drop_sequence;
    if (p.is_active(dropseq))
    {
        sequence * seq = p.get_sequence(dropseq);
        bool state = seq->get_trigger_state(tick);
        if (state)
        {
            p.push_trigger_undo();
            seq->del_trigger(tick);
            result = true;
        }
    }
    return result;
}

/**
 *  Handles a button-release event.  Why is m_adding_pressed modified
 *  conditionally when the same modification is then made unconditionally?
 *
 * \param ev
 *      The button-release event to process.
 *
 * \param roll
 *      The song editor piano roll that is the "parent" of this class.
 *
 * \return
 *      Returns true if a modification occurred.
 */

bool
FruityPerfInput::on_button_release_event (GdkEventButton * ev, perfroll & roll)
{
    bool result = false;
    m_current_x = int(ev->x);
    m_current_y = int(ev->y);

    /*
     * if (SEQ64_CLICK_LEFT(ev->button) ||
     *      SEQ64_CLICK_RIGHT(ev->button))
     *  m_adding_pressed = false;                   // done here...
     */

    perform & p = roll.perf();
    roll.m_moving = false;
    roll.m_growing = false;
    m_adding_pressed = false;                       // and here...???
    if (p.is_active(roll.m_drop_sequence))
    {
        roll.draw_all();
        // result = true;
    }
    update_mouse_pointer(roll);
    return result;
}

/**
 *  Handles a Fruity motion-notify event.
 *
 * \param ev
 *      The motion-notify event to process.
 *
 * \param roll
 *      The song editor piano roll that is the "parent" of this class.
 *
 * \return
 *      Returns true if a modification occurred, and sets the perform modified
 *      flag based on that result.
 */

bool
FruityPerfInput::on_motion_notify_event (GdkEventMotion * ev, perfroll & roll)
{
    bool result = false;
    perform & p = roll.perf();
    int dropseq = roll.m_drop_sequence;
    int x = int(ev->x);
    m_current_x = int(ev->x);
    m_current_y = int(ev->y);
    if (m_adding_pressed)
    {
        midipulse tick;
        roll.convert_x(x, tick);
        if (p.is_active(dropseq))
        {
            sequence * seq = p.get_sequence(dropseq);
            midipulse seqlength = seq->get_length();
            tick -= (tick % seqlength);

            midipulse length = seqlength;
            seq->grow_trigger(roll.m_drop_tick, tick, length);
            roll.draw_all();
            result = true;
        }
    }
    else if (roll.m_moving || roll.m_growing)
    {
        if (p.is_active(dropseq))
        {
            sequence * seq = p.get_sequence(dropseq);
            midipulse tick;
            roll.convert_x(x, tick);
            tick -= roll.m_drop_tick_trigger_offset;
            tick -= tick % roll.m_snap;
            if (roll.m_moving)
            {
                seq->move_selected_triggers_to(tick, true);
                result = true;
            }
            if (roll.m_growing)
            {
                result = true;
                if (roll.m_grow_direction)
                    seq->move_selected_triggers_to(tick, false, 0);
                else
                    seq->move_selected_triggers_to(tick - 1, false, 1);
            }
            roll.draw_all();
        }
    }
    update_mouse_pointer(roll);
    return result;
}

}           // namespace seq64

/*
 * fruityperfroll_input.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

