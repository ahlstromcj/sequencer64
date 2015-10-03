#ifndef SEQ64_GUI_DRAWINGAREA_GTK2_HPP
#define SEQ64_GUI_DRAWINGAREA_GTK2_HPP

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
 * \file          gui_drawingarea.hpp
 *
 *  This module declares/defines the base class for drawing on the piano
 *  roll of the patterns editor.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-09-21
 * \updates       2015-10-02
 * \license       GNU GPLv2 or above
 *
 */

#include "gui_palette_gtk2.hpp"         // #include <gtkmm/drawingarea.h>

namespace Gtk
{
    class Adjustment;
}

namespace seq64
{

class perform;                          // forward reference

/**
 *  Implements the basic drawing areas of the application.  Note that this
 *  class really "isn't a" gui_pallete_gtk2; it should simply have one.
 *  But that base class must be derived from Gtk::DrawingArea.  We don't
 *  want to waste some space by using a "has-a" relationship, and also put
 *  up with having to access the palette indirectly.  So, in this case, we
 *  tolerate the less strict implementation.
 */

class gui_drawingarea_gtk2 : public gui_palette_gtk2
{

public:

    /**
     *  A small helper structure representing a rectangle.
     */

    struct rect
    {
        public: int x, y, height, width;
    };

protected:

    static Gtk::Adjustment sm_hadjust_dummy;
    static Gtk::Adjustment sm_vadjust_dummy;

protected:              // private: should provide accessors

    Glib::RefPtr<Gdk::GC> m_gc;
    Glib::RefPtr<Gdk::Window> m_window;
    Gtk::Adjustment & m_vadjust;
    Gtk::Adjustment & m_hadjust;
    Glib::RefPtr<Gdk::Pixmap> m_pixmap;
    Glib::RefPtr<Gdk::Pixmap> m_background;
    Glib::RefPtr<Gdk::Pixmap> m_foreground;

    /**
     *  A frequent hook into the main perform object.  We could move this
     *  into yet another base class.  Probably not worth the effort.
     */

    perform & m_mainperf;

    /**
     *  Window sizes.  Could make this constant, but some windows are
     *  resizable.
     */

    int m_window_x;
    int m_window_y;

    /**
     *  The x and y value of the current location of the mouse (during
     *  dragging?)
     */

    int m_current_x;
    int m_current_y;

    /**
     *  These values are used when roping and highlighting a bunch of events.
     *  Provides the x and y value of where the dragging started.
     */

    int m_drop_x;
    int m_drop_y;

private:

    gui_drawingarea_gtk2 (const gui_drawingarea_gtk2 &);
    gui_drawingarea_gtk2 & operator = (const gui_drawingarea_gtk2 &);

public:

    gui_drawingarea_gtk2
    (
        perform & p,
        int window_x = 0,
        int window_y = 0
    );
    gui_drawingarea_gtk2
    (
        perform & a_perf,
        Gtk::Adjustment & a_hadjust,
        Gtk::Adjustment & a_vadjust,
        int window_x = 0,
        int window_y = 0
    );
    ~gui_drawingarea_gtk2 ();

    /**
     * \getter m_window_x
     */

    int window_x () const
    {
        return m_window_x;
    }

    /**
     * \getter m_window_y
     */

    int window_y () const
    {
        return m_window_y;
    }

    /**
     * \getter m_current_x
     */

    int current_x () const
    {
        return m_current_x;
    }

    /**
     * \getter m_current_y
     */

    int current_y () const
    {
        return m_current_y;
    }

    /**
     * \getter m_drop_x
     */

    int drop_x () const
    {
        return m_drop_x;
    }

    /**
     * \getter m_drop_y
     */

    int drop_y () const
    {
        return m_drop_y;
    }

protected:

    /**
     * \getter m_mainperf
     */

    perform & perf ()
    {
        return m_mainperf;
    }

private:

    void gtk_drawarea_init ();

protected:          // callbacks

    void on_realize ();

private:            // callbacks

#if 0
    bool on_button_press_event (GdkEventButton * a_ev);
    bool on_button_release_event (GdkEventButton * a_ev);
    bool on_key_press_event (GdkEventKey * a_p0);
    bool on_key_release_event (GdkEventKey * a_p0);
    void on_size_allocate (Gtk::Allocation &);
#endif  // 0

};

}           // namespace seq64

#endif      // SEQ64_GUI_DRAWINGAREA_GTK2_HPP

/*
 * gui_drawingarea.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
