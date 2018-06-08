#ifndef SEQ64_MIDIBUS_PM_HPP
#define SEQ64_MIDIBUS_PM_HPP

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
 * \file          midibus_pm.hpp
 *
 *  This module declares/defines the base class for MIDI I/O for Windows.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2017-08-30
 * \license       GNU GPLv2 or above
 *
 *  This midibus module is the Windows (PortMidi) version of the midibus
 *  module.  There's  enough commonality that is was worth creating a base
 *  class for all midibus classes.
 */

#include "midibase.hpp"
#include "portmidi.h"                   /* PortMIDI API header file         */

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{
    class event;

/**
 *  This class implements with Windows version of the midibus object.
 */

class midibus : public midibase
{
    /**
     *  The master MIDI bus sets up the buss.
     */

    friend class mastermidibus;

private:

    /**
     *  The PortMidiStream for the Windows/Linux/Mac OSX implementation.
     */

    PortMidiStream * m_pms;

public:

    /*
     * Supports a lot fewer parameters of Sequencer64 than do other APIs.
     */

    midibus
    (
        int index, int bus_id, int port_id, const std::string & client_name
    );

    virtual ~midibus ();

protected:

    virtual int api_poll_for_midi ();
    virtual bool api_init_in ();
    virtual bool api_init_out ();
    virtual void api_continue_from (midipulse tick, midipulse beats);
    virtual void api_start ();
    virtual void api_stop ();
    virtual void api_clock (midipulse tick);
    virtual void api_play (event * e24, midibyte channel);

    /*
     * Functions not implemented in PortMIDI.  For example, the "sub"
     * functions, which subscribe the application to a "virtual" port, can
     * be implemented in ALSA, but not in Windows.
     *
     * virtual bool api_init_out_sub ();        // subscribe to output
     * virtual bool api_init_in_sub ();         // subscribe to input
     * virtual bool api_deinit_in ();           // unsubscribe a port
     *
     * We should be able to implement this in a "sysex_fix" branch:
     *
     * virtual void api_sysex (event * e24);
     *
     * This function should be able to be implemented in Windows and ALSA:
     *
     * virtual void api_flush ();
     */

};          // class midibus (portmidi)

}           // namespace seq64

#endif      // SEQ64_MIDIBUS_PM_HPP

/*
 * midibus_pm.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

