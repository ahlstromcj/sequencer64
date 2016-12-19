#ifndef SEQ64_MIDI_API_HPP
#define SEQ64_MIDI_API_HPP

/**
 * \file          midi_api.hpp
 *
 *  An abstract base class for realtime MIDI input/output.
 *
 * \author        Gary P. Scavone; modifications by Chris Ahlstrom
 * \date          2016-11-14
 * \updates       2016-12-19
 * \license       See the rtexmidi.lic file.
 *
 *  Declares the following classes:
 *
 *      -   seq64::midi_api
 *      -   seq64::midi_in_api
 *      -   seq64::midi_out_api
 */

#include "midi_info.hpp"                /* holds basic API information  */

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{
    class event;

/**
 *  Subclasses of midi_in_api and midi_out_api contain all API- and
 *  OS-specific code necessary to fully implement the rtmidi API.
 *
 *  Note that midi_in_api and midi_out_api are abstract base classes and
 *  cannot be explicitly instantiated.  rtmidi_in and rtmidi_out will
 *  create instances of a midi_in_api or midi_out_api subclass.
 */

class midi_api
{

protected:

    midi_info & m_master_info;

    bool m_connected;
    std::string m_error_string;
    rterror_callback m_error_callback;
    bool m_first_error_occurred;
    void * m_error_callback_user_data;

    /*
     * FROM old midibase module:
     */

    /**
     *  Provides the index of the midibase object in either the input list or
     *  the output list.  Currently needed in the RtMidi code.  Otherwise, it
     *  is currently -1.
     */

    /*const*/ int m_bus_index;

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
     *  Holds the current PPQN value.  Currently used only in setting up the
     *  ALSA input API.
     */

    int m_ppqn;

    /**
     *  Holds the current BPM value.  Currently used only in setting up the
     *  ALSA input API.
     */

    int m_bpm;

public:

    midi_api
    (
        midi_info & masterinfo,
        int ppqn    = SEQ64_DEFAULT_PPQN,       // 192, see app_limits.h
        int bpm     = SEQ64_DEFAULT_BPM         // 120, see app_limits.h
    );
    virtual ~midi_api ();

public:

    virtual bool api_init_out () = 0;
    virtual bool api_init_out_sub () = 0;
    virtual bool api_init_in () = 0;
    virtual bool api_init_in_sub () = 0;
    virtual bool api_deinit_in () = 0;
    virtual void api_play (event * e24, midibyte channel) = 0;
    virtual void api_sysex (event * e24) = 0;
    virtual void api_continue_from (midipulse tick, midipulse beats) = 0;
    virtual void api_start () = 0;
    virtual void api_stop () = 0;
    virtual void api_flush () = 0;
    virtual void api_clock (midipulse tick) = 0;

public:

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

public:

#if 0
    virtual rtmidi_api get_current_api () const = 0;
    virtual void open_port
    (
        unsigned portnumber, const std::string & portname
    ) = 0;
    virtual void open_virtual_port (const std::string & portname) = 0;
    virtual void close_port () = 0;
    virtual bool poll_queue () const = 0;

    /**
     *  Gets the buss/client ID for a MIDI interfaces.  This is the left-hand
     *  side of a X:Y pair (such as 128:0).
     *
     *  This function is a new part of the RtMidi interface.
     *
     * \param index
     *      The ordinal index of the desired interface to look up.
     *
     * \return
     *      Returns the buss/client value as provided by the selected API.
     */

    virtual unsigned get_client_id (unsigned /*portnumber*/)
    {
        return SEQ64_BAD_PORT_ID;                   // TODO
    }

    virtual std::string get_client_name (unsigned /*portnumber*/)   // = 0;
    {
        return std::string("");                     // TODO
    }

    virtual unsigned get_port_count ()
    {
        return 0;                                   // TODO
    }

    virtual unsigned get_port_number (unsigned /*portnumber*/)         // = 0;
    {
        return SEQ64_BAD_PORT_ID;                   // TODO
    }

    virtual std::string get_port_name (unsigned /*portnumber*/)   // = 0;
    {
        return std::string("");                     // TODO
    }

    virtual void midi_mode (bool /*flag*/)
    {
        // no code
    }

    virtual void * midi_handle ()
    {
        return nullptr;
    }

    virtual std::string port_list () const
    {
        return std::string("base class midi_api cannot list ports");
    }
#endif  // 0

    /**
     * \getter m_connected
     */

    bool is_port_open () const
    {
        return m_connected;
    }

    /**
     * \getter m_ppqn
     *      This is the pulses per quarter note.
     *      Used only in the ALSA implementation at present.
     */

    int ppqn () const
    {
        return m_ppqn;
    }

    /**
     * \getter m_bpm
     *      This is the tempo value in beats per minute.
     *      Used only in the ALSA implementation at present.
     */

    int bpm () const
    {
        return m_bpm;
    }

    /**
     *  A basic error reporting function for rtmidi classes.
     */

    void error (rterror::Type type, const std::string & errorstring);

};          // class midi_api

/**
 *  MIDI Input API.
 */

class midi_in_api : public midi_api
{

public:

    midi_in_api
    (
        midi_info & masterinfo,
        int ppqn    = SEQ64_DEFAULT_PPQN,       // 192, see app_limits.h
        int bpm     = SEQ64_DEFAULT_BPM         // 120, see app_limits.h
    );

    virtual ~midi_in_api ();

};          // class midi_in_api

/**
 *  MIDI Output API.
 */

class midi_out_api : public midi_api
{

public:

    midi_out_api
    (
        midi_info & masterinfo,
        int ppqn    = SEQ64_DEFAULT_PPQN,       // 192, see app_limits.h
        int bpm     = SEQ64_DEFAULT_BPM         // 120, see app_limits.h
    );
    virtual ~midi_out_api ();

//  virtual void send_message (const std::vector<midibyte> & message) = 0;
//  virtual bool poll_queue () const;

};          // class midi_out_api

}           // namespace seq64

#endif      // SEQ64_MIDI_API_HPP

/*
 * midi_api.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

