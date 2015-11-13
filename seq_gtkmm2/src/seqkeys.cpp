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
 * \file          seqkeys.cpp
 *
 *  This module declares/defines the base class for the left-side piano of
 *  the pattern/sequence panel.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2015-11-10
 * \license       GNU GPLv2 or above
 *
 */

#include <gtkmm/adjustment.h>

#include "app_limits.h"                 /* SEQ64_SOLID_PIANOROLL_GRID   */
#include "click.hpp"                    /* SEQ64_CLICK_LEFT() etc.     */
#include "font.hpp"
#include "scales.h"
#include "seqkeys.hpp"
#include "sequence.hpp"

namespace seq64
{

/**
 *  Principal constructor.
 */

seqkeys::seqkeys
(
    sequence & seq,
    perform & p,
    Gtk::Adjustment & vadjust
) :
    gui_drawingarea_gtk2
    (
        p, adjustment_dummy(), vadjust,
        c_keyarea_x + 1,                        // 36 + 1
        10                                      // bogus y window height
    ),
    m_seq                   (seq),
    m_scroll_offset_key     (0),
    m_scroll_offset_y       (0),
    m_hint_state            (false),
    m_hint_key              (0),
    m_keying                (false),
    m_keying_note           (0),
    m_scale                 (0),
    m_key                   (0)
{
    // Empty body
}

/**
 *  Sets the musical scale, then resets.
 */

void
seqkeys::set_scale (int scale)
{
    if (m_scale != scale)
    {
        m_scale = scale;
        reset();
    }
}

/**
 *  Sets the musical key, then resets.
 */

void
seqkeys::set_key (int key)
{
    if (m_key != key)
    {
        m_key = key;
        reset();
    }
}

/**
 *  Resetting the keys view updates the pixmap and queues up a draw
 *  operation.
 */

void
seqkeys::reset ()
{
    update_pixmap();
    queue_draw();
}

/**
 *  Updates the pixmaps to prepare it for the next draw operation.
 */

void
seqkeys::update_pixmap ()
{
    int kx = c_keyoffset_x + 1;
    draw_rectangle_on_pixmap(black(), 0, 0, c_keyarea_x, c_keyarea_y);
    draw_rectangle_on_pixmap(white(), 1, 1, c_keyoffset_x-1, c_keyarea_y-2);

    for (int key = 0; key < c_num_keys; ++key)
    {
        draw_rectangle_on_pixmap
        (
            white(), kx, (c_key_y * key) + 1, c_key_x - 2, c_key_y - 1
        );

        int okey = (c_num_keys - key - 1) % SEQ64_OCTAVE_SIZE;
        if (is_black_key(okey))
        {
            draw_rectangle_on_pixmap
            (
                black(), kx, (c_key_y * key) + 2, c_key_x - 3, c_key_y - 3
            );
        }

        char notes[8];
        if (okey == m_key)                       /* notes */
        {
            int octave = ((c_num_keys - key - 1) / SEQ64_OCTAVE_SIZE) - 1;
            if (octave < 0)
                octave *= -1;

            snprintf(notes, sizeof notes, "%2s%1d", c_key_text[okey], octave);
            render_string_on_pixmap(2, c_key_y * key - 1, notes, font::BLACK);
        }
    }
}

/**
 *  Draws the updated pixmap on the drawable area of the window where the
 *  keys' location is hardwired.
 */

void
seqkeys::draw_area()
{
    update_pixmap();
    draw_drawable(0, m_scroll_offset_y, 0, 0, c_keyarea_x, c_keyarea_y);
}

/**
 *  Forces a draw operation on the whole window.
 */

void
seqkeys::force_draw ()
{
    draw_drawable(0, m_scroll_offset_y, 0, 0, m_window_x, m_window_y);
}

/**
 *  Takes the screen y coordinate, and returns the note value in the
 *  second parameter.
 */

void
seqkeys::convert_y (int y, int & note)
{
    note = (c_rollarea_y - y - 2) / c_key_y;
}

/**
 *  Sets a key to grey so that it can serve as a scale hint.
 */

void
seqkeys::set_hint_key (int key)
{
    draw_key(m_hint_key, false);
    m_hint_key = key;
    if (m_hint_state)
        draw_key(key, true);
}

/**
 *  Sets the hint state to the given value.
 *
 * \param state
 *      Provides the value for hinting, where true == on, false == off.
 */

void
seqkeys::set_hint_state (bool state)
{
    m_hint_state = state;
    if (! state)
        draw_key(m_hint_key, false);
}

/**
 *  Draws the given key according to the given state.  It accounts for the
 *  black keys and the white keys, and for the highlighting of the active key.
 *
 * \param key
 *      The key to be drawn.
 *
 * \param state
 *      How the key is to be drawn, where false == normal, true == grayed.  A
 *      key is greyed when the mouse cursor is at the same vertical location
 *      on the piano as the key.
 */

void
seqkeys::draw_key (int key, bool state)
{
    int k = key % SEQ64_OCTAVE_SIZE;                /* key in the octave    */
    key = c_num_keys - key - 1;

    int x = c_keyoffset_x + 1;
    int y = (c_key_y * key) + 2 - m_scroll_offset_y;
    int w = c_key_x - 3;                            /* x length of key      */
    int h = c_key_y - 3;                            /* y height of key      */
    m_gc->set_foreground(is_black_key(k) ? black() : white());
    if (state)
        draw_rectangle(grey(), x, y, w, h);
    else
        draw_rectangle(x, y, w, h);
}

/**
 *  Changes the y offset of the scrolling, and the forces a draw.
 */

void
seqkeys::change_vert ()
{
    m_scroll_offset_key = int(m_vadjust.get_value());
    m_scroll_offset_y = m_scroll_offset_key * c_key_y,
    force_draw();
}

/**
 *  Implements the on-realize event.  Call the base-class version and then
 *  allocates resources that could not be allocated in the constructor.
 *  It connects the change_vert() function and then calls it.
 */

void
seqkeys::on_realize ()
{
    gui_drawingarea_gtk2::on_realize();
    m_pixmap = Gdk::Pixmap::create(m_window, c_keyarea_x, c_keyarea_y, -1);
    update_pixmap();
    m_vadjust.signal_value_changed().connect
    (
        mem_fun(*this, &seqkeys::change_vert)
    );
    change_vert();
}

/**
 *  Implements the on-expose event, by drawing on the window.
 */

bool
seqkeys::on_expose_event (GdkEventExpose * ev)
{
    draw_drawable
    (
        ev->area.x, ev->area.y + m_scroll_offset_y,
        ev->area.x, ev->area.y,
        ev->area.width, ev->area.height
    );
    return true;
}

/**
 *  Implements the on-button-press event callback.  It currently handles
 *  only the left button.  This button, pressed on the piano keyboard,
 *  causes m_keying to be set to true, and the given note to play.
 */

bool
seqkeys::on_button_press_event (GdkEventButton * ev)
{
    if (CAST_EQUIVALENT(ev->type, SEQ64_BUTTON_PRESS))
    {
        if (SEQ64_CLICK_LEFT(ev->button))
        {
            int y = int(ev->y + m_scroll_offset_y);
            int note;
            m_keying = true;
            convert_y(y, note);
            m_seq.play_note_on(note);
            m_keying_note = note;
        }
    }
    return true;
}

/**
 *  Implements the on-button-release event callback.  It currently handles
 *  only the left button, and only if m_keying is true.
 *
 *  This function is used after pressing on one of the keys on the left-side
 *  piano keyboard, to make it play, and turns off the playing of the note.
 */

bool
seqkeys::on_button_release_event (GdkEventButton * ev)
{
    if (CAST_EQUIVALENT(ev->type, SEQ64_BUTTON_RELEASE))
    {
        if (SEQ64_CLICK_LEFT(ev->button) && m_keying)
        {
            m_keying = false;
            m_seq.play_note_off(m_keying_note);
        }
    }
    return true;
}

/**
 *  Implements the on-motion-notify event handler.
 */

bool
seqkeys::on_motion_notify_event (GdkEventMotion * a_p0)
{
    int note;
    int y = int(a_p0->y + m_scroll_offset_y);
    convert_y(y, note);
    set_hint_key(note);
    if (m_keying)
    {
        if (note != m_keying_note)
        {
            m_seq.play_note_off(m_keying_note);
            m_seq.play_note_on(note);
            m_keying_note = note;
        }
    }
    return false;
}

/*
 * ca 2015-07-24
 * Eliminate this annoying warning.  Will do it for Microsoft's bloddy
 * compiler later.
 */

#ifdef PLATFORM_GNU
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

/**
 *  Implements the on-enter notification event handler.
 */

bool
seqkeys::on_enter_notify_event (GdkEventCrossing * a_p0)
{
    set_hint_state(true);
    return false;
}

#ifdef PLATFORM_GNU
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

/**
 *  Implements the on-leave notification event handler.
 */

bool
seqkeys::on_leave_notify_event (GdkEventCrossing * p0)
{
    if (m_keying)
    {
        m_keying = false;
        m_seq.play_note_off(m_keying_note);
    }
    set_hint_state(false);
    return true;
}

/**
 *  Implements the on-size-allocation notification event handler.
 */

void
seqkeys::on_size_allocate (Gtk::Allocation & a_r)
{
    gui_drawingarea_gtk2::on_size_allocate(a_r);
    m_window_x = a_r.get_width();
    m_window_y = a_r.get_height();
    queue_draw();
}

/**
 *  Implements the on-scroll-event notification event handler.
 */

bool
seqkeys::on_scroll_event (GdkEventScroll * ev)
{
    double val = m_vadjust.get_value();
    if (CAST_EQUIVALENT(ev->direction, SEQ64_SCROLL_UP))
        val -= m_vadjust.get_step_increment() / 6;
    else if (CAST_EQUIVALENT(ev->direction, SEQ64_SCROLL_DOWN))
        val += m_vadjust.get_step_increment() / 6;
    else
        return true;

    m_vadjust.clamp_page(val, val + m_vadjust.get_page_size());
    return true;
}

}           // namespace seq64

/*
 * seqkeys.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

