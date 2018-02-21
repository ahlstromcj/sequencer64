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
 * \file          palette.cpp
 *
 *  This module declares/defines items for an abstract representation of the
 *  color of a sequence or panel item.  Colors are, of course, part of using a
 *  GUI, but here we are not tied to a GUI.
 *
 * \library       sequencer64 application
 * \author        Chris Ahlstrom
 * \date          2018-02-20
 * \updates       2018-02-21
 * \license       GNU GPLv2 or above
 *
 *  This module is inspired by MidiPerformance::getSequenceColor() in
 *  Kepler34.
 */

#include "palette.hpp"                  /* seq64::palette template class    */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{

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
 *      is returned.
 */

template <typename COLOR>
const COLOR &
palette<COLOR>::get_color (thumb_colors_t index) const
{
    return (index >= BLACK && index < NONE) ?
        container[index] : container[NONE] ;
}

}           // namespace seq64

/*
 * palette.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

