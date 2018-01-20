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
 * \updates       2018-01-20
 * \license       GNU GPLv2 or above
 *
 *  This module handles "fruity" interactions only in the piano roll
 *  section of the pattern editor.
 */

#include <gdkmm/cursor.h>

#include "click.hpp"                    /* SEQ64_CLICK_LEFT(), etc.     */
#include "event.hpp"                    /* seq64::event class           */
#include "fruityseqroll.hpp"            /* seq64::FruitySeqRollInput    */
#include "gui_key_tests.hpp"            /* seq64::is_no_modifier() etc. */
#include "perform.hpp"                  /* seq64::perform class         */
#include "seqroll.hpp"                  /* seq64::seqroll class         */
#include "sequence.hpp"                 /* seq64::sequence class        */
#include "seqkeys.hpp"                  /* seq64::seqkeys class         */

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
 * Default constructor.
 */

FruitySeqRollInput::FruitySeqRollInput
(
    perform & p,
    sequence & seq,
    int zoom,
    int snap,
    seqkeys & seqkeys_wid,
    int pos,
    Gtk::Adjustment & hadjust,
    Gtk::Adjustment & vadjust
) :
    seqroll
    (
        p, seq, zoom, snap, seqkeys_wid, pos, hadjust, vadjust
    ),
    m_can_add               (true),         /* seq24's m_canadd */
    m_erase_painting        (false),
    m_drag_paste_start_pos  ()              /* an array         */
{
    // no additional code
}

/**
 *  Updates the mouse pointer, implementing a context-sensitive mouse.
 *  This code is very similar to the base class version.
 */

void
FruitySeqRollInput::update_mouse_pointer (bool isadding)
{
    ///// seqroll::update_mouse_pointer(adding());

    midipulse droptick;
    int dropnote;
    convert_xy(current_x(), current_y(), droptick, dropnote);

    midipulse s, f;                     // start, end;
    int note;                           // midibyte
    bool intersect = m_seq.intersect_notes(droptick, dropnote, s, f, note);
    if (normal_action())
        get_window()->set_cursor(Gdk::Cursor(Gdk::LEFT_PTR));
    else if (! isadding && intersect && (note == dropnote))
    {
        long hsize = m_seq.handle_size(s, f);
        if (s <= droptick && droptick <= s + hsize)
            get_window()->set_cursor(Gdk::Cursor(Gdk::RIGHT_PTR)); // TRIAL!
            // get_window()->set_cursor(Gdk::Cursor(Gdk::CENTER_PTR));
        else if (f - hsize <= droptick && droptick <= f)
            get_window()->set_cursor(Gdk::Cursor(Gdk::LEFT_PTR));
        else
            get_window()->set_cursor(Gdk::Cursor(Gdk::CENTER_PTR));
    }
    else
        get_window()->set_cursor(Gdk::Cursor(Gdk::PENCIL));


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
 * \return
 *      Returns the value of needs_update.  It used to return only true.
 */

bool
FruitySeqRollInput::on_button_press_event (GdkEventButton * ev)
{
    midipulse tick_s, tick_f;
    int note_h, note_l;
    sequence & seq = m_seq;                         /* just do this once!   */
    int norm_x, snapped_x, snapped_y;
    bool needs_update = button_press_initial        /* focus grab and more! */
    (
        ev, norm_x, snapped_x, snapped_y            /* 3 side-effects       */
    );
    if (! needs_update)
    {
        int norm_x = snapped_x;
        if (SEQ64_CLICK_LEFT(ev->button))
        {
            set_current_drop_x(norm_x);             /* selection normal x   */
            convert_xy(m_drop_x, m_drop_y, tick_s, note_h);
            int eventcount = seq.select_note_events
            (
                tick_s, note_h, tick_s, note_h, sequence::e_would_select
            );

            if (m_can_add && (eventcount == 0) && ! is_ctrl_key(ev))
            {
                set_adding(true);              /* not on top of event */
                m_painting = true;             /* start the paint job */
                set_current_drop_x(snapped_x); /* adding, snapped x   */
                convert_xy(m_drop_x, m_drop_y, tick_s, note_h);

                /*
                 * Stazed fix, forwards the event to play the hint note.
                 */

                m_seqkeys_wid.set_listen_button_press(ev);
                eventcount = seq.select_note_events /* note already there?  */
                (
                    tick_s, note_h, tick_s, note_h, sequence::e_would_select
                );
                if (eventcount == 0)
                {
                    // TODO:  make sure sequence::push_undo() is called

                    add_note(tick_s, note_h); /* also does chords     */
                    needs_update = true;
                }
            }
            else                                            /* selecting    */
            {
                eventcount = seq.select_note_events
                (
                    tick_s, note_h, tick_s, note_h, sequence::e_is_selected
                );
                if (eventcount == 0)    /* no selected note under cursor    */
                {
                    eventcount = seq.select_note_events
                    (
                        tick_s, note_h, tick_s, note_h, sequence::e_would_select
                    );
                    if (eventcount > 0)
                    {
                        /*
                         * If clicking a note, unselect all if Ctrl not held.
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

                    /*
                     * If events selected, stop deselect in button_release(),
                     * otherwise start the selection box if Ctrl active.
                     */

                    if (eventcount > 0)
                        m_justselected_one = true;
                    else if (is_ctrl_key(ev))
                        m_selecting = true;

                    needs_update = true;
                }

                /*
                 * Check if note under cursor is selected.
                 */

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
                        convert_drop_xy(tick, note);

                        midipulse s, f;                 // start & finish ticks
                        int n;                          // note number
                        bool found = seq.intersect_notes(tick, note, s, f, n);
                        if (found && n == note)
                        {
                            midipulse hsize = seq.handle_size(s, f);
                            if (tick >= (f - hsize) && tick <= f)
                                right_handle = true;
                            else if (tick >= s && tick <= (s + hsize))
                                left_handle = true;
                            else
                                center_handle = true;
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
                        m_moving_init = true;
                        get_selected_box(tick_s, note_h, tick_f, note_l);

                        int adjusted_selected_x = m_selected.x();
                        snap_x(adjusted_selected_x);
                        m_move_snap_offset_x =
                            m_selected.x() - adjusted_selected_x;

                        /*
                         * Slightly clumsy.
                         */

                        int sx = m_selected.x();
                        snap_x(sx);                   /* align to draw */
                        m_selected.x(sx);
                        set_current_drop_x(snapped_x);

                        /*
                         * Stazed fix, forward the event to play hint note.
                         */

                        m_seqkeys_wid.set_listen_button_press(ev);
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
                        m_is_drag_pasting_start = true;
                        m_drag_paste_start_pos[0] = int(ev->x);
                        m_drag_paste_start_pos[1] = int(ev->y);
                    }

                    /*
                     * left click on the right handle = grow/resize event.
                     * seq24 also allows the middle-click w/out ctrl.
                     */

                    if ((left_handle || right_handle) && ! is_ctrl_key(ev))
                    {
                        /* get the box that selected elements are in */

                        m_growing = true;
                        get_selected_box(tick_s, note_h, tick_f, note_l);
                    }
                }
            }
        }
        if (SEQ64_CLICK_MIDDLE(ev->button))
        {
            if (! is_ctrl_key(ev))
            {
                /* Get the box that selected elements are in. */

                m_growing = true;
                get_selected_box(tick_s, note_h, tick_f, note_l);
            }
        }
        if (SEQ64_CLICK_RIGHT(ev->button))
        {
            set_current_drop_x(norm_x);           /* selection normal x */

            /* turn x,y in to tick/note */

            convert_xy(m_drop_x, m_drop_y, tick_s, note_h);
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

                m_selecting = true;   /* start the new selection box */
                needs_update = true;
            }
        }
    }
    update_mouse_pointer(adding());     /* context sensitive mouse pointer... */
    if (needs_update)                   /* if they clicked, something changed */
        seq.set_dirty();                /* redraw_events();                   */

    /*
     * \change ca 2018-01-20
     *      Why do we do this unconditionally?  Do it only if nothing above
     *      was acted on.
     */

    if (! needs_update)
        (void) seqroll::on_button_press_event(ev);

    return needs_update;
}

/**
 *  Implements the fruity handling for the on-button-release event.
 *
 * \param ev
 *      The button event.
 *
 * \return
 *      Returns the value of needs_update.  It used to return only true.
 */

bool
FruitySeqRollInput::on_button_release_event (GdkEventButton * ev)
{
    midipulse tick_s;
    midipulse tick_f;
    int note_h;
    int note_l;
    int x, y, w, h;
    sequence & seq = m_seq;                   /* just do this once!   */
    bool needs_update = false;
    m_current_x = int(ev->x + m_scroll_offset_x);
    m_current_y = int(ev->y + m_scroll_offset_y);
    snap_y(m_current_y);
    if (m_moving || m_is_drag_pasting)
        snap_x(m_current_x);

    int delta_x = current_x() - drop_x();
    int delta_y = current_y() - drop_y();
    midipulse delta_tick;
    int delta_note;

    /*
     * Stazed fix, forwards the event to turn off the hint note.  We had this
     * wrong, which lead to issue #105.
     */

    m_seqkeys_wid.set_listen_button_release(ev);

    /* middle click, or ctrl- (???) left click button up */

    if (SEQ64_CLICK_LEFT_MIDDLE(ev->button))
    {
        if (m_growing)        /* convert deltas into screen coordinates */
        {
            convert_xy(delta_x, delta_y, delta_tick, delta_note);

            /*
             * m_seq->push_undo();
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
    convert_xy            /* try to eliminate this */
    (
        current_x(), current_y(), current_tick, current_note
    );

    /*
     *  -   ctrl-left click button up for select/drag copy/paste
     *  -   left click button up for ending a move of selected notes
     */

    if (SEQ64_CLICK_LEFT(ev->button))
    {
        set_adding(false);
        if (m_is_drag_pasting)
        {
            m_is_drag_pasting = false;
            m_is_drag_pasting_start = false;

            /* convert deltas into screen coordinates */

            complete_paste();
            needs_update = true;
        }

        /* ctrl-left click but without movement - select a note */

        if (m_is_drag_pasting_start)
        {
            m_is_drag_pasting_start = false;

            /*
             * If ctrl-left click without movement and if note under
             * cursor is selected, and ctrl is held and button-down didn't
             * just select one, then deselect the note.
             */

            if
            (
                is_ctrl_key(ev) &&
                ! m_justselected_one &&
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
        m_justselected_one = false; /* clear flag on left button up */

        if (m_moving)
        {
            /**
             * If in moving mode, adjust for snap and convert deltas into
             * screen coordinates.  Since delta_note was from delta_y, it will
             * be flipped (delta_y[0] = note[127], etc.), so we have to
             * adjust.
             */

            delta_x -= m_move_snap_offset_x;      /* adjust for snap */
            convert_xy(delta_x, delta_y, delta_tick, delta_note);
            delta_note -= c_num_keys - 1;

            /*
             * m_seq->push_undo();
             */

            seq.move_selected_notes(delta_tick, delta_note);
            needs_update = true;
        }
    }

    /* right click or left click button up for selection box */

    if (SEQ64_CLICK_LEFT_RIGHT(ev->button))
    {
        if (m_selecting)
        {
            rect::xy_to_rect_get
            (
                drop_x(), drop_y(),
                current_x(), current_y(), x, y, w, h
            );
            convert_xy(x, y, tick_s, note_h);
            convert_xy(x + w, y + h, tick_f, note_l);
            (void) seq.select_note_events
            (
                tick_s, note_h, tick_f, note_l, sequence::e_toggle_selection
            );
            needs_update = true;
        }
    }
    if (SEQ64_CLICK_RIGHT(ev->button))
        m_erase_painting = false;

    m_selecting = false;            /* turn it all off */
    m_moving = false;
    m_growing = false;
    m_paste = false;
    m_moving_init = false;
    m_painting = false;
    seq.unpaint_all();
    update_mouse_pointer(adding()); /* context sensitive mouse pointer... */
    if (needs_update)               /* if they clicked, something changed */
        seq.set_dirty();            /* redraw_events();                   */

    (void) seqroll::on_button_release_event(ev);
    return needs_update;
}

/**
 *  Implements the fruity handling for the on-motion-notify event.
 *
 *  Why not just inherit and save all these indirect accesses to the seqroll?
 *  Well, that would make it more difficult to change the mode of interation,
 *  in the Options menu, on the fly. We did it anyway.  One will hardly ever
 *  change the mouse interaction mode.
 *
 * \param ev
 *      The motion event.
 *
 * \return
 *      Returns the value of needs_update.
 */

bool
FruitySeqRollInput::on_motion_notify_event (GdkEventMotion * ev)
{
    bool result = false;
    sequence & seq = m_seq;                   /* just do this once!   */
    set_current_offset_x_y(int(ev->x), int(ev->y));
    if (m_moving_init)
    {
        m_moving_init = false;
        m_moving = true;
    }
    update_mouse_pointer(adding()); /* context sensitive mouse pointer... */

    /**
     * In "fruity" interatction mode, ctrl-left-click-drag on selected note(s)
     * starts a copy/unselect/paste.  Doesn't begin the paste until the mouse
     * moves a few pixels, to filter out the unsteady hand.
     */

    if
    (
        m_is_drag_pasting_start &&
        (
            s_jitter_amount <= abs(m_drag_paste_start_pos[0] - int(ev->x)) ||
            s_jitter_amount <= abs(m_drag_paste_start_pos[1] - int(ev->y))
        )
    )
    {
        seq.copy_selected();
        seq.unselect();
        start_paste();
        m_is_drag_pasting_start = false;
        m_is_drag_pasting = true;
    }

    /*
     * seqroll::set_hint_note()
     */

    int note;
    midipulse tick;
    snap_y(m_current_y);
    convert_xy(0, m_current_y, tick, note);
    m_seqkeys_wid.set_hint_key(note);
    if (select_action())
    {
        if (drop_action())                        /* moving or paste  */
            snap_x(m_current_x);

        if (moving())                             /* stazed fix       */
            m_seqkeys_wid.set_listen_motion_notify(ev);

        draw_selection_on_window();
        result = true;
    }
    else if (m_painting)
    {
#ifdef SEQ64_STAZED_CHORD_GENERATOR
        if (m_chord != 0)     /* chord, don't allow move painting */
            result = true;
        else
#endif
        {
            snap_x(m_current_x);
            convert_xy(current_x(), current_y(), tick, note);
            add_note(tick, note);
            result = true;
        }
    }
    else if (m_erase_painting)
    {
        convert_xy(current_x(), current_y(), tick, note);
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

    (void) seqroll::on_motion_notify_event(ev);

    return result;
}

}           // namespace seq64

/*
 * fruityseqroll.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

