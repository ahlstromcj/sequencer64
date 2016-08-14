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
 * \file          gui_palette_gtk2.cpp
 *
 *  This module declares/defines the class for providing GTK/GDK colors.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-09-21
 * \updates       2016-08-14
 * \license       GNU GPLv2 or above
 *
 *  One possible idea would be a color configuration that would radically
 *  change drawing of the lines and pixmaps, opening up the way for night
 *  views and color schemes that match the desktop theme.
 */

#include "gui_palette_gtk2.hpp"         /* seq64::gui_palette_gtkw          */
#include "settings.hpp"                 /* seq64::rc() or seq64::usr()      */

#define STATIC_COLOR        gui_palette_gtk2::Color

namespace seq64
{

/**
 *  By default, the inverse color palette is not loaded.
 */

bool gui_palette_gtk2::m_is_inverse = false;

const STATIC_COLOR gui_palette_gtk2::m_black       = Color("black");
const STATIC_COLOR gui_palette_gtk2::m_white       = Color("white");
const STATIC_COLOR gui_palette_gtk2::m_red         = Color("red");
const STATIC_COLOR gui_palette_gtk2::m_orange      = Color("orange");
const STATIC_COLOR gui_palette_gtk2::m_dk_orange   = Color("dark orange");
const STATIC_COLOR gui_palette_gtk2::m_yellow      = Color("yellow");
const STATIC_COLOR gui_palette_gtk2::m_green       = Color("green");
const STATIC_COLOR gui_palette_gtk2::m_blue        = Color("blue");
const STATIC_COLOR gui_palette_gtk2::m_dk_cyan     = Color("dark cyan");

STATIC_COLOR gui_palette_gtk2::m_grey        = Color("grey");
STATIC_COLOR gui_palette_gtk2::m_dk_grey     = Color("grey50");
STATIC_COLOR gui_palette_gtk2::m_lt_grey     = Color("light grey");
STATIC_COLOR gui_palette_gtk2::m_blk_paint   = Color("black");
STATIC_COLOR gui_palette_gtk2::m_wht_paint   = Color("white");
STATIC_COLOR gui_palette_gtk2::m_blk_key     = Color("black");
STATIC_COLOR gui_palette_gtk2::m_wht_key     = Color("white");

/**
 *  Provides an alternate color palette, somewhat constrained by the colors
 *  in the font bitmaps.
 *
 *  Inverse is not a complete inverse.  It is more like a "night" mode.
 *  However, there are still some bright colors even in this mode.  Some
 *  colors, such as the selection color (orange) are the same in either mode.
 *
 * \param inverse
 *      If true, load the alternate palette.  Otherwise, load the default
 *      palette.
 */

void
gui_palette_gtk2::load_inverse_palette (bool inverse)
{
    if (inverse)
    {
        m_grey        = Color("grey");
        m_dk_grey     = Color("light grey");
        m_lt_grey     = Color("grey50");
        m_blk_paint   = Color("white");
        m_wht_paint   = Color("black");
        m_blk_key     = Color("black");
        m_wht_key     = Color("light grey");
        m_is_inverse  = true;
    }
    else
    {
        m_grey        = Color("grey");
        m_dk_grey     = Color("grey50");
        m_lt_grey     = Color("light grey");
        m_blk_paint   = Color("black");
        m_wht_paint   = Color("white");
        m_blk_key     = Color("black");
        m_wht_key     = Color("white");
        m_is_inverse  = false;
    }
}

/**
 *  Principal constructor.  In the constructor one can only allocate colors;
 *  get_window() returns 0 because this window has not yet been realized.
 *  Also note that the possible color names that can be used are found in
 *  /usr/share/X11/rgb.txt.
 */

gui_palette_gtk2::gui_palette_gtk2 ()
 :
    Gtk::DrawingArea    (),
    m_line_color        (Color("dark cyan")),           // alternative to black
    m_progress_color
    (
        Color(usr().progress_bar_colored() ? "dark cyan" : "black")
    ),
    m_bg_color          (),
    m_fg_color          ()
{
    Glib::RefPtr<Gdk::Colormap> colormap = get_default_colormap();
    colormap->alloc_color(const_cast<Color &>(m_black));
    colormap->alloc_color(const_cast<Color &>(m_white));
    colormap->alloc_color(const_cast<Color &>(m_grey));
    colormap->alloc_color(const_cast<Color &>(m_dk_grey));
    colormap->alloc_color(const_cast<Color &>(m_lt_grey));
    colormap->alloc_color(const_cast<Color &>(m_red));
    colormap->alloc_color(const_cast<Color &>(m_orange));
    colormap->alloc_color(const_cast<Color &>(m_dk_orange));
    colormap->alloc_color(const_cast<Color &>(m_yellow));
    colormap->alloc_color(const_cast<Color &>(m_green));
    colormap->alloc_color(const_cast<Color &>(m_blue));
    colormap->alloc_color(const_cast<Color &>(m_dk_cyan));
    colormap->alloc_color(const_cast<Color &>(m_blk_paint));
    colormap->alloc_color(const_cast<Color &>(m_wht_paint));
    colormap->alloc_color(const_cast<Color &>(m_blk_key));
    colormap->alloc_color(const_cast<Color &>(m_wht_key));
    colormap->alloc_color(const_cast<Color &>(m_line_color));
    colormap->alloc_color(const_cast<Color &>(m_progress_color));
}

/**
 *  Provides a destructor to delete allocated objects.
 */

gui_palette_gtk2::~gui_palette_gtk2 ()
{
    // Anything to do?
}

}           // namespace seq64

/*
 * gui_palette_gtk2.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

