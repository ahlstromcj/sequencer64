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
 * \file          font.cpp
 *
 *  This module declares/defines the base class for font handling.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2015-09-13
 * \license       GNU GPLv2 or above
 *
 *  The interesting thing about this font class is that it implements a
 *  font as a relatively large bitmap that holds each of the characters in
 *  the font in a kind of grid.
 *
 */

#include "easy_macros.h"
#include "font.hpp"
#include "pixmaps/font_w.xpm"           /* white on black (inverse video)   */
#include "pixmaps/font_b.xpm"           /* black on white                   */
#include "pixmaps/font_yb.xpm"          /* yellow on black (inverse video)  */
#include "pixmaps/font_y.xpm"           /* black on yellow                  */

namespace seq64
{

/**
 *    Rote default constructor.
 */

font::font ()
 :
    m_pixmap        (nullptr),
    m_black_pixmap  (),
    m_white_pixmap  (),
    m_b_on_y_pixmap (),
    m_y_on_b_pixmap (),
    m_clip_mask     ()
{
   // empty body
}

/**
 *  Initialization function for a window on which fonts will be drawn.
 *  This function loads two pixmaps that contain the characters to be used
 *  to draw text strings.  Both pixmaps provide a 16 x 16 grid of boxes,
 *  and each box contains one of the 256 characters in this font set.
 *
 *  One pixmap has white characters on a black background, and other other
 *  has black characters on a white background.  See the descriptions of
 *  the c_text_x and c_text_y variables in the globals module.
 */

void
font::init (Glib::RefPtr<Gdk::Window> a_window)
{
    m_black_pixmap = Gdk::Pixmap::create_from_xpm
    (
        a_window->get_colormap(), m_clip_mask, font_b_xpm
    );
    m_white_pixmap = Gdk::Pixmap::create_from_xpm
    (
        a_window->get_colormap(), m_clip_mask, font_w_xpm
    );
    m_b_on_y_pixmap = Gdk::Pixmap::create_from_xpm
    (
        a_window->get_colormap(), m_clip_mask, font_y_xpm
    );
    m_y_on_b_pixmap = Gdk::Pixmap::create_from_xpm
    (
        a_window->get_colormap(), m_clip_mask, font_yb_xpm
    );
}

/**
 *  Draws a text string. This function grabs the proper font bitmap,
 *  extracts the current character pixmap from it, and slaps it down where
 *  it needs to be to render the character in the string.
 *
 * \param a_gc
 *      Provides the graphics context for drawing the text using GTK+.
 *
 * \param x
 *      The horizontal location of the text.
 *
 * \param y
 *      The vertical location of the text.
 *
 * \param a_draw
 *      The drawable object on which to draw the text.
 *
 * \param str
 *      The string to draw.  Should use a constant string reference
 *      instead.
 *
 * \param col
 *      The font color to use to draw the string.  The only support values
 *      are font::BLACK and font::WHITE, and the correct colors are
 *      provided by selecting one of two font pixmaps, as described in the
 *      init() function.
 */

void
font::render_string_on_drawable
(
    Glib::RefPtr<Gdk::GC> a_gc,
    int x, int y,
    Glib::RefPtr<Gdk::Drawable> a_draw,
    const char * str,
    font::Color col
)
{
    int length = 0;
    if (not_nullptr(str))
        length = strlen(str);

    /*
     * The width is identical to c_text_x, but the height is not identical
     * to c_text_y.  Using the latter values causes artifacts on the
     * pattern grid.
     */

    int font_w = 6;                     // c_text_x == 6
    int font_h = 10;                    // c_text_y == 12
    if (col == font::BLACK)
        m_pixmap = &m_black_pixmap;
    else if (col == font::WHITE)
        m_pixmap = &m_white_pixmap;
    else if (col == font::BLACK_ON_YELLOW)
        m_pixmap = &m_b_on_y_pixmap;
    else if (col == font::YELLOW_ON_BLACK)
        m_pixmap = &m_y_on_b_pixmap;
    else
        m_pixmap = &m_black_pixmap; // user lied, provide a legal pointer

    for (int i = 0; i < length; ++i)
    {
        unsigned char c = (unsigned char) str[i];
        int pixbuf_index_x = c % 16;    // number of grids horizontally
        int pixbuf_index_y = c / 16;    // number of grids vertically
        pixbuf_index_x *= 9;            // width of grid (letter is 6 pixels)
        pixbuf_index_x += 2;            // add 2 for border?
        pixbuf_index_y *= 13;           // height of grid (letter is 12 pixels)
        pixbuf_index_y += 2;            // add 2 for border?
        a_draw->draw_drawable
        (
            a_gc, *m_pixmap, pixbuf_index_x, pixbuf_index_y,
            x + (i * font_w), y, font_w, font_h
        );
    }
}

}           // namespace seq64

/*
 * font.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
