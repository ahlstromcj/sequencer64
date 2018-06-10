#ifndef SEQ64_SEQEVENT_HPP
#define SEQ64_SEQEVENT_HPP

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
 * \file          seqevent.hpp
 *
 *  This module declares/defines the base class for the event pane.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2018-06-10
 * \license       GNU GPLv2 or above
 *
 *  The event pane is the thin gridded drawing-area below the seqedit's piano
 *  roll, and contains small boxes representing the position of each event.
 */

#include <gdkmm/pixmap.h>
#include <gdkmm/rectangle.h>
#include <gtkmm/window.h>

#include "globals.h"
#include "gui_drawingarea_gtk2.hpp"
#include "midibyte.hpp"                 /* seq64::midibyte, etc.        */

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
    class seqdata;
    class sequence;

/**
 *  Implements the piano event drawing area.
 */

class seqevent : public gui_drawingarea_gtk2
{

protected:

    /**
     *  Provides a reference to the sequence whose data is represented in this
     *  seqevent object.
     */

    sequence & m_seq;

    /**
     *  Zoom setting, means that one pixel == m_zoom ticks.
     */

    int m_zoom;

    /**
     *  The grid-snap setting for the event bar grid.  Same meaning as for the
     *  piano roll.  This value is the denominator of the note size used for
     *  the snap.
     */

    int m_snap;

    /**
     *  Used in drawing the event selection in the thing event row.
     */

    GdkRectangle m_old;

    /**
     *  Used in moving and pasting the selected events in the thin event row.
     */

    GdkRectangle m_selected;

    /**
     *  Provides the offset of the ticks in the event view based on where the
     *  scroll-bar has moved the view "window".
     */

    int m_scroll_offset_ticks;

    /**
     *  Provides the offset of the pixels in the event view based on where the
     *  scroll-bar has moved the view "window".  Set to m_scroll_offset_ticks
     *  divided by m_zoom.
     */

    int m_scroll_offset_x;

    /**
     *  The data view that parallels this event view.
     */

    seqdata & m_seqdata_wid;

    /**
     *  True if we're adding events via the mouse.
     */

    bool m_adding;

    /**
     *  Used when highlighting a bunch of events.
     */

    bool m_selecting;

    /**
     *  Used externally by the fruityseq and seq24seq modules, to initialize
     *  the act of moving events.
     */

    bool m_moving_init;

    /**
     *  Indicates that this pane is in the act of moving a selection.
     *
     *  WARNING:  This operation seems to have a bug.  It makes the events
     *  very very long.  This bug exists in Seq24.
     */

    bool m_moving;

    /**
     *  Used externally by the fruityseq and seq24seq modules, when growing
     *  the event duration.
     *
     *  Does growing work in this view?  Need to do some better testing.
     */

    bool m_growing;

    /**
     *  Used externally by the fruityseq and seq24seq modules, in painting
     *  the selected events.
     */

    bool m_painting;

    /**
     *  Indicates that we've selected some events and are in paste mode.
     */

    bool m_paste;

    /**
     *  Used externally by the fruityseq and seq24seq modules, in snapping.
     */

    int m_move_snap_offset_x;

    /**
     *  Indicates what is the data window currently editing.
     *  The current status/event byte.
     */

    midibyte m_status;

    /**
     *  Indicates what is the data window currently editing.
     *  The current MIDI CC value.
     */

    midibyte m_cc;

public:

    seqevent
    (
        perform & p,
        sequence & seq,
        int zoom,
        int snap,
        seqdata & seqdata_wid,
        Gtk::Adjustment & hadjust
    );

    /**
     *  Let's provide a do-nothing virtual destructor.
     */

    virtual ~seqevent ()
    {
        // I got nothin'
    }

    void reset ();
    void redraw ();
    void set_zoom (int zoom);
    void set_adding (bool adding);

    /**
     * \setter m_snap
     *      Simply sets the snap member.  The parameter is not validated.
     */

    void set_snap (int snap)
    {
        m_snap = snap;
    }

    void set_data_type (midibyte status, midibyte control);
    void update_sizes ();
    void draw_background ();
    void draw_events_on_pixmap ();
    void draw_pixmap_on_window ();
    void draw_selection_on_window ();
    void update_pixmap ();

protected:

    virtual void force_draw ();

    int idle_redraw ();
    void x_to_w (int x1, int x2, int & x, int & w);
    void drop_event (midipulse tick, bool istempo = false);
    void draw_events_on (Glib::RefPtr<Gdk::Drawable> draw);
    void start_paste ();
    void change_horz ();

    /**
     *  Takes the screen x coordinate, multiplies it by the current zoom, and
     *  returns the tick value in the given parameter.  Why not just return it
     *  normally?
     *
     * \param x
     *      The x (pixel) value to convert.
     *
     * \param [out] tick
     *      The destination for the converted x value.
     */

    void convert_x (int x, midipulse & tick)
    {
        tick = x * m_zoom;
    }

    /**
     *  Converts the given tick value to an x corrdinate, based on the zoom,
     *  and returns it via the second parameter.  Why not just return it
     *  normally?
     *
     * \param tick
     *      The tick (pulse) value to convert.
     *
     * \param [out] x
     *      The destination for the converted tick value.
     */

    void convert_t (midipulse tick, int & x)
    {
        x = tick / m_zoom;
    }

    /**
     *  This function performs a 'snap' on y.
     *
     * \param [out] y
     *      The return parameter for the conversion.  Why not just return the
     *      value?
     */

    void snap_y (int & y)
    {
        y -= (y % c_key_y);
    }

    void snap_x (int & x);

private:        // callbacks

    virtual void on_realize ();
    virtual bool on_expose_event (GdkEventExpose * ev);
    virtual bool on_button_press_event (GdkEventButton * ev);
    virtual bool on_button_release_event (GdkEventButton * ev);
    virtual bool on_motion_notify_event (GdkEventMotion * ev);
    virtual bool on_focus_in_event (GdkEventFocus *);
    virtual bool on_focus_out_event (GdkEventFocus *);
    virtual bool on_key_press_event (GdkEventKey * p0);
    virtual void on_size_allocate (Gtk::Allocation &);

};

}           // namespace seq64

#endif      // SEQ64_SEQEVENT_HPP

/*
 * seqevent.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

