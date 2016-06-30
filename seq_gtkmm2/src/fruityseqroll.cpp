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
 * \updates       2016-06-30
 * \license       GNU GPLv2 or above
 *
 *  This module handles "fruity" interactions only in the piano roll
 *  section of the pattern editor.
 */

#include <gdkmm/cursor.h>

#include "click.hpp"                    /* SEQ64_CLICK_LEFT(), etc.    */
#include "event.hpp"
#include "perform.hpp"
#include "seqroll.hpp"
#include "sequence.hpp"
#include "seqkeys.hpp"

namespace seq64
{

/**
 *  An internal variable for handle size.
 */

static const long s_handlesize = 16;

/**
 *  An internal variable for user-jitter control.
 */

static const int s_jitter_amount = 6;

/**
 *  An internal function used by the FruitySeqRollInput class.
 */

inline static long
clamp (long val, long low, long hi)
{
    return val < low ? low : (hi < val ? hi : val) ;
}

/**
 *  Updates the mouse pointer, implementing a context-sensitive mouse.
 *
 * \param sroll
 *      Provides the "parent" of this interaction class.
 */

void
FruitySeqRollInput::update_mouse_pointer (seqroll & sroll)
{
    sroll.update_mouse_pointer(m_adding);
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
    int norm_x, norm_y;
    bool needs_update = false;
    int snapped_x = norm_x = int(ev->x + sroll.m_scroll_offset_x);
    int snapped_y = norm_y = int(ev->y + sroll.m_scroll_offset_y);
    sroll.grab_focus();
    sroll.snap_x(snapped_x);
    sroll.snap_y(snapped_y);
    sroll.set_current_drop_y(snapped_y);            /* y is always snapped   */
    sroll.m_old.x = sroll.m_old.y = sroll.m_old.width = sroll.m_old.height = 0;
    if (sroll.m_paste)    /* ctrl-v pressed, waiting for click where to paste */
    {
        sroll.complete_paste(snapped_x, snapped_y);
        needs_update = true;
    }
    else
    {
        if (SEQ64_CLICK_LEFT(ev->button))
        {
            sroll.set_current_drop_x(norm_x);       /* selection normal x   */
            sroll.convert_xy(sroll.m_drop_x, sroll.m_drop_y, tick_s, note_h);
            int eventcount = sroll.m_seq.select_note_events
            (
                tick_s, note_h, tick_s, note_h, sequence::e_would_select
            );

            /*
             * If not on top of event then add one...
             */

            if (eventcount == 0 && ! (ev->state & SEQ64_CONTROL_MASK))
            {
                m_adding = true;
                sroll.m_painting = true;             /* start the paint job */
                sroll.set_current_drop_x(snapped_x); /* adding, snapped x   */
                sroll.convert_xy
                (
                    sroll.m_drop_x, sroll.m_drop_y, tick_s, note_h
                );

                /*
                 * Test if a note is already there; fake select, if so, no add.
                 */

                eventcount = sroll.m_seq.select_note_events
                (
                    tick_s, note_h, tick_s, note_h, sequence::e_would_select
                );
                if (eventcount == 0)
                {
                    /* add note, length = little less than snap */

                    sroll.m_seq.push_undo();

#ifdef SEQ64_STAZED_CHORD_GENERATOR
                    if (sroll.m_chord > 0)                  /* and less than? */
                    {
                        /*
                         * Needs to be a function in sequence class!  Include
                         * the push_undo() call!
                         */

                        for (int i = 0; i < c_chord_size; ++i)
                        {
                            int cnote = c_chord_table[sroll.m_chord][i];
                            if (cnote == -1)
                                break;

                            sroll.add_note(tick_s, note_h + cnote, false);
                        }
                    }
                    else
                        sroll.add_note(tick_s, note_h);
#else
                    sroll.add_note(tick_s, note_h);
#endif
                    needs_update = true;
                }
            }
            else                                            /* selecting */
            {
                /*
                 * If under the cursor is not a selected note...
                 */

                eventcount = sroll.m_seq.select_note_events
                (
                    tick_s, note_h, tick_s, note_h, sequence::e_is_selected
                );
                if (eventcount == 0)
                {
                    eventcount = sroll.m_seq.select_note_events
                    (
                        tick_s, note_h, tick_s, note_h, sequence::e_would_select
                    );
                    if (eventcount > 0)
                    {
                        /*
                         * If clicking a note, unselect all if ctrl not held.
                         */

                        if (! (ev->state & SEQ64_CONTROL_MASK))
                            sroll.m_seq.unselect();
                    }
                    else
                    {
                        /*
                         * If clicking empty space, then unselect all if
                         * ctrl-shift not held.
                         */

                        if
                        (
                            ! ( (ev->state & SEQ64_CONTROL_MASK) &&
                                (ev->state & SEQ64_SHIFT_MASK) )
                        )
                            sroll.m_seq.unselect();
                    }

                    eventcount = sroll.m_seq.select_note_events
                    (
                        tick_s, note_h, tick_s, note_h,
                        sequence::e_select_one  /* direct click, one event */
                    );

                    /* prevent deselect in button_release() */

                    if (eventcount)
                        sroll.m_justselected_one = true;

                    /* if nothing selected, start the selection box */

                    if (eventcount == 0 && (ev->state & SEQ64_CONTROL_MASK))
                        sroll.m_selecting = true;

                    needs_update = true;
                }

                /* if note under cursor is selected */

                eventcount = sroll.m_seq.select_note_events
                (
                    tick_s, note_h, tick_s, note_h, sequence::e_is_selected
                );
                if (eventcount > 0)
                {
                    bool right_mouse_handle = false;
                    bool center_mouse_handle = false;
                    {
                        midipulse drop_tick;
                        int drop_note;              // midibyte
                        sroll.convert_xy
                        (
                            sroll.m_drop_x, sroll.m_drop_y, drop_tick, drop_note
                        );
                        midipulse start, end;
                        int note;                   // midibyte
                        if
                        (
                            sroll.m_seq.intersect_notes
                            (
                                drop_tick, drop_note, start, end, note
                            ) &&
                            note == drop_note
                        )
                        {
                            long handle_size = clamp // 16 wide unless very small
                            (
                                s_handlesize, 0, (end - start) / 3
                            );
                            if
                            (
                                start <= drop_tick &&
                                drop_tick <= start + handle_size
                            )
                            {
                                center_mouse_handle = true;
                            }
                            else if
                            (
                                end - handle_size <= drop_tick && drop_tick <= end
                            )
                            {
                                right_mouse_handle = true;
                            }
                            else
                            {
                                center_mouse_handle = true;
                            }
                        }
                    }
                    if                              /* grab/move the note */
                    (
                        center_mouse_handle && SEQ64_CLICK_LEFT(ev->button) &&
                        ! (ev->state & SEQ64_CONTROL_MASK)
                    )
                    {
                        /*
                         * seqroll::align_selection() [proposed]:
                         *
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

                        sroll.snap_x(sroll.m_selected.x);
                        sroll.set_current_drop_x(snapped_x);
                    }
                    else if /* ctrl left click when stuff is already selected */
                    (
                        SEQ64_CLICK_LEFT(ev->button) &&
                        (ev->state & SEQ64_CONTROL_MASK) &&
                        sroll.m_seq.select_note_events
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
                    if /* left click on the right handle = grow/resize event  */
                    (
                        SEQ64_CLICK_MIDDLE(ev->button) ||
                        (
                            right_mouse_handle &&
                            SEQ64_CLICK_LEFT(ev->button) &&
                            ! (ev->state & SEQ64_CONTROL_MASK)
                        )
                    )
                    {
                        /* get the box that selected elements are in */

                        sroll.m_growing = true;
                        sroll.get_selected_box(tick_s, note_h, tick_f, note_l);
                    }
                }
            }
        }

        if (SEQ64_CLICK_RIGHT(ev->button))
        {
            sroll.set_current_drop_x(norm_x);           /* selection normal x */

            /* turn x,y in to tick/note */

            sroll.convert_xy(sroll.m_drop_x, sroll.m_drop_y, tick_s, note_h);
            if              /* erase event(s) under cursor if there is one */
            (
                sroll.m_seq.select_note_events
                (
                    tick_s, note_h, tick_s, note_h, sequence::e_would_select
                )
            )
            {
                /* right ctrl click: remove all selected notes */

                if (ev->state & SEQ64_CONTROL_MASK)
                {
                    sroll.m_seq.select_note_events
                    (
                        tick_s, note_h, tick_s, note_h, sequence::e_select_one
                    );
//                  sroll.m_seq.push_undo();
//                  sroll.m_seq.mark_selected();
//                  sroll.m_seq.remove_marked();
                    sroll.m_seq.remove_selected();  // consolidates the calls
                }
                else
                {
                    /*
                     * right click: remove only the note under the cursor,
                     * leave the selection intact.
                     */

                    // sroll.m_seq.push_undo();         // WHY???
                    sroll.m_seq.select_note_events
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
                if (!(ev->state & SEQ64_CONTROL_MASK))
                    sroll.m_seq.unselect();

                sroll.m_selecting = true;   /* start the new selection box */
                needs_update = true;
            }
        }
    }
    update_mouse_pointer(sroll);    /* context sensitive mouse pointer... */
    if (needs_update)               /* if they clicked, something changed */
        sroll.m_seq.set_dirty();    /* redraw_events();                   */

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

    /* middle click, or ctrl- (???) left click button up */

    if (SEQ64_CLICK_LEFT_MIDDLE(ev->button))
    {
        if (sroll.m_growing)
        {
            /* convert deltas into screen coordinates */

            sroll.convert_xy(delta_x, delta_y, delta_tick, delta_note);
            // sroll.m_seq.push_undo();         // moved into the functions
            if (ev->state & SEQ64_SHIFT_MASK)
                sroll.m_seq.stretch_selected(delta_tick);
            else
                sroll.m_seq.grow_selected(delta_tick);

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
     * -    ctrl-left click button up for select/drag copy/paste
     * -    left click button up for ending a move of selected notes
     */

    if (SEQ64_CLICK_LEFT(ev->button))
    {
        m_adding = false;
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
                ! sroll.m_justselected_one &&
                sroll.m_seq.select_note_events
                (
                    current_tick, current_note, current_tick, current_note,
                    sequence::e_is_selected
                ) &&
                (ev->state & SEQ64_CONTROL_MASK))
            {
                (void) sroll.m_seq.select_note_events
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
            // sroll. m_seq.push_undo();                // moved into the call
            sroll.m_seq.move_selected_notes(delta_tick, delta_note);
            needs_update = true;
        }
    }

    /* right click or left ctrl (???) click button up for selection box */

    if (SEQ64_CLICK_LEFT_RIGHT(ev->button))
    {
        if (sroll.m_selecting)
        {
            // int x, y, w, h;
            sroll.xy_to_rect
            (
                sroll.drop_x(), sroll.drop_y(),
                sroll.current_x(), sroll.current_y(),
                x, y, w, h
            );
            sroll.convert_xy(x, y, tick_s, note_h);
            sroll.convert_xy(x + w, y + h, tick_f, note_l);
            (void) sroll.m_seq.select_note_events
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
    sroll.m_seq.unpaint_all();
    update_mouse_pointer(sroll);        /* context sensitive mouse pointer... */
    if (needs_update)                   /* if they clicked, something changed */
        sroll.m_seq.set_dirty();        /* redraw_events();                   */

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
    sroll.m_current_x = int(ev->x  + sroll.m_scroll_offset_x);
    sroll.m_current_y = int(ev->y  + sroll.m_scroll_offset_y);
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
        sroll.m_seq.copy_selected();
        sroll.m_seq.unselect();
        sroll.start_paste();
        sroll.m_is_drag_pasting_start = false;
        sroll.m_is_drag_pasting = true;
    }

    int note;
    midipulse tick;
    sroll.snap_y(sroll.m_current_y);
    sroll.convert_xy(0, sroll.m_current_y, tick, note);
    sroll.m_seqkeys_wid.set_hint_key(note);
    if (sroll.m_selecting || sroll.m_moving || sroll.m_growing || sroll.m_paste)
    {
        if (sroll.m_moving || sroll.m_paste)
            sroll.snap_x(sroll.m_current_x);

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
            sroll.m_seq.select_note_events
            (
                tick, note, tick, note, sequence::e_would_select
            )
        )
        {
            /* remove only note under the cursor, leave selection intact */

            // sroll.m_seq.push_undo();             // WHY IS THIS NEEDED?
            sroll.m_seq.select_note_events
            (
                tick, note, tick, note, sequence::e_remove_one
            );
            sroll.m_seq.set_dirty();
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

