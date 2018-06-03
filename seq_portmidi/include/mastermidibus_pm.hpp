#ifndef SEQ64_MASTERMIDIBUS_PM_HPP
#define SEQ64_MASTERMIDIBUS_PM_HPP

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
 * \file          mastermidibus_pm.hpp
 *
 *  This module declares/defines the base class for MIDI I/O for Windows.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2017-05-27
 * \license       GNU GPLv2 or above
 *
 *  This mastermidibus module is the Windows (and Linux now!) version of the
 *  mastermidibus module using the PortMidi library.
 */

#include "mastermidibase.hpp"           /* seq64::mastermidibase ABC        */
#include "portmidi.h"                   /* PortMIDI API header file         */

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *  The class that "supervises" all of the midibus objects.  This
 *  implementation uses the PortMidi library, which supports Linux and
 *  Windows, but not JACK or Mac OSX.
 */

class mastermidibus : public mastermidibase
{

private:

    /*
     *  All members have been moved into the new base class.
     */

public:

    mastermidibus
    (
        int ppqn    = SEQ64_USE_DEFAULT_PPQN,
        midibpm bpm = SEQ64_DEFAULT_BPM        /* c_beats_per_minute */
    );
    virtual ~mastermidibus ();
    virtual bool activate ();

protected:

    virtual void api_init (int ppqn, midibpm /*bpm*/);
    virtual bool api_get_midi_event (event * in);
    virtual void api_set_ppqn (int ppqn);
    virtual void api_set_beats_per_minute (midibpm bpm);

    /*
     * TODO
     *
    virtual void api_flush ();
    virtual void api_start ();
    virtual void api_stop ();
    virtual void api_continue_from (midipulse tick);
    virtual void api_port_start (int client, int port);
     *
     */

};          // class mastermidibus

}           // namespace seq64

#endif      // SEQ64_MASTERMIDIBUS_PM_HPP

/*
 * mastermidibus_pm.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

