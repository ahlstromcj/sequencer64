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
 * \updates       2016-05-29
 * \license       GNU GPLv2 or above
 *
 *  This class supports the left side of the Performance window (also known
 *  as the Song window).  It also inherits from seqmenu as a means of
 *  supporting the same sequence-operation menus of the pattern editor.
 */

#include "gui_drawingarea_gtk2.hpp"
#include "seqmenu.hpp"

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
    class perfedit;

/**
 *  This class implements the left-side keyboard in the patterns window.  It
 *  inherits from gui_drawingarea_gtk2 to support the font, color, and other
 *  GUI functionality, and from seqmenu to support the right-click
 *  Edit/New/Cut right-click menu.
 *
 * \obsolete
 *      Note the usage of virtual base classes.  Since these can add some
 *      extra overhead, we should determine if we can do without the
 *      virtuality (and indeed it doesn't seem to be needed).
 */

class perfnames : public gui_drawingarea_gtk2, public seqmenu
{

    friend class perfedit;

private:

    /**
     *  Provides a link to the perfedit that created this object.  We want to
     *  support two perfedit windows, but the children of perfedit will have
     *  to communicate changes requiring a redraw through the parent.
     */

    perfedit & m_parent;

    /**
     *  Provides the number of the characters in the name box.  Pretty much
     *  hardwired to 24 at present.
     */

    int m_names_chars;

    /**
     *  Provides the "real" width of a character.  This value is obtained from
     *  a font-renderer accessor function.
     */

    int m_char_w;

    /**
     *  Provides the width of the "set number" box.  This used to be hardwired
     *  to 6 * 2 (character-width times two).
     */

    int m_setbox_w;

    /**
     *  Provides the width of the "name" box.  This used to be a weird
     *  calculation based on character width.
     */

    int m_namebox_w;

    /**
     *  Provides the width of the names box, which is the width of a character
     *  for 24 characters.
     */

    int m_names_x;

    /**
     *  Provides the height of the names box, which is hardwired to 24 pixels.
     *  This value was once 22 pixels, but we need a little extra room for our
     *  new font.  This extra room is compatible enough with the old font, as
     *  well.
     */

    int m_names_y;

    /**
     *  Provides the horizontal and vertical offsets of the text relative to
     *  the names box.  Currently hardwired.
     */

    int m_xy_offset;

    /**
     *  The number of sequences in a set, currently still hardwired to 32.
     */

    const int m_seqs_in_set;

    /**
     *  The maximum number of sequences, current 32 x 32 = 1024.
     */

    const int m_sequence_max;

    /**
     *  The offset from the 0th sequence, which is determined by the vertical
     *  view of the piano roll, controlled by the vertical scroll-bar.
     */

    int m_sequence_offset;

    /**
     *  Indicates if the given sequence is active or not.  If this really is
     *  the true meaning of this value, we ought to get it directly from the
     *  sequence if we can.
     */

    bool m_sequence_active[c_max_sequence];

public:

    perfnames
    (
        perform & p,
        perfedit & parent,
        Gtk::Adjustment & vadjust
    );

    /**
     *  Let's provide a do-nothing virtual destructor.
     */

    virtual ~perfnames ()
    {
        // I got nothin'
    }

    void redraw_dirty_sequences ();

private:

    void enqueue_draw ();
    int convert_y (int y);
    void draw_sequences ();
    void draw_sequence (int sequence);
    void change_vert ();

    /**
     *  Redraw the given sequence.  This function is a virtual function of
     *  seqmenu that must be overridden in this class.
     *
     * \param sequence
     *      Provides the number of the sequence to be redrawn.
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

};          // class perfnames

}           // namespace seq64

#endif      // SEQ64_PERFNAMES_HPP

/*
 * perfnames.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

