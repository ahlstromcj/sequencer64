#ifndef SEQ64_RECT_HPP
#define SEQ64_RECT_HPP

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
 * \file          rect.hpp
 *
 *  This module declares/defines the base class for a Sequencer64 rectangle.
 *
 * \library       sequencer64 application
 * \author        Chris Ahlstrom
 * \date          2017-09-16
 * \updates       2017-09-16
 * \license       GNU GPLv2 or above
 *
 *  Our version of the rectangle provides specific functionality not necessary
 *  found in, say the GdkMM rectangle.
 */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{

    class perform;

/**
 *  Supports a simple rectangle and some common manipulations needed by the
 *  user-interface.  Will eventually replace the gui_drawingarea_gtk2::rect
 *  structure.
 *
 *  One minor issue that may crop up in the transition from Gtkmm to Qt 5 is
 *  the exact meaning of the coordinates.  To be clarified later.  For now, it
 *  uses the current Gtkmm conventions.
 */

class rect
{

private:

    int m_x;        /**< The x coordinate of the first corner.              */
    int m_y;        /**< The y coordinate of the first corner.              */
    int m_width;    /**< The width of the rectangle.                        */
    int m_height;   /**< The height of the rectangle.                       */

public:

    rect ();
    rect (int x, int y, int width, int height);

    void get (int & x, int & y, int & width, int & height) const;
    void set (int x, int y, int width, int height);
    void xy_to_rect (int x1, int y1, int x2, int y2, rect & r);

    static void xy_to_rect_values
    (
        int x1, int y1, int x2, int y2,
        int & x, int & y, int & w, int & h
    );

private:

};

}           // namespace seq64

#endif      // SEQ64_RECT_HPP

/*
 * rect.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

