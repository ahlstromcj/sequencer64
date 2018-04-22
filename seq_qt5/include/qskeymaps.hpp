#ifndef SEQ64_QSKEYMAPS_HPP
#define SEQ64_QSKEYMAPS_HPP

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
 * \file          qskeymaps.hpp
 *
 *  The time bar shows markers and numbers for the measures of the song,
 *  and also depicts the left and right markers.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2018-03-24
 * \updates       2018-04-22
 * \license       GNU GPLv2 or above
 *
 */

#include <string>                       /* std::string                      */

/*
 *  Do not document a namespace, it breaks Doxygen.
 */

namespace seq64
{

/**
 *  Free functions in the seq64 namespace.
 */

#if __cplusplus < 201103L                   /* C++11                        */
extern void initialize_key_map ();
#endif

extern unsigned qt_map_to_gdk (unsigned qtkey, unsigned qttext);
extern std::string qt_key_name (unsigned qtkey, unsigned qttext);
extern unsigned gdk_map_to_qt (unsigned gdkkeycode);

}               // namespace seq64

#endif          // SEQ64_QSKEYMAPS_HPP

/*
 * qskeymaps.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

