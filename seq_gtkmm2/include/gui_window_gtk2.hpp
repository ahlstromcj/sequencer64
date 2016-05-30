#ifndef SEQ64_GUI_WINDOW_GTK2_HPP
#define SEQ64_GUI_WINDOW_GTK2_HPP

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
 * \file          gui_window_gtk2.hpp
 *
 *  This module declares/defines the base class for main window of some of the
 *  user-interface classes.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-09-22
 * \updates       2016-05-30
 * \license       GNU GPLv2 or above
 *
 *  This module declares/defines the base class for main window of the
 *  Performance Editor, also known as the Song Editor, the Pattern Editor, and
 *  the main window of the whole application.
 *
 */

/**
 *  Provides an abbreviated way to set up a button image.  Used in mainwnd,
 *  perfedit, and seqedit.
 */

#define PIXBUF_IMAGE(x)     Gtk::Image(Gdk::Pixbuf::create_from_xpm_data(x))

/*
 *  Since these items are pointers, we were able to move (most) of the
 *  included header files to the cpp file.   Except for the items that
 *  come from widget.h, perhaps because GdkEventAny was a typedef.
 *
 *  This base class supports access to the main performance object, the
 *  window size, and the redraw rate.
 */

namespace Gtk
{
    class Adjustment;
    class HScrollbar;
    class VScrollbar;
}

namespace seq64
{
    class perform;

/**
 *  This class supports a basic interface for Gtk::Window-derived objects.
 */

class gui_window_gtk2 : public Gtk::Window
{

private:

    /**
     *  The master object, sort of a sequence buss for all of the sequence.
     *  And a whole lot more than that.
     */

    perform & m_mainperf;

    /**
     *  Window sizes.  Could make this constant, but some windows are
     *  resizable.
     */

    int m_window_x;                     /**< The width of the window.       */
    int m_window_y;                     /**< The height of the window.      */

    /**
     *  Provides the timer period for the eventedit timer, used to determine
     *  the rate of redrawing.  This is currently hardwired to 40 ms in Linux,
     *  and 20 ms in Windows.  Note that mainwnd used 25 ms.
     */

    int m_redraw_period_ms;

    /**
     *  Indicates if on_realize() has been called.  In some cases, we don't
     *  want to draw in objects that haven't yet appeared, otherwise crashes
     *  occur.
     */

    bool m_is_realized;

public:

    gui_window_gtk2
    (
        perform & p,
        int window_x = 0,
        int window_y = 0
    );
    virtual ~gui_window_gtk2 ();

protected:

    /**
     * \getter m_mainperf
     */

    perform & perf ()
    {
        return m_mainperf;
    }

    /**
     *  Provides "quit" functionality that WE HAVE OVERLOOKED!!!  At some
     *  point we need to rectify this situation, probably for the sake of
     *  session support.
     */

    virtual void quit ()
    {
        // TO DO!!!!
    }

    /**
     * \getter m_redraw_period_ms
     */

    int redraw_period_ms () const
    {
        return m_redraw_period_ms;
    }

    /**
     * \getter m_is_realized
     */

    bool is_realized () const
    {
        return m_is_realized;
    }

    void scroll_hadjust (Gtk::Adjustment & hadjust, double step);
    void scroll_vadjust (Gtk::Adjustment & vadjust, double step);

protected:

    void on_realize ();

};              // class gui_window_gtk2

}               // namespace seq64

#endif          // SEQ64_GUI_WINDOW_GTK2_HPP

/*
 * gui_window_gtk2.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

