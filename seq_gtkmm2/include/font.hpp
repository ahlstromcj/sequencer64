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
 * \updates       2015-11-25
 * \license       GNU GPLv2 or above
 *
 */

#include <gtkmm/image.h>

#include "easy_macros.h"                /* is_nullptr() */

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
     *
     * \var BLACK_ON_CYAN
     *      A new color, for drawing black text on a cyan background.
     *
     * \var CYAN_ON_BLACK
     *      A new color, for drawing cyan text on a black background.
     */

    enum Color
    {
        BLACK,              /* font_b.xpm or wenfont_b.xpm  */
        WHITE,              /* font_w.xpm or wenfont_w.xpm  */
        BLACK_ON_YELLOW,    /* font_y.xpm or wenfont_y.xpm  */
        YELLOW_ON_BLACK,    /* font_yb.xpm or wenfont_y.xpm */
        BLACK_ON_CYAN,      /* cyan_wenfont_y.xpm           */
        CYAN_ON_BLACK       /* cyan_wenfont_y.xpm           */
    };

private:

    /**
     *  If true, use the new font, which is a little bit more modern looking.
     */

    bool m_use_new_font;

    /**
     *  Specifies the cell width of the whole cell.
     */

    int m_cell_w;

    /**
     *  Specfies the cell height of the whole cell.
     */

    int m_cell_h;

    /**
     *  Specifies the exact width of a character cell, in pixels.  Currently
     *  defaults to cf_text_w = 6.  Note that a lot of stuff depends on this
     *  being 6 at present, even with our new, slightly wider, font.
     */

    int m_font_w;

    /**
     *  Specifies the exact height of a character cell, in pixels.  Currently
     *  defaults to cf_text_h = 10.  Note that a lot of stuff depends on this
     *  being 10 at present, even with our new, slightly wider, font.
     *  But some of the drawing code doesn't use the character height, but the
     *  padded character height.
     */

    int m_font_h;

    /**
     *  Provides an ad hoc small horizontal or vertical offset for printing
     *  strings.
     */

    int m_offset;

    /**
     *  Provides a common constant used by much of the drawing code, but only
     *  marginally related to the padded character height.
     */

    int m_padded_h;

    /**
     *  Points to the current pixmap (m_black_pixmap or m_white_pixmap)
     *  to use to render a string.  This member used to be an object, but
     *  it's probably a bit faster to just use a pointer (or a reference).
     */

    mutable const Glib::RefPtr<Gdk::Pixmap> * m_pixmap;

    /**
     *  The pixmap in the file <tt> src/pixmaps/font_b.xpm </tt> is loaded
     *  into this object.  It contains a black font on a white background.
     *  The new-style font, if selected, is in the
     *  <tt> resources/pixmaps/wenfont_b.xmp </tt> pixmap.
     */

    Glib::RefPtr<Gdk::Pixmap> m_black_pixmap;

    /**
     *  The pixmap in the file <tt> src/pixmaps/font_w.xpm </tt> is loaded
     *  into this object.  It contains a black font on a white background.
     *  The new-style font, if selected, is in the
     *  <tt> resources/pixmaps/wenfont_w.xmp </tt> pixmap.
     */

    Glib::RefPtr<Gdk::Pixmap> m_white_pixmap;

    /**
     *  The pixmap in the file <tt> src/pixmaps/font_y.xpm </tt> is loaded
     *  into this object.  It contains a black font on a yellow background.
     *  The new-style font, if selected, is in the
     *  <tt> resources/pixmaps/wenfont_y.xmp </tt> pixmap.
     */

    Glib::RefPtr<Gdk::Pixmap> m_b_on_y_pixmap;

    /**
     *  The pixmap in the file <tt> src/pixmaps/font_yb.xpm </tt> is loaded
     *  into this object.  It contains a yellow font on a black background.
     *  The new-style font, if selected, is <tt>
     *  resources/pixmaps/wenfont_yb.xmp </tt> pixmap.
     */

    Glib::RefPtr<Gdk::Pixmap> m_y_on_b_pixmap;

    /**
     *  The pixmap in the file <tt> src/pixmaps/cyan_wenfont_y.xpm </tt> is
     *  loaded into this object.  It contains a black font on a cyan
     *  background.  It is available only for the new font-style.
     */

    Glib::RefPtr<Gdk::Pixmap> m_b_on_c_pixmap;

    /**
     *  The pixmap in the file <tt> src/pixmaps/cyan_wenfont_yb.xpm </tt> is
     *  loaded into this object.  It contains a cyan font on a black
     *  background.  It is available only for the new font-style.
     */

    Glib::RefPtr<Gdk::Pixmap> m_c_on_b_pixmap;

    /**
     *  This object is instantiated as a default object.  All we know is
     *  it seems to be a requirement for creating a pixmap object from an
     *  XMP file.
     */

    Glib::RefPtr<Gdk::Bitmap> m_clip_mask;

public:

    font ();

    void init (Glib::RefPtr<Gdk::Window> windo);
    void render_string_on_drawable
    (
        Glib::RefPtr<Gdk::GC> m_gc,
        int x,
        int y,
        Glib::RefPtr<Gdk::Drawable> drawable,
        const char * str,
        font::Color col
    ) const;

    /**
     * \getter m_font_w
     */

    int char_width () const
    {
        return m_font_w;
    }

    /**
     * \getter m_font_h
     */

    int char_height () const
    {
        return m_font_h;
    }

    /**
     * \getter m_padded_h
     */

    int padded_height () const
    {
        return m_padded_h;
    }

};

/**
 *  The p_font_renderer pointer was once created in the main module,
 *  sequencer64.cpp.  We've going to render this pointer obsolete, though, and
 *  use a smart factory function to ensure the existence of this pointer, and
 *  return a reference to the font object.
 *
 *  We wanted to make the font a const object, but mainwid::on_realize() calls
 *  the font::init() function with its window object, and using const is
 *  impractical.  We don't want to force every caller to deal with the
 *  overhead of passing even a null window pointer, either.
 *
 *  However, at some point we need some quarantee that the init() function is
 *  called before rendering a string.  Right now, we guarantee it only by
 *  build order.
 */

inline /* const */ font & font_render ()
{
    static font * sp_font_renderer = nullptr;
    if (is_nullptr(sp_font_renderer))
    {
        sp_font_renderer = new font;
        if (not_nullptr(sp_font_renderer))
        {
            // sp_font_renderer->init(m_window)
        }
        else
        {
            errprint("could not create the application font object");
        }
    }
    return *sp_font_renderer;
}

}           // namespace seq64

#endif      // SEQ64_FONT_HPP

/*
 * font.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

