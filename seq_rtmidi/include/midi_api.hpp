#ifndef SEQ64_MIDI_API_HPP
#define SEQ64_MIDI_API_HPP

/**
 * \file          midi_api.hpp
 *
 *  An abstract base class for realtime MIDI input/output.
 *
 * \author        Gary P. Scavone; modifications by Chris Ahlstrom
 * \date          2016-11-14
 * \updates       2017-01-09
 * \license       See the rtexmidi.lic file.
 *
 *  Declares the following classes:
 *
 *      -   seq64::midi_api
 *      -   seq64::midi_in_api
 *      -   seq64::midi_out_api
 */

#include "midibase.hpp"
#include "rterror.hpp"
#include "rtmidi_types.hpp"             /* SEQ64_NO_INDEX               */

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{
    class event;
    class midi_info;
    class midibus;

/**
 *  Subclasses of midi_in_api and midi_out_api contain all API- and
 *  OS-specific code necessary to fully implement the rtmidi API.
 *
 *  Note that midi_in_api and midi_out_api are abstract base classes and
 *  cannot be explicitly instantiated.  rtmidi_in and rtmidi_out will
 *  create instances of a midi_in_api or midi_out_api subclass.
 */

class midi_api : public midibase
{

private:

    midi_info & m_master_info;
    midibus & m_parent_bus;
    bool m_connected;

protected:

    std::string m_error_string;
    rterror_callback m_error_callback;
    bool m_first_error_occurred;
    void * m_error_callback_user_data;

public:

    midi_api
    (
        midibus & parentbus,
        midi_info & masterinfo,
        int index = SEQ64_NO_INDEX
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
    virtual void api_set_ppqn (int ppqn) = 0;
    virtual void api_set_beats_per_minute (int bpm) = 0;

    /*
     * The next two functions are provisional.  Currently useful only in the
     * midi_jack module.
     */

    virtual std::string api_get_bus_name ()
    {
        std::string sm_empty;
        return sm_empty;
    }
    virtual std::string api_get_port_name ()
    {
        std::string sm_empty;
        return sm_empty;
    }

public:

    /**
     * \getter m_connected
     */

    bool is_port_open () const
    {
        return m_connected;
    }

    /**
     * \getter m_master_info
     */

    midi_info & master_info ()
    {
        return m_master_info;
    }

    /**
     * \getter m_parent_bus
     */

    midibus & parent_bus ()
    {
        return m_parent_bus;
    }

    void master_midi_mode (bool input);

    /**
     *  A basic error reporting function for rtmidi classes.
     */

    void error (rterror::Type type, const std::string & errorstring);

protected:

    void set_port_open ()
    {
        m_connected = true;
    }

};          // class midi_api

/**
 *  MIDI Input API.
 */

class midi_in_api : public midi_api
{

public:

    midi_in_api
    (
        midibus & parentbus,
        midi_info & masterinfo,
        int index = SEQ64_NO_INDEX
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
        midibus & parentbus,
        midi_info & masterinfo,
        int index = SEQ64_NO_INDEX
    );
    virtual ~midi_out_api ();

};          // class midi_out_api

}           // namespace seq64

#endif      // SEQ64_MIDI_API_HPP

/*
 * midi_api.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

