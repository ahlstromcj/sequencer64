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
 * \file          seq24seqroll.cpp
 *
 *  This module declares/defines the base class for handling the Seq24
 *  mode of mouse interaction in the piano roll of the pattern/sequence
 *  editor.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2015-10-03
 * \license       GNU GPLv2 or above
 *
 */

#include <gdkmm/cursor.h>

#include "click.hpp"                    /* CLICK_IS_LEFT(), etc.    */
#include "event.hpp"
#include "seqroll.hpp"
#include "seqdata.hpp"
#include "seqevent.hpp"
#include "sequence.hpp"
#include "seqkeys.hpp"
#include "perform.hpp"

namespace seq64
{

/**
 *  Changes the mouse cursor pixmap according to whether a note is being
 *  added or not.
 *
 *  (Which?) popup menu calls this.  It is actually a right click, I
 *  think.
 */

void
Seq24SeqRollInput::set_adding (bool a_adding, seqroll & sroll)
{
    if (a_adding)
    {
        sroll.get_window()->set_cursor(Gdk::Cursor(Gdk::PENCIL));
        m_adding = true;
    }
    else
    {
        sroll.get_window()->set_cursor(Gdk::Cursor(Gdk::LEFT_PTR));
        m_adding = false;
    }
}

/**
 *  Implements the on-button-press event handling for the Seq24 style of
 *  mouse interaction.
 */

bool
Seq24SeqRollInput::on_button_press_event (GdkEventButton * a_ev, seqroll & sroll)
{
    long tick_s, tick_f;
    int note_h, note_l;
    int norm_x, norm_y, snapped_x, snapped_y;
    sroll.grab_focus();
    snapped_x = norm_x = int(a_ev->x + sroll.m_scroll_offset_x);
    snapped_y = norm_y = int(a_ev->y + sroll.m_scroll_offset_y);
    sroll.snap_x(snapped_x);
    sroll.snap_y(snapped_y);
    sroll.set_current_drop_y(snapped_y);    /* y is always snapped */
    sroll.m_old.x = 0;                      /* reset box for dirty redraw spot */
    sroll.m_old.y = 0;
    sroll.m_old.width = 0;
    sroll.m_old.height = 0;

    bool needs_update = false;
    if (sroll.m_paste)
    {
        sroll.convert_xy(snapped_x, snapped_y, tick_s, note_h);
        sroll.m_paste = false;
        sroll.m_seq.push_undo();
        sroll.m_seq.paste_selected(tick_s, note_h);
        needs_update = true;
    }
    else
    {
        int numsel;
        if (CLICK_IS_LEFT(a_ev->button) || CLICK_IS_MIDDLE(a_ev->button))
        {
            /*
             * Set the selection for normal x, then turn x,y in to tick,note.
             */

            sroll.set_current_drop_x(norm_x);
            sroll.convert_xy(sroll.m_drop_x, sroll.m_drop_y, tick_s, note_h);
            if (m_adding)
            {
                /*
                 * Start paint job.  If snapped, add the snapped x value.
                 */

                sroll.m_painting = true;
                sroll.set_current_drop_x(snapped_x);
                sroll.convert_xy
                (
                    sroll.m_drop_x, sroll.m_drop_y, tick_s, note_h
                );
                if      /* if note already there, fake a select, do not add */
                (
                    ! sroll.m_seq.select_note_events
                    (
                        tick_s, note_h, tick_s, note_h, sequence::e_would_select
                    )
                )
                {
                    sroll.m_seq.push_undo();
                    sroll.m_seq.add_note  /* length = little less than snap */
                    (
                        tick_s, sroll.m_note_length - 2, note_h, true
                    );
                    needs_update = true;
                }
            }
            else                                        /* selecting */
            {
                if
                (
                    ! sroll.m_seq.select_note_events
                    (
                        tick_s, note_h, tick_s, note_h, sequence::e_is_selected
                    )
                )
                {
                    if (! (a_ev->state & GDK_CONTROL_MASK))
                        sroll.m_seq.unselect();

                    numsel = sroll.m_seq.select_note_events
                    (
                        tick_s, note_h, tick_s, note_h,
                        sequence::e_select_one  /* direct click, one event */
                    );
                    if (numsel == 0) /* none selected, start selection box */
                    {
                        if (a_ev->button == 1)
                            sroll.m_selecting = true;
                    }
                    else
                        needs_update = true;
                }
                if
                (
                    sroll.m_seq.select_note_events
                    (
                        tick_s, note_h, tick_s, note_h, sequence::e_is_selected
                    )
                )
                {
                    /*
                     * Moving and selectiong, left-click (without Ctrl
                     * key) only.  Get the box that selected elements are
                     * in.
                     */

                    if
                    (
                        CLICK_IS_LEFT(a_ev->button) &&
                        ! (a_ev->state & GDK_CONTROL_MASK)
                    )
                    {
                        sroll.m_moving_init = true;
                        needs_update = true;
                        sroll.m_seq.get_selected_box
                        (
                            &tick_s, &note_h, &tick_f, &note_l
                        );
                        sroll.convert_tn_box_to_rect
                        (
                            tick_s, tick_f, note_h, note_l,
                            sroll.m_selected.x,
                            sroll.m_selected.y,
                            sroll.m_selected.width,
                            sroll.m_selected.height
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

                    /*
                     * Middle mouse button, or left-ctrl-click (2-button mice)
                     */

                    if
                    (
                        CLICK_IS_MIDDLE(a_ev->button) ||
                        (
                            CLICK_IS_LEFT(a_ev->button) &&
                            (a_ev->state & GDK_CONTROL_MASK)
                        )
                    )
                    {
                        sroll.m_growing = true;         /* moving, normal x */
                        sroll.m_seq.get_selected_box   /* selected elements */
                        (
                            &tick_s, &note_h, &tick_f, &note_l
                        );
                        sroll.convert_tn_box_to_rect
                        (
                            tick_s, tick_f, note_h, note_l,
                            sroll.m_selected.x,
                            sroll.m_selected.y,
                            sroll.m_selected.width,
                            sroll.m_selected.height
                        );
                    }
                }
            }
        }
        if (CLICK_IS_RIGHT(a_ev->button))
            set_adding(true, sroll);
    }
    if (needs_update)                   /* if they clicked, something changed */
        sroll.m_seq.set_dirty();       /* redraw_events();                   */

    return true;
}

/**
 *  Implements the on-button-release event handling for the Seq24 style of
 *  mouse interaction.
 */

bool
Seq24SeqRollInput::on_button_release_event
(
    GdkEventButton * a_ev, seqroll & sroll
)
{
    long tick_s;
    long tick_f;
    int note_h;
    int note_l;
    bool needs_update = false;
    sroll.m_current_x = int(a_ev->x + sroll.m_scroll_offset_x);
    sroll.m_current_y = int(a_ev->y + sroll.m_scroll_offset_y);
    sroll.snap_y(sroll.m_current_y);
    if (sroll.m_moving)
        sroll.snap_x(sroll.m_current_x);

    int delta_x = sroll.m_current_x - sroll.m_drop_x;
    int delta_y = sroll.m_current_y - sroll.m_drop_y;
    long delta_tick;
    int delta_note;
    if (CLICK_IS_LEFT(a_ev->button))
    {
        if (sroll.m_selecting)
        {
            int x, y, w, h;
            sroll.xy_to_rect
            (
                sroll.m_drop_x, sroll.m_drop_y,
                sroll.m_current_x, sroll.m_current_y,
                x, y, w, h
            );
            sroll.convert_xy(x, y, tick_s, note_h);
            sroll.convert_xy(x + w, y + h, tick_f, note_l);
            (void) sroll.m_seq.select_note_events
            (
                tick_s, note_h, tick_f, note_l, sequence::e_select
            );
            needs_update = true;
        }
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
    if (CLICK_IS_MIDDLE(a_ev->button) || CLICK_IS_LEFT(a_ev->button))
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
    if (CLICK_IS_RIGHT(a_ev->button))
    {
        /*
         * Minor new feature.  If the Super (Mod4, Windows) key is
         * pressed when release, keep the adding state in force.  One
         * can then use the unadorned left-click key to add notes.  Right
         * click to reset the adding mode.  This feature is enabled only
         * if allowed by the settings (but is true by default).
         * See the same code in perfrollinput.cpp.
         */

        bool addmode_exit  = ! global_allow_mod4_mode;
        if (! addmode_exit)
            addmode_exit = ! (a_ev->state & GDK_MOD4_MASK); // Mod4 held?

        if (addmode_exit)
            set_adding(false, sroll);
    }
    sroll.m_selecting = false;      /* turn it all off */
    sroll.m_moving = false;
    sroll.m_growing = false;
    sroll.m_paste = false;
    sroll.m_moving_init = false;
    sroll.m_painting = false;
    sroll.m_seq.unpaint_all();
    if (needs_update)               /* if they clicked, something changed */
        sroll.m_seq.set_dirty();    /* redraw_events();                   */

    return true;
}

/**
 *  Implements the on-motion-notify event handling for the Seq24 style of
 *  mouse interaction.
 */

bool Seq24SeqRollInput::on_motion_notify_event
(
    GdkEventMotion * a_ev, seqroll & sroll
)
{
    sroll.m_current_x = int(a_ev->x + sroll.m_scroll_offset_x);
    sroll.m_current_y = int(a_ev->y + sroll.m_scroll_offset_y);
    if (sroll.m_moving_init)
    {
        sroll.m_moving_init = false;
        sroll.m_moving = true;
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
        sroll.convert_xy(sroll.m_current_x, sroll.m_current_y, tick, note);
        sroll.m_seq.add_note(tick, sroll.m_note_length - 2, note, true);
        return true;
    }
    return false;
}

}           // namespace seq64

/*
 * seq24seqroll.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
