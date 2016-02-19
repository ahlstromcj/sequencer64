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
 * \file          gui_drawingarea_gtk2.hpp
 *
 *  This module declares/defines the base class for drawing on the piano
 *  roll of the patterns editor.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-09-21
 * \updates       2016-02-19
 * \license       GNU GPLv2 or above
 *
 *  We've added a number of wrapper functions for the "draw-rectangle",
 *  "draw-line", and "render-string" functions specific to Gtkmm-2.x so
 *  that we can hide access to the seq64::gui_drawingarea_gtk2::m_pixmap
 *  and seq64::gui_drawingarea_gtk2::m_window members.
 *
 *  Unfortunately, there are still bits of the code that access the Gtk/Gdk
 *  Drawable and Pixmap constructions, so we added overloads for those.  And
 *  we need both kinds of overloads, since there's something incompatible
 *  that prevents Drawable being used for a Pixmap parameter. (Perhaps there's
 *  a common base class for them that we can use instead?)  So, we have a lot
 *  of similar functions defined here, which might annoy some people.
 *
 *  And there are still a number of other Gtk/Gdk functions we could wrap.
 */

#include "font.hpp"                     /* font_render() function           */
#include "gui_palette_gtk2.hpp"         /* #include <gtkmm/drawingarea.h>   */

namespace Gtk
{
    class Adjustment;
}

namespace seq64
{

class perform;                          /* forward reference                */

extern Gtk::Adjustment & adjustment_dummy ();

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

protected:              // private: should provide accessors

    /**
     *  The graphics context, which is required for ever drawing and rendering
     *  operation.
     */

    Glib::RefPtr<Gdk::GC> m_gc;

    /**
     *  Provides the default "window".  Wrapper functions with undecorated
     *  wrapper names are used for accessing this item.  We hope to be able to
     *  hide this items completely some day.
     */

    Glib::RefPtr<Gdk::Window> m_window;

    /**
     *  Provides an object for vertical "adjustments".
     */

    Gtk::Adjustment & m_vadjust;

    /**
     *  Provides an object for horizontal "adjustments".
     */

    Gtk::Adjustment & m_hadjust;

    /**
     *  Provides the default "pixmap".  Wrapper functions with undecorated
     *  wrapper names are used for accessing this item.  We hope to be able to
     *  hide this items completely some day.
     */

    Glib::RefPtr<Gdk::Pixmap> m_pixmap;

    /**
     *  Another pixmap, used for backgrounds.  Our wrappers still leave this
     *  member exposed <giggle>.
     */

    Glib::RefPtr<Gdk::Pixmap> m_background;

    /**
     *  Another pixmap, used for foregrounds.  Our wrappers still leave this
     *  member exposed.
     */

    Glib::RefPtr<Gdk::Pixmap> m_foreground;

    /**
     *  A frequent hook into the main perform object.  We could move this
     *  into yet another base class, since a number of classes don't need it.
     *  Probably not worth the effort at this time.
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

    /**
     *  Clears the main window.  One less need to access m_window directly.
     */

    void clear_window ()
    {
        m_window->clear();
    }

    /**
     *  A small wrapper function for readability in line-drawing.  Sets the
     *  attributes of a line to be drawn.
     *
     * \param ls
     *      Provides the Gtk-specific line style.
     *
     * \param width
     *      Provides the width of the line to be drawn.  It defaults to the
     *      most common value, 1.
     */

    void set_line (Gdk::LineStyle ls, int width = 1)
    {
        m_gc->set_line_attributes(width, ls, Gdk::CAP_NOT_LAST, Gdk::JOIN_MITER);
    }

    /**
     *  A small wrapper function to draw a line on the window.
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

    void draw_line (int x1, int y1, int x2, int y2)
    {
        m_window->draw_line(m_gc, x1, y1, x2, y2);
    }

    void draw_line (const Color & c, int x1, int y1, int x2, int y2);

    /**
     *  A small wrapper function to draw a line on the pixmap.
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

    void draw_line_on_pixmap (int x1, int y1, int x2, int y2)
    {
        m_pixmap->draw_line(m_gc, x1, y1, x2, y2);
    }

    void draw_line_on_pixmap (const Color & c, int x1, int y1, int x2, int y2);

    /**
     *  A small wrapper function to draw a line on any pixmap (not a drawable,
     *  though, due to a compiler error after setting the given foreground
     *  color.
     *
     * \param pixmap
     *      Provides the Gdk::Pixmap pointer needed to draw the line.
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

    void draw_line
    (
        Glib::RefPtr<Gdk::Pixmap> & pixmap,
        int x1, int y1, int x2, int y2
    )
    {
        pixmap->draw_line(m_gc, x1, y1, x2, y2);
    }

    void draw_line
    (
        Glib::RefPtr<Gdk::Pixmap> & pixmap,
        const Color & c, int x1, int y1, int x2, int y2
    );

    /**
     *  A small wrapper function to draw a line on any pixmap (not a drawable,
     *  though, due to a compiler error after setting the given foreground
     *  color.
     *
     * \param drawable
     *      Provides the Gdk::Drawable pointer needed to draw the line.
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

    void draw_line
    (
        Glib::RefPtr<Gdk::Drawable> & drawable,
        int x1, int y1, int x2, int y2
    )
    {
        drawable->draw_line(m_gc, x1, y1, x2, y2);
    }

    void draw_line
    (
        Glib::RefPtr<Gdk::Drawable> & drawable,
        const Color & c, int x1, int y1, int x2, int y2
    );

    /**
     *  A small wrapper function for readability in string-drawing to the
     *  window.
     *
     * \param x
     *      The x-coordinate of the origin.
     *
     * \param y
     *      The y-coordinate of the origin.
     *
     * \param s
     *      The string to be drawn.
     *
     * \param color
     *      The color with which to draw the string.
     */

    void render_string
    (
        int x, int y, const std::string & s, font::Color color
    )
    {
        font_render().render_string_on_drawable
        (
            m_gc, x, y, m_window, s.c_str(), color
        );
    }

    /**
     *  A small wrapper function for readability in string-drawing to the
     *  pixmap.
     *
     * \param x
     *      The x-coordinate of the origin.
     *
     * \param y
     *      The y-coordinate of the origin.
     *
     * \param s
     *      The string to be drawn.
     *
     * \param color
     *      The color with which to draw the string.
     */

    void render_string_on_pixmap
    (
        int x, int y, const std::string & s, font::Color color
    )
    {
        font_render().render_string_on_drawable
        (
            m_gc, x, y, m_pixmap, s.c_str(), color
        );
    }

    /**
     *  A small wrapper function for readability in box-drawing on the window.
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

    void draw_rectangle (int x, int y, int lx, int ly, bool fill = true)
    {
        m_window->draw_rectangle(m_gc, fill, x, y, lx, ly);
    }

    void draw_rectangle
    (
        const Color & c, int x, int y, int lx, int ly, bool fill = true
    );

    /**
     *  A small wrapper function for readability in box-drawing on a
     *  "drawable" context, where the foreground color has already been
     *  specified.
     *
     * \param drawable
     *      The object on which to draw the rectangle.
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

    void draw_rectangle
    (
        Glib::RefPtr<Gdk::Drawable> & drawable,
        int x, int y, int lx, int ly, bool fill = true
    )
    {
        drawable->draw_rectangle(m_gc, fill, x, y, lx, ly);
    }

    void draw_rectangle
    (
        Glib::RefPtr<Gdk::Drawable> & drawable,
        const Color & c,
        int x, int y, int lx, int ly, bool fill = true
    );

    /**
     *  A small wrapper function for readability in box-drawing on a
     *  "pixmap" context, where the foreground color has already been
     *  specified.
     *
     * \param drawable
     *      The object on which to draw the rectangle.
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

    void draw_rectangle
    (
        Glib::RefPtr<Gdk::Pixmap> & pixmap,
        int x, int y, int lx, int ly, bool fill = true
    )
    {
        pixmap->draw_rectangle(m_gc, fill, x, y, lx, ly);
    }

    void draw_rectangle
    (
        Glib::RefPtr<Gdk::Pixmap> & pixmap,
        const Color & c,
        int x, int y, int lx, int ly, bool fill = true
    );

    /**
     *  A small wrapper function for readability in box-drawing on the pixmap.
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

    void draw_rectangle_on_pixmap
    (
        int x, int y, int lx, int ly, bool fill = true
    )
    {
        m_pixmap->draw_rectangle(m_gc, fill, x, y, lx, ly);
    }

    void draw_rectangle_on_pixmap
    (
        const Color & c, int x, int y, int lx, int ly, bool fill = true
    );

    void draw_normal_rectangle_on_pixmap
    (
        int x, int y, int lx, int ly, bool fill = true
    );

    /**
     *  Provides the most common use case for redrawing.
     */

    void draw_drawable
    (
        int xsrc, int ysrc,
        int xdest, int ydest,
        int width, int height
    )
    {
        m_window->draw_drawable
        (
            m_gc, m_pixmap, xsrc, ysrc, xdest, ydest, width, height
        );
    }

    void scroll_adjust (Gtk::Adjustment & adjust, double step);

protected:            // special dual setters for friend GUI classes

    void set_current_drop_x (int x)
    {
        m_current_x = m_drop_x = x;
    }

    void set_current_drop_y (int y)
    {
        m_current_y = m_drop_y = y;
    }

private:

    void gtk_drawarea_init ();

protected:          // callbacks

    void on_realize ();

private:            // callbacks

#if 0
    bool on_button_press_event (GdkEventButton *);
    bool on_button_release_event (GdkEventButton *);
    bool on_key_press_event (GdkEventKey *);
    bool on_key_release_event (GdkEventKey *);
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

