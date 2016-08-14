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
 * \updates       2016-08-14
 * \license       GNU GPLv2 or above
 *
 *  The interesting thing about this font class is that font files are not
 *  used.  Instead, the fonts are provided by pixmaps in the <tt>
 *  resources/pixmaps </tt> directory: Fonts are implemented as a relatively
 *  large bitmap that holds each of the font characters in a grid.  The grid
 *  is w = 16 cells wide by h = 16 cells high, to represent 256 characters.
 *  The coordinate of character c is (c % w, c / h).
 *
 *  Each character cell, including the unused border, is 10 pixels wide and
 *  14 pixels high.  Inside this cell is a 1-pixel blank border, or "inner
 *  padding".  The actual character size is 6 x 10 pixels.
 *
 *  We've also added some pixmaps for black lettering on a yellow background,
 *  and for yellow lettering on a black background, to handle "empty"
 *  patterns, which have no events, just some meta information.
 *
 *  Finally, we created another whole set of font pixmaps for an anti-aliased
 *  font.  This new font is selected by default.  The new pixmaps aren't quite
 *  the same size, and the lettering is marginally larger, so alternative
 *  sizing variables are defined if the new font is in force.
 *
 *  The new font was created by using a short C program to create a 16x16 text
 *  file, importing it into a LibreOffice spreadsheet, setting the text to the
 *  WenQuanYi Zen Hei Mono bold font, adding some of the extended ASCII
 *  characters used in Seq24's old PC font, sizing the grid as closely as
 *  possible to the original, and using the GIMP to generate the XPM files.
 *  This process was actually easier than trying to figure out how to write
 *  text using gtkmm.  See the contrib/ascii-matrix.* files for the source
 *  material.
 *
 *  Also note we had to reduce the specified size of the characters slightly
 *  to render them properly in the cramped spaces of the GUI.
 *
 * \warning
 *      Some global sizes, such as c_names_x, may depend on aspects of the
 *      character size!
 */

#include "easy_macros.h"
#include "font.hpp"
#include "settings.hpp"                 /* seq64::rc() or seq64::usr()      */
#include "gui_palette_gtk2.hpp"

/*
 * Old and New values for the fonts, and additional font files for cyan
 * coloring, available only with the new font.
 */

#include "pixmaps/font_w.xpm"           /* white on black (inverse video)   */
#include "pixmaps/font_b.xpm"           /* black on white                   */
#include "pixmaps/font_yb.xpm"          /* yellow on black (inverse video)  */
#include "pixmaps/font_y.xpm"           /* black on yellow                  */
#include "pixmaps/wenfont_w.xpm"        /* white on black (inverse video)   */
#include "pixmaps/wenfont_b.xpm"        /* black on white                   */
#include "pixmaps/wenfont_yb.xpm"       /* yellow on black (inverse video)  */
#include "pixmaps/wenfont_y.xpm"        /* black on yellow                  */
#include "pixmaps/cyan_wenfont_yb.xpm"  /* cyan on black (inverse video)    */
#include "pixmaps/cyan_wenfont_y.xpm"   /* black on cyan                    */

/*
 * Font grid sizes.
 */

static const int cf_grid_w = 16;    /**< Number of horizontal font cells.   */
static const int cf_grid_h = 16;    /**< Number of vertical font cells.     */

/*
 * New values for the font.
 */

static const int cf_cell_w = 11;    /**< Full width of character cell.      */
static const int cf_cell_h = 15;    /**< Full height of character cell.     */
static const int cf_offset =  3;    /**< x, y offsets of top left pixel.    */
static const int cf_text_w =  6;    /**< Doesn't include inner padding.     */
static const int cf_text_h = 11;    /**< Doesn't include inner padding.     */

/*
 * Old values for the font.
 */

static const int co_cell_w =  9;    /**< Full width of character cell.      */
static const int co_cell_h = 13;    /**< Full height of character cell.     */
static const int co_offset =  2;    /**< x, y offsets of top left pixel.    */
static const int co_text_w =  6;    /**< Doesn't include inner padding.     */
static const int co_text_h = 10;    /**< Doesn't include inner padding.     */

namespace seq64
{

/**
 *    Rote default constructor, except that it does add 1 to the cf_text_h or
 *    co_text_h values to use in m_padded_h.
 */

font::font ()
 :
    m_use_new_font  (usr().use_new_font()),
    m_cell_w        (cf_cell_w),
    m_cell_h        (cf_cell_h),
    m_font_w        (cf_text_w),
    m_font_h        (cf_text_h),
    m_offset        (cf_offset),
    m_padded_h      (cf_text_h + 1),    /* too tricky */
    m_pixmap        (nullptr),
    m_black_pixmap  (),
    m_white_pixmap  (),
    m_b_on_y_pixmap (),
    m_y_on_b_pixmap (),
    m_clip_mask     ()
{
    if (! m_use_new_font)
    {
        m_cell_w    = co_cell_w;
        m_cell_h    = co_cell_h;
        m_font_w    = co_text_w;
        m_font_h    = co_text_h;
        m_offset    = co_offset;
        m_padded_h  = co_text_h + 1;
    }
}

/**
 *  Initialization function for a window on which fonts will be drawn.  This
 *  function loads four pixmaps that contain the characters to be used to draw
 *  text strings.
 *
 *  One pixmap has white characters on a black background, one has black
 *  characters on a white background, one has yellow characters on a black
 *  background, and one has black characters on a yellow background.
 *
 * \param wp
 *      Provides the windows pointer for the window that holds the color map.
 */

void
font::init (Glib::RefPtr<Gdk::Window> wp)
{
    if (m_use_new_font)
    {
        m_black_pixmap = Gdk::Pixmap::create_from_xpm
        (
            wp->get_colormap(), m_clip_mask, wenfont_b_xpm
        );
        m_white_pixmap = Gdk::Pixmap::create_from_xpm
        (
            wp->get_colormap(), m_clip_mask, wenfont_w_xpm
        );
        m_b_on_y_pixmap = Gdk::Pixmap::create_from_xpm
        (
            wp->get_colormap(), m_clip_mask, wenfont_y_xpm
        );
        m_y_on_b_pixmap = Gdk::Pixmap::create_from_xpm
        (
            wp->get_colormap(), m_clip_mask, wenfont_yb_xpm
        );
        m_b_on_c_pixmap = Gdk::Pixmap::create_from_xpm
        (
            wp->get_colormap(), m_clip_mask, cyan_wenfont_y_xpm
        );
        m_c_on_b_pixmap = Gdk::Pixmap::create_from_xpm
        (
            wp->get_colormap(), m_clip_mask, cyan_wenfont_yb_xpm
        );
    }
    else
    {
        m_black_pixmap = Gdk::Pixmap::create_from_xpm
        (
            wp->get_colormap(), m_clip_mask, font_b_xpm
        );
        m_white_pixmap = Gdk::Pixmap::create_from_xpm
        (
            wp->get_colormap(), m_clip_mask, font_w_xpm
        );
        m_b_on_y_pixmap = Gdk::Pixmap::create_from_xpm
        (
            wp->get_colormap(), m_clip_mask, font_y_xpm
        );
        m_y_on_b_pixmap = Gdk::Pixmap::create_from_xpm
        (
            wp->get_colormap(), m_clip_mask, font_yb_xpm
        );
        m_b_on_c_pixmap = Gdk::Pixmap::create_from_xpm
        (
            wp->get_colormap(), m_clip_mask, font_y_xpm
        );
        m_c_on_b_pixmap = Gdk::Pixmap::create_from_xpm
        (
            wp->get_colormap(), m_clip_mask, font_yb_xpm
        );
    }
}

/**
 *  Draws a text string. This function grabs the proper font bitmap,
 *  extracts the current character pixmap from it, and slaps it down where
 *  it needs to be to render the character in the string.
 *
 * \param gc
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
 *      The font color to use to draw the string.  The supported values are
 *      font::BLACK, font::WHITE, font::BLACK_ON_YELLOW,
 *      font::YELLOW_ON_BLACK.  The actual correct colors are provided by
 *      selecting one of four font pixmaps, as described in the init()
 *      function.
 *
 * \param invert
 *      If true, apply color inversion, if specified.
 */

void
font::render_string_on_drawable
(
    Glib::RefPtr<Gdk::GC> gc,
    int x, int y,
    Glib::RefPtr<Gdk::Drawable> a_draw,
    const char * str,
    font::Color col,
    bool invert
) const
{
    int length = 0;
    if (not_nullptr(str))
        length = strlen(str);

    if (m_use_new_font)
        y += 1;                         /* make minor correction */
    else
        y += 2;                         /* make minor correction */

    if (col == font::BLACK)
        m_pixmap = &m_black_pixmap;
    else if (col == font::WHITE)
        m_pixmap = &m_white_pixmap;
    else if (col == font::BLACK_ON_YELLOW)
        m_pixmap = &m_b_on_y_pixmap;
    else if (col == font::YELLOW_ON_BLACK)
        m_pixmap = &m_y_on_b_pixmap;
    else if (col == font::BLACK_ON_CYAN)
        m_pixmap = &m_b_on_c_pixmap;
    else if (col == font::CYAN_ON_BLACK)
        m_pixmap = &m_c_on_b_pixmap;
    else
        m_pixmap = &m_black_pixmap;     /* user lied, provide legal pointer */

    if (gui_palette_gtk2::is_inverse() && invert)
        gc->set_function(Gdk::COPY_INVERT);  /* XOR or INVERT?              */

    for (int k = 0; k < length; ++k)
    {
        int c = int(str[k]);
        int pixbuf_index_x = c % cf_grid_w;
        int pixbuf_index_y = c / cf_grid_h;
        pixbuf_index_x *= m_cell_w;     /* width of grid (letter=6 pixels)  */
        pixbuf_index_x += m_offset;     /* add around 2 for border?         */
        pixbuf_index_y *= m_cell_h;     /* height of grid (letter=10 pixel) */
        pixbuf_index_y += m_offset;     /* add around 2 for border?         */
        a_draw->draw_drawable
        (
            gc, *m_pixmap, pixbuf_index_x, pixbuf_index_y,
            x + (k * m_font_w), y, m_font_w, m_font_h
        );
    }
    if (gui_palette_gtk2::is_inverse() && invert)
        gc->set_function(Gdk::COPY);    /* not NOOP or SET                  */
}

}           // namespace seq64

/*
 * font.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

