#ifndef SEQ64_PERFNAMES_HPP
#define SEQ64_PERFNAMES_HPP

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
 * \file          perfnames.hpp
 *
 *  This module declares/defines the base class for performance names.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2015-10-02
 * \license       GNU GPLv2 or above
 *
 *  This class supports the left side of the Performance window (also known
 *  as the Song window).
 */

#include "globals.h"
#include "gui_drawingarea_gtk2.hpp"
#include "seqmenu.hpp"

namespace Gtk
{
    class Adjustment;
}

namespace seq64
{

class perform;

/**
 *  This class implements the left-side keyboard in the patterns window.
 *
 * \obsolete
 *      Note the usage of virtual base classes.  Since these can add some
 *      extra overhead, we should determine if we can do without the
 *      virtuality (and indeed it doesn't seem to be needed).
 */

class perfnames : public gui_drawingarea_gtk2, public seqmenu
{

private:

    int m_names_x;
    int m_names_y;
    int m_seqs_in_set;
    int m_sequence_max;                         // CONST
    int m_sequence_offset;
    bool m_sequence_active[c_max_sequence];     // CONST

public:

    perfnames (perform & p, Gtk::Adjustment & vadjust);

    void redraw_dirty_sequences ();

private:

    int convert_y (int y);
    void draw_sequence (int sequence);
    void change_vert ();

    /**
     *  This function does nothing.
     */

    void update_pixmap ()
    {
        // Empty body
    }

    /**
     *  This function does nothing.
     */

    void draw_area ()
    {
        // Empty body
    }

    /**
     *  Redraw the given sequence.
     */

    void redraw (int sequence)
    {
        draw_sequence(sequence);
    }

private:    // Gtk callbacks

    void on_realize ();
    bool on_expose_event (GdkEventExpose * ev);
    bool on_button_press_event (GdkEventButton * ev);
    bool on_button_release_event (GdkEventButton * ev);
    void on_size_allocate (Gtk::Allocation &);
    bool on_scroll_event (GdkEventScroll * ev);

};

}           // namespace seq64

#endif      // SEQ64_PERFNAMES_HPP

/*
 * perfnames.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
