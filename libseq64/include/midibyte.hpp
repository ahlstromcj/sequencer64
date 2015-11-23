#ifndef SEQ64_MIDIBYTE_HPP
#define SEQ64_MIDIBYTE_HPP

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
 * \file          midibyte.hpp
 *
 *  This module declares at least one useful typedef.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-11-23
 * \updates       2015-11-23
 * \license       GNU GPLv2 or above
 *
 */

namespace seq64
{

/**
 *  Provides a fairly common type definition for a byte value.
 */

typedef unsigned char midibyte;

/**
 *  Distinguishes a bus number from other MIDI bytes.
 */

typedef unsigned char bussbyte;

}           // namespace seq64

#endif      // SEQ64_MIDIBYTE_HPP

/*
 * midibyte.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

