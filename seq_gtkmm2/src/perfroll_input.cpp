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
 * \updates       2017-09-27
 * \license       GNU GPLv2 or above
 *
 *  Now derived directly from perfroll.  No more AbstractPerfInput and no more
 *  passing a perfroll parameter around.
 */

#include <gtkmm/button.h>
#include <gdkmm/cursor.h>

#include "click.hpp"                    /* SEQ64_CLICK_LEFT(), etc.     */
#include "gui_key_tests.hpp"            /* seq64::is_super_key()        */
#include "perform.hpp"                  /* seq64::perform class         */
#include "perfroll_input.hpp"           /* seq64::Seq24PerfInput class  */
#include "sequence.hpp"                 /* seq64::sequence class        */
#include "settings.hpp"                 /* seq64::rc() or seq64::usr()  */

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

Seq24PerfInput::Seq24PerfInput
(
    perform & p,
    perfedit & parent,
    Gtk::Adjustment & hadjust,
    Gtk::Adjustment & vadjust,
    int ppqn
) :
    perfroll            (p, parent, hadjust, vadjust, ppqn),
    m_effective_tick    (0)
{
    // Empty body
}

/**
 *  Turns on/off the mode of adding triggers to the song performance.  Changes
 *  both the flag and the mouse cursor icon.
 *
 * \param adding
 *      Indicates the adding-triggers state to be set.
 */

void
Seq24PerfInput::activate_adding (bool adding)
{
    if (adding)
        get_window()->set_cursor(Gdk::Cursor(Gdk::PENCIL));
    else
        get_window()->set_cursor(Gdk::Cursor(Gdk::LEFT_PTR));

    set_adding(adding);
}

/**
 *  Handles the normal variety of button-press event.  Is there any easy way
 *  to use ctrl-left-click as the middle button here?
 *
 *  convert_drop_xy() uses m_drop_x and m_drop_y, and sets m_drop_tick and
 *  m_drop_sequence.  It gets the sequence number of the pattern clicked.  It
 *  doesn't matter which button was clicked here.
 *
 * Stazed:
 *
 *      m_drop_y will be adjusted by perfroll::change_vert() for any scroll
 *      after it was originally selected. The call here to draw_drawable_row()
 *      [now folded into draw_all()] will have the wrong y location and
 *      un-select will not occur (or the wrong sequence will be unselected) if
 *      the user scrolls the track up or down to a new y location, if not
 *      adjusted.
 *
 * Box set selection:
 *
 *      This setup is meant to support selecting multiple sequences in the
 *      perfroll, and later to support Kepler34's box-selection of multiple
 *      sequences for movement.  Here are the rules we want to implement:
 *
 *      -   Click or Shift-Click with nothing already selected will select the
 *          current drop sequence and add it to the set.
 *      -   Click with at least one *other* sequence selected will unselect
 *          the others and select the current sequence.
 *      -   Click or Shift-Click outside a sequence will deselect them all.
 *      -   Shift-Click will leave the other selected sequences alone and add
 *          the current drop sequence.
 *
 * \return
 *      Returns true if a modification occurred.
 */

#ifdef USE_ENABLE_BOX_SET

bool
Seq24PerfInput::on_button_press_event (GdkEventButton * ev)
{
    bool result = false;
    m_drop_x = int(ev->x);
    m_drop_y = int(ev->y);
    convert_drop_xy();

    bool dropseq_active = perf().is_active(m_drop_sequence);
    if (dropseq_active)
    {
        if (is_shift_key(ev))
        {
            perf().box_toggle_sequence(m_drop_sequence);
        }
        else
        {
            perf().unselect_all_triggers();
            perf().box_insert(m_drop_sequence);
        }
    }
//  else
//      return false;

    /*
     *  Let's make better use of the Ctrl key here.  First, let Ctrl-Left be
     *  handled exactly like the Middle click (it causes the segment/trigger
     *  to be split), then bug out.  The middle click in seq24 interaction
     *  mode is either for splitting the triggers or for setting the paste
     *  location of copy/paste.  Note that the "aaa_or_bbb_trigger()"
     *  functions will check the trigger status of the sequence to decide what
     *  to do.
     */

    if (SEQ64_CLICK_LEFT(ev->button))
    {
        if (is_ctrl_key(ev))
        {
            perf().paste_or_split_trigger(m_drop_sequence, m_drop_tick);
            // ?? set_adding_pressed(false);
            result = true;
        }
        else
        {
            /*
             * Add a new trigger if nothing is selected, otherwise delete it.
             * The adding flag is set on a right-click where there is no
             * trigger under the mouse, and is unset when the right-click is
             * released [if not in allow_mod4_mode()].
             */

            if (is_adding())
            {
                perf().add_or_delete_trigger(m_drop_sequence, m_drop_tick);
                set_adding_pressed(true);
                result = true;
            }
            else
            {
                /*
                 * Set this flag to tell on_motion_notify_event() to call
                 * perf().push_trigger_undo().  This section handles motions
                 * of the held mouse that grow or shrink the selected trigger,
                 * or else the moving of the selected trigger.
                 *
                 * Now how can we affect all of the shift-selected triggers?
                 * TBD.
                 *
                 * wscalex is 4 * 32 ticks/pixel.  What about zoom?  See
                 * perfroll; it sets m_perf_scale_x and m_zoom to
                 * c_perf_scale_x.  Let's use m_perf_scale_x instead and make
                 * it a memmber, m_w_scale_x.  This section is almost common
                 * code with the fruityperfrol_input.cpp module.
                 *
                 * If the current sequence is not part of the selection, then
                 * we need to unselect all sequences.
                 */

                perf().box_deselect_sequences(m_drop_sequence);

                midipulse tick0, tick1;
                m_have_button_press = perf().selected_trigger
                (
                    m_drop_sequence, m_drop_tick, tick0, tick1  /* side-effects */
                );

                int ydrop = m_drop_y % c_names_y;

                /*
                 * Check for a corner drag (on the small box at the left of
                 * the trigger segment) to "grow" the sequence start.
                 * Otherwise, check for a corner drag (on the small box at the
                 * right of the sequence) to "grow" the sequence end.
                 * Otherwise, we are moving the sequence.
                 */

                if
                (
                    m_drop_tick >= tick0 &&
                    m_drop_tick <= (tick0 + m_w_scale_x) &&
                    ydrop <= (sm_perfroll_size_box_click_w + 1)
                )
                {
                    m_growing = true;
                    m_grow_direction = true;
                    m_drop_tick_offset = m_drop_tick - tick0;
                }
                else if
                (
                    m_drop_tick >= (tick1 - m_w_scale_x) &&
                    m_drop_tick <= tick1 &&
                    ydrop >= (c_names_y - sm_perfroll_size_box_click_w - 1)
                )
                {
                    m_growing = true;
                    m_grow_direction = false;
                    m_drop_tick_offset = m_drop_tick - tick1;
                }
                else // if (m_drop_tick >= start_tick && m_drop_tick <= end_tick)
                {
                    m_moving = true;
                    m_drop_tick_offset = m_drop_tick - tick0;
                }
                draw_all();
            }
        }

#ifdef USE_SONG_BOX_SELECT

        /*
         * Doesn't seem to do anything at this point.
         */

        if (! m_box_select)                 /* select with a box    */
        {
            perf().unselect_all_triggers();
            snap_y(m_drop_y);          /* y snapped to rows    */
            m_current_x = m_drop_x;
            m_current_y = m_drop_y;
            m_box_select = true;
        }

#endif

    }
    else if (SEQ64_CLICK_RIGHT(ev->button))
    {
        activate_adding(true);

#ifdef USE_SONG_BOX_SELECT
        perf().unselect_all_triggers();
        m_box_select = false;
#endif
    }
    else if (SEQ64_CLICK_MIDDLE(ev->button))                   /* split    */
    {
        /*
         * Same treatment as Ctrl-Left click above.  HOWEVER, the code for
         * that currently does more than is done here !!!!!
         * TODO!!! Sad! :-D
         */

        perf().paste_or_split_trigger(m_drop_sequence, m_drop_tick);
        result = true;
    }
    (void) perfroll::on_button_press_event(ev);
    return result;
}

#else               // all old stuff, no new features

bool
Seq24PerfInput::on_button_press_event (GdkEventButton * ev)
{
    bool result = false;
    sequence * seq = perf().get_sequence(m_drop_sequence);
    bool dropseq_active = not_nullptr(seq);
    grab_focus();

    /*
     * This code causes the un-greying of the previously selected trigger
     * segment.  If commented out, then we can seemingly select more than one
     * trigger segment, but only the last one "selected" can be moved.  We'd
     * like to be able to select and move a bunch at once by holding a
     * modifier key.
     */

    if (dropseq_active)
    {
        seq->unselect_triggers();
        draw_all();
    }

    /*
     * convert_drop_xy() uses m_drop_x and m_drop_y, and sets m_drop_tick and
     * m_drop_sequence.  It gets the sequence number of the pattern clicked.
     * It doesn't matter which button was clicked here.
     */

    m_drop_x = int(ev->x);
    m_drop_y = int(ev->y);

    /*
     * This call can change m_drop_sequence, so we have to make the checks
     * again.
     */

    convert_drop_xy();
    seq = perf().get_sequence(m_drop_sequence);
    dropseq_active = not_nullptr(seq);
    if (! dropseq_active)
        return false;

    /*
     *  Let's make better use of the Ctrl key here.  First, let Ctrl-Left be
     *  handled exactly like the Middle click (it causes the segment/trigger
     *  to be split), then bug out.  Note that this middle-click code ought to
     *  be folded into a function.
     */

    if (is_ctrl_key(ev))
    {
        if (SEQ64_CLICK_LEFT(ev->button))
        {
            bool state = seq->get_trigger_state(m_drop_tick);
            if (state)
            {
                perf().split_trigger(m_drop_sequence, m_drop_tick);  // perf()!
            }
            else
            {
                perf().push_trigger_undo(m_drop_sequence);
                seq->paste_trigger(m_drop_tick);
            }
        }
        return true;
    }

    if (SEQ64_CLICK_LEFT(ev->button))
    {
        /*
         * Add a new sequence if nothing is selected.  The adding flag is set
         * on a right-click where there is no trigger under the mouse, and is
         * unset when the right-click is released [if not in
         * allow_mod4_mode()].
         */

        if (is_adding())
        {
            set_adding_pressed(true);

            // MAKE SURE that seq cannot be nullptr!!

            midipulse seqlength = seq->get_length();
            bool state = seq->get_trigger_state(m_drop_tick);
            if (state)
            {
                perf().push_trigger_undo(m_drop_sequence);   /* stazed fix   */
                seq->del_trigger(m_drop_tick);
            }
            else
            {
                m_drop_tick -= (m_drop_tick % seqlength);         /* snap         */
                perf().push_trigger_undo(m_drop_sequence);       /* stazed fix   */
                seq->add_trigger(m_drop_tick, seqlength);
                draw_all();
            }
            result = true;
        }
        else
        {
            /*
             * Set this flag to tell on_motion_notify_event() to call
             * perf().push_trigger_undo().  This section handles motions of the
             * held mouse that grow or shrink the selected trigger, or else
             * the moving of the selected trigger.
             */

            m_have_button_press = seq->select_trigger(m_drop_tick);

            midipulse tick0 = seq->selected_trigger_start();
            midipulse tick1 = seq->selected_trigger_end();
            int ydrop = m_drop_y % c_names_y;

            /*
             * Check for a corner drag (on the small box at the left of the
             * trigger segment) to "grow" the sequence start.  Otherwise,
             * check for a corner drag (on the small box at the right of the
             * sequence) to "grow" the sequence end.  Otherwise, we are moving
             * the sequence.
             */

            if
            (
                m_drop_tick >= tick0 && m_drop_tick <= (tick0 + m_w_scale_x) &&
                ydrop <= sm_perfroll_size_box_click_w + 1
            )
            {
                m_growing = true;
                m_grow_direction = true;
                m_drop_tick_offset = m_drop_tick - seq->selected_trigger_start();
            }
            else if
            (
                m_drop_tick >= (tick1 - m_w_scale_x) && m_drop_tick <= tick1 &&
                ydrop >= c_names_y - sm_perfroll_size_box_click_w - 1
            )
            {
                m_growing = true;
                m_grow_direction = false;
                m_drop_tick_offset = m_drop_tick - seq->selected_trigger_end();
            }
            else    // if (m_drop_tick >= start_tick && m_drop_tick <= end_tick)
            {
                m_moving = true;
                m_drop_tick_offset = m_drop_tick - seq->selected_trigger_start();
            }
            draw_all();
        }
    }
    else if (SEQ64_CLICK_RIGHT(ev->button))
    {
        activate_adding(true);
    }
    else if (SEQ64_CLICK_MIDDLE(ev->button))                   /* split    */
    {
        /*
         * The middle click in seq24 interaction mode is either for splitting
         * the triggers or for setting the paste location of copy/paste.
         */

        if (perf().is_active(m_drop_sequence))
        {
            bool state = seq->get_trigger_state(m_drop_tick);
            if (state)
            {
                seq->split_trigger(m_drop_tick);
                result = true;
            }
            else
            {
                perf().push_trigger_undo(m_drop_sequence);
                seq->paste_trigger(m_drop_tick);

                // Should we add this:  result = true;
            }
        }
    }
    (void) perfroll::on_button_press_event(ev);
    return result;
}

#endif  // USE_ENABLE_BOX_SET

/**
 *  Handles various button-release events.
 *  Any use for the middle-button or ctrl-left-click we can add?
 *
 * \return
 *      Returns true if any modification occurred.
 */

bool
Seq24PerfInput::on_button_release_event (GdkEventButton * ev)
{
    bool result = false;
    if (SEQ64_CLICK_LEFT(ev->button))
    {
        if (is_adding())
            set_adding_pressed(false);

#ifdef USE_SONG_BOX_SELECT

#ifdef USE_ENABLE_BOX_SET
            perf().select_triggers_in_range
            (
                m_box_select_low, m_box_select_high, tick_s, tick_f
            );

#else

        if (m_box_select)      /* calculate selected seqs in box   */
        {
            int x, y, w, h;         /* window dimensions                */
            midipulse tick_s;
            midipulse tick_f;
            m_current_x = ev->x;
            m_current_y = ev->y;
            snap_y(m_current_y);
            rect::xy_to_rect_values
            (
                m_drop_x, m_drop_y,
                m_current_x, m_current_y, x, y, w, h
            );
            convert_xy(x,     y, tick_s, m_box_select_low);
            convert_xy(x+w, y+h, tick_f, m_box_select_high);

            /*
             * May need a "shift-select" version of this function as well.
             */

        }

#endif  // USE_ENABLE_BOX_SET

#endif  // USE_SONG_BOX_SELECT

    }
    else if (SEQ64_CLICK_RIGHT(ev->button))
    {
        /*
         * Minor new feature.  If the Super (Mod4, Windows) key is
         * pressed when release, keep the adding-state in force.  One
         * can then use the unadorned left-click key to add material.  Right
         * click to reset the adding mode.  This feature is enabled only
         * if allowed by Options / Mouse (but is true by default).
         * See the same code in seq24seqcpp.
         */

        bool addmode_exit = ! rc().allow_mod4_mode();
        if (! addmode_exit)
            addmode_exit = ! is_super_key(ev);              /* Mod4 held? */

        if (addmode_exit)
        {
            set_adding_pressed(false);
            activate_adding(false);
        }
    }

    m_moving = m_growing = false;
    set_adding_pressed(false);
    m_effective_tick = 0;

#ifdef USE_SONG_BOX_SELECT
    m_box_select = false;
    m_last_tick = 0;
#endif

    if (perf().is_active(m_drop_sequence))
        draw_all();

    (void) perfroll::on_button_release_event( ev);
    return result;
}

/**
 *  Handles the normal motion-notify event.
 *
 * \return
 *      Returns true if a modification occurs.  This function used to always
 *      return true.
 */

bool
Seq24PerfInput::on_motion_notify_event (GdkEventMotion * ev)
{
    bool result = false;
    int x = int(ev->x);
    sequence * seq = perf().get_sequence(m_drop_sequence);
    if (is_nullptr(seq))
        return false;

    midipulse tick;
    if (is_adding() && is_adding_pressed())
    {
        convert_x(x, tick);

        midipulse seqlength = seq->get_length();

#ifdef USE_SONG_RECORDING
        if (perf().song_record_snap())         /* snap to seq length   */
#endif
            tick -= (tick % seqlength);

        // midipulse length = seqlength;

        seq->grow_trigger(m_drop_tick, tick, seqlength);
        draw_all();
        result = true;
    }
    else if (m_moving || m_growing)
    {
        /*
         * This code is necessary to insure that there is no push unless
         * we have a motion notification.
         */

        if (m_have_button_press)
        {
            perf().push_trigger_undo(m_drop_sequence);
            m_have_button_press = false;
        }

        midipulse tick;
        convert_x(x, tick);
        tick -= m_drop_tick_offset;

#ifdef USE_SONG_RECORDING
        if (perf().song_record_snap())         /* snap to seq length   */
#endif
            tick -= tick % m_snap;

        if (m_moving)
        {
#ifdef USE_SONG_BOX_SELECT
            if (m_last_tick != 0)
            {
                for
                (
                    int seqid = m_box_select_low;
                    seqid < m_box_select_high; ++seqid
                )
                {
                    sequence * sq = perf().get_sequence(seqid);
                    if (not_nullptr(sq))
                    {
                        seq->offset_selected_triggers_by(-(m_last_tick - tick));
                    }
                }
            }
#else
#ifdef USE_ENABLE_BOX_SET
            /////// perf().move_selected_triggers(tick);
            ///////
            /////// Currently this is needed to get the single trigger to
            /////// move, albeit only after the mouse is released.
            seq->move_selected_triggers_to(tick, true);
#else
            seq->move_selected_triggers_to(tick, true);
#endif
#endif
            result = true;
        }
        if (m_growing)
        {
            if (m_grow_direction)
            {
#ifdef USE_SONG_BOX_SELECT
                if (m_last_tick != 0)
                {
                    for
                    (
                        int seqid = m_box_select_low;
                        seqid < m_box_select_high; ++seqid
                    )
                    {
                        sequence * sq = get_sequence(seqid);
                        if (not_nullptr(sq))
                        {
                            sq->offset_selected_triggers_by
                            (
                                -(m_last_tick - tick), triggers::GROW_START
                            );
                        }
                    }
                }
#else
                seq->move_selected_triggers_to
                (
                    tick, false, triggers::GROW_START
                );
#endif
            }
            else
            {
#ifdef USE_SONG_BOX_SELECT
                if (m_last_tick != 0)
                {
                    for
                    (
                        int seqid = m_box_select_low;
                        seqid < m_box_select_high; ++seqid
                    )
                    {
                        sequence * sq = get_sequence(seqid);
                        if (not_nullptr(sq))
                        {
                            sq->offset_selected_triggers_by
                            (
                                -(m_last_tick - tick), triggers::GROW_END
                            );
                        }
                    }
                }
#else
                seq->move_selected_triggers_to
                (
                    tick - 1, false, triggers::GROW_END
                );
#endif
            }
            result = true;
        }
        draw_all();
    }
#ifdef USE_SONG_BOX_SELECT
    else if (m_box_select)
    {
        m_current_x = ev->x;
        m_current_y = ev->y;
        snap_y(m_current_y);
        convert_xy(0, m_current_y, tick, m_drop_sequence);
    }
    m_last_tick = tick;
#endif

    (void) perfroll::on_motion_notify_event(ev);
    return result;
}

/**
 *  Handles the keystroke motion-notify event for moving a pattern back and
 *  forth in the performance.
 *
 *  What happens when the mouse is used to drag the pattern is that, first,
 *  m_drop_tick is set by left-clicking into the pattern to select it.
 *  As the pattern is dragged, the drop-tick value does not change, but the
 *  tick (converted from the moving x value) does.
 *
 *  Then the button-handler sets m_moving = true, and calculates
 *  m_drop_tick_offset = m_drop_tick -
 *  perf().get_sequence(m_drop_sequence)->selected_trigger_start();
 *
 *  The motion handler sees that m_moving is true, gets the new tick
 *  value from the new x value, offsets it, and calls
 *  perf().get_sequence(m_drop_sequence)->move_selected_triggers_to(tick, true).
 *
 *  When the user releases the left button, then m_growing is turned of
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
Seq24PerfInput::handle_motion_key (bool is_left)
{
    bool result = false;
    bool ok = m_drop_sequence >= 0;
    if (ok)
    {
        if (m_effective_tick == 0)
            m_effective_tick = m_drop_tick;

        if (is_left)
        {
            /*
             * This needlessly prevents a selection from moving leftward
             * from its original position.  We'll just put up with the
             * annoyance of absorbed decrements and document it in
             * sequencer64-doc.
             *
             * if (m_effective_tick < m_drop_tick)
             *     m_effective_tick = m_drop_tick;
             */

            midipulse tick = m_effective_tick;
            m_effective_tick -= m_snap;
            if (m_effective_tick <= 0)
                m_effective_tick += m_snap;    /* retrench */

            if (m_effective_tick != tick)
                result = true;
        }
        else
        {
            m_effective_tick += m_snap;
            result = true;

            /*
             * What is the upper boundary here?
             */
        }

        midipulse tick = m_effective_tick - m_drop_tick_offset;
        tick -= tick % m_snap;

#ifdef USE_ENABLE_BOX_SET

        perf().box_move_selected_triggers(tick);

#else

        /*
         * Hmmmm, this overrides any result setting above.  Due to issues with
         * triggers::move_selected(), this always returns true.  Let's ignore
         * the return value for now.
         */

        sequence * sq = perf().get_sequence(m_drop_sequence);
        if (not_nullptr(sq))
            (void) sq->move_selected_triggers_to(tick, true);
#endif

    }
    return result;
}

}           // namespace seq64

/*
 * perfroll_input.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

