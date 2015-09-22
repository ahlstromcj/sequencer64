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
 * \updates       2015-09-21
 * \license       GNU GPLv2 or above
 *
 */

#if 0
#include <gtkmm/accelkey.h>
#include <gtkmm/adjustment.h>
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
 *  Principal constructor.
 */

gui_drawingarea_gtk2::gui_drawingarea_gtk2
(
    perform & a_perf,
    Gtk::Adjustment & a_hadjust,
    Gtk::Adjustment & a_vadjust
) :
    Gtk::DrawingArea        (),
    gui_base                (),
    m_gc                    (),
    m_window                (),
    m_black                 (Gdk::Color("black")),
    m_white                 (Gdk::Color("white")),
    m_grey                  (Gdk::Color("gray")),
    m_dk_grey               (Gdk::Color("gray50")),
    m_orange                (Gdk::Color("orange")),

    m_vadjust               (a_vadjust),
    m_hadjust               (a_hadjust),

    m_pixmap                (),
    m_background            (),         // another pixmap

    m_mainperf              (a_perf)    // ,

#if 0
    m_window_x              (10),       // why so small?
    m_window_y              (10),
    m_current_x             (0),
    m_current_y             (0),
    m_drop_x                (0),
    m_drop_y                (0),
    m_old                   (),
    m_pos                   (a_pos),
    m_zoom                  (a_zoom),   //
    m_snap                  (a_snap),   //
#endif

#if 0
    m_selected              (),
    m_selecting             (false),
    m_moving                (false),
    m_moving_init           (false),
    m_growing               (false),
    m_painting              (false),
    m_paste                 (false),
    m_is_drag_pasting       (false),
    m_is_drag_pasting_start (false),
#endif

#if 0
    m_move_delta_x          (0),
    m_move_delta_y          (0),
    m_move_snap_offset_x    (0),
    m_old_progress_x        (0),
    m_scroll_offset_ticks   (0),
    m_scroll_offset_key     (0),
    m_scroll_offset_x       (0),
    m_scroll_offset_y       (0),
    m_ignore_redraw         (false)
#endif
{
    Glib::RefPtr<Gdk::Colormap> colormap = get_default_colormap();
    colormap->alloc_color(m_black);
    colormap->alloc_color(m_white);
    colormap->alloc_color(m_grey);
    colormap->alloc_color(m_dk_grey);
    colormap->alloc_color(m_orange);
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

/**
 *  Provides a destructor to delete allocated objects.
 */

gui_drawingarea_gtk2::~gui_drawingarea_gtk2 ()
{
    // 
}

}           // namespace seq64

/*
 * gui_drawingarea_gtk2.cpp
 *
 * vim: sw=4 ts=4 wm=8 et ft=cpp
 */
