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
 * \updates       2017-05-12
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
#include "midibyte.hpp"                 /* seq64::midibyte typedef          */

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
     *  the output list.  Otherwise, it is currently -1.
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
     *  The type of clock to use.  The special value e_clock_disabled means
     *  we will not be using the port, so that a failure in setting up the
     *  port is not a "fatal error".  (We could have added an "m_outputing"
     *  boolean as an alternative.)
     */

    clock_e m_clock_type;

    /**
     *  This flag indicates if an input bus has been selected for action as an
     *  input device (such as a MIDI controller).  It is turned on if the user
     *  selects the port in the Options / MIDI Input tab.
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

    midibpm m_bpm;

    /**
     *  Another ID of the MIDI queue?  This is an implementation-dependent
     *  value.  For ALSA, it is the ALSA queue number.  For PortMidi, this is
     *  the old "m_pm_num" value.  For RtMidi, it is not currently used.
     */

    int m_queue;

    /**
     *  Holds the full display name of the bus, index, ID numbers, and item
     *  names.  Assembled by the set_name() function.
     */

    std::string m_display_name;

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
     *  Indicates if the port is to be an input (versus output) port.
     *  It matters when we are creating the name of the port, where we don't
     *  want an input virtual port to have the same name as an output virtual
     *  port... one of them will fail.
     */

    bool m_is_input_port;

    /**
     *  Indicates if the port is a system port.  Two examples are the ALSA
     *  System Timer buss and the ALSA System Announce bus, the latter being
     *  necessary for input subscription and notification.  For most ports,
     *  this value will be false.  A restricted setter is provided.
     *  Only the rtmidi ALSA implementation sets this flag.
     */

    bool m_is_system_port;

    /**
     *  Locking mutex.
     */

    mutex m_mutex;

public:

    midibase
    (
        const std::string & appname,                /* usually the app name */
        const std::string & busname     = "",       /* subsystem name       */
        const std::string & portname    = "",       /* rarely needed now    */
        int index                       = 0,        /* a display ordinal    */
        int bus_id                      = SEQ64_NO_BUS,
        int port_id                     = SEQ64_NO_PORT,
        int queue                       = SEQ64_NO_QUEUE,
        int ppqn                        = SEQ64_USE_DEFAULT_PPQN,
        midibpm bpm                     = SEQ64_DEFAULT_BPM,
        bool makevirtual                = false,
        bool isinput                    = false,
        bool makesystem                 = false
    );

    virtual ~midibase ();

    void show_bus_values ();

    static void show_clock (const std::string & context, midipulse tick);

    /**
     * \getter m_display_name
     */

    const std::string & display_name () const
    {
        return m_display_name;
    }

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

    std::string connect_name () const;

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

    /**
     * \getter  m_ppqn;
     */

    int ppqn () const
    {
        return m_ppqn;
    }

    /**
     * \getter  m_bpm;
     */

    midibpm bpm () const
    {
        return m_bpm;
    }

    /**
     *  Checks if the given parameters match the current bus and port numbers.
     */

    bool match (int bus, int port)
    {
        return (m_port_id == port) && (m_bus_id == bus);
    }

    /**
     * \getter m_is_virtual_port
     */

    bool is_virtual_port () const
    {
        return m_is_virtual_port;
    }

    /**
     * \setter m_is_virtual_port
     *      This function is needed in the rtmidi library to set the
     *      is-virtual flag in the api_init_*_sub() functions, so that
     *      midi_alsa, midi_jack (and any other additional APIs that end up
     *      supported by our heavily-refactored rtmidi library), as well as
     *      the original midibus, can know that they represent a virtual port.
     */

    void is_virtual_port (bool flag)
    {
        m_is_virtual_port = flag;
    }

    /**
     * \getter m_is_input_port
     */

    bool is_input_port () const
    {
        return m_is_input_port;
    }

    /**
     * \getter ! m_is_input_port
     */

    bool is_output_port () const
    {
        return ! m_is_input_port;
    }

    /**
     * \setter m_is_input_port
     */

    void is_input_port (bool flag)
    {
        m_is_input_port = flag;
    }

    /**
     * \getter m_is_system_port
     */

    bool is_system_port () const
    {
        return m_is_system_port;
    }

    /**
     * \setter m_is_system_port
     *      Can only set it to true.
     */

    void set_system_port_flag ()
    {
        m_is_system_port = true;
    }

    /**
     * \setter m_clock_type
     *      We removed the redundant set_clock_status() function.
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
     * \getter m_clock_type
     */

    bool port_disabled () const
    {
        return m_clock_type == e_clock_disabled;
    }

    /**
     * \getter m_clock_type
     */

    bool clock_enabled () const
    {
        return m_clock_type != e_clock_off && m_clock_type != e_clock_disabled;
    }

    /**
     * \getter m_inputing
     */

    bool get_input () const
    {
        return m_inputing;
    }

    /**
     * \setter m_inputing
     */

    void set_input_status (bool flag)
    {
        m_inputing = flag;
    }

    /**
     * \getter m_queue
     */

    int queue_number () const
    {
        return m_queue;
    }

    /**
     * \setter m_bus_id
     *      Useful for setting the buss ID when using the rtmidi_info object
     *      to create a list of busses and ports.  Would be protected, but
     *      midi_alsa needs to change this value to reflect the user-client ID
     *      actually assigned by ALSA.  (That value ranges from 128 to 191.)
     */

    void set_bus_id (int id)
    {
        m_bus_id = id;
    }

    void set_name
    (
        const std::string & appname,
        const std::string & busname,
        const std::string & portname
    );
    void set_alt_name
    (
        const std::string & appname,
        const std::string & busname,
        const std::string & portname
    );
    void set_multi_name
    (
        const std::string & appname,
        const std::string & localbusname,
        const std::string & remoteportname
    );

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
    bool get_midi_event (event * inev);
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
    bool set_input (bool inputing);

protected:

    /**
     * \setter m_display_name
     */

    void display_name (const std::string & name)
    {
        m_display_name = name;
    }

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
     * \setter m_port_id
     *      Useful for setting the port ID when using the rtmidi_info object
     *      to inspect and create a list of busses and ports.
     */

    void set_port_id (int id)
    {
        m_port_id = id;
    }

    /**
     *  Now defined in the ALSA implementation, and used by mastermidibus.
     *  Also used in the JACK implementation.
     */

    virtual int api_poll_for_midi ()
    {
        return 0;
    }

    /**
     *  Used in the JACK implementation.
     */

    virtual bool api_get_midi_event (event * inev)
    {
        errprint("api_get_midi_event() BASE VERSION CALLED");
        return not_nullptr(inev);
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
     *  The \a e24 parameter, the SysEx event pointer, is unused here.
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

};          // class midibase

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

