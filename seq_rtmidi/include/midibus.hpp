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
 *  This module declares/defines the base class for MIDI I/O for Windows.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2016-11-21
 * \updates       2016-11-21
 * \license       GNU GPLv2 or above
 *
 *  This midibus module is the RtMidi version of the midibus
 *  module.  There's almost enough commonality to be worth creating a base
 *  class for both classes.
 */

#include "app_limits.h"                 /* SEQ64_USE_DEFAULT_PPQN       */
#include "easy_macros.h"                /* for autoconf header files    */
#include "mutex.hpp"
#include "midibus_common.hpp"

/*
 * Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{

    class event;

/**
 *  This class implements with rtmidi version of the midibus object.
 */

class midibus
{
    /**
     *  The master MIDI bus sets up the buss.
     */

    friend class mastermidibus;

private:

    /**
     *  This is another name for "16 * 4".
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
     *  Provides the PPQN value in force, currently a constant.
     */

    int m_ppqn;

    /**
     *  TBD.  Is this a port number or a client number.
     */

    char m_pm_num;

    /**
     *  The type of clock to use.
     */

    clock_e m_clock_type;

    /**
     *  The name of the MIDI buss.
     */

    std::string m_name;

    /**
     *  The last (most recent?  final?) tick.
     */

    midipulse m_lasttick;

    /**
     *  Locking mutex.
     */

    mutex m_mutex;

    /**
     *
     */

    PortMidiStream * m_pms;

public:

    midibus (char id, char pm_num, const char * client_name);

    /*
     * midibus(char id, int queue);
     */

    ~midibus ();

    bool init_out ();
    bool init_in ();
    bool deinit_in ()
    {
        return false;
    }

    bool init_out_sub ()
    {
        return false;
    }

    bool init_in_sub ()
    {
        return false;
    }

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

    void play (event * e24, unsigned char channel);
    void sysex(event * e24);

    int poll_for_midi ();

    /*
     * Clock functions
     */

    void start ();
    void stop ();
    void clock (midipulse tick);
    void continue_from (midipulse tick);
    void init_clock (midipulse tick);

    /**
     * \setter m_clock_type
     *
     * \param clocktype
     *      The value used to set the clock-type.
     */

    void set_clock (clock_e clocktype)
    {
        m_clock_type = clocktype;
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

    void set_input (bool inputing)
    {
        if (m_inputing != inputing)
            m_inputing = inputing;
    }

    /**
     * \getter m_inputing
     */

    bool get_input () const
    {
        return m_inputing;
    }

    void flush ();

    static void set_clock_mod(int clockmod);
    static int get_clock_mod ();

};          // class midibus (rtmidi version)

}           // namespace seq64

#endif      // SEQ64_MIDIBUS_HPP

/*
 * midibus.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

