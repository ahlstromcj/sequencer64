#ifndef SEQ64_MIDI_API_HPP
#define SEQ64_MIDI_API_HPP

/**
 * \file          midi_api.hpp
 *
 *  An abstract base class for realtime MIDI input/output.
 *
 * \author        Gary P. Scavone; modifications by Chris Ahlstrom
 * \date          2016-11-14
 * \updates       2016-12-03
 * \license       See the rtexmidi.lic file.
 *
 *  Declares the following classes:
 *
 *      -   seq64::midi_api
 *      -   seq64::midi_in_api
 *      -   seq64::midi_out_api
 */

#include <string>

#include "app_limits.h"                 /* SEQ64_DEFAULT_PPQN etc.  */
#include "easy_macros.h"
#include "rterror.hpp"
#include "rtmidi_types.hpp"

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{

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

    void * m_api_data;
    bool m_connected;
    std::string m_error_string;
    rterror_callback m_error_callback;
    bool m_first_error_occurred;
    void * m_error_callback_user_data;

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
        int ppqn    = SEQ64_DEFAULT_PPQN,       // 192, see app_limits.h
        int bpm     = SEQ64_DEFAULT_BPM         // 120, see app_limits.h
    );
    virtual ~midi_api ();

    virtual rtmidi_api get_current_api () const = 0;
    virtual void open_port
    (
        unsigned portnumber, const std::string & portname
    ) = 0;
    virtual void open_virtual_port (const std::string & portname) = 0;
    virtual void close_port () = 0;
    virtual unsigned get_port_count () = 0;
    virtual std::string get_port_name (unsigned portnumber) = 0;
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

    virtual int get_client_id (int index)
    {
        return index;
    }

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
     *  Locks in the MIDI API error callback.
     *
     * \param errorcallback
     *      The function to be used as the error callback.
     *
     * \param userdata
     *      The data area associated with the callback, defaults to a null
     *      pointer.
     */

    void seterrorcallback (rterror_callback errorcallback, void * userdata)
    {
        m_error_callback = errorcallback;
        m_error_callback_user_data = userdata;
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

protected:

    rtmidi_in_data m_input_data;

public:

    midi_in_api
    (
        unsigned queuesizelimit,
        int ppqn    = SEQ64_DEFAULT_PPQN,       // 192, see app_limits.h
        int bpm     = SEQ64_DEFAULT_BPM         // 120, see app_limits.h
    );

    virtual ~midi_in_api ();
    virtual void ignore_types (bool midisysex, bool miditime, bool midisense);

    void set_callback (rtmidi_callback_t callback, void * userdata);
    void cancel_callback ();
    double get_message (std::vector<midibyte> & message);

    virtual bool poll_queue () const;

};          // class midi_in_api

/**
 *  MIDI Output API.
 */

class midi_out_api : public midi_api
{

public:

    midi_out_api (unsigned queuesizelimit = 0);
    virtual ~midi_out_api ();
    virtual void send_message (const std::vector<midibyte> & message) = 0;
    virtual bool poll_queue () const;

};          // class midi_out_api

}           // namespace seq64

#endif      // SEQ64_MIDI_API_HPP

/*
 * midi_api.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

