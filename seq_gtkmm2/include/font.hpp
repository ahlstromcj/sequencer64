#ifndef SEQ64_FONT_HPP
#define SEQ64_FONT_HPP

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
 * \file          font.hpp
 *
 *  This module declares/defines the base class for font handling.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2015-10-25
 * \license       GNU GPLv2 or above
 *
 */

#include <gtkmm/image.h>

namespace seq64
{

/**
 *  This class provides a wrapper for rendering fonts that are encoded as
 *  a 16 x 16 pixmap file in XPM format.
 */

class font
{

public:

    /**
     *  A simple enumeration to describe the basic colors used in writing
     *  text.  Basically, these two values cause the selection of one or
     *  another pixmap (font_b_xpm and font_w_xpm).  We've added two more
     *  pixmaps to draw black text on a yellow background (font_y.xpm) and
     *  yellow text on a black background (font_yb.xpm).
     *
     * \var BLACK
     *      The first supported color.  A black font on a white
     *      background.
     *
     * \var WHITE
     *      The second supported color. A white font on a black
     *      background.
     *
     * \var BLACK_ON_YELLOW
     *      A new color, for drawing black text on a yellow background.
     *
     * \var YELLOW_ON_BLACK
     *      A new color, for drawing yellow text on a black background.
     */

    enum Color
    {
        BLACK,              /* font_b.xpm  */
        WHITE,              /* font_w.xpm  */
        BLACK_ON_YELLOW,    /* font_y.xpm  */
        YELLOW_ON_BLACK     /* font_yb.xpm */
    };

private:

    /**
     *  Specifies the exact width of a character cell, in pixels.  Currently
     *  defaults to cf_text_w = 6.
     */

    int m_font_w;

    /**
     *  Specifies the exact height of a character cell, in pixels.  Currently
     *  defaults to cf_text_h = 10.
     */

    int m_font_h;

    /**
     *  Points to the current pixmap (m_black_pixmap or m_white_pixmap)
     *  to use to render a string.  This member used to be an object, but
     *  it's probably a bit faster to just use a pointer (or a reference).
     */

    Glib::RefPtr<Gdk::Pixmap> * m_pixmap;

    /**
     *  The pixmap in the file <tt> src/pixmaps/font_b.xpm </tt> is loaded
     *  into this object.  It contains a black font on a white background.
     */

    Glib::RefPtr<Gdk::Pixmap> m_black_pixmap;

    /**
     *  The pixmap in the file <tt> src/pixmaps/font_b.xpm </tt> is loaded
     *  into this object.  It contains a black font on a white background.
     */

    Glib::RefPtr<Gdk::Pixmap> m_white_pixmap;

    /**
     *  The pixmap in the file <tt> src/pixmaps/font_y.xpm </tt> is loaded
     *  into this object.  It contains a black font on a yellow background.
     */

    Glib::RefPtr<Gdk::Pixmap> m_b_on_y_pixmap;

    /**
     *  The pixmap in the file <tt> src/pixmaps/font_yb.xpm </tt> is loaded
     *  into this object.  It contains a yellow font on a black background.
     */

    Glib::RefPtr<Gdk::Pixmap> m_y_on_b_pixmap;

    /**
     *  This object is instantiated as a default object.  All we know is
     *  it seems to be a requirement for creating a pixmap object from an
     *  XMP file.
     */

    Glib::RefPtr<Gdk::Bitmap> m_clip_mask;

public:

    font ();

    void init (Glib::RefPtr<Gdk::Window> a_window);
    void render_string_on_drawable
    (
        Glib::RefPtr<Gdk::GC> m_gc,
        int x,
        int y,
        Glib::RefPtr<Gdk::Drawable> a_draw,
        const char * str,
        font::Color col
    );

};

/*
 * Global symbols.  The p_font_renderer pointer is defined in the main
 * module, sequencer26.cpp.
 */

extern font * p_font_renderer;

}           // namespace seq64

#endif      // SEQ64_FONT_HPP

/*
 * font.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
