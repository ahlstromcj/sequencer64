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
 * \file          selectionbox.cpp
 *
 *  This module declares/defines the base class for drawing on the piano
 *  roll of the sequence/pattern editor.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2017-09-16
 * \updates       2017-09-16
 * \license       GNU GPLv2 or above
 *
 *  There are a large number of existing items to discuss.  But for now let's
 *  talk about how to have the scrollbar follow the progress bar.
 */

#include "app_limits.h"                 /* SEQ64_SOLID_PIANOROLL_GRID   */
// #include "click.hpp"                    /* SEQ64_CLICK_LEFT(), etc.     */
// #include "event.hpp"
// #include "gdk_basic_keys.h"
// #include "gui_key_tests.hpp"            /* seq64::is_no_modifier() etc. */
// #include "keystroke.hpp"
#include "selectionbox.hpp"
// #include "perform.hpp"
// #include "settings.hpp"                 /* seq64::usr() and seq64::rc() */

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *  Principal constructor.
 *
 * \param p
 *      The performance object that helps control this piano roll.  Note that
 *      we can get the perform object from the sequence, and save a parameter.
 *      Low priority change.
 *
 * \param seq
 *      The sequence object represented by this piano roll.
 *
 * \param zoom
 *      The initial zoom of this piano roll.
 *
 * \param snap
 *      The initial grid snap of this piano roll.
 *
 * \param pos
 *      A position parameter.  See the description of selectionbox::m_pos.
 *      This is actually the sequence number, and is currently unused.
 *      However, we're sure we can find a use for it sometime.
 *
 * \param ppqn
 *      The initial value of the PPQN for this sequence.  Useful in scale
 *      calculations.
 */

selectionbox::selectionbox
(
    perform & p,
    sequence & seq,
    int zoom,
    int snapx,
    int snapy,
    int ppqn
) :
//  gui_drawingarea_gtk2    (p, hadjust, vadjust, 10, 10),
    m_old                   (),
    m_selected              (),
    m_zoom                  (zoom),
    m_snap_x                (snapx),
    m_snap_y                (snapy),
    m_ppqn                  (0),
    m_adding                (false),
    m_selecting             (false),
    m_moving                (false),
    m_moving_init           (false),
    m_growing               (false),
    m_painting              (false),    /* used in fruity & seq24           */
    m_paste                 (false),
    m_is_drag_pasting       (false),
    m_is_drag_pasting_start (false),
    m_justselected_one      (false),
//  m_drop_x                (0),
//  m_drop_y                (0),
//  m_current_x             (0),
//  m_current_y             (0),
    m_move_delta_x          (0),
    m_move_delta_y          (0),
    m_move_snap_offset_x    (0),        /* used in fruity                   */
{
    m_ppqn = choose_ppqn(ppqn);
    clear_old();
}

/**
 *  Provides a destructor to delete allocated objects.  The only thing to
 *  delete here is the clipboard.  Except it is never used, so is commented
 *  out.
 */

selectionbox::~selectionbox ()
{
}

/**
 *  Sets the zoom to the given value.
 *
 * \param zoom
 *      The desired zoom value, assumed to be validated already.  See the
 *      seqedit::set_zoom() function.
 */

void
selectionbox::set_zoom (int zoom)
{
    if (m_zoom != zoom)
    {
        m_zoom = zoom;
    }
}

/**
 *  Sets the music scale to the given value.
 *
 * \param scale
 *      The desired scale value.
 */

void
selectionbox::set_scale (int scale)
{
    if (m_scale != scale)
    {
        m_scale = scale;
    }
}

/**
 *  Draws the current selecton on the main window.  Note the parameters of
 *  draw_drawable(), which we need to be sure of to draw thicker boxes.
 *
 *      -   x and y position of rectangle to draw
 *      -   x and y position in drawable where rectangle should be drawn
 *      -   width and height of rectangle to draw
 *
 *  A final parameter of false draws an unfilled rectangle.  Orange makes it a
 *  little more clear that we're pasting, I think.  We also want to try to
 *  thicken the lines somehow.
 */

void
selectionbox::draw_selection_on_window ()
{
    const int thickness = 1;                /* normally 1               */
    int x = 0, y = 0, w = 0, h = 0;         /* used throughout          */
//  set_line(Gdk::LINE_SOLID, thickness);   /* set_line_attributes()    */
    if (select_action())                    /* select, grow, drop       */
    {
        /*
         * \todo
         *      Use rect::get().
         */

        x = m_old.x;
        y = m_old.y;
        w = m_old.width;
        h = m_old.height;
//      draw_drawable(x, y, x, y, w + 1, h + 1);    /* erase old rectangle */
    }
    if (selecting())
    {
        xy_to_rect(m_drop_x, m_drop_y, m_current_x, m_current_y, x, y, w, h);
        x -= m_scroll_offset_x;
        y -= m_scroll_offset_y;
        h += c_key_y;
    }
    if (drop_action())                      /* move, paste              */
    {
        x = m_selected.x + m_current_x - m_drop_x - m_scroll_offset_x;
        y = m_selected.y + m_current_y - m_drop_y - m_scroll_offset_y;
        w = m_selected.width;
        h = m_selected.height;
    }
    if (growing())
    {
        int delta_x = m_current_x - m_drop_x;
        x = m_selected.x - m_scroll_offset_x;
        y = m_selected.y - m_scroll_offset_y;
        w = delta_x + m_selected.width;
        h = m_selected.height;
        if (w < 1)
            w = 1;
    }
//  draw_rectangle(dark_orange(), x, y, w, h, false);
    m_old.x = x;
    m_old.y = y;
    m_old.width = w;
    m_old.height = h;
}

/*
 *  This function takes the given x and y screen coordinates, and returns
 *  the note and the tick via the pointer parameters.  This function is
 *  the "inverse" of convert_tn().
 *
 * \param x
 *      Provides the x value of the coordinate.
 *
 * \param y
 *      Provides the y value of the coordinate.
 *
 * \param [out] tick
 *      Provides the destination for the horizontal value in MIDI pulses.
 *
 * \param [out] note
 *      Provides the destination for the vertical value, a note value.
 */

void
selectionbox::convert_xy (int x, int y, midipulse & tick, int & note)
{
    tick = x * m_zoom;
    note = (c_rollarea_y - y - 2) / c_key_y;        // why -2 ?
}

/**
 * This function takes the given note and tick, and returns the screen
 * coordinates via the pointer parameters.  This function is the "inverse"
 * of convert_xy().
 *
 * \param tick
 *      Provides the horizontal value in MIDI pulses.
 *
 * \param note
 *      Provides the vertical value, a note value.
 *
 * \param [out] x
 *      Provides the destination x value of the coordinate.
 *
 * \param [out] y
 *      Provides the destination y value of the coordinate.
 */

void
selectionbox::convert_tn (midipulse tick, int note, int & x, int & y)
{
    x = tick / m_zoom;
    y = c_rollarea_y - ((note + 1) * c_key_y) - 1;
}

/**
 *  Converts a tick/note box to an x/y rectangle.
 *
 *  We should refactor this function to use the utility class selectionbox::rect as
 *  the destination for the conversion.
 *
 * \param tick_s
 *      The starting tick of the rectangle.
 *
 * \param tick_f
 *      The finishing tick of the rectangle.
 *
 * \param note_h
 *      The high note of the rectangle.
 *
 * \param note_l
 *      The low note of the rectangle.
 *
 * \param [out] x
 *      The destination for the x value in pixels.
 *
 * \param [out] y
 *      The destination for the y value in pixels.
 *
 * \param [out] w
 *      The destination for the rectangle width in pixels.
 *
 * \param [out] h
 *      The destination for the rectangle height value in pixels.
 */

void
selectionbox::convert_tn_box_to_rect
(
    midipulse tick_s, midipulse tick_f, int note_h, int note_l,
    int & x, int & y, int & w, int & h
)
{
    int x1, y1, x2, y2;
    convert_tn(tick_s, note_h, x1, y1);     /* convert box to X,Y values */
    convert_tn(tick_f, note_l, x2, y2);
    xy_to_rect(x1, y1, x2, y2, x, y, w, h);
    h += c_key_y;
}

/**
 *  A convenience function wrapping a common call to convert_tn_box_to_rect().
 *
 * \param tick_s
 *      The starting tick of the rectangle.
 *
 * \param tick_f
 *      The finishing tick of the rectangle.
 *
 * \param note_h
 *      The high note of the rectangle.
 *
 * \param note_l
 *      The low note of the rectangle.
 */

void
selectionbox::convert_sel_box_to_rect
(
    midipulse tick_s, midipulse tick_f, int note_h, int note_l
)
{
    convert_tn_box_to_rect
    (
        tick_s, tick_f, note_h, note_l,
        m_selected.x, m_selected.y, m_selected.width, m_selected.height
    );
}

/**
 *  A convenience function wrapping a common call to m_seq.get_selected_box()
 *  and convert_tn_box_to_rect().
 *
 * \param [out] tick_s
 *      The starting tick of the rectangle.
 *
 * \param [out] tick_f
 *      The finishing tick of the rectangle.
 *
 * \param [out] note_h
 *      The high note of the rectangle.
 *
 * \param [out] note_l
 *      The low note of the rectangle.
 */

void
selectionbox::get_selected_box
(
    midipulse & tick_s, int & note_h, midipulse & tick_f, int & note_l
)
{
    m_seq.get_selected_box(tick_s, note_h, tick_f, note_l);
    convert_sel_box_to_rect(tick_s, tick_f, note_h, note_l);
}

/**
 *  Starts a paste operation.
 */

void
selectionbox::start_paste ()
{
    snap_x(m_current_x);
    snap_y(m_current_y);                        // was snap_y(m_current_x) !!
    m_drop_x = m_current_x;
    m_drop_y = m_current_y;
    m_paste = true;

    /*
     * Get the box that selected elements are in.
     */

    midipulse tick_s, tick_f;
    int note_h, note_l;
    m_seq.get_clipboard_box(tick_s, note_h, tick_f, note_l);
    convert_sel_box_to_rect(tick_s, tick_f, note_h, note_l);
    m_selected.x += m_drop_x;
    m_selected.y = m_drop_y;
}

/**
 *  Completes a paste operation.
 */

void
selectionbox::complete_paste (int x, int y)
{
    midipulse tick;
    int note;
    convert_xy(m_current_x, m_current_y, tick, note);
    m_paste = false;

    /*
     * m_seq.push_undo();       // Why commented out?
     */

    m_seq.paste_selected(tick, note);
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
 *
 * \param [out] x
 *      Provides the x-value to be snapped and returned.  A return value would
 *      be better.
 */

void
selectionbox::snap_x (int & x)
{
    int mod = m_snap / m_zoom;
    if (mod <= 0)
        mod = 1;

    x -= x % mod;
}

/**
 *  Function to allow motion of the selection box via the arrow keys.  We now
 *  let the Enter key to finish pasting and deselect the moved notes.  With
 *  the mouse, selecting all notes, copying them, and moving the selection
 *  box, pasting can be completed with either a left-click or the Enter key.
 *
 *  We have a weird problem on our main system where the selection box is very
 *  flickery.  But it works fine on another system.  A Gtk-2 issue?  Now it
 *  seems to work fine, after an update.  No, it seems to work well in
 *  sequences that have non-note events amongst the note events.
 *
 * \param dx
 *      The amount to move the selection box.  Values are -1, 0, or 1.  -1 is
 *      left one snap, 0 is no movement horizontally, and 1 is right one snap.
 *
 * \param dy
 *      The amount to move the selection box.  Values are -1, 0, or 1.  -1 is
 *      up one snap, 0 is no movement vertically, and 1 is down one snap.
 */

void
selectionbox::move_selection_box (int dx, int dy)
{
    int x = m_old.x + dx * m_snap / m_zoom;
    int y = m_old.y + dy * c_key_y;
    set_current_offset_x_y(x, y);

	int note;
	midipulse tick;
    snap_y(m_current_y);
	convert_xy(0, m_current_y, tick, note);
    m_seqkeys_wid.set_hint_key(note);
    snap_x(m_current_x);
    draw_selection_on_window();         /* m_old.x = x, m_old.y = y handled */
}

/**
 *  Proposed new function to encapsulate the movement of selections even
 *  more fully.  Works with the four arrow keys.
 *
 *  Note that the movement vertically is different for the selection box versus
 *  the notes.  While the movement values are -1, 0, or 1, the differences are
 *  as follows:
 *
 *      -   Selection box vertical movement:
 *          -   -1 is up one note snap.
 *          -   0 is no vertical movement.
 *          -   +1 is down one note snap.
 *      -   Note vertical movement:
 *          -   -1 is down one note.
 *          -   0 is no note vertical movement.
 *          -   +1 is up one note.
 *
 * \param dx
 *      The amount to move the selection box or the selection horizontally.
 *      Values are -1 (left one time snap), 0 (no movement), and +1 (right one
 *      snap).  Obviously values other than +-1 can be used for larger
 *      movement, but the GUI doesn't yet support that ... we could implement
 *      movement by "pages" some day.
 *
 * \param dy
 *      The amount to move the selection box or the selection vertically.  See
 *      the notes above.
 */

void
selectionbox::move_selected_notes (int dx, int dy)
{
    if (m_paste)
    {
        move_selection_box(dx, dy);
    }
    else
    {
        int snap_x = dx * m_snap;                   /* time-stamp snap  */
        int snap_y = -dy;                           /* note pitch snap  */
        if (m_seq.any_selected_notes())             /* redundant!       */
            m_seq.move_selected_notes(snap_x, snap_y);
        else if (snap_x != 0)
            m_seq.set_last_tick(m_seq.get_last_tick() + snap_x);
    }
}

/**
 *  Proposed new function to encapsulate the movement of selections even
 *  more fully.
 *
 * \param dx
 *      The amount to grow the selection horizontally.  Values are -1 (left one
 *      time snap), 0 (no stretching), and +1 (right one snap).  Obviously
 *      values other than +-1 can be used for larger stretching, but the GUI
 *      doesn't yet support that.
 */

void
selectionbox::grow_selected_notes (int dx)
{
    if (! m_paste)
    {
        int snap_x = dx * m_snap;                   /* time-stamp snap  */
        m_growing = true;
        m_seq.grow_selected(snap_x);
    }
}

/**
 *  Changes the mouse cursor pixmap according to whether a note is being
 *  added or not.  What calls this?  It is actually a right click.
 *  Not present in the "fruity" implementation.  Now moved to the normal
 *  selectionbox class.
 *
 * \param adding
 *      True if adding a note.
 */

void
selectionbox::set_adding (bool adding)
{
    m_adding = adding;
//  if (adding)
//      get_window()->set_cursor(Gdk::Cursor(Gdk::PENCIL));
//  else
//      get_window()->set_cursor(Gdk::Cursor(Gdk::LEFT_PTR));
}

/**
 *  An internal function used by the FruitySeqRollInput class.
 */

inline static long
clamp (long val, long low, long hi)
{
    return val < low ? low : (hi < val ? hi : val) ;
}

}           // namespace seq64

/*
 * selectionbox.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

