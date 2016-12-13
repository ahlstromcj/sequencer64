#ifndef SEQ64_MIDIBASE_HPP
#define SEQ64_MIDIBASE_HPP

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
 * \file          midibase.hpp
 *
 *  This module declares/defines the base class for MIDI I/O under Linux.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2016-11-24
 * \updates       2016-12-12
 * \license       GNU GPLv2 or above
 *
 *  The midibase module is the new base class for the various implementations
 *  of the midibus module.  There is enough commonality to be worth creating a
 *  base class for all such classes.
 */

#include "app_limits.h"                 /* SEQ64_USE_DEFAULT_PPQN       */
#include "easy_macros.h"                /* for autoconf header files    */
#include "mutex.hpp"
#include "midibus_common.hpp"

/**
 *  Flags an unspecified buss number.
 */

#define SEQ64_NO_BUS                    (-1)

/**
 *  Flags an unspecified port number.
 */

#define SEQ64_NO_PORT                   (-1)

/**
 *  Flags an unspecified queue number.
 */

#define SEQ64_NO_QUEUE                  (-1)

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{
    class event;

/**
 *  This class implements with ALSA version of the midibase object.
 */

class midibase
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
     *  Provides the index of the midibase object in either the input list or
     *  the output list.  Currently needed in the RtMidi code.  Otherwise, it
     *  is currently -1.
     */

    const int m_bus_index;

    /**
     *  The buss ID of the midibase object.  For example, on one system the
     *  IDs are 14 (MIDI Through), 128 (TiMidity), and 129 (Yoshimi).
     */

    int m_bus_id;

    /**
     *  The port ID of the midibase object.
     */

    int m_port_id;

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
     *  Some APIs can control or use this value.
     */

    int m_ppqn;

    /**
     *  Provides the PPQN value in force, currently a constant.
     *  Some APIs can control or use this value.
     */

    int m_bpm;

    /**
     *  Another ID of the MIDI queue?  This is an implementation-dependent
     *  value.  For ALSA, it is the ALSA queue number.  For PortMidi, this is
     *  the old "m_pm_num" value.  For RtMidi, it is not currently used.
     */

    int m_queue;

    /**
     *  The name of the MIDI buss.  This should be something like a major device
     *  name or the name of a subsystem such as Timidity.
     */

    std::string m_bus_name;

    /**
     *  The name of the MIDI port.  This should be the name of a specific device
     *  or port on a major device.
     */

    std::string m_port_name;

    /**
     *  The last (most recent? final?) tick.
     */

    midipulse m_lasttick;

    /**
     *  Indicates if the port is to be a virtual port.  The default is to
     *  create a system port (true).
     */

    bool m_is_virtual_port;

    /**
     *  Locking mutex.
     */

    mutex m_mutex;

public:

    midibase
    (
        const std::string & client_name = "",       /* usually a full name  */
        const std::string & port_name   = "",       /* rarely needed now    */
        int index   = 0,                            /* a display ordinal    */
        int bus_id  = SEQ64_NO_BUS,
        int port_id = SEQ64_NO_PORT,
        int queue   = SEQ64_NO_QUEUE,
        int ppqn    = SEQ64_USE_DEFAULT_PPQN,       /* SEQ64_DEFAULT_PPQN   */
        int bpm     = SEQ64_DEFAULT_BPM,
        bool makevirtual = false
    );

    virtual ~midibase ();

    /**
     * \getter m_bus_name
     */

    const std::string & bus_name () const
    {
        return m_bus_name;
    }

    /**
     * \getter m_port_name
     */

    const std::string & port_name () const
    {
        return m_port_name;
    }

    /**
     * \getter m_bus_name and m_port_name
     */

    std::string connect_name () const
    {
        std::string result = m_bus_name;
        if (! m_port_name.empty())
        {
            result += ":";
            result += m_port_name;
        }
        return result;
    }

    /**
     * \getter m_bus_index
     */

    int get_bus_index () const
    {
        return m_bus_index;
    }

    /**
     * \getter m_bus_id
     */

    int get_bus_id () const
    {
        return m_bus_id;
    }

    /**
     * \getter m_port_id
     */

    int get_port_id () const
    {
        return m_port_id;
    }

    /*
     * The following are currently REDUNDANT, must clean 'em out once we
     * replace the ALSA members m_dest_addr_client and m_dest_addr_port
     * with m_bus_id and m_port_id.
     */

    /**
     * \getter m_dest_addr_client
     *      The address of client.

    virtual int get_client () const
    {
        return SEQ64_NO_BUS;
    }
     */

    /**
     * \getter m_dest_addr_port

    virtual int get_port () const
    {
        return SEQ64_NO_PORT;
    }
     */

    /**
     * \getter m_is_virtual_port
     */

    bool is_virtual_port () const
    {
        return m_is_virtual_port;
    }

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
     * \getter m_inputing
     */

    bool get_input () const
    {
        return m_inputing;
    }

    /**
     *  Set the clock mod to the given value, if legal.
     *
     * \param clockmod
     *      If this value is not equal to 0, it is used to set the static
     *      member m_clock_mod.
     */

    static void set_clock_mod (int clockmod)
    {
        if (clockmod != 0)
            m_clock_mod = clockmod;
    }

    /**
     *  Get the clock mod value.
     */

    static int get_clock_mod ()
    {
        return m_clock_mod;
    }

    int poll_for_midi ();
    bool init_out ();
    bool init_in ();
    bool deinit_in ();
    bool init_out_sub ();
    bool init_in_sub ();
    void play (event * e24, midibyte channel);
    void sysex (event * e24);
    void flush ();
    void start ();
    void stop ();
    void clock (midipulse tick);
    void continue_from (midipulse tick);
    void init_clock (midipulse tick);
    void print ();
    virtual void set_input (bool inputing);

protected:

    /**
     * \setter m_bus_name
     */

    void bus_name (const std::string & name)
    {
        m_bus_name = name;
    }

    /**
     * \setter m_port_name
     */

    void port_name (const std::string & name)
    {
        m_port_name = name;
    }

    /**
     * \setter m_bus_id
     *      Useful for setting the buss ID when using the rtmidi_info object
     *      to create a list of busses and ports.
     */

    void set_bus_id (int id)
    {
        m_bus_id = id;
    }

    /**
     * \setter m_port_id
     *      Useful for setting the port ID when using the rtmidi_info object
     *      to inspect and create a list of busses and ports.
     */

    void set_port_id (int id)
    {
        m_port_id = id;
    }

    /**
     * \getter m_queue
     */

    int queue_number () const
    {
        return m_queue;
    }

    /**
     *  Not defined in the ALSA implementation.
     */

    virtual int api_poll_for_midi ()
    {
        return 0;                       /* no code for alsa     */
    }

    /**
     *  Not defined in the PortMidi implementation.
     */

    virtual bool api_init_in_sub ()
    {
        return false;                   /* no code for portmidi */
    }

    /**
     *  Not defined in the PortMidi implementation.
     */

    virtual bool api_init_out_sub ()
    {
        return false;                   /* no code for portmidi */
    }

    /**
     *  Not defined in the PortMidi implementation.
     */

    virtual bool api_deinit_in ()
    {
        return false;                   /* no code for portmidi */
    }

    virtual void api_play (event * e24, midibyte channel) = 0;

    /**
     *  Handles implementation details for SysEx messages.
     *
     * \param e24
     *      The SysEx event pointer.
     */

    virtual void api_sysex (event * /* e24 */)
    {
        // no code for portmidi
    }

    /**
     *  Handles implementation details for the flush() function.
     */

    virtual void api_flush ()
    {
        // no code for portmidi
    }

protected:

    virtual bool api_init_in () = 0;
    virtual bool api_init_out () = 0;
    virtual void api_continue_from (midipulse tick, midipulse beats) = 0;
    virtual void api_start () = 0;
    virtual void api_stop () = 0;
    virtual void api_clock (midipulse tick) = 0;

protected:

    void set_name
    (
        const std::string & clientname,
        const std::string & portname
    );

};          // class midibase (ALSA version)

/*
 *  Free functions
 */

extern void millisleep (unsigned long ms);

}           // namespace seq64

#endif      // SEQ64_MIDIBASE_HPP

/*
 * midibase.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

