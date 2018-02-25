#ifndef SEQ64_GUI_PALETTE_GTK2_HPP
#define SEQ64_GUI_PALETTE_GTK2_HPP

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
 * \file          gui_palette_gtk2.hpp
 *
 *  This module declares/defines the class for providing GTK/GDK colors.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-09-21
 * \updates       2018-02-21
 * \license       GNU GPLv2 or above
 *
 *  This module defines some Gdk::Color objects.  However, note that this
 *  object is <i> deprecated </i> in favor of Gdk::RGBA, defined by the
 *  gdkmm/rgba.h header file.
 *
 *  Anyway, we still need this stuff.  We might consider replacing the color
 *  accessor names with names that reflect their usage [e.g. instead of
 *  using light_grey(), we could provide a scale_color() function instead,
 *  since light-grey is the color used to draw scales on the pattern editor.
 */

#include <gtkmm/drawingarea.h>          /* or #include <gtkmm/widget.h> */

#include "palette.hpp"                  /* seq64::palette               */

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *  Implements a stock palette of Gdk::Color elements.  Note that this
 *  class must be derived from Gtk::DrawingArea (or Gtk::Widget) in order
 *  to get access to the get_default_colormap() function used in the
 *  constructor.
 */

class gui_palette_gtk2 : public Gtk::DrawingArea    // or Gtk::Widget
{

protected:

    /**
     *  Provides a type for the color object.  The following uses are made of
     *  each color:
     *
     *  -   Black.  The background color of armed patterns.  The color of
     *      most lines in the user interface, including the main grid
     *      lines.  The default color of progress lines and text.
     *  -   White.  The default background color of just about everything
     *      drawn in the application.
     *  -   Grey.  The color of minor grid lines and the markers for the
     *      currently-selected scale.
     *  -   Dark grey.  The color of some grid lines, and the background
     *      of a queued pattern slot.
     *  -   Light grey.  The color of some grid lines.
     *  -   Red.  The optional color of progress bars.
     *  -   Orange.  The fill-in color for selected notes and events.
     *  -   Dark orange.  The color of selected event data lines and the
     *      color of the selection box for events to be pasted.
     *  -   Yellow.  The background of the pattern and name slots for empty
     *      patterns.  The text color for selected empty pattern slots.
     *  -   Green.  Not yet used.
     *  -   Blue.   Not yet used.
     *  -   Dark cyan.  The background color of muted patterns currently in
     *      edit, or the pattern that contains the original data for an
     *      imported SMF 0 song.  The text color of an unmuted pattern
     *      currently in edit.  These colors apply to the pattern editor and
     *      the song editor.  The color of the selected background pattern
     *      in the song editor.
     *  -   Line color. The generic line color, meant for expansion.
     *      Currently black.
     *  -   Progress color. The progress line color.  Black by default, but
     *      can be set to red.
     *  -   Background color.  The currently-in-use background color.  Can
     *      vary a lot when a pixmap is being redrawn.
     *  -   Foreground color.  The currently-in-use foreground color.  Can
     *      vary a lot when a pixmap is being redrawn.
     */

    typedef Gdk::Color Color;

protected:

    palette<Color> m_palette;

private:                            /* use the accessor functions           */

    /**
     *  Flags the presense of the inverse color palette.
     */

    static bool m_is_inverse;

    /*
     *  Colors that will remain constant, providing a brighter palette.
     */

    static const Color m_black;     /**< Provides the black color.          */
    static const Color m_red;       /**< Provides the red color.            */
    static const Color m_green;     /**< Provides the green color.          */
    static const Color m_yellow;    /**< Provides the yellow color.         */
    static const Color m_blue;      /**< Provides the blue color.           */
    static const Color m_magenta;   /**< Provides the magenta color.        */
    static const Color m_cyan;      /**< Provides the cyan color.          */
    static const Color m_white;     /**< Provides the white color.          */

    /*
     *  Colors that will remain constant.  We now provide a darker palette.
     *  Doesn't include dark-grey, which is an "invertible" color.
     */

    static const Color m_dk_black;  /**< Provides a blood-red color.        */
    static const Color m_dk_red;    /**< Provides a blood-red color.        */
    static const Color m_dk_green;  /**< Provides a dark green color.       */
    static const Color m_dk_yellow; /**< Provides a dark green color.       */
    static const Color m_dk_blue;   /**< Provides the dark blue color.      */
    static const Color m_dk_magenta; /**< Provides a dark magenta color.    */
    static const Color m_dk_cyan;   /**< Provides the dark cyan color.      */
    static const Color m_dk_white;  /**< Provides a greyish color.          */

    /*
     * Extended colors in the palette.  The greys are defined separately and
     * are invertible.
     */

    static const Color m_orange;    /**< Provides the orange color.         */
    static const Color m_pink;      /**< Provides the pink color.           */
    static const Color m_grey;      /**< Provides the unvarying grey color. */

    static const Color m_dk_orange; /**< Provides a dark orange color.      */
    static const Color m_dk_pink;   /**< Provides a dark pink color.        */
    static const Color m_dk_grey;   /**< The unvarying dark grey color.     */

    /*
     * Colors that can be "inverted" (i.e. changed for the inverse-color
     * mode).
     */

    static Color m_grey_paint;      /**< Provides the grey color.           */
    static Color m_dk_grey_paint;   /**< Provides the dark grey color.      */
    static Color m_lt_grey_paint;   /**< Provides the light grey color.     */
    static Color m_blk_paint;       /**< An invertible black color.         */
    static Color m_wht_paint;       /**< An invertible white color.         */
    static Color m_blk_key_paint;   /**< Provides the color of a black key. */
    static Color m_wht_key_paint;   /**< Provides the color of a white key. */
    static Color m_tempo_paint;     /**< The color of a tempo line.         */
    static Color m_sel_paint;       /**< The color of a selection box.      */

    /*
     * Non-static member colors.
     */

    Color m_line_color;             /**< Provides the line color.           */
    Color m_progress_color;         /**< Provides the progress bar color.   */
    Color m_bg_color;               /**< The current background color.      */
    Color m_fg_color;               /**< The current foreground color.      */

public:

    gui_palette_gtk2 ();
    ~gui_palette_gtk2 ();

    void initialize ();
    const Color & get_color (PaletteColor index) const
    {
        return m_palette.get_color(index);
    }

    static void load_inverse_palette (bool inverse = true);

    /**
     *  Indicates if the inverse color palette is loaded.
     */

    static bool is_inverse ()
    {
        return m_is_inverse;
    }

    /**
     * \getter m_line_color
     *      Provides an experimental way to change some line colors from black
     *      to something else.  Might eventually be selectable from the "user"
     *      configuration file
     */

    const Color & line_color () const
    {
        return m_line_color;
    }

    /**
     * \getter m_progress_color
     *      Provides an experimental way to change the progress line color
     *      from black to something else.  Now selectable from the "user"
     *      configuration file.
     */

    const Color & progress_color () const
    {
        return m_progress_color;
    }

    /**
     * \getter m_black
     *      Although these color getters return static values (if so
     *      compiled), these colors are used only in the window and
     *      drawing-area classes, so no need to make these functions static.
     */

    const Color & black () const
    {
        return m_black;
    }

    /**
     * \getter m_dk_red
     */

    const Color & dark_red () const
    {
        return m_dk_red;
    }

    /**
     * \getter m_dk_green
     */

    const Color & dark_green () const
    {
        return m_dk_green;
    }

    /**
     * \getter m_dk_orange
     */

    const Color & dark_orange () const
    {
        return m_dk_orange;
    }

    /**
     * \getter m_dk_blue
     */

    const Color & dark_blue () const
    {
        return m_dk_blue;
    }

    /**
     * \getter m_dk_magenta
     */

    const Color & dark_magenta () const
    {
        return m_dk_magenta;
    }

    /**
     * \getter m_dk_cyan
     */

    const Color & dark_cyan () const
    {
        return m_dk_cyan;
    }

    /**
     * \getter m_white
     */

    const Color & white () const
    {
        return m_white;
    }

    /**
     * \getter m_grey_paint
     */

    const Color & grey_paint () const
    {
        return m_grey_paint;
    }

    /**
     * \getter m_dk_grey_paint
     */

    const Color & dark_grey_paint () const
    {
        return m_dk_grey_paint;
    }

    /**
     * \getter m_lt_grey_paint
     */

    const Color & light_grey_paint () const
    {
        return m_lt_grey_paint;
    }

    /**
     * \getter m_red
     */

    const Color & red () const
    {
        return m_red;
    }

    /**
     * \getter m_orange
     */

    const Color & orange () const
    {
        return m_orange;
    }

    /**
     * \getter m_yellow
     */

    const Color & yellow () const
    {
        return m_yellow;
    }

    /**
     * \getter m_green
     */

    const Color & green () const
    {
        return m_green;
    }

    /**
     * \getter m_magenta
     */

    const Color & magenta () const
    {
        return m_magenta;
    }

    /**
     * \getter m_blue
     */

    const Color & blue () const
    {
        return m_blue;
    }

    /**
     * \getter m_blk_paint
     */

    const Color & black_paint () const
    {
        return m_blk_paint;
    }

    /**
     * \getter m_wht_paint
     */

    const Color & white_paint () const
    {
        return m_wht_paint;
    }

    /**
     * \getter m_blk_key_paint
     */

    const Color & black_key_paint () const
    {
        return m_blk_key_paint;
    }

    /**
     * \getter m_wht_key_paint
     */

    const Color & white_key_paint () const
    {
        return m_wht_key_paint;
    }

    /**
     * \getter m_tempo_paint
     */

    const Color & tempo_paint () const
    {
        return m_tempo_paint;
    }

    /**
     * \getter m_sel_paint
     */

    const Color & sel_paint () const
    {
        return m_sel_paint;
    }

    /**
     * \getter m_bg_color
     */

    const Color & bg_color () const
    {
        return m_bg_color;
    }

    /**
     * \setter m_bg_color
     */

    void bg_color (const Color & c)
    {
        m_bg_color = c;
    }

    /**
     * \getter m_fg_color
     */

    const Color & fg_color () const
    {
        return m_fg_color;
    }

    /**
     * \setter m_fg_color
     */

    void fg_color (const Color & c)
    {
        m_fg_color = c;
    }

};          // gui_palette_gtk2

}           // namespace seq64

#endif      // SEQ64_GUI_PALETTE_GTK2_HPP

/*
 * gui_palette_gtk2.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

