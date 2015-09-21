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
 * \file          gui_drawingarea.cpp
 *
 *  This module declares/defines the base class for drawing on
 *  Gtk::DrawingArea.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-09-21
 * \updates       2015-09-21
 * \license       GNU GPLv2 or above
 *
 */

#if 0
#include <gtkmm/accelkey.h>
#include <gtkmm/adjustment.h>
#include <gtkmm/button.h>
#include <gtkmm/window.h>
#include <gtkmm/accelgroup.h>
#include <gtkmm/box.h>
#include <gtkmm/main.h>
#include <gtkmm/menu.h>
#include <gtkmm/menubar.h>
#include <gtkmm/eventbox.h>
#include <gtkmm/window.h>
#include <gtkmm/table.h>
#include <gtkmm/drawingarea.h>
#include <gtkmm/widget.h>
#include <gtkmm/scrollbar.h>
#include <gtkmm/viewport.h>
#include <gtkmm/combo.h>
#include <gtkmm/label.h>
#include <gtkmm/toolbar.h>
#include <gtkmm/optionmenu.h>
#include <gtkmm/togglebutton.h>
#include <gtkmm/invisible.h>
#include <gtkmm/separator.h>
#include <gtkmm/tooltips.h>             // #include <gtkmm/tooltip.h>
#include <gtkmm/invisible.h>
#include <gtkmm/arrow.h>
#include <gtkmm/image.h>
#include <sigc++/bind.h>
#endif  // 0

#include "gui_drawingarea.hpp"

namespace seq64
{

/**
 *  Principal constructor.
 */

gui_drawingarea::gui_drawingarea
(
    perform * a_perf,
    int a_zoom,
    int a_snap,
    int a_pos,
    Gtk::Adjustment * a_hadjust,
    Gtk::Adjustment * a_vadjust
) :
    Gtk::DrawingArea        (),
    gui_base                (),
    m_gc                    (),
    m_window                (),
    m_black                 (Gdk::Color("black")),
    m_white                 (Gdk::Color("white")),
    m_grey                  (Gdk::Color("gray")),
    m_dk_grey               (Gdk::Color("gray50")),
    m_red                   (Gdk::Color("orange")),
    m_pixmap                (),
    m_mainperf              (a_perf),
    m_window_x              (10),       // why so small?
    m_window_y              (10),
    m_current_x             (0),
    m_current_y             (0),
    m_drop_x                (0),
    m_drop_y                (0),
    m_vadjust               (a_vadjust),
    m_hadjust               (a_hadjust),
    m_background            (),         // another pixmap
    m_old                   (),
    m_selected              (),
    m_pos                   (a_pos),
    m_zoom                  (a_zoom),   //
    m_snap                  (a_snap),   //
    m_selecting             (false),
    m_moving                (false),
    m_moving_init           (false),
    m_growing               (false),
    m_painting              (false),
    m_paste                 (false),
    m_is_drag_pasting       (false),
    m_is_drag_pasting_start (false),
    m_move_delta_x          (0),
    m_move_delta_y          (0),
    m_move_snap_offset_x    (0),
    m_old_progress_x        (0),
    m_scroll_offset_ticks   (0),
    m_scroll_offset_key     (0),
    m_scroll_offset_x       (0),
    m_scroll_offset_y       (0),
    m_ignore_redraw         (false)
{
    Glib::RefPtr<Gdk::Colormap> colormap = get_default_colormap();
    colormap->alloc_color(m_black);
    colormap->alloc_color(m_white);
    colormap->alloc_color(m_grey);
    colormap->alloc_color(m_dk_grey);
    colormap->alloc_color(m_red);
    add_events
    (
        Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK |
        Gdk::POINTER_MOTION_MASK | Gdk::KEY_PRESS_MASK |
        Gdk::KEY_RELEASE_MASK | Gdk::FOCUS_CHANGE_MASK |
        Gdk::ENTER_NOTIFY_MASK | Gdk::LEAVE_NOTIFY_MASK |
        Gdk::SCROLL_MASK
    );
    set_double_buffered(false);
}

/**
 *  Provides a destructor to delete allocated objects.
 */

gui_drawingarea::~gui_drawingarea ()
{
    // 
}

/**
 *  Change the horizontal scrolling offset and redraw.
 */

void
gui_drawingarea::change_horz ()
{
    m_scroll_offset_ticks = (int) m_hadjust->get_value();
    m_scroll_offset_x = m_scroll_offset_ticks / m_zoom;
    if (m_ignore_redraw)
        return;

    update_background();
    update_pixmap();
    force_draw();
}

/**
 *  Change the vertical scrolling offset and redraw.
 */

void
gui_drawingarea::change_vert ()
{
    m_scroll_offset_key = (int) m_vadjust->get_value();
    m_scroll_offset_y = m_scroll_offset_key * c_key_y;
    if (m_ignore_redraw)
        return;

    update_background();
    update_pixmap();
    force_draw();
}

/**
 *  This function basically resets the whole widget as if it was realized
 *  again.  It's almost identical to the change_horz() function!
 */

void
gui_drawingarea::reset ()
{
    m_scroll_offset_ticks = (int) m_hadjust->get_value();
    m_scroll_offset_x = m_scroll_offset_ticks / m_zoom;
    if (m_ignore_redraw)
        return;

    update_sizes();
    update_background();
    update_pixmap();
    queue_draw();
}

/**
 *  Redraws unless m_ignore_redraw is true.
 */

void
gui_drawingarea::redraw ()
{
    if (m_ignore_redraw)
        return;

    m_scroll_offset_ticks = (int) m_hadjust->get_value();
    m_scroll_offset_x = m_scroll_offset_ticks / m_zoom;
    update_background();
    update_pixmap();
    force_draw();
}

/**
 *  Redraws events unless m_ignore_redraw is true.
 */

void
gui_drawingarea::redraw_events ()
{
    if (m_ignore_redraw)
        return;

    update_pixmap();
    force_draw();
}

/**
 *  Draws the main pixmap.
 */

void
gui_drawingarea::draw_background_on_pixmap ()
{
    m_pixmap->draw_drawable
    (
        m_gc, m_background, 0, 0, 0, 0, m_window_x, m_window_y
    );
}

/**
 *  Updates the background of this window.
 */

void
gui_drawingarea::update_background ()
{
    m_gc->set_foreground(m_white);              /* clear background */
    m_background->draw_rectangle(m_gc, true, 0, 0, m_window_x, m_window_y);

    m_gc->set_foreground(m_grey);               /* draw horz grey lines */
    m_gc->set_line_attributes
    (
        1, Gdk::LINE_ON_OFF_DASH, Gdk::CAP_NOT_LAST, Gdk::JOIN_MITER
    );
    gint8 dash = 1;
    m_gc->set_dashes(0, &dash, 1);
    for (int i = 0; i < (m_window_y / c_key_y) + 1; i++)
    {
        int remkeys = c_num_keys - i;               /* remaining keys?      */
        int octkey = OCTAVE_SIZE - m_key;           /* used three times     */
        int modkey = (remkeys - m_scroll_offset_key + octkey);
        if (global_interactionmethod == e_fruity_interaction)
        {
            if ((modkey % OCTAVE_SIZE) == 0)
            {
                m_gc->set_foreground(m_dk_grey);    /* draw horz lines at C */
                m_gc->set_line_attributes
                (
                    1, Gdk::LINE_SOLID, Gdk::CAP_NOT_LAST, Gdk::JOIN_MITER
                );
            }
            else if ((modkey % OCTAVE_SIZE) == (OCTAVE_SIZE-1))
            {
                m_gc->set_foreground(m_grey); /* horz grey lines, other notes */
                m_gc->set_line_attributes
                (
                    1, Gdk::LINE_ON_OFF_DASH, Gdk::CAP_NOT_LAST, Gdk::JOIN_MITER
                );
            }
        }
        m_background->draw_line(m_gc, 0, i * c_key_y, m_window_x, i * c_key_y);
        if (m_scale != c_scale_off)
        {
            if (! c_scales_policy[m_scale][(modkey - 1) % OCTAVE_SIZE])
            {
                m_background->draw_rectangle
                (
                    m_gc, true, 0, i * c_key_y + 1, m_window_x, c_key_y - 1
                );
            }
        }
    }

    /*
     * int measure_length_64ths = m_seq->get_bpm() * 64 / m_seq->get_bw();
     * int measures_per_line = (256 / measure_length_64ths) / (32 / m_zoom);
     * if ( measures_per_line <= 0
     */

    int measures_per_line = 1;
    int ticks_per_measure =  m_seq->get_bpm() * (4 * c_ppqn) / m_seq->get_bw();
    int ticks_per_beat = (4 * c_ppqn) / m_seq->get_bw();
    int ticks_per_step = 6 * m_zoom;
    int ticks_per_m_line =  ticks_per_measure * measures_per_line;
    int end_tick = (m_window_x * m_zoom) + m_scroll_offset_ticks;
    int start_tick = m_scroll_offset_ticks -
         (m_scroll_offset_ticks % ticks_per_step);

    m_gc->set_foreground(m_grey);
    for (int i = start_tick; i < end_tick; i += ticks_per_step)
    {
        int base_line = i / m_zoom;
        if (i % ticks_per_m_line == 0)
        {
            m_gc->set_foreground(m_black);      /* solid line on every beat */
            m_gc->set_line_attributes
            (
                1, Gdk::LINE_SOLID, Gdk::CAP_NOT_LAST, Gdk::JOIN_MITER
            );
        }
        else if (i % ticks_per_beat == 0)
        {
            m_gc->set_foreground(m_dk_grey);
            m_gc->set_line_attributes
            (
                1, Gdk::LINE_SOLID, Gdk::CAP_NOT_LAST, Gdk::JOIN_MITER
            );
        }
        else
        {
            m_gc->set_line_attributes
            (
                1, Gdk::LINE_ON_OFF_DASH, Gdk::CAP_NOT_LAST, Gdk::JOIN_MITER
            );

            int i_snap = i - (i % m_snap);
            if (i == i_snap)
                m_gc->set_foreground(m_dk_grey);
            else
                m_gc->set_foreground(m_grey);

            gint8 dash = 1;
            m_gc->set_dashes(0, &dash, 1);
        }
        m_background->draw_line
        (
            m_gc, base_line - m_scroll_offset_x, 0,
            base_line - m_scroll_offset_x, m_window_y
        );
    }
    m_gc->set_line_attributes                   /* reset line style */
    (
        1, Gdk::LINE_SOLID, Gdk::CAP_NOT_LAST, Gdk::JOIN_MITER
    );
}

/**
 *  Sets the zoom to the given value, and then resets the view.
 */

void
gui_drawingarea::set_zoom (int a_zoom)
{
    if (m_zoom != a_zoom)
    {
        m_zoom = a_zoom;
        reset();
    }
}

/**
 *  This function draws the background pixmap on the main pixmap, and
 *  then draws the events on it.
 */

void
gui_drawingarea::update_pixmap ()
{
    draw_background_on_pixmap();
    draw_events_on_pixmap();
}

/**
 *  Fills the main pixmap with events.  Just calls draw_events_on().
 */

void
gui_drawingarea::draw_events_on_pixmap ()
{
    draw_events_on(m_pixmap);
}

/**
 *  Draw the events on the main window and on the pixmap.
 */

int
gui_drawingarea::idle_redraw ()
{
    draw_events_on(m_window);
    draw_events_on(m_pixmap);
    return true;
}

/**
 *  Set the pixmap into the window and then draws the selection on it.
 */

void
gui_drawingarea::force_draw ()
{
    m_window->draw_drawable(m_gc, m_pixmap, 0, 0, 0, 0, m_window_x, m_window_y);
    draw_selection_on_window();
}

/*
 *  This function takes the given x and y screen coordinates, and returns
 *  the note and the tick via the pointer parameters.  This function is
 *  the "inverse" of convert_tn().
 */

void
gui_drawingarea::convert_xy (int a_x, int a_y, long * a_tick, int * a_note)
{
    *a_tick = a_x * m_zoom;
    *a_note = (c_rollarea_y - a_y - 2) / c_key_y;
}

/**
 *  This function checks the mins / maxes, and then fills in the x, y,
 *  width, and height values.
 */

void
gui_drawingarea::xy_to_rect
(
    int a_x1, int a_y1, int a_x2, int a_y2,
    int * a_x, int * a_y, int * a_w, int * a_h)
{
    if (a_x1 < a_x2)
    {
        *a_x = a_x1;
        *a_w = a_x2 - a_x1;
    }
    else
    {
        *a_x = a_x2;
        *a_w = a_x1 - a_x2;
    }
    if (a_y1 < a_y2)
    {
        *a_y = a_y1;
        *a_h = a_y2 - a_y1;
    }
    else
    {
        *a_y = a_y2;
        *a_h = a_y1 - a_y2;
    }
}

/**
 *  Starts a paste operation.
 */

void
gui_drawingarea::start_paste()
{
    long tick_s;
    long tick_f;
    int note_h;
    int note_l;
    snap_x(&m_current_x);
    snap_y(&m_current_x);
    m_drop_x = m_current_x;
    m_drop_y = m_current_y;
    m_paste = true;

    /* get the box that selected elements are in */

    m_seq->get_clipboard_box(&tick_s, &note_h, &tick_f, &note_l);
    convert_tn_box_to_rect
    (
        tick_s, tick_f, note_h, note_l,
        &m_selected.x, &m_selected.y, &m_selected.width, &m_selected.height
    );

    /* adjust for clipboard being shifted to tick 0 */

    m_selected.x += m_drop_x;
    m_selected.y += (m_drop_y - m_selected.y);
}

/**
 *  Performs a 'snap' operation on the y coordinate.
 */

void
gui_drawingarea::snap_y (int * a_y)
{
    *a_y = *a_y - (*a_y % c_key_y);
}

/**
 *  Performs a 'snap' operation on the x coordinate.  This function is
 *  similar to snap_y(), but it calculates a modulo value from the snap
 *  and zoom settings.
 *
 *      -   m_snap = number pulses to snap to
 *      -   m_zoom = number of pulses per pixel
 *
 *  Therefore, m_snap / m_zoom = number pixels to snap to.
 */

void
gui_drawingarea::snap_x (int * a_x)
{
    int mod = (m_snap / m_zoom);
    if (mod <= 0)
        mod = 1;

    *a_x = *a_x - (*a_x % mod);
}

#if 0

/**
 *  Implements the on-realize event handling.
 */

void
gui_drawingarea::on_realize ()
{
    Gtk::DrawingArea::on_realize();
    set_flags(Gtk::CAN_FOCUS);
    m_window = get_window();
    m_gc = Gdk::GC::create(m_window);
    m_window->clear();
    m_hadjust->signal_value_changed().connect
    (
        mem_fun(*this, &gui_drawingarea::change_horz)
    );
    m_vadjust->signal_value_changed().connect
    (
        mem_fun(*this, &gui_drawingarea::change_vert)
    );
    update_sizes();
}

/**
 *  Implements the on-expose event handling.
 */

bool
gui_drawingarea::on_expose_event (GdkEventExpose * e)
{
    m_window->draw_drawable
    (
        m_gc, m_pixmap, e->area.x, e->area.y, e->area.x, e->area.y,
        e->area.width, e->area.height
    );
    draw_selection_on_window();
    return true;
}

/**
 *  Implements the on-button-press event handling.
 */

bool
gui_drawingarea::on_button_press_event (GdkEventButton * a_ev)
{
    bool result;
    switch (global_interactionmethod)
    {
    case e_fruity_interaction:
        result = m_fruity_interaction.on_button_press_event(a_ev, *this);
        break;

    case e_seq24_interaction:
        result = m_seq24_interaction.on_button_press_event(a_ev, *this);
        break;

    default:
        result = false;
        break;
    }
    return result;
}

/**
 *  Implements the on-button-release event handling.
 */

bool
gui_drawingarea::on_button_release_event (GdkEventButton * a_ev)
{
    bool result;
    switch (global_interactionmethod)
    {
    case e_fruity_interaction:
        result = m_fruity_interaction.on_button_release_event(a_ev, *this);
        break;

    case e_seq24_interaction:
        result = m_seq24_interaction.on_button_release_event(a_ev, *this);
        break;

    default:
        result = false;
        break;
    }
    return result;
}

/**
 *  Implements the on-motion-notify event handling.
 */

bool
gui_drawingarea::on_motion_notify_event (GdkEventMotion * a_ev)
{
    bool result;
    switch (global_interactionmethod)
    {
    case e_fruity_interaction:
        result = m_fruity_interaction.on_motion_notify_event(a_ev, *this);
        break;

    case e_seq24_interaction:
        result = m_seq24_interaction.on_motion_notify_event(a_ev, *this);
        break;

    default:
        result = false;
        break;
    }
    return result;
}

/**
 *  Implements the on-enter-notify event handling.
 */

bool
gui_drawingarea::on_enter_notify_event (GdkEventCrossing * a_p0)
{
    m_seqkeys_wid->set_hint_state(true);
    return false;
}

/**
 *  Implements the on-leave-notify event handling.
 */

bool
gui_drawingarea::on_leave_notify_event (GdkEventCrossing * a_p0)
{
    m_seqkeys_wid->set_hint_state(false);
    return false;
}

/**
 *  Implements the on-focus event handling.
 */

bool
gui_drawingarea::on_focus_in_event (GdkEventFocus *)
{
    set_flags(Gtk::HAS_FOCUS);
    return false;
}

/**
 *  Implements the on-unfocus event handling.
 */

bool
gui_drawingarea::on_focus_out_event (GdkEventFocus *)
{
    unset_flags(Gtk::HAS_FOCUS);
    return false;
}

/**
 *  Implements the on-key-press event handling.  The start/end key may be
 *  the same key (i.e. SPACEBAR).  Allow toggling when the same key is
 *  mapped to both triggers (i.e. SPACEBAR).
 */

bool
gui_drawingarea::on_key_press_event (GdkEventKey * a_p0)
{
    bool result = false;
    bool dont_toggle = PERFKEY(start) != PERFKEY(stop);
    if
    (
        a_p0->keyval ==  PERFKEY(start) &&
        (dont_toggle || ! global_is_pattern_playing)
    )
    {
        m_mainperf->position_jack(false);
        m_mainperf->start(false);
        m_mainperf->start_jack();
        global_is_pattern_playing = true;
    }
    else if
    (
        a_p0->keyval ==  PERFKEY(stop) &&
        (dont_toggle || global_is_pattern_playing)
    )
    {
        m_mainperf->stop_jack();
        m_mainperf->stop();
        global_is_pattern_playing = false;
    }
    if (a_p0->type == GDK_KEY_PRESS)
    {
        if (a_p0->keyval ==  GDK_Delete || a_p0->keyval == GDK_BackSpace)
        {
            m_seq->push_undo();
            m_seq->mark_selected();
            m_seq->remove_marked();
            result = true;
        }
        if (! global_is_pattern_playing)
        {
            if (a_p0->keyval == GDK_Home)
            {
                m_seq->set_orig_tick(0);
                result = true;
            }
            if (a_p0->keyval == GDK_Left)
            {
                m_seq->set_orig_tick(m_seq->get_last_tick() - m_snap);
                result = true;
            }
            if (a_p0->keyval == GDK_Right)
            {
                m_seq->set_orig_tick(m_seq->get_last_tick() + m_snap);
                result = true;
            }
        }
        if (a_p0->state & GDK_CONTROL_MASK)
        {
            if (a_p0->keyval == GDK_x || a_p0->keyval == GDK_X)     /* cut */
            {
                m_seq->push_undo();
                m_seq->copy_selected();
                m_seq->mark_selected();
                m_seq->remove_marked();
                result = true;
            }
            if (a_p0->keyval == GDK_c || a_p0->keyval == GDK_C)     /* copy */
            {
                m_seq->copy_selected();
                result = true;
            }
            if (a_p0->keyval == GDK_v || a_p0->keyval == GDK_V)     /* paste */
            {
                start_paste();
                result = true;
            }
            if (a_p0->keyval == GDK_z || a_p0->keyval == GDK_Z)     /* Undo */
            {
                m_seq->pop_undo();
                result = true;
            }
            if (a_p0->keyval == GDK_a || a_p0->keyval == GDK_A) /* select all */
            {
                m_seq->select_all();
                result = true;
            }
        }
    }
    if (result)
        m_seq->set_dirty();             // redraw_events();

    return result;
}

/**
 *  Implements the on-size-allocate event handling.
 */

void
gui_drawingarea::on_size_allocate (Gtk::Allocation & a_r)
{
    Gtk::DrawingArea::on_size_allocate(a_r);
    m_window_x = a_r.get_width();
    m_window_y = a_r.get_height();
    update_sizes();
}

/**
 *  Implements the on-scroll event handling.  This scroll event only
 *  handles basic scrolling without any modifier keys such as
 *  GDK_CONTROL_MASK or GDK_SHIFT_MASK.
 */

bool
gui_drawingarea::on_scroll_event (GdkEventScroll * a_ev)
{
    guint modifiers;                /* used to filter out caps/num lock etc. */
    modifiers = gtk_accelerator_get_default_mod_mask();
    if ((a_ev->state & modifiers) != 0)
        return false;

    double val = m_vadjust->get_value();
    if (a_ev->direction == GDK_SCROLL_UP)
        val -= m_vadjust->get_step_increment() / 6;
    else if (a_ev->direction == GDK_SCROLL_DOWN)
        val += m_vadjust->get_step_increment() / 6;
    else
        return true;

    m_vadjust->clamp_page(val, val + m_vadjust->get_page_size());
    return true;
}

#endif

}           // namespace seq64

/*
 * gui_drawingarea.cpp
 *
 * vim: sw=4 ts=4 wm=8 et ft=cpp
 */
