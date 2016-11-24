#ifndef SEQ64_MASTERMIDIBUS_HPP
#define SEQ64_MASTERMIDIBUS_HPP

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
 * \file          mastermidibus.hpp
 *
 *  This module declares/defines the base class for MIDI I/O for Windows.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2016-11-24
 * \license       GNU GPLv2 or above
 *
 *  This mastermidibus module is the Windows version of the mastermidibus
 *  module.  There's almost enough commonality to be worth creating a base
 *  class for both classes. We moved the mastermidibus class into its own
 *  module, this one.
 */

// #include "midibus_common.hpp"

#include "mastermidibase.hpp"           /* seq64::mastermidibase ABC    */
#include "portmidi.h"                   /* PortMIDI API header file         */

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{

// #ifdef PLATFORM_WINDOWS                // covers this whole module

/**
 *  The class that "supervises" all of the midibus objects?
 */

class mastermidibus : public mastermidibase
{

private:

    /*
     *  All member have been moved into the new base class.
     */

public:

    mastermidibus
    (
        int ppqn = SEQ64_USE_DEFAULT_PPQN,
        int bpm = c_beats_per_minute
    );
    virtual ~mastermidibus ();

    void api_init ();
//  std::string get_midi_out_bus_name (int a_bus);
//  std::string get_midi_in_bus_name (int a_bus);
    int api_poll_for_midi ();
    bool api_is_more_input ();
    bool api_get_midi_event (event *a_in);

};

// #endif      // PLATFORM_WINDOWS

}           // namespace seq64

#endif      // SEQ64_MASTERMIDIBUS_HPP

/*
 * mastermidibus.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
