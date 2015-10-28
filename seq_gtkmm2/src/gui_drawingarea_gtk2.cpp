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
 * \file          gui_drawingarea_gtk2.cpp
 *
 *  This module declares/defines the base class for drawing on
 *  Gtk::DrawingArea.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-09-21
 * \updates       2015-10-28
 * \license       GNU GPLv2 or above
 *
 */

#include <gtkmm/adjustment.h>

#include "gui_drawingarea_gtk2.hpp"
#include "perform.hpp"

namespace seq64
{

/**
 *  Provides a way to provide a dummy Gtk::Adjustment object, but not
 *  create one until it is actually needed, so that the Glib/Gtk
 *  infrastructure is ready for it.
 *
 *  This static object is used so we have an Adjustment to assign to
 *  the Adjustment members for classes that don't use them.  Clumsy?  We
 *  shall see.
 *
 *  Anyway, the parameters for this constructor are value, lower, upper,
 *  step-increment, and two more values.
 */

Gtk::Adjustment &
adjustment_dummy ()
{
    static Gtk::Adjustment sm_adjustment_dummy
    (
        0.0, 0.0, 1.0, 1.0, 1.0, 1.0
    );
    return sm_adjustment_dummy;
}


/**
 *  Perform-only constructor.
 */

gui_drawingarea_gtk2::gui_drawingarea_gtk2
(
    perform & perf,
    int windowx,
    int windowy
) :
    gui_palette_gtk2        (),
    m_gc                    (),
    m_window                (),
    m_vadjust               (adjustment_dummy()),
    m_hadjust               (adjustment_dummy()),
    m_pixmap                (),
    m_background            (),             // another pixmap
    m_foreground            (),             // another pixmap
    m_mainperf              (perf),
    m_window_x              (windowx),
    m_window_y              (windowy),
    m_current_x             (0),
    m_current_y             (0),
    m_drop_x                (0),
    m_drop_y                (0)
{
    gtk_drawarea_init();
}

/**
 *  Principal constructor.
 */

gui_drawingarea_gtk2::gui_drawingarea_gtk2
(
    perform & perf,
    Gtk::Adjustment & hadjust,
    Gtk::Adjustment & vadjust,
    int windowx,
    int windowy
) :
    gui_palette_gtk2        (),
    m_gc                    (),
    m_window                (),
    m_vadjust               (vadjust),
    m_hadjust               (hadjust),
    m_pixmap                (),
    m_background            (),             // another pixmap
    m_foreground            (),             // another pixmap
    m_mainperf              (perf),
    m_window_x              (windowx),
    m_window_y              (windowy),
    m_current_x             (0),
    m_current_y             (0),
    m_drop_x                (0),
    m_drop_y                (0)
{
    gtk_drawarea_init();
}

/**
 *  Provides a destructor to delete allocated objects.
 */

gui_drawingarea_gtk2::~gui_drawingarea_gtk2 ()
{
    // Empty body
}

/**
 *  Does basic initialization for each of the constructors.
 */

void
gui_drawingarea_gtk2::gtk_drawarea_init ()
{
    add_events
    (
        Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK |
        Gdk::POINTER_MOTION_MASK |
        Gdk::KEY_PRESS_MASK | Gdk::KEY_RELEASE_MASK |
        Gdk::FOCUS_CHANGE_MASK | Gdk::SCROLL_MASK |
        Gdk::ENTER_NOTIFY_MASK | Gdk::LEAVE_NOTIFY_MASK
    );
    set_double_buffered(false);
    if (m_window_x > 0 && m_window_y > 0)
        set_size_request(m_window_x, m_window_y);
}

/**
 *  A small wrapper function for readability in box-drawing.  It adds setting
 *  the foreground color to the draw_rectangle() function.
 *
 * \param c
 *      Provides the foreground color to set.
 *
 * \param x
 *      The x-coordinate of the origin.
 *
 * \param y
 *      The y-coordinate of the origin.
 *
 * \param lx
 *      The width of the box.
 *
 * \param ly
 *      The height of the box.
 *
 * \param fill
 *      If true, fill the rectangle with the current foreground color, as
 *      set by m_gc->set_foreground(color).  Defaults to true.
 */

void
gui_drawingarea_gtk2::draw_rectangle
(
    const Color & c, int x, int y, int lx, int ly, bool fill
)
{
    m_gc->set_foreground(c);
    draw_rectangle(x, y, lx, ly, fill);
}

/**
 *  A small wrapper function for readability in box-drawing on the pixmap.  It
 *  adds setting the foreground color to the draw_rectangle() function.
 *
 * \param c
 *      Provides the foreground color to set.
 *
 * \param x
 *      The x-coordinate of the origin.
 *
 * \param y
 *      The y-coordinate of the origin.
 *
 * \param lx
 *      The width of the box.
 *
 * \param ly
 *      The height of the box.
 *
 * \param fill
 *      If true, fill the rectangle with the current foreground color, as
 *      set by m_gc->set_foreground(color).  Defaults to true.
 */

void
gui_drawingarea_gtk2::draw_rectangle_on_pixmap
(
    const Color & c, int x, int y, int lx, int ly, bool fill
)
{
    m_gc->set_foreground(c);
    draw_rectangle_on_pixmap(x, y, lx, ly, fill);
}

/**
 *  A small wrapper function for readability in box-drawing on any drawable
 *  context.  It also supports setting the foreground color to the
 *  draw_rectangle() function.
 *
 *  We have a number of such functions:  for the main window, for the main
 *  pixmap, and for any drawing surface.  Is the small bit of conciseness
 *  worth it?
 *
 * \param drawable
 *      The surface on which to draw the box.
 *
 * \param c
 *      Provides the foreground color to set.
 *
 * \param x
 *      The x-coordinate of the origin.
 *
 * \param y
 *      The y-coordinate of the origin.
 *
 * \param lx
 *      The width of the box.
 *
 * \param ly
 *      The height of the box.
 *
 * \param fill
 *      If true, fill the rectangle with the current foreground color, as
 *      set by m_gc->set_foreground(color).  Defaults to true.
 */

void
gui_drawingarea_gtk2::draw_rectangle
(
    Glib::RefPtr<Gdk::Drawable> & drawable,
    const Color & c,
    int x, int y, int lx, int ly, bool fill
)
{
    m_gc->set_foreground(c);
    drawable->draw_rectangle(m_gc, fill, x, y, lx, ly);
}

/**
 *  A small wrapper function for readability in box-drawing on any pixmap
 *  context.  It also supports setting the foreground color to the
 *  draw_rectangle() function.
 *
 *  We have a number of such functions:  for the main window, for the main
 *  pixmap, and for any drawing surface.  Is the small bit of conciseness
 *  worth it?
 *
 * \param pixmap
 *      The surface on which to draw the box.
 *
 * \param c
 *      Provides the foreground color to set.
 *
 * \param x
 *      The x-coordinate of the origin.
 *
 * \param y
 *      The y-coordinate of the origin.
 *
 * \param lx
 *      The width of the box.
 *
 * \param ly
 *      The height of the box.
 *
 * \param fill
 *      If true, fill the rectangle with the current foreground color, as
 *      set by m_gc->set_foreground(color).  Defaults to true.
 */

void
gui_drawingarea_gtk2::draw_rectangle
(
    Glib::RefPtr<Gdk::Pixmap> & pixmap,
    const Color & c,
    int x, int y, int lx, int ly, bool fill
)
{
    m_gc->set_foreground(c);
    pixmap->draw_rectangle(m_gc, fill, x, y, lx, ly);
}

/**
 *  For this GTK callback, on realization of window, initialize the shiz.
 *  It allocates any additional resources that weren't initialized in the
 *  constructor.
 */

void
gui_drawingarea_gtk2::on_realize ()
{
    Gtk::DrawingArea::on_realize();         // base-class on_realize()
    m_window = get_window();                // more resources
    m_gc = Gdk::GC::create(m_window);       // graphics context?
    m_window->clear();
}

}           // namespace seq64

/*
 * gui_drawingarea_gtk2.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
