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
 * \updates       2016-05-17
 * \license       GNU GPLv2 or above
 *
 */

#include "gui_palette_gtk2.hpp"         /* seq64::gui_palette_gtkw          */
#include "settings.hpp"                 /* seq64::rc() or seq64::usr()      */

namespace seq64
{

/**
 *  Principal constructor.
 *  In the constructor you can only allocate colors;
 *  get_window() returns 0 because this window has not be realized.
 */

gui_palette_gtk2::gui_palette_gtk2 ()
 :
    Gtk::DrawingArea    (),
    m_black             (Color("black")),
    m_white             (Color("white")),
    m_grey              (Color("grey")),
    m_dk_grey           (Color("grey50")),
    m_lt_grey           (Color("light grey")),
    m_red               (Color("red")),
    m_orange            (Color("orange")),
    m_yellow            (Color("yellow")),
    m_green             (Color("green")),
    m_blue              (Color("blue")),
    m_dk_cyan           (Color("dark cyan")),
    m_line_color        (Color("dark cyan")),           // alternative to black
    m_progress_color    (Color(usr().progress_bar_colored() ? "red" : "black")),
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
    colormap->alloc_color(const_cast<Color &>(m_yellow));
    colormap->alloc_color(const_cast<Color &>(m_green));
    colormap->alloc_color(const_cast<Color &>(m_blue));
    colormap->alloc_color(const_cast<Color &>(m_dk_cyan));
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

