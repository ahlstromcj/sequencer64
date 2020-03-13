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
 * \file          seqevent.cpp
 *
 *  This module declares/defines the base class for the event pane.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2018-10-29
 * \license       GNU GPLv2 or above
 *
 *  We are currently trying to get event processing to accomodate tempo
 *  events.
 */

#include <gtkmm/accelkey.h>
#include <gtkmm/adjustment.h>
#include <gdkmm/cursor.h>               /* Gdk::Cursor(Gdk::PENCIL)     */

#include "click.hpp"                    /* SEQ64_CLICK_LEFT, etc.       */
#include "event.hpp"
#include "keystroke.hpp"                /* instead of gdk_basic_keys.h  */
#include "gui_key_tests.hpp"            /* seq64::is_no_modifier()      */
#include "perform.hpp"
#include "seqevent.hpp"
#include "seqdata.hpp"
#include "sequence.hpp"
#include "settings.hpp"                 /* seq64::rc_settings items     */

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *  Principal constructor.
 *
 * \param p
 *      The "parent" perform object controlling all of the sequences.
 *
 * \param seq
 *      The current sequence operated on by this object.
 *
 * \param zoom
 *      The initial zoom value.
 *
 * \param snap
 *      The initial snap value.
 *
 * \param seqdata_wid
 *      The data pane that this event pane is associated with.
 *
 * \param hadjust
 *      The horizontal scroll-bar.
 */

seqevent::seqevent
(
    perform & p,            // used only to satisfy gui_drawingarea_gtk2()
    sequence & seq,
    int zoom,
    int snap,
    seqdata & seqdata_wid,
    Gtk::Adjustment & hadjust
) :
    gui_drawingarea_gtk2    (p, hadjust, adjustment_dummy(), 10, c_eventarea_y),
    m_seq                   (seq),
    m_zoom                  (zoom),
    m_snap                  (snap),
    m_old                   (),
    m_selected              (),
    m_scroll_offset_ticks   (0),
    m_scroll_offset_x       (0),
    m_seqdata_wid           (seqdata_wid),
    m_adding                (false),
    m_selecting             (false),
    m_moving_init           (false),
    m_moving                (false),
    m_growing               (false),
    m_painting              (false),
    m_paste                 (false),
    m_move_snap_offset_x    (0),
    m_status                (EVENT_NOTE_ON),
    m_cc                    (0)
{
    memset(&m_old, 0, sizeof m_old);        /* from seq24 0.9.3 */
}

/**
 *  Changes the horizontal scrolling offset for ticks, then updates the
 *  pixmap and forces a redraw.  Very similar to seqroll::change_horz().
 *  Basically identical to seqdata::change_horz().
 */

void
seqevent::change_horz ()
{
    m_scroll_offset_ticks = int(m_hadjust.get_value());
    m_scroll_offset_x = m_scroll_offset_ticks / m_zoom;
    update_pixmap();
    force_draw();
}

/**
 *  Implements redraw while idling.  Who calls this routine?  Probably the
 *  default timer routine, but not sure.
 *
 * \return
 *      Always returns true.
 */

int
seqevent::idle_redraw ()
{
    draw_events_on(m_window);
    draw_events_on(m_pixmap);
    return true;
}

/**
 *  If the window is realized, this function creates a pixmap with the window
 *  dimensions, then updates the pixmap, and queues up a redraw.  This ends up
 *  filling the background with dotted lines, etc.
 */

void
seqevent::update_sizes ()
{
    if (is_realized())
    {
        m_pixmap = Gdk::Pixmap::create(m_window, m_window_x, m_window_y, -1);
        update_pixmap();
        queue_draw();
    }
}

/**
 *  This function basically resets the whole widget as if it was realized
 *  again.  Basically identical to seqtime::reset().
 */

void
seqevent::reset ()
{
    m_scroll_offset_ticks = int(m_hadjust.get_value());
    m_scroll_offset_x = m_scroll_offset_ticks / m_zoom;
    update_sizes();
    update_pixmap();
    draw_pixmap_on_window();
}

/**
 *  Adjusts the scrolling offset for ticks, updates the pixmap, and draws
 *  it on the window.  Somewhat similar to seqroll::redraw().
 */

void
seqevent::redraw ()
{
    m_scroll_offset_ticks = int(m_hadjust.get_value());
    m_scroll_offset_x = m_scroll_offset_ticks / m_zoom;
    update_pixmap();
    draw_pixmap_on_window();
}

/**
 *  This function updates the background.  It sets the foreground to
 *  white, draws the rectangle, in order to clear the pixmap.
 *  The build now causes solid lines to be drawn, in gray, instead of dotted
 *  black lines, for a smoother look.
 *
 *  Also, as a trial option, if the current data type is EVENT_NOTE_ON,
 *  EVENT_NOTE_OFF, and EVENT_AFTERTOUCH, we draw the background in light grey
 *  to remind the user that there are issues in copying or moving these events
 *  around (unlinked) by themselves.
 */

void
seqevent::draw_background ()
{
    Color minor_line_color = light_grey_paint();
    if (event::is_note_msg(m_status))
    {
        draw_rectangle_on_pixmap(light_grey_paint(), 0, 0, m_window_x, m_window_y);
        minor_line_color = dark_grey_paint();           /* white()? black()? */
    }
    else
        draw_rectangle_on_pixmap(white(), 0, 0, m_window_x, m_window_y);

    int bpbar = m_seq.get_beats_per_bar();
    int bwidth = m_seq.get_beat_width();
    int ticks_per_beat = 4 * perf().get_ppqn() / bwidth;
    int ticks_per_major = bpbar * ticks_per_beat;
    int ticks_per_step = 6 * m_zoom;
    int endtick = (m_window_x * m_zoom) + m_scroll_offset_ticks;
    int starttick = m_scroll_offset_ticks -
        (m_scroll_offset_ticks % ticks_per_step);

    m_gc->set_foreground(grey_paint());
    for (int tick = starttick; tick < endtick; tick += ticks_per_step)
    {
        int base_line = tick / m_zoom;
        if (tick % ticks_per_major == 0)            /* solid line every beat */
        {
            set_line(Gdk::LINE_SOLID, 2);
            m_gc->set_foreground(black());
        }
        else if (tick % ticks_per_beat == 0)
        {
            set_line(Gdk::LINE_SOLID);
            m_gc->set_foreground(grey_paint());
        }
        else
        {
            /*
             * This better matches what the seqroll draws for vertical lines.
             */

            // int tick_snap = tick - (tick % m_snap);
            int tick_snap = tick;
            if (m_snap > 0)
                tick_snap -= tick % m_snap;

            if (tick == tick_snap)
            {
                set_line(Gdk::LINE_SOLID);
                m_gc->set_foreground(minor_line_color);
            }
            else
            {
                set_line(Gdk::LINE_ON_OFF_DASH);
                m_gc->set_foreground(minor_line_color);
                gint8 dash = 1;
                m_gc->set_dashes(0, &dash, 1);
            }
        }
        int x = base_line - m_scroll_offset_x;
        draw_line_on_pixmap(x, 0, x, m_window_y);
    }
    set_line(Gdk::LINE_SOLID, 2);

    int wx = m_window_x + 1;
    int wy = m_window_y - 1;
    draw_rectangle_on_pixmap(black(), -1, 0, wx, wy, false);
}

/**
 *  Sets zoom to the given value, and resets if the value ended up being
 *  changed.
 *
 * \param z
 *      The desired zoom value, assumed to be validated already.  See the
 *      seqedit::set_zoom() function.
 */

void
seqevent::set_zoom (int z)
{
    if (m_zoom != z)
    {
        m_zoom = z;
        reset();
    }
}

/**
 *  Changes the mouse cursor to a pencil or a left pointer in the given
 *  seqevent object, depending on the first parameter.  Modifies m_adding
 *  as well.
 *
 * \param adding
 *      The value to set m_adding to, and if true, sets the mouse cursor to a
 *      pencil icon, otherwise sets it to a standard mouse-pointer icon.
 */

void
seqevent::set_adding (bool adding)
{
    m_adding = adding;
    if (adding)
        get_window()->set_cursor(Gdk::Cursor(Gdk::PENCIL));
    else
        get_window()->set_cursor(Gdk::Cursor(Gdk::LEFT_PTR));
}

/**
 *  Sets the status to the given parameter, and the CC value to the given
 *  optional control parameter, which defaults to 0.  Then redraws.
 *
 * \param status
 *      The status/event byte to set.  For example, EVENT_NOTE_ON and
 *      EVENT_NOTE off.  This byte should have the channel nybble cleared.
 *
 * \param control
 *      The MIDI CC byte to set.
 */

void
seqevent::set_data_type (midibyte status, midibyte control)
{
    m_status = status;
    m_cc = control;
    redraw();
}

/**
 *  Redraws the background pixmap on the main pixmap, then puts the events on.
 */

void
seqevent::update_pixmap ()
{
    draw_background();
    draw_events_on_pixmap();
    m_seqdata_wid.update_pixmap();
    m_seqdata_wid.draw_pixmap_on_window();
}

/**
 *  Draws events on the given drawable object.  Very similar to
 *  seqdata::draw_events_on().
 *
 *  This function exercises the new version of get_next_event(),
 *  get_next_event_match(), which allows (and forces) the caller to provide the
 *  event iterator.
 *
 * \param drawable
 *      The given drawable object.
 */

void
seqevent::draw_events_on (Glib::RefPtr<Gdk::Drawable> drawable)
{
    int starttick = m_scroll_offset_ticks;
    int endtick = (m_window_x * m_zoom) + m_scroll_offset_ticks;
    event_list::const_iterator ev;
    m_seq.reset_ex_iterator(ev);
    m_gc->set_foreground(black_paint());
    while (m_seq.get_next_event_match(m_status, m_cc, ev))
    {
        midipulse tick = ev->get_timestamp();
        bool selected = ev->is_selected();
        if (tick >= starttick && tick <= endtick)
        {
            int x = tick / m_zoom - m_scroll_offset_x;  /* screen coord     */
            draw_rectangle                              /* outer border     */
            (
                drawable, ev->is_tempo() ? tempo_paint() : black(),
                x, c_eventpadding_y, c_eventevent_x, c_eventevent_y
            );
            draw_rectangle                              /* inner color      */
            (
                drawable, selected ? orange() : white(),
                x, c_eventpadding_y+1, c_eventevent_x-3, c_eventevent_y-3
            );
        }
        ++ev;                                           /* now a must-do    */
    }
}

/**
 *  This function fills the main pixmap with events.
 */

void
seqevent::draw_events_on_pixmap ()
{
    draw_events_on(m_pixmap);
}

/**
 *  This function currently just queues up a draw operation for the pixmap.
 *
 *  Old comments:
 *
 *      It then tells event to do the same.  We changed something on this
 *      window, and chances are we need to update the event widget as well and
 *      update our velocity window.
 */

void
seqevent::draw_pixmap_on_window ()
{
    queue_draw();
}

/**
 *  This function checks the mins / maxes.  Then it fills in x
 *  and the width.
 *
 * \param x1
 *      The "left" x value.
 *
 * \param x2
 *      The "right" x value.
 *
 * \param [out] x
 *      The destination for the converted x value.
 *
 * \param [out] w
 *      The destination for the converted width value.
 */

void
seqevent::x_to_w (int x1, int x2, int & x, int & w)
{
    if (x1 < x2)
    {
        x = x1;
        w = x2 - x1;
    }
    else
    {
        x = x2;
        w = x1 - x2;
    }
}

/**
 *  Draw the selected events on the window.
 */

void
seqevent::draw_selection_on_window ()
{
    int x, w;
    int y = (c_eventarea_y - c_eventevent_y) / 2;
    int h = c_eventevent_y;
    set_line(Gdk::LINE_SOLID);

    /* replace old */

    draw_drawable(m_old.x, y, m_old.x, y, m_old.width + 1, h + 1);
    if (m_selecting)
    {
        x_to_w(m_drop_x, m_current_x, x, w);
        x -= m_scroll_offset_x;
        m_old.x = x;
        m_old.width = w;
        draw_rectangle(sel_paint(), x, y, w, h, false);
    }
    if (m_moving || m_paste)
    {
        int delta_x = m_current_x - m_drop_x;
        x = m_selected.x + delta_x;
        x -= m_scroll_offset_x;
        draw_rectangle(sel_paint(), x, y, m_selected.width, h, false);
        m_old.x = x;
        m_old.width = m_selected.width;
    }
}

/**
 *  Forces a draw on the current drawable area of the window.
 */

void
seqevent::force_draw ()
{
    gui_drawingarea_gtk2::force_draw();
    draw_selection_on_window();
}

/**
 *  Starts a paste operation.  It gets the clipboard box that selected
 *  elements are in, makes a coordinate conversion, and then, sets the
 *  m_selected rectangle to hold the (x,y,w,h) of the selected events.
 */

void
seqevent::start_paste ()
{
    snap_x(m_current_x);
    snap_y(m_current_x);
    m_drop_x = m_current_x;                 /* protected member access   */
    m_drop_y = m_current_y;
    m_paste = true;

    midipulse tick_s, tick_f;
    int note_h, note_l;
    m_seq.get_clipboard_box(tick_s, note_h, tick_f, note_l);

    int x, w;
    convert_t(tick_s, x);                   /* convert box to X,Y values */
    convert_t(tick_f, w);
    w -= x;                                 /* w is coordinates now      */
    m_selected.x = x;                       /* set the new selection     */
    m_selected.width = w;
    m_selected.y = (c_eventarea_y - c_eventevent_y) / 2;
    m_selected.height = c_eventevent_y;
    m_selected.x  += m_drop_x;              /* shift clipboard to tick 0 */
}

/**
 *  This function performs a 'snap' on x.
 *
 *      -   snap = number pulses to snap to
 *      -   m_zoom = number of pulses per pixel
 *      -   Therefore snap / m_zoom = number of pixels to snap to.
 *
 * \param [out] x
 *      The output destination for the snapped x value.
 */

void
seqevent::snap_x (int & x)
{
    int mod = (m_snap / m_zoom);
    if (mod <= 0)
        mod = 1;

    x -= (x % mod);       // *a_x = *a_x - (*a_x % mod);
}

/**
 *  Drops (adds) an event at the given tick. It sets the first byte properly
 *  for after-touch, program-change, channel-pressure, and pitch-wheel.  The
 *  type of event is determined by m_status.
 *
 * \param tick
 *      The destination time (division, pulse, tick) for the event to be
 *      dropped at.
 *
 * \param istempo
 *      Indicates if the event is a tempo event.  If so, we add a fake tempo
 *      event of BPM = 120.  Defaults to false.
 */

void
seqevent::drop_event (midipulse tick, bool istempo)
{
    if (istempo)
    {
        seq64::event e = create_tempo_event(tick, 120.0);   /* event.cpp */
        m_seq.add_event(e);
        m_seq.link_tempos();
    }
    else
    {
        midibyte status = m_status;

        /*
         * This fix breaks drawing events in the event pane.  Commented out.
         *
         * if (! event::is_strict_note_msg(status))     // a stazed fix
         * {
         */

            midibyte d0 = m_cc;
            midibyte d1 = 0x40;
            if (status == EVENT_AFTERTOUCH)
                d0 = 0;
            else if (status == EVENT_PROGRAM_CHANGE)
                d0 = 0;                                 /* d0 == new patch  */
            else if (status == EVENT_CHANNEL_PRESSURE)
                d0 = 0x40;                              /* d0 == pressure   */
            else if (status == EVENT_PITCH_WHEEL)
                d0 = 0;

            m_seq.add_event(tick, status, d0, d1, true);
    /*
     *  }
     */
    }
}

/**
 *  Implements the on-realize callback.  It calls the base-class version,
 *  and then allocates additional resource not allocated in the
 *  constructor.  Finally, it connects up the change_horz function.
 */

void
seqevent::on_realize ()
{
    gui_drawingarea_gtk2::on_realize();
    set_flags(Gtk::CAN_FOCUS);
    m_hadjust.signal_value_changed().connect
    (
        mem_fun(*this, &seqevent::change_horz)
    );
    update_sizes();
}

/**
 *  Implements the on-size-allocate event callback.  The m_window_x and
 *  m_window_y values are set to the allocation width and height,
 *  respectively.
 *
 * \param a
 *      The allocation to be processed.
 */

void
seqevent::on_size_allocate (Gtk::Allocation & a)
{
    gui_drawingarea_gtk2::on_size_allocate(a);
    m_window_x = a.get_width();
    m_window_y = a.get_height();
    update_sizes();
}

/**
 *  Implements the on-expose event callback.
 *
 * \param e
 *      The expose event.
 */

bool
seqevent::on_expose_event (GdkEventExpose * e)
{
    draw_drawable
    (
        e->area.x, e->area.y, e->area.x, e->area.y, e->area.width, e->area.height
    );
    draw_selection_on_window();
    return true;
}

/**
 *  Implements the on-button-press event callback.  It distinguishes
 *  between the Seq24 and Fruity varieties of mouse interaction.
 *
 *  Odd.  In the legacy code, each case fell through to the next case to the
 *  "default" case! We will assume for now that this is incorrect.
 *
 *  Note that returning "true" from a Gtkmm event-handler stops the
 *  propagation of the event to higher-level widgets.  The Fruity and Seq24
 *  event handlers return true, always.  In the legacy code, though, the
 *  fall-through code caused false to be returned, always.  Not sure what
 *  effect this had.  Added some fixes, but then commented them out until
 *  better testing can be done.
 *
 * \param ev
 *      The button event.
 *
 * \return
 *      Returns true if the button-press was handled.  Not sure the return
 *      code is meaningful.
 */

bool
seqevent::on_button_press_event (GdkEventButton * ev)
{
    bool result = false;
    midipulse tick_s, tick_w;
    grab_focus();
    convert_x(c_eventevent_x, tick_w);
    set_current_drop_x(int(ev->x + m_scroll_offset_x));
    m_old.x = m_old.y = m_old.width = m_old.height = 0;
    if (m_paste)
    {
        snap_x(m_current_x);
        convert_x(m_current_x, tick_s);
        m_paste = false;
        m_seq.paste_selected(tick_s, 0);      /* handles undo & modify    */
        result = true;
    }
    else
    {
        int x, w;
        midipulse tick_f;
        if (SEQ64_CLICK_LEFT(ev->button))
        {
            convert_x(m_drop_x, tick_s); /* x,y in to tick/note     */
            tick_f = tick_s + m_zoom;          /* shift back a few ticks  */
            tick_s -= tick_w;
            if (tick_s < 0)
                tick_s = 0;

            int eventcount = 0;
            if (m_adding)
            {
                m_painting = true;
                snap_x(m_drop_x);
                convert_x(m_drop_x, tick_s); /* x,y to tick/note    */
                eventcount = m_seq.select_events
                (
                    tick_s, tick_f, m_status, m_cc,
                    sequence::e_would_select
                );
                if (eventcount == 0)
                {
                    /*
                     * Add the event at the appropriate place.  As a new
                     * feature, if the Ctrl key is held, make it a Set Tempo
                     * event.
                     */

                    bool maketempo = is_ctrl_key(ev);
                    m_seq.push_undo();
                    drop_event(tick_s, maketempo);
                    result = true;
                }
            }
            else                                        /* selecting */
            {
                eventcount = m_seq.select_events
                (
                    tick_s, tick_f, m_status,
                    m_cc, sequence::e_is_selected
                );

#ifdef USE_STAZED_SELECTION_EXTENSIONS

                /*
                 * Stazed fix: if we didn't select anything (user clicked empty
                 * space), then unselect all notes, and start selecting.
                 *
                 * DOES THIS BREAK EVENT CREATION??????????
                 */

                if (event::is_strict_note_msg(m_status))
                {
                    m_seq.select_linked(tick_s, tick_f, m_status);
                    m_seq.set_dirty();
                }
#endif

                if (eventcount == 0)
                {
                    if (! is_ctrl_key(ev))
                        m_seq.unselect();

                    eventcount = m_seq.select_events
                    (
                        tick_s, tick_f, m_status, m_cc, sequence::e_select_one
                    );

                    /*
                     * If nothing selected (user clicked empty space),
                     * unselect all notes, and start selecting with a new
                     * selection box.
                     */

                    if (eventcount == 0)
                    {
                        m_selecting = true;
                    }
                    else
                    {
                        /**
                         * Needs update.
                         * m_seq.unselect();  ???????
                         */
                    }
                }
                eventcount = m_seq.select_events
                (
                    tick_s, tick_f, m_status, m_cc, sequence::e_is_selected
                );
                if (eventcount > 0)             /* get box selections are in */
                {
                    int note;
                    m_moving_init = true;
                    m_seq.get_selected_box(tick_s, note, tick_f, note);
                    tick_f += tick_w;
                    convert_t(tick_s, x); /* convert box to X,Y values */
                    convert_t(tick_f, w);
                    w -= x;                     /* w is coordinate now       */

                    /* set the m_selected rectangle for x,y,w,h */

                    m_selected.x = x;
                    m_selected.width = w;
                    m_selected.y = (c_eventarea_y - c_eventevent_y) / 2;
                    m_selected.height = c_eventevent_y;

                    /* save offset that we get from the snap above */

                    int adjusted_selected_x = m_selected.x;
                    snap_x(adjusted_selected_x);
                    m_move_snap_offset_x =
                        m_selected.x - adjusted_selected_x;

                    /* align selection for drawing */

                    snap_x(m_selected.x);
                    snap_x(m_current_x);
                    snap_x(m_drop_x);
                }
            }
        }
        if (SEQ64_CLICK_RIGHT(ev->button))
            set_adding(true);
    }

    if (result)
    {
        update_pixmap();          /* if they clicked, something changed */
        draw_pixmap_on_window();
        perf().modify();
    }
    return result;
}

/**
 *  Implements the on-button-release event callback.  It distinguishes
 *  between the Seq24 and Fruity varieties of mouse interaction.
 *
 *  Odd.  The fruity case fell through to the Seq24 case.  We will assume
 *  for now that this is correct.  Added some fixes, but then commented them
 *  out until better testing can be done.
 *
 * \param ev
 *      The button event.
 *
 * \return
 *      Returns true if the button-press was handled.
 */

bool
seqevent::on_button_release_event (GdkEventButton * ev)
{
    bool result = false;
    midipulse tick_s, tick_f;
    grab_focus();
    m_current_x = int(ev->x) + m_scroll_offset_x;
    if (m_moving)
        snap_x(m_current_x);

    int delta_x = m_current_x - m_drop_x;
    midipulse delta_tick;
    if (SEQ64_CLICK_LEFT(ev->button))
    {
        if (m_selecting)
        {
            int x, w;
            x_to_w(m_drop_x, m_current_x, x, w);
            convert_x(x, tick_s);
            convert_x(x + w, tick_f);
            (void) m_seq.select_events
            (
                tick_s, tick_f, m_status, m_cc, sequence::e_select
            );

#ifdef USE_STAZED_SELECTION_EXTENSIONS

            /*
             * Stazed fix: if we didn't select anything (user clicked empty
             * space), then unselect all notes, and start selecting.
             */

            if (event::is_strict_note_msg(m_status))
            {
                m_seq.select_linked(tick_s, tick_f, m_status);
            }
            m_seq.set_dirty();
#endif

        }
        if (m_moving)
        {
            delta_x -= m_move_snap_offset_x;        /* adjust for snap       */
            convert_x(delta_x, delta_tick);         /* to screen coordinates */
            m_seq.move_selected_notes(delta_tick, 0);
            result = true;
        }
        set_adding(m_adding);
    }
    if (SEQ64_CLICK_RIGHT(ev->button))
    {
        set_adding(false);
    }
    m_selecting = false;                      /* turn off              */
    m_moving = false;
    m_growing = false;
    m_moving_init = false;
    m_painting = false;
    m_seq.unpaint_all();
    update_pixmap();                  /* if a click, something changed */
    draw_pixmap_on_window();
    return result;
}

/**
 *  Implements the on-motion-notify event callback.  It distinguishes
 *  between the Seq24 and Fruity varieties of mouse interaction.
 *
 *  Odd.  The fruity case fell through to the Seq24 case.  We will assume
 *  for now that this is correct.  Added some fixes, but then commented them
 *  out until better testing can be done.
 *
 * \param ev
 *      The motion event.
 *
 * \return
 *      Returns true if the motion-event was handled.
 */

bool
seqevent::on_motion_notify_event (GdkEventMotion * ev)
{
    bool result = false;
    if (m_moving_init)
    {
        m_moving_init = false;
        m_moving = true;
    }
    if (m_selecting || m_moving || m_paste)
    {
        m_current_x = int(ev->x) + m_scroll_offset_x;
        if (m_moving || m_paste)
            snap_x(m_current_x);

        draw_selection_on_window();
    }
    if (m_painting)
    {
        midipulse tick = 0;
        m_current_x = int(ev->x) + m_scroll_offset_x;
        snap_x(m_current_x);
        convert_x(m_current_x, tick);
        drop_event(tick);
        result = true;
    }
    if (result)
        perf().modify();

    return result;
}

/**
 *  Responds to a focus event by setting the HAS_FOCUS flag.
 *  Parameter "ev" is the focus event, unused.
 *
 * \return
 *      Always returns false.
 */

bool
seqevent::on_focus_in_event (GdkEventFocus * /*ev*/)
{
    set_flags(Gtk::HAS_FOCUS);
    return false;
}

/**
 *  Responds to a unfocus event by resetting the HAS_FOCUS flag.
 *  Parameter "ev" is the focus event, unused.
 *
 * \return
 *      Always returns false.
 */

bool
seqevent::on_focus_out_event (GdkEventFocus * /*ev*/)
{
    unset_flags(Gtk::HAS_FOCUS);
    return false;
}

/**
 *  Implements the key-press event callback function.  It handles deleting a
 *  selection via the Backspace or Delete keys, cut via Ctrl-X, copy via
 *  Ctrl-C, paste via Ctrl-V, and undo via Ctrl-Z.  Would be nice to provide
 *  redo functionality via Ctrl-Y.  :-)
 *
 * \param ev
 *      The key-press event.
 *
 * \return
 *      Returns true if an event was handled.  Only some of the handled events
 *      also cause the perform modification flag to be set as a side-effect.
 */

bool
seqevent::on_key_press_event (GdkEventKey * ev)
{
    bool result = false;
    if (CAST_EQUIVALENT(ev->type, SEQ64_KEY_PRESS))
    {
        keystroke k(ev->keyval, SEQ64_KEYSTROKE_PRESS);
        if (k.is(SEQ64_Delete, SEQ64_BackSpace))
        {
            m_seq.cut_selected(false);      /* cut events without copying   */
            result = true;
        }
        if (is_ctrl_key(ev))
        {
            /*
             * Do we really need to test the capital letters?
             */

            if (k.is(SEQ64_x, SEQ64_X))     /* cut  */
            {
                m_seq.cut_selected();       /* cut events with copying      */
                result = true;
            }
            if (k.is(SEQ64_c, SEQ64_C))     /* copy */
            {
                m_seq.copy_selected();
                result = true;
            }
            if (k.is(SEQ64_v, SEQ64_V))     /* paste */
            {
                start_paste();              /* also calls perf().modify()   */
                result = true;
            }
            if (k.is(SEQ64_z, SEQ64_Z))     /* Undo */
            {
                m_seq.pop_undo();   // how to detect all modifications undone?
                result = true;
            }

            /*
             * No "Redo" support at present.
             */
        }
        if (! result)
        {
            if (k.is(SEQ64_p))
            {
                // WORK THIS OUT !!!!!!!!!!!
                // m_seq24_interaction.set_adding(true, *this);
                set_adding(true);
                result = true;
            }
            else if (k.is(SEQ64_x))         /* "x-scape" the mode   */
            {
                // WORK THIS OUT !!!!!!!!!!!
                // m_seq24_interaction.set_adding(false, *this);
                set_adding(false);
                result = true;
            }
        }
    }
    if (result)
    {
        redraw();
        m_seq.set_dirty();
    }
    return result;
}

}           // namespace seq64

/*
 * seqevent.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

