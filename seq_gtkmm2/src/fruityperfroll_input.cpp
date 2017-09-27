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
 *  This module declares/defines the base class for the "fruity" Performance
 *  window mouse input, FruityPerfInput.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-10-13
 * \updates       2017-09-20
 * \license       GNU GPLv2 or above
 *
 *  Now derived directly Seq24PerfInput.  No more AbstractPerfInput and no
 *  more passing a perfroll parameter around.
 */

#include <gtkmm/button.h>
#include <gdkmm/cursor.h>

#include "click.hpp"                    /* SEQ64_CLICK_LEFT(), etc.     */
#include "gui_key_tests.hpp"            /* seq64::is_no_modifier() etc. */
#include "fruityperfroll_input.hpp"     /* seq64::FruityPerfInput       */
#include "perform.hpp"                  /* seq64::perform class         */
#include "sequence.hpp"                 /* seq64::sequence class        */

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *  Principal constructor.
 *
 * \param p
 *      The perform object that this class will affect via user interaction.
 *
 * \param parent
 *      The perfedit object that holds this user-interface class.
 *
 * \param hadjust
 *      A horizontal adjustment object, passed along to the perfroll class.
 *
 * \param vadjust
 *      A vertical adjustment object, passed along to the perfroll class.
 *
 * \param ppqn
 *      The "resolution" of the MIDI file, used in zooming and scaling.
 */

FruityPerfInput::FruityPerfInput
(
    perform & p,
    perfedit & parent,
    Gtk::Adjustment & hadjust,
    Gtk::Adjustment & vadjust,
    int ppqn
) :
    Seq24PerfInput      (p, parent, hadjust, vadjust, ppqn)
{
    // Empty body
}

/**
 *  Updates the mouse pointer, implementing a context-sensitive mouse.
 *  Note that perform::convert_xy() returns its values via side-effects on the
 *  last two parameters.
 *
 * \param roll
 *      The song editor piano roll that is the "parent" of this class.
 */

void
FruityPerfInput::update_mouse_pointer ()
{
    perform & p = perf();
    midipulse droptick;
    int dropseq;
    convert_xy(m_current_x, m_current_y, droptick, dropseq);
    sequence * seq = p.get_sequence(dropseq);
    if (p.is_active(dropseq))
    {
        midipulse start;
        midipulse end;
        if (seq->intersect_triggers(droptick, start, end))
        {
            int ymod = m_current_y % c_names_y;
            if
            (
                start <= droptick && droptick <= (start + m_w_scale_x) &&
                (ymod <= sm_perfroll_size_box_click_w + 1)
            )
            {
                get_window()->set_cursor(Gdk::Cursor(Gdk::RIGHT_PTR));
            }
            else if
            (
                droptick <= end && (end - m_w_scale_x) <= droptick &&
                ymod >= (c_names_y - sm_perfroll_size_box_click_w - 1)
            )
            {
                get_window()->set_cursor(Gdk::Cursor(Gdk::LEFT_PTR));
            }
            else
                get_window()->set_cursor(Gdk::Cursor(Gdk::CENTER_PTR));
        }
        else
            get_window()->set_cursor(Gdk::Cursor(Gdk::PENCIL));
    }
    else
        get_window()->set_cursor(Gdk::Cursor(Gdk::CROSSHAIR));
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
FruityPerfInput::on_button_press_event (GdkEventButton * ev)
{
    bool result = false;
    perform & p = perf();
    grab_focus();
    int & dropseq = m_drop_sequence;       /* reference needed         */
    sequence * seq = p.get_sequence(dropseq);
    if (p.is_active(dropseq))
    {
        seq->unselect_triggers();
        draw_all();
    }

    m_drop_x = int(ev->x);
    m_drop_y = int(ev->y);
    m_current_x = int(ev->x);
    m_current_y = int(ev->y);
    convert_xy                                  /* side-effects             */
    (
        m_drop_x, m_drop_y, m_drop_tick, dropseq
    );
    if (SEQ64_CLICK_LEFT(ev->button))
    {
        result = on_left_button_pressed(ev);
    }
    else if (SEQ64_CLICK_RIGHT(ev->button))
    {
        result = on_right_button_pressed(ev);
    }
    else if (SEQ64_CLICK_MIDDLE(ev->button))    /* left-ctrl or middle      */
    {
        if (p.is_active(dropseq))               /* redundant check?         */
        {
            midipulse droptick = m_drop_tick;
            droptick -= droptick % m_snap;      /* stazed fix: grid snap    */
            bool state = seq->get_trigger_state(droptick);
            if (state)                          /* trigger click, split it  */
            {
                split_trigger(dropseq, droptick);
            }
            else                                /* track click, paste trig  */
            {
                p.push_trigger_undo(dropseq);   /* stazed fix               */
                seq->paste_trigger(droptick);
            }
            result = true;                      /* it did do something      */
        }
    }
    update_mouse_pointer();
    (void) Seq24PerfInput::on_button_press_event(ev);
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
FruityPerfInput::on_left_button_pressed (GdkEventButton * ev)
{
    bool result = false;
    perform & p = perf();
    int dropseq = m_drop_sequence;
    sequence * seq = p.get_sequence(dropseq);
    if (is_ctrl_key(ev))
    {
        if (p.is_active(dropseq))
        {
            midipulse droptick = m_drop_tick;
            droptick -= droptick % m_snap;         /* stazed: grid snap    */
            bool state = seq->get_trigger_state(droptick);
            if (state)
            {
                split_trigger(dropseq, droptick);
                result = true;
            }
            else                                        /* track, paste trig    */
            {
                p.push_trigger_undo(dropseq);           /* stazed code          */
                seq->paste_trigger(droptick);
            }
        }
    }
    else                    /* add a new note if we didn't select anything  */
    {
        midipulse droptick = m_drop_tick;

        /*
         * \change ca 2017-08-13
         *      Check status *before* turning this mode on!
         *
         * set_adding_pressed(true);
         *
         *      The code below seems somewhat different from the seq24
         *      original, but somwwhat seems to work.
         */

        if (p.is_active(dropseq))
        {
            midipulse seqlength = seq->get_length();
            bool state = seq->get_trigger_state(droptick);
            set_adding_pressed(true);
            if (state)      /* resize or move event based on where clicked  */
            {
                set_adding_pressed(false);

                /*
                 * Set the flag that tells the motion-notify callback to call
                 * push_trigger_undo().
                 */

                m_have_button_press = seq->select_trigger(droptick);

                midipulse starttick = seq->selected_trigger_start();
                midipulse endtick = seq->selected_trigger_end();
                int ydrop = m_drop_y % c_names_y;
                if
                (
                    droptick >= starttick && droptick <= (starttick +
                    m_w_scale_x) &&
                    ydrop <= (sm_perfroll_size_box_click_w + 1)
                )
                {           /* clicked left side: grow/shrink left side     */
                    m_growing = true;
                    m_grow_direction = true;
                    m_drop_tick_offset = m_drop_tick -
                        seq->selected_trigger_start();
                }
                else if
                (
                    droptick >= (endtick - m_w_scale_x) && droptick <= endtick &&
                    ydrop >= (c_names_y - sm_perfroll_size_box_click_w - 1)
                )
                {           /* clicked right side: grow/shrink right side   */
                    m_growing = true;
                    m_grow_direction = false;
                    m_drop_tick_offset = m_drop_tick -
                        seq->selected_trigger_end();
                }
                else        /* clicked in the middle - move it              */
                {
                    m_moving = true;
                    m_drop_tick_offset = m_drop_tick -
                         seq->selected_trigger_start();
                }
            }
            else                                    /* add a trigger        */
            {
                droptick -= (droptick % seqlength); /* snap to seqlength    */
                p.push_trigger_undo();
                seq->add_trigger(droptick, seqlength);
                result = true;
            }
            draw_all();
        }
    }
    return result;
}

/**
 *  Handles the right button of the mouse.  I don't get it, though... all
 *  three buttons are handled in the generic button-press callback.  Oh, this
 *  is a helper function.
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
FruityPerfInput::on_right_button_pressed (GdkEventButton * ev)
{
    bool result = false;
    perform & p = perf();
    midipulse droptick = m_drop_tick;
    int dropseq = m_drop_sequence;
    if (p.is_active(dropseq))
    {
        sequence * seq = p.get_sequence(dropseq);
        bool state = seq->get_trigger_state(droptick);
        if (state)
        {
            p.push_trigger_undo(dropseq);       /* stazed fix */
            seq->delete_trigger(droptick);
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
FruityPerfInput::on_button_release_event (GdkEventButton * ev)
{
    bool result = false;
    m_current_x = int(ev->x);
    m_current_y = int(ev->y);

    perform & p = perf();
    m_moving = false;
    m_growing = false;
    set_adding_pressed(false);
    if (p.is_active(m_drop_sequence))
        draw_all();

    update_mouse_pointer();
    (void) Seq24PerfInput::on_button_release_event(ev);
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
FruityPerfInput::on_motion_notify_event (GdkEventMotion * ev)
{
    bool result = false;
    perform & p = perf();
    int dropseq = m_drop_sequence;
    sequence * seq = p.get_sequence(dropseq);
    int x = int(ev->x);
    midipulse tick = 0;
    m_current_x = int(ev->x);
    m_current_y = int(ev->y);
    if (is_adding_pressed())
    {
        convert_x(x, tick);        /* side-effect */
        if (p.is_active(dropseq))
        {
            midipulse seqlength = seq->get_length();
            tick -= (tick % seqlength);

            midipulse length = seqlength;
            seq->grow_trigger(m_drop_tick, tick, length);
            draw_all();
            result = true;
        }
    }
    else if (m_moving || m_growing)
    {
        if (p.is_active(dropseq))
        {
            /*
             * This code is necessary to insure that there is no push unless
             * we have a motion notification.
             */

            if (m_have_button_press)
            {
                p.push_trigger_undo(dropseq);
                m_have_button_press = false;
            }
            convert_x(x, tick);                    /* side-effect  */
            tick -= m_drop_tick_offset;
            tick -= tick % m_snap;
            if (m_moving)
            {
                seq->move_selected_triggers_to(tick, true);
                result = true;
            }
            if (m_growing)
            {
                result = true;
                if (m_grow_direction)
                {
                    seq->move_selected_triggers_to
                    (
                        tick, false, triggers::GROW_START
                    );
                }
                else
                {
                    seq->move_selected_triggers_to
                    (
                        tick - 1, false, triggers::GROW_END
                    );
                }
            }
            draw_all();
        }
    }
    update_mouse_pointer();
    (void) Seq24PerfInput::on_motion_notify_event(ev);
    return result;
}

}           // namespace seq64

/*
 * fruityperfroll_input.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

