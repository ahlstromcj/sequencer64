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
 * \updates       2018-10-29
 * \license       GNU GPLv2 or above
 *
 *  One thing we must be sure of is the MIDI note range.  Obviously, in terms
 *  of MIDI byte values, these range from 0 to 127.  In the ISO system
 *  (see http://www.midikits.net/midi_analyser/midi_note_numbers_for_octaves.htm)
 *  the following key values exist:
 *
 *      -   <b>C-1</b>. The first octave is -1, and the first note is C, so
 *          that the lowest labelled note would be "C-1", with a MIDI note
 *          value of 0.
 *      -   <b>C4</b>. Middle C is "C4", with a MIDI note value of 60.
 *      -   <b>A4</b>. For A-440 tuning, the note is "A4", with a MIDI note
 *          value of 69.
 *      -   <b>G9</b>.  The highest labelled note in this system is "G9", with
 *          a MIDI note value of 127.
 *
 *  Various devices and MIDI applications can have different base values:
 *
 *      -   If the device/application considers the lowest (first) MIDI note
 *          octave to be 0, then middle C is still 60, but is called "C5".
 *      -   If the device/application considers the third MIDI note
 *          octave to be 0, then middle C is still 60, but is called "C3",
 *          and the highest note is "G8".
 *
 *  Variables:
 *
 * \verbatim
 *          <- sc_drawarea_x  ->
 *          <- sc_keyarea_x ->
 *           ----------------
 *          |     C5 |       | |    m_key_y
 *          |         -------| |
 *          |        |bbbbbbb| |
 *          |         -------| |
 *          |        |       | |
 *          |         -------| |
 *          |        |bbbbbbb| |
 *          |         -------| |
 *          |        |       | |
 *          |         -------| |
 *
 *                   <------>      sc_key_x
 *                   ^
 *                   |
 *                    ------------ sc_keyoffset_x
 * \endverbatim
 */

#include <gtkmm/adjustment.h>

#include "click.hpp"                    /* SEQ64_CLICK_LEFT() etc.      */
#include "font.hpp"
#include "globals.h"                    /* c_keyarea_y and more         */
#include "scales.h"
#include "seqkeys.hpp"
#include "sequence.hpp"
#include "settings.hpp"                 /* rc() and usr() accessors     */

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{

static const int sc_key_x = 20;
static const int sc_key_y =  8;

/**
 *  The dimensions and offset of the virtual keyboard at the left of the
 *  piano roll.
 */

static const int sc_keyarea_x = sc_key_x + 20 - 4;
static const int sc_drawarea_x = sc_keyarea_x + 1;
static const int sc_keyoffset_x = sc_keyarea_x - sc_key_x + 1;

/**
 *  Principal constructor.
 *
 * \param seq
 *      Provides the sequence object to which this seqkeys pane is associated.
 *
 * \param p
 *      Provides the performance object to which this seqkeys pane (and all
 *      sequences) are associated.
 *
 * \param vadjust
 *      The range object for the vertical scrollbar linked to the position in
 *      the seqkeys pane.
 */

seqkeys::seqkeys
(
    sequence & seq,
    perform & p,
    Gtk::Adjustment & vadjust
) :
    gui_drawingarea_gtk2
    (
        p, adjustment_dummy(), vadjust, sc_drawarea_x, 10   // bogus y height
    ),
    m_seq                   (seq),
    m_scroll_offset_key     (0),
    m_scroll_offset_y       (0),
    m_hint_state            (false),
    m_hint_key              (0),
    m_keying                (false),
    m_keying_note           (0),
    m_scale                 (0),
    m_key                   (0),
    m_key_y                 (usr().key_height()),
    m_keyarea_y             (m_key_y * c_num_keys + 1),
    m_drawarea_y            (m_keyarea_y - 2),
    m_show_octave_letters   (true)
{
    // Empty body
}

/**
 *  Sets the musical scale, then resets.  This function is called by the
 *  seqedit class.
 *
 * \param scale
 *      The musical scale value to be set.
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
 *
 * \param key
 *      The musical key value to be set.
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
 *  This function draws the keys, which range from 0 to 127
 *  (SEQ64_MIDI_COUNT_MAX - 1 = c_num_keys - 1).  Every octave,
 *  a key letter and number (e.g. "C4") is shown.  The letter is adjusted to
 *  match the current scale (e.g. "C#4").
 *
 *  We want to support an option to show the key number rather than the note
 *  letter/number combination, and perhaps to toggle between them.  The
 *  current difficulty is that the fonts used are just a little to high to fit
 *  within the vertical limits of each key.  We really don't want to change
 *  the vertical size at this time, so we just print every other note value.
 *
 *  Also note that this algorithm draws from the top down, so we have to
 *  account for that.
 */

void
seqkeys::update_pixmap ()
{
    int kx = sc_keyoffset_x + 4;    //  + 1;

    /*
     * Doesn't seem to be needed:
     *
     *  draw_rectangle_on_pixmap(black_paint(), 0, 0, sc_keyarea_x, sc_keyarea_y);
     */

    draw_rectangle_on_pixmap
    (
        white_paint(), 1, 1, sc_keyoffset_x + 2, m_drawarea_y
    );
    for (int key = 0; key < c_num_keys; ++key)
    {
        int yofkey = m_key_y * key;
        draw_rectangle_on_pixmap
        (
            white_key_paint(), kx, yofkey + 1, sc_key_x - 2, m_key_y - 1
        );

        int okey = (c_num_keys - key - 1) % SEQ64_OCTAVE_SIZE;
        if (is_black_key(okey))
        {
            draw_rectangle_on_pixmap
            (
                black_key_paint(), kx, yofkey + 2, sc_key_x - 2, m_key_y - 3
            );
        }

        char note[8];
        int keyvalue = c_num_keys - key - 1;
        bool inverse = usr().inverse_colors();
        --yofkey;
        if (m_show_octave_letters)
        {
            if (okey == m_key)                  /* octave note      */
            {
                int octave = (keyvalue / SEQ64_OCTAVE_SIZE) - 1;
                if (octave < 0)
                    octave *= -1;

                snprintf(note, sizeof note, "%2s%1d", c_key_text[okey], octave);
                render_string_on_pixmap(2, yofkey, note, font::BLACK, inverse);
            }
        }
        else
        {
            if ((keyvalue % 2) == 0)
            {
                snprintf(note, sizeof note, "%3d", keyvalue);
                render_string_on_pixmap(1, yofkey, note, font::BLACK, inverse);
            }
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
    draw_drawable(0, m_scroll_offset_y, 0, 0, sc_keyarea_x, m_keyarea_y);
}

/**
 *  Forces a draw operation on the whole window.  Unlike most other
 *  overridden versions of force_draw(), this one does not call the
 *  base-class version.
 */

void
seqkeys::force_draw ()
{
    draw_drawable(0, m_scroll_offset_y, 0, 0, m_window_x, m_window_y);
}

/**
 *  Takes the screen y coordinate, and returns the note value in the
 *  second parameter.
 *
 * \param y
 *      The y (vertical) screen coordinate to convert.
 *
 * \param [out] note
 *      The destination for the note calculation.  This would be better as a
 *      return value.
 */

void
seqkeys::convert_y (int y, int & note)
{
    note = (m_drawarea_y - y) / m_key_y;
}

/**
 *  Sets a key to grey so that it can serve as a scale hint.  If m_hint_state
 *  is true, the key is drawn (again).
 *
 * \param key
 *      The key value to set the hint-key to.
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

    int x = sc_keyoffset_x + 4; // + 1;
    int y = (m_key_y * key) + 2 - m_scroll_offset_y;
    int w = sc_key_x - 3;                           /* x length of key      */
    int h = m_key_y - 3;                           /* y height of key      */
    if (state)
    {
        if (usr().inverse_colors())
            draw_rectangle(orange(), x, y, w, h);   /* red()?               */
        else
            draw_rectangle(grey_paint(), x, y, w, h);
    }
    else
    {
        if (is_black_key(k))
        {
            m_gc->set_foreground(black_key_paint());
            draw_rectangle(x, y, w, h);
        }
        else
        {
            m_gc->set_foreground(white_key_paint());
            draw_rectangle(x, y, w, h);
        }
    }
}

/**
 *
 */


/**
 *  Changes the y offset of the scrolling, and the forces a draw.
 *
 *  Weird, in seq24 and here, the following was used, completely by accident!
 *  We fixed it, but must beware!
 *
\verbatim
    m_scroll_offset_y = m_scroll_offset_key * m_key_y,  // comma operator!!!
    force_draw();
\endverbatim
 */

void
seqkeys::change_vert ()
{
    m_scroll_offset_key = int(m_vadjust.get_value());
    m_scroll_offset_y = m_scroll_offset_key * m_key_y;
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
    m_pixmap = Gdk::Pixmap::create(m_window, sc_keyarea_x, m_keyarea_y, -1);
    update_pixmap();
    m_vadjust.signal_value_changed().connect
    (
        mem_fun(*this, &seqkeys::change_vert)
    );
    change_vert();
}

/**
 *  Implements the on-expose event, by drawing on the window.
 *
 * \param ev
 *      The expose-event object.
 */

bool
seqkeys::on_expose_event (GdkEventExpose * ev)
{
    draw_drawable
    (
        ev->area.x, ev->area.y + m_scroll_offset_y,
        ev->area.x, ev->area.y, ev->area.width, ev->area.height
    );
    return true;
}

/**
 *  Implements the on-button-press event callback.  It handles the left and
 *  right buttons.  The left button, pressed on the piano keyboard, causes
 *  m_keying to be set to true, and the given note to play.  The right button
 *  toggles the note display between letter/number and MIDI note number.
 *
 * \param ev
 *      The mouse-button event to use.
 *
 * \return
 *      Always returns true.
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
        else if (SEQ64_CLICK_RIGHT(ev->button))
        {
            m_show_octave_letters = ! m_show_octave_letters;
            reset();                /* draw_area() or update_pixmap() */
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
 *
 * \param ev
 *      The button-event.
 *
 * \return
 *      Always returns true.
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
 *  Implements the on-motion-notify event handler.  This allows
 *  rolling down the keyboard, playing the notes one-by-one.
 *
 * \param p0
 *      The motion event.
 *
 * \return
 *      Always returns false.
 */

bool
seqkeys::on_motion_notify_event (GdkEventMotion * p0)
{
    int note;
    int y = int(p0->y + m_scroll_offset_y);
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
 *  This greys the current key.
 */

bool
seqkeys::on_enter_notify_event (GdkEventCrossing *)
{
    set_hint_state(true);
    return false;
}

#ifdef PLATFORM_GNU
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

/**
 *  Implements the on-leave notification event handler.  This un-greys
 *  the current key and stops playing the note.
 */

bool
seqkeys::on_leave_notify_event (GdkEventCrossing *)
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
 *
 * \param all
 *      Provies the allocation and its width and height.
 */

void
seqkeys::on_size_allocate (Gtk::Allocation & all)
{
    gui_drawingarea_gtk2::on_size_allocate(all);
    m_window_x = all.get_width();
    m_window_y = all.get_height();
    queue_draw();
}

/**
 *  Implements the on-scroll-event notification event handler.
 *  Note that there is no usage of the modifier keys (e.g. Shift or Ctrl).
 *  Compare this function to seqedit::on_scroll_event().
 *
 * \param ev
 *      Provides the direction of the scroll event.
 *
 * \return
 *      Always returns true.
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

