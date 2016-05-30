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
 * \file          gui_window_gtk2.cpp
 *
 *  This module declares/defines the base class for Gtk::Window-derived
 *  objects.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-09-22
 * \updates       2016-05-30
 * \license       GNU GPLv2 or above
 *
 *  This module uses Gtk::Window as the base class, and also holds the main
 *  perform object and some window parameters.
 */

#include <gtkmm/adjustment.h>
#include <gtkmm/window.h>
#include <gtkmm/scrollbar.h>

#include "gui_window_gtk2.hpp"
#include "perform.hpp"
#include "settings.hpp"                 /* seq64::rc() or seq64::usr()      */

namespace seq64
{

/**
 *  Principal constructor, has a reference to the all-important perform
 *  object.
 *
 * \note
 *      We've collected the redraw timeouts into a base-class member.  Most
 *      were valued at c_redraw_ms (40 ms), but mainwnd used 25 ms, so beware.
 *      We will eventually make this a user-interface parameter.
 *
 * \param p
 *      Refers to the main performance object.
 *
 * \param window_x
 *      The width of the window.
 *
 * \param window_y
 *      The height of the window.
 */

gui_window_gtk2::gui_window_gtk2
(
    perform & p,
    int window_x,
    int window_y
) :
    Gtk::Window         (),
    m_mainperf          (p),
    m_window_x          (window_x),
    m_window_y          (window_y),
    m_redraw_period_ms  (usr().window_redraw_rate()),       /* 40, 25 ms    */
    m_is_realized       (false)
{
    add_events(Gdk::KEY_PRESS_MASK | Gdk::KEY_RELEASE_MASK | Gdk::SCROLL_MASK);
    if (window_x > 0 && window_y > 0)
        set_size_request(window_x, window_y);
}

/**
 *  This rote constructor does nothing.
 */

gui_window_gtk2::~gui_window_gtk2 ()
{
    // Empty body
}

/**
 *  This function provides optimization for the on_scroll_event() functions,
 *  and should provide support for having the seqedit/seqroll/seqtime/seqdata
 *  panes follow the scrollbar, in a future upgrade.  This function
 *  is currently duplicated in the gui_drawingarea_gtk2 and gui_window_gtk2
 *  modules.
 *
 * \param hadjust
 *      Provides a reference to the adjustment object to be adjusted.
 *
 * \param step
 *      Provides the step value to use for adjusting the horizontal scrollbar.
 *      If negative, the adjustment is leftward.  If positive, the adjustment
 *      is rightward.  It can be the value of m_hadjust->get_step_increment(),
 *      or provided especially to keep up with the progress bar.
 */

void
gui_window_gtk2::scroll_hadjust (Gtk::Adjustment & hadjust, double step)
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
 *  This function is the vertical version of scroll_hadjust().
 *
 * \param vadjust
 *      Provides a reference to the adjustment object to be adjusted.
 *
 * \param step
 *      Provides the step value to use for adjusting the horizontal scrollbar.
 *      If greater than 0, the movement is downward.  If less than zero, the
 *      movement is upward.
 */

void
gui_window_gtk2::scroll_vadjust (Gtk::Adjustment & vadjust, double step)
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
gui_window_gtk2::scroll_hset (Gtk::Adjustment & hadjust, double value)
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
gui_window_gtk2::scroll_vset (Gtk::Adjustment & vadjust, double value)
{
    double upper = vadjust.get_upper();
    if (value > upper - vadjust.get_page_size())
        value = upper - vadjust.get_page_size();
    else if (value < 0.0)
        value = 0.0;

    vadjust.set_value(value);
}

/**
 *  This callback function calls the base-class on_realize() function, and
 *  sets the m_is_realized flag.
 */

void
gui_window_gtk2::on_realize ()
{
    Gtk::Window::on_realize();
    m_is_realized = true;
}

}           // namespace seq64

/*
 * gui_window_gtk2.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

