#ifndef SEQ64_PALETTE_HPP
#define SEQ64_PALETTE_HPP

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
 * \file          palette.hpp
 *
 *  This module declares/defines items for an abstract representation of the
 *  color of a sequence or panel item.  Colors are, of course, part of using a
 *  GUI, but here we are not tied to a GUI.
 *
 * \library       sequencer64 application
 * \author        Chris Ahlstrom
 * \date          2018-02-18
 * \updates       2018-02-22
 * \license       GNU GPLv2 or above
 *
 *  This module is inspired by MidiPerformance::getSequenceColor() in
 *  Kepler34.
 */

#include <map>                          /* std::map container class         */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *  A new type to support the concept of sequence color.  This feature cannot
 *  be used in the current versions of Sequencer64, because their color is
 *  determined by the font bitmap.  The color will be a number pointing to an
 *  RGB entry in a palette.  A future feature, we're making room for it here.
 *
 *  This enumeration provide as stock palette of colors.  For example,
 *  Kepler34 creates a color-map in this manner:
 *
 *      extern QMap<thumb_colours_e, QColor> colourMap;
 *
 *  Of course, we would use std::map instead of QMap, and include a wrapper
 *  class to keep QColor unexposed to non-Qt entities.  Also, we define the
 *  colors in standard X-terminal order, not in Kepler34 order.
 */

enum thumb_colors_t
{

 /* Seq64 */            /* Kepler34 */

    BLACK,              //  0 WHITE
    RED,                //  1 RED
    GREEN,              //  2 GREEN
    YELLOW,             //  3 BLUE
    BLUE,               //  4 YELLOW
    MAGENTA,            //  5 PURPLE
    CYAN,               //  6 PINK
    WHITE,              //  7 ORANGE

    DK_BLACK,           //  8 place-holder
    DK_RED,             //  9 N/A
    DK_GREEN,           // 10 N/A
    DK_YELLOW,          // 11 N/A
    DK_BLUE,            // 12 N/A
    DK_MAGENTA,         // 13 N/A
    DK_CYAN,            // 14 N/A
    DK_WHITE,           // 15 N/A

    ORANGE,             // N/A
    PINK,               // N/A
    GREY,               // N/A

    DK_ORANGE,          // N/A
    DK_PINK,            // N/A
    DK_GREY,            // N/A

    NONE                // N/A
};

/**
 *  A generic collection of whatever types of color classes (QColor,
 *  Gdk::Color) one wants to hold, and reference by an index number.
 *  This template class is not meant to manage color, but just to point
 *  to them.
 */

template <typename COLOR>
class palette
{

private:

    /*
     *  Provides a type definition for the color-class of interest.
     *
     * typedef COLOR Color;
     */

    /**
     *  Provides an associative container of pointers to the color-class COLOR.
     *  A vector could be used instead of a map.
     */

    std::map<thumb_colors_t, const COLOR *> container;

public:

    palette ();                         /* initially empty, filled by add() */

    void add (thumb_colors_t index, const COLOR & color);
    const COLOR & get_color (thumb_colors_t index) const;

    /**
     *
     */

    void clear ()
    {
        container.clear();
    }

};          // class palette

/**
 *  Creates the palette, and inserts a default COLOR color object as
 *  the NONE entry.
 */

template <typename COLOR>
palette<COLOR>::palette ()
 :
    container   ()
{
    COLOR color;
    add(NONE, color);
}

/**
 *  Inserts a color-index/color pair into the palette.  There is no indication
 *  if the item was not added, which will occur only when the item is already in
 *  the container.
 *
 * \param index
 *      The index into the palette.
 *
 * \param color
 *      The COLOR color object to add to the palette.
 */

template <typename COLOR>
void
palette<COLOR>::add (thumb_colors_t index, const COLOR & color)
{
    std::pair<thumb_colors_t, const COLOR *> p = std::make_pair(index, &color);
    (void) container.insert(p);
}

/**
 *  Gets a color from the palette, based on the index value.
 *
 * \param index
 *      Indicates which color to get.  This index is checked for range, and, if
 *      out of range, the default color object, indexed by thumb_colors_t::NONE,
 *      is returned.  However, an exception will be thrown if the color does
 *      not exist.
 */

template <typename COLOR>
const COLOR &
palette<COLOR>::get_color (thumb_colors_t index) const
{
    return (index >= BLACK && index < NONE) ?
        *container.at(index) : *container.at(NONE) ;
}

}           // namespace seq64

#endif      // SEQ64_PALETTE_HPP

/*
 * palette.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

