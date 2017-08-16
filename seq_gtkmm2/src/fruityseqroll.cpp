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
 * \file          fruityseqroll.cpp
 *
 *  This module declares/defines the base class for seqroll interactions
 *  using the "fruity" mouse paradigm.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2017-08-14
 * \license       GNU GPLv2 or above
 *
 *  This module handles "fruity" interactions only in the piano roll
 *  section of the pattern editor.
 */

#include <gdkmm/cursor.h>

#include "click.hpp"                    /* SEQ64_CLICK_LEFT(), etc.     */
#include "event.hpp"
#include "gui_key_tests.hpp"            /* seq64::is_no_modifier() etc. */
#include "perform.hpp"
#include "seqroll.hpp"
#include "sequence.hpp"
#include "seqkeys.hpp"

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *  An internal variable for user-jitter control.
 */

static const int s_jitter_amount = 6;

/**
 *  Updates the mouse pointer, implementing a context-sensitive mouse.
 *
 * \param sroll
 *      Provides the "parent" of this interaction class.
 */

void
FruitySeqRollInput::update_mouse_pointer (seqroll & sroll)
{
    sroll.update_mouse_pointer(sroll.adding());
}

/**
 *  Implements the fruity on-button-press callback.
 *
 *  This function now uses the needs_update flag to determine if the perform
 *  object should modify().
 *
 * \param ev
 *      The button event.
 *
 * \param sroll
 *      The parent of this "fruity" interaction class.
 *
 * \return
 *      Returns the value of needs_update.  It used to return only true.
 */

bool
FruitySeqRollInput::on_button_press_event (GdkEventButton * ev, seqroll & sroll)
{
    midipulse tick_s, tick_f;
    int note_h, note_l;
    sequence & seq = sroll.m_seq;                   /* just do this once!   */
    int norm_x, snapped_x, snapped_y;
    bool needs_update = sroll.button_press_initial  /* focus grab and more! */
    (
        ev, norm_x, snapped_x, snapped_y            /* 3 side-effects       */
    );
    if (! needs_update)
    {
        int norm_x = snapped_x;
        if (SEQ64_CLICK_LEFT(ev->button))
        {
            sroll.set_current_drop_x(norm_x);       /* selection normal x   */
            sroll.convert_xy(sroll.m_drop_x, sroll.m_drop_y, tick_s, note_h);
            int eventcount = seq.select_note_events
            (
                tick_s, note_h, tick_s, note_h, sequence::e_would_select
            );
            if (eventcount == 0 && ! is_ctrl_key(ev))
            {
                sroll.set_adding(true);              /* not on top of event */
                sroll.m_painting = true;             /* start the paint job */
                sroll.set_current_drop_x(snapped_x); /* adding, snapped x   */
                sroll.convert_xy
                (
                    sroll.m_drop_x, sroll.m_drop_y, tick_s, note_h
                );

                /*
                 * Stazed fix, forwards the event to play the hint note.
                 */

                sroll.m_seqkeys_wid.set_listen_button_press(ev);
                eventcount = seq.select_note_events /* note already there?  */
                (
                    tick_s, note_h, tick_s, note_h, sequence::e_would_select
                );
                if (eventcount == 0)
                {
                    sroll.add_note(tick_s, note_h); /* also does chords     */
                    needs_update = true;
                }
            }
            else                                            /* selecting    */
            {
                /*
                 * If under the cursor is not a selected note...
                 */

                eventcount = seq.select_note_events
                (
                    tick_s, note_h, tick_s, note_h, sequence::e_is_selected
                );
                if (eventcount == 0)
                {
                    eventcount = seq.select_note_events
                    (
                        tick_s, note_h, tick_s, note_h, sequence::e_would_select
                    );
                    if (eventcount > 0)
                    {
                        /*
                         * If clicking a note, unselect all if ctrl not held.
                         */

                        if (! is_ctrl_key(ev))
                            seq.unselect();
                    }
                    else
                    {
                        /*
                         * If clicking empty space, then unselect all if
                         * ctrl-shift not held.
                         */

                        if (! is_ctrl_shift_key(ev))
                            seq.unselect();
                    }

                    eventcount = seq.select_note_events
                    (
                        tick_s, note_h, tick_s, note_h,
                        sequence::e_select_one  /* direct click, one event */
                    );

                    /* prevent deselect in button_release() */

                    if (eventcount)
                        sroll.m_justselected_one = true;

                    /* if nothing selected, start the selection box */

                    if (eventcount == 0 && is_ctrl_key(ev))
                        sroll.m_selecting = true;

                    needs_update = true;
                }

                /* if note under cursor is selected */

                eventcount = seq.select_note_events
                (
                    tick_s, note_h, tick_s, note_h, sequence::e_is_selected
                );
                if (eventcount > 0)
                {
                    /*
                     * Context sensitive mouse handling for fruity mode.
                     * In seq24, context handling for the left side of the
                     * event was not yet supported; we're going to try to
                     * rectify that.
                     */

                    bool left_handle = false;
                    bool right_handle = false;
                    bool center_handle = false;
                    {
                        midipulse tick;                 // drop tick
                        int note;                       // drop note
                        midipulse s, f;                 // start & finish ticks
                        int n;                          // note number
                        sroll.convert_drop_xy(tick, note);
                        bool found = seq.intersect_notes(tick, note, s, f, n);
                        if (found && n == note)
                        {
                            midipulse hsize = seq.handle_size(s, f);
                            if (tick >= (f - hsize) && tick <= f)
                            {
                                right_handle = true;
                            }
                            else if (tick >= s && tick <= (s + hsize))
                            {
                                left_handle = true;
                            }
                            else
                            {
                                center_handle = true;
                            }
                        }
                    }
                    bool grabmovenote = ! is_ctrl_key(ev) && center_handle;
                    if (grabmovenote)       /* grab/move the note           */
                    {
                        /*
                         * seqroll::align_selection() [proposed]:
                         * Get the box that selected elements are in.  Save
                         * offset that we get from the snap above.  Align
                         * selection for drawing.
                         */

                        needs_update = true;
                        sroll.m_moving_init = true;
                        sroll.get_selected_box(tick_s, note_h, tick_f, note_l);

                        int adjusted_selected_x = sroll.m_selected.x;
                        sroll.snap_x(adjusted_selected_x);
                        sroll.m_move_snap_offset_x =
                            sroll.m_selected.x - adjusted_selected_x;

                        sroll.snap_x(sroll.m_selected.x);   /* align to draw */
                        sroll.set_current_drop_x(snapped_x);

                        /*
                         * Stazed fix, forward the event to play hint note.
                         */

                        sroll.m_seqkeys_wid.set_listen_button_press(ev);
                    }
                    else if     /* Ctrl-L-click when stuff already selected */
                    (
                        is_ctrl_key(ev) && seq.select_note_events
                        (
                            tick_s, note_h, tick_s, note_h,
                            sequence::e_is_selected
                        )
                    )
                    {
                        sroll.m_is_drag_pasting_start = true;
                        m_drag_paste_start_pos[0] = int(ev->x);
                        m_drag_paste_start_pos[1] = int(ev->y);
                    }
                    if  /* left click on the right handle = grow/resize event  */
                    (
                        (left_handle || right_handle) && ! is_ctrl_key(ev)
                    )
                    {
                        /* get the box that selected elements are in */

                        sroll.m_growing = true;
                        sroll.get_selected_box(tick_s, note_h, tick_f, note_l);
                    }
                }
            }
        }
        if (SEQ64_CLICK_MIDDLE(ev->button))
        {
            /* get the box that selected elements are in */

            sroll.m_growing = true;
            sroll.get_selected_box(tick_s, note_h, tick_f, note_l);
        }
        if (SEQ64_CLICK_RIGHT(ev->button))
        {
            sroll.set_current_drop_x(norm_x);           /* selection normal x */

            /* turn x,y in to tick/note */

            sroll.convert_xy(sroll.m_drop_x, sroll.m_drop_y, tick_s, note_h);
            if              /* erase event(s) under cursor if there is one */
            (
                seq.select_note_events
                (
                    tick_s, note_h, tick_s, note_h, sequence::e_would_select
                )
            )
            {
                /* ctrl-right click: remove all selected notes */

                if (is_ctrl_key(ev))
                {
                    seq.select_note_events
                    (
                        tick_s, note_h, tick_s, note_h, sequence::e_select_one
                    );

                    /*
                     * Stazed fix.  TODO:  incorporated the mark-check and the
                     * push-undo into sequence::remove_selected().  Done.
                     *
                     * if (seq.mark_selected())
                     * {
                     *     seq.push_undo();
                     *     seq.remove_selected();
                     * }
                     */

                    seq.remove_selected();
                }
                else
                {
                    /*
                     * right click: remove only the note under the cursor,
                     * leave the selection intact.
                     */

                    seq.select_note_events
                    (
                        tick_s, note_h, tick_s, note_h, sequence::e_remove_one
                    );
                }

                /*
                 * hold down the right button, drag mouse around erasing notes:
                 */

                m_erase_painting = true;
                needs_update = true;    /* repaint... we've changed the notes */
            }
            else                                            /* selecting */
            {
                if (! is_ctrl_key(ev))
                    seq.unselect();

                sroll.m_selecting = true;   /* start the new selection box */
                needs_update = true;
            }
        }
    }
    update_mouse_pointer(sroll);        /* context sensitive mouse pointer... */
    if (needs_update)                   /* if they clicked, something changed */
        seq.set_dirty();                /* redraw_events();                   */

    return needs_update;
}

/**
 *  Implements the fruity handling for the on-button-release event.
 *
 * \param ev
 *      The button event.
 *
 * \param sroll
 *      The parent of this "fruity" interaction class.
 *
 * \return
 *      Returns the value of needs_update.  It used to return only true.
 */

bool
FruitySeqRollInput::on_button_release_event
(
    GdkEventButton * ev, seqroll & sroll
)
{
    midipulse tick_s;
    midipulse tick_f;
    int note_h;
    int note_l;
    int x, y, w, h;
    sequence & seq = sroll.m_seq;                   /* just do this once!   */
    bool needs_update = false;
    sroll.m_current_x = int(ev->x + sroll.m_scroll_offset_x);
    sroll.m_current_y = int(ev->y + sroll.m_scroll_offset_y);
    sroll.snap_y(sroll.m_current_y);
    if (sroll.m_moving || sroll.m_is_drag_pasting)
        sroll.snap_x(sroll.m_current_x);

    int delta_x = sroll.current_x() - sroll.drop_x();
    int delta_y = sroll.current_y() - sroll.drop_y();
    midipulse delta_tick;
    int delta_note;

    /*
     * Stazed fix, forwards the event to turn off the hint note.
     */

    sroll.m_seqkeys_wid.set_listen_button_press(ev);

    /* middle click, or ctrl- (???) left click button up */

    if (SEQ64_CLICK_LEFT_MIDDLE(ev->button))
    {
        if (sroll.m_growing)        /* convert deltas into screen coordinates */
        {
            sroll.convert_xy(delta_x, delta_y, delta_tick, delta_note);

            /*
             * sroll.m_seq->push_undo();
             */

            if (is_shift_key(ev))
                seq.stretch_selected(delta_tick);
            else
                seq.grow_selected(delta_tick);

            needs_update = true;
        }
    }

    midipulse current_tick;
    int current_note;
    sroll.convert_xy            /* try to eliminate this */
    (
        sroll.current_x(), sroll.current_y(), current_tick, current_note
    );

    /*
     *  -   ctrl-left click button up for select/drag copy/paste
     *  -   left click button up for ending a move of selected notes
     */

    if (SEQ64_CLICK_LEFT(ev->button))
    {
        sroll.set_adding(false);
        if (sroll.m_is_drag_pasting)
        {
            sroll.m_is_drag_pasting = false;
            sroll.m_is_drag_pasting_start = false;

            /* convert deltas into screen coordinates */

            sroll.complete_paste();
            needs_update = true;
        }

        /* ctrl-left click but without movement - select a note */

        if (sroll.m_is_drag_pasting_start)
        {
            sroll.m_is_drag_pasting_start = false;

            /*
             * If ctrl-left click without movement and if note under
             * cursor is selected, and ctrl is held and button-down didn't
             * just select one, then deselect the note.
             */

            if
            (
                is_ctrl_key(ev) &&
                ! sroll.m_justselected_one &&
                seq.select_note_events
                (
                    current_tick, current_note, current_tick, current_note,
                    sequence::e_is_selected
                )
            )
            {
                (void) seq.select_note_events
                (
                    current_tick, current_note, current_tick, current_note,
                    sequence::e_deselect
                );
                needs_update = true;
            }
        }
        sroll.m_justselected_one = false; /* clear flag on left button up */

        if (sroll.m_moving)
        {
            /**
             * If in moving mode, adjust for snap and convert deltas into
             * screen coordinates.  Since delta_note was from delta_y, it will
             * be flipped (delta_y[0] = note[127], etc.), so we have to
             * adjust.
             */

            delta_x -= sroll.m_move_snap_offset_x;      /* adjust for snap */
            sroll.convert_xy(delta_x, delta_y, delta_tick, delta_note);
            delta_note -= c_num_keys - 1;

            /*
             * sroll.m_seq->push_undo();
             */

            seq.move_selected_notes(delta_tick, delta_note);
            needs_update = true;
        }
    }

    /* right click or left click button up for selection box */

    if (SEQ64_CLICK_LEFT_RIGHT(ev->button))
    {
        if (sroll.m_selecting)
        {
            sroll.xy_to_rect
            (
                sroll.drop_x(), sroll.drop_y(),
                sroll.current_x(), sroll.current_y(), x, y, w, h
            );
            sroll.convert_xy(x, y, tick_s, note_h);
            sroll.convert_xy(x + w, y + h, tick_f, note_l);
            (void) seq.select_note_events
            (
                tick_s, note_h, tick_f, note_l, sequence::e_toggle_selection
            );
            needs_update = true;
        }
    }
    if (SEQ64_CLICK_RIGHT(ev->button))
        m_erase_painting = false;

    sroll.m_selecting = false;          /* turn it all off */
    sroll.m_moving = false;
    sroll.m_growing = false;
    sroll.m_paste = false;
    sroll.m_moving_init = false;
    sroll.m_painting = false;
    seq.unpaint_all();
    update_mouse_pointer(sroll);        /* context sensitive mouse pointer... */
    if (needs_update)                   /* if they clicked, something changed */
        seq.set_dirty();        /* redraw_events();                   */

    return needs_update;
}

/**
 *  Implements the fruity handling for the on-motion-notify event.
 *
 * \param ev
 *      The motion event.
 *
 * \param sroll
 *      The parent of this "fruity" interaction class.  (Why not just inherit
 *      and save all these indirect accesses to the seqroll? Well, that would
 *      make it more difficult to change the mode of interation, in the
 *      Options menu, on the fly.)
 *
 * \return
 *      Returns the value of needs_update.
 */

bool
FruitySeqRollInput::on_motion_notify_event
(
    GdkEventMotion * ev,
    seqroll & sroll
)
{
    bool result = false;
    sequence & seq = sroll.m_seq;                   /* just do this once!   */
    sroll.set_current_offset_x_y(int(ev->x), int(ev->y));
    if (sroll.m_moving_init)
    {
        sroll.m_moving_init = false;
        sroll.m_moving = true;
    }
    update_mouse_pointer(sroll);    /* context sensitive mouse pointer... */

    /**
     * In "fruity" interatction mode, ctrl-left-click-drag on selected note(s)
     * starts a copy/unselect/paste.  Doesn't begin the paste until the mouse
     * moves a few pixels, to filter out the unsteady hand.
     */

    if
    (
        sroll.m_is_drag_pasting_start &&
        (
            s_jitter_amount <= abs(m_drag_paste_start_pos[0] - int(ev->x)) ||
            s_jitter_amount <= abs(m_drag_paste_start_pos[1] - int(ev->y))
        )
    )
    {
        seq.copy_selected();
        seq.unselect();
        sroll.start_paste();
        sroll.m_is_drag_pasting_start = false;
        sroll.m_is_drag_pasting = true;
    }

    /*
     * seqroll::set_hint_note()
     */

    int note;
    midipulse tick;
    sroll.snap_y(sroll.m_current_y);
    sroll.convert_xy(0, sroll.m_current_y, tick, note);
    sroll.m_seqkeys_wid.set_hint_key(note);
    if (sroll.select_action())
    {
        if (sroll.drop_action())                        /* moving or paste  */
            sroll.snap_x(sroll.m_current_x);

        if (sroll.moving())                             /* stazed fix       */
            sroll.m_seqkeys_wid.set_listen_motion_notify(ev);

        sroll.draw_selection_on_window();
        result = true;
    }
    else if (sroll.m_painting)
    {
#ifdef SEQ64_STAZED_CHORD_GENERATOR
        if (sroll.m_chord != 0)     /* chord, don't allow move painting */
            result = true;
        else
#endif
        {
            sroll.snap_x(sroll.m_current_x);
            sroll.convert_xy(sroll.current_x(), sroll.current_y(), tick, note);
            sroll.add_note(tick, note);
            result = true;
        }
    }
    else if (m_erase_painting)
    {
        sroll.convert_xy(sroll.current_x(), sroll.current_y(), tick, note);
        if
        (
            seq.select_note_events
            (
                tick, note, tick, note, sequence::e_would_select
            )
        )
        {
            /* remove only note under the cursor, leave selection intact */

            seq.select_note_events
            (
                tick, note, tick, note, sequence::e_remove_one
            );
            seq.set_dirty();
        }
    }
    return result;
}

}           // namespace seq64

/*
 * fruityseqroll.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

