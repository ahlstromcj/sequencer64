#ifndef SEQ64_SEQTIME_HPP
#define SEQ64_SEQTIME_HPP

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
 * \file          seqtime.hpp
 *
 *  This module declares/defines the base class for drawing the
 *  time/measures bar at the top of the patterns/sequence editor.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2016-04-14
 * \license       GNU GPLv2 or above
 *
 */

#include <gtkmm/window.h>

#include "globals.h"                    /* SEQ64_USE_DEFAULT_PPQN   */
#include "gui_drawingarea_gtk2.hpp"

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace Gtk
{
    class Adjustment;
}

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{

    class perform;
    class sequence;

/**
 *    This class implements the piano time, whatever that is.
 */

class seqtime : public gui_drawingarea_gtk2
{

private:

    sequence & m_seq;
    int m_scroll_offset_ticks;
    int m_scroll_offset_x;

    /**
     * one pixel == m_zoom ticks
     */

    int m_zoom;
    int m_ppqn;

public:

    seqtime
    (
        sequence & seq,
        perform & p,
        int zoom,
        Gtk::Adjustment & hadjust,
        int ppqn = SEQ64_USE_DEFAULT_PPQN
    );

    /**
     *  Let's provide a do-nothing virtual destructor.
     */

    virtual ~seqtime ()
    {
        // I got nothin'
    }

    void reset ();
    void redraw ();
    void set_zoom (int zoom);

private:

    void draw_pixmap_on_window ();
    void draw_progress_on_window ();
    void update_pixmap ();
    void change_horz ();
    void update_sizes ();

    // virtual void force_draw ();

    /**
     *  Simply returns true.
     */

    bool idle_progress ()
    {
        return true;
    }

private:          // callbacks

    void on_realize ();
    bool on_expose_event (GdkEventExpose * a_ev);
    void on_size_allocate (Gtk::Allocation &);

    /**
     *  Implements the on-button-press event handler.  Simply returns false.
     */

    bool on_button_press_event (GdkEventButton * /*p0*/ )
    {
        return false;
    }

    /**
     *  Implements the on-button-release event handler.  Simply returns false.
     */

    bool on_button_release_event (GdkEventButton * /*p0*/ )
    {
        return false;
    }

};          // class seqtime

}           // namespace seq64

#endif      // SEQ64_SEQTIME_HPP

/*
 * seqtime.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
