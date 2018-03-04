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
 * \updates       2018-03-03
 * \license       GNU GPLv2 or above
 *
 *  Our version of the rectangle provides specific functionality not necessary
 *  found in, say the GdkMM rectangle.
 */

#include "midibyte.hpp"                 /* midipulse typedef                */

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

    /**
     *
     */

    void clear ()
    {
        m_x = m_y = m_width = m_height = 0;
    }

    static void xy_to_rect (int x1, int y1, int x2, int y2, rect & r);
    static void xy_to_rect_get
    (
        int x1, int y1, int x2, int y2,
        int & x, int & y, int & w, int & h
    );

    /**
     *
     */

    void xy_to_rect (int x1, int y1, int x2, int y2)
    {
        xy_to_rect(x1, y1, x2, y2, *this);
    }

    /**
     * \getter m_x
     */

    int x () const
    {
        return m_x;
    }

    /**
     * \setter m_x
     *      The width is assumed to be unchanged by this function.
     */

    void x (int v)
    {
        m_x = v;
    }

    /**
     * \setter m_x
     *      Provides a setter that uses the parameter to increment the member.
     *      The width is assumed to be unchanged by this function.
     */

    void x_incr (int v)
    {
        m_x += v;
    }

    /**
     * \getter m_y
     */

    int y () const
    {
        return m_y;
    }

    /**
     * \setter m_y
     *      The height is assumed to be unchanged by this function.
     */

    void y (int v)
    {
        m_y = v;
    }

    /**
     * \setter m_y
     *      Provides a setter that uses the parameter to increment the member.
     *      The height is assumed to be unchanged by this function.
     */

    void y_incr (int v)
    {
        m_y += v;
    }

    /**
     * \getter m_width
     */

    int width () const
    {
        return m_width;
    }

    /**
     * \setter m_width
     */

    void width (int w)
    {
        m_width = w;
    }

    /**
     * \setter m_width
     */

    void incr_width (int w)
    {
        m_width += w;
    }

    /**
     * \getter m_height
     */

    int height () const
    {
        return m_height;
    }

    /**
     * \setter m_height
     */

    void height (int h)
    {
        m_height = h;
    }

    /**
     * \setter m_height
     */

    void incr_height (int h)
    {
        m_height += h;
    }

    void xy_incr (int xv, int yv)
    {
        m_x += xv;
        m_y += yv;
    }

#if 0                               // see the cpp module for comments
    void convert_tn_box_to_rect
    (
        midipulse tick_s, midipulse tick_f, int note_h, int note_l
    );
#endif

private:

    /**
     *  The calculated width is always positive.  Follows the conventions of
     *  the xy_to_rect_get() function.
     */

    static int calculated_width (int x1, int x2)
    {
        return (x1 < x2) ? (x2 - x1) : (x1 - x2) ;
    }

    /**
     *  The calculated height is always positive.  Follows the conventions of
     *  the xy_to_rect_get() function.
     */

    static int calculated_height (int y1, int y2)
    {
        return (y1 < y2) ? (y2 - y1) : (y1 - y2) ;
    }

};

}           // namespace seq64

#endif      // SEQ64_RECT_HPP

/*
 * rect.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

