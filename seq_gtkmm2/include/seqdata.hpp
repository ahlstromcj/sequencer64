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
 * \updates       2015-10-03
 * \license       GNU GPLv2 or above
 *
 */

#include "globals.h"
#include "gui_drawingarea_gtk2.hpp"

namespace Gtk
{
    class Adjustment;
}

namespace seq64
{

class perform;
class sequence;

/**
 *    This class supports drawing piano-roll eventis on a window.
 */

class seqdata : public gui_drawingarea_gtk2
{

    friend class seqroll;
    friend class seqevent;

private:

    sequence & m_seq;

    /**
     *  one pixel == m_zoom ticks
     */

    int m_zoom;

    int m_scroll_offset_ticks;
    int m_scroll_offset_x;
    int m_background_tile_x;
    int m_background_tile_y;

    /**
     *  What is the data window currently editing?
     */

    unsigned char m_status;
    unsigned char m_cc;
    Glib::RefPtr<Gdk::Pixmap> m_numbers[c_dataarea_y];
    GdkRectangle m_old;
    bool m_dragging;

public:

    seqdata
    (
        sequence & seq,
        perform & p,
        int zoom,
        Gtk::Adjustment & hadjust
    );

    void reset ();

    /**
     *  Updates the pixmap and queues up a redraw operation.  We need to
     *  make this an inline function and use it as common code.
     */

    void redraw ()
    {
        update_pixmap();
        queue_draw();
    }

    void set_zoom (int a_zoom);
    void set_data_type (unsigned char a_status, unsigned char a_control);
    int idle_redraw ();

private:

    void update_sizes ();
    void update_pixmap ();
    void draw_line_on_window ();
    void convert_x (int x, long & tick);
    void xy_to_rect
    (
      int a_x1, int a_y1,
      int a_x2, int a_y2,
      int & r_x, int & r_y,
      int & r_w, int & r_h
   );
    void draw_events_on (Glib::RefPtr<Gdk::Drawable> a_draw);
    void change_horz ();
    void force_draw ();

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
    bool on_expose_event (GdkEventExpose * a_ev);
    bool on_button_press_event (GdkEventButton * a_ev);
    bool on_button_release_event (GdkEventButton * a_ev);
    bool on_motion_notify_event (GdkEventMotion * a_p0);
    bool on_leave_notify_event (GdkEventCrossing * p0);
    bool on_scroll_event (GdkEventScroll * a_ev);
    void on_size_allocate (Gtk::Allocation &);

};

}           // namespace seq64

#endif      // SEQ64_SEQDATA_HPP

/*
 * seqdata.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
