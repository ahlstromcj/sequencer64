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
 * \updates       2015-10-31
 * \license       GNU GPLv2 or above
 *
 *  This code was extracted from seqevent to make that module more
 *  manageable.
 */

#include <gdkmm/cursor.h>
#include <gtkmm/button.h>

#include "click.hpp"                    /* SEQ64_CLICK_LEFT() etc.       */
#include "fruityseq.hpp"
#include "seqevent.hpp"
#include "sequence.hpp"                 /* for full usage of seqevent       */

namespace seq64
{

/**
 *  Provides support for a context-sensitive mouse.
 */

void
FruitySeqEventInput::updateMousePtr (seqevent & seqev)
{
    long tick_s, tick_w, tick_f, pos;
    seqev.convert_x(seqev.m_current_x, tick_s);
    seqev.convert_x(c_eventevent_x, tick_w);
    tick_f = tick_s + tick_w;
    if (tick_s < 0)
        tick_s = 0;                    // clamp to 0

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
 */

bool
FruitySeqEventInput::on_button_press_event
(
    GdkEventButton * a_ev,
    seqevent & seqev
)
{
    long tick_s, tick_w;
    seqev.convert_x(c_eventevent_x, tick_w);
    seqev.m_drop_x = seqev.m_current_x = int(a_ev->x) + seqev.m_scroll_offset_x;
    seqev.m_old.x = seqev.m_old.y = seqev.m_old.width = seqev.m_old.height = 0;
    if (seqev.m_paste)
    {
        seqev.snap_x(seqev.m_current_x);
        seqev.convert_x(seqev.m_current_x, tick_s);
        seqev.m_paste = false;
        seqev.m_seq.push_undo();
        seqev.m_seq.paste_selected(tick_s, 0);
    }
    else
    {
        int x, w;
        long tick_f;
        if (SEQ64_CLICK_LEFT(a_ev->button))       /* Note 1   */
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
            if (! (a_ev->state & SEQ64_CONTROL_MASK) && eventcount == 0)
            {
                seqev.m_painting = true;
                seqev.snap_x(seqev.m_drop_x);
                seqev.convert_x(seqev.m_drop_x, tick_s); /* x,y into tick/note */
                eventcount = seqev.m_seq.select_events
                (
                    tick_s, tick_f, seqev.m_status, seqev.m_cc,
                    sequence::e_would_select
                );
                if (eventcount == 0)
                {
                    seqev.m_seq.push_undo();
                    seqev.drop_event(tick_s);
                }
            }
            else                                    /* selecting */
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
                    if (eventcount > 0)             /* if clicking event */
                    {
                        if (! (a_ev->state & SEQ64_CONTROL_MASK))
                            seqev.m_seq.unselect();
                    }
                    else /* clicking empty space, unselect all if no Ctrl-Sh  */
                    {
                        if
                        (
                            ! ((a_ev->state & SEQ64_CONTROL_MASK) &&
                                (a_ev->state & SEQ64_SHIFT_MASK))
                        )
                        {
                            seqev.m_seq.unselect();
                        }
                    }

                    /* on direct click select only one event */

                    eventcount = seqev.m_seq.select_events
                    (
                        tick_s, tick_f, seqev.m_status,
                        seqev.m_cc, sequence::e_select_one
                    );
                    if (eventcount)
                        m_justselected_one = true;  /* stop deselect on release */

                    /* if nothing selected, start the selection box */

                    if (eventcount == 0 && (a_ev->state & SEQ64_CONTROL_MASK))
                        seqev.m_selecting = true;
                }
                eventcount = seqev.m_seq.select_events
                (
                    tick_s, tick_f, seqev.m_status, seqev.m_cc,
                    sequence::e_is_selected
                );
                if (eventcount > 0)         /* if event under cursor selected */
                {
                    if (! (a_ev->state & SEQ64_CONTROL_MASK)) /* grab/move note */
                    {
                        seqev.m_moving_init = true;
                        int note;
                        seqev.m_seq.get_selected_box(tick_s, note, tick_f, note);
                        tick_f += tick_w;
                        seqev.convert_t(tick_s, x);  /* convert to X,Y values */
                        seqev.convert_t(tick_f, w);
                        w -= x;                      /* w is coordinates now  */

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
                        (a_ev->state & SEQ64_CONTROL_MASK) &&
                        seqev.m_seq.select_events(tick_s, tick_f,
                           seqev. m_status, seqev.m_cc, sequence::e_is_selected)
                    )
                    {
                        m_is_drag_pasting_start = true;
                    }
                }
            }

        }
        if (SEQ64_CLICK_RIGHT(a_ev->button))
        {
            seqev.convert_x(seqev.m_drop_x, tick_s); /* x,y in to tick/note   */
            tick_f = tick_s + seqev.m_zoom;          /* shift back some ticks */
            tick_s -= (tick_w);
            if (tick_s < 0)
                tick_s = 0;

            int eventcount = seqev.m_seq.select_events
            (
                tick_s, tick_f, seqev.m_status, seqev.m_cc,
                sequence::e_would_select
            );
            if (eventcount > 0)   /* erase event under cursor if there is one */
            {
                /* remove only note under cursor, leave selection intact */

                seqev.m_seq.push_undo();
                (void) seqev.m_seq.select_events
                (
                    tick_s, tick_f,
                    seqev.m_status, seqev.m_cc, sequence::e_remove_one
                );
                seqev.redraw();
                seqev.m_seq.set_dirty();
            }
            else                                        /* selecting          */
            {
                if (! (a_ev->state & SEQ64_CONTROL_MASK))
                    seqev.m_seq.unselect();             /* nothing selected   */

                seqev.m_selecting = true;               /* start select-box   */
            }
        }
    }
    seqev.update_pixmap();              /* if they clicked, something changed */
    seqev.draw_pixmap_on_window();
    updateMousePtr(seqev);
    return true;
}

/**
 *  Implements the on-button-release callback.
 */

bool
FruitySeqEventInput::on_button_release_event
(
    GdkEventButton * a_ev,
    seqevent & seqev
)
{
    long tick_s;
    long tick_f;
    seqev.grab_focus();
    seqev.m_current_x = int(a_ev->x) + seqev.m_scroll_offset_x;;
    if (seqev.m_moving || m_is_drag_pasting)
        seqev.snap_x(seqev.m_current_x);

    int delta_x = seqev.m_current_x - seqev.m_drop_x;
    long delta_tick;
    if (SEQ64_CLICK_LEFT(a_ev->button))
    {
        int current_x = seqev.m_current_x;
        long t_s, t_f;
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
            seqev.m_seq.push_undo();
            seqev.m_seq.paste_selected(t_s, 0);
        }

        /* ctrl-left click but without movement - select a note */

        if (m_is_drag_pasting_start)
        {
            m_is_drag_pasting_start = false;

            /*
             * If a ctrl-left click without movement and if note under
             * cursor is selected, and ctrl is held and button-down didn't
             * just select one.
             */

            if                                  /* deselect the event? */
            (
                ! m_justselected_one &&
                seqev.m_seq.select_events
                (
                    t_s, t_f,
                    seqev.m_status, seqev.m_cc, sequence::e_is_selected
                ) &&
                (a_ev->state & SEQ64_CONTROL_MASK)
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
            seqev.m_seq.push_undo();               /* still moves events      */
            seqev.m_seq.move_selected_notes(delta_tick, 0);
        }
    }
    if (SEQ64_CLICK_LEFT_RIGHT(a_ev->button))
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
    updateMousePtr(seqev);
    return true;
}

/**
 *  Implements the on-motion-notify callback.
 */

bool
FruitySeqEventInput::on_motion_notify_event
(
    GdkEventMotion * a_ev, seqevent & seqev
)
{
    long tick = 0;
    seqev.m_current_x = (int) a_ev->x  + seqev.m_scroll_offset_x;
    if (seqev.m_moving_init)
    {
        seqev.m_moving_init = false;
        seqev.m_moving = true;
    }
    updateMousePtr(seqev);      /* context sensitive mouse pointer... */

    /* ctrl-left click drag on selected note(s) starts a copy/unselect/paste */

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
        seqev.m_current_x = (int) a_ev->x  + seqev.m_scroll_offset_x;
        seqev.snap_x(seqev.m_current_x);
        seqev.convert_x(seqev.m_current_x, tick);
        seqev.drop_event(tick);
    }
    return true;
}

}           // namespace seq64

/*
 * fruityseq.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
