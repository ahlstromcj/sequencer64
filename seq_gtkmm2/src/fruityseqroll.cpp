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
 * \updates       2015-10-09
 * \license       GNU GPLv2 or above
 *
 *  This module handles "fruity" interactions only in the piano roll
 *  section of the pattern editor.
 */

#include <gdkmm/cursor.h>

#include "click.hpp"                    /* SEQ64_CLICK_LEFT(), etc.    */
#include "event.hpp"
#include "seqroll.hpp"
#include "sequence.hpp"
#include "seqkeys.hpp"

namespace seq64
{

/**
 *  An internal variable for handle size.
 */

const long s_handlesize = 16;

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
 */

void
FruitySeqRollInput::updateMousePtr (seqroll & sroll)
{
    long drop_tick;
    int drop_note;
    sroll.convert_xy
    (
        sroll.current_x(), sroll.current_y(), drop_tick, drop_note
    );
    long start, end, note;
    if
    (
        sroll.m_is_drag_pasting || sroll.m_selecting || sroll.m_moving ||
        sroll.m_growing || sroll.m_paste
    )
    {
        sroll.get_window()->set_cursor(Gdk::Cursor(Gdk::LEFT_PTR));
    }
    else if
    (
         ! m_adding &&
         sroll.m_seq.intersectNotes(drop_tick, drop_note, start, end, note) &&
         note == drop_note
    )
    {
        long handle_size = clamp(s_handlesize, 0, (end - start) / 3);
        if (start <= drop_tick && drop_tick <= start + handle_size)
            sroll.get_window()->set_cursor(Gdk::Cursor(Gdk::CENTER_PTR));
        else if (end - handle_size <= drop_tick && drop_tick <= end)
            sroll.get_window()->set_cursor(Gdk::Cursor(Gdk::LEFT_PTR));
        else
            sroll.get_window()->set_cursor(Gdk::Cursor(Gdk::CENTER_PTR));
    }
    else
        sroll.get_window()->set_cursor(Gdk::Cursor(Gdk::PENCIL));
}

/**
 *  Implements the fruity on-button-press callback.
 */

bool
FruitySeqRollInput::on_button_press_event
(
    GdkEventButton * a_ev, seqroll & sroll
)
{
    long tick_s;
    long tick_f;
    int note_h;
    int note_l;
    int norm_x, norm_y, snapped_x, snapped_y;
    bool needs_update = false;
    sroll.grab_focus();
    snapped_x = norm_x = (int)(a_ev->x + sroll.m_scroll_offset_x);
    snapped_y = norm_y = (int)(a_ev->y + sroll.m_scroll_offset_y);
    sroll.snap_x(snapped_x);
    sroll.snap_y(snapped_y);
    sroll.set_current_drop_y(snapped_y);            /* y is always snapped   */
    sroll.m_old.x = 0;                  /* reset box holds dirty redraw spot */
    sroll.m_old.y = 0;
    sroll.m_old.width = 0;
    sroll.m_old.height = 0;
    if (sroll.m_paste)    /* ctrl-v pressed, waiting for click where to paste */
    {
        sroll.convert_xy(snapped_x, snapped_y, tick_s, note_h);
        sroll.m_paste = false;
        sroll.m_seq.push_undo();
        sroll.m_seq.paste_selected(tick_s, note_h);
        needs_update = true;
    }
    else
    {
        if (SEQ64_CLICK_LEFT(a_ev->button))
        {
            sroll.set_current_drop_x(norm_x);       /* selection normal x   */

            /* turn x,y in to tick/note */

            sroll.convert_xy(sroll.m_drop_x, sroll.m_drop_y, tick_s, note_h);
            if                      /* if not on top of event then add one... */
            (
                m_canadd &&
                ! sroll.m_seq.select_note_events
                (
                    tick_s, note_h, tick_s, note_h, sequence::e_would_select
                ) &&
                ! (a_ev->state & GDK_CONTROL_MASK)
            )
            {
                sroll.m_painting = true;             /* start the paint job */
                m_adding = true;

                /* adding, snapped x */

                sroll.set_current_drop_x(snapped_x);
                sroll.convert_xy
                (
                    sroll.m_drop_x, sroll.m_drop_y, tick_s, note_h
                );

                /*
                 * Test if a note is already there; fake select, if so, no
                 * add.
                 */

                if
                (
                    ! sroll.m_seq.select_note_events
                    (
                        tick_s, note_h, tick_s, note_h, sequence::e_would_select
                    )
                )
                {
                    /* add note, length = little less than snap */

                    sroll.m_seq.push_undo();
                    sroll.m_seq.add_note
                    (
                        tick_s, sroll.m_note_length - 2, note_h, true
                    );
                    needs_update = true;
                }
            }
            else                                            /* selecting */
            {

                if          /* if under the cursor is not a selected note... */
                (
                    ! sroll.m_seq.select_note_events
                    (
                        tick_s, note_h, tick_s, note_h, sequence::e_is_selected
                    )
                )
                {
                    if                              /* if clicking a note ... */
                    (
                        sroll.m_seq.select_note_events
                        (
                            tick_s, note_h, tick_s, note_h,
                            sequence::e_would_select
                        )
                    )
                    {
                        /* ... unselect all if ctrl not held */

                        if (! (a_ev->state & GDK_CONTROL_MASK))
                            sroll.m_seq.unselect();
                    }
                    else                    /* if clicking empty space ... */
                    {
                        if      /* ... unselect all if ctrl-shift not held */
                        (
                            ! ( (a_ev->state & GDK_CONTROL_MASK) &&
                                (a_ev->state & GDK_SHIFT_MASK) )
                        )
                            sroll.m_seq.unselect();
                    }

                    /* on direct click select only one event */

                    int numsel = sroll.m_seq.select_note_events
                    (
                        tick_s, note_h, tick_s, note_h,
                        sequence::e_select_one
                    );

                    /* prevent deselect in button_release() */

                    if (numsel)
                        sroll.m_justselected_one = true;

                    /* if nothing selected, start the selection box */

                    if (numsel == 0 && (a_ev->state & GDK_CONTROL_MASK))
                        sroll.m_selecting = true;

                    needs_update = true;
                }

                /* if note under cursor is selected */

                if
                (
                    sroll.m_seq.select_note_events
                    (
                        tick_s, note_h, tick_s, note_h, sequence::e_is_selected
                    )
                )
                {
                    bool right_mouse_handle = false;
                    bool center_mouse_handle = false;
                    {
                        long drop_tick;
                        int drop_note;
                        sroll.convert_xy
                        (
                            sroll.m_drop_x, sroll.m_drop_y,
                            drop_tick, drop_note
                        );
                        long start, end, note;
                        if
                        (
                            sroll.m_seq.intersectNotes
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
                                end - handle_size <= drop_tick &&
                                drop_tick <= end
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
                        center_mouse_handle &&
                        SEQ64_CLICK_LEFT(a_ev->button) &&
                        ! (a_ev->state & GDK_CONTROL_MASK)
                    )
                    {
                        sroll.m_moving_init = true;
                        needs_update = true;

                        /* get the box that selected elements are in */

                        sroll.m_seq.get_selected_box
                        (
                            tick_s, note_h, tick_f, note_l
                        );
                        sroll.convert_tn_box_to_rect
                        (
                            tick_s, tick_f, note_h, note_l,
                            sroll.m_selected.x, sroll.m_selected.y,
                            sroll.m_selected.width, sroll.m_selected.height
                        );

                        /* save offset that we get from the snap above */

                        int adjusted_selected_x = sroll.m_selected.x;
                        sroll.snap_x(adjusted_selected_x);
                        sroll.m_move_snap_offset_x =
                            sroll.m_selected.x - adjusted_selected_x;

                        /* align selection for drawing */

                        sroll.snap_x(sroll.m_selected.x);
                        sroll.set_current_drop_x(snapped_x);
                    }
                    else if /* ctrl left click when stuff is already selected */
                    (
                        SEQ64_CLICK_LEFT(a_ev->button) &&
                        (a_ev->state & GDK_CONTROL_MASK) &&
                        sroll.m_seq.select_note_events
                        (
                            tick_s, note_h, tick_s, note_h,
                            sequence::e_is_selected
                        )
                    )
                    {
                        sroll.m_is_drag_pasting_start = true;
                        m_drag_paste_start_pos[0] = long(a_ev->x);
                        m_drag_paste_start_pos[1] = long(a_ev->y);
                    }
                    if /* left click on the right handle = grow/resize event  */
                    (
                        SEQ64_CLICK_MIDDLE(a_ev->button) ||
                        (
                            right_mouse_handle &&
                            SEQ64_CLICK_LEFT(a_ev->button) &&
                            ! (a_ev->state & GDK_CONTROL_MASK)
                        )
                    )
                    {
                        /* get the box that selected elements are in */

                        sroll.m_growing = true;
                        sroll.m_seq.get_selected_box
                        (
                            tick_s, note_h, tick_f, note_l
                        );
                        sroll.convert_tn_box_to_rect
                        (
                            tick_s, tick_f, note_h, note_l,
                            sroll.m_selected.x, sroll.m_selected.y,
                            sroll.m_selected.width, sroll.m_selected.height
                        );
                    }
                }
            }
        }

        if (SEQ64_CLICK_RIGHT(a_ev->button))
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

                if (a_ev->state & GDK_CONTROL_MASK)
                {
                    sroll.m_seq.select_note_events
                    (
                        tick_s, note_h, tick_s, note_h, sequence::e_select_one
                    );
                    sroll.m_seq.push_undo();
                    sroll.m_seq.mark_selected();
                    sroll.m_seq.remove_marked();
                }
                else
                {
                    /*
                     * right click: remove only the note under the cursor,
                     * leave the selection intact.
                     */

                    sroll.m_seq.push_undo();
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
                if (!(a_ev->state & GDK_CONTROL_MASK))
                    sroll.m_seq.unselect();

                sroll.m_selecting = true;   /* start the new selection box */
                needs_update = true;
            }
        }
    }

    updateMousePtr(sroll);          /* context sensative mouse pointer... */

    if (needs_update)               /* if they clicked, something changed */
    {
        sroll.m_seq.set_dirty(); // redraw_events();
    }
    return true;
}

/**
 *  Implements the fruity handling for the on-button-release event.
 */

bool
FruitySeqRollInput::on_button_release_event
(
    GdkEventButton * a_ev, seqroll & sroll
)
{
    long tick_s;
    long tick_f;
    int note_h;
    int note_l;
    int x, y, w, h;
    bool needs_update = false;
    sroll.m_current_x = int(a_ev->x + sroll.m_scroll_offset_x);
    sroll.m_current_y = int(a_ev->y + sroll.m_scroll_offset_y);
    sroll.snap_y(sroll.m_current_y);
    if (sroll.m_moving || sroll.m_is_drag_pasting)
        sroll.snap_x(sroll.m_current_x);

    int delta_x = sroll.current_x() - sroll.drop_x();
    int delta_y = sroll.current_y() - sroll.drop_y();
    long delta_tick;
    int delta_note;

    /* middle click, or ctrl- (???) left click button up */

    if (SEQ64_CLICK_LEFT_MIDDLE(a_ev->button))
    {
        if (sroll.m_growing)
        {
            /* convert deltas into screen coordinates */

            sroll.convert_xy(delta_x, delta_y, delta_tick, delta_note);
            sroll.m_seq.push_undo();
            if (a_ev->state & GDK_SHIFT_MASK)
                sroll.m_seq.stretch_selected(delta_tick);
            else
                sroll.m_seq.grow_selected(delta_tick);

            needs_update = true;
        }
    }

    long int current_tick;
    int current_note;
    sroll.convert_xy
    (
        sroll.current_x(), sroll.current_y(), current_tick, current_note
    );

    /*
     * -    ctrl-left click button up for select/drag copy/paste
     * -    left click button up for ending a move of selected notes
     */

    if (SEQ64_CLICK_LEFT(a_ev->button))
    {
        m_adding = false;
        if (sroll.m_is_drag_pasting)
        {
            sroll.m_is_drag_pasting = false;
            sroll.m_is_drag_pasting_start = false;

            /* convert deltas into screen coordinates */

            sroll.m_paste = false;
            sroll.m_seq.push_undo();
            sroll.m_seq.paste_selected(current_tick, current_note);
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
                (a_ev->state & GDK_CONTROL_MASK))
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
            delta_x -= sroll.m_move_snap_offset_x;      /* adjust for snap */

            /* convert deltas into screen coordinates */

            sroll.convert_xy(delta_x, delta_y, delta_tick, delta_note);

            /*
             * Since delta_note was from delta_y, it will be flipped
             * ( delta_y[0] = note[127], etc.,so we have to adjust.
             */

            delta_note = delta_note - (c_num_keys - 1);
            sroll.m_seq.push_undo();
            sroll.m_seq.move_selected_notes(delta_tick, delta_note);
            needs_update = true;
        }
    }

    /* right click or left ctrl (???) click button up for selection box */

    if (SEQ64_CLICK_LEFT_RIGHT(a_ev->button))
    {
        if (sroll.m_selecting)
        {
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
    if (SEQ64_CLICK_RIGHT(a_ev->button))
        m_erase_painting = false;

    sroll.m_selecting = false;          /* turn it all off */
    sroll.m_moving = false;
    sroll.m_growing = false;
    sroll.m_paste = false;
    sroll.m_moving_init = false;
    sroll.m_painting = false;
    sroll.m_seq.unpaint_all();
    updateMousePtr(sroll);              /* context sensitive mouse pointer... */
    if (needs_update)                   /* if they clicked, something changed */
        sroll.m_seq.set_dirty();        /* redraw_events();                   */

    return true;
}

/**
 *  Implements the fruity handling for the on-motion-notify event.
 */

bool
FruitySeqRollInput::on_motion_notify_event
(
    GdkEventMotion * a_ev, seqroll & sroll
)
{
    sroll.m_current_x = int(a_ev->x  + sroll.m_scroll_offset_x);
    sroll.m_current_y = int(a_ev->y  + sroll.m_scroll_offset_y);
    if (sroll.m_moving_init)
    {
        sroll.m_moving_init = false;
        sroll.m_moving = true;
    }
    updateMousePtr(sroll);              /* context sensitive mouse pointer... */

    /*
     * Ctrl-left click drag on selected note(s) starts a copy/unselect/paste.
     * Don't begin the paste until mouse moves a few pixels, filter out
     * the unsteady hand.
     */

    if
    (
        sroll.m_is_drag_pasting_start &&
        (
            6 <= abs(m_drag_paste_start_pos[0] - (long) a_ev->x) ||
            6 <= abs(m_drag_paste_start_pos[1] - (long) a_ev->y)
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
    long tick;
    sroll.snap_y(sroll.m_current_y);
    sroll.convert_xy(0, sroll.m_current_y, tick, note);
    sroll.m_seqkeys_wid.set_hint_key(note);
    if (sroll.m_selecting || sroll.m_moving || sroll.m_growing || sroll.m_paste)
    {
        if (sroll.m_moving || sroll.m_paste)
            sroll.snap_x(sroll.m_current_x);

        sroll.draw_selection_on_window();
        return true;
    }
    if (sroll.m_painting)
    {
        sroll.snap_x(sroll.m_current_x);
        sroll.convert_xy(sroll.current_x(), sroll.current_y(), tick, note);
        sroll.m_seq.add_note(tick, sroll.m_note_length - 2, note, true);
        return true;
    }
    if (m_erase_painting)
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

            sroll.m_seq.push_undo();
            sroll.m_seq.select_note_events
            (
                tick, note, tick, note, sequence::e_remove_one
            );
            sroll.m_seq.set_dirty();
        }
    }
    return false;
}

}           // namespace seq64

/*
 * fruityseqroll.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
