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
 * \file          rect.cpp
 *
 *  This module declares/defines the concrete class for a rectangle.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2017-09-16
 * \updates       2017-09-16
 * \license       GNU GPLv2 or above
 *
 */

#include "rect.hpp"                     /* seq64::rect                      */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 * \defaultctor
 */

rect::rect ()
 :
    m_x         (0),
    m_y         (0),
    m_width     (0),
    m_height    (0)
{
    // Empty body
}

/**
 *  Principal constructor.
 *
 * \param x
 *      The x coordinate.
 *
 * \param y
 *      The y coordinate.
 *
 * \param width
 *      The width value.
 *
 * \param height
 *      The width value.
 */

rect::rect (int x, int y, int width, int height)
 :
    m_x         (x),
    m_y         (y),
    m_width     (width),
    m_height    (height)
{
    // Empty body
}

/**
 *  Gets the rectangle values for primitive callers that don't store them as
 *  a rectangle.
 *
 * \param [out] x
 *      The destination x coordinate.
 *
 * \param [out] y
 *      The destination y coordinate.
 *
 * \param [out] width
 *      The destination width value.
 *
 * \param [out] height
 *      The destination width value.
 */

void
rect::get (int & x, int & y, int & width, int & height) const
{
    x = m_x;
    y = m_y;
    width = m_width;
    height = m_height;
}

/**
 *  Sets all of the members of the rectangle directly.
 *
 * \param x
 *      The x coordinate.
 *
 * \param y
 *      The y coordinate.
 *
 * \param width
 *      The width value.
 *
 * \param height
 *      The width value.
 */

void
rect::set (int x, int y, int width, int height)
{
    m_x = x;
    m_y = y;
    m_width = width;
    m_height = height;
}

/**
 *  Converts rectangle corner coordinates to a rect object, which includes
 *  width and height.
 *
 * \param x1
 *      The x value of the first corner.
 *
 * \param y1
 *      The y value of the first corner.
 *
 * \param x2
 *      The x value of the second corner.
 *
 * \param y2
 *      The y value of the second corner.
 *
 * \param [out] r
 *      The destination for the coordinate, width, and height.
 */

void
rect::xy_to_rect (int x1, int y1, int x2, int y2, rect & r)
{
    if (x1 < x2)
    {
        r.m_x = x1;
        r.m_width = x2 - x1;
    }
    else
    {
        r.m_x = x2;
        r.m_width = x1 - x2;
    }
    if (y1 < y2)
    {
        r.m_y = y1;
        r.m_height = y2 - y1;
    }
    else
    {
        r.m_y = y2;
        r.m_height = y1 - y2;
    }
}

/**
 *  Converts rectangle corner coordinates to a starting coordinate, plus a
 *  width and height.  This function checks the mins / maxes, and then fills
 *  in the x, y, width, and height values.  It picks the lowest x and y
 *  coordinate to use as the corner coordinate, so that the width and height
 *  are always positive.
 *
 * \param x1
 *      The x value of the first corner.
 *
 * \param y1
 *      The y value of the first corner.
 *
 * \param x2
 *      The x value of the second corner.
 *
 * \param y2
 *      The y value of the second corner.
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
rect::xy_to_rect_get
(
    int x1, int y1, int x2, int y2,
    int & x, int & y, int & w, int & h
)
{
    if (x1 < x2)
    {
        x = x1;
        w = x2 - x1;
    }
    else
    {
        x = x2;
        w = x1 - x2;
    }
    if (y1 < y2)
    {
        y = y1;
        h = y2 - y1;
    }
    else
    {
        y = y2;
        h = y1 - y2;
    }
}

#if 0

/**
 *  Converts a tick/note box to an x/y rectangle.
 *
 *  We should refactor this function to use the utility class seqroll::rect as
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
 * \param [out] destination
 *      The destination rectangle.
 */

void
rect::convert_tn_box_to_rect
(
    midipulse tick_s, midipulse tick_f, int note_h, int note_l
)
{
    int x1, y1, x2, y2;
    convert_tn(tick_s, note_h, x1, y1);     /* convert box to X,Y values */
    convert_tn(tick_f, note_l, x2, y2);
//  int x, y, w, h;
//  rect::xy_to_rect_get(x1, y1, x2, y2, x, y, w, h);
//  h += c_key_y;
//  set(x, y, w, h);
    xy_to_rect(x1, y1, x2, y2);
}

#endif  // 0

}           // namespace seq64

/*
 * rect.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
