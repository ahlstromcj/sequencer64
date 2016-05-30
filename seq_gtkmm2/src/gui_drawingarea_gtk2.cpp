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
 * \updates       2016-05-30
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
 *  A small wrapper function for readability in box-drawing on the pixmap.
 *  It uses Gtk to get the proper background styling for the rectange.
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
gui_drawingarea_gtk2::draw_normal_rectangle_on_pixmap
(
    int x, int y, int lx, int ly, bool fill
)
{
    m_pixmap->draw_rectangle
    (
        get_style()->get_bg_gc(Gtk::STATE_NORMAL), fill, x, y, lx, ly
    );
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
 *  A small wrapper function to draw a line on the window after setting
 *  the given foreground color.
 *
 * \param c
 *      The foreground color in which to draw the line.
 *
 * \param x1
 *      The x coordinate of the starting point.
 *
 * \param y1
 *      The y coordinate of the starting point.
 *
 * \param x2
 *      The x coordinate of the ending point.
 *
 * \param y2
 *      The y coordinate of the ending point.
 */

void
gui_drawingarea_gtk2::draw_line
(
    const Color & c, int x1, int y1, int x2, int y2
)
{
    m_gc->set_foreground(c);
    m_window->draw_line(m_gc, x1, y1, x2, y2);
}

/**
 *  A small wrapper function to draw a line on the pixmap after setting
 *  the given foreground color.
 *
 * \param c
 *      The foreground color in which to draw the line.
 *
 * \param x1
 *      The x coordinate of the starting point.
 *
 * \param y1
 *      The y coordinate of the starting point.
 *
 * \param x2
 *      The x coordinate of the ending point.
 *
 * \param y2
 *      The y coordinate of the ending point.
 */

void
gui_drawingarea_gtk2::draw_line_on_pixmap
(
    const Color & c, int x1, int y1, int x2, int y2
)
{
    m_gc->set_foreground(c);
    m_pixmap->draw_line(m_gc, x1, y1, x2, y2);
}

/**
 *  A small wrapper function to draw a line on the pixmap after setting
 *  the given foreground color.
 *
 * \param pixmap
 *      Provides the Gdk::Drawable pointer needed to draw the line.
 *
 * \param c
 *      The foreground color in which to draw the line.
 *
 * \param x1
 *      The x coordinate of the starting point.
 *
 * \param y1
 *      The y coordinate of the starting point.
 *
 * \param x2
 *      The x coordinate of the ending point.
 *
 * \param y2
 *      The y coordinate of the ending point.
 */

void
gui_drawingarea_gtk2::draw_line
(
    Glib::RefPtr<Gdk::Pixmap> & pixmap,
    const Color & c, int x1, int y1, int x2, int y2
)
{
    m_gc->set_foreground(c);
    pixmap->draw_line(m_gc, x1, y1, x2, y2);
}

/**
 *  A small wrapper function to draw a line on the drawable after setting
 *  the given foreground color.
 *
 * \param drawable
 *      Provides the Gdk::Drawable pointer needed to draw the line.
 *
 * \param c
 *      The foreground color in which to draw the line.
 *
 * \param x1
 *      The x coordinate of the starting point.
 *
 * \param y1
 *      The y coordinate of the starting point.
 *
 * \param x2
 *      The x coordinate of the ending point.
 *
 * \param y2
 *      The y coordinate of the ending point.
 */

void
gui_drawingarea_gtk2::draw_line
(
    Glib::RefPtr<Gdk::Drawable> & drawable,
    const Color & c, int x1, int y1, int x2, int y2
)
{
    m_gc->set_foreground(c);
    drawable->draw_line(m_gc, x1, y1, x2, y2);
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

/**
 *  This function provides optimization for the on_scroll_event() functions,
 *  and should provide support for having the seqedit/seqroll/seqtime/seqdata
 *  panes follow the scrollbar, in a future upgrade (now partly in place).
 *  This function is currently duplicated in the gui_drawingarea_gtk2 and
 *  gui_window_gtk2 modules.
 *
 * \param hadjust
 *      Provides a reference to the adjustment object to be adjusted.
 *      Do we really need this to be a parameter?  Why not just use the
 *      m_hadjust member?  (Note that this member is not present in the
 *      similar gui_window_gtk2 class.)
 *
 * \param step
 *      Provides the step value to use for adjusting the horizontal scrollbar.
 *      If negative, the adjustment is leftward.  If positive, the adjustment
 *      is rightward.  It can be the value of m_hadjust->get_step_increment(),
 *      or provided especially to keep up with the progress bar.
 */

void
gui_drawingarea_gtk2::scroll_hadjust (Gtk::Adjustment & hadjust, double step)
{
    double val = hadjust.get_value();
    double upper = hadjust.get_upper();
    double nextval = val + step;
    bool forward = step >= 0.0;
    if (forward)
    {
        if (nextval > upper - hadjust.get_page_size())
            nextval = upper - hadjust.get_page_size();
    }
    else
    {
        if (nextval < 0.0)
            nextval = 0.0;
    }
    hadjust.set_value(nextval);
}

/**
 *  This function is the vertical version of the scroll_hadjust() function,
 *  intended for adding keystroke vertical scrolling using the Page-Up and
 *  Page-Down keys, as a new feature of Sequencer64.
 *
 * \param vadjust
 *      Provides a reference to the adjustment object to be adjusted.
 *
 * \param step
 *      Provides the step value to use for adjusting the vertical scrollbar.
 *      If negative, the adjustment is upward.  If positive, the adjustment
 *      is downward.  It can be the value of m_vadjust->get_step_increment().
 */

void
gui_drawingarea_gtk2::scroll_vadjust (Gtk::Adjustment & vadjust, double step)
{
    double val = vadjust.get_value();
    double upper = vadjust.get_upper();
    double nextval = val + step;
    bool downward = step >= 0.0;
    if (downward)
    {
        if (nextval >= upper - vadjust.get_page_size())
            nextval = upper - vadjust.get_page_size();
    }
    else
    {
        if (nextval < 0.0)
            nextval = 0.0;
    }
    vadjust.set_value(nextval);
}

/**
 *
 */

void
gui_drawingarea_gtk2::scroll_hset (Gtk::Adjustment & hadjust, double value)
{
    double upper = hadjust.get_upper();
    if (value > upper - hadjust.get_page_size())
        value = upper - hadjust.get_page_size();
    else if (value < 0.0)
        value = 0.0;

    hadjust.set_value(value);
}

/**
 *
 */

void
gui_drawingarea_gtk2::scroll_vset (Gtk::Adjustment & vadjust, double value)
{
    double upper = vadjust.get_upper();
    if (value > upper - vadjust.get_page_size())
        value = upper - vadjust.get_page_size();
    else if (value < 0.0)
        value = 0.0;

    vadjust.set_value(value);
}

}           // namespace seq64

/*
 * gui_drawingarea_gtk2.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
