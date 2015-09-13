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
 * \file          seq24seq.cpp
 *
 *  This module declares/defines the mouse interactions for the "seq24"
 *  mode in the pattern/sequence editor.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-08-02
 * \updates       2015-09-13
 * \license       GNU GPLv2 or above
 *
 *  This code was extracted from seqevent to make that module more
 *  manageable.
 */

#include <gdkmm/cursor.h>
#include <gtkmm/button.h>

#include "seq24seq.hpp"
#include "seqevent.hpp"
#include "sequence.hpp"                 // needed for full usage of seqevent

namespace seq64
{

/**
 *  Changes the mouse cursor to a pencil or a left pointer in the given
 *  seqevent aobject, depending on the first parameter.  Modifies m_adding
 *  as well.
 */

void
Seq24SeqEventInput::set_adding (bool a_adding, seqevent & seqev)
{
    if (a_adding)
    {
        seqev.get_window()->set_cursor(Gdk::Cursor(Gdk::PENCIL));
        m_adding = true;
    }
    else
    {
        seqev.get_window()->set_cursor(Gdk::Cursor(Gdk::LEFT_PTR));
        m_adding = false;
    }
}

/**
 *  Implements the on-button-press event callback.
 */

bool
Seq24SeqEventInput::on_button_press_event
(
    GdkEventButton * a_ev, seqevent & seqev
)
{
    int x, w, numsel;
    long tick_s;
    long tick_f;
    long tick_w;
    seqev.convert_x(c_eventevent_x, &tick_w);

    /* set values for dragging */

    seqev.m_drop_x = seqev.m_current_x = (int) a_ev->x + seqev.m_scroll_offset_x;

    /* reset box that holds dirty redraw spot */

    seqev.m_old.x = 0;
    seqev.m_old.y = 0;
    seqev.m_old.width = 0;
    seqev.m_old.height = 0;
    if (seqev.m_paste)
    {
        seqev.snap_x(&seqev.m_current_x);
        seqev.convert_x(seqev.m_current_x, &tick_s);
        seqev.m_paste = false;
        seqev.m_seq->push_undo();
        seqev.m_seq->paste_selected(tick_s, 0);
    }
    else
    {
        if (a_ev->button == 1)                      /* left mouse button     */
        {
            seqev.convert_x(seqev.m_drop_x, &tick_s); /* x,y in to tick/note */
            tick_f = tick_s + (seqev.m_zoom);       /* shift back a few ticks */
            tick_s -= (tick_w);
            if (tick_s < 0)
                tick_s = 0;

            if (m_adding)
            {
                seqev.m_painting = true;
                seqev.snap_x(&seqev.m_drop_x);
                seqev.convert_x(seqev.m_drop_x, &tick_s); /* x,y to tick/note */

                /* add note, length = little less than snap */

                if
                (
                    ! seqev.m_seq->select_events(tick_s, tick_f,
                       seqev.m_status, seqev.m_cc, sequence::e_would_select)
                )
                {
                    seqev.m_seq->push_undo();
                    seqev.drop_event(tick_s);
                }
            }
            else                                        /* selecting */
            {
                if
                (
                    ! seqev.m_seq->select_events
                    (
                        tick_s, tick_f,
                        seqev.m_status, seqev.m_cc, sequence::e_is_selected
                    )
                )
                {
                    if (! (a_ev->state & GDK_CONTROL_MASK))
                    {
                        seqev.m_seq->unselect();
                    }
                    numsel = seqev.m_seq->select_events
                    (
                        tick_s, tick_f,
                        seqev.m_status, seqev.m_cc, sequence::e_select_one
                    );

                    /*
                     * If we didn't select anything (the user clicked
                     * empty space), then unselect all notes, and start
                     * selecting with a new selection box.
                     */

                    if (numsel == 0)
                    {
                        seqev.m_selecting = true;
                    }
                    else
                    {
                        /**
                         * \todo
                         *      Needs update.
                         */
                    }
                }
                if
                (
                    seqev.m_seq->select_events
                    (
                        tick_s, tick_f,
                        seqev.m_status, seqev.m_cc, sequence::e_is_selected
                    )
                )
                {
                    seqev.m_moving_init = true;
                    int note;

                    /* get the box that selected elements are in */

                    seqev.m_seq->get_selected_box(&tick_s, &note, &tick_f, &note);
                    tick_f += tick_w;
                    seqev.convert_t(tick_s, &x); /* convert box to X,Y values */
                    seqev.convert_t(tick_f, &w);
                    w = w - x;                   /* w is coordinate now */

                    /*
                     * Set the m_selected rectangle to hold the
                     * x,y,w,h of our selected events.
                     */

                    seqev.m_selected.x = x;
                    seqev.m_selected.width = w;
                    seqev.m_selected.y = (c_eventarea_y - c_eventevent_y) / 2;
                    seqev.m_selected.height = c_eventevent_y;

                    /* save offset that we get from the snap above */

                    int adjusted_selected_x = seqev.m_selected.x;
                    seqev.snap_x(&adjusted_selected_x);
                    seqev.m_move_snap_offset_x =
                        seqev.m_selected.x - adjusted_selected_x;

                    /* align selection for drawing */

                    seqev.snap_x(&seqev.m_selected.x);
                    seqev.snap_x(&seqev.m_current_x);
                    seqev.snap_x(&seqev.m_drop_x);
                }
            }
        }
        if (a_ev->button == 3)
        {
            set_adding(true, seqev);
        }
    }
    seqev.update_pixmap();          /* if they clicked, something changed */
    seqev.draw_pixmap_on_window();
    return true;
}

/**
 *  Implements the on-button-release callback.
 */

bool
Seq24SeqEventInput::on_button_release_event
(
    GdkEventButton * a_ev, seqevent & seqev
)
{
    long tick_s;
    long tick_f;
    int x, w;
    seqev.grab_focus();
    seqev.m_current_x = (int) a_ev->x  + seqev.m_scroll_offset_x;;
    if (seqev.m_moving)
        seqev.snap_x(&seqev.m_current_x);

    int delta_x = seqev.m_current_x - seqev.m_drop_x;
    long delta_tick;

    if (a_ev->button == 1)
    {
        if (seqev.m_selecting)
        {
            seqev.x_to_w(seqev.m_drop_x, seqev.m_current_x, &x, &w);
            seqev.convert_x(x,   &tick_s);
            seqev.convert_x(x + w, &tick_f);
            (void) seqev.m_seq->select_events
            (
                tick_s, tick_f, seqev.m_status, seqev.m_cc, sequence::e_select
            );
        }
        if (seqev.m_moving)
        {
            delta_x -= seqev.m_move_snap_offset_x; /* adjust for snap       */
            seqev.convert_x(delta_x, &delta_tick); /* to screen coordinates */
            seqev.m_seq->push_undo();              /* still moves events    */
            seqev.m_seq->move_selected_notes(delta_tick, 0);
        }
        set_adding(m_adding, seqev);
    }
    if (a_ev->button == 3)
    {
        set_adding(false, seqev);
    }
    seqev.m_selecting = false;                      /* turn off             */
    seqev.m_moving = false;
    seqev.m_growing = false;
    seqev.m_moving_init = false;
    seqev.m_painting = false;
    seqev.m_seq->unpaint_all();
    seqev.update_pixmap();            /* if they clicked, something changed */
    seqev.draw_pixmap_on_window();
    return true;
}

/**
 *  Implements the on-motion-notify event.
 */

bool
Seq24SeqEventInput::on_motion_notify_event
(
   GdkEventMotion * a_ev, seqevent & seqev
)
{
    long tick = 0;
    if (seqev.m_moving_init)
    {
        seqev.m_moving_init = false;
        seqev.m_moving = true;
    }
    if (seqev.m_selecting || seqev.m_moving || seqev.m_paste)
    {
        seqev.m_current_x = (int) a_ev->x  + seqev.m_scroll_offset_x;;
        if (seqev.m_moving || seqev.m_paste)
            seqev.snap_x(&seqev.m_current_x);

        seqev.draw_selection_on_window();
    }
    if (seqev.m_painting)
    {
        seqev.m_current_x = (int) a_ev->x  + seqev.m_scroll_offset_x;;
        seqev.snap_x(&seqev.m_current_x);
        seqev.convert_x(seqev.m_current_x, &tick);
        seqev.drop_event(tick);
    }
    return true;
}

}           // namespace seq64

/*
 * seq24seq.cpp
 *
 * vim: sw=4 ts=4 wm=8 et ft=cpp
 */
