#ifndef SEQ64_MIDI_API_HPP
#define SEQ64_MIDI_API_HPP

/**
 * \file          midi_api.hpp
 *
 *  An abstract base class for realtime MIDI input/output.
 *
 * \author        Gary P. Scavone; modifications by Chris Ahlstrom
 * \date          2016-11-14
 * \updates       2016-11-19
 * \license       See the rtexmidi.lic file.
 *
 *  Declares the following classes:
 *
 *      -   seq64::midi_api
 *      -   seq64::midi_in_api
 *      -   seq64::midi_out_api
 */

#include <string>

#include "easy_macros.h"

/**
 *  A macro to prepend a fully qualified function name to a string.
 */

#define func_message(x)     __func__ ## x

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
    std::string m_error_class;
    std::string m_error_string;
    rtmidierrorcallback m_error_callback;
    bool m_first_error_occurred;
    void * m_error_callback_user_data;

public:

    midi_api ();
    virtual ~midi_api ();

    virtual rtmidi::Api get_current_api () const = 0;
    virtual void open_port
    (
        unsigned portnumber, const std::string & portname
    ) = 0;
    virtual void open_virtual_port (const std::string & portname) = 0;
    virtual void close_port () = 0;
    virtual unsigned get_port_count () = 0;
    virtual std::string get_port_name (unsigned portnumber) = 0;

    /**
     * \getter m_connected
     */

    bool is_port_open () const
    {
        return m_connected;
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

    void seterrorcallback (rtmidierrorcallback errorcallback, void * userdata)
    {
        m_error_callback = errorcallback;
        m_error_callback_user_data = userdata;
    }

    /**
     *  A basic error reporting function for rtmidi classes.
     */

    void error (rterror::Type type, const std::string & errorstring);

protected:

    virtual void initialize (const std::string & clientname) = 0;

    /**
     *  Sets the error class name, which is used to keep messages shorter
     *  in the code.
     */

    void set_error_class (const std::string & name)
    {
        m_error_class = name;
    }

};          // class midi_api

/**
 *  MIDI Input API.
 */

class midi_in_api : public midi_api
{

protected:

    rtmidi_in_data m_input_data;

public:

    midi_in_api (unsigned queuesizelimit);

    virtual ~midi_in_api ();

    void set_callback (rtmidi_in::rtmidi_callback_t callback, void * userdata);
    void cancel_callback ();

    virtual void ignore_types (bool midisysex, bool miditime, bool midisense);

    double get_message (std::vector<midibyte> * message);

    /**
     *  A MIDI structure used internally by the class to store incoming
     *  messages.  Each message represents one and only one MIDI message.
     */

    struct midi_message
    {
        std::vector<midibyte> bytes;
        double timeStamp;

        midi_message () : bytes(), timeStamp(0.0)
        {
            // no body
        }
    };

    struct midi_queue
    {
        unsigned front;
        unsigned back;
        unsigned size;
        unsigned ringSize;
        midi_message * ring;

        midi_queue() : front(0), back(0), size(0), ringSize(0), ring(nullptr)
        {
            // no body
        }
    };

    /**
     *  The rtmidi_in_data structure is used to pass private class data to the
     *  MIDI input handling function or thread.
     */

    struct rtmidi_in_data
    {
        midi_queue queue;
        midi_message message;
        midibyte ignoreFlags;
        bool doInput;
        bool firstMessage;
        void * apiData;
        bool usingCallback;
        rtmidi_in::rtmidi_callback_t userCallback;
        void * userdata;
        bool continueSysex;

        rtmidi_in_data()
         :
            ignoreFlags(7),
            doInput(false),
            firstMessage(true),
            apiData(0),
            usingCallback(false),
            userCallback(0),
            userdata(0),
            continueSysex(false)
        {
            // no body
        }
    };
};

/*
 *  MIDI Output API.
 */

class midi_out_api : public midi_api
{

public:

    midi_out_api ();
    virtual ~midi_out_api ();
    virtual void send_message (std::vector<midibyte> * message) = 0;

};

}           // namespace seq64

#endif      // SEQ64_MIDI_API_HPP

/*
 * midi_api.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

