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
 * \updates       2016-08-13
 * \license       GNU GPLv2 or above
 *
 *  One possible idea would be a color configuration that would radically
 *  change drawing of the lines and pixmaps, opening up the way for night
 *  views and color schemes that match the desktop theme.
 */

#include "gui_palette_gtk2.hpp"         /* seq64::gui_palette_gtkw          */
#include "settings.hpp"                 /* seq64::rc() or seq64::usr()      */

namespace seq64
{

/**
 *  By default, the inverse color palette is not loaded.
 */

bool gui_palette_gtk2::m_is_inverse = false;

#ifdef USE_STATIC_MEMBER_COLORS         /* saves some space and time        */

STATIC_COLOR gui_palette_gtk2::m_black       = Color("black");
STATIC_COLOR gui_palette_gtk2::m_white       = Color("white");
STATIC_COLOR gui_palette_gtk2::m_grey        = Color("grey");
STATIC_COLOR gui_palette_gtk2::m_dk_grey     = Color("grey50");
STATIC_COLOR gui_palette_gtk2::m_lt_grey     = Color("light grey");
STATIC_COLOR gui_palette_gtk2::m_red         = Color("red");
STATIC_COLOR gui_palette_gtk2::m_orange      = Color("orange");
STATIC_COLOR gui_palette_gtk2::m_dk_orange   = Color("dark orange");
STATIC_COLOR gui_palette_gtk2::m_yellow      = Color("yellow");
STATIC_COLOR gui_palette_gtk2::m_green       = Color("green");
STATIC_COLOR gui_palette_gtk2::m_blue        = Color("blue");
STATIC_COLOR gui_palette_gtk2::m_dk_cyan     = Color("dark cyan");
STATIC_COLOR gui_palette_gtk2::m_blk_key     = Color("black");
STATIC_COLOR gui_palette_gtk2::m_wht_key     = Color("white");

#endif

/**
 *  Provides an alternate color palette, somewhat constrained by the colors
 *  in the font bitmaps.
 *
 *  Inverse is not a complete inverse.  It is more like a "night" mode.
 *  However, there are still some bright colors even in this mode.
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
        m_black       = Color("white");
        m_white       = Color("black");
        m_grey        = Color("grey");
        m_dk_grey     = Color("light grey");
        m_lt_grey     = Color("grey50");
        m_red         = Color("red");               // ("dark cyan");
        m_orange      = Color("blue");
        m_dk_orange   = Color("dark blue");
        m_yellow      = Color("blue");              // ("brown");
        m_green       = Color("green");             // ("red");
        m_blue        = Color("yellow");
        m_dk_cyan     = Color("red");               // ("dark cyan");
        m_blk_key     = Color("black");
        m_wht_key     = Color("grey");
        m_is_inverse  = true;
    }
    else
    {
        m_black       = Color("black");
        m_white       = Color("white");
        m_grey        = Color("grey");
        m_dk_grey     = Color("grey50");
        m_lt_grey     = Color("light grey");
        m_red         = Color("red");
        m_orange      = Color("orange");
        m_dk_orange   = Color("dark orange");
        m_yellow      = Color("yellow");
        m_green       = Color("green");
        m_blue        = Color("blue");
        m_dk_cyan     = Color("dark cyan");
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
#ifndef USE_STATIC_MEMBER_COLORS
    m_black             (Color("black")),
    m_white             (Color("white")),
    m_grey              (Color("grey")),
    m_dk_grey           (Color("grey50")),
    m_lt_grey           (Color("light grey")),
    m_red               (Color("red")),
    m_orange            (Color("orange")),
    m_dk_orange         (Color("dark orange")),
    m_yellow            (Color("yellow")),
    m_green             (Color("green")),
    m_blue              (Color("blue")),
    m_dk_cyan           (Color("dark cyan")),
    m_blk_key           (Color("black")),
    m_wht_key           (Color("black")),
#endif
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

