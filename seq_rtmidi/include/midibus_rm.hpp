#ifndef SEQ64_MIDIBUS_RM_HPP
#define SEQ64_MIDIBUS_RM_HPP

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
 * \file          midibus_rm.hpp
 *
 *  This module declares/defines the base class for MIDI I/O for Linux, Mac,
 *  and Windows, using a refactored RtMidi library..
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2016-11-21
 * \updates       2016-12-10
 * \license       GNU GPLv2 or above
 *
 *  This midibus module is the RtMidi version of the midibus
 *  module.
 */

#include "midibase.hpp"                 /* seq64::midibase class (new)  */

/*
 * Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{
    class event;
    class rtmidi;
    class rtmidi_info;

/**
 *  This class implements with rtmidi version of the midibus object.
 */

class midibus : public midibase
{
    /**
     *  The master MIDI bus sets up the buss.
     */

    friend class mastermidibus;

private:

    /**
     *  The RtMidi API interface object this midibus will be using.
     */

    rtmidi * m_rt_midi;

public:

    /*
     * Virtual-port and non-virtual-port constructors.
     */

    midibus
    (
        const std::string & clientname,
        const std::string & portname    = "",
        int index                       = 0,
        int bus_id                      = SEQ64_NO_BUS,
        int port_id                     = SEQ64_NO_PORT,
        int queue                       = SEQ64_NO_QUEUE,
        int ppqn                        = SEQ64_USE_DEFAULT_PPQN,
        int bpm                         = SEQ64_DEFAULT_BPM
    );

    midibus
    (
        /*const*/ rtmidi_info & rt,
        const std::string & clientname,             /* includes port name   */
        int index                       = 0,
        int ppqn                        = SEQ64_USE_DEFAULT_PPQN,
        int bpm                         = SEQ64_DEFAULT_BPM
    );

    virtual ~midibus ();

protected:


    virtual int api_poll_for_midi ();
    virtual bool api_init_in ();
    virtual bool api_init_out ();

    /*
     *  Provides common code between api_init_in() and api_initi_out().
     */

    bool api_init_common (rtmidi * rtm);

    /**
     *  Temporary easy implementation for now.
     */

    virtual bool api_init_in_sub ()
    {
        return api_init_in();
    }

    /**
     *  Temporary easy implementation for now.
     */

    virtual bool api_init_out_sub ()
    {
        return api_init_out();
    }

    virtual void api_continue_from (midipulse tick, midipulse beats);
    virtual void api_start ();
    virtual void api_stop ();
    virtual void api_clock (midipulse tick);
    virtual void api_play (event * e24, midibyte channel);

};          // class midibus (rtmidi version)

}           // namespace seq64

#endif      // SEQ64_MIDIBUS_RM_HPP

/*
 * midibus_rm.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

