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
 * \updates       2016-06-30
 * \license       GNU GPLv2 or above
 *
 */

#include <gdkmm/cursor.h>

#include "click.hpp"                    /* SEQ64_CLICK_LEFT(), etc.     */
#include "event.hpp"
#include "seqroll.hpp"
#include "seqdata.hpp"
#include "seqevent.hpp"
#include "sequence.hpp"
#include "seqkeys.hpp"
#include "perform.hpp"
#include "settings.hpp"                 /* seq64::rc() or seq64::usr()  */

namespace seq64
{

/**
 *  Changes the mouse cursor pixmap according to whether a note is being
 *  added or not.  What calls this?  It is actually a right click.
 *  Not present in the "fruity" implementation.
 *
 * \param adding
 *      True if adding a note.  Sets m_adding.
 *
 * \param sroll
 *      Provides the "parent" object to which to forward the set_cursor()
 *      calls.
 */

void
Seq24SeqRollInput::set_adding (bool adding, seqroll & sroll)
{
//  m_adding = adding;
    sroll.set_adding(adding);
}

/**
 *  Implements the on-button-press event handling for the Seq24 style of
 *  mouse interaction.
 *
 *  This function now uses the needs_update flag to determine if the perform
 *  object should modify().
 *
 * \param ev
 *      Provides the button-press event to process.
 *
 * \param sroll
 *      Provides the "parent" seqroll object for this class.
 *
 * \return
 *      Returns the value of needs_update.  It used to return only true.
 */

bool
Seq24SeqRollInput::on_button_press_event (GdkEventButton * ev, seqroll & sroll)
{
    midipulse tick_s, tick_f;
    int note_h, note_l;
    sequence & seq = sroll.m_seq;
#ifdef USE_NEW_FUNCTIONS
    int norm_x, snapped_x, snapped_y;
    bool needs_update = sroll.button_press_initial
    (
        ev, norm_x, snapped_x, snapped_y                /* 3 side-effects   */
    );
    if (! needs_update)
#else
    bool needs_update = false;
    int snapped_x = sroll.scroll_offset_x(int(ev->x));
    int snapped_y = sroll.scroll_offset_y(int(ev->y));
    int norm_x = snapped_x;                             /* needed later     */
    sroll.grab_focus();
    sroll.snap_x(snapped_x);
    sroll.snap_y(snapped_y);
    sroll.set_current_drop_y(snapped_y);                /* y always snapped */
    sroll.m_old.x = sroll.m_old.y = sroll.m_old.width = sroll.m_old.height = 0;
    if (sroll.m_paste)
    {
        sroll.complete_paste(snapped_x, snapped_y);
        needs_update = true;
    }
    else
#endif
    {
        if (SEQ64_CLICK_LEFT_MIDDLE(ev->button))        /* either button    */
        {
            sroll.set_current_drop_x(norm_x);           /* select normal x  */
            sroll.convert_xy(sroll.m_drop_x, sroll.m_drop_y, tick_s, note_h);
//          if (m_adding)
            if (sroll.adding())
            {
                sroll.m_painting = true;                /* start paint job  */
                sroll.set_current_drop_x(snapped_x);    /* used snapped x   */
                sroll.convert_xy(sroll.m_drop_x, sroll.m_drop_y, tick_s, note_h);

                int eventcount = seq.select_note_events
                (
                    tick_s, note_h, tick_s, note_h, sequence::e_would_select
                );
                if (eventcount == 0)                    /* no note? add one */
                {
                    /*
                     * Do we need push_undo() here?
                     */

                    sroll.add_note(tick_s, note_h);     /* also does chords */
                    needs_update = true;
                }
            }
            else                                        /* selecting        */
            {
                int eventcount = seq.select_note_events
                (
                    tick_s, note_h, tick_s, note_h, sequence::e_is_selected
                );
                if (eventcount == 0)
                {
                    if (! (ev->state & SEQ64_CONTROL_MASK))
                        seq.unselect();

                    eventcount = seq.select_note_events /* direct click     */
                    (
                        tick_s, note_h, tick_s, note_h, sequence::e_select_one
                    );
                    if (eventcount == 0)                /* none             */
                    {
                        if (SEQ64_CLICK_LEFT(ev->button))
                            sroll.m_selecting = true;   /* start selection  */
                    }
                    else
                        needs_update = true;
                }
                eventcount = seq.select_note_events
                (
                    tick_s, note_h, tick_s, note_h, sequence::e_is_selected
                );
                if (eventcount > 0)
                {
                    /*
                     * Moving and selecting, left-click (without Ctrl key)
                     * only.  Get the box that selected elements are in.
                     */

                    if
                    (
                        SEQ64_CLICK_LEFT(ev->button) &&
                        ! (ev->state & SEQ64_CONTROL_MASK)
                    )
                    {
#ifdef USE_NEW_FUNCTIONS
                        sroll.align_selection
                        (
                            tick_s, note_h, tick_f, note_l, snapped_x
                        );
#else
                        sroll.m_moving_init = true;
                        sroll.get_selected_box(tick_s, note_h, tick_f, note_l);

                        int adjusted_selected_x = sroll.m_selected.x;
                        sroll.snap_x(adjusted_selected_x);
                        sroll.m_move_snap_offset_x =
                            sroll.m_selected.x - adjusted_selected_x;

                        sroll.snap_x(sroll.m_selected.x);
                        sroll.set_current_drop_x(snapped_x);
#endif

                        needs_update = true;
                    }

                    /*
                     * Middle mouse button, or left-ctrl-click (2-button mice)
                     */

                    if (SEQ64_CLICK_CTRL_LEFT_MIDDLE(ev->button, ev->state))
                    {
                        sroll.m_growing = true;         /* moving, normal x  */
                        sroll.get_selected_box(tick_s, note_h, tick_f, note_l);
                    }
                }
            }
        }
        if (SEQ64_CLICK_RIGHT(ev->button))
            set_adding(true, sroll);
    }
    if (needs_update)               /* if they clicked, something changed */
        seq.set_dirty();            /* redraw_events();                   */

    return needs_update;
}

/**
 *  Implements the on-button-release event handling for the Seq24 style of
 *  mouse interaction.  This function now uses the needs_update flag to
 *  determine if the perform object should modify().
 *
 * \param ev
 *      Provides the button-release event to process.
 *
 * \param sroll
 *      Provides the "parent" seqroll object for this class.
 *
 * \return
 *      Returns the value of needs_update.  It used to return only true.
 */

bool
Seq24SeqRollInput::on_button_release_event
(
    GdkEventButton * ev,
    seqroll & sroll
)
{
    midipulse tick_s;
    midipulse tick_f;
    int note_h;
    int note_l;
    bool needs_update = false;
    sroll.m_current_x = int(ev->x + sroll.m_scroll_offset_x);
    sroll.m_current_y = int(ev->y + sroll.m_scroll_offset_y);
    sroll.snap_y(sroll.m_current_y);
    if (sroll.m_moving)
        sroll.snap_x(sroll.m_current_x);

    int delta_x = sroll.m_current_x - sroll.m_drop_x;
    int delta_y = sroll.m_current_y - sroll.m_drop_y;
    midipulse delta_tick;
    int delta_note;
    sequence & seq = sroll.m_seq;                   /* just do this once!   */
    if (SEQ64_CLICK_LEFT(ev->button))
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
            (void) seq.select_note_events
            (
                tick_s, note_h, tick_f, note_l, sequence::e_select
            );
            needs_update = true;
        }
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
            seq.move_selected_notes(delta_tick, delta_note);
            needs_update = true;
        }
    }
    if (SEQ64_CLICK_LEFT_MIDDLE(ev->button))
    {
        if (sroll.m_growing)
        {
            /**
             * A left/middle click converts deltas into screen coordinates,
             * then pushs the undo state.  Shift causes a "stretch selected"
             * which currently acts like a "move selected" operation.
             * Otherwise, Ctrl indirectly allows a "grow selected" operation.
             */

            sroll.convert_xy(delta_x, delta_y, delta_tick, delta_note);
            if (ev->state & SEQ64_SHIFT_MASK)
                seq.stretch_selected(delta_tick);
            else
                seq.grow_selected(delta_tick);

            needs_update = true;
        }
    }
    if (SEQ64_CLICK_RIGHT(ev->button))
    {
        /**
         * Minor new feature.  If the Super (Mod4, Windows) key is
         * pressed when release, keep the adding state in force.  One
         * can then use the unadorned left-click key to add notes.  Right
         * click to reset the adding mode.  This feature is enabled only
         * if allowed by the settings (but is true by default).
         * See the same code in perfrollinput.cpp.
         */

        bool addmode_exit = ! rc().allow_mod4_mode();
        if (! addmode_exit)
            addmode_exit = ! (ev->state & SEQ64_MOD4_MASK); /* Mod4 held? */

        if (addmode_exit)
            set_adding(false, sroll);
    }
    sroll.m_selecting = false;      /* turn it all off                      */
    sroll.m_moving = false;
    sroll.m_growing = false;
    sroll.m_paste = false;
    sroll.m_moving_init = false;
    sroll.m_painting = false;
    seq.unpaint_all();
    if (needs_update)               /* if they clicked, something changed   */
        seq.set_dirty();            /* redraw_events();                     */

    return needs_update;
}

/**
 *      Seq24-style on-motion mouse interaction.
 *
 * \param ev
 *      Provides the button-release event to process.
 *
 * \param sroll
 *      Provides the "parent" seqroll object for this class.
 *
 * \return
 *      Returns true if the event was processed.
 */

bool
Seq24SeqRollInput::on_motion_notify_event
(
    GdkEventMotion * ev,
    seqroll & sroll
)
{
    bool result = false;
    sroll.set_current_offset_x_y(int(ev->x), int(ev->y));
//  sroll.set_current_offset_y(int(ev->y));
    if (sroll.m_moving_init)
    {
        sroll.m_moving_init = false;
        sroll.m_moving = true;
    }
//  update_mouse_pointer(sroll);    /* context sensitive mouse pointer... */

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
        if (sroll.drop_action())
            sroll.snap_x(sroll.m_current_x);

        sroll.draw_selection_on_window();
        result = true;
    }
    else if (sroll.m_painting)
    {

#ifdef SEQ64_STAZED_CHORD_GENERATOR

        /*
         * We could allow move painting for chords, but that would take
         * some tricky code to move all of the notes of the chord.  And
         * allowing painting here currently affects only one note after the
         * chord itself is created.
         */

        if (sroll.m_chord != 0)     /* chord, don't allow move painting */
            result = true;
        else
#endif
        {
            /*
             * sroll.add_snapped_note() a potential replacement.
             */

            sroll.snap_x(sroll.m_current_x);
            sroll.convert_xy(sroll.m_current_x, sroll.m_current_y, tick, note);
            sroll.add_note(tick, note);
            result = true;
        }
    }
    return result;
}

}           // namespace seq64

/*
 * seq24seqroll.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

