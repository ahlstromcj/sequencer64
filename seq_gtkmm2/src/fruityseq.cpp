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
 * \file          fruityseq.cpp
 *
 *  This module declares/defines the mouse interactions for the "fruity"
 *  mode.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-08-02
 * \updates       2016-08-10
 * \license       GNU GPLv2 or above
 *
 *  This code was extracted from seqevent to make that module more
 *  manageable.
 */

#include <gdkmm/cursor.h>
#include <gtkmm/button.h>

#include "click.hpp"                    /* SEQ64_CLICK_LEFT() etc.       */
#include "fruityseq.hpp"
#include "gui_key_tests.hpp"            /* seq64::is_no_modifier() etc. */
#include "perform.hpp"
#include "seqevent.hpp"
#include "sequence.hpp"                 /* for full usage of seqevent       */

namespace seq64
{

/**
 *  Provides support for a context-sensitive mouse.
 *
 * \param seqev
 *      Provides the seqevent pane (actually a strip on the seqedit window)
 *      to update to show the proper mouse cursor (left pointer, center
 *      pointer, and pencil).
 */

void
FruitySeqEventInput::update_mouse_pointer (seqevent & seqev)
{
    midipulse tick_s, tick_w, pos;
    seqev.convert_x(seqev.m_current_x, tick_s);
    seqev.convert_x(c_eventevent_x, tick_w);

    midipulse tick_f = tick_s + tick_w; // can tick_f ever get < 0?
    if (tick_s < 0)
        tick_s = 0;                     // clamp to 0

    if (m_is_drag_pasting || seqev.m_selecting || seqev.m_moving || seqev.m_paste)
        seqev.get_window()->set_cursor(Gdk::Cursor(Gdk::LEFT_PTR));
    else if (seqev.m_seq.intersect_events(tick_s, tick_f, seqev.m_status, pos))
        seqev.get_window()->set_cursor(Gdk::Cursor(Gdk::CENTER_PTR));
    else
        seqev.get_window()->set_cursor(Gdk::Cursor(Gdk::PENCIL));
}

/**
 *  Implements the on-button-press event callback.  Handles dragging and other
 *  actions.
 *
 *  The first thing is to set the values for dragging, then reset the box that
 *  holds the dirty redraw spot.  If pasting, undo the clipboard, and paste the
 *  selected events.
 *
 *  Otherwise, process the mouse actions.  The current steps shown below are my
 *  initial guesses, to be verified at some point.
 *
 *  -#  Left button:
 *      -#  Click:
 *          -#  A click and release without a drag, or without a Ctrl-Shift,
 *              deselects the events.
 *          -#  A direct click on an event selects only that event.
 *      -#  Click-drag:
 *          -#  If events already selected, adds note and length to the selected
 *              notes.
 *          -#  Otherwise, select the notes and events.
 *          -#  If no events selected in the end, undo the selection.
 *  -   Ctrl-left button:
 *      -   TODO.
 *
 *  The opening part of this function matches that of Seq24SeqEventInput ::
 *  on_button_press_event().
 *
 * \param ev
 *      The button event for the press of a mouse button.
 *
 * \param seqev
 *      Provides the seqevent strip to be affected by this button event.
 *
 * \return
 *      Returns true if a modification was made.  It used to return true all
 *      the time.
 */

bool
FruitySeqEventInput::on_button_press_event
(
    GdkEventButton * ev,
    seqevent & seqev
)
{
    bool result = false;
    midipulse tick_s, tick_w;
    seqev.grab_focus();                 // NEW: I think this would be helpful
    seqev.convert_x(c_eventevent_x, tick_w);
    seqev.m_drop_x = seqev.m_current_x = int(ev->x) + seqev.m_scroll_offset_x;
    seqev.m_old.x = seqev.m_old.y = seqev.m_old.width = seqev.m_old.height = 0;
    if (seqev.m_paste)
    {
        seqev.snap_x(seqev.m_current_x);
        seqev.convert_x(seqev.m_current_x, tick_s);
        seqev.m_paste = false;
        seqev.m_seq.paste_selected(tick_s, 0);          /* does undo/mod    */
        seqev.m_seq.set_dirty();                        /* a stazed fix     */
        result = true;
    }
    else
    {
        int x, w;
        midipulse tick_f;
        if (SEQ64_CLICK_LEFT(ev->button))               /* Note 1   */
        {
            seqev.convert_x(seqev.m_drop_x, tick_s); /* x,y into tick/note    */
            tick_f = tick_s + seqev.m_zoom;          /* shift back some ticks */
            tick_s -= tick_w;
            if (tick_s < 0)
                tick_s = 0;

            int eventcount = seqev.m_seq.select_events
            (
                tick_s, tick_f, seqev.m_status, seqev.m_cc,
                sequence::e_would_select
            );
            if (! is_ctrl_key(ev) && eventcount == 0)
            {
                seqev.m_painting = true;
                seqev.snap_x(seqev.m_drop_x);
                seqev.convert_x(seqev.m_drop_x, tick_s); /* x,y-->tick/note  */
                eventcount = seqev.m_seq.select_events
                (
                    tick_s, tick_f, seqev.m_status, seqev.m_cc,
                    sequence::e_would_select
                );
                if (eventcount == 0)
                {
                    seqev.m_seq.push_undo();            /* add to add_event? */
                    seqev.drop_event(tick_s);           /* m_seq.add_event() */
                    result = true;
                }
            }
            else                                        /* selecting         */
            {
                eventcount = seqev.m_seq.select_events
                (
                    tick_s, tick_f, seqev.m_status, seqev.m_cc,
                    sequence::e_is_selected
                );
                if (eventcount == 0)
                {
                    eventcount = seqev.m_seq.select_events
                    (
                        tick_s, tick_f, seqev.m_status, seqev.m_cc,
                        sequence::e_would_select
                    );
                    if (eventcount > 0)                 /* if clicking event */
                    {
                        if (! is_ctrl_key(ev))
                            seqev.m_seq.unselect();
                    }
                    else    /* clickempty space, unselect all if no Ctrl-Sh  */
                    {
                        if (! is_ctrl_shift_key(ev))
                            seqev.m_seq.unselect();
                    }

                    /* on direct click select only one event */

                    eventcount = seqev.m_seq.select_events
                    (
                        tick_s, tick_f, seqev.m_status,
                        seqev.m_cc, sequence::e_select_one
                    );

                    /*
                     * Stazed fix:
                     */

#ifdef USE_STAZED_SELECTION_EXTENSIONS
                    if (event::is_strict_note_msg(seqev.m_status))
                    {
                        seqev.m_seq.select_linked(tick_s, tick_f, seqev.m_status);
                        seqev.m_seq.set_dirty();
                    }
#endif

                    if (eventcount)
                        m_justselected_one = true;  /* stop deselect on release */

                    /* if nothing selected, start the selection box */

                    if (is_ctrl_key(ev) && eventcount == 0)
                        seqev.m_selecting = true;
                }
                eventcount = seqev.m_seq.select_events
                (
                    tick_s, tick_f, seqev.m_status, seqev.m_cc,
                    sequence::e_is_selected
                );
                if (eventcount > 0)         /* if event under cursor selected */
                {
                    if (! is_ctrl_key(ev))          /* grab/move note       */
                    {
                        seqev.m_moving_init = true;
                        int note;
                        seqev.m_seq.get_selected_box(tick_s, note, tick_f, note);
                        tick_f += tick_w;
                        seqev.convert_t(tick_s, x); /* convert to X,Y values */
                        seqev.convert_t(tick_f, w);
                        w -= x;                     /* w is coordinates now  */

                        /* set the m_selected rectangle for x,y,w,h */

                        seqev.m_selected.x = x;
                        seqev.m_selected.width = w;
                        seqev.m_selected.y = (c_eventarea_y-c_eventevent_y) / 2;
                        seqev.m_selected.height = c_eventevent_y;

                        /* save offset that we get from the snap above */

                        int adjusted_selected_x = seqev.m_selected.x;
                        seqev.snap_x(adjusted_selected_x);
                        seqev.m_move_snap_offset_x =
                            seqev.m_selected.x - adjusted_selected_x;

                        /* align selection for drawing */

                        seqev.snap_x(seqev.m_selected.x);
                        seqev.snap_x(seqev.m_current_x);
                        seqev.snap_x(seqev.m_drop_x);
                    }
                    else if   /* Ctrl-Left-click when stuff already selected */
                    (
                        is_ctrl_key(ev) &&
                        seqev.m_seq.select_events(tick_s, tick_f,
                           seqev. m_status, seqev.m_cc, sequence::e_is_selected)
                    )
                    {
                        m_is_drag_pasting_start = true;
                    }
                }
            }

        }
        if (SEQ64_CLICK_RIGHT(ev->button))
        {
            seqev.convert_x(seqev.m_drop_x, tick_s); /* x,y in to tick/note   */
            tick_f = tick_s + seqev.m_zoom;          /* shift back some ticks */
            tick_s -= (tick_w);
            if (tick_s < 0)
                tick_s = 0;

            /*
             * Stazed fix: don't allow individual deletion of Note On/Off
             * events.  Should we do the same for AfterTouch events?  No, they
             * are not linked to Note On or Note Off events.
             */

            if (event::is_strict_note_msg(seqev.m_status))
                return true;

            int eventcount = seqev.m_seq.select_events
            (
                tick_s, tick_f, seqev.m_status, seqev.m_cc,
                sequence::e_would_select
            );
            if (eventcount > 0)   /* erase event under cursor if there is one */
            {
                /* remove only note under cursor, leave selection intact */

                (void) seqev.m_seq.select_events
                (
                    tick_s, tick_f,
                    seqev.m_status, seqev.m_cc, sequence::e_remove_one
                );
                seqev.redraw();
                seqev.m_seq.set_dirty();                /* take note!       */
                result = true;
            }
            else                                        /* selecting        */
            {
                if (! is_ctrl_key(ev))
                    seqev.m_seq.unselect();             /* nothing selected   */

                seqev.m_selecting = true;               /* start select-box   */
            }
        }
    }
    seqev.update_pixmap();              /* if they clicked, something changed */
    seqev.draw_pixmap_on_window();
    update_mouse_pointer(seqev);
    return result;                      // true;
}

/**
 *  Implements the on-button-release callback.
 *
 * \param ev
 *      The button event for the press of a mouse button.
 *
 * \param seqev
 *      Provides the seqevent strip to be affected by this button event.
 *
 * \return
 *      Returns true if a modification was made.  It used to return true all
 *      the time.
 */

bool
FruitySeqEventInput::on_button_release_event
(
    GdkEventButton * ev,
    seqevent & seqev
)
{
    bool result = false;
    midipulse tick_s;
    midipulse tick_f;
    seqev.grab_focus();
    seqev.m_current_x = int(ev->x) + seqev.m_scroll_offset_x;;
    if (seqev.m_moving || m_is_drag_pasting)
        seqev.snap_x(seqev.m_current_x);

    int delta_x = seqev.m_current_x - seqev.m_drop_x;
    midipulse delta_tick;
    if (SEQ64_CLICK_LEFT(ev->button))
    {
        int current_x = seqev.m_current_x;
        midipulse t_s, t_f;
        seqev.snap_x(current_x);
        seqev.convert_x(current_x, t_s);
        t_f = t_s + (seqev.m_zoom);                 /* shift back a few ticks */
        if (t_s < 0)
            t_s = 0;

        /*
         * Use the ctrl-left click button up for select/drag copy/paste;
         * use the left click button up for ending a move of selected notes.
         */

        if (m_is_drag_pasting)
        {
            m_is_drag_pasting = false;
            m_is_drag_pasting_start = false;
            seqev.m_paste = false; /* convert deltas into screen coordinates */
            seqev.m_seq.paste_selected(t_s, 0);
            result = true;
        }

        /* ctrl-left click but without movement - select a note */

        if (m_is_drag_pasting_start)
        {
            m_is_drag_pasting_start = false;

            /*
             * If a ctrl-left click without movement and if the note under
             * cursor is selected, and ctrl is held and button-down,
             * just select one.
             */

            if                                  /* deselect the event? */
            (
                is_ctrl_key(ev) &&
                ! m_justselected_one &&
                seqev.m_seq.select_events
                (
                    t_s, t_f, seqev.m_status, seqev.m_cc, sequence::e_is_selected
                ) 
            )
            {
                (void) seqev.m_seq.select_events
                (
                    t_s, t_f, seqev.m_status, seqev.m_cc, sequence::e_deselect
                );
            }
        }
        m_justselected_one = false;         /* clear flag on left button up */
        if (seqev.m_moving)
        {
            delta_x -= seqev.m_move_snap_offset_x; /* adjust for snap         */
            seqev.convert_x(delta_x, delta_tick);  /* deltas to screen coords */
            seqev.m_seq.move_selected_notes(delta_tick, 0);
            result = true;
        }
    }

    /*
     * Yet another stazed fix.  :-)
     */

    bool right = SEQ64_CLICK_RIGHT(ev->button);
    if (! right)
        right = is_ctrl_key(ev) && SEQ64_CLICK_LEFT(ev->button);

    if (right)
    {
        if (seqev.m_selecting)
        {
            int x, w;
            seqev.x_to_w(seqev.m_drop_x, seqev.m_current_x, x, w);
            seqev.convert_x(x, tick_s);
            seqev.convert_x(x + w, tick_f);
            (void) seqev.m_seq.select_events
            (
                tick_s, tick_f,
                seqev.m_status, seqev.m_cc, sequence::e_toggle_selection
            );

#ifdef USE_STAZED_SELECTION_EXTENSIONS

            /*
             * Stazed fix
             */

            if (event::is_strict_note_msg(seqev.m_status))
                seqev.m_seq.select_linked(tick_s, tick_f, seqev.m_status);
#endif

            /*
             * To update the select or unselect of notes by this action.
             * Not sure this makes sense, though.  How does selection dirty
             * anything?
             */

            seqev.m_seq.set_dirty();
        }
    }
    seqev.m_selecting = false;          /* turn it all off */
    seqev.m_moving = false;
    seqev.m_growing = false;
    seqev.m_moving_init = false;
    seqev.m_painting = false;
    seqev.m_seq.unpaint_all();
    seqev.update_pixmap();              /* if they clicked, something changed */
    seqev.draw_pixmap_on_window();
    update_mouse_pointer(seqev);
    return result;                      // true;
}

/**
 *  Implements the on-motion-notify callback.
 *
 * \param ev
 *      The button event for the press of a mouse button.
 *
 * \param seqev
 *      Provides the seqevent strip to be affected by this button event.
 *
 * \return
 *      Returns true if a modification occurred, and sets the perform modified
 *      flag based on that result.
 */

bool
FruitySeqEventInput::on_motion_notify_event
(
    GdkEventMotion * ev,
    seqevent & seqev
)
{
    bool result = false;
    midipulse tick = 0;
    seqev.m_current_x = (int) ev->x  + seqev.m_scroll_offset_x;
    if (seqev.m_moving_init)
    {
        seqev.m_moving_init = false;
        seqev.m_moving = true;
    }
    update_mouse_pointer(seqev);      /* context sensitive mouse pointer... */

    /*
     * Ctrl-left click drag on selected note(s) starts a copy/unselect/paste.
     */

    if (m_is_drag_pasting_start)
    {
        seqev.m_seq.copy_selected();
        seqev.m_seq.unselect();
        seqev.start_paste();
        m_is_drag_pasting_start = false;
        m_is_drag_pasting = true;
    }
    if (seqev.m_selecting || seqev.m_moving || seqev.m_paste)
    {
        if (seqev.m_moving || seqev.m_paste)
            seqev.snap_x(seqev.m_current_x);

        seqev.draw_selection_on_window();
    }
    if (seqev.m_painting)
    {
        seqev.m_current_x = int(ev->x)  + seqev.m_scroll_offset_x;
        seqev.snap_x(seqev.m_current_x);
        seqev.convert_x(seqev.m_current_x, tick);
        seqev.drop_event(tick);             // Why no push_undo()?
        result = true;
    }
    return result;
}

}           // namespace seq64

/*
 * fruityseq.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

