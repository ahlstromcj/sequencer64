#ifndef SEQ64_MIDIBASE_ABC_HPP
#define SEQ64_MIDIBASE_ABC_HPP

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
 * \file          midibase_abc.hpp
 *
 *  This module declares/defines the base class for MIDI I/O under Linux.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2016-12-18
 * \updates       2016-12-18
 * \license       GNU GPLv2 or above
 *
 *  The midibase_abc module is the new base class for the various implementations
 *  of the midibus module.  There is enough commonality to be worth creating a
 *  base class for all such classes.
 */

#include "app_limits.h"                 /* SEQ64_USE_DEFAULT_PPQN       */
#include "easy_macros.h"                /* for autoconf header files    */
#include "mutex.hpp"
#include "midibus_common.hpp"

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{
/**
 *  This class declares the midibase_abc abstract base class.
 */

class midibase_abc
{

public:

    midibase_abc ()
    {
        // No code
    }

    virtual ~midibase_abc ()
    {
        // No code
    }

protected:

    virtual bool api_init_in () = 0;
    virtual bool api_init_out () = 0;
    virtual void api_continue_from (midipulse tick, midipulse beats) = 0;
    virtual void api_start () = 0;
    virtual void api_stop () = 0;
    virtual void api_clock (midipulse tick) = 0;

};          // class midibase_abc (ALSA version)

}           // namespace seq64

#endif      // SEQ64_MIDIBASE_ABC_HPP

/*
 * midibase_abc.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

