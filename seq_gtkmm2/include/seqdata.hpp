#ifndef SEQ64_SEQDATA_HPP
#define SEQ64_SEQDATA_HPP

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
 * \file          seqdata.hpp
 *
 *  This module declares/defines the base class for plastering
 *  pattern/sequence data information in the data area of the pattern
 *  editor.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2018-06-02
 * \license       GNU GPLv2 or above
 *
 *  The data pane is the drawing-area below the seqedit's event area, and
 *  contains vertical lines whose height matches the value of each data event.
 *  The height of the vertical lines is editable via the mouse.
 */

#include "globals.h"
#include "gui_drawingarea_gtk2.hpp"
#include "midibyte.hpp"                 /* seq64::midibyte typedef          */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace Gtk
{
    class Adjustment;
}

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{
    class perform;
    class sequence;

/**
 *    This class supports drawing piano-roll eventis on a window.
 */

class seqdata : public gui_drawingarea_gtk2
{
    friend class lfownd;
    friend class seqevent;
    friend class seqroll;

private:

    /**
     *  Points to the sequence whose data is being affected by this class.
     */

    sequence & m_seq;

    /**
     *  Sets the zoom value for this part of the sequence editor,
     *  one pixel == m_zoom ticks, i.e. the unit is ticks/pixel.
     */

    int m_zoom;

    /**
     *  The value of the leftmost tick in the data pane.  Adjusted in the
     *  change_horz() function.
     */

    int m_scroll_offset_ticks;

    /**
     *  The value of the leftmost pixel in the data pane.  Adjusted in the
     *  change_horz() function.  It is the offset ticks divided by the zoom
     *  value, i.e. the unit is pixels..
     */

    int m_scroll_offset_x;

    /**
     *  The adjusted width of a digit in a data number.  By "adjusted", well
     *  this is just a minor tweak for appearances.
     */

    int m_number_w;

    /**
     *  The adjusted height of all digits in a data number.  Basically, the
     *  character height times 3.  By "adjusted", well this is just a minor
     *  tweak for appearances.
     */

    int m_number_h;

    /**
     *  A new value to make it easier to adapt the vertical number drawing of
     *  a data item's numeric value to a different font.  This value was
     *  hardwired as 8, for a character height of 10.
     */

    int m_number_offset_y;

    /**
     *  Holds the status byte of the next event in the sequence, and indicates
     *  What the data window is currently editing or drawing.
     */

    midibyte m_status;

    /**
     *  Holds the MIDI CC byte of the next event in the sequence, and
     *  indicates What the data window is currently editing or drawing.
     */

    midibyte m_cc;

    /**
     *  This rectangle is used in blanking out a data line in
     *  draw_line_on_window().
     */

    GdkRectangle m_old;

#ifdef USE_STAZED_SEQDATA_EXTENSIONS

    bool m_drag_handle;

#endif

    /**
     *  This value is true if the mouse is being dragged in the data pane,
     *  which is done in order to change the height and value of each data
     *  line.
     */

    bool m_dragging;

public:

    seqdata (sequence & seq, perform & p, int zoom, Gtk::Adjustment & hadjust);

    /**
     *  Let's provide a do-nothing virtual destructor.
     */

    virtual ~seqdata ()
    {
        // I got nothin'
    }

    void reset ();

    /**
     *  Calls change_horz() to update the pixmap and queue up a redraw
     *  operation.
     */

    void redraw ()
    {
        change_horz();
    }

    void set_zoom (int a_zoom);
    void set_data_type (midibyte status, midibyte control);

private:

    int idle_redraw ();
    void update_sizes ();
    void update_pixmap ();
    void draw_line_on_window ();
    void xy_to_rect
    (
        int x1, int y1,
        int x2, int y2,
        int & rx, int & ry,
        int & rw, int & rh
    );

    void draw_events_on (Glib::RefPtr<Gdk::Drawable> drawable);
    void change_horz ();

    /**
     *  This function takes screen coordinates, and gives the horizontaol
     *  tick value based on the current zoom, returned via the second
     *  parameter.
     */

    void convert_x (int x, midipulse & tick)
    {
        tick = x * m_zoom;
    }

    /**
     *  Convenience function for rendering numbers.
     *
     * \param pixmap
     *      The reference pointer to the GDK pixmap onto which this number
     *      will be drawing.
     *
     * \param x
     *      The x-coordinate of the position of the text.
     *
     * \param y
     *      The y-coordinate of the position of the text.
     *
     * \param num
     *      The number to be rendered.  This should be a string reference, but
     *      oh well.
     */

    void render_number
    (
        Glib::RefPtr<Gdk::Pixmap> & pixmap,
        int x, int y,
        const char * const num
    )
    {
        font_render().render_string_on_drawable
        (
            m_gc, x, y, pixmap, num, font::BLACK, true
        );
    }

    void render_digits
    (
        Glib::RefPtr<Gdk::Drawable> drawable,
        int digits, int x
    );

    /**
     *  Simply calls draw_events_on() for this object's built-in pixmap.
     */

    void draw_events_on_pixmap ()
    {
        draw_events_on(m_pixmap);
    }

    /**
     *  Simply queues up a draw operation.
     */

    void draw_pixmap_on_window ()
    {
        queue_draw();
    }

private:       // callbacks

    void on_realize ();
    bool on_expose_event (GdkEventExpose * ev);
    bool on_button_press_event (GdkEventButton * ev);
    bool on_button_release_event (GdkEventButton * ev);
    bool on_motion_notify_event (GdkEventMotion * ev);
    bool on_leave_notify_event (GdkEventCrossing * ev);
    bool on_scroll_event (GdkEventScroll * ev);
    void on_size_allocate (Gtk::Allocation &);

};          // class seqdata

}           // namespace seq64

#endif      // SEQ64_SEQDATA_HPP

/*
 * seqdata.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

