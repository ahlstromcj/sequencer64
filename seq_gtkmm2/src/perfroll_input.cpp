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
 * \updates       2018-08-04
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
 */

Seq24PerfInput::Seq24PerfInput
(
    perform & p,
    perfedit & parent,
    Gtk::Adjustment & hadjust,
    Gtk::Adjustment & vadjust
) :
    perfroll            (p, parent, hadjust, vadjust, p.get_ppqn()),
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
 *  Initial unselection:
 *
 *      This code causes the un-greying of the previously selected trigger
 *      segment.  If commented out, then we can seemingly select more than one
 *      trigger segment, but only the last one "selected" can be moved.  We'd
 *      like to be able to select and move a bunch at once by holding a modifier
 *      key.
 *
 *  convert_drop_xy():
 *
 *      convert_drop_xy() uses m_drop_x and m_drop_y, and sets m_drop_tick and
 *      m_drop_sequence.  It gets the sequence number of the pattern clicked.
 *      It doesn't matter which button was clicked.
 *
 *  is_adding():
 *
 *      The is_adding() status is set on a right-click with no trigger under the
 *      mouse, and unset when the right-click is released.
 *
 *  Ctrl key:
 *
 *      Note the is_ctrl_key() test.  We make better use of the Ctrl key here.
 *      Let Ctrl-left be handled exactly like the middle click (split the
 *      segment/trigger), then bug out.  Middle click in seq24 mouse mode is
 *      either for splitting the triggers or for setting the paste location of
 *      copy/paste.  The "a_or_b_trigger()" functions check the trigger status
 *      of the sequence to decide what to do, so that the caller doesn't have to
 *      do those checks.
 *
 *  Shift key:
 *
 *      TODO.
 *
 *  No modifier key:
 *
 *      Add a new sequence if nothing is selected.  The adding flag is set on a
 *      right-click where there is no trigger under the mouse, and is unset when
 *      the right-click is released [if not in allow_mod4_mode()].
 *
 *  Mouse buttons:
 *
 *      The middle click in seq24 interaction mode is either for splitting the
 *      triggers or for setting the paste location of copy/paste.
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
 *      This new setup is meant to support selecting multiple sequences in the
 *      perfroll, and later to support Kepler34's box-selection of multiple
 *      sequences for movement.  Here are the rules we want to implement:
 *
 *      -   Nothing selected.
 *          -   Outside a trigger segment (determined by the "intersect"
 *              function.)
 *          -   Inside a trigger segment.
 *      -   Click or Shift-Click with nothing already selected will select the
 *          current drop sequence and add it to the set.
 *      -   Click with at least one *other* sequence selected will unselect
 *          the others and select the current sequence.
 *      -   Click or Shift-Click outside a sequence will unselect them all.
 *      -   Shift-Click will leave the other selected sequences alone and add
 *          the current drop sequence.
 *
 * \return
 *      Returns true if a modification occurred, necessitating a redraw.
 */

bool
Seq24PerfInput::on_button_press_event (GdkEventButton * ev)
{
    bool result = false;
    sequence * seq = perf().get_sequence(m_drop_sequence);
    bool dropseq_active;
    grab_focus();
#ifndef SEQ64_SONG_BOX_SELECT
    dropseq_active = not_nullptr(seq);
    if (dropseq_active && ! is_shift_key(ev))       /* initial unselection  */
    {
        seq->unselect_triggers();
        result = true;                              // draw_all();
    }
#endif
    m_drop_x = int(ev->x);
    m_drop_y = int(ev->y);
    convert_drop_xy();                              /* set the m_drop_xxx's */
    seq = perf().get_sequence(m_drop_sequence);
    dropseq_active = not_nullptr(seq);

#ifdef SEQ64_SONG_BOX_SELECT
    bool in_seq = perf().is_active(m_drop_sequence);
    bool in_trigger = perf().intersect_triggers(m_drop_sequence, m_drop_tick);
#else
    if (! dropseq_active)
        return result;
#endif

    if (is_ctrl_key(ev))
    {
        if (SEQ64_CLICK_LEFT(ev->button))
        {
            perf().paste_or_split_trigger(m_drop_sequence, m_drop_tick);
            result = true;
        }
#ifdef SEQ64_SONG_BOX_SELECT_XXX
        else if (SEQ64_CLICK_RIGHT(ev->button) && ! in_trigger)
        {
            perf().unselect_all_triggers();
            result = true;                      // may cause "modify()" !!!
        }
#endif
    }
    else if (is_shift_key(ev))
    {
        // CURRENT ISSUES:
        //
        //  - Shift-click sometimes doesn't toggle a trigger segment.
        //  - Regular click doesn't unselect all the other selected segments.

#ifdef SEQ64_SONG_BOX_SELECT
        if (SEQ64_CLICK_LEFT(ev->button))
        {
            if (in_trigger)
            {
                perf().box_toggle_sequence(m_drop_sequence, m_drop_tick);
                result = true;
            }
        }
        else if (SEQ64_CLICK_RIGHT(ev->button))
        {
            if (! in_trigger)
            {
                perf().unselect_all_triggers();
                result = true;
            }
        }
#endif
    }
    else                                /* no modifier key pressed  */
    {
        if (SEQ64_CLICK_LEFT(ev->button))
        {
            if (is_adding())
            {
                set_adding_pressed(true);
                perf().add_or_delete_trigger(m_drop_sequence, m_drop_tick);
                draw_all();
                result = true;
            }
            else
            {
                if (check_handles())                /* can select trigger!  */
                {
                    draw_all();
                    result = true;                  /* sequence inserted    */
                }
            }
        }
        else if (SEQ64_CLICK_MIDDLE(ev->button))    /* split like Ctrl-left */
        {
            perf().paste_or_split_trigger(m_drop_sequence, m_drop_tick);
            result = true;
        }
        else if (SEQ64_CLICK_RIGHT(ev->button))
        {
            activate_adding(true);
#ifdef SEQ64_SONG_BOX_SELECT_XXX
            perf().unselect_all_triggers();
#endif
            result = true;
        }
    }
    (void) perfroll::on_button_press_event(ev);
#ifdef SEQ64_SONG_BOX_SELECT_XXX
    if (result)
        draw_all();
#endif

    return result;
}

/**
 *  Handles various button-release events.
 *  Any use for the middle-button or ctrl-left-click we can add?
 *
 * \param ev
 *      The button event to be handles as a button-release.
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
        if (is_adding())                /* we were in "paint" mode...       */
            set_adding_pressed(false);  /* ... and now we are not           */

#ifdef SEQ64_SONG_BOX_SELECT_XXX

        if (selecting())                /* calculate selected seqs in box   */
        {
            int x, y, w, h;             /* window dimension side-effects    */
            midipulse tick_s, tick_f;
            m_current_x = ev->x;
            m_current_y = ev->y;
            snap_y(m_current_y);
            rect::xy_to_rect_get
            (
                m_drop_x, m_drop_y, m_current_x, m_current_y, x, y, w, h
            );
            convert_xy(x,     y, tick_s, m_box_select_low);
            convert_xy(x+w, y+h, tick_f, m_box_select_high);
            perf().select_triggers_in_range
            (
                m_box_select_low, m_box_select_high, tick_s, tick_f
            );
            m_last_tick = 0;                    // ????
            selecting(false);
        }
#endif  // SEQ64_SONG_BOX_SELECT_XXX
    }
    else if (SEQ64_CLICK_RIGHT(ev->button))
    {
        /*
         * Minor new feature.  If the Super (Mod4, Windows) key is
         * pressed when release, keep the adding-state in force.  One
         * can then use the unadorned left-click key to add material.  Right
         * click to reset the adding mode.  This feature is enabled only
         * if allowed by Options / Mouse. See the same code in seq24seq.cpp.
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

    if (perf().is_active(m_drop_sequence))
        draw_all();

    (void) perfroll::on_button_release_event( ev);
    return result;
}

/**
 *  Handles the normal motion-notify event.  We now check to see if it is a
 *  drag-motion (left/middle/right-click-drag) to avoid flickering when just
 *  moving the mouse across the surface.
 *
 * \param ev
 *      Provides a pointer to the motion event.
 *
 * \return
 *      Returns true if a modification occurs.  This function used to always
 *      return true.
 */

bool
Seq24PerfInput::on_motion_notify_event (GdkEventMotion * ev)
{
    bool result = is_drag_motion(ev);                   /* left click-drag  */
    if (result)
    {
        int x = int(ev->x);

#ifdef SEQ64_SONG_BOX_SELECT_XXX

        int y = int(ev->y);

        set_current_offset_x_y(int(ev->x), int(ev->y)); /* adj for scroll   */
        if (is_left_drag(ev) && selecting())
        {
            m_selected.xy_to_rect(m_old.x(), m_old.y(), x, y);
            draw_selection_on_window();
            // return true;
        }

#endif

        sequence * seq = perf().get_sequence(m_drop_sequence);
        if (is_nullptr(seq))
            return false;           // Do we want to clear m_selected here?

        midipulse tick;
        if (is_adding() && is_adding_pressed())
        {
            convert_x(x, tick);

            midipulse seqlength = seq->get_length();

#ifdef SEQ64_SONG_RECORDING
            if (perf().song_record_snap())         /* snap to seq length   */
#endif
                tick -= (tick % seqlength);

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

#ifdef SEQ64_SONG_RECORDING
            if (perf().song_record_snap())         /* snap to seq length   */
#endif
                tick -= tick % m_snap_x;

            if (m_moving)
            {
#ifdef USE_SONG_BOX_SELECT                          // TODO
                if (m_last_tick != 0)
                {
                    midipulse offset = -(m_last_tick - tick);

                    /*
                     * Move this loop to perform!
                     */

                    for
                    (
                        int seqid = m_box_select_low;
                        seqid < m_box_select_high; ++seqid
                    )
                    {
                        sequence * sq = perf().get_sequence(seqid);
                        if (not_nullptr(sq))
                            seq->offset_triggers(offset);
                    }
                }
#else
                seq->move_triggers(tick, true);
#endif
                result = true;
            }
            if (m_growing)
            {
                if (m_grow_direction)
                {
#ifdef USE_SONG_BOX_SELECT                      // TODO
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
                                sq->offset_triggers
                                (
                                    -(m_last_tick - tick), triggers::GROW_START
                                );
                            }
                        }
                    }
#else
                    seq->move_triggers(tick, false, triggers::GROW_START);
#endif
                }
                else
                {
                    triggers::grow_edit_t growend = triggers::GROW_END;

#ifdef USE_SONG_BOX_SELECT                      // TODO
                    if (m_last_tick != 0)
                    {
                        midipulse offset = tick - m_last_tick;
                        for
                        (
                            int seqid = m_box_select_low;
                            seqid < m_box_select_high; ++seqid
                        )
                        {
                            sequence * sq = perf().get_sequence(seqid);
                            if (not_nullptr(sq))
                                sq->offset_triggers(offset, growend);
                        }
                    }
#else
                    seq->move_triggers(tick - 1, false, growend);
#endif
                }
                result = true;
            }
            draw_all();
        }
#ifdef USE_SONG_BOX_SELECT                      // TODO
        else if (selecting())
        {
            m_current_x = ev->x;
            m_current_y = ev->y;
            snap_y(m_current_y);
            convert_xy(0, m_current_y, tick, m_drop_sequence);
        }
        m_last_tick = tick;
#endif

        (void) perfroll::on_motion_notify_event(ev);
    }
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
 *  m_drop_tick_offset = m_drop_tick - (drop sequence)->selected_trigger_start().
 *  The motion handler sees that m_moving is true, gets the new tick
 *  value from the new x value, offsets it, and calls
 *  (drop sequence)->move_triggers(tick, true).
 *  When the user releases the left button, then m_growing is turned off
 *  and the roll draw_all()'s.
 *
 * \param is_left
 *      False denotes the right arrow key, and true denotes the left arrow
 *      key.
 *
 * \return
 *      Returns true if there was some action able to happen that would
 *      necessitate a window update.  We've updated triggers::move_triggers()
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
        /*
         * This value is set back to zero in the button-release callback.
         */

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
            m_effective_tick -= m_snap_x;

            /*
             * Should we set to 0 instead of undoing snap?
             */

            if (m_effective_tick <= 0)
                m_effective_tick += m_snap_x;    /* retrench */

            if (m_effective_tick != tick)
                result = true;
        }
        else
        {
            /*
             * What is the upper boundary here?
             */

            m_effective_tick += m_snap_x;
            result = true;
        }

        midipulse tick = m_effective_tick - m_drop_tick_offset;
        tick -= tick % m_snap_x;

#ifdef SEQ64_SONG_BOX_SELECT
        perf().box_move_triggers(tick);
#else

        /*
         * Hmmmm, this overrides any result setting above.  Due to issues with
         * triggers::move_triggers(), this always returns true.  Let's ignore
         * the return value for now.
         */

        sequence * sq = perf().get_sequence(m_drop_sequence);
        if (not_nullptr(sq))
            (void) sq->move_triggers(tick, true);
#endif

    }
    return result;
}

/**
 *  Pulled this function out to simplify the on-button-press callback.
 *
 *  Set this flag to tell on_motion_notify_event() to call
 *  perf().push_trigger_undo().  This section handles motions of the held mouse
 *  that grow or shrink the selected trigger, or else the moving of the
 *  selected trigger.
 *
 *  wscalex is 4 * 32 ticks/pixel.  What about zoom?  See perfroll; it sets
 *  m_perf_scale_x and m_zoom to c_perf_scale_x.  Let's use m_perf_scale_x
 *  instead and make it a memmber, m_w_scale_x.  This section is almost common
 *  code with the fruityperfrol_input.cpp module.
 *
 *  Check for a corner drag (on the small box at the left of the trigger
 *  segment) to "grow" the sequence start.  Otherwise, check for a corner drag
 *  (on the small box at the right of the sequence) to "grow" the sequence end.
 *  Otherwise, we are moving the sequence.
 *
 * \return
 *      Returns true if the mouse pointer is inside a trigger handle.
 */

bool
Seq24PerfInput::check_handles ()
{
    bool result = false;
    midipulse tick0, tick1;
    m_have_button_press = perf().selected_trigger
    (
        m_drop_sequence, m_drop_tick, tick0, tick1  /* side-effects */
    );
#ifdef SEQ64_SONG_BOX_SELECT
    if (m_have_button_press)
        perf().box_insert(m_drop_sequence, m_drop_tick);
#endif

    int ydrop = m_drop_y % c_names_y;

    /*
     * Check for a corner drag (on the small box at the left of the trigger
     * segment.
     */

    if
    (
        m_drop_tick >= tick0 && m_drop_tick <= (tick0 + m_w_scale_x) &&
        ydrop <= (sm_perfroll_size_box_click_w + 1)
    )
    {
        m_growing = true;
        m_grow_direction = true;
        m_drop_tick_offset = m_drop_tick - tick0;
        result = true;
    }
    else if
    (
        m_drop_tick >= (tick1 - m_w_scale_x) && m_drop_tick <= tick1 &&
        ydrop >= (c_names_y - sm_perfroll_size_box_click_w - 1)
    )
    {
        m_growing = true;
        m_grow_direction = false;
        m_drop_tick_offset = m_drop_tick - tick1;
        result = true;
    }
    else // if (m_drop_tick >= start_tick && m_drop_tick <= end_tick)
    {
        m_moving = true;                            // how can we know this?
        m_drop_tick_offset = m_drop_tick - tick0;
    }
    return result;
}

}           // namespace seq64

/*
 * perfroll_input.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

