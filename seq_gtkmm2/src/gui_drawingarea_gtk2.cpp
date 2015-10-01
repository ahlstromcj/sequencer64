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
 * \updates       2015-09-29
 * \license       GNU GPLv2 or above
 *
 */

#include <gtkmm/adjustment.h>

#if 0
#include <gtkmm/accelkey.h>
#include <gtkmm/button.h>
#include <gtkmm/window.h>
#include <gtkmm/accelgroup.h>
#include <gtkmm/box.h>
#include <gtkmm/main.h>
#include <gtkmm/menu.h>
#include <gtkmm/menubar.h>
#include <gtkmm/eventbox.h>
#include <gtkmm/window.h>
#include <gtkmm/table.h>
#include <gtkmm/drawingarea.h>
#include <gtkmm/widget.h>
#include <gtkmm/scrollbar.h>
#include <gtkmm/viewport.h>
#include <gtkmm/combo.h>
#include <gtkmm/label.h>
#include <gtkmm/toolbar.h>
#include <gtkmm/optionmenu.h>
#include <gtkmm/togglebutton.h>
#include <gtkmm/invisible.h>
#include <gtkmm/separator.h>
#include <gtkmm/tooltips.h>             // #include <gtkmm/tooltip.h>
#include <gtkmm/invisible.h>
#include <gtkmm/arrow.h>
#include <gtkmm/image.h>
#include <sigc++/bind.h>
#endif  // 0

#include "gui_drawingarea_gtk2.hpp"
#include "perform.hpp"

namespace seq64
{

/**
 *  These two objects are used so we have some Adjustments to assign to
 *  the Adjustment members for classes that don't use them.  Clumsy?  We
 *  shall see.
 *
 *  Anyway, the parameters for this constructor are value, lower, upper,
 *  step-increment, and two more values.
 */

Gtk::Adjustment gui_drawingarea_gtk2::m_hadjust_dummy
(
    0.0, 0.0, 1.0, 1.0, 1.0, 1.0
);

Gtk::Adjustment gui_drawingarea_gtk2::m_vadjust_dummy
(
    0.0, 0.0, 1.0, 1.0, 1.0, 1.0
);

/**
 *  Perform-only constructor.
 */

gui_drawingarea_gtk2::gui_drawingarea_gtk2
(
    perform & perf,
    int window_x,
    int window_y
) :
    gui_palette_gtk2        (),
    m_gc                    (),
    m_window                (),
    m_vadjust               (m_vadjust_dummy),
    m_hadjust               (m_hadjust_dummy),
    m_pixmap                (),
    m_background            (),             // another pixmap
    m_foreground            (),             // another pixmap
    m_mainperf              (perf),
    m_window_x              (window_x),
    m_window_y              (window_y),
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
    int window_x,
    int window_y
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
    m_window_x              (window_x),
    m_window_y              (window_y),
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
    if (m_window_x > 0 && m_window_y > 0)
        set_size_request(m_window_x, m_window_y);

    add_events
    (
        Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK |
        Gdk::POINTER_MOTION_MASK | Gdk::KEY_PRESS_MASK |
        Gdk::KEY_RELEASE_MASK | Gdk::FOCUS_CHANGE_MASK |
        Gdk::ENTER_NOTIFY_MASK | Gdk::LEAVE_NOTIFY_MASK |
        Gdk::SCROLL_MASK
    );
    set_double_buffered(false);
}

}           // namespace seq64

/*
 * gui_drawingarea_gtk2.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
