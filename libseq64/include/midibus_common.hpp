#ifndef SEQ64_MIDIBUS_COMMON_HPP
#define SEQ64_MIDIBUS_COMMON_HPP

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
 * \file          midibus_common.hpp
 *
 *  This module declares/defines the elements that are common to the Linux
 *  and Windows implmentations of midibus.
 *
 * \library       sequencer64 application
 * \author        Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2015-09-10
 * \license       GNU GPLv2 or above
 *
 */

#include <string>

#include "globals.h"
#include "event.hpp"
#include "mutex.hpp"

/*
 * Multiple forward references
 */

class mastermidibus;
class midibus;
class sequence;

/**
 *  Manifest global constants.
 *
 *  These constants were also defined in midibus_portmidi.h, but we made
 *  them common to both implementations here.
 */

const int c_midibus_output_size = 0x100000;     // 1048576
const int c_midibus_input_size =  0x100000;     // 1048576
const int c_midibus_sysex_chunk = 0x100;        //     256

/**
 *  A clock enumeration.  Not sure yet what these mean.
 *
 *  This enumeration Was also defined in midibus_portmidi.h, but we put it
 *  into this common module to avoid duplication.
 *
 * \var e_clock_off
 *
 * \var e_clock_pos
 *
 * \var e_clock_mod
 */

enum clock_e
{
    e_clock_off,
    e_clock_pos,
    e_clock_mod
};

#endif  // SEQ64_MIDIBUS_COMMON_HPP

/*
 * midibus_common.hpp
 *
 * vim: sw=4 ts=4 wm=8 et ft=cpp
 */
