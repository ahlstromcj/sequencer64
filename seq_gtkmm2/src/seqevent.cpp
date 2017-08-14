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
 * \updates       2017-08-14
 * \license       GNU GPLv2 or above
 *
 *  We are currently trying to get event processing to accomodate tempo
 *  events.
 */

#include <gtkmm/accelkey.h>
#include <gtkmm/adjustment.h>

#include "app_limits.h"                 /* SEQ64_SOLID_PIANOROLL_GRID   */
#include "event.hpp"
#include "gdk_basic_keys.h"
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
 *
 * \param ppqn
 *      The initial PPQN value.
 */

seqevent::seqevent
(
    perform & p,            // used only to satisfy gui_drawingarea_gtk2()
    sequence & seq,
    int zoom,
    int snap,
    seqdata & seqdata_wid,
    Gtk::Adjustment & hadjust,
    int ppqn
) :
    gui_drawingarea_gtk2     (p, hadjust, adjustment_dummy(), 10, c_eventarea_y),
    m_fruity_interaction     (),
    m_seq24_interaction      (),
    m_seq                    (seq),
    m_zoom                   (zoom),
    m_snap                   (snap),
    m_ppqn                   (0),
    m_old                    (),
    m_selected               (),
    m_scroll_offset_ticks    (0),
    m_scroll_offset_x        (0),
    m_seqdata_wid            (seqdata_wid),
    m_selecting              (false),
    m_moving_init            (false),
    m_moving                 (false),
    m_growing                (false),
    m_painting               (false),
    m_paste                  (false),
    m_move_snap_offset_x     (0),
    m_status                 (EVENT_NOTE_ON),
    m_cc                     (0)
{
    m_ppqn = choose_ppqn(ppqn);
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
 *  If the window is realized, this function creates a pixmap with window
 *  dimensions, the updates the pixmap, and queues up a redraw.  This ends up
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
 *  The build-time option SEQ64_SOLID_PIANOROLL_GRID causes solid lines to be
 *  drawn, in gray, instead of dotted black lines, for a smoother look.
 *
 *  Also, as a trial option, if the current data type is EVENT_NOTE_ON,
 *  EVENT_NOTE_OFF, and EVENT_AFTERTOUCH, we draw the background in light grey
 *  to remind the user that there are issues in copying or moving these events
 *  around (unlinked) by themselves.
 */

void
seqevent::draw_background ()
{

#ifdef SEQ64_SOLID_PIANOROLL_GRID
    Color minor_line_color = light_grey();
#else
    Color minor_line_color = grey();
#endif

    if (event::is_note_msg(m_status))
    {
        draw_rectangle_on_pixmap(light_grey(), 0, 0, m_window_x, m_window_y);
        minor_line_color = dark_grey();         /* or white()? black()? */
    }
    else
        draw_rectangle_on_pixmap(white(), 0, 0, m_window_x, m_window_y);

    int bpbar = m_seq.get_beats_per_bar();
    int bwidth = m_seq.get_beat_width();
    int ticks_per_beat = 4 * m_ppqn / bwidth;
    int ticks_per_major = bpbar * ticks_per_beat;
    int ticks_per_step = 6 * m_zoom;
    int endtick = (m_window_x * m_zoom) + m_scroll_offset_ticks;
    int starttick = m_scroll_offset_ticks -
        (m_scroll_offset_ticks % ticks_per_step);

    m_gc->set_foreground(grey());
    for (int tick = starttick; tick < endtick; tick += ticks_per_step)
    {
        int base_line = tick / m_zoom;
        if (tick % ticks_per_major == 0)       /* solid line on every beat */
        {
#ifdef SEQ64_SOLID_PIANOROLL_GRID
            set_line(Gdk::LINE_SOLID, 2);
#else
            set_line(Gdk::LINE_SOLID);
#endif
            m_gc->set_foreground(black());
        }
        else if (tick % ticks_per_beat == 0)
        {
            set_line(Gdk::LINE_SOLID);
            m_gc->set_foreground(grey());
        }
        else
        {
#ifdef SEQ64_SOLID_PIANOROLL_GRID

            /*
             * This better matches what the seqroll draws for vertical lines.
             */

            int tick_snap = tick - (tick % m_snap);
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
#else
            m_gc->set_foreground(grey());
            set_line(Gdk::LINE_ON_OFF_DASH);
            gint8 dash = 1;
            m_gc->set_dashes(0, &dash, 1);
#endif
        }
        int x = base_line - m_scroll_offset_x;
        draw_line_on_pixmap(x, 0, x, m_window_y);
    }
#ifdef SEQ64_SOLID_PIANOROLL_GRID
    set_line(Gdk::LINE_SOLID, 2);
#else
    set_line(Gdk::LINE_SOLID);
#endif
    draw_rectangle_on_pixmap
    (
        black(), -1, 0, m_window_x + 1, m_window_y - 1, false
    );
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
 *  This function exercises the new version of get_nexxt_event(),
 *  get_next_event_ex(), which allows (and forces) the caller to provide the
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
    while (m_seq.get_next_event_ex(m_status, m_cc, ev))
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
 *  This function currently just queues up a draw operation for the
 *  pixmap.
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
    draw_drawable                           /* replace old */
    (
        m_old.x, y, m_old.x, y, m_old.width + 1, h + 1
    );
    if (m_selecting)
    {
        x_to_w(m_drop_x, m_current_x, x, w);
        x -= m_scroll_offset_x;
        m_old.x = x;
        m_old.width = w;
#if 0
#ifdef SEQ64_USE_BLACK_SELECTION_BOX
        draw_rectangle(black(), x, y, w, h, false);
#else
        draw_rectangle(dark_orange(), x, y, w, h, false);
#endif
#endif
        draw_rectangle(sel_paint(), x, y, w, h, false);
    }
    if (m_moving || m_paste)
    {
        int delta_x = m_current_x - m_drop_x;
        x = m_selected.x + delta_x;
        x -= m_scroll_offset_x;
#if 0
#ifdef SEQ64_USE_BLACK_SELECTION_BOX
        draw_rectangle(black(), x, y, m_selected.width, h, false);
#else
        draw_rectangle(dark_orange(), x, y, m_selected.width, h, false);
#endif
#endif
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
 *  Drops (adds) an event at the given tick. It sets the first byte
 *  properly for after-touch, program-change, channel-pressure, and
 *  pitch-wheel.  The type of event is determined by m_status.
 *
 * \param tick
 *      The destination time (division, pulse, tick) for the event to be
 *      dropped at.
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
        if (! event::is_strict_note_msg(status))        /* a stazed fix     */
        {
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
        }
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
 * \param ev
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
    interaction_method_t interactionmethod = rc().interaction_method();
    switch (interactionmethod)
    {
    case e_fruity_interaction:
        (void) m_fruity_interaction.on_button_press_event(ev, *this);
        result = m_seq24_interaction.on_button_press_event(ev, *this);
        break;

    case e_seq24_interaction:
        result = m_seq24_interaction.on_button_press_event(ev, *this);
        break;

    default:
        break;
    }
    if (result)
        perf().modify();

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
    interaction_method_t interactionmethod = rc().interaction_method();
    switch (interactionmethod)
    {
    case e_fruity_interaction:
        (void) m_fruity_interaction.on_button_release_event(ev, *this);
        result = m_seq24_interaction.on_button_release_event(ev, *this);
        break;

    case e_seq24_interaction:
        result = m_seq24_interaction.on_button_release_event(ev, *this);
        break;

    default:
        break;
    }
    if (result)
        perf().modify();

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
    interaction_method_t interactionmethod = rc().interaction_method();
    switch (interactionmethod)
    {
    case e_fruity_interaction:
        (void) m_fruity_interaction.on_motion_notify_event(ev, *this);
        result = m_seq24_interaction.on_motion_notify_event(ev, *this);
        break;

    case e_seq24_interaction:
        result = m_seq24_interaction.on_motion_notify_event(ev, *this);
        break;

    default:
        break;
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
        if (ev->keyval == SEQ64_Delete || ev->keyval == SEQ64_BackSpace)
        {
            m_seq.cut_selected(false);      /* cut events without copying   */
            result = true;
        }
        if (is_ctrl_key(ev))
        {
            /*
             * Do we really need to test the capital letters?
             */

            if (ev->keyval == SEQ64_x || ev->keyval == SEQ64_X)     /* cut  */
            {
                m_seq.cut_selected();       /* cut events with copying      */
                result = true;
            }
            if (ev->keyval == SEQ64_c || ev->keyval == SEQ64_C)     /* copy */
            {
                m_seq.copy_selected();
                result = true;
            }
            if (ev->keyval == SEQ64_v || ev->keyval == SEQ64_V)    /* paste */
            {
                start_paste();              /* also calls perf().modify()   */
                result = true;
            }
            if (ev->keyval == SEQ64_z || ev->keyval == SEQ64_Z)     /* Undo */
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
            if (ev->keyval == SEQ64_p)
            {
                m_seq24_interaction.set_adding(true, *this);
                result = true;
            }
            else if (ev->keyval == SEQ64_x)         /* "x-scape" the mode   */
            {
                m_seq24_interaction.set_adding(false, *this);
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

