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
 * \updates       2018-02-20
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
 *
 */

template <class T>
palette<T>::palette ()
 :
    container   ()
{
}

/**
 *
 */

template <class T>
void
palette<T>::add (thumb_colors_t index, T * color)
{
}

template <class T>
T &
palette<T>::get_color (thumb_colors_t index)
{
}

}           // namespace seq64

/*
 * palette.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

