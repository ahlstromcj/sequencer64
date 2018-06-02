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
 *  and Windows, using a refactored "RtMidi" library.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2016-11-21
 * \updates       2017-02-19
 * \license       GNU GPLv2 or above
 *
 *  This midibus module is the RtMidi version of the midibus
 *  module.
 */

#include "midibase.hpp"                 /* seq64::midibase class (new)  */
#include "rtmidi_types.hpp"             /* SEQ64_MIDI_NORMAL_PORT       */

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
     *  The master MIDI bus sets up the buss, so it gets access to private
     *  details.
     */

    friend class mastermidibus;

private:

    /**
     *  The RtMidi API interface object this midibus will be creating and then
     *  using.
     */

    rtmidi * m_rt_midi;

    /**
     *  For Sequencer64, the ALSA model used requires that all the midibus
     *  objects use the same ASLA sequencer "handle".  The rtmidi_info object
     *  used for enumerating the ports is a good place to get this handle.
     *  It is an extension of the legacy RtMidi interface.
     */

    rtmidi_info & m_master_info;

public:

    /*
     * Virtual-port and non-virtual-port constructor.
     */

    midibus
    (
        rtmidi_info & rt,
        int index,
        bool makevirtual    = SEQ64_MIDI_NORMAL_PORT,
        bool isinput        = SEQ64_MIDI_OUTPUT_PORT,   /* gotcha! */
        int bussoverride    = SEQ64_NO_BUS,             /* was 0!! */
        bool makesystem     = false
    );

    virtual ~midibus ();

    virtual bool api_connect ();

protected:

    virtual bool api_init_in ();
    virtual bool api_init_in_sub ();
    virtual bool api_init_out ();
    virtual bool api_init_out_sub ();
    virtual bool api_deinit_in ();
    virtual bool api_get_midi_event (event * inev);
    virtual int api_poll_for_midi ();
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

