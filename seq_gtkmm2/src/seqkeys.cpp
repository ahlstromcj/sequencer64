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
 * \updates       2015-09-11
 * \license       GNU GPLv2 or above
 *
 */

#include <gtkmm/adjustment.h>

#include "font.hpp"
#include "seqkeys.hpp"
#include "sequence.hpp"

/**
 *  Principal constructor.
 */

seqkeys::seqkeys
(
    sequence * a_seq,
    Gtk::Adjustment * a_vadjust
) :
    Gtk::DrawingArea        (),
    m_gc                    (),
    m_window                (),
    m_black                 (Gdk::Color("black")),
    m_white                 (Gdk::Color("white")),
    m_grey                  (Gdk::Color("grey")),
    m_pixmap                (),
    m_window_x              (0),
    m_window_y              (0),
    m_vadjust               (a_vadjust),
    m_seq                   (a_seq),
    m_scroll_offset_key     (0),
    m_scroll_offset_y       (0),
    m_hint_state            (false),
    m_hint_key              (0),
    m_keying                (false),
    m_keying_note           (0),
    m_scale                 (0),
    m_key                   (0)
{
    add_events
    (
        Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK |
        Gdk::ENTER_NOTIFY_MASK | Gdk::LEAVE_NOTIFY_MASK |
        Gdk::POINTER_MOTION_MASK | Gdk::SCROLL_MASK
    );
    set_size_request(c_keyarea_x + 1, 10);          /* set default size */

    Glib::RefPtr<Gdk::Colormap> colormap = get_default_colormap();
    colormap->alloc_color(m_black);
    colormap->alloc_color(m_white);
    colormap->alloc_color(m_grey);
    set_double_buffered(false);
}

/**
 *  Sets the musical scale, then resets.
 */

void
seqkeys::set_scale (int a_scale)
{
    if (m_scale != a_scale)
    {
        m_scale = a_scale;
        reset();
    }
}

/**
 *  Sets the musical key, then resets.
 */

void
seqkeys::set_key (int a_key)
{
    if (m_key != a_key)
    {
        m_key = a_key;
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
    m_gc->set_foreground(m_black);
    m_pixmap->draw_rectangle(m_gc, true, 0, 0, c_keyarea_x, c_keyarea_y);
    m_gc->set_foreground(m_white);
    m_pixmap->draw_rectangle(m_gc, true, 1, 1, c_keyoffset_x-1, c_keyarea_y-2);
    for (int i = 0; i < c_num_keys; i++)
    {
        m_gc->set_foreground(m_white);
        m_pixmap->draw_rectangle
        (
            m_gc, true, c_keyoffset_x + 1,
            (c_key_y * i) + 1, c_key_x - 2, c_key_y - 1
        );

        int key = (c_num_keys - i - 1) % OCTAVE_SIZE;   /* key in the octave */
        if (key == 1 || key == 3 || key == 6 || key == 8 || key == 10)
        {
            m_gc->set_foreground(m_black);
            m_pixmap->draw_rectangle
            (
                m_gc, true, c_keyoffset_x + 1,
                (c_key_y * i) + 2, c_key_x - 3, c_key_y - 3
            );
        }

        char notes[20];
        if (key == m_key)                       /* notes */
        {
            int octave = ((c_num_keys - i - 1) / OCTAVE_SIZE) - 1;
            if (octave < 0)
                octave *= -1;

            snprintf(notes, sizeof(notes), "%2s%1d", c_key_text[key], octave);
            p_font_renderer->render_string_on_drawable
            (
                m_gc, 2, c_key_y * i - 1, m_pixmap, notes, font::BLACK
            );
        }

        /*
         * This commented code looks interesting.
         *
         *  snprintf
         *  (
         *      notes, sizeof(notes), "%c %d",
         *      c_scales_symbol[m_scale][key], m_scale
         *  );
         *  p_font_renderer->render_string_on_drawable
         *  (
         *      m_gc, 2 + (c_text_x * 4), c_key_y * i - 1,
         *      m_pixmap, notes, font::BLACK
         *  );
         */
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
    m_window->draw_drawable
    (
        m_gc, m_pixmap, 0, m_scroll_offset_y, 0, 0, c_keyarea_x, c_keyarea_y
    );
}

/**
 *  Forces a draw operation on the whole window.
 */

void
seqkeys::force_draw ()
{
    m_window->draw_drawable
    (
        m_gc, m_pixmap, 0, m_scroll_offset_y, 0, 0, m_window_x, m_window_y
    );
}

/**
 *  Takes the screen y coordinate, and returns the note value in the
 *  second parameter.
 */

void
seqkeys::convert_y (int a_y, int * a_note)
{
    *a_note = (c_rollarea_y - a_y - 2) / c_key_y;
}

/**
 *  Sets a key to grey so that it can serve as a scale hint.
 */

void
seqkeys::set_hint_key (int a_key)
{
    draw_key(m_hint_key, false);
    m_hint_key = a_key;
    if (m_hint_state)
        draw_key(a_key, true);
}

/**
 *  Sets the hint state to the given value.
 *
 * \param a_state
 *      Provides the value for hinting, where true == on, false == off.
 */

void
seqkeys::set_hint_state (bool a_state)
{
    m_hint_state = a_state;
    if (!a_state)
        draw_key(m_hint_key, false);
}

/**
 *  Draws the given key according to the given state.
 *
 * \param a_key
 *      The key to be drawn.
 *
 * \param a_state
 *      How the key is to be drawn, where false == normal, true == grayed.
 */

void
seqkeys::draw_key (int a_key, bool a_state)
{
    int key = a_key % OCTAVE_SIZE;               /* the key in the octave */
    a_key = c_num_keys - a_key - 1;
    if (key == 1 || key == 3 || key == 6 || key == 8 || key == 10)
        m_gc->set_foreground(m_black);
    else
        m_gc->set_foreground(m_white);

    m_window->draw_rectangle
    (
        m_gc, true, c_keyoffset_x + 1,
        (c_key_y * a_key) + 2 -  m_scroll_offset_y,
        c_key_x - 3, c_key_y - 3
    );
    if (a_state)
    {
        m_gc->set_foreground(m_grey);
        m_window->draw_rectangle
        (
            m_gc, true, c_keyoffset_x + 1,
            (c_key_y * a_key) + 2 - m_scroll_offset_y,
            c_key_x - 3, c_key_y - 3
        );
    }
}

/**
 *  Changes the y offset of the scrolling, and the forces a draw.
 */

void
seqkeys::change_vert ()
{
    m_scroll_offset_key = (int) m_vadjust->get_value();
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
    Gtk::DrawingArea::on_realize();
    m_window = get_window();
    m_gc = Gdk::GC::create(m_window);
    m_window->clear();
    m_pixmap = Gdk::Pixmap::create(m_window, c_keyarea_x, c_keyarea_y, -1);
    update_pixmap();
    m_vadjust->signal_value_changed().connect
    (
        mem_fun(*this, &seqkeys::change_vert)
    );
    change_vert();
}

/**
 *  Implements the on-expose event, by drawing on the window.
 */

bool
seqkeys::on_expose_event (GdkEventExpose * a_e)
{
    m_window->draw_drawable
    (
        m_gc, m_pixmap, a_e->area.x, a_e->area.y + m_scroll_offset_y,
        a_e->area.x, a_e->area.y, a_e->area.width, a_e->area.height
    );
    return true;
}

/**
 *  Implements the on-button-press event callback.  It currently handles
 *  only the left button.  This button, pressed on the piano keyboard,
 *  causes m_keying to be set to true, and the given note to play.
 */

bool
seqkeys::on_button_press_event (GdkEventButton * a_e)
{
    if (a_e->type == GDK_BUTTON_PRESS)
    {
        if (a_e->button == 1)
        {
            int y = int(a_e->y + m_scroll_offset_y);
            int note;
            m_keying = true;
            convert_y(y, &note);
            m_seq->play_note_on(note);
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
seqkeys::on_button_release_event (GdkEventButton * a_e)
{
    if (a_e->type == GDK_BUTTON_RELEASE)
    {
        if (a_e->button == 1 && m_keying)
        {
            m_keying = false;
            m_seq->play_note_off(m_keying_note);
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
    int y = (int) a_p0->y + m_scroll_offset_y;
    convert_y(y, &note);
    set_hint_key(note);
    if (m_keying)
    {
        if (note != m_keying_note)
        {
            m_seq->play_note_off(m_keying_note);
            m_seq->play_note_on(note);
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
        m_seq->play_note_off(m_keying_note);
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
    Gtk::DrawingArea::on_size_allocate(a_r);
    m_window_x = a_r.get_width();
    m_window_y = a_r.get_height();
    queue_draw();
}

/**
 *  Implements the on-scroll-event notification event handler.
 */

bool
seqkeys::on_scroll_event (GdkEventScroll * a_ev)
{
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

/*
 * seqkeys.cpp
 *
 * vim: sw=4 ts=4 wm=8 et ft=cpp
 */

