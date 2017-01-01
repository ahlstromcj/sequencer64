#ifndef SEQ64_BUSINFO_HPP
#define SEQ64_BUSINFO_HPP

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
 * \file          businfo.hpp
 *
 *  This module declares/defines the Master MIDI Bus base class.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2016-12-31
 * \updates       2017-01-01
 * \license       GNU GPLv2 or above
 *
 *  The businfo module defines the businfo and busarray classes so that we can
 *  start avoiding arrays and explicit access to them.
 */

#include <vector>                       /* for containing the bus objects   */

#include "midibus_common.hpp"           /* enum clock_e                     */
#include "midibus.hpp"                  /* seq64::midibus           */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{
    class event;
    class midibus;

/**
 *  A new class to consolidate a number of bus-related arrays into one array.
 *  There will be in input instance and an output instance of this object
 *  contained by mastermidibus.
 */

class businfo
{
    friend class busarray;

private:

    /**
     *  Points to an existing midibus object.
     */

    midibus * m_bus;

    /**
     *  Indicates if the existing bus is active.
     */

    bool m_active;

    /**
     *  Indicates if the existing bus is initialized.
     */

    bool m_initialized;

    /**
     *  Clock initialization.
     */

    clock_e m_init_clock;

    /**
     *  Input initialization?
     */

    bool m_init_input;

public:

    businfo ();
    businfo (midibus * bus, bool is_input, bool is_virtual);

    /**
     * We can't destroy the bus pointer.
     */

    ~businfo ()
    {
        // No code
    }

    void remove ()
    {
        if (not_nullptr(m_bus))
        {
            delete m_bus;
            m_bus = 0;
        }
    }

    const midibus * bus () const
    {
        return m_bus;
    }

    midibus * bus ()
    {
        return m_bus;
    }

    bool active () const
    {
        return m_active;
    }

    bool initialized () const
    {
        return m_initialized;
    }

    clock_e init_clock () const
    {
        return m_init_clock;
    }

    bool init_input () const
    {
        return m_init_input;
    }

public:

    void bus (midibus * b)
    {
        m_bus = b;
    }

    void activate ()
    {
        m_active = true;
        m_initialized = true;
    }

    void deactivate ()
    {
        m_active = false;
    }

    void init_clock (clock_e c)
    {
        m_init_clock = c;
    }

    void init_input (bool flag)
    {
        m_init_input = flag;
    }

private:

    void start ()
    {
        bus()->start();
    }

    void stop ()
    {
        bus()->stop();
    }

    void continue_from (midipulse tick)
    {
        bus()->continue_from(tick);
    }

    void init_clock (midipulse tick)
    {
        bus()->init_clock(tick);
    }

    void clock (midipulse tick)
    {
        bus()->clock(tick);
    }

    void sysex (event * ev)
    {
        bus()->sysex(ev);
    }

};          // class busarray

/**
 *  Holds a number of businfo objects.
 */

class busarray
{

private:

    /**
     *  The full set of businfo objects, only some of which will actually be
     *  used.
     */

    std::vector<businfo> m_container;

public:

    busarray ();
    ~busarray ();

    bool add (midibus * bus, bool is_input, bool is_virtual);

    int count () const
    {
        return int(m_container.size());
    }

    midibus * bus (bussbyte b)
    {
        midibus * result = nullptr;
        if (b < count())
            result = m_container[b].bus();

        return result;
    }

    void start ();
    void stop ();
    void continue_from (midipulse tick);
    void init_clock (midipulse tick);
    void clock (midipulse tick);
    void sysex (event * ev);
    void play (bussbyte bus, event * e24, midibyte channel);
    void set_clock (bussbyte bus, clock_e clocktype);
    void set_all_clocks ();
    clock_e get_clock (bussbyte bus);
    std::string get_midi_bus_name (int bus);
    void print ();
    void port_exit (int client, int port);
    void set_input (bussbyte bus, bool inputing);
    void set_all_inputs ();
    bool get_input (bussbyte bus);
    bool poll_for_midi ();
    int replacement_port (int bus, int port);

};          // class busarray

}           // namespace seq64

#endif      // SEQ64_BUSINFO_HPP

/*
 * businfo.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

