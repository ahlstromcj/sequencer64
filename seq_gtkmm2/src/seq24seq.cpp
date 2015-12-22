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
 *  mode in the pattern/sequence editor's event panel, the narrow string
 *  between the piano roll and the data panel that's at the bottom.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-08-02
 * \updates       2015-11-22
 * \license       GNU GPLv2 or above
 *
 *  This code was extracted from seqevent to make that module more
 *  manageable.
 *
 *  One thing to note is that the seqevent user-interface isn't very high, so
 *  that y values don't mean anything in it.  It's just high enough to be
 *  visible and move the mouse horizontally in it.
 */

#include <gdkmm/cursor.h>
#include <gtkmm/button.h>

#include "click.hpp"                    /* SEQ64_CLICK_LEFT(), etc.     */
#include "seq24seq.hpp"
#include "seqevent.hpp"
#include "sequence.hpp"                 /* for full usage of seqevent   */

namespace seq64
{

/**
 *  Changes the mouse cursor to a pencil or a left pointer in the given
 *  seqevent aobject, depending on the first parameter.  Modifies m_adding
 *  as well.
 */

void
Seq24SeqEventInput::set_adding (bool adding, seqevent & seqev)
{
    m_adding = adding;
    if (adding)
        seqev.get_window()->set_cursor(Gdk::Cursor(Gdk::PENCIL));
    else
        seqev.get_window()->set_cursor(Gdk::Cursor(Gdk::LEFT_PTR));
}

/**
 *  Implements the on-button-press event callback.  Set values for dragging,
 *  then reset the box that holds dirty redraw spot.  Then do the rest.
 *
 * \return
 *      Returns true if a likely modification was made.  This function used to
 *      return true all the time.
 */

bool
Seq24SeqEventInput::on_button_press_event
(
    GdkEventButton * a_ev,
    seqevent & seqev
)
{
    bool result = false;
    midipulse tick_s, tick_w;
    seqev.grab_focus();                 // NEW: I think this would be helpful
    seqev.convert_x(c_eventevent_x, tick_w);
    seqev.set_current_drop_x(int(a_ev->x + seqev.m_scroll_offset_x));
    seqev.m_old.x = seqev.m_old.y = seqev.m_old.width = seqev.m_old.height = 0;
    if (seqev.m_paste)
    {
        seqev.snap_x(seqev.m_current_x);
        seqev.convert_x(seqev.m_current_x, tick_s);
        seqev.m_paste = false;
        seqev.m_seq.push_undo();
        seqev.m_seq.paste_selected(tick_s, 0);
        result = true;
    }
    else
    {
        int x, w;
        midipulse tick_f;
        if (SEQ64_CLICK_LEFT(a_ev->button))
        {
            seqev.convert_x(seqev.m_drop_x, tick_s); /* x,y in to tick/note    */
            tick_f = tick_s + seqev.m_zoom;          /* shift back a few ticks */
            tick_s -= tick_w;
            if (tick_s < 0)
                tick_s = 0;

            int eventcount = 0;
            if (m_adding)
            {
                seqev.m_painting = true;
                seqev.snap_x(seqev.m_drop_x);
                seqev.convert_x(seqev.m_drop_x, tick_s); /* x,y to tick/note */
                eventcount = seqev.m_seq.select_events
                (
                    tick_s, tick_f, seqev.m_status, seqev.m_cc,
                    sequence::e_would_select
                );
                if (eventcount == 0)
                {
                    seqev.m_seq.push_undo();
                    seqev.drop_event(tick_s);
                    result = true;
                }
            }
            else                                        /* selecting */
            {
                eventcount = seqev.m_seq.select_events
                (
                    tick_s, tick_f, seqev.m_status,
                    seqev.m_cc, sequence::e_is_selected
                );
                if (eventcount == 0)
                {
                    if (! (a_ev->state & SEQ64_CONTROL_MASK))
                        seqev.m_seq.unselect();

                    eventcount = seqev.m_seq.select_events
                    (
                        tick_s, tick_f, seqev.m_status,
                        seqev.m_cc, sequence::e_select_one
                    );

                    /*
                     * If nothing selected (user clicked empty space),
                     * unselect all notes, and start selecting with a new
                     * selection box.
                     */

                    if (eventcount == 0)
                    {
                        seqev.m_selecting = true;
                    }
                    else
                    {
                        /**
                         * Needs update.
                         * seqev.m_seq.unselect();  ???????
                         */
                    }
                }
                eventcount = seqev.m_seq.select_events
                (
                    tick_s, tick_f, seqev.m_status,
                    seqev.m_cc, sequence::e_is_selected
                );
                if (eventcount > 0)             /* get box selections are in */
                {
                    seqev.m_moving_init = true;
                    int note;
                    seqev.m_seq.get_selected_box(tick_s, note, tick_f, note);
                    tick_f += tick_w;
                    seqev.convert_t(tick_s, x); /* convert box to X,Y values */
                    seqev.convert_t(tick_f, w);
                    w -= x;                     /* w is coordinate now       */

                    /* set the m_selected rectangle for x,y,w,h */

                    seqev.m_selected.x = x;
                    seqev.m_selected.width = w;
                    seqev.m_selected.y = (c_eventarea_y - c_eventevent_y) / 2;
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
            }
        }
        if (SEQ64_CLICK_RIGHT(a_ev->button))
            set_adding(true, seqev);
    }
    seqev.update_pixmap();              /* if they clicked, something changed */
    seqev.draw_pixmap_on_window();
    return result;                      // true;
}

/**
 *  Implements the on-button-release callback.
 *
 * \return
 *      Returns true if a likely modification was made.  This function used to
 *      return true all the time.
 */

bool
Seq24SeqEventInput::on_button_release_event
(
    GdkEventButton * a_ev,
    seqevent & seqev
)
{
    bool result = false;
    midipulse tick_s;
    midipulse tick_f;
    seqev.grab_focus();
    seqev.m_current_x = int(a_ev->x) + seqev.m_scroll_offset_x;
    if (seqev.m_moving)
        seqev.snap_x(seqev.m_current_x);

    int delta_x = seqev.m_current_x - seqev.m_drop_x;
    midipulse delta_tick;
    if (SEQ64_CLICK_LEFT(a_ev->button))
    {
        if (seqev.m_selecting)
        {
            int x, w;
            seqev.x_to_w(seqev.m_drop_x, seqev.m_current_x, x, w);
            seqev.convert_x(x, tick_s);
            seqev.convert_x(x + w, tick_f);
            (void) seqev.m_seq.select_events
            (
                tick_s, tick_f, seqev.m_status, seqev.m_cc, sequence::e_select
            );
        }
        if (seqev.m_moving)
        {
            delta_x -= seqev.m_move_snap_offset_x;  /* adjust for snap       */
            seqev.convert_x(delta_x, delta_tick);   /* to screen coordinates */
            seqev.m_seq.push_undo();                /* still moves events    */
            seqev.m_seq.move_selected_notes(delta_tick, 0);
            result = true;
        }
        set_adding(m_adding, seqev);
    }
    if (SEQ64_CLICK_RIGHT(a_ev->button))
    {
        set_adding(false, seqev);
    }
    seqev.m_selecting = false;                      /* turn off              */
    seqev.m_moving = false;
    seqev.m_growing = false;
    seqev.m_moving_init = false;
    seqev.m_painting = false;
    seqev.m_seq.unpaint_all();
    seqev.update_pixmap();                  /* if a click, something changed */
    seqev.draw_pixmap_on_window();
    return result;                          // true;
}

/**
 *  Implements the on-motion-notify event.
 *
 * \return
 *      Returns true if a likely modification was made.  This function used to
 *      return true all the time.
 */

bool
Seq24SeqEventInput::on_motion_notify_event
(
   GdkEventMotion * a_ev,
   seqevent & seqev
)
{
    bool result = false;
    midipulse tick = 0;
    if (seqev.m_moving_init)
    {
        seqev.m_moving_init = false;
        seqev.m_moving = true;
    }
    if (seqev.m_selecting || seqev.m_moving || seqev.m_paste)
    {
        seqev.m_current_x = int(a_ev->x) + seqev.m_scroll_offset_x;
        if (seqev.m_moving || seqev.m_paste)
            seqev.snap_x(seqev.m_current_x);

        seqev.draw_selection_on_window();
    }
    if (seqev.m_painting)
    {
        seqev.m_current_x = int(a_ev->x) + seqev.m_scroll_offset_x;
        seqev.snap_x(seqev.m_current_x);
        seqev.convert_x(seqev.m_current_x, tick);
        seqev.drop_event(tick);
        result = true;
    }
    return result;
}

}           // namespace seq64

/*
 * seq24seq.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

