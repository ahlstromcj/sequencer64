#ifndef SEQ64_MIDIBUS_PORTMIDI_HPP
#define SEQ64_MIDIBUS_PORTMIDI_HPP

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
 * \file          midibus_portmidi.hpp
 *
 *  This module declares/defines the base class for MIDI I/O for Windows.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2016-03-04
 * \license       GNU GPLv2 or above
 *
 *  The midibus_portmidi module is the Windows version of the midibus
 *  module.  There's almost enough commonality to be worth creating a base
 *  class for both classes, and it might be nice to put the mastermidibus
 *  classes into their own modules.
 */

#include "midibus_common.hpp"

namespace seq64
{

#ifdef PLATFORM_WINDOWS                // covers this whole module

/**
 *  This class implements with Windows version of the midibus object.
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

    char m_id;

    /**
     *  TBD
     */

    char m_pm_num;

    /**
     *  The type of clock to use.
     */

    clock_e m_clock_type;

    /**
     *  TBD
     */

    bool m_inputing;

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

    /**
     *  The PortMidiStream for the Windows implementation.
     */

    PortMidiStream * m_pms;

public:

    midibus (char a_id, char a_pm_num, const char * a_client_name);
    midibus(char a_id, int a_queue);

    ~midibus ();

    bool init_out ();
    bool init_in ();
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
    void sysex(event * a_e24);

    int poll_for_midi ();

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
     * \setter m_inputing
     *      Compare this Windows version to the Linux version.
     */

    void set_input (bool a_inputing)
    {
        if (m_inputing != a_inputing)
            m_inputing = a_inputing;
    }

    /**
     * \getter m_inputing
     */

    bool get_input () const
    {
        return m_inputing;
    }

    void flush ();

    static void set_clock_mod(int a_clock_mod);
    static int get_clock_mod ();

};

#endif      // PLATFORM_WINDOWS

}           // namespace seq64

#endif      // SEQ64_MIDIBUS_PORTMIDI_HPP

/*
 * midibus_portmidi.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
