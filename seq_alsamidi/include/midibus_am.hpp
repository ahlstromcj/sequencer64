#ifndef SEQ64_MIDIBUS_ALSAMIDI_HPP
#define SEQ64_MIDIBUS_ALSAMIDI_HPP

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
 * \file          midibus.hpp
 *
 *  This module declares/defines the base class for MIDI I/O under Linux.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2017-01-08
 * \license       GNU GPLv2 or above
 *
 *  The midibus module is the Linux version of the midibus module.
 *  There's almost enough commonality to be worth creating a base class
 *  for both classes.
 *
 *  We moved the mastermidibus class into its own module.
 */

#include "midibase.hpp"

#if SEQ64_HAVE_LIBASOUND
#include <alsa/asoundlib.h>
#include <alsa/seq_midi_event.h>
#else
#error ALSA not supported in this build, fix the project configuration.
#endif

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{
    class event;

/**
 *  This class implements with ALSA version of the midibus object.
 */

class midibus : public midibase
{
    /**
     *  The master MIDI bus sets up the buss.
     */

    friend class mastermidibus;

private:

    /**
     *  ALSA sequencer client handle.
     */

    snd_seq_t * const m_seq;

    /**
     *  Destination address of client.  Could potentially be replaced by
     *  midibase::m_bus_id.
     */

    const int m_dest_addr_client;

    /**
     *  Destination port of client.  Could potentially be replaced by
     *  midibase::m_port_id.
     */

    const int m_dest_addr_port;

    /**
     *  Local address of client.
     */

    const int m_local_addr_client;

    /**
     *  Local port of client.
     */

    int m_local_addr_port;

public:

    /*
     *  This version is used when querying for existing input ports in the
     *  ALSA system.  It is also used when creating the "announce buss".
     *  Does not yet directly include the concept of buss ID and port ID.
     */

    midibus
    (
        int localclient,
        int destclient,
        int destport,
        snd_seq_t * seq,
        const std::string & client_name,
        const std::string & port_name,
        int index,                              /* a display ordinal    */
        int queue,
        int ppqn = SEQ64_USE_DEFAULT_PPQN,
        int bpm  = SEQ64_DEFAULT_BPM
    );

    /*
     *  This version is used with the --manual-alsa-ports option, for both
     *  input and output busses.  Does not yet directly include the concept of
     *  buss ID and port ID.
     */

    midibus
    (
        int localclient,
        snd_seq_t * seq,
        int index,                              /* a display ordinal    */
        int bus_id,
        int queue,
        int ppqn = SEQ64_USE_DEFAULT_PPQN,
        int bpm  = SEQ64_DEFAULT_BPM
    );

    virtual ~midibus ();

    /**
     * \getter m_dest_addr_client
     *      The address of client.
     */

    virtual int get_client () const
    {
        return m_dest_addr_client;
    }

    /**
     * \getter m_dest_addr_port
     */

    virtual int get_port () const
    {
        return m_dest_addr_port;
    }

protected:

    virtual bool api_init_out ();
    virtual bool api_init_in ();
    virtual bool api_init_out_sub ();
    virtual bool api_init_in_sub ();
    virtual bool api_deinit_in ();
    virtual void api_play (event * e24, midibyte channel);
    virtual void api_sysex (event * e24);
    virtual void api_flush ();
    virtual void api_continue_from (midipulse tick, midipulse beats);
    virtual void api_start ();
    virtual void api_stop ();
    virtual void api_clock (midipulse tick);

private:

    bool set_virtual_name (int portid, const std::string & portname);

};          // class midibus (ALSA version)

}           // namespace seq64

#endif      // SEQ64_MIDIBUS_ALSAMIDI_HPP

/*
 * midibus.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

