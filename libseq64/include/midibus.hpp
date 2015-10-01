#ifndef SEQ64_MIDIBUS_HPP
#define SEQ64_MIDIBUS_HPP

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
 * \updates       2015-09-13
 * \license       GNU GPLv2 or above
 *
 *  The midibus module is the Linux version of the midibus module.
 *  There's almost enough commonality to be worth creating a base class
 *  for both classes.
 *
 *  We moved the mastermidibus class into its own module.
 */

#include "midibus_common.hpp"

#if SEQ64_HAVE_LIBASOUND                // covers this whole module

#include <alsa/asoundlib.h>
#include <alsa/seq_midi_event.h>

namespace seq64
{

/**
 *  Provides a class for handling the MIDI buss on Linux.
 */

class midibus
{
    /**
     *  The master MIDI bus sets up the buss.
     */

    friend class mastermidibus;

private:

    /**
     *  TBD
     */

    static int m_clock_mod;

    /**
     *  The ID of the midibus object.
     */

    int m_id;

    /**
     *  The type of clock to use.
     */

    clock_e m_clock_type;

    /**
     *  TBD
     */

    bool m_inputing;

    /**
     *  ALSA sequencer client handle.
     */

    snd_seq_t * const m_seq;

    /**
     *  Destination address of client.
     */

    const int m_dest_addr_client;

    /**
     *  Destination port of client.
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

    /**
     *  Another ID of the MIDI queue?
     */

    int m_queue;

    /**
     *  The name of the MIDI buss.
     */

    std::string m_name;

    /**
     *  The last (most recent?  final?) tick.
     */

    long m_lasttick;

    /**
     *  Locking mutex.
     */

    mutex m_mutex;

public:

    midibus
    (
        int a_localclient,
        int a_destclient,
        int a_destport,
        snd_seq_t * a_seq,
        const char * a_client_name,
        const char * a_port_name,
        int a_id,
        int a_queue
    );
    midibus
    (
        int a_localclient,
        snd_seq_t * a_seq,
        int a_id,
        int a_queue
    );

    ~midibus ();

    bool init_out ();
    bool init_in ();
    bool deinit_in ();
    bool init_out_sub ();
    bool init_in_sub ();
    void print ();

    /**
     * \getter n_name
     */

    const std::string & get_name () const
    {
        return m_name;
    }

    /**
     * \getter m_id
     */

    int get_id () const
    {
        return m_id;
    }

    void play (event * a_e24, unsigned char a_channel);
    void sysex (event * a_e24);

    /*
     * Clock functions
     */

    void start ();
    void stop ();
    void clock (long a_tick);
    void continue_from (long a_tick);
    void init_clock (long a_tick);

    /**
     * \setter m_clock_type
     */

    void set_clock (clock_e a_clock_type)
    {
        m_clock_type = a_clock_type;
    }

    /**
     * \getter m_clock_type
     */

    clock_e get_clock () const
    {
        return m_clock_type;
    }

    /**
     *  Input functions
     */

    void set_input (bool a_inputing);   // too much to inline

    /**
     * \getter m_inputing
     */

    bool get_input () const
    {
        return m_inputing;
    }
    void flush ();

    /**
     *  \getter m_dest_addr_client
     *      The address of client.
     */

    int get_client () const
    {
        return m_dest_addr_client;
    }

    /**
     *  \getter m_dest_addr_port
     */

    int get_port () const
    {
        return m_dest_addr_port;
    };

    /**
     *  Set the clock mod to the given value, if legal.
     */

    static void set_clock_mod (int a_clock_mod)
    {
        if (a_clock_mod != 0)
            m_clock_mod = a_clock_mod;
    }

    /**
     *  Get the clock mod.
     */

    static int get_clock_mod ()
    {
        return m_clock_mod;
    }

};

#endif      // SEQ64_HAVE_LIBASOUND

}           // namespace seq64

#endif      // SEQ64_MIDIBUS_HPP

/*
 * midibus.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
