#ifndef SEQ64_MASTERMIDIBUS_RM_HPP
#define SEQ64_MASTERMIDIBUS_RM_HPP

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
 * \file          mastermidibus_rm.hpp
 *
 *  This module declares/defines the base class for MIDI I/O for Linux, Mac,
 *  and Windows, via the RtMidi API.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2017-03-21
 * \license       GNU GPLv2 or above
 *
 *  This mastermidibus module is the Linux (and, soon, JACK) version of the
 *  mastermidibus module using the completely refactored RtMidi library.
 */

#include "mastermidibase.hpp"           /* seq64::mastermidibase ABC        */
#include "rtmidi_info.hpp"              /* seq64::rtmidi_info, new class    */

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
    friend class midi_alsa_info;
    friend class midi_jack_info;

private:

    /**
     *  Holds the basic MIDI input and output information for later re-use in
     *  the construction of midibus objects.
     */

    rtmidi_info m_midi_master;

    /**
     *  Indicates we are running with JACK MIDI enabled, and need to
     *  use each port's ability to poll for and get MIDI events, rather
     *  than use ALSA's method of calling functions from the "MIDI master"
     *  object.
     */

    bool m_use_jack_polling;

public:

    mastermidibus
    (
        int ppqn    = SEQ64_USE_DEFAULT_PPQN,
        midibpm bpm = SEQ64_DEFAULT_BPM         /* c_beats_per_minute   */
    );
    virtual ~mastermidibus ();

    virtual bool activate ();

protected:

    virtual bool api_get_midi_event (event * in);
    virtual int api_poll_for_midi ();
    virtual void api_init (int ppqn, midibpm bpm);

    /**
     *  Provides MIDI API-specific functionality for the set_ppqn() function.
     */

    virtual void api_set_ppqn (int p)
    {
        m_midi_master.api_set_ppqn(p);
    }

    /**
     *  Provides MIDI API-specific functionality for the
     *  set_beats_per_minute() function.
     */

    virtual void api_set_beats_per_minute (midibpm b)
    {
        m_midi_master.api_set_beats_per_minute(b);
    }

    virtual void api_flush ()
    {
        m_midi_master.api_flush();
    }

    virtual void api_port_start (mastermidibus & masterbus, int bus, int port)
    {
        m_midi_master.api_port_start(masterbus, bus, port);
    }

private:

    void port_list (const std::string & tag);

};          // class mastermidibus

}           // namespace seq64

#endif      // SEQ64_MASTERMIDIBUS_RM_HPP

/*
 * mastermidibus_rm.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

