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
 * \updates       2017-05-31
 * \license       GNU GPLv2 or above
 *
 *  The businfo module defines the businfo and busarray classes so that we can
 *  start avoiding arrays and explicit access to them.  This module replaces
 *  the following arrays from the mastermidibus class: m_buses_out[],
 *  m_buses_in[], m_buses_out_active[], m_buses_in_active[],
 *  m_buses_out_init[], m_buses_in_init[], m_init_clock[], and m_init_input[].
 *
 *  The businfo class holds a pointer to its midibus object.  We could make
 *  the values noted above part of the midibus class at some point.
 *
 *  The busarray class holds a number of businfo classes, and two busarrays
 *  are maintained, one for input and one for output.
 */

#include <vector>                       /* for containing the bus objects   */

#include "midibus_common.hpp"           /* enum clock_e                     */
#include "midibus.hpp"                  /* seq64::midibus                   */

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
    businfo (midibus * bus);
    businfo (const businfo & rhs);

    /**
     * We can't destroy the bus pointer.
     */

    ~businfo ()
    {
        // No code
    }

    /**
     *  Deletes and nullifies the m_bus pointer.
     */

    void remove ()
    {
        if (not_nullptr(m_bus))
        {
            delete m_bus;
            m_bus = nullptr;
        }
    }

    /**
     * \getter m_bus pointer, const version
     */

    const midibus * bus () const
    {
        return m_bus;
    }

    /**
     * \getter m_bus pointer
     */

    midibus * bus ()
    {
        return m_bus;
    }

    /**
     * \getter m_active
     */

    bool active () const
    {
        return m_active;
    }

    bool initialize ();

    /**
     * \getter m_initialized
     */

    bool initialized () const
    {
        return m_initialized;
    }

    /**
     * \getter m_init_clock
     */

    clock_e init_clock () const
    {
        return m_init_clock;
    }

    /**
     * \getter m_init_input
     */

    bool init_input () const
    {
        return m_init_input;
    }

public:

    /**
     * \setter m_bus
     */

    void bus (midibus * b)
    {
        m_bus = b;
    }

    /**
     * \setter m_active and m_initialized
     */

    void activate ()
    {
        m_active = true;
        m_initialized = true;
    }

    /**
     * \setter m_active and m_initialized
     */

    void deactivate ()
    {
        m_active = false;
        m_initialized = false;
    }

    /**
     * \setter m_init_clock and bus clock
     */

    void init_clock (clock_e clocktype)
    {
        m_init_clock = clocktype;
        if (not_nullptr(bus()))
            bus()->set_clock(clocktype);
    }

    /**
     * \setter m_init_input and bus input
     */

    void init_input (bool flag)
    {
        m_init_input = flag;

        /*
         * When clicking on the MIDI Input item, this is not needed...
         * it disables the detection of a change, so that init() and deinit()
         * do not get called.
         *
         * When starting up we need to honor the init-input flag if it is
         * set, and init() the bus.  But we don't need to call deinit() at
         * startup if it is false, since init() hasn't been called yet.
         */

        if (not_nullptr(bus()))
            bus()->set_input_status(flag);
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

private:

    void print () const;

};          // class businfo

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

    bool add (midibus * bus, clock_e clock);
    bool add (midibus * bus, bool inputing);
    bool initialize ();

    int count () const
    {
        return int(m_container.size());
    }

    midibus * bus (bussbyte b)
    {
        midibus * result = nullptr;
        if (b < bussbyte(count()))
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
    bool set_clock (bussbyte bus, clock_e clocktype);
    void set_all_clocks ();
    clock_e get_clock (bussbyte bus);
    std::string get_midi_bus_name (int bus);        // full version
    void print () const;
    void port_exit (int client, int port);
    bool set_input (bussbyte bus, bool inputing);
    void set_all_inputs ();
    bool get_input (bussbyte bus);
    bool is_system_port (bussbyte bus);
    int poll_for_midi ();
    bool get_midi_event (event * inev);
    int replacement_port (int bus, int port);

};          // class busarray

/*
 * Free functions
 */

extern void swap (busarray & buses0, busarray & buses1);

}           // namespace seq64

#endif      // SEQ64_BUSINFO_HPP

/*
 * businfo.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

